/*
 * =====================================================================================
 *
 *       Filename:  iomgr.cpp
 *
 *    Description:  implementation of IO manager
 *
 *        Version:  1.0
 *        Created:  06/27/2016 08:55:02 PM
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

#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <string>
#include "defines.h"
#include "registry.h"
#include "frmwrk.h"
#include "tasklets.h"
#include "jsondef.h"
#include <algorithm>
#include "portex.h"

#include "ifhelper.h"
#include "dbusport.h"

#include <fstream>
#include <memory>

#include "logclibase.h"

namespace rpcf
{

using namespace std;

gint32 CIoManager::SubmitIrpInternal(
    IPort* pPort,
    IRP* pIrp1,
    bool bTopmost )
{
    gint32 ret = 0;

    if( pIrp1 == nullptr || pPort == nullptr )
        return -EINVAL;

    IrpPtr pIrp( pIrp1 );

    do{
        pIrp->m_dwTid = GetTid();
        pIrp->SetSubmited();

        if( bTopmost )
            pPort = pPort->GetTopmostPort();

        pair< PortPtr, IrpCtxPtr >& oStackEntry =
            pIrp->m_vecCtxStack.back();

        // set the port ptr if not already set
        // on the port object stack
        oStackEntry.first = pPort;

        IrpCtxPtr& pCtx = oStackEntry.second;
        BufPtr pExtBuf;
        pCtx->GetExtBuf( pExtBuf );
        if( pExtBuf.IsEmpty() || pExtBuf->empty() )
            pPort->AllocIrpCtxExt( pCtx, pIrp1 );

        CStdRMutex a( pIrp->GetLock() );
        ret = pPort->SubmitIrp( pIrp );
        if( ret != STATUS_PENDING )
        {
            // make this irp unaccessable
            pIrp->SetState(
                IRP_STATE_READY, IRP_STATE_COMPLETED );
            pIrp->SetStatus( ret );
        }
        else
        {
            // we mark the pending only
            // when the irp is about to
            // leave the stack
            pIrp->MarkPending();
        }

    }while( 0 );

    return ret;
}

gint32 CIoManager::SubmitIrp(
    HANDLE hPort, IRP* pIrp, bool bCheckExist )
{
    gint32 ret = 0;

    if( pIrp == nullptr ||
        hPort == INVALID_HANDLE )
        return -EINVAL;

    do{
        PortPtr pPort;
        if( bCheckExist )
        {
            ret = GetPortPtr( hPort, pPort );
            if( ERROR( ret ) )
                break;
        }
        else
        {
            pPort = HandleToPort( hPort );
        }

        ret = SubmitIrpInternal( pPort, pIrp );

    }while( 0 );

    return ret;
}

gint32 CIoManager::CompleteAssocIrp(
    IRP* pSlave )
{
    gint32 ret = 0;

    if( pSlave == nullptr )
        return -EINVAL;

    // let's check if it has accociated irp we don't
    // check the slaves, because the slaves must
    // complete before the master completes
    do
    {
        IrpPtr pMaster = pSlave->GetMasterIrp();
        if( pMaster.IsEmpty()  )
        {
            ret = -EFAULT;
            break;
        }
        // well, we need to inform the marster irp
        // we are done

        IPort* pPort = pMaster->GetTopPort();
        if( pPort != nullptr )
        {
            ret = pPort->CompleteAssocIrp( pMaster, pSlave );
        }

    }while( 0 );

    // we are done with the master irp
    pSlave->SetMasterIrp( nullptr );

    return ret;
}

gint32 CIoManager::CancelAssocIrp( IRP* pSlave )
{
    gint32 ret = 0;

    if( pSlave == nullptr )
        return -EINVAL;

    // first let's check to cancel all the slave
    // irps

    // let's check if it has accociated irp
    // we don't check the slaves, because the
    // slaves must complete before the master
    // completes
    do
    {
        IRP* pMaster = pSlave->GetMasterIrp();
        if( pMaster == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        // well, we need to inform the marster
        // irp we are done
        IPort* pPort = pMaster->GetTopPort();
        if( pPort == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        // FIXME: is it right to complete the 
        // master here?
        ret = pPort->CompleteAssocIrp( pMaster, pSlave );

        if( ERROR( ret ) || ret == STATUS_PENDING ) 
        {
            // we don't care the return value
        }

        // we are done with the master irp
        pSlave->SetMasterIrp( nullptr );

        // recover the original value
        ret = pSlave->GetStatus();

    }while( 0 );

    return ret;
}

gint32 CIoManager::PostCompleteIrp( IRP* pIrp, bool bCancel )
{

    gint32 ret = 0;

    // wake up the user land
    if( pIrp->IsSyncCall() && pIrp->IsPending() )
    {
        // wake up the caller and we are done for
        // synchronous call, it implies that the
        // caller must hold a reference count to
        // the irp till the call finished
        Sem_Post( &pIrp->m_semWait );
    }
    else if( pIrp->IsSyncCall() && !pIrp->IsPending() )
    {
        // this is an immediate return how did we
        // land here
        ret = ERROR_STATE;
    }
    else if( !pIrp->IsSyncCall() && pIrp->IsPending() )
    {
        // asnynchronous call.
        // we don't know what could happen in user
        // callback, and let's schedule an
        // workitem to notify the caller
        if( unlikely( pIrp->CompleteInPlace() && 
            !pIrp->m_pCallback.IsEmpty() ) )
        {
            // complete the irp on the same thread
            // CompleteIrp is called. a
            // performance boost

            LONGWORD dwParam1 =
                ( LONGWORD )( ( IRP* )pIrp );
            LONGWORD dwParam2 = pIrp->m_dwContext;
            TaskletPtr pCb = pIrp->m_pCallback;
            pCb->OnEvent( eventIrpComp,
                dwParam1, dwParam2, nullptr );
        }
        else
        {
            // check to see if we have dedicated irp
            // completion thread
            ThreadPtr thrdPtr;
            IThread* pThrd = nullptr;
            ret = pIrp->GetIrpThread( thrdPtr );

            if( SUCCEEDED( ret ) )
                pThrd = thrdPtr;

            if( nullptr == pThrd )
            {
                // in case some interface use workitem
                // for irp completion
                if( !pIrp->m_pCallback.IsEmpty() )
                {
                    ObjPtr irpPtr( pIrp );
                    CCfgOpener oCfg;
                    ret = oCfg.SetObjPtr( propIrpPtr, irpPtr );
                    if( SUCCEEDED( ret ) )
                    {
                        ret = ScheduleTask(
                            clsid( CIoMgrIrpCompleteTask ),
                            oCfg.GetCfg() );
                    }
                }
            }
            else
            {
                // this is usually for irps from a
                // server interface
                if( pThrd && !pIrp->m_pCallback.IsEmpty() )
                {
                    CIrpCompThread* pIrpThrd = thrdPtr;
                    if( pIrpThrd )
                    {
                        pIrpThrd->AddIrp( pIrp );
                    }
                    else
                    {
                        ret = -EFAULT;
                    }
                }
            }
        }
    }
    else // ( !pIrp->IsSyncCall() && !pIrp->IsPending() )
    {
        // what's going on
    }

    // remove the mutal reference
    pIrp->ClearIrpThread();
    return ret;
}

gint32 CIoManager::CompleteIrp( IRP* pIrpComp )
{

    if( pIrpComp == nullptr )
        return -EINVAL;

    gint32 ret = 0;

    do{
        IrpPtr pIrp( pIrpComp );

        CStdRMutex a( pIrp->GetLock() );

        if( !pIrp->CanComplete() )
        {
            // no need to go further because the SubmitIrp
            // is not returned yet
            ret = ERROR_CANNOT_COMP;
            break;
        }

        // lock this irp
        if( pIrp->GetState() != IRP_STATE_READY )
        {
            ret = ERROR_STATE;
            break;
        }
        ret = pIrp->GetTopStack()->GetStatus();
        if( ERROR( ret ) )
        {
            // possibly the irp timeout
            if( pIrp->GetSlaveCount() )
            {
                // cancel the slave IRPs if any
                a.Unlock();
                CancelSlaves( pIrp, true, ret );
                a.Lock();
            }
        }

        ret = pIrp->CanContinue( IRP_STATE_COMPLETING );
        if( ERROR( ret ) )
            break;

        // walk through the irp context stack
        if( !pIrp->IsCbOnly() )
        {
            gint32 iSize = ( gint32 )pIrp->GetStackSize();
            if( iSize > 0 )
            {
                vector< pair< PortPtr, IrpCtxPtr > >&
                    vecStack = pIrp->m_vecCtxStack;

               for( gint32 i = iSize - 1; i >= 0; --i )
               {
                    pair< PortPtr, IrpCtxPtr >& oElem = vecStack[ i ];
                    if( !oElem.first.IsEmpty() )
                    {
                        pIrp->SetCurPos( i );
                        ret = oElem.first->CompleteIrp( pIrp );
                        if( ret == STATUS_PENDING )
                        {
                            // FIXME: Is it the responsibility of the
                            // guy who returns STATUS_PENDING to set the
                            // irp to the proper state
                            // 
                            pIrp->SetState(
                                IRP_STATE_COMPLETING, IRP_STATE_READY );

                            // NOTE: the user will have to reset
                            // the irp state to a proper value
                            //
                            // we need to stop the completion and
                            // return immediately
                            return ret;
                        }
                    }
                }

                // we should not pop out the first io stack
                // because it is owned by the user land
                IrpCtxPtr& pCtxPtr = vecStack.begin()->second;
                pIrp->SetStatus( pCtxPtr->GetStatus() );
            }
            else
            {
                pIrp->SetStatus( -EINVAL );
            }
        }
        else
        {
            IrpCtxPtr& pCtxPtr =
                pIrp->m_vecCtxStack.begin()->second;
            pIrp->SetStatus( pCtxPtr->GetStatus() );
        }

        // remove the timer if any
        if( pIrp->HasTimer() )
            pIrp->RemoveTimer();

        pIrp->SetState(
            IRP_STATE_COMPLETING, IRP_STATE_COMPLETED );

        a.Unlock();
        if( pIrp->GetMasterIrp() != nullptr )
        {
            this->CompleteAssocIrp( pIrp );
            // we don't care the return value from
            // CompleteAssocIrp
        }

        ret = PostCompleteIrp( pIrp );
        // till now, the irp will go out of
        // the port stack, and no access any more
    }while( 0 );

    return ret;
}

gint32 CIoMgrIrpCompleteTask::operator()(
    guint32 dwContext )
{

    gint32 ret = 0;

    do{
        CCfgOpener a( ( IConfigDb* )GetConfig() );
        ObjPtr objPtr;

        // get the pointer to the bus port
        ret = a.GetObjPtr( propIrpPtr, objPtr );
        if( ERROR( ret ) )
            break;

        a.RemoveProperty( propIrpPtr );

        IrpPtr pIrp = objPtr;
        if( pIrp.IsEmpty() )
        {
            ret = -EINVAL;
            break;
        }

        if( !pIrp->m_pCallback.IsEmpty() )
        {
            LONGWORD dwParam1 =
                ( LONGWORD )( ( IRP* )pIrp );

            LONGWORD dwParam2 =
                pIrp->m_dwContext;

            TaskletPtr pCb = pIrp->m_pCallback;
            pCb->OnEvent(
                eventIrpComp,
                dwParam1,
                dwParam2,
                nullptr );
        }

    }while( 0 );
    return ret;
}

gint32 CIoManager::CancelSlaves(
    IRP* pIrp, bool bForce, gint32 iRet )
{
    gint32 ret = 0;
    do{
        // lock this irp
        CStdRMutex a( pIrp->GetLock() );
        ret = pIrp->CanContinue( IRP_STATE_CANCEL_SLAVE );
        if( ERROR( ret ) )
        {
            // already cancelled or completed
            break;
        }

        if( pIrp->GetSlaveCount() == 0 )
        {
            pIrp->SetState(
                IRP_STATE_CANCEL_SLAVE, IRP_STATE_READY );

            break;
        }

        if( pIrp->m_mapSlaveIrps.size() == 0 )
        {
            // no active slave irps in the map
            // lift the bar to complete the master irp
            pIrp->SetMinSlaves( 0 );
            pIrp->SetState(
                IRP_STATE_CANCEL_SLAVE, IRP_STATE_READY );
            break;
        }

        IRP* pSlave = pIrp->m_mapSlaveIrps.begin()->first;

        // remove the slave here. if we do it after the
        // CancelIrp, there could be chance the master
        // be canceled by other guys.
        pIrp->RemoveSlaveIrp( pSlave );

        // cancel all the slaves first
        if( pIrp->GetSlaveCount() > 0 )
        {
            pIrp->SetState(
                IRP_STATE_CANCEL_SLAVE, IRP_STATE_READY );

            // let's cancel slave irps
            if( pSlave != nullptr )
            {
                // to avoid nested locking, we release the
                // marster irp's lock
                a.Unlock();
                CancelIrp( pSlave, bForce, iRet );
            }
            continue;
        }
        else if( pIrp->GetSlaveCount() == 0 )
        {
            // add a dummy irp to prevent the
            // completion of master irp in
            // CancelAssocIrp, because we are going to
            // cancel it. CancelAssocIrp happens when
            // the slave irp is being canceled from
            // somewhere else
            if( pSlave != nullptr )
            {
                // add a dummy slave to prevent the
                // CompleteIrp from being called in
                // CPort::CompleteAssocIrp
                pIrp->AddSlaveIrp( ( IRP* )nullptr );

                // reset the state to allow others to touch this
                // irp, especially when CompleteAssocIrp from
                // slave irp's CancelIrp
                pIrp->SetState(
                    IRP_STATE_CANCEL_SLAVE, IRP_STATE_READY );

                // let's cancel slave irps
                a.Unlock();
                CancelIrp( pSlave, bForce, iRet );
                continue;
            }
            else
            {
                // should be what we have added on purpose
                pIrp->RemoveSlaveIrp( pSlave );
            }
        }

        pIrp->SetState(
            IRP_STATE_CANCEL_SLAVE, IRP_STATE_READY );

        break;

    }while( 1 );

    return 0;
}

gint32 CIoManager::CancelIrp(
    IRP* pIrp, bool bForce, gint32 iRet )
{
    if( pIrp == nullptr )
        return -EINVAL;

    gint32 ret = 0;

    // here we take the ownership of pIrp
    // it will be released on return
    //
    // We don't need to increment the ref count
    // here because the reference count was
    // incremented by an earilier call to 
    // CIoManager::SubmitIrp,
 
    IrpPtr irpPtr( pIrp );

    do{
        // nasty logics here
        CancelSlaves( pIrp, bForce, iRet );
        
        CStdRMutex a( pIrp->GetLock() );
        if( !pIrp->CanComplete() )
            break;

        ret = pIrp->CanContinue( IRP_STATE_CANCELLING );
        if( ERROR( ret ) )
        {
            // already cancelled or completed
           break; 
        }

        // walk through the irp context stack
        if( !pIrp->IsCbOnly() )
        {
            if( pIrp->GetStackSize() > 0 )
            {
                gint32 iSize = ( gint32 )pIrp->GetStackSize();
                vector< pair< PortPtr, IrpCtxPtr > >&
                    vecStack = pIrp->m_vecCtxStack;

                bool bFirst = true;
                for( gint32 i = iSize - 1; i >= 0; --i )
                {
                    pair< PortPtr, IrpCtxPtr >& oElem = vecStack[ i ];
                    if( !oElem.first.IsEmpty() )
                    {
                        // set the status for the top stack
                        if( bFirst )
                        {
                            oElem.second->SetStatus(
                                iRet == 0 ? -ECANCELED : iRet );
                            bFirst = false;
                        }

                        pIrp->SetCurPos( i );
                        oElem.first->CancelIrp( pIrp, bForce );
                    }

                    // the context stack will be popped by the
                    // Port's CancelIrp
                    //
                    // Note that the top most IRP_CONTEXT will
                    // not be popped by the port at the bottom of
                    // the port stack
                    //
                    // the rest non-bottom ports will
                    // face the IRP_CONTEXT it prepared for
                    // the its lower port

                }
            }
        }
        pIrp->SetStatus( iRet );

        // we leave the last IRP_CONTEXT for the io manager
        // or the attached interface, at this point
        // the no port reference is held by this IRP
        //
        // till now, the irp will go out of
        // the port stack, and no access any more

        if( pIrp->HasTimer() )
        {
            GetUtils().GetTimerSvc().RemoveTimer( pIrp->m_iTimerId );
        }

        pIrp->SetState(
            IRP_STATE_CANCELLING, IRP_STATE_CANCELLED );

        a.Unlock();
        if( pIrp->GetMasterIrp() != nullptr )
        {
            this->CancelAssocIrp( pIrp );
        }

        ret = PostCompleteIrp( pIrp, true );

    }while( 0 );

    return ret;
}


CUtilities&
CIoManager::GetUtils() const
{
    return *m_pUtils;
}

CPnpManager&
CIoManager::GetPnpMgr() const
{
    return *m_pPnpMgr;
}

CDriverManager&
CIoManager::GetDrvMgr() const
{
    return *m_pDrvMgr;
}

CRegistry&
CIoManager::GetRegistry() const
{
    return *m_pReg;
}

CThreadPool&
CIoManager::GetIrpThreadPool() const
{
    return *m_pIrpThrdPool;
}

CThreadPool&
CIoManager::GetTaskThreadPool() const
{
    return *m_pTaskThrdPool;
}

CLoopPools&
CIoManager::GetLoopPools() const
{
    return *m_pLPools;
}

CThreadPools&
CIoManager::GetThreadPools() const
{
    return *m_pPools;
}

ObjPtr&
CIoManager::GetSyncIf() const
{
    return const_cast< ObjPtr& >( m_pSyncIf );
}

// registry helpers
gint32 CIoManager::RegisterObject(
    const std::string& strPath, const ObjPtr& pObj )
{
    gint32 ret = 0;
    do{
        CRegistry& oReg = GetRegistry();
        CStdRMutex a( oReg.GetLock() );
        ret = oReg.ChangeDir( strPath );
        if( ret == -ENOENT )
        {
            ret = oReg.MakeDir( strPath );
            if( SUCCEEDED( ret ) )
                ret = oReg.ChangeDir( strPath );
        }

        if( ERROR( ret ) )
            break;

        ret = oReg.SetObject( propObjPtr, pObj );

    }while( 0 );
    return ret;
}

gint32 CIoManager::UnregisterObject(
    const std::string& strPath )
{
    gint32 ret = 0;
    do{
        CRegistry& oReg = GetRegistry();
        CStdRMutex a( oReg.GetLock() );

        ret = oReg.ChangeDir( strPath );
        if( ERROR( ret ) )
            break;

        ret = oReg.RemoveProperty( propObjPtr );
    }while( 0 );

    return ret;
}

gint32 CIoManager::SetObject(
    const std::string& strPath, ObjPtr& pObj )
{
    gint32 ret = 0;
    do{
        CRegistry& oReg = GetRegistry();
        CStdRMutex a( oReg.GetLock() );
        ret = oReg.ChangeDir( strPath );
        if( ret == -ENOENT )
        {
            ret = oReg.MakeDir( strPath );
            if( SUCCEEDED( ret ) )
                ret = oReg.ChangeDir( strPath );
        }

        if( ERROR( ret ) )
            break;

        ret = oReg.SetObject( propObjPtr, pObj );

    }while( 0 );

    return ret;
}

gint32 CIoManager::GetObject(
    const std::string& strPath, ObjPtr& pObj )
{
    gint32 ret = 0;
    do{
        CRegistry& oReg = GetRegistry();
        CStdRMutex a( oReg.GetLock() );
        ret = oReg.ChangeDir( strPath );
        if( ERROR( ret ) )
            break;

        ret = oReg.GetObject( propObjPtr, pObj );

    }while( 0 );

    return ret;
}

// with the name strPort or config parameters from pcfg
gint32 CIoManager::OpenPortByPath(
    const std::string& strPath,
    HANDLE& hPort )
{
    gint32 ret = 0;

    ObjPtr objPtr;
    do{
        ret = this->GetObject( strPath, objPtr );
        if( ERROR( ret ) )
            break;

        CObjBase* pObj = objPtr;

        IPort* pPort = ( IPort* ) pObj;
        if( pPort == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        // further check to see if the port
        // registered with the handle map
        ret = PortExist( pPort );
        if( ERROR( ret ) )
        {
            // well, the port might not be
            // ready for user access yet
            break;
        }

        hPort = PortToHandle( pPort );

    }while( 0 );

    return ret;
}

gint32 CIoManager::CloseChildPort(
    IPort* pBusPort,
    IPort* pPort,
    IEventSink* pCallback )
{
    if( pPort == nullptr ||
        pBusPort == nullptr ||
        pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        IrpPtr pIrp( true );
        pIrp->AllocNextStack( nullptr );
        IrpCtxPtr& pIrpCtx = pIrp->GetTopStack(); 

        // stop the child port
        pIrpCtx->SetMajorCmd( IRP_MJ_PNP );
        pIrpCtx->SetMinorCmd( IRP_MN_PNP_STOP_CHILD );
        pIrpCtx->SetIoDirection( IRP_DIR_OUT );
        pBusPort->AllocIrpCtxExt( pIrpCtx );

        BufPtr pBuf( true );
        *pBuf = ObjPtr( pPort );
        pIrpCtx->SetReqData( pBuf );

        pIrp->SetCallback( pCallback, 0 );
        pIrp->SetTimer(
            PORT_START_TIMEOUT_SEC, this );

        HANDLE hBusPort =
            PortToHandle( pBusPort );

        pIrp->SetIrpThread( this );

        ret = SubmitIrp( hBusPort, pIrp, false );
        if( ret != STATUS_PENDING )
        {
            pIrp->RemoveTimer();
            pIrp->RemoveCallback();
            pIrp->ClearIrpThread();
        }

    }while( 0 );

    return ret;
}

gint32 CIoManager::ClosePort(
    HANDLE hPort,
    IEventSink* pIf, // the registered interface
    IEventSink* pCallback ) // the callback for complete stop
{
    // Note: we need to make sure all the irps are
    // completed or canceled and the interface
    // unregistered before calling this method
    // otherwise unexpected things would happen
    gint32 ret = 0;
    do{
        if( hPort == INVALID_HANDLE )
        {
            ret = -EINVAL;
            break;
        }
        bool bNoRef = false;
        PortPtr ptrPort;
        ret = GetPortPtr( hPort, ptrPort );
        if( ERROR( ret ) )
            break;

        CPort* pPort = static_cast< CPort* >(
            HandleToPort( hPort ) );

        if( pIf != nullptr )
            RemoveFromHandleMap( pPort, pIf );
        ret = IsPortNoRef( pPort, bNoRef );
        if( ERROR( ret ) )
        {
            ret = 0;
            break;
        }

        if( !bNoRef )
            break;

        if( pPort->Unloadable() && !IsStopping() )
        {
            TaskletPtr pDummy;
            if( pCallback == nullptr )
            {
                ret = pDummy.NewObj(
                    clsid( CIfDummyTask ) );
                if( SUCCEEDED( ret ) )
                    pCallback = pDummy;
            }

            IPort* pBusPort = nullptr;

            CCfgOpenerObj oPortCfg( pPort );
            ret = oPortCfg.GetPointer(
                propBusPortPtr, pBusPort );

            if( ERROR( ret ) )
            {
                ObjPtr pDriver;
                ret = oPortCfg.GetObjPtr(
                    propDrvPtr, pDriver );
                if( ERROR( ret ) )
                    break;

                CGenBusDriverEx* pBusDrv = pDriver;
                if( pBusDrv == nullptr )
                {
                    ret = -EFAULT;
                    break;
                }
                ret = pBusDrv->DestroyPortSynced(
                    pPort, pCallback );
            }
            else
            {
                ret = CloseChildPort( pBusPort,
                    pPort, pCallback );
            }
        }

    }while( 0 );

    return ret;
}

gint32 CIoManager::OpenPortByRef(
    HANDLE hPort,
    IEventSink* pEventSink )
{
    if( hPort == INVALID_HANDLE ||
        pEventSink == nullptr )
        return -EINVAL;

    PortPtr pPort;
    gint32 ret = GetPortPtr( hPort, pPort );
    if( ERROR( ret ) )
        return ret;

    return AddToHandleMap( pPort, pEventSink );
}

gint32 CIoManager::GetPortProp(
    HANDLE hPort,
    gint32 iProp,
    BufPtr& pBuf )
{
    if( hPort == INVALID_HANDLE )
        return -EINVAL;

    PortPtr pPort;
    gint32 ret = GetPortPtr( hPort, pPort );
    if( ERROR( ret ) )
        return ret;

    CCfgOpenerObj oCfg( ( CObjBase* )pPort );
    ret = oCfg.GetProperty( iProp, pBuf );

    return ret;
}

gint32 CIoManager::MakeBusRegPath(
    std::string& strPath,
    const std::string& strBusName )
{

    size_t pos;
    pos = strBusName.rfind( '.' );
    string strBusPort;
    if( pos == string::npos )
    {
        strBusPort = strBusName;
    }
    else if( pos + 1 < strBusName.size() )
    {
        strBusPort =
            strBusName.substr( pos + 1 );
    }
    else
    {
        return -EINVAL;
    }

    strPath = string( "/port/" )
        + GetModName()
        + "/bus/"
        + strBusPort;

    return 0;
}

// create a port in the hard way
gint32 CIoManager::OpenPortByCfg(
    const IConfigDb* pCfg, PortPtr& pPort )
{
    gint32 ret = 0;
    // we are sure the bus port will be loaded
    // earlier than the first call of OpenPortByCfg
    // our first try is to use bus name to create
    // the specified port
    do{
        if( pCfg == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        try{
            string strBusName;
            CCfgOpener a( pCfg );
            ret =  a.GetStrProp( propBusName, strBusName );
            if( ERROR( ret ) )
                break;

            string strPath;
            ret = MakeBusRegPath( strPath, strBusName );
            if( ERROR( ret ) )
                break;

            // get the pointer to the bus port
            ObjPtr objPtr;
            ret = this->GetObject( strPath, objPtr );
            if( ret == -ENOENT )
            {
                // possibly the driver and port are not
                // loaded yet
                ret = -EAGAIN;
                break;
            }
            if( ERROR( ret ) )
                break;

            CGenericBusPort* pBus = objPtr;

            if( pBus == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            // create the pdo port or open the
            // existing pdo port
            ret = pBus->OpenPdoPort( pCfg, pPort );
            if( ERROR( ret ) )
                break;

        }
        catch ( std::invalid_argument& e )
        {
            ret = -EINVAL;
        }

    }while( 0 );

    return ret;
}

gint32 CIoManager::OpenPort(
    const IConfigDb* pCfg,
    HANDLE& hPort )
{
    PortPtr pPort;
    gint32 ret = 0;

    do{
        if( pCfg == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        /*if( RunningOnMainThread() )
        {
            // we cannot open port on the
            // main IO thread
            ret = ERROR_WRONG_THREAD;
            break;
        }*/

        // FIXME: what is the ref count of pPort
        ret = this->OpenPortByCfg( pCfg, pPort );

        if( SUCCEEDED( ret ) || ret == EEXIST )
        {
            if( ret == EEXIST && !pPort.IsEmpty() )
            {
                // make sure the OpenPortByCfg
                // won't return a port stopped or
                // stopping
                CPort* pNewPort =
                    static_cast< CPort* >( pPort );

                CStdRMutex oPortLock(
                    pNewPort->GetLock() );

                guint32 dwState =
                    pNewPort->GetPortState();

                if( dwState == PORT_STATE_STOPPING 
                    || dwState == PORT_STATE_STOPPED
                    || dwState == PORT_STATE_ATTACHED
                    || dwState == PORT_STATE_STARTING )
                {
                    ret = ERROR_STATE;
                    break;
                }
            }

            // the new port is created 
            ObjPtr pObj;
            CCfgOpener a( pCfg );
            ret = a.GetObjPtr( propIfPtr, pObj );
            if( ERROR( ret ) )
            {
                // it is required to bind event
                // sink to the handle
                break;
            }

            IEventSink* pEvent = pObj;
            if( pEvent == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            ret = AddToHandleMap( pPort, pEvent );
            hPort = PortToHandle( pPort );
            break;
        }
        else if( ret == STATUS_PENDING )
        {
            // the response will be returned via the
            // pCallback. The hPort is valid though,
            // but you cannot use it right now, because
            // it is not added to the handle map yet.
            // It can be used as a hint in the
            // callback
            //
            // for the port to start, OpenPort will
            // propbably, end up at this point
            break;
        }

    }while( 0 );

    return ret;
}

