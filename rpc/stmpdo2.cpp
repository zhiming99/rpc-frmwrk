/*
 * =====================================================================================
 *
 *       Filename:  stmpdo2.cpp
 *
 *    Description:  implementation of CTcpStreamPdo2 and CRpcConnSock
 *
 *        Version:  1.0
 *        Created:  11/09/2019 03:30:31 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
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
#include "tcpport.h"
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "reqopen.h"
#include <fcntl.h>
#include "ifhelper.h"
#include "tcportex.h"

CRpcConnSock::CRpcConnSock(
    const IConfigDb* pCfg )
    : super( pCfg )
{
    gint32 ret = 0;
    SetClassId( clsid( CRpcConnSock ) );
    do{
        CCfgOpener oCfg(
            ( IConfigDb* )m_pCfg );

        ret = oCfg.CopyProp(
            propIpAddr, pCfg );

        gint32 iFd = -1;
        ret = oCfg.GetIntProp(
            propFd, ( guint32& )iFd );

        // passive connection
        if( SUCCEEDED( ret ) )
        {
            m_iFd = iFd;
            oCfg.CopyProp(
                propSrcTcpPort, pCfg );
            ret = oCfg.CopyProp(
                propDestTcpPort, pCfg );
        }
        else
        {
            ret = oCfg.CopyProp(
                propDestTcpPort, pCfg );
        }

        if( ERROR( ret ) )
            break;

    }while( 0 );

    if( ERROR( ret ) )
    {
        std::string strMsg = DebugMsg(
            ret, "Error in CRpcConnSock ctor" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CRpcConnSock::ActiveConnect(
    const std::string& strIpAddr )
{
    gint32 ret = 0;
    addrinfo *res = nullptr;

    do{
        CCfgOpener oCfg(
            ( IConfigDb* )m_pCfg );

        guint32 dwPortNum = 0;
        ret = oCfg.GetIntProp(
            propDestTcpPort, dwPortNum );

        if( ERROR( ret ) )
            dwPortNum = RPC_SVR_PORTNUM;

        /*  Connect to the remote host. */
        ret = GetAddrInfo( strIpAddr,
            dwPortNum, res );

        if( ERROR( ret ) )
            break;

        ret = connect( m_iFd,
            res->ai_addr, res->ai_addrlen );

        if( ret == 0 )
            break;

        if( errno == EINPROGRESS )
        {
            // watching if the socket can write,
            // indicating the connect is done
            StartWatch();
            ret = STATUS_PENDING;
        }
        else
        {
            ret = -errno;
            break;
        }

    }while( 0 );

    if( res != nullptr )
    {
        freeaddrinfo( res );
        res = nullptr;
    }

    if( ret != STATUS_PENDING )
    {
        StopWatch();
        if( SUCCEEDED( ret ) )
            StartWatch( false );
    }

    return ret;
}
/**
* @name Connect(): to actively connect to a remote
* server with the specified ip addr
* @{ */
/**
 * Note that, this is an asynchronous connect, and
 * the notification comes from the event
 * OnConnected  @} */

gint32 CRpcConnSock::Connect()
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg(
            ( IConfigDb* )m_pCfg );

        std::string strIpAddr;
        ret = oCfg.GetStrProp(
            propIpAddr, strIpAddr );

        if( ERROR( ret ) )
            break;

        guint8 arrAddrBytes[ IPV6_ADDR_BYTES ];
        guint32 dwSize = sizeof( arrAddrBytes );
        ret = IpAddrToBytes( strIpAddr.c_str(),
            arrAddrBytes, dwSize );

        if( ERROR( ret ) )
            break;

        int iFamily = AF_INET;
        if( dwSize == IPV6_ADDR_BYTES )
            iFamily = AF_INET6;

        ret = socket( iFamily,
            SOCK_STREAM | SOCK_NONBLOCK, 0 );

        if ( ret == -1 )
        {
            ret = -errno;
            break;
        }

        m_iFd = ret;

        ret = AttachMainloop();
        if( ERROR( ret ) )
            break;

        /*  Connect to the remote host. */
        m_bClient = true;
        ret = ActiveConnect( strIpAddr );

    }while( 0 );

    if( ERROR( ret ) )
    {
        DetachMainloop();
        CloseSocket();
    }

    return ret;
}

gint32 CRpcConnSock::Start()
{
    gint32 ret = 0;

    do{
        CStdRMutex oSockLock( GetLock() );
        if( GetState() != sockInit )
            return ERROR_STATE;

        if( m_iFd == -1 )
        {
            // FIXME: do we need to retry if the
            // connect failed?
            //
            // we need an active connect
            ret = Connect();
            if( ret == STATUS_PENDING )
                break;

            if( ERROR( ret ) )
            {
                break;
            }
        }
        else
        {
            // we are already connected
            int iFlags = fcntl( m_iFd, F_GETFL );
            iFlags |= O_NONBLOCK;
            ret = fcntl( m_iFd, F_SETFL, iFlags );
            if( ret == -1 )
            {
                ret = -errno;
                break;
            }

            ret = AttachMainloop();
        }

    }while( 0 );

    return ret;
}
/**
* @name Start_bh
* the bottom half of the Start process
* @{ */
/**  @} */

