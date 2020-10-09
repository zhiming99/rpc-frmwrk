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

#include <rpc.h>
#include <proxy.h>

#include "evtest.h"
#include "evtsvr.h"

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>

#include <cppunit/extensions/HelperMacros.h>

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

void CIfSmokeTest::testSvrStartStop()
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
    
    ret = pIf.NewObj(
        clsid( CEventServer ),
        oCfg.GetCfg() );

    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    ret = pIf->Start();
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );
    
    CEventServer* pSvr = pIf;
    CPPUNIT_ASSERT( pSvr != nullptr );

    guint32 dwCounter = 0;
    while( pSvr->IsConnected() )
    {
        std::string strMsg = "Hello, World! (";
        strMsg += std::to_string( dwCounter ) + ")";
        ret = pSvr->OnHelloWorld( strMsg );
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );
        sleep( 1 );
        dwCounter++;
    }

    ret = pIf->Stop();
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    pIf.Clear();
}

void CIfSmokeTest::testCliStartStop()
{

    gint32 ret = 0;
    CCfgOpener oCfg;
    InterfPtr pIf;

    CPPUNIT_ASSERT( !m_pMgr.IsEmpty() );

    oCfg.SetObjPtr( propIoMgr, m_pMgr );

    ret = CRpcServices::LoadObjDesc(
        OBJDESC_PATH,
        OBJNAME_SERVER,
        false, oCfg.GetCfg() );
    
    ret = pIf.NewObj(
        clsid( CEventClient ),
        oCfg.GetCfg() );
    
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    ret = pIf->Start();
    // CPPUNIT_ASSERT( SUCCEEDED( ret ) );
    
    CEventClient* pCli = pIf;
    if( pCli != nullptr )
    {
        while( pCli->GetState() == stateRecovery )
            sleep( 1 );

        while( pCli->IsConnected() )
            sleep( 1 );
    }
    else
    {
        CPPUNIT_ASSERT( false );
    }

    ret = pIf->Stop();
    // CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    pIf.Clear();
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
