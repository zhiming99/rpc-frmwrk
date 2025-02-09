// Generated by ridlc
// ../../ridl/.libs/ridlc -sO . ./appmon.ridl 
// Your task is to implement the following classes
// to get your rpc server work
#pragma once
#include "appmon.h"
#include "blkalloc.h"
#define Clsid_CAppManager_SvrBase    Clsid_Invalid

typedef enum : guint32
{
    ptInvalid = 0xffffffff,
    ptOutput = 0,
    ptInput = 1,
    ptSetPoint = 2,
} EnumPointType;

DECLARE_AGGREGATED_SERVER(
    CAppManager_SvrBase,
    CStatCounters_SvrBase,
    IIAppStore_SvrApi,
    IIDataProducer_SvrApi,
    CFastRpcServerBase );

extern rpcf::RegFsPtr g_pAppRegfs;

class CAppManager_SvrImpl
    : public CAppManager_SvrBase
{
    protected:

    RegFsPtr m_pAppRegfs;
    std::hashmap< HANDLE, StrSetPtr > m_mapAppOwners;

    public:
    typedef CAppManager_SvrBase super;
    CAppManager_SvrImpl( const IConfigDb* pCfg ) :
        super::virtbase( pCfg ), super( pCfg ),
        m_pAppRegfs( g_pAppRegfs )
    { SetClassId( clsid(CAppManager_SvrImpl ) ); }

    bool IsAppOnline( const stdstr& strAppName );

    gint32 OnPostStart(
        IEventSink* pCallback ) override
    {
        TaskletPtr pTask = GetUpdateTokenTask();
        StartQpsTask( pTask );
        if( !pTask.IsEmpty() )
            AllocReqToken();
        return super::OnPostStart( pCallback );
    }

    gint32 OnPreStop(
        IEventSink* pCallback ) override
    {
        StopQpsTask();
        return super::OnPreStop( pCallback );
    }

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
        std::vector<KeyValue>& arrValues /*[ In ]*/ ) override
    {
        DebugPrintEx( logErr, iRet,
            "request 'SetPointValues' is canceled." );
        return STATUS_SUCCESS;
    }
    //RPC Async Req Handler
    gint32 SetPointValues(
        IConfigDb* pReqCtx_,
        std::vector<KeyValue>& arrValues /*[ In ]*/ ) override;
    //RPC Async Req Cancel Handler
    gint32 OnGetPointValuesCanceled(
        IConfigDb* pReqCtx_, gint32 iRet,
        std::vector<std::string>& arrPtPaths /*[ In ]*/ ) override
    {
        DebugPrintEx( logErr, iRet,
            "request 'GetPointValues' is canceled." );
        return STATUS_SUCCESS;
    }
    //RPC Async Req Handler
    gint32 GetPointValues(
        IConfigDb* pReqCtx_,
        std::vector<std::string>& arrPtPaths /*[ In ]*/,
        std::vector<KeyValue>& arrKeyVals /*[ Out ]*/ ) override;
    gint32 OnPointChanged(
        const std::string& strPtPath /*[ In ]*/,
        const Variant& value /*[ In ]*/ ) override;
    gint32 OnPointsChanged(
        std::vector<KeyValue>& arrKVs /*[ In ]*/ ) override;
    //RPC Async Req Cancel Handler
    gint32 OnClaimAppInstsCanceled(
        IConfigDb* pReqCtx_, gint32 iRet,
        std::vector<std::string>& arrApps /*[ In ]*/ ) override
    {
        DebugPrintEx( logErr, iRet,
            "request 'ClaimAppInsts' is canceled." );
        return STATUS_SUCCESS;
    }
    //RPC Async Req Handler
    gint32 ClaimAppInsts(
        IConfigDb* pReqCtx_,
        std::vector<std::string>& arrApps /*[ In ]*/ ) override;
    //RPC Async Req Cancel Handler
    gint32 OnFreeAppInstsCanceled(
        IConfigDb* pReqCtx_, gint32 iRet,
        std::vector<std::string>& arrApps /*[ In ]*/ ) override
    {
        DebugPrintEx( logErr, iRet,
            "request 'FreeAppInsts' is canceled." );
        return STATUS_SUCCESS;
    }

    gint32 FreeAppInstsInternal( HANDLE hstm,
        std::vector<stdstr>& arrApps );

    //RPC Async Req Handler
    gint32 FreeAppInsts(
        IConfigDb* pReqCtx_,
        std::vector<std::string>& arrApps /*[ In ]*/ ) override;
    gint32 CreateStmSkel(
        HANDLE, guint32, InterfPtr& ) override;
    
    gint32 OnPreStart(
        IEventSink* pCallback ) override;

    gint32 NotifyValChange(
        const stdstr strPtPath,
        const Variant& value,
        RFHANDLE hcurStm );

    gint32 RemoveStmSkel( HANDLE hstm ) override;
};

class CAppManager_ChannelSvr
    : public CRpcStreamChannelSvr
{
    public:
    typedef CRpcStreamChannelSvr super;
    CAppManager_ChannelSvr(
        const IConfigDb* pCfg ) :
        super::virtbase( pCfg ), super( pCfg )
    { SetClassId( clsid(
        CAppManager_ChannelSvr ) ); }
};

