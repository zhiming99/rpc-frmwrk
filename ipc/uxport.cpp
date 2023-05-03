/*
 * =====================================================================================
 *
 *       Filename:  uxport.cpp
 *
 *    Description:  implementation of unix sock bus port and unix sock stream pdo
 *
 *        Version:  1.0
 *        Created:  06/11/2019 05:02:09 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi(woodhead99@gmail.com )
 *   Organization:  
 *
 *      Copyright:  2019 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#include "defines.h"
#include "stlcont.h"
#include "dbusport.h"
#include "ifhelper.h"
#include "emaphelp.h"
#include "uxport.h"

namespace rpcf
{

CRecvFilter::RecvState
CRecvFilter::GetRecvState() const
{
    if( m_dwBytesToRead == 0 )
    {
        if( m_dwOffsetRead == ( ( guint32 )-1 ) )
            return recvTok;
        return recvSize; 
    }
    return recvData;
}

gint32 CRecvFilter::OnIoReady()
{
    gint32 ret = 0;
    do{
        // read the header
        BufPtr pBuf;
        RecvState iStat = GetRecvState();
        bool bFirst = true;
        guint8 byToken = tokInvalid;

        if( iStat == recvTok )
        {
            ret = read( m_iFd, &byToken, sizeof( byToken ) );
            if( ERROR( ret ) )
            {
                if( errno == EAGAIN ||
                    errno == EWOULDBLOCK )
                {
                    ret = -EAGAIN;
                    break;
                }
            }
            if( ret == 0 && bFirst )
            {
                ret = -ENOTCONN;
                break;
            }
            bFirst = false;

            m_byToken = byToken;
            if( byToken == tokPing ||
                byToken == tokPong ||
                byToken == tokClose ||
                byToken == tokFlowCtrl ||
                byToken == tokLift )
            {
                ret = byToken;
                break;
            }

            if( byToken != tokData &&
                byToken != tokProgress )
            {
                ret = -EBADMSG;
                break;
            }

            // read the size of the packet
            ret = pBuf.NewObj();
            if( ERROR( ret ) )
                break;

            m_dwOffsetRead = 0;
            pBuf->Resize( sizeof( guint32 ) );
            m_queBufRead.push_back( pBuf );
        }

        while( GetRecvState() == recvSize )
        {
            pBuf = m_queBufRead.back();
            if( pBuf.IsEmpty() )
            {
                ret = -EFAULT;
                break;
            }

            guint32 iLeft =
                sizeof( guint32 ) - m_dwOffsetRead;

            guint8* pos = ( ( guint8* )pBuf->ptr() ) +
                m_dwOffsetRead;

            ret = read( m_iFd, pos, iLeft );

            if( ret == ( gint32 )iLeft )
            {
                m_dwBytesToRead = ntohl(
                    *( ( guint32* )pBuf->ptr() ) );
                if( m_dwBytesToRead >
                    STM_MAX_BYTES_PER_BUF || 
                    m_dwBytesToRead == 0 )
                {
                    ret = -EBADMSG;
                    break;
                }
                // leave some room for later usage
                guint32 dwBufOffset = UXBUF_OVERHEAD;
                pBuf->Resize(
                    m_dwBytesToRead + dwBufOffset );
                pBuf->SetOffset( dwBufOffset );
                m_dwOffsetRead = 0;
                // fall through
            }
            else if( ret > 0 )
            {
                m_dwOffsetRead += ret;
                continue;
            }
            else if( ret == -1 )
            {
                if( errno == EAGAIN ||
                    errno == EWOULDBLOCK )
                {
                    ret = -EAGAIN;
                    break;
                }
                ret = -errno;
                break;
            }
            else if( ret == 0 && bFirst )
            {
                ret = -ENOTCONN;
                break;
            }
        }

        if( ERROR( ret ) )
            break;

        if( GetRecvState() != recvData )
        {
            ret = ERROR_STATE;
            break;
        }

        ret = 0;
        while( m_dwBytesToRead > 0 )
        {
            if( m_queBufRead.empty() )
            {
                ret = -EFAULT;
                break;
            }

            if( pBuf.IsEmpty() )
                pBuf = m_queBufRead.back();

            if( pBuf.IsEmpty() )
            {
                ret = -EFAULT;
                break;
            }

            if( m_dwBytesToRead >
                pBuf->size() - m_dwOffsetRead )
            {
                ret = -ERANGE;
                break;
            }

            guint32 iLeft =
                pBuf->size() - m_dwOffsetRead;

            guint8* pos = ( guint8* )pBuf->ptr() +
                m_dwOffsetRead;

            ret = read( m_iFd, pos, iLeft );

            if( ret == ( gint32 )iLeft )
            {
                // the packet is done
                m_dwOffsetRead = ( guint32 )-1;
                m_dwBytesToRead = 0;
                m_dwBytesPending += pBuf->size();
                ret = m_byToken;
                m_byToken = 0;
                break;
            }
            else if( ret > 0 )
            {
                m_dwBytesToRead -= ret;
                m_dwOffsetRead += ret;
                continue;
            }
            else if( ret == -1 )
            {
                if( errno == EAGAIN ||
                    errno == EWOULDBLOCK )
                {
                    ret = -EAGAIN;
                    break;
                }
                ret = -errno;
                break;
            }
            else if( ret == 0 && bFirst )
            {
                ret = -ENOTCONN;
                break;
            }
        }

    }while( 0 );

    if( ERROR( ret ) && ret != -EAGAIN )
    {
        m_dwBytesToRead = 0;
        m_dwOffsetRead = ( guint32 )-1;
    }

    // DebugPrint( ret, "msg received" );
    return ret;
}

gint32 CRecvFilter::ReadStream(
    BufPtr& pBuf )
{
    if( m_queBufRead.empty() )
        return -ENOENT;

    // the read is not complete yet
    if( m_queBufRead.size() == 1 &&
        GetRecvState() != recvTok )
        return -ENOENT;

    pBuf = m_queBufRead.front();
    m_queBufRead.pop_front();
    m_dwBytesPending -= pBuf->size();
    return 0;
}

CSendQue::SendState
CSendQue::GetSendState() const
{
    if( m_dwBytesToWrite == 0 )
    {
        if( m_dwOffsetWrite == ( ( guint32 )-1 ) )
            return sendTok;
        return sendSize; 
    }
    return sendPayload;
}

gint32 CSendQue::OnIoReady()
{
    gint32 ret = 0;
    do{
        if( m_queBufWrite.empty() )
        {
            ret = -ENOENT;
            break;
        }

        STM_PACKET& oPkt =
            m_queBufWrite.front();

        if( m_dwRestBytes == 0 &&
            !oPkt.pData.IsEmpty() )
            m_dwRestBytes = oPkt.pData->size();

        guint8 byToken = oPkt.byToken;

        if( GetSendState() == sendTok )
        {
            ret = SendBytesNoSig( m_iFd, &byToken,
                sizeof( byToken ) );

            if( ret == -1 )
            {
                if( errno == EAGAIN ||
                    errno == EWOULDBLOCK )
                {
                    ret = -EAGAIN;
                    break;
                }
                ret = -errno;
                break;
            }

            if( byToken == tokPing ||
                byToken == tokPong ||
                byToken == tokClose ||
                byToken == tokFlowCtrl ||
                byToken == tokLift )
            {
                // next packet
                --m_dwBytesPending;
                m_queBufWrite.pop_front();
                ret = 0;
                continue;
            }

            if( byToken != tokData && 
                byToken != tokProgress )
            {
                ret = -EBADMSG;
                break;
            }

            if( byToken != tokData &&
                oPkt.dwSize > STM_MAX_BYTES_PER_BUF )
            {
                // only data packet can have bigger
                // size than STM_MAX_BYTES_PER_BUF
                ret = -EBADMSG;
                break;
            }

            m_dwOffsetWrite = 0;
        }

        guint32 dwPktSize = m_dwRestBytes -
            STM_MAX_BYTES_PER_BUF * m_dwBlockSent;

        if( dwPktSize > STM_MAX_BYTES_PER_BUF )
            dwPktSize = STM_MAX_BYTES_PER_BUF;

        while( GetSendState() == sendSize )
        {
            guint32 iLeft =
                sizeof( guint32 ) - m_dwOffsetWrite;

            guint32 dwSize = htonl( dwPktSize );
            guint8* pos = ( ( guint8* )&dwSize ) +
                m_dwOffsetWrite;

            ret = SendBytesNoSig( m_iFd, pos, iLeft );

            if( ret == ( gint32 )iLeft )
            {
                // setup the parameters for payload
                // transfer
                m_dwOffsetWrite = 0;
                m_dwBytesToWrite = dwPktSize;
                ret = 0;
                // fall through
            }
            else if( ret > 0 )
            {
                m_dwOffsetWrite += ret;
                ret = 0;
                continue;
            }
            else if( ret == -1 )
            {
                if( errno == EAGAIN ||
                    errno == EWOULDBLOCK )
                {
                    ret = -EAGAIN;
                    break;
                }
                ret = -errno;
                break;
            }
        }

        if( ERROR( ret ) )
            break;

        if( GetSendState() != sendPayload )
        {
            ret = ERROR_STATE;
            break;
        }

        BufPtr pBuf = oPkt.pData;
        if( pBuf.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        while( m_dwBytesToWrite > 0 )
        {
            if( m_dwBytesToWrite >
                dwPktSize - m_dwOffsetWrite )
            {
                ret = -ERANGE;
                break;
            }

            guint32 iLeft =
                dwPktSize - m_dwOffsetWrite;

            guint8* pos = ( ( guint8* )pBuf->ptr() ) +
                m_dwBlockSent * STM_MAX_BYTES_PER_BUF +
                m_dwOffsetWrite;

            ret = SendBytesNoSig( m_iFd, pos, iLeft );
            if( ret == ( gint32 )iLeft )
            {
                // current packet is done
                // reset the working params
                m_dwOffsetWrite = ( guint32 )-1;
                m_dwBytesToWrite = 0;

                m_dwBytesPending -= ( 1 +
                    sizeof( guint32 ) + dwPktSize );

                m_dwRestBytes -= dwPktSize;
                if( m_dwRestBytes > 0 )
                {
                    ++m_dwBlockSent;
                    continue;
                }
                m_queBufWrite.pop_front();
                ret = 0;
                break;
            }
            else if( ret > 0 )
            {
                m_dwBytesToWrite -= ret;
                m_dwOffsetWrite += ret;
                ret = 0;
                continue;
            }
            else if( ret == -1 )
            {
                if( errno == EAGAIN ||
                    errno == EWOULDBLOCK )
                {
                    ret = -EAGAIN;
                    break;
                }
                ret = -errno;
                break;
            }
        }

        if( ERROR( ret ) )
            break;

    }while( m_dwRestBytes > 0 );

    // DebugPrint( ret, "msg sent" );
    return ret;
}

gint32 CSendQue::SendPingPong( bool bPing )
{
    STM_PACKET oPkt;
    oPkt.byToken =
        bPing ? tokPing : tokPong;

    m_queBufWrite.push_back( oPkt );
    ++m_dwBytesPending;

    gint32 ret = OnIoReady();
    if( ret == -EAGAIN )
        ret = STATUS_PENDING;

    return ret;
}

gint32 CSendQue::SendClose()
{
    STM_PACKET oPkt;
    oPkt.byToken = tokClose;
    m_queBufWrite.push_back( oPkt );
    ++m_dwBytesPending;

    gint32 ret = OnIoReady();
    if( ret == -EAGAIN )
        ret = STATUS_PENDING;

    return ret;
}

gint32 CSendQue::WriteStream(
    BufPtr& pBuf, guint8 byToken )
{
    STM_PACKET oPkt;
    oPkt.byToken = byToken;
    oPkt.dwSize = pBuf->size();
    oPkt.pData = pBuf;
    m_queBufWrite.push_back( oPkt );

    m_dwBytesPending += ( 1 +
        sizeof( guint32 ) + pBuf->size() );

    if( m_queBufWrite.size() > 1 )
        return STATUS_PENDING;

    gint32 ret = OnIoReady();

    if( ret == -EAGAIN )
        ret = STATUS_PENDING;

    return ret;
}

CIoWatchTask::CIoWatchTask( const IConfigDb* pCfg )
    :super( pCfg ),
    m_iFd( -1 ),
    m_hReadWatch( INVALID_HANDLE ),
    m_hWriteWatch( INVALID_HANDLE ),
    m_hErrWatch( INVALID_HANDLE ),
    m_oRecvFilter( GetFd( pCfg ) ),
    m_oSendQue( GetFd( pCfg ) ),
    m_qwBytesRead( 0 ),
    m_qwBytesWrite( 0 ),
    m_dwState( stateStarting ),
    m_iWatTimerId( 0 )
{
    SetClassId( clsid( CIoWatchTask ) );
    CCfgOpener oCfg( ( IConfigDb* )GetConfig() );

    gint32 ret = 0;
    do{
        ret = oCfg.GetIntProp( propFd,
            ( guint32& )m_iFd );
        if( ERROR( ret ) )
            break;

        IPort* pPort = nullptr;
        ret = oCfg.GetPointer(
            propPortPtr, pPort );
        if( ERROR( ret ) )
            break;

        m_pPort = pPort;
        if( m_pPort.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        oCfg.RemoveProperty( propPortPtr );
        oCfg.RemoveProperty( propFd );

    }while( 0 );

    if( ERROR( ret ) )
    {
        std::string strMsg = DebugMsg( ret,
            "Error in CIoWatchTask ctor" );
        throw std::runtime_error( strMsg );
    }
}

// new data arrived
gint32 CIoWatchTask::OnProgress(
    BufPtr& pBuf )
{
    gint32 ret = 0;
    if( m_pPort.IsEmpty() )
        return -EFAULT;

    CUnixSockStmPdo* pPort = m_pPort;
    if( pPort == nullptr )
        return -EFAULT;

    ret = pPort->OnUxSockEvent(
        tokProgress, pBuf );

    if( ERROR( ret ) )
        return ret;

    return -EAGAIN;
}

// new data arrived
gint32 CIoWatchTask::OnDataReady(
    BufPtr pBuf )
{
    gint32 ret = 0;
    if( m_pPort.IsEmpty() )
        return -EFAULT;

    CUnixSockStmPdo* pPort = m_pPort;
    if( pPort == nullptr )
        return -EFAULT;

    m_qwBytesRead += pBuf->size();
    ret = pPort->OnStmRecv( pBuf );

    if( ERROR( ret ) )
        return ret;

    return -EAGAIN;
}

gint32 CIoWatchTask::OnSendReady()
{
    gint32 ret = 0;
    if( m_pPort.IsEmpty() )
        return -EFAULT;

    CUnixSockStmPdo* pPort = m_pPort;
    if( pPort == nullptr )
        return -EFAULT;

    ret = pPort->OnSendReady();

    if( ERROR( ret ) )
        return ret;

    return -EAGAIN;
}

// ping/pong packets
gint32 CIoWatchTask::OnPingPong( bool bPing )
{
    gint32 ret = 0;
    if( m_pPort.IsEmpty()  )
        return -EFAULT;

    CUnixSockStmPdo* pPort = m_pPort;
    if( pPort == nullptr )
        return -EFAULT;

    BufPtr pBuf;
    ret = pPort->OnUxSockEvent(
        bPing ? tokPing : tokPong, pBuf );

    if( ERROR( ret ) )
        return ret;

    return -EAGAIN;
}

// received by server to notify a client initiated
// close
gint32 CIoWatchTask::OnClose()
{
    if( m_pPort.IsEmpty() )
        return -EFAULT;

    CUnixSockStmPdo* pPort = m_pPort;
    if( pPort == nullptr )
        return -EFAULT;

    BufPtr pBuf;
    pPort->OnUxSockEvent( tokClose, pBuf );

    // return 0 to complete the task
    return 0;
}

void CIoWatchTask::OnError( gint32 iError )
{
    if( m_pPort.IsEmpty() )
        return;

    CUnixSockStmPdo* pPort = m_pPort;
    if( pPort == nullptr )
        return;

    BufPtr pBuf( true );
    pBuf->Resize( sizeof( guint32 ) +
        UXBUF_OVERHEAD );
    pBuf->SetOffset( UXBUF_OVERHEAD );
    *pBuf = ( guint32 ) htonl( iError );
    pPort->OnUxSockEvent( tokError, pBuf );

    // return 0 to complete the task
    return;
}

gint32 CIoWatchTask::OnIoWatchEvent(
    guint32 revents )
{
    gint32 ret = 0;
    do{
        if( ( revents & ( ~0x07 ) ) == 0 )
        {
            ret = OnIoReady( revents & 0x07 ); 
            break;
        }
        else if( revents & POLLHUP )
        {
            if( revents & POLLIN )
                OnIoReady( POLLIN );
            ret = OnIoError( POLLHUP );
            break;
        }
        if( revents & POLLERR )
        {
            ret = OnIoError( POLLERR );
            break;
        }

    }while( 0 );

    return ret;
}

gint32 CIoWatchTask::RunTask()
{
    gint32 ret = 0;
    do{
        IConfigDb* pCfg = GetConfig();
        CParamList oCfg( pCfg );

        CPort* pPort = m_pPort;
        if( unlikely( pPort == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }
        
        CIoManager* pMgr = pPort->GetIoMgr();
        CLoopPools& oLoops = pMgr->GetLoopPools();
        MloopPtr pLoop;
        ret = oLoops.AllocMainLoop(
            UXSOCK_TAG, pLoop );

        if( ERROR( ret ) )
            break;

        SetMainLoop( pLoop );

        oCfg.Push( m_iFd );
        oCfg.Push( ( guint32 )POLLIN );
        oCfg.Push( false );

        TaskletPtr pTask( this );

        ret = pLoop->AddIoWatch(
            pTask, m_hReadWatch );

        if( ERROR( ret ) )
            break;

        oCfg[ 1 ] = ( guint32 )POLLOUT;

        ret = pLoop->AddIoWatch(
            pTask, m_hWriteWatch );

        if( ERROR( ret ) )
        {
            pLoop->RemoveIoWatch(
                m_hReadWatch );
            m_hReadWatch = INVALID_HANDLE;
            break;
        }

        oCfg[ 1 ] = ( guint32 )0;
        ret = pLoop->AddIoWatch(
            pTask, m_hErrWatch );

        if( ERROR( ret ) )
        {
            pLoop->RemoveIoWatch(
                m_hReadWatch );
            m_hReadWatch = INVALID_HANDLE;
            pLoop->RemoveIoWatch(
                m_hWriteWatch );
            m_hWriteWatch = INVALID_HANDLE;
            break;
        }

        // pLoop->StartSource( m_hReadWatch, srcIo );
        
        // return STATUS_PENDING to wait for IO
        ret = STATUS_PENDING;

    }while( 0 );

    return ret;
}

gint32 CIoWatchTask::operator()(
    guint32 dwContext )
{
    gint32 ret = 0;

    EnumTaskState iState = GetTaskState();
    if( IsStopped( iState ) )
    {
        if( dwContext == eventIoWatch )
            return G_SOURCE_REMOVE;
        return ERROR_STATE;
    }

    switch( dwContext )
    {
    case eventIoWatch:
        {
            std::vector< LONGWORD > vecParams;
            GetParamList( vecParams );
            guint32 revents = vecParams[ 1 ];
            ret = OnIoWatchEvent( revents );

            if( ERROR( ret ) && ret != -EAGAIN )
                ret = G_SOURCE_REMOVE;
            else
                ret = G_SOURCE_CONTINUE;
            break;
        }
    default:
        {
            ret = super::operator()( dwContext );
            break;
        }
    }

    return SetError( ret );
}

gint32 CIoWatchTask::OnEvent(
    EnumEventId iEvent,
    LONGWORD dwParam1,
    LONGWORD dwParam2,
    LONGWORD* pData )
{
    // Lock to prevent re-entrance 
    gint32 ret = 0;
    switch( iEvent )
    {
    case eventIoWatch:
        {
            // we have some special processing for
            // eventIoWatch to bypass the normal
            // CIfParallelTask's process flow.
            CStdRTMutex oTaskLock( GetLock() );
            LwVecPtr pVec( true );
            std::vector< LONGWORD >& oVec = ( *pVec )();
            oVec.push_back( iEvent );
            oVec.push_back( dwParam1 );
            oVec.push_back( dwParam2 );
            oVec.push_back( ( LONGWORD )pData );

            CCfgOpener oCfg(
                ( IConfigDb* )GetConfig() );

            oCfg.SetObjPtr(
                propParamList, ObjPtr( pVec ) );

            // note: the return value won't not be
            // set by SetError
            ret = ( *this )( iEvent );
            oCfg.RemoveProperty( propParamList );
            break;
            
        }
    default:
        {
            ret = super::OnEvent( iEvent,
                dwParam1, dwParam2, pData );
            break;
        }
    }
    return ret;
}

gint32 CIoWatchTask::StartStopWatch(
    bool bStop, bool bWrite )
{
    gint32 ret = 0;
    CPort* pPort = m_pPort;

    if( pPort == nullptr )
        return -EFAULT;

    do{
        CMainIoLoop* pLoop = GetMainLoop();

#ifdef _USE_LIBEV
        HANDLE hWatch = m_hWriteWatch;
        if( unlikely( bWrite == false ) )
            hWatch = m_hReadWatch;

        if( bWrite )
        {
            if( bStop )
                ret = pLoop->StopSource(
                    hWatch, srcIo );
            else
                ret = pLoop->StartSource(
                    hWatch, srcIo );
        }
        else
        {
            HANDLE hStartWatch = INVALID_HANDLE;
            if( bStop )
            {
                hStartWatch = m_hErrWatch;
                hWatch = m_hReadWatch;
            }
            else
            {
                hStartWatch = m_hReadWatch;
                hWatch = m_hErrWatch;
            }

            ret = pLoop->StopSource(
                hWatch, srcIo );

            if( ERROR( ret ) )
                break;

            ret = pLoop->StartSource(
                hStartWatch, srcIo );
        }

    }while( 0 );
#else
    // not implemented yet

#endif
    return ret;
}

// watch handler
gint32 CIoWatchTask::OnIoReady( guint32 revent )
{
    gint32 ret = 0;
    do{
        if( revent & POLLIN )
        {
            do{
                ret = m_oRecvFilter.OnIoReady();
                if( ERROR( ret ) )
                    break;

                switch( ret )
                {
                case tokPong: 
                case tokPing:
                    {
                        OnPingPong( ret == tokPing );
                        break;
                    }
                case tokClose:
                    {
                        OnClose();
                        ret = 0;
                        break;
                    }
                case tokData:
                    {
                        BufPtr pBuf;
                        ret = m_oRecvFilter.ReadStream( pBuf );
                        if( ERROR( ret ) )
                            break;
                        OnDataReady( pBuf );
                        ret = tokData;
                        break;
                    }
                case tokProgress:
                    {
                        BufPtr pBuf;
                        ret = m_oRecvFilter.ReadStream( pBuf );
                        if( ERROR( ret ) )
                            break;
                        OnProgress( pBuf );
                        ret = tokProgress;
                        break;
                    }
                default:
                    break;
                }

                // till exhause all the data in the buffer
            }while( ret > 0 );

            break;
        }
        else if( revent & POLLOUT )
        {
            do{
                ret = m_oSendQue.OnIoReady();
                if( SUCCEEDED( ret ) &&
                    m_oSendQue.GetPendingWrite() == 0 )
                {
                    ret = OnSendReady();
                }
                else if( ret == -ENOENT )
                {
                    StopWatch();
                    ret = 0;
                }

            }while( 0 );
        }
        else
        {
            ret = -ENOTSUP;
        }

    }while( 0 );

    if( ERROR( ret ) && ret != -EAGAIN )
        OnError( ret );

    return 0;
}

gint32 CIoWatchTask::ReleaseChannel()
{
    gint32 ret = 0;
    do{
        auto pMgr = GetIoMgr();
        MloopPtr pLoop = GetMainLoop();
        if( unlikely( pLoop.IsEmpty() ) )
            break;

        QwVecPtr pvecHandles( true );
        ( *pvecHandles )().push_back(
            ( guint64 )m_hReadWatch );
        ( *pvecHandles )().push_back(
            ( guint64 )m_hWriteWatch );
        ( *pvecHandles )().push_back(
            ( guint64 )m_hErrWatch );
        ( *pvecHandles )().push_back(
            ( guint64 )m_iFd );

        gint32 (*func)( IMainLoop*, ObjPtr ) =
        ( []( IMainLoop* pLoop,
            ObjPtr pvecHandles )->gint32
        {
            CStlQwordVector* pqwVec = pvecHandles;
            std::vector< guint64 >&
                vecHandles = ( *pqwVec )();
            pLoop->RemoveIoWatch(
                ( HANDLE )vecHandles[ 0 ] );
            pLoop->RemoveIoWatch(
                ( HANDLE )vecHandles[ 1 ] );
            pLoop->RemoveIoWatch(
                ( HANDLE )vecHandles[ 2 ] );
            return 0;
        });

        TaskletPtr pTask;
        gint32 ret = NEW_FUNCCALL_TASK(
            pTask, pMgr, func,
            ( IMainLoop* )pLoop,
            pvecHandles );

        if( SUCCEEDED( ret ) )
        {
            CMainIoLoop* pMloop = pLoop;
            pMloop->AddTask( pTask );
        }
        else
        {
            pLoop->RemoveIoWatch( m_hReadWatch );
            pLoop->RemoveIoWatch( m_hWriteWatch );
            pLoop->RemoveIoWatch( m_hErrWatch );
        }

        m_hReadWatch = INVALID_HANDLE;
        m_hWriteWatch = INVALID_HANDLE;
        m_hErrWatch = INVALID_HANDLE;
        m_iFd = -1;

        StopTimer();
        CLoopPools& oLoops =
            GetIoMgr()->GetLoopPools();
        oLoops.ReleaseMainLoop(
            UXSOCK_TAG, pLoop );

    }while( 0 );

    return ret;
}

gint32 CIoWatchTask::StartStopTimer( bool bStart )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg( ( IConfigDb* )
            this->GetConfig() );

        CPort* pPort = nullptr;
        ret = oCfg.GetPointer(
            propPortPtr, pPort );

        if( ERROR( ret ) ) 
            break;

        if( unlikely( pPort == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }
        
        CCfgOpenerObj oPortCfg( pPort );
        guint32 dwTimeOutSec = 0;

        ret = oPortCfg.GetIntProp(
            propKeepAliveSec, dwTimeOutSec );

        if( ERROR( ret ) )
            break;

        if( dwTimeOutSec == 0 )
        {
            ret = -EINVAL;
            break;
        }

        CIoManager* pMgr = pPort->GetIoMgr();
        CUtilities& oUtils = pMgr->GetUtils();

        CTimerService& oTimerSvc =
            oUtils.GetTimerSvc();

        if( bStart )
        {
            m_iWatTimerId = oTimerSvc.AddTimer(
                dwTimeOutSec, this,
                eventKeepAlive );
        }
        else if( m_iWatTimerId > 0 )
        {
            oTimerSvc.RemoveTimer( m_iWatTimerId );
            m_iWatTimerId = 0;
        }

    } while( 0 );

    return ret;
}

CUnixSockStmPdo::CUnixSockStmPdo(
    const IConfigDb* pCfg ) :
    super( pCfg )
{
    SetClassId( clsid( CUnixSockStmPdo ) );
    m_dwFlags &= ~PORTFLG_TYPE_MASK;
    m_dwFlags |= PORTFLG_TYPE_PDO;

    CCfgOpener oCfg( pCfg );
    gint32 ret = 0;

    do{
        ret = oCfg.GetPointer(
            propBusPortPtr, m_pBusPort );

        if( ERROR( ret ) )
            break;

        ret = oCfg.GetBoolProp(
            propListenOnly, m_bListenOnly );

        if( ERROR( ret ) )
            m_bListenOnly = false;

        ret = oCfg.GetIntProp(
            propFd, m_dwFd );

        if( ERROR( ret ) )
            break;

        ret = 0;

    }while( 0 );

    if( ERROR( ret ) )
    {
        throw std::runtime_error(
            "Error in CUnixSockStmPdo ctor" );
    }
}

CUnixSockStmPdo::~CUnixSockStmPdo()
{
    // closing the socket here to avoid the fd
    // reused too early before the bus-port remove
    // this pdo from its pdo map.
    gint32& iFd = ( gint32& )m_dwFd;
    if( iFd < 0 )
        return;

    gint32 ret =
        shutdown( iFd, SHUT_RDWR );
    if( ERROR( ret ) )
    {
        DebugPrint( -errno,
            "Error shutdown socket[%d]",
            iFd );
    }
    close( iFd );
    iFd = -1;
}

gint32 CUnixSockStmPdo::PostStart( IRP* pIrp )
{
    CParamList oParams;

    oParams.SetPointer( propPortPtr, this );
    oParams.SetPointer( propIoMgr, GetIoMgr() );
    oParams.SetIntProp( propFd, m_dwFd );

    gint32 ret = 0;
    do{
        ret = m_pIoWatch.NewObj(
            clsid( CIoWatchTask ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        ret = ( *m_pIoWatch )( eventZero );
        if( ret == STATUS_PENDING )
            ret = 0;

    }while( 0 );

    return ret;
}

gint32 CUnixSockStmPdo::OnPortReady( IRP* pIrp )
{
    CIoWatchTask* pTask = m_pIoWatch;
    if( !m_bFlowCtrl )
        pTask->StartWatch( false );

    return super::OnPortReady( pIrp );
}

gint32 CUnixSockStmPdo::PreStop( IRP* pIrp )
{
    if( !m_pIoWatch.IsEmpty() )
    {
        // stop the watch
        m_pIoWatch->OnEvent( eventTaskComp,
            ERROR_PORT_STOPPED, 0, nullptr );

        m_pIoWatch.Clear();
    }
    return super::PreStop( pIrp );
}

gint32 CUnixSockStmPdo::RemoveIrpFromMap( IRP* pIrp )
{
    // NOTE: you should make sure the irp is
    // locked from outside
    if( pIrp == nullptr )
        return -EINVAL;

    IrpCtxPtr pCtx = pIrp->GetTopStack();
    if( pCtx->GetMajorCmd() != IRP_MJ_FUNC )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CStdRMutex oPortLock( GetLock() );
        std::deque< IrpPtr >* pQueue = nullptr;

        switch( pCtx->GetMinorCmd() )
        {
        case IRP_MN_WRITE:
            {
                pQueue = &m_queWritingIrps;
                break;
            }
        case IRP_MN_READ:
            {
                pQueue = &m_queReadingIrps;
                break;
            }
        case IRP_MN_IOCTL:
            {
                if( pCtx->GetCtrlCode() !=
                    CTRLCODE_LISTENING )
                {
                    ret = -EINVAL;
                    break;
                }

                pQueue = &m_queListeningIrps;
                break;
            }
        default:
            ret = -ENOTSUP; 
            break;
        }

        if( ERROR( ret ) )
            break;

        std::deque< IrpPtr >::iterator itr =
            pQueue->begin();

        ret = -ENOENT;
        while( itr != pQueue->end() )
        {
            if( pIrp->GetObjId() ==
                ( *itr )->GetObjId() )
            {
                pQueue->erase( itr );
                ret = 0;
                break;
            }
            ++itr;
        }

    }while( 0 );

    return ret;
}

gint32 CUnixSockStmPdo::CancelFuncIrp(
    IRP* pIrp, bool bForce )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        ret = RemoveIrpFromMap( pIrp );
        if( ERROR( ret ) )
            break;

        CStdRMutex oIrpLock( pIrp->GetLock() );
        ret = pIrp->CanContinue( IRP_STATE_READY );
        if( ERROR( ret ) )
            break;

        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        pCtx->SetStatus( ret = ERROR_CANCEL );

        oIrpLock.Unlock();
        super::CancelFuncIrp( pIrp, bForce );

    }while( 0 );

    return ret;
}

gint32 CUnixSockStmPdo::SubmitIoctlCmd(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    // let's process the func irps
    IrpCtxPtr pCtx = pIrp->GetCurCtx();
    do{
        if( pIrp->MajorCmd() != IRP_MJ_FUNC
            || pIrp->MinorCmd() != IRP_MN_IOCTL )
        {
            ret = -EINVAL;
            break;
        }

        switch( pIrp->CtrlCode() )
        {
        case CTRLCODE_LISTENING:
            {
                // server side I/O
                ret = HandleListening( pIrp );
                break;
            }
        case CTRLCODE_STREAM_CMD:
            {
                ret = HandleStreamCommand( pIrp );
                break;
            }
        default:
            {
                ret = -ENOTSUP;
                break;
            }
        }

    }while( 0 );

    if( ret != STATUS_PENDING )
        pCtx->SetStatus( ret );

    return ret;
}

gint32 CUnixSockStmPdo::HandleListening(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;

    // let's process the func irps
    IrpCtxPtr& pCtx = pIrp->GetTopStack();
    do{
        CIoWatchTask* pTask = m_pIoWatch;
        CStdRMutex oPortLock( GetLock() );
        std::deque< BufPtr >& oEvtQue =
            m_queEventPackets;
        if( !oEvtQue.empty() )
        {
            BufPtr pBuf = oEvtQue.front();        
            pCtx->m_pRespData = pBuf;
            oEvtQue.pop_front();
            ret = STATUS_SUCCESS;
            if( m_bFlowCtrl )
                break;
            if( oEvtQue.size() ==
                STM_MAX_QUEUE_SIZE - 1 )
                pTask->StartWatch( false );
            break;
        }

        if( m_queListeningIrps.size() >
            STM_MAX_QUEUE_SIZE )
        {
            // why send read request so many times
            ret = ERROR_QUEUE_FULL;
            break;
        }
        m_queListeningIrps.push_back( pIrp );
        ret = STATUS_PENDING;

    }while( 0 );

    return ret;
}

gint32 CUnixSockStmPdo::HandleStreamCommand(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    bool bNotify = false;
    CIoWatchTask* pTask = m_pIoWatch;
    if( pTask == nullptr )
        return -EFAULT;

    // let's process the func irps
    do{

        CStdRTMutex oTaskLock( pTask->GetLock() );
        CStdRMutex oPortLock( GetLock() );
        IrpCtxPtr& pCtx = pIrp->GetTopStack();

        bool bOut = true;
        if( pCtx->GetIoDirection() == IRP_DIR_IN )
            bOut = false;

        if( bOut && m_queWritingIrps.size() >=
            STM_MAX_QUEUE_SIZE )
            return ERROR_QUEUE_FULL;

        if( bOut && !m_queWritingIrps.empty() )
        {
            m_queWritingIrps.push_back( pIrp );
            if( m_queWritingIrps.size() ==
                STM_MAX_QUEUE_SIZE )
                bNotify = true;
            ret = STATUS_PENDING;
            break;
        }
            
        BufPtr pBuf = pCtx->m_pReqData;
        if( pBuf.IsEmpty() )
        {
            ret = -EINVAL;
            break;
        }

        guint8 cToken = pBuf->ptr()[ 0 ];
        switch( cToken )
        {
        case tokPong:
            ret = pTask->SendPingPong( false );
            break;

        case tokPing:
            ret = pTask->SendPingPong( true );
            break;

        case tokProgress:
            {
                pBuf->SetOffset( pBuf->offset() +
                    UXPKT_HEADER_SIZE );

                ret = pTask->WriteStream(
                    pBuf, tokProgress );

                // pBuf->SetOffset( pBuf->offset() -
                //     UXPKT_HEADER_SIZE );

                break;
            }
        case tokLift:
            {
                m_bFlowCtrl = false;
                if( m_queDataPackets.size() >=
                    STM_MAX_QUEUE_SIZE )
                {
                    // ret = ERROR_FAIL;
                    break;
                }
                pTask->StartWatch( false );
                break;
            }
        case tokFlowCtrl:
            {
                m_bFlowCtrl = true;
                pTask->StopWatch( false );
                break;
            }
        default:
            {
                ret = -EINVAL;
                break;
            }
        }

        if( ret == STATUS_PENDING )
        {
            m_queWritingIrps.push_back( pIrp );
            if( m_queWritingIrps.size() ==
                STM_MAX_QUEUE_SIZE )
                bNotify = true;
        }

    }while( 0 );
    if( ret == -EPIPE )
    {
        pTask->OnError( ret );
        return ret;
    }
    if( bNotify )
    {
        BufPtr pNullBuf;
        SendNotify( tokFlowCtrl, pNullBuf );
    }

    return ret;
}

gint32 CUnixSockStmPdo::OnSubmitIrp(
    IRP* pIrp )
{

    gint32 ret = 0;
    do{
        if( pIrp == nullptr ||
            pIrp->GetStackSize() == 0 )
        {
            ret = -EINVAL;
            break;
        }

        // let's process the func irps
        if( pIrp->MajorCmd() != IRP_MJ_FUNC )
        {
            ret = -EINVAL;
            break;
        }

        switch( pIrp->MinorCmd() )
        {
        case IRP_MN_READ:
            {
                ret = SubmitReadIrp( pIrp );
                break;
            }
        case IRP_MN_WRITE:
            {
                ret = SubmitWriteIrp( pIrp );
                break;
            }
        case IRP_MN_IOCTL:
            {
                switch( pIrp->CtrlCode() )
                {
                case CTRLCODE_LISTENING:
                case CTRLCODE_STREAM_CMD:
                    {
                        ret = SubmitIoctlCmd( pIrp );
                        break;
                    }
                default:
                    {
                        ret = -ENOTSUP;
                        break;
                    }
                }

                if( ret != -ENOTSUP )
                    break;
                // fall through
            }
        default:
            {
                ret = PassUnknownIrp( pIrp );
                break;
            }
        }

    }while( 0 );

    if( ret != STATUS_PENDING )
    {
        pIrp->GetCurCtx()->SetStatus( ret );
    }

    return ret;
}

gint32 CUnixSockStmPdo::SubmitReadIrp(
    PIRP pIrp )
{
    gint32 ret = 0;
    do{
        if( pIrp == nullptr ||
            pIrp->GetStackSize() == 0 )
        {
            ret = -EINVAL;
            break;
        }

        CIoWatchTask* pTask = m_pIoWatch;
        if( pTask == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        CStdRTMutex oTaskLock( pTask->GetLock() );
        CStdRMutex oPortLock( GetLock() );
        if( !m_queReadingIrps.empty() )
        {
            m_queReadingIrps.push_back( pIrp );
            ret = STATUS_PENDING;
            break;
        }
        else if( m_queDataPackets.empty() )
        {
            m_queReadingIrps.push_back( pIrp );
            ret = STATUS_PENDING;
            break;
        }
        else
        {
            guint32 dwSize = m_queDataPackets.size();
            IrpCtxPtr& pCtx = pIrp->GetCurCtx();
            pCtx->SetRespData(
                m_queDataPackets.front() );
            m_queDataPackets.pop_front();
            ret = STATUS_SUCCESS;

            // allow incoming data if the queue size
            // below the threshold
            if( m_bFlowCtrl )
                break;

            if( dwSize == STM_MAX_QUEUE_SIZE )
                pTask->StartWatch( false );

            break;
        }

    }while( 0 );

    return ret;
}

gint32 CUnixSockStmPdo::SubmitWriteIrp(
    PIRP pIrp )
{
    gint32 ret = 0;
    bool bNotify = false;
    CIoWatchTask* pTask = m_pIoWatch;
    if( pTask == nullptr )
        return -EFAULT;

    do{
        if( pIrp == nullptr ||
            pIrp->GetStackSize() == 0 )
        {
            ret = -EINVAL;
            break;
        }

        CStdRTMutex oTaskLock(
            pTask->GetLock() );
        CStdRMutex oPortLock( GetLock() );
        if( m_queWritingIrps.size() >=
            STM_MAX_QUEUE_SIZE )
        {
            ret = ERROR_QUEUE_FULL;
            break;
        }
        else if( !m_queWritingIrps.empty() )
        {
            m_queWritingIrps.push_back( pIrp );
            if( m_queWritingIrps.size() ==
                STM_MAX_QUEUE_SIZE )
                bNotify = true;
            ret = STATUS_PENDING;
            break;
        }
        else
        {
            IrpCtxPtr& pCtx = pIrp->GetCurCtx();
            BufPtr& pBuf = pCtx->m_pReqData;
            if( unlikely(
                pBuf.IsEmpty() || pBuf->empty() ) )
            {
                ret = -EINVAL;
                break;
            }
            else if( unlikely( pBuf->size() >
                MAX_BYTES_PER_TRANSFER ) )
            {
                ret = -E2BIG;
                break;
            }

            ret = pTask->WriteStream( pBuf );
            if( ret == STATUS_PENDING )
            {
                m_queWritingIrps.push_back( pIrp );
                if( m_queWritingIrps.size() ==
                    STM_MAX_QUEUE_SIZE )
                    bNotify = true;
            }

            break;
        }

    }while( 0 );

    if( ret == -EPIPE )
    {
        pTask->OnError( ret );
        return ret;
    }

    if( bNotify )
    {
        BufPtr pNullBuf( true );
        SendNotify( tokFlowCtrl, pNullBuf );
    }

    return ret;
}

gint32 CUnixSockStmPdo::SendNotify(
    guint8 byToken, BufPtr& pBuf )
{
    gint32 ret = 0;

    CStdRMutex oPortLock( GetLock() );
    guint32 dwPortState = GetPortState();
    if( !( dwPortState == PORT_STATE_READY
         || dwPortState == PORT_STATE_BUSY_SHARED ) )
        return ERROR_STATE;
        
    CIoWatchTask* pTask = m_pIoWatch;

    BufPtr pEvtBuf( true );
    switch( byToken )
    {
    case tokPing:
    case tokPong:
    case tokFlowCtrl:
    case tokLift:
        {
            *pEvtBuf = byToken;
            break;
        }
    case tokError:
        {
            *pEvtBuf = tokError;
            pEvtBuf->Append(
                ( guint8* )pBuf->ptr(),
                pBuf->size() );
            break;
        }
    case tokData:
    case tokProgress:
        {
            if( pBuf.IsEmpty() || pBuf->empty() )
            {
                ret = -EINVAL;
                break;
            }
            guint32 dwSize = htonl( pBuf->size() );
            if( pBuf->offset() >= UXPKT_HEADER_SIZE )
            {
                pBuf->SetOffset( pBuf->offset() -
                    UXPKT_HEADER_SIZE );
                pBuf->ptr()[ 0 ] = byToken;
                memcpy( pBuf->ptr() + 1,
                    &dwSize, sizeof( dwSize ) );
                pEvtBuf = pBuf;
            }
            else
            {
                *pEvtBuf = byToken;
                pEvtBuf->Append( ( guint8* )&dwSize,
                    sizeof( dwSize ) );
                pEvtBuf->Append( ( guint8* )pBuf->ptr(),
                    pBuf->size() );
            }
            break;
        }
    default:
        ret = -ENOTSUP;
        break;
    }

    if( ERROR( ret ) )
        return ret;

    m_queEventPackets.push_back( pEvtBuf );

    IrpPtr pEventIrp;
    if( !m_queListeningIrps.empty() )
    {
        pEventIrp = m_queListeningIrps.front(); 
        m_queListeningIrps.pop_front();
    }
    
    if( pEventIrp.IsEmpty() )
    {
        if( m_queEventPackets.size() >=
            STM_MAX_QUEUE_SIZE )
        {
            // stop reading from the unix sock
            pTask->StopWatch( false );
        }
        return 0;
    }

    BufPtr pPayload = m_queEventPackets.front();
    byToken = ( guint8 )pPayload->ptr()[0];
    m_queEventPackets.pop_front(); 
    oPortLock.Unlock();

    CStdRMutex oIrpLock( pEventIrp->GetLock() );
    ret = pEventIrp->CanContinue(
        IRP_STATE_READY );
    if( ERROR( ret ) )
    {
        CStdRMutex oPortLock( GetLock() );
        m_queEventPackets.push_front( pPayload );
        return -EAGAIN;
    }

    IrpCtxPtr& pCtx =
        pEventIrp->GetTopStack();

    switch( byToken )
    {
    case tokPing:
    case tokPong:
    case tokFlowCtrl:
    case tokLift:
    case tokError:
    case tokData:
    case tokProgress:
        {
            ret = STATUS_SUCCESS;
            pCtx->SetRespData( pPayload );
            pCtx->SetStatus( ret );
            break;
        }
    default:
        {
            ret = -ENOTSUP;
            pCtx->SetStatus( -ENOTSUP );
            break;
        }
    }
    oIrpLock.Unlock();

    auto pMgr = GetIoMgr();
    pMgr->CompleteIrp( pEventIrp );

    return STATUS_SUCCESS;
}

gint32 CUnixSockStmPdo::OnStmRecv(
    BufPtr& pBuf )
{
    if( pBuf.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;
    do{
        if( m_bListenOnly )
        {
            // forward the data to the listening queue
            ret = OnUxSockEvent( tokData, pBuf );
            break;
        }

        CIoWatchTask* pTask = m_pIoWatch;
        if( pTask == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        CStdRTMutex oTaskLock( pTask->GetLock() );
        CStdRMutex oPortLock( GetLock() );
        if( !m_queDataPackets.empty() )
        {
            m_queDataPackets.push_back( pBuf );
            if( m_queDataPackets.size() >=
                STM_MAX_QUEUE_SIZE )
            {
                // stop reading from the unix sock
                pTask->StopWatch( false );
            }
        }
        else if( m_queReadingIrps.empty() )
        {
            m_queDataPackets.push_back( pBuf );
        }
        else
        {
            IrpPtr pIrp = m_queReadingIrps.front();
            m_queReadingIrps.pop_front();
            oPortLock.Unlock();
            oTaskLock.Unlock();

            CStdRMutex oIrpLock( pIrp->GetLock() );
            ret = pIrp->CanContinue( IRP_STATE_READY );
            if( ERROR( ret ) )
                break;

            IrpCtxPtr& pCtx = pIrp->GetTopStack();
            pCtx->m_pRespData = pBuf;
            ret = 0;
            pCtx->SetStatus( ret );
            oIrpLock.Unlock();

            GetIoMgr()->CompleteIrp( pIrp );
        }

    }while( 0 );

    return ret;
}

gint32 CUnixSockStmPdo::OnSendReady()
{
    gint32 ret = 0;
    bool bPending = false;
    do{
        CIoWatchTask* pTask = m_pIoWatch;
        if( pTask == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        std::vector< IrpPtr > vecIrpToComp;
        IrpPtr pEventIrp;
        bool bNotify = false;

        CStdRTMutex oTaskLock( pTask->GetLock() );
        CStdRMutex oPortLock( GetLock() );
        if( IsConnected() == false )
        {
            ret = ERROR_STATE;
            break;
        }

        if( !m_queWritingIrps.empty() )
        {
            bool bFirst = true;
            while( !m_queWritingIrps.empty() )
            {
                guint32 dwSize = m_queWritingIrps.size();
                IrpPtr pHeadIrp = m_queWritingIrps.front();
                m_queWritingIrps.pop_front();

                if( bFirst )
                {
                    pHeadIrp->GetTopStack()->SetStatus( 0 );
                    bFirst = false;
                }

                vecIrpToComp.push_back( pHeadIrp );

                if( dwSize == 1 )
                {
                    break;
                }
                else if( dwSize == STM_MAX_QUEUE_SIZE )
                {
                    // notify the write control is released
                    bNotify = true;
                }

                IrpPtr pIrpToWrite =
                    m_queWritingIrps.front();

                oTaskLock.Unlock();
                oPortLock.Unlock();

                CStdRMutex oIrpLock(
                    pIrpToWrite->GetLock() );
                ret = pIrpToWrite->CanContinue(
                    IRP_STATE_READY );
                if( ERROR( ret ) )
                    break;

                oTaskLock.Lock();
                oPortLock.Lock();
                if( !( m_queWritingIrps.front() ==
                    pIrpToWrite ) )
                {
                    // the queue is changed during
                    // re-locking
                    continue;
                }

                IrpCtxPtr pCtx =
                    pIrpToWrite->GetTopStack();

                BufPtr& pBuf = pCtx->m_pReqData;
                if( unlikely(
                    pBuf.IsEmpty() || pBuf->empty() ) )
                {
                    pCtx->SetStatus( -EINVAL );
                }
                else if( unlikely( pBuf->size() >
                    MAX_BYTES_PER_TRANSFER ) )
                {
                    ret = -E2BIG;
                    break;
                }
                else
                {
                    ret = pTask->WriteStream(
                        pCtx->m_pReqData );
                    if( ret == STATUS_PENDING )
                    {
                        bPending = true;
                        break;
                    }
                    pCtx->SetStatus( ret );
                }
            }
        }
        else
        {
            pTask->StopWatch();
        }

        oPortLock.Unlock();
        oTaskLock.Unlock();

        for( auto pIrpToComp : vecIrpToComp )
        {
            CStdRMutex oIrpLock(
                pIrpToComp->GetLock() );

            ret = pIrpToComp->CanContinue(
                IRP_STATE_READY );

            if( ERROR( ret ) )
            {
                ret = 0;
                continue;
            }

            IrpCtxPtr& pCtx = pIrpToComp->GetTopStack();
            pCtx->SetStatus( ret = 0 );
            oIrpLock.Unlock();

            GetIoMgr()->CompleteIrp( pIrpToComp );
        }

        if( bNotify )
        {
            BufPtr pNullBuf;
            SendNotify( tokLift, pNullBuf );
        }

    }while( 0 );

    if( ERROR( ret ) )
        return ret;

    if( bPending )
        ret = STATUS_PENDING;

    return ret;
}

gint32 CUnixSockStmPdo::OnUxSockEvent(
    guint8 byToken, BufPtr& pBuf )
{
    return SendNotify( byToken, pBuf );
}

CUnixSockBusDriver::CUnixSockBusDriver(
    const IConfigDb* pCfg )
    : super( pCfg )
{
    SetClassId( clsid( CUnixSockBusDriver ) );
}

gint32 CUnixSockBusDriver::Probe(
        IPort* pLowerPort,
        PortPtr& pNewPort,
        const IConfigDb* pConfig )
{
    gint32 ret = 0;

    do{
        // we don't have dynamic bus yet
        CfgPtr pCfg( true );

        if( pConfig != nullptr )
            *pCfg = *pConfig;

        CCfgOpener oCfg( ( IConfigDb* )pCfg );
        ret = oCfg.SetStrProp( propPortClass,
            PORT_CLASS_UXSOCK_BUS );

        if( ERROR( ret ) )
            break;

        ret = oCfg.SetIntProp( propClsid,
            clsid( CUnixSockBusPort ) );

        if( ERROR( ret ) )
            break;

        ret = CreatePort( pNewPort, pCfg );
        if( ERROR( ret ) )
            break;

        if( pLowerPort != nullptr )
        {
            ret = pNewPort->AttachToPort(
                pLowerPort );
        }

        if( SUCCEEDED( ret ) )
        {
            CEventMapHelper< CPortDriver >
                oEvtHelper( this );

            oEvtHelper.BroadcastEvent(
                eventPnp,
                eventPortAttached,
                PortToHandle( pNewPort ),
                nullptr );
        }

    }while( 0 );

    return ret;
}

gint32 CUnixSockBusPort::CreatePdoPort(
    IConfigDb* pCfg,
    PortPtr& pNewPort )
{
    gint32 ret = 0;

    // Note that we are within a port lock
    do{
        if( pCfg == nullptr )
            return -EINVAL;

        if( !pCfg->exist( propPortClass ) )
        {
            ret = -EINVAL;
            break;
        }

        std::string strPortClass;
        CCfgOpener oExtCfg( ( IConfigDb* )pCfg );

        ret = oExtCfg.GetStrProp(
            propPortClass, strPortClass );

        // by default, we creaete the tcp stream
        // pdo, and the only pdo we support
        if( ERROR( ret ) )
        {
            strPortClass =
                PORT_CLASS_UXSOCK_STM_PDO;
        }

        if( strPortClass
            == PORT_CLASS_UXSOCK_STM_PDO )
        {
            ret = CreateUxStreamPdo(
                pCfg, pNewPort );
        }
        else if( strPortClass
            == PORT_CLASS_STMCP_PDO )
        {
            ret = CreateStmCpPdo(
                pCfg, pNewPort );
        }
        else
        {
            ret = -ENOTSUP;
        }

    }while( 0 );

    return ret;
}

gint32 CUnixSockBusPort::BuildPdoPortName(
    IConfigDb* pCfg,
    std::string& strPortName )
{
    if( pCfg == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CCfgOpener oCfgOpener( pCfg );
        if( pCfg->exist( propPortClass ) )
        {
            // port name is composed in the following
            // format:
            // <PortClassName> + "_" + <PortId>
            std::string strClass;
            ret = oCfgOpener.GetStrProp(
                propPortClass, strClass );

            if( ERROR( ret ) )
                break;

            if( strClass != PORT_CLASS_UXSOCK_STM_PDO && 
                strClass != PORT_CLASS_STMCP_PDO )
            {
                ret = -EINVAL;
                break;
            }

            guint32 dwPortId = 0;
            if( pCfg->exist( propPortId ) )
            {
                ret = oCfgOpener.GetIntProp(
                    propPortId, dwPortId );
                if( ERROR( ret ) )
                    break;

                if( dwPortId == ( guint32 )-1 )
                {
                    dwPortId = 
                        NewPdoId() + 0x80000000;
                    oCfgOpener.SetIntProp(
                        propPortId, dwPortId );
                }
            }
            else if( pCfg->exist( propFd ) )
            {
                ret = oCfgOpener.GetIntProp(
                    propFd, dwPortId );
                if( ERROR( ret ) )
                    break;
            }
            else
            {
                ret = -EINVAL;
                break;
            }

            strPortName = strClass +
                std::string( "_" )
                + std::to_string( dwPortId );
        }
        else if( pCfg->exist( propPortName ) )
        {
            ret = oCfgOpener.GetStrProp(
                propPortName, strPortName );

            if( ERROR( ret ) )
                break;
        }
        else
        {
            ret = ERROR_FAIL;
        }

    }while( 0 );

    return ret;
}

gint32 CUnixSockBusPort::CreateUxStreamPdo(
    IConfigDb* pCfg,
    PortPtr& pNewPort )
{
    if( pCfg == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        std::string strPortName;
        guint32 dwPortId = 0;

        CCfgOpener oExtCfg(
            ( IConfigDb* )pCfg );

        ret = oExtCfg.GetIntProp(
            propPortId, dwPortId );

        if( ERROR( ret ) )
        {
            ret = oExtCfg.GetIntProp(
                propFd, dwPortId );

            if( ERROR( ret ) )
                break;
        }

        // verify if the port already exists
        if( this->PortExist( dwPortId ) )
        {
            ret = -EEXIST;
            break;
        }

        ret = pNewPort.NewObj(
            clsid( CUnixSockStmPdo ), pCfg );

        // the pdo port `Start()' will be deferred
        // till the complete port stack is built.
        //
        // And well, we are done here

    }while( 0 );

    return ret;
}

gint32 CUnixSockBusPort::CreateStmCpPdo(
    IConfigDb* pCfg,
    PortPtr& pNewPort )
{
    if( pCfg == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        std::string strPortName;
        guint32 dwPortId = 0;

        CCfgOpener oExtCfg(
            ( IConfigDb* )pCfg );

        ret = oExtCfg.GetIntProp(
            propPortId, dwPortId );

        if( ERROR( ret ) )
            break;

        // verify if the port already exists
        if( this->PortExist( dwPortId ) )
        {
            ret = -EEXIST;
            break;
        }

        ret = pNewPort.NewObj(
            clsid( CStmCpPdo ), pCfg );

        // the pdo port `Start()' will be deferred
        // till the complete port stack is built.
        //
        // And well, we are done here

    }while( 0 );

    return ret;
}

gint32 CUnixSockBusPort::PreStart( IRP* pIrp )
{
    CIoManager* pMgr = GetIoMgr();
    CLoopPools& oPools = pMgr->GetLoopPools();

    guint32 dwCount = std::max( 1U,
        pMgr->GetNumCores() >> 1 );

    gint32 ret = oPools.CreatePool(
        UXSOCK_TAG, "UxLoop-", dwCount, 20 );
    if( ERROR( ret ) )
        return ret;

    return super::PreStart( pIrp );
}

gint32 CUnixSockBusPort::PostStop( IRP* pIrp )
{
    CIoManager* pMgr = GetIoMgr();
    CLoopPools& oPools = pMgr->GetLoopPools();
    oPools.DestroyPool( UXSOCK_TAG );
    return 0;
}

}
