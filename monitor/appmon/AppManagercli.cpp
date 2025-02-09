/****BACKUP YOUR CODE BEFORE RUNNING RIDLC***/
// ../../ridl/.libs/ridlc -sO . ./appmon.ridl 
// Implement the following methods
// to get the RPC proxy/server work
#include "rpc.h"
using namespace rpcf;
#include "stmport.h"
#include "fastrpc.h"
#include "appmon.h"
#include "AppManagercli.h"

/* Event handler */
gint32 CAppManager_CliImpl::OnPointChanged( 
    const std::string& strPtPath /*[ In ]*/,
    const Variant& value /*[ In ]*/ )
{
    // TODO: Process the event here
    return STATUS_SUCCESS;
}
/* Event handler */
gint32 CAppManager_CliImpl::OnPointsChanged( 
    std::vector<KeyValue>& arrKVs /*[ In ]*/ )
{
    // TODO: Process the event here
    return STATUS_SUCCESS;
}
/* Async callback handler */
gint32 CAppManager_CliImpl::ClaimAppInstsCallback( 
    IConfigDb* context, gint32 iRet )
{
    // TODO: Process the server response here
    // return code ignored
    return 0;
}
/* Async callback handler */
gint32 CAppManager_CliImpl::FreeAppInstsCallback( 
    IConfigDb* context, gint32 iRet )
{
    // TODO: Process the server response here
    // return code ignored
    return 0;
}
gint32 CAppManager_CliImpl::CreateStmSkel(
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
            strDesc,"AppManager_SvrSkel",
            false, oCfg.GetCfg() );
        if( ERROR( ret ) )
            break;
        ret = pIf.NewObj(
            clsid( CAppManager_CliSkel ),
            oCfg.GetCfg() );
    }while( 0 );
    return ret;
}
gint32 CAppManager_CliImpl::OnPreStart(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCtx;
        CCfgOpenerObj oIfCfg( this );
        oCtx[ propClsid ] = clsid( 
            CAppManager_ChannelCli );
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
        strInstName = "AppManager_ChannelSvr";
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