gint32 CIoManager::OnEvent( EnumEventId iEvent,
    LONGWORD dwParam1,
    LONGWORD dwParam2,
    LONGWORD* pData)
{
    gint32 ret = 0;
    switch( iEvent )
    {
    case eventWorkitem:
        {
            switch( ( EnumEventId )dwParam1 )
            {
            case eventStart:
                {
                    CParamList oParams;

                    oParams.SetPointer(
                        propIoMgr, this );

                    oParams.SetBoolProp(
                        propNoPort, true );

                    ret = GetSyncIf().NewObj(
                        clsid( CSimpleSyncIf ),
                        oParams.GetCfg() );
                    if( ERROR( ret ) )
                        break;

                    CRpcServices* pIf = GetSyncIf();
                    ret = pIf->Start();
                    if( ERROR( ret ) )
                        break;

                    // notified we can start.  the
                    // sem will be posted after
                    // the mainloop is started,
                    // which is important for
                    // receiving tcp socket events 
                    ret = GetPnpMgr().Start();
                    if( ERROR( ret ) )
                        break;

                    ret = GetDrvMgr().Start();

                    if( ERROR( ret ) )
                        GetPnpMgr().Stop();

                    m_iHcTimer = GetUtils().GetTimerSvc().
                        AddTimer( 30, this, eventHouseClean );

                    break;
                }
            default:
                break;
            }
            break;
        }
    case eventWorkitemCancel:
        {
            switch( ( EnumEventId )dwParam1 )
            {
            default:
                break;
            }
            break;
        }
    case eventTimeout:
        {
            switch( ( EnumEventId )dwParam1 )
            {
            case eventHouseClean:
                {
                    ClearStandAloneThreads();
                    m_iHcTimer = GetUtils().GetTimerSvc().
                        AddTimer( 30, this, eventHouseClean );
                    break;
                }
            default:
                break;
            }
            break;
        }
    default:
        break;
    }
    return ret;
}

