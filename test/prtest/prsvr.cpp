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

    // 1 indicates the input parameter is 1.
    ADD_USER_SERVICE_HANDLER_EX( 1,
        CPauseResumeServer::EchoUnknown,
        "Unknown" );

    ADD_USER_SERVICE_HANDLER_EX( 1,
        CPauseResumeServer::Echo,
        METHOD_Echo );

    ADD_USER_SERVICE_HANDLER_EX( 6,
        CPauseResumeServer::EchoMany,
        METHOD_EchoMany );

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

gint32 CPauseResumeServer::EchoUnknown(
    IEventSink* pCallback,
    const BufPtr& pBuf, // [ in ]
    BufPtr& pBufReply ) // [ out ]
{

    gint32 ret = 0;

    // business logics goes here
    BufPtr oBufReply = pBuf;

    return ret;
}

gint32 CPauseResumeServer::Echo(
    IEventSink* pCallback,
    const std::string& strText, // [ in ]
    std::string& strReply )     // [ out ]
{
    gint32 ret = 0;

    // business logics goes here
    strReply = strText;

    return ret;
}

gint32 CPauseResumeServer::EchoMany(
    IEventSink* pCallback, // mandatory
    gint32 i1, gint16 i2, gint64 i3, float i4, double i5, const std::string& strText,
    gint32& o1, gint16& o2, gint64& o3, float& o4, double& o5, std::string& strReply )
{
    o1 = i1 + 1;
    o2 = i2 + 1;
    o3 = i3 + 1;
    o4 = i4 + 1;
    o5 = i5 + 1;
    strReply = strText + " 2";

    return 0;
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
