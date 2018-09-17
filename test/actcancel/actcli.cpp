/*
 * =====================================================================================
 *
 *       Filename:  actcli.cpp
 *
 *    Description:  this example is to demonstrate the technique to actively
 *    cancel a on-going request from client side. both the proxy and server will
 *    handle the request in asynchronous style, to make a successful cancel.
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
#include "actcsvr.h"

using namespace std;


CActcClient::CActcClient(
    const IConfigDb* pCfg )
    : super( pCfg )
{
    SetClassId( clsid( CActcClient ) );
    Sem_Init( &m_semWait, 0, 0 );
}

CActcClient::~CActcClient()
{
    sem_destroy( &m_semWait );
}

gint32 CActcClient::InitUserFuncs()
{
    super::InitUserFuncs();

    BEGIN_PROXY_MAP( false );

    ADD_USER_PROXY_METHOD(
        METHOD_LongWait,
        const std::string& );

    END_PROXY_MAP;

    return 0;
}


gint32 CActcClient::LongWait(
    CfgPtr& pResp,
    const std::string& strText,
    std::string& strReply )
{

    gint32 ret = 0;
    if( pResp.IsEmpty() )
        pResp.NewObj();

    do{
        TaskletPtr pTask;
        ret = BIND_CALLBACK( pTask,
            &CActcClient::OnLongWaitComplete );

        if( ERROR( ret ) )
            break;

        CCfgOpenerObj oTaskCfg(
            ( CObjBase* )pTask );

        oTaskCfg.SetObjPtr(
            propRespPtr, ObjPtr( pResp ) );

        CCfgOpener oOption;
        ret = oOption.CopyProp(
            propIfName, this );

        if( ERROR( ret ) )
            break;

        // make the call
        ret = AsyncCall( pTask, oOption.GetCfg(),
            pResp, METHOD_LongWait, strText );

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

gint32 CActcClient::WaitForComplete()
{
    gint32 ret = 0;

    do{
        ret = sem_wait( &m_semWait );

    } while( ret == -1 && errno == EINTR );

    if( ret == -1 )
        ret = -errno;

    return ret;
}

gint32 CActcClient::OnLongWaitComplete(
    IEventSink* pTask,
    gint32 iRet,
    const string& strReply )
{
    if( pTask == nullptr )
        return -EINVAL;

    if( SUCCEEDED( iRet ) )
    {
        DebugPrint( iRet,
            "Entered the Complete callback with reply %s",
            strReply.c_str() );
    }

    CCfgOpenerObj oTaskCfg(
        ( CObjBase* )pTask );

    ObjPtr pObj;
    gint32 ret =  oTaskCfg.GetObjPtr(
        propRespPtr, pObj );

    if( ERROR( ret ) )
        return ret;

    CfgPtr pResp = pObj;

    CParamList oResp( ( IConfigDb* )pResp );
    oResp[ propReturnValue ] = iRet;

    if( SUCCEEDED( iRet ) )
        oResp.Push( strReply );

    Sem_Post( &m_semWait );
    return iRet;

}