CMainIoLoop* CIoManager::GetMainIoLoop() const
{
    return m_pLoop;
}

const string& CIoManager::GetModName() const
{
    return m_strModName;
}

gint32 CIoManager::AddToHandleMap(
    IPort* pPort, IEventSink* pEvent )
{
    if( pPort == nullptr )
        return -EINVAL;

    return m_oPortIfMap.AddToHandleMap(
        pPort, pEvent );
}

gint32 CIoManager::RemoveFromHandleMap(
    IPort* pPort, IEventSink* pEvent )
{
    return m_oPortIfMap.RemoveFromHandleMap(
        pPort, pEvent );
}

gint32 CIoManager::PortExist(
    IPort* pPort, vector< EventPtr >* pvecEvents )
{
    return m_oPortIfMap.PortExist( pPort, pvecEvents );
}

gint32 CIoManager::GetPortPtr(
    HANDLE hHandle, PortPtr& pPort ) const
{
    return m_oPortIfMap.GetPortPtr( hHandle, pPort );
}

gint32 CIoManager::IsPortNoRef(
    IPort* pPort, bool& bNoRef ) const
{
    return m_oPortIfMap.IsPortNoRef( pPort, bNoRef );
}

bool CIoManager::RunningOnMainThread() const
{

    CMainIoLoop* pMainLoop = GetMainIoLoop();
    if( pMainLoop == nullptr )
    {
        throw std::runtime_error(
            "Mainloop bad address" );
    }

    guint32 dwMainTid =
        pMainLoop->GetThreadId();

    return GetTid() == dwMainTid;
}

