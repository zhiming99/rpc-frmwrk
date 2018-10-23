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

#include "iftest.h"
#include "ifsvr.h"

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

extern void DumpObjs();
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

void CIfSmokeTest::testSvrStartStop()
{
    gint32 ret = 0;
    CCfgOpener oCfg;
    InterfPtr pIf;

    CPPUNIT_ASSERT( !m_pMgr.IsEmpty() );

    CfgPtr pCfg = oCfg.GetCfg();
    ret = CRpcServices::LoadObjDesc(
        "./echodesc.json",
        OBJNAME_ECHOSVR,
        true, pCfg );

    oCfg.SetObjPtr( propIoMgr, m_pMgr );
    ret = pIf.NewObj(
        clsid( CEchoServer ),
        oCfg.GetCfg() );
    
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    ret = pIf->Start();
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );
    
    while( pIf->GetState() == stateConnected )
        sleep( 1 );

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

    CfgPtr pCfg = oCfg.GetCfg();
    ret = CRpcServices::LoadObjDesc(
        "./echodesc.json",
        OBJNAME_ECHOSVR,
        false, pCfg );

    oCfg.SetObjPtr( propIoMgr, m_pMgr );
    ret = pIf.NewObj(
        clsid( CEchoClient ),
        oCfg.GetCfg() );
    
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    ret = pIf->Start();
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );
    
    while( pIf->GetState() != stateConnected )
        sleep( 1 );

    CEchoClient* pCli = pIf;
    if( pCli != nullptr )
    {
        std::string strText( "Hello world!" );
        std::string strReply;
        const char* szReply = nullptr;

        const char*& pszReply = szReply ; 
        DebugPrint( 0, "Start..." );
        ret = pCli->Echo( strText, strReply );
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );
        DebugPrint( 0, "Completed" );
        if( strText != strReply )
            CPPUNIT_ASSERT( false );

        ret = pCli->EchoPtr( strText.c_str(), pszReply );
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );
        DebugPrint( 0, "EchoPtr Completed" );
        if( strText != strReply )
            CPPUNIT_ASSERT( false );

        CParamList oParams;
        oParams.Push( std::string( "Hello, world" ) );
        oParams[ propClsid ] = Clsid_CEchoServer;
        gint32 iCount = 0;
        CfgPtr pCfgReply;
        ret = pCli->EchoCfg( 2, oParams.GetCfg(), iCount, pCfgReply );
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );
        DebugPrint( 0, "EchoCfg Completed" );
        if( iCount != 2 )
           CPPUNIT_ASSERT( false );

        BufPtr pText( true ), pBufReply;

        *pText = "Hello, World!";
        ret = pCli->EchoUnknown( pText, pBufReply );
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );
        DebugPrint( 0, "EchoUnknown Completed" );
    }
    else
    {
        CPPUNIT_ASSERT( false );
    }

    ret = pIf->Stop();
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

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
