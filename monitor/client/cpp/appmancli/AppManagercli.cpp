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
#include "monconst.h"
#include "signal.h"
#include <limits.h>

#define RECONNECT_INTERVAL 10

using IFCBSCTX=std::tuple< InterfPtr, PACBS, CfgPtr >;
struct CAppManContext
{
    mutable stdrmutex m_oAMLock;
    InterfPtr m_pAppManCli;
    PACBS   m_pacbs;
    CfgPtr  m_pCtx;

    gint32 GetAppManagercli( InterfPtr& pCli ) const
    {
        gint32 ret = 0;
        do{
            CStdRMutex oLock( m_oAMLock );
            InterfPtr pIf = m_pAppManCli; 
            if( pIf.IsEmpty() )
            {
                ret = -EFAULT;
                break;
            }
            pCli = pIf;
        }while( 0 );
        return ret;
    }

    void SetAppManagercli( InterfPtr& pCli )
    {
        CStdRMutex oLock( m_oAMLock );
        m_pAppManCli = pCli;
    }

    void ClearAppManagerCli()
    {
        CStdRMutex oLock( m_oAMLock );
        m_pAppManCli.Clear();
    }

    void SetAsyncCbs( PACBS& pacbs, CfgPtr& pCtx )
    {
        m_pacbs = pacbs;
        m_pCtx = pCtx;
    }

    gint32 GetAsyncCbs(
        PACBS& pacbs, CfgPtr& pCtx ) const
    {
        if( !m_pacbs )
            return -EFAULT;
        pacbs = m_pacbs;
        pCtx = m_pCtx;
        return 0;
    }

    gint32 GetAsyncCbs( PACBS& pacbs )
    {
        if( !m_pacbs )
            return -EFAULT;
        pacbs = m_pacbs;
        return 0;
    }

    void Clear()
    {
        CStdRMutex oLock( m_oAMLock );
        m_pAppManCli.Clear();
        m_pacbs.reset();
        m_pCtx.Clear();
    }
};

CAppManContext g_oAmctx;

gint32 GetAppManagercli( InterfPtr& pCli )
{ return g_oAmctx.GetAppManagercli( pCli ); }

gint32 CreateAppManagercli(
    CIoManager* pMgr,
    IEventSink* pCallback,
    IConfigDb* pCfg,
    InterfPtr& pAppMan )
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
                ret, 0, ( LONGWORD* )pTask );
            return ret;
        });

        TaskletPtr pStartCb;
        ret = NEW_COMPLETE_FUNCALL( 0, pStartCb,
            pMgr, func, nullptr, pCallback );
        if( ERROR( ret ) )
            break;

        InterfPtr pNewIf;
        ret = AsyncCreateIf<CAppManager_CliImpl>(
            pMgr, pStartCb, pCfg,
            clsid( CAppManager_CliImpl ),
            "invalidpath/appmondesc.json",
            "AppManager", pNewIf, false );
        pAppMan = pNewIf;
        if( ERROR( ret ) )
            ( *pStartCb )( eventCancelTask );
        else
        {
            DebugPrint( 0, "AppManagerCli created @0x%llx",
                ( guint64 )(CAppManager_CliImpl*)pNewIf );
            g_oAmctx.SetAppManagercli( pNewIf );
        }

    }while( 0 );
    return ret;
}

