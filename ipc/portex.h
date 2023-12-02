/*
 * =====================================================================================
 *
 *       Filename:  portex.h
 *
 *    Description:  shared extension of the CGenericBusPort and CGenBusDriver
 *
 *        Version:  1.0
 *        Created:  09/30/2020 08:30:10 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi(woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2019 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */

#pragma once
#include "defines.h"
#include "frmwrk.h"
#include "portdrv.h"
#include "port.h"

#include "emaphelp.h"
#include "iftasks.h"
#include "ifhelper.h"

namespace rpcf
{

template< class T,
    class T2 =typename std::enable_if<
        std::is_base_of< CPort, T >::value,
        T >::type,
    class T3 =typename std::enable_if<
        !std::is_base_of< CRpcServices, T >::value,
        T >::type >
gint32 AddSeqTaskTempl( T2* pObj,
    TaskGrpPtr& pQueuedTasks,
    TaskletPtr& pTask,
    bool bLong )
{
    if( pTask.IsEmpty() || pObj == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        bool bNew = false;
        TaskGrpPtr ptrSeqTasks;
        CIoManager* pMgr = pObj->GetIoMgr();

        CStdRMutex oPortLock( pObj->GetLock() );
        if( pQueuedTasks.IsEmpty() )
        {
            CParamList oParams;
            oParams[ propIoMgr ] = ObjPtr( pMgr );

            ret = pQueuedTasks.NewObj(
                clsid( CIfTaskGroup ),
                oParams.GetCfg() );

            if( ERROR( ret ) )
                break;

            pQueuedTasks->SetRelation(logicNONE );
            bNew = true;
        }
        ptrSeqTasks = pQueuedTasks;
        oPortLock.Unlock();

        CStdRTMutex oQueueLock(
            ptrSeqTasks->GetLock() );

        oPortLock.Lock();
        guint32 dwPortState =
            pObj->GetPortState();
        if( dwPortState == PORT_STATE_STOPPED ||
            dwPortState == PORT_STATE_REMOVED )
        {
            ret = ERROR_STATE;
            break;
        }
            
        if( pQueuedTasks != ptrSeqTasks )
            continue;

        ret = pQueuedTasks->AppendTask( pTask );
        if( ERROR( ret ) )
        {
            // the old seqTask is completed
            pQueuedTasks.Clear();
            continue;
        }

        pTask->MarkPending();
        if( SUCCEEDED( ret ) && bNew )
        {
            // a new pQueuedTasks, add and run
            oPortLock.Unlock();
            oQueueLock.Unlock();

            TaskletPtr pSeqTasks = pQueuedTasks;
            ret = pMgr->RescheduleTask(
                pSeqTasks, bLong );

            break;
        }
        if( SUCCEEDED( ret ) && !bNew )
        {
            // pQueuedTasks is already running
            // just waiting for the turn.
        }
        break;

    }while( 1 );
    return ret;
}

#define AddSeqTaskPort AddSeqTaskTempl< CPort, CPort >

template< class T >
class CStartStopSafeBusPort :
    public T
{
    public:
    typedef T super;

    CStartStopSafeBusPort( const IConfigDb* pCfg ) :
        super( pCfg )
    {}

    gint32 GetStartStopTaskGrp(
        IPort* pChildPort,
        TaskGrpPtr*& ppTaskgrp )
    {
        if( pChildPort == nullptr )
            return -EINVAL;

        gint32 ret = 0;
        do{
            CStdRMutex oLock( this->GetLock() ); 
            PortPtr pPort( pChildPort );
            std::map< PortPtr, TaskletPtr >::iterator
                itr = this->m_mapPort2TaskGrp.find(
                    PortPtr( pPort ) );
            if( itr == this->m_mapPort2TaskGrp.end() )
            {
                ret = -ENOENT;
                break;
            }

            // BUGBUG
            ppTaskgrp = reinterpret_cast
                < TaskGrpPtr* >( &itr->second );

        }while( 0 );

        return ret;
    }

    gint32 AddStartStopPortTask(
        IPort* pPort, TaskletPtr& pStopTask )
    {
        gint32 ret = 0;
        TaskGrpPtr* ppTaskGrp = nullptr;
        ret = GetStartStopTaskGrp(
            pPort, ppTaskGrp );

        if( ERROR( ret ) )
            return ret;

        ret = AddSeqTaskTempl( this,
            *ppTaskGrp, pStopTask, false );
        if( SUCCEEDED( ret ) )
            ret = STATUS_PENDING;

        return ret;
    }

    gint32 SendAttachedEvent(
        IPort* pNewPort, IEventSink* pEventSink )
    {
        if( pNewPort == nullptr ||
            pEventSink == nullptr )
            return -EINVAL;

        CCfgOpenerObj oPortCfg( pNewPort );
        oPortCfg.SetPointer(
            propEventSink, pEventSink );

        // the callback's OnIrpComplete will be called
        // when the port is started or failed
        CEventMapHelper< T > a( this );
        a.BroadcastEvent(
            eventPnp, eventPortAttached,
            PortToHandle( pNewPort ),
            nullptr );

        guint32 dwRet = 0;
        gint32 ret = oPortCfg.GetIntProp(
            propReturnValue, dwRet );

        if( ERROR( ret ) )
            return STATUS_PENDING;

        return dwRet;
    }

    gint32 OnChildPortStarted(
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
            CCfgOpenerObj oReq( pIoReq );
            IConfigDb* pResp;

            IrpPtr pIrpResp;
            ret = oReq.GetPointer(
                propRespPtr, pResp );
            if( SUCCEEDED( ret ) )
            {
                CCfgOpener oResp( pResp );
                ObjPtr pObj;
                ret = oResp.GetObjPtr(
                    propIrpPtr, pObj );
                if( SUCCEEDED( ret ) )
                    pIrpResp = pObj;
            }

            if( pIrpResp.IsEmpty() )
            {
                ret = pIrpResp.NewObj(
                    clsid( IoRequestPacket ) );
                if( ERROR( ret ) )
                    break;

                pIrpResp->AllocNextStack( nullptr );

                IrpCtxPtr& pCtx =
                    pIrpResp->GetTopStack();

                pCtx->SetStatus( ERROR_FAIL );
                pIrpResp->SetStatus( ERROR_FAIL );
                pIrpResp->SetState( IRP_STATE_READY,
                    IRP_STATE_COMPLETED );
            }

            PIRP pIrp = pIrpResp;

            // notify the CIfOpenPortTask
            pCallback->OnEvent( eventIrpComp,
                ( LONGWORD )( ( IRP* )pIrp ),
                0, nullptr );

            // complete the caller task and release
            // the start/stop seq queue
            CParamList oCtx( pReqCtx );
            IEventSink* pSeqTask = nullptr;
            ret = oCtx.GetPointer( 0, pSeqTask );
            if( SUCCEEDED( ret ) )
            {
                IEventSink* pCaller = nullptr;
                LONGWORD* pData = nullptr;
                ret = oCtx.GetPointer( 1, pCaller );
                if( SUCCEEDED( ret ) )
                {
                    CObjBase* pObjBase = pCaller;
                    pData = ( LONGWORD* ) pObjBase;
                }
                pSeqTask->OnEvent( eventTaskComp,
                   pIrp->GetStatus(), 0, pData ); 
                oCtx.ClearParams();
            }

        }while( 0 );

        return ret;
    }