gint32 CIoManager::EnumDBusNames(
    vector<string>& vecNames )
{
    gint32 ret = 0;
    do{
        string strModeName = GetModName();

        // NOTE: c++11 needed for to_string

        string strPath = string( "/port/" )
            + strModeName
            + "/bus";

        CRegistry& oReg = GetRegistry();
        CStdRMutex oLock( oReg.GetLock() );
        ret = oReg.ChangeDir( strPath );
        if( ERROR( ret ) )
            break;

        vector< string > vecBusNames;
        ret = oReg.ListDir( vecBusNames );
        if( ERROR( ret ) )
            break;

        for( guint32 i = 0; i < vecBusNames.size(); i++ )
        {
            gint32 ret1 = strncmp(
                vecBusNames[ i ].c_str(),
                PORT_CLASS_LOCALDBUS,
                sizeof( PORT_CLASS_LOCALDBUS ) - 1 );

            if( ret1 == 0 )
            {
                vecNames.push_back( vecBusNames[ i ] );
            }
        }

        if( vecNames.empty() )
            ret = -ENOENT;

    }while( 0 );
    return ret;
}

gint32 CIoManager::Start()
{
    gint32 ret = 0;
    do{
        if( m_bInit )
        {
            // well, it is already initialized,
            // let's fail
            ret = ERROR_STATE;
            break;
        }

        OutputMsg( 0, "IoMgr is starting..." );

        m_bInit = true;
        m_bStop = false;

        GetTaskThreadPool().Start();
        GetIrpThreadPool().Start();
        GetLoopPools().Start();

        // this task will be executed in the
        // mainloop
        CCfgOpener a;
        a.SetPointer( propIoMgr, this );

        TaskletPtr pTask;
        ret = pTask.NewObj(
            clsid( CIoMgrPostStartTask ),
            a.GetCfg() );

        if( ERROR( ret ) )
            break;

        ret = RescheduleTaskMainLoop( pTask );
        if( ERROR( ret ) )
            break;

        // let's start the main loop. The whole
        // system begin to run.
        ret = GetMainIoLoop()->Start();
        if( ERROR( ret ) )
            break;

        Sem_Wait( GetSyncSem() );
        ret = pTask->GetError();
        if( ERROR( ret ) )
            break;

        m_oLogger.Start();
        LOGINFO( this, 0, "IoMgr is started..." );

    }while( 0 );

    return ret;
}


gint32 CIoManager::ScheduleTaskMainLoop(
    EnumClsid iClsid, CfgPtr& pCfg )
{

    gint32 ret = 0;
    do{
        TaskletPtr pTask;
        ret = pTask.NewObj( iClsid, pCfg );
        if( ERROR( ret ) )
            break;
        RescheduleTaskMainLoop( pTask );

    }while( 0 );

    return ret;
}

gint32 CIoManager::RescheduleTaskMainLoop(
    TaskletPtr& pTask )
{

    gint32 ret = 0;
    do{
        if( pTask.IsEmpty() )
        {
            ret = -EINVAL;
            break;
        }
        pTask->MarkPending();
        CMainIoLoop* pMainLoop = GetMainIoLoop();
        if( pMainLoop->IsStopped() )
        {
            ret = ERROR_STATE;
            break;
        }
        GetMainIoLoop()->AddTask( pTask );

        if( !pTask->IsAsync() )
        {
            CTaskletSync* pSyncTask =
                static_cast< CTaskletSync* >( ( CTasklet* )pTask );

            if( pSyncTask != nullptr )
            {
                pSyncTask->WaitForComplete();
                ret = pSyncTask->GetError();
            }
        }

    }while( 0 );

    return ret;
}

gint32 CIoManager::ClearStandAloneThreads()
{
    CStdRMutex oLock( GetLock() );
    vector< ThreadPtr >::iterator itr =
        m_vecStandAloneThread.begin();

    while( itr != m_vecStandAloneThread.end() ) 
    {
        if( !( *itr )->IsRunning() )
        {
            ( *itr )->Stop();
            itr = m_vecStandAloneThread.erase( itr );
        }
        else
            ++itr;
    }

    if( !m_vecStandAloneThread.empty() )
        return -EEXIST;

    return 0;
}

void CIoManager::WaitThreadsQuit()
{
    while( ClearStandAloneThreads() == -EEXIST )
    {
        CStdRMutex oLock( GetLock() );
        if( m_vecStandAloneThread.empty() )
            break;
        ThreadPtr pThread =
            m_vecStandAloneThread.front();

        oLock.Unlock();
        if( !pThread.IsEmpty() )
            pThread->Join();
    }
}

gint32 CIoManager::StartStandAloneThread(
    ThreadPtr& pThread, EnumClsid iTaskClsid )
{
    gint32 ret = 0;
    
    ret = pThread.NewObj(
        clsid( COneshotTaskThread ) );

    if( ERROR( ret ) )
        return ret;

    CCfgOpenerObj oThrdCfg(
        ( CObjBase*) pThread );

    ret = oThrdCfg.SetIntProp(
        propClsid, iTaskClsid );

    if( ERROR( ret ) )
        return ret;

    pThread->Start(); 

    CStdRMutex oLock( GetLock() );
    m_vecStandAloneThread.push_back( pThread );
    ClearStandAloneThreads();

    return ret;
}

/**
* @name ScheduleTask
* @{ Schedule a deferred task, which will be run
* out of caller running context. if the task is 
* a synchronous task, this method will wait till
* the task is completed
**/
/**Parameters:
 *  iClsid [ in ]: the class id of the task to run
 *  pCfg [ in ]: the config db as the contructor
 *      parameter.
 *
 *  bOneShot [ in ]: the flag to indicate the task
 *      will run on the newly created thread other than
 *      the thread from a thread pool, and the newly
 *      created thread will quit after this task is 
 *      completed
 *@} */

gint32 CIoManager::ScheduleTask(
    EnumClsid iClsid,
    CfgPtr& pCfg,
    bool bOneshot )
{
    gint32 ret = 0;

    do{
        TaskletPtr pTask;
        ret = pTask.NewObj( iClsid, pCfg );
        if( ERROR( ret ) )
            break;

        ret = RescheduleTask(
            pTask, bOneshot );

    }while( 0 );

    return ret;
}

gint32 CIoManager::RescheduleTask(
    TaskletPtr& pTask, bool bOneshot )
{
    gint32 ret = 0;

    do{
        if( pTask.IsEmpty() )
        {
            ret = -EINVAL;
            break;
        }

        ThreadPtr pThread;

        if( unlikely( bOneshot ) )
        {
            ret = StartStandAloneThread(
                pThread, pTask->GetClsid() );

            if( ERROR( ret ) )
                break;

            pTask->MarkPending();

            CTaskThread* pTaskThread = pThread;
            pTaskThread->AddTask( pTask );
        }
        else
        {
            ret = GetTaskThreadPool().GetThread( pThread, true );
            if( ERROR( ret ) )
                break;

            CTaskThread* pgth =
                static_cast< CTaskThread* >( pThread );

            if( pgth )
            {
                pTask->MarkPending();
                pgth->AddTask( pTask );
            }
        }

        if( !pTask->IsAsync() )
        {
            CTaskletSync* pSyncTask =
                static_cast< CTaskletSync* >( ( CTasklet* )pTask );

            if( pSyncTask != nullptr )
            {
                pSyncTask->WaitForComplete();
                ret = pSyncTask->GetError();
            }
        }

    }while( 0 );

    return ret;
}

gint32 CIoManager::RescheduleTaskByTid(
    TaskletPtr& pTask, guint32 dwTid )
{
    gint32 ret = 0;

    do{
        if( pTask.IsEmpty() )
        {
            ret = -EINVAL;
            break;
        }
        if( dwTid == 0 )
            dwTid = rpcf::GetTid();

        ThreadPtr pThread;
        ret = GetTaskThreadPool().GetThreadByTid(
            pThread, dwTid );
        if( SUCCEEDED( ret ) )
        {
            CTaskThread* pgth =
                static_cast< CTaskThread* >( pThread );

            if( pgth )
            {
                pTask->MarkPending();
                pgth->AddTask( pTask );
            }
            break;
        }
        ret = RescheduleTask( pTask );

    }while( 0 );

    return ret;
}

gint32 CIoManager::Stop()
{
    gint32 ret = 0;
    DebugPrint( 0, "IoMgr is stopping..." );
    LOGINFO( this, 0, "IoMgr is shuting down..." );
    m_oLogger.Stop();

    if( true )
    {
        if( IsStopping() )
            return ERROR_STATE;
        m_bStop = true;
    }

    if( m_iHcTimer != 0 )
    {
        GetUtils().GetTimerSvc().
            RemoveTimer( m_iHcTimer );
        m_iHcTimer = 0;
    }

    CCfgOpener a;
    a.SetPointer( propIoMgr, this );

    ret = ScheduleTask(
        clsid( CIoMgrStopTask ),
        a.GetCfg(),
        true );
    WaitThreadsQuit();
    return ret;
}

