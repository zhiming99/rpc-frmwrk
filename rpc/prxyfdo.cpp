/*
 * =====================================================================================
 *
 *       Filename:  prxyfdo.cpp
 *
 *    Description:  implementation of the CDBusProxyFdo
 *
 *        Version:  1.0
 *        Created:  12/09/2016 09:48:56 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 * =====================================================================================
 */


#include <dbus/dbus.h>
#include <vector>
#include <string>
#include <regex>
#include "defines.h"
#include "prxyport.h"
#include "emaphelp.h"
#include "reqopen.h"


using namespace std;

#define PROXYFDO_LISTEN_INTERVAL 15
#define PROXYFDO_LISTEN_RETIRES  12

CDBusProxyFdo::CDBusProxyFdo( const IConfigDb* pCfg )
    : super( pCfg )
{
    SetClassId( clsid( CDBusProxyFdo ) );
}

gint32 CDBusProxyFdo::ClearDBusSetting(
    IMessageMatch* pMatch )
{
    // we don't touch dbus things
    return 0;
}

gint32 CDBusProxyFdo::SetupDBusSetting(
    IMessageMatch* pMatch )
{
    if( !IsConnected() )
        return ENOTCONN;
    return 0;
}

gint32 CDBusProxyFdo::BuildSendDataMsg(
    IRP* pIrp, DMsgPtr& pMsg )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    if( pMsg.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;

    do{
        ret = super::BuildSendDataMsg( pIrp, pMsg );
        if( ERROR( ret ) )
            break;

        // it is convenient to add ip address here
        // because in the pdo, the config data is
        // already serialized

        // correct the objpath and if name
        string strPath = DBUS_OBJ_PATH(
            MODNAME_RPCROUTER, OBJNAME_REQFWDR );

        pMsg.SetPath( strPath );

        string strIfName =
            DBUS_IF_NAME( IFNAME_REQFORWARDER );

        pMsg.SetInterface( strIfName );

        string strDest =
            DBUS_DESTINATION( MODNAME_RPCROUTER );

        pMsg.SetDestination( strDest );

    }while( 0 );

    return ret;
}

