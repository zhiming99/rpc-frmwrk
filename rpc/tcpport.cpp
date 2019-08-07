/*
 * =====================================================================================
 *
 *       Filename:  tcpport.cpp
 *
 *    Description:  implementation of CRpcStreamSock, CRpcListeningSock and related
 *                  classes
 *
 *        Version:  1.0
 *        Created:  05/07/2017 09:16:00 AM
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
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "reqopen.h"
#include <fcntl.h>

using namespace std;

#define RETRIABLE( iRet_ ) \
    ( iRet_ == EWOULDBLOCK || iRet_ == EAGAIN )

CRpcSocketBase::CRpcSocketBase(
    const IConfigDb* pCfg )
    : m_iState( sockInit ),
    m_iFd( -1 ),
    m_pMgr( nullptr ),
    m_dwRtt( 0 ),
    m_dwAgeSec( 0 ),
    m_iTimerId( 0 ),
    m_pParentPort( nullptr ),
    m_hIoRWatch( 0 ),
    m_hIoWWatch( 0 )
{
    gint32 ret = 0;
    do{
        if( pCfg == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        ret = m_pCfg.NewObj();
        if( ERROR( ret ) )
            break;

        *m_pCfg = *pCfg;

        CCfgOpener oCfg(
            ( IConfigDb* )pCfg );
        
        ret = oCfg.GetPointer(
            propIoMgr, m_pMgr );

        if( ERROR( ret ) )
            break;

        m_pCfg->RemoveProperty(
            propIoMgr );

        ret = oCfg.GetPointer(
            propPortPtr, m_pParentPort );

        if( ERROR( ret ) )
            break;

        m_pCfg->RemoveProperty(
            propPortPtr );

    }while( 0 );

    if( ERROR( ret ) )
    {
        string strMsg = DebugMsg( ret,
            "Error in CRpcSocketBase's ctor" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CRpcSocketBase::SetKeepAlive( bool bKeep )
{
    gint32 ret = 0;
    gint32 val = bKeep;
    gint32 interval = 120;

    do{
        if( m_iFd < 0 )
        {
            ret = ERROR_STATE;
            break;
        }

        if( setsockopt( m_iFd, SOL_SOCKET,
            SO_KEEPALIVE, &val, sizeof(val) ) == -1)
        {
            ret = -errno; 
            break;
        }

        if( !bKeep )
            break;

        /* Send first probe after `interval'
         * seconds. */
        val = interval;
        if( setsockopt(m_iFd, IPPROTO_TCP,
            TCP_KEEPIDLE, &val, sizeof(val) ) == -1 )
        {
            ret = -errno; 
            break;
        }

        /* Send next probes after the specified
         * interval. Note that we set the delay as
         * interval / 3, as we send three probes
         * before detecting an error (see the next
         * setsockopt call). */
        val = interval / 3;
        if( val == 0 ) val = 1;
        if( setsockopt(m_iFd, IPPROTO_TCP,
            TCP_KEEPINTVL, &val, sizeof(val) ) == -1 )
        {
            ret = -errno; 
            break;
        }

        /* Consider the socket in error state
         * after three we send three ACK probes
         * without getting a reply. */
        val = 3;
        if( setsockopt( m_iFd, IPPROTO_TCP,
            TCP_KEEPCNT, &val, sizeof(val) ) == -1 )
        {
            ret = -errno; 
            break;
        }

        // Disabling TCP_NODELAY could cause a 100ms
        // overall delay ( forwarder and bridge, each
        // contribute about 50ms ), when the traffic
        // load is low.  However the latency can drop
        // to less than 6ms when the traffic becomes
        // heavy. Enabling TCP_NODELAY can yield a
        // constant latency of 6ms.
        val = 1;
        if( setsockopt( m_iFd, IPPROTO_TCP,
            TCP_NODELAY, (void *)&val, sizeof(val) ) == -1 )
        {
            ret = -errno;
            break;
        }

    }while( 0 );

    return ret;
}

gint32 CRpcSockWatchCallback::operator()(
    guint32 event )
{
    gint32 ret = 0;
    do{
        std::vector< guint32 > vecParams;
        ret = GetParamList( vecParams );
        if( ERROR( ret ) )
        {
            ret = G_SOURCE_CONTINUE;
            break;
        }

        guint32 dwContext = vecParams[ 1 ];
        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );

        CRpcSocketBase* pSock = nullptr;
        ret = oCfg.GetPointer(
            propObjPtr, pSock );

        if( ERROR( ret ) )
        {
            ret = G_SOURCE_CONTINUE;
            break;
        }

        GIOCondition dwCond =
            ( GIOCondition )dwContext;

        ret = pSock->SockEventCallback(
            nullptr, dwCond, pSock );

    }while( 0 );

    return SetError( ret );
}

gint32 CRpcSocketBase::StopWatch( bool bWrite )
{
    gint32 ret = 0;
    CMainIoLoop* pLoop =
        GetIoMgr()->GetMainIoLoop();

#if _USE_LIBEV
    if( bWrite )
        ret = pLoop->StopSource(
            m_hIoWWatch, srcIo );
    else
        ret = pLoop->StopSource(
            m_hIoRWatch, srcIo );
#endif

    return ret;
}

gint32 CRpcSocketBase::StartWatch( bool bWrite )
{
    gint32 ret = 0;
    CMainIoLoop* pLoop =
        GetIoMgr()->GetMainIoLoop();

#if _USE_LIBEV
    if( bWrite )
        ret = pLoop->StartSource(
            m_hIoWWatch, srcIo );
    else
        ret = pLoop->StartSource(
            m_hIoRWatch, srcIo );
#endif

    return ret;
}