CIoManager::CIoManager( const std::string& strModName ) :
      m_strModName( strModName ),
      m_oLogger( this )
{
    gint32 ret = 0;
    SetClassId( clsid( CIoManager ) );

    do{
        CParamList a;
        a.SetPointer( propIoMgr, this );
        a.SetStrProp( propConfigPath,
            string( CONFIG_FILE ) );

        CfgPtr pCfg = a.GetCfg();

        ret = m_pReg.NewObj();
        if( ERROR( ret ) )
        {
            throw std::runtime_error(
                "CRegistry failed to initialize" );
        }
        m_pReg->MakeDir( "/cmdline" );

        StrVecPtr pvecPaths( true );
        // ( *pvecPaths )().insert( "." );

        std::string strPath;
        // get the libcombase.so's path
        ret = GetLibPath( strPath );
        if( SUCCEEDED( ret ) )
        {
            ( *pvecPaths )().push_back( strPath );
            stdstr strTestLibs = strPath + "/rpcf";
            ( *pvecPaths )().push_back( strTestLibs );
        }

        // get the executable's path
        strPath.clear();
        ret = GetModulePath( strPath ); 
        if( SUCCEEDED( ret ) )
        {
            ( *pvecPaths )().push_back( strPath );
            stdstr strTestBins = strPath + "/rpcf";
            ( *pvecPaths )().push_back( strTestBins );
        }

        if( ( *pvecPaths)().empty() )
        {
            throw std::runtime_error(
                "Error get search paths for"
                " config files" );
        }

        GetEnvLibPath( ( *pvecPaths )() );
        ObjPtr pObj = pvecPaths;
        this->SetCmdLineOpt(
            propSearchPaths, pObj );

        ret = m_pUtils.NewObj(
            clsid( CUtilities ), pCfg );
        if( ERROR( ret ) )
        {
            throw std::runtime_error(
                "CUtilities failed to initialize" );
        }

        ret = m_pPnpMgr.NewObj(
            clsid( CPnpManager ), pCfg );

        if( ERROR( ret ) )
        {
            throw std::runtime_error(
                "CPnpManager failed to initialize" );
        }

        ret = m_pLoop.NewObj(
            clsid( CMainIoLoop ), pCfg );

        if( ERROR( ret ) )
        {
            throw std::runtime_error(
                "CMainIoLoop failed to initialize" );
        }

        ret = m_pPools.NewObj(
            clsid( CThreadPools ), pCfg );

        if( ERROR( ret ) )
        {
            throw std::runtime_error(
                "CThreadPools failed to initialize" );
        }

        // load count
        a.Push( 1 );
        // max thread
        a.Push( 2 );
        a.Push( ( guint32 )clsid( CIrpCompThread ) );

        ret = m_pIrpThrdPool.NewObj(
            clsid( CIrpThreadPool ), pCfg );
        if( ERROR( ret ) )
        {
            throw std::runtime_error(
                "CIrpThreadPool failed to initialize" );
        }

        a.ClearParams();
        // load count
        a.Push( 1 );
        // max thread
        a.Push( 2 );
        a.Push( ( guint32 )clsid( CTaskThread ) );

        ret = m_pTaskThrdPool.NewObj(
            clsid( CTaskThreadPool ), pCfg );
        if( ERROR( ret ) )
        {
            throw std::runtime_error(
                "CTaskThreadPool failed to initialize" );
        }

        Sem_Init( &m_semSync, 0, 0 );

        ret = m_pLPools.NewObj(
            clsid( CLoopPools ), pCfg );
        if( ERROR( ret ) )
            break;

        m_iHcTimer = 0;

        m_dwNumCores = std::max( 1U,
            std::thread::hardware_concurrency() );

    }while( 0 );

    return;
}

static inline string ExtractModNameIoMgr(
    const IConfigDb* pCfg )
{

    if( pCfg == nullptr )
    {
        string strMsg = DebugMsg( -EINVAL,
            "Invalid argument");
        throw std::invalid_argument( strMsg );
    }

    CCfgOpener oCfg( pCfg );
    string strModName;
    gint32 ret = oCfg.GetStrProp(
        0, strModName );

    if( ERROR( ret ) )
    {
        string strMsg = DebugMsg( -EINVAL,
            "Error extract modname");
        throw std::invalid_argument( strMsg );
    }

    return strModName;
}

CIoManager::CIoManager(
    const IConfigDb* pCfg )
   :CIoManager( ExtractModNameIoMgr( pCfg ) )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg( pCfg );

        guint32 dwMaxThrds = 0;
        ret = oCfg.GetIntProp(
            propMaxIrpThrd, dwMaxThrds );
        if( SUCCEEDED( ret ) )
        {
            dwMaxThrds = std::min(
                dwMaxThrds,GetNumCores() );
            m_pIrpThrdPool->SetMaxThreads(
                dwMaxThrds );
        }

        ret = oCfg.GetIntProp(
            propMaxTaskThrd, dwMaxThrds );
        if( SUCCEEDED( ret ) )
        {
            dwMaxThrds = std::min(
                dwMaxThrds,GetNumCores() );

            // task thread count cannot be zero
            if( dwMaxThrds == 0 )
                dwMaxThrds = 1;

            m_pTaskThrdPool->SetMaxThreads(
                dwMaxThrds );
        }

        CCfgOpener oDrvCfg;
        oDrvCfg.SetPointer( propIoMgr, this );
        ret = oDrvCfg.CopyProp(
            propConfigPath, pCfg );
        if( ERROR( ret ) )
        {
            oDrvCfg.SetStrProp(
                propConfigPath, CONFIG_FILE );
        }

        ret = m_pDrvMgr.NewObj(
            clsid( CDriverManager ),
            oDrvCfg.GetCfg() );

        if( ERROR( ret ) )
        {
            throw std::runtime_error(
                "CDriverManager failed to initialize" );
        }

        Json::Value& oJsonCfg = m_pDrvMgr->GetJsonCfg();
        if( oJsonCfg == Json::Value::null )
        {
            throw std::invalid_argument(
                "No config" );
        }

        Json::Value& oArch = oJsonCfg[ JSON_ATTR_ARCH ];
        if( oArch != Json::Value::null && 
            oArch.isMember( JSON_ATTR_NUM_CORE ) &&
            oArch[ JSON_ATTR_NUM_CORE ] != Json::Value::null )
        {
            Json::Value& oCores = oArch[ JSON_ATTR_NUM_CORE ];
            string strVal = oCores.asString();
            guint32 dwNumCores = std::strtol(
                strVal.c_str(), nullptr, 10 );
            if( dwNumCores != 0 )
            {
                // the user wants to limit the 
                // physical number
                m_dwNumCores = std::min(
                    m_dwNumCores, dwNumCores );
            }
        }

        ret = oCfg.GetBoolProp(
            propEnableLogging, m_bLogging );
        if( ERROR( ret ) )
            m_bLogging = false;

    }while( 0 );
}

CIoManager::~CIoManager()
{
    sem_destroy( &m_semSync );
}

std::recursive_mutex& CIoManager::GetLock() const
{
    return m_oGrandLock;
}
gint32 CIoManager::ScheduleWorkitem(
    guint32 dwFlags,
    IEventSink* pCallback,
    LONGWORD dwContext,
    bool bLongWait )
{
    CWorkitemManager& oWiMgr =
        GetUtils().GetWorkitemManager();

    return oWiMgr.ScheduleWorkitem( dwFlags,
        pCallback, dwContext, bLongWait ); 
}

sem_t* CIoManager::GetSyncSem() const
{
    return ( sem_t* )&m_semSync;
}

gint32 CIoManager::RemoveTask(
    TaskletPtr& pTask )
{
    if( pTask.IsEmpty() )
        return -EINVAL;

    ThrdPoolPtr pThrdPool(
         &GetTaskThreadPool() );

    CTaskThreadPool* pTaskPool = pThrdPool;
    if( pTaskPool == nullptr )
        return -EFAULT;

    return pTaskPool->RemoveTask( pTask );
}

gint32 CIoManager::TryLoadClassFactory(
    const std::string& strPath )
{
    gint32 ret = 0;
    do{
        ret = access( strPath.c_str(), R_OK );
        if( SUCCEEDED( ret ) )
        {
            ret = CoLoadClassFactory(
                strPath.c_str() );
            break;
        }
        if( strPath[ 0 ] == '/' )
        {
            ret = -ENOENT;
            DebugPrint( ret,
                "Failed to load class factory %s",
                strPath.c_str() );
            break;
        }

        // relative path
        std::string strFile =
            basename( strPath.c_str() );

        ObjPtr pObj;
        ret = this->GetCmdLineOpt(
            propSearchPaths, pObj );
        if( ERROR( ret ) )
            break;

        StrVecPtr pvecPaths( pObj );
        if( pvecPaths.IsEmpty() )
        {
            ret = -ENOENT;
            break;
        }

        bool bLoaded = false;
        for( auto& elem : ( *pvecPaths )() )
        {
            std::string strFullPath =
                elem + "/" + strFile;

            ret = access(
                strFullPath.c_str(), R_OK );

            if( ret == -1 )
            {
                ret = -errno;
                continue;
            }

            ret = CoLoadClassFactory(
                strFullPath.c_str() );
            if( ERROR( ret ) )
                continue;

            bLoaded = true;
            break;
        }

        if( bLoaded )
            ret = 0;
        else
        {
            DebugPrint( ret,
                "Failed to load class factory %s",
                strPath.c_str() );
            ret = -ENOENT;
        }

    }while( 0 );

    return ret;
}

