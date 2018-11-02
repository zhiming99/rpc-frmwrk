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

#include "actctest.h"
#include "actcsvr.h"

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

        ret = CoUninitialize();
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );
        m_pMgr.Clear();

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
        OBJDESC_PATH,
        OBJNAME_SERVER,
        true, pCfg );

    if( ERROR( ret ) )
        return;

    ret = pIf.NewObj(
        clsid( CActcServer ),
        oCfg.GetCfg() );
    
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    ret = pIf->Start();
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );
    
    CActcServer* pSvr = pIf;
    while( pSvr->IsConnected() )
        sleep( 1 );

    ret = pIf->Stop();
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    pIf.Clear();
}
#endif

#ifdef CLIENT
void CIfSmokeTest::testCliActCancel()
{

    gint32 ret = 0;
    InterfPtr pIf;

    CPPUNIT_ASSERT( !m_pMgr.IsEmpty() );

    CCfgOpener oCfg;
    oCfg.SetObjPtr( propIoMgr, m_pMgr );

    CfgPtr pCfg = oCfg.GetCfg();

    ret = CRpcServices::LoadObjDesc(
        OBJDESC_PATH,
        OBJNAME_SERVER,
        false, pCfg );

    CPPUNIT_ASSERT( SUCCEEDED( ret ) );
    if( ERROR( ret ) )
        return;

    ret = pIf.NewObj(
        clsid( CActcClient ),
        oCfg.GetCfg() );
    
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    ret = pIf->Start();

    CPPUNIT_ASSERT( SUCCEEDED( ret ) );
    
    CActcClient* pCli = pIf;
    if( pCli != nullptr )
    {
        while( !pCli->IsConnected() )
            sleep( 1 );

        CfgPtr pResp( true );

        CParamList oParams( ( IConfigDb* )pResp );
        std::string strText( "Hello world!" );
        std::string strReply;

        printf( "\n" );
        DebugPrint( 0, "Start..." );

        // send out the request
        ret = pCli->LongWait(
            pResp, strText, strReply );

        CPPUNIT_ASSERT( ret == STATUS_PENDING );

        // NOTE: race condition exists. Normally we
        // should not touch pResp before the request
        // complete, otherwise there is the risk the
        // pResp being accessed by multiple users at
        // the same time.
        guint64 qwTaskId = 0;
        if( oParams.exist( propTaskId ) )
        {
            qwTaskId = oParams[ propTaskId ];
        }

        // we are about to actively cancel this request
        CPPUNIT_ASSERT( qwTaskId != 0 );
        ret = pCli->CancelRequest( qwTaskId );
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );
        if( SUCCEEDED( ret ) )
            DebugPrint( 0, "Request is successfully cancelled" );

        oParams.Clear();
    }
    else
    {
        CPPUNIT_ASSERT( false );
    }

    ret = pIf->Stop();
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    pIf.Clear();
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
