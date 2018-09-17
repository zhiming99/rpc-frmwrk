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
 * =====================================================================================
 */

#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <string>
#include "defines.h"
#include "registry.h"
#include "port.h"
#include "frmwrk.h"
#include "tasklets.h"
#include "jsondef.h"

using namespace std;

bool CIoManager::m_bInit( false );

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

        if( bTopmost )
            pPort = pPort->GetTopmostPort();

        pair< PortPtr, IrpCtxPtr >& oStackEntry =
            pIrp->m_vecCtxStack.back();

        // Fix the invalid port ptr with the topmost port
        // on the port object stack
        oStackEntry.first = pPort;
        pPort->AllocIrpCtxExt( oStackEntry.second );


        CStdRMutex a( pIrp->GetLock() );
        ret = pPort->SubmitIrp( pIrp );

        if( ret != STATUS_PENDING )
        {
            // make this irp unaccessable
            pIrp->SetState(
                IRP_STATE_READY, IRP_STATE_COMPLETED );
            pIrp->SetStatus( ret );
        }

    }while( 0 );

    return ret;
}

gint32 CIoManager::SubmitIrp(
    HANDLE hPort, IRP* pIrp, bool bCheckExist )
{
    gint32 ret = 0;

    if( pIrp == nullptr || hPort == 0 )
        return -EINVAL;

    do{
        IPort* pPort = HandleToPort( hPort );
        if( bCheckExist )
        {
            // let's find if the port existing
            ret = PortExist( pPort );
            if( ERROR( ret ) )
                break;
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
        //
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
        vector< pair< PortPtr, IrpCtxPtr > >::iterator itr ;

        if( pIrp->GetStackSize() > 0 )
        {
            itr = pIrp->m_vecCtxStack.end() - 1;
            do{
                if( !itr->first.IsEmpty()  )
                {
                    pIrp->SetCurPos( itr - pIrp->m_vecCtxStack.begin() );
                    ret = itr->first->CompleteIrp( pIrp );
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

            }while( itr-- != pIrp->m_vecCtxStack.begin() );

            // we should not pop out the first io stack
            // because it is owned by the user land
            itr = pIrp->m_vecCtxStack.begin();
            pIrp->SetStatus( itr->second->GetStatus() );
        }
        else
        {
            pIrp->SetStatus( -ENOENT );
        }

        // remove the timer if any
        if( pIrp->HasTimer() )
        {
            GetUtils().GetTimerSvc().RemoveTimer( pIrp->m_iTimerId );
        }

        pIrp->SetState(
            IRP_STATE_COMPLETING, IRP_STATE_COMPLETED );

        if( pIrp->GetMasterIrp() != nullptr )
        {
            a.Unlock();

            this->CompleteAssocIrp( pIrp );
            // we don't care the return value from
            // CompleteAssocIrp

            a.Lock();
            // no one else should touch this irp
            // because it is already marked as completed
        }

        if( pIrp->m_vecCtxStack.size() )
        {
            // we need to remove port reference at the
            // bottom of the port stack
            itr = pIrp->m_vecCtxStack.begin();
            itr->first = nullptr; ;
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
            guint32 dwParam1 =
                ( guint32 )( ( IRP* )pIrp );

            guint32 dwParam2 =
                pIrp->m_dwContext;

            pIrp->m_pCallback->OnEvent(
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
        vector< pair< PortPtr, IrpCtxPtr > >::iterator itr =
            pIrp->m_vecCtxStack.end() - 1;

        if( pIrp->GetStackSize() > 0 )
        {
            bool bFirst = true;
            do{
                if( !itr->first.IsEmpty() )
                {
                    // set the status for the top stack
                    if( bFirst )
                    {
                        itr->second->SetStatus(
                            iRet == 0 ? -ECANCELED : iRet );

                        bFirst = false;
                    }

                    pIrp->SetCurPos( itr - pIrp->m_vecCtxStack.begin() );
                    itr->first->CancelIrp( pIrp, bForce );
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

            }while( itr-- != pIrp->m_vecCtxStack.begin() );

            itr = pIrp->m_vecCtxStack.begin();
            pIrp->SetStatus( itr->second->GetStatus() );
        }
        else
        {
            pIrp->SetStatus( -ECANCELED );
        }

        if( pIrp->GetStackSize() > 0 )
        {
            // we need to remove port reference at the
            // bottom of the port stack
            itr = pIrp->m_vecCtxStack.begin();
            itr->first = nullptr;
        }

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

        if( pIrp->GetMasterIrp() != nullptr )
        {
            a.Unlock();
            ret = CancelAssocIrp( pIrp );
            a.Lock();
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
        if( ret )
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

gint32 CIoManager::ClosePort(
    HANDLE hPort, IEventSink* pEvent )
{
    // Note: we need to make sure all the irps are
    // completed or canceled and the interface
    // unregistered before calling this method
    // otherwise unexpected things would happen
    gint32 ret = 0;
    do{
        if( hPort == 0 || pEvent == nullptr )
        {
            ret = -EINVAL;
            break;
        }
        IPort* pPort = HandleToPort( hPort );
        ret = RemoveFromHandleMap( pPort, pEvent );
    }while( 0 );

    return ret;
}

gint32 CIoManager::OpenPortByRef(
    HANDLE hPort,
    IEventSink* pEventSink )
{
    if( hPort == 0 || pEventSink == nullptr )
        return -EINVAL;

    IPort* pPort = HandleToPort( hPort );
    gint32 ret = PortExist( pPort );

    if( ERROR( ret ) )
        return ret;

    return AddToHandleMap(
        pPort, pEventSink );
}

gint32 CIoManager::GetPortProp(
    HANDLE hPort,
    gint32 iProp,
    BufPtr& pBuf )
{
    IPort* pPort = HandleToPort( hPort );

    if( pPort == nullptr )
        return -EINVAL;

    gint32 ret = PortExist( pPort );
    if( ERROR( ret ) )
        return ret;

    CPort* pCPort =
        static_cast< CPort* >( pPort );

    ret = pCPort->GetProperty( iProp, *pBuf );

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
            else
            {
                CStdRMutex oPortLock( pBus->GetLock() );

                guint32 dwState = pBus->GetPortState();
                if( dwState == PORT_STATE_STARTING )
                {
                    // try it later
                    ret = -EAGAIN;
                    break;
                }
                else if( dwState != PORT_STATE_READY )
                {
                    ret = ERROR_STATE;
                    break;
                }
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

        if( RunningOnMainThread() )
        {
            // we cannot open port on the
            // main IO thread
            ret = ERROR_WRONG_THREAD;
            break;
        }

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
    guint32 dwParam1,
    guint32 dwParam2,
    guint32* pData)
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
                    // notified we can start.  the
                    // sem will be posted after
                    // the mainloop is started,
                    // which is important for
                    // receiving tcp socket events 
                    sem_wait( GetSyncSem() );

                    ret = GetPnpMgr().Start();
                    if( ERROR( ret ) )
                        break;

                    ret = GetDrvMgr().Start();

                    if( ERROR( ret ) )
                        GetPnpMgr().Stop();

                    break;
                }
            default:
                break;
            }
        }
    case eventWorkitemCancel:
        {
            switch( ( EnumEventId )dwParam1 )
            {
            default:
                break;
            }
        }
    default:
        break;
    }
    return ret;
}

CMainIoLoop& CIoManager::GetMainIoLoop() const
{
    return *m_pLoop;
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

bool CIoManager::RunningOnMainThread() const
{
    guint32 dwMainTid =
        GetMainIoLoop().GetThreadId();

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

        m_bInit = true;

        ret = GetUtils().Start();
        if( ERROR( ret ) )
            break;

        GetTaskThreadPool().Start();
        GetIrpThreadPool().Start();

        // the return value is the workitem id, and we
        // don't want it here
        ret = 0;

        // let's enter the main loop. The whole
        // system begin to run
        GetMainIoLoop().Start();

        // this task will be executed in the
        // mainloop
        CCfgOpener a;
        a.SetPointer( propIoMgr, this );

        ScheduleTaskMainLoop(
            clsid( CIoMgrPostStartTask ),
            a.GetCfg() );

        this->OnEvent( eventWorkitem,
            eventStart, 0, nullptr );

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

        pTask->MarkPending();
        GetMainIoLoop().AddTask( pTask );

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

    gint32 iSize =
        ( gint32 )m_vecStandAloneThread.size();

    size_t iStopTask = 0;

    vector< ThreadPtr >::iterator itr =
        m_vecStandAloneThread.begin();

    for( gint32 i = iSize - 1; i >= 0 ; --i )
    {
        if( !m_vecStandAloneThread[ i ]->IsRunning() )
            m_vecStandAloneThread.erase( itr + i );

        CCfgOpenerObj oThrdCfg(
            ( CObjBase* )m_vecStandAloneThread[ i ] );

        if( oThrdCfg.exist( propClsid ) )
        {
            // if there is a task CIoMgrStopTask in the
            // array, we don't need to count that
            // thread
            guint32 dwClsid;
            oThrdCfg.GetIntProp( propClsid, dwClsid );
            if( dwClsid == clsid( CIoMgrStopTask ) )
                iStopTask = 1;
        }
    }

    if( m_vecStandAloneThread.size() > iStopTask )
        return -EEXIST;

    return 0;
}

void CIoManager::WaitThreadsQuit()
{
    while( ClearStandAloneThreads() == -EEXIST )
    {
        sleep( 1 );
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

    ClearStandAloneThreads();

    CStdRMutex oLock( GetLock() );
    m_vecStandAloneThread.push_back( pThread );

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

        if( bOneshot )
        {
            ThreadPtr pThread;

            ret = StartStandAloneThread(
                pThread, iClsid );

            if( ERROR( ret ) )
                break;

            pTask->MarkPending();

            CTaskThread* pTaskThread = pThread;
            pTaskThread->AddTask( pTask );
        }
        else
        {

            ThreadPtr pThread;
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

gint32 CIoManager::RescheduleTask(
    TaskletPtr& pTask )
{
    gint32 ret = 0;

    do{
        if( pTask.IsEmpty() )
        {
            ret = -EINVAL;
            break;
        }

        ThreadPtr pThread;

        ret = GetTaskThreadPool().GetThread( pThread, true );
        if( ERROR( ret ) )
            break;

        CTaskThread* pgth =
            static_cast< CTaskThread* >( pThread );

        if( pgth )
            pgth->AddTask( pTask );

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

gint32 CIoManager::Stop()
{
    gint32 ret = 0;
    CCfgOpener a;
    a.SetPointer( propIoMgr, this );

    ret = ScheduleTask(
        clsid( CIoMgrStopTask ),
        a.GetCfg(),
        true );

    return ret;
}

CIoManager::CIoManager( const std::string& strModName ) :
      m_strModName( strModName )
{
    gint32 ret = 0;
    SetClassId( clsid( CIoManager ) );

    do{
        CParamList a;
        a.SetPointer( propIoMgr, this );
        a.SetStrProp( propConfigPath, string( CONFIG_FILE ) );

        CfgPtr pCfg = a.GetCfg();

        ret = m_pReg.NewObj();
        if( ERROR( ret ) )
        {
            throw std::runtime_error(
                "CRegistry failed to initialize" );
        }

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

        ret = m_pDrvMgr.NewObj(
            clsid( CDriverManager ), pCfg );

        if( ERROR( ret ) )
        {
            throw std::runtime_error(
                "CDriverManager failed to initialize" );
        }

        guint32 dwThreadCount = 1;
        guint32 dwThreadMax = 2;
        a.Push( dwThreadCount );
        a.Push( dwThreadMax );
        a.Push( ( guint32 )clsid( CIrpThreadPool ) );

        ret = m_pIrpThrdPool.NewObj(
            clsid( CIrpThreadPool ), pCfg );
        if( ERROR( ret ) )
        {
            throw std::runtime_error(
                "CIrpThreadPool failed to initialize" );
        }

        a.SetIntProp( 0, dwThreadCount );
        dwThreadMax = 3;
        a.SetIntProp( 1, dwThreadMax );
        a.SetIntProp( 2,
            ( guint32 )clsid( CTaskThreadPool ) );

        ret = m_pTaskThrdPool.NewObj(
            clsid( CTaskThreadPool ), pCfg );
        if( ERROR( ret ) )
        {
            throw std::runtime_error(
                "CTaskThreadPool failed to initialize" );
        }

        Sem_Init( &m_semSync, 0, 0 );

        Json::Value& oCfg = m_pDrvMgr->GetJsonCfg();
        if( oCfg == Json::Value::null )
        {
            throw std::invalid_argument(
                "No config" );
        }
        Json::Value& oArch = oCfg[ JSON_ATTR_ARCH ];
        if( oArch == Json::Value::null )
        {
            throw std::invalid_argument(
                "No Arch Configs" );
        }
        Json::Value& oCores = oArch[ JSON_ATTR_NUM_CORE ];
        if( oCores == Json::Value::null )
        {
            throw std::invalid_argument(
                "No Processors" );
        }

        string strVal = oCores.asString();
        m_dwNumCores = std::strtol(
            strVal.c_str(), nullptr, 10 );

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
    guint32 dwContext,
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
        ret = pMgr->GetMainIoLoop().Stop();
        ret = pMgr->GetDrvMgr().Stop();
        ret = pMgr->GetPnpMgr().Stop();
        ret = pMgr->GetUtils().Stop();
        ret = pMgr->GetIrpThreadPool().Stop();
        ret = pMgr->GetTaskThreadPool().Stop();
    }

    if( dwContext == eventOneShotTaskThrdCtx )
        Sem_Post( &m_semSync );

    return SetError( ret );
}

gint32 CIoMgrPostStartTask::operator()(
    guint32 dwContext )
{
    gint32 ret = 0;
    if( GetConfig().IsEmpty() )
        return SetError( -EFAULT );

    ObjPtr pObj;
    CCfgOpenerObj oCfg( this );
    CIoManager* pMgr = nullptr;
    ret = oCfg.GetPointer( propIoMgr, pMgr );
    if( ERROR( ret ) )
        return ret;

    if( pMgr )
        Sem_Post( pMgr->GetSyncSem() );

    return SetError( ret );
}

