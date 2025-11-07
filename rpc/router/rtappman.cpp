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
#include "jsondef.h"

using namespace rpcf;
#include "routmain.h"
#include "rtappman.h"

static gint32 DumpMmhNode( InterfPtr& pRouter,
    const Variant& oVar, BufPtr& pBuf )
{
    gint32 ret = 0;
    if( pRouter.IsEmpty() )
        return -EINVAL;
    do{
        Json::StreamWriterBuilder oBuilder;
        oBuilder["commentStyle"] = "None";
        oBuilder["indentation"] = "   ";
        Json::Value oArray( Json::arrayValue);
        const ObjVecPtr pvecNodes = ( const ObjPtr& )oVar;
        if( pvecNodes.IsEmpty() )
        {
            ret = -ENOENT;
            break;
        }

        const std::vector< ObjPtr >& vecNodes =
            ( *pvecNodes )();
        for( auto& elem : vecNodes )
        {
            CCfgOpener oConn( ( const IConfigDb* )elem );
            Json::Value oJConn( Json::objectValue );
            stdstr strVal;
            guint32 dwVal;
            bool bVal;
            ret = oConn.GetStrProp(
                propNodeName, strVal );
            if( SUCCEEDED( ret ) )
                oJConn[ JSON_ATTR_NODENAME ] = strVal;

            CRpcRouterBridge* prbt = pRouter;
            guint32 dwPortId;
            ret = prbt->GetProxyIdByNodeName(
                strVal, dwPortId );
            bVal = true;
            if( ERROR( ret ) )
                bVal = false;

            oJConn[ "online" ] = bVal ? "true" : "false";

            ret = oConn.GetStrProp(
                propAddrFormat, strVal );
            if( SUCCEEDED( ret ) )
                oJConn[ JSON_ATTR_ADDRFORMAT ] = strVal;

            ret = oConn.GetStrProp(
                propDestIpAddr, strVal );
            if( SUCCEEDED( ret ) )
                oJConn[ JSON_ATTR_IPADDR ] = strVal;

            ret = oConn.GetIntProp(
                propDestTcpPort, dwVal );
            if( SUCCEEDED( ret ) )
                oJConn[ JSON_ATTR_TCPPORT ] = dwVal;

            ret = oConn.GetBoolProp(
                propEnableSSL, bVal );
            if( SUCCEEDED( ret ) )
            {
                if( bVal )
                    oJConn[ JSON_ATTR_ENABLE_SSL ] = "true";
                else
                    oJConn[ JSON_ATTR_ENABLE_SSL ] = "false";
            }

            ret = oConn.GetBoolProp(
                propEnableWebSock, bVal );
            if( SUCCEEDED( ret ) )
            {
                if( bVal )
                    oJConn[ JSON_ATTR_ENABLE_WEBSOCKET ] = "true";
                else
                    oJConn[ JSON_ATTR_ENABLE_WEBSOCKET ] = "false";
            }

            ret = oConn.GetBoolProp(
                propCompress, bVal );
            if( SUCCEEDED( ret ) )
            {
                if( bVal )
                    oJConn[ JSON_ATTR_ENABLE_COMPRESS ] = "true";
                else
                    oJConn[ JSON_ATTR_ENABLE_COMPRESS ] = "false";
            }

            ret = oConn.GetBoolProp(
                propConnRecover, bVal );
            if( SUCCEEDED( ret ) )
            {
                if( bVal )
                    oJConn[ JSON_ATTR_CONN_RECOVER ] = "true";
                else
                    oJConn[ JSON_ATTR_CONN_RECOVER ] = "false";
            }
            ret = oConn.GetStrProp(
                propDestUrl, strVal );
            if( SUCCEEDED( ret ) )
                oJConn[ JSON_ATTR_DEST_URL ] = strVal;
            oArray.append( oJConn );
        }
        
        stdstr strOutput = Json::writeString(
            oBuilder, oArray );
        ret = pBuf.NewObj();
        if( ERROR( ret ) )
            break;
        ret = pBuf->Append( strOutput.c_str(),
            strOutput.size());

    }while( 0 );
    return ret;
}

