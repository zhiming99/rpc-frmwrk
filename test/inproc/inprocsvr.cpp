/*
 * =====================================================================================
 *
 *       Filename:  inprocsvr.cpp
 *
 *    Description:  Implementation of the server. Actually the server can be accessed
 *                  from anywhere. This example is to demonstrate it can be accessed
 *                  from the same process the client lives in.
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
#include "inprocsvr.h"
#include "ifhelper.h"

using namespace std;


CInProcServer::CInProcServer( const IConfigDb* pCfg )
    : super( pCfg )
{
    SetClassId( clsid( CInProcServer ) );
}

gint32 CInProcServer::InitUserFuncs()
{
    gint32 ret = super::InitUserFuncs();

    if( ERROR( ret ) )
        return ret;

    BEGIN_HANDLER_MAP;
    
    ADD_USER_SERVICE_HANDLER(
        CInProcServer::Echo,
        METHOD_Echo );

    ADD_USER_SERVICE_HANDLER(
        CInProcServer::EchoUnknown,
        "EchoUnknown" );

    END_HANDLER_MAP;

    return 0;
}

gint32 CInProcServer::OnHelloWorld(
    const std::string& strEvent )
{
    BROADCAST_USER_EVENT(
        GetClsid(), strEvent );
    return 0;
}

gint32 CInProcServer::Echo(
    IEventSink* pCallback,
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

gint32 CInProcServer::EchoUnknown(
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

    INIT_MAP_ENTRYCFG( CInProcServer );
    INIT_MAP_ENTRYCFG( CInProcClient );

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
