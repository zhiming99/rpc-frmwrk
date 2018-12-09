/*
 * =====================================================================================
 *
 *       Filename:  stream.cpp
 *
 *    Description:  implementation of stream related classes
 *
 *        Version:  1.0
 *        Created:  11/22/2018 11:21:25 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:  
 *
 * =====================================================================================
 */
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "frmwrk.h"
#include "stream.h"

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
        if( iStat == recvTok )
        {
            guint8 byToken = tokInvalid;
            ret = read( m_iFd, &byToken, sizeof( byToken ) );
            if( ERROR( ret ) )
            {
                if( errno == EAGAIN || errno == EWOULDBLOCK )
                {
                    ret = -EAGAIN;
                    break;
                }
            }

            if( byToken == tokPing ||
                byToken == tokPong ||
                byToken == tokClose )
            {
                ret = byToken;
                break;
            }

            if( byToken != tokData )
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
                pBuf->Resize( m_dwBytesToRead );
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
        }

        if( ERROR( ret ) )
            break;

        if( GetRecvState() != recvData )
        {
            ret = ERROR_STATE;
            break;
        }

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
                ret = tokData;
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
        }

    }while( 0 );

    if( ERROR( ret ) && ret != -EAGAIN )
    {
        m_dwBytesToRead = 0;
        m_dwOffsetRead = ( guint32 )-1;
    }

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
    return sendData;
}