gint32 DestroyAppManagercli(
    CIoManager* pMgr, IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        InterfPtr pCli;
        ret = GetAppManagercli( pCli );
        if( ERROR( ret ) )
            break;

        {
            CRpcServices* pSvc = pCli;
            CStdRMutex oIfLock( pSvc->GetLock() );
            EnumIfState iState = pSvc->GetState();
            if( iState == stateStopped )
                break;
        }

        if( pCallback == nullptr )
        {
            ret = pCli->Stop();
            break;
        }
        if( pCli->GetState() != stateStopping )
        {
            gint32 (*func)( IEventSink*,
                IEventSink* ) =
            ([]( IEventSink* pTask,
                IEventSink* pCb )->gint32
            {
                if( pCb != nullptr )
                {
                    pCb->OnEvent( eventTaskComp,
                        0, 0, nullptr );
                }
                g_oAmctx.ClearAppManagerCli();;
                return 0;
            });
            TaskletPtr pStopTask;
            ret = NEW_COMPLETE_FUNCALL( 0, pStopTask,
                pMgr, func, nullptr, pCallback );
            if( ERROR( ret ) )
                break;
            CRpcServices* pSvc = pCli;
            ret = pSvc->QueueStopTask( pCli, pStopTask );
            if( ERROR( ret ) )
            {
                ( *pStopTask )( eventCancelTask );
                OutputMsg( ret,
                    "Error stop CAppManager_CliImpl" );
            }
        }
        else
        {
            // wait it complete
            gint32 (*func)( IEventSink* )
            ([]( IEventSink* pCb )->gint32
            {
                pCb && pCb->OnEvent( eventTaskComp,
                        0, 0, nullptr );
                g_oAmctx.ClearAppManagerCli();
                return 0;
            });
            TaskletPtr pWaitTask;
            ret = NEW_FUNCCALL_TASKEX( pWaitTask,
                pMgr, func, pCallback );
            if( ERROR( ret ) )
                break;
            CRpcServices* pSvc = pCli;
            ret = pMgr->AddSeqTask( pWaitTask, false );
            if( ERROR( ret ) )
            {
                ( *pWaitTask )( eventCancelTask );
                OutputMsg( ret,
                    "Error stop CAppManager_CliImpl" );
            }
        }
        if( SUCCEEDED( ret ) )
            ret = STATUS_PENDING;
    }while( 0 );
    if( ret != STATUS_PENDING )
        g_oAmctx.ClearAppManagerCli();

    return ret;
}

/* Async callback handler */
gint32 CAppManager_CliImpl::ListAppsCallback( 
    IConfigDb* context, gint32 iRet,
    std::vector<std::string>& arrApps /*[ In ]*/ )
{
    PACBS pCbs;
    gint32 ret = g_oAmctx.GetAsyncCbs( pCbs );
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
    gint32 ret = g_oAmctx.GetAsyncCbs( pCbs );
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
    gint32 ret = g_oAmctx.GetAsyncCbs( pCbs );
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
    gint32 ret = g_oAmctx.GetAsyncCbs( pCbs );
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
    gint32 ret = g_oAmctx.GetAsyncCbs( pCbs );
    if( ERROR( ret ) )
        return ret;
    return pCbs->GetPointValueCallback( context, iRet, rvalue );
}
/* Async callback handler */
gint32 CAppManager_CliImpl::SetLargePointValueCallback( 
    IConfigDb* context, gint32 iRet )
{
    PACBS pCbs;
    gint32 ret = g_oAmctx.GetAsyncCbs( pCbs );
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
    gint32 ret = g_oAmctx.GetAsyncCbs( pCbs );
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
    gint32 ret = g_oAmctx.GetAsyncCbs( pCbs );
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
    gint32 ret = g_oAmctx.GetAsyncCbs( pCbs );
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
    gint32 ret = g_oAmctx.GetAsyncCbs( pCbs );
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
    gint32 ret = g_oAmctx.GetAsyncCbs( pCbs );
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
    gint32 ret = g_oAmctx.GetAsyncCbs( pCbs );
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
        g_oAmctx.GetAsyncCbs( pCbs, pContext );
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
    gint32 ret = g_oAmctx.GetAsyncCbs( pCbs, pContext );
    if( ERROR( ret ) )
        return ret;
    return pCbs->OnPointsChanged( pContext, arrKVs );
}

/* Async callback handler */
gint32 CAppManager_CliImpl::ClaimAppInstCallback( 
    IConfigDb* context, gint32 iRet,
    std::vector<KeyValue>& arrPtToGet /*[ In ]*/ )
{
    PACBS pCbs;
    gint32 ret = g_oAmctx.GetAsyncCbs( pCbs );
    if( ERROR( ret ) )
        return ret;
    return pCbs->ClaimAppInstCallback(
        context, iRet, arrPtToGet );
}

