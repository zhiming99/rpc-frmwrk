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
 *      Copyright:  2019 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
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
guint32 g_dwRole = 1;
bool g_bCompress = false;

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
        {
            pSvc->SetCmdLineOpt(
                propObjDescPath,
                ROUTER_OBJ_DESC );

            pSvc->SetCmdLineOpt(
                propRouterRole, g_dwRole );

            if( g_bCompress )
            {
                pSvc->SetCmdLineOpt(
                    propCompress, g_bCompress );
            }

            ret = pSvc->Start();
        }
        else
            ret = -EFAULT;

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

CfgPtr CIfRouterTest::InitRouterCfg(
    guint32 dwRole )
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

        oCfg[ propRouterRole ] = dwRole;

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

    CfgPtr pCfgFwdr;
    CfgPtr pCfgBdge;

    if( g_dwRole & 0x01 )
        pCfgFwdr = InitRouterCfg( 0x01 );

    if( g_dwRole & 0x02 )
        pCfgBdge = InitRouterCfg( 0x02 );

    InterfPtr pIfFwdr;
    InterfPtr pIfBdge;

    gint32 ret = 0;
    do{
        if( !pCfgFwdr.IsEmpty() )
        {
            ret =  pIfFwdr.NewObj(
                clsid( CRpcRouterImpl ),
                pCfgFwdr );

            if( ERROR( ret ) )
                break;
                
            CInterfaceServer* pSvrFwdr = pIfFwdr;
            if( unlikely( pSvrFwdr == nullptr ) )
            {
                ret = -EFAULT;
                break;
            }
            ret = pIfFwdr->Start();
            if( ERROR( ret ) )
                break;
        }

        if( !pCfgBdge.IsEmpty() )
        {
            ret =  pIfBdge.NewObj(
                clsid( CRpcRouterImpl ), pCfgBdge );

            CInterfaceServer* pSvrBdge = pIfBdge;
            if( unlikely( pSvrBdge == nullptr ) )
            {
                ret = -EFAULT;
                break;
            }
            ret = pIfBdge->Start();
            if( ERROR( ret ) )
                break;
        }


        if( g_dwRole == 0x01 )
        {
            CInterfaceServer* pSvr = pIfFwdr;
            while( pSvr->IsConnected() )
                sleep( 1 );

        }
        else if( g_dwRole == 0x02 )
        {
            CInterfaceServer* pSvr = pIfBdge;
            while( pSvr->IsConnected() )
                sleep( 1 );
        }
        else
        {
            CInterfaceServer* pSvrFwdr = pIfFwdr;
            CInterfaceServer* pSvrBdge = pIfBdge;
            while( pSvrFwdr->IsConnected() &&
                pSvrBdge->IsConnected() )
                sleep( 1 );
        }

    }while( 0 );

    if( !pIfFwdr.IsEmpty() )
        ret = pIfFwdr->Stop();

    if( !pIfBdge.IsEmpty() )
        ret = pIfBdge->Stop();

    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    return;
}

int main( int argc, char** argv )
{
    CppUnit::TextUi::TestRunner runner;
    CppUnit::TestFactoryRegistry& registry =
        CppUnit::TestFactoryRegistry::getRegistry();

    int opt = 0;
    int ret = 0;
    while( ( opt = getopt( argc, argv, "cr:" ) ) != -1 )
    {
        switch (opt)
        {
        case 'c':
            {
                // compression required
                g_bCompress = true;
                break;
            }
        case 'r':
            {
                g_dwRole = ( guint32 )atoi( optarg );
                if( g_dwRole == 0 || g_dwRole > 3 )
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
