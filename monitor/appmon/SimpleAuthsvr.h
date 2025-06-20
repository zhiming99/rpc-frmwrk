// GENERATED BY RIDLC, MAKE SURE TO BACKUP BEFORE RUNNING RIDLC AGAIN
// Copyright (C) 2024  zhiming <woodhead99@gmail.com>
// This program can be distributed under the terms of the GNU GPLv3.
// ../../ridl/.libs/ridlc --server -sO . ./appmon.ridl 
#pragma once
#include "appmon.h"

#include "IUserManagersvr.h"

#define Clsid_CSimpleAuth_SvrSkel_Base Clsid_Invalid

DECLARE_AGGREGATED_SKEL_SERVER(
    CSimpleAuth_SvrSkel_Base,
    CStatCountersServerSkel,
    IIUserManager_SImpl );

class CSimpleAuth_SvrSkel :
    public CSimpleAuth_SvrSkel_Base
{
    public:
    typedef CSimpleAuth_SvrSkel_Base super;
    CSimpleAuth_SvrSkel( const IConfigDb* pCfg ):
        super::virtbase( pCfg ), super( pCfg )
    { SetClassId( clsid( CSimpleAuth_SvrSkel ) ); }

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
    //RPC Async Req Cancel Handler
    gint32 OnCheckClientTokenCanceled(
        IConfigDb* pReqCtx_, gint32 iRet,
        ObjPtr& oClientInfo /*[ In ]*/,
        ObjPtr& oSvrInfo /*[ In ]*/ ) override
    {
        DebugPrintEx( logErr, iRet,
            "request 'CheckClientToken' is canceled." );
        return STATUS_SUCCESS;
    }
    //RPC Async Req Handler
    gint32 CheckClientToken(
        IConfigDb* pReqCtx_,
        ObjPtr& oClientInfo /*[ In ]*/,
        ObjPtr& oSvrInfo /*[ In ]*/,
        ObjPtr& oInfo /*[ Out ]*/ ) override;
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

