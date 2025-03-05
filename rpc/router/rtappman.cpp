/*
 * =====================================================================================
 *
 *       Filename:  rtappman.cpp
 *
 *    Description:  router's state report and simple auth support 
 *
 *        Version:  1.0
 *        Created:  02/27/2025 03:39:52 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:  
 *
 *      Copyright:  2025 Ming Zhi( woodhead99@gmail.com )
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
#include <limits.h>
#include "rpc.h"
#include "ifhelper.h"
#include "frmwrk.h"
#include "rpcroute.h"
#include "fastrpc.h"

#include "signal.h"

using namespace rpcf;
#include "routmain.h"
#include "AppManagercli.h"
#include "rtappman.h"

namespace rpcf
{

gint32 CreateAppManSync( CIoManager* pMgr,
    IConfigDb* pCfg, InterfPtr& pAppMan )
{
    gint32 ret = 0;
    do{
        TaskletPtr pTask;
        ret = pTask.NewObj(
            clsid( CSyncCallback ) );
        if( ERROR( ret ) )
            break;
        CSyncCallback* pSync = pTask;
        ret = CreateAppManagercli(
            pMgr, pTask, pCfg );
        if( ret == STATUS_PENDING )
        {
            ret = pSync->WaitForComplete();
            if( ERROR( ret ) )
                break;
            ret = pSync->GetError();
        }
        if( SUCCEEDED( ret ) )
        {
            Variant oVar;
            ret = pSync->GetProperty(
                propRespPtr, oVar );
            if( ERROR( ret ) )
                break;
            IConfigDb* pCfg = ( ObjPtr& )oVar;
            ret = pCfg->GetProperty( 0, oVar );
            if( ERROR( ret ) )
                break;
            pAppMan = ( ObjPtr& )oVar;
        }
    }while( 0 );
    return ret;
}

gint32 GetPointValuesToUpdate(
    InterfPtr& pRouter,
    std::vector< KeyValue >& veckv )
{
    gint32 ret = 0;
    do{
        KeyValue okv;
        okv.strKey = RTAPPNAME "/" O_SESSION;
        okv.oValue = 0;
        veckv.push_back( okv );
        CRpcRouterBridge* prb = pRouter;
        CRpcRouter* prt = pRouter;
        stdstr strVal;
        ret = DumpSessions( prb, strVal );
        if( SUCCEEDED( ret ) )
            veckv.back().oValue = strVal;

        okv.strKey = RTAPPNAME "/" O_BDGE_LIST;
        okv.oValue = 0;
        veckv.push_back( okv );
        ret = DumpBdgeList( prb, strVal );
        if( SUCCEEDED( ret ) )
            veckv.back().oValue = strVal;

        okv.strKey = RTAPPNAME "/" O_BPROXY_LIST;
        okv.oValue = 0;
        veckv.push_back( okv );
        ret = DumpBdgeProxyList( prt, strVal );
        if( SUCCEEDED( ret ) )
            veckv.back().oValue = strVal;

        okv.strKey = RTAPPNAME "/" O_RPROXY_LIST;
        okv.oValue = 0;
        veckv.push_back( okv );
        ret = DumpReqProxyList( prb, strVal );
        if( SUCCEEDED( ret ) )
            veckv.back().oValue = strVal;

        timespec ts = prt->GetStartTime();
        okv.strKey = RTAPPNAME "/" O_UPTIME;
        timespec ts2;
        ret = clock_gettime( CLOCK_REALTIME, &ts2 );
        if( SUCCEEDED( ret ) )
        {
            guint32 dwUptime =
                ts2.tv_sec - ts.tv_sec;
            okv.oValue = dwUptime;
            veckv.push_back( okv );
        }
        
        guint32 dwObjs = ( guint32 )
            CObjBase::GetActCount();
        okv.strKey = RTAPPNAME "/" O_OBJ_COUNT;
        okv.oValue = dwObjs;
        veckv.push_back( okv );


    }while( 0 );
    return ret;
}

//RPC event handler 'OnPointChanged'
gint32 CAsyncAMCallbacks::OnPointChanged(
    IConfigDb* context, 
    const std::string& strPtPath /*[ In ]*/,
    const Variant& value /*[ In ]*/ )
{
    gint32 ret = 0;
    do{
        if( strPtPath != "rpcrouter1/rpt_timer" )
        {
            ret = -ENOTSUP;
            break;
        }
        if( ( ( guint32& )value ) != 1 )
            break;

        std::vector< KeyValue > veckv;
        ret = GetPointValuesToUpdate(
            m_pRtBdge, veckv );
        if( ERROR( ret ) )
            break;

        InterfPtr pIf;
        ret = GetAppManagercli( pIf );
        if( ERROR( ret ) )
            break;

        CAppManager_CliImpl* pamc = pIf;
        CCfgOpener oCfg;
        oCfg.SetPointer( propIfPtr, pamc );
        ret = pamc->SetPointValues(
            oCfg.GetCfg(), veckv );
        if( ret == STATUS_PENDING )
            ret = 0;
    }while( 0 );
    return ret;
}

