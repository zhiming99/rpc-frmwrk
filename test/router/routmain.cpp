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
#include <frmwrk.h>
#include <rpcroute.h>

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

CPPUNIT_TEST_SUITE_REGISTRATION( CIfRouterTest );

// router role: 1. reqfwdr, 2. bridge, 3. both
guint32 dwRole = 1;


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

        CIoManager* pSvc = m_pMgr;
        if( pSvc != nullptr )
            ret = pSvc->Start();
        else
            ret = -EFAULT;

        pSvc->SetCmdLineOpt(
            propRouterRole, dwRole );

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

CfgPtr CIfRouterTest::InitRouterCfg()
{
    CfgPtr ptrCfg;
    gint32 ret = 0;

    do{
        ret = CRpcServices::LoadObjDesc(
            ROUTER_OBJ_DESC,
            OBJNAME_ROUTER,
            true, ptrCfg );

        if( ERROR( ret ) )
            break;

        CCfgOpener oCfg( ( IConfigDb* )ptrCfg );
        oCfg[ propIoMgr ] = m_pMgr;
        oCfg[ propIfStateClass ] =
            clsid( CIfRouterState );

    }while( 0 );

    if( ERROR( ret ) )
    {
        std::string strMsg = DebugMsg(
            ret, "Error loading file router.json" );
        throw std::runtime_error( strMsg );
    }

    return ptrCfg;
}

void CIfRouterTest::testSvrStartStop()
{
    CPPUNIT_ASSERT( !m_pMgr.IsEmpty() );

    CfgPtr pCfg = InitRouterCfg();
    CPPUNIT_ASSERT( !pCfg.IsEmpty() );

    InterfPtr pIf;
    gint32 ret = pIf.NewObj(
        clsid( CRpcRouterImpl ), pCfg );
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    CInterfaceServer* pSvr = pIf;
    CPPUNIT_ASSERT( pSvr != nullptr );

    CPPUNIT_ASSERT( SUCCEEDED( pIf->Start() ) );

    while( pSvr->IsConnected() )
        sleep( 1 );

    CPPUNIT_ASSERT( SUCCEEDED( pIf->Stop() ) );
    
    return;
}

int main( int argc, char** argv )
{
    CppUnit::TextUi::TestRunner runner;
    CppUnit::TestFactoryRegistry& registry =
        CppUnit::TestFactoryRegistry::getRegistry();

    int opt = 0;
    int ret = 0;
    while( ( opt = getopt( argc, argv, "r:" ) ) != -1 )
    {
        switch (opt)
        {
        case 'r':
            {
                dwRole = ( guint32 )atoi( optarg );
                if( dwRole == 0 || dwRole > 3 )
                    ret = -EINVAL;
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
            "Usage: %s [-r <role number>]\n",
            argv[ 0 ] );
        exit( -ret );
    }

    runner.addTest( registry.makeTest() );
    bool wasSuccessful = runner.run( "", false );

    return !wasSuccessful;
}
