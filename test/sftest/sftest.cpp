/*
 * =====================================================================================
 *
 *       Filename:  sftest.cpp
 *
 *    Description:  implementation test for the CSendFetchServer/CSendFetchClient.
 *                  Note that this object is a multi-if object, unlike the
 *                  objects of all the earlier tests.
 *
 *        Version:  1.0
 *        Created:  11/1/2018 11:25:09 AM
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

#include "sftest.h"
#include "sfsvr.h"

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
        "./sfdesc.json",
        OBJNAME_ECHOSVR,
        true, pCfg );

    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    ret = pIf.NewObj(
        clsid( CSendFetchServer ),
        oCfg.GetCfg() );
    
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    ret = pIf->Start();
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );
    
    CSendFetchServer* pSvr = pIf;
    while( pSvr->IsConnected() )
    {
        sleep( 1 );
        std::string strIfName = "CSendFetchServer";
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
        "./sfdesc.json",
        OBJNAME_ECHOSVR,
        false, pCfg );

    CPPUNIT_ASSERT( SUCCEEDED( ret ) );
    if( ERROR( ret ) )
        return;

    ret = pIf.NewObj(
        clsid( CSendFetchClient ),
        oCfg.GetCfg() );
    
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    ret = pIf->Start();

    CPPUNIT_ASSERT( SUCCEEDED( ret ) );
    
    CSendFetchClient* pCli = pIf;
    CPPUNIT_ASSERT( pCli != nullptr );

    do{
        while( !pCli->IsConnected() )
            sleep( 1 );

        std::string strText( "Hello world!" );
        std::string strReply;

        // method from interface CEchoServer
        DebugPrint( 0, "Start..." );
        ret = pCli->Echo( strText, strReply );
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );
        DebugPrint( 0, "Echo Completed" );
        if( strText != strReply )
            CPPUNIT_ASSERT( false );
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );

        // method from interface CSendFetchServer
        DebugPrint( 0, "Enumerate intefaces..." );
        IntVecPtr pClsids( true );
        ret = pCli->EnumInterfaces( pClsids );
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );
        DebugPrint( 0, "Found %d interfaces",
            ( *pClsids )().size() );

        for( auto iClsid : ( *pClsids )() )
        {
            const std::string& strName =
                CoGetIfNameFromIid( ( EnumClsid )iClsid );
            if( strName.empty() )
            {
                const char* pName =
                    CoGetClassName( ( EnumClsid )iClsid );
                if( pName == nullptr )
                    continue;

                DebugPrint( 0, "Find interface: %s", pName );
            }
            else
            {
                DebugPrint( 0, "Find interface: %s",
                    strName.c_str() );
            }
        }

        DebugPrint( 0, "EnumInterfaces Completed" );

        system( "echo Hello, World! > ./hello-1.txt" );
        // method from interface CFileTransferServer
        ret = pCli->UploadFile(
            std::string( "./hello-1.txt" ) );

        if( ERROR( ret ) )
            break;

        if( SUCCEEDED( ret ) );
            DebugPrint( 0, "Upload Completed" );

        // method from interface CFileTransferServer
        ret = pCli->DownloadFile(
            // server side file to download
            std::string( "./hello-1.txt" ), 
            // local file to save to
            std::string( "./dload-1.txt" ) ); 

        CPPUNIT_ASSERT( SUCCEEDED( ret ) );
        DebugPrint( 0, "Download Completed" );

        BufPtr pCount( true );
        ret = pCli->GetCounter(
            propMsgCount, pCount );
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );
        DebugPrint( 0, "GetCounter Completed, recv count is %d",
            ( guint32 )*pCount  );

        ret = pCli->GetCounter(
            propMsgRespCount, pCount );
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );
        DebugPrint( 0, "GetCounter Completed, resp count is %d",
            ( guint32 )*pCount  );

        if( bPause )
        {
            ret = pCli->Pause_Proxy();
            CPPUNIT_ASSERT( SUCCEEDED( ret ) );
            DebugPrint( 0, "Pause interface Completed" );
        }

    }while( 0 );

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
