// Generated by ridlc
#include "rpc.h"
#include "proxy.h"
using namespace rpcf;
#include "StressSvccli.h"

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
        oParams.Push( "StressTestcli" );

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


FactoryPtr InitClassFactory()
{
    BEGIN_FACTORY_MAPS;

    INIT_MAP_ENTRYCFG( CStressSvc_CliImpl );
    
    
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

gint32 maincli(
    CStressSvc_CliImpl* pIf,
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
            false, oParams.GetCfg() );
        if( ERROR( ret ) )
            break;
        
        ret = pIf.NewObj(
            clsid( CStressSvc_CliImpl ),
            oParams.GetCfg() );
        if( ERROR( ret ) )
            break;
        
        CStressSvc_CliImpl* pSvc = pIf;
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
        
        // Stopping the object
        ret = pSvc->Stop();
        
    }while( 0 );

    DestroyContext();
    return ret;
}

//-----Your code begins here---
extern std::atomic< guint32 > g_dwCounter;
std::atomic< bool > g_bQueueFull( false );
std::atomic< guint32 > g_dwReqs( 0 );
gint32 g_iConcurrency = 0;

gint32 echocli(
    CStressSvc_CliImpl* pIf )
{
    gint32 ret = 0;

    for( int i = 0; i < g_iConcurrency; i++ )
    {
        CParamList oParams;
        guint32 dwIdx = ++g_dwCounter;
        oParams.Push( dwIdx );

        stdstr strMsg = "Hello, Server ";
        strMsg += std::to_string( dwIdx );

        DebugPrint( 0, "req %d, pending %d, thread %d",
             dwIdx, ( guint32 )g_dwReqs,
             rpcf::GetTid() );

        stdstr strResp;
        ret = pIf->Echo(
             oParams.GetCfg(), strMsg, strResp );
        if( ERROR( ret ) )
              break;

        if( ret == STATUS_PENDING )
            g_dwReqs++;
    }
    while( true )
    {
        DebugPrint( 0, "pending %d, thread %d",
             ( guint32 )g_dwReqs, rpcf::GetTid() );
        sleep( 20 );
    }
    return STATUS_SUCCESS;
}

gint32 pingcli(
    CStressSvc_CliImpl* pIf )
{
    gint32 ret = 0;

    for( int i = 0; i < 10000; i++ )
    {
        CParamList oParams;
        guint32 dwIdx = ++g_dwCounter;
        oParams.Push( dwIdx );

        stdstr strMsg = "Hello, Server ";
        strMsg += std::to_string( dwIdx );

        DebugPrint( 0, "req %d, pending %d, thread %d",
             dwIdx, ( guint32 )g_dwReqs,
             rpcf::GetTid() );

        ret = pIf->Ping( strMsg );

        if( ret == STATUS_PENDING )
            g_dwReqs++;
    }
    while( true )
    {
        DebugPrint( 0, "pending %d, thread %d",
             ( guint32 )g_dwReqs, rpcf::GetTid() );
        sleep( 20 );
    }
    return STATUS_SUCCESS;
}

gint32 usage( const char* cmdline )
{
    fprintf( stderr,
        "Usage: -[pe] %s \n" 
        "\t -p no-reply call test, 10000 cases in parallel\n"
        "\t -e <count> continously echo test with count echos in parallel\n",
        cmdline );
    return 0;
}

gint32 maincli(
    CStressSvc_CliImpl* pIf,
    int argc, char** argv )
{
    int opt = 0;
    int cmd = -1;
    int ret = 0;
    while( ( opt = getopt(
            argc, argv, "pe:" ) ) != -1 )
    {
        ret = 0;
        switch (opt)
        {
        case 'p':
            {
                cmd = 0;
                break;
            }
        case 'e':
            {
                g_iConcurrency = atoi( optarg );
                cmd = 1;
                break;
            }
        default: /*  '?' */
            cmd = 3;
            break;
        }
    }
    if( cmd == 0 )
        ret = pingcli( pIf );
    else if( cmd == 1 )
        ret = echocli( pIf );
    else
        ret = usage( argv[ 0 ] );

    return ret;
}
