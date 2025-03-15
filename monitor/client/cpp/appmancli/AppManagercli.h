// GENERATED BY RIDLC, MAKE SURE TO BACKUP BEFORE RUNNING RIDLC AGAIN
// Copyright (C) 2024  zhiming <woodhead99@gmail.com>
// This program can be distributed under the terms of the GNU GPLv3.
// ../../../../ridl/.libs/ridlc --sync_mode IAppStore=async_p --sync_mode IUserManager=async_p --services=AppManager,SimpleAuth --client -slO . ../../../../monitor/appmon/appmon.ridl 
#pragma once
#include "appmon.h"

#include "IAppStorecli.h"
#include "IDataProducercli.h"

gint32 GetAppManagercli( InterfPtr& pCli );

/*gint32 CreateAppManagercli(
    CIoManager* pMgr,
    IEventSink* pCallback,
    IConfigDb* pCfg );
*/

gint32 DestroyAppManagercli(
    CIoManager* pMgr,
    IEventSink* pCallback );

using PTCHGCB=void* (*)( IConfigDb* pContext,
    const std::string& strPtPath, const Variant& oVar );

struct IAsyncAMCallbacks;
using PACBS=std::shared_ptr<IAsyncAMCallbacks>;

gint32 StartStdAppManCli( CRpcServices* pSvc,
    const std::string& strAppInst,
    InterfPtr& pAppMan,
    PACBS& pacbsIn );

gint32 StopStdAppManCli();

