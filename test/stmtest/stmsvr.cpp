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

using namespace rpcf;
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
    BufPtr pBuf;
    CfgPtr pCfg;
    gint32 ret = 0;

    // get channel specific context
    ret = GetContext( hChannel, pCfg );
    if( ERROR( ret ) )
        return ret;

    CParamList oCfg( pCfg );
    guint32 dwCount = oCfg[ 0 ];

    do{
        if( ERROR( iRet ) )
        {
            ret = iRet;
            break;
        }

        ret = PeekStream( hChannel, pBuf );
        if( ret == -EAGAIN )
        {
            PauseReadNotify( hChannel, false );
            ret = 0;
            break;
        }
        else if( ERROR( ret ) )
        {
            break;
        }

        std::string strMsg = DebugMsg( 0,
            "this is the %d msg ", dwCount + 1 );

        BufPtr pNewBuf( true );
        *pNewBuf = strMsg;

        ret = WriteStreamNoWait(
            hChannel, pNewBuf );

        if( ret == STATUS_PENDING )
        {
            printf( "Proxy @0x%x says(%d): %s\n",
                ( guint32 )hChannel, dwCount++,
                ( char* )pBuf->ptr() );
            // remove the buf from the pending
            // queue.
            ReadStreamNoWait( hChannel, pBuf );
            ret = 0;
            break;
        }
        else if( ret == ERROR_QUEUE_FULL )
        {
            // wait till another OnSendDone_Loop,
            // when the writing queue has free
            // slot
            ret = 0;
            break;
        }

        if( ERROR( ret ) )
            break;

    }while( 0 );

    if( SUCCEEDED( ret ) )
    {
        oCfg[ 0 ] = dwCount;
    }
    else if( ERROR( ret ) )
    {
        DebugPrintEx( logNotice, ret,
            "Error in OnSendDone_Loop"
            " on stream 0x%x", hChannel );
    }

    return 0;
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
            WriteStreamNoWait( hChannel, pBuf );
            oCfg.Push( dwCount );
            ret = 0;
            break;
        }
        else if( ERROR( ret ) )
        {
            break;
        }

    }while( 0 );

    if( ERROR( ret ) )
    {
        DebugPrintEx( logNotice, ret,
            "Error in OnWriteEnabled_Loop"
            " on stream 0x%x", hChannel );
    }

    return ret;
}

gint32 CMyStreamServer::OnRecvData_Loop(
    HANDLE hChannel, gint32 iRet )
{
    BufPtr pBuf;
    CfgPtr pCfg;
    gint32 ret = 0;

    bool bPaused = false;
    ret = IsReadNotifyPaused(
        hChannel, bPaused );

    if( ERROR( ret ) )
        return ret;

    if( bPaused )
        return 0;

    // get channel specific context
    ret = GetContext( hChannel, pCfg );
    if( ERROR( ret ) )
        return ret;

    CParamList oCfg( pCfg );
    guint32 dwCount = oCfg[ 0 ];

    do{
        if( ERROR( iRet ) )
        {
            ret = iRet;
            break;
        }

        ret = PeekStream( hChannel, pBuf );
        if( ret == -EAGAIN )
        {
            ret = 0;
            break;
        }
        else if( ERROR( ret ) )
        {
            break;
        }
        PauseReadNotify( hChannel, true );

        std::string strMsg = DebugMsg( 0,
            "this is the %d msg ", dwCount + 1 );

        BufPtr pNewBuf( true );
        *pNewBuf = strMsg;

        ret = WriteStreamNoWait(
            hChannel, pNewBuf );

        if( ret == STATUS_PENDING )
        {
            printf( "Proxy @0x%x says(%d): %s\n",
                ( guint32 )hChannel, dwCount++,
                ( char* )pBuf->ptr() );
            // remove the buf from the pending
            // queue.
            ReadStreamNoWait( hChannel, pBuf );
            ret = 0;
            break;
        }
        else if( ret == ERROR_QUEUE_FULL )
        {
            // wait till another OnSendDone_Loop,
            // when the writing queue has free
            // slot
            ret = 0;
            break;
        }

        if( ERROR( ret ) )
            break;

    }while( 0 );

    if( SUCCEEDED( ret ) )
    {
        oCfg[ 0 ] = dwCount;
    }
    else if( ERROR( ret ) )
    {
        DebugPrintEx( logNotice, ret,
            "Error in OnRecvData_Loop"
            " on stream 0x%x", hChannel );
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
