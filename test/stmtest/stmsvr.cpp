/*
 * =====================================================================================
 *
 *       Filename:  stmsvr.cpp
 *
 *    Description:  Implementation of interface class CEchoServer, and CMyStreamServer,
 *                  and global helper function, InitClassFactory and DllLoadFactory
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

// implementation of interface CEchoServer
gint32 CEchoServer::InitUserFuncs()
{
    BEGIN_IFHANDLER_MAP( CEchoServer );

    ADD_USER_SERVICE_HANDLER_EX( 1,
        CEchoServer::Echo,
        METHOD_Echo );

    END_IFHANDLER_MAP;

    return 0;
}

gint32 CEchoServer::Echo(
    IEventSink* pCallback,
    const std::string& strText, // [ in ]
    std::string& strReply )     // [ out ]
{
    gint32 ret = 0;

    // business logics goes here
    strReply = strText;

    return ret;
}

// implementation of interface CMyStreamServer
gint32 CMyStreamServer::OnConnected(
    HANDLE hChannel )
{
    std::string strGreeting = "Hello, Proxy";
    BufPtr pBuf( true );
    *pBuf = strGreeting;
    WriteStream( hChannel, pBuf, nullptr );
    return 0;
}

static int iMsgCount = 0;
gint32 CMyStreamServer::OnStmRecv(
    HANDLE hChannel, BufPtr& pBuf )
{
    printf( "Proxy says(%d): %s\n",
        iMsgCount, ( char* )pBuf->ptr() );

    std::string strMsg = DebugMsg( 0,
        "this is the %d msg", iMsgCount++ );
    BufPtr pNewBuf( true );
    *pNewBuf = strMsg;

    // Send a reply, if there is an error, the stream
    // channel will be closed. The code is for demo
    // only.
    //
    // The return code of ERROR_QUEUE_FULL should be
    // handled correctly.  that is if this error is
    // returned, you need to resend the data if
    // necessary, when the flow control is lifted, as
    // you can refer to stmcli.cpp and stmtest.cpp
    // for detail.
    // 
    WriteStream( hChannel, pNewBuf, nullptr );
    return 0;
}

gint32 CMyStreamServer::OnClose(
    HANDLE hChannel,
    IEventSink* pCallback )
{
    iMsgCount = 0;
    return 0;
}

static FactoryPtr InitClassFactory()
{
    BEGIN_FACTORY_MAPS;

    INIT_MAP_ENTRYCFG( CStreamingServer );
    INIT_MAP_ENTRYCFG( CStreamingClient );

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
