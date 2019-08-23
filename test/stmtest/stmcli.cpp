/*
 * =====================================================================================
 *
 *       Filename:  stmcli.cpp
 *
 *    Description:  implementation of the proxy interface class CEchoClient, and CMyStreamProxy
 *
 *        Version:  1.0
 *        Created:  12/04/2018 10:37:00 AM
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
#include "stmsvr.h"

using namespace std;

// implementation of proxy for CEchoServer
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

// implementation of proxy for CMyStreamServer
gint32 CMyStreamProxy::OnConnected(
    HANDLE hChannel )
{
    BufPtr pBuf( true );
    *pBuf = std::string( "Hello, Server" );
    WriteStream( hChannel, pBuf, nullptr );
    DebugPrint( 0, "say hello to server" );
    return 0;
}

gint32 CMyStreamProxy::OnStmRecv(
    HANDLE hChannel, BufPtr& pBuf )
{
    // note that, the two sides should be aware of
    // the content of the pBuf
    std::string strMsg(
        ( char* )pBuf->ptr() );

    printf( "server says: %s\n",
        strMsg.c_str() );
    return 0;
}