/* Async callback handler */
gint32 CAppManager_CliImpl::FreeAppInstsCallback( 
    IConfigDb* context, gint32 iRet )
{
    PACBS pCbs;
    gint32 ret = g_oAmctx.GetAsyncCbs( pCbs );
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
        gint32 ret = g_oAmctx.GetAsyncCbs(
            pCbs, pContext );
        if( ERROR( ret ) )
            break;
        pCbs->OnSvrOffline( pContext, this );
    }while( 0 );
    return ret;
}

gint32 CreateStdAppManSync(
    CIoManager* pMgr,
    IConfigDb* pCfg,
    InterfPtr& pAppMan )
{
    gint32 ret = 0;
    do{
        TaskletPtr pTask;
        ret = pTask.NewObj(
            clsid( CSyncCallback ) );
        if( ERROR( ret ) )
            break;
        CSyncCallback* pSync = pTask;
        ret = CreateAppManagercli(
            pMgr, pTask, pCfg, pAppMan );
        if( ret == STATUS_PENDING )
        {
            ret = pSync->WaitForComplete();
            if( ERROR( ret ) )
                break;
            ret = pSync->GetError();
        }
        if( SUCCEEDED( ret ) )
        {
            Variant oVar;
            ret = pSync->GetProperty(
                propRespPtr, oVar );
            if( ERROR( ret ) )
                break;
            IConfigDb* pCfg = ( ObjPtr& )oVar;
            ret = pCfg->GetProperty( 0, oVar );
            if( ERROR( ret ) )
                break;
            pAppMan = ( ObjPtr& )oVar;
        }
    }while( 0 );
    return ret;
}

gint32 ClaimApp( CRpcServices* pSvc,
    const std::string& strAppInst,
    InterfPtr& pAppMan,
    PACBS& pacbsIn )
{
    gint32 ret = 0;
    do{
        InterfPtr pIf;
        if( pSvc != nullptr )
            pIf = pSvc;

        PACBS pacbs;
        if( !pacbsIn )
        {
            ret = -EINVAL;
            break;
        }

        pacbs = pacbsIn;
        pacbs->SetInterface( pIf );

        CParamList oParams;
        oParams.SetPointer(
            PROP_TARGET_IF, pSvc );
        oParams.SetStrProp(
            PROP_APP_NAME, strAppInst );
        CAppManager_CliImpl* pamc = pAppMan;
        g_oAmctx.SetAsyncCbs( pacbs, oParams.GetCfg() );

        std::vector< KeyValue > veckv;
        ret = pacbs->GetPointValuesToUpdate(
            pIf, veckv);
        if( ERROR( ret ) )
        {
            DebugPrint( ret,
                "Error getting point values" );
            break;
        }

        pacbs->GetPointValuesToInit( pIf, veckv);

        std::vector< KeyValue > rveckv;
        ret = pamc->ClaimAppInst(
            oParams.GetCfg(),
            strAppInst, veckv, rveckv );
        if( ret == STATUS_PENDING )
            ret = 0;
    }while( 0 );
    return ret;
}

static gint32 ReconnectTimerFunc(   
    IConfigDb* pCtx, CIoManager* pMgr,
    CAppManager_CliImpl* pOldIf );

gint32 StartStdAppManCli(
    CRpcServices* pSvc,
    const std::string& strAppInst1,
    InterfPtr& pAppMan,
    PACBS& pacbsIn )
{
    gint32 ret = 0;
    // if( pSvc == nullptr )
    //     return -EINVAL;

    do{
        InterfPtr pAppMan;
        CIoManager* pMgr = pSvc->GetIoMgr();
        CParamList oParams;
        stdstr strAppInst = strAppInst1;
        Variant oVar;
        ret = pSvc->GetProperty(
            propMonAppInsts, oVar );
        if( SUCCEEDED( ret ) )
        {
            StrVecPtr pAppNames = oVar.m_pObj;
            if( ( *pAppNames )().size() )
                strAppInst = ( *pAppNames )().front();
        }
        if( strAppInst.empty() )
        {
            ret = -EINVAL;
            break;
        }

        oParams.SetStrProp(
            PROP_APP_NAME, strAppInst );
        ret = CreateStdAppManSync(
            pMgr, oParams.GetCfg(), pAppMan );
        if( ERROR( ret ) )
        {
            CAppManager_CliImpl* pamc = pAppMan;
            if( pamc == nullptr )
                break;
            pamc->Stop();
            break;
        }
        if( !pacbsIn )
            pacbsIn.reset( new CAsyncStdAMCallbacks );
        ret = ClaimApp( pSvc, strAppInst,
            pAppMan, pacbsIn );

    }while( 0 );
    return ret;
}

