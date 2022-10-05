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
            ++itr->second;
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

template< typename T1, typename T >
gint32 CIfStartRecvMsgTask2::HandleIncomingMsg2(
    ObjPtr& ifPtr, T1& pMsg )
{
    gint32 ret = 0;

    // servicing the incoming request/event
    // message
    do{
        CFastRpcSkelSvrBase* pIf = ifPtr;

        if( pIf == nullptr || pMsg.IsEmpty() )
            return -EFAULT;

        CParamList oParams;
        ret = oParams.SetObjPtr( propIfPtr, pIf );
        if( ERROR( ret ) )
            break;

        BufPtr pBuf;
        pBuf.NewObj();
        *pBuf = pMsg;
        ret = oParams.SetProperty( propMsgPtr, pBuf );
        if( ERROR( ret ) )
            break;

        // copy the match ptr if this task is for
        // a proxy interface
        oParams.CopyProp( propMatchPtr, this );

        TaskletPtr pTask;

        // DebugPrint( 0, "probe: before invoke" );
        ret = pTask.NewObj(
            clsid( CIfInvokeMethodTask2 ),
            oParams.GetCfg() );
        if( ERROR( ret ) )
            break;

        CIoManager* pMgr = pIf->GetIoMgr();
        ret = DEFER_CALL( pMgr, ObjPtr( pIf ),
            &CFastRpcSkelSvrBase::AddAndRunInvTask,
            pTask, false );

        break;

    }while( 0 );

    return ret; 
}

