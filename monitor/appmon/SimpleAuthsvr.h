// Generated by ridlc
// ../../ridl/.libs/ridlc -sO . ./appmon.ridl 
// Your task is to implement the following classes
// to get your rpc server work
#pragma once
#include "appmon.h"

#define Clsid_CSimpleAuth_SvrBase    Clsid_Invalid

DECLARE_AGGREGATED_SERVER(
    CSimpleAuth_SvrBase,
    CStatCounters_SvrBase,
    IIUserManager_SvrApi,
    CFastRpcServerBase );

class CSimpleAuth_SvrImpl
    : public CSimpleAuth_SvrBase
{
    public:
    typedef CSimpleAuth_SvrBase super;
    CSimpleAuth_SvrImpl( const IConfigDb* pCfg ) :
        super::virtbase( pCfg ), super( pCfg )
    { SetClassId( clsid(CSimpleAuth_SvrImpl ) ); }

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
    gint32 OnGetUidByOAuth2NameCanceled(
        IConfigDb* pReqCtx_, gint32 iRet,
        const std::string& strOA2Name /*[ In ]*/ ) override
    {
        DebugPrintEx( logErr, iRet,
            "request 'GetUidByOAuth2Name' is canceled." );
        return STATUS_SUCCESS;
    }
    //RPC Async Req Handler
    gint32 GetUidByOAuth2Name(
        IConfigDb* pReqCtx_,
        const std::string& strOA2Name /*[ In ]*/,
        gint32& dwUid /*[ Out ]*/ ) override;
    //RPC Async Req Cancel Handler
    gint32 OnGetUidByKrb5NameCanceled(
        IConfigDb* pReqCtx_, gint32 iRet,
        const std::string& strKrb5Name /*[ In ]*/ ) override
    {
        DebugPrintEx( logErr, iRet,
            "request 'GetUidByKrb5Name' is canceled." );
        return STATUS_SUCCESS;
    }
    //RPC Async Req Handler
    gint32 GetUidByKrb5Name(
        IConfigDb* pReqCtx_,
        const std::string& strKrb5Name /*[ In ]*/,
        gint32& dwUid /*[ Out ]*/ ) override;
    //RPC Async Req Cancel Handler
    gint32 OnGetUidByUserNameCanceled(
        IConfigDb* pReqCtx_, gint32 iRet,
        const std::string& strUserName /*[ In ]*/ ) override
    {
        DebugPrintEx( logErr, iRet,
            "request 'GetUidByUserName' is canceled." );
        return STATUS_SUCCESS;
    }
    //RPC Async Req Handler
    gint32 GetUidByUserName(
        IConfigDb* pReqCtx_,
        const std::string& strUserName /*[ In ]*/,
        gint32& dwUid /*[ Out ]*/ ) override;
    //RPC Async Req Cancel Handler
    gint32 OnGetPasswordSaltCanceled(
        IConfigDb* pReqCtx_, gint32 iRet,
        gint32 dwUid /*[ In ]*/ ) override
    {
        DebugPrintEx( logErr, iRet,
            "request 'GetPasswordSalt' is canceled." );
        return STATUS_SUCCESS;
    }
    //RPC Async Req Handler
    gint32 GetPasswordSalt(
        IConfigDb* pReqCtx_,
        gint32 dwUid /*[ In ]*/,
        std::string& strSalt /*[ Out ]*/ ) override;
    gint32 CreateStmSkel(
        HANDLE, guint32, InterfPtr& ) override;
    
    gint32 OnPreStart(
        IEventSink* pCallback ) override;
};

class CSimpleAuth_ChannelSvr
    : public CRpcStreamChannelSvr
{
    public:
    typedef CRpcStreamChannelSvr super;
    CSimpleAuth_ChannelSvr(
        const IConfigDb* pCfg ) :
        super::virtbase( pCfg ), super( pCfg )
    { SetClassId( clsid(
        CSimpleAuth_ChannelSvr ) ); }
};

