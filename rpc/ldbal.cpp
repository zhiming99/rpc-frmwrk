/*
 * =====================================================================================
 *
 *       Filename:  ldbal.cpp
 *
 *    Description:  load balance related methods 
 *
 *        Version:  1.0
 *        Created:  01/29/2021 02:11:20 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi(woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2021 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */

#include "configdb.h"
#include "defines.h"
#include "proxy.h"
#include "rpcroute.h"
#include "dbusport.h"
#include "connhelp.h"

namespace rpcfrmwrk
{

gint32 CRpcTcpBridge::OnCheckRouterPathCompleteLB(
    IEventSink* pCallback, 
    IEventSink* pIoReq,
    IConfigDb* pReqCtx )
{
    if( pCallback == nullptr ||
        pIoReq == nullptr ||
        pReqCtx == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    std::string strPath, strNode;
    bool bLast = false;
    do{
        CCfgOpener oReqCtx( pReqCtx );
        ret = oReqCtx.GetBoolProp(
            0, bLast );
        if( ERROR( ret ) )
            break;

        ret = oReqCtx.GetStrProp(
            propRouterPath, strPath );
        if( ERROR( ret ) )
            break;

        ret = oReqCtx.GetStrProp(
            1, strNode );
        if( ERROR( ret ) )
            break;

        CCfgOpenerObj oReq( pIoReq );
        IConfigDb* pResp = nullptr;
        ret = oReq.GetPointer(
            propRespPtr, pResp );
        if( ERROR( ret ) )
            break;

        CCfgOpener oResp( pResp );
        gint32 iRet = 0;
        ret = oResp.GetIntProp(
            propReturnValue,
            ( guint32& ) iRet );

        if( ERROR( ret ) )
            break;

        if( ERROR( iRet ) )
        {
            ret = iRet;
            break;
        }

        CCfgOpenerObj oIfCfg( this );
        guint32 dwPortId = 0;
        ret = oIfCfg.GetIntProp(
            propPortId, dwPortId );
        if( ERROR( ret ) )
            break;

        CRpcRouterBridge* pRouter =
        static_cast< CRpcRouterBridge* >
            ( GetParent() );

        guint32 dwProxyId;
        ret = pRouter->GetProxyIdByNodeName(
            strNode, dwProxyId );
        if( ERROR( ret ) )
            break;

        ret = pRouter->AddRefCount(
            strNode, dwPortId, dwProxyId );

    }while( 0 );

    if( !bLast && ERROR( ret ) )
        return ret;

    CParamList oResp;
    oResp[ propReturnValue ] = ret;

    if( SUCCEEDED( ret ) )
        oResp[ propRouterPath ] = strPath;

    OnServiceComplete(
        oResp.GetCfg(), pCallback );

    if( pReqCtx )
    {
        CParamList oContext( pReqCtx );
        oContext.ClearParams();
    }

    return ret;
}

gint32 CRpcTcpBridge::OnCheckRouterPathCompleteLB2(
    IEventSink* pCallback, 
    IEventSink* pIoReq,
    IConfigDb* pReqCtx )
{
    if( pCallback == nullptr ||
        pIoReq == nullptr ||
        pReqCtx == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CCfgOpener oReqCtx( pReqCtx );
        IEventSink* pInvTask = nullptr;
        ret = oReqCtx.GetPointer(
            propTaskPtr, pInvTask );
        if( ERROR( ret ) )
            break;

        ret = OnCheckRouterPathCompleteLB(
            pInvTask, pIoReq, pReqCtx );

        oReqCtx.RemoveProperty(
            propTaskPtr );

    }while( 0 );

    pCallback->OnEvent(
        eventTaskComp, ret, 0, 0 );

    return ret;
}

gint32 CRpcTcpBridge::CheckRouterPathAgainLB(
    IEventSink* pCallback,
    IEventSink* pInvTask,
    IConfigDb* pReqCtx )
{
    if( pReqCtx == nullptr ||
        pInvTask == nullptr ||
        pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;

    CRpcRouterBridge* pRouter =
        static_cast< CRpcRouterBridge* >
            ( GetParent() );

    CParamList oReqCtx( pReqCtx );
    std::string strPath, strNode, strNext;
    bool bLast = false;
    do{
        ret = oReqCtx.GetBoolProp( 0, bLast );
        if( ERROR( ret ) )
            break;

        ret = oReqCtx.GetStrProp( 1, strNode );
        if( ERROR( ret ) )
            break;

        ret = oReqCtx.GetStrProp( 2, strNext );
        if( ERROR( ret ) )
            break;

        ret = oReqCtx.GetStrProp(
            propRouterPath, strPath );
        if( ERROR( ret ) )
            break;

        guint32 dwPortId = 0;
        ret = oReqCtx.GetIntProp(
            propConnHandle, dwPortId );
        if( ERROR( ret ) )
            break;

        guint32 dwProxyId = 0;
        ret = pRouter->GetProxyIdByNodeName(
            strNode, dwProxyId );
        if( ERROR( ret ) )
        {
            //proxy creation failed
            break;
        }

        if( strNext == "/" )
        {
            ret = pRouter->FindRefCount(
                strNode, dwPortId );

            if( SUCCEEDED( ret ) )
                break;

            ret = pRouter->AddRefCount(
                strNode, dwPortId, dwProxyId );

            if( ERROR( ret ) )
                break;

            break;
        }

        InterfPtr pIf;
        ret = pRouter->GetBridgeProxy(
            dwProxyId, pIf );
        if( ERROR( ret ) )
            break;

        CRpcTcpBridgeProxy* pProxy = pIf;
        if( unlikely( pProxy == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }

        oReqCtx[ propRouterPath ] = strPath;

        CCfgOpener oReqCtx2;
        oReqCtx2.SetStrProp(
            propRouterPath, strNext );

        QwVecPtr pvecIdsCpy;
        CStlQwordVector* pvecIds; 
        ret = oReqCtx.GetPointer(
            propObjList, pvecIds );
        if( ERROR( ret ) )
            break;

        ret = pvecIdsCpy.NewObj();
        if( ERROR( ret ) )
            break;

        ( *pvecIdsCpy )() = ( *pvecIds )();
        oReqCtx2.SetObjPtr( propObjList,
            ObjPtr( pvecIdsCpy ) );

        AddCheckStamp( oReqCtx2.GetCfg() );
        oReqCtx.SetPointer(
            propTaskPtr, pInvTask );

        TaskletPtr pChkRtCb;
        ret = NEW_PROXY_RESP_HANDLER2( 
            pChkRtCb, ObjPtr( this ),
            &CRpcTcpBridge::OnCheckRouterPathCompleteLB2,
            pCallback, pReqCtx );
        if( ERROR( ret ) )
            break;

        ret = pProxy->CheckRouterPath(
            oReqCtx2.GetCfg(), pChkRtCb );

        if( ERROR( ret ) )
        {
            (*pChkRtCb)( eventCancelTask );
        }

    }while( 0 );

    if( ret == STATUS_PENDING )
        return ret;

    oReqCtx.RemoveProperty( propTaskPtr );
    // move on to next node
    if( ERROR( ret ) && !bLast )
        return ret;

    CParamList oResp;
    oResp[ propReturnValue ] = ret;

    if( SUCCEEDED( ret ) )
        oResp[ propRouterPath ] = strPath;

    OnServiceComplete(
        oResp.GetCfg(), pInvTask );

    return ret;
}

gint32 CRpcTcpBridge::OnOpenPortCompleteLB(
    IEventSink* pCallback,
    IEventSink* pIoReq,
    IConfigDb* pReqCtx )
{
    if( pCallback == nullptr ||
        pIoReq == nullptr ||
        pReqCtx == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    bool bLast = false;
    IEventSink* pInvTask = nullptr;
    CCfgOpener oReqCtx( pReqCtx );
    do{
        ret = oReqCtx.GetPointer( 0, pInvTask );
        if( ERROR( ret ) )
            break;

        ret = oReqCtx.GetBoolProp( 1, bLast );
        if( ERROR( ret ) )
            break;

        IConfigDb* pResp = nullptr;
        CCfgOpenerObj oReq( pIoReq );
        ret = oReq.GetPointer(
            propRespPtr, pResp );
        if( ERROR( ret ) )
            break;

        CCfgOpener oResp( pResp );
        guint32 iRet = 0;
        ret = oResp.GetIntProp(
            propReturnValue, iRet );

        if( ERROR( ret ) )
            break;

        if( ERROR( iRet ) )
        {
            ret = iRet;
            break;
        }

    }while( 0 );

    CParamList oResp;
    oResp[ propReturnValue ] = ret;

    if( ERROR( ret ) && bLast )
    {
        // stop CheckRouterPath with error
        OnServiceComplete(
            oResp.GetCfg(), pInvTask );
    }

    pCallback->OnEvent(
        eventTaskComp, ret, 0, 0 );

    oReqCtx.RemoveProperty( 0 );

    return ret;
}

gint32 CRpcTcpBridge::OpenPortLB(
    IEventSink* pCallback,
    IEventSink* pInvTask,
    IEventSink* pOpTask,
    bool bLast )
{
    if( pCallback == nullptr ||
        pInvTask == nullptr ||
        pOpTask == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    CRpcRouterBridge* pRouter =
        static_cast< CRpcRouterBridge* >
            ( GetParent() );
    do{
        CParamList oReqCtx;
        oReqCtx.Push( ObjPtr( pInvTask ) );
        oReqCtx.Push( bLast );

        TaskletPtr pRespTask;
        ret = NEW_PROXY_RESP_HANDLER(
            pRespTask, ObjPtr( this ),
            &CRpcTcpBridge::OnOpenPortCompleteLB,
            pCallback, oReqCtx.GetCfg() );

        if( ERROR( ret ) )
            break;

        CIfRetryTask* pTask = static_cast
            < CIfRetryTask* >( pOpTask );
        pTask->SetClientNotify( pRespTask );
        // The method's main task is just to put
        // the pOpTask to the sequential task
        // queue of the pRouter.
        TaskletPtr pTemp = ObjPtr( pOpTask );
        ret = pRouter->AddSeqTask( pTemp );

        if( ERROR( ret ) )
        {
            ( *pRespTask )( eventCancelTask );
            break;
        }

        ret = STATUS_PENDING;

    }while( 0 );

    if( ret == STATUS_PENDING )
        return ret;

    if( ERROR( ret ) )
    {
        TaskletPtr pTask = ObjPtr( pOpTask );
        if( !pTask.IsEmpty() )
            ( *pTask )( eventCancelTask );
    }

    // move on to next node in the topmost taskgrp
    if( ERROR( ret ) && !bLast )
        return ret;

    if( ERROR( ret ) )
    {
        CParamList oParams;
        oParams[ propReturnValue ] = ret;
        OnServiceComplete(
            oParams.GetCfg(), pInvTask );
    }

    return ret;
}

gint32 CRpcTcpBridge::CheckRouterPathLB(
    IEventSink* pCallback,
    IConfigDb* pReqCtx,
    const std::string& strOldPath,
    const std::string& strNext,
    const std::string& strLBNode,
    const std::vector< std::string >& vecNodes )
{
    if( pReqCtx == nullptr ||
        pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    CRpcRouterBridge* pRouter =
        static_cast< CRpcRouterBridge* >
            ( GetParent() );

    std::string strPath = strOldPath;
    do{
        InterfPtr pIf;

        TaskGrpPtr pGrp;
        CParamList oTaskParam;
        oTaskParam[ propIfPtr ] = ObjPtr( this );
        ret = pGrp.NewObj(
            clsid( CIfTaskGroup ),
            oTaskParam.GetCfg() );
        if( ERROR( ret ) )
            break;

        pGrp->SetRelation( logicOR );
        guint32 dwPortId = 0;
        CCfgOpener oCtx( pReqCtx );
        ret = oCtx.GetIntProp(
            propConnHandle, dwPortId );
        if( ERROR( ret ) )
            break;

        // probe the candidate nodes till find the
        // first one that is working
        bool bLast = false;
        size_t i = 0;
        for( ;i < vecNodes.size(); ++i )
        {
            if( i + 1 == vecNodes.size() )
                bLast = true;

            CParamList oReqCtx;
            oReqCtx.CopyProp(
                propConnHandle, pReqCtx );

            oReqCtx.Push( bLast );
            const std::string& strNode =
                vecNodes[ i ];

            strPath = std::string( "/" ) +
                strNode + strNext;

            oReqCtx[ propRouterPath ] = strPath;
            oReqCtx.Push( strNode );
            oReqCtx.Push( strNext );
            oReqCtx.Push( i );

            guint32 dwProxyId = 0;
            ret = pRouter->GetProxyIdByNodeName(
                strNode, dwProxyId );

            TaskletPtr pCreateIf;
            if( ERROR( ret ) )
            {
                // the bridge proxy is yet to create.
                CParamList oParams;

                CfgPtr pConnParams;
                ret = pRouter->GetConnParamsByNodeName(
                    strNode, pConnParams );
                if( ERROR( ret ) )
                {
                    ret = -ENOTDIR;
                    break;
                }

                bool bServer = false;
                oParams.Push( bServer );

                oParams.SetPointer(
                    propIoMgr, GetIoMgr() );

                oParams.SetPointer(
                    propIfPtr, this );

                oParams.SetPointer( propConnParams,
                    ( IConfigDb* )pConnParams );

                oParams.SetObjPtr( propRouterPtr,
                    ObjPtr( pRouter ) );

                oParams.SetStrProp(
                    propNodeName, strNode );

                oParams.CopyProp(
                    propPortId, this );

                TaskletPtr pTask;
                ret = pTask.NewObj(
                    clsid( CRouterOpenBdgePortTask ),
                    oParams.GetCfg() );

                if( ERROR( ret ) )
                    break;

                TaskletPtr pOpenPort;
                ret = DEFER_IFCALLEX_NOSCHED2(
                    0, pOpenPort, ObjPtr( this ), 
                    &CRpcTcpBridge::OpenPortLB,
                    nullptr, pCallback,
                    ( IEventSink* )pTask,
                    bLast );
                if( ERROR( ret ) )
                {
                    ( *pTask )( eventCancelTask );
                    break;
                }

                TaskletPtr pChkRt;
                ret = DEFER_IFCALLEX_NOSCHED2(
                    0, pChkRt, ObjPtr( this ),
                    &CRpcTcpBridge::CheckRouterPathAgainLB,
                    nullptr, pCallback,
                    oReqCtx.GetCfg() );

                if( ERROR( ret ) )
                {
                    ( *pTask )( eventCancelTask );
                    ( *pOpenPort )( eventCancelTask );
                    break;
                }

                TaskGrpPtr pCreateGrp;
                CParamList oTaskParam;
                oTaskParam[ propIfPtr ] = ObjPtr( this );
                ret = pCreateGrp.NewObj(
                    clsid( CIfTaskGroup ),
                    oTaskParam.GetCfg() );

                if( ERROR( ret ) )
                {
                    ( *pTask )( eventCancelTask );
                    ( *pOpenPort )( eventCancelTask );
                    ( *pChkRt )( eventCancelTask );
                    break;
                }

                pCreateGrp->SetRelation( logicAND );
                pCreateGrp->AppendTask( pOpenPort );
                pCreateGrp->AppendTask( pChkRt );
                TaskletPtr pTemp =
                    TaskletPtr( pCreateGrp );
                ret = pGrp->AppendTask( pTemp );
                if( ERROR( ret ) )
                {
                    ( *pCreateGrp )( eventCancelTask );
                    break;
                }
            }
            else
            {
                if( strNext == "/" )
                {
                    ret = pRouter->FindRefCount(
                        strNode, dwPortId );

                    if( SUCCEEDED( ret ) )
                        break;

                    ret = pRouter->AddRefCount(
                        strNode, dwPortId, dwProxyId );

                    if( ERROR( ret ) )
                        break;

                    ret = 0;
                    break;
                }

                InterfPtr pIf;    
                ret = pRouter->GetBridgeProxy(
                    dwProxyId, pIf );
                if( ERROR( ret ) )
                    break;

                CRpcTcpBridgeProxy* pProxy = pIf;
                if( unlikely( pProxy == nullptr ) )
                {
                    ret = -EFAULT;
                    break;
                }

                // oReqCtx2 as the reqctx to next node
                CCfgOpener oReqCtx2;
                oReqCtx2[ propRouterPath ] = strNext;

                QwVecPtr pvecIdsCpy;
                CStlQwordVector* pvecIds; 

                ret = oReqCtx2.CopyProp(
                    propObjList, pReqCtx );
                if( ERROR( ret ) )
                    break;

                ret = oReqCtx2.GetPointer(
                    propObjList, pvecIds );
                if( ERROR( ret ) )
                    break;


                ret = pvecIdsCpy.NewObj();
                if( ERROR( ret ) )
                    break;

                ( *pvecIdsCpy )() = ( *pvecIds )();
                oReqCtx2.SetObjPtr( propObjList,
                    ObjPtr( pvecIdsCpy ) );

                AddCheckStamp( oReqCtx2.GetCfg() );

                TaskletPtr pMajorCall;
                ret = DEFER_CALL_NOSCHED(
                    pMajorCall, ObjPtr( pProxy ),
                    &CRpcTcpBridgeProxy::CheckRouterPath,
                    oReqCtx2.GetCfg(),
                    ( IEventSink* )nullptr );
                if( ERROR( ret ) )
                    break;

                TaskletPtr pIoTask;
                ret = NEW_PROXY_IOTASK(
                    1, pIoTask, pMajorCall, ObjPtr( this ),
                    &CRpcTcpBridge::OnCheckRouterPathCompleteLB,
                    pCallback, oReqCtx.GetCfg() );

                if( ERROR( ret ) )
                    break;

                ret = pGrp->AppendTask( pIoTask );
                if( ERROR( ret ) )
                {
                    ( *pIoTask )( eventCancelTask );
                    break;
                }
            }
        }
        if( ERROR( ret ) )
        {
            ( *pGrp )( eventCancelTask );
            break;
        }

        if( pGrp->GetTaskCount() > 0 )
        {
            TaskletPtr pTask = ObjPtr( pGrp );
            CIoManager* pMgr = GetIoMgr();
            ret = pMgr->RescheduleTask( pTask );
        }

        if( ERROR( ret ) )
        {
            ( *pGrp )( eventCancelTask );
            break;
        }
        ret = STATUS_PENDING;

    }while( 0 );

    if( ret != STATUS_PENDING )
    {
        CParamList oResp;
        oResp[ propReturnValue ] = ret;
        if( SUCCEEDED( ret ) )
            oResp[ propRouterPath ] = strPath;
        SetResponse(
            pCallback, oResp.GetCfg() );
    }

    return ret;
}

}
