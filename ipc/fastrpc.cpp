/*
 * =====================================================================================
 *
 *       Filename:  fastrpc.cpp
 *
 *    Description:  Implementation of support classes for rpc-over-stream
 *
 *        Version:  1.0
 *        Created:  08/17/2022 09:34:22 AM
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

#include <vector>
#include <string>
#include <regex>
#include "rpc.h"
using namespace rpcf;
#include "defines.h"
#include "ifhelper.h"
#include "stmport.h"
#include "fastrpc.h"

namespace rpcf
{

gint32 CRpcStmChanSvr::AcceptNewStream(
    IEventSink* pCallback,
    IConfigDb* pDataDesc )
{
    gint32 ret = 0;
    do{
        if( pDataDesc == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        ret = super::AcceptNewStream(
            pCallback, pDataDesc );
        if( ERROR( ret ) )
            break;

        IConfigDb* ptctx;
        CCfgOpener oDesc( pDataDesc );
        ret = oDesc.GetPointer(
            propTransCtx, ptctx );
        if( ERROR( ret ) )
            break;

        stdstr strSess;
        CCfgOpener oCtx( ptctx ); 
        ret = oCtx.GetStrProp(
            propSessHash, strSess );
        if( ERROR( ret ) )
            break;

        stdstr strPath;
        ret = oCtx.GetStrProp(
            propRouterPath, strPath );
        if( ERROR( ret ) )
            break;
            
        CWriteLock oLock( this->GetSharedLock() );

        auto itr = m_mapSessRefs.find( strSess );
        if( itr == m_mapSessRefs.end() )
            m_mapSessRefs[ strSess ] = 1;
        else
        {
            if( itr->second >=
                MAX_REQCHAN_PER_SESS )
            {
                ret = -ERANGE;
                break;
            }
            itr->second++;
        }

    }while( 0 );

    return ret;
}

gint32 CFastRpcServerBase::OnPreStop(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    if( pCallback == nullptr )
        return -EINVAL;
    do{
        CStdRMutex oLock( GetLock() );
        if( m_mapSkelObjs.empty() )
            break;

        std::vector< InterfPtr > vecIfs;
        for( auto& elem : m_mapSkelObjs )
            vecIfs.push_back( elem.second );
        m_mapSkelObjs.clear();
        oLock.Unlock();

        CParamList oParams;
        oParams[ propIfPtr ] = ObjPtr( this );

        TaskGrpPtr pTaskGrp;
        ret = pTaskGrp.NewObj(
            clsid( CIfTaskGroup ),
            oParams.GetCfg() );
        if( ERROR( ret ) )
            break;

        pTaskGrp->SetRelation( logicNONE );
        pTaskGrp->SetClientNotify( pCallback );

        for( auto& elem : vecIfs )
        {
            TaskletPtr pTask;
            ret = DEFER_IFCALLEX_NOSCHED2( 
                0, pTask, ObjPtr( elem ),
                &CRpcServices::StopEx,
                nullptr );
            if( ERROR( ret ) )
                continue;

            pTaskGrp->AppendTask( pTask );
        }

        if( pTaskGrp->GetTaskCount() == 0 )
            break;

        TaskletPtr pGrp = pTaskGrp;
        auto pMgr = GetIoMgr();
        ret = pMgr->RescheduleTask( pGrp );
        if( ERROR( ret ) )
            ( *pGrp )( eventCancelTask );
        else
            ret = STATUS_PENDING;

    }while( 0 );

    return ret;
}

gint32 CFastRpcServerBase::OnStartSkelComplete(
    IEventSink* pCallback,
    IEventSink* pIoReq,
    IConfigDb* pReqCtx )
{
    if( pIoReq == nullptr ||
        pReqCtx == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CCfgOpener oReqCtx( pReqCtx );

        HANDLE hstm = INVALID_HANDLE;
        ret = oReqCtx.GetIntPtr(
            propStmHandle, ( guint32*& )hstm );
        if( ERROR( ret ) )
            break;

        bool bStart = true;
        ret = oReqCtx.GetBoolProp(
            0, bStart );
        if( ERROR( ret ) )
            break;

        HANDLE hPort = INVALID_HANDLE;
        ret = oReqCtx.GetIntPtr(
            1, ( guint32*& )hPort );
        if( ERROR( ret ) )
            break;

        if( bStart )
        {
            IConfigDb* pResp = nullptr;
            CCfgOpenerObj oReq( pIoReq );
            ret = oReq.GetPointer(
                propRespPtr, pResp );
            if( SUCCEEDED( ret ) )
            {
                gint32 iRet = 0;
                guint32* pRet = ( guint32* )&iRet;
                CCfgOpener oResp( pResp );
                oResp.GetIntProp(
                    propReturnValue, *pRet );
                if( ERROR( iRet ) )
                {
                }
            }
            break;
        }

        bool bClosePort = false;
        ret = oReqCtx.GetBoolProp(
            2, bClosePort );
        if( ERROR( ret ) )
            break;

        RemoveStmSkel( hstm );
        if( bClosePort )
        {
            auto pMgr = this->GetIoMgr();
            pMgr->ClosePort(
                hPort, nullptr, nullptr );
        }

    }while( 0 );
    if( pCallback != nullptr )
    {
        pCallback->OnEvent(
            eventTaskComp, ret, 0, nullptr );
    }

    return ret;
}

gint32 CFastRpcServerBase::AddStmSkel(
    HANDLE hstm, InterfPtr& pIf )
{
    CStdRMutex oLock( GetLock() );
    m_mapSkelObjs[ hstm ] = pIf;
    return 0;
}

gint32 CFastRpcServerBase::RemoveStmSkel(
    HANDLE hstm )
{
    CStdRMutex oLock( GetLock() );
    m_mapSkelObjs.erase( hstm );
    return 0;
}

gint32 CFastRpcServerBase::GetStmSkel(
    HANDLE hstm, InterfPtr& pIf )
{
    CStdRMutex oLock( GetLock() );
    if( m_mapSkelObjs.empty() )
        return -ENOENT;
    if( hstm == INVALID_HANDLE )
    {
        auto itr = m_mapSkelObjs.begin();
        pIf = itr->second;
        return STATUS_SUCCESS;
    }

    auto itr = m_mapSkelObjs.find( hstm );
    if( itr == m_mapSkelObjs.end() )
        return -ENOENT;

    pIf = itr->second;
    return STATUS_SUCCESS;
}

gint32 CFastRpcServerBase::OnRmtSvrEvent(
    EnumEventId iEvent,
    IConfigDb* pEvtCtx,
    HANDLE hPort )
{
    if( pEvtCtx == nullptr )
        return -EINVAL;
    gint32 ret = 0;
    do{
        bool bOnline =
            ( iEvent == eventRmtSvrOnline );

        CCfgOpener oEvtCtx( pEvtCtx );
        HANDLE hstm = INVALID_HANDLE;
        ret = oEvtCtx.GetIntPtr(
            propStmHandle, ( guint32*& )hstm );
        if( ERROR( ret ) )
            break;
        
        InterfPtr pIf;
        if( bOnline )
        {
            PortPtr pPort;
            auto pMgr = this->GetIoMgr();
            ret = pMgr->GetPortPtr(
                hPort, pPort );
            if( ERROR( ret ) )
                break;

            Variant var;
            ret = pPort->GetProperty(
                propPortId, var );
            if( ERROR( ret ) )
                break;

            guint32 dwPortId = var;
            
            ret = CreateStmSkel(
                hstm, dwPortId, pIf );
            if( SUCCEEDED( ret ) )
                AddStmSkel( hstm, pIf );
        }
        else
            ret = GetStmSkel( hstm, pIf );

        if( ERROR( ret ) )
        {
            CIoManager* pMgr = GetIoMgr();
            pMgr->ClosePort(
                hPort, nullptr, nullptr );
            break;
        }

        CParamList oReqCtx;
        oReqCtx.SetPointer(
            propIfPtr, ( IEventSink* )pIf );

        oReqCtx.SetIntPtr(
            propStmHandle, ( guint32*& )hstm );

        // whether start or stop
        oReqCtx.Push( bOnline );

        // just for allocating prop 1
        oReqCtx.Push( bOnline );
        // fill prop 1 with an unsupport type
        oReqCtx.SetIntPtr( 1, ( guint32*& )hPort );

        TaskletPtr pTask;
        if( bOnline )
        {
            ret = DEFER_IFCALLEX_NOSCHED2(
                0, pTask, ObjPtr( pIf ),
                &CRpcServices::StartEx,
                nullptr );
        }
        else
        {
            bool bClosePort = false;
            CRpcServices* pSvc = pIf;
            if( pSvc->GetPortHandle() ==
                INVALID_HANDLE )
                bClosePort = true;

            // explicitly close the port
            oReqCtx.Push( bClosePort );
            ret = DEFER_IFCALLEX_NOSCHED2(
                0, pTask, ObjPtr( pIf ),
                &CRpcServices::StopEx,
                nullptr );
        }
        if( ERROR( ret ) )
            break;

        TaskletPtr pRespCb;
        ret = NEW_PROXY_RESP_HANDLER2(
            pRespCb, ObjPtr( this ), 
            &CFastRpcServerBase::OnStartSkelComplete,
            nullptr,
            ( IConfigDb* )oReqCtx.GetCfg() );
        if( ERROR( ret ) )
        {
            ( *pTask )( eventCancelTask );
            break;
        }
        CIfRetryTask* pRetry = pTask;
        pRetry->SetClientNotify( pRespCb );
        ret = this->AddSeqTask( pTask );
        if( SUCCEEDED( ret ) )
            ret = pTask->GetError();

        if( ret != STATUS_PENDING )
            ( *pRespCb )( eventCancelTask );
                
    }while( 0 );

    return ret;
}

gint32 CFastRpcServerBase::GetStream(
    IEventSink* pCallback,
    HANDLE& hStream )
{
    CCfgOpenerObj oTaskCfg( pCallback );
    return  oTaskCfg.GetIntPtr(
        propStmHandle, ( guint32*& )hStream );
}

gint32 CFastRpcServerBase::CheckReqCtx(
    IEventSink* pCallback,
    DMsgPtr& pMsg )
{
    if( pCallback == nullptr || pMsg.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;
    do{
        ObjPtr pCtxObj;
        IConfigDb* pReqCtx;
        ret = pMsg.GetObjArgAt( 1, pCtxObj );
        if( ERROR( ret ) )
        {
            ret = 0;
            break;
        }

        pReqCtx = pCtxObj;
        if( pReqCtx == nullptr )
        {
            // ipc clients
            break;
        }

        TaskletPtr pCb = ObjPtr( pCallback );
        CCfgOpener oTaskCfg(
            ( IConfigDb* )pCb->GetConfig() );

        oTaskCfg.CopyProp(
            propRouterPath, pReqCtx );

        oTaskCfg.CopyProp(
            propSessHash, pReqCtx );

        CCfgOpener oReqCtx( pReqCtx );
        oTaskCfg.CopyProp(
            propTimestamp, pReqCtx );

        oTaskCfg.CopyProp(
            propStmHandle, pReqCtx );

        CIfInvokeMethodTask* pInv =
            ObjPtr( pCallback );

        if( likely( pInv != nullptr ) &&
            pInv->GetTimeLeft() <= 0 )
        {
            ret = -ETIMEDOUT;
            break;
        }

    }while( 0 );

    return ret;
}

gint32 CFastRpcProxyBase::OnPostStart(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    if( pCallback == nullptr )
        return -EINVAL;
    do{
        InterfPtr pIf;
        ret = CreateStmSkel( pIf );
        if( ERROR( ret ) )
            break;

        m_pSkelObj = pIf;
        ret = pIf->StartEx( pCallback );

    }while( 0 );

    return ret;
}

gint32 CFastRpcProxyBase::OnPreStop(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    if( pCallback == nullptr )
        return -EINVAL;
    do{
        if( m_pSkelObj.IsEmpty() )
            break;

        ret = m_pSkelObj->StopEx(
            pCallback );

    }while( 0 );

    return ret;
}


}