gint32 CSendQue::OnIoReady()
{
    gint32 ret = 0;
    do{
        if( m_queBufWrite.empty() )
            break;

        STM_PACKET& oPkt =
            m_queBufWrite.front();

        if( GetSendState() == sendTok )
        {
            ret = Send( m_iFd, &oPkt.byToken,
                sizeof( oPkt.byToken ) );

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

            if( oPkt.byToken == tokPing ||
                oPkt.byToken == tokPong ||
                oPkt.byToken == tokClose )
            {
                // next packet
                --m_dwBytesPending;
                m_queBufWrite.pop_front();
                continue;
            }

            if( oPkt.byToken != tokData )
            {
                ret = -ENOTSUP;
                break;
            }

            m_dwOffsetWrite = 0;
        }
        while( GetSendState() == sendSize )
        {
            guint32 iLeft =
                sizeof( guint32 ) - m_dwOffsetWrite;

            guint32 dwSize = htonl( oPkt.dwSize );
            guint8* pos = ( ( guint8* )&dwSize ) +
                m_dwOffsetWrite;

            ret = Send( m_iFd, pos, iLeft );

            if( ret == ( gint32 )iLeft )
            {
                m_dwOffsetWrite = 0;
                m_dwBytesToWrite = oPkt.dwSize;
                // fall through
            }
            else if( ret > 0 )
            {
                m_dwOffsetWrite += ret;
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

        if( GetSendState() != sendData )
        {
            ret = ERROR_STATE;
            break;
        }

        BufPtr pBuf;

        while( m_dwBytesToWrite > 0 )
        {
            if( pBuf.IsEmpty() )
                pBuf = oPkt.pData;

            if( pBuf.IsEmpty() )
            {
                ret = -EFAULT;
                break;
            }

            if( m_dwBytesToWrite >
                pBuf->size() - m_dwOffsetWrite )
            {
                ret = -ERANGE;
                break;
            }

            guint32 iLeft =
                pBuf->size() - m_dwOffsetWrite;

            guint8* pos = ( ( guint8* )pBuf->ptr() ) +
                m_dwOffsetWrite;

            ret = Send( m_iFd, pos, iLeft );
            if( ret == ( gint32 )iLeft )
            {
                // the packet is done
                m_dwOffsetWrite = ( guint32 )-1;
                m_dwBytesToWrite = 0;

                m_dwBytesPending -= ( 1 +
                    sizeof( guint32 ) + pBuf->size() );

                m_queBufWrite.pop_front();
                ret = 0;
                break;
            }
            else if( ret > 0 )
            {
                m_dwBytesToWrite -= ret;
                m_dwOffsetWrite += ret;
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

        break;

    }while( 1 );

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
    BufPtr& pBuf )
{
    STM_PACKET oPkt;
    oPkt.byToken = tokData;
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

// new data arrived
gint32 CIoWatchTask::OnDataReady(
    BufPtr pBuf )
{
    gint32 ret = 0;
    if( m_pStream != nullptr )
    {
        m_qwBytesRead += pBuf->size();
        ret = m_pStream->OnStmRecv(
            ( HANDLE )this, pBuf );
    }
    return ret;
}

gint32 CIoWatchTask::OnSendReady()
{
    gint32 ret = 0;
    if( m_pStream != nullptr )
    {
        ret = m_pStream->OnSendReady(
            ( HANDLE )this );
    }
    return ret;
}

// ping/pong packets
gint32 CIoWatchTask::OnPingPong(
    bool bPing )
{
    gint32 ret = 0;
    if( m_pStream != nullptr )
    {
        ret = m_pStream->OnPingPong(
            ( HANDLE )this, bPing );
    }
    return ret;
}

// received by server to notify a client initiated
// close
gint32 CIoWatchTask::OnClose()
{
    if( m_pStream == nullptr )
        return -EFAULT;

    IStreamServer* pSvr = dynamic_cast
        < IStreamServer* >( m_pStream );
    if( pSvr == nullptr )
        return -EFAULT;

    pSvr->OnClose( ( HANDLE )this );
    return 0;
}

void CIoWatchTask::OnError( gint32 iError )
{
    if( m_pStream == nullptr )
        return;

    m_pStream->OnChannelError(
        ( HANDLE )this, iError );
}

gint32 CIoWatchTask::OnIoWatchEvent(
    guint32 revents )
{
    gint32 ret = 0;
    do{
        if( revents & POLLERR ||
            revents & POLLHUP )
        {
            ret = OnIoError( revents );
        }
        else if( revents & POLLIN ||
            revents == POLLOUT )
        {
            ret = OnIoReady( revents ); 
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

        ret = oCfg.GetIntProp( 0,
            ( guint32& )m_iFd );

        if( ERROR( ret ) )
            break;

        CRpcServices* pIf = nullptr;
        ret = oCfg.GetPointer(
            propIfPtr, pIf );

        if( ERROR( ret ) ) 
            break;

        if( unlikely( pIf == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }

        m_pStream = dynamic_cast
            < IStream* >( pIf );

        if( unlikely( m_pStream == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }
        
        CIoManager* pMgr = pIf->GetIoMgr();
        CMainIoLoop* pLoop = pMgr->GetMainIoLoop();
        if( unlikely( pLoop == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }

        oCfg.ClearParams();
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
            m_hReadWatch = 0;
            break;
        }

        pLoop->StartSource( m_hReadWatch, srcIo );
        
        // return STATUS_PENDING to wait for IO
        ret = STATUS_PENDING;

    }while( 0 );

    return ret;
}

gint32 CIoWatchTask::operator()(
    guint32 dwContext )
{
    gint32 ret = 0;
    try{
        EnumTaskState iState = GetTaskState();

        if( iState == stateStopped )
            return ERROR_STATE;

        if( iState == stateStarted )
        {
            switch( dwContext )
            {
            case eventIoWatch:
                {
                    std::vector< guint32 > vecParams;
                    ret = GetParamList( vecParams );
                    if( ERROR( ret ) )
                        break;

                    guint32 dwCount = GetArgCount(
                        &IEventSink::OnEvent );

                    if( vecParams.size() < dwCount )
                    {
                        ret = -EINVAL;
                        break;
                    }
                    
                    guint32 revents = vecParams[ 1 ];
                    ret = OnIoWatchEvent( revents );

                    if( ERROR( ret ) && ret != -EAGAIN )
                        ret = G_SOURCE_REMOVE;
                    else
                        ret = G_SOURCE_CONTINUE;

                    return ret;
                }
            default:
                {
                    break;
                }
            }
        }
        ret = super::operator()( dwContext );
    }
    catch( std::runtime_error& e )
    {
        // lock failed
        ret = -EACCES;
    }
    return ret;
}

gint32 CIoWatchTask::OnEvent(
    EnumEventId iEvent,
    guint32 dwParam1,
    guint32 dwParam2,
    guint32* pData )
{
    // Lock to prevent re-entrance 
    gint32 ret = 0;
    try{
        CStdRTMutex oTaskLock( GetLock() );
        switch( iEvent )
        {
        case eventIoWatch:
            {
                // we have some special processing for
                // eventIoWatch to bypass the normal
                // CIfParallelTask's process flow.
                IntVecPtr pVec( true );
                std::vector< guint32 >& oVec = ( *pVec )();
                oVec.push_back( ( guint32 )iEvent );
                oVec.push_back( ( guint32 )dwParam1 );
                oVec.push_back( ( guint32 )dwParam2 );
                oVec.push_back( ( guint32 )pData);

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
    }
    catch( std::runtime_error& e )
    {
        // lock failed
        ret = -EACCES;
    }
    return ret;
}

gint32 CIoWatchTask::StartStopWatch(
    bool bStop, bool bWrite )
{
    gint32 ret = 0;
    if( m_pStream == nullptr )
        return -EINVAL;;

    CRpcServices* pIf = dynamic_cast
        < CRpcServices* >( m_pStream );

    if( pIf == nullptr )
        return -EFAULT;

    CIoManager* pMgr = pIf->GetIoMgr();
    CMainIoLoop* pLoop = pMgr->GetMainIoLoop();
#ifdef _USE_LIBEV
    HANDLE hWatch = m_hWriteWatch;
    if( unlikely( bWrite == false ) )
        hWatch = m_hReadWatch;

    if( bStop )
        ret = pLoop->StopSource( hWatch, srcIo );
    else
        ret = pLoop->StartSource( hWatch, srcIo );
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

                if( ret == tokPong || ret == tokPing )
                {
                    OnPingPong( ret == tokPing );
                }
                else if( ret == tokClose )
                {
                    OnClose();
                }
                else if( ret == tokData )
                {
                    BufPtr pBuf;
                    ret = m_oRecvFilter.ReadStream( pBuf );
                    if( ERROR( ret ) )
                    {
                        ret = -EAGAIN;
                        break;
                    }
                    ret = OnDataReady( pBuf );
                }
            // till exhause all the data in the buffer
            }while( ret > 0 );

            if( ret == -EAGAIN )
                ret = 0;

            break;
        }
        else if( revent & POLLOUT )
        {
            do{
                ret = m_oSendQue.OnIoReady();
                if( ret == -EAGAIN )
                    StartWatch();
                else
                {
                    // no data to send
                    StopWatch();
                }

                if( SUCCEEDED( ret ) &&
                    m_oSendQue.GetPendingWrite() == 0 )
                    ret = OnSendReady();

            }while( 0 );
        }
        else
        {
            ret = -ENOTSUP;
        }

    }while( 0 );

    if( ERROR( ret ) && ret != -EAGAIN )
        OnError( ret );

    return ret;
}

gint32 CIoWatchTask::ReleaseChannel()
{
    gint32 ret = 0;
    do{
        CRpcServices* pIf = dynamic_cast
            < CRpcServices* >( m_pStream );
        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        CIoManager* pMgr = pIf->GetIoMgr();
        CMainIoLoop* pLoop =
            pMgr->GetMainIoLoop();

        if( unlikely( pLoop == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }

        pLoop->RemoveIoWatch( m_hReadWatch );
        pLoop->RemoveIoWatch( m_hWriteWatch );
        m_hReadWatch = m_hWriteWatch = 0;
        HANDLE hChannel = ( HANDLE )( CObjBase* )this;
        m_pStream->RemoveChannel( hChannel );

    }while( 0 );

    if( m_iFd > 0 )
    {
        close( m_iFd );
        m_iFd = -1;
    }

    UNREFERENCED( ret == 0 );

    return 0;
}

gint32 CIoWatchTaskProxy::OnConnected()
{
    if( m_pStream == nullptr )
        return -EFAULT;

    IStreamProxy* pStmProxy = dynamic_cast
        < IStreamProxy* >( m_pStream );
    if( pStmProxy == nullptr )
        return -EFAULT;

    return pStmProxy->OnConnected(
        ( HANDLE )this );
}

gint32 CIoWatchTaskProxy::OnPingPong(
    bool bPing )
{
    gint32 ret = 0;
    do{
        if( m_pStream == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        bool bConnected = false;
        if( bPing == false &&
            m_dwState == stateStarting )
        {
            m_dwState = stateConnected;
            bConnected = true;
        }
        if( m_dwState == stateConnected &&
            !bConnected )
        {
            CIfIoReqTask* pIoTask = m_pIoTask;
            if( pIoTask == nullptr )
                break;

            // we have the risk to dead-lock, defer the
            // call out of this lock
            CRpcServices* pIf = dynamic_cast
                < CRpcServices* >( m_pStream );

            if( pIf != nullptr )
            {
                DEFER_CALL( pIf->GetIoMgr(), pIoTask,
                    &CIfIoReqTask::ResetTimer );
            }
            ret = StartTimer();
            if( ERROR( ret ) )
                break;

            ret = m_pStream->OnPingPong(
                ( HANDLE )this, bPing );

            break;
        }

        if( bConnected )
        {
            ret = OnConnected();
            break;
        }

    }while( 0 );

    return ret;
}

gint32 CIoWatchTaskProxy::OnKeepAlive(
    guint32 dwContext )
{
    SendPingPong();
    return STATUS_PENDING;
}

gint32 CIoWatchTaskProxy::RunTask()
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg( ( IConfigDb* )
            this->GetConfig() );

        ObjPtr pObj;
        ret = oCfg.GetObjPtr( 1, pObj );
        if( ERROR( ret ) )
            break;

        SetReqTask( pObj );
        ret = super::RunTask();
        if( ERROR( ret ) )
            break;

        ret = StartTimer();
        if( ERROR( ret ) )
            break;

        ret = STATUS_PENDING;

    }while( 0 );

    return ret;
}

gint32 CIoWatchTaskProxy::ReleaseChannel()
{
    gint32 ret = super::ReleaseChannel();
    m_pIoTask.Clear();

    if( m_iTimerId <= 0 )
        return ret;

    return StopTimer();
}

gint32 CIoWatchTaskProxy::StartStopTimer( bool bStart )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg( ( IConfigDb* )
            this->GetConfig() );

        CRpcServices* pIf = nullptr;
        ret = oCfg.GetPointer(
            propIfPtr, pIf );

        if( ERROR( ret ) ) 
            break;

        if( unlikely( pIf == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }
        
        CCfgOpenerObj oIfCfg( pIf );
        guint32 dwTimeOutSec = 0;

        ret = oIfCfg.GetIntProp(
            propKeepAliveSec, dwTimeOutSec );

        if( ERROR( ret ) )
            break;

        if( dwTimeOutSec == 0 )
        {
            ret = -EINVAL;
            break;
        }

        CIoManager* pMgr = pIf->GetIoMgr();
        CUtilities& oUtils = pMgr->GetUtils();
        CTimerService& oTimerSvc =
            oUtils.GetTimerSvc();

        if( bStart )
        {
            m_iTimerId = oTimerSvc.AddTimer(
                dwTimeOutSec, this, eventKeepAlive );
        }
        else if( m_iTimerId > 0 )
        {
            oTimerSvc.RemoveTimer( m_iTimerId );
            m_iTimerId = 0;
        }

    } while( 0 );

    return ret;
}

gint32 CIoWatchTaskServer::RunTask()
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg( ( IConfigDb* )
            this->GetConfig() );

        ObjPtr pObj;
        ret = oCfg.GetObjPtr( 1, pObj );
        if( ERROR( ret ) )
            break;

        m_pInvTask = pObj;
        if( m_pInvTask.IsEmpty() )
        {
            ret = -EINVAL;
            break;
        }

        ret = oCfg.GetObjPtr( 2, pObj );
        if( ERROR( ret ) )
            break;

        m_pDataDesc = pObj;
        if( m_pDataDesc.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        ret = super::RunTask();
        if( ERROR( ret ) )
            break;

        // notify the client that we are ready for
        // receiving
        ret = SendPingPong( false );
        if( ERROR( ret ) )
            break;

         ret = STATUS_PENDING;

    }while( 0 );

    return ret;
}

gint32 IStream::GetWatchTask(
    HANDLE hChannel,
    TaskletPtr& pTask ) const
{
    gint32 ret = 0;

    if( hChannel == 0 )
        return -EINVAL;

    do{
        CRpcServices* pIf =
            dynamic_cast< CRpcServices* >
            ( const_cast< IStream* >( this ) );

        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        CStdRMutex oIfLock( pIf->GetLock() );
        WATCH_MAP::const_iterator citr =
        m_mapWatches.find( hChannel );
        if( citr == m_mapWatches.cend() )
        {
            ret = -ENOENT;
            break;
        }

        const TaskletPtr& pTemp = citr->second;

        pTask = const_cast< TaskletPtr& >( pTemp );

    }while( 0 );

    return ret;
}

gint32 IStream::RemoveChannel( HANDLE hChannel )
{
    if( hChannel == 0 )
        return -EINVAL;

    CRpcServices* pIf =
        dynamic_cast< CRpcServices* >
        ( const_cast< IStream* >( this ) );

    if( pIf == nullptr )
        return -EFAULT;

    CStdRMutex oIfLock( pIf->GetLock() );
    m_mapWatches.erase( hChannel );

    return 0;
}

gint32 IStream::WriteStream(
    HANDLE hChannel, BufPtr& pBuf )
{
    TaskletPtr pTask;
    if( hChannel == 0 || pBuf.IsEmpty() )
        return -EINVAL;
    
    gint32 ret = 0;
    do{
        ret = GetWatchTask(
            hChannel, pTask );

        if( ERROR( ret ) )
            break;

        ret = CanContinue();
        if( ERROR( ret ) )
            break;

        CIoWatchTask* pWatchTask = pTask;
        if( pWatchTask == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ret = pWatchTask->WriteStream( pBuf );

        if( ret == STATUS_PENDING )
            pWatchTask->StartWatch();

    }while( 0 );

    return ret;
}

gint32 IStream::SendPingPong(
    HANDLE hChannel, bool bPing )
{
    TaskletPtr pTask;
    if( hChannel == 0 )
        return -EINVAL;

    gint32 ret = GetWatchTask(
        hChannel, pTask );

    if( ERROR( ret ) )
        return ret;

    ret = CanContinue();
    if( ERROR( ret ) )
        return ret;

    CIoWatchTask* pWatchTask = pTask;
    if( pWatchTask == nullptr )
        return -EFAULT;

    return pWatchTask->SendPingPong( bPing );
}

gint32 CStreamProxy::CancelReqTask(
    IEventSink* pTask, IEventSink* pWatchTask )
{
    if( pTask == nullptr )
        return -EINVAL;

    CIoWatchTaskProxy* pTaskProxy = static_cast
        < CIoWatchTaskProxy* >( pWatchTask );

    if( pTaskProxy == nullptr )
        return 0;

    // stop the keep-alive heartbeat 
    pTaskProxy->ChangeState( stateStopping );
    pTaskProxy->StopTimer();

    guint64 qwTaskId = pTask->GetObjId();
    guint64 qwThisTaksId = 0;

    gint32 ret = UserCancelRequest(
        qwThisTaksId, qwTaskId );

    if( ERROR( ret ) )
        return ret;
    
    if( pWatchTask == nullptr )
        return 0;
    return pTaskProxy->WaitForComplete();
}

gint32 CStreamProxy::OnChannelError(
    HANDLE hChannel, gint32 iError )
{
    gint32 ret = 0;
    TaskletPtr pTask;
    do{
        ret = GetWatchTask( hChannel, pTask );
        if( ERROR( ret ) )
            break;

        CIoWatchTaskProxy* pTaskProxy = pTask;
        if( unlikely( pTaskProxy == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }

        // stop watch to prevent duplicated error
        pTaskProxy->StopWatch( true );
        // stop watch to prevent duplicated error
        pTaskProxy->StopWatch( false );

        TaskletPtr pReqTask =
            pTaskProxy->GetReqTask();

        if( unlikely( pReqTask.IsEmpty() ) )
        {
            ret = -EFAULT;
            break;
        }

        // this is a sync call, let's call it on
        // a stand-alone thread
        TaskletPtr pDeferTask;
        ObjPtr pObj( this );
        ret = DEFER_CALL_NOSCHED(
            pDeferTask, pObj,
            &CStreamProxy::CancelReqTask,
            pReqTask, pTask );

        if( ERROR( ret ) )
            break;

        GetIoMgr()->RescheduleTask(
            pDeferTask, true );

    }while( 0 );

    return ret;
}

gint32 CStreamProxy::CreateSocket(
    int& fdLocal, int& fdRemote )
{
    gint32 ret = 0;
    do{
        int fds[ 2 ];
        ret = socketpair( AF_UNIX,
            SOCK_STREAM | SOCK_NONBLOCK,
            0, fds );
        if( ret == -1 )
        {
            ret = -errno;
            break;
        }

        fdLocal = fds[ 0 ];
        fdRemote = fds[ 1 ];

    }while( 0 );

    return ret;
}

gint32 CStreamProxy::SendRequest(
    IConfigDb* pDataDesc,
    int fd, IEventSink* pCallback )
{
    if( pDataDesc == nullptr || fd < 0 ||
        pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        guint32 dwOffset = 0;
        guint32 dwSize = 0;
        ret = FetchData( pDataDesc,
            fd, dwOffset, dwSize, pCallback );

        if( ret == STATUS_PENDING )
            break;

    }while( 0 );

    return ret;
}
gint32 CStreamProxy::OpenChannel(
    IConfigDb* pDataDesc,
    int& fd, HANDLE& hChannel,
    IEventSink* pCallback )
{
    if( pCallback == nullptr )
        return -EINVAL;

    int iRemote = -1, iLocal = -1;
    gint32 ret = 0;
    do{
        ret = CreateSocket( iLocal, iRemote );
        if( ERROR( ret ) )
            break;

        CCfgOpener oDesc( pDataDesc );
        // add the iid to the data desc
        oDesc.SetIntProp(
            propIid, iid( IStream ) );

        ret = SendRequest( pDataDesc,
            iRemote, pCallback );
 
        if( ret != STATUS_PENDING )
            break;

        guint64 qwTaskId = 0;
        ret = oDesc.GetQwordProp(
            propTaskId, qwTaskId );

        if( ERROR( ret ) )
            break;

        TaskletPtr pReqTask;
        ret = m_pRootTaskGroup->FindTask(
            qwTaskId, pReqTask );

        if( ERROR( ret ) )
            break;

        CParamList oParams;
        oParams[ propIfPtr ] = ObjPtr( this );
        oParams[ propIoMgr ] = ObjPtr( GetIoMgr() );

        oParams.Push( iLocal );
        oParams.Push( ObjPtr( pReqTask ) );
        oParams.Push( ObjPtr( pDataDesc ) );

        TaskletPtr pTask;
        ret = pTask.NewObj(
            clsid( CIoWatchTaskProxy ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        CObjBase* pObjBase = pTask;
        hChannel = ( HANDLE )pObjBase;
        if( true )
        {
            CStdRMutex oIfLock( GetLock() ); 
            m_mapWatches[ hChannel ] = pTask;
        }

        // insert this task to the completion chain of
        // pCallback
        ret = InterceptCallback( pTask, pReqTask );
        if( ERROR( ret ) )
            break;

        // start listening on the fds
        ret = ( *pTask )( eventZero );
        if( ret != STATUS_PENDING )
        {
            iLocal = -1;
            RemoveInterceptCallback(
                pTask, pCallback );
            break;
        }

    }while( 0 );

    if( ret != STATUS_PENDING )
    {
        if( iLocal >= 0 )
        {
            close( iLocal );
            iLocal = -1;
        }
    }

    if( iRemote >= 0 )
    {
        close( iRemote );
        iRemote = -1;
    }

    return ret;
}

// call this helper to start a stream channel
gint32 CStreamProxy::StartStream(
    HANDLE& hChannel, TaskletPtr& pSyncTask )
{
    gint32 ret = 0;
    do{
        int fd = -1;
        CParamList oParams;

        ret = pSyncTask.NewObj(
            clsid( CSyncCallback ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        ret = OpenChannel( oParams.GetCfg(),
            fd, hChannel, pSyncTask );

        if( ERROR( ret ) )
            break;

        if( ret != STATUS_PENDING )
            ret = ERROR_FAIL;
        else
            ret = 0;

    }while( 0 );

    return ret;
}

// call this method when the proxy want to end the
// stream actively
gint32 CStreamProxy::CloseChannel(
    HANDLE hChannel )
{
    TaskletPtr pTask;

    gint32 ret = GetWatchTask(
        hChannel, pTask );

    if( ERROR( ret ) )
        return ret;

    CIoWatchTaskProxy* pWatchTask = pTask;
    if( pWatchTask == nullptr )
        return -EFAULT;

    // stop I/O watch
    pWatchTask->StopWatch( true );
    pWatchTask->StopWatch( false );

    ret = pWatchTask->CloseChannel();
    if( ERROR( ret ) )
        return ret;

    // wait for the close process to complete
    ret = pWatchTask->WaitForComplete();

    return ret;
}

// call this method when the proxy encounters
// fatal error
gint32 CStreamProxy::CancelChannel(
    HANDLE hChannel )
{
    TaskletPtr pTask;
    gint32 ret = 0;
    do{
        ret = GetWatchTask(
            hChannel, pTask );

        if( ERROR( ret ) )
            return ret;

        CIoWatchTaskProxy* pWatchTask = pTask;
        if( pWatchTask == nullptr )
            return -EFAULT;

        TaskletPtr pReqTask = 
            pWatchTask->GetReqTask();

        if( pReqTask.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        ret = CancelReqTask(
            pReqTask, pTask );

    }while( 0 );

    return ret;
}

gint32 CStreamServer::OnChannelError(
    HANDLE hChannel, gint32 iError )
{
    // we will close the channel by complete the
    // invoke task
    if( hChannel == 0 )
        return -EINVAL;

    gint32 ret = 0;
    TaskletPtr pTask;
    do{
        ret = GetWatchTask( hChannel, pTask );
        if( ERROR( ret ) )
            break;

        CIoWatchTaskServer* pTaskSvr = pTask;
        if( unlikely( pTaskSvr == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }

        TaskletPtr pInvTask =
            pTaskSvr->GetInvokeTask();

        if( unlikely( pInvTask.IsEmpty() ) )
        {
            ret = -EFAULT;
            break;
        }

        // complete this invoke task
        CParamList oParams;
        oParams[ propReturnValue ] = iError;

        // to avoid call to OnServiceComplete
        // in the opposite order of locking, that is
        // invoke's lock and watch's lock
        ObjPtr pObj( this );
        ret = DEFER_CALL(
            GetIoMgr(), pObj,
            &CStreamServer::CompleteInvokeTask,
            ( ( IConfigDb* )oParams.GetCfg() ),
            ( ( IEventSink* )pInvTask ) );

    }while( 0 );

    return ret;
}

gint32 CStreamServer::CanContinue()
{
    if( !IsConnected() )
        return ERROR_STATE;
    
    EnumClsid iid = iid( IStream );

    std::string strIfName =
        CoGetIfNameFromIid( iid );

    ToPublicName( strIfName );

    if( IsPaused( strIfName ) )
        return ERROR_STATE;

    return 0;
}

gint32 CStreamServer::OpenChannel(
    IConfigDb* pDataDesc,
    int& fd, HANDLE& hChannel,
    IEventSink* pCallback )
{
    if( fd < 0 || pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CParamList oParams;
        oParams[ propIfPtr ] = ObjPtr( this );
        oParams[ propIoMgr ] = ObjPtr( GetIoMgr() );

        oParams.Push( fd );
        oParams.Push( ObjPtr( pCallback ) );
        oParams.Push( ObjPtr( pDataDesc ) );

        TaskletPtr pTask;
        ret = pTask.NewObj(
            clsid( CIoWatchTaskServer ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        CObjBase* pWatchTask = pTask;
        hChannel = ( HANDLE )pWatchTask;
        if( true )
        {
            CStdRMutex oIfLock( GetLock() ); 
            m_mapWatches[ hChannel ] = pTask;
        }

        // insert this task to the completion chain of
        // pCallback
        ret = InterceptCallback( pTask, pCallback );
        if( ERROR( ret ) )
            break;

        // let's rock
        ret = ( *pTask )( eventZero );
        if( ret != STATUS_PENDING )
        {
            fd = -1;
            RemoveInterceptCallback(
                pTask, pCallback );
            break;
        }

    }while( 0 );

    if( ret != STATUS_PENDING )
    {
        if( fd >= 0 )
        {
            close( fd );
            fd = -1;
        }
    }

    return ret;
}

gint32 CStreamServer::FetchData_Server(
    IConfigDb* pDataDesc,           // [in]
    gint32& fd,                     // [out]
    guint32& dwOffset,              // [in, out]
    guint32& dwSize,                // [in, out]
    IEventSink* pCallback )
{
    HANDLE hChannel = 0;
    gint32 ret = OpenChannel( pDataDesc,
        fd, hChannel, pCallback );

    if( ret != STATUS_PENDING )
        return ret;

    OnConnected( hChannel );
    return ret;
}

gint32 CStreamServer::OnClose(
    HANDLE hChannel )
{
    // defer the CloseChannel call
    return DEFER_CALL(  GetIoMgr(), this,
        &CStreamServer::CloseChannel,
        hChannel );
}

gint32 CStreamServer::CloseChannel(
    HANDLE hChannel )
{
    gint32 ret = 0;
    do{
        TaskletPtr pTask;
        ret = GetWatchTask( hChannel, pTask );
        if( ERROR( ret ) )
            break;

        CIoWatchTaskServer* pSvrTask = pTask;
        if( pSvrTask == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        TaskletPtr pInvTask =
            pSvrTask->GetInvokeTask();

        if( pInvTask.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        // setup the response
        CParamList oParams;
        oParams[ propReturnValue ] = 0;
        CParamList oDesc;

        oDesc[ propIid ] = iid( IStream );
        // total bytes read
        oDesc.Push( pSvrTask->GetBytesRead() );
        // total bytes write
        oDesc.Push( pSvrTask->GetBytesWrite() );

        // data desc
        oParams.Push( ObjPtr( oDesc.GetCfg() ) );
        // fd
        oParams.Push( -1 );
        // offset
        oParams.Push( 0 );
        // size
        oParams.Push( 0 );

        ret = this->OnServiceComplete(
            oParams.GetCfg(), pInvTask );

    }while( 0 );

    return ret;
}

gint32 CIoWatchTaskServer::OnPingPong(
    bool bPing )
{
    gint32 ret = 0;
    do{
        ret = super::OnPingPong( bPing );
        if( ERROR( ret ) )
            break;

        SendPingPong( false );
        CIfInvokeMethodTask* pInvTask = m_pInvTask;

        if( unlikely( pInvTask == nullptr ) )
            break;
        // we have the risk to dead-lock, defer the
        // call out of this lock
        CRpcServices* pIf = dynamic_cast
            < CRpcServices* >( m_pStream );
        if( pIf != nullptr )
        {
            DEFER_CALL( pIf->GetIoMgr(), pInvTask,
                &CIfInvokeMethodTask::ResetTimer );
        }

    }while( 0 );

    return ret;
}
