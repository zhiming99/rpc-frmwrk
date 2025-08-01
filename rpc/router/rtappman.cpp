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

using namespace rpcf;
#include "routmain.h"
#include "rtappman.h"

gint32 CAsyncAMCallbacks::GetPointValuesToUpdate(
    InterfPtr& pRouter,
    std::vector< KeyValue >& veckv )
{
    gint32 ret = 0;
    do{
        KeyValue okv;
        BufPtr pBuf( true );
        okv.strKey = O_SESSION;
        okv.oValue = 0;
        veckv.push_back( okv );
        CRpcRouterBridge* prb = pRouter;
        CRpcRouter* prt = pRouter;
        stdstr strVal;
        ret = DumpSessions( prb, strVal );
        if( SUCCEEDED( ret ) )
        {
            ret = pBuf->Append( strVal.c_str(),
                strVal.size());
            if( SUCCEEDED( ret ) )
                veckv.back().oValue = pBuf;
            pBuf.Clear();
        }

        pBuf.NewObj();
        okv.strKey = O_BDGE_LIST;
        okv.oValue = 0;
        veckv.push_back( okv );
        ret = DumpBdgeList( prb, strVal );
        if( SUCCEEDED( ret ) )
        {
            ret = pBuf->Append( strVal.c_str(),
                strVal.size());
            if( SUCCEEDED( ret ) )
                veckv.back().oValue = pBuf;
            pBuf.Clear();
        }

        pBuf.NewObj();
        okv.strKey = O_BPROXY_LIST;
        okv.oValue = 0;
        veckv.push_back( okv );
        ret = DumpBdgeProxyList( prt, strVal );
        if( SUCCEEDED( ret ) )
        {
            ret = pBuf->Append( strVal.c_str(),
                strVal.size());
            if( SUCCEEDED( ret ) )
                veckv.back().oValue = pBuf;
            pBuf.Clear();
        }

        pBuf.NewObj();
        okv.strKey = O_RPROXY_LIST;
        okv.oValue = 0;
        veckv.push_back( okv );
        ret = DumpReqProxyList( prb, strVal );
        if( SUCCEEDED( ret ) )
        {
            ret = pBuf->Append( strVal.c_str(),
                strVal.size());
            if( SUCCEEDED( ret ) )
                veckv.back().oValue = pBuf;
            pBuf.Clear();
        }

        timespec ts = prt->GetStartTime();
        okv.strKey = O_UPTIME;
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
        okv.strKey = O_OBJ_COUNT;
        okv.oValue = dwObjs;
        veckv.push_back( okv );

        CRpcRouterManager* prtmgr =
            ObjPtr( prb->GetParent() );
        if( prtmgr == nullptr )
            break;
        Variant oVar;
        ret = prtmgr->GetProperty(
            propRxBytes, oVar );
        if( SUCCEEDED( ret ) )
        {
            okv.strKey = O_RXBYTE;
            okv.oValue = oVar;
            veckv.push_back( okv );
        }

        ret = prtmgr->GetProperty(
            propTxBytes, oVar );
        if( SUCCEEDED( ret ) )
        {
            okv.strKey = O_TXBYTE;
            okv.oValue = oVar;
            veckv.push_back( okv );
        }

        ret = prtmgr->GetProperty(
            propMaxConns, oVar );
        if( SUCCEEDED( ret ) )
        {
            okv.strKey = S_MAX_CONN;
            okv.oValue = oVar;
            veckv.push_back( okv );
        }

        ret = prtmgr->GetProperty(
            propMaxPendings, oVar );
        if( SUCCEEDED( ret ) )
        {
            okv.strKey = S_MAX_PENDINGS;
            okv.oValue = oVar;
            veckv.push_back( okv );
        }

        ret = prtmgr->GetProperty(
            propSendBps, oVar );
        if( SUCCEEDED( ret ) )
        {
            okv.strKey = S_MAX_SENDBPS;
            okv.oValue = oVar;
            veckv.push_back( okv );
        }

        ret = prtmgr->GetProperty(
            propRecvBps, oVar );
        if( SUCCEEDED( ret ) )
        {
            okv.strKey = S_MAX_RECVBPS;
            okv.oValue = oVar;
            veckv.push_back( okv );
        }

        okv.strKey = O_CONNECTIONS;
        okv.oValue = ( guint32 )
            prb->GetBridgeCount();
        veckv.push_back( okv );

        okv.strKey = O_VMSIZE_KB;
        okv.oValue = GetVmSize();
        veckv.push_back( okv );

        okv.strKey = O_OPEN_FILES;
        guint32 dwCount = 0;
        ret = GetOpenFileCount(
            getpid(), dwCount );
        if( SUCCEEDED( ret ) )
        {
            okv.oValue = dwCount;
            veckv.push_back( okv );
        }

        okv.strKey = O_CPU_LOAD;
        okv.oValue = GetCpuUsage();
        veckv.push_back( okv );

    }while( 0 );
    if( veckv.size() && ERROR( ret ) )
        ret = 0;
    return ret;
}

gint32 CAsyncAMCallbacks::GetPointValuesToInit(
    InterfPtr& pIf,
    std::vector< KeyValue >& veckv )
{
    return super::GetPointValuesToInit( pIf, veckv );
}

gint32 StartAppManCli(
    CRpcServices* pSvc, InterfPtr& pAppMan )
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

        PACBS pacbs( new CAsyncAMCallbacks );
        auto pcacbs = ( CAsyncAMCallbacks* )pacbs.get();

        // ignore the return status
        // in order to allowing reconnection if the
        // appmonsvr is not online yet
        StartStdAppManCli(
            pIf, RTAPPNAME, pAppMan, pacbs );
        
    }while( 0 );
    return ret;
}

gint32 StopAppManCli()
{
    return StopStdAppManCli();
}