gint32 StopStdAppManCli()
{
    InterfPtr pIf;
    gint32 ret = GetAppManagercli( pIf );
    if( ERROR( ret ) )
    {
        g_oAmctx.Clear();
        return ret;
    }
    CAppManager_CliImpl* pamc = pIf;
    while( pamc->GetState() == stateConnected )
    {
        TaskletPtr pTask;
        pTask.NewObj( clsid( CSyncCallback ) );
        CParamList oParams;
        oParams.SetPointer( propEventSink,
            ( IEventSink* )pTask );

        PACBS pCbs;
        CfgPtr pContext;
        ret = g_oAmctx.GetAsyncCbs(
            pCbs, pContext );
        if( SUCCEEDED( ret ) )
        {
            std::vector< stdstr > vecApps;
            CCfgOpener oCtx(
                ( IConfigDb* )pContext );
            stdstr strApp;
            ret = oCtx.GetStrProp(
                PROP_APP_NAME, strApp );
            if( SUCCEEDED( ret ) )
                vecApps.push_back( strApp );
            if( vecApps.empty() )
                break;
            ret = pamc->FreeAppInsts(
                oParams.GetCfg(), vecApps );
            if( ret == STATUS_PENDING )
            {
                CSyncCallback* pSync = pTask;
                pSync->WaitForComplete();
                ret = pSync->GetError();
                if( ERROR( ret ) )
                    OutputMsg( ret,
                    "Error FreeAppInsts" );
            }
        }
        break;
    }

    TaskletPtr pTask;
    pTask.NewObj( clsid( CSyncCallback ) );
    CSyncCallback* pSync = pTask;
    ret = DestroyAppManagercli(
        pamc->GetIoMgr(), pSync );
    if( ret == STATUS_PENDING )
    {
        pSync->WaitForComplete();
        ret = pSync->GetError();
    }
    g_oAmctx.Clear();
    return ret;
}

//RPC event handler 'OnPointChanged'
gint32 CAsyncStdAMCallbacks::OnPointChanged(
    IConfigDb* context, 
    const std::string& strPtPath /*[ In ]*/,
    const Variant& value /*[ In ]*/ )
{
    gint32 ret = 0;
    do{
        PACBS pCbs;
        stdstr strApp;
        CfgPtr pContext;
        InterfPtr pIf;
        ret = GetAppManagercli( pIf );
        CAppManager_CliImpl* pamc = pIf;
        g_oAmctx.GetAsyncCbs( pCbs, pContext );
        CCfgOpener oCtx( ( IConfigDb* )pContext );
        ret = oCtx.GetStrProp(
            PROP_APP_NAME, strApp );
        if( ERROR( ret ) )
            break;
        CRpcServices* pTargetIf;
        ret = oCtx.GetPointer(
            PROP_TARGET_IF, pTargetIf );
        if( ERROR( ret ) )
            break;
        stdstr strExpPath = strApp + "/restart";
        if( strExpPath == strPtPath &&
            ( ( guint32& )value  == 1 ) )
        {
            kill( getpid(), SIGINT );
            break;
        }
        else if( strPtPath ==
            strApp + "/" S_MAX_QPS )
        {
            pTargetIf->SetProperty(
                propQps, value );
            break;
        }
        else if( strPtPath ==
            strApp + "/" S_MAX_STM_PER_SESS )
        {
            pTargetIf->SetProperty(
                propStmPerSess, value );
            break;
        }

        strExpPath = strApp + "/rpt_timer";
        if( strPtPath != strExpPath )
        {
            ret = -ENOTSUP;
            break;
        }
        if( ( ( guint32& )value ) != 1 )
            break;

        std::vector< KeyValue > veckv;
        InterfPtr ptrTarget = pTargetIf;
        ret = GetPointValuesToUpdate(
            ptrTarget, veckv );
        if( ERROR( ret ) )
            break;

        CCfgOpener oCfg;
        oCfg.SetPointer(
            PROP_TARGET_IF, pTargetIf );
        ret = pamc->SetPointValues(
            oCfg.GetCfg(), strApp, veckv );
        if( ret == STATUS_PENDING )
            ret = 0;
    }while( 0 );
    return ret;
}

