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
#include "jsondef.h"

namespace rpcf
{

gint32 FASTRPC_MSG::Serialize( CBuffer& oBuf )
{
    gint32 ret = 0;
    do{
        guint32 dwSize = 0;
        guint32 dwOrigOff = oBuf.offset();

        ret = oBuf.Append(
            ( guint8* )&dwSize, sizeof( dwSize ) );
        if( ERROR( ret ) )
            break;

        ret = oBuf.Append(
            &m_bType, sizeof( m_bType ) );
        if( ERROR( ret ) )
            break;

        ret = oBuf.Append(
            m_szSig, sizeof( m_szSig ) );

        ret = m_pCfg->Serialize( oBuf );
        if( ERROR( ret ) )
            break;

        dwSize = htonl(
            oBuf.size() - sizeof( dwSize ) );
        char* pLoc = oBuf.ptr();
        memcpy( pLoc, &dwSize, sizeof( dwSize ) );

    }while( 0 );

    return ret;
}

gint32 FASTRPC_MSG::Deserialize( CBuffer& oBuf )
{
    gint32 ret = 0;
    do{
        guint32 dwSize = 0;
        char* pLoc = oBuf.ptr();
        memcpy( &dwSize,
            pLoc, sizeof( guint32 ) );
        dwSize = ntohl( dwSize );
        if( dwSize > oBuf.size() )
        {
            ret = -ERANGE;
            break;
        }
        pLoc += sizeof( guint32 );
        m_bType = pLoc[ 0 ];
        if( m_bType != typeObj )
        {
            ret = -EINVAL;
            break;
        }
        pLoc++;
        m_szSig[ 0 ] = *pLoc++;
        m_szSig[ 1 ] = *pLoc++;
        m_szSig[ 2 ] = *pLoc++;
        if( m_szSig[ 0 ] != 'F' ||
            m_szSig[ 1 ] != 'R' ||
            m_szSig[ 2 ] != 'M' )
        {
            ret = -EBADMSG;
            break;
        }

        if( pLoc - oBuf.ptr() >= oBuf.size() )
        {
            ret = -ERANGE;
            break;
        }

        if( m_pCfg.IsEmpty() )
            m_pCfg.NewObj();

        ret = m_pCfg->Deserialize( pLoc,
            oBuf.size() - ( pLoc - oBuf.ptr() ) );

    }while( 0 );

    return ret;
}

bool CRpcStmChanCli::IsLocalStream() const
{
    CLocalProxyState* p = this->m_pIfStat;
    if( p != nullptr )
        return true;
    return false;
}

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

