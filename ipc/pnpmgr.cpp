/*
 * =====================================================================================
 *
 *       Filename:  pnpmgr.cpp
 *
 *    Description:  implementation of pnp manager
 *
 *        Version:  1.0
 *        Created:  07/09/2016 01:48:12 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
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

#include <vector>
#include <string>
#include "defines.h"
#include "port.h"
#include "frmwrk.h"
#include "stlcont.h"
#include "emaphelp.h"
#include "connhelp.h"
#include "ifhelper.h"

namespace rpcf
{
using namespace std;

CPnpManager::CPnpManager( const IConfigDb* pCfg )
{
    SetClassId( clsid( CPnpManager ) );
    CCfgOpener a( pCfg );

    gint32 ret = a.GetPointer(
        propIoMgr, m_pIoMgr );

    if( ERROR( ret ) ) 
        throw std::invalid_argument(
            "the pointer to io manager is empty" );
}

void CPnpManager::OnPortAttached(
    IPort* pPort )
{
    gint32 ret = 0;

    if( pPort == nullptr )
        return;
    
    do{
        CIoManager* pMgr = GetIoMgr();
        IrpPtr pMasterIrp;

        CCfgOpenerObj oPortCfg( pPort );
        if( oPortCfg.exist( propMasterIrp ) )
        {
            ObjPtr pObj;
            oPortCfg.GetObjPtr(
                propMasterIrp, pObj );

            pMasterIrp = pObj;

            oPortCfg.RemoveProperty(
                propMasterIrp );
        }

        // Note: If the BuildPortStack failed, we keep
        // the start process moving on with the ports
        // ready, but replacing the callback with a new
        // one to perform stoping and destroying
        // instead.
        TaskletPtr pStopTask;
        ret = pMgr->GetDrvMgr().BuildPortStack( pPort );
        if( ERROR( ret ) )
        {
            IEventSink* pCb = nullptr;
            oPortCfg.GetPointer(
                propEventSink, pCb );

            gint32 (*func)( CPort*, IEventSink*, gint32 ) =
            ([]( CPort* pPort, IEventSink*
                pCb, gint32 iRet )->gint32
            {
                IrpPtr pIrp( true );
                pIrp->AllocNextStack( pPort );
                IrpCtxPtr pIrpCtx = pIrp->GetTopStack();
                pIrpCtx->SetMajorCmd( IRP_MJ_PNP );
                pIrpCtx->SetMinorCmd( IRP_MN_PNP_START );
                pIrpCtx->SetIoDirection( IRP_DIR_OUT );
                pIrp->SetState( IRP_STATE_READY,
                    IRP_STATE_COMPLETED );
                pIrp->SetStatus( iRet );
                pCb->OnEvent( eventIrpComp,
                    ( LONGWORD )( ( IRP* )pIrp ),
                    iRet, nullptr );
                return iRet;
            });

            CPort *pCPort = ObjPtr( pPort );
            TaskletPtr pCbTask; 
            NEW_FUNCCALL_TASK(
                pCbTask, pMgr, func, pCPort,
                ( IEventSink* )pCb, ret );

            if( pCb != nullptr )
                oPortCfg.RemoveProperty( propEventSink );

            TaskletPtr pWrapper;
            CCfgOpener oCfg;
            oCfg.SetPointer( propIoMgr, pMgr );

            pWrapper.NewObj( clsid( CTaskWrapper ),
                oCfg.GetCfg() );

            CTaskWrapper* ptw = pWrapper;
            ptw->SetCompleteTask( pCbTask );

            oCfg.SetObjPtr( propPortPtr,
                ObjPtr( ( IPort* )pPort ) );
                
            ret = pStopTask.NewObj(
                clsid( CPnpMgrStopPortAndDestroyTask ),
                oCfg.GetCfg() );

            if( ERROR( ret ) )
                break;

            if( pCb != nullptr )
            {
                CIfRetryTask* pTask = pStopTask;
                pTask->SetClientNotify( ptw );
            }

            pWrapper.NewObj( clsid( CTaskWrapper ),
                oCfg.GetCfg() );

            ptw = pWrapper;
            ptw->SetCompleteTask( pStopTask );
            oPortCfg.SetPointer( propEventSink, ptw );
        }

        // notify the ports the stack is built
        // a chance for port to register with registry
        IrpPtr pIrp( true );
        pIrp->AllocNextStack( nullptr );
        IrpCtxPtr& pIrpCtx = pIrp->GetTopStack(); 

        // star the port
        pIrpCtx->SetMajorCmd( IRP_MJ_PNP );
        pIrpCtx->SetMinorCmd( IRP_MN_PNP_STACK_BUILT );

        pIrpCtx->SetIoDirection( IRP_DIR_OUT );

        HANDLE hPort = PortToHandle( pPort );

        // no check of the port existance, because
        // we are sure it exists
        ret = pMgr->SubmitIrp( hPort, pIrp, false );
        if( ERROR( ret ) )
            break;

        EventPtr pEvent( this );

        // NOTE: we are already the bottom port
        // no need to get bottom port again
        CEventMapHelper< CPort >
            oEvtMap( static_cast< CPort* >( pPort ) );

        oEvtMap.SubscribeEvent( pEvent );

        // using the deferred call to prevent the
        // callback from the StartPortsInternal to
        // happen before the caller quits
        ObjPtr pPortObj( pPort );
        ObjPtr pIrpObj( ( CObjBase*) pMasterIrp );

        ret = DEFER_CALL( pMgr, this,
            &CPnpManager::StartPortStack,
            pPortObj,
            pIrpObj );

        if( SUCCEEDED( ret ) )
            ret = STATUS_PENDING;

    }while( 0 );

    if( ret != STATUS_PENDING )
    {
        CCfgOpenerObj oPortCfg( pPort );
        oPortCfg.SetIntProp(
            propReturnValue, ret );
    }

    return;
}

/**
* @name CPnpMgrStartPortCompletionTask::operator()
* @{ */
/** 
    four input parameters in the task
    propIrpPtr: pointer to the irp completed
    propIoMgr: pointer to the io manager
    propPortPtr: pointer to the port started
    propEventSink: pointer to the EventSink to send
    the event to
 * @} */