gint32 CIoManager::TryFindDescFile(
    const std::string& strFileName,
    std::string& strPath,
    bool bSkipLocal )
{
    gint32 ret = 0;
    do{
        ObjPtr pObj;

        // relative path
        std::string strFile =
            basename( strFileName.c_str() );

        if( !bSkipLocal )
        {
            stdstr strLocal = "./" + strFile;
            ret = access( strLocal.c_str(), R_OK );
            if( ret == 0 )
            {
                strPath = strLocal;
                ret = STATUS_SUCCESS;
                break;
            }
        }

        ret = GetCmdLineOpt(
            propSearchPaths, pObj );
        if( ERROR( ret ) )
            break;

        StrVecPtr pvecPaths( pObj );
        if( pvecPaths.IsEmpty() )
        {
            ret = -ENOENT;
            break;
        }
        bool bFound = false;
        std::string strFullPath;
        for( auto& elem : ( *pvecPaths )() )
        {
            strFullPath = elem + "/" +  strFile;
            ret = access(
                strFullPath.c_str(), R_OK );

            if( ret == 0 )
            {
                bFound = true;
                break;
            }

            strFullPath = elem + "/../etc/rpcf/" + strFile;
            ret = access(
                strFullPath.c_str(), R_OK );
            if( ret == 0 )
            {
                bFound = true;
                break;
            }
            ret = -errno;
        }

        if( bFound )
        {
            strPath = strFullPath;
            ret = 0;
            break;
        }

        ret = FindInstCfg(
            strFile, strPath );

    }while( 0 );

    return ret;
}

template<>
gint32 CIoManager::GetCmdLineOpt(
    EnumPropId iPropId, std::string& oVal )
{
    gint32 ret = 0;
    do{
        CStdRMutex oRegLock( m_pReg->GetLock() );
        ret = m_pReg->ChangeDir( "/cmdline" );
        if( ERROR( ret ) )
            break;

        Variant oVar;
        ret = m_pReg->GetProperty( iPropId, oVar );
        if( ERROR( ret ) )
            break;

        oVal = ( stdstr& )oVar;
    }while( 0 );

    return ret;
}

gint32 CIoManager::GetRouterName(
    std::string& strRtName )
{
    gint32 ret = GetCmdLineOpt(
        propRouterName, strRtName );
    if( ERROR( ret ) )
         strRtName = MODNAME_RPCROUTER;
    return 0;
}

gint32 CIoManager::AddSeqTask(
    TaskletPtr& pTask, bool bLong )
{
    if( IsStopping() )
        return ERROR_STATE;

    CRpcServices* pIf = GetSyncIf();
    if( pIf == nullptr )
        return -EFAULT;

    return pIf->AddSeqTask( pTask, bLong );
}
gint32 CIoManager::AddAndRun(
    TaskletPtr& pTask, bool bImmediate )
{
    CRpcServices* pIf = GetSyncIf();
    if( pIf == nullptr )
        return -EFAULT;

    return pIf->AddAndRun( pTask, bImmediate );
}

gint32 CIoManager::AppendAndRun(
    TaskletPtr& pTask )
{
    CRpcServices* pIf = GetSyncIf();
    if( pIf == nullptr )
        return -EFAULT;

    return pIf->AppendAndRun( pTask );
}

bool CIoManager::HasBuiltinRt()
{
    bool bBuiltinRt = false;
    GetCmdLineOpt(
        propBuiltinRt, bBuiltinRt );
    return bBuiltinRt;
}

bool CIoManager::IsLogging()
{ return m_bLogging; }

static std::vector< stdstr > s_vecLogLevel = {

    "EMERG",
    "ALERT",
    "CRITI",
    "ERROR",
    "WARN",
    "NOTE", // = 5
    "INFO",
};

gint32 CIoManager::LogMessage(
    guint32 dwLogLevel, const std::string& szFile,
    gint32 iLineNum, const std::string& strFmt,
    gint32 ret, ... )
{
    if( !IsLogging() )
        return ERROR_STATE;

    if( dwLogLevel >= s_vecLogLevel.size() )
        return -EINVAL;

    char szBuf[ 8192 ];
    if( strFmt.size() >= sizeof( szBuf ) )
        return -E2BIG;

    gint32 iSize = sizeof( szBuf );
    szBuf[ iSize - 1 ] = 0;

    va_list argptr;
    va_start(argptr, ret );
    vsnprintf( szBuf, iSize - 1,
        strFmt.c_str(), argptr );
    va_end(argptr);

    stdstr strFile = "[";
    strFile += this->GetModName() + "][";
    strFile += s_vecLogLevel[ dwLogLevel ] + "] ";
    strFile += szFile;

    stdstr strFinalMsg = DebugMsgInternal(
        ret, szBuf, strFile.c_str(), iLineNum );
    return m_oLogger.PushMessage( strFinalMsg );
}

void CIoManager::LoggerThread( IEventSink* pCb )
{
    m_oLogger.ThreadProc( pCb );
}

CLogger::CLogger( CIoManager* pMgr )
{
    m_pMgr = pMgr;
    Sem_Init( &m_semMsgs, 0, 0 );
    Sem_Init( &m_semStop, 0, 0 );
    Sem_Init( &m_semStart, 0, 0 );
}

gint32 CLogger::ThreadProc( IEventSink* pCb )
{
    gint32 ret = 0;
    ObjPtr pLogCli;
    do{
        ret = m_pMgr->TryLoadClassFactory(
            "liblogcli.so" );
        if( ERROR( ret ) )
            break;

        CParamList oParams;
        oParams.SetPointer( propIoMgr, m_pMgr );

        ret = CRpcServices::LoadObjDesc(
            "./loggerdesc.json", "LogService",
            false, oParams.GetCfg() );
        if( ERROR( ret ) )
            break;

        ret = pLogCli.NewObj(
            clsid( CLogService_CliImpl ),
            oParams.GetCfg() );
        if( ERROR( ret ) )
            break;

        CRpcServices* pSvc = pLogCli;
        ret = pSvc->Start();
        if( ERROR( ret ) )
        {
            DebugPrintEx( logErr, ret,
                "Warning failed to start logger" );
            break;
        }

        m_pLogCli = pLogCli;
        Sem_Post( &m_semStart );

        while( !m_bExit )
        {
            CStdRMutex oSvcLock( pSvc->GetLock() );
            if( pSvc->GetState() != stateConnected )
            {
                DebugPrintEx( logErr, ERROR_STATE,
                    "Warning logger is offline, "
                    "and waiting for it online" );
            }
            oSvcLock.Unlock();

            ret = Sem_TimedwaitSec( &m_semMsgs, 10 );
            if( ret == -EAGAIN )
                continue;

            if( ERROR( ret ) )
            {
                DebugPrintEx( logErr, ERROR_STATE,
                    "Error logger internal error, "
                    "logging disabled." );
                break;
            }

            CStdRMutex oLock( GetLock() );
            if( m_queMsgs.empty() )
                continue;

            stdstr strMsg = std::move(
                m_queMsgs.front() );
            m_queMsgs.pop_front();
            oLock.Unlock();
            this->SendLogMsg( strMsg );
        }

        do{
            CStdRMutex oLock( GetLock() );
            m_bQuit = true;
            if( m_queMsgs.empty() )
                break;
            stdstr strMsg = std::move(
                m_queMsgs.front() );
            m_queMsgs.pop_front();
            oLock.Unlock();
            this->SendLogMsg( strMsg );

        }while( 1 );

        m_pLogCli.Clear();
        pSvc->Stop();

    }while( 0 );

    CStdRMutex oLock( GetLock() );
    m_bQuit = true;
    Sem_Post( &m_semStop );
    return 0;
}

gint32 CLogger::Start()
{
    TaskletPtr pTask; 

    if( !m_pMgr->IsLogging() )
        return 0;

    gint32 (*func)( IEventSink*, CIoManager* ) =
    ([]( IEventSink* pCb, CIoManager* pMgr )->gint32
    {
        pMgr->LoggerThread( pCb );
        return 0;
    });
    gint32 ret = NEW_FUNCCALL_TASK2(
        0, pTask, m_pMgr, func, nullptr, m_pMgr );
    if( ERROR( ret ) )
        return ret;

    m_pMgr->RescheduleTask( pTask, true );
    Sem_Wait( &m_semStart );
    return 0;
}

gint32 CLogger::Stop()
{

    CStdRMutex oLock( GetLock() );
    if( !m_pMgr->IsLogging() ||
        m_bExit )
        return 0;
    m_bExit = true;
    Sem_Post( &m_semMsgs );
    oLock.Unlock();
    Sem_Wait( &m_semStop );
    return 0;
}

gint32 CLogger::SendLogMsg(
    const stdstr& strMsg )
{
    gint32 ret = 0;
    do{
        ILogSvc_PImpl* pSvc = m_pLogCli;
        ret = pSvc->LogMessage( strMsg );

    }while( 0 );
    return ret;
}

gint32 CLogger::PushMessage(
    const stdstr& strMsg )
{
    CStdRMutex oLock( GetLock() );
    if( m_bExit || m_bQuit )
        return 0;
    m_queMsgs.push_back( strMsg );
    Sem_Post( &m_semMsgs );
    return 0;
}

gint32 CIoMgrStopTask::operator()(
    guint32 dwContext )
{
    gint32 ret = 0;
    CIoManager* pMgr;
    CCfgOpenerObj oCfg( this );
    ret = oCfg.GetPointer( propIoMgr, pMgr );
    if( ERROR( ret ) )
        return ret;

    if( pMgr )
    {
        // NOTE: we cannot stop task
        // thread pool here
        ret = pMgr->GetDrvMgr().Stop();
        ret = pMgr->GetPnpMgr().Stop();
        ObjPtr pObj = pMgr->GetSyncIf();
        if( !pObj.IsEmpty() )
        {
            CRpcServices* pIf = pObj;
            pIf->Stop();
        }
        ret = pMgr->GetTaskThreadPool().Stop();
        ret = pMgr->GetIrpThreadPool().Stop();
        ret = pMgr->GetLoopPools().Stop();
        ret = pMgr->GetMainIoLoop()->Stop();
        ret = pMgr->GetUtils().Stop();
        ret = pMgr->GetThreadPools().Stop();
    }

    if( dwContext == eventOneShotTaskThrdCtx )
        Sem_Post( &m_semSync );

    return SetError( ret );
}