        Variant oVar;
        ret = this->GetProperty(
            propStmPerSess, oVar );
        if( ERROR( ret ) )
        {
            oVar = MAX_REQCHAN_PER_SESS;
            ret = 0;
        }
        CWriteLock oLock( this->GetSharedLock() );
        auto itr = m_mapSessRefs.find( strSess );
        if( itr != m_mapSessRefs.end() )
        {
            if( itr->second >= ( guint32& )oVar )
            {
                ret = -ERANGE;
                DebugPrintEx( logErr, ret,
                    "Stream limits reached, "
                    "and new stream is rejected" );
                break;
            }
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

        pTask->MarkPending();
        CIoManager* pMgr = pIf->GetIoMgr();
        ret = DEFER_CALL( pMgr, ObjPtr( pIf ),
            &CFastRpcSkelSvrBase::AddAndRunInvTask,
            pTask );

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
        if( ret != -EAGAIN )
        {
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
        }

        do{
            // continue receiving the message
            CParamList oParams;

            // clone all the parameters in this
            // task
            oParams[ propIfPtr ] = pObj;
            oParams.CopyProp( propMatchPtr, this );
            ObjPtr pObj = oParams.GetCfg();

            ret = this->StartNewRecv( pObj );

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

        // start another recvmsg request
        TaskletPtr pTask;
        ret = pTask.NewObj(
            clsid( CIfStartRecvMsgTask2 ),
            oParams.GetCfg() );
        if( ERROR( ret ) )
            break;

        TaskletPtr pQpsTask = pIf->GetQpsTask();
        if( !pQpsTask.IsEmpty() )
        {
            CIfRetryTask* pRetry = pQpsTask;
            CStdRTMutex oTaskLock(
                pRetry->GetLock() );

            ret = pIf->AllocReqToken();
            if( ERROR( ret ) )
            {
                CStdRMutex oIfLock(
                    pIf->GetLock() );

                ret = pIf->QueueStartTask(
                    pTask, pMatch );            

                break;
            }
        }

        CIfParallelTaskGrp* pPara = nullptr;
        TaskGrpPtr pGrp = pIf->GetGrpRfc();
        CIfParallelTaskGrpRfc* pGrpRfc = pGrp;
        bool bFull = false;
        if( true )
        {
            CStdRTMutex oLock( pGrp->GetLock() );
            ret = pGrpRfc->IsQueueFull( bFull );
            if( ERROR( ret ) )
                break;
            if( bFull )
            {
                CStdRMutex oIfLock( pIf->GetLock() );
                ret = pIf->QueueStartTask(
                    pTask, pMatch );            
                break;
            }
            Variant oVar;
            ret = pGrp->GetProperty(
                propParentTask, oVar );
            if( ERROR( ret ) )
                break;

            pPara = ( ObjPtr& )oVar;
            ret = pPara->AppendTask( pTask );
            if( ERROR( ret ) )
                break;
        }

        // add the task to concurrent taskgrp
        ret = ( *pPara )( eventZero );
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
    oCfg.GetPointer( propIfPtr, pIf );
    if( !oCfg.exist( propQueFull ) ) 
        pIf->NotifyInvTaskComplete();

    return super::OnComplete( iRet );
}

CFastRpcSkelSvrBase::CFastRpcSkelSvrBase(
    const IConfigDb* pCfg ) :
    super( pCfg )
{
    CCfgOpener oCfg( pCfg );
    bool bRfcEnabled = true;
    gint32 ret = oCfg.GetBoolProp(
        propEnableRfc, bRfcEnabled );
    if( SUCCEEDED( ret ) )
        m_bRfcEnabled = bRfcEnabled;
}

gint32 CFastRpcSkelSvrBase::SetLimit(
    guint32 dwMaxRunning,
    guint32 dwMaxPending,
    bool bNoResched )
{
    TaskGrpPtr pGrp;
    CIfParallelTaskGrpRfc* pGrpRfc = m_pGrpRfc;
    if( pGrpRfc )
        pGrpRfc->SetLimit( dwMaxRunning,
            dwMaxPending, bNoResched );

    return STATUS_SUCCESS;
}

gint32 CFastRpcSkelSvrBase::StartRecvTasks(
    std::vector< MatchPtr >& vecMatches )
{
    gint32 ret = 0;

    if( vecMatches.empty() )
        return 0;

    do{
        if( !m_bRfcEnabled )
        {
            ret = super::StartRecvTasks(
                vecMatches );
            break;
        }

        CCfgOpener oCfg;
        oCfg[ propIfPtr ] = ObjPtr( this );
        ret = m_pGrpRfc.NewObj(
            clsid( CIfParallelTaskGrpRfc ),
            oCfg.GetCfg() );
        if( ERROR( ret ) )
            break;

        TaskletPtr pPHTask;
        ret = pPHTask.NewObj(
            clsid( CIfCallbackInterceptor ),
            oCfg.GetCfg() );
        if( ERROR( ret ) )
            break;

        m_pGrpRfc->AppendTask( pPHTask );

        this->SetLimit( 3,
            STM_MAX_PACKETS_REPORT - 2,
            true );

        TaskletPtr pTask( m_pGrpRfc );
        ret = this->AddAndRun( pTask );
        if( ERROR( ret ) )
            return ret;

        TaskletPtr pRecvMsgTask;
        for( auto pMatch : vecMatches )
        {
            oCfg[ propMatchPtr ] =
                ObjPtr( pMatch );

            ret = pRecvMsgTask.NewObj(
                clsid( CIfStartRecvMsgTask2 ),
                oCfg.GetCfg() );

            if( ERROR( ret ) )
                break;

            ret = AddAndRun( pRecvMsgTask );
            if( ERROR( ret ) )
                break;

            if( ret == STATUS_PENDING )
                ret = 0;
        }

    }while( 0 );

    return ret;
}

gint32 CFastRpcSkelSvrBase::AddAndRunInvTask(
    TaskletPtr& pTask )
{
    CIfParallelTaskGrpRfc* pGrpRfc = m_pGrpRfc;
    gint32 ret = pGrpRfc->AddAndRun( pTask );

    if( SUCCEEDED( ret ) )
        return ret;

    if( ERROR( ret ) )
    {
        CCfgOpener oResp;
        oResp[ propReturnValue ] = ret;
        CCfgOpener oTaskCfg( ( IConfigDb* )
            pTask->GetConfig() );
        oTaskCfg[ propQueFull ] = true;
        OnServiceComplete(
            oResp.GetCfg(), pTask );
    }
    return ret;
}

gint32 CFastRpcSkelSvrBase::NotifyInvTaskComplete()
{
    gint32 ret = 0;
    do{
        stdrtmutex* pmutex = nullptr;
        stdrtmutex oFakeMutex;
        CTokenBucketTask* pTask = GetQpsTask();
        if( pTask != nullptr )
        {
            pmutex = &pTask->GetLock();
        }
        else
            pmutex = &oFakeMutex;

        CStdRTMutex oTaskLock( *pmutex );
        if( pTask != nullptr )
        {
            ret = this->AllocReqToken();
            if( ERROR( ret ) )
            {
                ret = 0;
                break;
            }
        }
        CStdRMutex oIfLock( GetLock() );
        if( m_queStartTasks.size() > 0  )
        {
            TaskletPtr pTask =
                m_queStartTasks.front().first;
            m_queStartTasks.pop_front();
            oIfLock.Unlock();
            oTaskLock.Unlock();
            ret = AddAndRun( pTask, false );
        }
    }while( 0 );
    return STATUS_SUCCESS;
}

gint32 CFastRpcSkelSvrBase::NotifyTokenAvailable()
{
    gint32 ret = STATUS_SUCCESS;
    do{
        bool bFull = false;
        stdrtmutex* pmutex = nullptr;
        stdrtmutex oFakeMutex;
        CIfParallelTaskGrpRfc* pGrp = m_pGrpRfc;
        if( IsRfcEnabled() )
        {
            pmutex = &pGrp->GetLock();
        }
        else
            pmutex = &oFakeMutex;
        CStdRTMutex oTaskLock( *pmutex );
        if( IsRfcEnabled() )
        {
            ret = pGrp->IsQueueFull( bFull );
            if( ERROR( ret ) || bFull )
                break;
        }
        CStdRMutex oIfLock( GetLock() );
        if( m_queStartTasks.size() > 0  )
        {
            TaskletPtr pTask =
                m_queStartTasks.front().first;
            m_queStartTasks.pop_front();
            oIfLock.Unlock();
            oTaskLock.Unlock();
            gint32 ret = AddAndRun( pTask, false );
            if( ERROR( ret ) )
                break;
        }
    }while( 0 );
    return ret;
}

gint32 CFastRpcSkelSvrBase::NotifySkelReady(
    PortPtr& pPort )
{
    // call this method in OnPreStart, which is the
    // only place to guarantee this request to succeed.
    if( pPort.IsEmpty() )
        return -EINVAL;

    IrpPtr pIrp( true );
    pIrp->AllocNextStack( nullptr );
    IrpCtxPtr& pCtx = pIrp->GetTopStack();
    pCtx->SetMajorCmd( IRP_MJ_FUNC );
    pCtx->SetMinorCmd( IRP_MN_IOCTL );
    pCtx->SetCtrlCode( CTRLCODE_SKEL_READY );
    pPort->AllocIrpCtxExt( pCtx );
    pCtx->SetIoDirection( IRP_DIR_OUT ); 
    pIrp->SetSyncCall( true );

    auto pMgr = this->GetIoMgr();
    HANDLE hPort = PortToHandle( pPort );
    return pMgr->SubmitIrp( hPort,
        pIrp, false );
}

gint32 CFastRpcSkelSvrBase::OnKeepAliveOrig(
    IEventSink* pTask )
{
    if( pTask == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CIfInvokeMethodTask* pInvTask =
            ObjPtr( pTask );

        CStdRTMutex oLock( pInvTask->GetLock() );
        CCfgOpener oInvCfg(
            ( IConfigDb* )pInvTask->GetConfig() );

        IConfigDb* pCfg = nullptr;
        ret = oInvCfg.GetPointer(
            propReqPtr, pCfg ); 
        if( ERROR( ret ) )
            break;

        CReqOpener oReq( pCfg );
        guint64 iTaskId = 0;
        ret = oReq.GetQwordProp(
            propTaskId, iTaskId );
        if( ERROR( ret ) )
            break;

        CReqBuilder okaReq( this );
        stdstr strIfName =
            DBUS_IF_NAME( "IInterfaceServer" );
        okaReq.SetIfName( strIfName );
        okaReq.SetMethodName(
            SYS_EVENT_KEEPALIVE );

        okaReq.SetCallFlags( 
           DBUS_MESSAGE_TYPE_SIGNAL |
           CF_ASYNC_CALL |
           CF_NON_DBUS );

        stdstr strVal;
        ret = oReq.GetStrProp(
            propDestDBusName, strVal );
        if( SUCCEEDED( ret ) )
            okaReq.SetSender( strVal );

        ret = oReq.GetStrProp(
            propSrcDBusName, strVal );
        if( SUCCEEDED( ret ) )
            okaReq.SetDestination( strVal );

        oLock.Unlock();

        okaReq.Push( iTaskId );
        okaReq.Push( ( guint32 )KATerm );

        TaskletPtr pDummyTask;

        ret = pDummyTask.NewObj(
            clsid( CIfDummyTask ) );

        if( ERROR( ret ) )
            break;

        ret = BroadcastEvent(
            okaReq.GetCfg(), pDummyTask );

    }while( 0 );

    return ret;
}

gint32 CFastRpcSkelSvrBase::OnPostStart(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        Variant oVar;
        ret = this->GetProperty(
            propEnableQps, oVar );
        if( ERROR( ret ) )
        {
            ret = 0;
            break;
        }
        if( !( bool& )oVar )
        {
            ret = 0;
            break;
        }

        auto pMgr = this->GetIoMgr();
        ret = pMgr->GetCmdLineOpt(
            propBuiltinRt, oVar );
        if( SUCCEEDED( ret ) && ( ( bool& )oVar ) )
        {
            // builtin-rt server
            m_dwLoopTag = NETSOCK_TAG;
        }
        else
        {
            // micro-service server
            m_dwLoopTag = UXSOCK_TAG;
        }

        CLoopPools& oPools = pMgr->GetLoopPools();
        ret = oPools.AllocMainLoop(
            m_dwLoopTag, m_pLoop );

        if( ERROR( ret ) )
        {
            m_dwLoopTag = 0;
            m_pLoop = pMgr->GetMainIoLoop();
        }

        ret = 0;
        gint32 (*func)( CFastRpcSkelSvrBase*) =
        ([]( CFastRpcSkelSvrBase* pIf )->gint32
        {
            CFastRpcServerBase* pSvr =
                ObjPtr( pIf->GetStreamIf() );
            guint64 qwTokens =
                pSvr->GetPerSkelTokens();
            pIf->SetMaxTokens( qwTokens );
            if( pIf->IsRfcEnabled() )
            {
                pIf->NotifyTokenAvailable();
                // DebugPrint( qwTokens, "skelsvr token " );
            }
            return STATUS_PENDING;
        });

        TaskletPtr pTask;
        NEW_FUNCCALL_TASKEX( pTask,
            this->GetIoMgr(), func, this );

        oVar = ObjPtr( m_pLoop );
        pTask->SetProperty( propLoopPtr, oVar );
        ret = StartQpsTask( pTask );
        if( SUCCEEDED( ret ) )
        {
            // start the timer
            this->AllocReqToken();
        }

    }while( 0 );

    return ret;
}

gint32 CFastRpcServerBase::OnStartSkelComplete(
    IEventSink* pCallback,
    IEventSink* pIoReq,
    IConfigDb* pReqCtx )
{
    if( pIoReq == nullptr || pReqCtx == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    HANDLE hstm = INVALID_HANDLE;
    bool bStart = true;
    do{
        CCfgOpener oReqCtx( pReqCtx );

        ret = oReqCtx.GetIntPtr(
            propStmHandle, ( guint32*& )hstm );
        if( ERROR( ret ) )
            break;

        HANDLE hPort = INVALID_HANDLE;
        ret = oReqCtx.GetIntPtr(
            1, ( guint32*& )hPort );
        if( ERROR( ret ) )
            break;

        CCfgOpenerObj oIoReq( pIoReq );
        IConfigDb* pResp = nullptr;
        ret = oIoReq.GetPointer(
            propRespPtr, pResp );
        if( ERROR( ret ) )
            break;

        gint32 iRet = 0;
        CCfgOpener oResp( pResp );
        ret = oResp.GetIntProp(
            propReturnValue, ( guint32& )iRet );
        if( ERROR( ret ) )
            break;

        oReqCtx.GetBoolProp( 0, bStart );
        if( bStart )
        {
            if( ERROR( iRet ) )
            {
                ret = iRet;
                // avoid both NotifySkelReady and the
                // following stop-task calling StopEx
                // on pIf.
                break;
            }
            CFastRpcSkelSvrBase* pIf;
            ret = oReqCtx.GetPointer(
                propIfPtr, pIf );
            if( SUCCEEDED( ret ) )
            {
                PortPtr pPort = pIf->GetPort();
                pIf->NotifySkelReady( pPort );
            }
            break;
        }

        bool bClosePort = false;
        ret = oReqCtx.GetBoolProp(
            2, bClosePort );
        if( ERROR( ret ) )
        {
            ret = 0;
            break;
        }

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
    gint32 ret = 0;
    do{
        CStdRMutex oLock( GetLock() );
        if( m_mapSkelObjs.empty() )
        {
            ret = -ENOENT;
            break;
        }
        if( hstm == INVALID_HANDLE )
        {
            // for broadcasting events
            auto itr = m_mapSkelObjs.begin();
            pIf = itr->second;
            break;
        }

        auto itr = m_mapSkelObjs.find( hstm );
        if( itr == m_mapSkelObjs.end() )
        {
            ret = -ENOENT;
            break;
        }

        pIf = itr->second;

    }while( 0 );

    return ret;
}

gint32 CFastRpcServerBase::EnumStmSkels(
    std::vector< InterfPtr >& vecIfs ) const
{
    CStdRMutex oLock( GetLock() );
    if( m_mapSkelObjs.empty() )
        return -ENOENT;

    for( auto& elem : m_mapSkelObjs )
        vecIfs.push_back( elem.second );

    return STATUS_SUCCESS;
}

guint32 CFastRpcServerBase::GetStmSkelCount() const
{
    CStdRMutex oLock( GetLock() );
    return m_mapSkelObjs.size();
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

        PortPtr pPort;
        ret = GetIoMgr()->GetPortPtr(
            hPort, pPort );
        if( ERROR( ret ) )
            break;

        Variant oVar;
        pPort->GetProperty( propBusId, oVar );
        guint32 dwBusId = oVar;
        if( dwBusId != this->GetBusId() )
            break;

        CCfgOpener oEvtCtx( pEvtCtx );
        HANDLE hstm = INVALID_HANDLE;
        ret = oEvtCtx.GetIntPtr(
            propStmHandle, ( guint32*& )hstm );
        if( ERROR( ret ) )
            break;
        
        InterfPtr pIf;
        if( bOnline )
        {
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
                ( bOnline ? "CreateStmSkel" :
                    "GetStmSkel" ) + " failed";

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
        TaskletPtr pStopTask;
        TaskletPtr pTask;
        if( bOnline )
        {
            // set qps settings
            Variant oVar, oQpsPol;
            gint32 iRet = this->GetProperty(
                propEnableQps, oVar );
            bool bQps = oVar;
            iRet = this->GetProperty(
                propQpsPolicy, oQpsPol );

            if( bQps && SUCCEEDED( iRet ) )
            {
                pIf->SetProperty(
                    propEnableQps, oVar );
                guint64 qwMaxTokens;
                this->GetMaxTokens( qwMaxTokens );
                guint32 qwCount =
                    this->GetStmSkelCount();

                qwCount = qwMaxTokens / qwCount;
                if( qwCount == 0 )
                    qwCount = 1;
                pIf->SetProperty(
                    propQps, qwCount );
            }

            DEFER_IFCALLEX_NOSCHED2(
                0, pStartTask, ObjPtr( pIf ),
                &CRpcServices::StartEx,
                nullptr );

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

        if( ERROR( ret ) )
        {
            pRetry->ClearClientNotify();
            ( *pTask )( eventCancelTask );
            ( *pRespCb )( eventCancelTask );
        }
                
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

gint32 CFastRpcServerBase::OnPreStartComplete(
    IEventSink* pCallback,
    IEventSink* pIoReq,
    IConfigDb* pReqCtx )
{
    if( pCallback == nullptr ||
        pReqCtx == nullptr ||
        pIoReq == nullptr )
        return -EINVAL;
    gint32 ret = 0;
    do{
        CCfgOpenerObj oCfg( pIoReq );
        IConfigDb* pResp;
        ret = oCfg.GetPointer(
            propRespPtr, pResp );
        if( ERROR( ret ) )
            break;

        CCfgOpener oResp( pResp );
        gint32 iRet;
        ret = oResp.GetIntProp(
            propReturnValue, ( guint32& )iRet );
        if( ERROR( ret ) )
            break;

        ret = iRet;
        if( ERROR( ret ) )
            break;

        guint32 dwBusId = 0;
        CCfgOpener oCtx( pReqCtx );
        ret = oCtx.GetIntProp(
            propBusId, dwBusId );
        if( ERROR( ret ) )
            break;

        SetBusId( dwBusId );
        CCfgOpenerObj oIfCfg( this );
        stdstr strName;
        ret = oIfCfg.GetStrProp(
            propObjName, strName );
        if( ERROR( ret ) )
            break;
        
        ObjPtr pDrv;
        auto oDrvMgr = GetIoMgr()->GetDrvMgr();
        ret = oDrvMgr.GetDriver( true,
            DBUS_STREAM_BUS_DRIVER, pDrv );
        if( ERROR( ret ) )
            break;

        CDBusStreamBusDrv* pdrv = pDrv;
        if( pdrv == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        pdrv->BindNameBus(
            strName + "_s", dwBusId );

    }while( 0 );

    pCallback->OnEvent( eventTaskComp, ret, 0, 0 );
    return 0;
}

gint32 CFastRpcServerBase::OnPreStart(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        ObjPtr pDrv;
        auto oDrvMgr = GetIoMgr()->GetDrvMgr();
        ret = oDrvMgr.GetDriver( true,
            DBUS_STREAM_BUS_DRIVER, pDrv );
        if( ERROR( ret ) )
        {
            DebugPrintEx( logErr, ret, 
                "Driver %s is not loaded, "
                "Please check your config.",
                DBUS_STREAM_BUS_DRIVER );
            break;
        }

        CDBusStreamBusDrv* pdrv = pDrv;
        if( pdrv == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        IConfigDb* pCtx = nullptr;
        CCfgOpenerObj oIfCfg( this );
        ret = oIfCfg.GetPointer(
            propSkelCtx, pCtx );
        if( ERROR( ret ) )
            break;

        TaskletPtr pRespTask;
        ret = NEW_PROXY_RESP_HANDLER2(
            pRespTask, this,
            &CFastRpcServerBase::OnPreStartComplete,
            pCallback, pCtx );
        if( ERROR( ret ) )
            break;

        PortPtr pBusPort;
        CfgPtr ctxPtr( pCtx );

        ret = pdrv->Probe2( nullptr,
            pBusPort, ctxPtr, pRespTask );
        if( ERROR( ret ) )
        {
            ( *pRespTask )( eventCancelTask );
            break;
        }

        if( ret == STATUS_PENDING )
            break;

        guint32 dwBusId = 0;
        CCfgOpener oCtx( pCtx );
        ret = oCtx.GetIntProp(
            propBusId, dwBusId );
        if( ERROR( ret ) )
            break;

        SetBusId( dwBusId );
        stdstr strName;
        ret = oIfCfg.GetStrProp(
            propObjName, strName );
        if( ERROR( ret ) )
            break;

        pdrv->BindNameBus(
            strName + "_s", dwBusId );

    }while( 0 );

    return ret;
}

gint32 CFastRpcServerBase::OnPostStart(
    IEventSink* pCallback )
{
    CTokenBucketTask* ptk = m_pQpsTask;
    if( ptk == nullptr )
        return 0;
    Variant oVar;
    gint32 ret = this->GetProperty(
        propQpsPolicy, oVar );
    if( ERROR( ret ) )
        return 0;

    if( ( ( stdstr& )oVar ) == JSON_VALUE_QPOL_EQUAL )
    {
        // init per-session tokens
        guint64 qwTokens = 0;
        ptk->GetMaxTokens( qwTokens );
        m_qwPerSessTokens = qwTokens;
    }
    return 0;
}

gint32 CFastRpcServerBase::OnPostStop(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        ObjPtr pDrv;
        auto oDrvMgr = GetIoMgr()->GetDrvMgr();
        ret = oDrvMgr.GetDriver( true,
            DBUS_STREAM_BUS_DRIVER, pDrv );
        if( ERROR( ret ) )
            break;

        CDBusStreamBusDrv* pdrv = pDrv;
        if( pdrv == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        PortPtr pPort;
        ret = pdrv->GetPortById(
            GetBusId(), pPort );
        if( ERROR( ret ) )
            break;

        CCfgOpenerObj oIfCfg( this );
        stdstr strName;
        ret = oIfCfg.GetStrProp(
            propObjName, strName );
        if( ERROR( ret ) )
            break;

        pdrv->RemoveBinding( strName + "_s" );

        CDBusStreamBusPort* pBus = pPort;
        ret = pdrv->DestroyPortSynced( 
            pBus, pCallback );

    }while( 0 );

    return ret;
}

gint32 CFastRpcSkelProxyBase::BuildBufForIrp(
    BufPtr& pBuf, IConfigDb* pReqCall )
{
    return BuildBufForIrpInternal( pBuf, pReqCall );
}

gint32 CFastRpcSkelProxyBase::OnKeepAliveTerm(
    IEventSink* pTask )
{
    gint32 ret = 0;

    // servicing the incoming request/event
    // message
    do{
        CCfgOpenerObj oCfg( pTask );

        IConfigDb* pCfg = nullptr;
        ret = oCfg.GetPointer( propMsgPtr, pCfg ); 
        if( ERROR( ret ) )
            break;

        CReqOpener oEvent( pCfg );
        guint64 iTaskId = 0;
        ret = oEvent.GetQwordProp( 0, iTaskId );
        if( ERROR( ret ) )
            break;

        guint32 iPhase = 0;
        ret = oEvent.GetIntProp( 1, iPhase );
        if( ERROR( ret ) )
            break;

        if( iPhase != ( guint32 )KATerm )
        {
            ret = -EINVAL;
            break;
        }

        TaskGrpPtr pRootGrp;
        TaskletPtr pTaskToKa;
        if( true )
        {
            CStdRMutex oIfLock( GetLock() );
            if( !IsConnected() )
            {
                ret = ERROR_STATE;
                break;
            }
            pRootGrp = GetTaskGroup();
            if( pRootGrp.IsEmpty() )
            {
                ret = -ENOENT;    
                break;
            }
        }

        ret = pRootGrp->FindTask(
            iTaskId, pTaskToKa );

        if( ERROR( ret ) )
            break;

        if( pTaskToKa.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        CIfIoReqTask* pReqTask = pTaskToKa;
        if( pReqTask == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        // send a keepalive event
        EnumEventId iEvent = ( EnumEventId )
            ( eventTryLock | eventKeepAlive );

        pTaskToKa->OnEvent( iEvent,
            KATerm, 0, nullptr );

    }while( 0 );

    return ret;
}

gint32 CFastRpcSkelSvrBase::SendResponse(
    IEventSink* pInvTask,
    IConfigDb* pReq,
    CfgPtr& pRespData )
{
    // called to send back the response
    if( pReq == nullptr ||
        pRespData.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CReqOpener oReq( pReq );
        gint32 iReqType = 0;
        ret = oReq.GetReqType(
            ( guint32& )iReqType ); 

        if( ERROR( ret ) )
            break;

        if( iReqType !=
            DBUS_MESSAGE_TYPE_METHOD_CALL )
        {
            ret = -EINVAL;
            break;
        }

        CCfgOpener oOptions;
        oOptions[ propCallFlags ] = ( guint32 )
            ( DBUS_MESSAGE_TYPE_METHOD_RETURN |
            CF_NON_DBUS );

        CParamList oParams(
            ( IConfigDb* )pRespData );

        gint32 iRet = 0;
        ret = oParams.GetIntProp(
            propReturnValue, ( guint32& )iRet );
        if( ERROR( ret ) )
            break;

        ObjPtr pObj =
            ( IConfigDb* )oOptions.GetCfg();
        oParams[ propCallOptions ] = pObj;

        CFastRpcServerBase* pSvr = GetParentIf();
        oParams[ propSeqNo ] = pSvr->NewSeqNo();
        ret = oParams.CopyProp(
            propSeqNo2, propSeqNo, pReq );
        if( ERROR( ret ) )
            break;

        CIoManager* pMgr = GetIoMgr();
        IrpPtr pIrp;

        ret = pIrp.NewObj();
        if( ERROR( ret ) )
            break;

        ret = pIrp->AllocNextStack( nullptr );
        if( ERROR( ret ) )
            break;

        IrpCtxPtr& pIrpCtx = pIrp->GetTopStack(); 

        pIrpCtx->SetMajorCmd( IRP_MJ_FUNC );
        pIrpCtx->SetMinorCmd( IRP_MN_IOCTL );
        pIrpCtx->SetCtrlCode( CTRLCODE_SEND_RESP );
        pIrpCtx->SetIoDirection( IRP_DIR_OUT ); 

        BufPtr pBuf( true );
        *pBuf = pRespData;
        pIrpCtx->SetReqData( pBuf );

        TaskletPtr pTask;
        ret = pTask.NewObj( clsid( CIfDummyTask ) );

        if( SUCCEEDED( ret ) )
        {
            EventPtr pEvent;
            pEvent = pTask;
            // NOTE: we have fed a dummy task here
            pIrp->SetCallback( pEvent, 0 );
        }

        IncCounter( propMsgRespCount );
        pIrp->SetTimer(
            IFSTATE_ENABLE_EVENT_TIMEOUT, pMgr );

        ret = pMgr->SubmitIrp(
            GetPortHandle(), pIrp );

        if( ret == STATUS_PENDING )
            break;

        if( ret == -EAGAIN )
            ret = STATUS_MORE_PROCESS_NEEDED;

        pIrp->RemoveTimer();
        pIrp->RemoveCallback();

    }while( 0 );

    return ret;
}

gint32 CFastRpcSkelSvrBase::BuildBufForIrp(
    BufPtr& pBuf, IConfigDb* pReqCall )
{
    return BuildBufForIrpInternal( pBuf, pReqCall );
}

gint32 CFastRpcSkelSvrBase::OnPreStop(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        StopQpsTask();
        auto pMgr = this->GetIoMgr();
        guint32 dwTag = 0;
        if( !m_pLoop.IsEmpty() &&
            m_dwLoopTag != 0 )
        {
            CLoopPools& oPools =
                pMgr->GetLoopPools();
            oPools.ReleaseMainLoop(
                m_dwLoopTag, m_pLoop );
        }
        m_pLoop.Clear();
        m_dwLoopTag = 0;

        HANDLE hstm = INVALID_HANDLE;
        CCfgOpenerObj oIfCfg( this );
        ret = oIfCfg.GetIntPtr(
            propStmHandle,
            ( guint32*& )hstm );
        if( SUCCEEDED( ret ) )
        {
            CFastRpcServerBase* pParent =
                GetParentIf();
            pParent->RemoveStmSkel( hstm );
        }

    }while( 0 );
    return ret;
}

gint32 CFastRpcSkelSvrBase::QueueStartTask(
    TaskletPtr& pTask, MatchPtr pMatch )
{
    gint32 ret = 0;
    bool bNoEnt = true;
    do{
        for( auto& e : m_queStartTasks )
        {
            if( e.second == pMatch->GetObjId() )
            {
                bNoEnt = false;
                break;
            }
        }
        if( bNoEnt )
            m_queStartTasks.push_back(
                { pTask, pMatch->GetObjId() } );
        else
            ret = -EEXIST;
    }while( 0 );
    return ret;
}

gint32 CFastRpcProxyBase::OnPreStartComplete(
    IEventSink* pCallback,
    IEventSink* pIoReq,
    IConfigDb* pReqCtx )
{
    if( pCallback == nullptr ||
        pReqCtx == nullptr ||
        pIoReq == nullptr )
        return -EINVAL;
    gint32 ret = 0;
    do{
        CCfgOpenerObj oCfg( pIoReq );
        IConfigDb* pResp;
        ret = oCfg.GetPointer(
            propRespPtr, pResp );
        if( ERROR( ret ) )
            break;

        CCfgOpener oResp( pResp );
        gint32 iRet;
        ret = oResp.GetIntProp(
            propReturnValue, ( guint32& )iRet );
        if( ERROR( ret ) )
            break;

        ret = iRet;
        if( ERROR( ret ) )
            break;

        guint32 dwBusId = 0;
        CCfgOpener oCtx( pReqCtx );
        ret = oCtx.GetIntProp(
            propBusId, dwBusId );
        if( ERROR( ret ) )
            break;

        SetBusId( dwBusId );

        CCfgOpenerObj oIfCfg( this );
        stdstr strName;
        ret = oIfCfg.GetStrProp(
            propObjName, strName );
        if( ERROR( ret ) )
            break;
        
        ObjPtr pDrv;
        auto oDrvMgr = GetIoMgr()->GetDrvMgr();
        ret = oDrvMgr.GetDriver( true,
            DBUS_STREAM_BUS_DRIVER, pDrv );
        if( ERROR( ret ) )
            break;

        CDBusStreamBusDrv* pdrv = pDrv;
        if( pdrv == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        pdrv->BindNameBus(
            strName + "_p" , dwBusId );

    }while( 0 );

    if( ERROR( ret ) )
    {
        // with state set to stateStartFailed,
        // further start will not happen even if the
        // error occurs only in this callback, while
        // the parent taskgroup is reported success.
        this->SetStateOnEvent(
            eventPortStartFailed );
    }

    pCallback->OnEvent( eventTaskComp, ret, 0, 0 );
    return 0;
}

gint32 CFastRpcProxyBase::OnPreStart(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        ObjPtr pDrv;
        auto oDrvMgr = GetIoMgr()->GetDrvMgr();
        ret = oDrvMgr.GetDriver( true,
            DBUS_STREAM_BUS_DRIVER, pDrv );
        if( ERROR( ret ) )
            break;

        CDBusStreamBusDrv* pdrv = pDrv;
        if( pdrv == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        IConfigDb* pCtx = nullptr;
        CCfgOpenerObj oIfCfg( this );
        ret = oIfCfg.GetPointer(
            propSkelCtx, pCtx );
        if( ERROR( ret ) )
            break;

        TaskletPtr pRespTask;
        ret = NEW_PROXY_RESP_HANDLER2(
            pRespTask, this,
            &CFastRpcProxyBase::OnPreStartComplete,
            pCallback, pCtx );
        if( ERROR( ret ) )
            break;

        PortPtr pBusPort;
        CfgPtr ctxPtr( pCtx );

        ret = pdrv->Probe2( nullptr,
            pBusPort, ctxPtr, pRespTask );
        if( ERROR( ret ) )
        {
            ( *pRespTask )( eventCancelTask );
            break;
        }

        if( ret == STATUS_PENDING )
            break;

        guint32 dwBusId = 0;
        CCfgOpener oCtx( pCtx );
        ret = oCtx.GetIntProp(
            propBusId, dwBusId );
        if( ERROR( ret ) )
            break;

        SetBusId( dwBusId );

        stdstr strName;
        ret = oIfCfg.GetStrProp(
            propObjName, strName );
        if( ERROR( ret ) )
            break;

        pdrv->BindNameBus(
            strName + "_p", dwBusId );

    }while( 0 );

    if( ERROR( ret ) )
    {
        // start failed.
        this->SetStateOnEvent(
            eventPortStartFailed );
    }

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

        ObjPtr pDrv;
        auto oDrvMgr = GetIoMgr()->GetDrvMgr();
        ret = oDrvMgr.GetDriver( true,
            DBUS_STREAM_BUS_DRIVER, pDrv );
        if( ERROR( ret ) )
            break;

        CDBusStreamBusDrv* pdrv = pDrv;
        if( pdrv == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        PortPtr pPort;
        ret = pdrv->GetPortById(
            GetBusId(), pPort );
        if( ERROR( ret ) )
            break;

        Variant oVar;
        ret = pPort->GetProperty(
            propFetchTimeout, oVar );
        if( SUCCEEDED( ret ) )
        {
            pIf->SetProperty(
                propFetchTimeout, oVar );
        }
        m_pSkel = pIf;
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
        if( m_pSkel.IsEmpty() )
            break;

        ret = m_pSkel->StopEx(
            pCallback );

    }while( 0 );

    return ret;
}

gint32 CFastRpcProxyBase::OnPostStop(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        m_pSkelObj = nullptr;
        m_pSkel.Clear();
        ObjPtr pDrv;
        auto oDrvMgr = GetIoMgr()->GetDrvMgr();
        ret = oDrvMgr.GetDriver( true,
            DBUS_STREAM_BUS_DRIVER, pDrv );
        if( ERROR( ret ) )
            break;

        CDBusStreamBusDrv* pdrv = pDrv;
        if( pdrv == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        PortPtr pPort;
        ret = pdrv->GetPortById(
            GetBusId(), pPort );
        if( ERROR( ret ) )
            break;

        CDBusStreamBusPort* pBus = pPort;
        guint32 dwCount = pBus->GetChildCount();
        if( dwCount > 0 )
            break;

        CCfgOpenerObj oIfCfg( this );
        stdstr strName;
        ret = oIfCfg.GetStrProp(
            propObjName, strName );
        if( ERROR( ret ) )
            break;

        ret = pdrv->RemoveBinding( strName  + "_p" );
        if( ERROR( ret ) )
        {
            // someone else has removed the binding
            break;
        }

        ret = pdrv->DestroyPortSynced( 
            pBus, pCallback );

    }while( 0 );

    return ret;
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

        PortPtr pPort;
        ret = GetIoMgr()->GetPortPtr(
            hPort, pPort );
        if( ERROR( ret ) )
            break;

        Variant oVar;
        guint32 dwBusId = 0;
        pPort->GetProperty( propBusId, oVar );
        dwBusId = oVar;
        if( dwBusId != this->GetBusId() )
            break;

        CRpcServices* pSvc = GetStmSkel();
        if( pSvc == nullptr ||
            pSvc->GetPortHandle() != hPort )
            break;

        // pSvc->SetStateOnEvent( cmdShutdown );
        // stop at this point could result in segment
        // fault. ClosePort is good.
        // pSvc->ClosePort( nullptr );
        this->QueueStopTask( this, nullptr );

    }while( 0 );

    return ret;
}

InterfPtr CFastRpcProxyBase::GetStmSkel() const
{ return InterfPtr( m_pSkelObj ); }

gint32 CFastRpcServerBase::SetLimit(
    guint32 dwMaxRunning,
    guint32 dwMaxPending,
    bool bNoResched )
{
    std::vector< InterfPtr > vecSkels;
    gint32 ret = EnumStmSkels( vecSkels );
    if( ERROR( ret ) )
        return ret;
    for( auto e : vecSkels )
    {
        CFastRpcSkelSvrBase* pSkel = e;
        pSkel->SetLimit( dwMaxRunning,
            dwMaxPending, bNoResched );
    }
    return STATUS_SUCCESS;
}

TaskletPtr CFastRpcServerBase::GetUpdateTokenTask()
{
    TaskletPtr pTask;

    Variant oVar, oQpsPol;
    gint32 ret = this->GetProperty(
        propEnableQps, oVar );
    if( ERROR( ret ) )
        return pTask;

    bool bQps = oVar;
    ret = this->GetProperty(
        propQpsPolicy, oQpsPol );
    if( ERROR( ret ) ||
        ( ( stdstr& )oQpsPol ) !=
            JSON_VALUE_QPOL_EQUAL )
        return pTask;

    gint32 (*func)( CFastRpcServerBase*) =
    ([]( CFastRpcServerBase* pIf )->gint32
    {
        guint32 dwCount = pIf->GetStmSkelCount();
        if( dwCount == 0 )
            dwCount = 1;
        guint64 qwMaxTokens = 0;
        pIf->GetMaxTokens( qwMaxTokens );
        if( qwMaxTokens == 0 )
            return STATUS_PENDING;
        guint64 qwPerSkelToken = qwMaxTokens / dwCount;
        if( qwPerSkelToken == 0 )
            qwPerSkelToken = 1;
        pIf->SetPerSkelTokens( qwPerSkelToken );
        // to keep the timer alive
        pIf->AllocReqToken();
        // DebugPrint( 0, "Current skel count %d, "
        //     "tokens per skel %lld",
        //     dwCount, qwPerSkelToken );
        return STATUS_PENDING;
    });

    NEW_FUNCCALL_TASKEX( pTask,
        this->GetIoMgr(), func, this );
    return pTask;
}

gint32 CFastRpcServerBase::GetStmChanSvr(
    InterfPtr& pIf ) const
{
    gint32 ret = 0;
    do{
        ObjPtr pDrv;
        auto oDrvMgr =
            GetIoMgr()->GetDrvMgr();
        ret = oDrvMgr.GetDriver( true,
            DBUS_STREAM_BUS_DRIVER, pDrv );
        if( ERROR( ret ) )
            break;

        CDBusStreamBusDrv* pdrv = pDrv;
        if( pdrv == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        PortPtr pPort;
        ret = pdrv->GetPortById(
            GetBusId(), pPort );
        if( ERROR( ret ) )
            break;
        CDBusStreamBusPort* pBus = pPort;
        pIf = pBus->GetStreamIf();
        if( pIf.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }
    }while( 0 );
    return ret;
}
gint32 CFastRpcServerBase::GetProperty(
    gint32 iProp, Variant& oVal ) const
{
    gint32 ret = 0;
    switch( iProp )
    {
    case propUptime:
        {
            timespec ts = this->GetStartTime();
            timespec ts2;
            clock_gettime( CLOCK_REALTIME, &ts2 );
            oVal = ( guint32 )
                ( ts2.tv_sec - ts.tv_sec );
            break;
        }
    case propPendingTasks:
        {
            gint32 ret = 0;
            std::vector< InterfPtr > vecIfs;
            ret = EnumStmSkels( vecIfs );
            if( ERROR( ret ) )
            {
                oVal = ( guint32 )0;
                ret = 0;
                break;
            }
            guint32 dwTasks = 0;
            for( auto& elem : vecIfs )
            {
                CFastRpcSkelSvrBase* pSkel = elem;
                TaskGrpPtr pGrp = pSkel->GetGrpRfc();
                if( pGrp.IsEmpty() )
                    continue;

                CIfParallelTaskGrpRfc* pGrpRfc = pGrp;
                dwTasks += pGrpRfc->GetTaskCount();
            }
            oVal = dwTasks;
            break;
        }
    case propConnections:
        {
            oVal = GetStmSkelCount();
            break;
        }
    case propStmPerSess:
        {
            InterfPtr pChan;
            ret = GetStmChanSvr( pChan );
            if( ERROR( ret ) )
                break;
            Variant oVar;
            ret = pChan->GetProperty(
                propStmPerSess, oVar );
            if( ERROR( ret ) )
                oVal = MAX_REQCHAN_PER_SESS;
            ret = 0;
            break;
        }
    case propRxBytes:
    case propTxBytes:
        {
            auto pSvc = const_cast
                < CFastRpcServerBase* >( this );
            auto psc = dynamic_cast
                < CStatCounters_SvrBase*>( pSvc );
            if( !psc )
            {
                ret = -EFAULT;
                break;
            }
            guint64 qwStmBytes = 0;
            psc->GetCounter2( iProp, qwStmBytes );
            guint64 qwCmdBytes = 0;

            do{
                InterfPtr pChan;
                ret = GetStmChanSvr( pChan );
                if( ERROR( ret ) )
                    break;
                CStatCountersServer* psc = pChan;
                if( psc )
                {
                    psc->GetCounter2(
                        iProp, qwCmdBytes );
                }
            }while( 0 );
            oVal = qwStmBytes + qwCmdBytes;
            ret = 0;
            break;
        }
    case propFailureCount:
    case propMsgRespCount:
    case propMsgCount:
        {
            auto pSvc = const_cast
                < CFastRpcServerBase* >( this );
            auto psc = dynamic_cast
                < CStatCounters_SvrBase*>( pSvc );
            if( !psc )
            {
                ret = -EFAULT;
                break;
            }
            guint32 dwVal = 0;
            ret = psc->GetCounter2( iProp, dwVal );
            if( SUCCEEDED( ret ) )
                oVal = dwVal;
            break;
        }
    case propObjCount:
        oVal = CObjBase::GetActCount();
        break;
    case propQps:
        {
            Variant bEnable;
            ret = super::GetProperty(
                propEnableQps, bEnable );
            if( ERROR( ret ) || !(bool& )bEnable )
            {
                oVal = ( guint32 )0;
                ret = 0;
            }
            else
            {
                ret = super::GetProperty(
                    propQps, oVal );
            }
            break;
        }
    default:
        ret = -ENOENT;
        break;
    }
    return ret;
}

gint32 CFastRpcServerBase::SetProperty(
    gint32 iProp, const Variant& oVal )
{
    gint32 ret = 0;
    switch( iProp )
    {
    case propRxBytes:
    case propTxBytes:
    case propFailureCount:
    case propMsgRespCount:
    case propObjCount:
    case propMsgCount:
        break;
    case propStmPerSess:
        {
            InterfPtr pChan;
            ret = GetStmChanSvr( pChan );
            if( ERROR( ret ) )
                break;
            auto dwCount = ( guint32& )oVal;
            if( MAX_REQCHAN_PER_SESS > dwCount )
                dwCount = MAX_REQCHAN_PER_SESS;
            Variant oVar( dwCount );
            pChan->SetProperty(
                propStmPerSess, oVar );
            ret = 0;
            break;
        }
    case propQps:
        {
            Variant bEnable;
            ret = super::GetProperty(
                propEnableQps, bEnable );
            if( ERROR( ret ) ||
                !(bool& )bEnable )
            {
                // don't try further
                ret = 0;
                break;
            }
            super::SetProperty( propQps, oVal );
            SetMaxTokens( ( guint64& )oVal );
            break;
        }
    default:
        ret = -ENOENT;
        break;
    }
    return ret;
}

gint32 CFastRpcServerBase::SetMaxTokens(
    guint64 qwMaxTokens )
{
    gint32 ret = 0;
    do{
        std::vector< InterfPtr > vecIfs;
        ret = EnumStmSkels( vecIfs );
        if( ERROR( ret ) )
            break;

        guint64 qwPerSkelToken =
            qwMaxTokens / vecIfs.size();
        if( qwPerSkelToken == 0 )
            qwPerSkelToken = 1;

        this->SetPerSkelTokens( qwPerSkelToken );
        for( auto& elem : vecIfs )
        {
            CFastRpcSkelSvrBase* pSkel = elem;
            pSkel->SetMaxTokens( qwPerSkelToken );
        }

    }while( 0 );
    return ret;
}

gint32 CFastRpcSkelServerState::SetupOpenPortParams(
    IConfigDb* pParams )
{
    gint32 ret = 0;
    do{
        ret = super::SetupOpenPortParams(
            pParams );
        if( ERROR( ret ) )
            break;

        CCfgOpenerObj oIfCfg( this );
        IConfigDb* pCtx = nullptr;
        ret = oIfCfg.GetPointer(
            propSkelCtx, pCtx );
        if( ERROR( ret ) )
            break;

        CCfgOpener oCtx( pCtx );
        guint32 dwBusId = 0;
        ret = oCtx.GetIntProp(
            propBusId, dwBusId );
        if( ERROR( ret ) )
            break;

        CCfgOpener oParams( pParams );
        stdstr strName;
        ret = oParams.GetStrProp(
            propBusName, strName );
        if( ERROR( ret ) )
            break;
        strName.push_back( '_' );
        strName += std::to_string( dwBusId );

        oParams.SetStrProp(
            propBusName, strName );

    }while( 0 );

    return ret;
}

gint32 CFastRpcSkelProxyState::SetupOpenPortParams(
    IConfigDb* pParams )
{
    gint32 ret = 0;
    do{
        ret = super::SetupOpenPortParams(
            pParams );
        if( ERROR( ret ) )
            break;

        CCfgOpenerObj oIfCfg( this );
        IConfigDb* pCtx = nullptr;
        ret = oIfCfg.GetPointer(
            propSkelCtx, pCtx );
        if( ERROR( ret ) )
            break;

        CCfgOpener oCtx( pCtx );
        guint32 dwBusId = 0;
        ret = oCtx.GetIntProp(
            propBusId, dwBusId );
        if( ERROR( ret ) )
            break;

        CCfgOpener oParams( pParams );
        stdstr strName;
        ret = oParams.GetStrProp(
            propBusName, strName );
        if( ERROR( ret ) )
            break;
        strName.push_back( '_' );
        strName += std::to_string( dwBusId );

        oParams.SetStrProp(
            propBusName, strName );

        oParams.CopyProp( propOpenPortTimeout,
            propFetchTimeout, this );

    }while( 0 );

    return ret;
}

gint32 CStatCounters_SvrBase::GetCounter2(
    guint32 iPropId, guint32& dwVal )
{
    gint32 ret = 0;
    do{
        dwVal = 0;
        CFastRpcServerBase* pSvr =
            ObjPtr( this );

        super::GetCounter2( iPropId, dwVal );

        std::vector< InterfPtr > vecIfs;
        ret = pSvr->EnumStmSkels( vecIfs );
        if( ERROR( ret ) )
        {
            ret = STATUS_SUCCESS;
            break;
        }

        for( auto& elem : vecIfs )
        {
            CStatCountersServerSkel* pSvc = elem;
            guint32 dwVal0 = 0;
            ret = pSvc->GetCounter2(
                iPropId, dwVal0 );
            if( SUCCEEDED( ret ) )
                dwVal += dwVal0;
        }
        ret = STATUS_SUCCESS;

    }while( 0 );
    return ret;
}

gint32 CStatCounters_SvrBase::GetCounter2(
    guint32 iPropId, guint64& qwVal )
{
    gint32 ret = 0;
    do{
        qwVal = 0;
        CFastRpcServerBase* pSvr =
            ObjPtr( this );

        super::GetCounter2( iPropId, qwVal );

        std::vector< InterfPtr > vecIfs;
        ret = pSvr->EnumStmSkels( vecIfs );
        if( ERROR( ret ) )
        {
            ret = STATUS_SUCCESS;
            break;
        }

        for( auto& elem : vecIfs )
        {
            CStatCountersServerSkel* pSvc = elem;
            guint32 qwVal0 = 0;
            ret = pSvc->GetCounter2(
                iPropId, qwVal0 );
            if( SUCCEEDED( ret ) )
                qwVal += qwVal0;
        }
        ret = STATUS_SUCCESS;

    }while( 0 );
    return ret;
}

gint32 CStatCounters_CliBase::GetCounter2(
    guint32 iPropId, guint32& dwVal )
{
    gint32 ret = 0;
    do{
        CFastRpcProxyBase* pProxy =
            ObjPtr( this );

        CStatCountersProxySkel* pSkel =
            pProxy->GetStmSkel();
        if( pSkel == nullptr )
        {
            ret = STATUS_SUCCESS;
            break;
        }

        ret = pSkel->GetCounter2(
            iPropId, dwVal );

    }while( 0 );
    return ret;
}

gint32 CStatCounters_CliBase::GetCounter2(
    guint32 iPropId, guint64& qwVal )
{
    gint32 ret = 0;
    do{
        CFastRpcProxyBase* pProxy =
            ObjPtr( this );

        CStatCountersProxySkel* pSkel =
            pProxy->GetStmSkel();
        if( pSkel == nullptr )
        {
            ret = STATUS_SUCCESS;
            break;
        }

        ret = pSkel->GetCounter2(
            iPropId, qwVal );

    }while( 0 );
    return ret;
}

 
gint32 CFastRpcProxyState::OpenPort(
    IEventSink* pCallback )
{
    // let's prepare the parameters
    gint32 ret = OpenPortInternal( pCallback );
    if( ret == STATUS_PENDING || ERROR( ret ) )
        return ret;
    do{

        PortPtr pPort;
        ret = GetIoMgr()->GetPortPtr(
            GetHandle(), pPort );

        if( ERROR( ret ) )
            break;
        
        if( pPort->GetClsid() ==
            clsid( CDBusLocalPdo ) )
            break;

        CPort* pcp = pPort;
        PortPtr pPdo;
        ret = pcp->GetPdoPort( pPdo );
        if( ERROR( ret ) )
            break;

        ret = CopyProp( propConnHandle,
            ( CObjBase*)pPdo );
        if( ERROR( ret ) )
            break;

        ret = CopyProp( propConnParams,
            ( CObjBase*)pPdo );
        if( ERROR( ret ) )
            break;

    }while( 0 );
    if( ERROR( ret ) )
        OutputMsg( ret, "Error OpenPort from "
            "CFastRpcProxyState" );
    return ret;
}

}
