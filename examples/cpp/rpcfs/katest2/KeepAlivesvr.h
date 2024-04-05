// Generated by ridlc
// ridlc -s -O . ../../katest.ridl 
// Your task is to implement the following classes
// to get your rpc server work
#pragma once
#include "katest.h"

#define Clsid_CKeepAlive_SvrBase    Clsid_Invalid

DECLARE_AGGREGATED_SERVER(
    CKeepAlive_SvrBase,
    CStatCounters_SvrBase,
    IIKeepAlive_SvrApi,
    CFastRpcServerBase );

class CKeepAlive_SvrImpl
    : public CKeepAlive_SvrBase
{
    public:
    typedef CKeepAlive_SvrBase super;
    CKeepAlive_SvrImpl( const IConfigDb* pCfg ) :
        super::virtbase( pCfg ), super( pCfg )
    { SetClassId( clsid(CKeepAlive_SvrImpl ) ); }

    gint32 OnPostStart(
        IEventSink* pCallback ) override
    {
        StartQpsTask();
        return super::OnPostStart( pCallback );
    }

    gint32 OnPreStop(
        IEventSink* pCallback ) override
    {
        StopQpsTask();
        return super::OnPreStop( pCallback );
    }

    gint32 LongWaitCb(
        IEventSink*, IConfigDb* pReqCtx_,
        const std::string& i0 );

    // IKeepAlive
    virtual gint32 LongWait(
        IConfigDb* pReqCtx_,
        const std::string& i0 /*[ In ]*/,
        std::string& i0r /*[ Out ]*/ );
    
    // RPC Async Req Cancel Handler
    // Rewrite this method to release the resources
    gint32 OnLongWaitCanceled(
        IConfigDb* pReqCtx,
        gint32 iRet,
        const std::string& i0 /*[ In ]*/ );
    gint32 CreateStmSkel(
        HANDLE, guint32, InterfPtr& ) override;
    
    gint32 OnPreStart(
        IEventSink* pCallback ) override;
};

class CKeepAlive_ChannelSvr
    : public CRpcStreamChannelSvr
{
    public:
    typedef CRpcStreamChannelSvr super;
    CKeepAlive_ChannelSvr(
        const IConfigDb* pCfg ) :
        super::virtbase( pCfg ), super( pCfg )
    { SetClassId( clsid(
        CKeepAlive_ChannelSvr ) ); }
};