// RPC Async Req Callback
gint32 CAsyncAMCallbacks::ClaimAppInstCallback(
    IConfigDb* context, 
    gint32 iRet,
    std::vector<KeyValue>& arrPtToGet /*[ In ]*/ )
{
    gint32 ret = 0;
    do{
        if( ERROR( iRet ) )
        {
            OutputMsg( iRet, "Error ClaimAppInst" );
            kill( getpid(), SIGINT );
            break;
        }
        OutputMsg( iRet, "Successfully claimed "
            "application " RTAPPNAME );
    }while( 0 );
    return ret;
}

gint32 CAsyncAMCallbacks::FreeAppInstsCallback(
    IConfigDb* context, 
    gint32 iRet )
{
    if( context == nullptr )
        return -EINVAL;
    gint32 ret = 0;
    do{
        CCfgOpener oCfg( context );
        CSyncCallback* pSync = nullptr;
        ret = oCfg.GetPointer(
            propEventSink, pSync );
        if( ERROR( ret ) )
            break;
        pSync->OnEvent(
            eventTaskComp, iRet, 0, nullptr );
    }while( 0 );
    return ret;
}

gint32 CAsyncAMCallbacks::OnSvrOffline(
    IConfigDb* context,
    CAppManager_CliImpl* pIf )
{
    kill( getpid(), SIGINT );
    return 0;
}

}

gint32 StartAppManCli(
    CRpcServices* pSvc,
    InterfPtr& pAppMan )
{
    gint32 ret = 0;
    if( pSvc == nullptr )
        return -EINVAL;

    do{
        CRpcRouterManager* prtm = ObjPtr( pSvc );
        std::vector< InterfPtr > vecRouters;
        prtm->GetRouters( vecRouters );
        if( vecRouters.empty() )
            break;

        InterfPtr pIf;
        CRpcRouterBridge* prt = nullptr;
        for( auto& elem : vecRouters )
        {
            prt = elem;
            if( prt != nullptr )
            {
                pIf = elem;
                break;
            }
        }
        if( prt == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        InterfPtr pAppMan;
        CParamList oParams;
        oParams.SetPointer( propIfPtr, prt );
        ret = CreateAppManSync( prt->GetIoMgr(),
            oParams.GetCfg(), pAppMan );
        if( ERROR( ret ) )
            break;
        PACBS pacbs( new CAsyncAMCallbacks );
        CParamList oParams2;
        oParams2.SetPointer( propIfPtr, prt );
        CAppManager_CliImpl* pamc = pAppMan;
        pamc->SetAsyncCallbacks(
            pacbs, oParams2.GetCfg() );

        std::vector< KeyValue > veckv;
        std::vector< KeyValue > rveckv;
        ret = GetPointValuesToUpdate( pIf, veckv);
        if( ERROR( ret ) )
        {
            OutputMsg( ret,
                "Error getting point values" );
            break;
        }
        ret = pamc->ClaimAppInst(
            oParams2.GetCfg(),
            RTAPPNAME, veckv, rveckv );
        
    }while( 0 );
    return ret;
}

gint32 StopAppManCli()
{
    InterfPtr pIf;
    gint32 ret = GetAppManagercli( pIf );
    if( ERROR( ret ) )
        return ret;
    CAppManager_CliImpl* pamc = pIf;
    if( pamc->GetState() == stateConnected )
    {
        TaskletPtr pTask;
        pTask.NewObj( clsid( CSyncCallback ) );
        CParamList oParams;
        oParams.SetPointer( propEventSink,
            ( IEventSink* )pTask );
        std::vector< stdstr > vecApps;
        vecApps.push_back( RTAPPNAME );

        PACBS pCbs;
        CfgPtr pContext;
        ret = pamc->GetAsyncCallbacks(
            pCbs, pContext );
        if( SUCCEEDED( ret ) )
        {
            ret = pamc->FreeAppInsts(
                oParams.GetCfg(), vecApps );
            if( ret == STATUS_PENDING )
            {
                CSyncCallback* pSync = pTask;
                pSync->WaitForComplete();
                ret = pSync->GetError();
                if( ERROR( ret ) )
                    OutputMsg( ret,
                    "Error FreeAppInsts" );
            }
        }
    }
    ret = DestroyAppManagercli(
        pamc->GetIoMgr(), nullptr );
    pamc->ClearCallbacks();
    return ret;
}

