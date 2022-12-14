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

gint32 CFastRpcServerBase::EnumStmSkels(
    std::vector< InterfPtr >& vecIfs )
{
    CStdRMutex oLock( GetLock() );
    if( m_mapSkelObjs.empty() )
        return -ENOENT;

    for( auto& elem : m_mapSkelObjs )
        vecIfs.push_back( elem.second );

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
        pdrv->BindNameBus( strName, dwBusId );

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

    }while( 0 );

    return ret;
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

        pdrv->RemoveBinding( strName );

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
        pdrv->BindNameBus( strName, dwBusId );

    }while( 0 );

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

        ret = pdrv->RemoveBinding( strName );
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

        CRpcServices* pSvc = m_pSkelObj;
        pSvc->SetStateOnEvent( cmdShutdown );
        // stop at this point could result in segment
        // fault. ClosePort is good.
        pSvc->ClosePort( nullptr );
        
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

    }while( 0 );

    return ret;
}

}