gint32 CPnpMgrStartPortCompletionTask::operator()(
    guint32 dwContext )
{
    gint32 ret = 0;
    gint32 iRet = 0;
    gint32 ret2 = 0;

    CIoManager* pMgr = nullptr;
    ObjPtr pPort;

    EnumEventId iEventId = eventPortStarted;

    do{
        CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
        ObjPtr evtSinkPtr;
        ObjPtr pObj;

        ret = oCfg.GetObjPtr( propIrpPtr, pObj );

        if( ERROR( ret ) )
            break;

        IrpPtr pIrp = pObj;

        if( pIrp.IsEmpty() 
            || pIrp->GetStackSize() == 0 )
        {
            ret = -EINVAL;
            break;
        }

        oCfg.RemoveProperty( propIrpPtr );

        ret = oCfg.GetPointer( propIoMgr, pMgr );
        if( ERROR( ret ) )
            break;

        if( pMgr == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        iRet = pIrp->GetStatus();

        ret2 = oCfg.GetObjPtr( propPortPtr, pPort );
        if( ERROR( ret2 ) )
            break;

        oCfg.RemoveProperty( propPortPtr );
        if( pPort.IsEmpty() )
        {
            ret2 = -EFAULT;
            break;
        }

        // without this sink, we cannot make the binding
        ret2 = oCfg.GetObjPtr(
            propEventSink, evtSinkPtr );

        if( ERROR( ret2 ) )
            break;

        IEventSink* pEventSink = evtSinkPtr;
        if( nullptr == pEventSink )
        {
            oCfg.RemoveProperty( propEventSink );
            ret2 = -EINVAL;
            break;
        }

        if( ERROR( iRet ) )
        {
            iEventId = eventPortStartFailed;
        }

        // notify the caller,the port is started
        // or failed

        HANDLE hPort = PortToHandle( pPort );

        BufPtr pBuf( true );
        *pBuf = hPort;

        IrpCtxPtr& pCtx = pIrp->GetTopStack();

        pCtx->SetRespData( pBuf );

        ret2 = pEventSink->OnEvent( eventIrpComp,
            ( LONGWORD )( ( IRP* )pIrp ), 0, nullptr );

        oCfg.RemoveProperty( propEventSink );

    }while( 0 );


    if( SUCCEEDED( ret ) )
    {
        CConnPointHelper oConnPoint( pMgr );

        if( ERROR( ret2 ) || ERROR( iRet ) )
        {
            iEventId = eventPortStartFailed;

            if( ERROR( iRet ) )
            {
                ret = iRet;
            }
            else if( ERROR( ret2 ) )
            {
                ret = ret2;
            }
        }

        oConnPoint.BroadcastEvent(
            propStartStopEvent,
            iEventId,
            PortToHandle( pPort ),
            ret,
            nullptr );

        if( iEventId == eventPortStartFailed )
        {
            CPnpManager& oPnpMgr =
                pMgr->GetPnpMgr();

            // destroy the port
            oPnpMgr.DestroyPortStack( pPort );
        }
    }

    return SetError( ret );
}

gint32 CPnpManager::StartPortsInternal(
    IPort* pPort, IRP* pMastIrp )
{
    if( pPort == nullptr )
        return  -EINVAL;

    gint32 ret = 0;
    IrpPtr pMasterIrp( pMastIrp );
    CIoManager* pMgr = GetIoMgr();
    if( pMgr->IsStopping() )
        ret = ERROR_STATE;
    else do{

        IrpPtr pIrp( true );
        pIrp->AllocNextStack( nullptr );
        IrpCtxPtr& pIrpCtx = pIrp->GetTopStack(); 

        // star the port
        pIrpCtx->SetMajorCmd( IRP_MJ_PNP );
        pIrpCtx->SetMinorCmd( IRP_MN_PNP_START );
        pIrpCtx->SetIoDirection( IRP_DIR_OUT );

        // we can wait 2 minutes for
        // a start operation
        guint32 dwTimeout;

        Variant oVar;
        ret = pPort->GetProperty(
            propOpenPortTimeout, oVar );
        if( SUCCEEDED( ret ) )
            dwTimeout = ( guint32& )oVar;
        else
            dwTimeout = PORT_START_TIMEOUT_SEC;

        pIrp->SetTimer( dwTimeout, pMgr );


        if( true  )
        {
            // set up the callback task
            CCfgOpenerObj oCfg( pPort );
            ObjPtr pObj;

            ret = oCfg.GetObjPtr( propEventSink, pObj );
            if( ERROR( ret ) )
            {
                // for CTcpStreamPdo, the port
                // might be opened first on the
                // connection, when the
                // propEventSink is missing
                ret = pObj.NewObj(
                    clsid( CIfDummyTask ) );
            }

            if( SUCCEEDED( ret ) )
            {
                // we don't want this eventsink to live
                // through the port live cycle
                pPort->RemoveProperty( propEventSink );

                CCfgOpener a;

                ret = a.SetObjPtr(
                    propIrpPtr, ObjPtr( ( IRP* )pIrp ) );

                if( ERROR( ret ) )
                    break;

                ret = a.SetPointer( propIoMgr, pMgr );

                if( ERROR( ret ) )
                    break;

                ret = a.SetObjPtr(
                    propEventSink, pObj );

                if( ERROR( ret ) )
                    break;

                ret = a.SetObjPtr(
                    propPortPtr, ObjPtr( pPort ) );

                if( ERROR( ret ) )
                    break;

                TaskletPtr pTask;

                pTask.NewObj(
                    clsid( CPnpMgrStartPortCompletionTask ),
                    a.GetCfg() );

                pIrp->SetCallback(
                    EventPtr( ( CTasklet* )pTask ), 0 );

                pIrp->SetSyncCall( false );
            }
        }

        ret = 0;
        while( !pMasterIrp.IsEmpty() )
        {
            // FIXME: not a serious handling of pMasterIrp
            // for master irp, we don't need a callback
            // though it is an asynchronous call
            CStdRMutex oIrpLock( pMasterIrp->GetLock() );
            ret = pMasterIrp->CanContinue( IRP_STATE_READY );
            if( ERROR( ret ) )
                break;

            IPort* pMasterPort = pMasterIrp->GetTopPort();
            CPort* pCPort = static_cast< CPort* >( pMasterPort );
            ret = pCPort->MakeAssocIrp( pMasterIrp, pIrp );
            break;
        }

        // no check of the port existance, because
        // we are sure it exists
        if( SUCCEEDED( ret ) )
            ret = pMgr->SubmitIrpInternal( pPort, pIrp );

        if( ret == STATUS_PENDING )
            break;

        EventPtr pCallback = pIrp->m_pCallback;

        pIrp->RemoveTimer();
        pIrp->RemoveCallback();
        pIrp->SetStatus( ret );

        if( !pMasterIrp.IsEmpty() )
        {
            pMgr->CompleteAssocIrp( pIrp );
        }

        // add the event sink to the handle map and
        // send out the port started event if succeeded
        // otherwise, eventPortStartFailed
        CTasklet* pTask = pCallback;
        if( pTask != nullptr )
        {
            ret = ( *pTask )( eventZero );
        }

    }while( 0 );

    if( ERROR( ret ) )
    {
        guint32 dwState = pPort->GetPortState();
        if( dwState != PORT_STATE_REMOVED )
        {
            pMgr->ClosePort(
                PortToHandle( pPort ), nullptr );
        }
    }

    return ret;
}

// -------------------------------------------------------------------------------
// Stop methods begin
// fully asynchronous and concurrent capable
//
// SC  ST  QS  DS  Where to complete
// no  yes no  yes completed by the last CompleteAssocIrp
// no  yes no  no  call completeirp from CGenBusPortStopChildTask
// no  yes yes yes completed by the last CompleteAssocIrp
// no  yes yes no  completed by the last CPnpMgrQueryStopCompletion
// 
// SC: sync call
// ST: switch thread
// QS: Pending in QueryStop
// DS: Pending in DoStop
//
gint32 CPnpManager::QueryStop(
    IPort* pPort, IRP* pIrp, IRP* pMastIrp )
{
    gint32 ret = 0;
    IrpPtr pMasterIrp( pMastIrp );
    do{
        if( pPort == nullptr || pIrp == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        if( pIrp->GetStackSize() == 0 )
        {
            ret = -EINVAL;
            break;
        }
        IrpCtxPtr pIrpCtx = pIrp->GetTopStack(); 

        // star the port
        pIrpCtx->SetMajorCmd( IRP_MJ_PNP );
        pIrpCtx->SetMinorCmd( IRP_MN_PNP_QUERY_STOP );
        pPort->AllocIrpCtxExt( pIrpCtx );

        // we will wait 2 minutes for
        // a query stop operation
        pIrp->SetTimer( PORT_START_TIMEOUT_SEC + 1,
            GetIoMgr() );

        HANDLE hPort = PortToHandle( pPort );
        
        if( pMasterIrp.IsEmpty() )
        {
            pIrp->SetSyncCall( true );
        }
        else
        {
            // set up a callback for query stop in case it returns
            // pending
            EventPtr pEvent;
            CCfgOpener oArgs;

            oArgs.SetPointer( propIoMgr, GetIoMgr() );
            oArgs.SetObjPtr( propIrpPtr, ObjPtr( pMasterIrp ) );
            oArgs.SetObjPtr( propObjPtr, ObjPtr( pPort ) );

            pEvent.NewObj(
                clsid( CPnpMgrQueryStopCompletion ),
                oArgs.GetCfg() );

            pIrp->SetSyncCall( false );
            pIrp->SetCallback( pEvent, hPort );

            CStdRMutex oLock( pMasterIrp->GetLock() );
            ret = pMasterIrp->CanContinue( IRP_STATE_READY );
            if( ERROR( ret ) )
                break;

            // NASTY LOGIC ALERT:
            // lock the master irp from going away when
            // query stop is completed by some irp first
            // and the pMasterIrp is not associated and 
            // go away. But we cannot make the association 
            // right away, because the query stop's completion
            // could also treat the irp->m_pMasterIrp as
            // its own master irp and complete it
            // before the stop command is executed
            //
            // prevent outer caller from completing the
            // master irp
            pMasterIrp->AddSlaveIrp( pIrp );
        }

        // no check of the port existance, because
        // we are sure it exists
        ret = GetIoMgr()->SubmitIrp( hPort, pIrp, false );
        if( ret != STATUS_PENDING )
        {
            // it is possible the return value is 
            // -EAGAIN during stopping, which indicate
            // to retry this irp later sometime
            pIrp->RemoveTimer();
            pIrp->RemoveCallback();

            // FIXME: not a proper approach
            if( !pMasterIrp.IsEmpty() )
            {
                CStdRMutex oLock(
                    pMasterIrp->GetLock() );

                ret = pMasterIrp->CanContinue(
                    IRP_STATE_READY );

                if( ERROR( ret ) )
                    break;

                pMasterIrp->RemoveSlaveIrp( pIrp );
            }
        }
        else
        {
            // let's wait for this irp
            if( pMasterIrp.IsEmpty() )
            {
                ret = pIrp->WaitForComplete();
                if( ret == -EINVAL )
                {
                    // FIXME: too bad, don't know what to do
                    // suicide
                    throw std::runtime_error(
                        "waiting semaphore failed" );
                }
                else if( ret == 0 )
                {
                    ret = pIrp->GetStatus();
                }
            }
        }

    }while( 0 );

    return ret;
}

gint32 CPnpManager::DoStop(
    IPort* pPort, IRP* pIrp, IRP* pMasterIrp )
{
    gint32 ret = 0;
    do{
        EventPtr pEvent;
        CCfgOpener c;
        c.SetPointer( propIoMgr, GetIoMgr() );
        c.SetObjPtr( propObjPtr, ObjPtr( pPort ) );
        c.SetObjPtr( propIrpPtr, ObjPtr( pMasterIrp ) );

        LwVecPtr pParamList( true );
        // the irp to send to the port
        ( *pParamList )().push_back( eventIrpComp );
        ( *pParamList )().push_back( ( LONGWORD )pIrp );
        ( *pParamList )().push_back( eventZero );

        // a flag to indicate we are in the DoStop
        // a bad tweak
        ( *pParamList )().push_back( 1 );

        c.SetObjPtr(
            propParamList, ObjPtr( ( CObjBase* )pParamList ) );

        TaskletPtr pTask;
        if( pMasterIrp != nullptr )
        {
            pTask.NewObj(
                clsid( CPnpMgrQueryStopCompletion ), c.GetCfg() );
        }
        else
        {
            pTask.NewObj(
                clsid( CPnpMgrDoStopNoMasterIrp ), c.GetCfg() );
        }

        ret = ( *pTask )( eventZero );
        
    }while( 0 );

    return ret;
}

gint32 CPnpMgrQueryStopCompletion::operator()(
    guint32 dwContext )
{
    // this task is the callback from QueryStop irp,
    // it will initiate a PNP_STOP request to fully
    // stop the port stack.
    gint32 ret = 0;
    do{
        ObjPtr pObj;
        CCfgOpener a( ( IConfigDb* )GetConfig() );

        CIoManager* pMgr = nullptr;
        ret = a.GetPointer( propIoMgr, pMgr  );
        if( ERROR( ret ) )
            break;
        a.RemoveProperty( propIoMgr );

        PortPtr pPort;
        ret = a.GetObjPtr( propObjPtr, pObj );
        if( ERROR( ret ) )
            break;
        pPort = ( IPort* )pObj;


        ret = a.GetObjPtr( propParamList, pObj );
        if( ERROR( ret ) )
            break;

        LwVecPtr pParamList( pObj );
        a.RemoveProperty( propParamList );

        ret = a.GetObjPtr( propIrpPtr, pObj );
        if( ERROR( ret ) )
            break;

        IrpPtr pMasterIrp( pObj );
        a.RemoveProperty( propIrpPtr );
        if( pMasterIrp.IsEmpty() )
        {
            ret = -EINVAL;
            break;
        }
        guint32 dwArgCount =
            GetArgCount( &IEventSink::OnEvent );

        if( ( *pParamList )().size() == dwArgCount
            && ( *pParamList )()[ 3 ] == 1 )
        {
            // we are not from an irp completion
        }

        // the first parameter is the pointer to
        // the irp this irp is guarded by refcount
        // in the OnEvent callback
        IrpPtr pIrp = reinterpret_cast< IRP* >(
            ( *pParamList )()[ 1 ] );

        if( pIrp.IsEmpty() ||
            pIrp->GetStackSize() == 0 )
        {
            ret = -EINVAL;
            break;
        }
 
        IrpPtr pIrpOld;
        if( dwContext == eventIrpComp )
        {
            pIrpOld = pIrp;
            ret = pIrp.NewObj();

            if( ERROR( ret ) )
                break;

            ret = pIrp->AllocNextStack( pPort );
            if( ERROR( ret ) )
                break;
        }

        IrpCtxPtr& pIrpCtx = pIrp->GetTopStack();

        if( true )
        {
            CStdRMutex oIrpLock( pMasterIrp->GetLock() );
            ret = pMasterIrp->CanContinue( IRP_STATE_READY );
            if( ERROR( ret ) )
                break;

            if( dwContext == eventIrpComp && !pIrpOld.IsEmpty() )
                pMasterIrp->RemoveSlaveIrp( pIrpOld );

            // we don't care if the irp is
            // succeeded or not
            pIrpCtx->SetMajorCmd( IRP_MJ_PNP );
            pIrpCtx->SetMinorCmd( IRP_MN_PNP_STOP );
            pPort->AllocIrpCtxExt( pIrpCtx, ( PIRP )pIrp );

            // clear the callback if any
            IPort* pMasterPort = pMasterIrp->GetTopPort();

            ret = static_cast< CPort* >
                ( pMasterPort )->MakeAssocIrp( pMasterIrp, pIrp );

            if( ERROR( ret ) )
            {
                // nothing to do
                break;
            }
        }

        // we will wait 2 minutes for stop request
        pIrp->SetTimer( PORT_START_TIMEOUT_SEC + 2, pMgr );
        HANDLE hPort = PortToHandle( ( IPort* )pPort );
        ret = pMgr->SubmitIrp( hPort, pIrp, false );

        if( ret == STATUS_PENDING )
        {
            // in the other irp completion the
            // association will be gone via
            // CompleteAssocIrp
            break;
        }
        else
        {
            pIrp->RemoveTimer();
        }

        pMgr->CompleteAssocIrp( pIrp );
        // pMasterIrp, when goes out this scope,
        // should have 1 reference, that is kept
        // by the workitem
        //
        // FIXME:should we complete the master irp
        // here?

    }while( 0 );

    return SetError( ret );
}

gint32 CPnpMgrDoStopNoMasterIrp::operator()(
    guint32 dwContext )
{
    // this task is the callback from QueryStop irp,
    // it will initiate a PNP_STOP request to fully
    // stop the port stack.
    gint32 ret = 0;
    do{
        ObjPtr pObj;
        CCfgOpener oCfg( ( IConfigDb* )m_pCtx );

        CIoManager* pMgr = nullptr;
        ret = oCfg.GetPointer( propIoMgr, pMgr  );
        if( ERROR( ret ) )
            break;

        PortPtr pPort;
        ret = oCfg.GetObjPtr( propObjPtr, pObj );
        if( ERROR( ret ) )
            break;

        pPort = ( IPort* )pObj;
        ret = oCfg.GetObjPtr( propParamList, pObj );
        if( ERROR( ret ) )
            break;

        LwVecPtr pParamList( pObj );
        oCfg.RemoveProperty( propParamList );

        // the first parameter is the pointer to
        // the irp which is guarded by refcount in
        // the OnEvent callback
        IrpPtr pIrp = reinterpret_cast< IRP* >(
            ( *pParamList )()[ 1 ] );

        if( pIrp.IsEmpty() ||
            pIrp->GetStackSize() == 0 )
        {
            ret = -EINVAL;
            break;
        }

        IrpCtxPtr& pIrpCtx = pIrp->GetTopStack();

        // we don't care if the irp is
        // succeeded or not
        pIrpCtx->SetMajorCmd( IRP_MJ_PNP );
        pIrpCtx->SetMinorCmd( IRP_MN_PNP_STOP );
        pPort->AllocIrpCtxExt( pIrpCtx, ( PIRP )pIrp );
        pIrp->SetSyncCall( true );

        // clear the callback if any
        pIrp->RemoveCallback();
        if( ERROR( ret ) )
        {
            // nothing to do
            break;
        }

        // we will wait 2 minutes for stop request
        pIrp->SetTimer( PORT_START_TIMEOUT_SEC + 3, pMgr );
        HANDLE hPort = PortToHandle( ( IPort* )pPort );

        // no need to check existance of the port
        ret = pMgr->SubmitIrp( hPort, pIrp, false );
        if( ret == STATUS_PENDING )
        {
            ret = pIrp->WaitForComplete();
            if( ERROR( ret ) )
                break;
            ret = pIrp->GetStatus();
            break;
        }
        else
        {
            pIrp->RemoveTimer();
        }

    }while( 0 );

    return SetError( ret );
}

gint32 CPnpMgrStopPortAndDestroyTask::RunTask()
{
    return OnScheduledTask( eventZero );
}

gint32 CPnpMgrStopPortAndDestroyTask::OnIrpComplete(
    PIRP pIrp )
{
    gint32 ret = 0;
    
    do{
        CIoManager* pMgr;
        CParamList oParams( GetConfig() );

        ret = oParams.GetPointer(
            propIoMgr, pMgr );

        if( ERROR( ret ) )
            break;

        CPort* pPort = nullptr;
        ret = oParams.GetPointer(
            propPortPtr, pPort );

        if( ERROR( ret ) )
            break;

        EventPtr pCallback;
        CPnpManager& oPnpMgr = pMgr->GetPnpMgr();

        ret = GetClientNotify( pCallback );
        if( SUCCEEDED( ret ) )
        {
            ret = oPnpMgr.DestroyPortStack(
                pPort, pCallback );
            if( ret == STATUS_PENDING )
            {
                // the callback is passed on
                ClearClientNotify();

                // and this task has retired
                ret = 0;
            }
        }
        else
        {
            ret = 0;
            oPnpMgr.DestroyPortStack(
                pPort, pCallback );
        }

    }while( 0 );

    return ret;
}

gint32 CPnpMgrStopPortAndDestroyTask::OnScheduledTask(
    guint32 dwContext )
{
    // parameters:
    // propIoMgr
    // propPortPtr: the port to stop and destroy
    gint32 ret = 0;
    do{
        CIoManager* pMgr;
        CParamList oParams( GetConfig() );
        ret = oParams.GetPointer( propIoMgr, pMgr );
        if( ERROR( ret ) )
            break;

        ObjPtr pObj;
        ret = oParams.GetObjPtr(
            propPortPtr, pObj );
        if( ERROR( ret ) )
            break;

        CPort* pPort = pObj;
        if( pPort == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        // FIXME: this irp is not through the port
        // stack, so it is out of the irp tracking
        // mechanism, IrpIn and IrpOut. it could be a
        // problem when canceling happens on it.
        //
        // For the dbusbus port, when completing this
        // irp, the port is already in the stopped
        // state because the slave irp is the true `stop'
        // irp which physically stop the port.
        //
        // this irp is solely for synchronous purpose
        IrpPtr pMasterIrp( true );
        pMasterIrp->AllocNextStack( pPort );
        IrpCtxPtr& pIrpCtx = pMasterIrp->GetTopStack(); 

        // star the port
        pIrpCtx->SetMajorCmd( IRP_MJ_PNP );
        pIrpCtx->SetMinorCmd( IRP_MN_PNP_STOP );
        pIrpCtx->SetIoDirection( IRP_DIR_OUT );
        pPort->AllocIrpCtxExt( pIrpCtx,
            ( PIRP )pMasterIrp );

        pMasterIrp->SetCallback(
            EventPtr( this ), 0 );

        pMasterIrp->SetSyncCall( false );

        pMasterIrp->SetTimer(
            PORT_START_TIMEOUT_SEC *
            PORT_MAX_GENERATIONS, pMgr );

        pMasterIrp->SetCbOnly( true );

        CPnpManager& oPnpMgr = pMgr->GetPnpMgr();

        // there is chance the slave irp time-out and
        // then this master irp is completed by
        // CompleteAssocIrp, which is not an expected
        // behavior.
        CStdRMutex oIrpLock(
            pMasterIrp->GetLock() );

        ret = oPnpMgr.StopPortStack(
            pPort, pMasterIrp );

        if( ret == STATUS_PENDING )
        {
            pMasterIrp->ResetTimer();
            pMasterIrp->MarkPending();
            break;
        }

        pMasterIrp->SetState( IRP_STATE_READY,
            IRP_STATE_COMPLETED );

        pMasterIrp->RemoveCallback();
        pMasterIrp->RemoveTimer();
        pMasterIrp->SetStatus( ret );

        oIrpLock.Unlock();

        // on error stopping, we do nothing, otherwise,
        // endless stop
        guint32 dwState = pPort->GetPortState();
        if( dwState == PORT_STATE_STOPPED )
            oPnpMgr.DestroyPortStack( pPort );

    }while( 0 );

    return SetError( ret );
}

/**
* @name StopPortsInternal
* @{ */
/** 
 * pPort [in] : the pointer to the port to stop
 * pMasterIrp [in]: the pointer to an IRP which
 * belongs to the parent port of pPort and its
 * completion depends on the completion of
 * stopping the pPort @} */

gint32 CPnpManager::StopPortsInternal(
    IPort* pPort, IRP* pMastIrp )
{
    gint32 ret = 0;
    IrpPtr pMasterIrp( pMastIrp );
    do{
        if( pPort == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        pPort = pPort->GetTopmostPort();

        IrpPtr pQueryIrp( true );
        pQueryIrp->AllocNextStack( pPort );

        ret = QueryStop( pPort, pQueryIrp, pMasterIrp );
        if( ret == STATUS_PENDING )
            break;

        // ignore whether the QueryStop failed or not
        IrpPtr pStopIrp( true );
        pStopIrp->AllocNextStack( pPort );

        ret = DoStop( pPort, pStopIrp, pMasterIrp );

    }while( 0 );

    return ret;
}

gint32 CPnpManager::StartPortStack(
    ObjPtr pPortObj, ObjPtr pMasterIrpObj )
{
    if( pPortObj.IsEmpty() )
        return -EINVAL;

    IPort* pPort = pPortObj;
    if( pPort == nullptr )
        return -EINVAL;

    IRP* pMasterIrp = pMasterIrpObj;

    gint32 ret = StartPortsInternal(
        pPort, pMasterIrp );

    return ret;
}
/**
* @name StopPortStack, to stop a whole stack of
* the ports. 
* @{ */
/** pPort: a port on the stack 
 *
 *  pMasterIrp: The stop irp to complete. Note
 *  that if the pMasterIrp is nullptr, this call
 *  will be a synchronous call. So be careful
 *  where you call this method
 *
 *  bSendEvent: whether to send out a stopping
 *      event to the connection point
 * @} */

gint32 CPnpManager::StopPortStack(
    IPort* pPort, IRP* pMasterIrp, bool bSendEvent )
{
    gint32 ret = 0;

    IPort* pBottomPort =
        pPort->GetBottomPort();


    if( bSendEvent )
    {
        CConnPointHelper oConnPoint( GetIoMgr() );
        oConnPoint.BroadcastEvent(
            propStartStopEvent,
            eventPortStopping, 
            PortToHandle( pBottomPort ),
            0,
            nullptr );
    }

    ret = StopPortsInternal( pPort, pMasterIrp );
    // NOTE: we cannot follow a OnPortStopped
    // notification because there could be other
    // pdo ports stopped during this call
    return ret;
}

void CPnpManager::OnPortStartStop(
    IPort* pTopPort, bool bStop )
{
    // to notify the subscriber a port
    // is coming
    if( pTopPort == nullptr )
        return;

    IPort* pBottomPort =
        pTopPort->GetBottomPort();

    EnumEventId eventId;

    if( bStop )
        eventId = eventPortStopped;
    else
        eventId = eventPortStarted;

    // notify the related upper interfaces
    // about the port event
    CConnPointHelper oConnPoint( GetIoMgr() );
    oConnPoint.BroadcastEvent(
        propStartStopEvent,
        eventId, 
        PortToHandle( pBottomPort ),
        0,
        nullptr );
}

void CPnpManager::OnPortStopped( IPort* pTopPort )
{
    if( pTopPort == nullptr )
        return;
    OnPortStartStop( pTopPort, true );
    return;
}

void CPnpManager::OnPortStarted( IPort* pTopPort )
{
    if( pTopPort == nullptr )
        return;

    OnPortStartStop( pTopPort, false );
    return;
}

gint32 CPnpManager::DestroyPortStack(
    IPort* pPortToDestroy,
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        CIoManager* pMgr = GetIoMgr();
        if( pPortToDestroy == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        deque< PortPtr > quePorts; 
        PortPtr pReadyPort;

        IPort* pPort =
            pPortToDestroy->GetTopmostPort();

        while( pPort != nullptr )
        {
            PortPtr pPortIf;
            pPortIf = pPort;
            quePorts.push_back( pPortIf );

            CPort* pPortImpl = pPortIf;

            CStdRMutex oPortLock(
                pPortImpl->GetLock() );

            if( pReadyPort.IsEmpty() )
            {
                // this is the case when the port
                // failed to start, for example,
                // CTcpStreamPdo
                if( pPortImpl->GetPortState()
                    == PORT_STATE_READY )
                    pReadyPort = pPort;
            }
            pPort = pPort->GetLowerPort();
        }

        IrpPtr pIrp( true );
        pIrp->AllocNextStack( nullptr );
        IrpCtxPtr& pIrpCtx = pIrp->GetTopStack(); 

        // star the port
        pIrpCtx->SetMajorCmd( IRP_MJ_PNP );
        pIrpCtx->SetMinorCmd( IRP_MN_PNP_STACK_DESTROY );

        pIrpCtx->SetIoDirection( IRP_DIR_OUT );

        pPort = pPortToDestroy;

        // no check of the port existance, because
        // we are sure it exists
        if( pPort != pReadyPort )
        {
            ret = pMgr->SubmitIrpInternal( pPort, pIrp, false );
        }

        if( ret == ERROR_STATE )
        {
            // some underlying ports  is still in
            // READY state, we cannot delete them
            // directly, instead, stop them first
            ret = 0;
        }

        PortPtr portPtr;
        while( quePorts.size() > 0 )
        {
            // remove the port stack from top down
            portPtr = quePorts.front();

            if( portPtr.IsEmpty() )
            {
                string strError = DebugMsg(
                    -EFAULT, "port ptr is null" );
                throw std::runtime_error( strError );
            }

            if( portPtr == pReadyPort )
            {
                // we need to stop the port of portPtr,
                // and the underlying ports which is
                // still in READY state, before
                // destroying them
                CCfgOpener oCfg;
                ret = oCfg.SetPointer( propIoMgr, pMgr );

                ret = oCfg.SetObjPtr(
                    propPortPtr,
                    ObjPtr( ( IPort* )portPtr ) );
                    
                TaskletPtr pStopTask;
                ret = pStopTask.NewObj(
                    clsid( CPnpMgrStopPortAndDestroyTask ),
                    oCfg.GetCfg() );

                if( ERROR( ret ) )
                    break;

                if( pCallback != nullptr )
                {
                    CIfRetryTask* pTask = pStopTask;
                    pTask->SetClientNotify( pCallback );
                }

                ret = pMgr->RescheduleTask( pStopTask );

                if( SUCCEEDED( ret ) )
                    ret = STATUS_PENDING;

                // we can quit and let the task do
                // the rest work
                break;
            }

            quePorts.pop_front();
            CPort* pPortImpl = portPtr;

            if( quePorts.size() )
            {
                pPortImpl->DetachFromPort(
                    quePorts.front() );
            }

            CCfgOpenerObj oPortCfg( pPortImpl );
            guint32 dwType = 0;
            ret = oPortCfg.GetIntProp( propPortType, dwType );
            if( ERROR( ret ) )
            {
                string strError = DebugMsg(
                    ret, "Fatal Error: propPortType is missing" );
                throw std::runtime_error( strError );
            }

            switch( dwType )
            {
            case PORTFLG_TYPE_PDO:
                {
                    ObjPtr pObj;
                    ret = oPortCfg.GetObjPtr( propBusPortPtr, pObj );
                    if( ERROR( ret ) )
                        break;

                    CGenericBusPort* pBus = pObj;
                    if( pBus == nullptr )
                    {
                        ret = -EFAULT;
                        break;
                    }

                    guint32 dwPortId;
                    ret = oPortCfg.GetIntProp( propPortId, dwPortId );
                    if( ERROR( ret ) )
                        break;

                    pBus->RemovePdoPort( dwPortId );
                    oPortCfg.RemoveProperty( propBusPortPtr );
                    break;
                }
            case PORTFLG_TYPE_FDO:
                {
                    ObjPtr pObj;
                    // at this moment, we should be OK
                    // to be the only user to access this
                    // port
                    ret = oPortCfg.GetObjPtr( propDrvPtr, pObj );
                    if( ERROR( ret ) )
                        break;

                    IPortDriver* pDrv = pObj;
                    if( pDrv != nullptr )
                    {
                        ret = pDrv->RemovePort( portPtr );
                    }
                    oPortCfg.RemoveProperty( propDrvPtr );
                    break;
                }
             case PORTFLG_TYPE_FIDO:
                {
                    ObjPtr pObj;
                    // at this moment, we should be OK
                    // to be the only user to access this
                    // port
                    ret = oPortCfg.GetObjPtr( propDrvPtr, pObj );
                    if( ERROR( ret ) )
                        break;

                    IPortDriver* pDrv = pObj;
                    if( pDrv != nullptr )
                    {
                        ret = pDrv->RemovePort( portPtr );
                    }
                    oPortCfg.RemoveProperty( propDrvPtr );
                    break;
                }
            default:
                {
                    ret = -ENOTSUP;
                    break;
                }
            }
        }
    }while( 0 );

    return ret;
}

void CPnpManager::HandleCPEvent(
    EnumEventId iEvent,
    LONGWORD dwParam1,
    LONGWORD dwParam2,
    LONGWORD* pData )
{
    CConnPointHelper oConnPoint( GetIoMgr() );

    switch( iEvent )
    {
    case cmdPause:
    case cmdResume:
        {
            oConnPoint.BroadcastEvent(
                propAdminEvent, iEvent, dwParam1, 0, pData );
            break;
        }
    case cmdShutdown:
        {
            oConnPoint.BroadcastEvent(
                propAdminEvent, iEvent, dwParam1, 0, pData );

            // graceful shutdown
            TaskletPtr pTask;
            gint32 ret = DEFER_CALL_NOSCHED(
                pTask, this, &CIoManager::Stop );

            if( ERROR( ret ) )
                break;

            // schedule a new thread for this
            // call, otherwise there is risk of
            // endless wait
            ret = GetIoMgr()->RescheduleTask(
                pTask, true );

            break;
        }
    case eventDBusOffline:
        {
            oConnPoint.BroadcastEvent(
                propDBusSysEvent, iEvent, dwParam1, 0, pData );

            // graceful shutdown
            TaskletPtr pTask;

            gint32 ret = DEFER_CALL_NOSCHED(
                pTask, this, &CIoManager::Stop );

            if( ERROR( ret ) )
                break;

            // schedule a new thread for this
            // call, otherwise there is risk of
            // endless wait
            ret = GetIoMgr()->RescheduleTask(
                pTask, true );

            DebugPrint( ret,
                "DBusOffline triggered a full stop" );

            break;
        }
    case eventModOnline:
    case eventModOffline:
        {
            oConnPoint.BroadcastEvent(
                propDBusModEvent, iEvent, dwParam1, 0, pData );
            break;
        }
    case eventRmtDBusOffline:
    case eventRmtSvrOffline:
    case eventRmtSvrOnline:
    case eventRmtDBusOnline:
        {
            // the connection point broadcast will
            // be sent in the tasklet
            //
            // dwParam1: the peer ip address
            //
            // pData: the proxy port affected this
            // could be a lengthy operation, let's
            // use a standalone task for it
            //
            // schedule a task to stop the related
            // proxy port or the TcpStreamPort
            oConnPoint.BroadcastEvent(
                propRmtSvrEvent, iEvent, dwParam1, 0, pData );
            break;
        }
    case eventRmtModOnline:
    case eventRmtModOffline:
        {
            // dwParam1 is the ip addr, and the
            // pData is the name of the interface
            oConnPoint.BroadcastEvent(
                propRmtModEvent, iEvent, dwParam1, 0, pData );
            break;
        }
    case eventDBusOnline:
        {
            break;
        }
    default:
        {

            // dwParam1 is the ip addr, and the
            // pData is the name of the interface
            oConnPoint.BroadcastEvent(
                propCustomEvent, iEvent, dwParam1, 0, pData );
            break;
        }
    }
}

gint32 CPnpManager::OnEvent(
        EnumEventId iEvent,
        LONGWORD dwParam1,
        LONGWORD dwParam2,
        LONGWORD* pData  )
{

    switch( iEvent )
    {
    case eventPnp:
        {
            IPort* pPort = HandleToPort( dwParam2 );
            if( pPort == nullptr )
                break;

            switch( ( EnumEventId )dwParam1 )
            {
            case eventPortStarted:
                {
                    OnPortStarted( pPort );
                    break;
                }
            case eventPortStopped:
                {
                    OnPortStopped( pPort );
                    break;
                }
            case eventPortAttached:
                {
                    OnPortAttached( pPort );
                    break;
                }
            default:
                break;
            }
        }
    case eventConnPoint:
        {
            EnumEventId iSubEvent =
                ( EnumEventId )dwParam1;

            HandleCPEvent(
                iSubEvent, dwParam2, 0, pData );
            break;
        }
    default:
        break;
    }

    return 0;
}

gint32 CPnpManager::Stop()
{
    gint32 ret = 0;
    do{
        CRegistry& oReg =
            GetIoMgr()->GetRegistry();

        string strPath = REG_IO_EVENTMAP_DIR(
            GetIoMgr()->GetModName().c_str() );

        CStdRMutex a( oReg.GetLock() );
        if( !oReg.ExistingDir( strPath ) )
        {
            // fine, no need to continue
            break;
        }

        ret = oReg.ChangeDir( strPath );
        if( ERROR( ret ) )
            break;

        // remove the pnp event connection point
        ret = oReg.RemoveProperty(
            propStartStopEvent );

        ret = oReg.RemoveProperty(
            propDBusModEvent );

        ret = oReg.RemoveProperty(
            propDBusSysEvent );

        ret = oReg.RemoveProperty(
            propRmtSvrEvent );

        ret = oReg.RemoveProperty(
            propRmtModEvent );

        ret = oReg.RemoveProperty(
            propAdminEvent );

        ret = oReg.RemoveProperty(
            propCustomEvent );

    }while( 0 );

    return ret;
}

gint32 CPnpManager::Start()
{
    gint32 ret = 0;
    CRegistry& oReg =
        GetIoMgr()->GetRegistry();

    vector< EnumPropId > vecEvents;

    do{
        string strPath = REG_IO_EVENTMAP_DIR(
            GetIoMgr()->GetModName().c_str() );

        CStdRMutex a( oReg.GetLock() );
        ret = oReg.ExistingDir( strPath );
        if( ERROR( ret ) )
        {
            ret = oReg.MakeDir( strPath );
            if( ERROR( ret ) )
                break;
        }

        ret = oReg.ChangeDir( strPath );
        if( ERROR( ret ) )
            break;

        // set up the well-known pnp event
        // connection points

        ObjPtr pEvtMap;
        CCfgOpenerObj oCfg( &oReg );

        ret = pEvtMap.NewObj(
            clsid( CStlEventMap ) );

        if( ERROR( ret ) )
            break;
        
        ret = oCfg.SetObjPtr(
            propStartStopEvent, pEvtMap );

        if( ERROR( ret ) )
            break;
        
        vecEvents.push_back( propStartStopEvent );

        ret = pEvtMap.NewObj(
            clsid( CStlEventMap ) );

        if( ERROR( ret ) )
            break;
        
        ret = oCfg.SetObjPtr(
            propDBusModEvent, pEvtMap );

        if( ERROR( ret ) )
            break;
        
        vecEvents.push_back( propDBusModEvent );

        ret = pEvtMap.NewObj(
            clsid( CStlEventMap ) );

        if( ERROR( ret ) )
            break;

        ret = oCfg.SetObjPtr(
            propDBusSysEvent, pEvtMap );

        if( ERROR( ret ) )
            break;
        
        vecEvents.push_back( propDBusSysEvent );

        ret = pEvtMap.NewObj(
            clsid( CStlEventMap ) );

        if( ERROR( ret ) )
            break;
        
        ret = oCfg.SetObjPtr(
            propRmtSvrEvent, pEvtMap );

        if( ERROR( ret ) )
            break;
        
        vecEvents.push_back( propRmtSvrEvent );

        ret = pEvtMap.NewObj(
            clsid( CStlEventMap ) );

        if( ERROR( ret ) )
            break;
        
        ret = oCfg.SetObjPtr(
            propRmtModEvent, pEvtMap );

        if( ERROR( ret ) )
            break;
        
        vecEvents.push_back( propRmtModEvent );

        ret = pEvtMap.NewObj(
            clsid( CStlEventMap ) );

        if( ERROR( ret ) )
            break;
        
        ret = oCfg.SetObjPtr(
            propAdminEvent, pEvtMap );

        if( ERROR( ret ) )
            break;
        
        vecEvents.push_back( propAdminEvent );

        ret = pEvtMap.NewObj(
            clsid( CStlEventMap ) );

        if( ERROR( ret ) )
            break;
        
        ret = oCfg.SetObjPtr(
            propCustomEvent, pEvtMap );

        if( ERROR( ret ) )
            break;
        
        vecEvents.push_back( propCustomEvent );

    }while( 0 );

    if( ERROR( ret ) )
    {
        while( !vecEvents.empty()  )
        {
            oReg.RemoveProperty(
                vecEvents.back() );
            vecEvents.pop_back();
        }
    }

    return ret;
}

}
