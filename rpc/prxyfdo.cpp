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
 *      Copyright:  2019 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
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

extern gint64 GetRandom();
static std::atomic< int > s_atmSeqNo( GetRandom() );
extern CConnParamsProxy GetConnParams(
    const CObjBase* pObj );

CDBusProxyFdo::CDBusProxyFdo( const IConfigDb* pCfg )
    : super( pCfg )
{
    SetClassId( clsid( CDBusProxyFdo ) );
}

gint32 CDBusProxyFdo::ClearDBusSetting(
    IMessageMatch* pMatch )
{ return 0; }

gint32 CDBusProxyFdo::SetupDBusSetting(
    IMessageMatch* pMatch )
{ return 0; }

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
        IrpCtxPtr& pCtx = pIrp->GetCurCtx();
        CfgPtr pCfg;
        ret = pCtx->GetReqAsCfg( pCfg );
        if( ERROR( ret ) )
            break;

        CCfgOpener oParams( ( IConfigDb* )pCfg );
        IConfigDb* pDataDesc = nullptr;
        ret = oParams.GetPointer( 0, pDataDesc );
        if( ERROR( ret ) )
            break;

        CCfgOpener oDataDesc( pDataDesc );

        // transfer context provides some
        // transportation and security related
        // information.
        //
        CCfgOpener oTransCtx;
        ret = oTransCtx.CopyProp(
            propConnHandle, this );
        if( ERROR( ret ) )
            break;

        oTransCtx.CopyProp(
            propRouterPath, this );

        oDataDesc.SetPointer( propTransCtx,
            ( IConfigDb* )oTransCtx.GetCfg() );

        ret = super::BuildSendDataMsg(
            pIrp, pMsg );

        if( ERROR( ret ) )
            break;

        // it is convenient to add ip address here
        // because in the pdo, the config data is
        // already serialized

        // correct the objpath and if name
        string strRtName;
        GetIoMgr()->GetRouterName( strRtName );

        string strPath = DBUS_OBJ_PATH(
            strRtName, OBJNAME_REQFWDR );

        pMsg.SetPath( strPath );

        string strIfName =
            DBUS_IF_NAME( IFNAME_REQFORWARDER );

        pMsg.SetInterface( strIfName );

        string strDest = DBUS_DESTINATION2(
                strRtName,
                OBJNAME_REQFWDR );

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
    IrpCtxPtr pCtx = pIrp->GetCurCtx();

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
        pNextIrpCtx->SetIoDirection( IRP_DIR_INOUT );
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
    // let's process the func irps
    IrpCtxPtr pCtx = pIrp->GetCurCtx();

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

    if( ret != STATUS_PENDING )
        pCtx->SetStatus( ret );

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
        IrpCtxPtr pCtx = pIrp->GetCurCtx();

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
            pCtx->SetRespData(
                pNextIrpCtx->m_pRespData );
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
        IrpCtxPtr pCtx = pIrp->GetCurCtx();

        // client side I/O
        guint32 dwIoDir = pCtx->GetIoDirection();
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

        // set the serial before forwarding
        // for message deserialization only.
        pMsg.SetSerial( s_atmSeqNo++ );

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

