/*
 * =====================================================================================
 *
 *       Filename:  asynctest.cpp
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

#include <rpc.h>
#include <proxy.h>

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

using namespace rpcf;
#include "prtest.h"
#include "prsvr.h"

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
        "./prdesc.json",
        OBJNAME_ECHOSVR,
        true, pCfg );

    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    ret = pIf.NewObj(
        clsid( CPauseResumeServer ),
        oCfg.GetCfg() );
    
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    ret = pIf->Start();
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );
    
    CPauseResumeServer* pSvr = pIf;
    while( pSvr->IsConnected() )
        sleep( 1 );

    ret = pIf->Stop();
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    pIf.Clear();
}
#endif

#ifdef CLIENT
void CIfSmokeTest::testCliStartStop()
{

    gint32 ret = 0;
    InterfPtr pIf;

    CPPUNIT_ASSERT( !m_pMgr.IsEmpty() );

    CCfgOpener oCfg;
    oCfg.SetObjPtr( propIoMgr, m_pMgr );

    CfgPtr pCfg = oCfg.GetCfg();
    ret = CRpcServices::LoadObjDesc(
        "./prdesc.json",
        OBJNAME_ECHOSVR,
        false, pCfg );

    CPPUNIT_ASSERT( SUCCEEDED( ret ) );
    if( ERROR( ret ) )
        return;

    ret = pIf.NewObj(
        clsid( CPauseResumeClient ),
        oCfg.GetCfg() );
    
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    ret = pIf->Start();

    CPPUNIT_ASSERT( SUCCEEDED( ret ) );
    
    CPauseResumeClient* pCli = pIf;
    if( pCli != nullptr )
    {
        while( !pCli->IsConnected() )
            sleep( 1 );

        std::string strText( "Hello world!" );
        std::string strReply;

        BufPtr pText( true ), pBufReply;
        *pText = "Hello, World!";

        DebugPrint( 0, "Echo 1st time..." );
        ret = pCli->Echo( strText, strReply );
        CPPUNIT_ASSERT( ERROR( ret ) );
        DebugPrint( 0, "Completed" );

        ret = pCli->EchoUnknown( pText, pBufReply );
        CPPUNIT_ASSERT( ERROR( ret ) );
        DebugPrint( 0, "EchoUnknown Completed" );

        DebugPrint( 0, "Resuming the interface" );
        ret = pCli->Resume_Proxy();
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );

        DebugPrint( 0, "Start..." );
        ret = pCli->Echo( strText, strReply );
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );
        DebugPrint( 0, "Completed" );
        if( strText != strReply )
            CPPUNIT_ASSERT( false );

        DebugPrint( 0, "Start EchoUnknown... " );
        ret = pCli->EchoUnknown( pText, pBufReply );
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );
        DebugPrint( 0, "EchoUnknown Completed" );

        DebugPrint( 0, "Start EchoMany... " );
        gint32 o1; gint16 o2; gint64 o3; float o4; double o5;
        ret = pCli->EchoMany(
            1, 2, 3, 4, 5, strText,
            o1, o2, o3, o4, o5, strReply );
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );
        if( strText + " 2" != strReply )
            CPPUNIT_ASSERT( false );

        DebugPrint( 0, "o1=%d, o2=%hd, o3=%lld, o4=%f, o5=%g, strReply=%s",
             o1, o2, o3, o4, o5, strReply.c_str() );

        DebugPrint( 0, "EchoMany Completed" );

        DebugPrint( 0, "Start LongWait 20s ... " );
        ret = pCli->LongWait( strText, strReply );
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );
        DebugPrint( 0, "Completed" );
        DebugPrint( 0, "Pausing the interface" );
        ret = pCli->Pause_Proxy();
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    }
    else
    {
        CPPUNIT_ASSERT( false );
    }

    ret = pIf->Stop();

    pIf.Clear();

    CPPUNIT_ASSERT( SUCCEEDED( ret ) );
}
#endif

int main( int argc, char** argv )
{

    CppUnit::TextUi::TestRunner runner;
    CppUnit::TestFactoryRegistry &registry =
        CppUnit::TestFactoryRegistry::getRegistry();

    runner.addTest( registry.makeTest() );
    bool wasSuccessful = runner.run( "", false );

    return !wasSuccessful;
}
