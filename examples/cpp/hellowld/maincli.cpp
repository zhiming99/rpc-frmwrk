// GENERATED BY RIDLC, MAKE SURE TO BACKUP BEFORE RUNNING RIDLC AGAIN
// Copyright (C) 2024  zhiming <woodhead99@gmail.com>
// This program can be distributed under the terms of the GNU GPLv3.
// ridlc -O ./hellowld ../hellowld.ridl 
#include "rpc.h"
#include "proxy.h"
using namespace rpcf;
#include "HelloWorldSvccli.h"

ObjPtr g_pIoMgr;

FactoryPtr InitClassFactory()
{
    BEGIN_FACTORY_MAPS;

    INIT_MAP_ENTRYCFG( CHelloWorldSvc_CliImpl );
    
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
        // load class factory for 'HelloWorld'
        FactoryPtr p = InitClassFactory();
        ret = CoAddClassFactory( p );
        if( ERROR( ret ) )
            break;
        
        CParamList oParams;
        oParams.Push( "HelloWorldcli" );

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
        if( ERROR( ret ) )
            break;
        
        
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

gint32 maincli( CHelloWorldSvc_CliImpl* pIf, int argc, char** argv );

int main( int argc, char** argv )
{
    gint32 ret = 0;
    do{
        std::string strDesc = "./HelloWorlddesc.json";
        ret = InitContext();
        if( ERROR( ret ) )
            break;
        
        CRpcServices* pSvc = nullptr;
        InterfPtr pIf;
        do{
            CParamList oParams;
            oParams[ propIoMgr ] = g_pIoMgr;
            
            ret = CRpcServices::LoadObjDesc(
                strDesc, "HelloWorldSvc",
                false, oParams.GetCfg() );
            if( ERROR( ret ) )
                break;
            ret = pIf.NewObj(
                clsid( CHelloWorldSvc_CliImpl ),
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
    CHelloWorldSvc_CliImpl* pIf,
    int argc, char** argv )
{
    //-----Adding your code here---
    std::string strResp;
    gint32 ret = pIf->Echo(
        "Hello, World!", strResp );
    if( ERROR( ret ) )
        OutputMsg( ret, "Error Echo request" );
    else
        OutputMsg( ret,
            "Echo request succeeded with resp '%s'",
            strResp.c_str() );

    return ret;
}

