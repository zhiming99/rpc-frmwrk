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

using namespace rpcf;
#include "stmsvr.h"
#include <climits>

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

        auto& stmctx = GetContext2( hChannel );
        std::string strMsg(
            ( char* )pBuf->ptr() );

        size_t pos = strMsg.rfind( ' ' ); 
        if( pos == stdstr::npos )
            continue;

        pos = strMsg.rfind( ' ', pos - 1 );
        if( pos == stdstr::npos )
            continue;

        size_t end = pos;
        pos = strMsg.rfind( ' ', pos - 1 );
        if( pos == stdstr::npos )
            continue;

        stdstr strVal = strMsg.substr(
            pos + 1, end - pos - 1 );

        guint32 dwCount = strtoul(
            strVal.c_str(), nullptr, 10 );
        if( dwCount == ULONG_MAX )
            continue;

        if( ( dwCount & 0x3ff ) == 0 )
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

    // get channel specific context
    auto& stmctx = GetContext2( hChannel );

    BufPtr pBuf( true );

    do{
        guint32 dwCount = stmctx.GetCounter();
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

        stmctx.IncCounter();

    }while( 0 );
    return ret;
}

gint32 CMyStreamProxy::OnSendDone_Loop(
    HANDLE hChannel, gint32 iRet )
{
    gint32 ret = 0;

    CfgPtr pCfg;
    // get channel specific context
    auto& stmctx = GetContext2( hChannel );

    do{
        if( ERROR( iRet ) )
        {
            ret = iRet;
            break;
        }

        stmctx.IncCounter();
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
    auto& stmctx = GetContext2( hChannel );

    do{
        // this handler will be the first one to
        // call after the loop starts
        guint32 dwCount = stmctx.GetCounter();
        if( dwCount == 0 )
        {
            // the first time, send greetings
            BufPtr pBuf( true );
            *pBuf = std::string( "Hello, Server" );
            WriteStreamNoWait( hChannel, pBuf );
            DebugPrint( 0, "say hello to server" );
            stmctx.IncCounter();
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
    gint32 ret = StopLoop();
    RemoveContext( hChannel );
    return ret;
}
