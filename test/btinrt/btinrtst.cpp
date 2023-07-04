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
#include <frmwrk.h>

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

using namespace rpcf;
#include "btinrtsvr.h"
#include "btinrtst.h"

CPPUNIT_TEST_SUITE_REGISTRATION( CIfSmokeTest );

static bool g_bAuth = false;
static bool g_bRfc = false;
static bool g_bSepConn = false;
char g_szKeyPass[ SSL_PASS_MAX + 1 ] = {0};

void CIfSmokeTest::setUp()
{
    gint32 ret = 0;
    do{
        // initialize the rpc library
        ret = CoInitialize( 0 );
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );

        CParamList oParams;

        ret = oParams.Push(
            std::string( MODULE_NAME ) );
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );

        guint32 dwNumThrds =
            ( guint32 )std::max( 1U,
            std::thread::hardware_concurrency() );

        if( dwNumThrds > 1 )
            dwNumThrds = ( dwNumThrds >> 1 );

        oParams[ propMaxTaskThrd ] = dwNumThrds;

        // weird, more threads will have worse
        // performance of handshake
        oParams[ propMaxIrpThrd ] = 2;

        // create the iomanager
        ret = m_pMgr.NewObj(
            clsid( CIoManager ),
            oParams.GetCfg() );

        CPPUNIT_ASSERT( SUCCEEDED( ret ) );

        // start everything
        CIoManager* pSvc = m_pMgr;
        pSvc->SetCmdLineOpt( propObjDescPath,
            "./btinrt.json" );

        pSvc->SetRouterName( ROUTER_NAME );

        if( g_bAuth )
        {
            pSvc->SetCmdLineOpt(
                propHasAuth, g_bAuth );
        }

        if( g_bRfc )
        {
            pSvc->SetCmdLineOpt(
                propEnableRfc, g_bRfc );
        }

#ifdef CLIENT
        pSvc->SetCmdLineOpt(
            propSepConns, g_bSepConn );

        pSvc->SetCmdLineOpt(
            propRouterRole, 1 );
#endif
#ifdef SERVER
        pSvc->SetCmdLineOpt(
            propRouterRole, 2 );
#endif
        ret = pSvc->TryLoadClassFactory(
            "./librpc.so" );
        if( ERROR( ret ) )
            break;

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

    InterfPtr pRouter;
    ret = StartRouter( pRouter );

    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    oCfg.SetObjPtr( propIoMgr, m_pMgr );
    CfgPtr pCfg = oCfg.GetCfg();
    ret = CRpcServices::LoadObjDesc(
        "./btinrt.json",
        OBJNAME_ECHOSVR,
        true, pCfg );

    oCfg[ propSvrInstName ] =
        stdstr( "BtinrtSvr" );

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
    gint32 dwIndex = 0;
    while( pSvr->IsConnected() )
    {
        std::string strMsg = std::to_string( dwIndex++ );
        strMsg += ": Hello, World!";
        ret = pSvr->OnHelloWorld( strMsg );
        if( ERROR( ret ) )
            break;
        sleep( 1 );
    }

    // stop the server
    ret = pIf->Stop();
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    pIf.Clear();

    ret = pRouter->Stop();
    if( ret == STATUS_PENDING )
    {
        while( pIf->GetState() != stateStopped )
            sleep( 1 );
        ret = 0;
    }

}
#endif

