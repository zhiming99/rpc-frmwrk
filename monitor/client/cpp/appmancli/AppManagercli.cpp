/****BACKUP YOUR CODE BEFORE RUNNING RIDLC***/
// ../../../../ridl/.libs/ridlc --sync_mode IAppStore=async_p --services=AppManager,SimpleAuth --client -slO . ../../../../monitor/appmon/appmon.ridl 
// Implement the following methods
// to get the RPC proxy/server work
#include "rpc.h"
using namespace rpcf;
#include "stmport.h"
#include "fastrpc.h"
#include "appmon.h"
#include "AppManagercli.h"

InterfPtr g_pAppManCli;
stdrmutex g_oAMLock;
gint32 GetAppManagercli(
    CIoManager* pMgr, InterfPtr& pCli )
{
    gint32 ret = 0;
    do{
        CStdRMutex oLock( g_oAMLock );
        InterfPtr pIf = g_pAppManCli; 
        if( pIf.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }
        pCli = pIf;
    }while( 0 );
    return ret;
}

gint32 CreateAppManagercli( CIoManager* pMgr,
    IEventSink* pCallback, IConfigDb* pCfg )
{
    gint32 ret = 0;
    do{
        gint32 (*func)( IEventSink*, IEventSink* ) =
        ([]( IEventSink* pTask, IEventSink* pCb )->gint32
        {
            gint32 ret = 0;
            ObjPtr ptrResp;
            do{
                IConfigDb* pResp = nullptr;
                CCfgOpenerObj oReq( pTask );
                ret = oReq.GetObjPtr(
                    propRespPtr, ptrResp );
                if( ERROR( ret ) )
                    break;
                pResp = ptrResp;
                CCfgOpener oResp( pResp );
                gint32 iRet = 0;
                ret = oResp.GetIntProp(
                    propReturnValue,
                    ( guint32& ) iRet );
                if( ERROR( ret ) )
                    break;
                if( ERROR( iRet ) )
                {
                    ret = iRet;
                    break;
                }

                ObjPtr pIf;
                ret = oResp.GetObjPtr( 0, pIf );
                if( SUCCEEDED( ret ) )
                {
                    CStdRMutex oLock( g_oAMLock );
                    g_pAppManCli = pIf;
                }
            }while( 0 );
            if( ptrResp.IsEmpty() )
            {
                CCfgOpener oCfg;
                oCfg.SetIntProp( propReturnValue, ret );
                ptrResp = oCfg.GetCfg();
            }
            Variant oVar( ptrResp );
            pCb->SetProperty( propRespPtr, oVar );
            pCb->OnEvent( eventTaskComp,
                0, 0, ( LONGWORD* )pTask );
            return ret;
        });

        TaskletPtr pStartCb;
        ret = NEW_COMPLETE_FUNCALL( 0, pStartCb,
            pMgr, func, nullptr, pCallback );
        if( ERROR( ret ) )
            break;

        ret = AsyncCreateIf <CAppManager_CliImpl,
            clsid( CAppManager_CliImpl )>(
            pMgr, pStartCb, pCfg,
            "./appmondesc.json", "AppManager",
            false );

        if( ERROR( ret ) )
            ( *pStartCb )( eventCancelTask );

    }while( 0 );
    return ret;
}

gint32 DestroyAppManagercli(
    CIoManager* pMgr,
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        InterfPtr pCli;
        ret = GetAppManagercli( pMgr, pCli );
        if( ERROR( ret ) )
            break;

        if( pCli->GetState() == stateStopping ||
            pCli->GetState() == stateStopped )
            break;

        if( pCallback == nullptr )
        {
            ret = pCli->Stop();
            break;
        }
        gint32 (*func)( IEventSink* pCb ) =
        ([]( IEventSink* pCb )->gint32
        {
            if( pCb != nullptr )
            {
                pCb->OnEvent( eventTaskComp,
                    0, 0, nullptr );
            }
            CStdRMutex oLock( g_oAMLock );
            g_pAppManCli.Clear();
            return 0;
        });
        TaskletPtr pStopTask;
        ret = NEW_FUNCCALL_TASKEX(
            pStopTask, pMgr, func, pCallback );
        if( ERROR( ret ) )
            break;
        CRpcServices* pSvc = pCli;
        ret = pSvc->QueueStopTask( pCli, pStopTask );
        if( ERROR( ret ) )
        {
            ( *pStopTask )( eventCancelTask );
            OutputMsg( ret,
                "Error stop CSimpleAuth_CliImpl" );
        }
    }while( 0 );
    return ret;
}

