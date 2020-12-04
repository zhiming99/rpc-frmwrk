/*
 * =====================================================================================
 *
 *       Filename:  stmtest.cpp
 *
 *    Description:  implementation test for the CStreamingServer/CStreamingClient.
 *                  Note that this object is a multi-if object, unlike the
 *                  objects of all the earlier tests.
 *
 *        Version:  1.0
 *        Created:  12/04/2018 10:37:00 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com ), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <string>
#include <iostream>
#include <unistd.h>
#include <cmath>

#include <rpc.h>
#include <proxy.h>

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

using namespace rpcfrmwrk;
#include "stmtest.h"
#include "stmsvr.h"

#define NSEC_PER_SEC 1000000000

CPPUNIT_TEST_SUITE_REGISTRATION( CIfSmokeTest );

void CIfSmokeTest::setUp()
{
    gint32 ret = 0;
    do{
        ret = CoInitialize( 0 );
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );

        CParamList oParams;
        ret = oParams.Push( std::string( MODULE_NAME ) );
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );

        /**
        * @brief 
        * the number is configurable of threads
        * for irp completion and tasks. For client
        * side you can specify a memory-saving
        * settings, and on the server side, you
        * can specify a performance friendly
        * settings. Usually, more threads brings
        * benenfits in perofrmance. but not
        * always, tweak the numbers suitable for
        * your hardware settings. And please be
        * noticed that, the min propMaxTaskThrd is
        * 1. The max of both numbers cannot
        * exceeds CIoManager::GetNumCores.
        *
        */
#if defined( CLIENT )
        oParams[ propMaxIrpThrd ] = 0;
        oParams[ propMaxTaskThrd ] = 1;
#elif defined( SERVER )
        // if server needs more performance
        oParams[ propMaxIrpThrd ] = 0;
        oParams[ propMaxTaskThrd ] = 2;
#endif

        ret = m_pMgr.NewObj(
            clsid( CIoManager ),
            oParams.GetCfg() );

        CPPUNIT_ASSERT( SUCCEEDED( ret ) );

        IService* pSvc = m_pMgr;
        if( pSvc != nullptr )
            ret = pSvc->Start();
        else
            ret = -EFAULT;

        CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    }while( 0 );
}

void CIfSmokeTest::tearDown()
{
    gint32 ret = 0;
    do{
        IService* pSvc = m_pMgr;
        CPPUNIT_ASSERT( pSvc != nullptr );

        ret = pSvc->Stop();
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );

        m_pMgr.Clear();

        ret = CoUninitialize();
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );

        CPPUNIT_ASSERT( CObjBase::GetActCount() == 0 );

    }while( 0 );
}

#ifdef SERVER
void CIfSmokeTest::testSvrStartStop()
{
    gint32 ret = 0;
    InterfPtr pIf;

    CPPUNIT_ASSERT( !m_pMgr.IsEmpty() );

    CCfgOpener oCfg;
    oCfg.SetObjPtr( propIoMgr, m_pMgr );

    CfgPtr pCfg = oCfg.GetCfg();
    ret = CRpcServices::LoadObjDesc(
        "./stmdesc.json",
        OBJNAME_ECHOSVR,
        true, pCfg );

    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    ret = pIf.NewObj(
        clsid( CStreamingServer ),
        oCfg.GetCfg() );
    
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    ret = pIf->Start();
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );
    
    // using the mainloop solution is the only way
    // to handle more than one stream channels as
    // a server.
    CStreamingServer* pSvr = pIf;
    ret = pSvr->StartLoop();
    if( ERROR( ret ) )
    {
        DebugPrint( ret, "Error Quit the loop" );
    }

    ret = pIf->Stop();
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    pIf.Clear();
}
#endif