gint32 CDBusProxyFdo::HandleRegMatchLocal(
    IRP* pIrp )
{
    gint32 ret = 0;
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    do{
        IrpCtxPtr& pTopCtx =
            pIrp->GetTopStack();

        ret = pTopCtx->GetStatus();
        if( ERROR( ret ) )
            break;

        MatchMap* pMap;
        IrpCtxPtr& pCtx = pIrp->GetCurCtx();
        GetMatchMap( pIrp, pMap );
        MatchPtr pMatch =
            ( ObjPtr& )*pCtx->m_pReqData;

        CStdRMutex oPortLock( GetLock() );
        MatchMap::iterator itr =
            pMap->find( pMatch );

        bool bConnected = true;
        if( ret == ENOTCONN )
            bConnected = false;

        if( itr == pMap->end() )
        {
            MATCH_ENTRY oMe;
            oMe.SetConnected( bConnected );
            oMe.AddRef();
            ( *pMap )[ pMatch ] = oMe;
        }
        else
        {
            MATCH_ENTRY& oMe = itr->second;
            oMe.AddRef();
            oMe.SetConnected( bConnected );
        }
        ret = -ret;

    }while( 0 );

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

        BufPtr pReqData( true );
        *pReqData = *pCtx->m_pReqData;

        pNextIrpCtx->SetReqData( pReqData );

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
        if( likely( ret == STATUS_PENDING ) )
            break;

        if( bReg )
        {
            ret = HandleRegMatchLocal( pIrp );
        }
        else
        {
            ret = super::HandleUnregMatch( pIrp );
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
        
        IrpCtxPtr pCtx =
            pIrp->GetCurCtx();

        IrpCtxPtr& pTopCtx =
            pIrp->GetTopStack();

        ret = pTopCtx->GetStatus();
        guint32 dwCtrlCode = pIrp->CtrlCode();

        switch( dwCtrlCode )
        {
        case CTRLCODE_SEND_REQ:
            {
                // the data has been prepared in
                // the DispatchRespMsg, we don't
                // need further actions
                if( pTopCtx->GetCtrlCode() !=
                    CTRLCODE_FORWARD_REQ )
                    break;

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
                    pCtx->SetRespData(
                        pTopCtx->m_pRespData );
                }
                else
                {
                    ret = -EINVAL;
                }
                break;
            }
        case CTRLCODE_REG_MATCH:
            {
                ret = HandleRegMatchLocal( pIrp );
                break;
            }
        case CTRLCODE_UNREG_MATCH:
            {
                // this is a request of best efforts
                // whether error or not  from remote.
                // the unregistration process moves on
                ret = super::HandleUnregMatch( pIrp );
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
        if( !pIrp->IsIrpHolder() )
            pIrp->PopCtxStack();

    }while( 0 );

    return ret;
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

    CIoManager* pMgr;
    ret = oParams.GetPointer( propIoMgr, pMgr );
    if( ERROR( ret ) )
        return SetError( ret );

    ObjPtr portPtr;
    ret = oParams.GetObjPtr(
        propPortPtr, portPtr );

    if( ERROR( ret ) )
        return SetError( ret );

    CDBusProxyFdo* pPort = portPtr;
    if( pPort == nullptr )
    {
        ret = -EINVAL;
        return SetError( ret );
    }

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

            IrpCtxPtr& pCtx = pIrp->GetTopStack();
            DMsgPtr pMsg = *pCtx->m_pRespData;
            bool bEvent = false;
            if( pMsg.GetType() ==
                DBUS_MESSAGE_TYPE_SIGNAL )
                bEvent = true;

            if( !pMsg.IsEmpty() )
            {
                ret = pPort->DispatchData(
                    pCtx->m_pRespData );
                if( ERROR( ret ) && !bEvent )
                {
                    DebugPrint( ret,
                        "Failed to Dispatch Incoming event" );
                    ret = 0;
                }
            }
        }
        else if( dwContext == eventCancelTask )
        {
            IRP* pIrp = nullptr;
            ret = oParams.GetPointer(
                propIrpPtr, pIrp );
            if( SUCCEEDED( ret ) )
            {
                CStdRMutex oIrpLock(
                    pIrp->GetLock() );
                ret = pIrp->CanContinue(
                    IRP_STATE_READY );
                if( SUCCEEDED( ret ) )
                {
                    pIrp->RemoveCallback();
                    oIrpLock.Unlock();
                    pMgr->CancelIrp( pIrp,
                        true, -ECANCELED );
                }
            }
            oParams.RemoveProperty(
                propPortPtr );
            oParams.RemoveProperty(
                propIoMgr );
            ret = -ECANCELED;
            break;
        }

        IrpPtr pIrp( true );
        pIrp->AllocNextStack( pPort );

        IrpCtxPtr pCtx = pIrp->GetTopStack();

        pCtx->SetMajorCmd( IRP_MJ_FUNC );
        pCtx->SetMinorCmd( IRP_MN_IOCTL );
        pCtx->SetCtrlCode( CTRLCODE_LISTENING_FDO );
        pPort->AllocIrpCtxExt( pCtx );

        pCtx->SetIoDirection( IRP_DIR_IN ); 
        pIrp->SetSyncCall( false );

        // NOTE: no timer here
        //
        // set a callback
        pIrp->SetCallback( this, 0 );
        oParams.SetPointer( propIrpPtr,
            ( PIRP )pIrp );

        ret = pMgr->SubmitIrpInternal(
            pPort, pIrp, false );

        if( ret != STATUS_PENDING )
            pIrp->RemoveCallback();

        // continue to get next event message
        if( SUCCEEDED( ret ) )
        {
            DMsgPtr pMsg = *pCtx->m_pRespData;
            if( !pMsg.IsEmpty() )
            {
                bool bEvent = false;
                if( pMsg.GetType() ==
                    DBUS_MESSAGE_TYPE_SIGNAL )
                    bEvent = true;

                ret = pPort->DispatchData(
                    pCtx->m_pRespData );
                if( ERROR( ret ) && !bEvent )
                {
                    DebugPrint( ret,
                        "Failed to Dispatch Incoming event" );
                    ret = 0;
                }
            }
            else
            {
                ret = -EBADMSG;
                break;
            }
            continue;
        }

        switch( ret )
        {

        case STATUS_PENDING:
            break;

        case -ENOTCONN:
            {
                DebugPrint( 0,
                    "The server is not online?, retry scheduled..." );
                // fall through
            }
        case -ETIMEDOUT:
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

    if( ret != STATUS_PENDING )
    {
        oParams.RemoveProperty(
            propIrpPtr );
    }

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
        PortPtr pPdo;
        ret = GetPdoPort( pPdo );
        if( ERROR( ret ) )
            break;

        IPort* pPdoPort = pPdo;
        if( unlikely( pPdoPort == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }

        CCfgOpenerObj oPortCfg( this );

        ret = oPortCfg.CopyProp(
            propConnHandle, pPdoPort );

        if( ERROR( ret ) )
            break;

        ret = oPortCfg.CopyProp(
            propConnParams, pPdoPort );

        if( ERROR( ret ) )
            break;

        ret = oPortCfg.CopyProp(
            propRouterPath, pPdoPort );

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

            string strRtName;
            GetIoMgr()->GetRouterName( strRtName );

            string strPath = DBUS_OBJ_PATH(
                strRtName, OBJNAME_REQFWDR );

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
                propConnHandle, this );

            if( ERROR( ret ) )
                break;

            CConnParamsProxy oConn =
                GetConnParams( this );

            ret = matchRmtEvt.CopyProp(
                propRouterPath, this );

            if( ERROR( ret ) )
                break;

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

    ( *m_pListenTask )( eventCancelTask );

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

        ret = m_pListenTask.NewObj(
            clsid( CProxyFdoListenTask ),
            oCfg.GetCfg() );

        if( ERROR( ret ) )
            break;

        GetIoMgr()->RescheduleTask(
            m_pListenTask );

    }while( 0 );

    ret = super::OnPortReady( pIrp );

    return ret;
}

