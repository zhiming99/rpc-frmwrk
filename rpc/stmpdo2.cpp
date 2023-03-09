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
#include <sys/ioctl.h>
#include "reqopen.h"
#include <fcntl.h>
#include "ifhelper.h"
#include "tcportex.h"

namespace rpcf
{

inline gint32 SendBytesWithFlag( int iFd,
    void* pBuf, guint32 dwSize,
    int dwFlags = 0 )
{
    dwFlags |= MSG_NOSIGNAL;
    return send( iFd, pBuf, dwSize, dwFlags );
}

CRpcConnSock::CRpcConnSock(
    const IConfigDb* pCfg )
    : super( pCfg )
{
    gint32 ret = 0;
    SetClassId( clsid( CRpcConnSock ) );
    do{
        CCfgOpener oCfg(
            ( IConfigDb* )m_pCfg );

        gint32 iFd = -1;
        ret = oCfg.GetIntProp(
            propFd, ( guint32& )iFd );

        // passive connection
        if( SUCCEEDED( ret ) )
            m_iFd = iFd;

        ret = 0;

    }while( 0 );

    if( ERROR( ret ) )
    {
        std::string strMsg = DebugMsg(
            ret, "Error in CRpcConnSock ctor" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CRpcConnSock::ActiveConnect()
{
    gint32 ret = 0;
    addrinfo *res = nullptr;

    do{
        CCfgOpener oCfg(
            ( IConfigDb* )m_pCfg );

        IConfigDb* pConnParams = nullptr;
        ret = oCfg.GetPointer(
            propConnParams, pConnParams );
        if( ERROR( ret ) )
            break;

        CConnParams oConnParams( pConnParams );
        std::string strIpAddr =
            oConnParams.GetDestIpAddr();

        guint32 dwPortNum =
            oConnParams.GetDestPortNum();

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

        IConfigDb* pConnParams = nullptr;
        ret = oCfg.GetPointer(
            propConnParams, pConnParams );
        if( ERROR( ret ) )
            break;

        CConnParams oConnParams( pConnParams );
        std::string strIpAddr =
            oConnParams.GetDestIpAddr();

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
        ret = ActiveConnect();

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

gint32 CRpcConnSock::OnConnected()
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg(
            ( IConfigDb* )m_pCfg );

        IConfigDb* pConnParams = nullptr;
        ret = oCfg.GetPointer(
            propConnParams, pConnParams );
        if( ERROR( ret ) )
            break;

        CConnParams oConnParams( pConnParams );
        if( oConnParams.IsServer() )
            break;

        sockaddr_in6 oAddr;
        socklen_t iSize = sizeof( oAddr );
        ret = getsockname( m_iFd,
            ( sockaddr* )&oAddr, &iSize );

        if( ret == -1 )
        {
            ret = -errno;
            break;
        }

        char szNode[ 32 ];
        char szServ[ 8 ];

        ret = getnameinfo( ( sockaddr* )&oAddr,
            iSize, szNode, sizeof( szNode ),
            szServ, sizeof( szServ ),
            NI_NUMERICHOST | NI_NUMERICSERV );

        if( ret != 0 )
            break;
            
        std::string strSrcIp = szNode;
        if( strSrcIp.empty() )
        {
            ret = -EFAULT;
            break;
        }

        guint32 dwSrcPortNum =
            std::stoi( szServ );

        // fill the abscent the connection parameters
        oConnParams.SetSrcIpAddr( strSrcIp );
        oConnParams.SetSrcPortNum( dwSrcPortNum );

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
        if( ERROR( ret ) )
            break;

        ret = this->OnConnected();
        if( ERROR( ret ) )
            break;

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
                ret = -ENOTCONN;
            }
            else if( dwCondition & G_IO_IN )
            {
                ret = OnReceive();
                if( ret == -ENOTCONN || 
                    ret == -EPIPE )
                    OnDisconnected();
            }
            else if( dwCondition & G_IO_OUT )
            {
                ret = OnSendReady();
                if( ERROR( ret ) )
                    OnDisconnected();
            }
            else
            {
                ret = ERROR_STATE;
            }

            if( SUCCEEDED( ret ) )
            {
                m_dwAgeSec.store( 0 );
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
            sse.m_iEvtSrc = GetClsid();
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
    case eventTimeout:
        {
            CStdRMutex oSockLock( GetLock() );
            EnumSockState iState = GetState();
            if( iState == sockStopped )
            {
                ret = ERROR_STATE;
                break;
            }

            if( dwParam1 == eventAgeSec )
            {
                guint32 dwRet = m_dwAgeSec.
                    fetch_add( SOCK_TIMER_SEC );
                if( dwRet + SOCK_TIMER_SEC <
                    SOCK_LIFE_SEC )
                {
                    ret = StartTimer();
                    if( ret > 0 )
                    {
                        m_iTimerId = ret;
                        ret = STATUS_SUCCESS;
                    }
                    else
                    {
                        m_iTimerId = 0;
                    }
                    break;
                }

                oSockLock.Unlock();
                // NOTE: we don't call
                // OnDisconnected because the
                // connection will down after that
                // call
                CTcpStreamPdo2* pPort =
                    ObjPtr( m_pParentPort );
                if( pPort != nullptr )
                    ret = pPort->OnDisconnected();
            }
            else
            {
                ret = -ENOTSUP;
            }

            break;
        }
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
        // Sock's Stop must go after the reference
        // of m_pParentPort, since there is a race
        // condition between sock's Stop and the
        // port's PreStop. if it runs ahead of
        // pPort->OnDisconnected, the
        // m_pParentPort could be released and
        // become invalid, and the sock's later
        // call of pPort->OnDisconnected will
        // crash.
        Stop();

    }while( 0 );

    return ret;
}

gint32 CRpcConnSock::Stop()
{
    CStdRMutex oSockLock( GetLock() );
    gint32 ret = super::Stop();
    m_pStartTask.Clear();
    m_pParentPort = nullptr;

    return ret;
}

CMainIoLoop* CRpcConnSock::GetMainLoop() const
{
    CTcpStreamPdo2* pPort =
        ObjPtr( m_pParentPort );
    return pPort->GetMainLoop();
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
    sse.m_iEvtSrc = GetClsid();

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

            oIrpLock.Unlock();
            GetIoMgr()->CompleteIrp( pIrp );
        }

        break;

    }while( 1 );

    return ret;
}

gint32 CTcpStreamPdo2::OnReceive(
    gint32 iFd )
{
    if( iFd < 0 )
        return -EINVAL;

    gint32 ret = 0;
    bool bFirst = true;
    // DebugPrint( 0, "probe: recv tcp msg" );
    do{
        STREAM_SOCK_EVENT sse;
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
            if( errno == ENOTCONN ||
                errno == ECONNRESET ||
                errno == EIO ||
                errno == ENXIO )
                ret = -ENOTCONN;
            else
                ret = -errno;
            break;
        }
        else if( ret != ( gint32 )dwBytes )
        {
            DebugPrint( 0, "Surprise!!!, the #bytes"
            "received is not as expected" );
        }
        else
        {
            ret = 0;
        }

        sse.m_pInBuf = pInBuf;
        sse.m_iEvent = sseRetWithBuf;
        sse.m_iEvtSrc = GetClsid();
        
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

            oIrpLock.Unlock();
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
    oIrpLock.Unlock();
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
    oIrpLock.Unlock();
    pMgr->CompleteIrp( pIrp );

    return 0;
}

#define STMPDO2_MAX_IDX_PER_REQ 1024

gint32 CBytesSender::SendImmediate(
    gint32 iFd, PIRP pIrpLocked )
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

        if( pIrpLocked != pIrp )
        {
            ret = -ENOENT;
            break;
        }

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
            guint32 dwSendFlag = 0;
            if( m_iBufIdx < iCount - 1 )
                dwSendFlag = MSG_MORE;

            guint32 dwMaxBytes =
                MAX_BYTES_PER_TRANSFER;

            guint32 dwSize = std::min(
                pBuf->size() - m_dwOffset,
                dwMaxBytes );

            ret = SendBytesWithFlag( iFd,
                pBuf->ptr() + m_dwOffset,
                dwSize, dwSendFlag );

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

    if( ret == ERROR_NOT_HANDLED ||
        ret == -ENOENT ||
        ret == STATUS_PENDING )
        return ret;

    SetSendDone( ret );
    return ret;
}

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
            return ERROR_NOT_HANDLED;

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
            guint32 dwSendFlag = 0;
            if( m_iBufIdx < iCount - 1 )
                dwSendFlag = MSG_MORE;