gint32 CDBusProxyFdo::HandleSendData( IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    // let's process the func irps
    if( pIrp->MajorCmd() != IRP_MJ_FUNC ||
        pIrp->MinorCmd() != IRP_MN_IOCTL )
        return -EINVAL;

    gint32 ret = 0;

    guint32 dwCtrlCode = pIrp->CtrlCode();
    IrpCtxPtr& pCtx = pIrp->GetCurCtx();

    do{
        DMsgPtr pMsg;
        ret = pMsg.NewObj();

        if( ERROR( ret ) )
            break;

        ret = BuildSendDataMsg( pIrp, pMsg );
        if( ERROR( ret ) )
            break;

        IPort* pPdoPort = GetLowerPort();
        if( pPdoPort == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        pIrp->AllocNextStack( pPdoPort );

        IrpCtxPtr& pNextIrpCtx =
            pIrp->GetTopStack(); 

        pNextIrpCtx->SetMajorCmd( IRP_MJ_FUNC );
        pNextIrpCtx->SetMinorCmd( IRP_MN_IOCTL );
        pNextIrpCtx->SetCtrlCode( dwCtrlCode );
        pPdoPort->AllocIrpCtxExt( pNextIrpCtx );

        BufPtr pBuf( true );
        *pBuf = pMsg;
        pNextIrpCtx->SetReqData( pBuf );

        ret = pPdoPort->SubmitIrp( pIrp );

        if(  ret == STATUS_PENDING )
            break;
        
        if( SUCCEEDED( ret ) )
        {
            // cannot understand how it can
            // succeed immediately
            ret = ERROR_FAIL;
        }

        pIrp->PopCtxStack();
        pCtx->SetStatus( ret );

    }while( 0 );

    return ret;
}

gint32 CDBusProxyFdo::SubmitIoctlCmd( IRP* pIrp )
{
    if( pIrp == nullptr
        || pIrp->GetStackSize() == 0 )
        return -EINVAL;

    // let's process the func irps
    if( pIrp->MajorCmd() != IRP_MJ_FUNC
        || pIrp->MinorCmd() != IRP_MN_IOCTL )
        return -EINVAL;

    gint32 ret = 0;

    switch( pIrp->CtrlCode() )
    {
    case CTRLCODE_LISTENING_FDO:
        {   
            // this is a fdo internal command and
            // should not be used by external
            // interfaces
            ret = HandleListeningFdo( pIrp );
            break;
        }
    case CTRLCODE_FETCH_DATA:
    case CTRLCODE_SEND_DATA:
        {
            // the SEND_DATA and FETCH_DATA are
            // specific for big data transfer. and
            // are now used only by the SendData
            // and FetchData method from the
            // interfaces
            ret = HandleSendData( pIrp );
            break;
        }
    default:
        {
            ret = super::SubmitIoctlCmd( pIrp );
            break;
        }
    }
    return ret;
}

gint32 CDBusProxyFdo::ScheduleDispEvtTask(
    DBusMessage* pMsg )
{
    if( pMsg == nullptr )
        return -EINVAL;
    // TODO: let's schedule a task to send out
    // an irp to listen to the event from
    // the remote server. this irp is 
    // maintained by the fdo
 
    gint32 ret = 0;
    do{
        CParamList oCfg;

        ret = oCfg.SetPointer( propIoMgr, GetIoMgr() );
        if( ERROR( ret ) )
            break;

        ret = oCfg.SetObjPtr(
            propPortPtr, ObjPtr( this ) );

        if( ERROR( ret ) )
            break;

        ret = oCfg.SetMsgPtr(
            propMsgPtr, DMsgPtr( pMsg ) );

        if( ERROR( ret ) )
            break;

        ret = GetIoMgr()->ScheduleTask(
            clsid( CProxyFdoDispEvtTask ), 
            oCfg.GetCfg() );

    }while( 0 );

    return ret;
}


gint32 CDBusProxyFdo::HandleListeningFdo(
    IRP* pIrp )
{

    if( pIrp == nullptr
        || pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;

    do{
        IrpCtxPtr& pCtx = pIrp->GetCurCtx();

        // client side I/O
        guint32 dwIoDir = pCtx->GetIoDirection();

        ret = -EINVAL;

        if( unlikely( dwIoDir != IRP_DIR_IN ) )
        {
            // wrong direction
            break;
        }

        IPort* pPdoPort = GetLowerPort();
        if( unlikely( pPdoPort == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }

        ret = pIrp->AllocNextStack( pPdoPort );
        IrpCtxPtr& pNextIrpCtx = pIrp->GetTopStack(); 

        // send down as a forward request 
        pNextIrpCtx->SetMajorCmd( IRP_MJ_FUNC );
        pNextIrpCtx->SetMinorCmd( IRP_MN_IOCTL );
        pNextIrpCtx->SetCtrlCode( CTRLCODE_LISTENING );

        pPdoPort->AllocIrpCtxExt( pNextIrpCtx );

        ret = pPdoPort->SubmitIrp( pIrp );

        if(  ret == STATUS_PENDING )
            break;

        if( SUCCEEDED( ret ) )
        {
            // let's dispatch this message to
            // all the listeners
            DMsgPtr pMsg = *pNextIrpCtx->m_pReqData;
            if( !pMsg.IsEmpty() )
            {
                ret = ScheduleDispEvtTask( pMsg );
            }
        }

        pIrp->PopCtxStack();
        pCtx->SetStatus( ret );

        break;

    }while( 0 );

    return ret;
}


/**
* @name HandleSendReq
* @{ the handler for the requests that will be
* forwarded to the remote server. The request will
* fail immediately if the remote server is not
* online. Actually it will forward the the request
* to the pdo with a FORWARD REQUEST to indicate
* the requst will send to the local
* CRpcReqForwarder and which will forward the
* request to the remote server */
/** pIrp: the m_pReqData contains a dbus message
 * to send  @} */

gint32 CDBusProxyFdo::HandleSendReq(
    IRP* pIrp )
{

    if( pIrp == nullptr
        || pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;

    do{
        IrpCtxPtr& pCtx = pIrp->GetCurCtx();

        // client side I/O
        guint32 dwIoDir = pCtx->GetIoDirection();

        ret = -EINVAL;
        DMsgPtr pMsg = *pCtx->m_pReqData;

        if( pMsg.IsEmpty() )
            break;

        if( IsIfSvrOnline( pMsg ) == ENOTCONN )
        {
            ret = -ENOTCONN;
            break;
        }

        if( ERROR( ret ) )
            break;

        if( dwIoDir == IRP_DIR_IN )
        {
            // wrong direction
            break;
        }

        IPort* pPdoPort = GetLowerPort();
        if( pPdoPort == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        ret = pIrp->AllocNextStack( pPdoPort );

        IrpCtxPtr& pNextIrpCtx = pIrp->GetTopStack(); 

        // send down as a forward request 
        pNextIrpCtx->SetMajorCmd( IRP_MJ_FUNC );
        pNextIrpCtx->SetMinorCmd( IRP_MN_IOCTL );
        pNextIrpCtx->SetCtrlCode( CTRLCODE_FORWARD_REQ );
        pNextIrpCtx->SetIoDirection( IRP_DIR_INOUT );
        pPdoPort->AllocIrpCtxExt( pNextIrpCtx );

        pNextIrpCtx->SetReqData( pCtx->m_pReqData );

        ret = pPdoPort->SubmitIrp( pIrp );
        if(  ret == STATUS_PENDING )
            break;

        if( SUCCEEDED( ret ) )
        {
            pCtx->SetRespData(
                pNextIrpCtx->m_pRespData );
        }

        pIrp->PopCtxStack();
        pCtx->SetStatus( ret );

        break;

    }while( 0 );

    return ret;
}

gint32 CDBusProxyFdo::MatchExist( IRP* pIrp )
{
    gint32 ret = 0;
    if( pIrp == nullptr
        || pIrp->GetStackSize() == 0 )
        return -EINVAL;

    MatchMap* pMap;

    IrpCtxPtr& pCtx = pIrp->GetCurCtx();
    GetMatchMap( pIrp, pMap );

    MatchPtr pMatch =
        ( ObjPtr& )*pCtx->m_pReqData;

    CStdRMutex oPortLock( GetLock() );

    if( pMap->find( pMatch ) == pMap->end() )
    {
        ret = ERROR_FALSE;
    }
    else
    {
        ret = ( *pMap )[ pMatch ].RefCount();
    }
        
    return ret;
}

gint32 CDBusProxyFdo::HandleRegMatchInternal(
    IRP* pIrp, bool bReg )
{
    // well, we need to first send this
    // request to the remote interface
    // before register it locally
    if( pIrp == nullptr
        || pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;

    do{
        // first check locally if we have already
        // registered, before we cross the network
        ret = MatchExist( pIrp );
        if( ret == ERROR_FALSE && !bReg )
        {
            ret = -ENOENT;
            break;
        }
        else if( ( ret == ERROR_FALSE && bReg )
            || ( ret == 1 && !bReg ) )
        {
            // move on
            // full register/unregister
        }
        else if( ERROR( ret ) )
        {
            break;
        }
        else if( ret == 0 )
        {
            ret = ERROR_FAIL;
            break;
        }
        else if( ret > 0 && bReg )
        {
            ret = super::HandleRegMatch( pIrp );
            break;
        }
        else if( ret > 1 && !bReg )
        {
            ret = super::HandleUnregMatch( pIrp );
            break;
        }
        
        // let's start a full register
        IrpCtxPtr pCtx = pIrp->GetCurCtx();

        IPort* pPdoPort = GetLowerPort();
        if( pPdoPort == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ret = pIrp->AllocNextStack( pPdoPort );
        IrpCtxPtr pNextIrpCtx = pIrp->GetTopStack(); 
        pNextIrpCtx->SetMajorCmd( IRP_MJ_FUNC );
        pNextIrpCtx->SetMinorCmd( IRP_MN_IOCTL );
        pNextIrpCtx->SetIoDirection( IRP_DIR_INOUT );
        pNextIrpCtx->SetReqData( pCtx->m_pReqData );

        if( bReg )
        {
            pNextIrpCtx->SetCtrlCode(
                CTRLCODE_RMT_REG_MATCH );
        }
        else
        {
            pNextIrpCtx->SetCtrlCode(
                CTRLCODE_RMT_UNREG_MATCH );
        }


        pPdoPort->AllocIrpCtxExt( pNextIrpCtx );
        ret = pPdoPort->SubmitIrp( pIrp );
        if(  ret == STATUS_PENDING )
            break;

        if( SUCCEEDED( ret ) )
        {
            if( bReg )
            {
                ret = super::HandleRegMatch( pIrp );
            }
            else
            {
                ret = super::HandleUnregMatch( pIrp );
            }
        }

        pIrp->PopCtxStack();
        pCtx->SetStatus( ret );

        break;

    }while( 0 );

    return ret;
}

gint32 CDBusProxyFdo::HandleRegMatch(
    IRP* pIrp )
{
    if( pIrp == nullptr
        || pIrp->GetStackSize() == 0 )
        return -EINVAL;

    return HandleRegMatchInternal(
        pIrp, true );
}

gint32 CDBusProxyFdo::HandleUnregMatch(
    IRP* pIrp )
{
    if( pIrp == nullptr
        || pIrp->GetStackSize() == 0 )
        return -EINVAL;

    return HandleRegMatchInternal(
        pIrp, false );
}

gint32 CDBusProxyFdo::CompleteIoctlIrp(
    IRP* pIrp )
{
    gint32 ret = 0;

    do{
        if( pIrp == nullptr 
           || pIrp->GetStackSize() == 0 )
        {
            ret = -EINVAL;
            break;
        }

        if( pIrp->MajorCmd() != IRP_MJ_FUNC 
            && pIrp->MinorCmd() != IRP_MN_IOCTL )
        {
            ret = -EINVAL;
            break;
        }
        
        IrpCtxPtr& pCtx =
            pIrp->GetCurCtx();

        IrpCtxPtr& pTopCtx =
            pIrp->GetTopStack();

        ret = pTopCtx->GetStatus();
        guint32 dwCtrlCode = pIrp->CtrlCode();

        switch( dwCtrlCode )
        {
        case CTRLCODE_FORWARD_REQ:
            {
                // the data has been prepared in
                // the DispatchRespMsg, we don't
                // need further actions
                if( SUCCEEDED( ret ) )
                    pCtx->SetRespData( pTopCtx->m_pRespData );
                
                break;
            }
        case CTRLCODE_LISTENING_FDO:
            {
                // schedule a task to dispatch the
                // event
                if( ERROR( ret ) )
                    break;

                DMsgPtr pMsg = *pTopCtx->m_pRespData;
                if( !pMsg.IsEmpty() )
                {
                    ret = ScheduleDispEvtTask( pMsg );
                }
                else
                {
                    ret = -EINVAL;
                }
                break;
            }
        case CTRLCODE_REG_MATCH:
            {
                if( SUCCEEDED( ret ) )
                {
                    ret = super::HandleRegMatch( pIrp );
                }
                break;
            }
        case CTRLCODE_UNREG_MATCH:
            {
                if( SUCCEEDED( ret ) )
                {
                    ret = super::HandleUnregMatch( pIrp );
                }
                break;
            }

        case CTRLCODE_FETCH_DATA:
        case CTRLCODE_SEND_DATA:
            {
                if( ERROR( ret ) )
                    break;

                DMsgPtr pRespMsg =
                    *pTopCtx->m_pRespData;

                if( !pRespMsg.IsEmpty() )
                {
                    guint32 dwVal = 0;
                    ret = pRespMsg.GetIntArgAt(
                        0, dwVal );

                    if( ERROR( ret ) )
                        break;

                    ret = ( gint32& )dwVal;
                    if( ERROR( ret ) )
                    {
                        break;
                    }

                    // return the response message
                    pCtx->m_pRespData =
                        pTopCtx->m_pRespData;

                    break;
                }
                else
                {
                    ret = -EFAULT;
                }
                break;

            }
            
        default:
            ret = super::CompleteIoctlIrp( pIrp );
            break;
        }

        pCtx->SetStatus( ret );

        // to prevent the caller to copy the
        // return code from the pTopCtx, pop the
        // top stack here.
        if( pIrp->IsIrpHolder() )
            pIrp->PopCtxStack();

    }while( 0 );

    return ret;
}

gint32 CProxyFdoDispEvtTask::operator()(
    guint32 dwContext )
{
    CCfgOpener oParams( ( IConfigDb* )m_pCtx );

    gint32 ret = 0;

    do{
        CIoManager* pMgr;
        ret = oParams.GetPointer( propIoMgr, pMgr );
        if( ERROR( ret ) )
            break;

        ObjPtr portPtr;
        ret = oParams.GetObjPtr(
            propPortPtr, portPtr );

        if( ERROR( ret ) )
            break;

        CDBusProxyFdo* pPort = portPtr;

        if( pPort == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        BufPtr pBuf( true );

        DMsgPtr pEvtMsg;

        ret = oParams.GetMsgPtr(
            propMsgPtr, pEvtMsg );

        if( ERROR( ret ) )
            break;

        string strMethod =
            pEvtMsg.GetMember();

        if( strMethod == SYS_EVENT_KEEPALIVE )
        {
            // fix the obj path and interface name
            ObjPtr pObj;
            string strIfName, strObjPath;
            ret = pEvtMsg.GetObjArgAt( 0, pObj );
            if( ERROR( ret ) )
                break;

            IConfigDb* pReq = pObj;
            if( pReq == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            CReqOpener oReq( pReq );
            ret = oReq.GetStrProp(
                2, strIfName );

            if( ERROR( ret ) )
                break;

            ret = oReq.GetStrProp(
                3, strObjPath );

            pEvtMsg.SetInterface( strIfName );
            pEvtMsg.SetPath( strObjPath );
        }

        *pBuf = pEvtMsg;
        ret = pPort->DispatchData( *pBuf );

    }while( 0 );

    return SetError( ret );
}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  CProxyFdoListenTask
 *      Method:  CProxyFdoListenTask :: Process()
 * Description:  send out an irp to listen to the event from rpc forwader
 *   Parameter:  dwContext ignored
 *              propIoMgr the pointer to the iomanager
 *              propPortPtr the pointer to this fdo port
 *--------------------------------------------------------------------------------------
 */
gint32 CProxyFdoListenTask::Process(
    guint32 dwContext )
{
    CCfgOpener oParams(
        ( IConfigDb* )GetConfig() );
    gint32 ret = 0;

    do{
        if( dwContext == eventIrpComp )
        {
            IrpPtr pIrp;
            ret = GetIrpFromParams( pIrp );
            if( ERROR( ret ) )
                break;

            if( pIrp.IsEmpty() ||
                pIrp->GetStackSize() == 0 )   
                break;

            ret = pIrp->GetStatus();
            if( ret == -EAGAIN ||
                ret == -ENOTCONN ||
                ret == -ETIMEDOUT ||
                ret == STATUS_MORE_PROCESS_NEEDED )
            {
                if( CanRetry() )
                    ret = STATUS_MORE_PROCESS_NEEDED;
                else if( ret == STATUS_MORE_PROCESS_NEEDED ) 
                    ret = -ETIMEDOUT;
                break;
            }
            if( ERROR( ret ) )
                break;
        }

        CIoManager* pMgr;
        ret = oParams.GetPointer( propIoMgr, pMgr );
        if( ERROR( ret ) )
            break;

        ObjPtr portPtr;
        ret = oParams.GetObjPtr(
            propPortPtr, portPtr );

        if( ERROR( ret ) )
            break;

        IPort* pPort = portPtr;
        if( pPort == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        IrpPtr pIrp( true );
        pIrp->AllocNextStack( pPort );

        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        pPort->AllocIrpCtxExt( pCtx );

        pCtx->SetMajorCmd( IRP_MJ_FUNC );
        pCtx->SetMinorCmd( IRP_MN_IOCTL );
        pCtx->SetCtrlCode( CTRLCODE_LISTENING_FDO );

        pCtx->SetIoDirection( IRP_DIR_IN ); 
        pIrp->SetSyncCall( false );

        // NOTE: no timer here
        //
        // set a callback
        pIrp->SetCallback( this, 0 );

        ret = pMgr->SubmitIrpInternal(
            pPort, pIrp, false );

        if( ret != STATUS_PENDING )
            pIrp->RemoveCallback();

        // continue to get next event message
        if( SUCCEEDED( ret ) )
            continue;

        switch( ret )
        {

        case STATUS_PENDING:
            break;

        case -ETIMEDOUT:
        case -ENOTCONN:
            {
                DebugPrint( 0,
                    "The server is not online?, retry scheduled..." );
            }
        case -EAGAIN:
            {
                ret = STATUS_MORE_PROCESS_NEEDED;
                break;
            }

        case ERROR_PORT_STOPPED:
        case -ECANCELED:
            {
                // if canncelled, or disconned, no
                // need to continue
                break;
            }

        // otherwise something must be wrong and
        // stop emitting irps
        }

        break;

    }while( 1 );

    return SetError( ret );
}

gint32 CDBusProxyFdo::PostStart(
    IRP* pIrp )
{
    if( pIrp == nullptr
        || pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;

    ret = super::PostStart( pIrp );
    if( ERROR( ret ) )
        return ret;
    do{
        IPort* pPdoPort = GetLowerPort();
        if( unlikely( pPdoPort == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }

        CCfgOpenerObj oPortCfg( this );
        ret = oPortCfg.CopyProp(
            propIpAddr, pPdoPort );

        if( ERROR( ret ) )
            break;

        if( true )
        {
            CCfgOpener matchCfg;
            ret = matchCfg.SetStrProp(
                propObjPath, DBUS_SYS_OBJPATH );

            if( ERROR( ret ) )
                break;

            ret = matchCfg.SetStrProp(
                propIfName, DBUS_SYS_INTERFACE );

            if( ERROR( ret ) )
                break;

            ret = matchCfg.SetIntProp(
                propMatchType, matchClient );

            if( ERROR( ret ) )
                break;

            m_matchModOnOff.NewObj(
                clsid( CMessageMatch ),
                matchCfg.GetCfg() );
        }

        m_matchDBusOff.NewObj(
            clsid( CDBusDisconnMatch ),
            nullptr );
        
        if( true )
        {
            CCfgOpener matchRmtEvt;

            string strPath = DBUS_OBJ_PATH(
                MODNAME_RPCROUTER, OBJNAME_REQFWDR );

            string strIfName =
                DBUS_IF_NAME( IFNAME_REQFORWARDER );

            ret = matchRmtEvt.SetStrProp(
                propObjPath, strPath );

            if( ERROR( ret ) )
                break;

            ret = matchRmtEvt.SetStrProp(
                propIfName, strIfName );

            if( ERROR( ret ) )
                break;

            ret = matchRmtEvt.SetStrProp(
                propMethodName,
                string( SYS_EVENT_RMTSVREVENT ) );

            if( ERROR( ret ) )
                break;

            ret = matchRmtEvt.SetIntProp(
                propMatchType, matchClient );

            if( ERROR( ret ) )
                break;

            ret = matchRmtEvt.CopyProp(
                propIpAddr, this );

            if( ERROR( ret ) )
            {
                break;
            }

            ret = m_matchRmtSvrEvt.NewObj(
                clsid( CProxyMsgMatch ),
                matchRmtEvt.GetCfg() );

            if( ERROR( ret ) )
                break;
        }

    }while( 0 );

    return ret;
}

gint32 CDBusProxyFdo::PreStop(
    IRP* pIrp )
{
    m_matchModOnOff.Clear();
    m_matchDBusOff.Clear();
    m_matchRmtSvrEvt.Clear();

    return super::PreStop( pIrp );
}

gint32 CDBusProxyFdo::OnPortReady(
    IRP* pIrp )
{
    if( pIrp == nullptr
        || pIrp->GetStackSize() == 0 )
        return -EINVAL;

    // TODO: let's schedule a task to send out an
    // irp to listen to the event from the remote
    // server. this irp is maintained by the fdo
 
    gint32 ret = 0;
    do{
        CParamList oCfg;

        ret = oCfg.SetPointer( propIoMgr, GetIoMgr() );
        if( ERROR( ret ) )
            break;

        ret = oCfg.SetObjPtr(
            propPortPtr, ObjPtr( this ) );

        if( ERROR( ret ) )
            break;


        ret = oCfg.SetIntProp(
            propRetries, PROXYFDO_LISTEN_RETIRES );

        if( ERROR( ret ) )
            break;

        ret = oCfg.SetIntProp(
            propIntervalSec, PROXYFDO_LISTEN_INTERVAL );

        if( ERROR( ret ) )
            break;

        GetIoMgr()->ScheduleTask(
            clsid( CProxyFdoListenTask ),
            oCfg.GetCfg() );

    }while( 0 );

    ret = super::OnPortReady( pIrp );

    return ret;
}

gint32 CProxyFdoRmtDBusOfflineTask::operator()(
    guint32 dwContext )
{

    gint32 ret = 0;

    CParamList a( ( IConfigDb* )m_pCtx );
    CIoManager* pMgr = nullptr;
    string strIfName;

    do{
        ret = a.GetPointer( propIoMgr, pMgr );
        if( ERROR( ret ) )
            break;

        ObjPtr pObj;
        ret = a.GetObjPtr(
            propPortPtr, pObj );
        if( ERROR( ret ) )
            break;

        CPort* pPort = pObj;
        if( pPort == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        string strIpAddr;
        ret = a.GetStrProp(
            propIpAddr, strIpAddr );

        if( ERROR( ret ) )
            break;

        // broadcast this event the pnp manager
        // will stop all the ports
        CEventMapHelper< CPort > oEvtHelper( pPort );

        guint32 eventId = eventRmtDBusOffline;

        guint32 hPort =
            PortToHandle( ( ( IPort* )this ) );

        // we will pass the ip address and the
        // port handle to the subscribers
        ret = oEvtHelper.BroadcastEvent(
            eventConnPoint, eventId,
            hPort, ( guint32* )strIpAddr.c_str() );

    }while( 0 );

    return ret;
}

DBusHandlerResult
CDBusProxyFdo::PreDispatchMsg(
    gint32 iMsgType, DBusMessage* pMsg )
{

    DBusHandlerResult ret =
        DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    CStdRMutex oPortLock( GetLock() );

    guint32 dwPortState = GetPortState();

    if( !CanAcceptMsg( dwPortState ) )
        return DBUS_HANDLER_RESULT_HANDLED;

    oPortLock.Unlock();

    do{
        gint32 ret1 =
            m_matchModOnOff->IsMyMsgIncoming( pMsg );

        if( SUCCEEDED( ret1 ) )
        {
            ret = DispatchDBusSysMsg( pMsg );

            if( ret != DBUS_HANDLER_RESULT_NOT_YET_HANDLED )
                break;

            ret = super::PreDispatchMsg(
                iMsgType, pMsg );

            break;
        }

        ret1 = m_matchDBusOff->IsMyMsgIncoming( pMsg );
        if( SUCCEEDED( ret1 ) )
        {
            // remote dbus system is about to shut down,
            // let's notify the pnp manager
            ret1 = 0;
            do{

                CParamList oParams;
                ret1 = oParams.SetPointer( propIoMgr, GetIoMgr() );
                if( ERROR( ret1 ) )
                    break;

                ret1 = oParams.SetObjPtr(
                    propPortPtr, ObjPtr( this ) );

                if( ERROR( ret1 ) )
                    break;

                ret1 = oParams.CopyProp(
                    propIpAddr, this );

                if( ERROR( ret1 ) )
                    break;

                ret1 = GetIoMgr()->ScheduleTask(
                    clsid( CProxyFdoRmtDBusOfflineTask ),
                    oParams.GetCfg() );

            }while( 0 );

            if( SUCCEEDED( ret1 ) )
                ret = DBUS_HANDLER_RESULT_HALT;

            break;
        }

        ret1 = m_matchRmtSvrEvt->IsMyMsgIncoming( pMsg );
        if( SUCCEEDED( ret1 ) )
        {
            DMsgPtr ptrMsg( pMsg );
            std::string strMethod = ptrMsg.GetMember();
            if( strMethod != SYS_EVENT_RMTSVREVENT )
                break;

            BufPtr pBuf( true );
            bool bOnline = false;
            ret1 = ptrMsg.GetBoolArgAt(
                1, bOnline );

            if( ERROR( ret1 ) )
                break;

            oPortLock.Lock();
            SetConnected( bOnline );
            if( !bOnline )
            {
                // all the matches are offline now
                MatchMap* pMatchMap;
                ret1 = GetMatchMap(
                    nullptr, pMatchMap );

                if( ERROR( ret1 ) )
                    break;

                for( auto oMe : *pMatchMap )
                {
                    oMe.second.SetConnected(
                        bOnline );
                }
            }
        }
        else
        {
            ret = super::PreDispatchMsg(
                iMsgType, pMsg );
        }

    }while( 0 );

    return ret;
}

gint32 CProxyFdoModOnOfflineTask::operator()(
    guint32 dwContext )
{

    gint32 ret = 0;

    CParamList a( ( IConfigDb* )m_pCtx );
    CIoManager* pMgr = nullptr;
    string strModName;

    do{
        ret = a.GetPointer( propIoMgr, pMgr );
        if( ERROR( ret ) )
            break;

        guint32 dwFlags = 0;
        ret = a.Pop( dwFlags );
        if( ERROR( ret ) )
            break;

        bool bOnline = 
            ( ( dwFlags & 0x01 ) > 0 );

        ret = a.GetStrProp(
            propModName, strModName );
        if( ERROR( ret ) )
            break;

        ObjPtr pObj;
        ret = a.GetObjPtr(
            propPortPtr, pObj );
        if( ERROR( ret ) )
            break;

        CPort* pPort = pObj;
        if( pPort == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        string strIpAddr;
        ret = a.GetStrProp(
            propIpAddr, strIpAddr );

        if( ERROR( ret ) )
            break;

        // broadcast this event
        // the pnp manager will stop all the
        // ports
        CEventMapHelper< CPort > oEvtHelper( pPort );

        guint32 eventId = eventRmtModOnline;

        if( bOnline )
            eventId = eventRmtModOffline;

        ret = oEvtHelper.BroadcastEvent(
            eventConnPoint, eventId,
            ( guint32 )strIpAddr.c_str(),
            ( guint32* )strModName.c_str() );

    }while( 0 );

    return ret;
}

gint32 CDBusProxyFdo::ScheduleModOnOfflineTask(
    const string strModName, guint32 dwFlags )
{
    gint32 ret = 0;

    do{
        CParamList oParams;
        ret = oParams.SetPointer( propIoMgr, GetIoMgr() );
        if( ERROR( ret ) )
            break;

        guint32 dwFlags = 0;
        ret = oParams.Push( dwFlags );
        if( ERROR( ret ) )
            break;

        ret = oParams.SetStrProp(
            propModName, strModName );
        if( ERROR( ret ) )
            break;

        ret = oParams.SetObjPtr(
            propPortPtr, ObjPtr( this ) );

        if( ERROR( ret ) )
            break;

        ret = oParams.CopyProp(
            propIpAddr, this );
        
        if( ERROR( ret ) )
            break;

        ret = GetIoMgr()->ScheduleTask(
            clsid( CProxyFdoModOnOfflineTask ),
            oParams.GetCfg() );

    }while( 0 );

    return ret;
}

