/****BACKUP YOUR CODE BEFORE RUNNING RIDLC***/
// ridlc -bsO . ../../testypes.ridl 
// Implement the following methods
// to get the RPC proxy/server work
#include "rpc.h"
using namespace rpcf;
#include "stmport.h"
#include "fastrpc.h"
#include "TestTypes.h"
#include "TestTypesSvccli.h"

/* Async callback handler */
gint32 CTestTypesSvc_CliImpl::Echo3Callback( 
    IConfigDb* context, gint32 iRet,
    const std::string& strResp /*[ In ]*/ )
{
    gint32 ret = 0;
    {
        // TODO: Process the server response here
        // return code ignored
        CCfgOpener oCtx( context );
        guint32 *val0, *val1;
        oCtx.GetIntPtr( 0, val0 );
        oCtx.GetIntPtr( 1, val1 );
        auto& idx = *( ( std::atomic< int >* )val0 );
        auto& count = *( ( std::atomic< int >* )val1);
        CSyncCallback* pCallback = nullptr;
        oCtx.GetPointer( 2, pCallback );
        
        idx++;
        OutputMsg( ret,
            "Server resp( %d ): %s",
            idx.load(), strResp.c_str() );
        count--;
        if( count == 0 && pCallback )
            pCallback->OnEvent(
                eventTaskComp, 0, 0, 0 );
    }while( 0 );
    return 0;
}
/* Event handler */
gint32 CTestTypesSvc_CliImpl::OnHelloWorld( 
    const std::string& strMsg /*[ In ]*/ )
{
    // TODO: Process the event here
    return STATUS_SUCCESS;
}
gint32 CTestTypesSvc_CliImpl::CreateStmSkel(
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
            strDesc,"TestTypesSvc_SvrSkel",
            false, oCfg.GetCfg() );
        if( ERROR( ret ) )
            break;
        ret = pIf.NewObj(
            clsid( CTestTypesSvc_CliSkel ),
            oCfg.GetCfg() );
    }while( 0 );
    return ret;
}
gint32 CTestTypesSvc_CliImpl::OnPreStart(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCtx;
        CCfgOpenerObj oIfCfg( this );
        oCtx[ propClsid ] = clsid( 
            CTestTypesSvc_ChannelCli );
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
        strInstName = "TestTypesSvc_ChannelSvr";
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