gint32 CRpcConnSock::Start_bh()
{
    gint32 ret = 0;

    do{
        CStdRMutex oSockLock( GetLock() );
        if( GetState() != sockInit )
        {
            return ERROR_STATE;
        }

        if( GetState() != sockInit )
        {
            ret = ERROR_STATE;
            break;
        }

        // tcp-level keep alive
        ret = SetKeepAlive( true );
        if( ERROR( ret ) )
            break;

        ret = StartTimer();
        if( ERROR( ret ) )
            break;

        ret = StartWatch( false );
        SetState( sockStarted );
        
    }while( 0 );

    return ret;
}

gint32 CRpcConnSock::DispatchSockEvent(
     GIOCondition  dwCondition )
{
    gint32 ret = 0; 

    /* 
    +--------------------------------------------------------------------+
    |                            I/O events                              |
    +-----------+-----------+--------------------------------------------+
    |Event      | Poll flag | Occurrence                                 |
    +-----------+-----------+--------------------------------------------+
    |Read       | POLLIN    | New data arrived.                          |
    +-----------+-----------+--------------------------------------------+
    |Read       | POLLIN    | A connection setup has been completed (for |
    |           |           | connection-oriented sockets)               |
    +-----------+-----------+--------------------------------------------+
    |Read       | POLLHUP   | A disconnection request has been initiated |
    |           |           | by the other end.                          |
    +-----------+-----------+--------------------------------------------+
    |Read       | POLLHUP   | A  connection  is broken (only for connec- |
    |           |           | tion-oriented protocols).  When the socket |
    |           |           | is written SIGPIPE is also sent.           |
    +-----------+-----------+--------------------------------------------+
    |Write      | POLLOUT   | Socket  has  enough  send buffer space for |
    |           |           | writing new data.                          |
    +-----------+-----------+--------------------------------------------+
    |Read/Write | POLLIN |  | An outgoing connect(2) finished.           |
    |           | POLLOUT   |                                            |
    +-----------+-----------+--------------------------------------------+
    |Read/Write | POLLERR   | An asynchronous error occurred.            |
    +-----------+-----------+--------------------------------------------+
    |Read/Write | POLLHUP   | The other end has shut down one direction. |
    +-----------+-----------+--------------------------------------------+
    |Exception  | POLLPRI   | Urgent data arrived.  SIGURG is sent then. |
    +-----------+-----------+--------------------------------------------+
    */

    EnumSockState iState = sockInvalid;
    if( true )
    {
        CStdRMutex oSockLock( GetLock() );
        iState = GetState();
        if( iState == sockStopped )
            return ERROR_STATE;

        ret = GetAsyncErr();
    }

    if( ERROR( ret ) )
    {
        OnError( ret );
        return ret;
    }

    switch( iState )
    {
    case sockInit:
        {
            StopWatch();
            if( m_pStartTask.IsEmpty() )
            {
                ret = -EFAULT;
                break;
            }

            CStmSockConnectTask2*
                pConnTask = m_pStartTask;

            pConnTask->WaitInitDone();
            ret = m_pStartTask->OnEvent(
                eventNewConn,
                dwCondition,
                0, nullptr );

            break;
        }
    case sockStarted:
        {
            if( dwCondition & (
                G_IO_HUP | G_IO_ERR | G_IO_NVAL ) )
            {
                OnDisconnected();
            }
            else if( dwCondition & G_IO_IN )
            {
                m_dwAgeSec = 0;
                ret = OnReceive();
                if( ret == -ENOTCONN || 
                    ret == -EPIPE )
                    OnDisconnected();
            }
            else if( dwCondition & G_IO_OUT )
            {
                m_dwAgeSec = 0;
                ret = OnSendReady();
                if( ERROR( ret ) )
                    OnDisconnected();
            }
            else
            {
                ret = ERROR_STATE;
            }
            break;
        }
    default:
        {
            ret = ERROR_STATE;
            break;
        }
    }

    return ret;
}