gint32 CProxyFdoRmtDBusOfflineTask::operator()(
    guint32 dwContext )
{

    gint32 ret = 0;
    CParamList oTaskCfg(
        ( IConfigDb* )GetConfig() );

    do{
        CPort* pPort = nullptr;
        ret = oTaskCfg.GetPointer(
            propPortPtr, pPort );
        if( ERROR( ret ) )
            break;

        guint32 eventId = eventRmtDBusOffline;
        PortPtr pPdoPort;
        ret =pPort->GetPdoPort( pPdoPort );
        if( ERROR( ret ) )
            break;

        HANDLE hPort = PortToHandle( pPdoPort );

        CCfgOpener oTransCtx;
        ret = oTransCtx.CopyProp(
            propConnHandle, pPort );
        if( ERROR( ret ) )
            break;

        ret = oTransCtx.CopyProp(
            propRouterPath, pPort );
        if( ERROR( ret ) )
            break;

        // broadcast this event the pnp manager
        // will stop all the ports
        CEventMapHelper< CPort >
            oEvtHelper( pPdoPort );

        IConfigDb* pTransCtx =
            oTransCtx.GetCfg();

        // we will pass the ip address and the
        // port handle to the subscribers
        ret = oEvtHelper.BroadcastEvent(
            eventConnPoint, eventId, 
            ( LONGWORD )pTransCtx,
            ( LONGWORD* )hPort );

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
                // all the event matches are offline now
                MatchMap& oMatchMap = m_mapEvtTable;
                for( auto oMe : oMatchMap )
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
    CParamList oTaskCfg(
        ( IConfigDb* )GetConfig() );

    string strModName;
    do{
        guint32 dwFlags = 0;
        ret = oTaskCfg.Pop( dwFlags );
        if( ERROR( ret ) )
            break;

        bool bOnline = 
            ( ( dwFlags & 0x01 ) > 0 );

        ret = oTaskCfg.GetStrProp(
            propModName, strModName );
        if( ERROR( ret ) )
            break;

        CPort* pPort = nullptr;
        ret = oTaskCfg.GetPointer(
            propPortPtr, pPort );
        if( ERROR( ret ) )
            break;

        PortPtr pPdoPort;
        ret = pPort->GetPdoPort( pPdoPort );
        if( ERROR( ret ) )
            break;

        CCfgOpener oTransCtx;
        ret = oTransCtx.CopyProp(
            propConnHandle, pPort );
        if( ERROR( ret ) )
            break;

        CConnParamsProxy oConn =
            GetConnParams( pPort );

        ret = oTransCtx.CopyProp(
            propRouterPath,
            ( IConfigDb* )oConn.GetCfg() );

        if( ERROR( ret ) )
            break;

        // broadcast this event
        // the pnp manager will stop all the
        // ports
        CEventMapHelper< CPort >
            oEvtHelper( pPdoPort );

        guint32 eventId = eventRmtModOnline;

        if( !bOnline )
            eventId = eventRmtModOffline;

        IConfigDb* pTransCtx =
            oTransCtx.GetCfg();

        ret = oEvtHelper.BroadcastEvent(
            eventConnPoint, eventId,
            ( LONGWORD )pTransCtx,
            ( LONGWORD* )strModName.c_str() );

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

        ret = oParams.Push( dwFlags );
        if( ERROR( ret ) )
            break;

        ret = oParams.SetStrProp(
            propModName, strModName );
        if( ERROR( ret ) )
            break;

        IPort* pPdoPort = GetBottomPort();
        ret = oParams.SetObjPtr(
            propPortPtr, ObjPtr( pPdoPort ) );

        if( ERROR( ret ) )
            break;

        ret = GetIoMgr()->ScheduleTask(
            clsid( CProxyFdoModOnOfflineTask ),
            oParams.GetCfg() );

    }while( 0 );

    return ret;
}

