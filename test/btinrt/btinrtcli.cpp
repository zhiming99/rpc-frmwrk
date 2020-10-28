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

using namespace rpcfrmwrk;
#include "btinrtsvr.h"

using namespace std;

gint32 CEchoClient::InitUserFuncs()
{
    super::InitUserFuncs();

    BEGIN_HANDLER_MAP;

    ADD_USER_EVENT_HANDLER(
        CEchoClient::OnHelloWorld,
        EVENT_HelloWorld );

    END_HANDLER_MAP;

    return 0;
}

gint32 CEchoClient::OnHelloWorld(
    IEventSink* pContext,
    const std::string& strText )
{
    gint32 ret = 0;
    static guint32 dwCount = 0;

    do{
        printf( "%d: Received Event: %s\n",
            dwCount++, strText.c_str() );
    }while( 0 );

    return ret;
}
