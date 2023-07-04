// GENERATED BY RIDLC, MAKE SURE TO BACKUP BEFORE RUNNING RIDLC AGAIN
// Copyright (C) 2023  zhiming <woodhead99@gmail.com>
// This program can be distributed under the terms of the GNU GPLv3.
// ridlc -bsO . ../../testypes.ridl 
#include "rpc.h"
#include "proxy.h"
using namespace rpcf;
#include "stmport.h"
#include "fastrpc.h"
#include "TestTypesSvccli.h"

ObjPtr g_pIoMgr;


FactoryPtr InitClassFactory()
{
    BEGIN_FACTORY_MAPS;

    INIT_MAP_ENTRYCFG( CTestTypesSvc_CliImpl );
    INIT_MAP_ENTRYCFG( CTestTypesSvc_CliSkel );
    INIT_MAP_ENTRYCFG( CTestTypesSvc_ChannelCli );
    
    INIT_MAP_ENTRY( FILE_INFO );
    
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

static std::string g_strDrvPath;
static std::string g_strObjDesc;
static std::string g_strRtDesc;
static bool g_bAuth = false;
static ObjPtr g_pRouter;
char g_szKeyPass[ SSL_PASS_MAX + 1 ] = {0};

namespace rpcf{
extern "C" gint32 CheckForKeyPass(
    bool& bPrompt );
}
void Usage( char* szName )
{
    fprintf( stderr,
        "Usage: %s [OPTION]\n"
        "\t [ -a to enable authentication ]\n"
        "\t [ -d to run as a daemon ]\n"
        "\t [ --driver <path> to specify the path to the customized 'driver.json'. ]\n"
        "\t [ --objdesc <path> to specify the path to the object description file. ]\n"
        "\t [ --router <path> to specify the path to the customized 'router.json'. ]\n"
        "\t [ -h this help ]\n", szName );
}

#include <getopt.h>
#include <sys/stat.h>
int _main( int argc, char** argv);
int main( int argc, char** argv )
{
    bool bDaemon = false;
    int opt = 0;
    int ret = 0;
    do{
        gint32 iOptIdx = 0;
        struct option arrLongOptions[] = {
            {"driver",   required_argument, 0,  0 },
            {"objdesc",  required_argument, 0,  0 },
            {"router",   required_argument, 0,  0 },
            {0,             0,                 0,  0 }
        };            
        while( ( opt = getopt_long( argc, argv, "had",
            arrLongOptions, &iOptIdx ) ) != -1 )
        {
            switch( opt )
            {
                case 0:
                {
                    struct stat sb;
                    ret = lstat( optarg, &sb );
                    if( ret < 0 )
                    {
                        perror( strerror( errno ) );
                        ret = -errno;
                        break;
                    }
                    if( ( sb.st_mode & S_IFMT ) != S_IFLNK &&
                        ( sb.st_mode & S_IFMT ) != S_IFREG )
                    {
                        fprintf( stderr, "Error invalid file '%s'.\n", optarg );
                        ret = -EINVAL;
                        break;
                    }
                    if( iOptIdx == 0 )
                        g_strDrvPath = optarg;
                    else if( iOptIdx == 1 )
                        g_strObjDesc = optarg;
                    else if( iOptIdx == 2 )
                        g_strRtDesc = optarg;
                    else
                    {
                        fprintf( stderr, "Error invalid option.\n" );
                        Usage( argv[ 0 ] );
                        ret = -EINVAL;
                    }
                    break;
                }
                case 'a':
                    { g_bAuth = true; break; }
                case 'd':
                    { bDaemon = true; break; }
                case 'h':
                default:
                    { Usage( argv[ 0 ] ); exit( 0 ); }
            }
        }
        if( ERROR( ret ) )
            break;
        if( true )
        {
            bool bPrompt = false;
            bool bExit = false;
            ret = CheckForKeyPass( bPrompt );
            while( SUCCEEDED( ret ) && bPrompt )
            {
                char* pPass = getpass( "SSL Key Password:" );
                if( pPass == nullptr )
                {
                    bExit = true;
                    ret = -errno;
                    break;
                }
                size_t len = strlen( pPass );
                len = std::min(
                    len, ( size_t )SSL_PASS_MAX );
                memcpy( g_szKeyPass, pPass, len );
                break;
            }
            if( bExit )
                break;
        }
        if( bDaemon )
        {
            ret = daemon( 1, 0 );
            if( ret < 0 )
            { ret = -errno; break; }
        }
        ret = _main( argc, argv );
    }while( 0 );
    return ret;
}

gint32 InitContext()
{
    gint32 ret = CoInitialize( 0 );
    if( ERROR( ret ) )
        return ret;
    do{
        // load class factory for 'TestTypes'
        FactoryPtr p = InitClassFactory();
        ret = CoAddClassFactory( p );
        if( ERROR( ret ) )
            break;
        
        CParamList oParams;
        oParams.Push( "TestTypescli" );

        // adjust the thread number if necessary
        guint32 dwNumThrds =
            ( guint32 )std::max( 1U,
            std::thread::hardware_concurrency() );
        if( dwNumThrds > 1 )
            dwNumThrds = ( dwNumThrds >> 1 );
        oParams[ propMaxTaskThrd ] = dwNumThrds;
        oParams[ propMaxIrpThrd ] = 2;
        if( g_strDrvPath.size() )
            oParams[ propConfigPath ] = g_strDrvPath;
        
        ret = g_pIoMgr.NewObj(
            clsid( CIoManager ), 
            oParams.GetCfg() );
        if( ERROR( ret ) )
            break;

        CIoManager* pSvc = g_pIoMgr;
        pSvc->SetCmdLineOpt(
            propRouterRole, 1 );
        ret = pSvc->Start();
        if( ERROR( ret ) )
            break;
        
        // create and start the router
        std::string strRtName = "TestTypes_Router";
        pSvc->SetRouterName( strRtName + "_" +
            std::to_string( getpid() ) );
        stdstr strDescPath;
        if( g_strRtDesc.size() )
            strDescPath = g_strRtDesc;
        else if( g_bAuth )
            strDescPath = "./rtauth.json";
        else
            strDescPath = "./router.json";
        
        if( g_bAuth )
        {
            pSvc->SetCmdLineOpt(
                propHasAuth, g_bAuth );
        }
        CCfgOpener oRtCfg;
        oRtCfg.SetStrProp( propSvrInstName,
            MODNAME_RPCROUTER );
        oRtCfg[ propIoMgr ] = g_pIoMgr;
        CIoManager* pMgr = g_pIoMgr;
        pMgr->SetCmdLineOpt( propSvrInstName,
            MODNAME_RPCROUTER );
        ret = CRpcServices::LoadObjDesc(
            strDescPath,
            OBJNAME_ROUTER,
            true, oRtCfg.GetCfg() );
        if( ERROR( ret ) )
            break;
        oRtCfg[ propIfStateClass ] =
            clsid( CIfRouterMgrState );
        oRtCfg[ propRouterRole ] = 1;
        ret =  g_pRouter.NewObj(
            clsid( CRpcRouterManagerImpl ),
            oRtCfg.GetCfg() );
        if( ERROR( ret ) )
            break;
        CInterfaceServer* pRouter = g_pRouter;
        if( unlikely( pRouter == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }
        ret = pRouter->Start();
        if( ERROR( ret ) )
        {
            pRouter->Stop();
            g_pRouter.Clear();
            break;
        }
    }while( 0 );

    return ret;
}

gint32 DestroyContext()
{
    if( !g_pRouter.IsEmpty() )
    {
        IService* pRt = g_pRouter;
        pRt->Stop();
        g_pRouter.Clear();
    }
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

gint32 maincli( CTestTypesSvc_CliImpl* pIf, int argc, char** argv );

int _main( int argc, char** argv )
{
    gint32 ret = 0;
    do{
        ret = InitContext();
        if( ERROR( ret ) )
            break;
        
        std::string strDesc;
        if( g_strObjDesc.empty() )
            strDesc = "./TestTypesdesc.json";
        else
            strDesc = g_strObjDesc;
        
        CRpcServices* pSvc = nullptr;
        InterfPtr pIf;
        do{
            CParamList oParams;
            oParams.Clear();
            oParams[ propIoMgr ] = g_pIoMgr;
            
            ret = CRpcServices::LoadObjDesc(
                strDesc, "TestTypesSvc",
                false, oParams.GetCfg() );
            if( ERROR( ret ) )
                break;
            ret = pIf.NewObj(
                clsid( CTestTypesSvc_CliImpl ),
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
gint32 maincli( CTestTypesSvc_CliImpl* pIf, int argc, char** argv )
{
    //-----Adding your code here---
    gint32 ret = 0;
    for( int i = 0; i < 1000; i++ )
    {
        stdstr strResp;
        ret = pIf->Echo( "Hello, World", strResp );
        if( ERROR( ret ) )
            break;
        OutputMsg( ret,
            "Server resp( %d ): %s", i, strResp.c_str() );
    }
    return STATUS_SUCCESS;
}

