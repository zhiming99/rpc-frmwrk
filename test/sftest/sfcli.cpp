/*
 * =====================================================================================
 *
 *       Filename:  sfcli.cpp
 *
 *    Description:  implementation of the proxy interface class CEchoClient.
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
#include "sfsvr.h"

using namespace std;

CEchoClient::CEchoClient(
    const IConfigDb* pCfg )
    : super( pCfg )
{
}

gint32 CEchoClient::InitUserFuncs()
{
    BEGIN_IFPROXY_MAP( CEchoServer, false );

    ADD_USER_PROXY_METHOD_EX( 1,
        CEchoClient::Echo,
        METHOD_Echo );

    END_PROXY_MAP;

    return 0;
}

gint32 CEchoClient::Echo(
    const std::string& strText, // [ in ]
    std::string& strReply ) // [ out ]
{
    return FORWARD_IF_CALL(
        iid( CEchoServer ),
        1, METHOD_Echo,
        strText, strReply  );
}

