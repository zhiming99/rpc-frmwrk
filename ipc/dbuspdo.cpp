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
#include "port.h"
#include "dbusport.h"

namespace rpcf
{

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

        if( strObjPath[ 0 ] == '.' )
            strObjPath = strObjPath.substr( 1 );
       
        CStdRMutex oPortLock( GetLock() );
        if( m_mapRegObjs.find( strObjPath )
            != m_mapRegObjs.end() )
        {
            // already registered
            m_mapRegObjs[ strObjPath ]++;
            break;
        }
        else
        {
            m_mapRegObjs[ strObjPath ] = 1;
        }
        oPortLock.Unlock();

        // NOTE: Need to verify if the call will
        // bypass the mainloop and callback we
        // registered, so that it can be safely
        // called, without extra steps from the
        // dbus message callback
        CDBusBusPort *pBusPort = static_cast
            < CDBusBusPort* >( m_pBusPort );

        ret = pBusPort->RegBusName(
            strObjPath, 0 );

        if( ERROR( ret ) ) 
        {
            // rollback
            oPortLock.Lock();
            if( m_mapRegObjs.find( strObjPath ) !=
                m_mapRegObjs.end() )
            {
                gint32 iCount =
                    --m_mapRegObjs[ strObjPath ];
                if( iCount <= 0 )
                    m_mapRegObjs.erase( strObjPath );
            }
        }

    }while( 0 );

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

        if( strObjPath[ 0 ] == '.' )
            strObjPath = strObjPath.substr( 1 );
       
        CStdRMutex oPortLock( GetLock() );
        bool bRelease = false;

        std::map< std::string, gint32 >::iterator
            itr = m_mapRegObjs.find( strObjPath );
        if( itr != m_mapRegObjs.end() )
        {
            gint32 iCount = --itr->second;
            if( iCount <= 0 )
            {
                m_mapRegObjs.erase( itr );
                bRelease = true;
            }
        }
        oPortLock.Unlock();
        if( bRelease )
        {
            CDBusBusPort *pBusPort = static_cast
                < CDBusBusPort* >( m_pBusPort );

            ret = pBusPort->
                ReleaseBusName( strObjPath );
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

        guint32* pdwValue;

        ret = oCfg.GetIntPtr(
            propDBusConn, ( guint32*& )pdwValue );

        if( ERROR( ret ) )
            break;

        m_pDBusConn = ( DBusConnection* )pdwValue;

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
        if( m_pDBusConn != nullptr )
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
}

gint32 CDBusLocalPdo::OnModOnOffline(
    DBusMessage* pDBusMsg )
{
    gint32 ret = 0;

    if( pDBusMsg == nullptr )
        return -EINVAL;

    vector< IrpPtr > vecIrpsToCancel;

    do{
        bool bOnline = false;
        char* pszModName = nullptr;
        CDBusError oError;
        DMsgPtr pMsg( pDBusMsg );

        if( pMsg.GetMember() == "NameOwnerChanged" )
        {
            // remote module online/offline
            char* pszOldOwner = nullptr;
            char* pszNewOwner = nullptr; 

            if( !dbus_message_get_args(
                pDBusMsg, oError,
                DBUS_TYPE_STRING, &pszModName,
                DBUS_TYPE_STRING, &pszOldOwner,
                DBUS_TYPE_STRING, &pszNewOwner,
                DBUS_TYPE_INVALID ) )
            {
                ret = oError.Errno();
                break;
            }

            if( pszNewOwner[ 0 ] != 0 )
            {
                // the name has been assigned to
                // some process
                bOnline = true;
            }
        }
        /*else if( pMsg.GetMember() == "NameLost" )
        {
            // local module offline
            if( !dbus_message_get_args(
                pDBusMsg, oError,
                DBUS_TYPE_STRING, &pszModName,
                DBUS_TYPE_INVALID ) )
            {
                ret = oError.Errno();
                break;
            }
        }
        else if( pMsg.GetMember() == "NameAcquired" )
        {
            // local module online
            if( !dbus_message_get_args(
                pDBusMsg, oError,
                DBUS_TYPE_STRING, &pszModName,
                DBUS_TYPE_INVALID ) )
            {
                ret = oError.Errno();
                break;
            }
            bOnline = true;
        }*/
        else
        {
            break;
        }

        ret = super::OnModOnOffline( pMsg,
            bOnline, std::string( pszModName ) );

    }while( 0 );

    return ret;
}

