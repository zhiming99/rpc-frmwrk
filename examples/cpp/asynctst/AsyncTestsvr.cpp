/****BACKUP YOUR CODE BEFORE RUNNING RIDLC***/
// ridlc -O . ../../asynctst.ridl 
// Implement the following methods
// to get the RPC proxy/server work
#include "rpc.h"
using namespace rpcf;
#include "asynctst.h"
#include "AsyncTestsvr.h"
#include "ifhelper.h"

gint32 CAsyncTest_SvrImpl::LongWaitCb(
    IEventSink*, IConfigDb* pReqCtx_,
    const std::string& i0 )
{
    // call the complete handler with the response
    // parameters and status code
    LongWaitComplete( pReqCtx_, 0, i0 );
    return 0;
}

// IAsyncTest Server
/* Async Req */
gint32 CAsyncTest_SvrImpl::LongWait(
    IConfigDb* pReqCtx_,
    const std::string& i0 /*[ In ]*/,
    std::string& i0r /*[ Out ]*/ )
{
    gint32 ret = 0;

    ADD_TIMER( this, pReqCtx_, 5, 
        &CAsyncTest_SvrImpl::LongWaitCb,
        i0 );

    if( ERROR( ret ) )
        return ret;

    return  STATUS_PENDING;
}

// RPC Async Req Cancel Handler
// Rewrite this method to release the resources
gint32 CAsyncTest_SvrImpl::OnLongWaitCanceled(
    IConfigDb* pReqCtx,
    gint32 iRet,
    const std::string& i0 /*[ In ]*/ )
{
    OutputMsg( iRet,
        "request 'LongWait' is canceled." );
    CParamList oReqCtx( pReqCtx );

    IEventSink* pTaskEvt = nullptr;
    gint32 ret = oReqCtx.GetPointer(
        propContext, pTaskEvt );

    if( ERROR( ret ) )
        return ret;

    // cancel the timer task
    TaskletPtr pTask = ObjPtr( pTaskEvt );
    ( *pTask )( eventCancelTask );
    return 0;
}

/* Sync Req */
gint32 CAsyncTest_SvrImpl::LongWaitNoParam()
{ return STATUS_SUCCESS; }

gint32 CAsyncTest_SvrImpl::LongWait2Cb(
    IConfigDb* pReqCtx,
    const std::string& i1 )
{
    if( pReqCtx == nullptr )
        return 0;

    OutputMsg( 0, "LongWait2 completed with %s",
        i1.c_str() );

    // call the predefined complete handler, with
    // the status code and response parameters
    LongWait2Complete( pReqCtx, 0, i1 );
    return 0;
}

/* Async Req */
gint32 CAsyncTest_SvrImpl::LongWait2(
    IConfigDb* pReqCtx_,
    const std::string& i1 /*[ In ]*/,
    std::string& i1r /*[ Out ]*/ )
{
    // schedule a defer call to complete the
    // request
    CIoManager* pMgr = GetIoMgr();
    gint32 ret = DEFER_CALL(
        pMgr, this,
        &CAsyncTest_SvrImpl::LongWait2Cb,
        pReqCtx_, i1 );

    if( ERROR( ret ) )
        return ret;

    return STATUS_PENDING;
}


