// GENERATED BY RIDLC, MAKE SURE TO BACKUP BEFORE RUNNING RIDLC AGAIN
// Copyright (C) 2024  zhiming <woodhead99@gmail.com>
// This program can be distributed under the terms of the GNU GPLv3.
// ../../ridl/.libs/ridlc --server -sO . ./appmon.ridl 
#pragma once
#include "appmon.h"
#include "blkalloc.h"
#include <unordered_set>
#include "IAppStoresvr.h"
#include "IAppMonitorsvr.h"

struct CFlockHelper
{
    gint32 m_iFd = -1;
    public:
    CFlockHelper( gint32 iFd, bool bRead = true );
    ~CFlockHelper();
};

struct CFileHandle
{
    RFHANDLE m_hFile = INVALID_HANDLE;
    ObjPtr m_pFs;
    public:
    CFileHandle( ObjPtr pFs, RFHANDLE hFile )
    { m_hFile = hFile; m_pFs = pFs; }
    ~CFileHandle();
};
#define Clsid_CAppMonitor_SvrSkel_Base Clsid_Invalid

DECLARE_AGGREGATED_SKEL_SERVER(
    CAppMonitor_SvrSkel_Base,
    CStatCountersServerSkel,
    IIAppStore_SImpl,
    IIAppMonitor_SImpl );

class CAppMonitor_SvrSkel :
    public CAppMonitor_SvrSkel_Base
{
    public:
    typedef CAppMonitor_SvrSkel_Base super;
    CAppMonitor_SvrSkel( const IConfigDb* pCfg ):
        super::virtbase( pCfg ), super( pCfg )
    { SetClassId( clsid( CAppMonitor_SvrSkel ) ); }

    gint32 InvokeUserMethod(
        IConfigDb* pParams,
        IEventSink* pCallback ) override
    {
        CRpcServices* pSvc = nullptr;
        if( !IsRfcEnabled() )
        {
            if( m_pQpsTask.IsEmpty() )
                pSvc = this->GetStreamIf();
            else
                pSvc = this;
            gint32 ret = pSvc->AllocReqToken();
            if( ERROR( ret ) )
                return ret;
        }
        return super::InvokeUserMethod(
            pParams, pCallback );
    }
};
extern rpcf::RegFsPtr g_pAppRegfs;
#define Clsid_CAppMonitor_SvrBase    Clsid_Invalid

DECLARE_AGGREGATED_SERVER(
    CAppMonitor_SvrBase,
    CStatCounters_SvrBase,
    CStreamServerAsync,
    IIAppStore_SvrApi,
    IIAppMonitor_SvrApi,
    CFastRpcServerBase );

