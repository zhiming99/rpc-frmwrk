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
#include "asyncsvr.h"

using namespace std;

CAsyncClient::CAsyncClient(
    const IConfigDb* pCfg )
    : super( pCfg )
{
    SetClassId( clsid( CAsyncClient ) );
    Sem_Init( &m_semWait, 0, 0 );
}

gint32 CAsyncClient::InitUserFuncs()
{
    super::InitUserFuncs();

    BEGIN_PROXY_MAP( false );

    ADD_USER_PROXY_METHOD(
        METHOD_LongWait,
        const std::string& );

    ADD_USER_PROXY_METHOD(
        METHOD_LongWaitNoParam );

    END_PROXY_MAP;

    return 0;
}


gint32 CAsyncClient::LongWait(
    const std::string& strText,
    std::string& strReply )
{

    gint32 ret = 0;

    do{
        CfgPtr pResp;

        // make the call
        ret = SyncCall( pResp, METHOD_LongWait, strText );
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

gint32 CAsyncClient::WaitForComplete()
{
    gint32 ret = 0;

    do{
        ret = sem_wait( &m_semWait );

    } while( ret == -1 && errno == EINTR );

    if( ret == -1 )
        ret = -errno;

    return ret;
}

gint32 CAsyncClient::OnLongWaitNoParamComplete(
    IEventSink* pCallback,
    gint32 iRet )
{
    DebugPrint( iRet, "Entered the Complete callback" );
    Sem_Post( &m_semWait );
    return iRet;
}

gint32 CAsyncClient::LongWaitNoParam()
{
    gint32 ret = 0;

    do{
        CfgPtr pResp;

        TaskletPtr pTask;
        ret = BIND_CALLBACK( pTask,
            &CAsyncClient::OnLongWaitNoParamComplete );

        CCfgOpener oOption;
        ret = oOption.CopyProp(
            propIfName, this );

        if( ERROR( ret ) )
            break;

        // make the call
        ret = AsyncCall( pTask, oOption.GetCfg(),
            pResp, METHOD_LongWaitNoParam );

        if( ret == STATUS_PENDING )
            break;

        if( ERROR( ret ) )
            break;

        if( SUCCEEDED( ret ) )
        {
            CCfgOpener oResp(
                ( IConfigDb* )pResp );

            ret = oResp[ propReturnValue ];
        }

    }while( 0 );

    return ret;
}
