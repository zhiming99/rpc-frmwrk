/*
 * =====================================================================================
 *
 *       Filename:  dbusport.cpp
 *
 *    Description:  implementation of dbus port classes
 *
 *        Version:  1.0
 *        Created:  07/09/2016 11:22:40 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 * =====================================================================================
 */


#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <glib.h>
#include <vector>
#include <string>
#include <regex>
#include "defines.h"
#include "port.h"
#include "dbusport.h"
#include "stlcont.h"
//#include "connhelp.h"
#include "emaphelp.h"


using namespace std;


gint32 Ip4AddrToBytes(
    const string& strIpAddr,
    guint8* pBytes,
    guint32& dwSize )
{
    gint32 ret = 0;

    do{
        if( pBytes == nullptr ||
            dwSize < IPV6_ADDR_BYTES ||  // to accomodate the size of ipv6
            strIpAddr.empty() )
        {
            ret = -EINVAL;
            break;
        }
        // syntax check
        if( !std::regex_match( strIpAddr,
            std::regex( "^(?:[0-9]{1,3}\\.){3}[0-9]{1,3}$" ) ) )
        {
            // bad format ip address
            ret = -EINVAL;
            break;
        }

        memset( pBytes, 0, dwSize );
        std::size_t iPos = strIpAddr.find( '.' );
        std::size_t iBegin = 0;
        gint32 iIdx = 0;

        while( iPos != string::npos )
        {
            if( iBegin == iPos )
            {
                ret = -EINVAL;
                break;
            }
            string strComp = strIpAddr.substr( iBegin, iPos );
            gint32 iRet =( gint32 )atoi( strComp.c_str() );
            // let's do a semantic check
            if( iRet > 255 )
            {
                ret = -EINVAL;
                break;
            }
            pBytes[ iIdx++ ] = ( guint8 )iRet;

            iBegin = iPos + 1;
            if( iBegin >= strIpAddr.size() )
            {
                ret = -EINVAL;
            }
            iPos = strIpAddr.find( '.', iBegin );
        }

        if( ret < 0 )
            break;

        pBytes[ iIdx++ ] =
            atoi( strIpAddr.substr( iBegin ).c_str() );

        // not a valid ip addr if iIdx != 4
        if( iIdx != IPV4_ADDR_BYTES &&
            iIdx != IPV6_ADDR_BYTES )
            ret = -EINVAL;

        dwSize = iIdx;

    }while( 0 );

    return ret;
}

/**
* @name ByteToHexChar
* @{ */
/** 
 * pRet must be a ptr to a byte array * of 2 bytes or longer @}
 * byte contains the binary byte to convert
 * the return value is stored in two chars, that is,
 * the higher 4 bits are converted to a hex digit in pRet[ 0 ],
 * and the lower 4 bits are converted to a hex digit in
 * pRet[ 1 ] */

static void BinToHexChar( guint8 byte, guint8* pRet )
{
    if( pRet == nullptr )
        return;

    if( ( byte & 0x0F ) < 0x0A )
        pRet[ 1 ] = byte + 0x30;
    else
        pRet[ 1 ] = byte + 0x40;
    
    if( ( byte & 0xF0 ) < 0xA0 )
        pRet[ 0 ] = ( byte >> 4 ) + 0x30;
    else
        pRet[ 0 ] = ( byte >> 4 ) + 0x40;

    return;
}


static gint32 BytesToString(
    const guint8* bytes,
    guint32 dwSize,
    string& strRet )
{
    if( bytes == nullptr
        || dwSize == 0
        || dwSize > 1024 )
    {
        return -EINVAL;
    }

    guint8* pszRet =
        ( guint8* )alloca( dwSize * 2 + 1 );

    pszRet[ dwSize * 2 ] = 0;

    for( guint32 i = 0; i < dwSize * 2; i += 2 )
    {
        BinToHexChar(
            bytes[ ( i >> 1 ) ],
            pszRet + i );
    }

    strRet = ( char* )pszRet;

    return 0;
}