gint32 CRpcSocketBase::AttachMainloop()
{
    if( m_iFd <= 0 )
        return ERROR_STATE;

    gint32 ret = 0;

    do{
        // poll
        guint32 dwOpt = ( guint32 )G_IO_IN;

        CIoManager* pMgr = GetIoMgr();
        CMainIoLoop* pMainLoop =
            pMgr->GetMainIoLoop();

        CParamList oParams;

        oParams.Push( m_iFd );
        oParams.Push( dwOpt );
        // don't start immediately
        oParams.Push( false );

        oParams[ propObjPtr ] =
            ObjPtr( this );

        TaskletPtr pCallback;
        ret = pCallback.NewObj(
            clsid( CRpcSockWatchCallback ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        ret = pMainLoop->AddIoWatch(
            pCallback, m_hIoRWatch );

        if( ERROR( ret ) )
            break;

        CCfgOpenerObj oTaskCfg(
            ( CObjBase* )pCallback );
        dwOpt = ( guint32 )G_IO_OUT;
        oTaskCfg.SetIntProp( 1, dwOpt );
        // don't start immediately
        oTaskCfg.SetBoolProp( 2, false );

        ret = pMainLoop->AddIoWatch(
            pCallback, m_hIoWWatch );

        if( ERROR( ret ) )
            break;

    }while( 0 );

    if( ERROR( ret ) )
    {
        // rollback
        DetachMainloop();
    }

    return ret;
}

gint32 CRpcSocketBase::DetachMainloop()
{

    CIoManager* pMgr = GetIoMgr();
    CMainIoLoop* pLoop = pMgr->GetMainIoLoop();

    if( m_hIoRWatch != 0 )
    {
        pLoop->RemoveIoWatch( m_hIoRWatch );
        m_hIoRWatch = 0;
    }

    if( m_hIoWWatch != 0 )
    {
        pLoop->RemoveIoWatch( m_hIoWWatch );
        m_hIoWWatch = 0;
    }

    return 0;
}

/**
* @name SockEventCallback
* gio watch Callback for socket events.
* @{ */
/** Note that the 3rd param, `data' must be a
 * pointer to CRpcSockBase, otherwise unexpect
 * result
 * @} */

gboolean CRpcSocketBase::SockEventCallback(
    GIOChannel* source,
    GIOCondition condition,
    gpointer data)
{
    CRpcSocketBase* pThis = reinterpret_cast
        < CRpcSocketBase* >( data );

    if( pThis == nullptr )
        return G_SOURCE_REMOVE;

    pThis->OnEvent( eventSocket,
        ( guint32 ) condition, 0, nullptr );

    return G_SOURCE_CONTINUE;
}

gint32 CRpcSocketBase::UpdateRttTimeMs()
{
    tcp_info ti;
    memset( &ti, 0, sizeof( ti ) );
    socklen_t tisize = sizeof(ti);

    // PORTING ALERT: linux specific
    gint32 ret = getsockopt(m_iFd,
        IPPROTO_TCP, TCP_INFO, &ti, &tisize); 
    if( ret == -1 )
    {
        ret = -errno;
    }
    else
    {
        m_dwRtt = ti.tcpi_rtt;
    }

    return ret;
}

gint32 CRpcSocketBase::GetRttTimeMs(
    guint32 dwMilSec)
{
    gint32 ret = 0;

    if( m_dwRtt == 0 )
    {
        ret = UpdateRttTimeMs();
        if( SUCCEEDED( ret ) )
            dwMilSec = m_dwRtt;
    }
    else
    {
        dwMilSec = m_dwRtt;
    }

    return ret;
}

gint32 CRpcSocketBase::StartTimer()
{
    gint32 ret = 0;
    do{
        if( GetState() == sockStopped )
            return ERROR_STATE;

        CIoManager *pMgr = GetIoMgr();
        if( pMgr == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        CUtilities& oUtils =
            pMgr->GetUtils();

        CTimerService& oTimerSvc =
            oUtils.GetTimerSvc();

        ret = oTimerSvc.AddTimer(
            SOCK_TIMER_SEC, this, eventAgeSec ); 

        if( ERROR( ret ) )
            break;

        m_iTimerId = ret;

    }while( 0 );

    return ret;
}

gint32 CRpcSocketBase::OnEvent(
        EnumEventId iEvent,
        guint32 dwParam1,
        guint32 dwParam2,
        guint32* pData )
{
    gint32 ret = 0;
    switch( iEvent )
    {
    case eventTimeout:
        {
            if( dwParam1 == eventAgeSec )
            {
                m_dwAgeSec += SOCK_TIMER_SEC;
                if( m_dwAgeSec >= SOCK_LIFE_SEC )
                {
                    // NOTE: we don't call
                    // OnDisconnected because the
                    // connection will lose after
                    // that call
                    ret = m_pParentPort->OnEvent(
                        eventDisconn, 0, 0, nullptr );
                    break;
                }

                // start the next timer
                if( true )
                {
                    CRpcStreamSock* pSock = nullptr;
                    if( GetClsid() == clsid( CRpcStreamSock ) )
                    {
                        CStdRMutex oSockLock( GetLock() );
                        pSock = static_cast< CRpcStreamSock* >( this );
                        pSock->AgeStreams( SOCK_TIMER_SEC );
                    }
                    if( pSock != nullptr )
                        pSock->ClearIdleStreams();
                }

                ret = StartTimer();
                if( SUCCEEDED( ret ) )
                {
                    m_iTimerId = ret;
                }
                else
                {
                    m_iTimerId = 0;
                }
            }
            else
            {
                ret = -ENOTSUP;
            }

            break;
        }
    case eventSocket:
        {
            ret = DispatchSockEvent(
                ( GIOCondition )dwParam1 );
            break;
        }
    default:
        {
            ret = -ENOTSUP;
        }
    }

    return ret;
}

gint32 CRpcSocketBase::StopTimer()
{
    gint32 ret = 0;

    if( m_iTimerId == 0 )
        return 0;
    do{
        CIoManager *pMgr = GetIoMgr();
        if( pMgr == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        CUtilities& oUtils =
            pMgr->GetUtils();

        CTimerService& oTimerSvc =
            oUtils.GetTimerSvc();

        oTimerSvc.RemoveTimer(
            m_iTimerId ); 

        m_iTimerId = 0;

    }while( 0 );

    return ret;
}

gint32 CRpcSocketBase::Stop()
{
    gint32 ret = 0;
    do{
        CStdRMutex oSockLock( GetLock() );
        if( GetState() == sockStopped )
        {
            ret = ERROR_STATE;
            break;
        }
        DetachMainloop();
        StopTimer();
        ret = CloseSocket();

        if( ERROR( ret ) )
        {
            DebugPrint( ret,
                "Error close socket..." );
        }

        SetState( sockStopped );

    }while( 0 );

    return ret;
}

gint32 CRpcSocketBase::CloseSocket()
{
    int ret = 0;
 
    do{
        if( m_iFd < 0 )
            ret = -ENOTSOCK;

        ret = shutdown( m_iFd, SHUT_RDWR );
        if( ERROR( ret ) )
        {
            DebugPrint( -errno,
                "Error shutdown socket[%d]",
                m_iFd );
        }
        ret = close( m_iFd );

        if( ret == -1 && errno == EINTR )
            continue;

        else if( ret == -1 )
            ret = -errno;

        break;

    } while( 1 );

    m_iFd = -1;

    return ret;
}

gint32 CRpcSocketBase::SetState(
    EnumSockState iState )
{
    CStdRMutex oSockLock( GetLock() );
    m_iState = iState;
    return 0;
}

EnumSockState CRpcSocketBase::GetState() const
{
    CStdRMutex oSockLock( GetLock() );
    return m_iState;
}

gint32 CRpcSocketBase::GetAsyncErr() const
{
    gint32 ret = 0;
    do{
        gint32 iVal = 0;
        socklen_t iLen = sizeof( iVal );

        ret = getsockopt( m_iFd,
            SOL_SOCKET, SO_ERROR, &iVal, &iLen );

        if( ret == -1 )
        {
            ret = -errno;
            break;
        }
        ret = -iVal;

    }while( 0 );

    return ret;
}

gint32 CRpcStreamSock::GetStreamByPeerId(
    gint32 iPeerId, StmPtr& pStm )
{
    if( iPeerId < 0 )
        return -EINVAL;

    if( IsReserveStm( iPeerId ) )
    {
        return GetStream( iPeerId, pStm );
    }

    for( auto&& oElem : m_mapStreams )
    {
        gint32 iStmId;
        gint32 ret = ( oElem.second )->
            GetPeerStmId( iStmId );

        if( ERROR( ret ) )
            continue;

        pStm = oElem.second;
        return 0;
    }
    return -ENOENT;
}

gint32 CRpcStreamSock::GetStreamFromIrp(
    IRP* pIrp, StmPtr& pStm )
{
    if( pIrp == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        guint32 dwMajorCmd = pCtx->GetMajorCmd();
        guint32 dwMinorCmd = pCtx->GetMinorCmd();

        if( dwMajorCmd == IRP_MJ_FUNC &&
            ( dwMinorCmd == IRP_MN_READ ||
              dwMinorCmd == IRP_MN_WRITE ) )
        {
            CfgPtr pCfg;
            ret = pCtx->GetReqAsCfg( pCfg );
            if( ERROR( ret ) )
                break;

            CCfgOpener oCfg(
                ( IConfigDb* )pCfg );
           
            gint32 iStmId;
            ret = oCfg.GetIntProp(
                propStreamId, ( guint32& )iStmId );

            if( ERROR( ret ) )
                break;
            
            ret = GetStream( iStmId, pStm );
            if( ERROR( ret ) )
                break;
        }
        else
        {
            ret = GetCtrlStmFromIrp( pIrp, pStm );
        }

    }while( 0 );

    return ret;
}

gint32 CRpcStreamSock::RemoveIrpFromMap(
    IRP* pIrp )
{
    gint32 ret = 0;

    do{
        CStdRMutex oSockLock( GetLock() );

        StmPtr pStm;
        ret = GetStreamFromIrp( pIrp, pStm );
        if( ERROR( ret ) )
            break;

        ret = pStm->RemoveIrpFromMap( pIrp );

    }while( 0 );

    return ret;
}

gint32 CRpcStreamSock::NewStreamId(
    gint32& iStream )
{
    CStdRMutex oSockLock( GetLock() );
    iStream = GetSeedStmId();
    return 0;
}

gint32 CRpcStreamSock::ExistStream(
    gint32 iStream )
{
    gint32 ret = 0;
    if( iStream < 0 )
        return -EINVAL;

    do{
        CStdRMutex oSockLock( GetLock() );
        if( m_mapStreams.find( iStream )
            != m_mapStreams.end() )
        {
            break;
        }
        else
        {
            ret = -ENOENT;
        }

    }while( 0 );

    return ret;
}

gint32 CRpcStreamSock::AddStream(
    gint32 iStream,
    guint16 wProtoId,
    gint32 iPeerStmId )
{
    gint32 ret = 0;

    if( iStream < 0 )
        return -EINVAL;

    do{
        CStdRMutex oSockLock( GetLock() );
        if( m_mapStreams.find( iStream )
            != m_mapStreams.end() )
        {
            ret = -EEXIST;
            break;
        }

        CParamList oParams;

        guint32 dwVal = iStream;
        oParams.Push( dwVal );

        dwVal = wProtoId;
        oParams.Push( dwVal );

        ret = oParams.SetPointer( propIoMgr, GetIoMgr() );
        if( ERROR( ret ) )
            break;

        ret = oParams.SetObjPtr(
            propSockPtr, ObjPtr( this ) );

        if( ERROR( ret ) )
            break;

        StmPtr pStm;
        if( wProtoId == protoStream ||
            wProtoId == protoDBusRelay )
        {
            ret = pStm.NewObj(
                clsid( CRpcStream ),
                oParams.GetCfg() );
        }
        else if( wProtoId == protoControl )
        {
            ret = pStm.NewObj(
                clsid( CRpcControlStream ),
                oParams.GetCfg() );
        }
        else
        {
            ret = -ENOTSUP;
        }

        if( ERROR( ret ) )
            break;

        m_mapStreams[ iStream ] = pStm;
        pStm->SetPeerStmId( iPeerStmId );

    }while( 0 );

    return ret;
}

gint32 CRpcStreamSock::RemoveStream(
    gint32 iStream )
{
   if( iStream < 0 )
       return -EINVAL;

    gint32 ret = 0;
    do{
        CStdRMutex oSockLock( GetLock() );
        if( m_itrCurStm != m_mapStreams.end() )
        {
            gint32 iThisStm =
                m_itrCurStm->second->GetStreamId();

            if( iThisStm == iStream )
                m_itrCurStm = m_mapStreams.end();
        }

        if( m_mapStreams.find( iStream )
            == m_mapStreams.end() )
        {
            ret = -ENOENT;
            break;
        }
        m_mapStreams.erase( iStream );

        if( m_iCurSendStm == iStream )
            m_iCurSendStm = -1;

    }while( 0 );

    return ret;
}

CRpcStreamSock::CRpcStreamSock(
    const IConfigDb* pCfg )
    : super( pCfg ),
    m_iCurSendStm( -1 ),
    m_iStmCounter( STMSOCK_STMID_FLOOR )
{
    gint32 ret = 0;
    SetClassId( clsid( CRpcStreamSock ) );
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

        m_itrCurStm = m_mapStreams.begin();

    }while( 0 );

    if( ERROR( ret ) )
    {
        string strMsg = DebugMsg( ret,
            "Error in CRpcStreamSock ctor" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CRpcStreamSock::ActiveConnect(
    const string& strIpAddr )
{
    gint32 ret = 0;

    do{
        CCfgOpener oCfg(
            ( IConfigDb* )m_pCfg );

        guint32 dwPortNum = 0;
        ret = oCfg.GetIntProp(
            propDestTcpPort, dwPortNum );

        if( ERROR( ret ) )
            dwPortNum = RPC_SVR_PORTNUM;

        /*  Connect to the remote host. */
        sockaddr_in sin;

        memset( &sin, 0, sizeof( sin ) );
        sin.sin_family = AF_INET;
        sin.sin_port = htons(
            ( guint16 )dwPortNum );

        sin.sin_addr.s_addr =
            inet_addr( strIpAddr.c_str() );

        ret = connect( m_iFd, ( sockaddr* )&sin,
            sizeof( sin ) );

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

gint32 CRpcStreamSock::Connect()
{
    gint32 ret = 0;
    do{
        ret = socket( AF_INET,
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

        CCfgOpener oCfg(
            ( IConfigDb* )m_pCfg );

        string strIpAddr;
        ret = oCfg.GetStrProp(
            propIpAddr, strIpAddr );

        if( ERROR( ret ) )
            break;

        /*  Connect to the remote host. */
        ret = ActiveConnect( strIpAddr );

    }while( 0 );

    if( ERROR( ret ) )
    {
        DetachMainloop();
        CloseSocket();
    }

    return ret;
}

gint32 CRpcStreamSock::Start()
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
            fcntl( m_iFd, F_SETFL,
                iFlags | O_NONBLOCK );

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

gint32 CRpcStreamSock::Start_bh()
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

        ret = AddStream( TCP_CONN_DEFAULT_STM,
            protoDBusRelay,
            TCP_CONN_DEFAULT_STM );

        if( ERROR( ret ) )
            break;
        
        ret = AddStream( TCP_CONN_DEFAULT_CMD,
            protoControl,
            TCP_CONN_DEFAULT_CMD );

        if( ERROR( ret ) )
            break;

        ret = StartWatch( false );
        SetState( sockStarted );
        
    }while( 0 );

    return ret;
}


gint32 CRpcStreamSock::CancelAllIrps(
    gint32 iErrno )
{
    gint32 ret = 0;

    CStdRMutex oSockLock( GetLock() );
    IrpVecPtr pIrpVec( true );
    CParamList oParams;

    if( GetState() == sockStarted ||
        GetState() == sockDisconnected )
    {
        for( auto&& oPair : m_mapStreams )
        {
            StmPtr& pStm = oPair.second;
            vector< IrpPtr > vecIrps;
            pStm->GetAllIrpsQueued( vecIrps );

            for( auto&& pIrp : vecIrps )
            {
                ( *pIrpVec )().push_back( pIrp );
            }

            pStm->ClearAllQueuedIrps();
        }
    }
    oSockLock.Unlock();

    while( !( *pIrpVec )().empty()
        || oParams.GetCfg()->exist( propIrpPtr ) )
    {
        ret = oParams.Push(
            *( guint32* )&iErrno );

        if( ERROR( ret ) )
            break;

        ObjPtr pObj;

        if( !( *pIrpVec )().empty() )
        {
            pObj = pIrpVec;
            ret = oParams.Push( pObj );
            if( ERROR( ret ) )
                break;
        }

        oParams.SetPointer( propIoMgr, GetIoMgr() );

        // NOTE: this is a double check of the
        // remaining IRPs before the STOP begins.
  
        TaskletPtr pTask;
        pTask.NewObj(
            clsid( CCancelIrpsTask ),
            oParams.GetCfg() );

        ret = ( *pTask )( eventZero );
        break;
    }

    oSockLock.Lock();
    return ret;
}

gint32 CRpcStreamSock::Stop()
{
    gint32 ret = 0;
    do{
        CancelAllIrps(
            ERROR_PORT_STOPPED );

        CStdRMutex oSockLock( GetLock() );
        if( GetState() == sockStopped )
        {
            ret = ERROR_STATE;
            break;
        }
        ret = RemoveStream(
            TCP_CONN_DEFAULT_CMD );

        if( ERROR( ret ) )
            break;

        ret = RemoveStream(
            TCP_CONN_DEFAULT_STM );

        if( ERROR( ret ) )
            break;

    }while( 0 );
    ret = super::Stop();

    return ret;
}

gint32 CRpcStreamSock::DispatchSockEvent(
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

            CStmSockConnectTask*
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
            if( dwCondition & G_IO_HUP )
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
                OnSendReady();
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

gint32 CRpcStreamSock::OnReceive()
{
    gint32 ret = 0;
    IrpPtr pIrp;

    do{
        StmPtr pStm;

        if( true )
        {
            CStdRMutex oSockLock( GetLock() );
            if( GetState() != sockStarted )
            {
                ret = ERROR_STATE;
                break;
            }

            if( m_pPackReceiving.IsEmpty() )
            {
                ret = m_pPackReceiving.NewObj(
                    clsid( CIncomingPacket ) );

                if( ERROR( ret ) )
                    break;
            }

            CIncomingPacket* pIn =
                m_pPackReceiving;

            if( pIn == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            ret = pIn->FillPacket( m_iFd );
            if( ret == STATUS_PENDING )
            {
                break;
            }
            else if( ERROR( ret ) )
            {
                // NOTE: we are entering a wrong
                // state cannot recover, we need to
                // close the stream
                m_pPackReceiving.Clear();
                break;
            }
            else if( SUCCEEDED( ret ) )
            {
                gint32 iStmId = 0;
                pIn->GetPeerStmId( iStmId );

                gint32 iStmPeerId = 0;
                pIn->GetStreamId( iStmPeerId );

                guint32 dwProtoId = 0;
                pIn->GetProtoId( dwProtoId );

                guint32 dwSeqNo = 0;
                pIn->GetSeqNo( dwProtoId );


                ret = GetStream( iStmId, pStm );
                if( ERROR( ret ) )
                {
                    // Fine, let's notify the
                    // caller the target stream
                    // does not exist
                    CParamList oParams;
                    ret = oParams.SetIntProp(
                        propStreamId, iStmId );

                    if( ERROR( ret ) )
                        break;

                    ret = oParams.SetIntProp(
                        propPeerStmId, iStmPeerId );

                    if( ERROR( ret ) )
                        break;

                    ret = oParams.SetIntProp(
                        propProtoId, dwProtoId );

                    if( ERROR( ret ) )
                        break;

                    ret = oParams.SetIntProp(
                        propSeqNo, dwSeqNo );

                    if( ERROR( ret ) )
                        break;
                    ret = oParams.SetPointer(
                        propIoMgr, GetIoMgr() );

                    if( ERROR( ret ) )
                        break;

                    ret = oParams.SetObjPtr(
                        propPortPtr,
                        ObjPtr( m_pParentPort ) );

                    if( ERROR( ret ) )
                        break;

                    GetIoMgr()->ScheduleTask(
                        clsid( CStmSockInvalStmNotifyTask ),
                        oParams.GetCfg() );

                    break;
                }

                gint32 iRemoteId;
                pStm->GetPeerStmId( iRemoteId );
                if( iRemoteId != iStmPeerId )
                {
                    // wrong destination
                    ret = -EINVAL;
                    break;
                }

                ret = pStm->RecvPacket(
                    m_pPackReceiving );

                m_pPackReceiving.Clear();
                if( ERROR( ret ) )
                    break;
            }
        }

        if( pStm.IsEmpty() )
            continue;

        pStm->Refresh();
        pStm->DispatchDataIncoming( this );

    }while( 1 );

    if( ERROR( ret ) )
        m_pPackReceiving.Clear();

    return ret;
}

gint32 CRpcStreamSock::GetStreamToSend(
    StmPtr& pStm )
{
    gint32 ret = 0;
    guint32 dwLeft = m_mapStreams.size();
    if( m_mapStreams.empty() )
    {
        m_itrCurStm = m_mapStreams.end();
        return -ENOENT;
    }

    while( dwLeft > 0 )
    {
        if( m_itrCurStm == m_mapStreams.end() )
        {
            m_itrCurStm = m_mapStreams.begin();
        }

        if( m_itrCurStm->second->HasBufToSend() )
        {
            pStm = m_itrCurStm->second;
            break;
        }

        ++m_itrCurStm;
        --dwLeft;

    };
    if( dwLeft == 0 )
        ret = -ENOENT;

    return ret;
}

gint32 CRpcStreamSock::OnSendReady()
{
    return StartSend();
}

// event dispatcher
gint32 CRpcStreamSock::OnEvent(
    EnumEventId iEvent,
    guint32 dwParam1,
    guint32 dwParam2,
    guint32* pData )
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
            ret = CStmSockConnectTask::NewTask(
                GetIoMgr(),
                this,
                TASKLET_RETRY_TIMES,
                m_pStartTask );

            if( ERROR( ret ) )
                break;

            ret = m_pStartTask->OnEvent( iEvent,
                dwParam1, 0, nullptr );

            if( ERROR( ret ) ) 
                break;
                
            CStmSockConnectTask*
                pConnTask = m_pStartTask;
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

gint32 CStmSockCompleteIrpsTask::operator()(
    guint32 dwContext )
{
    gint32 ret = 0;
    do{
        CParamList oCfg(
            ( IConfigDb* )GetConfig() );

        ObjPtr pObj;
        ret = oCfg.GetObjPtr( 0, pObj );
        if( ERROR( ret ) )
            break;

        CIoManager* pMgr = nullptr;
        ret = oCfg.GetPointer( propIoMgr, pMgr );
        if( ERROR( ret ) )
            break;

        IrpVec2Ptr pIrpVec;
        pIrpVec = pObj;
        if( pIrpVec.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }
        vector< pair< gint32, IrpPtr > >&
            vecIrpsToComplete = ( *pIrpVec )();

        for( auto&& oPair : vecIrpsToComplete )
            pMgr->CompleteIrp( oPair.second );

    }while( 0 );

    return SetError( ret );
}

gint32 CRpcStreamSock::ScheduleCompleteIrpTask(
    IrpVec2Ptr& pIrpVec, bool bSync )
{
    if( pIrpVec.IsEmpty() )
        return -EINVAL;

    if( ( *pIrpVec )().empty() )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CParamList oParams;
        ObjPtr pObj = pIrpVec;
        oParams.Push( pObj );
        oParams.SetPointer( propIoMgr, GetIoMgr() );
        if( bSync )
        {
            TaskletPtr pTask;
            ret = pTask.NewObj(
                clsid( CStmSockCompleteIrpsTask ),
                oParams.GetCfg() );

            if( ERROR( ret ) )
                break;

            ( *pTask )( eventZero );
            ret = pTask->GetError();
        }
        else
        {
            ret = GetIoMgr()->ScheduleTask(
                clsid( CStmSockCompleteIrpsTask ),
                oParams.GetCfg() );
        }
    }while( 0 );

    return ret;
}
/**
* @name CRpcStreamSock::StartSend: to physically
* put the data to the socket's send buffer, till
* the send buffer is full or the queued the
* packets are all sent.
* @{ */
/** Parameter:
 * pIrpLocked: the irp current in processing if it
 * is from within a SubmitIrp method. if
 * pIrpLocked is empty, this method is called from
 * an event handler.
 * @} */

gint32 CRpcStreamSock::StartSend( IRP* pIrpLocked )
{
    gint32 ret = 0;
    gint32 retLocked = STATUS_PENDING;

    IrpVec2Ptr pIrpVec( true );
    vector< CStlIrpVector2::ElemType >&
        vecIrpComplete = ( *pIrpVec )();

    CIoManager* pMgr = GetIoMgr();
    if( pMgr == nullptr )
        return -EFAULT;

    do{
        StmPtr pStream;
        CStdRMutex oSockLock( GetLock() );
        if( GetState() != sockStarted )
        {
            ret = ERROR_STATE;
            break;
        }

        if( m_iCurSendStm >= 0 )
        {
            if( m_mapStreams.find( m_iCurSendStm )
                == m_mapStreams.end() )
            {
                m_iCurSendStm = -1;
                ret = GetStreamToSend( pStream );
                if( ERROR( ret ) )
                    break;

                m_iCurSendStm =
                    pStream->GetStreamId();
                continue;
            }
            else
            {
                pStream =
                    m_mapStreams[ m_iCurSendStm ];
            }

            IrpPtr pIrp;

            StartWatch();
            ret = pStream->StartSend( m_iFd, pIrp );
            if( ret == STATUS_PENDING )
                break;

            m_iCurSendStm = -1;
            if( !pIrp.IsEmpty() )
            {
                if( pIrpLocked == pIrp )
                {
                    IrpCtxPtr& pCtx =
                        pIrp->GetTopStack();
                    pCtx->SetStatus( ret );
                    retLocked = ret;
                }
                else
                {
                    // don't push the locked irp
                    // to this vector
                    vecIrpComplete.push_back(
                        CStlIrpVector2::ElemType( ret, pIrp ) );
                }
            }
            else
            {
                // it could be true if the pIrp is
                // canceled when the sending is
                // going on
            }
            if( SUCCEEDED( ret ) )
                continue;
        }
        else
        {
            ret = GetStreamToSend( pStream );
            if( ERROR( ret ) )
            {
                ret = 0;
                break;
            }
            m_iCurSendStm =
                pStream->GetStreamId();
            continue;
        }
        break;

    }while( 1 );

    if( ret != STATUS_PENDING )
        StopWatch();

    if( pIrpLocked == nullptr )
    {
        // the caller is io event handler from main
        // loop
        if( !vecIrpComplete.empty() )
        {
            ret = ScheduleCompleteIrpTask(
                pIrpVec, true );
        }
    }
    else
    {
        // this is an io request from SubmitIrp
        if( !vecIrpComplete.empty() )
        {
            ret = ScheduleCompleteIrpTask(
                pIrpVec, false );
        }

        if( SUCCEEDED( ret ) )
            ret = retLocked;
    }
    return ret;
}

gint32 CRpcStreamSock::HandleReadIrp(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CStdRMutex oSockLock( GetLock() );
        if( GetState() != sockStarted )
        {
            ret = -ENETDOWN;
            break;
        }
        
        IrpCtxPtr& pCtx = pIrp->GetCurCtx();
        guint32 dwMajorCmd = pCtx->GetMajorCmd();
        guint32 dwMinorCmd = pCtx->GetMinorCmd();
        if( dwMajorCmd == IRP_MJ_FUNC && 
            dwMinorCmd == IRP_MN_READ )
        {
            CfgPtr pCfg;
            ret = pCtx->GetReqAsCfg( pCfg );
            if( ERROR( ret ) )
                break;

            CCfgOpener oCfg(
                ( IConfigDb* )pCfg );
           
            gint32 iStmId;
            ret = oCfg.GetIntProp(
                propStreamId, ( guint32& )iStmId );

            if( ERROR( ret ) )
                break;
            
            StmPtr pStm;
            ret = GetStream( iStmId, pStm );
            if( ERROR( ret ) )
                break;

            ret = pStm->HandleReadIrp( pIrp );
        }
        else
        {
            ret = -ENOTSUP;
            break;
        }

    }while( 0 );

    return ret;
}

gint32 CRpcStreamSock::HandleWriteIrp(
    IRP* pIrp )
{
    if( pIrp == nullptr
        || pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    IrpCtxPtr& pCtx = pIrp->GetCurCtx();

    do{
        CStdRMutex oSockLock( GetLock() );
        if( GetState() != sockStarted )
        {
            ret = -ENETDOWN;
            break;
        }
        
        guint32 dwMajorCmd = pCtx->GetMajorCmd();
        guint32 dwMinorCmd = pCtx->GetMinorCmd();
        if( dwMajorCmd == IRP_MJ_FUNC && 
            dwMinorCmd == IRP_MN_WRITE )
        {
            CfgPtr pCfg;
            ret = pCtx->GetReqAsCfg( pCfg );
            if( ERROR( ret ) )
                break;

            CCfgOpener oCfg(
                ( IConfigDb* )pCfg );
           
            gint32 iStmId;
            ret = oCfg.GetIntProp(
                propStreamId, ( guint32& )iStmId );

            if( ERROR( ret ) )
                break;

            BufPtr pPayload( true );
            ret = oCfg.GetProperty(
                0, *pPayload );

            if( ERROR( ret ) )
                break;
            
            StmPtr pStm;
            ret = GetStream( iStmId, pStm );
            if( ERROR( ret ) )
                break;

            ret = pStm->HandleWriteIrp(
                pIrp, pPayload );
        }
        else
        {
            ret = -ENOTSUP;
            break;
        }
    }while( 0 );

    return ret;
}

gint32 CRpcStreamSock::HandleIoctlIrp(
    IRP* pIrp )
{
    // NOTE: you should make sure the irp is
    // locked from outside
    //
    //
    // properties from irp's m_pReqData
    //
    // 0:               the stream id to open/close
    //
    // propStreamId:    For CloseStream and OpenStream, the stream
    //                  id via which the command is sent
    //
    //                  For OpenLocalStream, ignored
    //                  For CloseLocalStream, the stream to close 
    //
    // propPeerStmId:   Peer stream id 
    //
    // propProtoId:     the protocol of the stream
    //
    // propSeqNo:       added by the socket , not by the irp maker
    //
    // propReqType:     the request type, a request,
    //      a response or an event
    //
    // propCmdId:       the request id, or an
    //      event id,
    //

    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CStdRMutex oSockLock( GetLock() );
        if( GetState() != sockStarted )
        {
            ret = -ENETDOWN;
            break;
        }
        
        IrpCtxPtr& pCtx = pIrp->GetCurCtx();
        guint32 dwCtrlCode = pCtx->GetCtrlCode();

        CfgPtr pCfg;
        ret = pCtx->GetReqAsCfg( pCfg );
        if( ERROR( ret ) )
            break;

        CReqOpener oCfg(
            ( IConfigDb* )pCfg );

        CRpcControlStream* pCtrlStm = nullptr;
        ObjPtr pPortObj( m_pParentPort );
        CTcpStreamPdo* pParentPort = pPortObj;

        if( !pParentPort->IsImmediateReq( pIrp ) )
        {
            // check stream existance
            StmPtr pStream;
            ret = GetCtrlStmFromIrp(
                pIrp, pStream );

            pCtrlStm = pStream;
            if( pCtrlStm == nullptr )
            {
                // CRpcStream does not support
                // ioctrl code
                ret = -ENOTSUP;
                break;
            }
        }

        switch( dwCtrlCode )
        {
        case CTRLCODE_GET_RMT_STMID:
        case CTRLCODE_GET_LOCAL_STMID:
            {
                gint32 iStmId = 0;
                gint32 iResultId = 0;
                guint32 dwProtoId = 0;
                // check steam existance
                ret = oCfg.GetIntProp( 0,
                    ( guint32& )iStmId );

                if( ERROR( ret ) )
                    break;

                iResultId = iStmId;
                if( iStmId == TCP_CONN_DEFAULT_CMD )
                {
                    dwProtoId = protoControl;
                }
                else if( iStmId == TCP_CONN_DEFAULT_STM )
                {
                    dwProtoId = protoDBusRelay;
                }
                else if( dwCtrlCode == CTRLCODE_GET_LOCAL_STMID )
                {
                    StmPtr pStream;
                    ret = GetStreamByPeerId( iStmId, pStream );

                    if( ERROR( ret ) )
                        break;

                    iStmId = pStream->GetStreamId();
                    if( ERROR( ret ) )
                        break;

                    dwProtoId = pStream->GetProtoId();
                }
                else if( dwCtrlCode == CTRLCODE_GET_RMT_STMID )
                {
                    StmPtr pStream;
                    ret = GetStream( iStmId, pStream );

                    if( ERROR( ret ) )
                        break;

                    ret = pStream->GetPeerStmId( iResultId );
                    if( ERROR( ret ) )
                        break;

                    dwProtoId = pStream->GetProtoId();
                }

                CParamList oParams;
                oParams.SetIntProp( propReturnValue, 0 );
                oParams.Push( ( guint32 )iResultId );
                oParams.Push( dwProtoId );

                BufPtr pBuf( true );
                *pBuf = ObjPtr( oParams.GetCfg() );
                pCtx->SetRespData( pBuf );

                break;
            }
        case CTRLCODE_CLOSE_STREAM_LOCAL_PDO:
            {
                gint32 iStreamId = -1;
                ret = oCfg.GetIntProp( 0,
                    ( guint32& )iStreamId );

                if( ERROR( ret ) )
                    break;

                if( IsReserveStm( iStreamId ) )
                {
                    ret = -EINVAL;
                    break;
                }

                StmPtr pStream;
                ret = GetStream( iStreamId, pStream );

                if( ERROR( ret ) )
                    break;

                gint32 iPeerStm = 0;

                pStream->GetPeerStmId( iPeerStm );
                guint32 dwProtoId =
                    pStream->GetProtoId();

                IrpVecPtr pIrpVec( true );
                ret = pStream->GetAllIrpsQueued(
                    ( *pIrpVec )() );

                if( ERROR( ret ) )
                    break;

                ret = RemoveStream( iStreamId );
                if( ERROR( ret ) )
                    break;

                // cancel all the contained irps
                CParamList oParams;
                ret = oParams.Push( ERROR_CANCEL );

                if( ERROR( ret ) )
                    break;

                ObjPtr pObj;
                if( !( *pIrpVec )().empty() )
                {
                    pObj = pIrpVec;
                    if( pObj.IsEmpty() )
                    {
                        ret = -EFAULT;
                        break;
                    }
                    ret = oParams.Push( pObj );
                    if( ERROR( ret ) )
                        break;

                    oParams.SetPointer( propIoMgr, GetIoMgr() );
                    ret = GetIoMgr()->ScheduleTask(
                        clsid( CCancelIrpsTask ),
                        oParams.GetCfg() );
                }

                if( true )
                {
                    // set the response data
                    CfgPtr pRespCfg( true );

                    CParamList oRespCfg(
                        ( IConfigDb* )pRespCfg );

                    oRespCfg.Push( iStreamId );
                    oRespCfg.Push( dwProtoId );
                    oRespCfg.Push( iPeerStm );

                    oRespCfg.SetIntProp(
                        propReturnValue, ret );

                    BufPtr pBuf( true );
                    *pBuf = ObjPtr( pRespCfg );

                    pCtx->SetRespData( pBuf );
                    // let the client check the
                    // response to find the new
                    // stream id
                    ret = 0;
                }
                break;
            }
        case CTRLCODE_OPEN_STREAM_LOCAL_PDO:
            {
                gint32 iStreamId = -1;

                gint32 iRequiredStm = -1;
                ret = oCfg.GetIntProp( 0,
                    ( guint32& )iRequiredStm );

                if( ERROR( ret ) )
                    break;

                guint32 dwProtoId = 0;
                ret = oCfg.GetIntProp( 1,
                    ( guint32& )dwProtoId );

                if( ERROR( ret ) )
                    break;

                if( IsReserveStm( iRequiredStm ) )
                {
                    ret = -EINVAL;
                    break;
                }

                ret = NewStreamId( iStreamId );
                if( ERROR( ret ) )
                    break;

                ret = AddStream( iStreamId,
                    dwProtoId, iRequiredStm );

                if( ERROR( ret ) )
                    break;

                if( true )
                {
                    // set the response data
                    CfgPtr pRespCfg( true );

                    CParamList oRespCfg(
                        ( IConfigDb* )pRespCfg );

                    oRespCfg.Push( iStreamId );
                    oRespCfg.Push( dwProtoId );
                    oRespCfg.Push( iRequiredStm );

                    oRespCfg.SetIntProp(
                        propReturnValue, ret );

                    BufPtr pBuf( true );
                    *pBuf = ObjPtr( pRespCfg );

                    pCtx->SetRespData( pBuf );
                    // let the client check the
                    // response to find the new
                    // stream id
                    ret = 0;
                }
                break;
            }
        case CTRLCODE_CLOSE_STREAM_PDO:
            {   
                // this command happens only on
                // proxy side
                guint32 dwType =
                    DBUS_MESSAGE_TYPE_METHOD_CALL;

                ret = oCfg.GetReqType( dwType );
                if( ERROR( ret ) )
                    break;

                if( dwType ==
                    DBUS_MESSAGE_TYPE_METHOD_CALL )
                {
                    gint32 iLocalStm = 0;
                    ret = oCfg.GetIntProp( 0,
                        ( guint32& )iLocalStm );

                    if( ERROR( ret ) )
                        break;

                    StmPtr pStm;
                    ret = GetStream( iLocalStm, pStm );
                    if( ERROR( ret ) )
                        break;

                    gint32 iPeerStmId = 0;
                    pStm->GetPeerStmId( iPeerStmId );
                    oCfg.Push( iPeerStmId );
                }

                ret = pCtrlStm->HandleIoctlIrp( pIrp );

                break;
            }
        case CTRLCODE_INVALID_STREAM_ID_PDO:
            {
                guint32 dwSize = 0;
                ret = oCfg.GetSize( dwSize );
                if( ERROR( ret ) || dwSize < 4 )
                {
                    // at least for parameters
                    ret = -EINVAL;
                    break;
                }
                ret = pCtrlStm->HandleIoctlIrp( pIrp );
                break;
            }
        case CTRLCODE_OPEN_STREAM_PDO:
            {
                // this command happens only on
                // proxy side
                // the new stream id will be
                // returned after this call
                // completes
                guint32 dwType =
                    DBUS_MESSAGE_TYPE_METHOD_CALL;

                ret = oCfg.GetReqType( dwType );
                if( ERROR( ret ) )
                    break;

                if( dwType == DBUS_MESSAGE_TYPE_METHOD_CALL )
                {
                    if( !oCfg.exist( 0 ) )
                    {
                        ret = -EINVAL;
                        break;
                    }
                    gint32 iStreamId = 0;
                    ret = NewStreamId( iStreamId );
                    guint32 dwProtocol = 0;

                    // adjust the params to send
                    oCfg.Pop( dwProtocol );
                    oCfg.Push( iStreamId );
                    oCfg.Push( dwProtocol );
                }
                ret = pCtrlStm->HandleIoctlIrp( pIrp );
                break;
            }
        case CTRLCODE_LISTENING:
            {
                ret = pCtrlStm->HandleIoctlIrp( pIrp );
                break;
            }
        case CTRLCODE_REG_MATCH:
        case CTRLCODE_UNREG_MATCH:
            {
                StmPtr pStream;
                ret = GetCtrlStmFromIrp(
                    pIrp, pStream );

                pCtrlStm = pStream;
                if( pCtrlStm == nullptr )
                {
                    ret = -EFAULT;
                    break;
                }
                ret = pCtrlStm->HandleIoctlIrp( pIrp );
                break;
            }
        default:
            {
                ret = -ENOTSUP;
                break;
            }
        }
    }while( 0 );

    return ret;
}

gint32 CRpcStreamSock::OnDisconnected()
{
    Stop();
    gint32 ret = m_pParentPort->OnEvent(
        eventDisconn, 0, 0, nullptr );

    return ret;
}

gint32 CRpcStreamSock::OnError(
    gint32 iError )
{
    gint32 ret = 0; 
    IrpPtr pIrp;

    do{
        if( GetState() == sockInit )
        {
            if( !m_pStartTask.IsEmpty() )
            {
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
            this->OnDisconnected();
        }
    }while( 0 );
    
    return ret;
}

gint32 CRpcStreamSock::QueueIrpForResp(
    IRP* pIrp )
{
    if( pIrp == nullptr
        || pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CStdRMutex oSockLock( GetLock() );

        StmPtr pStm;
        ret = GetCtrlStmFromIrp( pIrp, pStm );
        if( ERROR( ret ) )
            break;

        CRpcControlStream* pCtrlStm = pStm;
        if( pCtrlStm == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        ret = pCtrlStm->QueueIrpForResp( pIrp );

    }while( 0 );

    return ret;
}

gint32 CRpcStreamSock::GetCtrlStmFromIrp(
    IRP* pIrp, StmPtr& pStm )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;

    do{
        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        if( pCtx->GetMajorCmd() != IRP_MJ_FUNC ||
            pCtx->GetMinorCmd() != IRP_MN_IOCTL )
        {
            ret = -ENOTSUP;
            break;
        }

        switch( pCtx->GetCtrlCode() )
        {
        case CTRLCODE_OPEN_STREAM_PDO:
        case CTRLCODE_CLOSE_STREAM_PDO:
        case CTRLCODE_LISTENING:
        case CTRLCODE_REG_MATCH:
        case CTRLCODE_UNREG_MATCH:
            {
                CfgPtr pCfg;
                ret = pCtx->GetReqAsCfg( pCfg );
                if( ERROR( ret ) )
                    break;

                CCfgOpener oCfg(
                    ( IConfigDb* )pCfg );
               
                gint32 iStmId = 0;
                ret = oCfg.GetIntProp( propStreamId,
                    ( guint32& )iStmId );

                if( ERROR( ret ) )
                    break;
                
                ret = GetStream( iStmId, pStm );
                break;
            }
        default:
            {
                ret = -ENOTSUP;
                break;
            }
        }
    }while( 0 );

    return ret;
}

CCarrierPacket::CCarrierPacket()
    : super()
{
}

CCarrierPacket::~CCarrierPacket()
{
}

CIncomingPacket::CIncomingPacket()
    : CCarrierPacket()
{
    m_dwBytesToRecv = 0;
    m_dwOffset = 0;
    SetClassId( clsid( CIncomingPacket ) );
}

gint32 CIncomingPacket::FillPacket(
    gint32 iFd )
{
    if( iFd < 0 )
        return -EINVAL;

    gint32 ret = 0;
    bool bFirst = true;
    guint32 dwBufOffset = 0;
    do{
        guint32 dwSize = m_dwBytesToRecv; 

        if( dwSize == 0 )
        {
            dwSize = sizeof( CPacketHeader );
            dwSize -= m_dwOffset;
        }

        if( dwSize > MAX_BYTES_PER_TRANSFER )
        {
            // possibly a bad packet header
            ret = -EINVAL;
            break;
        }

        if( m_pBuf.IsEmpty() )
        {
            ret = m_pBuf.NewObj();
            if( ERROR( ret ) )
                break;
            m_pBuf->Resize(
                sizeof( CPacketHeader ) );
        }

        if( m_dwOffset < sizeof( CPacketHeader ) )
            dwBufOffset = m_dwOffset;
        else
            dwBufOffset = m_dwOffset -
                sizeof( CPacketHeader );

        ret = read( iFd,
            m_pBuf->ptr() + dwBufOffset, dwSize );

        if( ret == -1 && RETRIABLE( errno ) )
        {
            // no bytes to read
            ret = STATUS_PENDING;
            break;
        }
        else if( ret == -1 )
        {
            // asynchronous error
            ret = -errno;
            break;
        }
        else if( ret == 0 && bFirst )
        {
            // connection lost
            ret = -ENOTCONN;
            break;
        }

        bFirst = false;
        m_dwOffset += ret;
        if( m_dwOffset < sizeof( CPacketHeader ) )
        {
            // the header is yet to read
            ret = STATUS_PENDING;
            break;
        }
        else if( m_dwOffset == sizeof( CPacketHeader )
            && m_dwBytesToRecv == 0 )
        {
            // the header is read, and the payload
            // is yet to receive.
            ret = m_oHeader.Deserialize(
                m_pBuf->ptr(),
                sizeof( CPacketHeader ) );

            if( ERROR( ret ) )
                break;

            m_dwBytesToRecv = m_oHeader.m_dwSize;
            m_pBuf->Resize( m_dwBytesToRecv );

            // continue to read payload data
            continue;
        }
        else if( ret < ( gint32 )dwSize )
        {
            m_dwBytesToRecv -= ret;
            ret = STATUS_PENDING;
            break;
        }

        // the packet is done
        m_dwBytesToRecv = 0;
        m_dwOffset = 0;
        ret = 0;
        break;

    }while( 1 );

    return ret;
}


CRpcStream::CRpcStream(
    const IConfigDb* pCfg )
    : super(),
    m_atmSeqNo( 1 ),
    m_dwAgeSec( 0 )
{
    gint32 ret = 0;
    // parameters:
    // 0: an integer as Stream Id
    // 1: protocol id
    // propIoMgr: pointer to the io manager
    SetClassId( clsid( CRpcStream ) );
    do{
        if( pCfg == nullptr )
        {
            ret = -EINVAL;
            break;
        }
        CCfgOpener oCfg( pCfg );

        ret = oCfg.GetPointer(
            propIoMgr, m_pMgr );

        if( ERROR( ret ) )
            break;

        if( oCfg.exist( 0 ) )
        {
            ret = oCfg.GetIntProp( 0,
                ( guint32& )m_iStmId );

            if( ERROR( ret ) )
                break;

            if( IsReserveStm( m_iStmId ) )
            {
                m_iPeerStmId = m_iStmId;
            }
            else
            {
                m_iPeerStmId = -1;
            }
        }

        if( oCfg.exist( 1 ) )
        {
            guint32 dwVal = 0;

            ret = oCfg.GetIntProp( 1, dwVal );
            if( ERROR( ret ) )
                break;

            m_wProtoId = dwVal;
        }

        ObjPtr pObj;
        ret = oCfg.GetObjPtr(
            propSockPtr, pObj );

        if( ERROR( ret ) )
            break;

        m_pStmSock = pObj;
        if( m_pStmSock == nullptr )
        {
            ret = -EFAULT;
        }

    }while( 0 );

    if( ERROR( ret ) )
    {
        string strMsg = DebugMsg( ret, 
            "Error in CRpcStream ctor" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CRpcStream::QueuePacketOutgoing(
    CCarrierPacket* pPacket )
{
    if( pPacket == nullptr )
        return -EINVAL;

    m_queBufToSend.push_back(
        PacketPtr( pPacket ) );

    return 0;
}

gint32 CRpcStream::RecvPacket(
        CCarrierPacket* pPacketToRecv )
{
    if( pPacketToRecv == nullptr )
        return -EINVAL;

    PacketPtr pPacket( pPacketToRecv );
    m_queBufToRecv.push_back( pPacket );
    guint32 dwSize = m_queBufToRecv.size();

    m_atmSeqNo++;
    // we need to consider better flow control
    // method, like leaky bucket?
    if( dwSize > STM_MAX_RECV_PACKETS )
        m_queBufToRecv.pop_front();

    return 0;
}

gint32 CRpcStream::SetRespReadIrp(
    IRP* pIrp, CBuffer* pBuf )
{
    gint32 ret = 0;
    do{
        CStdRMutex oIrpLock( pIrp->GetLock() );
        ret = pIrp->CanContinue(
            IRP_STATE_READY );
        if( ERROR( ret ) )
        {
            // no irp to complete
            break;
        }

        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        guint32 dwMajorCmd = pIrp->MajorCmd();
        guint32 dwMinorCmd = pIrp->MinorCmd();

        if( dwMajorCmd == IRP_MJ_FUNC &&
            dwMinorCmd == IRP_MN_READ )
        {
            if( pCtx->m_pRespData.IsEmpty() )
                pCtx->SetRespData( pBuf );
            else
            {
                if( pBuf->GetObjId() !=
                    pCtx->m_pRespData->GetObjId() )
                {
                    guint8* pData =
                        ( guint8* )pBuf->ptr();
                    pCtx->m_pRespData->Append(
                        pData, pBuf->size() );
                }
                else
                {
                    // the resp is already set in
                    // GetReadIrpsToComp
                }
            }
            pCtx->SetStatus( 0 );
        }
        else
        {
            pCtx->SetStatus( -ENOTSUP );
            ret = -ENOTSUP;
        }

    }while( 0 );

    return ret;
}

gint32 CRpcStream::HandleReadIrp(
    IRP* pIrp )
{
    gint32 ret = 0;
    if( pIrp == nullptr )
        return -EINVAL;

    do{
        Refresh();
        // NOTE: no locking of IRP
        IrpPtr ptrIrp( pIrp );
        if( m_queBufToRecv.empty() )
        {
            m_queReadIrps.push_back( pIrp );
            ret = STATUS_PENDING;
            break;
        }
        else if( m_queReadIrps.size() > 0 )
        {
            m_queReadIrps.push_back( pIrp );
            ret = STATUS_PENDING;
            break;
        }

        PacketPtr ptrPacket =
            m_queBufToRecv.front();

        m_queBufToRecv.pop_front();

        CIncomingPacket* pPacket = ptrPacket;
        if( pPacket == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        BufPtr pBuf;
        IrpCtxPtr& pCtx = pIrp->GetCurCtx();
        ret = pPacket->GetPayload( pBuf );
        if( ERROR( ret ) )
            break;

        // get the user required size from the irp
        CfgPtr pCfg;
        ret = pCtx->GetReqAsCfg( pCfg );
        if( ERROR( ret ) )
            break;

        CParamList oReq( ( IConfigDb* )pCfg );
        guint32 dwReqSize = 0;

        ret = oReq.GetIntProp(
            propByteCount, dwReqSize );

        if( ERROR( ret ) )
        {
            dwReqSize = pBuf->size();
            ret = 0;
        }

        if( dwReqSize < pBuf->size() )
        {
            ret = -EBADMSG;
            break;
        }

        if( pCtx->m_pRespData.IsEmpty() )
        {
            pCtx->SetRespData( pBuf );
        }
        else
        {
            pCtx->m_pRespData->Append(
                ( guint8* )pBuf->ptr(),
                pBuf->size() );
        }

        guint32 dwCurSize =
            pCtx->m_pRespData->size();

        if( dwCurSize > dwReqSize )
        {
            ret = -EBADMSG;
            break;
        }
        else if( dwCurSize == dwReqSize )
            break;

        continue;

    }while( 1 );

    return ret;
}

gint32 CRpcStream::HandleWriteIrp(
    IRP* pIrp,
    BufPtr& pPayload,
    guint32 dwSeqNo )
{
    if( pIrp == nullptr
        || pPayload.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;
    Refresh();
    do{
        PacketPtr pPacket;
        pPacket.NewObj( clsid( COutgoingPacket ) );
        COutgoingPacket* pOp = pPacket;

        CPacketHeader oHeader;
        oHeader.m_iStmId = m_iStmId;
        oHeader.m_iPeerStmId = m_iPeerStmId;
        oHeader.m_dwFlags = 0;
        oHeader.m_wProtoId = m_wProtoId;
        if( dwSeqNo != 0 )
            oHeader.m_dwSeqNo = dwSeqNo;
        else
            oHeader.m_dwSeqNo = GetSeqNo();

        pOp->SetHeader( &oHeader );
        pOp->SetIrp( pIrp );

        ret = pOp->SetBufToSend( pPayload );
        if( ERROR( ret ) )
            break;

        ret = QueuePacketOutgoing( pOp );

    }while( 0 );

    return ret;
}

gint32 CRpcStream::GetReadIrpsToComp(
    vector< pair< IrpPtr, BufPtr > >& vecIrps )
{
    gint32 ret = 0;

    do{
        CStdRMutex oSockLock(
            m_pStmSock->GetLock() );

        if( m_queReadIrps.empty() ||
            m_queBufToRecv.empty() )
            break;

        IrpPtr pIrp = m_queReadIrps.front();
        m_queReadIrps.pop_front();
        oSockLock.Unlock();

        CStdRMutex oIrpLock( pIrp->GetLock() );
        ret = pIrp->CanContinue( IRP_STATE_READY );

        // discard the irp
        if( ERROR( ret ) )
            continue;

        oSockLock.Lock();
        if( m_queBufToRecv.empty() )
            break;

        PacketPtr pPacket = m_queBufToRecv.front();
        m_queBufToRecv.pop_front();

        BufPtr pBuf;
        ret = pPacket->GetPayload( pBuf );
        if( ERROR( ret ) )
            break;

        CfgPtr pReq;
        IrpCtxPtr pCtx = pIrp->GetTopStack();
        ret = pCtx->GetReqAsCfg( pReq );
        if( ERROR( ret ) )
            break;

        CParamList oReq(
            ( IConfigDb* )pReq );

        guint32 dwReqSize = 0;
        ret = oReq.GetIntProp(
            propByteCount, dwReqSize );

        if( ERROR( ret ) )
        {
            // no size reqeust, just return the
            // current packet
            dwReqSize = pBuf->size();
            ret = 0;
        }

        guint32 dwRecvSize = pBuf->size();
        if( dwReqSize < dwRecvSize )
        {
            ret = -EBADMSG;
            break;
        }
        else if( dwReqSize == dwRecvSize )
        {
            // move on
        }
        else
        {
            // dwReqSize > dwRecvSize
            if( pCtx->m_pRespData.IsEmpty() )
            {
                pCtx->SetRespData( pBuf );
                m_queReadIrps.push_front( pIrp );
                continue;
            }

            guint32 dwSizeToRecv = dwReqSize -
                pCtx->m_pRespData->size();

            if( dwSizeToRecv >= dwRecvSize )
            {
                ret = pCtx->m_pRespData->Append(
                    ( guint8* )pBuf->ptr(),
                    pBuf->size() );

                if( ERROR( ret ) )
                    break;

                if( dwSizeToRecv == dwRecvSize )
                {
                    pBuf = pCtx->m_pRespData;
                    pCtx->m_pRespData.Clear();
                }
                else
                {
                    // put the irp back
                    m_queReadIrps.push_front( pIrp );
                    continue;
                }
            }
            else
            {
                // dwSizeToRecv < dwRecvSize,
                // split the recv buffer
                ret = pCtx->m_pRespData->Append(
                    ( guint8* )pBuf->ptr(),
                    dwRecvSize );

                if( ERROR( ret ) )
                    break;

                guint32 dwResidual = 
                    pBuf->size() - dwRecvSize;

                memmove( pBuf->ptr() + dwRecvSize,
                    pBuf->ptr(), dwResidual );

                pBuf->Resize( dwResidual );
                pPacket->SetPayload( pBuf );
                m_queBufToRecv.push_front( pPacket );

                pBuf = pCtx->m_pRespData;
                pCtx->m_pRespData.Clear();
            }
        }
        vecIrps.push_back(
            pair< IrpPtr, BufPtr >( pIrp, pBuf ) );

    }while( 1 );

    return ret;
}

gint32 CRpcStream::StartSend(
    gint32 iFd, IrpPtr& pIrpToComplete )
{
    if( iFd < 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        PacketPtr pHead = m_queBufToSend.front();        
        COutgoingPacket* pOut = pHead;
        if( pOut == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        ret = pOut->StartSend( iFd );

        if( ret == STATUS_PENDING )
            break;

        pOut->GetIrp( pIrpToComplete );
        m_queBufToSend.pop_front();

    }while( 0 );

    return ret;
}

gint32 CRpcStream::RemoveIrpFromMap(
    IRP* pIrp )
{
    // NOTE: you should make sure the irp is
    // locked ahead of this call
    if( pIrp == nullptr )
        return -EINVAL;

    IrpCtxPtr pCtx = pIrp->GetTopStack();
    if( pCtx->GetMajorCmd() != IRP_MJ_FUNC )
        return -EINVAL;

    gint32 ret = 0;
    guint32 i = 0;

    switch( pCtx->GetMinorCmd() )
    {
    case IRP_MN_WRITE:
        {
            guint32 dwCount = m_queBufToSend.size();
            for( i = 0; i < dwCount; ++i )
            {
                PacketPtr& pPack = m_queBufToSend[ i ];
                COutgoingPacket* pOut = pPack;
                if( pOut == nullptr )
                    continue;
                IrpPtr ptrIrp;
                pOut->GetIrp( ptrIrp );
                if( pIrp == ptrIrp )
                {
                    m_queBufToSend.erase(
                        m_queBufToSend.begin() + i );
                    break;
                }
            }
            if( i == dwCount )
                ret = -ENOENT;
            break;
        }
    case IRP_MN_READ:
        {
            guint32 dwCount = m_queReadIrps.size();
            for( i = 0; i < dwCount; ++i )
            {
                IrpPtr ptrIrp = m_queReadIrps[ i ];
                if( pIrp == ptrIrp )
                {
                    m_queReadIrps.erase(
                        m_queReadIrps.begin() + i );
                    break;
                }
            }
            if( i == dwCount )
                ret = -ENOENT;
            break;
        }
    default:
        ret = -EINVAL; 
    }

    return ret;
}

gint32 CRpcStream::DispatchDataIncoming(
    CRpcSocketBase* pParentSock )
{
    if( pParentSock == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{

        CIoManager* pMgr = GetIoMgr();
        vector< pair< IrpPtr, BufPtr > > vecIrps;
        if( true )
        {
            ret = GetReadIrpsToComp( vecIrps );
        }

        for( auto&& oPair : vecIrps )
        {
            ret = SetRespReadIrp(
                oPair.first, oPair.second );

            if( SUCCEEDED( ret ) )
                pMgr->CompleteIrp( oPair.first );
        }

    }while( 0 );

    return ret;
}

gint32 CRpcStream::GetAllIrpsQueued(
    std::vector< IrpPtr >& vecIrps )
{

    gint32 ret = 0;
    for( auto&& pIrp : m_queReadIrps )
    {
        vecIrps.push_back( pIrp );
    }
    for( auto&& pPacket : m_queBufToSend )
    {
        COutgoingPacket* pOut = pPacket;
        if( pOut == nullptr )
            continue;
        IrpPtr pIrp;
        ret = pOut->GetIrp( pIrp );
        if( ERROR( ret ) )
            continue;
        vecIrps.push_back( pIrp );
    }
    return 0;
}

CRpcControlStream::CRpcControlStream(
    const IConfigDb* pCfg )
    : super( pCfg )
{
    SetClassId( clsid( CRpcControlStream ) );
}

gint32 CRpcControlStream::RecvPacket(
        CCarrierPacket* pPacketToRecv )
{
    if( pPacketToRecv == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        PacketPtr pPacket( pPacketToRecv );

        BufPtr pBuf;
        ret = pPacket->GetPayload( pBuf );
        if( ERROR( ret ) )
            break;

        CfgPtr pCfg( true );
        ret = pCfg->Deserialize( *pBuf );
        if( ERROR( ret ) )
        {
            pBuf.Clear();
            break;
        }

        m_queBufToRecv2.push_back( pCfg );
        guint32 dwSize = m_queBufToRecv2.size();

        // queue overflow
        if( dwSize > STM_MAX_RECV_PACKETS )
            m_queBufToRecv2.pop_front();

    }while( 0 );

    return 0;
}

gint32 CRpcControlStream::HandleListening(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    IrpCtxPtr& pCtx = pIrp->GetCurCtx();
    gint32 ret = 0;
    do{
        // NOTE: no locking of IRP
 
        CfgPtr pCfg;
        ret = pCtx->GetReqAsCfg( pCfg );
        if( ERROR( ret ) )
            break;

        CCfgOpener oCfg( ( IConfigDb* )pCfg );

        CMessageMatch* pMatch = nullptr;
        ret = oCfg.GetPointer(
            propMatchPtr, pMatch );

        if( ERROR( ret ) )
            break;
        
        if( pMatch->ToString() !=
            m_pStmMatch->ToString() )
        {
            ret = -EINVAL;
            break;
        }

        EnumMatchType dwMatchType =
            m_pStmMatch->GetType();

        std::deque< IrpPtr >* pQueIrp =
            &m_queReadIrps;

        guint32 dwReqType =
            DBUS_MESSAGE_TYPE_METHOD_CALL;

        if( dwMatchType == matchClient )
        {
            pQueIrp = &m_queReadIrpsEvt;
            dwReqType =
                DBUS_MESSAGE_TYPE_SIGNAL;
        }

        if( m_queBufToRecv2.empty() ||
            pQueIrp->size() )
        {
            pQueIrp->push_back( pIrp );
            ret = STATUS_PENDING;
            break;
        }

        deque< CfgPtr >::iterator itr =
            m_queBufToRecv2.begin();

        while( itr != m_queBufToRecv2.end() )
        {
            CfgPtr pCfg =
                m_queBufToRecv2.front();

            CReqOpener oReq( pCfg );
            guint32 dwInReqType = 0;
            ret = oReq.GetReqType( dwInReqType );
            if( ERROR( ret ) )
            {
                ++itr;
                continue;
            }

            if( dwReqType != dwInReqType )
            {
                ++itr;
                continue;
            }

            BufPtr pBuf( true );
            *pBuf = ObjPtr( pCfg );
            pCtx->SetRespData( pBuf );
            m_queBufToRecv2.erase( itr );
            ret = 0;
            break;
        }

        if( itr == m_queBufToRecv2.end() )
        {
            ret = STATUS_PENDING;
            pQueIrp->push_back( pIrp );
        }

    }while( 0 );

    if( ERROR( ret ) ||
        ret == STATUS_PENDING )
        return ret;

    return ret;
}

/**
* @name HandleIoctlIrp, to setup the parameters to
* send and put the packet to the sending queue
* @{ */
/**  @} */
gint32 CRpcControlStream::HandleIoctlIrp(
    IRP* pIrp )
{
    if( pIrp == nullptr )
        return -EINVAL;

    IrpCtxPtr pCtx = pIrp->GetCurCtx();
    if( pCtx->GetMajorCmd() != IRP_MJ_FUNC ||
        pCtx->GetMinorCmd() != IRP_MN_IOCTL )
        return -EINVAL;

    guint32 dwCtrlCode = pCtx->GetCtrlCode();
    gint32 ret = 0;

    if( dwCtrlCode == CTRLCODE_LISTENING )
        return HandleListening( pIrp );

    // get the config
    CfgPtr pCfg;
    ret = pCtx->GetReqAsCfg( pCfg );
    if( ERROR( ret ) )
        return ret;

    CReqOpener oCfg( ( IConfigDb* )pCfg );
    guint32 dwReqType = 0;
    ret = oCfg.GetReqType( dwReqType );
    if( ERROR( ret ) )
        return ret;

    BufPtr pPayload( true );
    Refresh();

    switch( dwCtrlCode )
    {
    case CTRLCODE_CLOSE_STREAM_PDO:
        {
            guint32 dwSeqNo = 0;

            if( dwReqType ==
                DBUS_MESSAGE_TYPE_METHOD_RETURN )
            {
                ret = oCfg.GetIntProp(
                    propSeqNo, dwSeqNo );

                ret = oCfg.GetCfg()->
                    Serialize( *pPayload );

                if( ERROR( ret ) )
                    break;
            }
            else
            {
                CReqBuilder oParams(
                    ( const IConfigDb* )pCfg ) ;

                dwSeqNo = GetSeqNo();
                ret = oParams.SetIntProp(
                    propSeqNo, dwSeqNo );

                // copy the sequence no to the
                // request cfg for reference in
                // the future. the SeqNo is only
                // generatated at this point
                ret = oCfg.SetIntProp(
                    propSeqNo, dwSeqNo );

                gint32 iPeerStm = 0;
                ret = oCfg.GetIntProp(
                    1, ( guint32& )iPeerStm );

                if( ERROR( ret ) )
                    break;

                oParams.Push( iPeerStm );

                IConfigDb* pCfg = oParams.GetCfg();
                ret = pCfg->Serialize( *pPayload );
                if( ERROR( ret ) )
                    break;
            }

            ret = super::HandleWriteIrp(
                pIrp, pPayload, dwSeqNo );

            break;
        }
    case CTRLCODE_OPEN_STREAM_PDO:
        {
            guint32 dwSeqNo = 0;

            if( dwReqType ==
                DBUS_MESSAGE_TYPE_METHOD_RETURN )
            {
                ret = oCfg.GetIntProp(
                    propSeqNo, dwSeqNo );

                ret = oCfg.GetCfg()->
                    Serialize( *pPayload );

                if( ERROR( ret ) )
                    break;
            }
            else
            {
                dwSeqNo = GetSeqNo();

                // copy the sequence #NO to the
                // request cfg for reference in
                // the future. the SeqNo is only
                // generatated at this point
                ret = oCfg.SetIntProp(
                    propSeqNo, dwSeqNo );

                IConfigDb* pCfg = oCfg.GetCfg();
                ret = pCfg->Serialize( *pPayload );
                if( ERROR( ret ) )
                    break;
            }

            ret = super::HandleWriteIrp(
                pIrp, pPayload, dwSeqNo );

            break;
        }
    case CTRLCODE_INVALID_STREAM_ID_PDO:
        {
            guint32 dwSeqNo = 0;

            ret = oCfg.GetIntProp(
                propSeqNo, dwSeqNo );

            if( ERROR( ret ) )
                break;

            IConfigDb* pCfg = oCfg.GetCfg();
            ret = pCfg->Serialize( *pPayload );
            if( ERROR( ret ) )
                break;

            ret = super::HandleWriteIrp(
                pIrp, pPayload, dwSeqNo );

            break;
        }
    case CTRLCODE_RMTSVR_OFFLINE_PDO:
    case CTRLCODE_RMTMOD_OFFLINE_PDO:
        {
            guint32 dwSeqNo = GetSeqNo();

            ret = oCfg.SetIntProp(
                propSeqNo, dwSeqNo );

            IConfigDb* pCfg = oCfg.GetCfg();
            ret = pCfg->Serialize( *pPayload );
            if( ERROR( ret ) )
                break;

            ret = super::HandleWriteIrp(
                pIrp, pPayload, dwSeqNo );

            break;
        }
    case CTRLCODE_REG_MATCH:
        {
            CStdRMutex oSockLock(
                m_pStmSock->GetLock() );

            if( !m_pStmMatch.IsEmpty() )
            {
                // unregister the old match first
                ret = -EEXIST;
                break;
            }

            ObjPtr pMatch;
            ret = oCfg.GetObjPtr(
                propMatchPtr, pMatch );

            if( ERROR( ret ) )
                break;

            m_pStmMatch = pMatch;
            if( m_pStmMatch.IsEmpty() )
                ret = -EINVAL;

            break;
        }
    case CTRLCODE_UNREG_MATCH:
        {
            CStdRMutex oSockLock(
                m_pStmSock->GetLock() );

            CMessageMatch* pMatch = nullptr;
            ret = oCfg.GetPointer(
                propMatchPtr, pMatch );

            if( ERROR( ret ) )
                break;

            
            if( pMatch->ToString() !=
                m_pStmMatch->ToString() )
            {
                ret = -EINVAL;
                break;
            }

            m_pStmMatch.Clear();
            break;
        }
    default:
        ret = -ENOTSUP;
        break;
    }

    return ret;
}

gint32 CRpcControlStream::QueueIrpForResp(
    IRP* pIrp )
{
    // NOTE: you should make sure the irp is
    // locked from outside
    if( pIrp == nullptr )
        return -EINVAL;

    IrpCtxPtr pCtx = pIrp->GetTopStack();
    if( pCtx->GetMajorCmd() != IRP_MJ_FUNC ||
        pCtx->GetMinorCmd() != IRP_MN_IOCTL )
        return -EINVAL;

    guint32 dwCtrlCode = pCtx->GetCtrlCode();

    if( dwCtrlCode != CTRLCODE_CLOSE_STREAM_PDO &&
        dwCtrlCode != CTRLCODE_OPEN_STREAM_PDO &&
        dwCtrlCode != CTRLCODE_LISTENING )
        return -ENOTSUP;

    gint32 ret = 0;
    do{
        if( dwCtrlCode == CTRLCODE_LISTENING )
        {
            EnumMatchType dwType =
                m_pStmMatch->GetType();

            if( ERROR( ret ) )
                break;

            deque< IrpPtr >* pQueIrps = &m_queReadIrps;
            if( dwType == matchClient )
                pQueIrps = &m_queReadIrpsEvt;

            pQueIrps->push_back( pIrp );
            break;
        }

        CfgPtr pCfg;
        ret = pCtx->GetReqAsCfg( pCfg );
        if( ERROR( ret ) )
            break;

        CCfgOpener oCfg( ( IConfigDb* )pCfg );
        guint32 dwSeqNo = 0;
        ret = oCfg.GetIntProp(
            propSeqNo, dwSeqNo );

        if( ERROR( ret ) )
            break;

        bool bFound = false;
        for( auto pCfg : m_queBufToRecv2 )
        {
            // it is possible the reply arrives ahead
            // of this irp geting queued here
            CReqOpener oCfg( ( IConfigDb* )pCfg );

            guint32 dwType = 0;
            ret = oCfg.GetReqType( dwType );
            if( ERROR( ret ) )
            {
                ret = 0;
                continue;
            }

            if( dwType != DBUS_MESSAGE_TYPE_METHOD_RETURN )
                continue;

            guint32 dwSeqNo = 0;
            ret = oCfg.GetIntProp(
                propSeqNo, dwSeqNo );

            // discard the packet
            if( ERROR( ret ) )
            {
                ret = 0;
                continue;
            }

            bFound = true;
            BufPtr pBuf( true );
            *pBuf = ObjPtr( pCfg );
            pCtx->SetRespData( pBuf );
            pCtx->SetStatus( 0 );
            break;
        }

        if( !bFound )
        {
            m_mapIrpsForResp[ dwSeqNo ] =
                IrpPtr( pIrp );
        }

    }while( 0 );

    return ret;
}

gint32 CRpcControlStream::GetReadIrpsToComp(
    vector< pair< IrpPtr, BufPtr > >& vecIrps )
{
    // this method override the base method, and
    // goes beyond READ irps, and it will also
    // scan the req map to find the irp to match
    // the incoming packet
    gint32 ret = 0;

    CStdRMutex oSockLock(
        m_pStmSock->GetLock() );

    if( m_queReadIrps.size() +
        m_queReadIrpsEvt.size() +
        m_mapIrpsForResp.size() == 0 ||
        m_queBufToRecv2.size() == 0 )
        return -ENOENT;

    deque< CfgPtr > quePktPutback;
    while( m_queBufToRecv2.size() != 0 )
    {
        CfgPtr pCfg = m_queBufToRecv2.front();
        m_queBufToRecv2.pop_front();

        CReqOpener oCfg( ( IConfigDb* )pCfg );

        guint32 dwType = 0;
        ret = oCfg.GetReqType( dwType );
        if( ERROR( ret ) )
            continue;

        IrpPtr pIrp;
        if( dwType == DBUS_MESSAGE_TYPE_METHOD_CALL )
        {
            if( m_queReadIrps.empty() )
            {
                quePktPutback.push_back( pCfg );
                continue;
            }

            pIrp = m_queReadIrps.front();
            m_queReadIrps.pop_front();
        }
        else if( dwType == DBUS_MESSAGE_TYPE_SIGNAL )
        {
            if( m_queReadIrpsEvt.empty() )
            {
                quePktPutback.push_back( pCfg );
                continue;
            }

            pIrp = m_queReadIrpsEvt.front();
            m_queReadIrpsEvt.pop_front();
        }
        else if( dwType == DBUS_MESSAGE_TYPE_METHOD_RETURN )
        {
            guint32 dwSeqNo = 0;

            ret = oCfg.GetIntProp(
                propSeqNo, dwSeqNo );

            // discard the packet
            if( ERROR( ret ) )
                continue;

            // discard the packet, since no irp is
            // waiting for it, probably the irp is
            // canceled
            if( m_mapIrpsForResp.find( dwSeqNo )
                == m_mapIrpsForResp.end() )
                continue;

            pIrp = m_mapIrpsForResp[ dwSeqNo ];
            m_mapIrpsForResp.erase( dwSeqNo );
        }

        BufPtr pDeserialized( true );
        *pDeserialized = ObjPtr( pCfg );
        vecIrps.push_back( pair< IrpPtr, BufPtr >
            ( pIrp, pDeserialized ) );
    }

    m_queBufToRecv2 = quePktPutback;

    return ret;
}

gint32 CRpcControlStream::SetRespReadIrp(
    IRP* pIrp, CBuffer* pBuf )
{
    gint32 ret = 0;
    do{
        CStdRMutex oIrpLock( pIrp->GetLock() );
        ret = pIrp->CanContinue(
            IRP_STATE_READY );

        if( ERROR( ret ) )
            break;

        guint32 dwMajorCmd = pIrp->MajorCmd();
        guint32 dwMinorCmd = pIrp->MinorCmd();

        IrpCtxPtr& pCtx = pIrp->GetTopStack();

        if( dwMajorCmd != IRP_MJ_FUNC ||
            dwMinorCmd != IRP_MN_IOCTL )
        {
            ret = -EINVAL;
            break;
        }

        switch( pCtx->GetCtrlCode() )
        {
        case CTRLCODE_OPEN_STREAM_PDO:
        case CTRLCODE_CLOSE_STREAM_PDO:
        case CTRLCODE_LISTENING:
            {
                pCtx->SetRespData( pBuf );
                pCtx->SetStatus( 0 );
                break;
            }
        default:
            ret = -ENOTSUP;
            break;
        }

    }while( 0 );

    return ret;
}

/**
* @name RemoveIoctlRequest: remove irp of a client
* request from the internal queue or map
* @{ */
/**  @} */

gint32 CRpcControlStream::RemoveIoctlRequest(
        IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        EnumIoctlStat iState = reqStatInvalid;

        ret = CTcpStreamPdo::GetIoctlReqState(
            pIrp, iState );

        if( ERROR( ret ) )
            break;

        if( iState == reqStatOut )
        {
            // let's find the irp in the sending
            // queue
            deque< PacketPtr >:: iterator itr =
                m_queBufToSend.begin();

            while( itr != m_queBufToSend.end() )
            {
                COutgoingPacket* pOut = *itr;
                if( pOut == nullptr )
                {
                    ++itr;
                    continue;
                }

                IrpPtr pIrpToRmv;
                ret = pOut->GetIrp( pIrpToRmv );
                if( ERROR( ret ) )
                {
                    ++itr;
                    continue;
                }

                if( pIrp == pIrpToRmv &&
                    itr != m_queBufToSend.begin() )
                {
                    m_queBufToSend.erase( itr );
                    break;
                }
                else if( pIrp == pIrpToRmv )
                {
                    if( !pOut->IsSending() )
                    {
                        m_queBufToSend.pop_front();
                    }
                    else
                    {
                        // NOTE: we cannot remove
                        // a packet in the sending
                        // process, so we detach
                        // the irp from the
                        // outgoing packet
                        pOut->SetIrp( nullptr );
                    }
                    break;
                }
                ++itr;
            }
            if( itr == m_queBufToSend.end() )
            {
                ret = -ENOENT;
                break;
            }
        }
        else if( iState == reqStatIn )
        {
            CfgPtr pCfg;
            ret = pCtx->GetReqAsCfg( pCfg );
            if( ERROR( ret ) )
                break;

            CCfgOpener oCfg( ( IConfigDb* )pCfg );

            guint32 dwSeqNo = 0;

            ret = oCfg.GetIntProp(
                propSeqNo, dwSeqNo );

            if( ERROR( ret ) )
                break;

            if( m_mapIrpsForResp.find( dwSeqNo ) !=
                m_mapIrpsForResp.end() )
            {
                m_mapIrpsForResp.erase( dwSeqNo );
            }
            else
            {
                ret = -ENOENT;
            }
        }
        else
        {
            ret = -EINVAL;
        }

    }while( 0 );

    return ret;
}

gint32 CRpcControlStream::RemoveIrpFromMap(
        IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        switch( pCtx->GetCtrlCode() )
        {
        case CTRLCODE_OPEN_STREAM_PDO:
        case CTRLCODE_CLOSE_STREAM_PDO:
        case CTRLCODE_INVALID_STREAM_ID_PDO:
            {
                ret = RemoveIoctlRequest( pIrp );
                break;
            }
        case CTRLCODE_LISTENING:
            {
                EnumMatchType dwType =
                    m_pStmMatch->GetType();

                deque< IrpPtr >* pQueIrps =
                    &m_queReadIrps;

                if( dwType == matchClient )
                    pQueIrps = &m_queReadIrpsEvt;

                deque< IrpPtr >::iterator itr = 
                    pQueIrps->begin();

                while( itr != pQueIrps->end() )
                {
                    if( pIrp == *itr )
                    {
                        pQueIrps->erase( itr );
                        break;
                    }
                    ++itr;
                }
                if( itr == pQueIrps->end() )
                    ret = -ENOENT;

                break;
            }
        default:
            {
                ret = -ENOTSUP;
                break;
            }
        }
    }while( 0 );

    return ret;
}

gint32 CRpcControlStream::GetAllIrpsQueued(
    std::vector< IrpPtr >& vecIrps )
{
    super::GetAllIrpsQueued( vecIrps );
    for( auto&& oPair : m_mapIrpsForResp )
    {
        vecIrps.push_back( oPair.second );
    }
    for( auto&& pIrp : m_queReadIrpsEvt )
    {
        vecIrps.push_back( pIrp );
    }
    return 0;
}

gint32 COutgoingPacket::SetHeader(
    CPacketHeader* pHeader )
{
    if( pHeader == nullptr )
        return -EINVAL;

    m_oHeader.m_iStmId = pHeader->m_iStmId;
    m_oHeader.m_dwFlags = pHeader->m_dwFlags;
    m_oHeader.m_iPeerStmId = pHeader->m_iPeerStmId;
    m_oHeader.m_dwSeqNo = pHeader->m_dwSeqNo;
    m_oHeader.m_wProtoId = pHeader->m_wProtoId;
    m_oHeader.m_wReserved = pHeader->m_wReserved;

    return 0;
}

gint32 COutgoingPacket::SetIrp( IRP* pIrp )
{
    if( pIrp == nullptr
        || pIrp->GetStackSize() == 0 )
        return -EINVAL;

    m_pIrp = pIrp;
    return 0;
}

gint32 COutgoingPacket::SetBufToSend(
    CBuffer* pBuf )
{
    if( pBuf == nullptr )
        return -EINVAL;

    if( pBuf->empty() ||
        pBuf->size() > MAX_BYTES_PER_TRANSFER )
        return -EINVAL;

    m_pBuf = pBuf;
    m_oHeader.m_dwSize = m_pBuf->size();
    m_dwBytesToSend =  m_oHeader.m_dwSize
        + sizeof( CPacketHeader );

    m_dwOffset = 0;
    m_oHeader.hton();
    
    return 0;
}

gint32 COutgoingPacket::StartSend(
    gint32 iFd )
{
    if( iFd < 0 )
        return -EINVAL;

    gint32 ret = 0;
    if( m_dwBytesToSend == 0 )
        return 0;

    if( m_dwBytesToSend > MAX_BYTES_PER_TRANSFER )
        return -EINVAL;

    do{
        if( m_dwOffset < sizeof( CPacketHeader ) )
        {
            guint32 dwSize = 
                sizeof( CPacketHeader ) - m_dwOffset;

            char* pStart = 
                ( ( char* )&m_oHeader ) + m_dwOffset;

            ret = send( iFd, pStart, dwSize, 0 );
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

            m_dwOffset += ret;
            m_dwBytesToSend -= ret;
            if( m_dwOffset == sizeof( CPacketHeader ) )
            {
                // recover from the eariler hton
                m_oHeader.ntoh();
            }
            else if( m_dwOffset < sizeof( CPacketHeader ) )
                 continue;

            ret = 0;
        }

        if( m_dwOffset >= sizeof( CPacketHeader ) )
        {
            guint32 dwActOffset = m_dwOffset
                - sizeof( CPacketHeader );

            guint32 dwSize = m_pBuf->size()
                - dwActOffset;

            if( dwSize > RPC_MAX_BYTES_PACKET )
                dwSize = RPC_MAX_BYTES_PACKET;

            char* pStart = ( ( char* )m_pBuf->ptr() )
                + dwActOffset;

            ret = send( iFd, pStart, dwSize, 0 );
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

            m_dwOffset += ret;
            m_dwBytesToSend -= ret;
            ret = 0;
        }

        if( m_dwBytesToSend > 0 )
        {
            continue;
        }
        else
        {
            // succeeded
        }
        break;

    }while( 1 );

    return ret;
}

CRpcListeningSock::CRpcListeningSock(
    const IConfigDb* pCfg )
    : super( pCfg )
{
    gint32 ret = 0;

    SetClassId( clsid( CRpcListeningSock ) );

    do{
        if( pCfg == nullptr )
        {
            ret = -EINVAL;
            break;
        }

    }while( 0 );

    if( ERROR( ret ) )
    {
        string strMsg = DebugMsg( ret,
            "Error in CRpcListeningSock ctor" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CRpcListeningSock::Connect()
{
    // setup a socket to listen to the connection
    // request
    gint32 ret = 0;
    do{
        ret = socket( AF_INET,
            SOCK_STREAM | SOCK_NONBLOCK, 0 );

        if ( ret == -1 )
        {
            ret = -errno;
            break;
        }

        m_iFd = ret;

        gint32 dwReuseAddr = 1;
        ret = setsockopt( m_iFd, SOL_SOCKET,
            SO_REUSEADDR, &dwReuseAddr,
            sizeof( dwReuseAddr ) );

        CCfgOpener oCfg(
            ( IConfigDb* )m_pCfg );

        string strIpAddr;
        ret = oCfg.GetStrProp(
            propIpAddr, strIpAddr );

        if( ERROR( ret ) )
            strIpAddr = "0.0.0.0";

        guint32 dwPortNum = 0;

        ret = oCfg.GetIntProp(
            propSrcTcpPort, dwPortNum );

        if( ERROR( ret ) )
            dwPortNum = RPC_SVR_PORTNUM;

        /*  Connect to the remote host. */
        sockaddr_in sin;

        memset( &sin, 0, sizeof( sin ) );
        sin.sin_family = AF_INET;
        sin.sin_port = htons(
            ( gint16 )dwPortNum );

        sin.sin_addr.s_addr =
            inet_addr( strIpAddr.c_str() );

        ret = bind( m_iFd,
            ( sockaddr* ) &sin, sizeof( sin ) );

        if( ret == -1 )
        {
            ret = -errno;
            break;
        }

        ret = AttachMainloop();
        if( ERROR( ret ) )
            break;

        ret = listen( m_iFd, 5 );
        if( ret == -1 )
        {
            ret = -errno;
            break;
        }
        StartWatch( false );

    }while( 0 );

    if( ERROR( ret ) )
    {
        DetachMainloop();
        CloseSocket();
    }

    return ret;
}

gint32 CRpcListeningSock::Start()
{
    gint32 ret = 0;

    CStdRMutex oSockLock( GetLock() );
    if( GetState() != sockInit )
        return ERROR_STATE;

    // we need an active connect
    ret = Connect();
    if( ERROR( ret ) )
        return ret;

    SetState( sockStarted );
    return 0;
}

gint32 CRpcListeningSock::OnConnected()
{
    // a new connection coming
    gint32 ret = 0;
    gint32 newFd = -1;
    do{
        newFd = accept( m_iFd, NULL, NULL );
        if( newFd == -1 )
        {
            if( !RETRIABLE( errno ) )
            {
                ret = -errno;
                break;
            }
            else
            {
                ret = STATUS_PENDING;
            }
            break;
        }
        StartWatch();
        // let's schedule a task to start the port
        // building process
        m_pParentPort->OnEvent(
            eventNewConn, newFd, 0, nullptr );

    }while( newFd != -1 );

    return ret;
}

gint32 CRpcListeningSock::OnError(
    gint32 iError )
{
    return 0;
}

gint32 CRpcListeningSock::DispatchSockEvent(
     GIOCondition dwCondition )
{
    gint32 ret = 0; 
    EnumSockState iState = sockInvalid;
    CStdRMutex oSockLock( GetLock() );

    iState = GetState();
    if( iState == sockStopped )
        return ERROR_STATE;

    ret = GetAsyncErr();
    if( ERROR( ret ) )
    {
        OnError( ret );
        return ret;
    }

    switch( iState )
    {
    case sockStarted:
        {
            if( dwCondition == G_IO_IN )
            {
                OnConnected();
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

gint32 CStmSockConnectTask::RemoveConnTimer()
{
    gint32 ret = 0;
    do{
        if( m_iTimerId == 0 )
            break;

        CIoManager* pMgr = GetIoMgr();
        if( unlikely( pMgr == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }

        CTimerService& otsvc =
            pMgr->GetUtils().GetTimerSvc();

        otsvc.RemoveTimer( m_iTimerId );
        m_iTimerId = 0;

    }while( 0 );

    return ret;
}

gint32 CStmSockConnectTask::AddConnTimer()
{
    gint32 ret = 0;
    do{
        // the connection could be long, restart
        // the irp timer.
        PIRP pIrp = nullptr;
        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );

        ret = oCfg.GetPointer( propIrpPtr, pIrp );
        if( ERROR( ret ) )
            break;

        pIrp->ResetTimer();
        CIoManager* pMgr = GetIoMgr();
        if( unlikely( pMgr == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }

        guint32 dwTimeWait =
            pIrp->m_dwTimeoutSec;

        if( unlikely( dwTimeWait == 0 ) )
            dwTimeWait = PORT_START_TIMEOUT_SEC - 5;

        CTimerService& otsvc =
            pMgr->GetUtils().GetTimerSvc();

        m_iTimerId = otsvc.AddTimer(
            dwTimeWait, this, eventRetry );

    }while( 0 );

    return ret;
}

gint32 CStmSockConnectTask::OnStart(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    SetConnState( connProcess );

    gint32 ret = 0;
    do{
        CCfgOpener oCfg( ( IConfigDb* )m_pCtx );
        ret = oCfg.SetObjPtr( propIrpPtr,
            ObjPtr( pIrp ) );

        CRpcStreamSock* pSock = nullptr;
        ret = GetSockPtr( pSock );
        if( ERROR( ret ) )
            break;

        ret = AddConnTimer();
        if( ERROR( ret ) )
            break;

        ret = pSock->Start();
        if( ret != STATUS_PENDING )
        {
            RemoveConnTimer();
        }

    }while( 0 );
    
    return ret;
}

gint32 CStmSockConnectTask::Retry()
{
    gint32 ret = DecRetries( nullptr );

    if( ERROR( ret ) )
        return -ETIMEDOUT;

    CCfgOpener oCfg( ( IConfigDb* )m_pCtx );

    string strIpAddr;
    ret = oCfg.GetStrProp(
        propIpAddr, strIpAddr );

    if( ERROR( ret ) )
        return ret;
    
    CRpcStreamSock* pSock = nullptr;
    ret = GetSockPtr( pSock );
    if( ERROR( ret ) )
        return ret;

    AddConnTimer();
    ret = pSock->ActiveConnect( strIpAddr );
    if( ret != STATUS_PENDING )
    {
        RemoveConnTimer();
    }

    return ret;
}

gint32 CStmSockConnectTask::OnError(
    gint32 iRet )
{
    gint32 ret = 0;
    switch( -iRet )
    {
    case ETIMEDOUT:
        {
            ret = Retry();
            break;
        }
    /* Connection refused */
    case ECONNREFUSED:
    /* Host is down */
    case EHOSTDOWN:
    /* No route to host */
    case EHOSTUNREACH:
        {
            // retry
            ret = iRet;
            break;
        }
    /* Operation already in progress */
    case EALREADY:
    /* Operation now in progress */
    case EINPROGRESS:
        {
            ret = STATUS_PENDING;
            break;
        }
    default:
        {
            ret = iRet;
        }
        break;;
    }
    return ret;
}

gint32 CStmSockConnectTask::OnConnected()
{
    CRpcStreamSock* pSock = nullptr;
    gint32 ret = GetSockPtr( pSock );
    if( ERROR( ret ) )
        return ret;

    return pSock->Start_bh();
}

gint32 CStmSockConnectTask::SetSockState(
    EnumSockState iState )
{
    CRpcStreamSock* pSock = nullptr;
    gint32 ret = GetSockPtr( pSock );
    if( ERROR( ret ) )
        return ret;

    return pSock->SetState( iState );
}

gint32 CStmSockConnectTask::GetMasterIrp(
    IRP*& pIrp )
{ 
    CCfgOpener oCfg( ( IConfigDb* )m_pCtx );
    ObjPtr pObj;

    gint32 ret = oCfg.GetObjPtr(
        propIrpPtr, pObj );

    if( ERROR( ret ) )
        return ret;

    pIrp = pObj;
    if( pIrp == nullptr )
        return -EFAULT;

    return 0;
}

gint32 CStmSockConnectTask::DecRetries(
    guint32* pdwRetries )
{
    CCfgOpener oCfg( ( IConfigDb* )m_pCtx );

    guint32 dwRetries = 0;

    gint32 ret = oCfg.GetIntProp(
        propRetries, dwRetries );

    if( ERROR( ret ) )
        return ret;

    if( dwRetries == 0 )
        return -EOVERFLOW;

    --dwRetries;

    ret = oCfg.SetIntProp(
        propRetries, dwRetries );

    if( ERROR( ret ) )
        return ret;

    if( pdwRetries != nullptr )
        *pdwRetries = dwRetries;

    return 0;
}

gint32 CStmSockConnectTask::GetRetries(
    guint32& dwRetries )
{
    CCfgOpener oCfg( ( IConfigDb* )m_pCtx );

    gint32 ret = oCfg.GetIntProp(
        propRetries, dwRetries );

    if( ERROR( ret ) )
        return ret;

    return 0;
}

gint32 CStmSockConnectTask::GetSockPtr(
    CRpcStreamSock*& pSock )
{
    CCfgOpener oCfg( ( IConfigDb* )m_pCtx );
    ObjPtr pObj;

    gint32 ret = oCfg.GetObjPtr(
        propSockPtr, pObj );

    if( ERROR( ret ) )
        return ret;

    pSock = pObj;
    if( pSock == nullptr )
        return -EFAULT;

    return 0;
}

gint32 CStmSockConnectTask::CompleteTask(
    gint32 iRet )
{
    IRP* pIrp = nullptr;
    gint32 ret = 0;

    if( GetConnState() != connProcess )
        return ERROR_STATE;

    do{
        RemoveConnTimer();
        ret = GetMasterIrp( pIrp );
        if( ERROR( ret ) )
            break;

        if( pIrp == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        else
        {
            CStdRMutex oIrpLock(
                pIrp->GetLock() );

            ret = pIrp->CanContinue(
                IRP_STATE_READY);

            if( ERROR( ret ) )
                break;

            IrpCtxPtr& pCtx =
                pIrp->GetTopStack();

            pCtx->SetStatus( iRet );
        }

        CIoManager* pMgr = GetIoMgr();
        if( pMgr == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        pMgr->CompleteIrp( pIrp );

    }while( 0 );

    if( SUCCEEDED( iRet ) )
        SetConnState( connCompleted );
    else
        SetConnState( connStopped );

    m_pCtx->RemoveProperty( propIrpPtr );
    m_pCtx->RemoveProperty( propSockPtr );
    m_pCtx->RemoveProperty( propIoMgr );

    return ret;
}

gint32 CStmSockConnectTask::StopTask(
    gint32 iRet )
{
    if( GetConnState() != connProcess )
        return ERROR_STATE;

    CRpcStreamSock* pSock = nullptr;
    gint32 ret = GetSockPtr( pSock );

    if( SUCCEEDED( ret ) )
    {
        pSock->Stop();
    }

    CompleteTask( iRet );
    return 0;
}


CIoManager* CStmSockConnectTask::GetIoMgr()
{
    CCfgOpener oCfg( ( IConfigDb* )m_pCtx );
    ObjPtr pObj;

    CIoManager* pMgr = nullptr;
    gint32 ret = oCfg.GetPointer( propIoMgr, pMgr );

    if( ERROR( ret ) )
        return nullptr;

    return pMgr;
}

gint32 CStmSockConnectTask::operator()(
    guint32 dwContext )
{
    gint32 ret = 0;

    EnumEventId iEvent =
        ( EnumEventId )dwContext; 

    CStdRMutex oTaskLock( GetLock() );

    gint32 iState = GetConnState();

    if( iState == connStopped ||
        iState == connCompleted )
        return ERROR_STATE;

    CRpcStreamSock* pSock = nullptr;
    ret = GetSockPtr( pSock );

    if( ERROR( ret ) )
        return ret;

    switch( iEvent )
    {
    case eventStart:
        {
            IrpPtr pIrp;
            if( iState != connInit )
            {
                ret = ERROR_STATE;
                break;
            }
            ret = GetIrpFromParams( pIrp );
            if( ERROR( ret ) )
                break;

            ret = OnStart( pIrp );
            if( SUCCEEDED( ret ) )
            {
                ret = OnConnected();
            }
            break;
        }
    case eventNewConn:
        {
            if( iState != connProcess )
            {
                ret = ERROR_STATE;
                break;
            }
            guint32 dwConnFlags = ( G_IO_OUT );

            vector< guint32 > vecParams;
            ret = GetParamList( vecParams );
            if( ERROR( ret ) )
                break;

            guint32 dwCondition = vecParams[ 1 ];

            if( dwCondition == dwConnFlags )
                ret = OnConnected();
            else
                ret = ERROR_STATE;

            break;
        }
    case eventConnErr:
        {
            if( iState != connProcess )
            {
                ret = ERROR_STATE;
                break;
            }
            vector< guint32 > vecParams;
            ret = GetParamList( vecParams );
            if( ERROR( ret ) )
                break;

            guint32 dwParam1 = vecParams[ 1 ];
            ret = OnError( ( gint32 )dwParam1 ); 

            break;
        }

    // from CPort::PreStop
    case eventStop:
        {
            ret = ERROR_PORT_STOPPED;
            break;
        }
    case eventTimeout:
        {
            m_iTimerId = 0;
            vector< guint32 > vecParams;
            ret = GetParamList( vecParams );
            if( ERROR( ret ) )
                break;

            EnumEventId iSubEvt =
                ( EnumEventId )vecParams[ 1 ];
            if( iSubEvt != eventRetry )
            {
                ret = -ENOTSUP;
                break;
            }

            ret = OnError( -ETIMEDOUT );
            break;
        }
    // only from CTcpStreamPdo::CancelStartIrp
    case eventCancelTask:
        {
            // remove the master irp, to prevent
            // the irp from being completed again
            m_pCtx->RemoveProperty( propIrpPtr );

            vector< guint32 > vecParams;
            ret = GetParamList( vecParams );
            if( ERROR( ret ) )
                break;
            ret = ( gint32 )vecParams[ 1 ];
            break;
        }
    default:
        {
            ret = -ENOTSUP;
            break;
        }
    }

    if( SUCCEEDED( ret ) )
    {
        CompleteTask( ret );
    }
    else if( ERROR( ret ) )
    {
        StopTask( ret );
    }

    // BUGBUG: even STATE_PENDING needs to be set
    // to let the caller know the call result. the
    // return value is not passed on by
    // CTasklet::OnEvent 
    return SetError( ret );
}

gint32 CStmSockConnectTask::NewTask(
    CIoManager* pMgr,
    CRpcStreamSock* pSock,
    gint32 iRetries,
    TaskletPtr& pTask )
{
    CCfgOpener oCfg;
    if( pMgr == nullptr ||
        pSock == nullptr ||
        iRetries <= 0 )
        return -EINVAL;

    gint32 ret = 0;

    do{
        oCfg.SetPointer( propIoMgr, pMgr );

        ret = oCfg.SetObjPtr(
            propSockPtr, ObjPtr( pSock ) );

        if( ERROR( ret ) )
            break;

        ret = oCfg.SetIntProp(
            propRetries, iRetries );

        if( ERROR( ret ) )
            break;

        ret = oCfg.CopyProp(
            propIpAddr, pSock );    

        if( ERROR( ret ) )
            break;

        ret = pTask.NewObj(
            clsid( CStmSockConnectTask ),
            oCfg.GetCfg() );

    }while( 0 );

    return ret;
}

gint32 CStmSockInvalStmNotifyTask::Process(
    guint32 dwContext )
{
    // parameters: 
    //
    // propStreamId: the invalid stream id
    // propPeerStmId:
    // propSeqNo:
    // propProtoId
    //
    // propPortPtr: the port to send this request
    // to
    //
    // propIoMgr: the pointer to the io manager
    //
    gint32 ret = 0;
    do{
        CCfgOpener oCfg( ( IConfigDb* )*this );
        gint32 iStreamId = 0;

        ret = oCfg.GetIntProp(
            propStreamId, ( guint32& )iStreamId );

        if( ERROR( ret ) )
            break;

        ObjPtr pObj;
        ret = oCfg.GetObjPtr(
            propPortPtr, pObj );

        if( ERROR( ret ) )
            break;

        IPort* pPort = pObj;
        CIoManager* pMgr = nullptr;
        ret = oCfg.GetPointer( propIoMgr, pMgr );

        if( ERROR( ret ) )
            break;

        CReqBuilder oParams;

        oParams.SetIfName(
            DBUS_IF_NAME( IFNAME_TCP_BRIDGE ) );

        oParams.SetObjPath(
            DBUS_OBJ_PATH( OBJNAME_TCP_BRIDGE ) );

        oParams.SetSender(
            DBUS_DESTINATION( pMgr->GetModName() ) );

        oParams.SetMethodName(
            BRIDGE_EVENT_INVALIDSTM );

        oParams.SetIntProp( propStreamId,
            TCP_CONN_DEFAULT_CMD );

        // send this notification through the
        // default command channel
        guint32 dwVal = 0;

        // the offending stream id
        oParams.Push( iStreamId );

        oCfg.GetIntProp( propPeerStmId, dwVal );
        oParams.Push( dwVal );

        oCfg.GetIntProp( propSeqNo, dwVal );
        oParams.SetIntProp( propSeqNo, dwVal );
        oParams.Push( dwVal );

        oCfg.GetIntProp( propProtoId, dwVal );
        oParams.Push( dwVal );

        oParams.SetIntProp( propCmdId,
            CTRLCODE_INVALID_STREAM_ID_PDO );

        oParams.SetCallFlags( CF_NON_DBUS |
            CF_ASYNC_CALL |
            DBUS_MESSAGE_TYPE_SIGNAL );
        
        // start to read from the default dbus
        // stream
        IrpPtr pIrp( true );
        ret = pIrp->AllocNextStack( nullptr );
        if( ERROR( ret ) )
            break;

        IrpCtxPtr& pCtx = pIrp->GetTopStack(); 
        pCtx->SetMajorCmd( IRP_MJ_FUNC );
        pCtx->SetMinorCmd( IRP_MN_IOCTL );
        pCtx->SetCtrlCode( CTRLCODE_INVALID_STREAM_ID_PDO );

        // NOTE: this request is one-way only, we
        // don't expect a response and we don't
        // set the callback and the timer.
        pCtx->SetIoDirection( IRP_DIR_OUT ); 

        BufPtr pBuf( true );
        *pBuf = ObjPtr( oParams.GetCfg() );

        pCtx->SetReqData( pBuf );
        pIrp->SetSyncCall( false );

        // NOTE: this irp will only go through the
        // stream pdo, and not come down all the
        // way from the top of the port stack
        ret = pMgr->SubmitIrpInternal(
            pPort, pIrp, false );

        if( ret == ERROR_STATE )
            break;

        // retry this task if not at the right time
        if( ret == -EAGAIN )
            ret = STATUS_MORE_PROCESS_NEEDED;

        if( ERROR( ret ) )
            break;

    }while( 0 );

    return ret;
}