// RPC Async Req Callback
gint32 CAsyncStdAMCallbacks::ClaimAppInstCallback(
    IConfigDb* context, 
    gint32 iRet,
    std::vector<KeyValue>& arrPtToGet /*[ In ]*/ )
{
    gint32 ret = 0;
    do{
        PACBS pCbs;
        stdstr strApp;
        CfgPtr pContext;
        InterfPtr pIf;
        ret = GetAppManagercli( pIf );
        if( ERROR( ret ) )
            break;
        CAppManager_CliImpl* pamc = pIf;
        g_oAmctx.GetAsyncCbs( pCbs, pContext );
        CCfgOpener oCtx( ( IConfigDb* )pContext );
        ret = oCtx.GetStrProp(
            PROP_APP_NAME, strApp );
        if( ERROR( ret ) )
            break;
        if( ERROR( iRet ) )
        {
            OutputMsg( iRet, "Error ClaimAppInst" );
            kill( getpid(), SIGINT );
            break;
        }
        OutputMsg( iRet, "Successfully claimed "
            "application %s", strApp.c_str() );
    }while( 0 );
    return ret;
}

// RPC Async Req Callback
gint32 CAsyncStdAMCallbacks::FreeAppInstsCallback(
    IConfigDb* context, 
    gint32 iRet )
{
    if( context == nullptr )
        return -EINVAL;
    gint32 ret = 0;
    do{
        CCfgOpener oCfg( context );
        CSyncCallback* pSync = nullptr;
        ret = oCfg.GetPointer(
            propEventSink, pSync );
        if( ERROR( ret ) )
            break;
        pSync->OnEvent(
            eventTaskComp, iRet, 0, nullptr );
    }while( 0 );
    return ret;
}

gint32 CAsyncStdAMCallbacks::OnSvrOffline(
    IConfigDb* context,
    CAppManager_CliImpl* pIf )
{
    DebugPrintEx( logErr, -ENOTCONN,
        "Error AppManager is offline, "
        "reconnect will be scheduled" );
    kill( getpid(), SIGUSR1 );
    ScheduleReconnect( context, pIf );
    return 0;
}

