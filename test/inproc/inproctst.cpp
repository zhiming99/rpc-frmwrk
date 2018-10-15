/*
 * =====================================================================================
 *
 *       Filename:  iftest.cpp
 *
 *    Description:  implementation if the test classes
 *
 *        Version:  1.0
 *        Created:  07/15/2018 11:25:09 AM
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
#include <functional>

#include <rpc.h>
#include <proxy.h>

#include "inproctst.h"
#include "inprocsvr.h"

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>

#include <cppunit/extensions/HelperMacros.h>

CPPUNIT_TEST_SUITE_REGISTRATION( CIfSmokeTest );

std::atomic< bool > bExit( false );

void CIfSmokeTest::setUp()
{
    gint32 ret = 0;
    do{
        ret = CoInitialize( 0 );
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );

        CParamList oParams;

        ret = oParams.Push( std::string( MODULE_NAME ) );
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );

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

        ret = CoUninitialize();
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );
        m_pMgr.Clear();

        CPPUNIT_ASSERT( CObjBase::GetActCount() == 0 );

    }while( 0 );
}

void CIfSmokeTest::testSvrStartStop(
    IEventSink* pSyncTask )
{
    gint32 ret = 0;
    CCfgOpener oCfg;
    InterfPtr pIf;

    CPPUNIT_ASSERT( !m_pMgr.IsEmpty() );

    oCfg.SetObjPtr( propIoMgr, m_pMgr );

    ret = CRpcServices::LoadObjDesc(
        OBJDESC_PATH,
        OBJNAME_SERVER,
        true, oCfg.GetCfg() );
    
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    ret = pIf.NewObj(
        clsid( CInProcServer ),
        oCfg.GetCfg() );

    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    ret = pIf->Start();
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    CTasklet* pTask =
        static_cast< CTasklet* >( pSyncTask );

    // let the client to resume
    ( *pTask )( eventTaskComp );
    
    CInProcServer* pSvr = pIf;
    CPPUNIT_ASSERT( pSvr != nullptr );

    while( pIf->GetState() == stateConnected &&
        !bExit )
    {
        ret = pSvr->OnHelloWorld(
            "Hello, World from server!" );
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );
        sleep( 1 );
    }

    ret = pIf->Stop();
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    pIf.Clear();
}

using namespace std::placeholders;

gint32 CIfSmokeTest::startServer(
    std::thread*& pThread, IEventSink* pCallback )
{
    auto fn = std::bind(
        &CIfSmokeTest::testSvrStartStop,
        this, pCallback );

    pThread = new std::thread( fn );
    if( pThread == nullptr )
        return -ENOMEM;

    return 0;
}

void CIfSmokeTest::testCliStartStop()
{
    gint32 ret = 0;
    CCfgOpener oCfg;
    InterfPtr pIf;

    // start the server on another thread. Sure you can
    // use a single thread to run server/proxy, which
    // just needs some more effort.
    TaskletPtr pTask;
    ret = pTask.NewObj(
        clsid( CSyncCallback ) );

    if( ERROR( ret ) )
        return;

    std::thread* pSvrThrd = nullptr;
    ret = startServer( pSvrThrd, pTask );

    CPPUNIT_ASSERT( SUCCEEDED( ret ) );
    CSyncCallback* pSyncTask = 
        static_cast< CSyncCallback* >( pTask );

    // we need to wait the server to start, otherwise
    // there could be wired situation, for example, one
    // interface is connected, and the other interface
    // is not.
    pSyncTask->WaitForComplete();

    CPPUNIT_ASSERT( !m_pMgr.IsEmpty() );

    oCfg.SetObjPtr( propIoMgr, m_pMgr );

    ret = CRpcServices::LoadObjDesc(
        OBJDESC_PATH,
        OBJNAME_SERVER,
        false, oCfg.GetCfg() );

    CPPUNIT_ASSERT( SUCCEEDED( ret ) );
    
    ret = pIf.NewObj(
        clsid( CInProcClient ),
        oCfg.GetCfg() );
    
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    ret = pIf->Start();
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );
    
    while( pIf->GetState() != stateConnected )
        sleep( 1 );

    CInProcClient* pCli = pIf;
    if( pCli != nullptr )
    {
        sem_t semWait;
        Sem_Init( &semWait, 0, 0 );
        gint32 i = 0;

        // wait for event
        do{
            ret = Sem_TimedwaitSec( &semWait, 1 );
            printf( "\t%d\t\n", i++ );

            std::string strText( "Hello world from client!" );
            std::string strReply;

            // call the Echo method
            DebugPrint( 0, "Start..." );
            ret = pCli->Echo( strText, strReply );
            CPPUNIT_ASSERT( SUCCEEDED( ret ) );

            DebugPrint( 0, "Completed" );
            if( strText != strReply )
                CPPUNIT_ASSERT( false );

            // call the `EchoUnknown' method
            BufPtr pText( true ), pBufReply;
            *pText = "Hello, World from client 2!";
            ret = pCli->EchoUnknown( pText, pBufReply );
            CPPUNIT_ASSERT( SUCCEEDED( ret ) );
            DebugPrint( 0, "EchoUnknown Completed" );

        }while( !bExit );
    }
    else
    {
        CPPUNIT_ASSERT( false );
    }

    ret = pIf->Stop();
    // CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    pIf.Clear();

    bExit = true;
    pSvrThrd->join();
    delete pSvrThrd;
}

int main( int argc, char** argv )
{

    CppUnit::TextUi::TestRunner runner;
    CppUnit::TestFactoryRegistry &registry =
        CppUnit::TestFactoryRegistry::getRegistry();

    runner.addTest( registry.makeTest() );
    bool wasSuccessful = runner.run( "", false );

    return !wasSuccessful;
}
