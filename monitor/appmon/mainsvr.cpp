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
#include "AppMonitorsvr.h"
#include "blkalloc.h"

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
static std::atomic< bool > g_bExit = {false};
static std::atomic< bool > g_bRestart = {false};
static InterfPtr g_pSvcIf;

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

    INIT_MAP_ENTRYCFG( CAppMonitor_SvrImpl );
    INIT_MAP_ENTRYCFG( CAppMonitor_SvrSkel );
    INIT_MAP_ENTRYCFG( CAppMonitor_ChannelSvr );
    
    INIT_MAP_ENTRY( TimeSpec );
    INIT_MAP_ENTRY( FileStat );
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

gint32 ServiceMain( CAppMonitor_SvrImpl* pIf );

gint32 RunSvcObj()
{
    gint32 ret = 0;
    do{
        std::string strDesc = "./appmondesc.json";
        
        CRpcServices* pSvc = nullptr;
        InterfPtr pIf;
        do{
            CParamList oParams;
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
            if( ERROR( ret ) )
                break;
            if( pSvc->GetState()!= stateConnected )
            {
                ret = ERROR_STATE;
                break;
            }
        }while( 0 );
        if( ERROR( ret ) )
            break;
        if( !g_bFuse ) 
            ret = ServiceMain( pIf );
        else
        {
            g_pSvcIf = pIf;
            break;
        }
        // Stopping the object
        if( !pIf.IsEmpty() )
            pIf->Stop();
    }while( 0 );

    return ret;
}

gint32 ServiceMain( CAppMonitor_SvrImpl* pIf )
{
    gint32 ret = 0;
    signal( SIGINT, SignalHandler );
    signal( SIGQUIT, SignalHandler );
    signal( SIGHUP, SignalHandler );
    while( pIf->IsConnected() && !g_bExit )
        sleep( 1 );
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
            if( !g_pSvcIf.IsEmpty() )
            {
                g_pSvcIf->Stop();
                g_pSvcIf.Clear();
            }
        }
#endif
    }while( 0 );
    if( true )
    {
        if( dwStop > 0 )
        {
            g_pUserRegfs->Stop();
            if( ret > 0 )
                ret = -ret;
            g_pUserRegfs.Clear();
        }
        if( dwStop > 1 )
        {
            g_pAppRegfs->Stop();
            if( ret > 0 )
                ret = -ret;
            g_pAppRegfs.Clear();
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
        "\t [ -d to run as a daemon ]\n"
        "\t [ -g send logs to log server ]\n"
        "\t [ -i FORMAT the app-reg file]\n"
        "\t [ -u to enable fuse to dump debug information ]\n"
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
                "you may want to format it first" );
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
                "file, you may want to format it "
                "first" );
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
            stdstr strCmd; 
            for( int i = 0; i < argc; i++ )
            {
                strCmd.append( argv[ i ] );
                strCmd.append( " " );
            }
            system( strCmd.c_str() );
        }
        break;
    }while( true );
    return ret;
}
