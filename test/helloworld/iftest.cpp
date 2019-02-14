/*
 * =====================================================================================
 *
 *       Filename:  iftest.cpp
 *
 *    Description:  implementation if the unit test classes. This is an example
 *                  of simple object with single interface.
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
        // initialize the rpc library
        ret = CoInitialize( 0 );
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );

        CParamList oParams;

        ret = oParams.Push( std::string( MODULE_NAME ) );
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );

        // create the iomanager
        ret = m_pMgr.NewObj(
            clsid( CIoManager ),
            oParams.GetCfg() );

        CPPUNIT_ASSERT( SUCCEEDED( ret ) );

        // start everything
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

        // stop everything
        ret = pSvc->Stop();
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );

        // final call
        ret = CoUninitialize();
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );
        m_pMgr.Clear();

        DebugPrint( 0, "#Leaked objects is %d",
            CObjBase::GetActCount() );

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

    // create the interface server
    oCfg.SetObjPtr( propIoMgr, m_pMgr );
    ret = pIf.NewObj(
        clsid( CEchoServer ),
        oCfg.GetCfg() );
    
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    // start the server
    ret = pIf->Start();
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );
    
    CEchoServer* pSvr = pIf;
    // waiting for requests
    while( pSvr->IsConnected() )
        sleep( 1 );

    // stop the server
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

    // create the proxy object
    oCfg.SetObjPtr( propIoMgr, m_pMgr );
    ret = pIf.NewObj(
        clsid( CEchoClient ),
        oCfg.GetCfg() );
    
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    // start the proxy
    ret = pIf->Start();
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );
    
    CEchoClient* pCli = pIf;
    if( pCli != nullptr )
    {
        // make sure the server is online
        while( !pCli->IsConnected() )
            sleep( 1 );

        std::string strText( "Hello world!" );
        std::string strReply;
        const char* szReply = nullptr;

        // string echo
        const char*& pszReply = szReply ; 
        DebugPrint( 0, "Start..." );
        ret = pCli->Echo( strText, strReply );
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );
        DebugPrint( 0, "Completed" );
        if( strText != strReply )
            CPPUNIT_ASSERT( false );

        // pointer echo
        ret = pCli->EchoPtr( strText.c_str(), pszReply );
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );
        DebugPrint( 0, "EchoPtr Completed" );
        if( strText != strReply )
            CPPUNIT_ASSERT( false );

        // echo an object
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

        // echo a blob
        *pText = "Hello, World!";
        ret = pCli->EchoUnknown( pText, pBufReply );
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );
        DebugPrint( 0, "EchoUnknown Completed" );
        ret = pCli->Ping();
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );
        DebugPrint( 0, "Ping Completed" );
    }
    else
    {
        CPPUNIT_ASSERT( false );
    }

    // stop the proxy
    ret = pIf->Stop();
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    // release all the resources of the proxy
    pIf.Clear();
}

int main( int argc, char** argv )
{

    // register test with cppunit 
    CppUnit::TextUi::TestRunner runner;
    CppUnit::TestFactoryRegistry &registry =
        CppUnit::TestFactoryRegistry::getRegistry();

    runner.addTest( registry.makeTest() );
    bool wasSuccessful = runner.run( "", false );

    return !wasSuccessful;
}
