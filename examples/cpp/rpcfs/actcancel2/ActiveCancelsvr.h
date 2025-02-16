// GENERATED BY RIDLC, MAKE SURE TO BACKUP BEFORE RUNNING RIDLC AGAIN
// Copyright (C) 2024  zhiming <woodhead99@gmail.com>
// This program can be distributed under the terms of the GNU GPLv3.
// ridlc -s -O . ../../../actcancel.ridl 
#pragma once
#include "actcancel.h"

#include "IActiveCancelsvr.h"

#define Clsid_CActiveCancel_SvrSkel_Base Clsid_Invalid

DECLARE_AGGREGATED_SKEL_SERVER(
    CActiveCancel_SvrSkel_Base,
    CStatCountersServerSkel,
    IIActiveCancel_SImpl );

class CActiveCancel_SvrSkel :
    public CActiveCancel_SvrSkel_Base
{
    public:
    typedef CActiveCancel_SvrSkel_Base super;
    CActiveCancel_SvrSkel( const IConfigDb* pCfg ):
        super::virtbase( pCfg ), super( pCfg )
    { SetClassId( clsid( CActiveCancel_SvrSkel ) ); }

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

#define Clsid_CActiveCancel_SvrBase    Clsid_Invalid

DECLARE_AGGREGATED_SERVER(
    CActiveCancel_SvrBase,
    CStatCounters_SvrBase,
    IIActiveCancel_SvrApi,
    CFastRpcServerBase );

class CActiveCancel_SvrImpl
    : public CActiveCancel_SvrBase
{
    public:
    typedef CActiveCancel_SvrBase super;
    CActiveCancel_SvrImpl( const IConfigDb* pCfg ) :
        super::virtbase( pCfg ), super( pCfg )
    { SetClassId( clsid(CActiveCancel_SvrImpl ) ); }

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


    gint32 LongWaitCb(
        IEventSink*, IConfigDb* pReqCtx_,
        const std::string& i0 );
    
    //RPC Async Req Cancel Handler
    gint32 OnLongWaitCanceled(
        IConfigDb* pReqCtx_, gint32 iRet,
        const std::string& i0 ) override;

    //RPC Async Req Handler
    gint32 LongWait(
        IConfigDb* pReqCtx_,
        const std::string& i0 /*[ In ]*/,
        std::string& i0r /*[ Out ]*/ ) override;
    gint32 CreateStmSkel(
        HANDLE, guint32, InterfPtr& ) override;
    
    gint32 OnPreStart(
        IEventSink* pCallback ) override;
};

class CActiveCancel_ChannelSvr
    : public CRpcStreamChannelSvr
{
    public:
    typedef CRpcStreamChannelSvr super;
    CActiveCancel_ChannelSvr(
        const IConfigDb* pCfg ) :
        super::virtbase( pCfg ), super( pCfg )
    { SetClassId( clsid(
        CActiveCancel_ChannelSvr ) ); }
};
