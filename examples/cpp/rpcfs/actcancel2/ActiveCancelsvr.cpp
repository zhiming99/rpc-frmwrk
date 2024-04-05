/****BACKUP YOUR CODE BEFORE RUNNING RIDLC***/
// Implement the following methods
// to get the RPC proxy/server work
#include "rpc.h"
using namespace rpcf;
#include "stmport.h"
#include "fastrpc.h"
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

    // cancel the timer object
    TaskletPtr pTask = ObjPtr( pTaskEvt );
    ( *pTask )( eventCancelTask );
    return 0;
}

gint32 CActiveCancel_SvrImpl::CreateStmSkel(
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
        std::string strDesc;
        CCfgOpenerObj oIfCfg( this );
        ret = oIfCfg.GetStrProp(
            propObjDescPath, strDesc );
        if( ERROR( ret ) )
            break;
        ret = CRpcServices::LoadObjDesc(
            strDesc,"ActiveCancel_SvrSkel",
            true, oCfg.GetCfg() );
        if( ERROR( ret ) )
            break;
        oCfg.CopyProp( propSkelCtx, this );
        oCfg[ propPortId ] = dwPortId;
        ret = pIf.NewObj(
            clsid( CActiveCancel_SvrSkel ),
            oCfg.GetCfg() );
    }while( 0 );
    return ret;
}
gint32 CActiveCancel_SvrImpl::OnPreStart(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCtx;
        CCfgOpenerObj oIfCfg( this );
        oCtx[ propClsid ] = clsid( 
            CActiveCancel_ChannelSvr );
        oCtx.CopyProp( propObjDescPath, this );
        oCtx.CopyProp( propSvrInstName, this );
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
        strInstName = "ActiveCancel_ChannelSvr";
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
