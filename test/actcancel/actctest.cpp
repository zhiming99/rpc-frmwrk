/*
 * =====================================================================================
 *
 *       Filename:  asynctest.cpp
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

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>

#include <cppunit/extensions/HelperMacros.h>


#include <rpc.h>
#include <proxy.h>

using namespace rpcf;
#include "actctest.h"
#include "actcsvr.h"

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
        OBJDESC_PATH,
        OBJNAME_SERVER,
        true, pCfg );

    if( ERROR( ret ) )
        return;

    ret = pIf.NewObj(
        clsid( CActcServer ),
        oCfg.GetCfg() );
    
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    ret = pIf->Start();
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );
    
    CActcServer* pSvr = pIf;
    while( pSvr->IsConnected() )
        sleep( 1 );

    ret = pIf->Stop();

    pIf.Clear();

    CPPUNIT_ASSERT( SUCCEEDED( ret ) );
}
#endif

#ifdef CLIENT
void CIfSmokeTest::testCliActCancel()
{

    gint32 ret = 0;
    InterfPtr pIf;

    CPPUNIT_ASSERT( !m_pMgr.IsEmpty() );

    CCfgOpener oCfg;
    oCfg.SetObjPtr( propIoMgr, m_pMgr );

    CfgPtr pCfg = oCfg.GetCfg();

    ret = CRpcServices::LoadObjDesc(
        OBJDESC_PATH,
        OBJNAME_SERVER,
        false, pCfg );

    CPPUNIT_ASSERT( SUCCEEDED( ret ) );
    if( ERROR( ret ) )
        return;

    ret = pIf.NewObj(
        clsid( CActcClient ),
        oCfg.GetCfg() );
    
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );

    ret = pIf->Start();

    CPPUNIT_ASSERT( SUCCEEDED( ret ) );
    
    CActcClient* pCli = pIf;
    while( pCli != nullptr )
    {
        while( !pCli->IsConnected() )
            sleep( 1 );

        CfgPtr pResp( true );

        CParamList oParams( ( IConfigDb* )pResp );
        std::string strText( "Hello world!" );
        std::string strReply;

        printf( "\n" );
        DebugPrint( 0, "Start..." );

        // send out the request
        ret = pCli->LongWait(
            pResp, strText, strReply );

        CPPUNIT_ASSERT( ret == STATUS_PENDING );

        // NOTE: race condition exists. Normally we
        // should not touch pResp before the request
        // complete, otherwise there is the risk the
        // pResp being accessed by multiple users at
        // the same time.
        guint64 qwTaskId = 0;
        if( oParams.exist( propTaskId ) )
        {
            qwTaskId = oParams[ propTaskId ];
        }

        //FIXME: if the system is too busy, there is
        //risk the cancel command can arrive ahead of
        //the request to cancel.

        // we are about to actively cancel this request
        OutputMsg( 0, "testing sync canceling..." );
        CPPUNIT_ASSERT( qwTaskId != 0 );
        do{
            ret = pCli->CancelRequest( qwTaskId );
            if( SUCCEEDED( ret ) )
            {
                OutputMsg( 0, "Request is successfully cancelled" );
                break;
            }
            else
            {
                if( ret == -ENOENT )
                {
                    OutputMsg( ret, "Request is not found to cancel" );
                }
                else
                {
                    OutputMsg( ret, "canceling failed" );
                    break;
                }
            }

        }while( 0 );
        if( ERROR( ret ) )
            break;

        oParams.Clear();
        oParams.Reset();

        // send out the request again for asynchronous cancel
        OutputMsg( 0, "testing async canceling..." );
        ret = pCli->LongWait(
            oParams.GetCfg(), strText, strReply );

        if( ret != STATUS_PENDING )
        {
            ret = ERROR_STATE;
            break;
        }
        qwTaskId = oParams[ propTaskId ];

        do{
            TaskletPtr pTask;
            ret = pTask.NewObj(
                clsid( CIoReqSyncCallback ) );
            if( ERROR( ret ) )
                break;

            ret = pCli->CancelReqAsync(
                pTask, qwTaskId );

            if( SUCCEEDED( ret ) )
            {
                OutputMsg( 0,
                    "Request is successfully cancelled" );
                break;
            }

            if( ret == STATUS_PENDING )
            do{
                CIoReqSyncCallback* pSync = pTask;
                pSync->WaitForComplete();
                ret = pSync->GetError();
                if( ERROR( ret ) )
                    break;

                IConfigDb* pResp = nullptr;
                CCfgOpenerObj oTask( pSync );
                ret = oTask.GetPointer(
                    propRespPtr, pResp );
                if( ERROR( ret ) )
                    break;

                gint32 iRet = 0;
                CCfgOpener oResp( pResp );
                ret = oResp.GetIntProp(
                    propReturnValue,
                    ( guint32& )iRet );
                if( SUCCEEDED( ret ) )
                    ret = iRet;

            }while( 0 );

            if( ret == -ENOENT )
            {
                OutputMsg( ret,
                    "Request is not found to cancel" );
            }
            else if( ERROR( ret ) )
            {
                OutputMsg( ret, "canceling failed" );
                break;
            }
            else
            {
                OutputMsg( ret,
                    "Request 2 is successfully cancelled" );
                break;
            }

        }while( 0 );

        oParams.Clear();
        break;
    }

    if( pCli == nullptr )
        CPPUNIT_ASSERT( false );

    pIf->Stop();
    pIf.Clear();
    CPPUNIT_ASSERT( SUCCEEDED( ret ) );
}

#endif

int main( int argc, char** argv )
{

    CppUnit::TextUi::TestRunner runner;
    CppUnit::TestFactoryRegistry &registry =
        CppUnit::TestFactoryRegistry::getRegistry();

    runner.addTest( registry.makeTest() );
    bool wasSuccessful = runner.run( "", false );

    return !wasSuccessful;
}
