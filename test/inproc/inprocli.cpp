/*
 * =====================================================================================
 *
 *       Filename:  inprocli.cpp
 *
 *    Description:  implementation of the in-process client
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
#include <unistd.h>
#include <cppunit/TestFixture.h>
#include "inprocsvr.h"
#include "ifhelper.h"

using namespace std;

CInProcClient::CInProcClient(
    const IConfigDb* pCfg )
    : super( pCfg )
{
    SetClassId( clsid( CInProcClient ) );
}

gint32 CInProcClient::InitUserFuncs()
{
    super::InitUserFuncs();

    // event map
    BEGIN_HANDLER_MAP;

    ADD_USER_EVENT_HANDLER(
        CInProcClient::OnHelloWorld,
        EVENT_HelloWorld );

    END_HANDLER_MAP;

    // method proxy
    BEGIN_PROXY_MAP( false );

    ADD_USER_PROXY_METHOD(
        METHOD_Echo, const stdstr& );

    ADD_USER_PROXY_METHOD(
        "Unknown", const BufPtr& );

    END_PROXY_MAP;

    return 0;
}


gint32 CInProcClient::OnHelloWorld(
    IEventSink* pContext,
    const std::string& strText )
{
    gint32 ret = 0;

    do{
        printf( "Received Event: %s\n",
            strText.c_str() );
    }while( 0 );

    return ret;
}

gint32 CInProcClient::Echo(
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

gint32 CInProcClient::EchoUnknown(
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

