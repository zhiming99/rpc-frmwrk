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
#include "prsvr.h"
#include "proxy.h"
#include "frmwrk.h"

using namespace std;

CPauseResumeServer::CPauseResumeServer(
    const IConfigDb* pCfg )
    : super( pCfg )
{
    SetClassId( clsid( CPauseResumeServer ) );
}

gint32 CPauseResumeServer::InitUserFuncs()
{
    super::InitUserFuncs();

    BEGIN_HANDLER_MAP;

    ADD_USER_SERVICE_HANDLER(
        CPauseResumeServer::LongWait,
        METHOD_LongWait );

    ADD_USER_SERVICE_HANDLER(
        CPauseResumeServer::EchoUnknown,
        "Unknown" );

    ADD_USER_SERVICE_HANDLER(
        CPauseResumeServer::Echo,
        METHOD_Echo );

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
*   the return value of the response. it is a mandatory
*   parameter for the async callback. but not used in
*   this callback. it must be the first parameter in
*   the callback, if you want to use the BIND_CALLBACK
*   helper macro.
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

gint32 CPauseResumeServer::OnLongWaitComplete(
    IEventSink* pCallback,
    const std::string& strText )
{

    // business logics goes here
    std::string strReply( strText );
    strReply += " 2";

    // wait for 20 seconds
    sleep( 20 );

    // make the response
    gint32 ret = this->FillAndSetResponse(
        pCallback, 0, strReply );

    if( ERROR( ret ) )
        return ret;

    // one extra step for async handler, that is, to
    // notify completion of this async call via the
    // callback. And eventTaskComp is the event for
    // this purpose.
    pCallback->OnEvent(
        eventTaskComp, ret, 0, 0 );

    return ret;
}

gint32 CPauseResumeServer::EchoUnknown(
    IEventSink* pCallback,
    const BufPtr& pBuf )
{

    gint32 ret = 0;

    // business logics goes here
    BufPtr oBufReply = pBuf;

    // make the response
    gint32 iRet = 0;
    ret = this->FillAndSetResponse(
        pCallback, iRet, pBuf );

    if( SUCCEEDED( ret ) )
        return iRet;

    return ret;
}

gint32 CPauseResumeServer::Echo(
    IEventSink* pCallback,
    const std::string& strText )
{
    gint32 ret = 0;

    // business logics goes here
    std::string strReply( strText );

    // make the response
    gint32 iRet = 0;
    ret = this->FillAndSetResponse(
        pCallback, iRet, strReply );

    if( SUCCEEDED( ret ) )
        return iRet;

    return ret;
}

gint32 CPauseResumeServer::LongWait(
    IEventSink* pCallback,
    const std::string& strText )
{
    gint32 ret = 0;

    TaskletPtr pTask;

    // new a callback task and bind it with the
    // callback method we want to call when this call
    // finally complete
    ret = DEFER_CALL_NOSCHED(
        pTask, ObjPtr( this ),
        &CPauseResumeServer::OnLongWaitComplete,
        pCallback, strText );

    if( ERROR( ret ) )
        return ret;

    // create a standalone thread for this long
    // task
    GetIoMgr()->RescheduleTask( pTask, true );

    return STATUS_PENDING;
}

static FactoryPtr InitClassFactory()
{
    BEGIN_FACTORY_MAPS;

    INIT_MAP_ENTRYCFG( CPauseResumeServer );
    INIT_MAP_ENTRYCFG( CPauseResumeClient );

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