static gint32 ReconnectTimerFunc(   
    IConfigDb* pCtx, CIoManager* pMgr,
    CAppManager_CliImpl* pOldIf )
{
    gint32 ret = 0;
    DebugPrint( ret, "Info: Start reconnecting..." );
    gint32 (*comp_func)( IEventSink*,
        IConfigDb*, CIoManager*,
        CAppManager_CliImpl* ) =
    ([]( IEventSink* pTask,
        IConfigDb* pCtx,
        CIoManager* pMgr,
        CAppManager_CliImpl* pOldIf )->gint32
    {
        gint32 ret = 0;
        ObjPtr ptrResp;
        InterfPtr pAppMan;
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

            CRpcServices* pNewIf;
            ret = oResp.GetPointer(
                0, pNewIf );
            if( SUCCEEDED( ret ) )
                pAppMan = pNewIf;
            if( ERROR( iRet ) )
            {
                ret = iRet;
                break;
            }
            CCfgOpener oCtx( pCtx );
            ObjPtr pSvc;
            oCtx.GetObjPtr(
                PROP_TARGET_IF, pSvc );
            stdstr strApp;
            oCtx.GetStrProp(
                PROP_APP_NAME, strApp );
            PACBS pacbs;
            CfgPtr pOldCtx;
            ret = g_oAmctx.GetAsyncCbs(
                pacbs, pOldCtx );
            if( ERROR( ret ) )
                break;
            ret = ClaimApp( pSvc, strApp,
                pAppMan, pacbs );
        }while( 0 );
        if( SUCCEEDED( ret ) )
        {
            DebugPrint( ret, "Info: Reconnected successfully" );
            return ret;
        }
        if( !pAppMan.IsEmpty() )
        {
            // stop the newly created pAppMan
            gint32 (*stop_func)( IEventSink*,
                IConfigDb*, CIoManager*,
                CAppManager_CliImpl* ) =
            ([]( IEventSink* pTask,
                IConfigDb* pCtx,
                CIoManager* pMgr,
                CAppManager_CliImpl* pOldIf )->gint32
            {
                return 0;
            });
            TaskletPtr pStopCb;
            ret = NEW_COMPLETE_FUNCALL(
                0, pStopCb, pMgr, stop_func,
                nullptr, pCtx, pMgr, pOldIf );
            if( ERROR( ret ) )
                return ret;
            ret = DestroyAppManagercli(
                pMgr, pStopCb );
            if( ret == STATUS_PENDING )
                return 0;
            ( *pStopCb)( eventCancelTask );
        }
        return 0;
    });
    do{
        TaskletPtr pStartCb;
        ret = NEW_COMPLETE_FUNCALL(
            0, pStartCb, pMgr, comp_func,
            nullptr, pCtx, pMgr, pOldIf );
        if( ERROR( ret ) )
            break;
        InterfPtr pAppMan;
        ret = CreateAppManagercli(
            pMgr, pStartCb, pCtx, pAppMan );
        if( ERROR( ret ) )
        {
            ( *pStartCb )( eventCancelTask );
            break;
        }
        if( ret == STATUS_PENDING )
            ret = 0;
    }while( 0 );
    return ret;
}

gint32 CAsyncStdAMCallbacks::ScheduleReconnect(
    IConfigDb* pContext,
    CAppManager_CliImpl* pOldIf )
{
    gint32 ret = 0;
    CIoManager* pMgr = pOldIf->GetIoMgr();
    DebugPrintEx( logErr, ret,
        "Error scheduling reconnect to "
        "AppManager failed" );
    TaskletPtr pTimer;
    ret = ADD_TIMER_FUNC( pTimer, 
        RECONNECT_INTERVAL, pMgr,
        ReconnectTimerFunc, pContext,
        pMgr, pOldIf );
    return ret;
}

static gint32 GetPointValuesToUpdateCli(
    InterfPtr& pIf,
    std::vector< KeyValue >& veckv )
{
    gint32 ret = 0;
    do{
        KeyValue okv;
        okv.strKey = O_VMSIZE_KB;
        okv.oValue = GetVmSize();
        veckv.push_back( okv );

        okv.strKey = O_OBJ_COUNT;
        okv.oValue = CObjBase::GetActCount();
        veckv.push_back( okv );

        okv.strKey = O_CPU_LOAD;
        okv.oValue = GetCpuUsage();
        veckv.push_back( okv );

        okv.strKey = O_OPEN_FILES;
        guint32 dwCount = 0;
        ret = GetOpenFileCount(
            getpid(), dwCount );
        if( SUCCEEDED( ret ) )
        {
            okv.oValue = dwCount;
            veckv.push_back( okv );
        }
        ret = 0;
    }while( 0 );
    return 0;
}