gint32 CIfStartRecvMsgTask2::OnIrpComplete(
    PIRP pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = pIrp->GetStatus(); 
    if( ERROR( ret ) )
    {
        if( ret != -EAGAIN )
        {
            // fatal error, just quit
            DebugPrint( ret,
                "Cannot continue to receive message" );
            return ret;
        }
        ret = 0;
    }

    CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
    ObjPtr pObj;
    
    ret = oCfg.GetObjPtr( propIfPtr, pObj );
    if( ERROR( ret ) )
        return ret;

    CRpcServices* pIf = pObj;
    if( pIf == nullptr )
        return -EFAULT;

    IMessageMatch* pMatch = nullptr;
    ret = oCfg.GetPointer( propMatchPtr, pMatch );
    if( pMatch == nullptr )
        return -EFAULT;

    do{
        if( !pIf->IsConnected() )
        {
            ret = ERROR_STATE;
            break;
        }

        CIoManager* pMgr = pIf->GetIoMgr();
        if( pMgr == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        // whether error or not receiving, we
        // proceed to handle the incoming irp now
        ret = pIrp->GetStatus();
        if( ret == -EAGAIN )
            break;

        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        if( !pCtx->IsNonDBusReq() )
        {
            DMsgPtr pMsg = *pCtx->m_pRespData;
            if( pMsg.IsEmpty() )
            {
                ret = -EFAULT;
                break;
            }

            HandleIncomingMsg2( pObj, pMsg );
        }
        else
        {
            CfgPtr pCfg;
            ret = pCtx->GetRespAsCfg( pCfg );
            if( ERROR( ret ) )
                break;

            HandleIncomingMsg2( pObj, pCfg );
        }

        do{
            // continue receiving the message
            CParamList oParams;

            // clone all the parameters in this
            // task
            oParams[ propIfPtr ] = pObj;
            oParams.CopyProp( propMatchPtr, this );
            oParams.CopyProp( propExtInfo, this );
            ObjPtr pObj = oParams.GetCfg();

            // offload the output tasks to the
            // task threads
            ret = this->StartNewRecv( pObj );
            //
            // ret = DEFER_CALL( pMgr, ObjPtr( this ),
            //     &CIfStartRecvMsgTask::StartNewRecv,
            //     pObj );

        }while( 0 );

        if( ERROR( ret ) )
            break;

    }while( 0 );

    // this task is retired
    return 0;
}

gint32 CIfStartRecvMsgTask2::StartNewRecv(
    ObjPtr& pCfg )
{
    // NOTE: no touch of *this, so no locking
    gint32 ret = 0;
    if( pCfg.IsEmpty() )
        return -EINVAL;

    CCfgOpener oCfg( ( IConfigDb* )pCfg );

    CFastRpcSkelSvrBase* pIf = nullptr;
    ret = oCfg.GetPointer( propIfPtr, pIf );
    if( ERROR( ret ) )
        return -EINVAL;

    ObjPtr pMatch;
    ret = oCfg.GetObjPtr( propMatchPtr, pMatch );
    if( ERROR( ret ) )
        return -EINVAL;

    do{
        CParamList oParams;

        oParams[ propIfPtr ] = ObjPtr( pIf );
        oParams[ propMatchPtr ] = pMatch;

        // to pass some more information to the new
        // task
        oParams.CopyProp( propExtInfo, pCfg );

        // start another recvmsg request
        TaskletPtr pTask;
        ret = pTask.NewObj(
            clsid( CIfStartRecvMsgTask2 ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        CStdRMutex oIfLock( pIf->GetLock() );
        if( pIf->GetPendingInvCount() >=
            STM_MAX_PACKETS_REPORT )
        {
            pIf->QueueStartTask( pTask );            
            ret = STATUS_PENDING;
            break;
        }
        oIfLock.Unlock();
        // add an concurrent task, and run it
        // directly.
        ret = pIf->AddAndRun( pTask, true );
        if( ERROR( ret ) )
        {
            DebugPrint( ret,
                "The new RecvMsgTask failed to AddAndRun" );
            break;
        }
        ret = pTask->GetError();
        if( ret == STATUS_PENDING )
            break;

        if( ERROR( ret ) )
        {
            DebugPrint( ret,
                "The new RecvMsgTask failed" );
            break;
        }

   }while( 0 );

   return ret;
}

gint32 CIfInvokeMethodTask2::OnComplete(
    gint32 iRet )
{
    CFastRpcSkelSvrBase* pIf = nullptr;
    CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
    gint32 ret = oCfg.GetPointer( propIfPtr, pIf );
    if( SUCCEEDED( ret ) )
        pIf->NotifyInvTaskComplete();

    return super::OnComplete( iRet );
}

gint32 CFastRpcSkelSvrBase::StartRecvTasks(
    std::vector< MatchPtr >& vecMatches )
{
    gint32 ret = 0;

    if( vecMatches.empty() )
        return 0;

    do{
        CParamList oParams;
        ret = oParams.SetObjPtr(
            propIfPtr, ObjPtr( this ) );

        if( ERROR( ret  ) )
            break;

        CCfgOpenerObj oIfCfg( this );
        bool bRfcEnabled;
        ret = oIfCfg.GetBoolProp(
            propEnableRfc, bRfcEnabled );
        if( ERROR( ret ) )
            bRfcEnabled = false;

        EnumClsid iClsid;
        if( bRfcEnabled ) 
            iClsid = clsid( CIfStartRecvMsgTask2 );
        else
            iClsid = clsid( CIfStartRecvMsgTask );

        TaskletPtr pRecvMsgTask;
        for( auto pMatch : vecMatches )
        {
            oParams[ propMatchPtr ] =
                ObjPtr( pMatch );

            ret = pRecvMsgTask.NewObj(
                iClsid, oParams.GetCfg() );

            if( ERROR( ret ) )
                break;

            ret = AddAndRun( pRecvMsgTask );
            if( ERROR( ret ) )
                break;

            if( ret == STATUS_PENDING )
                ret = 0;
        }

        if( ERROR( ret ) )
            break;

    }while( 0 );

    return ret;
}

gint32 CFastRpcSkelSvrBase::AddAndRunInvTask(
    TaskletPtr& pTask,
    bool bImmediate )
{
    CStdRMutex oIfLock( GetLock() );
    m_dwPendingInv++;
    oIfLock.Unlock();
    gint32 ret = AddAndRun( pTask, bImmediate );
    if( ERROR( ret ) )
    {
        CStdRMutex oIfLock( GetLock() );
        m_dwPendingInv--;
        return ret;
    }
    return ret;
}

gint32 CFastRpcSkelSvrBase::NotifyInvTaskComplete()
{
    CStdRMutex oIfLock( GetLock() );
    m_dwPendingInv--;
    if( m_dwPendingInv < 
        STM_MAX_PACKETS_REPORT )
    {
        if( m_queStartTasks.size() > 0  )
        {
            TaskletPtr pTask =
                m_queStartTasks.front();
            m_queStartTasks.pop_front();
            oIfLock.Unlock();
            gint32 ret = AddAndRun( pTask, false );
            if( ERROR( ret ) )
                return ret;
        }
    }
    return STATUS_SUCCESS;
}

gint32 CFastRpcSkelSvrBase::NotifyStackReady(
    PortPtr& pPort )
{
    // call this method in OnPreStart, which is the
    // only place to guarantee this request to succeed.
    if( pPort.IsEmpty() )
        return -EINVAL;

    IrpPtr pIrp( true );
    pIrp->AllocNextStack( nullptr );
    IrpCtxPtr& pCtx = pIrp->GetTopStack();
    pCtx->SetMajorCmd( IRP_MJ_PNP );
    pCtx->SetMinorCmd( IRP_MN_PNP_STACK_READY );
    pPort->AllocIrpCtxExt( pCtx );
    pCtx->SetIoDirection( IRP_DIR_OUT ); 
    pIrp->SetSyncCall( true );

    auto pMgr = this->GetIoMgr();
    HANDLE hPort = PortToHandle( pPort );
    return pMgr->SubmitIrp( hPort,
        pIrp, false );
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
            break;

        bool bClosePort = false;
        ret = oReqCtx.GetBoolProp(
            2, bClosePort );
        if( ERROR( ret ) )
            break;

        RemoveStmSkel( hstm );
        CCfgOpenerObj oIfCfg( this );

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

gint32 CFastRpcServerBase::BroadcastEvent(
    IConfigDb* pReqCall,
    IEventSink* pCallback )
{
    if( pReqCall == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        TaskGrpPtr pGrp;
        CCfgOpener oGrpCfg;
        oGrpCfg[ propIfPtr ] = ObjPtr( this );
        ret = pGrp.NewObj(
            clsid( CIfParallelTaskGrp ),
            oGrpCfg.GetCfg() );

        if( ERROR( ret ) )
            break;

        gint32 ( *func )( IEventSink*,
            CRpcServices*, IConfigDb* ) =
            ([]( IEventSink* pCallback,
                 CRpcServices* pIf,
                IConfigDb* pReqCall )->gint32
        {
            gint32 ret = 0;
            do{
                if( pIf == nullptr ||
                    pReqCall == nullptr )
                {
                    ret = -EINVAL;
                    break;
                }

                guint64 iTaskId = 0;
                CfgPtr pRespData( true );
                ret = pIf->RunIoTask(
                    pReqCall, pRespData,
                    pCallback, &iTaskId );

            }while( 0 );
            return ret;
        });

        auto pMgr = this->GetIoMgr();
        CStdRMutex oLock( GetLock() );
        for( auto& elem : m_mapSkelObjs )
        {
            TaskletPtr pTask;
            CRpcServices* pIf = elem.second;
            ret = NEW_FUNCCALL_TASK2( 0, 
                pTask, pMgr, func,
                nullptr,
                ( CRpcServices* )pIf,
                pReqCall );
            if( ERROR( ret ) )
                continue;
            pGrp->AppendTask( pTask );
        }
        oLock.Unlock();

        CIfParallelTaskGrp *pParaGrp = pGrp;
        if( pParaGrp->GetPendingCountLocked() == 0 )
            break;

        TaskletPtr pGrpTask = pParaGrp;
        ret = this->AddAndRun( pGrpTask );

    }while( 0 );

    return ret;
}

gint32 CFastRpcServerBase::AddStmSkel(
    HANDLE hstm, InterfPtr& pIf )
{
    if( hstm == INVALID_HANDLE )
        return -EINVAL;

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
        // for broadcasting events
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
        PortPtr pPort;
        if( bOnline )
        {
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
            stdstr strFunc = stdstr( "Warning " ) +
                ( bOnline ? "CreateStmKel" :
                    "GetStmKel" ) + " failed";

            DebugPrint( ret, 
                strFunc.c_str() );

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

        // just make room for prop 1
        oReqCtx.Push( bOnline );

        // set prop 1 with an unsupport type by Push
        oReqCtx.SetIntPtr( 1, ( guint32*& )hPort );

        TaskletPtr pStartTask;
        TaskletPtr pTask;
        if( bOnline )
        {
            TaskletPtr pStopTask;
            gint32 (*func)( IEventSink*,
                    CFastRpcSkelSvrBase*, IPort*) =
                ( []( IEventSink* pCb,
                    CFastRpcSkelSvrBase* pIf,
                    IPort* pPort)->gint32
                {
                    if( pIf == nullptr ||
                        pPort == nullptr ||
                        pCb == nullptr )
                        return -EINVAL;
                    PortPtr portPtr( pPort );
                    pIf->NotifyStackReady( portPtr );
                    return pIf->StartEx( pCb );
                });

            NEW_FUNCCALL_TASK2(
                0, pStartTask,
                GetIoMgr(), func, nullptr,
                ( CFastRpcSkelSvrBase* )pIf,
                ( IPort* )pPort );

            DEFER_IFCALLEX_NOSCHED2(
                0, pStopTask, ObjPtr( pIf ),
                &CRpcServices::StopEx,
                nullptr );

            CParamList oParams;
            oParams[ propIfPtr ] = ObjPtr( this );

            TaskGrpPtr pTaskGrp;
            pTaskGrp.NewObj(
                clsid( CIfTaskGroup ),
                oParams.GetCfg() );

            pTaskGrp->SetRelation( logicOR );
            pTaskGrp->AppendTask( pStartTask );
            pTaskGrp->AppendTask( pStopTask );
            pTask = pTaskGrp;
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
            DEFER_IFCALLEX_NOSCHED2(
                0, pTask, ObjPtr( pIf ),
                &CRpcServices::StopEx,
                nullptr );
        }

        TaskletPtr pRespCb;
        NEW_PROXY_RESP_HANDLER2(
            pRespCb, ObjPtr( this ), 
            &CFastRpcServerBase::OnStartSkelComplete,
            nullptr,
            ( IConfigDb* )oReqCtx.GetCfg() );

        CIfRetryTask* pRetry =
            bOnline ? pStartTask : pTask;
        pRetry->SetClientNotify( pRespCb );
        auto pMgr = this->GetIoMgr();
        ret = pMgr->RescheduleTask( pTask );
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

gint32 CFastRpcSkelSvrBase::CheckReqCtx(
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

gint32 CFastRpcProxyBase::OnPostStop(
    IEventSink* pCallback )
{
    m_pSkelObj.Clear();
    return 0;
}

gint32 CFastRpcProxyBase::OnRmtSvrEvent(
    EnumEventId iEvent,
    IConfigDb* pEvtCtx,
    HANDLE hPort )
{
    if( pEvtCtx == nullptr )
        return -EINVAL;
    gint32 ret = 0;
    do{
        if( iEvent == eventRmtSvrOnline )
            break;
        CRpcServices* pSvc = m_pSkelObj;
        pSvc->SetStateOnEvent( cmdShutdown );
        // stop at this point could result in segment
        // fault. ClosePort is good.
        pSvc->ClosePort( nullptr );
        
    }while( 0 );

    return ret;
}

}
