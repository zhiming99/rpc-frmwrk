/*
 * =====================================================================================
 *
 *       Filename:  dbuspdo.cpp
 *
 *    Description:  implementation of dbus local pdo
 *
 *        Version:  1.0
 *        Created:  10/01/2016 01:32:16 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 * =====================================================================================
 */

#include <dbus/dbus.h>
#include <glib.h>
#include <vector>
#include <string>
#include <regex>
#include "defines.h"
#include "port.h"
#include "dbusport.h"


using namespace std;

/**
* @name CDBusLocalPdo::SetupDBusSetting
* @{ */
/** setup the match or name on dbus. 
 * this method support both call and signal
 * settings
 * @} */

gint32 CDBusLocalPdo::SetupDBusSetting(
    IMessageMatch* pMsgMatch )
{
    if( pMsgMatch == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    CDBusError dbusError;
    EnumMatchType iType = pMsgMatch->GetType();

    do{
        if( iType == matchClient )
        {
            ret = super::SetupDBusSetting( pMsgMatch );
            break;
        }

        // server will register the objPath
        // on the dbus
        if( iType != matchServer )
        {
            ret = -EINVAL;
            break;
        }
        string strObjPath;
        CCfgOpenerObj oProps( pMsgMatch );
        ret = oProps.GetStrProp(
            propObjPath, strObjPath );

        if( ERROR( ret ) )
            break;

        if( strObjPath.empty() )
        {
            ret = -EINVAL;
            break;
        }

        std::replace( strObjPath.begin(),
            strObjPath.end(), '/', '.');
       
        CStdRMutex oPortLock( GetLock() );
        if( m_mapRegObjs.find( strObjPath )
            != m_mapRegObjs.end() )
        {
            // already registered
            m_mapRegObjs[ strObjPath ]++;
            break;
        }

        oPortLock.Unlock();

        // NOTE: Need to verify if the call will
        // bypass the mainloop and callback we
        // registered, so that it can be safely
        // called, without extra steps from the
        // dbus message callback
        ret = dbus_bus_request_name(
            m_pDBusConn,
            strObjPath.c_str(),
            0, dbusError );

        oPortLock.Lock();

        if( ret == -1 ) 
        {
            // register this object
            if( dbusError.GetName() != nullptr )
            {
                ret = dbusError.Errno();
                break;
            }
            else
            {
                ret = ERROR_FAIL;
                break;
            }
        }

        // otherwise, return TRUE or FALSE
        if( ret == 0 )
        {
            ret = ENOTCONN;
        }
        else if( ret == 1 )
        {
            // register this object in the memory
            if( m_mapRegObjs.find( strObjPath )
                == m_mapRegObjs.end() )
                m_mapRegObjs[ strObjPath ] = 1;
            else
                m_mapRegObjs[ strObjPath ]++;

            ret = 0;
        }
        else
        {
            ret = ERROR_FAIL;
        }
    }
    while( 0 );

    // the user interface will register itself
    // with the registry
    return ret;
}

/**
* @name ClearDBusSetting
* @{ */
/** Clear the footprint on dbus. 
 * this method clear either call or signal
 * setting
 * @} */
gint32 CDBusLocalPdo::ClearDBusSetting(
    IMessageMatch* pMsgMatch )
{
    if( pMsgMatch == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    CDBusError dbusError;
    EnumMatchType iType = pMsgMatch->GetType();

    do{
        if( iType == matchClient )
        {
            ret = super::ClearDBusSetting( pMsgMatch );
            break;
        }
        // server will register the objPath
        // on the dbus
        string strObjPath;
        CCfgOpenerObj oProps( pMsgMatch );
        ret = oProps.GetStrProp(
            propObjPath, strObjPath );

        if( ERROR( ret ) )
            break;

        if( strObjPath.empty() )
        {
            ret = -EINVAL;
            break;
        }

        std::replace( strObjPath.begin(),
            strObjPath.end(), '/', '.');
       
        CStdRMutex oPortLock( GetLock() );
        bool bRelease = false;
        if( m_mapRegObjs.find( strObjPath )
            != m_mapRegObjs.end() )
        {
            m_mapRegObjs[ strObjPath ]--;
            if( m_mapRegObjs[ strObjPath ] <= 0 )
            {
                m_mapRegObjs.erase( strObjPath );
                bRelease = true;
            }
            break;
        }

        if( bRelease )
        {
            dbus_bus_release_name(
                m_pDBusConn,
                strObjPath.c_str(),
                dbusError );
        }

    }while( 0 );

    if( SUCCEEDED( ret ) )
        return ret;

    return ret;
}

CDBusLocalPdo::CDBusLocalPdo( const IConfigDb* pCfg )
    : super( pCfg )
{
    gint32 ret = 0;

    SetClassId( clsid( CDBusLocalPdo ) );

    do{
        CCfgOpener oCfg( pCfg );

        guint32 dwValue;

        ret = oCfg.GetIntProp(
            propDBusConn, ( guint32& )dwValue );

        if( ERROR( ret ) )
            break;

        m_pDBusConn = ( DBusConnection* )dwValue;

        if( m_pDBusConn == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        // we did not add ref to the reference in 
        // the config block, so we explictly add one
        dbus_connection_ref( m_pDBusConn );

        // initialize the message match for the DBus
        // system messages, such as NameOwnerChanged

    }while( 0 );

    if( ERROR( ret ) )
    {
        dbus_connection_unref( m_pDBusConn );
        m_pDBusConn = nullptr;

        string strMsg =
            DebugMsg( ret, "Error in CDBusLocalPdo initialization" );

        throw std::runtime_error( strMsg );
    }

    return;
}

CDBusLocalPdo::~CDBusLocalPdo()
{
    if( m_pDBusConn )
    {
        dbus_connection_unref( m_pDBusConn );
        m_pDBusConn = nullptr;
    }
}

DBusHandlerResult CDBusLocalPdo::PreDispatchMsg(
    gint32 iType, DBusMessage* pMsg )
{

    DBusHandlerResult ret =
        DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    gint32 ret1 =
        m_matchDBus->IsMyMsgIncoming( pMsg );

    if( SUCCEEDED( ret1  ) )
    {
        // dbus system message
        ret = DispatchDBusSysMsg( pMsg );
    }

    if( ret == DBUS_HANDLER_RESULT_NOT_YET_HANDLED )
    {
        ret = super::PreDispatchMsg( iType, pMsg );
    }

    return ret;
}

DBusHandlerResult CRpcBasePortEx::DispatchMsg(
    gint32 iType, DBusMessage* pMsg )
{
    DBusHandlerResult ret =
        DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    if( pMsg == nullptr )
        return ret;

    switch( iType )
    {
    case DBUS_MESSAGE_TYPE_METHOD_CALL:
        {
            if( SUCCEEDED( DispatchReqMsg( pMsg ) ) )
                ret = DBUS_HANDLER_RESULT_HANDLED; 
            break;
        }
    case DBUS_MESSAGE_TYPE_SIGNAL:
    case DBUS_MESSAGE_TYPE_METHOD_RETURN:
    case DBUS_MESSAGE_TYPE_ERROR:
    default:
        {
            ret = super::DispatchMsg( iType, pMsg );
            break;
        }
    }

    return ret;
}

gint32 CRpcBasePortEx::GetMatchMap(
    IMessageMatch* pMatch, MatchMap*& pMap )
{
    gint32 ret = 0;

    if( pMatch == nullptr )
        return -EINVAL;

    gint32 iMatchType = pMatch->GetType();

    if( iMatchType == matchClient )
        pMap = &m_mapEvtTable;
    else
        pMap = &m_mapReqTable;

    return ret;
}


gint32 CDBusLocalPdo::HandleSendResp(
    IRP* pIrp )
{

    gint32 ret = -EINVAL;

    if( pIrp == nullptr
        || pIrp->GetStackSize() == 0 )
    {
        return ret;
    }

    do{

        // let's process the func irps
        IrpCtxPtr& pCtx = pIrp->GetCurCtx();
        guint32 dwIoDir = pCtx->GetIoDirection();

        if( dwIoDir != IRP_DIR_OUT )
        {
            ret = -EINVAL;
            break;
        }

        ret = HandleSendReq( pIrp );

    }while( 0 );

    return ret;
}

gint32 CDBusLocalPdo::SubmitIoctlCmd(
    IRP* pIrp )
{
    if( pIrp == nullptr
        || pIrp->GetStackSize() == 0 )
    {
        return -EINVAL;
    }

    gint32 ret = 0;

    // let's process the func irps
    IrpCtxPtr& pCtx = pIrp->GetCurCtx();

    do{

        if( pIrp->MajorCmd() != IRP_MJ_FUNC
            || pIrp->MinorCmd() != IRP_MN_IOCTL )
        {
            ret = -EINVAL;
            break;
        }

        switch( pIrp->CtrlCode() )
        {
        case CTRLCODE_SEND_RESP:
        case CTRLCODE_SEND_EVENT:
            {
                // server side I/O
                ret = HandleSendResp( pIrp );
                break;
            }
        default:
            {
                ret = super::SubmitIoctlCmd( pIrp );
                break;
            }
        }
    }while( 0 );

    if( ret != STATUS_PENDING )
        pCtx->SetStatus( ret );

    return ret;
}

gint32 CDBusLocalPdo::Stop( IRP *pIrp )
{
    if( pIrp == nullptr
        || pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = super::Stop( pIrp );
    m_matchDBus.Clear();

    return ret;
}

gint32 CDBusLocalPdo::PostStart(
    IRP* pIrp )
{
    gint32 ret = 0;

    if( pIrp == nullptr
        || pIrp->GetStackSize() == 0 )
        return -EINVAL;

    do{
        ret = m_matchDBus.NewObj(
            clsid( CDBusSysMatch ) );

        if( ERROR( ret ) )
            break;

    }while( 0 );

    ret = super::PostStart( pIrp );

    return ret;
}

CDBusLoopbackPdo::CDBusLoopbackPdo (
    const IConfigDb* pCfg )
    : super( pCfg )
{
}

gint32 CDBusLoopbackPdo::SendDBusMsg(
    DBusMessage* pMsg,
    guint32* pdwSerial )
{
    if( pMsg == nullptr )
        return -EINVAL;

    gint32 ret = 0;

    if( m_pBusPort->GetClsid()
        == clsid( CDBusBusPort ) )
    {
        CDBusBusPort* pBus = static_cast
            < CDBusBusPort* >( m_pBusPort );

        ret = pBus->SendLpbkMsg( pMsg,
            pdwSerial );
    }
    else
    {
        ret = -EINVAL;
    }

    return ret;
}

