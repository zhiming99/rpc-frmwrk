// GENERATED BY RIDLC, MAKE SURE TO BACKUP BEFORE RUNNING RIDLC AGAIN
// Copyright (C) 2024  zhiming <woodhead99@gmail.com>
// This program can be distributed under the terms of the GNU GPLv3.
// ../../ridl/.libs/ridlc -sO . ./appmon.ridl 
#pragma once
#include "appmon.h"

#include "IAppStorecli.h"
#include "IDataProducercli.h"

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
    
    //RPC event handler 'OnPointChanged'
    gint32 OnPointChanged(
        const std::string& strPtPath /*[ In ]*/,
        const Variant& value /*[ In ]*/ ) override;
    //RPC event handler 'OnPointsChanged'
    gint32 OnPointsChanged(
        std::vector<KeyValue>& arrKVs /*[ In ]*/ ) override;
    // RPC Async Req Callback
    gint32 ClaimAppInstsCallback(
        IConfigDb* context, 
        gint32 iRet,
        std::vector<KeyValue>& arrInitKVs /*[ In ]*/ ) override;
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

