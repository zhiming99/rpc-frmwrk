/*
 * =====================================================================================
 *
 *       Filename:  ifclient.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/15/2018 01:15:23 PM
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


static CfgPtr InitClientCfg( const IConfigDb* pCfg )
{
    CParamList oParams;
    *oParams.GetCfg() = *pCfg;

    // port related information
    oParams[ propPortClass ] =
        PORT_CLASS_LOCALDBUS_PDO;

    oParams[ propPortId ] = 0;

    string strBusName = PORT_CLASS_LOCALDBUS;
    strBusName += "_0";

    if( !oParams.exist( propBusName ) )
        oParams[ propBusName ] = strBusName;

    if( !oParams.exist( propSrcDBusName ) )
        oParams[ propSrcDBusName ] =
            DBUS_DESTINATION( MOD_CLIENT_NAME );

    // dbus related information
    string strObjPath = DBUS_OBJ_PATH(
        MOD_SERVER_NAME, OBJNAME_ECHOSVR );

    string strDest( strObjPath );

    std::replace( strDest.begin(),
        strDest.end(), '/', '.');

    if( strDest[ 0 ] == '.' )
        strDest = strDest.substr( 1 );

    if( !oParams.exist( propDestDBusName ) )
        oParams[ propDestDBusName ] = strDest;

    if( !oParams.exist( propMatchType ) )
        oParams[ propMatchType ] = matchClient;

    if( !oParams.exist( propIfName ) )
        oParams[ propIfName ] =
            DBUS_IF_NAME( OBJNAME_ECHOSVR );

    if( !oParams.exist( propObjPath ) )
        oParams[ propObjPath ] = strObjPath;

    // timer information
    if( !oParams.exist( propTimeoutSec ) )
        oParams[ propTimeoutSec ] =
            IFSTATE_DEFAULT_IOREQ_TIMEOUT;

    if( !oParams.exist( propKeepAliveSec ) )
        oParams[ propKeepAliveSec ] =
            IFSTATE_DEFAULT_IOREQ_TIMEOUT;

    return oParams.GetCfg();
}

CEchoClient::CEchoClient(
    const IConfigDb* pCfg )
    : super( InitClientCfg( pCfg ) )
{
    SetClassId( clsid( CEchoClient ) );
}

gint32 CEchoClient::InitUserFuncs()
{
    super::InitUserFuncs();

    BEGIN_PROXY_MAP( false );

    ADD_USER_PROXY_METHOD(
        METHOD_Echo, const stdstr& );

    ADD_USER_PROXY_METHOD(
        METHOD_EchoPtr, const char* );

    ADD_USER_PROXY_METHOD(
        METHOD_EchoCfg, gint32, const CfgPtr& );

    ADD_USER_PROXY_METHOD(
        "Unknown", const BufPtr& );

    END_PROXY_MAP;

    return 0;
}


gint32 CEchoClient::Echo(
    const std::string& strText,
    std::string& strReply )
{

    gint32 ret = 0;

    do{
        CfgPtr pResp;

        // make the call
        ret = SyncCall( pResp, METHOD_Echo, strText );
        if( ERROR( ret ) )
            break;

        // fill the output parameter
        gint32 iRet = 0;
        ret = FillArgs( pResp, iRet, strReply );

        if( SUCCEEDED( ret ) )
            ret = iRet;

    }while( 0 );

    return ret;
}

gint32 CEchoClient::EchoPtr(
    const char* szText,
    const char*& szReply )
{

    gint32 ret = 0;

    do{
        CfgPtr pResp;

        // make the call
        ret = SyncCall( pResp, METHOD_EchoPtr, szText );
        if( ERROR( ret ) )
            break;

        // fill the output parameter
        gint32 iRet = 0;
        ret = FillArgs( pResp, iRet, szReply );

        if( SUCCEEDED( ret ) )
            ret = iRet;

    }while( 0 );

    return ret;
}

gint32 CEchoClient::EchoCfg(
    gint32 iCount,
    const CfgPtr& pCfg,
    gint32& iCountReply,
    CfgPtr& pCfgReply )
{

    gint32 ret = 0;

    do{
        CfgPtr pResp;

        // make the call
        ret = SyncCall( pResp, METHOD_EchoCfg, iCount, pCfg );
        if( ERROR( ret ) )
            break;

        // fill the output parameter
        gint32 iRet = 0;
        ret = FillArgs( pResp, iRet, iCountReply, pCfgReply );

        if( SUCCEEDED( ret ) )
            ret = iRet;

    }while( 0 );

    return ret;
}

gint32 CEchoClient::EchoUnknown(
    const BufPtr& pText,
    BufPtr& pReply )
{

    gint32 ret = 0;

    do{
        CfgPtr pResp;

        // make the call
        ret = SyncCall( pResp, "Unknown", pText );
        if( ERROR( ret ) )
            break;

        // fill the output parameter
        gint32 iRet = 0;
        ret = FillArgs( pResp, iRet, pReply );

        if( SUCCEEDED( ret ) )
            ret = iRet;

    }while( 0 );

    return ret;
}
