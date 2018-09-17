/*
 * =====================================================================================
 *
 *       Filename:  actcsvr.cpp
 *
 *    Description:  implementation of server capable of accepting and canceling
 *                  the active request sent from the client.
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
#include "actcsvr.h"
#include "ifhelper.h"
#include "frmwrk.h"

using namespace std;

CActcServer::CActcServer( const IConfigDb* pCfg )
    : super( pCfg )
{
    SetClassId( clsid( CActcServer ) );
}

gint32 CActcServer::InitUserFuncs()
{
    // don't forget to call the super init user funcs
    gint32 ret = super::InitUserFuncs();
    if( ERROR( ret ) )
        return ret;

    BEGIN_HANDLER_MAP;

    ADD_USER_SERVICE_HANDLER(
        CActcServer::LongWait,
        METHOD_LongWait );

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
*   but the position can change in the argument list.
*
* @strText:
*   the text to echo
* @} */

gint32 CActcServer::OnLongWaitComplete(
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

    // we don't need to care about the timer resource
    // because it is already released
    return ret;
}

gint32 CActcServer::LongWait(
    IEventSink* pCallback,
    const std::string& strText )
{
    gint32 ret = 0;

    TaskletPtr pTask;

    // new a callback task and bind it with the
    // callback method we want to call when this call
    // finally complete
    //
    // NOTE: it would be better to use CAsyncCallback
    // if you are about to schedule a task to do the
    // business logic, for example, via RunIoRequest.
    // The CAsyncCallback carries, because the callback
    // has the context pointer and the same response
    // value in its parameter list as to send back to
    // the client
    ret = DEFER_CALL_NOSCHED( pTask, ObjPtr( this ),
        &CActcServer::OnLongWaitComplete,
        pCallback, strText );

    if( ERROR( ret ) )
        return ret;

    // schedule a timer as the async approach for this
    // example
    CUtilities& oUtils =
        GetIoMgr()->GetUtils();

    CTimerService& oTimerSvc =
        oUtils.GetTimerSvc();

    // schedule a timer, when the timer is due, the
    // OnLongWaitComplete will be called
    gint32 iTimer = oTimerSvc.AddTimer(
        3, pTask, 0 );

    if( iTimer < 0 )
        return iTimer;

    // we need to handle the canceling operation
    TaskletPtr pCancelTask;

    // bind a cancel callback to release the timer
    // otherwise, the resouce will piled up in the
    // CTimerService object, together with the deferred
    // call above
    //
    // DEFER_CALL_ONESHOT will always be called
    // wheather LongWait succeeds, failed or canceled,
    // by intercepting before the CIfInvokeMethodTask
    // is about to complete, unlike the
    // OnLongWaitComplete, which will only be called
    // along the branch the business logic get
    // returned.
    // 
    // So to determine whether or not the task is
    // canceled, you need to retrieve the
    // propReturnValue property from the pCallback 
    //
    // please see the OnLongWaitCanceled for detail
    // 
    DEFER_CALL_ONESHOT( pCancelTask,
        pCallback, this,
        &CActcServer::OnLongWaitCanceled,
        pCallback, iTimer );    

    // notify the caller, we have not completed yet
    return STATUS_PENDING;
}

// this callback will be called whether the task
// succeeded or canceled.  and it will release the
// timer resource properly
gint32 CActcServer::OnLongWaitCanceled(
    IEventSink* pCallback,
    gint32 iTimerId )
{
    CCfgOpenerObj oCfg( pCallback );
    ObjPtr pObj;

    gint32 ret = oCfg.GetObjPtr(
        propRespPtr, pObj ); 

    if( SUCCEEDED( ret ) )
    {
        CCfgOpener oResp( ( IConfigDb* )pObj );
        guint32 dwRet = oResp[ propReturnValue ];
        if( -ECANCELED == ( gint32 )dwRet )
        {
            DebugPrint( dwRet,
                "hello, the method is canceled" );
        }
    }
    
    CUtilities& oUtils =
        GetIoMgr()->GetUtils();

    CTimerService& oTimerSvc =
        oUtils.GetTimerSvc();

    oTimerSvc.RemoveTimer( iTimerId ); 

    return 0;
}

static FactoryPtr InitClassFactory()
{
    BEGIN_FACTORY_MAPS;

    INIT_MAP_ENTRYCFG( CActcServer );
    INIT_MAP_ENTRYCFG( CActcClient );

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