gint32 CRpcConnSock::OnError(
    gint32 iError )
{
    gint32 ret = 0; 
    IrpPtr pIrp;

    do{
        if( GetState() == sockInit )
        {
            if( !m_pStartTask.IsEmpty() )
            {
                CStmSockConnectTask2*
                    pConnTask = m_pStartTask;
                pConnTask->WaitInitDone();
                ret = m_pStartTask->OnEvent(
                    eventConnErr, iError, 0, nullptr );
            }
            else
            {
                ret = iError;
            }
        }
        else if( GetState() == sockStarted )
        {
            CTcpStreamPdo2* pPort =
                ObjPtr( m_pParentPort );

            if( pPort == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            STREAM_SOCK_EVENT sse;
            sse.m_iEvent = sseError;
            sse.m_iData = iError;
            pPort->OnStmSockEvent( sse );
            ret = iError;
        }

    }while( 0 );
    
    return ret;
}

gint32 CRpcConnSock::OnReceive()
{
    gint32 ret = 0;
    IrpPtr pIrp;

    do{
        CTcpStreamPdo2* pPort =
            ObjPtr( m_pParentPort );

        if( pPort == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ret = pPort->OnReceive( m_iFd );

    }while( 0 );

    return ret;
}

gint32 CRpcConnSock::OnSendReady()
{
    gint32 ret = 0;
    do{
        CTcpStreamPdo2* pPort =
            ObjPtr( m_pParentPort );

        if( pPort == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ret = pPort->OnSendReady( m_iFd );

    }while( 0 );

    return ret;
}

// event dispatcher
gint32 CRpcConnSock::OnEvent(
    EnumEventId iEvent,
    LONGWORD dwParam1,
    LONGWORD dwParam2,
    LONGWORD* pData )
{
    gint32 ret = 0;
    switch( iEvent )
    {
    case eventCancelTask:
        {
            // cancel the start task
            if( m_pStartTask.IsEmpty() )
            {
                ret = ERROR_STATE;
                break;
            }
            ret = m_pStartTask->OnEvent( iEvent,
                dwParam1, dwParam2, pData );
            break;
        }
    case eventStop:
        {
            EnumSockState iState;

            CStdRMutex oSockLock( GetLock() );
            iState = GetState();
            if( iState == sockInit )
            {

                TaskletPtr pTask = m_pStartTask;
                oSockLock.Unlock();
                // we could be here if the start
                // irp is canceled or timed out.
                if( !pTask.IsEmpty() )
                {
                    pTask->OnEvent( iEvent,
                        dwParam1, dwParam2, pData );
                }
                ret = Stop();
            }
            else if( iState == sockStarted )
            {
                oSockLock.Unlock();
                ret = Stop();
            }
            else if( iState == sockStopped ) 
            {
                ret = 0;
            }
            else
            {
                ret = ERROR_STATE;
            }

            break;
        }
    case eventStart:
        {
            ret = CStmSockConnectTask2::NewTask(
                GetIoMgr(),
                this,
                TASKLET_RETRY_TIMES,
                m_pStartTask,
                clsid( CStmSockConnectTask2 ) );

            if( ERROR( ret ) )
                break;

            ret = m_pStartTask->OnEvent( iEvent,
                dwParam1, 0, nullptr );

            if( ERROR( ret ) ) 
                break;
                
            CStmSockConnectTask2*
                pConnTask = m_pStartTask;

            // NOTE: SetInitDone does not mean the
            // connection is done. It just
            // notifies the reentrancy is allowed
            // to the `connection task'. Possibly
            // the socket's initialization could
            // have been waiting to call
            // eventNewConn on this task before
            // OnEvent returns. The sync here
            // serves to prevent the task's
            // propParamList overwritten due to
            // reentrancy.
            pConnTask->SetInitDone();
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

gint32 CRpcConnSock::OnDisconnected()
{
    Stop();
    gint32 ret = 0;
    do{
        CTcpStreamPdo2* pPort =
            ObjPtr( m_pParentPort );

        if( pPort == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        pPort->OnDisconnected();

    }while( 0 );

    return ret;
}

gint32 CRpcConnSock::Stop()
{
    gint32 ret = super::Stop();
    m_pStartTask.Clear();
    return ret;
}

CTcpStreamPdo2::CTcpStreamPdo2(
    const IConfigDb* pCfg ) :
    super( pCfg ),
    m_oSender( this )
{
    SetClassId( clsid( CTcpStreamPdo2 ) );
    m_dwFlags &= ~PORTFLG_TYPE_MASK;
    m_dwFlags |= PORTFLG_TYPE_PDO;

    CCfgOpener oCfg( pCfg );
    gint32 ret = 0;

    do{
        ret = oCfg.GetPointer(
            propBusPortPtr, m_pBusPort );

        if( ERROR( ret ) )
            break;

    }while( 0 );

    if( ERROR( ret ) )
    {
        throw std::runtime_error(
            "Error in CTcpStreamPdo2 ctor" );
    }
}

CTcpStreamPdo2::~CTcpStreamPdo2()
{;}

gint32 CTcpStreamPdo2::OnDisconnected()
{
    STREAM_SOCK_EVENT sse;
    sse.m_iEvent = sseError;
    sse.m_iData = -ENOTCONN;

    return OnStmSockEvent( sse );
}

gint32 CTcpStreamPdo2::OnStmSockEvent(
    STREAM_SOCK_EVENT& sse )
{
    gint32 ret = 0;
    do{
        CStdRMutex oPortLock( GetLock() );
        if( m_queListeningIrps.empty() )
        {
            m_queEvtToRecv.push_back( sse );
        }
        else
        {
            IrpPtr pIrp =
                m_queListeningIrps.front();
            m_queListeningIrps.pop_front();
            oPortLock.Unlock();

            CStdRMutex oIrpLock(
                pIrp->GetLock() );

            ret = pIrp->CanContinue(
                IRP_STATE_READY );

            if( ERROR( ret ) )
            {
                ret = 0;
                continue;
            }

            BufPtr pRespBuf( true );
            pRespBuf->Resize( sizeof( sse ) );

            memcpy( pRespBuf->ptr(),
                &sse, sizeof( sse ) );

            IrpCtxPtr pCtx = pIrp->GetTopStack();
            pCtx->SetRespData( pRespBuf );
            pCtx->SetStatus( 0 );

            GetIoMgr()->CompleteIrp( pIrp );
        }

        break;

    }while( 1 );

    return ret;
}

#include <sys/ioctl.h>
gint32 CTcpStreamPdo2::OnReceive(
    gint32 iFd )
{
    if( iFd < 0 )
        return -EINVAL;

    gint32 ret = 0;
    bool bFirst = true;
    do{
        STREAM_SOCK_EVENT sse;
        sse.m_iEvent = sseRetWithBuf;
        guint32 dwBytes = 0;
        ret = ioctl( iFd, FIONREAD, &dwBytes );
        if( ret == -1 )
        {
            ret = -errno;
            break;
        }
        if( dwBytes == 0 && bFirst )
        {
            ret = -ENOTCONN;
            break;
        }
        else if( dwBytes == 0 )
        {
            ret = 0;
            break;
        }
        
        if( dwBytes > MAX_BYTES_PER_TRANSFER )
            dwBytes = MAX_BYTES_PER_TRANSFER;

        bFirst = false;
        BufPtr pInBuf( true );
        pInBuf->Resize( dwBytes );
        if( ERROR( ret ) )
            break;

        ret = read( iFd, pInBuf->ptr(), dwBytes );
        if( ret == -1 && RETRIABLE( errno ) )
        {
            ret = 0;
            break;
        }
        else if( ret == -1 )
        {
            ret = -errno;
            break;
        }
        else if( ret != ( gint32 )dwBytes )
        {
            DebugPrint( 0, "Surprise!!!, the bytes"\
            "returned is not the same as expected" );
        }
        else
        {
            ret = 0;
        }

        sse.m_pInBuf = pInBuf;
        
        CStdRMutex oPortLock( GetLock() );
        m_queEvtToRecv.push_back( sse );
        if( !m_queListeningIrps.empty() )
        {
            IrpPtr pIrp =
                m_queListeningIrps.front();

            oPortLock.Unlock();

            CStdRMutex oIrpLock( pIrp->GetLock() );
            oPortLock.Lock();

            if( m_queListeningIrps.empty() ||
                m_queEvtToRecv.empty() )
                continue;

            IrpPtr& pIrp2 =
                m_queListeningIrps.front();

            // some other guys is working on
            // this queue
            if( !( pIrp2 == pIrp ) )
                continue;

            m_queListeningIrps.pop_front();

            ret = pIrp->CanContinue(
                IRP_STATE_READY );

            if( ERROR( ret ) )
                continue;

            BufPtr pRespBuf( true );
            pRespBuf->Resize( sizeof( sse ) );
            void* pBlob = pRespBuf->ptr();

            new ( pBlob ) STREAM_SOCK_EVENT( 
                m_queEvtToRecv.front() );
            m_queEvtToRecv.pop_front();

            oPortLock.Unlock();

            IrpCtxPtr pCtx = pIrp->GetTopStack();
            pCtx->SetRespData( pRespBuf );
            pCtx->SetStatus( 0 );

            GetIoMgr()->CompleteIrp( pIrp );
            continue;
        }

        break;

    }while( 1 );

    return ret;
}

gint32 CBytesSender::SetSendDone(
    gint32 iRet )
{
    CTcpStreamPdo2* pPdo = m_pPort;
    if( pPdo == nullptr )
        return -EFAULT;

    CStdRMutex oPortLock( pPdo->GetLock() );
    if( m_pIrp.IsEmpty() )
        return 0;
    IrpPtr pIrp = m_pIrp;
    m_pIrp.Clear();
    m_iBufIdx = -1;
    m_dwOffset = 0;
    oPortLock.Unlock();

    if( pIrp.IsEmpty() ||
        pIrp->GetStackSize() == 0 )
        return 0;

    CStdRMutex oIrpLock( pIrp->GetLock() );

    gint32 ret = pIrp->CanContinue(
        IRP_STATE_READY );

    if( ERROR( ret ) )
        return 0;

    IrpCtxPtr& pCtx = pIrp->GetTopStack();
    pCtx->SetStatus( iRet );
    CIoManager* pMgr = pPdo->GetIoMgr();
    pMgr->CompleteIrp( pIrp );

    return 0;
}

gint32 CBytesSender::CancelSend(
    const bool& bCancelIrp )
{
    CTcpStreamPdo2* pPdo = m_pPort;
    if( pPdo == nullptr )
        return -EFAULT;

    CStdRMutex oPortLock( pPdo->GetLock() );
    if( m_pIrp.IsEmpty() )
        return -ENOENT;

    IrpPtr pIrp = m_pIrp;
    m_pIrp.Clear();
    m_iBufIdx = -1;
    m_dwOffset = 0;
    oPortLock.Unlock();

    if( !bCancelIrp )
        return 0;

    CIoManager* pMgr = pPdo->GetIoMgr();
    CStdRMutex oIrpLock( pIrp->GetLock() );
    gint32 ret = pIrp->CanContinue(
        IRP_STATE_READY );

    if( ERROR( ret ) )
        return ret;

    IrpCtxPtr& pCtx = pIrp->GetTopStack();
    pCtx->SetStatus( ERROR_CANCEL );
    pMgr->CompleteIrp( pIrp );

    return 0;
}

#define STMPDO2_MAX_IDX_PER_REQ 1024

gint32 CBytesSender::OnSendReady(
    gint32 iFd )
{
    if( iFd < 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        IrpPtr pIrp;
        CPort* pPort = m_pPort;
        if( unlikely( pPort == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }
        CStdRMutex oPortLock( pPort->GetLock() );
        if( m_pIrp.IsEmpty() )
        {
            ret = ERROR_NOT_HANDLED;
            break;
        }
        pIrp = m_pIrp;
        oPortLock.Unlock();

        CStdRMutex oIrpLock( pIrp->GetLock() );
        oPortLock.Lock();
        if( !( pIrp == m_pIrp ) )
            continue;

        ret = m_pIrp->CanContinue(
            IRP_STATE_READY );

        if( ERROR( ret ) )
            break;
            
        IrpCtxPtr& pCtx = m_pIrp->GetTopStack();
        CfgPtr pCfg;
        ret = pCtx->GetReqAsCfg( pCfg );
        if( ERROR( ret ) )
        {
            BufPtr pPayload = pCtx->m_pReqData;
            if( pPayload.IsEmpty() ||
                pPayload->empty() )
            {
                ret = -EFAULT;
                break;
            }

            // a single buffer to send
            CParamList oParams;
            oParams.Push( pPayload );
            pCfg = oParams.GetCfg();
        }

        CParamList oParams( pCfg );
        gint32 iCount = 0;
        guint32& dwCount = ( guint32& )iCount;
        ret = oParams.GetSize( dwCount );
        if( ERROR( ret ) )
            break;

        if( unlikely( dwCount == 0  ||
            dwCount > STMPDO2_MAX_IDX_PER_REQ ) )
        {
            ret = -EINVAL;
            break;
        }

        if( m_iBufIdx >= iCount )
        {
            // send done
            break;
        }

        BufPtr pBuf;
        ret = oParams.GetProperty(
            m_iBufIdx, pBuf );

        if( ERROR( ret ) )
            break;

        if( unlikely( m_dwOffset >=
            pBuf->size() ) )
        {
            ret = -ERANGE;
            break;
        }

        do{
            guint32 dwMaxBytes =
                MAX_BYTES_PER_TRANSFER;

            guint32 dwSize = std::min(
                pBuf->size() - m_dwOffset,
                dwMaxBytes );

            ret = SendBytesNoSig( iFd,
                pBuf->ptr() + m_dwOffset,
                dwSize );

            if( ret == -1 && RETRIABLE( errno ) )
            {
                ret = STATUS_PENDING;
                break;
            }
            else if( ret == -1 )
            {
                ret = -errno;
                break;
            }

            guint32 dwMaxSize = pBuf->size();
            guint32 dwNewOff = m_dwOffset + ret;

            ret = 0;
            if( dwMaxSize == dwNewOff )
            {
                // done with this buf
                m_dwOffset = 0;
                ++m_iBufIdx;

                if( m_iBufIdx == iCount ) 
                {
                    break;
                }
                else if( unlikely(
                    m_iBufIdx > iCount ) )
                {
                    ret = -EOVERFLOW;
                    break;
                }

                ret = oParams.GetProperty(
                    m_iBufIdx, pBuf );

                if( ERROR( ret ) )
                    break;

                continue;
            }
            else if( dwMaxSize < dwNewOff )
            {
                ret = -EOVERFLOW;
                break;
            }

            m_dwOffset = dwNewOff;

        }while( 1 );

    }while( 0 );

    if( ret != STATUS_PENDING )
        SetSendDone( ret );

    return ret;
}

bool CBytesSender::IsSendDone() const
{
    if( m_pIrp.IsEmpty() )
        return true;

    return false;
}

gint32 CTcpStreamPdo2::OnSendReady(
    gint32 iFd )
{
    if( iFd < 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        ret = m_oSender.OnSendReady( iFd );
        CStdRMutex oPortLock( GetLock() );
        CRpcConnSock* pSock = m_pConnSock;

        if( SUCCEEDED( ret ) ||
            ret == ERROR_NOT_HANDLED )
        {
            ret = 0;
            if( m_queWriteIrps.empty() )
            {
                if( m_oSender.IsSendDone() )
                {
                    pSock->StopWatch();
                    break;
                }
            }
            else if( m_oSender.IsSendDone() )
            {
                IrpPtr pIrp =
                    m_queWriteIrps.front();
                m_oSender.SetIrpToSend( pIrp );
                m_queWriteIrps.pop_front();
            }
            continue;
        }
        else if( ret == STATUS_PENDING )
        {
            break;
        }

        if( ERROR( ret ) )
        {
            pSock->StopWatch();
            break;
        }

    }while( 1 );

    return ret;
}

gint32 CTcpStreamPdo2::PostStart(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;

    do{
        CParamList oParams;

        ret = oParams.CopyProp(
            propIpAddr, this );

        if( ERROR( ret ) )
            break;

        oParams.CopyProp(
            propSrcTcpPort, this );

        ret = oParams.CopyProp(
            propDestTcpPort, this );

        if( ERROR( ret ) )
            break;

        ret = oParams.CopyProp(
            propFd, this );

        oParams.SetPointer(
            propIoMgr, GetIoMgr() );

        oParams.SetObjPtr(
            propPortPtr, ObjPtr( this ) );

        ret = m_pConnSock.NewObj(
            clsid( CRpcConnSock ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        CRpcConnSock* pSock = m_pConnSock;
        ret = pSock->OnEvent( eventStart,
            ( LONGWORD )pIrp, 0, nullptr );

    }while( 0 );

    return ret;
}

gint32 CTcpStreamPdo2::RemoveIrpFromMap(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CStdRMutex oPortLock( GetLock() );
        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        guint32 dwMajor = pCtx->GetMajorCmd();
        guint32 dwMinor = pCtx->GetMinorCmd();
        guint32 dwCtrlCode = pCtx->GetCtrlCode();
        if( dwMajor != IRP_MJ_FUNC )
        {
            ret = -EINVAL;
            break;
        }
        if( dwMinor == IRP_MN_WRITE )
        {
            bool bErased = false;

            IrpPtr pIrpInProcess =
                m_oSender.GetIrp();

            if( pIrp == pIrpInProcess )
            {
                bErased = true;
                m_oSender.CancelSend( false );
            }

            if( bErased )
                break;

            std::deque< IrpPtr >::iterator itr =
                m_queWriteIrps.begin();

            while( itr != m_queWriteIrps.end() )
            {
                if( pIrp == *itr )
                {
                    bErased = true;
                    m_queWriteIrps.erase( itr );
                    break;
                }
                ++itr;
            }
            break;
        }
        else if( dwMinor == IRP_MN_IOCTL &&
            dwCtrlCode == CTRLCODE_LISTENING )
        {
            bool bErased = false;
            std::deque< IrpPtr >::iterator itr =
                m_queListeningIrps.begin();

            while( itr != m_queListeningIrps.end() )
            {
                if( pIrp == *itr )
                {
                    bErased = true;
                    m_queListeningIrps.erase( itr );
                    break;
                }
                ++itr;
            }
            if( !bErased )
                ret = -ENOENT;

            break;
        }
        else
        {
            ret = -EINVAL;
        }

    }while( 0 );

    return ret;
}

gint32 CTcpStreamPdo2::CancelFuncIrp(
    IRP* pIrp, bool bForce )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        ret = RemoveIrpFromMap( pIrp );

    }while( 0 );

    if( SUCCEEDED( ret ) )
    {
        IrpCtxPtr& pCtx = pIrp->GetCurCtx();
        pCtx->SetStatus( ERROR_CANCEL );
        super::CancelFuncIrp( pIrp, bForce );
    }

    return ret;
}

gint32 CTcpStreamPdo2::PreStop(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CIoManager* pMgr = GetIoMgr();
        guint32 dwStepNo = 0;
        GetPreStopStep( pIrp, dwStepNo );
        while( dwStepNo == 0 )
        {
            SockPtr pSock;
            CStdRMutex oSockLock(
                m_pConnSock->GetLock() );

            EnumSockState iState =
                m_pConnSock->GetState();

            if( iState == sockStopped )
            {
                SetPreStopStep( pIrp, 1 );
                break;
            }

            pSock = m_pConnSock;
            if( pSock.IsEmpty() )
            {
                SetPreStopStep( pIrp, 1 );
                break;
            }

            if( pMgr->RunningOnMainThread() )
            {
                // we need to run on the mainloop
                m_pConnSock.Clear();
                oSockLock.Unlock();
                SetPreStopStep( pIrp, 1 );
                ret = pSock->OnEvent(
                    eventStop, 0, 0, nullptr );
            }
            else
            {
                TaskletPtr pTask;
                DEFER_CALL_NOSCHED(
                    pTask, ObjPtr( pMgr ),
                    &CIoManager::CompleteIrp,
                    pIrp );

                ret = pMgr->RescheduleTaskMainLoop( pTask );
                if( SUCCEEDED( ret ) )
                    ret = STATUS_PENDING;
            }
            break;
        }

        // don't move away from PreStop
        if( ret == STATUS_PENDING )
            ret = STATUS_MORE_PROCESS_NEEDED;

        if( ret == STATUS_MORE_PROCESS_NEEDED )
            break;

        GetPreStopStep( pIrp, dwStepNo );
        if( dwStepNo == 1 )
        {
            SetPreStopStep( pIrp, 2 );
            ret = super::PreStop( pIrp );
        }

        if( ret == STATUS_PENDING ||
            ret == STATUS_MORE_PROCESS_NEEDED )
            break;

        GetPreStopStep( pIrp, dwStepNo );
        if( dwStepNo == 2 )
        {
            CCfgOpenerObj oPortCfg( this );

            guint32 dwPortId = 0;
            oPortCfg.GetIntProp(
                propPortId, dwPortId );

            DebugPrint( ret,
                "CTcpStreamPdo2 PreStop, portid=%d",
                dwPortId );
            break;
        }

    }while( 0 );

    return ret;
}
gint32 CTcpStreamPdo2::Stop(
    IRP* pIrp )
{
    CancelAllIrps( ERROR_PORT_STOPPED );
    m_oSender.Clear();
    return super::Stop( pIrp );
}
gint32 CTcpStreamPdo2::OnSubmitIrp(
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
                case CTRLCODE_IS_CLIENT:
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
    {
        pIrp->GetCurCtx()->SetStatus( ret );
    }

    return ret;
}

gint32 CTcpStreamPdo2::SubmitIoctlCmd(
    IRP* pIrpLocked )
{
    if( pIrpLocked == nullptr ||
        pIrpLocked->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    switch( pIrpLocked->CtrlCode() )
    {
    case CTRLCODE_LISTENING:
        {   
            ret = SubmitListeningCmd(
                pIrpLocked );
            break;
        }
    case CTRLCODE_IS_CLIENT:
        {
            CRpcConnSock* pSock = m_pConnSock;
            if( pSock == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            if( pSock->IsClient() )
            {
                ret = 0;
            }
            else
            {
                ret = ERROR_FALSE;
            }
            break;
        }
    default:
        {
            ret = -EINVAL;
            break;
        }
    }

    return ret;
}

gint32 CTcpStreamPdo2::SubmitListeningCmd(
    IRP* pIrpLocked )
{
    if( pIrpLocked == nullptr ||
        pIrpLocked->GetStackSize() == 0 )
        return -EINVAL;

    using ElemType = std::pair< IrpPtr, BufPtr >;
    using ElemType2 = CStlIrpVector2::ElemType;

    IrpVec2Ptr pIrpVec( true );
    std::vector< CStlIrpVector2::ElemType >&
        vecIrps2 = ( *pIrpVec )();
    std::vector< ElemType > vecIrps;

    gint32 ret = 0;
    bool bCompleted = false;

    IrpCtxPtr& pCtx =
        pIrpLocked->GetCurCtx();

    do{
        CStdRMutex oPortLock( GetLock() );
        if( pCtx->GetCtrlCode() !=
            CTRLCODE_LISTENING )
            break;

        m_queListeningIrps.push_back(
            pIrpLocked );

        if( m_queEvtToRecv.empty() )
        {
            ret = STATUS_PENDING;
            break;
        }

        while( !m_queEvtToRecv.empty() &&
            !m_queListeningIrps.empty() )
        {
            STREAM_SOCK_EVENT& sse =
                m_queEvtToRecv.front();
            BufPtr pBuf( true );
            pBuf->Resize( sizeof( sse ) );

            void* ptr = pBuf->ptr();
            new ( ptr ) STREAM_SOCK_EVENT( sse );

            IrpPtr pIrp =
                m_queListeningIrps.front();

            vecIrps.push_back(
                ElemType( pIrp, pBuf ) );

            if( pIrpLocked == pIrp )
                bCompleted = true;

            sse.m_pInBuf.Clear();
            m_queListeningIrps.pop_front();
            m_queEvtToRecv.pop_front();
        }

    }while( 0 );

    if( ret == STATUS_PENDING )
        return ret;

    if( vecIrps.empty() )
        return ret;

    if( vecIrps.size() == 1 && bCompleted )
    {
        pCtx->SetRespData(
            vecIrps.front().second );
        pCtx->SetStatus( ret );
    }
    else
    {
        for( auto elem : vecIrps )
        {
            CStdRMutex oLock(
                elem.first->GetLock() );

            if( elem.first->GetStackSize() == 0 )
                continue;

            gint32 iRet = elem.first->
                CanContinue( IRP_STATE_READY );

            if( ERROR( iRet ) )
            {
                // bugbug: the event is discarded
                // as the irp is canceled. the
                // events afterwards are not valid
                continue;
            }

            IrpCtxPtr& pTopCtx =
                elem.first->GetTopStack();

            pTopCtx->SetRespData(
                elem.second );

            pTopCtx->SetStatus( 0 );

            vecIrps2.push_back( ElemType2(
                0, elem.first ) );
        }

        if( !vecIrps2.empty() )
        {
            ret = ScheduleCompleteIrpTask(
                GetIoMgr(), pIrpVec, false );
            if( SUCCEEDED( ret ) )
            {
                ret = STATUS_PENDING;
            }
            else
            {
                pCtx->SetStatus( ret );
            }
        }
        else
        {
            ret = ERROR_FAIL;
            pCtx->SetStatus( ret );
        }
    }

    return ret;
}

gint32 CTcpStreamPdo2::SubmitWriteIrp(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    return StartSend( pIrp );
}

gint32 CTcpStreamPdo2::StartSend(
    IRP* pIrpLocked  )
{
    if( pIrpLocked == nullptr ||
        pIrpLocked->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        IrpCtxPtr& pCtx = pIrpLocked->GetCurCtx();
        if( pCtx->GetMajorCmd() != IRP_MJ_FUNC &&
            pCtx->GetMinorCmd() != IRP_MN_WRITE )
            break;

        CStdRMutex oPortLock( GetLock() );
        m_queWriteIrps.push_back( pIrpLocked );

        CRpcConnSock* pSock = m_pConnSock;
        if( unlikely( pSock == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }

        if( pIrpLocked == m_queWriteIrps.front() )
        {
            if( m_oSender.IsSendDone() )
            {
                // start sending immediately
                m_oSender.SetIrpToSend(
                    m_queWriteIrps.front() );
                m_queWriteIrps.pop_front();
                oPortLock.Unlock();
                ret = pSock->OnSendReady();
                if( ret != STATUS_PENDING )
                    break;
            }
        }

        pSock->StartWatch();
        ret = STATUS_PENDING;

    }while( 0 );

    if( ERROR( ret ) )
    {
        // we need to remove the irp from
        // the outgoing queue
        RemoveIrpFromMap( pIrpLocked );
    }

    return ret;
}

gint32 CTcpStreamPdo2::AllocIrpCtxExt(
    IrpCtxPtr& pIrpCtx,
    void* pContext ) const
{
    gint32 ret = 0;
    switch( pIrpCtx->GetMajorCmd() )
    {
    case IRP_MJ_PNP:
        {
            switch( pIrpCtx->GetMinorCmd() )
            {
            case IRP_MN_PNP_STOP:
                {
                    BufPtr pBuf( true );
                    if( ERROR( ret ) )
                        break;

                    pBuf->Resize( sizeof( guint32 ) +
                        sizeof( PORT_START_STOP_EXT ) );

                    memset( pBuf->ptr(),
                        0, pBuf->size() ); 

                    pIrpCtx->SetExtBuf( pBuf );
                    break;
                }
            default:
                ret = -ENOTSUP;
                break;
            }
            break;
        }
    default:
        {
            ret = -ENOTSUP;
            break;
        }
    }

    if( ret == -ENOTSUP )
    {
        // let's try it on CPort
        ret = super::AllocIrpCtxExt(
            pIrpCtx, pContext );
    }

    return ret;
}

gint32 CTcpStreamPdo2::CompleteIoctlIrp(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;

    IrpCtxPtr pCtx = pIrp->GetCurCtx();
    guint32 dwCtrlCode = pIrp->CtrlCode();
    switch( dwCtrlCode )
    {
    case CTRLCODE_LISTENING:
        {
            BufPtr& pBuf = pCtx->m_pRespData;
            if( pBuf.IsEmpty() || pBuf->empty() )
            {
                ret = -EBADMSG;
                pCtx->SetStatus( ret );
            }
            else
            {
                STREAM_SOCK_EVENT* psse =
                ( STREAM_SOCK_EVENT* )pBuf->ptr();
                if( psse->m_iEvent != sseRetWithBuf )
                    break;

                if( psse->m_pInBuf.IsEmpty() )
                {
                    ret = -EBADMSG;
                    pCtx->SetStatus( ret );
                }
            }
            break;
        }
    default:
        {
            ret = -ENOTSUP;
        }
    }

    return ret;
}

gint32 CTcpStreamPdo2::CancelAllIrps(
    gint32 iErrno )
{
    gint32 ret = 0;

    CStdRMutex oPortLock( GetLock() );
    IrpVecPtr pIrpVec( true );
    CParamList oParams;
    std::vector< IrpPtr >& vecIrps =
        ( *pIrpVec )();

    for( auto&& elem : m_queWriteIrps )
        vecIrps.push_back( elem );

    m_queWriteIrps.clear();

    for( auto&& elem : m_queListeningIrps )
        vecIrps.push_back( elem );
    m_queListeningIrps.clear();

    CIoManager* pMgr = GetIoMgr();
    oPortLock.Unlock();

    m_oSender.SetSendDone( iErrno );

    while( !vecIrps.empty() )
    {
        ret = oParams.Push(
            *( guint32* )&iErrno );

        if( ERROR( ret ) )
            break;

        ObjPtr pObj;

        if( !vecIrps.empty() )
        {
            pObj = pIrpVec;
            ret = oParams.Push( pObj );
            if( ERROR( ret ) )
                break;
        }

        oParams.SetPointer( propIoMgr, pMgr );

        // NOTE: this is a double check of the
        // remaining IRPs before the STOP begins.
  
        TaskletPtr pTask;
        pTask.NewObj(
            clsid( CCancelIrpsTask ),
            oParams.GetCfg() );

        // NOTE: Assumes there is no pending
        // completion. And the irp's callback
        // is still yet to be called when this
        // method returns.
        ( *pTask )( eventZero );
        ret = pTask->GetError();
        break;
    }

    return ret;
}
