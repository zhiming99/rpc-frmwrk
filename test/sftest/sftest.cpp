/*
 * =====================================================================================
 *
 *       Filename:  sftest.cpp
 *
 *    Description:  implementation test for the CSimpFileServer/CSimpFileClient.
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

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

using namespace rpcf;
#include "sftest.h"
#include "sfsvr.h"

#define NSEC_PER_SEC 1000000000

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
        clsid( CSimpFileServer ),
        oCfg.GetCfg() );
    
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    ret = pIf->Start();
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );
    
    CSimpFileServer* pSvr = pIf;
    ret = pSvr->StartLoop();

    ret = pIf->Stop();
    pIf.Clear();

    CPPUNIT_ASSERT( SUCCEEDED( ret ) );
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
        clsid( CSimpFileClient ),
        oCfg.GetCfg() );
    
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    ret = pIf->Start();

    CPPUNIT_ASSERT( SUCCEEDED( ret ) );
    
    CSimpFileClient* pCli = pIf;
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

        // method from interface CSimpFileServer
        DebugPrint( 0, "Enumerate intefaces..." );
        IntVecPtr pClsids( true );
        ret = pCli->EnumInterfaces( pClsids );
        CPPUNIT_ASSERT( SUCCEEDED( ret ) );
        DebugPrint( 0, "Found %d interfaces",
            ( *pClsids )().size() );

        for( auto iClsid : ( *pClsids )() )
        {
            const std::string& strName =
                CoGetIfNameFromIid( ( EnumClsid )iClsid, "p" );
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

        timespec ts = { 0 }, ts2={ 0 };
        clock_gettime( CLOCK_REALTIME, &ts );

        DebugPrint( 0, "Uploading test..." );
        // method from interface CFileTransferServer
        ret = pCli->UploadFile(
            std::string( "./f100M.dat" ) );

        clock_gettime( CLOCK_REALTIME, &ts2 );
        int iCarry = 0;
        double nsec = .0;
        if( ts2.tv_nsec < ts.tv_nsec )
        {
            nsec = ts2.tv_nsec + ( 10 ^ 9 ) - ts.tv_nsec;
            iCarry = 1;
        }
        else
        {
            nsec = ts2.tv_nsec - ts.tv_nsec;
        }

        double dbTime1 = ( ( double )( ts2.tv_sec - ts.tv_sec - iCarry ) ) +
            ( nsec ) / NSEC_PER_SEC;
        if( SUCCEEDED( ret ) )
            DebugPrint( 0, "Upload Completed, time = %g secs", dbTime1 );
        else
            DebugPrint( ret, "Upload failed, time = %g secs", dbTime1 );

        if( ERROR( ret ) )
            break;

        DebugPrint( 0, "Downloading test..." );
        clock_gettime( CLOCK_REALTIME, &ts );
        ret = pCli->DownloadFile(
            std::string( "./f100M.dat" ) );

        clock_gettime( CLOCK_REALTIME, &ts2 );
        iCarry = 0;
        nsec = .0;
        if( ts2.tv_nsec < ts.tv_nsec )
        {
            nsec = ts2.tv_nsec + ( 10 ^ 9 ) - ts.tv_nsec;
            iCarry = 1;
        }
        else
        {
            nsec = ts2.tv_nsec - ts.tv_nsec;
        }

        double dbTime2 = ( ( double )( ts2.tv_sec - ts.tv_sec - iCarry ) ) +
            ( nsec ) / NSEC_PER_SEC;

        if( SUCCEEDED( ret ) )
            DebugPrint( 0, "Download Completed, time = %g secs", dbTime2 );
        else
            DebugPrint( ret, "Download failed, time = %g secs", dbTime2 );

        // CPPUNIT_ASSERT( SUCCEEDED( ret ) );
        DebugPrint( 0, "Upload/Download Completed" );

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
    pIf.Clear();

    CPPUNIT_ASSERT( SUCCEEDED( ret ) );
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