gint32 CIoMgrPostStartTask::operator()(
    guint32 dwContext )
{
    gint32 ret = 0;
    ObjPtr pObj;
    CIoManager* pMgr = nullptr;
    do{
        IConfigDb* pCfg = GetConfig();
        if( pCfg == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        CCfgOpener oCfg( pCfg );

        ret = oCfg.GetPointer( propIoMgr, pMgr );
        if( ERROR( ret ) )
            break;

        ret = pMgr->GetUtils().Start();
        if( ERROR( ret ) )
            break;

        ret = pMgr->OnEvent( eventWorkitem,
            eventStart, 0, ( LONGWORD* )this );

    }while( 0 );

    SetError( ret );

    if( pMgr )
        Sem_Post( pMgr->GetSyncSem() );

    return ret;
}

gint32 CSimpleSyncIf::OnPreStop(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        gint32 (*func)( IEventSink* pCb ) =
        ([]( IEventSink* pCb )->gint32
        {
            if( pCb == nullptr )
                return -EFAULT;
            pCb->OnEvent( eventTaskComp, 0, 0, nullptr );
            return 0;
        });
        CIoManager* pMgr = GetIoMgr();
        TaskletPtr pTailTask;
        ret = NEW_FUNCCALL_TASK( pTailTask,
            pMgr, func, pCallback );
        if( ERROR( ret ) )
            break;
        ret = AddSeqTask( pTailTask );
        if( ERROR( ret ) )
        {
            ( *pTailTask )( eventCancelTask );
            break;
        }
        if( SUCCEEDED( ret ) )
            ret = pTailTask->GetError();

    }while( 0 );

    return ret;
}

gint32 CSimpleSyncIf::Stop()
{ return CRpcInterfaceBase::Stop(); }

gint32 CSimpleSyncIf::OnPostStop(
    IEventSink* pCallback )
{ return super::OnPostStop( pCallback ); }

static gint32 GenHashInstId(
    guint32 dwPort,
    const stdstr& strIpAddr,
    stdstr& strHash )
{
    gint32 ret = 0;
    do{
        stdstr strVal1 = "t_";
        strVal1 += strIpAddr + "_" +
            std::to_string( dwPort );

        guint32 dwHash;
        ret = GenStrHash( strVal1, dwHash );
        if( ERROR( ret ) )
            break;

        dwHash = htonl( dwHash );
        ret = BytesToString( ( guint8* )&dwHash,
            sizeof( dwHash ), strHash );

    }while( 0 );
    return ret;
}

stdstr InstIdFromObjDesc(
    const stdstr& strDesc,
    const stdstr& strObj )
{
    gint32 ret = 0;
    stdstr strVal = std::string( "t" ) +
        std::to_string( RPC_SVR_DEFAULT_PORTNUM );
    do{
        Json::Value oVal;
        ret = ReadJsonCfgFile( strDesc, oVal );
        if( ERROR( ret ) )
            break;
        if( !oVal.isMember( JSON_ATTR_OBJARR ) ||
            !oVal[ JSON_ATTR_OBJARR ].isArray() ||
            !oVal[ JSON_ATTR_OBJARR ].size() )
        {
            ret = -ENOENT;
            break;
        }
        Json::Value oObjArr =
            oVal[ JSON_ATTR_OBJARR ];

        gint32 i = 0;
        for( ; i < oObjArr.size(); i++ )
        {
            Json::Value& oElem = oObjArr[ i ];
            if( oElem == Json::Value::null )
                continue;
            if( !oElem.isObject() ||
                !oElem.isMember( JSON_ATTR_OBJNAME ) )
                continue;

            if( oElem[ JSON_ATTR_OBJNAME ].asString() !=
                strObj )
                continue;

            bool bws = false;
            if( oElem.isMember(
                JSON_ATTR_ENABLE_WEBSOCKET ) )
            {
                Json::Value& val = oElem[
                    JSON_ATTR_ENABLE_WEBSOCKET ];
                if( val.isString() &&
                    val.asString() == "true" )
                    bws = true;
            }

            if( !oElem.isMember( JSON_ATTR_TCPPORT ) ||
                !oElem[ JSON_ATTR_TCPPORT ].isString() )
            {
                ret = -EINVAL;
                break;
            }

            stdstr strVal1 =
                oElem[ JSON_ATTR_TCPPORT ].asString();

            guint32 dwVal = strtoul(
                strVal1.c_str(), nullptr, 10 );

            if( dwVal > 65535 )
            {
                ret = -EINVAL;
                break;
            }

            if( bws )
            {
                // for websocket, we use the URL to
                // generate the instance id instead
                if( !oElem.isMember( JSON_ATTR_DEST_URL) )
                {
                    ret = -EINVAL;
                    break;
                }
                Json::Value& val =
                    oElem[ JSON_ATTR_DEST_URL ];
                if( !val.isString() )
                {
                    ret = -EINVAL;
                    break;
                }

                stdstr strVal2 = val.asString();
                ret = GenHashInstId(
                    0, strVal2, strVal );
                break;
            }

            if( dwVal < RPC_SVR_DEFAULT_PORTNUM )
            {
                ret = -EINVAL;
                break;
            }

            if( !oElem.isMember( JSON_ATTR_IPADDR ) ||
                !oElem[ JSON_ATTR_IPADDR ].isString() )
            {
                ret = -EINVAL;
                break;
            }
            stdstr strVal2 =
                oElem[ JSON_ATTR_IPADDR ].asString();

            stdstr strIpAddr;
            ret = NormalizeIpAddrEx(
                strVal2, strIpAddr );
            if( ERROR( ret ) )
                break;

            if( strIpAddr == "0.0.0.0" ||
                strIpAddr == "::" ||
                strIpAddr == "::0" )
            {
                ret = -EINVAL;
                break;
            }
            ret = GenHashInstId(
                dwVal, strIpAddr, strVal );
            break;
        }

        if( ERROR( ret ) )
            break;

        if( i == oObjArr.size() )
            ret = -ENOENT;

    }while( 0 );

    if( ERROR( ret ) )
        return "";

    return strVal;
}

stdstr InstIdFromDrv(
    const stdstr& strDrv )
{
    gint32 ret = 0;
    stdstr strVal = std::string( "t" ) +
        std::to_string( RPC_SVR_DEFAULT_PORTNUM );
    do{
        Json::Value oVal;
        ret = ReadJsonCfg( strDrv, oVal );
        if( ERROR( ret ) )
            break;

        if( !oVal.isMember( JSON_ATTR_PORTS ) ||
            !oVal[ JSON_ATTR_PORTS ].isArray() ||
            !oVal[ JSON_ATTR_PORTS ].size() )
        {
            ret = -ENOENT;
            break;
        }

        Json::Value& oPorts =
            oVal[ JSON_ATTR_PORTS ];

        gint32 i = 0;
        for( ; i < oPorts.size(); i++ )
        {
            Json::Value& oPort = oPorts[ i ];
            if( oPort == Json::Value::null )
                continue;

            if( !oPort.isMember( JSON_ATTR_PORTCLASS ) ||
                !oPort[ JSON_ATTR_PORTCLASS ].isString() )
                continue;

            string strPortClass =
                oPort[ JSON_ATTR_PORTCLASS ].asString();
            if( strPortClass != PORT_CLASS_RPC_TCPBUS )
                continue;

            if( !oPort.isMember( JSON_ATTR_PARAMETERS ) || 
                !oPort[ JSON_ATTR_PARAMETERS ].isArray() ||
                oPort[ JSON_ATTR_PARAMETERS ].size() == 0 )
            {
                ret = -ENOENT;
                break;
            }
            Json::Value& oParam =
                oPort[ JSON_ATTR_PARAMETERS ][ 0 ];
            if( !oParam.isMember( JSON_ATTR_TCPPORT ) ||
                !oParam[ JSON_ATTR_TCPPORT ].isString() )
            {
                ret = -EINVAL;
                break;
            }

            stdstr strVal1 =
                oParam[ JSON_ATTR_TCPPORT ].asString();

            guint32 dwVal = strtoul(
                strVal1.c_str(), nullptr, 10 );
            if( dwVal > 65535 ||
                dwVal < RPC_SVR_DEFAULT_PORTNUM )
            {
                ret = -EINVAL;
                break;
            }

            if( !oParam.isMember( JSON_ATTR_BINDADDR ) ||
                !oParam[ JSON_ATTR_BINDADDR ].isString() )
            {
                ret = -EINVAL;
                break;
            }
            stdstr strVal2 =
                oParam[ JSON_ATTR_BINDADDR ].asString();

            stdstr strIpAddr;
            ret = NormalizeIpAddrEx(
                strVal2, strIpAddr );
            if( ERROR( ret ) )
                break;

            ret = GenHashInstId(
                dwVal, strIpAddr, strVal );
            break;
        }

        if( ERROR( ret ) )
            break;

        if( i == oPorts.size() )
            ret = -ENOENT;

    }while( 0 );

    if( ERROR( ret ) )
        return "";

    return strVal;
}

/**
* @name UpdateObjDesc update the attributes in the desc
* file and return a new desc file on success
* @{
    @param strDesc a path to the object description
    file to update

    @param pCfg A map with multiple input parameters.
    The valid key value is one of the following.

         <p>101: the role of the rpcrouter, only valid
         when builti-in router is enabled.</p>

         <p>102: a boolean telling whether the
         authoration enabled or not, [optional].</p>

         <p>104: a string of the application name.</p>

         <p>107: a string as the server instance name
         of the builtin router. [optional]</p>

         <p>108: a string as the server instance name
         of the stand-alone router. [optional]</p>

         <p>109: a string as the ip address to connect
         toor bind to [optional]</p>

         <p>110: a string as the port numer to connect
         to or listen on [optional]</p>

         <p>111: a boolean to tell if the router run as a 
         kinit proxy</p>

    @param strNewDesc an new desc file, or empty if
    nothing to update. and property 107 is set with new
    instance name if both 107 and 108 are not present.

    @return a status code.
*/
/**  @} */

