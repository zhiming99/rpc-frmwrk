/*
 * =====================================================================================
 *
 *       Filename:  stmtest.cpp
 *
 *    Description:  implementation test for the CStreamingServer/CStreamingClient.
 *                  Note that this object is a multi-if object, unlike the
 *                  objects of all the earlier tests.
 *
 *        Version:  1.0
 *        Created:  12/04/2018 10:37:00 AM
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

#include "stmtest.h"
#include "stmsvr.h"

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
        "./stmdesc.json",
        OBJNAME_ECHOSVR,
        true, pCfg );

    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    ret = pIf.NewObj(
        clsid( CStreamingServer ),
        oCfg.GetCfg() );
    
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    ret = pIf->Start();
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );
    
    CStreamingServer* pSvr = pIf;
    while( pSvr->IsConnected() )
    {
        sleep( 1 );
        std::string strIfName = "CStreamingServer";
        if( pSvr->IsPaused( strIfName ) )
            break;
    }

    ret = pIf->Stop();
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    pIf.Clear();
}
#endif

#ifdef CLIENT
bool bPause = false;
void CIfSmokeTest::testCliStartStop()
{
    gint32 ret = 0;
    InterfPtr pIf;

    CPPUNIT_ASSERT( !m_pMgr.IsEmpty() );

    CCfgOpener oCfg;
    oCfg.SetObjPtr( propIoMgr, m_pMgr );

    CfgPtr pCfg = oCfg.GetCfg();
    ret = CRpcServices::LoadObjDesc(
        "./stmdesc.json",
        OBJNAME_ECHOSVR,
        false, pCfg );

    CPPUNIT_ASSERT( SUCCEEDED( ret ) );
    if( ERROR( ret ) )
        return;

    ret = pIf.NewObj(
        clsid( CStreamingClient ),
        oCfg.GetCfg() );
    
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    ret = pIf->Start();

    CPPUNIT_ASSERT( SUCCEEDED( ret ) );
    
    CStreamingClient* pCli = pIf;
    if( pCli != nullptr )
    {
        while( !pCli->IsConnected() )
            sleep( 1 );

        std::string strText( "Hello world!" );
        std::string strReply;

        // method from interface CEchoServer
        DebugPrint( 0, "Start..." );
        ret = pCli->Echo( strText, strReply );
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );
        DebugPrint( 0, "Completed" );
        if( strText != strReply )
            CPPUNIT_ASSERT( false );
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );

        // method from interface CStreamingServer
        IntVecPtr pClsids( true );
        ret = pCli->EnumInterfaces( pClsids );
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );
        DebugPrint( 0, "Completed" );

        system( "echo Hello, World! > ./hello-1.txt" );
        // method from interface CStreamingServer
        ret = pCli->UploadFile(
            std::string( "./hello-1.txt" ) );
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );
        DebugPrint( 0, "Upload Completed" );

        // method from interface CStreamingServer
        ret = pCli->DownloadFile(
            // server side file to download
            std::string( "./hello-1.txt" ), 
            // local file to save to
            std::string( "./dload-1.txt" ) ); 

        CPPUNIT_ASSERT( SUCCEEDED( ret ) );
        DebugPrint( 0, "Download Completed" );

        HANDLE hChannel = 0;
        TaskletPtr pSyncTask;
        ret = pCli->StartStream( hChannel, pSyncTask );
        CPPUNIT_ASSERT( SUCCEEDED( ret  ) );

        guint32 dwCount = 0;
        while( !pCli->CanSend() )
            sleep( 1 );

        printf( "Testing stream creation, \
            double-direction communication and active close\n" );

        BufPtr pBuf( true );
        *pBuf = std::string( "Server, are you ok?" );
        while( dwCount++ < 10 )
        {
            printf( "Writing to server... \n" );
            ret = pCli->WriteStream( hChannel, pBuf );
            if( SUCCEEDED( ret ) || ret == -EAGAIN )
            {
                sleep( 1 );
                continue;
            }
            break;
        }
        CPPUNIT_ASSERT( SUCCEEDED( ret  ) || ret == -EAGAIN );

        ret = pCli->CloseChannel( hChannel );
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );
        pSyncTask.Clear();

        printf( "Testing stream creation, \
            double-direction communication and active cancel\n" );

        ret = pCli->StartStream( hChannel, pSyncTask );
        CPPUNIT_ASSERT( SUCCEEDED( ret  ) );

        while( !pCli->CanSend() )
            sleep( 1 );

        *pBuf = std::string( "Server, i'm back." );
        while( dwCount++ < 20 )
        {
            printf( "Writing to server2... \n" );
            ret = pCli->WriteStream( hChannel, pBuf );
            if( SUCCEEDED( ret ) || ret == -EAGAIN )
            {
                sleep( 1 );
                continue;
            }
            break;
        }
        CPPUNIT_ASSERT( SUCCEEDED( ret  ) || ret == -EAGAIN );

        ret = pCli->CancelChannel( hChannel );
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );

        if( bPause )
        {
            ret = pCli->Pause_Proxy();
            CPPUNIT_ASSERT( SUCCEEDED( ret ) );
            DebugPrint( 0, "Pause interface Completed" );
        }
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

#ifdef CLIENT
    if( argc > 1 && strcmp( argv[ 1 ], "p" ) == 0 )
        bPause = true;
#endif

    runner.addTest( registry.makeTest() );
    bool wasSuccessful = runner.run( "", false );

    return !wasSuccessful;
}