DBusHandlerResult CDBusLocalPdo::DispatchDBusSysMsg(
    DBusMessage* pMessage )
{
    DBusHandlerResult ret =
        DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    if( pMessage == nullptr )
        return ret;

    DMsgPtr pMsg( pMessage );
    string strMember = pMsg.GetMember();

    if( strMember.empty() )
        return ret;

    if( strMember == "NameOwnerChanged" ||
        strMember == "NameLost" ||
        strMember == "NameAcquired" )
    {
        if( SUCCEEDED( OnModOnOffline( pMsg ) ) )
        {
            ret = DBUS_HANDLER_RESULT_HANDLED;
        }
    }

    return ret;
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
    else if( ret1 == -EBADMSG )
    {
        // this is a dbuserror message to notify the
        // client dbus_bus_remove_match failed.
        return DBUS_HANDLER_RESULT_HANDLED;
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
            if( DispatchReqMsg( pMsg ) != -ENOENT )
                ret = DBUS_HANDLER_RESULT_HANDLED; 
            break;
        }
    case DBUS_MESSAGE_TYPE_SIGNAL:
    case DBUS_MESSAGE_TYPE_METHOD_RETURN:
    case DBUS_MESSAGE_TYPE_ERROR:
    default:
        {
            // DebugPrint( 0, "probe: Recv dbus msg" );
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

    ClearDBusSetting( m_matchDBus );
    m_matchDBus.Clear();

    if( m_pDBusConn )
    {
        dbus_connection_unref( m_pDBusConn );
        m_pDBusConn = nullptr;
    }

    return ret;
}

gint32 CDBusLocalPdo::PostStart(
    IRP* pIrp )
{
    gint32 ret = 0;

    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    ret = m_matchDBus.NewObj(
        clsid( CDBusSysMatch ) );

    if( ERROR( ret ) )
        return ret;

    ret = SetupDBusSetting( m_matchDBus );
    if( ERROR( ret ) )
        return ret;

    ret = super::PostStart( pIrp );

    return ret;
}

CDBusLoopbackPdo::CDBusLoopbackPdo (
    const IConfigDb* pCfg )
    : super( pCfg )
{
    SetClassId( clsid( CDBusLoopbackPdo ) );

    gint32 ret = m_matchDBus.NewObj(
        clsid( CDBusSysMatch ) );
    if( ERROR( ret ) )
    {
        string strMsg = DebugMsg( ret,
            "Error in CDBusLoopbackPdo initialization" );
        throw std::runtime_error( strMsg );
    }
}

CDBusLoopbackPdo::~CDBusLoopbackPdo()
{
    m_matchDBus.Clear();
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

        DMsgPtr pMsgPtr( pMsg );

        pMsgPtr.SetSender(
            LOOPBACK_DESTINATION );

        ret = pBus->SendLpbkMsg( pMsg,
            pdwSerial );
    }
    else
    {
        ret = -EINVAL;
    }

    return ret;
}

