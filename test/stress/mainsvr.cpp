// Generated by ridlc
#include "rpc.h"
#include "proxy.h"
using namespace rpcf;
#include "StressSvcsvr.h"

ObjPtr g_pIoMgr;

extern FactoryPtr InitClassFactory();
gint32 InitContext()
{
    gint32 ret = CoInitialize( 0 );
    if( ERROR( ret ) )
        return ret;
    do{
        // load class factory for 'StressTest'
        FactoryPtr p = InitClassFactory();
        ret = CoAddClassFactory( p );
        if( ERROR( ret ) )
            break;
        
        CParamList oParams;
        oParams.Push( "StressTestsvr" );

        // adjust the thread number if necessary
        oParams[ propMaxIrpThrd ] = 2;
        oParams[ propMaxTaskThrd ] = 2;
        
        ret = g_pIoMgr.NewObj(
            clsid( CIoManager ), 
            oParams.GetCfg() );
        if( ERROR( ret ) )
            break;
        
        IService* pSvc = g_pIoMgr;
        ret = pSvc->Start();
        
    }while( 0 );

    if( ERROR( ret ) )
    {
        g_pIoMgr.Clear();
        CoUninitialize();
    }
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


FactoryPtr InitClassFactory()
{
    BEGIN_FACTORY_MAPS;

    INIT_MAP_ENTRYCFG( CStressSvc_SvrImpl );
    
    
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

gint32 mainsvr( 
    CStressSvc_SvrImpl* pIf,
    int argc, char** argv );

int main( int argc, char** argv)
{
    gint32 ret = 0;
    do{
        ret = InitContext();
        if( ERROR( ret ) )
            break;
        
        InterfPtr pIf;
        CParamList oParams;
        oParams[ propIoMgr ] = g_pIoMgr;
        ret = CRpcServices::LoadObjDesc(
            "./StressTestdesc.json",
            "StressSvc",
            true, oParams.GetCfg() );
        if( ERROR( ret ) )
            break;
        
        ret = pIf.NewObj(
            clsid( CStressSvc_SvrImpl ),
            oParams.GetCfg() );
        if( ERROR( ret ) )
            break;
        
        CStressSvc_SvrImpl* pSvc = pIf;
        ret = pSvc->Start();
        if( ERROR( ret ) )
            break;
        
        mainsvr( pSvc, argc, argv );
        
        // Stopping the object
        ret = pSvc->Stop();
        
    }while( 0 );

    DestroyContext();
    return ret;
}

// -----Your code begins here---
gint32 mainsvr( 
    CStressSvc_SvrImpl* pIf,
    int argc, char** argv )
{
    // replace 'sleep' with your code for
    // advanced control
    guint32 dwCount = 0;
    while( pIf->IsConnected() )
    {
        dwCount++;
        sleep( 20 );
        if( ( dwCount & 0x1 ) == 0 )
        {
            // send a event every 32s
            stdstr strMsg = "hello, there ";
            strMsg += std::to_string( dwCount );
            pIf->OnHelloWorld( strMsg );
        }
    }
    return STATUS_SUCCESS;
}