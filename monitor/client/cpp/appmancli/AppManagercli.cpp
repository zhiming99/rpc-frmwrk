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
gint32 GetAppManCli(
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

gint32 CreateAppManCli( CIoManager* pMgr,
    IEventSink* pCallback, IConfigDb* pCfg )
{
    gint32 ret = 0;
    do{
        stdstr strDesc = "./appmondesc.json";
        InterfPtr pIf;
        CParamList oParams;
        if( pCfg != nullptr )
        {
            CCfgOpener oCfg( pCfg );
            CIoManager* pMgr2;
            ret = oCfg.GetPointer( propIoMgr, pMgr2 );
            if( ERROR( ret ) )
                oCfg.SetPointer( propIoMgr, pMgr );
        }
        else
        {
            oParams.SetPointer( propIoMgr, pMgr );
            pCfg = oParams.GetCfg();
        }
        CfgPtr ptrCfg( pCfg );
        ret = CRpcServices::LoadObjDesc( strDesc,
            "CAppManager", false, ptrCfg );
        if( ERROR( ret ) )
            break;
        ret = pIf.NewObj(
            clsid( CAppManager_CliImpl ),
            oParams.GetCfg() );
        if( ERROR( ret ) )
            break;

        gint32 (*pChecker)( IEventSink*,
            CAppManager_CliImpl*, IEventSink*,
            IConfigDb* ) = ([]( IEventSink* pTask,
            CAppManager_CliImpl* pIf,
            IEventSink* pCb, IConfigDb* pCfg )->gint32
        {
            gint32 ret = 0;
            do{
                CCfgOpenerObj oReq( pCb );
                IConfigDb* pResp = nullptr;
                ret = oReq.GetPointer(
                    propRespPtr, pResp );
                if( ERROR( ret ) )
                    break;
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
            }while( 0 );
            if( SUCCEEDED( ret ) )
            {
                InterfPtr pOld;
                CIoManager* pMgr = pIf->GetIoMgr();
                CStdRMutex oLock( g_oAMLock );
                g_pAppManCli = pIf;
            }
            if( pCb == nullptr )
                return ret;
            pCb->OnEvent( eventTaskComp,
                ret, 0, nullptr );

            return ret;
        });

        TaskletPtr pStartCb;
        ret = NEW_COMPLETE_FUNCALL( 0,
            pStartCb, pMgr, pChecker,
            nullptr, pIf, pCallback, pCfg );
        if( ERROR( ret ) )
            break;

        gint32 (*pStart)( IEventSink*,
            CAppManager_CliImpl*,
            IEventSink* ) = ([](
            IEventSink* pTask,
            CAppManager_CliImpl* pIf,
            IEventSink* pCallback )->gint32
        {
            gint32 ret = 0;
            do{
                ret = pIf->StartEx( pCallback );
                if( ERROR( ret ) )
                {
                    pIf->Stop();
                    break;
                }

                if( ret == STATUS_PENDING )
                    break;

                if( SUCCEEDED( ret ) )
                {
                    CStdRMutex oLock( g_oAMLock );
                    g_pAppManCli = pIf;
                }

                if( pCallback )
                {
                    ObjPtr pObj( pCallback );
                    CIfRetryTask* pRetry( pObj );
                    pRetry->ClearClientNotify();
                    pRetry->OnEvent( eventTaskComp,
                        ret, 0, nullptr );
                }
            }while( 0 );
            return ret;
        });

        TaskletPtr pStartTask;
        ret = NEW_FUNCCALL_TASKEX2( 0, 
            pStartTask, pMgr, pStart,
            nullptr, pIf,
            ( IEventSink* )pStartCb );
        if( ERROR( ret ) )
            break;

        CIfRetryTask* pRetry = pStartCb;
        pRetry->SetClientNotify( pStartTask );

        ret = pMgr->AddAndRun( pStartTask, false );
        if( ERROR( ret ) )
        {
            pRetry->ClearClientNotify();
            ( *pStartTask )( eventCancelTask );
            ( *pStartCb )( eventCancelTask );
        }

    }while( 0 );
    return ret;
}
/* Async callback handler */
gint32 CAppManager_CliImpl::ListAppsCallback( 
    IConfigDb* context, gint32 iRet,
    std::vector<std::string>& arrApps /*[ In ]*/ )
{
    CfgPtr pContext;
    IAsyncCallbacks* pCbs;
    gint32 ret = GetAsyncCallbacks( pCbs, pContext );
    if( ERROR( ret ) )
        return ret;
    return pCbs->ListAppsCallback(
        pContext, iRet, arrApps );
}
/* Async callback handler */
gint32 CAppManager_CliImpl::ListPointsCallback( 
    IConfigDb* context, gint32 iRet,
    std::vector<std::string>& arrPoints /*[ In ]*/ )
{
    CfgPtr pContext;
    IAsyncCallbacks* pCbs;
    gint32 ret = GetAsyncCallbacks( pCbs, pContext );
    if( ERROR( ret ) )
        return ret;
    return 0;
}
/* Async callback handler */
gint32 CAppManager_CliImpl::ListAttributesCallback( 
    IConfigDb* context, gint32 iRet,
    std::vector<std::string>& arrAttributes /*[ In ]*/ )
{
    CfgPtr pContext;
    IAsyncCallbacks* pCbs;
    gint32 ret = GetAsyncCallbacks( pCbs, pContext );
    if( ERROR( ret ) )
        return ret;
    return pCbs->ListAttributesCallback(
        pContext, iRet, arrAttributes ); 
}
/* Async callback handler */
gint32 CAppManager_CliImpl::SetPointValueCallback( 
    IConfigDb* context, gint32 iRet )
{
    CfgPtr pContext;
    IAsyncCallbacks* pCbs;
    gint32 ret = GetAsyncCallbacks( pCbs, pContext );
    if( ERROR( ret ) )
        return ret;
    return pCbs->SetPointValueCallback( pContext, iRet );
}
/* Async callback handler */
gint32 CAppManager_CliImpl::GetPointValueCallback( 
    IConfigDb* context, gint32 iRet,
    const Variant& rvalue /*[ In ]*/ )
{
    CfgPtr pContext;
    IAsyncCallbacks* pCbs;
    gint32 ret = GetAsyncCallbacks( pCbs, pContext );
    if( ERROR( ret ) )
        return ret;
    return pCbs->GetPointValueCallback( pContext, iRet, rvalue );
}
/* Async callback handler */
gint32 CAppManager_CliImpl::SetLargePointValueCallback( 
    IConfigDb* context, gint32 iRet )
{
    CfgPtr pContext;
    IAsyncCallbacks* pCbs;
    gint32 ret = GetAsyncCallbacks( pCbs, pContext );
    if( ERROR( ret ) )
        return ret;
    return pCbs->SetLargePointValueCallback( pContext, iRet );
}
/* Async callback handler */
gint32 CAppManager_CliImpl::GetLargePointValueCallback( 
    IConfigDb* context, gint32 iRet,
    BufPtr& value /*[ In ]*/ )
{
    CfgPtr pContext;
    IAsyncCallbacks* pCbs;
    gint32 ret = GetAsyncCallbacks( pCbs, pContext );
    if( ERROR( ret ) )
        return ret;
    return pCbs->GetLargePointValueCallback( pContext, iRet, value );
}
/* Async callback handler */
gint32 CAppManager_CliImpl::SubscribeStreamPointCallback( 
    IConfigDb* context, gint32 iRet )
{
    CfgPtr pContext;
    IAsyncCallbacks* pCbs;
    gint32 ret = GetAsyncCallbacks( pCbs, pContext );
    if( ERROR( ret ) )
        return ret;
    return pCbs->SubscribeStreamPointCallback( pContext, iRet );
}
/* Async callback handler */
gint32 CAppManager_CliImpl::SetAttrValueCallback( 
    IConfigDb* context, gint32 iRet )
{
    CfgPtr pContext;
    IAsyncCallbacks* pCbs;
    gint32 ret = GetAsyncCallbacks( pCbs, pContext );
    if( ERROR( ret ) )
        return ret;
    return pCbs->SetAttrValueCallback( pContext, iRet );
}
/* Async callback handler */
gint32 CAppManager_CliImpl::GetAttrValueCallback( 
    IConfigDb* context, gint32 iRet,
    const Variant& rvalue /*[ In ]*/ )
{
    CfgPtr pContext;
    IAsyncCallbacks* pCbs;
    gint32 ret = GetAsyncCallbacks( pCbs, pContext );
    if( ERROR( ret ) )
        return ret;
    return pCbs->GetAttrValueCallback(
        pContext, iRet, rvalue );
}
/* Async callback handler */
gint32 CAppManager_CliImpl::SetPointValuesCallback( 
    IConfigDb* context, gint32 iRet )
{
    CfgPtr pContext;
    IAsyncCallbacks* pCbs;
    gint32 ret = GetAsyncCallbacks( pCbs, pContext );
    if( ERROR( ret ) )
        return ret;
    return pCbs->SetPointValuesCallback( pContext, iRet );
}
/* Async callback handler */
gint32 CAppManager_CliImpl::GetPointValuesCallback( 
    IConfigDb* context, gint32 iRet,
    std::vector<KeyValue>& arrKeyVals /*[ In ]*/ )
{
    CfgPtr pContext;
    IAsyncCallbacks* pCbs;
    gint32 ret = GetAsyncCallbacks( pCbs, pContext );
    if( ERROR( ret ) )
        return ret;
    return pCbs->GetPointValuesCallback(
        pContext, iRet, arrKeyVals );
}
/* Event handler */
gint32 CAppManager_CliImpl::OnPointChanged( 
    const std::string& strPtPath /*[ In ]*/,
    const Variant& value /*[ In ]*/ )
{
    CfgPtr pContext;
    IAsyncCallbacks* pCbs;
    gint32 ret = GetAsyncCallbacks( pCbs, pContext );
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
    IAsyncCallbacks* pCbs;
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
    CfgPtr pContext;
    IAsyncCallbacks* pCbs;
    gint32 ret = GetAsyncCallbacks( pCbs, pContext );
    if( ERROR( ret ) )
        return ret;
    return pCbs->ClaimAppInstsCallback(
        pContext, iRet, arrInitKVs );
}

/* Async callback handler */
gint32 CAppManager_CliImpl::FreeAppInstsCallback( 
    IConfigDb* context, gint32 iRet )
{
    CfgPtr pContext;
    IAsyncCallbacks* pCbs;
    gint32 ret = GetAsyncCallbacks( pCbs, pContext );
    if( ERROR( ret ) )
        return ret;
    return pCbs->FreeAppInstsCallback(
        pContext, iRet );
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
        CfgPtr pContext;
        IAsyncCallbacks* pCbs;
        gint32 ret = GetAsyncCallbacks( pCbs, pContext );
        if( ERROR( ret ) )
            break;
        pCbs->OnSvrOffline( pContext );
    }while( 0 );
    return ret;
}