gint32 UpdateObjDesc(
    const stdstr strDesc,
    IConfigDb* pCfg,
    stdstr& strNewDesc )
{
    if( strDesc.empty() ||
        pCfg == nullptr )
        return -EINVAL;
    gint32 ret = 0;
    do{
        CCfgOpener oCfg( pCfg );
        guint32 dwVal;
        guint32 dwRole;
        ret = oCfg.GetIntProp( 101, dwVal );
        if( ERROR( ret ) )
            break;
        dwRole = dwVal;

        bool bChanged = false;
        bool bProxy = ( ( dwVal & 1 ) > 0 );
        bool bAuth = false;
        oCfg.GetBoolProp( 102, bAuth );

        stdstr strAppName;
        ret = oCfg.GetStrProp( 104, strAppName );
        if( ERROR( ret ) )
            break;

        stdstr strInst;
        oCfg.GetStrProp( 107, strInst );

        stdstr strSaInst;
        if( bProxy )
            oCfg.GetStrProp( 108, strSaInst );

        stdstr strIpAddr;
        stdstr strRawAddr;
        ret = oCfg.GetStrProp( 109, strRawAddr );
        bool bDomain = false;
        if( SUCCEEDED( ret ) )
        {
            ret = NormalizeIpAddrEx2(
                strRawAddr, strIpAddr, bDomain );
            if( ERROR( ret ) )
            {
                DebugPrintEx( logErr, ret,
                    "Error invalid IP address" );
                break;
            }
        }

        stdstr strPort;
        oCfg.GetStrProp( 110, strPort );

        Json::Value oVal;
        ret = ReadJsonCfgFile( strDesc, oVal );
        if( ERROR( ret ) )
            break;

        Json::Value& oObjArr =
            oVal[ JSON_ATTR_OBJARR ];

        gint32 i;

        // update auth info
        if( bProxy )
        {
            if( !oVal.isMember( JSON_ATTR_OBJARR ) ||
                !oVal[ JSON_ATTR_OBJARR ].isArray() ||
                !oVal[ JSON_ATTR_OBJARR ].size() )
            {
                ret = -ENOENT;
                break;
            }
            for( i = 0; i < oObjArr.size(); i++ )
            {
                Json::Value& oElem = oObjArr[ i ];
                if( oElem == Json::Value::null )
                    continue;
                if( !oElem.isObject() )
                    continue;
                if( !oElem.isMember( JSON_ATTR_AUTHINFO ) )
                    continue;
                if( oElem.isMember(
                    JSON_ATTR_AUTHINFO) && !bAuth )
                {
                    oElem.removeMember(
                        JSON_ATTR_AUTHINFO );
                    bChanged = true;
                }
                else //( isMember( JSON_ATTR_AUTHINFO ) && bAuth )
                {
                    if( strRawAddr.empty() )
                        continue;
                    Json::Value& oAuth =
                        oElem[ JSON_ATTR_AUTHINFO ];
                    if( !oAuth.isMember( JSON_ATTR_SVCNAME ) ||
                        !oAuth[ JSON_ATTR_SVCNAME ].isString() )
                        continue;
                    bChanged = true;
                    Json::Value& oSvc =
                        oAuth[ JSON_ATTR_SVCNAME ];
                    stdstr strSvc = oSvc.asString();
                    size_t pos = strSvc.find_first_of( '@' );
                    if( pos != std::string::npos )
                        strSvc.erase( pos );
                    strSvc.push_back( '@' );
                    // built a new svc-host address
                    strSvc += strRawAddr;
                    oSvc = strSvc;
                }
            }
            if( ERROR( ret ) )
                break;
        }

        // update ip address
        if( strIpAddr.size() || strPort.size() )
        {
            for( i = 0; i < oObjArr.size(); i++ )
            {
                Json::Value& oElem = oObjArr[ i ];
                if( oElem == Json::Value::null )
                    continue;
                if( !oElem.isObject() )
                    continue;
                if( strIpAddr.size() &&
                    oElem.isMember( JSON_ATTR_IPADDR ) )
                {
                    oElem[ JSON_ATTR_IPADDR ] =
                        strIpAddr;
                    bChanged = true;
                }
                if( strPort.size() &&
                    oElem.isMember( JSON_ATTR_TCPPORT ) )
                {
                    oElem[ JSON_ATTR_TCPPORT ] =
                        strPort;
                    bChanged = true;
                }
            }
        }

        //update server name
        if( strInst.size() )
        {
            // using the name from cmdline
            oVal[ JSON_ATTR_SVRNAME ] = strInst;
            bChanged = true;
        }
        else if( strSaInst.empty() )
        {
            // generate the server name if no
            // stand-alone router name is given
            // otherwise, using the original name from
            // the file.
            stdstr strInstId;
            for( i = 0; i< oObjArr.size(); i++ )
            {
                Json::Value& oElem = oObjArr[ i ];
                if( oElem == Json::Value::null )
                    continue;
                if( !oElem.isObject() )
                    continue;
                const char* p =
                    JSON_ATTR_ENABLE_WEBSOCKET;
                if( oElem.isMember( p ) &&
                    oElem[ p ].isString() &&
                    oElem[ p ].asString() == "true" )
                {
                    p = JSON_ATTR_DEST_URL;
                    stdstr strVal =
                        oElem[ p ].asString();
                    ret = GenHashInstId(
                        0, strVal, strInstId );
                    break;
                }

                if( !oElem.isMember( JSON_ATTR_IPADDR ) )
                    continue;

                strIpAddr = oElem[
                    JSON_ATTR_IPADDR ].asString();
                strPort = oElem[
                    JSON_ATTR_TCPPORT ].asString();

                guint32 dwPort = strtoul(
                    strPort.c_str(), nullptr, 10 );
                ret = GenHashInstId(
                    dwPort, strIpAddr, strInstId );

                break;
            }
            if( ERROR( ret ) )
                break;
            if( strInstId.empty() )
            {
                ret = -ENOENT;
                break;
            }

            stdstr strNewName =
                strAppName + "_rt_" + strInstId;
            oVal[ JSON_ATTR_SVRNAME ] = strNewName;
            oCfg[ 107 ] = strNewName;
            bChanged = true;
        }

        if( !bChanged )
            break;

        char szNewDesc[32];
        if( dwRole & 2 )
            strcpy( szNewDesc, "/tmp/rpcfos_XXXXXX" );
        else
            strcpy( szNewDesc, "/tmp/rpcfod_XXXXXX" );

        int iFd = mkstemp( szNewDesc );
        if( iFd < 0 )
        {
            ret = -errno;
            break;
        }
        close( iFd );

        Json::StreamWriterBuilder oBuilder;
        oBuilder["commentStyle"] = "None";
        oBuilder["indentation"] = "    ";
        std::unique_ptr<Json::StreamWriter> pWriter(
            oBuilder.newStreamWriter() );
        std::ofstream ofs;
        ofs.open ( szNewDesc ,
            std::ofstream::out |
            std::ofstream::trunc);
        pWriter->write( oVal, &ofs );
        ofs.close();
        strNewDesc = szNewDesc;

    }while( 0 );

    return ret;
}

/**
* @name UpdateDrvCfg update the attributes in the
* driver.json file and return a new desc file on
* success
* @{
    @param strDriver a path to the driver.json
    file to update

    @param pCfg A map with multiple input parameters.
    The valid key value is one of the following.

         <p>101: the role of the rpcrouter, only valid
         when builti-in router is enabled.</p>

         <p>109: a string as the ip address to bind to
         </p>

         <p>110: a string as the port numer to listen
         on </p>

    @param strNewDrv an new driver.json, or empty if
    nothing to update. 

    @return a status code.
*/
/**  @} */

gint32 UpdateDrvCfg(
    const stdstr strDriver,
    IConfigDb* pCfg,
    stdstr& strNewDrv )
{
    if( strDriver.empty() ||
        pCfg == nullptr )
        return -EINVAL;
    gint32 ret = 0;
    do{
        CCfgOpener oCfg( pCfg );
        guint32 dwVal;
        ret = oCfg.GetIntProp( 101, dwVal );
        if( ERROR( ret ) )
            break;

        bool bChanged = false;
        bool bProxy = ( dwVal == 1 );

        stdstr strAppName;
        ret = oCfg.GetStrProp( 104, strAppName );
        if( ERROR( ret ) )
            break;

        stdstr strIpAddr;
        stdstr strRawAddr;
        ret = oCfg.GetStrProp( 109, strRawAddr );
        if( SUCCEEDED( ret ) )
        {
            ret = NormalizeIpAddrEx(
                strRawAddr, strIpAddr );
            if( ERROR( ret ) )
            {
                DebugPrintEx( logErr, ret,
                    "Error invalid ip address" );
                break;
            }
        }

        stdstr strPort;
        oCfg.GetStrProp( 110, strPort );

        Json::Value oVal;
        ret = ReadJsonCfgFile( strDriver, oVal );
        if( ERROR( ret ) )
            break;

        Json::Value& oPorts =
            oVal[ JSON_ATTR_PORTS ];

        gint32 i;

        // update ip address
        for( i = 0; i < oPorts.size(); i++ )
        {
            Json::Value& oElem = oPorts[ i ];
            if( oElem == Json::Value::null )
                continue;
            if( !oElem.isObject() ||
                !oElem.isMember( JSON_ATTR_PORTCLASS ) )
                continue;

            Json::Value& oClass =
                oElem[ JSON_ATTR_PORTCLASS ];
            if( oClass.asString() !=
                PORT_CLASS_RPC_TCPBUS )
                continue;

            Json::Value& oParams =
                oElem[ JSON_ATTR_PARAMETERS ];

            if( !oParams.isArray() ||
                !oParams.size() )
            {
                ret = -EINVAL;
            }

            Json::Value& oParam = oParams[ 0 ];
            if( !oParam.isMember( JSON_ATTR_BINDADDR ) ||
                !oParam.isMember( JSON_ATTR_TCPPORT ) )
            {
                ret = -EINVAL;
                break;
            }

            if( strIpAddr.size() )
            {
                oParam[ JSON_ATTR_BINDADDR ] =
                    strIpAddr;
                bChanged = true;
            }

            if( strPort.size() )
            {
                oParam[ JSON_ATTR_TCPPORT ] =
                    strPort;
                bChanged = true;
            }
        }

        if( ERROR( ret ) )
            break;

        if( !bChanged )
            break;

        char szNewDrv[] = "/tmp/rpcfdrv_XXXXXX";
        int iFd = mkstemp( szNewDrv );
        if( iFd < 0 )
        {
            ret = -errno;
            break;
        }
        close( iFd );

        Json::StreamWriterBuilder oBuilder;
        oBuilder["commentStyle"] = "None";
        oBuilder["indentation"] = "    ";
        std::unique_ptr<Json::StreamWriter> pWriter(
            oBuilder.newStreamWriter() );
        std::ofstream ofs;
        ofs.open ( szNewDrv ,
            std::ofstream::out |
            std::ofstream::trunc);
        pWriter->write( oVal, &ofs );
        ofs.close();
        strNewDrv = szNewDrv;

    }while( 0 );

    return ret;
}

}
