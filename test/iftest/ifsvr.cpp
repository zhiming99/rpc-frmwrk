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

CEchoServer::CEchoServer( const IConfigDb* pCfg )
    : super( pCfg )
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
