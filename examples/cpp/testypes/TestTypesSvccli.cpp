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

extern sem_t  semPendings;
/* Async callback handler */
gint32 CTestTypesSvc_CliImpl::Echo3Callback( 
    IConfigDb* context, gint32 iRet,
    const std::string& strResp /*[ In ]*/ )
{
    do{
        CCfgOpener oCtx( context );
        guint32 *val0, *val1, *val2;
        // counters shared with the caller function
        oCtx.GetIntPtr( 0, val0 );
        oCtx.GetIntPtr( 1, val1 );
        oCtx.GetIntPtr( 2, val2 );
        auto& idx = *( ( std::atomic< int >* )val0 );
        auto& count = *( ( std::atomic< int >* )val1);
        auto& failures = *( ( std::atomic< int >* )val2);
        
        int iIdx = idx++;
        if( SUCCEEDED( iRet ) )
        {
            OutputMsg( 0,
                "Server resp( %d ): %s",
                iIdx, strResp.c_str() );
        }
        else if( ERROR( iRet ) )
        {
            failures++;
            OutputMsg( iRet,
                "Server resp( %d ): failure %d",
                iIdx, failures.load() );
        }
        Sem_Post( &semPendings );
        if( --count == 0 )
        {
            CSyncCallback* pSync = nullptr;
            gint32 ret = oCtx.GetPointer( 3, pSync );
            if( ERROR( ret ) )
                break;
            pSync->OnEvent(
                eventTaskComp, 0, 0, nullptr );
        }
    }while( 0 );
    return 0;
}
/* Async callback handler */
gint32 CTestTypesSvc_CliImpl::EchoByteArrayCallback( 
    IConfigDb* context, gint32 iRet,
    BufPtr& pRespBuf /*[ In ]*/ )
{
    do{
        CCfgOpener oCtx( context );
        guint32 *val0, *val1, *val2;
        // counters shared with the caller function
        oCtx.GetIntPtr( 0, val0 );
        oCtx.GetIntPtr( 1, val1 );
        oCtx.GetIntPtr( 2, val2 );
        auto& idx = *( ( std::atomic< int >* )val0 );
        auto& count = *( ( std::atomic< int >* )val1);
        auto& failures = *( ( std::atomic< int >* )val2);
        
        int iIdx = idx++;
        if( SUCCEEDED( iRet ) )
        {
            OutputMsg( 0,
                "Server resp( %d ): %s",
                iIdx, pRespBuf->ptr() );
        }
        else if( ERROR( iRet ) )
        {
            failures++;
            OutputMsg( iRet,
                "Server resp( %d ): failure %d",
                iIdx, failures.load() );
        }
        Sem_Post( &semPendings );
        if( --count == 0 )
        {
            CSyncCallback* pSync = nullptr;
            gint32 ret = oCtx.GetPointer( 3, pSync );
            if( ERROR( ret ) )
                break;
            pSync->OnEvent(
                eventTaskComp, 0, 0, nullptr );
        }
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
