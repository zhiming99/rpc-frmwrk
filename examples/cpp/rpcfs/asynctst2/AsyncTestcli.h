// GENERATED BY RIDLC, MAKE SURE TO BACKUP BEFORE RUNNING RIDLC AGAIN
// Copyright (C) 2024  zhiming <woodhead99@gmail.com>
// This program can be distributed under the terms of the GNU GPLv3.
// ridlc -s -O . ../../../asynctst.ridl 
#pragma once
#include "asynctst.h"

#include "IAsyncTestcli.h"

DECLARE_AGGREGATED_SKEL_PROXY(
    CAsyncTest_CliSkel,
    CStatCountersProxySkel,
    IIAsyncTest_PImpl );

#define Clsid_CAsyncTest_CliBase    Clsid_Invalid

DECLARE_AGGREGATED_PROXY(
    CAsyncTest_CliBase,
    CStatCounters_CliBase,
    IIAsyncTest_CliApi,
    CFastRpcProxyBase );

class CAsyncTest_CliImpl
    : public CAsyncTest_CliBase
{
    sem_t m_semWait;
    gint32 m_iError = STATUS_SUCCESS;

    public:
    typedef CAsyncTest_CliBase super;
    CAsyncTest_CliImpl( const IConfigDb* pCfg ) :
        super::virtbase( pCfg ), super( pCfg )
    {
        SetClassId( clsid(CAsyncTest_CliImpl ) );
        Sem_Init( &m_semWait, 0, 0 );
    }

    inline void NotifyComplete()
    { Sem_Post( &m_semWait ); }

    inline void WaitForComplete()
    { Sem_Wait( &m_semWait ); }

    inline void SetError( gint32 iError )
    { m_iError = iError; }

    inline gint32 GetError() const
    { return m_iError; }

 // RPC Async Req Callback
    gint32 LongWaitCallback(
        IConfigDb* context, 
        gint32 iRet,const std::string& i0r /*[ In ]*/ ) override;

    // RPC Async Req Callback
    gint32 LongWaitNoParamCallback(
        IConfigDb* context, 
        gint32 iRet ) override;
    gint32 CreateStmSkel(
        InterfPtr& pIf ) override;
    
    gint32 OnPreStart(
        IEventSink* pCallback ) override;
};

class CAsyncTest_ChannelCli
    : public CRpcStreamChannelCli
{
    public:
    typedef CRpcStreamChannelCli super;
    CAsyncTest_ChannelCli(
        const IConfigDb* pCfg ) :
        super::virtbase( pCfg ), super( pCfg )
    { SetClassId( clsid(
        CAsyncTest_ChannelCli ) ); }
};

