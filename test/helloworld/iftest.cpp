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

using namespace rpcfrmwrk;

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
        oParams[ propMaxTaskThrd ] = 1;
#endif
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

        m_pMgr.Clear();
        // final call
        ret = CoUninitialize();
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );

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
    oCfg.SetObjPtr( propIoMgr, m_pMgr );
    ret = CRpcServices::LoadObjDesc(
        "./hwdesc.json",
        OBJNAME_ECHOSVR,
        true, pCfg );

    // create the interface server
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
    CPPUNIT_ASSERT( !m_pMgr.IsEmpty() );

    // load the configuration
    CCfgOpener oCfg;
    CfgPtr pCfg = oCfg.GetCfg();
    oCfg.SetObjPtr( propIoMgr, m_pMgr );
    ret = CRpcServices::LoadObjDesc(
        "./hwdesc.json",
        OBJNAME_ECHOSVR,
        false, pCfg );

    // create the proxy object
    InterfPtr pIf;
    ret = pIf.NewObj(
        clsid( CEchoClient ),
        oCfg.GetCfg() );
    
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    do{
        // start the proxy
        ret = pIf->Start();
        if( ERROR( ret ) )
            break;
        
        CEchoClient* pCli = pIf;
        if( unlikely( pCli == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }

        // test if the server is online
        if( !pCli->IsConnected() )
            break;

        std::string strText( "Hello world!" );
        std::string strReply;
        const char* szReply = nullptr;

        // string echo
        const char*& pszReply = szReply ; 
        DebugPrint( 0, "Start..." );
        ret = pCli->Echo( strText, strReply );
        if( ERROR( ret ) )
            break;

        DebugPrint( 0, "Completed" );
        if( strText != strReply )
            CPPUNIT_ASSERT( false );

        // pointer echo
        ret = pCli->EchoPtr( strText.c_str(), pszReply );
        if( ERROR( ret ) )
            break;

        DebugPrint( 0, "EchoPtr Completed" );
        if( strText != strReply )
            CPPUNIT_ASSERT( false );

        // echo an object
        CParamList oParams;
        oParams.Push( std::string( "Hello, world" ) );
        oParams[ propClsid ] = Clsid_CEchoServer;
        gint32 iCount = 0;

        // the pointer to hold the object echoed
        // back
        CfgPtr pCfgReply;
        ret = pCli->EchoCfg( 2,
            oParams.GetCfg(), iCount, pCfgReply );
        if( ERROR( ret ) )
            break;

        DebugPrint( 0, "EchoCfg Completed" );
        // check the result
        if( iCount != 2 )
        {
            ret = ERROR_FAIL;
            DebugPrint( ret, "EchoCfg data dose not match" );
            break;
        }

        BufPtr pText( true ), pBufReply;

        // echo a blob
        *pText = "Hello, World!";
        ret = pCli->EchoUnknown( pText, pBufReply );
        if( ERROR( ret ) )
            break;

        strReply = ( std::string )*pBufReply;
        if( strReply != ( const char* )*pText )
        {
            DebugPrint( 0, "EchoUnknown response"\
                "different from the original text" );
        }
        else
        {
            DebugPrint( 0, "EchoUnknown Completed" );
        }

        ret = pCli->Ping();
        if( ERROR( ret ) )
            break;
        DebugPrint( 0, "Ping Completed" );

    }while( 0 );

    if( ERROR( ret ) )
    {
        DebugPrint( ret, "Error, quit loop" );
    }

    // stop the proxy
    ret = pIf->Stop();
    if( ret == STATUS_PENDING )
    {
        // the underlying port is also stopping,
        // wait till it is done. otherwise the
        // iomanager's stop could go wrong
        while( pIf->GetState() == stateStopped )
            sleep( 1 );

        ret = 0;
    }

    // release all the resources of the proxy
    pIf.Clear();
}

bool test()
{
    bool wasSuccessful = false; 

    // register test with cppunit 
    CppUnit::TextUi::TestRunner runner;
    CppUnit::TestFactoryRegistry &registry =
        CppUnit::TestFactoryRegistry::getRegistry();
    runner.addTest( registry.makeTest() );

    wasSuccessful = runner.run( "", false );

    return !wasSuccessful;
}

int main( int argc, char** argv )
{
    return test();
}