CfgPtr CIfSmokeTest::InitRouterCfg()
{
    CParamList oParams;
    gint32 ret = 0;

    do{

        stdstr strDescPath;
        strDescPath = "./router.json";
        if( g_bAuth )
            strDescPath = "./rtauth.json";

        std::string strRtName;
        CIoManager* pMgr = m_pMgr;
        pMgr->GetRouterName( strRtName );

        oParams.SetStrProp( propSvrInstName,
            MODNAME_RPCROUTER );

        oParams[ propIoMgr ] = m_pMgr;
        pMgr->SetCmdLineOpt( propSvrInstName,
            MODNAME_RPCROUTER );
        ret = CRpcServices::LoadObjDesc(
            strDescPath,
            OBJNAME_ROUTER,
            true, oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        oParams[ propIfStateClass ] =
            clsid( CIfRouterMgrState );

#ifdef CLIENT
        oParams[ propRouterRole ] = 1;
#endif
#ifdef SERVER
        oParams[ propRouterRole ] = 2;
#endif

    }while( 0 );

    if( ERROR( ret ) )
    {
        std::string strMsg = DebugMsg(
            ret, "Error loading file router.json" );
        throw std::runtime_error( strMsg );
    }

    return oParams.GetCfg();
}

gint32 CIfSmokeTest::StartRouter( InterfPtr& pRt )
{
    gint32 ret = 0;
    do{
        CfgPtr pCfg = InitRouterCfg();
        if( !pCfg.IsEmpty() )
        {
            InterfPtr pIf;
            ret =  pIf.NewObj(
                clsid( CRpcRouterManagerImpl ),
                pCfg );

            if( ERROR( ret ) )
                break;
                
            CInterfaceServer* pRouter = pIf;
            if( unlikely( pRouter == nullptr ) )
            {
                ret = -EFAULT;
                break;
            }
            ret = pRouter->Start();
            if( ERROR( ret ) )
                break;

            pRt = pIf;
        }
   }while( 0 );

   return ret;
}

#ifdef CLIENT
void CIfSmokeTest::testCliStartStop()
{

    gint32 ret = 0;
    CCfgOpener oCfg;
    InterfPtr pIf;

    CPPUNIT_ASSERT( !m_pMgr.IsEmpty() );

    InterfPtr pRouter;
    ret = StartRouter( pRouter );
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    oCfg.SetObjPtr( propIoMgr, m_pMgr );
    CfgPtr pCfg = oCfg.GetCfg();
    ret = CRpcServices::LoadObjDesc(
        "./btinrt.json",
        OBJNAME_ECHOSVR,
        false, pCfg );

    oCfg[ propSvrInstName ] =
        stdstr( "BtinrtSvr" );

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

    DebugPrint( 0, "Test start" );
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

        DebugPrint( 0, "loop %d:", i );

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
        OutputMsg( 0, "Loop done" );

    }while( ++i < 100 );

    if( ERROR( ret ) )
    {
        DebugPrint( ret, "Error, quit loop" );
    }
    else
    {

        OutputMsg( ret, "successfully completed" );
    }
    // stop the proxy
    ret = pIf->Stop();
    if( ret == STATUS_PENDING )
    {
        // the underlying port is also stopping,
        // wait till it is done. otherwise the
        // iomanager's stop could go wrong
        while( pIf->GetState() != stateStopped )
            sleep( 1 );

        ret = 0;
    }

    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    // release all the resources of the proxy
    pIf.Clear();

    ret = pRouter->Stop();
    if( ret == STATUS_PENDING )
    {
        while( pIf->GetState() != stateStopped )
            sleep( 1 );
        ret = 0;
    }

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
    int opt = 0;
    int ret = 0;
    while( ( opt = getopt( argc, argv, "acf" ) ) != -1 )
    {
        switch (opt)
        {
        case 'a':
            {
                g_bAuth = true;
                break;
            }
        case 'c':
            {
#ifdef CLIENT
                g_bSepConn = true;
#endif
                break;
            }
        case 'f':
            {
                g_bRfc = true;
                break;
            }
        default: /*  '?' */
            ret = -EINVAL;
            break;
        }

        if( ERROR( ret ) )
            break;
    }

    if( ERROR( ret ) )
    {
        fprintf( stderr,
            "Usage: %s [Options]\n" 
            "\t [-a to enable authentication ]\n"
            "\t [-c to establish a seperate connection to the same bridge per client, only for role 1]\n"
            "\t [-f to enable request-based flow control on the gateway bridge, ignore it if no massive connections ]\n",
            argv[ 0 ] );

        exit( -ret );
    }
    return test();
}
