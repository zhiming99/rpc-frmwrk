// Generated by ridlc
// ridlc -s -O . ../../katest.ridl 
#include "rpc.h"
#include "proxy.h"
using namespace rpcf;
#include "stmport.h"
#include "fastrpc.h"
#include "KeepAlivecli.h"

ObjPtr g_pIoMgr;


FactoryPtr InitClassFactory()
{
    BEGIN_FACTORY_MAPS;

    INIT_MAP_ENTRYCFG( CKeepAlive_CliImpl );
    INIT_MAP_ENTRYCFG( CKeepAlive_CliSkel );
    INIT_MAP_ENTRYCFG( CKeepAlive_ChannelCli );
    
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
        // load class factory for 'katest'
        FactoryPtr p = InitClassFactory();
        ret = CoAddClassFactory( p );
        if( ERROR( ret ) )
            break;
        
        CParamList oParams;
        oParams.Push( "katestcli" );

        // adjust the thread number if necessary
        oParams[ propMaxIrpThrd ] = 0;
        oParams[ propMaxTaskThrd ] = 1;
        
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

gint32 maincli(
    CKeepAlive_CliImpl* pIf,
    int argc, char** argv );

int main( int argc, char** argv)
{
    gint32 ret = 0;
    do{
        ret = InitContext();
        if( ERROR( ret ) )
            break;
        
        stdstr strDesc = "./katestdesc.json";
        CKeepAlive_CliImpl* pSvc = nullptr;
        InterfPtr pIf;
        do{
            CParamList oParams;
            oParams[ propIoMgr ] = g_pIoMgr;
            
            ret = CRpcServices::LoadObjDesc(
                strDesc, "KeepAlive",
                false, oParams.GetCfg() );
            if( ERROR( ret ) )
                break;
            ret = pIf.NewObj(
                clsid( CKeepAlive_CliImpl ),
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

            maincli( pSvc, argc, argv );
            
        }while( 0 );
        
        // Stopping the object
        if( pSvc != nullptr )
            ret = pSvc->Stop();
    }while( 0 );

    DestroyContext();
    return ret;
}

//-----Your code begins here---
gint32 maincli(
    CKeepAlive_CliImpl* pIf,
    int argc, char** argv )
{
    gint32 ret = 0;
    do{
        stdstr i0 = "Hello, katest!";
        stdstr i0r;
        ret = pIf->LongWait( i0, i0r );
        if( ERROR( ret ) )
        {
            OutputMsg( ret, "LongWait failed with error " );
            break;
        }

        OutputMsg( ret,
            "LongWait completed with response %s",
            i0r.c_str() );

    }while( 0 );

    return ret;
}