    gint32 NotifyPortAttached(
        IPort* pNewPort )
    {
        gint32 ret = 0;
        do{
            TaskletPtr pCallback;
            TaskletPtr pTask;

            IEventSink* pIrpCb = nullptr;
            CCfgOpenerObj oPortCfg( pNewPort );
            ret = oPortCfg.GetPointer(
                propEventSink, pIrpCb );

            if( SUCCEEDED( ret ) )
            {
                CParamList oCtx;

                ret = NEW_OBJ_RESP_HANDLER2(
                    pCallback, this,
                    &CStartStopSafeBusPort::OnChildPortStarted,
                    pIrpCb,
                    ( IConfigDb* )oCtx.GetCfg() );

                if( ERROR( ret ) )
                    break;

                ret = DEFER_OBJCALL_NOSCHED(
                    pTask, this,
                    &CStartStopSafeBusPort::SendAttachedEvent,
                    pNewPort, pCallback );

                if( ERROR( ret ) )
                    break;

                oCtx.Push( ObjPtr( pTask ) );
                oCtx.Push( ObjPtr( pCallback ) );
            }
            else
            {
                ret = DEFER_OBJCALL_NOSCHED2(
                    1, pTask, this,
                    &CStartStopSafeBusPort::SendAttachedEvent,
                    pNewPort, nullptr );

                if( ERROR( ret ) )
                    break;
            }

            ret = AddStartStopPortTask(
                pNewPort, pTask );

            if( ERROR( ret ) )
            {
                if( !pTask.IsEmpty() )
                    ( *pTask )( eventCancelTask );
                if( !pCallback.IsEmpty() )
                    ( *pCallback )( eventCancelTask );
            }

        }while( 0 );

        return ret;
    }