template< typename ToCreate, EnumClsid iClsid >
gint32 AsyncCreateIf( CIoManager* pMgr,
    IEventSink* pCallback, IConfigDb* pCfg,
    const stdstr& strDesc,
    const stdstr& strObjName,
    InterfPtr& pNewIf,
    bool bServer )
{
    gint32 ret = 0;
    do{
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
            strObjName, bServer, ptrCfg );
        if( ERROR( ret ) )
            break;
        ret = pIf.NewObj( iClsid, ptrCfg );
        if( ERROR( ret ) )
            break;
        pNewIf = pIf;
        gint32 (*pChecker)( IEventSink*,
            ToCreate*, IEventSink*,
            IConfigDb* ) = ([]( IEventSink* pTask,
            ToCreate* pIf,
            IEventSink* pCb, IConfigDb* pCfg )->gint32
        {
            gint32 ret = 0;
            do{
                CCfgOpenerObj oReq( pTask );
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
            if( pCb == nullptr )
                return ret;

            CCfgOpener oCfg;
            oCfg.SetIntProp( propReturnValue, ret );
            oCfg.SetPointer( 0, pIf );
            ObjPtr pobjIf( oCfg.GetCfg() );
            Variant oVar( pobjIf );
            pCb->SetProperty( propRespPtr, oVar );
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
            ToCreate*, IEventSink* ) = ([](
            IEventSink* pTask, ToCreate* pIf,
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

                if( pCallback == nullptr )
                    break;

                CCfgOpener oCfg;
                oCfg.SetIntProp( propReturnValue, ret );
                oCfg.SetPointer( 0, pIf );
                ObjPtr pobjIf( oCfg.GetCfg() );
                Variant oVar( pobjIf );
                pCallback->SetProperty( propRespPtr, oVar );
                ObjPtr pObj( pCallback );
                CIfRetryTask* pRetry( pObj );
                pRetry->ClearClientNotify();
                pRetry->OnEvent( eventTaskComp,
                    ret, 0, nullptr );
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

        ret = pMgr->AddSeqTask( pStartTask );
        if( ERROR( ret ) )
        {
            pRetry->ClearClientNotify();
            ( *pStartTask )( eventCancelTask );
            ( *pStartCb )( eventCancelTask );
        }
        if( SUCCEEDED( ret ) )
        {
            ret = STATUS_PENDING;
        }

    }while( 0 );
    return ret;
}

class CAppManager_CliImpl;
struct IAsyncAMCallbacks
{
    InterfPtr m_pIf;
    inline void SetInterface( InterfPtr& pIf )
    { m_pIf = pIf; }

    virtual gint32 GetPointValuesToUpdate(
        InterfPtr& pIf,
        std::vector< KeyValue >& veckv )
    { return 0; } 

    virtual gint32 SetInitPointValues(
        InterfPtr& pIf,
        std::vector< KeyValue >& veckv )
    { return 0; } 

    // RPC Async Req Callback
    virtual gint32 ListAppsCallback(
        IConfigDb* context, 
        gint32 iRet,
        std::vector<std::string>& arrApps /*[ In ]*/ )
    { return 0;}
    // RPC Async Req Callback
    virtual gint32 ListPointsCallback(
        IConfigDb* context, 
        gint32 iRet,
        std::vector<std::string>& arrPoints /*[ In ]*/ )
    { return 0;}
    // RPC Async Req Callback
    virtual gint32 ListAttributesCallback(
        IConfigDb* context, 
        gint32 iRet,
        std::vector<std::string>& arrAttributes /*[ In ]*/ )
    { return 0;}
    // RPC Async Req Callback
    virtual gint32 SetPointValueCallback(
        IConfigDb* context, 
        gint32 iRet )
    { return 0;}
    // RPC Async Req Callback
    virtual gint32 GetPointValueCallback(
        IConfigDb* context, 
        gint32 iRet,
        const Variant& rvalue /*[ In ]*/ ) 
    { return 0;}
    // RPC Async Req Callback
    virtual gint32 SetLargePointValueCallback(
        IConfigDb* context, 
        gint32 iRet )
    { return 0;}
    // RPC Async Req Callback
    virtual gint32 GetLargePointValueCallback(
        IConfigDb* context, 
        gint32 iRet,
        BufPtr& value /*[ In ]*/ )
    { return 0;}
    // RPC Async Req Callback
    virtual gint32 SubscribeStreamPointCallback(
        IConfigDb* context, 
        gint32 iRet )
    { return 0;}
    // RPC Async Req Callback
    virtual gint32 SetAttrValueCallback(
        IConfigDb* context, 
        gint32 iRet )
    { return 0;}
    // RPC Async Req Callback
    virtual gint32 GetAttrValueCallback(
        IConfigDb* context, 
        gint32 iRet,
        const Variant& rvalue /*[ In ]*/ )
    { return 0;}
    // RPC Async Req Callback
    virtual gint32 SetPointValuesCallback(
        IConfigDb* context, 
        gint32 iRet )
    { return 0; }
    // RPC Async Req Callback
    virtual gint32 GetPointValuesCallback(
        IConfigDb* context, 
        gint32 iRet,
        std::vector<KeyValue>& arrKeyVals /*[ In ]*/ )
    { return 0;}
    //RPC event handler 'OnPointChanged'
    virtual gint32 OnPointChanged(
        IConfigDb* context, 
        const std::string& strPtPath /*[ In ]*/,
        const Variant& value /*[ In ]*/ )
    { return 0;}
    //RPC event handler 'OnPointsChanged'
    virtual gint32 OnPointsChanged(
        IConfigDb* context, 
        std::vector<KeyValue>& arrKVs /*[ In ]*/ )
    { return 0;}
    // RPC Async Req Callback
    virtual gint32 ClaimAppInstCallback(
        IConfigDb* context, 
        gint32 iRet,
        std::vector<KeyValue>& arrPtToGet /*[ In ]*/ )
    { return 0;}
    // RPC Async Req Callback
    virtual gint32 FreeAppInstsCallback(
        IConfigDb* context, 
        gint32 iRet )
    { return 0;}

    virtual gint32 OnSvrOffline( IConfigDb* context,
        CAppManager_CliImpl* pIf )
    { return 0; }
};

struct CAsyncStdAMCallbacks : public IAsyncAMCallbacks
{
    gint32 GetPointValuesToUpdate(
        InterfPtr& pIf,
        std::vector< KeyValue >& veckv ) override
    { return 0; }

    // RPC Async Req Callback
    gint32 SetPointValuesCallback(
        IConfigDb* context, 
        gint32 iRet ) override
    { return 0; }

    // RPC Async Req Callback
    gint32 GetPointValueCallback(
        IConfigDb* context, 
        gint32 iRet,
        const Variant& rvalue /*[ In ]*/ )  override
    { return 0; }

    //RPC event handler 'OnPointChanged'
    gint32 OnPointChanged(
        IConfigDb* context, 
        const std::string& strPtPath /*[ In ]*/,
        const Variant& value /*[ In ]*/ ) override;

    // RPC Async Req Callback
    gint32 ClaimAppInstCallback(
        IConfigDb* context, 
        gint32 iRet,
        std::vector<KeyValue>& arrPtToGet /*[ In ]*/ ) override;

    // RPC Async Req Callback
    gint32 FreeAppInstsCallback(
        IConfigDb* context, 
        gint32 iRet ) override;

    gint32 OnSvrOffline(
        IConfigDb* context,
        CAppManager_CliImpl* pIf ) override;

    gint32 ScheduleReconnect(
        IConfigDb* pContext,
        CAppManager_CliImpl* pOldIf );
};

DECLARE_AGGREGATED_SKEL_PROXY(
    CAppManager_CliSkel,
    CStatCountersProxySkel,
    IIAppStore_PImpl,
    IIDataProducer_PImpl );

#define Clsid_CAppManager_CliBase    Clsid_Invalid

DECLARE_AGGREGATED_PROXY(
    CAppManager_CliBase,
    CStatCounters_CliBase,
    CStreamProxyAsync,
    IIAppStore_CliApi,
    IIDataProducer_CliApi,
    CFastRpcProxyBase );

class CAppManager_CliImpl
    : public CAppManager_CliBase
{
    PACBS m_pAsyncCbs;
    CfgPtr m_pContext;
    public:

    public:
    typedef CAppManager_CliBase super;
    CAppManager_CliImpl( const IConfigDb* pCfg ) :
        super::virtbase( pCfg ), super( pCfg )
    { SetClassId( clsid(CAppManager_CliImpl ) ); }

    /* The following 2 methods are important for */
    /* streaming transfer. rewrite them if necessary */
    gint32 OnStreamReady( HANDLE hChannel ) override
    { return super::OnStreamReady( hChannel ); } 
    
    gint32 OnStmClosing( HANDLE hChannel ) override
    { return super::OnStmClosing( hChannel ); }

    inline void SetAsyncCallbacks(
        PACBS& pCbs, const CfgPtr& pcontext )
    {
        CStdRMutex  oLock( GetLock() );
        m_pAsyncCbs = pCbs;
        m_pContext = pcontext;
    };

    inline gint32 GetAsyncCallbacks(
        PACBS& pCbs, CfgPtr& pContext ) const
    {
        CStdRMutex  oLock( GetLock() );
        if( m_pAsyncCbs == nullptr )
            return -EINVAL;
        pCbs = m_pAsyncCbs;
        pContext = m_pContext;
        return 0;
    }

    inline gint32 GetAsyncCallbacks(
        PACBS& pCbs ) const
    {
        CStdRMutex  oLock( GetLock() );
        if( m_pAsyncCbs == nullptr )
            return -EINVAL;
        pCbs = m_pAsyncCbs;
        return 0;
    }

    inline void ClearCallbacks()
    {
        CStdRMutex  oLock( GetLock() );
        m_pAsyncCbs.reset();
        m_pContext.Clear();
    };
    
    // RPC Async Req Callback
    gint32 ListAppsCallback(
        IConfigDb* context, 
        gint32 iRet,
        std::vector<std::string>& arrApps /*[ In ]*/ ) override;
    // RPC Async Req Callback
    gint32 ListPointsCallback(
        IConfigDb* context, 
        gint32 iRet,
        std::vector<std::string>& arrPoints /*[ In ]*/ ) override;
    // RPC Async Req Callback
    gint32 ListAttributesCallback(
        IConfigDb* context, 
        gint32 iRet,
        std::vector<std::string>& arrAttributes /*[ In ]*/ ) override;
    // RPC Async Req Callback
    gint32 SetPointValueCallback(
        IConfigDb* context, 
        gint32 iRet ) override;
    // RPC Async Req Callback
    gint32 GetPointValueCallback(
        IConfigDb* context, 
        gint32 iRet,
        const Variant& rvalue /*[ In ]*/ ) override;
    // RPC Async Req Callback
    gint32 SetLargePointValueCallback(
        IConfigDb* context, 
        gint32 iRet ) override;
    // RPC Async Req Callback
    gint32 GetLargePointValueCallback(
        IConfigDb* context, 
        gint32 iRet,
        BufPtr& value /*[ In ]*/ ) override;
    // RPC Async Req Callback
    gint32 SubscribeStreamPointCallback(
        IConfigDb* context, 
        gint32 iRet ) override;
    // RPC Async Req Callback
    gint32 SetAttrValueCallback(
        IConfigDb* context, 
        gint32 iRet ) override;
    // RPC Async Req Callback
    gint32 GetAttrValueCallback(
        IConfigDb* context, 
        gint32 iRet,
        const Variant& rvalue /*[ In ]*/ ) override;
    // RPC Async Req Callback
    gint32 SetPointValuesCallback(
        IConfigDb* context, 
        gint32 iRet ) override;
    // RPC Async Req Callback
    gint32 GetPointValuesCallback(
        IConfigDb* context, 
        gint32 iRet,
        std::vector<KeyValue>& arrKeyVals /*[ In ]*/ ) override;
    //RPC event handler 'OnPointChanged'
    gint32 OnPointChanged(
        const std::string& strPtPath /*[ In ]*/,
        const Variant& value /*[ In ]*/ ) override;
    //RPC event handler 'OnPointsChanged'
    gint32 OnPointsChanged(
        std::vector<KeyValue>& arrKVs /*[ In ]*/ ) override;
    // RPC Async Req Callback
    gint32 ClaimAppInstCallback(
        IConfigDb* context, 
        gint32 iRet,
        std::vector<KeyValue>& arrPtToGet /*[ In ]*/ ) override;
    // RPC Async Req Callback
    gint32 FreeAppInstsCallback(
        IConfigDb* context, 
        gint32 iRet ) override;

    gint32 CreateStmSkel(
        InterfPtr& pIf ) override;
    
    gint32 OnPreStart(
        IEventSink* pCallback ) override;

    gint32 CancelRequest(
        guint64 qwTaskToCancel )
    {
        InterfPtr pIf = this->GetStmSkel();
        if( pIf.IsEmpty() )
            return -EFAULT;
        CInterfaceProxy* pProxy = pIf;
        return pProxy->CancelRequest(
            qwTaskToCancel );
    }
    gint32 OnPostStop(
        IEventSink* pCallback ) override;
};

class CAppManager_ChannelCli
    : public CRpcStreamChannelCli
{
    public:
    typedef CRpcStreamChannelCli super;
    CAppManager_ChannelCli(
        const IConfigDb* pCfg ) :
        super::virtbase( pCfg ), super( pCfg )
    { SetClassId( clsid(
        CAppManager_ChannelCli ) ); }
};

