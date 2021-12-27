/****BACKUP YOUR CODE BEFORE RUNNING RIDLC***/
// Implement the following methods
// to get the RPC proxy/server work
#include "rpc.h"
using namespace rpcf;
#include "actcancel.h"
#include "ActiveCancelsvr.h"

gint32 CActiveCancel_SvrImpl::LongWaitCb(
    IEventSink*, IConfigDb* pReqCtx_,
    const std::string& i0 )
{
    // call the complete handler with the response
    // parameters and status code
    LongWaitComplete( pReqCtx_, 0, i0 );
    return 0;
}

// IActiveCancel Server
/* Async Req */
gint32 CActiveCancel_SvrImpl::LongWait(
    IConfigDb* pReqCtx_,
    const std::string& i0 /*[ In ]*/,
    std::string& i0r /*[ Out ]*/ )
{
    OutputMsg( 0, "i0 is %s", i0.c_str() );

    gint32 ret = 0;

    ADD_TIMER( this, pReqCtx_, 30,
        &CActiveCancel_SvrImpl::LongWaitCb,
        i0 );

    if( ERROR( ret ) )
        return ret;

    return STATUS_PENDING;
}


// RPC Async Req Cancel Handler
// Rewrite this method to release the resources
gint32 CActiveCancel_SvrImpl::OnLongWaitCanceled(
    IConfigDb* pReqCtx,
    gint32 iRet,
    const std::string& i0 /*[ In ]*/ )
{
    OutputMsg( iRet,
        "request 'LongWait' is canceled." );
    CParamList oReqCtx( pReqCtx );

    // retrieve the timer object
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
