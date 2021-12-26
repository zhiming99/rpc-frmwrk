/****BACKUP YOUR CODE BEFORE RUNNING RIDLC***/
// Implement the following methods
// to get the RPC proxy/server work
#include "rpc.h"
using namespace rpcf;
#include "asynctst.h"
#include "AsyncTestsvr.h"

gint32 CAsyncTest_SvrImpl::LongWaitCb(
    IEventSink* pCallback,
    IConfigDb* pReqCtx_,
    const std::string& i0 )
{
    CParamList oReqCtx( pReqCtx_);
    // call the complete handler with the response
    // parameters and status code
    LongWaitComplete( pReqCtx_, 0, i0 );
    pReqCtx_->RemoveAll();
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
    TaskletPtr pTask;
    do{
        CParamList oReqCtx( pReqCtx_ );
        // schedule a timer to complete the request
        ret = DEFER_IFCALLEX_NOSCHED2(
            0, pTask, ObjPtr( this ),
            &CAsyncTest_SvrImpl::LongWaitCb,
            nullptr, pReqCtx_, i0 );

        if( ERROR( ret ) )
        {
            ( *pTask )( eventCancelTask );
            return ret;
        }

        ObjPtr pTaskObj = pTask;
        oReqCtx.Push( pTaskObj );
        CIfDeferCallTaskEx* pTaskEx = pTask;
        ret = pTaskEx->EnableTimer(
            3, eventRetry );
        if( ERROR( ret ) )
            break;

        ret = STATUS_PENDING;
    }while( 0 );
    return ret;
}

// RPC Async Req Cancel Handler
// Rewrite this method to release the resources
gint32 CAsyncTest_SvrImpl::OnLongWaitCanceled(
    IConfigDb* pReqCtx,
    gint32 iRet,
    const std::string& i0 /*[ In ]*/ )
{
    DebugPrintEx( logErr, iRet,
        "request 'LongWait' is canceled." );
    CParamList oReqCtx( pReqCtx );

    // clear the timer task
    IEventSink* pTaskEvt = nullptr;
    gint32 ret = oReqCtx.GetPointer(
        0, pTaskEvt );

    if( ERROR( ret ) )
        return ret;

    TaskletPtr pTask = ObjPtr( pTaskEvt );
    ( *pTask )( eventCancelTask );
    pReqCtx->RemoveAll();
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