gint32 CDBusLoopbackPdo::SetupDBusSetting(
        IMessageMatch* pMsgMatch )
{
    if( pMsgMatch == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    CDBusError dbusError;
    do{
        EnumMatchType iMatchType =
            pMsgMatch->GetType();

        // test if the local server is ready
        if( iMatchType != matchClient )
            break;

        string strDest =
            pMsgMatch->GetDest();

        if( strDest.empty() )
        {
            ret = -EINVAL;
            break;
        }

        CDBusBusPort *pBusPort = static_cast
            < CDBusBusPort* >( m_pBusPort );

        // ret = pBusPort->IsDBusSvrOnline( strDest );
        ret = pBusPort->IsRegBusName( strDest );
        if( ret == ERROR_FALSE )
            ret = ENOTCONN;
        // NOTE: we don't have AddRules here, and all
        // the signals from this server will be looped
        // back

    }while( 0 );

    return ret;
}

DBusHandlerResult CDBusLoopbackPdo::PreDispatchMsg(
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

gint32 CDBusLoopbackPdo::SubmitIoctlCmd(
    IRP* pIrp )
{
    if( pIrp == nullptr
        || pIrp->GetStackSize() == 0 )
    {
        return -EINVAL;
    }

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
        case CTRLCODE_SEND_RESP:
        case CTRLCODE_SEND_EVENT:
            {
                // server side I/O
                ret = super::HandleSendReq( pIrp );
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

#include "ifhelper.h"
gint32 CDBusLoopbackPdo2::ClearDBusSetting(
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
            ret = super::ClearDBusSetting(
                pMsgMatch );
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

        if( strObjPath[ 0 ] == '.' )
            strObjPath = strObjPath.substr( 1 );
       
        CStdRMutex oPortLock( GetLock() );
        bool bRelease = false;

        std::map< std::string, gint32 >::iterator
            itr = m_mapRegObjs.find( strObjPath );
        if( itr != m_mapRegObjs.end() )
        {
            gint32 iCount = --itr->second;
            if( iCount <= 0 )
            {
                m_mapRegObjs.erase( itr );
                bRelease = true;
            }
        }
        oPortLock.Unlock();
        if( bRelease )
        {
            CDBusBusPort *pBusPort = static_cast
                < CDBusBusPort* >( m_pBusPort );

            ret = pBusPort->
                ReleaseBusName( strObjPath );
        }

    }while( 0 );

    if( SUCCEEDED( ret ) )
        return ret;

    return ret;
}

gint32 CDBusLoopbackPdo2::SetupDBusSetting(
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

        if( strObjPath[ 0 ] == '.' )
            strObjPath = strObjPath.substr( 1 );
       
        CStdRMutex oPortLock( GetLock() );
        if( m_mapRegObjs.find( strObjPath )
            != m_mapRegObjs.end() )
        {
            // already registered
            m_mapRegObjs[ strObjPath ]++;
            break;
        }
        else
        {
            m_mapRegObjs[ strObjPath ] = 1;
        }
        oPortLock.Unlock();

        // NOTE: Need to verify if the call will
        // bypass the mainloop and callback we
        // registered, so that it can be safely
        // called, without extra steps from the
        // dbus message callback
        CDBusBusPort *pBusPort = static_cast
            < CDBusBusPort* >( m_pBusPort );

        ret = pBusPort->RegBusName(
            strObjPath, 0 );

        if( ERROR( ret ) ) 
        {
            // rollback
            oPortLock.Lock();
            if( m_mapRegObjs.find( strObjPath ) !=
                m_mapRegObjs.end() )
            {
                gint32 iCount =
                    --m_mapRegObjs[ strObjPath ];
                if( iCount <= 0 )
                    m_mapRegObjs.erase( strObjPath );
            }
        }

    }while( 0 );

    // the user interface will register itself
    // with the registry
    return ret;
}

gint32 CDBusLoopbackPdo2::SendDBusMsg(
    DBusMessage* pMsg,
    guint32* pdwSerial )
{
    if( pMsg == nullptr )
        return -EINVAL;

    gint32 ret = 0;

    do{
        if( m_pBusPort->GetClsid()
            != clsid( CDBusBusPort ) )
        {
            ret = -EINVAL;
            break;
        }

        auto pBus = static_cast< CDBusBusPort* >
            ( m_pBusPort );

        DMsgPtr pMsgPtr( pMsg );
        pMsgPtr.SetSender(
            LOOPBACK_DESTINATION );

        gint32 dwSerial =
            pBus->LabelMessage( pMsgPtr );

        gint32 ( *func )( CDBusBusPort*,
            DMsgPtr& ) =
        ([]( CDBusBusPort* pBus,
            DMsgPtr& pMsg )->gint32
        {
            pBus->OnLpbkMsgArrival( pMsg );
            return 0;
        });

        TaskletPtr pTask;
        CIoManager* pMgr = GetIoMgr();
        ret = NEW_FUNCCALL_TASK(
            pTask, pMgr, func, pBus, pMsgPtr );

        if( ERROR( ret ) )
            break;

        if( pdwSerial != nullptr )
            *pdwSerial = dwSerial;
        // FIXME: it is dangerous to call this method
        // because the life-time of pMsg is not
        // contained, and could cause dbus to abort if
        // dbus_shutdown is already called. The
        // RescheduleTaskMainLoop is a safe approach,
        // but has bad performance.
        ret = pMgr->RescheduleTask( pTask );

    }while( 0 );

    return ret;
}

}
