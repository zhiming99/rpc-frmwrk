// GENERATED BY RIDLC, MAKE SURE TO BACKUP BEFORE RUNNING RIDLC AGAIN
// Copyright (C) 2024  zhiming <woodhead99@gmail.com>
// This program can be distributed under the terms of the GNU GPLv3.
// ../../../../ridl/.libs/ridlc --sync_mode IAppStore=async_p --sync_mode IUserManager=async_p --services=AppManager,SimpleAuth --client -slO . ../../../../monitor/appmon/appmon.ridl 
#include "rpc.h"
#include "proxy.h"
using namespace rpcf;
#include "stmport.h"
#include "fastrpc.h"
#include "AppManagercli.h"
#include "SimpleAuthcli.h"

extern ObjPtr g_pIoMgr;

FactoryPtr InitClassFactory()
{
    BEGIN_FACTORY_MAPS;

    INIT_MAP_ENTRYCFG( CAppManager_CliImpl );
    INIT_MAP_ENTRYCFG( CAppManager_CliSkel );
    INIT_MAP_ENTRYCFG( CAppManager_ChannelCli );
    INIT_MAP_ENTRYCFG( CSimpleAuth_CliImpl );
    INIT_MAP_ENTRYCFG( CSimpleAuth_CliSkel );
    INIT_MAP_ENTRYCFG( CSimpleAuth_ChannelCli );
    
    INIT_MAP_ENTRY( KeyValue );
    
    END_FACTORY_MAPS;
}

extern "C"
gint32 DllLoadFactory( FactoryPtr& pFactory )
{
    pFactory = InitClassFactory();
    if( pFactory.IsEmpty() )
        return -EFAULT;
    return STATUS_SUCCESS;
}

