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

gint32 CMyStreamServer::OnSendDone_Loop(
    HANDLE hChannel, gint32 iRet )
{
    gint32 ret = 0;

    do{
        // this handler will be the first one to
        // call after the loop starts
        //
        // Enable incoming data notification via
        // OnRecvData_Loop
        PauseReadNotify( hChannel, false );

    }while( 0 );

    if( ERROR( ret ) )
        StopLoop();

    return ret;
}

// implementation of interface CMyStreamServer
gint32 CMyStreamServer::OnWriteEnabled_Loop(
    HANDLE hChannel )
{
    gint32 ret = 0;

    CfgPtr pCfg;
    // get channel specific context
    ret = GetContext( hChannel, pCfg );
    if( ERROR( ret ) )
        return ret;

    CParamList oCfg( pCfg );
    guint32 dwCount = 0;

    do{
        // this handler will be the first one to
        // call after the loop starts
        ret = oCfg.GetIntProp( 0, dwCount );
        if( ret == -ENOENT )
        {
            // the first time, send greetings
            BufPtr pBuf( true );
            *pBuf = std::string( "Hello, Proxy" );
            WriteMsg( hChannel, pBuf, -1 );
            oCfg.Push( dwCount );
            ret = 0;
            break;
        }
        else if( ERROR( ret ) )
        {
            break;
        }

        // Enable incoming data notification via
        // OnRecvData_Loop
        PauseReadNotify( hChannel, false );

    }while( 0 );

    if( ERROR( ret ) )
        StopLoop();

    return ret;
}

gint32 CMyStreamServer::OnRecvData_Loop(
    HANDLE hChannel )
{
    BufPtr pBuf;
    CfgPtr pCfg;

    // get channel specific context
    gint32 ret = GetContext(
        hChannel, pCfg );
    if( ERROR( ret ) )
        return ret;

    CParamList oCfg( pCfg );
    guint32 dwCount = oCfg[ 0 ];

    do{
        ret = ReadMsg( hChannel, pBuf, -1 );
        if( ret == -EAGAIN )
        {
            ret = 0;
            break;
        }
        else if( ERROR( ret ) )
        {
            break;
        }

        printf( "Proxy says(%d): %s\n",
            dwCount, ( char* )pBuf->ptr() );

        std::string strMsg = DebugMsg( 0,
            "this is the %d msg", ++dwCount );

        BufPtr pNewBuf( true );
        *pNewBuf = strMsg;

        ret = WriteMsg( hChannel, pNewBuf, -1 );
        if( ret == ERROR_QUEUE_FULL )
        {
            // Disable incoming data notification
            // via OnRecvData_Loop till the flow
            // control is lifted.
            PauseReadNotify( hChannel, true );
            ret = 0;
            break;
        }
        else if( ret == STATUS_PENDING )
        {
            // Disable incoming data notification
            // via OnRecvData_Loop till the write
            // complete
            PauseReadNotify( hChannel, true );
            ret = 0;
            break;
        }

    }while( 1 );

    if( ERROR( ret ) )
    {
        StopLoop();
    }
    else
    {
        oCfg[ 0 ] = dwCount;
    }

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
