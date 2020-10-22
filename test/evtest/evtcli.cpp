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
#include <unistd.h>
#include <cppunit/TestFixture.h>
#include "evtsvr.h"
#include "ifhelper.h"

using namespace std;

CEventClient::CEventClient(
    const IConfigDb* pCfg )
    : super( pCfg )
{
    SetClassId( clsid( CEventClient ) );
}

gint32 CEventClient::InitUserFuncs()
{
    super::InitUserFuncs();

    BEGIN_HANDLER_MAP;

    ADD_USER_EVENT_HANDLER(
        CEventClient::OnHelloWorld,
        EVENT_HelloWorld );

    END_HANDLER_MAP;

    return 0;
}


gint32 CEventClient::OnHelloWorld(
    IEventSink* pContext,
    const std::string& strText )
{
    gint32 ret = 0;
    static guint32 dwCount = 0;
    std::string strOutput =
        DebugMsg( 0, "%d: %s",
        dwCount++,
        strText.c_str() );

    strOutput.push_back( '\n' );
    printf( strOutput.c_str() );
    return ret;
}

