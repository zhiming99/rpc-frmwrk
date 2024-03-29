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

using namespace rpcf;

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

        m_pMgr.Clear();
        // final call
        ret = CoUninitialize();
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );

        DebugPrint( 0, "#Leaked objects is %d",
            CObjBase::GetActCount() );

        CPPUNIT_ASSERT( CObjBase::GetActCount() == 0 );

    }while( 0 );
}

#ifdef SERVER
void CIfSmokeTest::testSvrStartStop()
{
    gint32 ret = 0;
    CCfgOpener oCfg;
    InterfPtr pIf;

    CPPUNIT_ASSERT( !m_pMgr.IsEmpty() );

    CfgPtr pCfg = oCfg.GetCfg();
    oCfg.SetObjPtr( propIoMgr, m_pMgr );
    ret = CRpcServices::LoadObjDesc(
        "./echodesc.json",
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
    pIf->Stop();
    pIf.Clear();

    if( ERROR( ret ) )
        exit( ret );
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );
}
#endif

#ifdef CLIENT
void CIfSmokeTest::testCliStartStop()
{

    gint32 ret = 0;
    CCfgOpener oCfg;
    InterfPtr pIf;

    CPPUNIT_ASSERT( !m_pMgr.IsEmpty() );

    CfgPtr pCfg = oCfg.GetCfg();
    oCfg.SetObjPtr( propIoMgr, m_pMgr );
    ret = CRpcServices::LoadObjDesc(
        "./echodesc.json",
        OBJNAME_ECHOSVR,
        false, pCfg );

    // create the proxy object
    ret = pIf.NewObj(
        clsid( CEchoClient ),
        oCfg.GetCfg() );
    
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    // start the proxy
    ret = pIf->Start();
    
    CEchoClient* pCli = pIf;
    if( ERROR( ret ) )
        pCli = nullptr;

    gint32 i = 0;

    if( pCli != nullptr )
    do{
        // make sure the server is online
        while( 1 )
        {
            if( pCli->GetState() == stateRecovery )
            {
                sleep( 1 );
                continue;
            }
            break;
        }

        if( !pCli->IsConnected() )
            break;

        OutputMsg( 0, "loop %d:", i );

        std::string strText( "Hello world!" );
        std::string strReply;
        const char* szReply = nullptr;

        // string echo
        const char*& pszReply = szReply ; 
        OutputMsg( 0, "Start..." );
        ret = pCli->Echo( strText, strReply );
        // CPPUNIT_ASSERT( SUCCEEDED( ret ) );
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
        CfgPtr pCfgReply;
        ret = pCli->EchoCfg( 2, oParams.GetCfg(), iCount, pCfgReply );
        if( ERROR( ret ) )
            break;
        DebugPrint( 0, "EchoCfg Completed" );
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
        DebugPrint( 0, "EchoUnknown Completed" );
        ret = pCli->Ping();
        if( ERROR( ret ) )
            break;
        DebugPrint( 0, "Ping Completed" );

        double fret = .0;
        ret = pCli->Echo2( 2, 4.0, fret );
        if( ERROR( ret ) )
            break;
        DebugPrint( 0, "Echo2 Completed, %g", fret );

    }while( ++i < 100 );

    if( ERROR( ret ) )
    {
        OutputMsg( ret, "Error, quit loop" );
    }
    // stop the proxy
    pIf->Stop();

    // release all the resources of the proxy
    pIf.Clear();

    CPPUNIT_ASSERT( SUCCEEDED( ret ) );
}

void CIfSmokeTest::testDBusLeak()
{
    FILE* fp = fopen( "dmsgdmp", "rb" );
    if( fp == nullptr )
        return;

    gint32 ret = 0;
    do{
        fseek( fp, 0, SEEK_END );
        guint32 iSize = ( guint32 )ftell( fp );
        rewind( fp );

        BufPtr pBuf( true );
        pBuf->Resize( iSize + sizeof( guint32 ) );

        *( ( gint32* )pBuf->ptr() ) = iSize;
        ret = fread( pBuf->ptr() + sizeof( guint32 ), 1, iSize, fp );
        if( ret == 0 )
        {
            ret = ERROR_FAIL;
            break;
        }

        for( int i = 0; i < 1000000; i++ )
        {
            DMsgPtr pMsg;
            ret = pMsg.Deserialize( pBuf );
            if( ERROR( ret ) )
                break;

            printf( "%s\n", pMsg.DumpMsg().c_str() );
            pMsg.Clear();
        }

    }while( 0 );

    if( fp != nullptr )
        fclose( fp );

    return;
}
void CIfSmokeTest::testObjAlloc()
{
    for( int i = 0; i < 10000000; i++ )
    {
        BufPtr pBuf( true );
        pBuf->Resize( 128 );
        pBuf.Clear();
    }
}
#endif

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
