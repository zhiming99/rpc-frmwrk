// GENERATED BY RIDLC, MAKE SURE TO BACKUP BEFORE RUNNING RIDLC AGAIN
// Copyright (C) 2025  zhiming <woodhead99@gmail.com>
// This program can be distributed under the terms of the GNU GPLv3.
// ../../../../ridl/.libs/ridlc -m timer1 --server -sO . ./apptimer.ridl 
#include "rpc.h"
#include "proxy.h"
using namespace rpcf;
#include "stmport.h"
#include "fastrpc.h"
#include "AppManagercli.h"
#include "frmwrk.h"

ObjPtr g_pIoMgr;
#include <signal.h>
#include <stdlib.h>
#include <limits.h>

static std::atomic< bool > g_bExit( false );
static bool g_bLogging = false;

void SignalHandler( int signum )
{
    if( signum == SIGINT )
        g_bExit = true;
    else if( signum == SIGUSR1 )
        OutputMsg( 0,
            "remote connection is down" );
}

std::atomic< guint32 > g_dwInterval( 10 );

struct CAsyncTimerCallbacks : public CAsyncStdAMCallbacks
{
    typedef CAsyncStdAMCallbacks super;

    gint32 GetPointValuesToUpdate(
        InterfPtr& pIf,
        std::vector< KeyValue >& veckv ) override
   {
        gint32 ret = 0;
        KeyValue okv;
        okv.strKey = O_PID;
        okv.oValue = ( guint32 )getpid();
        veckv.push_back( okv );

        okv.strKey = O_OBJ_COUNT;
        okv.oValue = ( guint32 )
            CObjBase::GetActCount();
        veckv.push_back( okv );
        return ret;
   }

    gint32 GetPointValuesToInit(
        InterfPtr& pIf,
        std::vector< KeyValue >& veckv ) override
    {
        gint32 ret = 0;
        do{
            KeyValue okv;
            char szPath[PATH_MAX];
            if( getcwd( szPath, sizeof(szPath)) != nullptr)
            {        
                okv.strKey = O_WORKING_DIR;
                BufPtr pBuf( true );
                ret = pBuf->Append( szPath,
                    strlen( szPath ) );
                if( SUCCEEDED( ret ) )
                {
                    okv.oValue = pBuf;
                    veckv.push_back( okv );
                }
            }

            okv.strKey = S_CMDLINE;
            stdstr strModPath;
            ret = GetModulePath( strModPath );
            if( ERROR( ret ) )
                break;
            strModPath += "/apptimer -d";
            BufPtr pBuf( true );
            ret = pBuf->Append(
                strModPath.c_str(),
                strModPath.size() );
            if( ERROR( ret ) )
                break;
            okv.oValue = pBuf;
            veckv.push_back( okv );

        }while( 0 );
        return 0;
    }

    gint32 SetPointValueCallback( 
        IConfigDb* context, gint32 iRet )
    { return 0; }

    //RPC event handler 'OnPointChanged'
    gint32 OnPointChanged(
        IConfigDb* context, 
        const std::string& strPtPath /*[ In ]*/,
        const Variant& value /*[ In ]*/ ) override
    {
        do{
            if( strPtPath != "timer1/interval1" )
            {
                super::OnPointChanged(
                    context, strPtPath, value );
                break;
            }
            guint32 dwWaitSec = value;
            if( dwWaitSec> 3600 )
                break;

            g_dwInterval = dwWaitSec;
            OutputMsg( 0,
                "interval changed to %d",
                dwWaitSec );

        }while( 0 );
        return 0;
    }

};

gint32 InitContext()
{
    gint32 ret = CoInitialize( 0 );
    if( ERROR( ret ) )
        return ret;
    do{
        CParamList oParams;
        oParams.Push( "appmonsvr" );
        oParams[ propSearchLocal ] = false;
        oParams[ propEnableLogging ] = g_bLogging;

        // adjust the thread number if necessary
        oParams[ propMaxIrpThrd ] = 0;
        oParams[ propMaxTaskThrd ] = 4;
        
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

static void Usage( const char* szName )
{
    fprintf( stderr,
        "Usage: %s [OPTIONS] <mount point> \n"
        "\t [ -d to run as a daemon ]\n"
        "\t [ -g send logs to log server ]\n"
        "\t [ -h this help ]\n", szName );
}

gint32 TimerLoop();

int main( int argc, char** argv )
{
    gint32 ret = 0;
    do{
        bool bDaemon = false;
        int opt = 0;
        while( ( opt = getopt( argc, argv, "hdg" ) ) != -1 )
        {
            switch( opt )
            {
                case 'd':
                    { bDaemon = true; break; }
                case 'g':
                    { g_bLogging = true; break; }
                case 'h':
                default:
                    { Usage( argv[ 0 ] ); exit( 0 ); }
            }
        }
        if( ERROR( ret ) )
            break;

        if( bDaemon )
        {
            ret = daemon( 1, 0 );
            if( ret < 0 )
            { ret = -errno; break; }
        }

        ret = InitContext();
        if( ERROR( ret ) )
            break;
        
        CIoManager* pMgr = g_pIoMgr;
        ret = pMgr->TryLoadClassFactory(
            "libappmancli.so" );
        if( ERROR( ret ) )
            break;

        InterfPtr pIf;
        PACBS pacbsIn( new CAsyncTimerCallbacks );
        InterfPtr pAppMan;
        // the pIf is just a placeholder, not for real.
        pIf = pMgr->GetSyncIf();
        ret = StartStdAppManCli(
            pIf, "timer1", pAppMan, pacbsIn );
        if( ERROR( ret ) )
            break;
        
        if( SUCCEEDED( ret ) )
            ret = TimerLoop();
            
        StopStdAppManCli();
    }while( 0 );

    DestroyContext();
    return ret;
}

// Function to wait for a specified number of seconds
// (supports fractional seconds)
void WaitForSeconds(double seconds)
{
    struct timeval tv;
    // Extract integer part of seconds
    tv.tv_sec = static_cast<int>(seconds);
    // Extract fractional part of seconds
    tv.tv_usec = static_cast<int>((seconds - tv.tv_sec) * 1e6);
    select(0, nullptr, nullptr, nullptr, &tv);
}

//-----Your code begins here---
gint32 TimerLoop()
{
    gint32 ret = 0;
    auto iOldSig = 
        signal( SIGINT, SignalHandler );
    signal( SIGUSR1, SignalHandler );

    guint32 dwTicks = 0;
    while( !g_bExit )
    {
        dwTicks++;
        InterfPtr pProxy;
        ret = GetAppManagercli( pProxy );
        if( ERROR( ret ) )
        {
            ret = 0;
            WaitForSeconds( 1 );
            continue;
        }
        CAppManager_CliImpl* pam = pProxy;

        CCfgOpener oCfg;
        if( dwTicks % g_dwInterval )
        {
            WaitForSeconds( 1 );
            continue;
        }
        oCfg.SetIntProp( propContext, 2 );
        Variant var( ( guint32 )1);
        pam->SetPointValue( oCfg.GetCfg(),
            "timer1/clock1", var );
        OutputMsg( 0, 
            "Info send clock1 %d",
            dwTicks );

        WaitForSeconds( 1 );
    }

    ret = STATUS_SUCCESS;

    signal( SIGINT, iOldSig );
    return ret;
}

