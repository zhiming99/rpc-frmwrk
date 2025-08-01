/*
 * =====================================================================================
 *
 *       Filename:  rtdump.cpp
 *
 *    Description:  functions of dumping router infomation to string
 *
 *        Version:  1.0
 *        Created:  03/01/2025 12:43:51 PM
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

#include <rpc.h>
using namespace rpcf;
#include <ifhelper.h>
#include <frmwrk.h>
#include <rpcroute.h>
#include "routmain.h"
#include "jsondef.h"
#include "security/security.h"
#include "taskschd.h"

namespace rpcf 
{
Json::Value DumpConnParams( const IConfigDb* pConn )
{
    bool bVal;
    gint32 ret = 0;
    Json::Value oConnVal( Json::objectValue );
    CConnParams oConn( pConn );

    stdstr strVal = oConn.GetSrcIpAddr();
    if( strVal.size() )
        oConnVal[ "SrcIpAddr" ] = strVal;

    guint32 dwVal = oConn.GetSrcPortNum();
    if( dwVal != 0 )
        oConnVal[ "SrcPortNum" ] = dwVal;

    strVal = oConn.GetDestIpAddr();
    if( strVal.size() )
        oConnVal[ "DestIpAddr" ] = strVal;

    dwVal = oConn.GetDestPortNum();

    if( dwVal != 0 )
        oConnVal[ "DestPortNum" ] = dwVal;

    bVal = oConn.IsCompression();
    oConnVal[ JSON_ATTR_ENABLE_COMPRESS ] = bVal;

    bVal = oConn.IsWebSocket();
    oConnVal[ JSON_ATTR_ENABLE_WEBSOCKET ] = bVal;

    if( bVal )
        oConnVal[ JSON_ATTR_DEST_URL ] = oConn.GetUrl();

    bVal = oConn.IsSSL();
    oConnVal[ JSON_ATTR_ENABLE_SSL ] = bVal;

    bVal = oConn.HasAuth(); 
    oConnVal[ JSON_ATTR_HASAUTH ] = bVal;

    if( !bVal )
        return oConnVal;

    ObjPtr pa;
    Variant oVar;
    ret = pConn->GetProperty( propAuthInfo, oVar );
    if( SUCCEEDED( ret ) )
    {
        pa = ( ObjPtr& )oVar;
        CCfgOpener oAuth( ( IConfigDb* )pa );
        ret = oAuth.GetStrProp(
            propAuthMech, strVal );
        const char* szMech = JSON_ATTR_HASAUTH;
        if( SUCCEEDED( ret ) )
            oConnVal[ szMech ] = strVal;
    }
    return oConnVal;
}

gint32 DumpStream(
    InterfPtr& pUxStream,
    Json::Value& oStmInfo )
{
    if( pUxStream.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CRpcServices* pSvc = pUxStream;
        CStdRMutex oLock( pSvc->GetLock() ); 
        EnumIfState iState = pSvc->GetState();
        stdstr strState = IfStateToString( iState );
        oStmInfo[ "StreamState" ] = strState;

        CCfgOpenerObj oIfCfg( pSvc );
        IConfigDb* pDesc;
        ret = oIfCfg.GetPointer(
            propDataDesc, pDesc );

        if( SUCCEEDED( ret ) )
        {
            stdstr strName;
            CCfgOpener oDesc( pDesc );
            ret = oDesc.GetStrProp(
                propNodeName, strName );
            if( SUCCEEDED( ret ) )
                oStmInfo[ "ClientName" ] = strName;
        }

        if( pSvc->IsServer() )
        {
            CUnixSockStmServer* pSvr = pUxStream; 
            guint64 qwRxBytes, qwTxBytes;
            pSvr->GetBytesTransfered(
                qwRxBytes, qwTxBytes );
            oStmInfo[ "BytesToServer" ] =
                ( Json::UInt64 )qwRxBytes;
            oStmInfo[ "BytesToClient" ] =
                ( Json::UInt64 )qwTxBytes;
            oStmInfo[ "CanSend" ] =
                pSvr->CanSend();
        }
        else
        {
            CUnixSockStmProxy* pProxy = pUxStream; 
            guint64 qwRxBytes, qwTxBytes;
            pProxy->GetBytesTransfered(
                qwRxBytes, qwTxBytes );
            oStmInfo[ "BytesToClient" ] =
                ( Json::UInt64 )qwRxBytes;
            oStmInfo[ "BytesToServer" ] =
                ( Json::UInt64 )qwTxBytes;
            oStmInfo[ "CanSend" ] =
                pProxy->CanSend();
            CRpcTcpBridgeProxyStream* pbpstm =
                pUxStream;
            if( pbpstm == nullptr )
                break;
            oStmInfo[ "BridgeStreamId" ] =
                pbpstm->GetStreamId( false );
            oStmInfo[ "BridgeProxyStreammId" ] =
                pbpstm->GetStreamId( true );

            CRpcTcpBridgeProxy* pbp =
                pbpstm->GetBridgeProxy();

            oLock.Unlock();
            if( pbp == nullptr )
                break;
            CCfgOpenerObj oIfCfg( pbp );
            guint32 dwPortId = 0;
            ret = oIfCfg.GetIntProp(
                propPortId, dwPortId );
            if( ERROR( ret ) )
                break;
            oStmInfo[ "ProxyPortId" ] =
                ( Json::UInt )dwPortId;
        }

    }while( 0 );

    return ret;
}

gint32 DumpBdgeList(
    CRpcRouterBridge* pRouter,
    stdstr& strOutput )
{
    gint32 ret = 0;
    if( pRouter == nullptr )
        return -EINVAL;

    do{
        Json::StreamWriterBuilder oBuilder;
        oBuilder["commentStyle"] = "None";
        oBuilder["indentation"] = "   ";
        Json::Value oVal( Json::objectValue);

        CStatCountersServer* psc =
            ObjPtr( pRouter );

        std::vector< InterfPtr > vecBdges;
        pRouter->GetBridges( vecBdges );

        oVal[ "NumConnections" ] =
            ( guint32 )vecBdges.size();

        Json::Value oArray( Json::arrayValue );
        guint32 dwVal = 0;
        guint32 dwReqs = 0, dwResps = 0, dwEvts = 0, dwFails = 0;
        guint64 qwRouterRx = 0, qwRouterTx = 0, qwVal = 0;
        for( auto& elem : vecBdges )
        {
            Json::Value oBridge;
            CRpcTcpBridge* pBridge = elem;
            CStatCountersServer* pStat = elem;
            CStreamServerRelay* pstm = elem;
            CStreamServerRelayMH* pstmmh = elem;

            CCfgOpenerObj oIfCfg(
                ( CObjBase* )elem );
            stdstr strVal;
            bool bVal;
            ret = oIfCfg.GetStrProp(
                propSessHash, strVal );
            if( SUCCEEDED( ret ) )
                oBridge[ "SessionHash" ] = strVal;
                
            IConfigDb* pConn;
            ret = oIfCfg.GetPointer(
                propConnParams, pConn );
            if( SUCCEEDED( ret ) )
            {
                oBridge[ "ConnParams" ] =
                    DumpConnParams( pConn );
            }

            ret = oIfCfg.GetIntProp(
                propPortId, dwVal );
            if( SUCCEEDED( ret ) )
                oBridge[ JSON_ATTR_PORTID ] = dwVal;

            stdstr strState = IfStateToString(
                elem->GetState() );

            oBridge[ "IfStat" ] = strState;
            bVal = pBridge->IsRfcEnabled();

            oBridge[ "RfcEnabled" ] = bVal;

            if( bVal )
            {
                ret = oIfCfg.GetIntProp(
                    propMaxReqs, dwVal );
                if( SUCCEEDED( ret ) )
                    oBridge[ "MaxReqs" ] = dwVal;

                ret = oIfCfg.GetIntProp(
                    propMaxPendings, dwVal );
                if( SUCCEEDED( ret ) )
                    oBridge[ "MaxPendings" ] = dwVal;
            }

            ret = pStat->GetCounter2(
                propMsgCount, dwVal );
            if( SUCCEEDED( ret ) )
            {
                oBridge[ "InRequests" ] = dwVal;
                dwReqs += dwVal;
            }

            ret = pStat->GetCounter2(
                propMsgRespCount, dwVal );
            if( SUCCEEDED( ret ) )
            {
                oBridge[ "OutResponses" ] = dwVal;
                dwResps += dwVal;
            }

            ret = pStat->GetCounter2(
                propEventCount, dwVal );
            if( SUCCEEDED( ret ) )
            {
                oBridge[ "Events" ] = dwVal;
                dwEvts += dwVal;
            }

            ret = pStat->GetCounter2(
                propFailureCount, dwVal );
            if( SUCCEEDED( ret ) )
            {
                oBridge[ "FailedRequests" ] = dwVal;
                dwFails += dwVal;
            }

            TaskGrpPtr pGrp;
            if( !pBridge->IsRfcEnabled() )
            {
                ret = pBridge->GetParallelGrp( pGrp );
            }
            else
            {
                DMsgPtr pMsg;
                ret = pBridge->GetGrpRfc( pMsg, pGrp );
                CIfParallelTaskGrpRfc* pGrpRfc = pGrp;
                dwVal = pGrpRfc->GetTaskAdded();
                oBridge[ "NumTaskAdded" ] = dwVal;
                dwVal = pGrpRfc->GetTaskRejected();
                oBridge[ "NumTaskRejected" ] = dwVal;
            }

            if( SUCCEEDED( ret ) )
            {
                CIfParallelTaskGrp* pParaGrp = pGrp;
                dwVal = pParaGrp->GetTaskCount();
                guint32 dwPending =
                    pParaGrp->GetPendingCountLocked();

                oBridge[ "ActiveTasks" ] =
                    dwVal - dwPending;

                oBridge[ "PendingTasks" ] =
                    dwPending;
            }

            IStream* ps = pstm;
            dwVal = ps->GetStreamCount();
            oBridge[ "NumStreams" ] = dwVal;
            guint64 qwRxTotal = 0, qwTxTotal = 0;
            if( dwVal > 0 )
            {
                Json::Value oStreams( Json::arrayValue );
                std::vector< InterfPtr > vecUxStms;
                ps->EnumStreams( vecUxStms );
                for( auto& elem : vecUxStms )
                {
                    Json::Value oVal( Json::objectValue );
                    DumpStream( elem, oVal );
                    CUnixSockStmProxyRelay* pux = elem; 
                    guint64 qwRxBytes, qwTxBytes;
                    pux->GetBytesTransfered(
                        qwRxBytes, qwTxBytes );
                    oStreams.append( oVal  );
                    qwRxTotal += qwRxBytes;
                    qwTxTotal += qwTxBytes;
                }
                oBridge[ "Streams" ] = oStreams;
                oBridge[ "BytesToClient" ] =
                    ( Json::UInt64 )qwTxTotal;
                oBridge[ "BytesToServer" ] =
                    ( Json::UInt64 )qwRxTotal;
            }

            guint64 qwRxTotalR = 0, qwTxTotalR = 0;
            ps = pstmmh;
            dwVal = ps->GetStreamCount();
            oBridge[ "NumStreamsRelay" ] = dwVal;
            if( dwVal > 0 )
            {
                Json::Value oStreams( Json::arrayValue );
                std::vector< InterfPtr > vecUxStms;
                pstmmh->EnumStreams( vecUxStms );
                for( auto& elem : vecUxStms )
                {
                    Json::Value oVal( Json::objectValue );
                    DumpStream( elem, oVal );
                    oStreams.append( oVal  );
                    CUnixSockStmProxyRelay* pux = elem; 
                    guint64 qwRxBytes, qwTxBytes;
                    pux->GetBytesTransfered(
                        qwRxBytes, qwTxBytes );
                    qwRxTotalR += qwRxBytes;
                    qwTxTotalR += qwTxBytes;
                }
                oBridge[ "StreamsRelay" ] = oStreams;
                oBridge[ "BytesRelayCli" ] =
                    ( Json::UInt64 )qwTxTotalR;
                oBridge[ "BytesRelayRx" ] =
                    ( Json::UInt64 )qwRxTotalR;
            }
            pStat->GetCounter2( propRxBytes, qwVal );
            oBridge[ "BytesThruToCli" ] = ( Json::UInt64 )
                ( qwRxTotalR + qwRxTotal + qwVal );

            qwRouterRx += 
                qwRxTotalR + qwRxTotal + qwVal;

            qwVal = 0;
            pStat->GetCounter2( propTxBytes, qwVal );
            oBridge[ "BytesThruToSvr" ] = ( Json::UInt64 )
                ( qwTxTotalR + qwTxTotal + qwVal );

            timespec tv;
            clock_gettime( CLOCK_REALTIME, &tv );
            guint32 dwts = tv.tv_sec -
                pBridge->GetStartSec();

            oBridge[ "UpTimeSec" ] = dwts;

            oArray.append( oBridge );

            qwRouterTx +=
                qwTxTotalR + qwTxTotal + qwVal;
        }

        qwVal = 0;
        psc->GetCounter2( propRxBytes, qwVal );
        oVal[ "TotalBytesToCli" ] =
            ( Json::UInt64 )( qwRouterRx + qwVal );

        qwVal = 0;
        psc->GetCounter2( propTxBytes, qwVal );
        oVal[ "TotalBytesToSvr" ] =
            ( Json::UInt64 )( qwRouterTx + qwVal );
        
        ret = psc->GetCounter2(
            propMsgCount, dwVal );
        if( SUCCEEDED( ret ) )
            oVal[ "TotalInRequests" ] =
                dwVal + dwReqs;

        ret = psc->GetCounter2(
            propEventCount, dwVal );
        if( SUCCEEDED( ret ) )
            oVal[ "TotalEvents" ] =
                dwVal + dwEvts;

        ret = psc->GetCounter2(
            propMsgRespCount, dwVal );
        if( SUCCEEDED( ret ) )
            oVal[ "TotalOutResponses" ] =
                dwVal + dwResps;

        ret = psc->GetCounter2(
            propFailureCount, dwVal );
        if( SUCCEEDED( ret ) )
            oVal[ "TotalFailures" ] =
                dwVal + dwFails;

        oVal[ "Connections" ] = oArray;
        strOutput = Json::writeString(
            oBuilder, oVal );
        strOutput.append( 1, '\n' );

    }while( 0 );
    return ret;
}

gint32 DumpBdgeProxyList(
    CRpcRouter* pRouter,
    stdstr& strOutput )
{
    gint32 ret = 0;
    do{
        Json::StreamWriterBuilder oBuilder;
        oBuilder["commentStyle"] = "None";
        oBuilder["indentation"] = "   ";
        Json::Value oVal( Json::objectValue );

        std::vector< InterfPtr > vecProxies;
        pRouter->GetBridgeProxies( vecProxies );
        CRpcRouterReqFwdr* pRfRouter = ObjPtr( pRouter );

        oVal[ "NumConnections" ] =
            ( guint32 )vecProxies.size();

        guint64 qwRouterRx = 0, qwRouterTx = 0, qwVal = 0;
        Json::Value oArray( Json::arrayValue );
        guint32 dwVal = 0;
        for( auto& elem : vecProxies )
        {
            Json::Value oProxy;
            CRpcTcpBridgeProxy* pProxy = elem;
            CStatCountersProxy* pStat = elem;
            CStreamProxyRelay* pstm = elem;

            CCfgOpenerObj oIfCfg(
                ( CObjBase* )elem );
            stdstr strVal;
            bool bVal;
            ret = oIfCfg.GetStrProp(
                propSessHash, strVal );
            if( SUCCEEDED( ret ) )
                oProxy[ "SessionHash" ] = strVal;
                
            IConfigDb* pConn;
            ret = oIfCfg.GetPointer(
                propConnParams, pConn );
            if( SUCCEEDED( ret ) )
            {
                oProxy[ "ConnParams" ] =
                    DumpConnParams( pConn );
            }

            ret = oIfCfg.GetIntProp(
                propPortId, dwVal );
            if( SUCCEEDED( ret ) )
                oProxy[ JSON_ATTR_PORTID ] = dwVal;

            ret = oIfCfg.GetStrProp(
                propNodeName, strVal );
            if( SUCCEEDED( ret ) )
                oProxy[ JSON_ATTR_NODENAME ] = strVal;

            stdstr strState = IfStateToString(
                elem->GetState() );

            oProxy[ "IfStat" ] = strState;
            bVal = pProxy->IsRfcEnabled();

            oProxy[ "RfcEnabled" ] = bVal;

            if( bVal )
            {
                ret = oIfCfg.GetIntProp(
                    propMaxReqs, dwVal );
                if( SUCCEEDED( ret ) )
                    oProxy[ "MaxReqs" ] = dwVal;

                ret = oIfCfg.GetIntProp(
                    propMaxPendings, dwVal );
                if( SUCCEEDED( ret ) )
                    oProxy[ "MaxPendings" ] = dwVal;
            }

            ret = pStat->GetCounter2(
                propMsgRespCount, dwVal );
            if( SUCCEEDED( ret ) )
                oProxy[ "RequestsSent" ] = dwVal;

            TaskGrpPtr pGrp;
            if( !pProxy->IsRfcEnabled() )
            {
                ret = pProxy->GetParallelGrp( pGrp );
            }
            else
            {
                ret = pProxy->GetGrpRfc( pGrp );
                CIfParallelTaskGrpRfc* pGrpRfc = pGrp;
                dwVal = pGrpRfc->GetTaskAdded();
                oProxy[ "NumTaskAdded" ] = dwVal;
                dwVal = pGrpRfc->GetTaskRejected();
                oProxy[ "NumTaskRejected" ] = dwVal;
            }

            if( SUCCEEDED( ret ) )
            {
                CIfParallelTaskGrp* pParaGrp = pGrp;
                dwVal = pParaGrp->GetTaskCount();
                guint32 dwPending =
                    pParaGrp->GetPendingCountLocked();

                oProxy[ "ActiveTasks" ] =
                    dwVal - dwPending;
                oProxy[ "PendingTasks" ] =
                    dwPending;
            }

            if( pRfRouter != nullptr )
            {
                IStream* ps = pstm;
                dwVal = ps->GetStreamCount();
                oProxy[ "NumStreams" ] = dwVal;
                guint64 qwRxTotal = 0, qwTxTotal = 0;
                if( dwVal > 0 )
                {
                    Json::Value oStreams( Json::arrayValue );
                    std::vector< InterfPtr > vecUxStms;
                    ps->EnumStreams( vecUxStms );
                    for( auto& elem : vecUxStms )
                    {
                        Json::Value oVal( Json::objectValue );
                        DumpStream( elem, oVal );
                        oStreams.append( oVal  );
                        qwRxTotal +=
                            oVal[ "BytesToServer" ].asUInt64();
                        qwTxTotal +=
                            oVal[ "BytesToClient" ].asUInt64();
                    }
                    oProxy[ "Streams" ] = oStreams;
                }

                qwVal = 0;
                pStat->GetCounter2( propRxBytes, qwVal );
                oProxy[ "BytesThruToSvr" ] =
                    ( Json::UInt64 )( qwRxTotal + qwVal );
                qwRouterRx = qwRxTotal + qwVal;

                qwVal = 0;
                pStat->GetCounter2( propTxBytes, qwVal );
                oProxy[ "BytesThruToCli" ] =
                    ( Json::UInt64 )( qwTxTotal + qwVal );
                qwRouterTx = qwTxTotal + qwVal;
            }

            timespec tv;
            clock_gettime( CLOCK_REALTIME, &tv );

            guint32 dwts =
                tv.tv_sec - pProxy->GetStartSec();

            oProxy[ "UpTimeSec" ] = dwts;

            oArray.append( oProxy );
        }

        if( pRfRouter != nullptr )
        {
            CStatCountersServer* psc = ObjPtr( pRouter );
            qwVal = 0;
            psc->GetCounter2( propRxBytes, qwVal );
            oVal[ "TotalBytesToSvr" ] = ( Json::UInt64 )
                ( qwRouterRx + qwVal );

            qwVal = 0;
            psc->GetCounter2( propTxBytes, qwVal );
            oVal[ "TotalBytesToCli" ] = ( Json::UInt64 )
                ( qwRouterTx + qwVal );
        }

        oVal[ "Connections" ] = oArray;
        strOutput = Json::writeString(
            oBuilder, oVal );
        strOutput.append( 1, '\n' );

    }while( 0 );

    return ret;
}

gint32 DumpSessions(
    CRpcRouterBridge* pRouter,
    stdstr& strOutput )
{
    gint32 ret = 0;
    if( pRouter == nullptr )
        return -EFAULT;
    do{
        Json::StreamWriterBuilder oBuilder;
        oBuilder["commentStyle"] = "None";
        oBuilder["indentation"] = "   ";
        Json::Value oVal( Json::objectValue);

        std::vector< InterfPtr > vecBdges;
        pRouter->GetBridges( vecBdges );

        oVal[ "NumConnections" ] =
            ( guint32 )vecBdges.size();

        IAuthenticateServer* pas;
        ret = pRouter->QueryInterface(
            iid( IAuthenticateServer ),
            ( void*& )pas );

        if( ERROR( ret ) )
        {
            pas = nullptr;
            ret = 0;
        }

        Json::Value oArray( Json::arrayValue );
        guint32 dwVal = 0;
        for( auto& elem : vecBdges )
        {
            Json::Value oBridge;
            CRpcTcpBridge* pBridge = elem;

            CCfgOpenerObj oIfCfg(
                ( CObjBase* )elem );
            stdstr strVal;
            bool bVal;
            ret = oIfCfg.GetStrProp(
                propSessHash, strVal );
            if( SUCCEEDED( ret ) )
                oBridge[ "SessionHash" ] = strVal;
                
            IConfigDb* pConn;
            ret = oIfCfg.GetPointer(
                propConnParams, pConn );
            if( SUCCEEDED( ret ) )
            {
                oBridge[ "ConnParams" ] =
                    DumpConnParams( pConn );
            }
            if( pas == nullptr )
            {
                oArray.append( oBridge );
                continue;
            }

            if( strVal.substr( 0, 2 ) != "AU" )
            {
                oArray.append( oBridge );
                continue;
            }
            
            CfgPtr pCfg( true );
            ret = pas->InquireSess( strVal, pCfg );
            if( ERROR( ret ) )
                continue;

            Json::Value oVal( Json::objectValue );
            CCfgOpener oCfg( ( IConfigDb* )pCfg );
            ret = oCfg.GetStrProp(
                propUserName, strVal );
            if( SUCCEEDED( ret ) )
                oVal[ "UserName" ] = strVal;

            ret = oCfg.GetStrProp(
                propServiceName, strVal );
            if( SUCCEEDED( ret ) )
                oVal[ "ServiceName" ] = strVal;

            ret = oCfg.GetStrProp(
                propEmail, strVal );
            if( SUCCEEDED( ret ) )
                oVal[ "Email" ] = strVal;

            ret = oCfg.GetIntProp(
                propTimeoutSec, dwVal );
            if( SUCCEEDED( ret ) )
                oVal[ "ExpireInSec" ] = dwVal;

            ret = oCfg.GetStrProp(
                propAuthMech, strVal );
            if( SUCCEEDED( ret ) )
                oVal[ "AuthMech" ] = strVal;

            oBridge[ "AuthInfo" ] = oVal;
            oArray.append( oBridge );
            ret = 0;
        }
        oVal[ "Sessions" ] = oArray;
        strOutput = Json::writeString(
            oBuilder, oVal );
        strOutput.append( 1, '\n' );

    }while( 0 );

    return ret;
}

gint32 DumpReqProxyList(
    CRpcRouterBridge* pRouter,
    stdstr& strOutput )
{
    gint32 ret = 0;
    if( pRouter == nullptr )
        return -EINVAL;

    do{
        Json::StreamWriterBuilder oBuilder;
        oBuilder["commentStyle"] = "None";
        oBuilder["indentation"] = "   ";
        Json::Value oVal( Json::objectValue);

        ARR_REQPROXIES vecProxies;
        pRouter->GetReqFwdrProxies( vecProxies );

        oVal[ "NumReqProxies" ] =
            ( guint32 )vecProxies.size();

        guint32 dwVal = 0;
        Json::Value oArray( Json::arrayValue );
        for( auto& elem : vecProxies )
        {
            Json::Value oProxy;

            CRpcReqForwarderProxy* pProxy =
                elem.second;

            CStatCountersProxy* pStat =
                elem.second;

            CCfgOpenerObj oIfCfg(
                ( CObjBase* )pProxy );
            stdstr strVal;
            bool bVal;
            strVal = elem.first;
            if( SUCCEEDED( ret ) )
                oProxy[ "Destination" ] = strVal;
                
            stdstr strState = IfStateToString(
                pProxy->GetState() );

            oProxy[ "IfStat" ] = strState;

            ret = pStat->GetCounter2(
                propMsgRespCount, dwVal );
            if( SUCCEEDED( ret ) )
                oProxy[ "RequestsSent" ] = dwVal;

            TaskGrpPtr pGrp;
            ret = pProxy->GetParallelGrp( pGrp );
            if( SUCCEEDED( ret ) )
            {
                CIfParallelTaskGrp* pParaGrp = pGrp;
                dwVal = pParaGrp->GetTaskCount();
                guint32 dwPending =
                    pParaGrp->GetPendingCountLocked();

                oProxy[ "ActiveTasks" ] =
                    dwVal - dwPending;
                oProxy[ "PendingTasks" ] =
                    dwPending;
            }

            Json::Value oMatches( Json::arrayValue );
            std::vector< MatchPtr > vecMatches;
            pProxy->GetActiveInterfaces( vecMatches );
            for( auto& elem : vecMatches )
            {
                stdstr strMatch = elem->ToString();
                oMatches.append( strMatch );
            }

            oProxy[ "Matches" ] = oMatches;

            timespec tv;
            clock_gettime( CLOCK_REALTIME, &tv );

            guint32 dwts =
                tv.tv_sec - pProxy->GetStartSec();

            oProxy[ "UpTimeSec" ] = dwts;
            oArray.append( oProxy );
        }

        oVal[ "ReqProxies" ] = oArray ;
        strOutput = Json::writeString(
            oBuilder, oVal );
        strOutput.append( 1, '\n' );

    }while( 0 );

    return ret;
}

gint32 DumpReqFwdrInfo(
    CRpcRouter* pRouter,
    stdstr& strOutput )
{
    gint32 ret = 0;
    if( pRouter == nullptr )
        return -EINVAL;
    do{
        Json::StreamWriterBuilder oBuilder;
        oBuilder["commentStyle"] = "None";
        oBuilder["indentation"] = "   ";
        Json::Value oVal( Json::objectValue );

        CRpcRouterReqFwdr* pRfRouter =
            ObjPtr( pRouter );

        if( pRfRouter == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        InterfPtr pIf;
        pRfRouter->GetReqFwdr( pIf );
        CRpcReqForwarder* pReqFwdr = pIf;
        CStatCountersServer* psc = pIf;

        std::vector< REFCOUNT_ELEM > vecRegObjs;
        pRfRouter->GetRefCountObjs( vecRegObjs );

        oVal[ "NumRefCountObjs" ] =
            ( guint32 )vecRegObjs.size();

        guint32 dwVal = 0;
        Json::Value oArray( Json::arrayValue );
        for( auto& elem : vecRegObjs )
        {
            Json::Value oRegObj( Json::objectValue );
            CRegisteredObject* pRegObj = elem.first;

            oRegObj[ "ProxyPortId" ] =
                pRegObj->GetPortId();

            oRegObj[ "SrcUniqName" ] =
                pRegObj->GetUniqName();
                
            oRegObj[ "SrcDBusName" ] =
                pRegObj->GetSrcDBusName();

            oRegObj[ "ReferenceCount" ] =
                elem.second;

            oArray.append( oRegObj );
        }

        oVal[ "RegObjects" ] = oArray;
        if( !pRfRouter->IsRfcEnabled() )
        {
            strOutput = Json::writeString(
                oBuilder, oVal );
            strOutput.append( 1, '\n' );
            break;
        }

        ITaskScheduler* pSched =
            pReqFwdr->GetScheduler();
        std::vector< InterfPtr > vecIfs;
        pSched->EnumIfs( vecIfs );
        stdstr strVal;
        Json::Value oMap( Json::objectValue );
        guint32 dwTaskAdded = 0, dwTaskRejected = 0;
        for( auto& elem : vecIfs )
        {
            guint32 dwPortId = 0;
            CCfgOpenerObj oIfCfg( ( CObjBase* )elem );
            ret = oIfCfg.GetIntProp(
                propPortId, dwPortId );
            if( ERROR( ret ) )
                continue;
            std::vector< TaskGrpPtr > vecGrps;
            pSched->EnumTaskGrps( elem, vecGrps );
            Json::Value oArrGrps( Json::arrayValue );
            for( auto& pGrp : vecGrps )
            {
                CIfParallelTaskGrpRfc* pGrpRfc = pGrp;
                CStdRTMutex oTaskLock( pGrpRfc->GetLock() );

                CCfgOpener oGrpCfg(
                    ( IConfigDb* )pGrp->GetConfig() );
                Json::Value oElem;
                ret = oGrpCfg.GetStrProp(
                    propSrcUniqName, strVal );
                if( SUCCEEDED( ret ) )
                    oElem[ "SrcUniqName" ] = strVal;

                ret = oGrpCfg.GetStrProp(
                    propSrcDBusName, strVal );

                if( SUCCEEDED( ret ) )
                    oElem[ "SrcDBusName" ] = strVal;

                oElem[ "MaxPendings" ] = ( guint32 )
                    pGrpRfc->GetMaxPending();

                oElem[ "MaxRunning" ] = ( guint32 )
                    pGrpRfc->GetMaxRunning();

                dwVal = pGrpRfc->GetTaskAdded();
                oElem[ "TaskAdded" ] = dwVal;
                dwTaskAdded += dwVal;

                dwVal = pGrpRfc->GetTaskRejected();
                oElem[ "TaskRejected" ] = dwVal;
                dwTaskRejected += dwVal;

                oElem[ "RunningTask" ] = ( guint32 )
                    pGrpRfc->GetRunningCount();

                oElem[ "PendingTask" ] = ( guint32 )
                    pGrpRfc->GetPendingCountLocked();

                oTaskLock.Unlock();
                oArrGrps.append( oElem );
            }
            strVal = "Port-";
            strVal += std::to_string( dwPortId );
            oMap[ strVal ] = oArrGrps;
        }

        oVal[ "RFCGroups" ] = oMap;

        ret = psc->GetCounter2(
            propTaskAdded, dwVal );
        if( SUCCEEDED( ret ) )
            oVal[ "TotalTaskAdded" ] =
                dwVal + dwTaskAdded;

        ret = psc->GetCounter2(
            propTaskRejected, dwVal );
        if( SUCCEEDED( ret ) )
            oVal[ "TotalTaskRejected" ] =
                dwVal + dwTaskRejected;

        strOutput = Json::writeString(
            oBuilder, oVal );
        strOutput.append( 1, '\n' );

    }while( 0 );

    return ret;
}

}