stdstr g_strMonName = RTAPPNAME;
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

        CRpcRouterManager* prtmgr =
            ObjPtr( prb->GetParent() );
        if( prtmgr == nullptr )
            break;

        Variant oVar;
        okv.strKey = O_MMHNODE_LIST;
        ret = prtmgr->GetProperty(
            propMmhNodeList, oVar );
        if( SUCCEEDED( ret ) )
        {
            ret = DumpMmhNode( pRouter, oVar, pBuf );
            if( SUCCEEDED( ret ) )
            {
                okv.oValue = pBuf;
                veckv.push_back( okv );
            }
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

        /*ret = prtmgr->GetProperty(
            propMaxConns, oVar );
        if( SUCCEEDED( ret ) )
        {
            okv.strKey = S_MAX_CONN;
            okv.oValue = oVar;
            veckv.push_back( okv );
        }*/

        ret = prtmgr->GetProperty(
            propMaxPendings, oVar );
        if( SUCCEEDED( ret ) )
        {
            okv.strKey = S_MAX_PENDINGS;
            okv.oValue = oVar;
            veckv.push_back( okv );
        }

        /*ret = prtmgr->GetProperty(
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
        }*/

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
            ret = iRet;
            break;
        }
        CRpcRouter* pBdgeRt = ObjPtr( m_pIf );
        CRpcRouterManager* prtmgr =
            ObjPtr( pBdgeRt->GetParent() );
        for( auto& kv : arrPtToGet )
        {
            if( kv.strKey == S_SESS_TIME_LIMIT )
            {
                prtmgr->SetProperty(
                    propSessTimeLimit, kv.oValue );
            }
            else if( kv.strKey == S_MAX_CONN )
            {
                prtmgr->SetProperty(
                    propMaxConns, kv.oValue );
            }
            else if( kv.strKey == S_MAX_RECVBPS )
            {
                prtmgr->SetProperty(
                    propRecvBps, kv.oValue );
            }
            else if( kv.strKey == S_MAX_SENDBPS )
            {
                prtmgr->SetProperty(
                    propSendBps, kv.oValue );
            }
        }
    }while( 0 );

    return super::ClaimAppInstCallback(
        context, iRet, arrPtToGet );
}

gint32 CAsyncAMCallbacks::OnPointChanged(
    IConfigDb* context, 
    const std::string& strPtPath /*[ In ]*/,
    const Variant& value /*[ In ]*/ )
{
    do{
        CRpcRouter* pBdgeRt = ObjPtr( m_pIf );
        CRpcRouterManager* prtmgr =
            ObjPtr( pBdgeRt->GetParent() );
        if( unlikely( prtmgr == nullptr ) )
        {
            super::OnPointChanged(
                context, strPtPath, value );
            break;
        }
        if( strPtPath ==
            g_strMonName + "/" S_SESS_TIME_LIMIT  )
        {
            guint32 dwWaitSec = value;
            if( dwWaitSec == 0 )
                break;
            if( dwWaitSec > 86400 * 31 )
                break;
            prtmgr->SetProperty(
                propSessTimeLimit, value );
        }
        else if( strPtPath ==
            g_strMonName + "/" S_MAX_RECVBPS )
        {
            guint64 qwVal = value;
            prtmgr->SetProperty(
                propRecvBps, value );
        }
        else if( strPtPath ==
            g_strMonName + "/" S_MAX_SENDBPS )
        {
            guint64 qwVal = value;
            prtmgr->SetProperty(
                propSendBps, value );
        }
        else if( strPtPath ==
            g_strMonName + "/" S_MAX_CONN )
        {
            guint32 dwVal = value;
            if( dwVal == 0 )
                break;
            prtmgr->SetProperty(
                propMaxConns, value );
        }
        else
        {
            super::OnPointChanged(
                context, strPtPath, value );
        }
    }while( 0 );
    return 0;
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

        Variant oVar;
        PACBS pacbs( new CAsyncAMCallbacks );
        auto pcacbs = ( CAsyncAMCallbacks* )pacbs.get();
        if( g_strMonName == RTAPPNAME )
        { 
            ret = pSvc->GetProperty(
                propMonAppInsts, oVar );
            if( SUCCEEDED( ret ) )
            {
                StrVecPtr pvecStrs = oVar.m_pObj;
                if( !pvecStrs.IsEmpty() )
                {
                    if( ( *pvecStrs )().size() )
                        g_strMonName = ( *pvecStrs )().front();
                }
            }
            else
            {
                ret = 0;
            }
        }
        // ignore the return status
        // in order to allowing reconnection if the
        // appmonsvr is not online yet
        StartStdAppManCli(
            pIf, g_strMonName, pAppMan, pacbs );
        
    }while( 0 );
    return ret;
}

gint32 StopAppManCli()
{
    return StopStdAppManCli();
}

