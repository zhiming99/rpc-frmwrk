/*
 * =====================================================================================
 *
 *       Filename:  routmain.cpp
 *
 *    Description:  the main entry of the stand-alone rpc router.
 *
 *        Version:  1.0
 *        Created:  12/21/2018 03:42:25 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:  
 *
 * =====================================================================================
 */

#include <string>
#include <iostream>
#include <unistd.h>

#include "routmain.h"
#include <ifhelper.h>

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

CPPUNIT_TEST_SUITE_REGISTRATION( CIfRouterTest );

bool bPause = false;
void CIfRouterTest::setUp()
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

void CIfRouterTest::tearDown()
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

void CIfRouterTest::testSvrStartStop()
{
    gint32 ret = 0;
    InterfPtr pIf;

    CPPUNIT_ASSERT( !m_pMgr.IsEmpty() );

    CCfgOpener oCfg;
    oCfg.SetObjPtr( propIoMgr, m_pMgr );

    ObjPtr pRouter;

    ret = pRouter.NewObj( clsid( CRpcRouter ),
        oCfg.GetCfg() );

    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    IService* pSvc = pRouter;
    CPPUNIT_ASSERT( pSvc != nullptr );

    ret = pSvc->Start();
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    while( true )
    {
        sleep( 1 );
    }

    ret = pSvc->Stop();
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );
    pRouter.Clear();

    return;
}

int main( int argc, char** argv )
{

    CppUnit::TextUi::TestRunner runner;
    CppUnit::TestFactoryRegistry& registry =
        CppUnit::TestFactoryRegistry::getRegistry();

    if( argc > 1 && strcmp( argv[ 1 ], "p" ) == 0 )
        bPause = true;

    runner.addTest( registry.makeTest() );
    bool wasSuccessful = runner.run( "", false );

    return !wasSuccessful;
}
