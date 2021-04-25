/*
 * =====================================================================================
 *
 *       Filename:  main.cpp
 *
 *    Description:  for the smoke test purpose
 *
 *        Version:  1.0
 *        Created:  06/14/2018 09:27:18 PM
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

#include "proxy.h"
#include "frmwrk.h"

using namespace rpcf;
#include "asyncsvr.h"

using namespace std;

CAsyncServer::CAsyncServer( const IConfigDb* pCfg )
    : super( pCfg )
{
    SetClassId( clsid( CAsyncServer ) );
}

gint32 CAsyncServer::InitUserFuncs()
{
    super::InitUserFuncs();

    BEGIN_HANDLER_MAP;

    ADD_USER_SERVICE_HANDLER(
        CAsyncServer::LongWait,
        METHOD_LongWait );

    ADD_USER_SERVICE_HANDLER(
        CAsyncServer::LongWaitNoParam,
        METHOD_LongWaitNoParam );

    END_HANDLER_MAP;
    return 0;
}
/**
* @name OnLongWaitComplete: the callback of the async
* hander `LongWait'.
*
* @{ */
/**
* @iRet:
*   the return value of the response. it is a
*   mandatory parameter for the async callback. but not
*   used in this callback. it must be the first
*   parameter in the callback, if you want to use the
*   BIND_CALLBACK helper macro.
*
* @pCallback:
*   the callback the same as the one passed to
*   LongWait. it is actually the CIfInvokeMethodTask,
*   to receive the response data. it is also mandatory,
*   but the position can change.
*
* @strText:
*   the text to echo
* @} */

gint32 CAsyncServer::OnLongWaitComplete(
    IEventSink* pCallback,
    const std::string& strText )
{

    // business logics goes here
    std::string strReply( strText );

    strReply += " 2";
    gint32 ret = this->FillAndSetResponse(
        pCallback, 0, strReply );

    if( ERROR( ret ) )
        return ret;

    // notify to complete this async call. And
    // eventTaskComp is the event for this purpose.
    pCallback->OnEvent(
        eventTaskComp, ret, 0, 0 );

    return ret;
}

gint32 CAsyncServer::LongWait(
    IEventSink* pCallback,
    const std::string& strText )
{
    gint32 ret = 0;

    TaskletPtr pTask;

    // new a callback task and bind it with the
    // callback method we want to call when this call
    // finally complete
    ret = NewDeferredCall( pTask, ObjPtr( this ),
        &CAsyncServer::OnLongWaitComplete,
        pCallback, strText );

    if( ERROR( ret ) )
        return ret;

    // schedule a timer as the async approach for this
    // example
    CUtilities& oUtils =
        GetIoMgr()->GetUtils();

    CTimerService& oTimerSvc =
        oUtils.GetTimerSvc();

    // schedule timer
    gint32 iTimer = oTimerSvc.AddTimer(
        3, pTask, 0 );

    if( iTimer < 0 )
        return iTimer;

    // notify the caller, we have not complete yet
    return STATUS_PENDING;
}

gint32 CAsyncServer::LongWaitNoParam(
    IEventSink* pCallback )
{
    // Please note the async approach on the client
    // side

    gint32 ret = 0;
    sleep( 2 );

    // make the response
    ret = this->FillAndSetResponse(
        pCallback, ret );

    return ret;
}

static FactoryPtr InitClassFactory()
{
    BEGIN_FACTORY_MAPS;

    INIT_MAP_ENTRYCFG( CAsyncServer );
    INIT_MAP_ENTRYCFG( CAsyncClient );

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
