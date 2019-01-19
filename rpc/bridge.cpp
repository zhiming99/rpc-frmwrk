/*
 * =====================================================================================
 *
 *       Filename:  bridge.cpp
 *
 *    Description:  implementation of CRpcTcpBridge and CRpcTcpBridgeProxy
 *
 *        Version:  1.0
 *        Created:  07/29/2017 09:57:27 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi(woodhead99@gmail.com )
 *   Organization:
 *
 * =====================================================================================
 */


#include "configdb.h"
#include "defines.h"
#include "rpcroute.h"
#include "dbusport.h"
#include "tcpport.h"
#include "emaphelp.h"
#include "ifhelper.h"

using namespace std;

CRpcTcpBridgeProxy::CRpcTcpBridgeProxy(
    const IConfigDb* pCfg )
    : CInterfaceProxy( pCfg ),
    super( pCfg ),
    CRpcTcpBridgeShared( this )
{
    gint32 ret = 0;

    do{
        ObjPtr pObj;
        CCfgOpener oCfg( pCfg );

        ret = oCfg.GetObjPtr(
            propParentPtr, pObj );

        if( ERROR( ret ) )
            break;

        m_pParent = pObj;
        if( m_pParent == nullptr )
        {
            ret = -EFAULT;
            break;
        }

    }while( 0 );

    if( ERROR( ret ) )
    {
        string strMsg = DebugMsg( ret,
            "Error occurs in CRpcTcpBridgeProxy ctor" );
        throw runtime_error( strMsg );
    }
}

gint32 CRpcTcpBridgeProxy::InitUserFuncs()
{
    BEGIN_IFHANDLER_MAP( CRpcTcpBridge );

    ADD_EVENT_HANDLER(
        CRpcTcpBridgeProxy::OnRmtModOffline,
        BRIDGE_EVENT_MODOFF );

    ADD_EVENT_HANDLER(
        CRpcTcpBridgeProxy::OnInvalidStreamId,
        BRIDGE_EVENT_INVALIDSTM );

    END_IFHANDLER_MAP;

    return 0;
}

gint32 CRpcTcpBridgeProxy::EnableRemoteEvent(
    IEventSink* pCallback,
    IMessageMatch* pMatch )
{
    if( pMatch == nullptr ||
        pCallback == nullptr )
        return -EINVAL;

    return EnableRemoteEventInternal(
        pCallback, pMatch, true );
}

gint32 CRpcTcpBridgeProxy::DisableRemoteEvent(
    IEventSink* pCallback,
    IMessageMatch* pMatch )
{
    if( pMatch == nullptr ||
        pCallback == nullptr )
        return -EINVAL;

    return EnableRemoteEventInternal(
        pCallback, pMatch, false );
}

