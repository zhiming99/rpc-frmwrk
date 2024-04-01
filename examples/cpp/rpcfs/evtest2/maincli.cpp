// GENERATED BY RIDLC, MAKE SURE TO BACKUP BEFORE RUNNING RIDLC AGAIN
// Copyright (C) 2024  zhiming <woodhead99@gmail.com>
// This program can be distributed under the terms of the GNU GPLv3.
// ridlc -s -O . ../../../evtest.ridl 
#include "rpc.h"
#include "proxy.h"
using namespace rpcf;
#include "stmport.h"
#include "fastrpc.h"
#include "EventTestcli.h"

ObjPtr g_pIoMgr;

FactoryPtr InitClassFactory()
{
    BEGIN_FACTORY_MAPS;

    INIT_MAP_ENTRYCFG( CEventTest_CliImpl );
    INIT_MAP_ENTRYCFG( CEventTest_CliSkel );
    INIT_MAP_ENTRYCFG( CEventTest_ChannelCli );
    
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

gint32 InitContext()
{
    gint32 ret = CoInitialize( 0 );
    if( ERROR( ret ) )
        return ret;
    do{
        // load class factory for 'evtest'
        FactoryPtr p = InitClassFactory();
        ret = CoAddClassFactory( p );
        if( ERROR( ret ) )
            break;
        
        CParamList oParams;
        oParams.Push( "evtestcli" );

        // adjust the thread number if necessary
        oParams[ propMaxIrpThrd ] = 0;
        oParams[ propMaxTaskThrd ] = 1;
        
        ret = g_pIoMgr.NewObj(
            clsid( CIoManager ), 
            oParams.GetCfg() );
        if( ERROR( ret ) )
            break;

        CIoManager* pSvc = g_pIoMgr;
        ret = pSvc->Start();
        
    }while( 0 );

    return ret;
}

gint32 DestroyContext()
{
    IService* pSvc = g_pIoMgr;
    if( pSvc != nullptr )
    {
        pSvc->Stop();
        g_pIoMgr.Clear();
    }

    CoUninitialize();
    DebugPrintEx( logErr, 0,
        "#Leaked objects is %d",
        CObjBase::GetActCount() );
    return STATUS_SUCCESS;
}

gint32 maincli(
    CEventTest_CliImpl* pIf,
    int argc, char** argv );

int main( int argc, char** argv)
{
    gint32 ret = 0;
    do{
        std::string strDesc = "./evtestdesc.json";
        ret = InitContext();
        if( ERROR( ret ) )
            break;
        
        CRpcServices* pSvc = nullptr;
        InterfPtr pIf;
        do{
            CParamList oParams;
            oParams[ propIoMgr ] = g_pIoMgr;
            
            ret = CRpcServices::LoadObjDesc(
                strDesc, "EventTest",
                false, oParams.GetCfg() );
            if( ERROR( ret ) )
                break;
            ret = pIf.NewObj(
                clsid( CEventTest_CliImpl ),
                oParams.GetCfg() );
            if( ERROR( ret ) )
                break;
            pSvc = pIf;
            ret = pSvc->Start();
            if( ERROR( ret ) )
                break;
            while( pSvc->GetState()== stateRecovery )
                sleep( 1 );
            
            if( pSvc->GetState() != stateConnected )
            {
                ret = ERROR_STATE;
                break;
            }
        }while( 0 );
        
        if( SUCCEEDED( ret ) )
            ret = maincli( pIf, argc, argv );
            
        // Stopping the object
        if( !pIf.IsEmpty() )
            pIf->Stop();
    }while( 0 );

    DestroyContext();
    return ret;
}

//-----Your code begins here---
gint32 maincli(
    CEventTest_CliImpl* pIf,
    int argc, char** argv )
{
    //-----Adding your code here---
    while( true )
        sleep( 3 );
    return STATUS_SUCCESS;
}

