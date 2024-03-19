/****BACKUP YOUR CODE BEFORE RUNNING RIDLC***/
// ridlc -s -O . ../../../asynctst.ridl 
// Implement the following methods
// to get the RPC proxy/server work
#include "rpc.h"
using namespace rpcf;
#include "stmport.h"
#include "fastrpc.h"
#include "asynctst.h"
#include "AsyncTestcli.h"

/* Async callback handler */
gint32 CAsyncTest_CliImpl::LongWaitCallback( 
    IConfigDb* context, gint32 iRet,
    const std::string& i0r /*[ In ]*/ )
{
    if( ERROR( iRet ) )
        OutputMsg( iRet,
            "LongWait returned with status" );
    else
        OutputMsg( iRet,
            "LongWait returned with response %s", i0r.c_str() );

    SetError( iRet );
    NotifyComplete();
    return 0;
}
/* Async callback handler */
gint32 CAsyncTest_CliImpl::LongWaitNoParamCallback( 
    IConfigDb* context, gint32 iRet )
{
    OutputMsg( iRet,
        "LongWaitNoParam returned with status" );
    SetError( iRet );
    NotifyComplete();
    return 0;
}
gint32 CAsyncTest_CliImpl::CreateStmSkel(
    InterfPtr& pIf )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg;
        auto pMgr = this->GetIoMgr();
        oCfg[ propIoMgr ] = ObjPtr( pMgr );
        oCfg[ propIsServer ] = false;
        oCfg.SetPointer( propParentPtr, this );
        oCfg.CopyProp( propSkelCtx, this );
        std::string strDesc;
        CCfgOpenerObj oIfCfg( this );
        ret = oIfCfg.GetStrProp(
            propObjDescPath, strDesc );
        if( ERROR( ret ) )
            break;
        ret = CRpcServices::LoadObjDesc(
            strDesc,"AsyncTest_SvrSkel",
            false, oCfg.GetCfg() );
        if( ERROR( ret ) )
            break;
        ret = pIf.NewObj(
            clsid( CAsyncTest_CliSkel ),
            oCfg.GetCfg() );
    }while( 0 );
    return ret;
}
gint32 CAsyncTest_CliImpl::OnPreStart(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCtx;
        CCfgOpenerObj oIfCfg( this );
        oCtx[ propClsid ] = clsid( 
            CAsyncTest_ChannelCli );
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
        strInstName = "AsyncTest_ChannelSvr";
        oCtx[ 0 ] = strInstName;
        strInstName += szBuf;
        oCtx[ propObjInstName ] = strInstName;
        oCtx[ propIsServer ] = false;
        oIfCfg.SetPointer( propSkelCtx,
            ( IConfigDb* )oCtx.GetCfg() );
        ret = super::OnPreStart( pCallback );
    }while( 0 );
    return ret;
}