/* Async callback handler */
gint32 CAppManager_CliImpl::ListAppsCallback( 
    IConfigDb* context, gint32 iRet,
    std::vector<std::string>& arrApps /*[ In ]*/ )
{
    PACBS pCbs;
    gint32 ret = GetAsyncCallbacks( pCbs );
    if( ERROR( ret ) )
        return ret;
    return pCbs->ListAppsCallback(
        context, iRet, arrApps );
}
/* Async callback handler */
gint32 CAppManager_CliImpl::ListPointsCallback( 
    IConfigDb* context, gint32 iRet,
    std::vector<std::string>& arrPoints /*[ In ]*/ )
{
    PACBS pCbs;
    gint32 ret = GetAsyncCallbacks( pCbs );
    if( ERROR( ret ) )
        return ret;
    return pCbs->ListPointsCallback(
        context, iRet, arrPoints );
}
/* Async callback handler */
gint32 CAppManager_CliImpl::ListAttributesCallback( 
    IConfigDb* context, gint32 iRet,
    std::vector<std::string>& arrAttributes /*[ In ]*/ )
{
    PACBS pCbs;
    gint32 ret = GetAsyncCallbacks( pCbs );
    if( ERROR( ret ) )
        return ret;
    return pCbs->ListAttributesCallback(
        context, iRet, arrAttributes ); 
}
/* Async callback handler */
gint32 CAppManager_CliImpl::SetPointValueCallback( 
    IConfigDb* context, gint32 iRet )
{
    PACBS pCbs;
    gint32 ret = GetAsyncCallbacks( pCbs );
    if( ERROR( ret ) )
        return ret;
    return pCbs->SetPointValueCallback( context, iRet );
}
/* Async callback handler */
gint32 CAppManager_CliImpl::GetPointValueCallback( 
    IConfigDb* context, gint32 iRet,
    const Variant& rvalue /*[ In ]*/ )
{
    PACBS pCbs;
    gint32 ret = GetAsyncCallbacks( pCbs );
    if( ERROR( ret ) )
        return ret;
    return pCbs->GetPointValueCallback( context, iRet, rvalue );
}
/* Async callback handler */
gint32 CAppManager_CliImpl::SetLargePointValueCallback( 
    IConfigDb* context, gint32 iRet )
{
    PACBS pCbs;
    gint32 ret = GetAsyncCallbacks( pCbs );
    if( ERROR( ret ) )
        return ret;
    return pCbs->SetLargePointValueCallback( context, iRet );
}
/* Async callback handler */
gint32 CAppManager_CliImpl::GetLargePointValueCallback( 
    IConfigDb* context, gint32 iRet,
    BufPtr& value /*[ In ]*/ )
{
    PACBS pCbs;
    gint32 ret = GetAsyncCallbacks( pCbs );
    if( ERROR( ret ) )
        return ret;
    return pCbs->GetLargePointValueCallback(
        context, iRet, value );
}
/* Async callback handler */
gint32 CAppManager_CliImpl::SubscribeStreamPointCallback( 
    IConfigDb* context, gint32 iRet )
{
    PACBS pCbs;
    gint32 ret = GetAsyncCallbacks( pCbs );
    if( ERROR( ret ) )
        return ret;
    return pCbs->SubscribeStreamPointCallback(
        context, iRet );
}
/* Async callback handler */
gint32 CAppManager_CliImpl::SetAttrValueCallback( 
    IConfigDb* context, gint32 iRet )
{
    PACBS pCbs;
    gint32 ret = GetAsyncCallbacks( pCbs );
    if( ERROR( ret ) )
        return ret;
    return pCbs->SetAttrValueCallback( context, iRet );
}
/* Async callback handler */
gint32 CAppManager_CliImpl::GetAttrValueCallback( 
    IConfigDb* context, gint32 iRet,
    const Variant& rvalue /*[ In ]*/ )
{
    PACBS pCbs;
    gint32 ret = GetAsyncCallbacks( pCbs );
    if( ERROR( ret ) )
        return ret;
    return pCbs->GetAttrValueCallback(
        context, iRet, rvalue );
}
/* Async callback handler */
gint32 CAppManager_CliImpl::SetPointValuesCallback( 
    IConfigDb* context, gint32 iRet )
{
    CfgPtr pContext;
    PACBS pCbs;
    gint32 ret = GetAsyncCallbacks( pCbs );
    if( ERROR( ret ) )
        return ret;
    return pCbs->SetPointValuesCallback( context, iRet );
}
/* Async callback handler */
gint32 CAppManager_CliImpl::GetPointValuesCallback( 
    IConfigDb* context, gint32 iRet,
    std::vector<KeyValue>& arrKeyVals /*[ In ]*/ )
{
    PACBS pCbs;
    gint32 ret = GetAsyncCallbacks( pCbs );
    if( ERROR( ret ) )
        return ret;
    return pCbs->GetPointValuesCallback(
        context, iRet, arrKeyVals );
}
/* Event handler */
gint32 CAppManager_CliImpl::OnPointChanged( 
    const std::string& strPtPath /*[ In ]*/,
    const Variant& value /*[ In ]*/ )
{
    CfgPtr pContext;
    PACBS pCbs;
    gint32 ret =
        GetAsyncCallbacks( pCbs, pContext );
    if( ERROR( ret ) )
        return ret;
    return pCbs->OnPointChanged(
        pContext, strPtPath, value );
}