gint32 CAsyncStdAMCallbacks::GetPointValuesToUpdate(
    InterfPtr& pIf,
    std::vector< KeyValue >& veckv )
{
    gint32 ret = 0;
    do{
        CFastRpcServerBase* pSvr = pIf;
        if( pSvr == nullptr )
        {
            ret = GetPointValuesToUpdateCli(
                pIf, veckv );
            break;
        }
        KeyValue okv;
        okv.strKey = O_CONNECTIONS;
        okv.oValue = ( guint32 )
            pSvr->GetStmSkelCount();;
        veckv.push_back( okv );

        okv.strKey = O_VMSIZE_KB;
        okv.oValue = GetVmSize();
        veckv.push_back( okv );

        okv.strKey = O_OBJ_COUNT;
        okv.oValue = CObjBase::GetActCount();
        veckv.push_back( okv );

        okv.strKey = O_PENDINGS;
        ret = pIf->GetProperty(
            propPendingTasks, okv.oValue );
        if( SUCCEEDED( ret ) )
            veckv.push_back( okv );

        okv.strKey = O_RXBYTE;
        ret = pIf->GetProperty(
            propRxBytes, okv.oValue );
        if( SUCCEEDED( ret ) )
            veckv.push_back( okv );

        okv.strKey = O_TXBYTE;
        ret = pIf->GetProperty(
            propTxBytes, okv.oValue );
        if( SUCCEEDED( ret ) )
            veckv.push_back( okv );

        okv.strKey = O_UPTIME;
        ret = pIf->GetProperty(
            propUptime, okv.oValue );
        if( SUCCEEDED( ret ) )
            veckv.push_back( okv );

        okv.strKey = O_FAIL_COUNT;
        ret = pIf->GetProperty(
            propFailureCount, okv.oValue );
        if( SUCCEEDED( ret ) )
            veckv.push_back( okv );

        okv.strKey = O_REQ_COUNT;
        ret = pIf->GetProperty(
            propMsgCount, okv.oValue );
        if( SUCCEEDED( ret ) )
            veckv.push_back( okv );

        okv.strKey = O_RESP_COUNT;
        ret = pIf->GetProperty(
            propMsgRespCount, okv.oValue );
        if( SUCCEEDED( ret ) )
            veckv.push_back( okv );

        okv.strKey = O_CPU_LOAD;
        okv.oValue = GetCpuUsage();
        veckv.push_back( okv );

        okv.strKey = O_OPEN_FILES;
        guint32 dwCount = 0;
        ret = GetOpenFileCount(
            getpid(), dwCount );
        if( SUCCEEDED( ret ) )
        {
            okv.oValue = dwCount;
            veckv.push_back( okv );
        }

        okv.strKey = S_MAX_QPS;
        ret = pSvr->GetProperty(
            propQps, okv.oValue );
        if( SUCCEEDED( ret ) )
            veckv.push_back( okv );

        okv.strKey = S_MAX_STM_PER_SESS;
        ret = pSvr->GetProperty(
            propStmPerSess, okv.oValue );
        if( SUCCEEDED( ret ) )
            veckv.push_back( okv );
        ret = 0;
    }while( 0 );
    return 0;
}

gint32 CAsyncStdAMCallbacks::GetPointValuesToInit(
    InterfPtr& pIf,
    std::vector< KeyValue >& veckv )
{
    gint32 ret = 0;
    do{
        KeyValue okv;
        okv.strKey = O_WORKING_DIR;
        char szPath[PATH_MAX];
        if( getcwd( szPath, sizeof(szPath)) != nullptr)
        {        
            BufPtr pBuf( true );
            ret = pBuf->Append( szPath,
                strlen( szPath ) );
            if( SUCCEEDED( ret ) )
            {
                okv.oValue = pBuf;
                veckv.push_back( okv );
            }
        }

        okv.strKey = O_PID;
        okv.oValue = ( guint32 )getpid();
        veckv.push_back( okv );

        okv.strKey = S_CMDLINE;
        stdstr strCmdLine;
        CRpcServices* pSvc = pIf;
        CIoManager* pMgr = pSvc->GetIoMgr();
        ret = pMgr->GetCmdLineOpt(
            propCmdLine, strCmdLine );
        if( SUCCEEDED( ret ) )
        {
            BufPtr pBuf( true );
            ret = pBuf->Append(
                strCmdLine.c_str(),
                strCmdLine.size() );
            if( ERROR( ret ) )
                break;
            okv.oValue = pBuf;
            veckv.push_back( okv );
        }
        ret = 0;

    }while( 0 );
    return ret;
}
