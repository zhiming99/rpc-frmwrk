/*
 * =====================================================================================
 *
 *       Filename:  mainsvr.cpp
 *
 *    Description:  declarations of application monitor's main function and
 *    global functions.
 *
 *        Version:  1.0
 *        Created:  01/09/2025 03:00:45 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi(woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2024 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#ifdef __FreeBSD__
#include <sys/socket.h>
#include <sys/un.h>
#endif

#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>
#include "rpc.h"
#include "proxy.h"
using namespace rpcf;
#include "stmport.h"
#include "fastrpc.h"
#include "blkalloc.h"
#include "AppManagersvr.h"
#include "AppMonitorsvr.h"
#include "SimpleAuthsvr.h"

#define USER_REGISTRY   "usereg.dat"
#define APP_REGISTRY    "appreg.dat"
static stdstr g_strMPoint;
static bool g_bFuse = false;

RegFsPtr g_pUserRegfs;
RegFsPtr g_pAppRegfs;

static stdstr g_strUserReg;
static stdstr g_strAppReg; 

ObjPtr g_pIoMgr;
std::set< guint32 > g_setMsgIds;
static bool g_bLogging = false;
static bool g_bFormat = false;
static bool g_bLocal = false;
stdstr g_strCmd;
static std::atomic< bool > g_bExit = {false};
static std::atomic< bool > g_bRestart = {false};
std::vector< InterfPtr > g_vecIfs;
static stdrmutex g_oRegLock;

extern gint32 StartLocalAppMancli();
extern gint32 StopLocalAppMancli();

const stdstr& GetMountPoint()
{ return g_strMPoint; }

RegFsPtr GetRegFs( bool bUser )
{
    CStdRMutex oLock( g_oRegLock );
    if( bUser )
        return g_pUserRegfs;
    return g_pAppRegfs;
}

void SetRegFs( RegFsPtr& pRegfs, bool bUser )
{
    CStdRMutex oLock( g_oRegLock );
    if( bUser )
        g_pUserRegfs = pRegfs;
    else
        g_pAppRegfs = pRegfs;
}

void SignalHandler( int signum )
{
    if( signum == SIGINT )
    {
        g_bExit = true;
    }
    else if( signum == SIGHUP )
    {
        g_bExit = true;
        g_bRestart = true;
    }
}

FactoryPtr InitClassFactory()
{
    BEGIN_FACTORY_MAPS;

    INIT_MAP_ENTRYCFG( CAppManager_SvrImpl );
    INIT_MAP_ENTRYCFG( CAppManager_SvrSkel );
    INIT_MAP_ENTRYCFG( CAppManager_ChannelSvr );
    INIT_MAP_ENTRYCFG( CAppMonitor_SvrImpl );
    INIT_MAP_ENTRYCFG( CAppMonitor_SvrSkel );
    INIT_MAP_ENTRYCFG( CAppMonitor_ChannelSvr );
    INIT_MAP_ENTRYCFG( CSimpleAuth_SvrImpl );
    INIT_MAP_ENTRYCFG( CSimpleAuth_SvrSkel );
    INIT_MAP_ENTRYCFG( CSimpleAuth_ChannelSvr );
    
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

gint32 InitContext()
{
    gint32 ret = CoInitialize( 0 );
    if( ERROR( ret ) )
        return ret;
    do{
        // load class factory for 'appmon'
        FactoryPtr p = InitClassFactory();
        ret = CoAddClassFactory( p );
        if( ERROR( ret ) )
            break;
        
        CParamList oParams;
        oParams.Push( "appmonsvr" );

        oParams[ propEnableLogging ] = g_bLogging;
        oParams[ propSearchLocal ] = g_bLocal;
        oParams[ propConfigPath ] =
            "invalidpath/driver.json";

        // adjust the thread number if necessary
        oParams[ propMaxIrpThrd ] = 0;
        oParams[ propMaxTaskThrd ] = 4;

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

gint32 ServiceMain(
    CAppManager_SvrImpl* pIf,
    CAppMonitor_SvrImpl* pIf1,
    CSimpleAuth_SvrImpl* pIf2 );

gint32 RunSvcObj()
{
    gint32 ret = 0;
    do{
        std::string strDesc =
            "invalidpath/appmondesc.json";
        
        CRpcServices* pSvc = nullptr;
        InterfPtr pIf;
        std::vector< InterfPtr > vecIfs;
        do{
            CParamList oParams;
            oParams[ propIoMgr ] = g_pIoMgr;
            
            ret = CRpcServices::LoadObjDesc(
                strDesc, "AppManager",
                true, oParams.GetCfg() );
            if( ERROR( ret ) )
                break;
            ret = pIf.NewObj(
                clsid( CAppManager_SvrImpl ),
                oParams.GetCfg() );
            if( ERROR( ret ) )
                break;
            pSvc = pIf;
            ret = pSvc->Start();
            vecIfs.push_back( pIf );
            if( ERROR( ret ) )
                break;
            if( pSvc->GetState()!= stateConnected )
            {
                ret = ERROR_STATE;
                break;
            }

            oParams.Clear();
            oParams[ propIoMgr ] = g_pIoMgr;
            
            ret = CRpcServices::LoadObjDesc(
                strDesc, "AppMonitor",
                true, oParams.GetCfg() );
            if( ERROR( ret ) )
                break;
            ret = pIf.NewObj(
                clsid( CAppMonitor_SvrImpl ),
                oParams.GetCfg() );
            if( ERROR( ret ) )
                break;
            pSvc = pIf;
            ret = pSvc->Start();
            vecIfs.push_back( pIf );
            if( ERROR( ret ) )
                break;
            if( pSvc->GetState()!= stateConnected )
            {
                ret = ERROR_STATE;
                break;
            }

            oParams.Clear();
            oParams[ propIoMgr ] = g_pIoMgr;
            
            ret = CRpcServices::LoadObjDesc(
                strDesc, "SimpleAuth",
                true, oParams.GetCfg() );
            if( ERROR( ret ) )
                break;
            ret = pIf.NewObj(
                clsid( CSimpleAuth_SvrImpl ),
                oParams.GetCfg() );
            if( ERROR( ret ) )
                break;
            pSvc = pIf;
            ret = pSvc->Start();
            vecIfs.push_back( pIf );
            if( ERROR( ret ) )
                break;
            if( pSvc->GetState()!= stateConnected )
            {
                ret = ERROR_STATE;
                break;
            }
        }while( 0 );
        
        g_vecIfs = vecIfs;
        if( ERROR( ret ) )
            break;

        ret = StartLocalAppMancli();
        if( ERROR( ret ) )
            break;
        if( !g_bFuse ) 
        {
            ret = ServiceMain(
                vecIfs[0],
                vecIfs[1],
                vecIfs[2]);
        }

    }while( 0 );

    if( ERROR( ret ) )
    {
        // Stopping the objects
        for( auto& pInterf : g_vecIfs )
            pInterf->Stop();
        g_vecIfs.clear();
    }

    return ret;
}

gint32 ServiceMain(
    CAppManager_SvrImpl* pIf,
    CAppMonitor_SvrImpl* pIf1,
    CSimpleAuth_SvrImpl* pIf2 )
{
    gint32 ret = 0;
    signal( SIGINT, SignalHandler );
    signal( SIGQUIT, SignalHandler );
    signal( SIGHUP, SignalHandler );
    while( !g_bExit )
    {
        if( !pIf->IsConnected() ||
            !pIf1->IsConnected() ||
            !pIf2->IsConnected() )
            break;
        sleep( 1 );
    }
    ret = STATUS_SUCCESS;
    return ret;
}

extern gint32 FuseMain( int argc, char** argv );

int _main( int argc, char** argv)
{
    gint32 ret = 0;
    gint32 dwStop = 0;
    do{ 
        CParamList oParams;
        oParams.SetStrProp(
            propConfigPath, g_strUserReg );
        ret = g_pUserRegfs.NewObj(
            clsid( CRegistryFs ),
            oParams.GetCfg() );
        if( ERROR( ret ) )
            break;

        ret = g_pUserRegfs->Start();
        if( ERROR( ret ) )
            break;

        dwStop++;
        CParamList oParams2;
        oParams.SetStrProp(
            propConfigPath, g_strAppReg );
        ret = g_pAppRegfs.NewObj(
            clsid( CRegistryFs ),
            oParams.GetCfg() );
        if( ERROR( ret ) )
            break;

        dwStop++;
        ret = g_pAppRegfs->Start();
        if( ERROR( ret ) )
            break;
        
        ret = RunSvcObj();
        if( ERROR( ret ) )
            break;
#if FUSE3
        if( g_bFuse )
        {
            if( ERROR( ret ) )
                break;
            ret = FuseMain( argc, argv );
            for( auto pIf : g_vecIfs )
                pIf->Stop();
            g_vecIfs.clear();
        }
#endif
        StopLocalAppMancli();

    }while( 0 );
    if( true )
    {
        RegFsPtr pEmpty;
        if( dwStop > 0 )
        {
            RegFsPtr pRegfs = GetRegFs( true );
            SetRegFs( pEmpty, true );
            if( !pRegfs.IsEmpty() )
            {
                pRegfs->Stop();
                pRegfs.Clear();
            }
            if( ret > 0 )
                ret = -ret;
        }
        if( dwStop > 1 )
        {
            RegFsPtr pRegfs = GetRegFs( false );
            SetRegFs( pEmpty, false );
            if( !pRegfs.IsEmpty() )
            {
                pRegfs->Stop();
                pRegfs.Clear();
            }
            if( ret > 0 )
                ret = -ret;
        }
    }

    if( ERROR( ret ) )
    {
        OutputMsg( ret, "Warning, regfs quitting "
            "with error" );
    }

    return ret;
}

static void Usage( const char* szName )
{
    fprintf( stderr,
        "Usage: %s [OPTIONS] <mount point> \n"
        "\t [ -d Run as a daemon ]\n"
        "\t [ -g Send logs to log server ]\n"
        "\t [ -i FORMAT the app-reg file]\n"
        "\t [ -u Enable fuse to dump debug information ]\n"
        "\t [ -l Search first the current directory for configuration files ]\n"
        "\t [ -h this help ]\n", szName );
}

static gint32 RunMain(
    bool bDaemon, int argc, char** argv )
{
    gint32 ret = 0;
    do{
        if( bDaemon )
        {
            ret = daemon( 1, 0 );
            if( ret < 0 )
            { ret = -errno; break; }
        }
        ret = InitContext();
        if( ERROR( ret ) ) 
        {
            DestroyContext();
            OutputMsg( ret, "Error start regfsmnt" );
            break;
        }
        ret = _main( argc, argv );

        DestroyContext();
    }while( 0 );
    return ret;
}

int main( int argc, char** argv)
{
    int ret = 0;
    do{
        bool bDaemon = false;
        bool bDebug = false;
        int opt = 0;
        while( ( opt = getopt( argc, argv, "hgdiul" ) ) != -1 )
        {
            switch( opt )
            {
                case 'd':
                    { bDaemon = true; break; }
                case 'g':
                    { g_bLogging = true; break; }
                case 'i':
                    { g_bFormat = true; break; }
                case 'u':
                    { bDebug = true; break; }
                case 'l':
                    { g_bLocal = true; break; }
                case 'h':
                default:
                    { Usage( argv[ 0 ] ); exit( 0 ); }
            }
        }
        if( ERROR( ret ) )
            break;

        g_strCmd.clear();
        for( int i = 0; i < argc; i++ )
        {
            g_strCmd.append( argv[ i ] );
            if( i < argc - 1 )
                g_strCmd.append( " " );
        }

        stdstr strHomeDir = GetHomeDir();
        g_strUserReg =
            strHomeDir + "/.rpcf/" USER_REGISTRY;
        ret = access( g_strUserReg.c_str(),
            R_OK | W_OK );
        if( ret == -1 )
        {
            ret = -errno;
            OutputMsg( ret,
                "Error invalid user registry file, "
                "you may want to format '%s' first with "
                "'inituser.sh' command",
                g_strUserReg.c_str() );
            break;
        }

        g_strAppReg =
            strHomeDir + "/.rpcf/" APP_REGISTRY;
        ret = access( g_strAppReg.c_str(),
            R_OK | W_OK );
        if( ret == -1 )
        {
            ret = -errno;
            OutputMsg( ret,
                "Error invalid application registry "
                "file '%s', you may want to format it "
                "first with 'initappreg.sh' command",
                g_strAppReg.c_str() );
            break;
        }

#if FUSE3
        if( optind < argc )
        {
            g_strMPoint = argv[ optind ];
            if( g_strMPoint.size() > REG_MAX_PATH - 1 )
            {
                ret = -ENAMETOOLONG;
                OutputMsg( ret,
                    "Error invalid mount point" );
                break;
            }
        }
        if( g_strMPoint.size() )
        {
            g_bFuse = true;
            struct stat sb;
            if( lstat( g_strMPoint.c_str(), &sb ) == -1 )
            {
                ret = mkdir( g_strMPoint.c_str(), 0755 );
                if( ret < 0 )
                {
                    OutputMsg( ret,
                        "Error create the directory for"
                        "mount point: %s.",
                        strerror( errno) );
                    ret = -errno;
                    break;
                }
            }
            else
            {
                mode_t iFlags =
                    ( sb.st_mode & S_IFMT );

                if( iFlags != S_IFDIR )
                {
                    ret = -EINVAL;
                    OutputMsg( ret,
                        "Error mount point is not a "
                        "directory" );
                    Usage( argv[ 0 ] );
                    ret = -errno;
                    break;
                }
            }

            std::vector< std::string > strArgv;
            strArgv.push_back( argv[ 0 ] );
            strArgv.push_back( "-f" );
            if( bDebug )
                strArgv.push_back( "-d" );
            strArgv.push_back( g_strMPoint );
            int argcf = strArgv.size();
            char* argvf[ 100 ];
            size_t dwCount = std::min(
                strArgv.size(),
                sizeof( argvf ) / sizeof( argvf[ 0 ] ) );
            BufPtr pArgBuf( true );
            size_t dwOff = 0;
            for( size_t i = 0; i < dwCount; i++ )
            {
                ret = pArgBuf->Append(
                    strArgv[ i ].c_str(),
                    strArgv[ i ].size() + 1 );
                if( ERROR( ret ) )
                    break;
                argvf[ i ] = ( char* )dwOff;
                dwOff += strArgv[ i ].size() + 1;
            }
            if( ERROR( ret ) )
                break;
            for( size_t i = 0; i < dwCount; i++ )
                argvf[ i ] += ( intptr_t )pArgBuf->ptr();
            ret = RunMain( bDaemon, argcf, argvf );
        }
        else
#endif
        {
            ret = RunMain( bDaemon, argc, argv );
        }
        if( g_bRestart )
        {
            system( g_strCmd.c_str() );
        }
        break;
    }while( true );
    return ret;
}
