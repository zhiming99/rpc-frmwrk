/*
 * =====================================================================================
 *
 *       Filename:  main.cpp
 *
 *    Description:  for the smoke test purpose
 *
 *        Version:  1.0
 *        Created:  06/14/2018 09:27:18 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com ), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <rpc.h>
#include <string>
#include <iostream>
#include <unistd.h>
#include <cppunit/TestFixture.h>
#include "ifsvr.h"
#include "ifhelper.h"

using namespace std;


static CfgPtr InitSvrCfg( const IConfigDb* pCfg )
{
    CParamList oParams;
    *oParams.GetCfg() = *pCfg;

    // port related information
    if( !oParams.exist( propPortClass ) )
        oParams[ propPortClass ] =
            PORT_CLASS_LOCALDBUS_PDO;

    if( !oParams.exist( propPortId ) )
        oParams[ propPortId ] = 0;

    string strBusName = PORT_CLASS_LOCALDBUS;
    strBusName += "_0";

    if( !oParams.exist( propBusName ) )
        oParams[ propBusName ] = strBusName;

    // dbus related information
    if( !oParams.exist( propDestDBusName ) )
        oParams[ propDestDBusName ] =
            DBUS_DESTINATION( MOD_SERVER_NAME );

    if( !oParams.exist( propMatchType ) )
        oParams[ propMatchType ] = matchServer;

    if( !oParams.exist( propIfName ) )
        oParams[ propIfName ] =
            DBUS_IF_NAME( OBJNAME_ECHOSVR  );

    if( !oParams.exist( propObjPath ) )
        oParams[ propObjPath ] = DBUS_OBJ_PATH(
            MOD_SERVER_NAME, OBJNAME_ECHOSVR );

    // timer information
    if( !oParams.exist( propTimeoutSec ) )
        oParams[ propTimeoutSec ] =
            IFSTATE_DEFAULT_IOREQ_TIMEOUT;

    if( !oParams.exist( propKeepAliveSec ) )
        oParams[ propKeepAliveSec ] =
            IFSTATE_DEFAULT_IOREQ_TIMEOUT;

    return oParams.GetCfg();
}

CEchoServer::CEchoServer( const IConfigDb* pCfg )
    : super( InitSvrCfg( pCfg ) )
{
    SetClassId( clsid( CEchoServer ) );
}

gint32 CEchoServer::InitUserFuncs()
{
    BEGIN_HANDLER_MAP;

    ADD_USER_SERVICE_HANDLER(
        CEchoServer::Echo,
        METHOD_Echo );

    ADD_USER_SERVICE_HANDLER(
        CEchoServer::EchoPtr,
        METHOD_EchoPtr );

    ADD_USER_SERVICE_HANDLER(
        CEchoServer::EchoCfg,
        METHOD_EchoCfg );

    ADD_USER_SERVICE_HANDLER(
        CEchoServer::EchoUnknown,
        "Unknown" );

    END_HANDLER_MAP;
    return 0;
}


gint32 CEchoServer::Echo( IEventSink* pCallback,
    const std::string& strText )
{
    gint32 ret = 0;

    // business logics goes here
    std::string strReply( strText );

    // make the response
    gint32 iRet = 0;
    ret = this->FillAndSetResponse(
        pCallback, iRet, strReply );

    if( SUCCEEDED( ret ) )
        return iRet;

    return ret;
}

gint32 CEchoServer::EchoPtr(
    IEventSink* pCallback,
    const char* szText )
{
    gint32 ret = 0;

    // business logics goes here

    // make the response
    gint32 iRet = 0;
    ret = this->FillAndSetResponse(
        pCallback, iRet, szText );

    if( SUCCEEDED( ret ) )
        return iRet;

    return ret;
}

gint32 CEchoServer::EchoCfg(
    IEventSink* pCallback,
    gint32 iCount,
    const CfgPtr& pCfg )
{

    gint32 ret = 0;

    // business logics goes here

    // make the response
    gint32 iRet = 0;
    ret = this->FillAndSetResponse(
        pCallback, iRet, iCount, pCfg );

    if( SUCCEEDED( ret ) )
        return iRet;

    return ret;
}

gint32 CEchoServer::EchoUnknown(
    IEventSink* pCallback,
    const BufPtr& pBuf )
{

    gint32 ret = 0;

    // business logics goes here

    // make the response
    gint32 iRet = 0;
    ret = this->FillAndSetResponse(
        pCallback, iRet, pBuf );

    if( SUCCEEDED( ret ) )
        return iRet;

    return ret;
}

static FactoryPtr InitClassFactory()
{
    BEGIN_FACTORY_MAPS;

    INIT_MAP_ENTRYCFG( CEchoServer );
    INIT_MAP_ENTRYCFG( CEchoClient );

    END_FACTORY_MAPS;
};

extern "C"
gint32 DllLoadFactory( FactoryPtr& pFactory )
{
    pFactory = InitClassFactory();
    if( pFactory.IsEmpty() )
        return -EFAULT;

    return 0;
}