            guint32 dwMaxBytes =
                MAX_BYTES_PER_TRANSFER;

            guint32 dwSize = std::min(
                pBuf->size() - m_dwOffset,
                dwMaxBytes );

            ret = SendBytesWithFlag( iFd,
                pBuf->ptr() + m_dwOffset,
                dwSize, dwSendFlag );

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

gint32 CTcpStreamPdo2::SendImmediate(
    gint32 iFd, PIRP pIrp )
{
    if( iFd < 0 || pIrp == nullptr )
        return -EINVAL;

    gint32 ret = m_oSender.SendImmediate(
        iFd, pIrp );

    if( ERROR( ret ) && (
        ret != -ENOENT &&
        ret != ERROR_NOT_HANDLED ) )
   { 
        TaskletPtr pDisTask;
        gint32 iRet = DEFER_CALL_NOSCHED(
            pDisTask, ObjPtr( this ),
            &CTcpStreamPdo2::OnDisconnected );
        if( SUCCEEDED( iRet ) )
        {
            CIoManager* pMgr = GetIoMgr();
            pMgr->RescheduleTask( pDisTask );
        }
   }
   else if( SUCCEEDED( ret ) )
   {
        CRpcConnSock* pSock = m_pConnSock;
        pSock->Refresh();
   }

   return ret;
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
        CRpcTcpBusPort* pBus =
            ObjPtr( m_pBusPort );

        ret = pBus->AllocMainLoop( m_pLoop );
        if( ERROR( ret ) )
            break;

        CParamList oParams;

        // share the conn parameters with the sock
        // object
        ret = oParams.CopyProp(
            propConnParams, this );

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

gint32 CTcpStreamPdo2::OnPortReady(
    IRP* pIrp )
{
    gint32 ret = 0;
    do{
        guint32 dwPortId = 0;
        CCfgOpenerObj oPortCfg( this );
        ret = oPortCfg.GetIntProp(
            propPortId, dwPortId );

        if( ERROR( ret ) )
            break;

        IConfigDb* pcp = nullptr;

        // update the connparams
        ret = oPortCfg.CopyProp(
            propConnParams,
            ( CObjBase* )m_pConnSock );

        if( ERROR( ret ) )
            break;

        ret = oPortCfg.GetPointer(
            propConnParams, pcp );
        if( ERROR( ret ) )
            break;

        CConnParams oConnParams( pcp );
        if( m_pBusPort == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        CRpcTcpBusPort* pBus =
            ObjPtr( m_pBusPort );

        if( pBus == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        // for client side, the connection
        // information is complete at this
        // point
        ret = pBus->BindPortIdAndAddr(
            dwPortId, oConnParams );

    }while( 0 );

    if( ERROR( ret ) )
        return ret;

    return super::OnPortReady( pIrp );
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
            CMainIoLoop* pLoop = ObjPtr( m_pLoop );
            if( pLoop->GetThreadId() == rpcf::GetTid() )
            {
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

                // we need to run on the mainloop
                pTask->MarkPending();
                pLoop->AddTask( pTask );
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
            ret = DEFER_CALL( pMgr,
                ObjPtr( this ),
                &CTcpStreamPdo2::CancelListeningIrp,
                pIrp, ERROR_PORT_STOPPED );

            if( SUCCEEDED( ret ) )
                ret = STATUS_MORE_PROCESS_NEEDED;
        }

        if( ret == STATUS_PENDING ||
            ret == STATUS_MORE_PROCESS_NEEDED )
            break;

        GetPreStopStep( pIrp, dwStepNo );
        if( dwStepNo == 2 )
        {
            SetPreStopStep( pIrp, 3 );
            ret = super::PreStop( pIrp );
        }

        if( ret == STATUS_PENDING ||
            ret == STATUS_MORE_PROCESS_NEEDED )
            break;

        GetPreStopStep( pIrp, dwStepNo );
        if( dwStepNo == 3 )
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

    CRpcTcpBusPort* pBus =  
        ObjPtr( m_pBusPort );
    pBus->ReleaseMainLoop( m_pLoop );
    m_pLoop.Clear();

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
                case CTRLCODE_RESET_CONNECTION:
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
    case CTRLCODE_RESET_CONNECTION:
        {
            CRpcConnSock* pSock = m_pConnSock;
            if( pSock == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            ret = pSock->ResetSocket();
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

            STREAM_SOCK_EVENT* ptr =
                ( STREAM_SOCK_EVENT* )pBuf->ptr();
            new ( ptr ) STREAM_SOCK_EVENT( sse );

            IrpPtr pIrp =
                m_queListeningIrps.front();

            vecIrps.push_back(
                ElemType( pIrp, pBuf ) );

            if( pIrpLocked == pIrp )
                bCompleted = true;

            sse.m_pInBuf.Clear();
            if( ptr->m_pInBuf.IsEmpty() )
            {
                DebugPrint( ret, "strange, the"
                " inbuf is empty()" );
            }
            m_queListeningIrps.pop_front();
            m_queEvtToRecv.pop_front();
        }

    }while( 0 );

    if( ret == STATUS_PENDING )
        return ret;

    if( vecIrps.empty() )
    {
        ret = -ENOENT;
        pCtx->SetStatus( ret );
        return ret;
    }

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

    return StartSend2( pIrp );
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
                gint32 iFd = -1;
                ret = pSock->GetSockFd( iFd );
                if( ERROR( ret ) )
                {
                    ret = ERROR_PORT_STOPPED;
                    break;
                }
                m_oSender.SetIrpToSend(
                    m_queWriteIrps.front() );
                m_queWriteIrps.pop_front();
                oPortLock.Unlock();
                ret = SendImmediate( iFd, pIrpLocked );
                if( ret == ERROR_NOT_HANDLED ||
                    ret == -ENOENT )
                {
                    // the irp is gone
                    ret = STATUS_PENDING;
                    break;
                }
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

gint32 CTcpStreamPdo2::StartSend2(
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

        gint32 iFd = -1;
        ret = pSock->GetSockFd( iFd );
        if( ERROR( ret ) )
        {
            ret = ERROR_PORT_STOPPED;
            break;
        }

        if( !m_oSender.IsSendDone() ||
            m_queWriteIrps.size() > 1 )
        {
            ret = STATUS_PENDING;
            break;
        }

        // start sending immediately
        m_oSender.SetIrpToSend(
            m_queWriteIrps.front() );
        m_queWriteIrps.pop_front();

        oPortLock.Unlock();

        ret = SendImmediate( iFd, pIrpLocked );
        if( ret == ERROR_NOT_HANDLED ||
            ret == -ENOENT )
        {
            // the irp is gone
            ret = STATUS_PENDING;
            break;
        }
        else if( ret == STATUS_PENDING )
        {
            pSock->StartWatch();
            break;
        }
        else if( ERROR( ret ) )
        {
            break;
        }

        oPortLock.Lock();
        if( !m_oSender.IsSendDone() )
            break;
        if( m_queWriteIrps.empty() )
            break;

        // schedule a task for the rest irps it
        // could contends with other writing
        // threads and the OnSendReady, but it can
        // reduce the delay time
        CIoManager* pMgr = GetIoMgr();
        DEFER_CALL( pMgr, ObjPtr( this ),
            &CTcpStreamPdo2::StartSend3 );

    }while( 0 );

    if( ERROR( ret ) )
    {
        // we need to remove the irp from
        // the outgoing queue
        RemoveIrpFromMap( pIrpLocked );
    }

    return ret;
}

gint32 CTcpStreamPdo2::StartSend3()
{
    gint32 ret = 0;
    do{
        CStdRMutex oPortLock( GetLock() );

        CRpcConnSock* pSock = m_pConnSock;
        if( unlikely( pSock == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }

        gint32 iFd = -1;
        ret = pSock->GetSockFd( iFd );
        if( ERROR( ret ) )
            break;

        if( !m_oSender.IsSendDone() )
            break;

        if( m_queWriteIrps.empty() )
            break;

        IrpPtr pIrp = m_queWriteIrps.front();
        // start sending immediately
        m_oSender.SetIrpToSend( pIrp );
        m_queWriteIrps.pop_front();

        oPortLock.Unlock();

        ret = SendImmediate( iFd, pIrp );
        if( ret == ERROR_NOT_HANDLED ||
            ret == -ENOENT )
        {
            break;
        }
        else if( ret == STATUS_PENDING )
        {
            pSock->StartWatch();
            break;
        }
        else if( ERROR( ret ) )
        {
            break;
        }

    }while( 1 );

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

gint32 CTcpStreamPdo2::CancelListeningIrp(
    PIRP pStopIrp, gint32 iErrno )
{
    CStdRMutex oPortLock( GetLock() );
    CParamList oParams;
    std::vector< IrpPtr > vecIrps;

    for( auto& elem : m_queListeningIrps )
        vecIrps.push_back( elem );
    oPortLock.Unlock();

    DebugPrint( iErrno, "Canceling listening IRPs..." );
    
    STREAM_SOCK_EVENT sse;
    sse.m_iEvent = sseError;
    sse.m_iData = iErrno;
    sse.m_iEvtSrc = GetClsid();
    gint32 ret = 0;

    CIoManager* pMgr = GetIoMgr();
    for( auto& elem : vecIrps )
    {
        PIRP pIrp = elem;
        CStdRMutex oIrpLock( pIrp->GetLock() );
        ret = pIrp->CanContinue( IRP_STATE_READY );
        if( ERROR( ret ) )
        {
            ret = 0;
            continue;
        }
        if( pIrp->GetStackSize() == 0 )
            continue;

        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        BufPtr pRespBuf( true );
        pRespBuf->Resize( sizeof( sse ) );
        memcpy( pRespBuf->ptr(),
            &sse, sizeof( sse ) );
        pCtx->SetRespData( pRespBuf );
        pCtx->SetStatus( 0 );
        oIrpLock.Unlock();
        pMgr->CompleteIrp( pIrp );
    }
    pMgr->CompleteIrp( pStopIrp );
    return 0;
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

void CTcpStreamPdo2::OnPortStartFailed(
        IRP* pIrp, gint32 ret )
{
    m_oSender.Clear();
    return;
}

gint32 CTcpStreamPdo2::GetProperty(
    gint32 iProp, Variant& oBuf ) const
{
    gint32 ret = 0;
    if( iProp == propRttMs )
    {
        CRpcConnSock* pSock = m_pConnSock;
        guint32 dwRtt;
        ret = pSock->GetRttTimeMs( dwRtt );
        if( ERROR( ret ) )
            return ret;
        oBuf = dwRtt;
        return 0;
    }
    return super::GetProperty( iProp, oBuf );
}

static gint32 GetCfgFile( stdstr& strFile )
{
    gint32 ret = 0;
    do{
        ret = access( CONFIG_FILE, R_OK );
        if( ret == 0 )
        {
            strFile = CONFIG_FILE;
            break;
        }
        ret = FindInstCfg(
            CONFIG_FILE, strFile );

    }while( 0 );
    return ret;
}

#include "jsondef.h"
bool IsVerifyPeerEnabled(
    const Json::Value& oValue,
    const stdstr& strPortClass )
{
    bool bRet = false;
    do{
        if( !oValue.isObject() ||
            oValue.empty() )
            break;

        if( !oValue.isMember( JSON_ATTR_PORTS ) ||
            !oValue[ JSON_ATTR_PORTS ].isArray() )
            break;

        const Json::Value& oPorts =
            oValue[ JSON_ATTR_PORTS ];

        if( oPorts == Json::Value::null )
            break;

        if( !oPorts.isArray() ||
            oPorts.size() == 0 )
            break;

        guint32 i = 0;
        for( ; i < oPorts.size(); i++ )
        {
            const Json::Value& elem = oPorts[ i ];
            if( elem == Json::Value::null )
                continue;

            if( strPortClass !=
                elem[ JSON_ATTR_PORTCLASS ].asString() )
                continue;


            if( !elem.isMember( JSON_ATTR_VERIFY_PEER ) )
                break;
            
            const Json::Value& oVal =
                elem[ JSON_ATTR_VERIFY_PEER ];

            if( oVal.empty() || !oVal.isString() )
                break;

            if( oVal.asString() == "true" )
                bRet = true;
        }

    }while( 0 );

    return bRet;
}

bool IsSSLEnabled(
    const Json::Value& oValue,
    bool& bOpenSSL,
    const stdstr& strPortClass =
        PORT_CLASS_TCP_STREAM_PDO2 )
{
    bool bRet = false;

    do{
        if( !oValue.isObject() ||
            oValue.empty() )
            break;

        if( !oValue.isMember( JSON_ATTR_MATCH ) ||
            !oValue[ JSON_ATTR_MATCH ].isArray() )
            break;

        const Json::Value& oMatchArray =
            oValue[ JSON_ATTR_MATCH ];

        if( oMatchArray == Json::Value::null )
            break;

        if( !oMatchArray.isArray() ||
            oMatchArray.size() == 0 )
            break;

        guint32 i = 0;
        for( ; i < oMatchArray.size(); i++ )
        {
            const Json::Value& elem = oMatchArray[ i ];
            if( elem == Json::Value::null )
                continue;

            if( strPortClass !=
                elem[ JSON_ATTR_PORTCLASS ].asString() )
                continue;


            if( !elem.isMember( JSON_ATTR_PROBESEQ ) )
                break;

            const Json::Value& oDrvArray =
                elem[ JSON_ATTR_PROBESEQ ];

            if( !oDrvArray.isArray() ||
                oDrvArray.size() == 0 )
                break;

            const Json::Value& oDrvName =
                oDrvArray[ 0 ];

            if( oDrvName.empty() ||
                !oDrvName.isString() )
                break;

            if( oDrvName.asString() ==
                "RpcOpenSSLFidoDrv" )
            {
                bOpenSSL = true;
                bRet = true;
            }
            else if( oDrvName.asString() ==
                "RpcGmSSLFidoDrv" )
            {
                bOpenSSL = false;
                bRet = true;
            }
            break;
        }

    }while( 0 );

    return bRet;
}

gint32 CheckForKeyPass( bool& bPrompt )
{
    gint32 ret = -ENOENT;
    do{
        stdstr strFile;
        ret = GetCfgFile( strFile );        
        if( ERROR( ret ) )
            break;
        
        Json::Value oValue;
        ret = ReadJsonCfg( strFile, oValue );
        if( ERROR( ret ) )
            break;

        bool bOpenSSL = false;
        if( !IsSSLEnabled( oValue, bOpenSSL ) )
        {
            ret = ERROR_FALSE;
            break;
        }

        stdstr strPortClass;
        if( bOpenSSL )
            strPortClass = PORT_CLASS_OPENSSL_FIDO;
        else
            strPortClass = PORT_CLASS_GMSSL_FIDO;

        if( !oValue.isMember( JSON_ATTR_PORTS ) ||
            !oValue[ JSON_ATTR_PORTS ].isArray() )
        {
            ret = -EINVAL;
            break;
        }

        Json::Value& oPortArray =
            oValue[ JSON_ATTR_PORTS ];

        ret = -ENOENT;
        if( oPortArray == Json::Value::null )
            break;

        if( !oPortArray.isArray() ||
            oPortArray.size() == 0 )
            break;

        guint32 i = 0;
        for( ; i < oPortArray.size(); i++ )
        {
            Json::Value& elem = oPortArray[ i ];
            if( elem == Json::Value::null )
                continue;

            if( strPortClass !=
                elem[ JSON_ATTR_PORTCLASS ].asString() )
                continue;

            if( !elem.isMember( JSON_ATTR_PARAMETERS ) ||
                !elem[ JSON_ATTR_PARAMETERS ].isObject() )
                break;

            Json::Value& oParams =
                elem[ JSON_ATTR_PARAMETERS ];

            if( !oParams.isMember( JSON_ATTR_SECRET_FILE ) )
                break;

            Json::Value& oVal =
                oParams[ JSON_ATTR_SECRET_FILE ];
            if( oVal.empty() || !oVal.isString() )
                break;

            bPrompt = false;
            if( oVal.asString() == "console" )
            {
                bPrompt = true;
                ret = 0;
            }
            break;
        }

    }while( 0 );

    return ret;
}

}