    gint32 DestroyPortSynced(
        IPort* pPort, IEventSink* pCb )
    {
        if( pPort == nullptr )
        return -EINVAL; 

        CIoManager* pMgr = this->GetIoMgr();
        CPnpManager* pPnpMgr = &pMgr->GetPnpMgr();

        gint32 ret = 0;

        TaskletPtr pCallback;
        if( pCb == nullptr )
        {
            pCallback.NewObj( clsid( CSyncCallback ) );
        }
        else
        {
            pCallback = ObjPtr( pCb );
        }

        do{
            TaskletPtr pStopTask;
            ret = DEFER_OBJCALL_NOSCHED2(
                1, pStopTask, pPnpMgr,
                &CPnpManager::DestroyPortStack,
                pPort, nullptr );

            CIfRetryTask* pRetryTask = pStopTask;
            pRetryTask->SetClientNotify( pCallback );
            // callback must be called all the way
            pRetryTask->MarkPending();

            ret = AddStartStopPortTask(
                pPort, pStopTask );

            if( ERROR( ret ) && pCb != nullptr )
            {
                // NOTE: we don't know if the
                // callback need to be completed
                // at this point, so defer the
                // eventCancelTask, otherwise, a
                // deadlock could happen when the
                // callback was reentered on the
                // same thread.
                ret = DEFER_CALL(
                    pMgr, ObjPtr( pRetryTask ),
                    &IEventSink::OnEvent,
                    eventCancelTask, 0, 0,
                    nullptr );

                if( SUCCEEDED( ret ) )
                    ret = STATUS_PENDING;
                break;
            }
            else if( ERROR( ret ) )
            {
                //pRetryTask->MarkPending( false );
                const char* szName = 
                    CoGetClassName( pPort->GetClsid() );

                DebugPrintEx( logErr, ret,
                    "Error stopping port %s@0x%llx", 
                    szName );

                ( *pRetryTask )( eventCancelTask );
                break;
            }

            if( ret == STATUS_PENDING &&
                pCb == nullptr )
            {
                CSyncCallback* pSyncCb = pCallback;
                ret = pSyncCb->WaitForComplete();
            }

        }while( 0 );

        return ret;
    }
};

class CGenericBusPortEx :
    public CStartStopSafeBusPort< CGenericBusPort >
{
    public:
    typedef CStartStopSafeBusPort< CGenericBusPort > super;

    CGenericBusPortEx( const IConfigDb* pCfg )
        : super( pCfg )
    {}
};

class CGenBusDriverEx :
    public CStartStopSafeBusPort< CGenBusDriver >
{
    public:
    typedef CStartStopSafeBusPort< CGenBusDriver > super;

    CGenBusDriverEx( const IConfigDb* pCfg )
        : super( pCfg )
    {}

    gint32 Stop();
};

}