/* Event handler */
gint32 CAppManager_CliImpl::OnPointsChanged( 
    std::vector<KeyValue>& arrKVs /*[ In ]*/ )
{
    CfgPtr pContext;
    PACBS pCbs;
    gint32 ret = GetAsyncCallbacks( pCbs, pContext );
    if( ERROR( ret ) )
        return ret;
    return pCbs->OnPointsChanged( pContext, arrKVs );
}

/* Async callback handler */
gint32 CAppManager_CliImpl::ClaimAppInstsCallback( 
    IConfigDb* context, gint32 iRet,
    std::vector<KeyValue>& arrInitKVs /*[ In ]*/ )
{
    PACBS pCbs;
    gint32 ret = GetAsyncCallbacks( pCbs );
    if( ERROR( ret ) )
        return ret;
    return pCbs->ClaimAppInstsCallback(
        context, iRet, arrInitKVs );
}

/* Async callback handler */
gint32 CAppManager_CliImpl::FreeAppInstsCallback( 
    IConfigDb* context, gint32 iRet )
{
    PACBS pCbs;
    gint32 ret = GetAsyncCallbacks( pCbs );
    if( ERROR( ret ) )
        return ret;
    return pCbs->FreeAppInstsCallback(
        context, iRet );
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

gint32 CAppManager_CliImpl::OnPostStop(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        ret = super::OnPostStop( pCallback );
        LOGWARN( this->GetIoMgr(), ret, 
            "Warning CAppManager_CliImpl stopped" );
        PACBS pCbs;
        CfgPtr pContext;
        gint32 ret = GetAsyncCallbacks( pCbs, pContext );
        if( ERROR( ret ) )
            break;
        pCbs->OnSvrOffline( pContext, this );
    }while( 0 );
    return ret;
}