gint32 Ip4AddrToByteStr(
    const string& strIpAddr,
    string& strRet )
{

    // MAX_IPADDR_SIZE is far enough for an ip addr
    // whether ipv4 or ipv6
    guint8 bytes[ MAX_IPADDR_SIZE ];
    guint32 dwSize = MAX_IPADDR_SIZE;
    gint32 ret = 0;

    do{
        ret = Ip4AddrToBytes(
            strIpAddr, bytes, dwSize );

        if( ret < 0 )
            break;

        ret = BytesToString(
            bytes, dwSize, strRet );

    }while( 0 );

    return ret;
}

CDBusBusPort::CDBusBusPort( const IConfigDb* pConfig )
    : super( pConfig ),
    m_pDBusConn( nullptr )
{
    SetClassId( clsid( CDBusBusPort ) );
}

gint32 CDBusBusPort::BuildPdoPortName(
    const IConfigDb* pCfg,
    string& strPortName ) const
{
    gint32 ret = 0;
    if( pCfg == nullptr )
        return -EINVAL;

    do{
        CCfgOpener oCfgOpener( pCfg );
        if( pCfg->exist( propPortClass ) )
        {
            // port name is composed in the following
            // format:
            // <PortClassName> + "_" + <IPADDR in hex>
            string strClass;
            ret = oCfgOpener.GetStrProp(
                propPortClass, strClass );

            if( ERROR( ret ) )
                break;

            if( strClass != PORT_CLASS_DBUS_PROXY_PDO
                && strClass != PORT_CLASS_LOCALDBUS_PDO
                && strClass != PORT_CLASS_LOOPBACK_PDO )
            {
                // We only support to port classes
                // RpcProxy and LocalDBusPdo
                ret = -EINVAL;
                break;
            }
            if( strClass == PORT_CLASS_DBUS_PROXY_PDO
                && pCfg->exist( propIpAddr ) )
            {
                // ip addr must exist
                string strIpAddr, strRet;

                ret = oCfgOpener.GetStrProp(
                    propIpAddr, strIpAddr );

                if( ERROR( ret ) )
                    break;

                ret = Ip4AddrToByteStr(
                    strIpAddr, strRet );

                if( ERROR( ret ) )
                    break;

                strPortName = strClass + "_" + strRet;
            }
            else if( strClass == PORT_CLASS_LOCALDBUS_PDO
                || strClass == PORT_CLASS_LOOPBACK_PDO )
            {
                if( pCfg->exist( propPortId ) )
                {
                    guint32 dwPortId = 0;
                    string strRet;
                    ret = oCfgOpener.GetIntProp(
                        propPortId, dwPortId );

                    if( ERROR( ret ) )
                        break;

                    strPortName = strClass + string( "_" )
                        + std::to_string( dwPortId );
                }
                else
                {
                    strPortName = strClass + string( "_0" ); 
                }
            }
            else
            {
                ret = -ENOTSUP;
            }
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

gint32 CDBusBusPort::CreatePdoPort(
    IConfigDb* pCfg,
    PortPtr& pNewPort )
{
    gint32 ret = 0;

    // please be aware we are within a
    // port lock
    do{
        if( pCfg == nullptr )
            return -EINVAL;

        if( !pCfg->exist( propPortClass ) )
        {
            ret = -EINVAL;
            break;
        }

        string strPortClass;

        CCfgOpener oExtCfg( ( IConfigDb* )pCfg );

        ret = oExtCfg.GetStrProp(
            propPortClass, strPortClass );

        if( ERROR( ret ) )
            break;

        if( strPortClass
            == PORT_CLASS_DBUS_PROXY_PDO )
        {
            ret = CreateRpcProxyPdo(
                pCfg, pNewPort );
        }
        else if( strPortClass
            == PORT_CLASS_LOCALDBUS_PDO )
        {
            ret = CreateLocalDBusPdo(
                pCfg, pNewPort );
        }
        else if( strPortClass
            == PORT_CLASS_LOOPBACK_PDO )
        {
            ret = CreateLoopbackPdo(
                pCfg, pNewPort );
        }
        else
        {
            ret = -ENOTSUP;
            break;
        }

    }while( 0 );

    return ret;
}

gint32 CDBusBusPort::CreateRpcProxyPdo(
    const IConfigDb* pCfg,
    PortPtr& pNewPort )
{
    if( pCfg == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        string strPortName;
        guint32 dwPortId = 0;

        CCfgOpener oExtCfg( ( IConfigDb* )pCfg );

        ret = oExtCfg.GetIntProp(
            propPortId, dwPortId );

        if( ERROR( ret ) )
        {
            string strIpAddr;
            ret = oExtCfg.GetStrProp(
                propIpAddr, strIpAddr );

            if( ERROR( ret ) )
                break;

            guint8 arrBuf[ sizeof( guint32 ) * 2 ] = { 0 };
            guint8* pBuf = arrBuf;
            guint32 dwSize = sizeof( arrBuf );

            ret = Ip4AddrToBytes(
                strIpAddr, arrBuf, dwSize );

            if( ERROR( ret ) )
                break;

            dwPortId = *( guint32* )( pBuf );
        }

        // verify if the port already exists
        if( this->PortExist( dwPortId ) )
        {
            ret = -EEXIST;
            break;
        }

        ret = pNewPort.NewObj(
            clsid( CDBusProxyPdo ), pCfg );

        // the pdo port `Start()' will be deferred
        // till the complete port stack is built.
        //
        // And well, we are done here

    }while( 0 );

    return ret;
}

gint32 CDBusBusPort::CreateLocalDBusPdo(
    const IConfigDb* pCfg,
    PortPtr& pNewPort )
{
    if( pCfg == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        string strPortName;
        guint32 dwPortId = 0;

        CCfgOpener oExtCfg( ( IConfigDb* )pCfg );

        if( this->PortExist( dwPortId ) )
        {
            ret = -EEXIST;
            break;
        }

        // must-have properties in creation
        dwPortId = NewPdoId();
        if( dwPortId > 0 )
        {
            // we support just one
            ret = -ERANGE;
            break;
        }
        oExtCfg.SetIntProp( propPortId, dwPortId );

        PortPtr portPtr;
        portPtr.NewObj(
            clsid( CDBusLocalPdo ), pCfg );

        pNewPort = ( IPort* )portPtr;
        m_iLocalPortId = dwPortId;
        // the pdo port `Start()' will be deferred
        // till the complete port stack is built.
        //
        // And well, we are done here

    }while( 0 );

    return ret;
}

gint32 CDBusBusPort::CreateLoopbackPdo(
    const IConfigDb* pCfg,
    PortPtr& pNewPort )
{
    if( pCfg == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        string strPortName;
        guint32 dwPortId = 0;

        CCfgOpener oExtCfg( ( IConfigDb* )pCfg );

        if( this->PortExist( dwPortId ) )
        {
            ret = -EEXIST;
            break;
        }

        // must-have properties in creation
        dwPortId = NewPdoId();
        oExtCfg.SetIntProp( propPortId, dwPortId );

        PortPtr portPtr;
        portPtr.NewObj(
            clsid( CDBusLoopbackPdo ), pCfg );

        pNewPort = ( IPort* )portPtr;
        m_iLpbkPortId = dwPortId;
        // the pdo port `Start()' will be deferred
        // till the complete port stack is built.
        //
        // And well, we are done here

    }while( 0 );

    return ret;
}

gint32 CDBusBusPort::Start( IRP *pIrp )
{

    gint32 ret = 0;
    if( GetLowerPort() )
    {
        // nothing to do here if it is
        // sitting on some other port
        pIrp->GetCurCtx()->SetStatus( 0 );
        return 0;
    }

    // let's create the DBus connection
    CDBusError error;

    do{
        string strModName;
        string strPortName;

        strModName = GetIoMgr()->GetModName();

        CCfgOpener a( ( IConfigDb* )m_pCfgDb );
        ret = a.GetStrProp(
            propPortName, strPortName );

        if( ERROR( ret ) )
            break;

        // NOTE: c++11 needed for to_string
        string strBusName = string( DBUS_NAME_PREFIX )
            + strModName
            + string( "." )
            + strPortName;

        m_pDBusConn = dbus_bus_get_private(
            DBUS_BUS_SYSTEM, error );

        if ( nullptr == m_pDBusConn )
        {
            // fatal error, we cannot run without
            // the dbus-daemon.
            ret = error.Errno();
            break;
        }
        
        // don't exit on disconnection, we need to
        // do some clean-up work
        dbus_connection_set_exit_on_disconnect(
            m_pDBusConn, FALSE );

        error.Reset();

        // request the name for this connection
        ret = dbus_bus_request_name(
            m_pDBusConn,
            strBusName.c_str(),
            ( DBUS_NAME_FLAG_REPLACE_EXISTING
            | DBUS_NAME_FLAG_DO_NOT_QUEUE ),
            error );

        if( ERROR( ret ) )
        {
            ret = error.Errno();
            break;
        }
        
        // add the default entry for this connection
        if( FALSE == dbus_connection_add_filter( 
            m_pDBusConn, DBusMessageCallback, this, NULL ) )
        {
            break;
        }

        GMainContext* pCtx =
            GetIoMgr()->GetMainIoLoop().GetMainCtx();

        dbus_connection_setup_with_g_main(
            m_pDBusConn, pCtx );

    }while( 0 );


    if( ERROR( ret ) )
    {
        if( m_pDBusConn )
        {
            dbus_connection_unref( m_pDBusConn );
            m_pDBusConn = nullptr;
        }
    }

    if( pIrp != nullptr )
    {
        pIrp->GetCurCtx()->SetStatus( ret );
    }
    return ret;
}

DBusHandlerResult
CDBusBusPort::DBusMessageCallback(
     DBusConnection *pConn,
     DBusMessage *pMsg,
     void *pData)
{
    if( pConn == nullptr
        || pData == nullptr 
        || pMsg == nullptr )
    {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    CDBusBusPort *pBus =
        reinterpret_cast< CDBusBusPort* >( pData );

    return pBus->OnMessageArrival( pMsg );
}

gint32 CDBusBusPortDBusOfflineTask::operator()(
    guint32 dwContext )
{
    gint32 ret = 0;
    CCfgOpener a( ( IConfigDb* )m_pCtx );
    CIoManager* pMgr = nullptr;
    ObjPtr pObj;

    do{
        ret = a.GetPointer( propIoMgr, pMgr );
        if( ERROR( ret ) )
            break;

        ret = a.GetObjPtr( propPortPtr, pObj );
        if( ERROR( ret ) )
            break;

        // broadcast this event
        // the pnp manager will stop all the
        // ports
        CDBusBusPort* pPort = pObj;
        if( pPort == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        CEventMapHelper< CPort > a( pPort );
        ret = a.BroadcastEvent(
            eventConnPoint,
            eventDBusOffline, 0,
            nullptr );

    }while( 0 );

    return ret;
}

gint32 CDBusBusPort::HandleDBusDisconn()
{
    CParamList a;
    gint32 ret = 0;
    ret = a.SetPointer( propIoMgr, GetIoMgr() );
    if( ERROR( ret ) )
        return ret;

    ObjPtr pObj( this );
    ret = a.SetObjPtr( propPortPtr, pObj );
    if( ERROR( ret ) )
        return ret;

    // FIXME: do we need to broadcast this message to
    // the child PDO's ?
    ret = GetIoMgr()->ScheduleTask(
        clsid( CDBusBusPortDBusOfflineTask ),
        a.GetCfg() );

    return ret;
}
gint32 CDBusBusPort::ReplyWithError(
    DBusMessage* pMessage )
{
    if( pMessage == nullptr )
        return -EINVAL;

    DMsgPtr pMsg( pMessage ); 
    string strPath = pMsg.GetPath();
    if( strPath.empty()  )
        return -EINVAL;

    string strModName =
        GetIoMgr()->GetModName();

    string strDest =
        DBUS_DESTINATION( strModName );

    string strError;
    string strDesc;
    if( strPath.substr( 0, strDest.size() ) == strDest )
    {
        strError = DBUS_ERROR_NO_SERVER;
        strDesc = "Please retry later";
    }
    else
    {
        strError = DBUS_ERROR_NO_REPLY;
        strDesc = "Please check the parameters";
    }

    DBusMessage* pReply = dbus_message_new_error(
        pMessage, strError.c_str(), strDesc.c_str() );

    return SendDBusMsg( pReply, nullptr );
}

DBusHandlerResult CDBusBusPort::OnMessageArrival(
    DBusMessage* pMessage )
{
    DBusHandlerResult ret =
        DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    do{
        if( pMessage == nullptr )
            break;

        DMsgPtr pMsg( pMessage );

        BufPtr pBuf( true );
        gint32 iType = pMsg.GetType();

        if( iType == DBUS_MESSAGE_TYPE_SIGNAL) 
        {
            // handle the disconnection message
            gint32 ret1 = 
                m_pMatchDisconn->IsMyMsgIncoming( pMsg );

            if( SUCCEEDED( ret1 ) )
            {
                if( ERROR( HandleDBusDisconn() ) )
                    break;

                // don't go forward
                ret = DBUS_HANDLER_RESULT_HANDLED;
                break;
            }
        }

        ( *pBuf ) = pMsg;

        vector< PortPtr > vecPorts;

        if( !pBuf.IsEmpty() )
        {
            CStdRMutex oPortLock( GetLock() );
            guint32 dwPortState = GetPortState();

            if( !( dwPortState == PORT_STATE_READY
                 || dwPortState == PORT_STATE_BUSY_SHARED
                 || dwPortState == PORT_STATE_BUSY
                 || dwPortState == PORT_STATE_BUSY_RESUME ) )
            {
                ret = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
                break;
            }

            map<guint32, PortPtr>::iterator
                itr = m_mapId2Pdo.begin();

            while( itr != m_mapId2Pdo.end() )
            {
                guint32 dwPortId = itr->first;
                if( dwPortId != ( guint32 )m_iLocalPortId
                    && dwPortId != ( guint32 )m_iLpbkPortId )
                    vecPorts.push_back( itr->second );
                ++itr;
            }

            // place the localdbus port at the
            // last. give some certainty in the
            // order of the message processing
            vecPorts.push_back(
                m_mapId2Pdo[ m_iLocalPortId ] );
        }

        if( vecPorts.empty() )
        {
            ret = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
            break;
        }

        gint32 ret1 = 0;

        for( guint32 i = 0; i < vecPorts.size(); ++i )
        {
            PortPtr pPort = vecPorts[ i ];
            if( pPort.IsEmpty() )
                continue;

            IRpcPdoPort* pPdoPort =
                dynamic_cast< IRpcPdoPort* >( ( IPort* )pPort );

            if( pPdoPort == nullptr )
                continue;

            guint32* pData = ( guint32* )( ( CBuffer* )pBuf );
            ret1 = pPdoPort->OnEvent(
                cmdDispatchData, 0, 0, pData );

            switch( iType )
            {
            case DBUS_MESSAGE_TYPE_METHOD_CALL:
                {
                    if( SUCCEEDED( ret1 ) || ret1 != -ENOENT )
                        ret = DBUS_HANDLER_RESULT_HANDLED;
                    break;
                }
            case DBUS_MESSAGE_TYPE_SIGNAL:
                {
                    // iterate through all the pdo ports
                    ret = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
                    if( ret1 == ERROR_PREMATURE )
                    {
                        ret = DBUS_HANDLER_RESULT_HANDLED;
                    }
                    break;
                }
            case DBUS_MESSAGE_TYPE_ERROR:
            case DBUS_MESSAGE_TYPE_METHOD_RETURN:
                {
                    if( SUCCEEDED( ret1 ) || ret1 != -ENOENT)
                        ret = DBUS_HANDLER_RESULT_HANDLED;
                    break;
                }
            default:
                {
                    ret = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
                    break;
                }
            }

            if( ret == DBUS_HANDLER_RESULT_HANDLED )
                break;
        }

        if( iType == DBUS_MESSAGE_TYPE_METHOD_CALL
            && ret == DBUS_HANDLER_RESULT_NOT_YET_HANDLED )
        {
            // reply with dbus error
            ReplyWithError( pMessage );
        }

    }while( 0 );

    return ret;
}

gint32 CDBusBusPort::SendDBusMsg(
    DBusMessage *pDBusMsg, guint32* pdwSerial )
{
    gint32 ret = 0;

    if( pDBusMsg == nullptr )
        return -EINVAL;

    ret = FilterLpbkMsg( pDBusMsg, pdwSerial );
    if( ret != ERROR_NOT_HANDLED )
        return ret;

    ret = 0;
    if( !dbus_connection_send(
        m_pDBusConn, pDBusMsg, pdwSerial ) )
        ret = -ENOMEM;
    return ret;
}

gint32 CDBusBusPort::Stop( IRP *pIrp )
{
    gint32 ret = super::Stop( pIrp );
    // TODO: stop all the pdo's created by this port
    if( m_pDBusConn )
    {
        dbus_connection_unref( m_pDBusConn );
        m_pDBusConn = nullptr;
    }

    if( !m_pMatchDisconn.IsEmpty() )
    {
        m_pMatchDisconn.Clear();
    }
    return ret;
}

gint32 CDBusBusPort::GetChildPorts(
        vector< PortPtr >& vecChildPorts )
{
    PortPtr pPort;
    GetPdoPort( m_iLocalPortId, pPort );

    // add the local dbus port
    vecChildPorts.push_back( pPort );
    gint32 ret = 0;

    for( guint32 i = 0; i < m_vecProxyPortIds.size(); i++ )
    {
        gint32 pid = m_vecProxyPortIds[ i ];
        ret = GetPdoPort( pid, pPort );
        if( ERROR( ret ) )
            continue;

        vecChildPorts.push_back( pPort );
    }
    return ret;
}

gint32 CDBusBusPort::OnPortReady( IRP* pIrp )
{
    if( pIrp == nullptr )
        return -EINVAL;

    return super::OnPortReady( pIrp );
}

gint32 CDBusBusPortCreatePdoTask::operator()(
    guint32 dwContext )
{

    // no ref increment, to match the
    // addref before schedule this event
    CCfgOpener oCfg( ( IConfigDb* )m_pCtx );

    PortPtr pNewPort;
    ObjPtr pObj;

    gint32 ret =
        oCfg.GetObjPtr( propBusPortPtr, pObj );

    if( ERROR( ret ) )
        return ret;

    CGenericBusPort* pPort = pObj;
    if( pPort )
    {
        ret = pPort->OpenPdoPort(
            oCfg.GetCfg(), pNewPort );
    }
    else
    {
        ret = -EINVAL;
    }

    return SetError( ret );
}

gint32 CDBusBusPort::PostStart( IRP* pIrp )
{
    gint32 ret = 0;
    do{
        if( pIrp == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        //
        // an attempt failed to defer the irp
        // complete after all the children ports
        // are created, simply because there is a
        // moment when children are started, this
        // port is still in the starting state
        //
        // but if we complete the irp after this
        // method returns, the pdo is yet to
        // create and if there is a `stop port'
        // coming, the situation will become
        // complicated
        //

        // create the local dbus pdo here
        //
        // NOTE: do we need to add the registry?
        // Let's schedule a tasklet to create the
        // pdo
        CCfgOpener newCfg;

        // param port class
        ret = newCfg.SetStrProp(
            propPortClass, PORT_CLASS_LOCALDBUS_PDO );

        if( ERROR( ret ) )
            break;

        // param bus name
        ret = newCfg.CopyProp( propBusName, m_pCfgDb );
        if( ERROR( ret ) )
            break;

        // param io manager
        ret = newCfg.SetPointer( propIoMgr, GetIoMgr() );

        if( ERROR( ret ) )
            break;

        // param dbus connection
        ret = newCfg.SetIntProp(
            propDBusConn, ( guint32 )m_pDBusConn );

        if( ERROR( ret ) )
            break;

        // param parent port
        ret = newCfg.SetObjPtr(
            propBusPortPtr, ObjPtr( this ) );

        if( ERROR( ret ) )
            break;

        // no driver ptr because this is a 
        // pdo

        TaskletPtr pTask;
        pTask.NewObj(
            clsid( CDBusBusPortCreatePdoTask ),
            newCfg.GetCfg() );

        // although a synchrous call of this task
        // the PORT_START process will begin after
        // this call returns.
        ( *pTask )( eventZero );
        ret = pTask->GetError();
        if(  ERROR( ret ) )
            break;

        ret = m_pMatchDisconn.NewObj(
            clsid( CDBusDisconnMatch ),
            nullptr );

        if( ERROR( ret ) )
            break;

        // create the loopback pdo
        ret = newCfg.SetStrProp(
            propPortClass, PORT_CLASS_LOOPBACK_PDO ); 

        if( ERROR( ret ) )
            break;

        ( *pTask )( eventZero );
        ret = pTask->GetError();

    }while( 0 );

    return ret;
}

gint32 CDBusBusPort::OnEvent( EnumEventId iEvent,
        guint32 dwParam1,
        guint32 dwParam2,
        guint32* pData )
{
    return super::OnEvent(
        iEvent, dwParam1, dwParam2, pData );
}

//-------------------------------------
// Loopback related methods
//-------------------------------------
gint32 CDBusBusPort::InitLpbkMatch()
{
    gint32 ret = 0;

    do{
        CIoManager* pMgr = GetIoMgr();
        CParamList oParams;
        oParams.SetPointer( propIoMgr, pMgr );

        string strUniqName;
        const char* pszName =
            dbus_bus_get_unique_name( m_pDBusConn );
        
        if( pszName != nullptr )
            strUniqName = pszName;

        if( strUniqName.empty()  )
        {
            ret = -EFAULT;
            break;
        }

        ret = oParams.SetStrProp(
            propDestUniqName, strUniqName );

        if( ERROR( ret ) )
            break;

        ret = oParams.SetIntProp(
            propMatchType, matchClient );

        ret = oParams.SetStrProp(
            propSrcUniqName, strUniqName );

        if( ERROR( ret ) )
            break;

        ret = m_pMatchLpbkProxy.NewObj(
            clsid( CDBusLoopbackMatch ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        ret = oParams.SetIntProp(
            propMatchType, matchServer );

        ret = m_pMatchLpbkServer.NewObj(
            clsid( CDBusLoopbackMatch ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

    }while( 0 );

    return ret;
}

gint32 CDBusTransLpbkMsgTask::operator()(
    guint32 dwContext ) 
{
    gint32 ret = 0;
    do{
        CParamList oParams( m_pCtx );

        bool bReq = false;
        ret = oParams.Pop( bReq );
        if( ERROR( ret ) )
            break;

        DMsgPtr pMsg;
        ret = oParams.GetMsgPtr(
            propMsgPtr, pMsg );
        if( ERROR( ret ) )
            break;

        ObjPtr pObj;
        ret = oParams.GetObjPtr(
            propObjPtr, pObj );

        if( ERROR( ret ) )
            break;

        CDBusBusPort* pPort = pObj;
        if( pPort == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        if( bReq )
        {
            ret = pPort->OnMessageArrival( pMsg );
        }
        else
        {
            ret = pPort->OnLpbkMsgArrival( pMsg );
        }

    }while( 0 );

    return ret;
}

gint32 CDBusBusPort::ScheduleLpbkTask(
    MatchPtr& pFilter,
    DBusMessage *pDBusMsg,
    guint32* pdwSerial )
{

    gint32 ret = 0;
    do{
        ret = pFilter->IsMyMsgOutgoing( pDBusMsg );
        if( ERROR( ret ) )
            break;

        CParamList oParams;
        ret = oParams.SetObjPtr(
            propPortPtr, ObjPtr( this ) );

        if( ERROR( ret ) )
            break;

        ret = oParams.SetMsgPtr(
            propMsgPtr, pDBusMsg );

        if( ERROR( ret ) )
            break;

        gint32 iType = pFilter->GetType();
        bool bReq = true;

        if( iType == matchServer )
            bReq = false;

        oParams.Push( bReq );
        DMsgPtr pMsg( pDBusMsg );
        
        if( DBUS_MESSAGE_TYPE_METHOD_CALL
            == pMsg.GetType()
            && iType == matchClient )
        {
            // sending request, a serial number is
            // needed
            pMsg.SetSerial( ( guint32 )pDBusMsg );
            if( pdwSerial )
                *pdwSerial = ( guint32 )pDBusMsg;
        }

        CIoManager* pMgr = GetIoMgr();

        ret = pMgr->ScheduleTask(
            clsid( CDBusTransLpbkMsgTask ),
            oParams.GetCfg() );

    }while( 0 );

    if( ERROR( ret ) )
        ret = ERROR_NOT_HANDLED;

    return ret;
}

gint32 CDBusBusPort::SendLpbkMsg(
    DBusMessage *pDBusMsg, guint32* pdwSerial )
{
    if( pDBusMsg == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        MatchPtr pFilter = m_pMatchLpbkProxy;
        ret = ScheduleLpbkTask(
            pFilter, pDBusMsg, pdwSerial );

    }while( 0 );

    return ret;
}

DBusHandlerResult
CDBusBusPort::OnLpbkMsgArrival(
    DBusMessage* pMsg )
{
    gint32 ret = 0;

    if( pMsg == nullptr )
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    PortPtr pPort;
    BufPtr pBuf( true );

    do{
        CStdRMutex oPortLock( GetLock() );
        guint32 dwPortState = GetPortState();
        if( !( dwPortState == PORT_STATE_READY
             || dwPortState == PORT_STATE_BUSY_SHARED
             || dwPortState == PORT_STATE_BUSY
             || dwPortState == PORT_STATE_BUSY_RESUME ) )
        {
            ret = ERROR_STATE;
            break;
        }

        ret = GetPdoPort( m_iLpbkPortId, pPort );
        if( ERROR( ret ) )
            break;

        *pBuf = pMsg;
        if( pBuf.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

    }while( 0 );

    if( SUCCEEDED( ret ) )
    {
        CRpcPdoPort* pPdoPort = pPort;
        if( pPdoPort == nullptr )
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

        guint32* pData = ( guint32* )( ( CBuffer* )pBuf );

        ret = pPdoPort->OnEvent(
            cmdDispatchData, 0, 0, pData );

        if( ret == ERROR_PREMATURE )
            return DBUS_HANDLER_RESULT_HANDLED;
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

gint32 CDBusBusPort::FilterLpbkMsg(
    DBusMessage* pDBusMsg, guint32* pdwSerial )
{
    DMsgPtr pMsg( pDBusMsg );
    if( pMsg.IsEmpty() )
        return -EINVAL;

    MatchPtr pFilter = m_pMatchLpbkServer;
    gint32 ret = ScheduleLpbkTask(
        pFilter, pDBusMsg, pdwSerial );

    if( pMsg.GetType()
        == DBUS_MESSAGE_TYPE_SIGNAL )
    {
        // for signal, always not handled
        ret = ERROR_NOT_HANDLED;
    }

    return ret;
}