gint32 CRpcTcpBridgeProxy::EnableRemoteEventInternal(
    IEventSink* pCallback,
    IMessageMatch* pMatch,
    bool bEnable )
{
    gint32 ret = 0;
    do{
        if( pMatch == nullptr ||
            pCallback == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        CReqBuilder oBuilder( this );
        oBuilder.Push( ObjPtr( pMatch ) );
        if( bEnable )
        {
            oBuilder.SetMethodName(
                SYS_METHOD_ENABLERMTEVT );
        }
        else
        {
            oBuilder.SetMethodName(
                SYS_METHOD_DISABLERMTEVT );
        }

        oBuilder[ propStreamId ] =
            TCP_CONN_DEFAULT_STM;

        oBuilder[ propProtoId ] = protoDBusRelay;

        oBuilder.SetCallFlags( CF_WITH_REPLY
           | DBUS_MESSAGE_TYPE_METHOD_CALL 
           | CF_ASYNC_CALL );

        // a tcp default round trip time
        oBuilder.SetTimeoutSec(
            IFSTATE_DEFAULT_IOREQ_TIMEOUT ); 

        oBuilder.SetKeepAliveSec(
            IFSTATE_DEFAULT_IOREQ_TIMEOUT / 2 ); 

        CParamList oRespCfg;

        CfgPtr pRespCfg = oRespCfg.GetCfg();
        if( pRespCfg.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        ret = RunIoTask( oBuilder.GetCfg(),
            pRespCfg, pCallback );

        if( ret == STATUS_PENDING )
            break;

        if( SUCCEEDED( ret ) )
        {
            CCfgOpener oCfg(
                ( IConfigDb* )pRespCfg );

            gint32 iRet = 0;
            ret = oCfg.GetIntProp(
                propReturnValue,
                ( guint32& )iRet );

            if( ERROR( ret ) )
                break;

            ret = iRet;
        }
    }while( 0 );

    return ret;
}

gint32 CRpcTcpBridgeProxy::BuildBufForIrp(
    BufPtr& pBuf, IConfigDb* pReqCall )
{
    if( pReqCall == nullptr )
        return -EINVAL;

    gint32 ret = 0;

    do{
        if( pBuf.IsEmpty() )
        {
            ret = pBuf.NewObj();
            if( ERROR( ret ) )
                break;
        }

        CReqOpener oReq( pReqCall );
        string strMethod;
        ret = oReq.GetMethodName( strMethod );
        if( ERROR( ret ) )
            break;

        if( strMethod == SYS_METHOD_FORWARDREQ )
        {
            ret = BuildBufForIrpFwrdReq(
                pBuf, pReqCall );
        }
        else
        {
            ret = super::BuildBufForIrp(
                pBuf, pReqCall );
        }

    }while( 0 );

    return ret;
}

gint32 CRpcTcpBridgeProxy::BuildBufForIrpFwrdReq(
    BufPtr& pBuf, IConfigDb* pReqCall )
{
    if( pReqCall == nullptr ||
        pBuf.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;
    do{
        DMsgPtr pMsg, pMsgToFwrd;
        string strIpAddr;
        CReqOpener oReq( pReqCall );

        ret = oReq.GetStrProp( 0, strIpAddr );
        if( ERROR( ret ) )
            break;

        ret = oReq.GetMsgPtr( 1, pMsgToFwrd );
        if( ERROR( ret ) )
            break;

        string strModName =
            GetIoMgr()->GetModName();

        string strIfName = DBUS_IF_NAME(
            IFNAME_TCP_BRIDGE );

        string strObjPath =DBUS_OBJ_PATH(
            strModName, OBJNAME_TCP_BRIDGE );

        string strSender =
            DBUS_DESTINATION( strModName  );


        ret = pMsg.SetPath( strObjPath );
        if( ERROR( ret ) )
            break;

        ret = pMsg.SetInterface( strIfName );
        if( ERROR( ret ) )
            break;

        // set to the obj path deliberately
        ret = pMsg.SetDestination( strObjPath );
        if( ERROR( ret ) )
            break;

        ret = pMsg.SetSender( strSender );
        if( ERROR( ret ) ) 
            break;

        ret = pMsg.SetMember(
            SYS_METHOD_FORWARDREQ );

        if( ERROR( ret ) )
            break;

        BufPtr pReqMsgBuf( true );
        ret = pMsgToFwrd.Serialize( *pReqMsgBuf );
        if( ERROR( ret ) )
            break;

        guint64 qwTaskId;
        ret = oReq.GetTaskId( qwTaskId );
        if( ERROR( ret ) )
            break;

        // NOTE: the task id is for keep-alive
        // purpose
        const char* pszIp = strIpAddr.c_str();
        if( !dbus_message_append_args( pMsg,
            DBUS_TYPE_STRING, &pszIp,
            DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
            &pReqMsgBuf->ptr(), pReqMsgBuf->size(),
            DBUS_TYPE_UINT64, &qwTaskId,
            DBUS_TYPE_INVALID ) )
        {
            ret = -ENOMEM;
            break;
        }
        *pBuf = pMsg;

    }while( 0 );

    return ret;
}

gint32 CRpcTcpBridgeProxy::ForwardRequest(
    const std::string& strDestIp,   // [ in ]
    DBusMessage* pReqMsg,           // [ in ]
    DMsgPtr& pRespMsg,              // [ out ]
    IEventSink* pCallback )
{
    if( pReqMsg == nullptr ||
        pCallback == nullptr )
        return -EINVAL;

    if( strDestIp.empty() )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CReqBuilder oBuilder( this );

        // redudent information `strDestIp'
        // just to conform to the rule
        oBuilder.Push( strDestIp );
        oBuilder.Push( DMsgPtr( pReqMsg ) );

        oBuilder.SetMethodName(
            SYS_METHOD_FORWARDREQ );

        oBuilder.SetCallFlags( CF_WITH_REPLY
           | DBUS_MESSAGE_TYPE_METHOD_CALL 
           | CF_ASYNC_CALL );

        // a tcp default round trip time
        oBuilder.SetTimeoutSec(
            IFSTATE_DEFAULT_IOREQ_TIMEOUT ); 

        oBuilder.SetKeepAliveSec(
            IFSTATE_DEFAULT_IOREQ_TIMEOUT / 2 ); 

        oBuilder[ propStreamId ] =
            TCP_CONN_DEFAULT_STM;

        oBuilder[ propProtoId ] = protoDBusRelay;

        CParamList oRespCfg;
        CfgPtr pRespCfg = oRespCfg.GetCfg();
        if( pRespCfg.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        ret = RunIoTask( oBuilder.GetCfg(),
            pRespCfg, pCallback );

        if( ret == STATUS_PENDING )
            break;

        if( SUCCEEDED( ret ) )
        {
            CCfgOpener oCfg(
                ( IConfigDb* )pRespCfg );

            gint32 iRet = 0;
            ret = oCfg.GetIntProp(
                propReturnValue,
                ( guint32& )iRet );

            if( ERROR( ret ) )
                break;

            ret = iRet;
        }

    }while( 0 );

    return ret;
}

gint32 CRpcTcpBridgeProxy::FillRespDataFwrdReq(
    IRP* pIrp, CfgPtr& pResp )
{
    gint32 ret = 0;
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    if( pResp.IsEmpty() )
        return -EINVAL;

    do{
        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        if( pCtx->GetMajorCmd() != IRP_MJ_FUNC ||
            pCtx->GetMinorCmd() != IRP_MN_IOCTL )
        {
            ret = -ENOTSUP;
            break;
        }

        guint32 dwCtrlCode = pCtx->GetCtrlCode();
        if( dwCtrlCode != CTRLCODE_FORWARD_REQ )
        {
            ret = -EINVAL;
            break;
        }

        CParamList oParams( ( IConfigDb* )pResp );

        DMsgPtr pFwrdMsg;
        pFwrdMsg = ( DMsgPtr& )*pCtx->m_pRespData;

        gint32 iRet;
        ret = pFwrdMsg.GetIntArgAt( 0, ( guint32& )iRet );
        if( ERROR( ret ) )
            break;

        oParams.SetIntProp( propReturnValue, iRet );
        oParams.Push( pFwrdMsg );

    }while( 0 );

    return ret;
}

gint32 CRpcTcpBridgeProxy::FillRespData(
    IRP* pIrp, CfgPtr& pResp )
{
    gint32 ret = 0;
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    if( pResp.IsEmpty() )
        return -EINVAL;

    // retrieve the data from the irp
    IrpCtxPtr& pCtx = pIrp->GetTopStack();
    if( pCtx->GetMajorCmd() != IRP_MJ_FUNC ||
        pCtx->GetMinorCmd() != IRP_MN_IOCTL )
        return -ENOTSUP;

    guint32 dwCtrlCode = pCtx->GetCtrlCode();

    CParamList oParams( ( IConfigDb* )pResp );
    oParams.SetIntProp( propReturnValue,
        pIrp->GetStatus() );

    switch( dwCtrlCode )
    {
    case CTRLCODE_FORWARD_REQ:
        {
            // we received a forward req message
            FillRespDataFwrdReq( pIrp, pResp );
            break;
        }
    case CTRLCODE_OPEN_STREAM_PDO:
        {
            CfgPtr pCfg;
            ret = pCtx->GetRespAsCfg( pCfg );
            if( ERROR( ret ) )
                break;

            ret = oParams.CopyProp(
                propReturnValue, pCfg );

            if( ERROR( ret ) )
                break;

            ret = oParams.CopyParams( pCfg );
            break;
        }
    case CTRLCODE_CLOSE_STREAM_PDO:
    case CTRLCODE_INVALID_STREAM_ID_PDO:
        {
            CfgPtr pCfg;
            ret = pCtx->GetRespAsCfg( pCfg );
            if( ERROR( ret ) )
                break;

            ret = oParams.CopyProp(
                propReturnValue, pCfg );

            if( ERROR( ret ) )
                break;

            break;
        }
    default:
        {
            // SEND_DATA and FETCH_DATA will go here
            ret = super::FillRespData(
                pIrp, pResp );
            break;
        }
    }
    return ret;
}

gint32 CRpcTcpBridgeProxy::ForwardEvent(
    const string& strSrcIp,
    DBusMessage* pEvtMsg,
    IEventSink* pCallback )
{
    if( pEvtMsg == nullptr ||
        pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;

    do{
        InterfPtr pIf;
        CRpcRouter* pRouter = GetParent();
        pRouter->GetReqFwdr( pIf );
        CRpcReqForwarder* pReqFwdr = pIf;
        if( pReqFwdr == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        // user the server ip instead
        string strSvrIp;
        CCfgOpenerObj oCfg( this );
        oCfg.GetStrProp( propIpAddr, strSvrIp );
        ret = pReqFwdr->ForwardEvent(
            strSvrIp, pEvtMsg, pCallback );

    }while( 0 );

    return ret;
}

gint32 CRpcTcpBridgeProxy::OnProgressNotify(
    guint32 dwProgress,
    guint64 iTaskId )
{

    gint32 ret = 0;
    do{
        if( !IsConnected() )
        {
            // NOTE: this is not a serious state
            // check
            ret = ERROR_STATE;
            break;
        }

        TaskletPtr pTaskToNotify;
        if( true )
        {
            TaskGrpPtr pTaskGrp = GetTaskGroup();
            if( pTaskGrp.IsEmpty() )
            {
                ret = -ENOENT;    
                break;
            }
            ret = pTaskGrp->FindTask(
                iTaskId, pTaskToNotify );

            if( ERROR( ret ) )
                break;
        }

        if( pTaskToNotify.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        // send a keepalive event
        EnumEventId iEvent = ( EnumEventId )
            ( eventTryLock | eventRpcNotify );

        pTaskToNotify->OnEvent( iEvent,
            eventProgress, dwProgress, nullptr );

    }while( 0 );

    return ret;
}

gint32 CRpcTcpBridgeProxy::DoInvoke(
    DBusMessage* pEvtMsg,
    IEventSink* pCallback )
{
    gint32 ret = 0;

    do{
        if( pEvtMsg == nullptr ||
            pCallback == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        DMsgPtr pMsg( pEvtMsg );

        if( pMsg.GetType()
            != DBUS_MESSAGE_TYPE_SIGNAL )
        {
            ret = -EINVAL;
            break;
        }

        string strMethod = pMsg.GetMember();
        if( strMethod.empty() )
        {
            ret = -EBADMSG;
            break;
        }

        if( strMethod == SYS_EVENT_KEEPALIVE )
        {
            ret = OnKeepAlive( pCallback, KARelay );
            return ret;
        }
        else if( strMethod == SYS_EVENT_FORWARDEVT )
        {
            string strSrcIp;
            CCfgOpenerObj oCfg( this );
            ret = oCfg.GetStrProp(
                propIpAddr, strSrcIp );

            if( ERROR( ret ) )
                break;

            if( strSrcIp.empty() )
            {
                ret = -EINVAL;
                break;
            }

            DMsgPtr pUserEvtMsg;
            ret = pMsg.GetMsgArgAt(
                1, pUserEvtMsg );

            if( ERROR( ret ) )
                break;

            ret = ForwardEvent( strSrcIp,
                pUserEvtMsg, pCallback );
        }
        else if( strMethod == IF_EVENT_PROGRESS )
        {
            guint32 dwProgress;
            ret = pMsg.GetIntArgAt(
                0, dwProgress );

            if( ERROR( ret ) )
                break;

            guint64 qwTaskId;
            ret = pMsg.GetInt64ArgAt(
                1, qwTaskId );

            if( ERROR( ret ) )
                break;
            OnProgressNotify(
                dwProgress, qwTaskId );
        }
        else
        {
            ret = super::DoInvoke(
                pEvtMsg, pCallback );
        }

    }while( 0 );

    return ret;
}

gint32 CRpcTcpBridgeProxy::SetupReqIrp(
    IRP* pIrp,
    IConfigDb* pReqCall,
    IEventSink* pCallback )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    if( pReqCall == nullptr ||
        pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;

    do{
        CReqOpener oReq( pReqCall );

        string strMethod;
        ret = oReq.GetMethodName( strMethod );
        if( ERROR( ret ) )
            break;

        if( strMethod == IF_METHOD_LISTENING )
        {
            ret = SetupReqIrpListening(
                pIrp, pReqCall, pCallback ); 
            break;
        }

        else if( strMethod == SYS_METHOD_ENABLERMTEVT ||
            strMethod == SYS_METHOD_DISABLERMTEVT )
        {
            IrpCtxPtr& pIrpCtx = pIrp->GetTopStack(); 
            pIrpCtx->SetMajorCmd( IRP_MJ_FUNC );
            pIrpCtx->SetMinorCmd( IRP_MN_IOCTL );
            pIrpCtx->SetCtrlCode( CTRLCODE_SEND_REQ );

            SetBdgeIrpStmId( pIrp,
                ( guint32& )oReq[ propStreamId ] );

            guint32 dwIoDir = IRP_DIR_OUT;
            if( oReq.HasReply() )
                dwIoDir = IRP_DIR_INOUT;

            pIrpCtx->SetIoDirection( dwIoDir ); 

            BufPtr pBuf( true );
            *pBuf = ( ObjPtr& )oReq.GetCfg();

            if( ERROR( ret ) )
                break;

            pIrpCtx->SetReqData( pBuf );
            pIrp->SetCallback( pCallback, 0 );
            pIrp->SetIrpThread( GetIoMgr() );

            guint32 dwTimeoutSec = 0;
            ret = oReq.GetTimeoutSec( dwTimeoutSec );

            if( SUCCEEDED( ret ) &&
                dwTimeoutSec != 0 )
            {
                pIrp->SetTimer(
                    dwTimeoutSec, GetIoMgr() );
            }
        }
        else if( strMethod == BRIDGE_METHOD_OPENSTM ||
            strMethod == BRIDGE_METHOD_CLOSESTM )
        {
            guint32 dwCmdId = 0;
            ret = oReq.GetIntProp( propCmdId, dwCmdId );
            if( ERROR( ret ) )
                break;

            IrpCtxPtr& pIrpCtx = pIrp->GetTopStack(); 
            pIrpCtx->SetMajorCmd( IRP_MJ_FUNC );
            pIrpCtx->SetMinorCmd( IRP_MN_IOCTL );
            pIrpCtx->SetCtrlCode( dwCmdId );

            SetBdgeIrpStmId( pIrp,
                ( guint32& )oReq[ propStreamId ] );

            guint32 dwIoDir = IRP_DIR_OUT;
            if( oReq.HasReply() )
                dwIoDir = IRP_DIR_INOUT;

            pIrpCtx->SetIoDirection( dwIoDir ); 

            BufPtr pBuf( true );
            *pBuf = ( ObjPtr& )oReq.GetCfg();

            if( ERROR( ret ) )
                break;

            pIrpCtx->SetReqData( pBuf );
            pIrp->SetCallback( pCallback, 0 );
            pIrp->SetIrpThread( GetIoMgr() );

            guint32 dwTimeoutSec = 0;
            ret = oReq.GetTimeoutSec( dwTimeoutSec );

            if( SUCCEEDED( ret ) &&
                dwTimeoutSec != 0 )
            {
                pIrp->SetTimer(
                    dwTimeoutSec, GetIoMgr() );
            }
        }
        else if( strMethod == SYS_METHOD_FETCHDATA ||
            strMethod == SYS_METHOD_SENDDATA )
        {
            SetBdgeIrpStmId( pIrp,
                TCP_CONN_DEFAULT_STM );

            ret = super::SetupReqIrp( pIrp,
                pReqCall, pCallback );

            break;
        }

    }while( 0 );

    return ret;
}

gint32 CRpcTcpBridgeProxy::OpenStream_Proxy(
    guint32 wProtocol,
    gint32& iNewStm,
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        CReqBuilder oBuilder( this );

        oBuilder.SetCallFlags( CF_WITH_REPLY
           | DBUS_MESSAGE_TYPE_METHOD_CALL 
           | CF_ASYNC_CALL );

        // a tcp default round trip time
        oBuilder.SetTimeoutSec(
            IFSTATE_DEFAULT_IOREQ_TIMEOUT ); 

        oBuilder.SetKeepAliveSec(
            IFSTATE_DEFAULT_IOREQ_TIMEOUT / 2 ); 

        oBuilder.SetMethodName(
            BRIDGE_METHOD_OPENSTM );

        oBuilder.SetIntProp(
            propStreamId, TCP_CONN_DEFAULT_STM );

        oBuilder[ propProtoId ] = protoDBusRelay;

        oBuilder.SetIntProp( propCmdId,
            CTRLCODE_SEND_REQ );

        oBuilder.Push( ( guint32 )wProtocol );

        CfgPtr pRespCfg( true );

        ret = RunIoTask( oBuilder.GetCfg(),
            pRespCfg, pCallback );

        if( ret == STATUS_PENDING )
            break;

        if( SUCCEEDED( ret ) )
        {
            CCfgOpener oCfg(
                ( IConfigDb* )pRespCfg );

            gint32 iRet = 0;
            ret = oCfg.GetIntProp(
                propReturnValue,
                ( guint32& )iRet );

            if( ERROR( ret ) )
                break;

            ret = oCfg.GetIntProp(
                0, ( guint32& )iNewStm );

            if( ERROR( ret ) )
                break;

            CIfParallelTask* pTask =
                ObjPtr( pCallback );

            if( pTask != nullptr )
                pTask->SetRespData( pRespCfg );

            ret = iRet;
        }
    }while( 0 );

    return ret;
}

gint32 CRpcTcpBridgeProxy::CloseStream_Proxy(
    gint32 iStreamId,
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        CReqBuilder oBuilder( this );

        oBuilder.SetCallFlags( CF_WITH_REPLY |
            DBUS_MESSAGE_TYPE_METHOD_CALL  |
            CF_ASYNC_CALL );

        // a tcp default round trip time
        oBuilder.SetTimeoutSec(
            IFSTATE_DEFAULT_IOREQ_TIMEOUT ); 

        oBuilder.SetKeepAliveSec(
            IFSTATE_DEFAULT_IOREQ_TIMEOUT / 2 ); 

        oBuilder.SetMethodName(
            BRIDGE_METHOD_CLOSESTM );

        oBuilder.Push( ( guint32 )iStreamId );

        oBuilder.SetIntProp( propCmdId,
            CTRLCODE_CLOSE_STREAM_PDO );

        oBuilder.SetIntProp(
            propStreamId, TCP_CONN_DEFAULT_STM );

        oBuilder[ propProtoId ] = protoDBusRelay;

        oBuilder.SetIntProp( propCmdId,
            CTRLCODE_SEND_REQ );

        CfgPtr pRespCfg( true );
        ret = RunIoTask( oBuilder.GetCfg(),
            pRespCfg, pCallback );

        if( ret == STATUS_PENDING )
            break;

        if( SUCCEEDED( ret ) )
        {
            CCfgOpener oCfg(
                ( IConfigDb* )pRespCfg );

            gint32 iRet = 0;
            ret = oCfg.GetIntProp(
                propReturnValue,
                ( guint32& )iRet );

            if( ERROR( ret ) )
                break;

            CIfParallelTask* pTask =
                ObjPtr( pCallback );

            if( pTask != nullptr )
            {
                pTask->SetRespData( pRespCfg );
            }
            ret = iRet;
        }
    }while( 0 );

    return ret;
}

gint32 CRpcTcpBridgeProxy::SendFetch_Proxy(
        IConfigDb* pDataDesc,           // [in, out]
        gint32& fd,                      // [out]
        guint32& dwOffset,               // [in, out]
        guint32& dwSize,                 // [in, out]
        IEventSink* pCallback )
{
    if( pDataDesc == nullptr ||
        pCallback == nullptr ||
        dwSize == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CParamList oParams;
        oParams.SetObjPtr(
            propIoMgr, GetIoMgr() );
        oParams.SetObjPtr(
            propEventSink, pCallback );
        oParams.SetObjPtr(
            propIfPtr, this );

        CCfgOpener oDesc( pDataDesc );
        string strMethod =
            oDesc[ propMethodName ];

        bool bFetch = false;
        if( strMethod == SYS_METHOD_FETCHDATA )
            bFetch = true;
        else if( strMethod == SYS_METHOD_SENDDATA )
            bFetch = false;
        else
        {
            ret = -EINVAL;
            break;
        }
        
        CParamList oCallArgs;


        oCallArgs.Push( ObjPtr( pDataDesc ) );
        oCallArgs.Push( fd );
        oCallArgs.Push( dwOffset );
        oCallArgs.Push( dwSize );

        oParams.SetObjPtr( propReqPtr,
            oCallArgs.GetCfg() );

        TaskletPtr pTask;
        ret = pTask.NewObj(
            clsid( CBdgeProxyOpenStreamTask ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        ret = GetIoMgr()->RescheduleTask( pTask );

        if( ERROR( ret ) )
            break;

        ret = pTask->GetError();
        if( SUCCEEDED( ret ) && bFetch )
        {
            // it should be impossible to success
            // immediately
            ret = ERROR_FAIL;
        }

    }while( 0 );

    return ret;
}

gint32 CRpcTcpBridgeProxy::SendData_Proxy(
        IConfigDb* pDataDesc,           // [in]
        gint32 fd,                      // [in]
        guint32 dwOffset,               // [in]
        guint32 dwSize,                 // [in]
        IEventSink* pCallback )
{
    if( pDataDesc == nullptr ||
        pCallback == nullptr ||
        dwSize == 0 )
        return -EINVAL;

    return SendFetch_Proxy( pDataDesc,
        fd, dwOffset, dwSize, pCallback );
}

gint32 CRpcTcpBridgeProxy::FetchData_Proxy(
        IConfigDb* pDataDesc,           // [in, out]
        gint32& fd,                      // [out]
        guint32& dwOffset,               // [in, out]
        guint32& dwSize,                 // [in, out]
        IEventSink* pCallback )
{
    if( pDataDesc == nullptr ||
        pCallback == nullptr ||
        dwSize == 0 )
        return -EINVAL;

    return SendFetch_Proxy( pDataDesc,
        fd, dwOffset, dwSize, pCallback );
}

gint32 CBdgeProxyReadWriteComplete::OnIrpComplete(
    PIRP pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = super::OnIrpComplete( pIrp );
    if( ret == STATUS_MORE_PROCESS_NEEDED ||
        ret == STATUS_PENDING )
        return ret;

    do{
        CCfgOpenerObj oCfg( this );

        EventPtr pCallback;
        ret = GetInterceptTask( pCallback );
        if( ERROR( ret ) )
            break;
        
        guint32 dwCmdId = 0;
        ret = oCfg.GetIntProp(
            propCmdId, dwCmdId );

        if( ERROR( ret ) )
            break;

        if( dwCmdId == IRP_MN_READ )
        {
            pCallback->OnEvent( eventIrpComp,
                ( guint32 )pIrp, 0, 0 );
        }
        else if( dwCmdId == IRP_MN_WRITE )
        {
            pCallback->OnEvent( eventIrpComp,
                ( guint32 )pIrp, 0, 0 );
        }

    }while( 0 );

    return ret;
}


gint32 CRpcTcpBridgeProxy::StartEx(
    IEventSink* pCallback )
{
    if( pCallback == nullptr )
        return -EINVAL;
        
    gint32 ret = 0;
    do{
        TaskletPtr pTask;
        ret = pTask.NewObj( clsid( CTaskWrapper ) );
        if( ERROR( ret ) )
            break;

        CTaskWrapper* pWrapper = pTask;
        if( pWrapper == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        ret = pWrapper->SetTask( pCallback );
        if( ERROR( ret ) )
            break;

        CParamList oReq( ( IConfigDb* )
            pWrapper->GetConfig() );

        oReq.SetStrProp( propMethodName,
            IF_METHOD_LISTENING );

        oReq.SetIntProp( propStreamId,
            TCP_CONN_DEFAULT_CMD );

        oReq.SetIntProp( propProtoId,
            protoControl );

        oReq.SetIntProp( propCmdId,
            CTRLCODE_LISTENING );

        oReq.SetBoolProp(
            propSubmitPdo, true );

        ret = super::StartEx( pWrapper );

        if( ERROR( ret ) )
            break;

    }while( 0 );

    if( ret == STATUS_MORE_PROCESS_NEEDED )
        ret = STATUS_PENDING;

    return ret;
}

gint32 CRpcTcpBridgeProxy::OnRmtModOffline(
    IEventSink* pCallback,
    const std::string& strModName )
{
    if( pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;

    do{
        string strIpAddr;
        CCfgOpenerObj oCfg( this );

        ret = oCfg.GetStrProp(
            propDestIpAddr, strIpAddr );

        if( ERROR( ret ) )
            break;

        // pass on this event to the pnp manager
        CEventMapHelper< CGenericInterface >
            oEvtHelper( this );

        oEvtHelper.BroadcastEvent(
            eventConnPoint,
            eventRmtModOffline,
            ( guint32 )strIpAddr.c_str(),
            ( guint32* )strModName.c_str() );

    }while( 0 );

    return ret;
}

gint32 CRpcTcpBridgeProxy::OnInvalidStreamId(
    IEventSink* pCallback,
    gint32 iPeerStreamId,
    gint32 iStreamId,
    guint32 dwSeqNo,
    guint32 dwProtocol )
{
    if( pCallback == nullptr )
        return -EINVAL;

    return 0;
}

bool CRpcInterfaceProxy::IsConnected(
    const char* szAddr )
{
    if( !super::IsConnected( szAddr ) )
        return false;
    return  m_pParent->IsConnected( szAddr );
}

gint32 CRpcInterfaceProxy::OnKeepAlive(
    IEventSink* pTask, EnumKAPhase iPhase )
{
    if( pTask == nullptr )
        return -EINVAL;

    if( iPhase == KAOrigin )
    {
        return OnKeepAliveOrig( pTask );
    }
    else if( iPhase == KARelay )
    {
        return OnKeepAliveRelay( pTask );
    }

    return -EINVAL;
}

gint32 CRpcInterfaceProxy::OnKeepAliveOrig(
    IEventSink* pTask )
{
    gint32 ret = 0;

    // servicing the incoming request/event
    // message
    do{
        DMsgPtr pMsg;
        CCfgOpenerObj oCfg( pTask );

        ret = oCfg.GetMsgPtr( propMsgPtr, pMsg );
        if( ERROR( ret ) )
            break;

        if( pMsg.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        if( !IsConnected() )
        {
            // NOTE: this is not a serious state
            // check
            ret = ERROR_STATE;
            break;
        }

        ObjPtr pObj;
        ret = pMsg.GetObjArgAt( 0, pObj );
        if( ERROR( ret ) )
            break;

        
        IConfigDb* pCfg = pObj;
        if( pCfg == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        CReqOpener oEvent( pCfg );
        guint64 iTaskId = 0;
        ret = oEvent.GetQwordProp( 0, iTaskId );
        if( ERROR( ret ) )
            break;

        TaskletPtr pTaskToKa;
        if( true )
        {
            TaskGrpPtr pTaskGrp = GetTaskGroup();
            if( pTaskGrp.IsEmpty() )
            {
                ret = -ENOENT;    
                break;
            }
            ret = pTaskGrp->FindTask(
                iTaskId, pTaskToKa );

            if( ERROR( ret ) )
                break;
        }

        if( pTaskToKa.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        CIfIoReqTask* pReqTask = pTaskToKa;
        if( pReqTask == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        // send a keepalive event
        EnumEventId iEvent = ( EnumEventId )
            ( eventTryLock | eventKeepAlive );

        ret = pTaskToKa->OnEvent( iEvent,
            KARelay, 0, nullptr );

    }while( 0 );

    // whether or not succeeded, we return pending
    // state to avoid the task get completed
    return STATUS_PENDING;
}

gint32 CRpcInterfaceProxy::OnKeepAliveRelay(
    IEventSink* pTask )
{
    return OnKeepAliveOrig( pTask );    
}

//------------------------------
//CRpcTcpBridge methods
//
CRpcTcpBridge::CRpcTcpBridge(
    const IConfigDb* pCfg )
    : CAggInterfaceServer( pCfg ), super( pCfg ),
    CRpcTcpBridgeShared( this )
{
    // SetClassId( clsid( CRpcTcpBridge) );
    gint32 ret = 0;

    do{
        ObjPtr pObj;
        CCfgOpener oCfg( pCfg );

        ret = oCfg.GetObjPtr(
            propParentPtr, pObj );

        if( ERROR( ret ) )
            break;

        m_pParent = pObj;
        if( m_pParent == nullptr )
        {
            ret = -EFAULT;
            break;
        }

    }while( 0 );

    if( ERROR( ret ) )
    {
        string strMsg = DebugMsg( ret,
            "Error occurs in CRpcTcpBridge ctor" );
        throw runtime_error( strMsg );
    }
}

CRpcTcpBridge::~CRpcTcpBridge()
{
}

gint32 CRpcTcpBridge::EnableRemoteEvent(
    IEventSink* pCallback, IMessageMatch* pMatch )
{
    if( pMatch == nullptr ||
        pCallback == nullptr )
        return -EINVAL;

    return EnableRemoteEventInternal(
        pCallback, pMatch, true );
}

gint32 CRpcTcpBridge::DisableRemoteEvent(
    IEventSink* pCallback, IMessageMatch* pMatch )
{
    if( pMatch == nullptr ||
        pCallback == nullptr )
        return -EINVAL;

    return EnableRemoteEventInternal(
        pCallback, pMatch, false );
}

gint32 CRpcTcpBridge::EnableRemoteEventInternal(
    IEventSink* pCallback,
    IMessageMatch* pMatch,
    bool bEnable )
{
    if( pMatch == nullptr ||
        pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;

    do{
        // NOTE: we use the original match for
        // dbus listening
        CParamList oParams;
        CCfgOpenerObj oMatch( pMatch );

        // overwrite the propIpAddr property
        // with the peer ip address, instead
        // ourself's
        oMatch.CopyProp( propIpAddr, this );
        oMatch.CopyProp( propSrcTcpPort, this );
        oMatch.CopyProp( propPortId, this );

        oParams.Push( bEnable );
        oParams.Push( ObjPtr( pMatch ) );

        // this callback is intercepted by
        // CRouterEnableEventRelayTask
        oParams.SetObjPtr( propEventSink,
            ObjPtr( pCallback ) );

        ret = oParams.SetObjPtr(
            propIfPtr, ObjPtr( this ) );

        if( ERROR( ret ) )
            break;

        oParams.SetObjPtr(
            propIoMgr, GetIoMgr() );

        // create a task for this async call
        TaskletPtr pTask;
        ret = pTask.NewObj(
            clsid( CBridgeAddRemoteMatchTask ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        CBridgeAddRemoteMatchTask*
            pMatchTask = pTask;

        CfgPtr pResp( true );
        pMatchTask->SetRespData( pResp );

        ret = ( *pTask )( eventZero );

    }while( 0 );

    return ret;
}

gint32 CBridgeAddRemoteMatchTask::AddRemoteMatchInternal(
    CRpcRouter* pRouter,
    IMessageMatch* pMatch,
    bool bEnable )
{
    gint32 ret = 0;
    do{
        if( pRouter == nullptr ||
            pMatch == nullptr )
            return false;

        // add the match to the m_mapRmtMatches
        MatchPtr pRtMatch;
        ret = pRouter->GetMatchToAdd(
            pMatch, true, pRtMatch );

        if( ERROR( ret ) )
            break;

        if( bEnable )
        {
            ret = pRouter->AddRemoteMatch(
                pMatch, this );
        }
        else
        {
            ret = pRouter->RemoveRemoteMatch(
                pMatch, this );
        }
    }while( 0 );

    return ret;
}

gint32 CBridgeAddRemoteMatchTask::RunTask()
{
    gint32 ret = 0;
    do{
        CParamList oParams( m_pCtx );
        InterfPtr pIf;
        ObjPtr pObj;
        ret = oParams.GetObjPtr( propIfPtr, pObj );
        if( ERROR( ret ) )
            break;

        pIf = pObj;
        if( pIf.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        CRpcTcpBridge* pBridge = pIf;
        if( pBridge == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        CRpcRouter* pRouter = pBridge->GetParent();
        if( pRouter == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        bool bEnable = false;
        MatchPtr pMatch;

        ret = oParams.GetBoolProp( 0, bEnable );
        if( ERROR( ret ) )
            break;

        ret = oParams.GetObjPtr( 1, pObj );
        if( ERROR( ret ) )
            break;
        pMatch = pObj;

        ret = AddRemoteMatchInternal(
            pRouter, pMatch, bEnable );

    }while( 0 );

    if( ret != STATUS_PENDING ||
        ret != STATUS_MORE_PROCESS_NEEDED )
    {
        ret = OnTaskComplete( ret );
    }

    return ret;
}

gint32 CBridgeAddRemoteMatchTask::OnTaskComplete(
    gint32 iRetVal )
{
    // OnTaskComplete will be called when the 
    gint32 ret = 0;
    CParamList oParams( m_pCtx );

    do{
        EventPtr pEvent;
        ret = GetInterceptTask( pEvent );
        if( ERROR( ret ) )
            break;

        InterfPtr pIf;
        CRpcTcpBridge* pBridge = nullptr;
        
        do{
            ObjPtr pObj;
            ret = oParams.GetObjPtr( propIfPtr, pObj );
            if( ERROR( ret ) )
                break;

            pIf = pObj;
            if( pIf.IsEmpty() )
            {
                ret = -EFAULT;
                break;
            }

            pBridge = pIf;
            if( pBridge == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            if( ERROR( iRetVal ) )
            {
                // rollback what we have done
                bool bEnable = false;
                MatchPtr pMatch;

                ret = oParams.GetBoolProp(
                    0, bEnable );

                if( ERROR( ret ) )
                    break;

                ret = oParams.GetObjPtr( 1, pObj );

                if( ERROR( ret ) )
                    break;

                pMatch = pObj;
                if( bEnable )
                {
                    // FIXME: unknown side effects.
                    // remove the failed match
                    AddRemoteMatchInternal(
                        pBridge->GetParent(),
                        pMatch, false );
                }
            }

        }while( 0 );

        if( IsPending() )
        {
            CParamList oResp;
            oResp[ propReturnValue ] = iRetVal;
            ret = pBridge->SetResponse(
                pEvent, oResp.GetCfg() );
        }

    }while( 0 );

    if( ret != STATUS_PENDING &&
        ret != STATUS_MORE_PROCESS_NEEDED )
        oParams.ClearParams();

    return ret;
}

gint32 CRpcTcpBridge::BuildBufForIrpFwrdEvt(
    BufPtr& pBuf, IConfigDb* pReqCall )
{
    if( pReqCall == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        DMsgPtr pMsg;

        ret = pMsg.NewObj( ( EnumClsid )
            DBUS_MESSAGE_TYPE_SIGNAL );

        if( ERROR( ret ) )
            break;

        ret = pMsg.SetMember(
            SYS_EVENT_FORWARDEVT );

        if( ERROR( ret ) )
            break;

        string strVal;
        string strModName =
            GetIoMgr()->GetModName();

        ret = pMsg.SetSender(
            DBUS_DESTINATION( strVal ) );

        if( ERROR( ret ) )
            break;

        strVal = DBUS_OBJ_PATH(
            strModName, OBJNAME_TCP_BRIDGE );

        ret = pMsg.SetPath( strVal );

        if( ERROR( ret ) )
            break;

        strVal = DBUS_IF_NAME(
            IFNAME_TCP_BRIDGE );

        pMsg.SetInterface( strVal );

        CReqOpener oReq( pReqCall );
        DMsgPtr pEvtMsg;

        ret = oReq.GetMsgPtr( 1, pEvtMsg );
        if( ERROR( ret ) )
            break;

        string strIpAddr;
        ret = oReq.GetStrProp( 0, strIpAddr );
        if( ERROR( ret ) )
            break;

        BufPtr pMsgBuf( true );
        ret = pEvtMsg.Serialize( pMsgBuf ); 
        if( ERROR( ret ) )
            break;

        const char* pszIp = strIpAddr.c_str();
        if( !dbus_message_append_args( pMsg,
            DBUS_TYPE_STRING, &pszIp,
            DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
            &pMsgBuf->ptr(), pMsgBuf->size(),
            DBUS_TYPE_INVALID ) )
        {
            ret = -ENOMEM;
            break;
        }

        *pBuf = pMsg;

    }while( 0 );

    return ret;
}

gint32 CRpcTcpBridge::CheckReqToFwrd(
    CRpcRouter* pRouter,
    const string &strIpAddr,
    DMsgPtr& pMsg,
    MatchPtr& pMatchHit )
{
    gint32 ret = pRouter->CheckReqToRelay(
        strIpAddr, pMsg, pMatchHit );

    return ret;
}

gint32 CRpcTcpBridge::CheckSendDataToFwrd(
    IConfigDb* pDataDesc )
{
    gint32 ret = 0;
    if( pDataDesc == nullptr )
        return -EINVAL;

    do{
        CRpcRouter* pRouter = GetParent();
        if( pRouter == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        CCfgOpener oParams( pDataDesc );
        string strIpAddr;
        ret = oParams.GetStrProp(
            propIpAddr, strIpAddr );
        if( ERROR( ret ) )
            break;

        DMsgPtr pMsgToCheck;
        ret = pMsgToCheck.NewObj();
        if( ERROR( ret ) )
            break;

        string strObjPath;
        ret = oParams.GetStrProp(
            propObjPath, strObjPath );

        if( ERROR( ret ) )
            break;
        pMsgToCheck.SetPath( strObjPath );

        string strIf;
        ret = oParams.GetStrProp(
            propIfName, strIf );
        if( ERROR( ret ) )
            break;
        pMsgToCheck.SetInterface( strIf );

        string strSender;
        ret = oParams.GetStrProp(
            propSrcDBusName, strSender );

        if( ERROR( ret ) )
            break;

        pMsgToCheck.SetSender( strIf );

        MatchPtr pMatch;
        ret = pRouter->CheckReqToRelay(
            strIpAddr, pMsgToCheck, pMatch );

    }while( 0 );

    return ret;
}

gint32 CRpcTcpBridge::ForwardEvent(
    const string& strDestIp,
    DBusMessage* pEvtMsg,
    IEventSink* pCallback )
{
    if( pEvtMsg == nullptr ||
        pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CReqBuilder oBuilder( this );

        CCfgOpenerObj oCfg( this );

        // the ip addr is just a placeholder
        std::string strSrcIp;
        oCfg.GetStrProp( propIpAddr, strSrcIp );

        oBuilder.Push( strSrcIp );
        oBuilder.Push( DMsgPtr( pEvtMsg ) );

        oBuilder.SetMethodName(
            SYS_EVENT_FORWARDEVT );

        oBuilder.SetCallFlags( 
           DBUS_MESSAGE_TYPE_SIGNAL 
           | CF_ASYNC_CALL );

        // a tcp default round trip time
        oBuilder.SetTimeoutSec(
            IFSTATE_DEFAULT_IOREQ_TIMEOUT ); 

        oBuilder.SetKeepAliveSec(
            IFSTATE_DEFAULT_IOREQ_TIMEOUT / 2 ); 

        oBuilder[ propStreamId ] =
            TCP_CONN_DEFAULT_STM;

        CParamList oRespCfg;
        CfgPtr pRespCfg = oRespCfg.GetCfg();
        if( pRespCfg.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        ret = RunIoTask( oBuilder.GetCfg(),
            pRespCfg, pCallback );

        if( ERROR( ret ) )
            break;

    }while( 0 );

    return ret;
}

gint32 CRpcInterfaceServer::SetupReqIrpFwrdEvt(
    IRP* pIrp,
    IConfigDb* pReqCall,
    IEventSink* pCallback )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    if( pReqCall == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        ret = super::SetupReqIrp(
            pIrp, pReqCall, pCallback );

        if( ERROR( ret ) )
            break;

        // make correction to the control code and
        // the io direction
        IrpCtxPtr& pIrpCtx = pIrp->GetTopStack(); 
        pIrpCtx->SetCtrlCode( CTRLCODE_SEND_EVENT );

        // FIXME: we don't know if the request
        // needs a reply, and the flag is set
        // to CF_WITH_REPLY mandatorily
        pIrpCtx->SetIoDirection( IRP_DIR_OUT ); 

        CCfgOpener oReq( pReqCall );
        SetBdgeIrpStmId( pIrp,
            ( guint32& )oReq[ propStreamId ] );

        // we don't need a timer
        if( pCallback != nullptr )
        {
            pIrp->SetCallback( pCallback, 0 );
            pIrp->SetIrpThread( GetIoMgr() );
        }

    }while( 0 );

    return ret;
}

gint32 CRpcInterfaceServer::SetupReqIrp(
    IRP* pIrp,
    IConfigDb* pReqCall,
    IEventSink* pCallback )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    if( pReqCall == nullptr ||
        pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;

    do{
        CReqOpener oReq( pReqCall );
        string strMethod;

        ret = oReq.GetMethodName( strMethod );

        if( SUCCEEDED( ret ) )
        {
            if( strMethod == SYS_EVENT_FORWARDEVT ||
                strMethod == SYS_EVENT_KEEPALIVE )
            {
                ret = SetupReqIrpFwrdEvt( pIrp,
                    pReqCall, pCallback );
            }
            else
            {
                ret = -ENOTSUP;
            }
            break;
        }
        else
        {
            // possibly the method name will not
            // be set for some requests,
            // especially for CRpcTcpBridge and
            // CRpcTcpBridgeProxy
            ret = super::SetupReqIrp( pIrp,
                pReqCall, pCallback );

            break;
        }

    }while( 0 );

    return ret;
}

gint32 CRpcInterfaceServer::BuildBufForIrp(
    BufPtr& pBuf, IConfigDb* pReqCall )
{
    if( pReqCall == nullptr )
        return -EINVAL;

    gint32 ret = 0;

    do{
        if( pBuf.IsEmpty() )
        {
            ret = pBuf.NewObj();
            if( ERROR( ret ) )
                break;
        }

        CReqOpener oReq( pReqCall );
        string strMethod;
        ret = oReq.GetMethodName( strMethod );
        if( ERROR( ret ) )
            break;

        if( strMethod == SYS_EVENT_FORWARDEVT )
        {
            ret = BuildBufForIrpFwrdEvt(
                pBuf, pReqCall );
        }
        else
        {
            ret = super::BuildBufForIrp(
                pBuf, pReqCall );
        }

    }while( 0 );

    return ret;
}

gint32 CRpcInterfaceServer::OnKeepAlive(
    IEventSink* pTask, EnumKAPhase iPhase )
{
    if( iPhase == KARelay )
    {
        return OnKeepAliveRelay( pTask );
    }

    return -EINVAL;
}

gint32 CRpcTcpBridge::OnKeepAliveRelay(
    IEventSink* pTask )
{
    if( pTask == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CCfgOpenerObj oCfg( pTask );
        DMsgPtr pMsg;

        ret = oCfg.GetMsgPtr( propMsgPtr, pMsg );
        if( ERROR( ret ) )
            break;

        if( pMsg.GetType() !=
            DBUS_MESSAGE_TYPE_METHOD_CALL )
        {
            ret = -ENOTSUP;
            break;
        }
        // note that we only do keep-alive for
        // FORWARD_REQUEST
        if( pMsg.GetMember() !=
            SYS_METHOD_FORWARDREQ )
        {
            ret = -EINVAL;
            break;
        }

        if( !IsConnected() )
        {
            // NOTE: this is not a serious state
            // check
            ret = ERROR_STATE;
            break;
        }

        CReqBuilder okaReq( this );

        guint64 iTaskId = 0;
        ret = pMsg.GetInt64ArgAt( 2, iTaskId );
        if( ERROR( ret ) )
            break;

        okaReq.Push( iTaskId );
        okaReq.Push( ( guint32 )KARelay );

        okaReq.SetMethodName(
            SYS_EVENT_KEEPALIVE );

        okaReq.SetCallFlags( 
           DBUS_MESSAGE_TYPE_SIGNAL
           | CF_ASYNC_CALL );
        
        okaReq[ propStreamId ] =
            TCP_CONN_DEFAULT_STM;

        string strVal = pMsg.GetDestination( );
        if( strVal.empty() )
        {
            ret = -EINVAL;
            break;
        }
        okaReq.SetSender( strVal );

        strVal = pMsg.GetSender( );
        okaReq.SetDestination( strVal );

        TaskletPtr pDummyTask;

        ret = pDummyTask.NewObj(
            clsid( CIfDummyTask ) );

        if( ERROR( ret ) )
            break;

        ret = BroadcastEvent(
            okaReq.GetCfg(), pDummyTask );

    }while( 0 );

    return ret;
}

gint32 CRpcInterfaceServer::ForwardRequest(
    const string& strIpAddr,
    DBusMessage* pFwdrMsg,
    DMsgPtr& pRespMsg,
    IEventSink* pCallback )
{
    if( pFwdrMsg == nullptr ||
        pCallback == nullptr ||
        strIpAddr.empty() )
        return -EINVAL;

    gint32 ret = 0;
    pRespMsg.Clear();

    do{
        DMsgPtr fwdrMsg( pFwdrMsg );
        CRpcRouter* pRouter = GetParent();

        MatchPtr pMatch;
        ret = CheckReqToFwrd( pRouter,
            strIpAddr, fwdrMsg, pMatch );

        // invalid request
        if( ERROR( ret ) )
            break;

        CParamList oParams;
        oParams.SetObjPtr(
            propIoMgr, GetIoMgr() );

        oParams.SetObjPtr(
            propEventSink, ObjPtr( pCallback ) );

        oParams.SetObjPtr(
            propIfPtr, ObjPtr( this ) );

        CParamList oResp;

        oParams.SetObjPtr(
            propRespPtr, oResp.GetCfg() );

        oParams.SetObjPtr( propRouterPtr,
            ObjPtr( GetParent( ) ) );

        CCfgOpenerObj oMatch(
            ( CObjBase* )pMatch ); 

        std::string strDest;
        ret = oMatch.GetStrProp(
            propDestDBusName, strDest );

        if( ERROR( ret ) )
            break;

        oParams.Push( strIpAddr );
        oParams.Push( fwdrMsg );
        oParams.Push( strDest );

        TaskletPtr pTask;
        ret = pTask.NewObj(
            clsid( CReqFwdrForwardRequestTask ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        // ( *pTask )( eventZero );
        ret = GetIoMgr()->RescheduleTask( pTask );
        if( ERROR( ret ) )
            break;

        ret = pTask->GetError();
        if( ret == STATUS_PENDING )
            break;

    }while( 0 );

    // the return value indicates if the response
    // message is generated or not.
    return ret;
}

bool CRpcInterfaceServer::IsConnected(
    const char* szAddr )
{
    if( !super::IsConnected( szAddr ) )
        return false;
    return  m_pParent->IsConnected( szAddr );
}

gint32 CRpcInterfaceServer::SendFetch_Server(
    IConfigDb* pDataDesc,           // [in, out]
    gint32& fd,                     // [out]
    guint32& dwOffset,              // [in,out]
    guint32& dwSize,                // [in,out]
    IEventSink* pCallback )
{
    gint32 ret = 0;

    if( pDataDesc == nullptr ||
        pCallback == nullptr )
        return -EINVAL;

    do{
        CCfgOpener oDataDesc( pDataDesc );
        string strIpAddr;
        ret = oDataDesc.GetStrProp(
            propIpAddr, strIpAddr );

        if( ERROR( ret ) )
            break;

        string strMethod =
            oDataDesc[ propMethodName ];

        bool bSend = false;
        if( strMethod == SYS_METHOD_SENDDATA )
        {
            bSend = true;
        }
        else if( strMethod != SYS_METHOD_FETCHDATA )
        {
            ret = -EINVAL;
            break;
        }

        CParamList oParams;
        oParams.SetObjPtr(
            propIoMgr, GetIoMgr() );

        // key property for intercept task
        oParams.SetObjPtr(
            propEventSink, ObjPtr( pCallback ) );

        oParams.SetObjPtr(
            propIfPtr, ObjPtr( this ) );

        CParamList oResp;

        oParams.SetObjPtr(
            propRespPtr, oResp.GetCfg() );

        oParams.SetObjPtr( propRouterPtr,
            ObjPtr( GetParent() ) );

        oParams.Push( strIpAddr );
        oParams.Push( ObjPtr( pDataDesc ) );
        oParams.Push( ( guint32 )fd );
        oParams.Push( dwOffset );
        oParams.Push( dwSize );

        TaskletPtr pTask;
        if( bSend )
        {
            ret = pTask.NewObj(
                clsid( CReqFwdrSendDataTask ),
                oParams.GetCfg() );
        }
        else
        {
            ret = pTask.NewObj(
                clsid( CReqFwdrFetchDataTask ),
                oParams.GetCfg() );
        }

        if( ERROR( ret ) )
            break;

        ret = GetIoMgr()->RescheduleTask( pTask );
        if( ERROR( ret ) )
            break;

        ret = pTask->GetError();
        if( ret == STATUS_PENDING )
            break;

    }while( 0 );

    // the return value indicates if the response
    // message is generated or not.
    return ret;
}

gint32 CRpcInterfaceServer::DoInvoke(
    DBusMessage* pReqMsg,
    IEventSink* pCallback )
{
    gint32 ret = 0;
    if( pReqMsg == nullptr )
        return -EINVAL;

    do{
        CCfgOpenerObj oCfg( this );
        DMsgPtr pMsg( pReqMsg );
        CParamList oResp;

        string strMethod = pMsg.GetMember();
        if( strMethod.empty() )
        {
            ret = -EBADMSG;
            break;
        }

        vector< DMsgPtr::ARG_ENTRY > vecArgs;
        ret = pMsg.GetArgs( vecArgs );
        if( ERROR( ret ) )
            break;

        CfgPtr pCfg( true );
        do{
            if( strMethod == SYS_METHOD_FORWARDREQ )
            {
                guint32 iArgCount = GetArgCount(
                    &IRpcReqProxyAsync::ForwardRequest );

                iArgCount -= 2;

                if( vecArgs.empty()
                    || vecArgs.size() != iArgCount )
                {
                    ret = -EBADMSG;
                    break;
                }

                string strIpAddr =
                    ( string& )*vecArgs[ 0 ].second;

                if( GetClsid() == clsid( CRpcTcpBridgeImpl ) )
                {
                    // we will use the source ip
                    // address as the input
                    // parameter
                    ret = oCfg.GetStrProp(
                        propIpAddr, strIpAddr );

                    if( ERROR( ret ) )
                        break;
                }

                DMsgPtr pFwdrMsg;
                ret = pFwdrMsg.Deserialize(
                    *vecArgs[ 1 ].second );

                if( ERROR( ret ) )
                    break;

                DMsgPtr pRespMsg;
                ret = ForwardRequest( strIpAddr,
                    pFwdrMsg, pRespMsg, pCallback );

                if( ret == STATUS_PENDING )
                {
                    // the return value will be send
                    // in OnFetchDataComplete
                    break;
                }

                // whether successful or not, we
                // put the result in the response
                oResp.SetIntProp(
                    propReturnValue, ret );

                if( !pRespMsg.IsEmpty() )
                    oResp.Push( pRespMsg );

                break;
            }
            else
            {
                ret = -ENOTSUP;
                break;
            }

        }while( 0 );

        if( ret == STATUS_PENDING )
            break;

        if( ret != -ENOTSUP )
        {
            // for not supported commands we will
            // try to handle them in the base
            // class
            SetResponse( pCallback, oResp.GetCfg() );
        }

    }while( 0 );

    if( ret == -ENOTSUP )
    {
        ret = super::DoInvoke(
            pReqMsg, pCallback );
    }

    return ret;
}

gint32 CRpcTcpBridge::SendFetch_Server(
    IConfigDb* pDataDesc,           // [in, out]
    gint32& fd,                      // [out]
    guint32& dwOffset,               // [in, out]
    guint32& dwSize,                 // [in, out]
    IEventSink* pCallback )
{
    gint32 ret = 0;

    if( pDataDesc == nullptr ||
        pCallback == nullptr )
        return -EINVAL;

    do{
        ret = CheckSendDataToFwrd( pDataDesc );
        if( ERROR( ret ) )
            break;

        CCfgOpenerObj oCfg( pCallback );
        DMsgPtr pMsg;

        ret = oCfg.GetMsgPtr( propMsgPtr, pMsg );
        if( ERROR( ret ) )
            break;

        if( pMsg.GetType() !=
            DBUS_MESSAGE_TYPE_METHOD_CALL )
        {
            ret = -ENOTSUP;
            break;
        }

        // note that we only do keep-alive for
        // FORWARD_REQUEST

        bool bSend = true;
        string strCall = pMsg.GetMember();
        if( strCall == SYS_METHOD_FETCHDATA )
        {
            bSend = false;
        }
        else if( strCall != SYS_METHOD_SENDDATA )
        {
            ret = -EINVAL;
            break;
        }

        if( !IsConnected() )
        {
            // NOTE: this is not a serious state
            // check
            ret = ERROR_STATE;
            break;
        }

        guint64 iTaskId = 0;
        ret = pMsg.GetInt64ArgAt( 4, iTaskId );
        if( ERROR( ret ) )
            break;

        // notify the client, we are in progress
        ret = OnProgressNotify( 0, iTaskId );
        if( ERROR( ret ) )
            break;

        // we need to receive the data before
        // forwarding to the proxy
        CParamList oSendArg;
        oSendArg.Push( ObjPtr( pDataDesc ) );
        oSendArg.Push( fd );
        oSendArg.Push( dwOffset );
        oSendArg.Push( dwSize );

        CParamList oTaskParams;
        oTaskParams[ propReqPtr ] =
            oSendArg.GetCfg();

        oTaskParams[ propIfPtr ] =
            ObjPtr( this );

        oTaskParams[ propTaskId ] = iTaskId;

        oTaskParams[ propEventSink ] =
            ObjPtr( pCallback );

        TaskletPtr pTask;

        if( bSend )
        {
            ret = pTask.NewObj( 
                clsid( CBdgeStartRecvDataTask ),
                oTaskParams.GetCfg() );
        }
        else
        {
            ret = pTask.NewObj(
                clsid( CBdgeStartFetchDataTask ),
                oTaskParams.GetCfg() );
        }

        if( ERROR( ret ) )
            break;

        ret = GetIoMgr()->RescheduleTask( pTask );

        if( SUCCEEDED( ret ) )
            ret = pTask->GetError();

    }while( 0 );

    // the return value indicates if the response
    // message is generated or not.
    return ret;
}

gint32 CRpcTcpBridge::SetupReqIrpOnProgress(
    IRP* pIrp,
    IConfigDb* pReqCall,
    IEventSink* pCallback )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    if( pCallback == nullptr ||
        pReqCall == nullptr )
        return -EINVAL;

    gint32 ret = 0;

    do{
        IrpCtxPtr& pIrpCtx = pIrp->GetTopStack(); 

        DMsgPtr pMsg;
        ret = pMsg.NewObj( ( EnumClsid )
            DBUS_MESSAGE_TYPE_SIGNAL );

        if( ERROR( ret ) )
            break;

        ret = pMsg.SetMember(
            SYS_EVENT_FORWARDEVT );

        if( ERROR( ret ) )
            break;

        string strVal;
        string strModName = 
            GetIoMgr()->GetModName();

        ret = pMsg.SetSender(
            DBUS_DESTINATION( strVal ) );

        if( ERROR( ret ) )
            break;

        strVal = DBUS_OBJ_PATH(
            strModName, OBJNAME_TCP_BRIDGE );

        ret = pMsg.SetPath( strVal );

        if( ERROR( ret ) )
            break;

        strVal = DBUS_IF_NAME(
            IFNAME_TCP_BRIDGE );

        pMsg.SetInterface( strVal );

        CParamList oReq( pReqCall ); 
        guint64 qwTaskId = oReq[ 0 ];
        guint32 dwProgress = oReq[ 1 ];

        // NOTE: the task id is for keep-alive
        // purpose
        if( !dbus_message_append_args( pMsg,
            DBUS_TYPE_UINT64, &qwTaskId,
            DBUS_TYPE_UINT32, &dwProgress,
            DBUS_TYPE_INVALID ) )
        {
            ret = -ENOMEM;
            break;
        }

        BufPtr pBuf( true );
        *pBuf = pMsg;

        pIrpCtx->SetReqData( pBuf );
        pIrpCtx->SetMajorCmd( IRP_MJ_FUNC );
        pIrpCtx->SetMinorCmd( IRP_MN_IOCTL );
        pIrpCtx->SetCtrlCode( CTRLCODE_SEND_EVENT );

        pIrpCtx->SetIoDirection( IRP_DIR_OUT );
        pIrp->SetCallback( pCallback, 0 );
        pIrp->SetIrpThread( GetIoMgr() );

    }while( 0 );

    return ret;
}

gint32 CRpcTcpBridge::SetupReqIrp(
    IRP* pIrp,
    IConfigDb* pReqCall,
    IEventSink* pCallback )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    if( pReqCall == nullptr ||
        pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;

    do{
        CReqOpener oReq( pReqCall );

        string strMethod;
        ret = oReq.GetMethodName( strMethod );
        if( ERROR( ret ) )
            break;

        if( strMethod == IF_METHOD_LISTENING )
        {
            ret = SetupReqIrpListening(
                pIrp, pReqCall, pCallback );
        }
        else if( strMethod == IF_EVENT_PROGRESS )
        {

            ret = SetupReqIrpOnProgress(
                pIrp, pReqCall, pCallback );
        }
        else
        {
            ret = -ENOTSUP;
        }
        break;

    }while( 0 );

    if( ret == -ENOTSUP )
    {
        ret = super::SetupReqIrp( pIrp,
            pReqCall, pCallback );
    }

    return ret;
}

// we will start two StartRecvMsg task to
// listen on two streams, the default cmd
// and the default stream
gint32 CRpcTcpBridge::StartEx(
    IEventSink* pCallback )
{
    if( pCallback == nullptr )
        return -EINVAL;
        
    gint32 ret = 0;
    do{
        TaskletPtr pTask;
        ret = pTask.NewObj( clsid( CTaskWrapper ) );
        if( ERROR( ret ) )
            break;

        CTaskWrapper* pWrapper = pTask;
        if( pWrapper == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        ret = pWrapper->SetTask( pCallback );
        if( ERROR( ret ) )
            break;

        CParamList oReq( ( IConfigDb* )
            pWrapper->GetConfig() );

        oReq.SetStrProp( propMethodName,
            IF_METHOD_LISTENING );

        oReq.SetIntProp( propStreamId,
            TCP_CONN_DEFAULT_STM );

        oReq.SetIntProp( propProtoId,
            protoDBusRelay );

        oReq.SetIntProp( propCmdId,
            CTRLCODE_LISTENING );

        ret = super::StartEx(
            ( IEventSink* )pWrapper );

        if( ERROR( ret ) )
            break;

        ret = oReq.SetObjPtr(
            propIfPtr, ObjPtr( this ) );

        if( ERROR( ret ) )
            break;

        // one more listening task on the default
        // command stream
        CfgPtr pCfg( true );
        CCfgOpener oReq2(
            ( IConfigDb* )pCfg );

        oReq2.CopyProp(
            propMethodName, pWrapper );

        oReq2.CopyProp(
            propCmdId, pWrapper );

        ret = oReq2.SetIntProp(
            propStreamId,
            TCP_CONN_DEFAULT_CMD );

        oReq2.SetIntProp( propProtoId,
            protoControl );

        oReq2.SetBoolProp(
            propSubmitPdo, true );

        TaskletPtr pRecvMsgTask;
        ret = pRecvMsgTask.NewObj(
            clsid( CIfStartRecvMsgTask ),
            oReq2.GetCfg() );

        if( ERROR( ret ) )
            break;

        // this task will run later when the first
        // two have done
        ret = AddAndRun( pRecvMsgTask );
        if( ERROR( ret ) )
            break;

    }while( 0 );

    if( ret == STATUS_MORE_PROCESS_NEEDED )
        ret = STATUS_PENDING;

    return ret;
}


gint32 CRpcTcpBridge::SetResponse(
    IEventSink* pCallback,
    CfgPtr& pRespData )
{
    if( pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CReqBuilder oResp(
            ( IConfigDb* )pRespData );

        CTasklet* pTask = ObjPtr( pCallback );
        if( pTask == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        CParamList oTask( pTask->GetConfig() );

        ObjPtr pObj;
        gint32 iType = 0;
        ret = pTask->GetConfig()->
            GetPropertyType( propMsgPtr, iType );

        if( ERROR( ret ) )
            break;

        if( iType == typeDMsg )
        {
            super::SetResponse(
                pTask, pRespData );
            break;
        }
        else if( iType != typeObj )
        {
            ret = -EFAULT;
            break;
        }

        ret = oTask.GetObjPtr(
            propMsgPtr, pObj );

        if( ERROR( ret ) )
            break;

        IConfigDb* pReq = pObj;
        if( pReq == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        oResp.CopyProp( propSeqNo, pReq );
        oResp.SetCallFlags( CF_NON_DBUS |
            DBUS_MESSAGE_TYPE_METHOD_RETURN );

        super::SetResponse( pCallback,
            oResp.GetCfg() );

    }while( 0 );

    return ret;
}

gint32 CRpcTcpBridge::CloseStream_Server(
    IEventSink* pCallback,
    gint32 iStreamId )
{
    // server side processing of the
    // request
    gint32 ret = 0;

    do{
        if( IsReserveStm( iStreamId ) )
        {
            ret = -EINVAL;
            break;
        }

        IPort* pPort = GetPort();
        if( pPort == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        gint32 iRet = CloseLocalStream(
            pPort, iStreamId );

        if( pCallback != nullptr )
        {
            ObjPtr pObj;
            CCfgOpenerObj oTask( pCallback );

            ret = oTask.GetObjPtr(
                propMsgPtr, pObj );

            if( ERROR( ret ) )
                break;

            const IConfigDb* pReq = pObj;
            if( pReq == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            // inherit the information from the
            // request
            CReqBuilder oResp( pReq );
            oResp.ClearParams();
            oResp.Push( iStreamId );
            oResp.SetReturnValue( iRet );
            ret = SetResponse(
                pCallback, oResp.GetCfg() );
        }

    }while( 0 );


    return ret;
}

gint32 CRpcTcpBridge::OpenStream_Server(
    IEventSink* pCallback,
    gint32 iPeerStm,
    guint32 wProtocol )
{
    gint32 ret = 0;
    gint32  iNewStm = 0;

    do{
        if( !IsConnected() )
        {
            // NOTE: this is not a serious check
            ret = ERROR_STATE;
            break;
        }

        IPort* pPort = GetPort();

        gint32 iRet = OpenLocalStream( pPort,
            iPeerStm, wProtocol, iNewStm );

        if( pCallback != nullptr )
        {
            ObjPtr pObj;
            CCfgOpenerObj oTask( pCallback );

            ret = oTask.GetObjPtr(
                propMsgPtr, pObj );

            if( ERROR( ret ) )
                break;

            const IConfigDb* pReq = pObj;
            if( pReq == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            // inherit the information from the
            // request
            CReqBuilder oResp( pReq );

            // the return value
            oResp.ClearParams();
            oResp.Push( iNewStm );
            oResp.Push( ( guint32 )wProtocol );
            oResp.Push( iPeerStm );

            oResp.SetReturnValue( iRet );

            ret = SetResponse(
                pCallback, oResp.GetCfg() );
        }

    }while( 0 );

    return ret;
}

gint32 CRpcTcpBridge::InitUserFuncs()
{
    BEGIN_IFHANDLER_MAP( CRpcTcpBridge );

    ADD_SERVICE_HANDLER(
        CRpcTcpBridge::OpenStream,
        BRIDGE_METHOD_OPENSTM );

    ADD_SERVICE_HANDLER(
        CRpcTcpBridge::CloseStream,
        BRIDGE_METHOD_CLOSESTM );

    ADD_SERVICE_HANDLER(
        CRpcTcpBridge::EnableRemoteEvent,
        SYS_METHOD_ENABLERMTEVT );

    ADD_SERVICE_HANDLER(
        CRpcTcpBridge::DisableRemoteEvent,
        SYS_METHOD_DISABLERMTEVT );

    END_HANDLER_MAP;

    return 0;
}

gint32 CRpcTcpBridge::SendResponse(
    IConfigDb* pReq, CfgPtr& pResp )
{
    if( pReq == nullptr || pResp.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CReqBuilder oResp(
            ( IConfigDb* )pResp );

        CReqOpener oReq( pReq );

        guint32 dwFlags = 0;
        ret = oReq.GetCallFlags( dwFlags );
        if( ERROR( ret ) )
            break;

        if( !( dwFlags & CF_WITH_REPLY ) )
            break;

        if( ( dwFlags & CF_MESSAGE_TYPE_MASK ) !=
            DBUS_MESSAGE_TYPE_METHOD_CALL )
            break;

        DMsgPtr& pMsg = oResp[ 0 ];
        BufPtr pBuf( true );
        *pBuf = pMsg;

        IPort* pPort = GetPort();
        pPort = pPort->GetTopmostPort();

        IrpPtr pIrp( true );
        ret = pIrp->AllocNextStack( nullptr );
        if( ERROR( ret ) )
            break;

        IrpCtxPtr& pCtx = pIrp->GetTopStack(); 
        pPort->AllocIrpCtxExt( pCtx, ( PIRP )pIrp );

        pCtx->SetMajorCmd( IRP_MJ_FUNC );
        pCtx->SetMinorCmd( IRP_MN_IOCTL );
        pCtx->SetCtrlCode( CTRLCODE_SEND_RESP );
        pCtx->SetIoDirection( IRP_DIR_OUT ); 

        pCtx->SetReqData( pBuf );

        CPort* pPort2 = ( CPort* )pPort;
        CIoManager* pMgr = pPort2->GetIoMgr();
        ret = pMgr->SubmitIrp(
            PortToHandle( pPort ), pIrp );

        if( ret == STATUS_PENDING )
            break;

        if( ret == ERROR_STATE )
            break;

        if( ret == -EAGAIN )
            ret = STATUS_MORE_PROCESS_NEEDED;

        if( ERROR( ret ) )
            break;

    }while( 0 );

    return ret;
}

gint32 CRpcTcpBridge::OnProgressNotify(
    guint32 dwProgress,
    guint64 iTaskId )
{
    gint32 ret = 0;
    do{
        if( !IsConnected() )
        {
            // NOTE: this is not a serious state
            // check
            ret = ERROR_STATE;
            break;
        }

        CReqBuilder oNotifyReq( this );

        oNotifyReq.Push( iTaskId );
        oNotifyReq.Push( dwProgress );

        oNotifyReq.SetStrProp( propMethodName,
            IF_EVENT_PROGRESS );

        oNotifyReq.SetCallFlags( 
           DBUS_MESSAGE_TYPE_SIGNAL
           | CF_ASYNC_CALL );

        // no response 
        TaskletPtr pDummyTask;

        ret = pDummyTask.NewObj(
            clsid( CIfDummyTask ) );

        if( ERROR( ret ) )
            break;

        ret = BroadcastEvent(
            oNotifyReq.GetCfg(), pDummyTask );

        if( ret == STATUS_PENDING )
            ret = 0;

    }while( 0 );

    return ret;
}

gint32 CRpcTcpBridgeShared::OpenLocalStream(
    IPort* pPort,
    gint32 iStreamId,
    guint16 wProtoId,
    gint32& iStmIdOpened )
{
    if( pPort == nullptr )
        return -EINVAL;
        
    gint32 ret = 0;
    do{
        // start to read from the default
        // dbus stream
        IrpPtr pIrp( true );
        ret = pIrp->AllocNextStack( nullptr );
        if( ERROR( ret ) )
            break;

        if( IsReserveStm( iStreamId ) )
            return -EINVAL;

        IrpCtxPtr& pCtx = pIrp->GetTopStack(); 

        pCtx->SetMajorCmd( IRP_MJ_FUNC );
        pCtx->SetMinorCmd( IRP_MN_IOCTL );
        pCtx->SetCtrlCode( CTRLCODE_OPEN_STREAM_LOCAL_PDO );
        pCtx->SetIoDirection( IRP_DIR_INOUT ); 

        BufPtr pBuf( true );
        CParamList oReq;

        oReq.Push( iStreamId ); // 0: peer stm id
        oReq.Push( wProtoId );  // 1: protocol

        *pBuf = ObjPtr( 
            ( IConfigDb* )oReq.GetCfg() );

        pCtx->SetReqData( pBuf );

        // NOTE: this irp will only go through the
        // stream pdo, and not come down all the
        // way from the top of the port stack
        CPort* pPort2 = ( CPort* )pPort;
        CIoManager* pMgr = pPort2->GetIoMgr();
        ret = pMgr->SubmitIrpInternal(
            pPort, pIrp, false );

        if( ERROR( ret ) )
            break;

        if( SUCCEEDED( ret ) )
        {
            ret = 0;
            pCtx = pIrp->GetTopStack();

            CfgPtr pCfg;
            ret = pCtx->GetRespAsCfg( pCfg );
            if( ERROR( ret ) )
                break;

            CCfgOpener oCfg( ( IConfigDb* )pCfg );
            gint32 iRet = 0;

            ret = oCfg.GetIntProp(
                propReturnValue, ( guint32& )iRet );

            if( ERROR( ret ) )
                break;
            
            if( ERROR( iRet ) )
            {
                ret = iRet;
                break;
            }

            gint32 iNewStmId = 0;
            ret = oCfg.GetIntProp(
                0, ( guint32& )iNewStmId ); // 0: stream opened
            
            if( ERROR( ret ) )
                break;

            iStmIdOpened = iNewStmId;
        }

    }while( 0 );

    return ret;
}

gint32 CRpcTcpBridgeShared::CloseLocalStream(
    IPort* pPort, gint32 iStreamId )
{
    if( pPort == nullptr )
        return -EINVAL;
        
    gint32 ret = 0;
    do{
        // start to read from the default
        // dbus stream
        IrpPtr pIrp( true );
        ret = pIrp->AllocNextStack( nullptr );
        if( ERROR( ret ) )
            break;

        IrpCtxPtr& pCtx = pIrp->GetTopStack(); 

        pCtx->SetMajorCmd( IRP_MJ_FUNC );
        pCtx->SetMinorCmd( IRP_MN_IOCTL );
        pCtx->SetCtrlCode( CTRLCODE_CLOSE_STREAM_LOCAL_PDO );
        pCtx->SetIoDirection( IRP_DIR_OUT ); 

        BufPtr pBuf( true );

        // a local request, no need of CReqBuilder
        CParamList oParams;
        ret = oParams.Push( iStreamId );

        ObjPtr pObj = oParams.GetCfg();
        *pBuf = pObj;

        pCtx->SetReqData( pBuf );

        CPort* pPort2 = ( CPort* )pPort;
        CIoManager* pMgr = pPort2->GetIoMgr();
        pIrp->SetSyncCall( true );

        // NOTE: this irp will only go through the
        // stream pdo, and not come down all the
        // way from the top of the port stack
        ret = pMgr->SubmitIrpInternal(
            pPort, pIrp, false );

        if( ret == ERROR_STATE )
            break;

        if( ERROR( ret ) )
            break;

    }while( 0 );

    return ret;
}

gint32 CRpcTcpBridgeShared::ReadStream(
    gint32 iStreamId,
    BufPtr& pDestBuf,
    guint32 dwSizeToRead,
    IEventSink* pCallback )
{
    BufPtr pBuf( true );
    gint32 ret = ReadWriteStream( iStreamId,
        pBuf, dwSizeToRead, pCallback, true );
    if( ERROR( ret ) )
        return ret;

    pDestBuf = pBuf;
    return 0;
}

gint32 CRpcTcpBridgeShared::WriteStream(
    gint32 iStreamId,
    CBuffer* pSrcBuf,
    guint32 dwSizeToWrite,
    IEventSink* pCallback )
{
    return ReadWriteStream( iStreamId,
        pSrcBuf, dwSizeToWrite, pCallback, false );
}

gint32 CRpcTcpBridgeShared::ReadWriteStream(
    gint32 iStreamId,
    CBuffer* pSrcBuf,
    guint32 dwSize,
    IEventSink* pCallback,
    bool bRead )
{
    gint32 ret = 0;
    TaskletPtr pCbTask;

    do{
        CCfgOpener oCfg;
        CRpcInterfaceBase* pIf = GetIfPtr();
        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        guint32 dwCmdId;
        guint32 dwIoDir;

        if( bRead )
        {
            dwCmdId = IRP_MN_READ;
            dwIoDir = IRP_DIR_IN;
        }
        else
        {
            dwCmdId = IRP_MN_WRITE;
            dwIoDir = IRP_DIR_OUT;
        }

        oCfg.SetObjPtr( propIfPtr,
            ObjPtr( GetIfPtr() ) );

        oCfg.SetObjPtr( propEventSink,
            ObjPtr( pCallback ) );

        oCfg.SetIntProp( propCmdId, dwCmdId );

        ret = pCbTask.NewObj(
            clsid( CBdgeProxyReadWriteComplete ),
            oCfg.GetCfg() );

        if( ERROR( ret ) )
            break;

        // activatate the task to a intial state
        // without actual Read/Write
        ( *pCbTask )( eventZero );

        guint32 dwTimeoutSec = 0;

        CCfgOpenerObj oIfCfg( pIf ); 

        ret = oIfCfg.GetIntProp(
            propTimeoutSec, dwTimeoutSec );

        if( ERROR( ret ) )
            dwTimeoutSec = IFSTATE_DEFAULT_IOREQ_TIMEOUT;

        // start to read from the default
        // dbus stream
        IrpPtr pIrp( true );
        ret = pIrp->AllocNextStack( nullptr );
        if( ERROR( ret ) )
            break;

        if( iStreamId == TCP_CONN_DEFAULT_CMD )
        {
            ret = -EINVAL;
            break;
        }

        IrpCtxPtr& pCtx = pIrp->GetTopStack(); 

        pCtx->SetMajorCmd( IRP_MJ_FUNC );
        pCtx->SetMinorCmd( dwCmdId );
        pCtx->SetIoDirection( dwIoDir ); 

        BufPtr pBuf( true );
        CParamList oReq;

        oReq.SetIntProp(
            propStreamId, iStreamId );

        oReq.SetIntProp(
            propByteCount, dwSize );

        if( bRead )
        {
            *pBuf = ObjPtr( 
                ( IConfigDb* )oReq.GetCfg() );
        }
        else
        {
            oReq.Push( ObjPtr( pSrcBuf ) );
            *pBuf = ObjPtr( 
                ( IConfigDb* )oReq.GetCfg() );
        }

        pCtx->SetReqData( pBuf );

        EventPtr pEvent = pCbTask;
        pIrp->SetCallback( pEvent, 0 );
        pIrp->SetIrpThread( pIf->GetIoMgr() );
        if( dwTimeoutSec != 0 )
        {
            pIrp->SetTimer(
                dwTimeoutSec, pIf->GetIoMgr() );
        }

        // NOTE: this irp will only go through the
        // stream pdo, and not come down all the
        // way from the top of the port stack
        HANDLE hPort = pIf->GetPortHandle();
        CIoManager* pMgr = pIf->GetIoMgr();

        CCfgOpenerObj( ( CObjBase* )pCbTask ).
            SetObjPtr( propIrpPtr, pIrp );

        ret = pMgr->SubmitIrp( hPort, pIrp );

        if( ret == STATUS_PENDING )
        {
            if( pCallback != nullptr )
                break;

            CIfRetryTask* pRetryTask = pCbTask;
            if( pRetryTask != nullptr )
            {
                pRetryTask->WaitForComplete();
                ret = pRetryTask->GetError();
                if( SUCCEEDED( ret ) && bRead )
                {
                    IrpCtxPtr& pCtx = pIrp->GetTopStack();
                    pSrcBuf = pCtx->m_pRespData;
                }
            }
            else
            {
                ret = -EFAULT;
            }
        }
        else
        {
            CCfgOpenerObj( ( CObjBase* )pCbTask ).
                RemoveProperty( propIrpPtr );
            pIrp->RemoveCallback();
            pIrp->RemoveTimer();
            pIrp->ClearIrpThread();
        }

    }while( 0 );

    return ret;
}

gint32 CRpcTcpBridgeShared::SetupReqIrpListeningShared(
    IRP* pIrp,
    IConfigDb* pReqCall,
    IEventSink* pCallback )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    if( pCallback == nullptr ||
        pReqCall == nullptr )
        return -EINVAL;

    gint32 ret = 0;

    do{
        InterfPtr pIf = GetIfPtr();
        if( pIf.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        CIoManager* pMgr = nullptr;
        IrpCtxPtr& pIrpCtx = pIrp->GetTopStack(); 
        if( pIf->IsServer() )
        {
            CRpcTcpBridge* pBridge = pIf;
            ret = pBridge->CRpcTcpBridge::super::SetupReqIrp(
                pIrp, pReqCall, pCallback );
            pMgr = pBridge->GetIoMgr();
        }
        else
        {
            CRpcTcpBridgeProxy* pProxy = pIf;
            ret = pProxy->CRpcTcpBridgeProxy::super::SetupReqIrp(
                pIrp, pReqCall, pCallback );
            pMgr = pProxy->GetIoMgr();
        }

        if( ERROR( ret ) )
            break;

        CReqBuilder oReq( pReqCall );
        
        oReq.CopyProp( propStreamId, pCallback );
        oReq.CopyProp( propProtoId, pCallback );
        oReq.CopyProp( propCmdId, pCallback );
        oReq.CopyProp( propMethodName, pCallback );

        if( pIf->IsServer() )
        {
            oReq.SetIntProp(
                propMatchType, matchServer );
        }
        else
        {
            oReq.SetIntProp(
                propMatchType, matchClient );
        }

        guint32 dwProtoId = oReq[ propProtoId ];
        if( dwProtoId == protoDBusRelay )
        {
            oReq.SetCallFlags(
                CF_WITH_REPLY | CF_ASYNC_CALL |
                DBUS_MESSAGE_TYPE_METHOD_CALL );
            SetBdgeIrpStmId( pIrp,
                ( guint32& )oReq[ propStreamId ] );
        }
        else if( dwProtoId == protoControl )
        {
            oReq.SetCallFlags( CF_WITH_REPLY |
                CF_NON_DBUS | CF_ASYNC_CALL |
                DBUS_MESSAGE_TYPE_METHOD_CALL );
            pIrpCtx->SetNonDBusReq( true );
        }
        BufPtr pBuf( true );
        *pBuf = ObjPtr( pReqCall );

        pIrpCtx->SetReqData( pBuf );
        pIrpCtx->SetIoDirection( IRP_DIR_IN );
        pIrp->SetCallback( pCallback, 0 );
        pIrp->SetIrpThread( pMgr );

    }while( 0 );

    return ret;
}