class CAppMonitor_SvrImpl
    : public CAppMonitor_SvrBase
{
    protected:

    std::hashmap< stdstr, gint32 > m_mapOA2Name2Uid;
    std::hashmap< stdstr, gint32 > m_mapKrb5Name2Uid;
    std::hashmap< stdstr, gint32 > m_mapName2Uid;

    using UID2GIDS = std::hashmap< guint32, IntSetPtr >;
    UID2GIDS m_mapUid2Gids;

    std::hashmap< HANDLE, StrSetPtr > m_mapListeners;

    gint32 GetLoginInfo(
        IConfigDb* pCtx, CfgPtr& pInfo ) const;

    gint32 GetUid( IConfigDb* pInfo,
        guint32& dwUid ) const;

    RegFsPtr m_pAppRegfs;

    gint32 RemoveListenerInternal(
        HANDLE hstm, CAccessContext* pac );

    bool IsAppSubscribed( HANDLE hstm,
        const stdstr& strApp ) const;

    gint32 LoadAssocMaps(
        RegFsPtr& pfs, int iAuthMech );

    gint32 LoadUserGrpsMap();

    public:
    typedef CAppMonitor_SvrBase super;
    CAppMonitor_SvrImpl( const IConfigDb* pCfg ) :
        super::virtbase( pCfg ), super( pCfg ),
        m_pAppRegfs( g_pAppRegfs )
    { SetClassId( clsid(CAppMonitor_SvrImpl ) ); }

    /* The following 3 methods are important for */
    /* streaming transfer. rewrite them if necessary */
    gint32 OnStreamReady( HANDLE hChannel ) override
    { return super::OnStreamReady( hChannel ); } 
    
    gint32 OnStmClosing( HANDLE hChannel ) override
    { return super::OnStmClosing( hChannel ); }
    
    gint32 AcceptNewStream(
        IEventSink* pCb, IConfigDb* pDataDesc ) override
    { return STATUS_SUCCESS; }
    
    gint32 OnPostStart(
        IEventSink* pCallback ) override;

    gint32 OnPreStop(
        IEventSink* pCallback ) override
    {
        StopQpsTask();
        return super::OnPreStop( pCallback );
    }

    gint32 RemoveStmSkel( HANDLE hstm ) override;

    //RPC Async Req Cancel Handler
    gint32 OnListAppsCanceled(
        IConfigDb* pReqCtx_, gint32 iRet ) override
    {
        DebugPrintEx( logErr, iRet,
            "request 'ListApps' is canceled." );
        return STATUS_SUCCESS;
    }
    //RPC Async Req Handler
    gint32 ListApps(
        IConfigDb* pReqCtx_,
        std::vector<std::string>& arrApps /*[ Out ]*/ ) override;
    //RPC Async Req Cancel Handler
    gint32 OnListPointsCanceled(
        IConfigDb* pReqCtx_, gint32 iRet,
        const std::string& strAppName /*[ In ]*/ ) override
    {
        DebugPrintEx( logErr, iRet,
            "request 'ListPoints' is canceled." );
        return STATUS_SUCCESS;
    }
    //RPC Async Req Handler
    gint32 ListPoints(
        IConfigDb* pReqCtx_,
        const std::string& strAppName /*[ In ]*/,
        std::vector<std::string>& arrPoints /*[ Out ]*/ ) override;
    //RPC Async Req Cancel Handler
    gint32 OnListAttributesCanceled(
        IConfigDb* pReqCtx_, gint32 iRet,
        const std::string& strPtPath /*[ In ]*/ ) override
    {
        DebugPrintEx( logErr, iRet,
            "request 'ListAttributes' is canceled." );
        return STATUS_SUCCESS;
    }
    //RPC Async Req Handler
    gint32 ListAttributes(
        IConfigDb* pReqCtx_,
        const std::string& strPtPath /*[ In ]*/,
        std::vector<std::string>& arrAttributes /*[ Out ]*/ ) override;
    //RPC Async Req Cancel Handler
    gint32 OnSetPointValueCanceled(
        IConfigDb* pReqCtx_, gint32 iRet,
        const std::string& strPtPath /*[ In ]*/,
        const Variant& value /*[ In ]*/ ) override
    {
        DebugPrintEx( logErr, iRet,
            "request 'SetPointValue' is canceled." );
        return STATUS_SUCCESS;
    }
    //RPC Async Req Handler
    gint32 SetPointValue(
        IConfigDb* pReqCtx_,
        const std::string& strPtPath /*[ In ]*/,
        const Variant& value /*[ In ]*/ ) override;
    //RPC Async Req Cancel Handler
    gint32 OnGetPointValueCanceled(
        IConfigDb* pReqCtx_, gint32 iRet,
        const std::string& strPtPath /*[ In ]*/ ) override
    {
        DebugPrintEx( logErr, iRet,
            "request 'GetPointValue' is canceled." );
        return STATUS_SUCCESS;
    }
    //RPC Async Req Handler
    gint32 GetPointValue(
        IConfigDb* pReqCtx_,
        const std::string& strPtPath /*[ In ]*/,
        Variant& rvalue /*[ Out ]*/ ) override;
    //RPC Async Req Cancel Handler
    gint32 OnSetLargePointValueCanceled(
        IConfigDb* pReqCtx_, gint32 iRet,
        const std::string& strPtPath /*[ In ]*/,
        BufPtr& value /*[ In ]*/ ) override
    {
        DebugPrintEx( logErr, iRet,
            "request 'SetLargePointValue' is canceled." );
        return STATUS_SUCCESS;
    }
    //RPC Async Req Handler
    gint32 SetLargePointValue(
        IConfigDb* pReqCtx_,
        const std::string& strPtPath /*[ In ]*/,
        BufPtr& value /*[ In ]*/ ) override;
    //RPC Async Req Cancel Handler
    gint32 OnGetLargePointValueCanceled(
        IConfigDb* pReqCtx_, gint32 iRet,
        const std::string& strPtPath /*[ In ]*/ ) override
    {
        DebugPrintEx( logErr, iRet,
            "request 'GetLargePointValue' is canceled." );
        return STATUS_SUCCESS;
    }
    //RPC Async Req Handler
    gint32 GetLargePointValue(
        IConfigDb* pReqCtx_,
        const std::string& strPtPath /*[ In ]*/,
        BufPtr& value /*[ Out ]*/ ) override;
    //RPC Async Req Cancel Handler
    gint32 OnSubscribeStreamPointCanceled(
        IConfigDb* pReqCtx_, gint32 iRet,
        const std::string& strPtPath /*[ In ]*/,
        HANDLE hstm_h /*[ In ]*/ ) override
    {
        DebugPrintEx( logErr, iRet,
            "request 'SubscribeStreamPoint' is canceled." );
        return STATUS_SUCCESS;
    }
    //RPC Async Req Handler
    gint32 SubscribeStreamPoint(
        IConfigDb* pReqCtx_,
        const std::string& strPtPath /*[ In ]*/,
        HANDLE hstm_h /*[ In ]*/ ) override;
    //RPC Async Req Cancel Handler
    gint32 OnSetAttrValueCanceled(
        IConfigDb* pReqCtx_, gint32 iRet,
        const std::string& strAttrPath /*[ In ]*/,
        const Variant& value /*[ In ]*/ ) override
    {
        DebugPrintEx( logErr, iRet,
            "request 'SetAttrValue' is canceled." );
        return STATUS_SUCCESS;
    }
    //RPC Async Req Handler
    gint32 SetAttrValue(
        IConfigDb* pReqCtx_,
        const std::string& strAttrPath /*[ In ]*/,
        const Variant& value /*[ In ]*/ ) override;
    //RPC Async Req Cancel Handler
    gint32 OnGetAttrValueCanceled(
        IConfigDb* pReqCtx_, gint32 iRet,
        const std::string& strAttrPath /*[ In ]*/ ) override
    {
        DebugPrintEx( logErr, iRet,
            "request 'GetAttrValue' is canceled." );
        return STATUS_SUCCESS;
    }
    //RPC Async Req Handler
    gint32 GetAttrValue(
        IConfigDb* pReqCtx_,
        const std::string& strAttrPath /*[ In ]*/,
        Variant& rvalue /*[ Out ]*/ ) override;
    //RPC Async Req Cancel Handler
    gint32 OnSetPointValuesCanceled(
        IConfigDb* pReqCtx_, gint32 iRet,
        const std::string& strAppName /*[ In ]*/,
        std::vector<KeyValue>& arrValues /*[ In ]*/ ) override
    {
        DebugPrintEx( logErr, iRet,
            "request 'SetPointValues' is canceled." );
        return STATUS_SUCCESS;
    }
    //RPC Async Req Handler
    gint32 SetPointValues(
        IConfigDb* pReqCtx_,
        const std::string& strAppName /*[ In ]*/,
        std::vector<KeyValue>& arrValues /*[ In ]*/ ) override;
    //RPC Async Req Cancel Handler
    gint32 OnGetPointValuesCanceled(
        IConfigDb* pReqCtx_, gint32 iRet,
        const std::string& strAppName /*[ In ]*/,
        std::vector<std::string>& arrPtPaths /*[ In ]*/ ) override
    {
        DebugPrintEx( logErr, iRet,
            "request 'GetPointValues' is canceled." );
        return STATUS_SUCCESS;
    }
    //RPC Async Req Handler
    gint32 GetPointValues(
        IConfigDb* pReqCtx_,
        const std::string& strAppName /*[ In ]*/,
        std::vector<std::string>& arrPtPaths /*[ In ]*/,
        std::vector<KeyValue>& arrKeyVals /*[ Out ]*/ ) override;

    gint32 OnPointChangedInternal(
        const std::string& strPtPath /*[ In ]*/,
        const Variant& value /*[ In ]*/,
        HANDLE hcurStm = INVALID_HANDLE );

    gint32 OnPointChanged(
        const std::string& strPtPath /*[ In ]*/,
        const Variant& value /*[ In ]*/ ) override
    { return OnPointChangedInternal( strPtPath, value ); }

    gint32 OnPointsChangedInternal(
        std::vector<KeyValue>& arrKVs /*[ In ]*/,
        HANDLE hcurStm = INVALID_HANDLE );

    gint32 OnPointsChanged(
        std::vector<KeyValue>& arrKVs /*[ In ]*/ ) override
    { return OnPointsChangedInternal( arrKVs ); }
    //RPC Async Req Cancel Handler
    gint32 OnRegisterListenerCanceled(
        IConfigDb* pReqCtx_, gint32 iRet,
        std::vector<std::string>& arrApps /*[ In ]*/ ) override
    {
        DebugPrintEx( logErr, iRet,
            "request 'RegisterListener' is canceled." );
        return STATUS_SUCCESS;
    }
    //RPC Async Req Handler
    gint32 RegisterListener(
        IConfigDb* pReqCtx_,
        std::vector<std::string>& arrApps /*[ In ]*/ ) override;
    //RPC Async Req Cancel Handler
    gint32 OnRemoveListenerCanceled(
        IConfigDb* pReqCtx_, gint32 iRet ) override
    {
        DebugPrintEx( logErr, iRet,
            "request 'RemoveListener' is canceled." );
        return STATUS_SUCCESS;
    }
    //RPC Async Req Handler
    gint32 RemoveListener(
        IConfigDb* pReqCtx_ ) override;
    gint32 CreateStmSkel(
        HANDLE, guint32, InterfPtr& ) override;
    
    gint32 OnPreStart(
        IEventSink* pCallback ) override;
    bool IsUserValid( guint32 dwUid ) const;
    gint32 GetAccessContext(
        IConfigDb* pReqCtx,
        CAccessContext& oac ) const;
};

class CAppMonitor_ChannelSvr
    : public CRpcStreamChannelSvr
{
    public:
    typedef CRpcStreamChannelSvr super;
    CAppMonitor_ChannelSvr(
        const IConfigDb* pCfg ) :
        super::virtbase( pCfg ), super( pCfg )
    { SetClassId( clsid(
        CAppMonitor_ChannelSvr ) ); }

    gint32 OnStreamReady( HANDLE hstm ) override;
};

