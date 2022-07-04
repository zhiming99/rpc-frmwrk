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

        std::vector< InterfPtr > vecBdges;
        pRouter->GetBridges( vecBdges );

        oVal[ "NumConnections" ] =
            ( guint32 )vecBdges.size();

        Json::Value oArray( Json::arrayValue );
        for( auto& elem : vecBdges )
        {
            Json::Value oBridge;
            CRpcTcpBridge* pBridge = elem;
            CStatCountersServer* pStat = elem;
            CCfgOpenerObj oIfCfg(
                ( CObjBase* )elem );
            stdstr strVal;
            guint32 dwVal;
            bool bVal;
            ret = oIfCfg.GetStrProp(
                propSessHash, strVal );
            if( SUCCEEDED( ret ) )
                oBridge[ "SessionHash" ] = strVal;
                
            ObjPtr pObj;
            ret = oIfCfg.GetObjPtr(
                propConnParams, pObj );
            if( SUCCEEDED( ret ) )
            {
                Json::Value oConnVal( Json::objectValue );
                CConnParams oConn(
                    ( IConfigDb* )pObj );

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
                oBridge[ "ConnParams" ] = oConnVal;
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

            timespec tv;
            clock_gettime( CLOCK_REALTIME, &tv );
            guint32 dwts = tv.tv_sec -
                pBridge->GetStartSec();

            oBridge[ "UpTimeSec" ] =
                ( Json::UInt )dwts;

            oArray.append( oBridge );
        }

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