#ifdef CLIENT
bool bPause = false;
void CIfSmokeTest::testCliStartStop()
{
    gint32 ret = 0;
    InterfPtr pIf;

    CPPUNIT_ASSERT( !m_pMgr.IsEmpty() );

    CCfgOpener oCfg;
    oCfg.SetObjPtr( propIoMgr, m_pMgr );

    CfgPtr pCfg = oCfg.GetCfg();
    ret = CRpcServices::LoadObjDesc(
        "./stmdesc.json",
        OBJNAME_ECHOSVR,
        false, pCfg );

    CPPUNIT_ASSERT( SUCCEEDED( ret ) );
    if( ERROR( ret ) )
        return;

    ret = pIf.NewObj(
        clsid( CStreamingClient ),
        oCfg.GetCfg() );
    
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    ret = pIf->Start();

    CPPUNIT_ASSERT( SUCCEEDED( ret ) );
    
    CStreamingClient* pCli = pIf;
    if( pCli == nullptr )
        return;

    do{
        // the server is not up yet
        while( pCli->GetState() == stateRecovery )
            sleep( 1 );

        if( !pCli->IsConnected() )
        {
            // possibly the router is down
            break;
        }

        std::string strText( "Hello world!" );
        std::string strReply;

        // method from interface CEchoServer
        DebugPrint( 0, "Start..." );
        ret = pCli->Echo( strText, strReply );
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );
        DebugPrint( 0, "Completed" );
        if( strText != strReply )
            CPPUNIT_ASSERT( false );
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );

        // method from interface CStreamingServer
        IntVecPtr pClsids( true );
        ret = pCli->EnumInterfaces( pClsids );
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );
        DebugPrint( 0, "Completed" );

        // IStream implementation test
        printf( "Testing stream creation, " 
            "double-direction communication"
            " and active cancel\n" );

        timespec ts = { 0 }, ts2={ 0 };
        clock_gettime( CLOCK_REALTIME, &ts );

        DebugPrint( ret, "MainLoopStart..." );

        //
        // method 1: asynchronous way to
        // send/receive the stream data from the
        // server, all the logics are in the
        // stmcli.cpp
        //
        // async call for LOOP_COUNT times
        ret = pCli->StartLoop();

        clock_gettime( CLOCK_REALTIME, &ts2 );
        int iCarry = 0;
        double nsec = .0;
        if( ts2.tv_nsec < ts.tv_nsec )
        {
            nsec = ts2.tv_nsec + ( 10 ^ 9 ) - ts.tv_nsec;
            iCarry = 1;
        }
        else
        {
            nsec = ts2.tv_nsec - ts.tv_nsec;
        }

        double dbTime1 = ( ( double )( ts2.tv_sec - ts.tv_sec - iCarry ) ) +
            ( nsec ) / NSEC_PER_SEC;

        CPPUNIT_ASSERT( SUCCEEDED( ret  ) ||
            ret == STATUS_PENDING );

        sleep( 1 );

        //
        // method 2: synchronous way to
        // send/receive stream data from the
        // server. you don't need the effort in
        // stmcli.cpp. But the performance could
        // suffer if the traffic is busy.
        //
        HANDLE hChannel = 0; 
        ret = pCli->StartStream( hChannel );
        if( ERROR( ret ) )
            break;

        BufPtr pBuf( true );
        DebugPrint( ret, "SyncLoopStart..." );
        clock_gettime( CLOCK_REALTIME, &ts );
        // sync call for LOOP_COUNT times
        for( int i = 0; i < LOOP_COUNT; i++ )
        {
            std::string strMsg = DebugMsg( i,
                "a message to server" );

            *pBuf = strMsg;
            ret = pCli->WriteStream( hChannel, pBuf );
            if( ERROR( ret ) )
                break;

            pBuf->Resize( 0 );
            ret = pCli->ReadStream( hChannel, pBuf );
            if( ERROR( ret ) )
                break;

            printf( "Server says: %s\n", pBuf->ptr() );
        }
        clock_gettime( CLOCK_REALTIME, &ts2 );

        iCarry = 0;
        nsec = .0;
        if( ts2.tv_nsec < ts.tv_nsec )
        {
            nsec = ts2.tv_nsec + NSEC_PER_SEC - ts.tv_nsec;
            iCarry = 1;
        }
        else
        {
            nsec = ts2.tv_nsec - ts.tv_nsec;
        }

        double dbTime2 = ( ( double )( ts2.tv_sec - ts.tv_sec - iCarry ) ) +
            ( nsec ) / NSEC_PER_SEC;

        // NOTE that the performance is MainLoop
        // wins when the loop is over 10000, and
        // below 10000, the SyncLoop could be
        // better.
        DebugPrint( ret, "MainLoop takes %g secs", dbTime1 );
        DebugPrint( ret, "SyncLoop takes %g secs", dbTime2 );
        DebugPrint( ret, " MainLoop is %g time faster than SyncLoop",
            dbTime2/dbTime1 );

        CPPUNIT_ASSERT( SUCCEEDED( ret ) );

        if( bPause )
        {
            ret = pCli->Pause_Proxy();
            CPPUNIT_ASSERT( SUCCEEDED( ret ) );
            DebugPrint( 0, "Pause interface Completed" );
        }

    }while( 0 );

    ret = pIf->Stop();
    pIf.Clear();
}
#endif

int main( int argc, char** argv )
{

    CppUnit::TextUi::TestRunner runner;
    CppUnit::TestFactoryRegistry &registry =
        CppUnit::TestFactoryRegistry::getRegistry();

#ifdef CLIENT
    if( argc > 1 && strcmp( argv[ 1 ], "p" ) == 0 )
        bPause = true;
#endif

    runner.addTest( registry.makeTest() );
    bool wasSuccessful = runner.run( "", false );

    return !wasSuccessful;
}
