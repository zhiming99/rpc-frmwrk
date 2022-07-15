/*
 * =====================================================================================
 *
 *       Filename:  rtfiles.cpp
 *
 *    Description:  implementations of the file objects exported by rpcrouter
 *
 *        Version:  1.0
 *        Created:  07/02/2022 05:51:33 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2022 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */

#include "routmain.h"
#include <ifhelper.h>
#include <frmwrk.h>
#include <rpcroute.h>
#include "fuseif.h"
#include "jsondef.h"

using namespace rpcf;
#include "rtfiles.h"

Json::Value DumpConnParams( const IConfigDb* pConn )
{
    bool bVal;

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
        if( pSvc->IsServer() )
        {
            CUnixSockStmServer* pSvr = pUxStream; 
            guint64 qwRxBytes, qwTxBytes;
            pSvr->GetBytesTransfered(
                qwRxBytes, qwTxBytes );
            oStmInfo[ "BytesReceived" ] =
                ( Json::UInt64 )qwRxBytes;
            oStmInfo[ "BytesSent" ] =
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
            oStmInfo[ "BytesReceived" ] =
                ( Json::UInt64 )qwRxBytes;
            oStmInfo[ "BytesSent" ] =
                ( Json::UInt64 )qwTxBytes;
            oStmInfo[ "CanSend" ] =
                pProxy->CanSend();
        }
        CCfgOpenerObj oIfCfg( pSvc );
        IConfigDb* pDesc;
        ret = oIfCfg.GetPointer(
            propDataDesc, pDesc );
        if( ERROR( ret ) )
            break;

        stdstr strName;
        CCfgOpener oDesc( pDesc );
        ret = oDesc.GetStrProp(
            propNodeName, strName );
        if( ERROR( ret ) )
            break;
        oStmInfo[ "ClientName" ] = strName;

    }while( 0 );

    return ret;
}

gint32 CFuseBdgeList::UpdateContent() 
{
    gint32 ret = 0;
    do{
        Json::StreamWriterBuilder oBuilder;
        oBuilder["commentStyle"] = "None";
        oBuilder["indentation"] = "   ";
        Json::Value oVal( Json::objectValue);

        CRpcRouterBridge* pRouter =
            GetUserObj();
        if( pRouter == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        CStatCountersServer* psc =
            GetUserObj();

        std::vector< InterfPtr > vecBdges;
        pRouter->GetBridges( vecBdges );

        oVal[ "NumConnections" ] =
            ( guint32 )vecBdges.size();

        Json::Value oArray( Json::arrayValue );
        guint32 dwVal = 0;
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
            if( pStat )
            {
                ret = pStat->GetCounter2(
                    propMsgCount, dwVal );
                if( SUCCEEDED( ret ) )
                    oBridge[ "InRequests" ] = dwVal;

                ret = pStat->GetCounter2(
                    propMsgRespCount, dwVal );
                if( SUCCEEDED( ret ) )
                    oBridge[ "OutResponses" ] = dwVal;
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
                    oStreams.append( oVal  );
                    qwRxTotal += oVal[ "BytesReceived" ].asUInt64();
                    qwTxTotal += oVal[ "BytesSent" ].asUInt64();
                }
                oBridge[ "Streams" ] = oStreams;
                oBridge[ "StreamsTotalRx" ] =
                    ( Json::UInt64 )qwRxTotal;
                oBridge[ "StreamsTotalTx" ] =
                    ( Json::UInt64 )qwTxTotal;
            }

            qwRxTotal = 0, qwTxTotal = 0;
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
                    qwRxTotal += oVal[ "BytesReceived" ].asUInt64();
                    qwTxTotal += oVal[ "BytesSent" ].asUInt64();
                }
                oBridge[ "StreamsRelay" ] = oStreams;
                oBridge[ "StreamsRelayTotalRx" ] = qwRxTotal;
                oBridge[ "StreamsRelayTotalTx" ] = qwTxTotal;
            }

            timespec tv;
            clock_gettime( CLOCK_REALTIME, &tv );
            guint32 dwts = tv.tv_sec -
                pBridge->GetStartSec();

            oBridge[ "UpTimeSec" ] =
                ( Json::UInt )dwts;

            oArray.append( oBridge );
        }

        ret = psc->GetCounter2(
            propMsgCount, dwVal );
        if( SUCCEEDED( ret ) )
            oVal[ "TotalInRequests" ] = dwVal;

        ret = psc->GetCounter2(
            propEventCount, dwVal );
        if( SUCCEEDED( ret ) )
            oVal[ "TotalEvents" ] = dwVal;

        ret = psc->GetCounter2(
            propMsgRespCount, dwVal );
        if( SUCCEEDED( ret ) )
            oVal[ "TotalOutResponses" ] = dwVal;

        oVal[ "Connections" ] = oArray;
        m_strContent = Json::writeString(
            oBuilder, oVal );

    }while( 0 );

    return ret;
}

gint32 AddFilesAndDirsBdge(
    CRpcServices* pSvc )
{
    gint32 ret = 0;
    do{
        CRpcRouterManagerImpl* prmgr =
            ObjPtr( pSvc );
        if( prmgr == nullptr )
        {
            ret = -EFAULT;
            break;
        }
 
        std::vector< InterfPtr > vecRts;
        prmgr->GetRouters( vecRts );
        if( vecRts.empty() )
            break;

        CRpcRouterBridge* pRouter = nullptr;
        for( auto& elem : vecRts )
        {
            pRouter = elem;
            if( pRouter == nullptr )
                continue;
            break;
        }

        InterfPtr pRootIf = GetRootIf();
        CFuseRootServer* pSvr = pRootIf;
        if( pSvr == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        auto pList = new CFuseBdgeList( nullptr );
        pList->SetMode( S_IRUSR );
        pList->DecRef();
        ObjPtr pObj( pRouter );
        pList->SetUserObj( pObj );

        auto pEnt = DIR_SPTR( pList );
        pSvr->Add2UserDir( pEnt );

    }while( 0 );

    return ret;
}

gint32 AddFilesAndDirsReqFwdr(
    CRpcServices* pSvc )
{ return 0; }

