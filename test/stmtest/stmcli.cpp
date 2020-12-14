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

using namespace rpcfrmwrk;
#include "stmsvr.h"

using namespace std;

gint32 CMyStreamProxy::OnRecvData_Loop(
    HANDLE hChannel, gint32 iRet )
{
    BufPtr pBuf;
    gint32 ret = 0;
    do{
        if( ERROR( iRet ) )
        {
            ret = iRet;
            break;
        }

        ret = ReadStreamNoWait( hChannel, pBuf );
        if( ret == -EAGAIN )
        {
            ret = 0;
            break;
        }

        if( ERROR( ret ) )
            break;

        std::string strMsg(
            ( char* )pBuf->ptr() );

        printf( "server says: %s\n",
            strMsg.c_str() );

    }while( 1 );

    if( ERROR( ret ) )
        StopLoop( ret );

    return ret;
}

gint32 CMyStreamProxy::SendMessage(
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
    BufPtr pBuf( true );

    do{
        ret = oCfg.GetIntProp( 0, dwCount );
        if( ERROR( ret ) )
            break;

        if( dwCount == LOOP_COUNT )
        {
            StopLoop();
            break;
        }

        std::string strMsg = DebugMsg(
            dwCount, "a message to server" );

        *pBuf = strMsg;
        ret = WriteStreamNoWait( hChannel, pBuf );
        if( ret == ERROR_QUEUE_FULL )
        {
            printf( "Queue full the message later...\n" );
            ret = 0;
            break;
        }
        else if( ret == STATUS_PENDING )
        {
            break;
        }

        if( ERROR( ret ) )
            break;

        if( SUCCEEDED( ret ) )
            dwCount++;

    }while( 0 );


    if( SUCCEEDED( ret ) )
    {
        // update the context for next run
        oCfg.SetIntProp( 0, dwCount );
    }

    return ret;
}

gint32 CMyStreamProxy::OnSendDone_Loop(
    HANDLE hChannel, gint32 iRet )
{
    gint32 ret = 0;

    CfgPtr pCfg;
    // get channel specific context
    ret = GetContext( hChannel, pCfg );
    if( ERROR( ret ) )
        return ret;

    do{
        if( ERROR( iRet ) )
        {
            ret = iRet;
            break;
        }

        CParamList oCfg( pCfg );
        guint32 dwCount = oCfg[ 0 ];
        ++dwCount;
        oCfg[ 0 ] = dwCount;

        ret = SendMessage( hChannel );

    }while( 0 );

    if( ERROR( ret ) )
    {
        CancelChannel( hChannel );
        StopLoop( ret );
    }

    return ret;
}

gint32 CMyStreamProxy::OnWriteEnabled_Loop(
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
            *pBuf = std::string( "Hello, Server" );
            WriteStreamNoWait( hChannel, pBuf );
            DebugPrint( 0, "say hello to server" );
            oCfg.Push( ++dwCount );
            ret = 0;
        }
        else if( ERROR( ret ) )
            break;

    }while( 0 );

    if( ERROR( ret ) )
    {
        CancelChannel( hChannel );
        StopLoop( ret );
    }

    return ret;
}

gint32 CMyStreamProxy::OnStart_Loop()
{
    HANDLE hChannel = INVALID_HANDLE;
    gint32 ret = StartStream( hChannel );
    if( ERROR( ret ) )
        StopLoop( ret );

    return ret;
}

gint32 CMyStreamProxy::OnCloseChannel_Loop(
    HANDLE hChannel )
{
    return StopLoop();
}
