/****BACKUP YOUR CODE BEFORE RUNNING RIDLC***/
// Implement the following methods
// to get the RPC proxy/server work
#include "rpc.h"
using namespace rpcf;
#include "stmport.h"
#include "fastrpc.h"
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
gint32 CAsyncTest_SvrImpl::CreateStmSkel(
    HANDLE hStream, guint32 dwPortId, InterfPtr& pIf )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg;
        auto pMgr = this->GetIoMgr();
        oCfg[ propIoMgr ] = ObjPtr( pMgr );
        oCfg[ propIsServer ] = true;
        oCfg.SetIntPtr( propStmHandle,
            ( guint32*& )hStream );
        oCfg.SetPointer( propParentPtr, this );
        ret = CRpcServices::LoadObjDesc(
            "./asynctstdesc.json",
            "AsyncTest_SvrSkel",
            true, oCfg.GetCfg() );
        if( ERROR( ret ) )
            break;
        oCfg.CopyProp( propSkelCtx, this );
        oCfg[ propPortId ] = dwPortId;
        ret = pIf.NewObj(
            clsid( CAsyncTest_SvrSkel ),
            oCfg.GetCfg() );
    }while( 0 );
    return ret;
}
gint32 CAsyncTest_SvrImpl::OnPreStart(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCtx;
        CCfgOpenerObj oIfCfg( this );
        oCtx[ propClsid ] = clsid( 
            CAsyncTest_ChannelSvr );
        oCtx.CopyProp( propObjDescPath, this );
        stdstr strInstName;
        ret = oIfCfg.GetStrProp(
            propObjName, strInstName );
        if( ERROR( ret ) )
            break;
        oCtx[ 1 ] = strInstName;
        guint32 dwHash = 0;
        ret = GenStrHash( strInstName, dwHash );
        if( ERROR( ret ) )
            break;
        char szBuf[ 16 ];
        sprintf( szBuf, "_%08X", dwHash );
        strInstName = "AsyncTest_ChannelSvr";
        oCtx[ 0 ] = strInstName;
        strInstName += szBuf;
        oCtx[ propObjInstName ] = strInstName;
        oCtx[ propIsServer ] = true;
        oIfCfg.SetPointer( propSkelCtx,
            ( IConfigDb* )oCtx.GetCfg() );
        ret = super::OnPreStart( pCallback );
    }while( 0 );
    return ret;
}
