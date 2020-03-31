/*
 * =====================================================================================
 *
 *       Filename:  iftasks.cpp
 *
 *    Description:  implementation of the tasks involved
 *                  in the interface operations
 *
 *        Version:  1.0
 *        Created:  02/12/2017 09:20:55 PM
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

#include <deque>
#include <vector>
#include <string>
#include <semaphore.h>
#include "defines.h"
#include "frmwrk.h"
#include "emaphelp.h"
#include "connhelp.h"
#include "proxy.h"
#include "ifhelper.h"

#define IF_GENERIC_RETIRES         3
#define IF_GENERIC_INTERVAL        60

using namespace std;

CIfStartRecvMsgTask::CIfStartRecvMsgTask(
    const IConfigDb* pCfg ) :
    super( pCfg )
{
    gint32 ret = 0;

    CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
    // parameter in the config is
    // propIfPtr: the pointer the interface object
    SetClassId( clsid( CIfStartRecvMsgTask ) );

    // retry after one second
    ret = oCfg.SetIntProp( propIntervalSec, 1 ); 
    if( ERROR( ret ) )
    {
        string strMsg = DebugMsg( ret,
            "Error in CIfStartRecvMsgTask ctor" );
        throw std::runtime_error( strMsg );
    }
}

template< typename T1, typename T >
gint32 CIfStartRecvMsgTask::HandleIncomingMsg(
    ObjPtr& ifPtr, T1& pMsg )
{
    gint32 ret = 0;

    // servicing the incoming request/event
    // message
    do{
        CRpcServices* pIf = ifPtr;

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

        ret = pTask.NewObj(
            clsid( CIfInvokeMethodTask ),
            oParams.GetCfg() );
        if( ERROR( ret ) )
            break;

        // put the InvokeMethod in a managed
        // environment instead of letting it
        // running untracked
        ret = pIf->AddAndRun( pTask );
        if( ERROR( ret ) )
            break;

        ret = pTask->GetError();
        if( ret == STATUS_PENDING )
        {
            // CIfStartRecvMsgTask is done, the
            // CIfInvokeMethodTask will take over
            ret = 0;
        }

    }while( 0 );

    return ret; 
}

gint32 CIfStartRecvMsgTask::StartNewRecv(
    ObjPtr& pCfg )
{
    // NOTE: no touch of *this, so no locking
    gint32 ret = 0;
    if( pCfg.IsEmpty() )
        return -EINVAL;

    CCfgOpener oCfg( ( IConfigDb* )pCfg );

    CRpcServices* pIf = nullptr;
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
            clsid( CIfStartRecvMsgTask ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

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

        // succeeded, let's repeat by spawning
        // new CIfStartRecvMsgTask for the
        // next pending message in the port
        if( ret == STATUS_PENDING )
            break;

        if( ERROR( ret ) )
        {
            DebugPrint( ret,
                "The new RecvMsgTask failed" );
            break;
        }
        // succeeded, issue the next recv req

   }while( 1 );

   return ret;
}

gint32 CIfStartRecvMsgTask::OnIrpComplete(
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
            ret = DEFER_CALL( pMgr, ObjPtr( this ),
                &CIfStartRecvMsgTask::StartNewRecv,
                pObj );

        }while( 0 );

        if( ERROR( ret ) )
            break;

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

            HandleIncomingMsg( pObj, pMsg );
        }
        else
        {
            CfgPtr pCfg;
            ret = pCtx->GetRespAsCfg( pCfg );
            if( ERROR( ret ) )
                break;

            HandleIncomingMsg( pObj, pCfg );
        }

    }while( 0 );

    // this task is retired
    return 0;
}

gint32 CIfStartRecvMsgTask::RunTask()
{
    gint32 ret = 0;
    do{
        ObjPtr pObj;
        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );
        
        CRpcInterfaceBase* pIf = nullptr;
        ret = oCfg.GetPointer( propIfPtr, pIf );
        if( ERROR( ret ) )
            break;

        CMessageMatch* pMatch = nullptr;
        ret = oCfg.GetPointer(
            propMatchPtr, pMatch );

        if( ERROR( ret ) )
            break;

        CCfgOpenerObj oMatch( pMatch );
        bool bDummy = false;

        ret = oMatch.GetBoolProp(
            propDummyMatch, bDummy );

        if( ERROR( ret ) )
            bDummy = false;

        if( bDummy )
            break;

        if( !pIf->IsConnected() )
        {
            ret = ERROR_STATE;
            break;
        }
        ret = pIf->StartRecvMsg( this, pMatch );

    }while( 0 );

    return ret;
}

gint32 CIfStartRecvMsgTask::OnCancel(
    guint32 dwContext )
{
    return super::OnCancel( dwContext );
}

CIfRetryTask::CIfRetryTask(
    const IConfigDb* pCfg ) :
    super( pCfg )
{
    gint32 ret = 0;
    Sem_Init( &m_semWait, 0, 0 );
    SetError( STATUS_PENDING );

    do{
        CCfgOpener oCfg( ( IConfigDb* )GetConfig() );

        CIoManager* pMgr = nullptr;
        ret = GET_IOMGR( oCfg, pMgr );
        if( ERROR( ret ) )
            break;

        if( !oCfg.exist( propIoMgr ) )
            oCfg.SetPointer( propIoMgr, pMgr );

        if( !oCfg.exist( propRetries ) )
        {
            ret = oCfg.SetIntProp(
                propRetries, IF_GENERIC_RETIRES );
        }
        if( ERROR( ret ) )
            break;

        if( !oCfg.exist( propIntervalSec ) )
        {
            ret = oCfg.SetIntProp(
                propIntervalSec, IF_GENERIC_INTERVAL );
        }
        if( ERROR( ret ) )
            break;

    }while( 0 );

    if( ERROR( ret ) )
    {
        string strMsg = DebugMsg(
            ret, "Error occurs in CIfRetryTask ctor" );
        SetError( ret );
        throw std::runtime_error( strMsg );
    }
}

gint32 CIfRetryTask::OnEvent(
    EnumEventId iEvent,
    LONGWORD dwParam1,
    LONGWORD dwParam2,
    LONGWORD* pData )
{ 
    gint32 iPropId = 0;

    // FIXME: this ease the issue of paramlist race
    // condition, but not fixed it
    switch( iEvent & ~eventTryLock )
    {
    case eventKeepAlive:
        {
            iPropId = propKAParamList;
            break;
        }
    case eventTimeout:
        {
            iPropId = propTimerParamList;
            break;
        }
    default:
        {
            iPropId = propParamList;
            break;
        }
    }

    LwVecPtr pVec( true );

    ( *pVec )().push_back( iEvent );
    ( *pVec )().push_back( dwParam1 );
    ( *pVec )().push_back( dwParam2 );
    ( *pVec )().push_back( ( LONGWORD )pData );

    CStdRTMutex oTaskLock( GetLock() );
    CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
    oCfg.SetObjPtr( iPropId, ObjPtr( pVec ) );
    oTaskLock.Unlock();

    ( *this )( iEvent );

    oTaskLock.Lock();
    oCfg.RemoveProperty( iPropId );

    return GetError();
}

template< class ClassName >
gint32 CIfRetryTask::DelayRun(
    guint32 dwSecsDelay,
    EnumEventId iEvent,
    LONGWORD dwParam1,
    LONGWORD dwParam2,
    LONGWORD* pdata )
{
    // some thread is working on this object
    // let's do it later
    CIoManager* pMgr = nullptr;
    CCfgOpenerObj oCfg( this );
    gint32 ret = GET_IOMGR( oCfg, pMgr );
    if( ERROR( ret ) )
        return ret;

    if( dwSecsDelay > 0 )
    {
        ret = DEFER_CALL_DELAY( pMgr,
            dwSecsDelay,
            ObjPtr( this ),
            &ClassName::OnEvent,
            iEvent, dwParam1, dwParam2, pdata );
    }
    else
    {
        ret = DEFER_CALL( pMgr,
            ObjPtr( this ),
            &ClassName::OnEvent,
            iEvent, dwParam1, dwParam2, pdata );
    }

    return ret;
}

gint32 CIfRetryTask::OnRetry()
{
    DebugPrint( 0, "Retrying task %s @0x%x",
        CoGetClassName( GetClsid() ),
        ( intptr_t )this );
    return RunTask();
}

gint32 CIfRetryTask::OnCancel(
    guint32 dwContext )
{
    super::OnCancel( dwContext );
    return 0;
}

gint32 CIfRetryTask::OnIrpComplete(
    IRP* pIrp )
{
    gint32 ret = 0;

    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
    {
        ret = -EFAULT;
    }
    else
    {
        ret = pIrp->GetStatus();
        if( Retriable( ret ) )
            ret = STATUS_MORE_PROCESS_NEEDED;
        else if( ret == -EAGAIN )
            ret = ERROR_FAIL;
    }

    return ret;
}

gint32 CIfRetryTask::RunTask() 
{
    return 0;
}

gint32 CIfRetryTask::OnComplete(
    gint32 iRetVal )
{
    gint32 ret = 0;
    
    CCfgOpener oParams(
        ( IConfigDb* )GetConfig() );

    do{
        ObjPtr pObj;
        bool bNotify = false;

        ret = oParams.GetBoolProp(
            propNotifyClient, bNotify );

        if( ERROR( ret ) )
        {
            bNotify = false;
            ret = 0;
        }

        if( bNotify && IsPending() )
        {
            ret = oParams.GetObjPtr(
                propEventSink, pObj );

            if( SUCCEEDED( ret ) )
            {
                IEventSink* pEvent = pObj;
                if( pEvent == nullptr )
                {
                    ret = -EFAULT;
                    break;
                }

                CObjBase* pObjBase = this;
                LONGWORD* pData =
                    ( LONGWORD* ) pObjBase;
                pEvent->OnEvent( eventTaskComp,
                    iRetVal, 0, pData );
                
            }
            else
            { 
                // a synchronous call, trigger a
                // semaphore
                Sem_Post( &m_semWait );
            }
        }

        if( !m_pParentTask.IsEmpty() )
        {
            ObjPtr pObj( m_pParentTask );
            CIfTaskGroup* pTaskGrp = static_cast
                < CIfTaskGroup* >( m_pParentTask );

            m_pParentTask.Clear();
            if( pTaskGrp != nullptr )
            {
                pTaskGrp->OnChildComplete(
                    iRetVal, this );
                break;
            }
        }

    }while( 0 );

    // clear all the objPtr we used
    RemoveProperty( propIfPtr );
    RemoveProperty( propEventSink );
    RemoveProperty( propNotifyClient );
    RemoveProperty( propIrpPtr );

    // NOTE: Moved setting task status to the
    // bottom due to a concurrent bug, when the
    // client caller is expecting the
    // synchronization of get/set response to take
    // effect in the CIoReqSyncCallback's task
    // complete handler(TCH). But the whole things
    // happens in an unexpected sequence that the
    // client caller could possibly get the return
    // code even after this OnComplete of
    // CIfIoReqTask is entered, but before the TCH
    // ( the sync point) is called. ( Usually the
    // client caller should get a STATUS_PENDING
    // very quickly before the server side enters
    // OnComplete.)
    //
    // If the task status is set at the very
    // beginning of this method, the client caller
    // may not wait for the sync signal due to the
    // task has a non-pending return code, and
    // proceed to handle its request as an
    // immediate successful req instead, while the
    // response is yet to set in the upcoming TCH
    // call. Thus the client code may grab an
    // empty response sometimes, in
    // CInterfaceProxy::SyncCallEx.
    //
    // This situation usually happens when the
    // client and server are both in a single
    // process
    // 
    SetError( iRetVal );
    return ret; 
}

gint32 CIfRetryTask::Process( guint32 dwContext )
{
    gint32 ret = 0;
    EnumEventId iEvent = ( EnumEventId )dwContext;
    if( this->GetTid() != ::GetTid() )
        MarkPending();

    do{
        switch( iEvent )
        {
        case eventZero:
            // the task is run manually
        case eventTaskThrdCtx:
            // the task is run from the task
            // thread
        case eventOneShotTaskThrdCtx:
            // the task is run from a one-shot
            // task thread
            {
                ret = RunTask();
                break;
            }
        case eventTaskComp:
            {
                vector< LONGWORD > vecParams;
                ret = GetParamList( vecParams );
                if( ERROR( ret ) )
                    break;

                guint32 dwContext = vecParams[ 1 ];
                ret = OnTaskComplete( dwContext );
                break;
            }
        case eventIrpComp:
            {   
                IrpPtr pIrp;

                ret = GetIrpFromParams( pIrp );
                if( ERROR( ret ) )
                    break;

                ret = OnIrpComplete( pIrp );

                break;
            }
        case eventTimeout:
            {
                vector< LONGWORD > vecParams;
                ret = GetParamList( vecParams,
                    propTimerParamList );

                if( ERROR( ret ) )
                    break;

                if( vecParams[ 1 ] == eventRetry )
                {
                    ret = OnRetry();
                }
                break;
            }
        case eventTimeoutCancel:
            {   
                OnCancel( iEvent );
                ret = ERROR_TIMEOUT;
                break;
            }
        case eventCancelTask:
            {
                ret = OnCancel( iEvent );
                if( ret != STATUS_PENDING )
                    ret = ERROR_CANCEL;
                break;
            }
        case eventUserCancel:
            {
                OnCancel( iEvent );
                ret = ERROR_USER_CANCEL;
                break;
            }
        default:
            {
                ret = -ENOTSUP;
                break;
            }
        }

        if( ret == STATUS_PENDING )
        {
            break;
        }
        else if( Retriable( ret ) )
        {
            ret = STATUS_MORE_PROCESS_NEEDED;
            break;
        }
        else if( ret == -EAGAIN )
            ret = ERROR_FAIL;

        OnComplete( ret );
        break;

    }while( 0 );

    if( ret == STATUS_PENDING ||
        ret == STATUS_MORE_PROCESS_NEEDED )
        MarkPending();

    return ret;
}

gint32 CIfRetryTask::WaitForComplete()
{
    // NOTE: please call this method from outside
    // the rpc-frmwrk's threads for performance
    // concerns
    gint32 ret = 0; 

    // warning: no locking here
    CCfgOpener oCfg( ( IConfigDb* )GetConfig() );

    guint32 dwInterval = 0;

    ret = oCfg.GetIntProp(
        propIntervalSec, dwInterval );

    if( ERROR( ret ) )
        return ret;

    if( dwInterval > 100000 )
        return -EINVAL;

    do{
        ret = Sem_TimedwaitSec( &m_semWait, dwInterval );

        if( ret == -EAGAIN )
            continue;

        // whether succeeded or error, end the loop
        break;

    }while( 1 );

    return ret;
}

gint32 CIfRetryTask::GetProperty( gint32 iProp,
        CBuffer& oBuf ) const
{
    gint32 ret = 0;
    switch( iProp )
    {
    case propParentTask:
        {
            if( m_pParentTask.IsEmpty() )
            {
                ret = -ENOENT;
                break;
            }
            oBuf = ObjPtr( m_pParentTask );
            break;
        }
    default:
        {
            ret = super::GetProperty(
                iProp, oBuf );
                break;
        }
    }

    return ret;
}
gint32 CIfRetryTask::SetProperty(
    gint32 iProp, const CBuffer& oBuf )
{
    gint32 ret = 0;
    switch( iProp )
    {
    case propParentTask:
        {
            ObjPtr& pObj = oBuf;
            if( pObj.IsEmpty() )
                m_pParentTask.Clear();
            else
                m_pParentTask = pObj;
            break;
        }
    default:
        {
            ret = super::SetProperty(
                iProp, oBuf );
                break;
        }
    }

    return ret;
}
gint32 CIfRetryTask::RemoveProperty(
    gint32 iProp )
{
    gint32 ret = 0;
    switch( iProp )
    {
    case propParentTask:
        {
            m_pParentTask.Clear();
            break;
        }
    default:
        {
            ret = super::RemoveProperty( iProp );
                break;
        }
    }

    return ret;
}

gint32 CIfRetryTask::EnumProperties(
    std::vector< gint32 >& vecProps ) const
{
    gint32 ret = 0;
    ret = super::EnumProperties( vecProps );
    if( ERROR( ret ) )
        return ret;

    vecProps.push_back( propParentTask );

    return ret;
}

gint32 CIfRetryTask::GetPropertyType(
    gint32 iProp, gint32& iType ) const
{
    gint32 ret = 0;
    switch( iProp )
    {
    case propParentTask:
        {
            iType = typeObj;
            break;
        }
    default:
        {
            ret = super::GetPropertyType(
                iProp, iType );
                break;
        }
    }

    return ret;
}

CIfEnableEventTask::CIfEnableEventTask(
    const IConfigDb* pCfg ) :
        super( pCfg )
{
    gint32 ret = 0;
    SetClassId( clsid( CIfEnableEventTask ) );

    CParamList oParams(
        ( IConfigDb* )GetConfig() );
    do{
        ret = oParams.SetIntProp(
            propRetries,
            IF_GENERIC_RETIRES ); 

        if( ERROR( ret ) )
            break;

        ret = oParams.SetIntProp(
            propIntervalSec,
            IF_GENERIC_INTERVAL ); 

        if( ERROR( ret ) )
            break;

    }while( 0 );

    if( ERROR( ret ) )
    {
        string strMsg = DebugMsg( ret,
            "Error in CIfEnableEventTask ctor" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CIfEnableEventTask::OnIrpComplete(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;

    do{
        CParamList oParams(
            ( IConfigDb* )GetConfig() );

        CRpcInterfaceBase* pIf = nullptr;

        ret = oParams.GetPointer(
            propIfPtr, pIf );

        if( unlikely( pIf == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }

        ret = pIrp->GetStatus();
        if( unlikely( ret == -ENOTCONN ) )
        {
            ret = pIf->SetStateOnEvent(
                eventModOffline );
        }
        else if( Retriable( ret ) )
        {
            ret = STATUS_MORE_PROCESS_NEEDED;
            break;
        }
        else if( ret == -EAGAIN )
            ret = ERROR_FAIL;

    }while( 0 );

    return ret;
}

gint32 CIfEnableEventTask::RunTask()
{
    gint32 ret = 0;
    
    do{
        CParamList oParams(
            ( IConfigDb* )GetConfig() );

        ObjPtr pObj;
        ret = oParams.GetObjPtr( propIfPtr, pObj );

        if( ERROR( ret ) )
            break;

        CRpcInterfaceBase* pIf = pObj;
        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        bool bEnable = true;
        bool bDummy = false;

        // note: we do not use pop, because we want to
        // retry when failed
        ret = oParams.GetBoolProp( 0, bEnable );
        if( ERROR( ret ) )
            break;

        IMessageMatch* pMatch = nullptr;

        ret = oParams.GetPointer(
            propMatchPtr, pMatch );

        if( ERROR( ret ) )
            break;

        CCfgOpenerObj oMatch( pMatch );
        ret = oMatch.GetBoolProp(
            propDummyMatch, bDummy );

        if( ERROR( ret ) )
            bDummy = false;
    
        if( !bDummy )
        {
            if( bEnable )
            {
                ret = pIf->EnableEvent(
                    pMatch, this );
            }
            else
            {
                ret = pIf->DisableEvent(
                    pMatch, this );
            }
        }
        else
        {
            // this match is for state transition only
            ret = 0;
        }

        if( ret == -EAGAIN )
        {
            ret = ERROR_FAIL;
            if( CanRetry() )
                ret = STATUS_MORE_PROCESS_NEEDED;
            break;
        }
        else if( ret == -ENOTCONN )
        {
            // note, we don't assume ENOTCONN as an
            // error and the match is valid and
            // the communication channel is kept.
            ret = pIf->SetStateOnEvent(
                eventModOffline );
        }

    }while( 0 );

    return ret;
}

CIfOpenPortTask::CIfOpenPortTask(
    const IConfigDb* pCfg )
    : super( pCfg )
{
    SetClassId( clsid( CIfOpenPortTask ) );
}

gint32 CIfOpenPortTask::OnIrpComplete(
    PIRP pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;

    do{
        ObjPtr pObj;
        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );        

        ret = oCfg.GetObjPtr( propIfPtr, pObj );
        if( ERROR( ret ) )
            break;

        CRpcInterfaceBase* pIf = pObj;
        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        HANDLE hPort = INVALID_HANDLE;

        gint32 iRet = pIrp->GetStatus();
        IrpCtxPtr& pCtx = pIrp->GetTopStack();

        if( ERROR( iRet ) )
        {
            ret = iRet;
            if( !Retriable( ret ) )
            {
                EnumEventId iEvent = eventPortStartFailed;
                if( !pCtx->m_pRespData.IsEmpty() )
                {
                    hPort = ( HANDLE& )*pCtx->m_pRespData;
                }
                pIf->OnPortEvent( iEvent, hPort );
            }
            else 
            {
                pIf->SetStateOnEvent( eventRetry );
                if( ret == -EAGAIN )
                    ret = STATUS_MORE_PROCESS_NEEDED;
            }
        }
        else
        {
            EnumEventId iEvent = eventPortStarted;
            hPort = ( HANDLE& )*pCtx->m_pRespData;
            ret = pIf->OnPortEvent( iEvent, hPort );
        }

    }while( 0 );
    
    return ret;
}

gint32 CIfOpenPortTask::RunTask()
{
    gint32 ret = 0;
    
    do{
        ObjPtr pObj;
        CParamList oParams(
            ( IConfigDb* )GetConfig() );

        ret = oParams.GetObjPtr(
            propIfPtr, pObj );

        if( ERROR( ret ) )
            break;

        CRpcInterfaceBase* pIf = pObj;
        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ret = pIf->OpenPort( this );
        if( Retriable( ret ) )
        {
            pIf->SetStateOnEvent( eventRetry );
            ret = STATUS_MORE_PROCESS_NEEDED;
            break;
        }
        else if( ret == -EAGAIN )
            ret = ERROR_FAIL;
        // otherwise, just quit with error.

    }while( 0 );

    return ret;
}

void CIfTaskGroup::PopTask()
{
    if( !m_queTasks.empty() )
        m_queTasks.pop_front();
    else
    {
        DebugPrint( -ERANGE, "no more emelemt" );
    }
}

bool CIfTaskGroup::IsCanceling() const
{
    if( GetConfig()->exist( propCanceling ) )
        return true;

    return false;
}

void CIfTaskGroup::SetCanceling( bool bCancel )
{
    CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
    if( bCancel )
    {
        oCfg[ propCanceling ] = true;
        oCfg[ propCancelTid ] = GetTid();
    }
    else
    {
        oCfg.RemoveProperty( propCanceling );
        oCfg.RemoveProperty( propCancelTid );
    }
}

gint32 CIfTaskGroup::InsertTask(
    TaskletPtr& pTask )
{
    if( pTask.IsEmpty() )
        return -EINVAL;

    CStdRTMutex oTaskLock( GetLock() );

    if( IsCanceling() )
        return ERROR_STATE;

    if( IsRunning() )
        return ERROR_STATE;

    CCfgOpenerObj oCfg( ( CObjBase* )pTask );

    gint32 ret = oCfg.SetObjPtr(
        propParentTask, ObjPtr( this ) );

    if( ERROR( ret ) )
        return ret;

    m_queTasks.push_front( pTask );

    return 0;
}

gint32 CIfTaskGroup::AppendTask(
    TaskletPtr& pTask )
{
    if( pTask.IsEmpty() )
        return -EINVAL;

    CStdRTMutex oTaskLock( GetLock() );

    CCfgOpenerObj oCfg( ( CObjBase* )pTask );

    gint32 ret = oCfg.SetObjPtr(
        propParentTask, ObjPtr( this ) );

    if( ERROR( ret ) )
        return ret;

    if( IsCanceling() )
        return ERROR_STATE;

    // canceling or cleanup is going on
    EnumTaskState iStat = GetTaskState();

    if( unlikely( iStat == stateStopped ) )
        return ERROR_STATE;

    if( iStat == stateStarted && !IsRunning() )
        return ERROR_STATE;

    m_queTasks.push_back( pTask );

    return 0;
}

gint32 CIfTaskGroup::AppendAndRun(
    TaskletPtr& pTask )
{
    if( pTask.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;
    bool bImmediate = false;

    CStdRTMutex oTaskLock( GetLock() );
    do{
        ret = AppendTask( pTask );
        if( ERROR( ret ) )
            break;

        if( GetTaskCount() == 1 )
        {
            bImmediate = true;
            m_vecRetVals.clear();
        }

    }while( 0 );

    if( SUCCEEDED( ret ) &&
        bImmediate == true )
    {
        oTaskLock.Unlock();
        ( *this )( eventZero );
    }
    else if( SUCCEEDED( ret ) )
    {
        // there is some other tasks running
        pTask->MarkPending();
    }
    
    return ret;
}

gint32 CIfTaskGroup::CancelRemainingTasks()
{
    gint32 ret = 0;

    CStdRTMutex oTaskLock( GetLock() );
    CCfgOpener oCfg( ( IConfigDb* )GetConfig() );

    if( IsCanceling() )
        return STATUS_PENDING;

    if( oCfg.exist( propNoResched ) )
        return STATUS_PENDING;
    SetCanceling( true );
    oCfg[ propContext ] = eventCancelTask;
    oCfg[ propNoResched ] = true;

    do{
        TaskletPtr pTask;
        if( m_queTasks.empty() )
            break;

        pTask = m_queTasks.front();

        // release the lock to avoid nested chain of
        // locks along the taskgroup tree
        oTaskLock.Unlock();

        ( *pTask )( eventCancelTask );

        oTaskLock.Lock();

    }while( 1 );

    oCfg.RemoveProperty( propNoResched );
    oCfg.RemoveProperty( propContext );
    SetCanceling( false );

    return ret;
}

gint32 CIfTaskGroup::OnRetry()
{
    if( true )
    {
        // clear the return values if any on retry
        CStdRTMutex oTaskLock( GetLock() );
        m_vecRetVals.clear();
    }
    return RunTask();
}

bool CIfTaskGroup::IsRunning() const
{
    CStdRTMutex oTaskLock( GetLock() );
    return GetConfig()->exist( propRunning );
}

void CIfTaskGroup::SetRunning( bool bRunning )
{
    CStdRTMutex oTaskLock( GetLock() );
    CCfgOpener oCfg(
        ( IConfigDb* ) GetConfig() );
    if( bRunning )
    {
        if( !oCfg.exist( propRunning ) )
            oCfg[ propRunning ] = true;
    }
    else
    {
        oCfg.RemoveProperty( propRunning );
    }
    return;
}

gint32 CIfTaskGroup::RunTask()
{
    // So far, except the first call of RunTask is
    // from AddAndRun, AppendAndRun or direct
    // call, the rest RunTask has to be called
    // from the OnChildComplete.

    gint32 ret = 0;
    guint32 dwContext = eventZero;

    CStdRTMutex oTaskLock( GetLock() );

    if( IsCanceling() )
    {
        // we are coming here probably because the
        // ::Process is prempted by the canceling
        // call
        return STATUS_PENDING;
    }

    CCfgOpener oCfg(
        ( IConfigDb* )GetConfig() );

    if( oCfg.exist( propNoResched ) )
        return STATUS_PENDING;

    EnumTaskState iState = GetTaskState();
    if( iState == stateStarting )
    {
        SetTaskState( stateStarted );
        SetRunning();
    }
    else if( iState == stateStopped )
    {
        return ERROR_STATE;
    }
    else if( !IsRunning() &&
        iState == stateStarted )
    {
        // the taskgroup is about to complete
        return ERROR_STATE;
    }

    if( !m_vecRetVals.empty() )
    {
        // last task's return value
        gint32 iRet = m_vecRetVals.back();

        if( ERROR( iRet ) &&
            GetRelation() == logicAND )
        {
            SetRunning( false );
            oTaskLock.Unlock();
            ret = CancelRemainingTasks();
            if( ret == STATUS_PENDING )
                return ret;
            return iRet;
        }

        // using not ERROR instead of SUCCEEDED in
        // case iRet contains some information
        // other than error besides success
        if( !ERROR( iRet ) &&
            GetRelation() == logicOR )
        {
            SetRunning( false );
            oTaskLock.Unlock();
            ret = CancelRemainingTasks();
            if( ret == STATUS_PENDING )
                return ret;
            return iRet;
        }
        
        // in case queue is empty or relation is
        // logicNONE
        ret = iRet;
    }

    while( !m_queTasks.empty() )
    {
        TaskletPtr pTask;

        if( IsCanceling() )
        {
            // we are coming here because the Process()
            // is prempted by the canceling call
            ret = STATUS_PENDING;
            break;
        }

        pTask = m_queTasks.front();
        if( pTask.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        CCfgOpenerObj oChildCfg(
            ( CObjBase* )pTask );

        ObjPtr pTaskObj;
        ret = oChildCfg.GetObjPtr(
            propParentTask, pTaskObj );

        if( ERROR( ret ) )
        {
            // if no parent task, the taskgroup
            // cannot move forward
            m_queTasks.pop_front();
            continue;
        }

        // tell this->OnChildComplete not to reschedule
        // this taskgrp because it is running
        oCfg[ propNoResched ] = true;

        // release the lock to avoid nested chain of
        // locking
        oTaskLock.Unlock();

        ( *pTask )( dwContext );
        ret = pTask->GetError();

        oTaskLock.Lock();
        oCfg.RemoveProperty( propNoResched );

        if( unlikely( ret ==
            STATUS_MORE_PROCESS_NEEDED ) )
        {
            // retry will happen on the child task,
            // let's pending
            // Taskgroup will not return
            // STATUS_MORE_PROCESS_NEEDED
            ret = STATUS_PENDING;
        }
        if( ret == STATUS_PENDING )
            break;

        if( ERROR( ret ) &&
            GetRelation() == logicAND )
        {
            // no need to go further if AND
            // relationship between tasks
            //
            // all the remaining tasks in this
            // task group will be canceled
            if( m_queTasks.size() > 0 )
            {
                SetRunning( false );
                oTaskLock.Unlock();
                gint32 iRet = CancelRemainingTasks();
                if( iRet == STATUS_PENDING )
                    ret = iRet;
            }
            break;
        }

        if( !ERROR( ret ) &&
            GetRelation() == logicOR )
        {
            if( m_queTasks.size() > 0 )
            {
                SetRunning( false );
                oTaskLock.Unlock();
                gint32 iRet = CancelRemainingTasks();
                if( iRet == STATUS_PENDING )
                    ret = iRet;
            }
            break;
        }
    }

    if( ret != STATUS_PENDING )
    {
        // for those Unlock'd condition, this call
        // does not have effect
        SetRunning( false );
    }

    return ret;
}

gint32 CIfTaskGroup::OnChildComplete(
    gint32 iRet, CTasklet* pChild )
{
    gint32 ret = 0;
    if( pChild == nullptr )
        return -EINVAL;

    do{
        bool bSync = false;

        CStdRTMutex oTaskLock( GetLock() );

        CfgPtr pCfg = GetConfig();
        CCfgOpener oCfg( ( IConfigDb* )pCfg );

        if( IsCanceling() )
        {
            guint32 dwTid = oCfg[ propCancelTid ];
            if( dwTid != GetTid() )
            {
                // an active canceling is going on from
                // anther thread while an async task
                // completion happens
                ret = ERROR_STATE;
                break;
            }
            else
            {
                // we are within the canceling process
                // just move on
            }
        }

        TaskletPtr pHead = m_queTasks.front();
        if( pHead->GetObjId() != pChild->GetObjId() )
        {
            // Possibly racing in this function between
            // a canceling thread and an async task
            // completion.
            // or a task in the queue is timeout before
            // get chance to run.
            return -ENOENT;
        }

        if( pCfg->exist( propNoResched ) )
            bSync = true;

        PopTask();
        m_vecRetVals.push_back( iRet );
        if( iRet == STATUS_PENDING )
        {
#ifdef DEBUG
            DebugPrint( iRet, "a task in quitting abnormally..." );
            std::string strDump;
            pHead->Dump( strDump );
            DebugPrint( ret, "for class %s", strDump.c_str() );
#endif
        }

        if( bSync )
            break;

        CIoManager* pMgr = nullptr;

        ret = GET_IOMGR( oCfg, pMgr );
        if( ERROR( ret ) )
            break;

        oTaskLock.Unlock();

        // DebugPrint( ( intptr_t )this, "Reschedule TaskGroup" );
        // regain control by rescheduling this task
        TaskletPtr pThisTask( this );
        ret = pMgr->RescheduleTask( pThisTask );

    }while( 0 );

    return ret;
}

gint32 CIfTaskGroup::Process(
    guint32 dwContext )
{
    gint32 ret = 0;

    {
        CStdRTMutex oTaskLock( GetLock() );
        CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
        guint32 dwCtx = 0;

        ret = oCfg.GetIntProp(
            propContext, dwCtx );

        // override the original dwContext if there is
        // a propContext property in the config
        if( SUCCEEDED( ret ) )
            dwContext = dwCtx;
    }

    ret = super::Process( dwContext );

    return ret;
}

// wait to cancel the running child task
gint32 CIfTaskGroup::WaitingToCancel(
    TaskletPtr& pTask )
{
    gint32 ret = 0;
    if( pTask.IsEmpty() )
        return -EINVAL;

    do{
        // FIXME: we are stucking here
        gint32 iRetry = 3;
        while( pTask->IsInProcess() && iRetry > 0 )
        {
            // wait for 3ms before canceling
            usleep( 1000 );
            iRetry--;
        }

        if( pTask->GetError() ==
            STATUS_PENDING )
        {
            ( *pTask )( eventCancelTask );
        }

    }while( 0 );

    return ret;
}

gint32 CIfTaskGroup::OnCancel(
    guint32 dwContext )
{
    do{
        CStdRTMutex oTaskLock( GetLock() );
        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );

        if( GetTaskState() == stateStopped )
            return ERROR_STATE;

        if( IsCanceling() )
        {
            // reentrancy, possibly a reschedule
            // happens while canceling starts
            return STATUS_PENDING;
        }

        if( oCfg.exist( propNoResched ) )
        {
            // some thread is working on this object
            // let's do it later
            return STATUS_PENDING;
        }

        if( m_queTasks.empty() )
            break;

        SetCanceling( true );

        // NOTE: by setting propContext to
        // eventCancelTask, RunTask will not be entered
        // again
        oCfg[ propContext ] = eventCancelTask;
        oCfg[ propNoResched ] = true;

        TaskletPtr pHead;
        if( m_queTasks.front()->IsInProcess() )
        {
            pHead = m_queTasks.front();
            PopTask();
        }

        do{
            TaskletPtr pTask;
            if( m_queTasks.empty() )
                break;

            pTask = m_queTasks.front();

            // release the lock to avoid nested chain of
            // locks along the taskgroup tree
            oTaskLock.Unlock();

            ( *pTask )( eventCancelTask );

            oTaskLock.Lock();

        }while( 1 );

        if( !pHead.IsEmpty() )
        {
            m_queTasks.push_back( pHead );
            oTaskLock.Unlock();

            WaitingToCancel( pHead );

            oTaskLock.Lock();
            if( m_queTasks.size() )
            {
                if( m_queTasks.back() == pHead )
                    m_queTasks.pop_back();
            }
        }

        oCfg.RemoveProperty( propNoResched );
        oCfg.RemoveProperty( propContext );

        SetCanceling( false );
        SetRunning( false );
        break;

    }while( 1 );

    return -ECANCELED;
}

gint32 CIfTaskGroup::OnComplete( gint32 iRet )
{
    gint32 ret = super::OnComplete( iRet );
    SetTaskState( stateStopped );
    return ret;
}

bool CIfTaskGroup::exist( TaskletPtr& pTask )
{
    CStdRTMutex oTaskLock( GetLock() );

    deque< TaskletPtr >::iterator itr;
    itr = std::find( m_queTasks.begin(),
        m_queTasks.end(), pTask );

    if( itr == m_queTasks.end() )
        return false;

    return true;
}

gint32 CIfTaskGroup::RemoveTask(
    TaskletPtr& pTask )
{
    CStdRTMutex oTaskLock( GetLock() );

    deque< TaskletPtr >::iterator itr;
    itr = std::find( m_queTasks.begin(),
        m_queTasks.end(), pTask );

    if( itr != m_queTasks.end() )
    {
        m_queTasks.erase( itr );
        return 0;
    }
    return -ENOENT;
}

gint32 CIfTaskGroup::FindTaskByRmtId(
    guint64 iTaskId, TaskletPtr& pRet )
{
    gint32 ret = -ENOENT;

    CStdRTMutex oTaskLock( GetLock() );
    for( auto&& pTask : m_queTasks )
    {
        CCfgOpenerObj oCfg( ( CObjBase* )pTask );
        if( pTask->GetClsid() ==
            clsid( CIfInvokeMethodTask ) )
        {
            guint64 qwRmtId = 0;

            ret = oCfg.GetQwordProp(
                propRmtTaskId, qwRmtId );

            if( ERROR( ret ) )
                continue;

            if( qwRmtId == iTaskId )
            {
                pRet = pTask;
                break;
            }
            continue;
        }
        
        CIfTaskGroup* pGrp = pTask;
        if( pGrp != nullptr )
        {
            ret = pGrp->FindTaskByRmtId(
                iTaskId, pRet );

            if( SUCCEEDED( ret ) )
                break;
        }
    }
    return ret;
}

gint32 CIfTaskGroup::FindTask(
    guint64 iTaskId, TaskletPtr& pRet )
{
    gint32 ret = -ENOENT;

    CStdRTMutex oTaskLock( GetLock() );
    for( auto&& pTask : m_queTasks )
    {
        if( pTask->GetObjId() == iTaskId )
        {
            pRet = pTask;
            ret = 0;
            break;
        }

        CIfTaskGroup* pGrp = pTask;
        if( pGrp != nullptr )
        {
            ret = pGrp->FindTask( iTaskId, pRet );
            if( SUCCEEDED( ret ) )
                break;
        }
    }
    return ret;
}

gint32 CIfTaskGroup::FindTaskByClsid(
    EnumClsid iClsid,
    std::vector< TaskletPtr >& vecTasks )
{
    if( iClsid == clsid( Invalid ) )
        return -EINVAL;

    bool bFound = false;
    CStdRTMutex oTaskLock( GetLock() );
    for( auto elem : m_queTasks )
    {
        if( iClsid == elem->GetClsid() )
        {
            vecTasks.push_back( elem );
            bFound = true;
        }
    }

    if( !bFound )
        return -ENOENT;

    return 0;
}

gint32 CIfTaskGroup::GetHeadTask(
    TaskletPtr& pHead )
{
    CStdRTMutex oTaskLock( GetLock() );
    if( m_queTasks.empty() )
        return -ENOENT;
    pHead = m_queTasks.front();
    return 0;
}

gint32 CIfTaskGroup::GetTailTask(
    TaskletPtr& pTail )
{
    CStdRTMutex oTaskLock( GetLock() );
    if( m_queTasks.empty() )
        return -ENOENT;

    pTail = m_queTasks.back();
    return 0;
}

gint32 CIfRootTaskGroup::OnComplete(
    gint32 iRetVal )
{
    SetRunning( true );
    return 0;
}

gint32 CIfRootTaskGroup::OnChildComplete(
    gint32 iRet, CTasklet* pChild )
{
    gint32 ret = 0;
    if( pChild == nullptr )
        return -EINVAL;

    do{
        bool bSync = false;

        CStdRTMutex oTaskLock( GetLock() );

        CfgPtr pCfg = GetConfig();
        CCfgOpener oCfg(
            ( IConfigDb* )pCfg );

        if( IsCanceling() )
        {
            guint32 dwTid = oCfg[ propCancelTid ];
            if( dwTid != GetTid() )
            {
                // an active canceling is going on from
                // anther thread while an async task
                // completion happens
                ret = ERROR_STATE;
                break;
            }
            else
            {
                // we are within the canceling process
                // just move on
            }
        }

        TaskletPtr pHead = m_queTasks.front();
        if( pHead->GetObjId() != pChild->GetObjId() )
        {
            // Possibly racing in this function between
            // a canceling thread and an async task
            // completion.
            return -ENOENT;
        }

        if( pCfg->exist( propNoResched ) )
            bSync = true;

        PopTask();
        m_vecRetVals.push_back( iRet );

        if( bSync )
            break;

        // don't reschedule on empty task queue,
        // otherwise, the child task/taskgroup
        // could be run twice in the extreme
        // situation.
        if( m_queTasks.empty() )
        {
            // there could be too many ret values
            // if we don't clean up
            m_vecRetVals.clear();

            // no need to reschedule
            break;
        }

        CIoManager* pMgr = nullptr;
        ret = GET_IOMGR( oCfg, pMgr );
        if( ERROR( ret ) )
            break;

        oTaskLock.Unlock();
        // regain control by rescheduling this task
        TaskletPtr pThisTask( this );
        ret = pMgr->RescheduleTask( pThisTask );

    }while( 0 );

    return ret;
}

gint32 CIfStopTask::OnIrpComplete(
    PIRP pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 iRet = pIrp->GetStatus();
    gint32 ret = 0;

    do{
        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );

        CRpcInterfaceBase* pIf = nullptr;
        ret = oCfg.GetPointer( propIfPtr, pIf );
        if( ERROR( ret ) )
            break;

        pIf->OnPortEvent( eventPortStopped,
            pIf->GetPortHandle() );

    }while( 0 );

    return iRet;
}

gint32 CIfStopTask::RunTask()
{
    gint32 ret = 0;

    do{
        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );

        CRpcInterfaceBase* pIf = nullptr;
        ret = oCfg.GetPointer( propIfPtr, pIf );
        if( ERROR( ret ) )
            break;

        if( unlikely( pIf == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }

        if( pIf->GetState() == stateStopped )
        {
            ret = ERROR_STATE;
            break;
        }

#ifdef DEBUG
        guint32 dwServer = 0;
        if( pIf->IsServer() )
            dwServer = 1;

        DebugPrint( dwServer,
            "About to close port, %d", GetTid() );

#endif
        ret = pIf->ClosePort( this );
        if( ret == STATUS_PENDING )
            break;

        pIf->OnPortEvent( eventPortStopped,
            pIf->GetPortHandle() );

    }while( 0 );

    return ret;
}

gint32 CIfStopRecvMsgTask::RunTask()
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
        ObjPtr pObj;
        
        ret = oCfg.GetObjPtr( propIfPtr, pObj );

        if( ERROR( ret ) )
            break;

        CRpcInterfaceBase* pIf = pObj;

        CIoManager* pMgr = pIf->GetIoMgr();
        if( pMgr == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        ret = pIf->CancelRecvMsg();

    }while( 0 );

    return ret;
}

gint32 CIfPauseResumeTask::RunTask()
{
    gint32 ret = 0;
    do{
        CParamList oParams(
            ( IConfigDb* )GetConfig() );
        ObjPtr pObj;
        
        ret = oParams.GetObjPtr(
            propIfPtr, pObj );

        if( ERROR( ret ) )
            break;

        CRpcInterfaceBase* pIf = pObj;

        CIoManager* pMgr = pIf->GetIoMgr();
        if( pMgr == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        IEventSink* pCallback = nullptr;

        ret = oParams.GetObjPtr(
            propEventSink, pObj );

        if( SUCCEEDED( ret ) )
            pCallback = pObj;


        bool bResume = false;

        ret = oParams.GetBoolProp( 0, bResume );
        if( ERROR( ret ) )
            break;

        if( bResume )
        {
            ret = pIf->DoResume( pCallback );
        }
        else
        {
            ret = pIf->DoPause( pCallback );
        }

    }while( 0 );

    return ret;
}

CIfCpEventTask::CIfCpEventTask(
    const IConfigDb* pCfg )
    : super( pCfg )
{
    SetClassId( clsid( CIfCpEventTask ) );
}

gint32 CIfCpEventTask::OnIrpComplete(
    PIRP pIrp )
{
    if( pIrp == nullptr )
        return -EINVAL;

    return 0;
}

gint32 CIfCpEventTask::RunTask()
{

    gint32 ret = 0;
    do{

        CParamList oParams(
           ( IConfigDb* )GetConfig() ); 

        EnumEventId iEvent;

        guint32 iVal = 0;
        ret = oParams.GetIntProp( 0, iVal );

        if( ERROR( ret ) )
            break;
        iEvent = ( EnumEventId )iVal;

        ObjPtr pObj;
        ret = oParams.GetObjPtr(
            propIfPtr, pObj );

        if( ERROR( ret ) )
            break;

        CRpcBaseOperations* pIf = pObj;
        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        switch( iEvent )
        {
        case eventModOffline:
        case eventModOnline:
            {
                string strModName;
                ret = oParams.GetStrProp(
                    1, strModName );

                if( ERROR( ret ) )
                    break;

                ret = pIf->DoModEvent(
                    iEvent, strModName );
                break;

            }
        case eventRmtModOffline:
        case eventRmtModOnline:
            {
                string strModName;
                ret = oParams.GetStrProp(
                    1, strModName );

                if( ERROR( ret ) )
                    break;

                IConfigDb* pEvtCtx = nullptr;
                ret = oParams.GetPointer(
                    2, pEvtCtx );

                if( ERROR( ret ) )
                    break;

                ret = pIf->DoRmtModEvent(
                    iEvent, strModName, pEvtCtx );
                break;
            }
        default:
            ret = -ENOTSUP;
            break;
        }

    }while( 0 );

    return ret;
}

gint32 CIfParallelTaskGrp::RemoveTask(
    TaskletPtr& pTask )
{
    gint32 ret = 0;

    ret = m_setTasks.erase( pTask );
    if( ret == 0 )
        ret = m_setPendingTasks.erase( pTask );

    if( ret == 0 )
        return -ENOENT;

    return 0;
}

gint32 CIfParallelTaskGrp::OnChildComplete(
    gint32 iRet, CTasklet* pChild )
{
    gint32 ret = 0;

    if( pChild == nullptr )
        return -EINVAL;

    do{
        CStdRTMutex oTaskLock( GetLock() );

        CfgPtr pCfg = GetConfig();
        CCfgOpener oCfg( ( IConfigDb* )pCfg );

        if( IsCanceling() )
        {
            guint32 dwTid = oCfg[ propCancelTid ];
            if( dwTid != GetTid() )
            {
                // an active canceling is going on from
                // anther thread
                ret = ERROR_STATE;
                break;
            }
            else
            {
                // we are within the canceling process
                // just move on
            }
        }

        TaskletPtr taskPtr = pChild;
        RemoveTask( taskPtr );

        if( pCfg->exist( propNoResched ) )
        {
            ret = 0;
            break;
        }

        if( GetPendingCount() == 0 &&
            GetTaskCount() > 0 )
        {
            // NOTE: this check will effectively reduce
            // the possibility RunTask to run on two
            // thread at the same time, and lower the
            // probability get blocked by reentrance lock
            // no need to reschedule
            ret = 0;
            break;
        }

        CIoManager* pMgr = nullptr;

        ret = GET_IOMGR( oCfg, pMgr );
        if( ERROR( ret ) )
            break;

        // regain control by rescheduling this
        // task
        TaskletPtr pThisTask( this );
        ret = pMgr->RescheduleTask( pThisTask );

    }while( 0 );

    return ret;
}

gint32 CIfParallelTaskGrp::RunTask()
{
    gint32 ret = 0;

    CStdRTMutex oTaskLock( GetLock() );
    if( IsCanceling() )
    {
        // canceling is in process, we just quit
        // quietly
        return STATUS_PENDING;
    }

    CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
    // some other guy is processing the tasks
    if( oCfg.exist( propNoResched ) )
        return STATUS_PENDING;

    oCfg[ propNoResched ] = true;
    do{
        std::set< TaskletPtr > setTasksToRun;
        EnumTaskState iState = GetTaskState();
        if( iState == stateStarting )
        {
            SetTaskState( stateStarted );
            SetRunning();
        }
        else if( iState == stateStopped )
        {
            oCfg.RemoveProperty( propNoResched );
            ret = ERROR_STATE;
            break;
        }
        else if( !IsRunning() &&
            iState == stateStarted )
        {
            oCfg.RemoveProperty( propNoResched );
            ret = ERROR_STATE;
            break;
        }

        setTasksToRun.clear();
        setTasksToRun = m_setPendingTasks;

        m_setTasks.insert(
            m_setPendingTasks.begin(),
            m_setPendingTasks.end() );

        m_setPendingTasks.clear();
        oTaskLock.Unlock();

        for( auto pTask : setTasksToRun )
        {
            if( !pTask.IsEmpty() )
                ( *pTask )( eventZero );
        }

        oTaskLock.Lock();
        if( IsCanceling() )
        {
            oCfg.RemoveProperty( propNoResched );
            SetRunning( false );
            return STATUS_PENDING;
        }

        // new tasks come in
        if( GetPendingCount() > 0 )
            continue;

        ret = oCfg.RemoveProperty( propNoResched );
        // pending means there are tasks running, does
        // not indicate a specific task's return value
        if( GetTaskCount() > 0 )
            ret = STATUS_PENDING;

        break;

    }while( 1 );

    if( ret != STATUS_PENDING )
        SetRunning( false );

    // we will be removed from the root task
    // group
    return ret;
}

gint32 CIfParallelTaskGrp::RunTaskDirect(
    TaskletPtr& pTask )
{
    if( pTask.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;

    do{
        CStdRTMutex oTaskLock( GetLock() );
        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );

        ObjPtr pIf;
        ret = oCfg.GetObjPtr( propIfPtr, pIf );
        if( ERROR( ret ) )
            break;

        CRpcServices* pService = pIf;
        if( pService == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        if( IsCanceling() )
        {
            ret = ERROR_STATE;
            break;
        }

        EnumTaskState iState = GetTaskState();
        if( iState == stateStopped )
        {
            ret = ERROR_STATE;
            break;
        }

        bool bRunning = IsRunning();
        if( !bRunning && iState == stateStarted )
        {
            // the taskgroup is about to complete
            ret = ERROR_STATE;
            break;
        }

        CCfgOpenerObj oChildCfg(
            ( CObjBase* )pTask );

        oChildCfg.SetObjPtr(
            propParentTask, ObjPtr( this ) );

        CIfRetryTask* pRetryTask = pTask;
        if( pRetryTask == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        if( !bRunning )
            m_setPendingTasks.insert( pTask );
        else
            m_setTasks.insert( pTask );

        oTaskLock.Unlock();

        if( bRunning )
        {
            // run this task immediately
            ( *pTask )( eventZero );
            break;
        }
        else
        {
            ret = STATUS_PENDING;
        }

    }while( 0 );

    return ret;
}

gint32 CIfParallelTaskGrp::AddAndRun(
    TaskletPtr& pTask )
{
    if( pTask.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;

    do{
        CStdRTMutex oTaskLock( GetLock() );
        CCfgOpener oCfg( ( IConfigDb* )GetConfig() );

        ObjPtr pIf;
        ret = oCfg.GetObjPtr( propIfPtr, pIf );
        if( ERROR( ret ) )
            break;

        CRpcServices* pService = pIf;
        if( pService == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        CIoManager* pMgr = pService->GetIoMgr();

        if( IsCanceling() )
        {
            ret = ERROR_STATE;
            break;
        }

        bool bRunning = IsRunning();

        CCfgOpenerObj oChildCfg(
            ( CObjBase* )pTask );

        oChildCfg.SetObjPtr(
            propParentTask, ObjPtr( this ) );

        m_setPendingTasks.insert( pTask );
        oTaskLock.Unlock();

        if( bRunning )
        {
            // reschedule this task immediately
            ret = ( *this )( eventZero );

            if( ret == -EDEADLK )
            {
                TaskletPtr pThisGrp( this );
                pTask->MarkPending();
                pMgr->RescheduleTask( pThisGrp );
                ret = STATUS_PENDING;
            }
            break;
        }
        else
        {
            pTask->MarkPending();
            ret = STATUS_PENDING;
        }

    }while( 0 );

    return 0;
}

gint32 CIfParallelTaskGrp::AppendTask(
    TaskletPtr& pTask )
{
    if( pTask.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;

    do{
        CStdRTMutex oTaskLock( GetLock() );
        if( IsCanceling() )
        {
            ret = ERROR_STATE;
            break;
        }
        EnumTaskState iState = GetTaskState();
        if( iState == stateStopped )
        {
            ret = ERROR_STATE;
            break;
        }
        if( !IsRunning() && iState == stateStarted )
        {
            ret = ERROR_STATE;
            break;
        }

        CCfgOpenerObj oChildCfg(
            ( CObjBase* )pTask );

        oChildCfg.SetObjPtr(
            propParentTask, ObjPtr( this ) );

        m_setPendingTasks.insert( pTask );

    }while( 0 );

    return ret;
}

gint32 CIfParallelTaskGrp::FindTask(
    guint64 iTaskId, TaskletPtr& pRet )
{
    gint32 ret = -ENOENT;

    CStdRTMutex oTaskLock( GetLock() );

    for( auto&& pTask : m_setTasks )
    {
        if( pTask->GetObjId() == iTaskId )
        {
            pRet = pTask;
            ret = 0;
            break;
        }

        CIfTaskGroup* pGrp = pTask;
        if( pGrp != nullptr )
        {
            ret = pGrp->FindTask( iTaskId, pRet );
            if( SUCCEEDED( ret ) )
                break;
        }
    }

    for( auto&& pTask : m_setPendingTasks )
    {
        if( pTask->GetObjId() == iTaskId )
        {
            pRet = pTask;
            ret = 0;
            break;
        }

        CIfTaskGroup* pGrp = pTask;
        if( pGrp != nullptr )
        {
            ret = pGrp->FindTask( iTaskId, pRet );
            if( SUCCEEDED( ret ) )
                break;
        }
    }
    return ret;
}

gint32 CIfParallelTaskGrp::FindTaskByRmtId(
    guint64 iTaskId, TaskletPtr& pRet )
{
    gint32 ret = 0;

    gint32 i = 0;
    bool bFound = false;

    CStdRTMutex oTaskLock( GetLock() );
    std::set< TaskletPtr >* psetTasks = &m_setTasks;
    do{
        for( auto&& pTask : *psetTasks )
        {
            const IConfigDb* pTaskCfg =
                ( IConfigDb* )*pTask;

            if( pTaskCfg == nullptr )
                continue;

            if( pTask->GetClsid() ==
                clsid( CIfInvokeMethodTask ) )
            {
                CCfgOpener oCfg( pTaskCfg );
                guint64 qwRmtId = 0;

                ret = oCfg.GetQwordProp(
                    propRmtTaskId, qwRmtId );

                if( ERROR( ret ) )
                    continue;

                if( qwRmtId == iTaskId )
                {
                    pRet = pTask;
                    bFound = true;
                    break;
                }
                continue;
            }
            
            CCfgOpener oCfg( pTaskCfg );
            CIfTaskGroup* pGrp = pTask;
            if( pGrp != nullptr )
            {
                ret = pGrp->FindTaskByRmtId(
                    iTaskId, pRet );

                if( SUCCEEDED( ret ) )
                {
                    bFound = true;
                    break;
                }
            }
        }

        if( bFound )
            break;
        ++i;
        psetTasks = &m_setPendingTasks;

    }while( i < 2 );

    return ret;
}

gint32 CIfParallelTaskGrp::FindTaskByClsid(
    EnumClsid iClsid,
    std::vector< TaskletPtr >& vecTasks )
{
    if( iClsid == clsid( Invalid ) )
        return -EINVAL;

    bool bFound = false;
    CStdRTMutex oTaskLock( GetLock() );
    for( auto elem : m_setTasks )
    {
        if( iClsid == elem->GetClsid() )
        {
            vecTasks.push_back( elem );
            bFound = true;
        }
    }
    for( auto elem : m_setPendingTasks )
    {
        if( iClsid == elem->GetClsid() )
        {
            vecTasks.push_back( elem );
            bFound = true;
        }
    }
    if( !bFound )
        return -ENOENT;

    return 0;
}

gint32 CIfParallelTaskGrp::OnCancel(
    guint32 dwContext )
{
    gint32 ret = 0;
    CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
    dwContext = eventCancelTask;

    CStdRTMutex oTaskLock( GetLock() );
    if( IsCanceling() )
        return STATUS_PENDING;

    if( oCfg.exist( propNoResched ) )
    {
        // some thread is working on this object
        // let's do it later
        return STATUS_PENDING;
    }

    SetCanceling( true );
    oCfg[ propNoResched ] = true;

    do{
        std::vector< TaskletPtr > vecTasks;

        // when canceling, no re-schedule of this
        // task on child task completion
        if( m_setPendingTasks.size() > 0 )
        {
            vecTasks.insert(
                vecTasks.end(),
                m_setPendingTasks.begin(),
                m_setPendingTasks.end() );

            m_setPendingTasks.clear();
        }

        if( m_setTasks.size() > 0 )
        {
            vecTasks.insert(
                vecTasks.end(),
                m_setTasks.begin(),
                m_setTasks.end() );

            m_setTasks.clear();
        }

        oTaskLock.Unlock();

        for( auto elem : vecTasks )
        {
            TaskletPtr& pTask = elem;
            CIfParallelTask* pParaTask = pTask;

            if( pParaTask != nullptr )
            {
                // for parallel task
                ( *pTask )( eventTryLock | dwContext );
            }
            else
            {
                ( *pTask )( dwContext );
            }
        }

        // call the super class to cleanup
        // super::OnCancel( dwContext );
        oTaskLock.Lock();

    }while( 0 );
    RemoveProperty( propNoResched );
    SetCanceling( false );
    SetRunning( false );

    return ret;
}

gint32 CIfCleanupTask::OnIrpComplete(
    PIRP pIrp )
{
    gint32 ret = 0;
    do{
        ret = RunTask();

    }while( 0 );

    return ret;
}

gint32 CIfCleanupTask::RunTask()
{
    
    gint32 ret = 0;
    do{
        CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
        ObjPtr pObj;
        
        ret = oCfg.GetObjPtr( propIfPtr, pObj );

        if( ERROR( ret ) )
            break;

        CRpcServices* pIf = pObj;

        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ret = pIf->OnPostStop( this );

    }while( 0 );

    return ret;
}

gint32 CIfCleanupTask::OnTaskComplete(
    gint32 iRetVal )
{
    return RunTask();
}

CIfParallelTask::CIfParallelTask(
    const IConfigDb* pCfg ) :
    super( pCfg ),
    m_iTaskState( stateStarting )
{

    // parameter in the config is
    // propIfPtr: the pointer the interface object
    // or propIoMgr, either must exist
    gint32 ret = 0;
    CCfgOpener oCfg( ( IConfigDb* )GetConfig() );

    // retry after one second
    ret = oCfg.SetIntProp( propIntervalSec, 3 ); 

    if( ERROR( ret ) )
    {
        string strMsg = DebugMsg( ret,
            "Error in CIfParallelTask ctor" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CIfParallelTask::OnEvent(
    EnumEventId iEvent,
    LONGWORD dwParam1,
    LONGWORD dwParam2,
    LONGWORD* pData )
{

    // Lock to prevent re-entrance 
    gint32 ret = 0;
    try{
        CStdRTMutex oTaskLock( GetLock() );
        ret = super::OnEvent(
            iEvent, dwParam1, dwParam2, pData );
    }
    catch( std::runtime_error& e )
    {
        // lock failed
        ret = -EACCES;
    }
    return ret;
}

/**
* @name CIfParallelTask::operator()
* the main entery for a CIfParallelTask task. if
* you returns from RunTask with the
* STATUS_PENDING, you should be aware that the
* OnCancel could be called sometime when the
* timeout, or sys admin event can come to cancel
* the task. if RunTask is a synchronous call, you
* don't bother, because the oncancel has not
* chance to execute before RunTask returns
* @{ */
/**  @} */

gint32 CIfParallelTask::operator()(
    guint32 dwContext )
{
    // Lock to prevent re-entrance 
    guint32 dwTryMs = 0;
    gint32 ret = 0;

    if( dwContext & eventTryLock )
    {
        // wait 1 second before lock failed
        dwTryMs = 1000;
        dwContext &= ~eventTryLock;
    }

    try{
        CStdRTMutex oTaskLock( GetLock(), dwTryMs );

        if( GetTaskState() == stateStopped )
            return ERROR_STATE;

        if( GetTaskState() == stateStarting )
        {
            switch( dwContext )
            {
            case eventZero:
            case eventTaskThrdCtx:
            case eventOneShotTaskThrdCtx:
            case eventIrpComp:
            case eventTimeout:
                {
                    SetTaskState( stateStarted );
                    break;
                }
            case eventTaskComp:
            case eventCancelTask:
            case eventTimeoutCancel:
            case eventUserCancel:
            default:
                {
                    break;
                }
            }
        }
        ret = super::operator()( dwContext );
    }
    catch( std::runtime_error& e )
    {
        // lock failed
        ret = -EACCES;
    }
    return ret;
}

gint32 CIfParallelTask::OnComplete(
    gint32 iRet )
{
    gint32 ret = super::OnComplete( iRet );
    SetTaskState( stateStopped );
    return ret;
}

gint32 CIfParallelTask::OnCancel(
    guint32 dwContext )
{
    if( GetTaskState() == stateStopped )
        return 0;

    CancelIrp();
    super::OnCancel( dwContext );
    return 0;
}

gint32 CIfParallelTask::CancelIrp()
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
        ObjPtr pObj;
        ret = oCfg.GetObjPtr( propIrpPtr, pObj );
        if( SUCCEEDED( ret ) )
        {
            RemoveProperty( propIrpPtr );

            IrpPtr pIrp = pObj;
            if( pIrp.IsEmpty()
                || pIrp->GetStackSize() == 0 )
            {
                ret = -EFAULT;
                break;
            }

            CIoManager* pMgr = nullptr;
            ret = GET_IOMGR( oCfg, pMgr );
            if( ERROR( ret ) )
                break;

            ret = pMgr->CancelIrp( pIrp );
        }

    }while( 0 );

    return ret;
}


gint32 CIfParallelTask::SetReqCall(
    IConfigDb* pReq )
{
    CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
    return oCfg.SetObjPtr(
        propReqPtr, ObjPtr( pReq ) );
}

gint32 CIfParallelTask::SetRespData(
    IConfigDb* pResp )
{
    CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
    CfgPtr pCfg;
    if( pResp == nullptr )
    {
        pCfg.NewObj();
    }
    else
    {
        pCfg = pResp;
    }
    return oCfg.SetObjPtr( propRespPtr,
        ObjPtr( ( IConfigDb* )pCfg ) );
}

gint32 CIfParallelTask::GetReqCall(
    CfgPtr& pReq )
{
    CCfgOpener oCfg( ( IConfigDb* )GetConfig() );

    ObjPtr pObj;
    gint32 ret = oCfg.GetObjPtr(
        propReqPtr, pObj );

    if( ERROR( ret ) )
        return ret;

    pReq = pObj;
    if( pReq.IsEmpty() )
        return -EFAULT;

    return 0;
}

gint32 CIfParallelTask::GetRespData(
    CfgPtr& pResp )
{
    CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
    ObjPtr pObj;
    gint32 ret = oCfg.GetObjPtr(
        propRespPtr, pObj );

    if( ERROR( ret ) )
        return ret;

    pResp = pObj;
    if( pResp.IsEmpty() )
        return -EFAULT;

    return 0;
}

gint32 CIfParallelTask::Process(
    guint32 dwContext )
{
    gint32 ret = 0;
    switch( dwContext )
    {
    case eventKeepAlive:
        {
            // handle eventKeepAlive from a task
            // call
            ret = OnKeepAlive( eventKeepAlive );
            break;
        }
    case eventTimeout:
        {
            // handle eventKeepAlive on timeout
            // timer expired
            vector< LONGWORD > vecParams;

            ret = GetParamList( vecParams,
                propTimerParamList );

            if( ERROR( ret ) )
                break;

            if( vecParams.size() < 2 )
            {
                ret = -EINVAL;
                break;
            }

            if( vecParams[ 1 ] == eventKeepAlive )
            {
                ret = OnKeepAlive( eventTimeout );
                break;
            }
            else if( vecParams[ 1 ] == eventTimeoutCancel )
            {
                // redirect to CIfRetryTask::Process
                dwContext = vecParams[ 1 ];
            }
            ret = -ENOTSUP;
            break;
        }
    default:
        {
            ret = -ENOTSUP;
            break;
        }
    }

    if( ret == -ENOTSUP )
    {
        return super::Process( dwContext );
    }

    do{
        if( ret == STATUS_PENDING )
        {
            break;
        }
        else if( Retriable( ret ) )
        {
            ret = STATUS_MORE_PROCESS_NEEDED;
            break;
        }
        else if( ret == -EAGAIN )
            ret = ERROR_FAIL;

        OnComplete( ret );

    }while( 0 );

    if( ret == STATUS_PENDING ||
        ret == STATUS_MORE_PROCESS_NEEDED )
        MarkPending();

    return ret;
}

gint32 CIfParallelTask::GetProperty( gint32 iProp,
        CBuffer& oBuf ) const
{
    CStdRTMutex oTaskLock( GetLock() );
    if( iProp == propTaskState )
    {
        oBuf = GetTaskState();
        return 0;
    }

    gint32 ret = GetConfig()->GetProperty(
        iProp, oBuf );

    if( SUCCEEDED( ret ) )
        return ret;

    return super::GetProperty( iProp, oBuf );
}

gint32 CIfParallelTask::SetProperty( gint32 iProp,
    const CBuffer& oBuf )
{
    CStdRTMutex oTaskLock( GetLock() );
    if( iProp == propTaskState )
    {
        guint32 dwState = oBuf;
        SetTaskState( ( EnumTaskState )dwState );
        return 0;
    }
    return super::SetProperty( iProp, oBuf );
}

gint32 CIfParallelTask::RemoveProperty( gint32 iProp )
{
    if( iProp == propTaskState )
        return -ENOTSUP;

    CStdRTMutex oTaskLock( GetLock() );
    return super::RemoveProperty( iProp );
}

gint32 CIfParallelTask::EnumProperties(
    std::vector< gint32 >& vecProps ) const
{
    vecProps.push_back( propTaskState );
    CStdRTMutex oTaskLock( GetLock() );
    return super::EnumProperties( vecProps );
}

gint32 CIfParallelTask::GetPropertyType(
    gint32 iProp, gint32& iType ) const
{
    if( iProp == propTaskState )
    {
        iType = typeUInt32;
        return 0;
    }
    CStdRTMutex oTaskLock( GetLock() );
    return super::GetPropertyType( iProp, iType );
}

/**
* @name SubmitIoRequest: build the dbus message
* with the pReq and send an irp down to the
* transport layer. and the return values are
* returned in pResp. it has special processing for
* SendData and FetchData, which does not require a
* pEventSink as callback if it is asynchronous.
* instead, the callback is OnSendDataComplete and
* OnFetchDataComplete for proxy interface.
*
* params:
*   pReq: contains all the necessary input
*   parameters for the server to handle the
*   request.
*
*   the properties in this package includes
* 
*   propReturnValue: the int value as the method
*   return. ( only in pResp )
*
*   property 0..n: the parameters of the method to
*   call ( both in the pReq and pResp )
*
*   propMethodName: the method name to call
*
*   propCallOptions: the config db ptr for all the
*   options, or extra informations for this call
*
*       propCallFlags: the options for this method
*           call
*
*           the flags are:
*               keep-alive
*               with-reply
*               sync/async
*               msg-type: DBUS_MESSAGE_TYPE_XXX
*
*       propKeepAliveSec: if the keep-alive is
*       set, the heartbeat will be sent at the
*       rate of half of this value. ( in pReq only
*       )
*
*       propTimeoutSec: the seconds before the
*       request is canceled. it is effective for
*       asynchronous service request, that is the
*       RunTask returns STATUS_PENDING on return.
*
*   propEventSink: the pointer to the IEventSink
*   as the callback from the user. if it does not
*   exist, the call is a synchronous call and the
*   semaphore will be posted at the end of the
*   method call, whether completed or canceled.(
*   only in pReq )
*
*   propNotifyClient: the bool value to indicate
*   whether to notify the caller of this request
*   ( only in pReq )
*
*   propIpAddr: the remote server's ip address if
*   this interface is a remote proxy client.(
*   optional) ( in pReq, it is the destination ip
*   address, and in pResp, it is the source ip
*   address )
*
*   propMsgPtr: the pointer to req message if the
*   request is to send a response message.
*
*   propRespPtr:
*       the pointer to receive the response data
*
*   propReqPtr:
*       the pointer to the io request information
*   
* @{ */
/**  @} */

gint32 CIfIoReqTask::SubmitIoRequest()
{
    gint32 ret = 0;

    do{
        CCfgOpener oCfg( ( IConfigDb* )GetConfig() );

        CfgPtr pReq;
        ret = GetReqCall( pReq );
        if( ERROR( ret ) )
            break;

        CfgPtr pResp;
        ret = GetRespData( pResp );
        if( ERROR( ret ) )
            break;

        if( pResp.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        ObjPtr pObj;
        CRpcServices* pIf = nullptr;

        ret = oCfg.GetObjPtr( propIfPtr, pObj );
        if( ERROR( ret ) )
            break;

        pIf = pObj;
        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        ret = pIf->SendMethodCall(
            pReq, pResp, this );

        if( Retriable( ret ) )
            ret = STATUS_MORE_PROCESS_NEEDED;
        else if( ret == -EAGAIN )
            ret = ERROR_FAIL;

    }while( 0 );

    return ret;
}

gint32 CIfIoReqTask::OnCancel(
    guint32 dwContext )
{
    if( GetTaskState() != stateStarted )
        return 0;

    gint32 ret = 0;

    do{
        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );
        CRpcServices* pIf = nullptr;
        
        ObjPtr pObj;
        ret = oCfg.GetObjPtr( propIfPtr, pObj );
        if( ERROR( ret ) )
            break;

        pIf = pObj;

        CfgPtr pResp;
        ret = GetRespData( pResp );
        if( ERROR( ret ) )
            break;

        if( pIf == nullptr || pResp.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        ret = pIf->OnCancel(
            ( IConfigDb* )pResp, this );

    }while( 0 );

    super::OnCancel( dwContext );

    return ret;
}

gint32 CIfIoReqTask::FilterMessageSend( 
    IConfigDb* pReqMsg )
{
    gint32 ret = 0;

    if( pReqMsg == nullptr )
        return -EINVAL;

    do{
        bool bSend = true;

        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );

        CRpcServices* pIf = nullptr;
        
        ObjPtr pObj;
        ret = oCfg.GetObjPtr( propIfPtr, pObj );
        if( ERROR( ret ) )
            break;

        pIf = pObj;

        // filter the message
        if( pIf != nullptr )
        {
            IEventSink* pFilter = nullptr;
            std::vector< LONGWORD > vecParams;

            ret = GetParamList( vecParams );
            if( ERROR( ret ) )
            {
                ret = 0;
            }
            else if( vecParams[ 0 ] == eventFilterComp )
            {
                guint32 dwCount = GetArgCount(
                    &IEventSink::OnEvent );

                if( vecParams.size() >= dwCount )
                {
                    ret = vecParams[ 1 ];
                    CObjBase* pObj =
                        ( CObjBase* )vecParams[ 3 ];
                    pFilter = static_cast
                        < IEventSink* >( pObj );
                }
            }
            else
            {
                // maybe a retry happens, not our call
                ret = 0;
                break;
            }

            // the result of the last filter
            if( ret == ERROR_PREMATURE )
                bSend = false;

            if( bSend )
            {
                ret = pIf->FilterMessage( this,
                    pReqMsg, false, pFilter ); 
            }

            // filter is still working
            if( ret == STATUS_PENDING )
                break;

            if( ERROR( ret ) )
                ret = ERROR_PREMATURE;
        }

    }while( 0 );

    return ret;
}

gint32 CIfIoReqTask::Process(
    guint32 dwContext )
{
    gint32 ret = 0;
    switch( dwContext )
    {
    case eventFilterComp:
        {
            ret = OnFilterComp();
            break;
        }
    default:
        {
            ret = -ENOTSUP;
            break;
        }
    }

    if( ret == -ENOTSUP )
    {
        return super::Process( dwContext );
    }

    do{
        if( ret == STATUS_PENDING )
        {
            break;
        }
        else if( Retriable( ret ) )
        {
            ret = STATUS_MORE_PROCESS_NEEDED;
            break;
        }
        else if( ret == -EAGAIN )
            ret = ERROR_FAIL;

        OnComplete( ret );

    }while( 0 );

    if( ret == STATUS_PENDING ||
        ret == STATUS_MORE_PROCESS_NEEDED )
        MarkPending();

    return ret;
}

gint32 CIfIoReqTask::OnFilterComp()
{
    gint32 ret = 0;

    do{
        vector< LONGWORD > vecParams;
        ret = GetParamList( vecParams );
        if( ERROR( ret ) )
            break;

        gint32 iRet = ( LONGWORD )vecParams[ 1 ];

        if( iRet == ERROR_PREMATURE )
        {
            ret = iRet;
            break;
        }

        if( ERROR( iRet ) )
        {
            CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );

            ObjPtr pObj;
            ret = oCfg.GetObjPtr( propMsgPtr, pObj );
            if( ERROR( ret ) )
                break;

            IConfigDb* pMsg = pObj;
            if( pMsg == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            CObjBase* pObjbase =
                ( CObjBase* )vecParams[ 3 ];

            IEventSink* pFilterTask = static_cast
                < IEventSink* >( pObjbase );

            IMessageFilter* pMsgFilt = dynamic_cast
                < IMessageFilter* >( pFilterTask );
            
            if( pMsgFilt == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            EnumStrategy iStrat = stratIgnore;
            pMsgFilt->GetFilterStrategy(
                this, pMsg, iStrat );

            if( iStrat == stratIgnore )
            {
                ret = 0;
            }
            else
            {
                ret = ERROR_PREMATURE;
                break;
            }
        }

        // continue to the task
        ret = RunTask();

    }while( 0 );

    if( ret != STATUS_PENDING )
        OnTaskComplete( ret );

    return ret;
}

gint32 CIfIoReqTask::RunTask()
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );

        CfgPtr pReq;
        ret = GetReqCall( pReq );
        if( ERROR( ret ) )
            break;

        ret = FilterMessageSend( pReq );
        if( ret == STATUS_PENDING )
            break;

        if( ERROR( ret ) )
            break;

        ret = SubmitIoRequest();

    }while( 0 );

    return ret;
}

gint32 CIfIoReqTask::OnIrpComplete(
    IRP* pIrp )
{
    // for asynchronous call
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        gint32 iRet = pIrp->GetStatus();

        ObjPtr pObj;
        CParamList oCfg( GetConfig() );

        CfgPtr pResp;
        ret = GetRespData( pResp );
        if( SUCCEEDED( ret ) )
        {
            CCfgOpener oRespCfg(
                ( IConfigDb* )pResp );

            oRespCfg[ propReturnValue ] = iRet;
            if( ERROR( iRet ) )
            {
                ret = iRet;
                break;
            }
        }
        else
        {
            CCfgOpener oRespCfg;
            oRespCfg[ propReturnValue ] = iRet;

            pResp = oRespCfg.GetCfg();
            SetRespData( pResp );

            if( ERROR( iRet ) )
                break;
        }

        // for out transfer, we only need to
        // return the pIrp->GetStatus()
        IrpCtxPtr& pCtx =
            pIrp->GetTopStack();

        guint32 dwIoDir =
            pCtx->GetIoDirection();

        if(  dwIoDir == IRP_DIR_OUT )
            break;

        ret = oCfg.GetObjPtr( propIfPtr, pObj );

        if( ERROR( ret ) )
            break;

        CRpcServices* pIf = pObj;
        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        ret = pIf->FillRespData( pIrp, pResp );
        if( SUCCEEDED( ret ) )
        {
            SetRespData( pResp );
        }

    }while( 0 );

    return ret;
}

gint32 CIfIoReqTask::OnComplete(
    gint32 iRet )
{
    CParamList oParams( GetConfig() );
    ObjPtr pObj;

    super::OnComplete( iRet );

    oParams.RemoveProperty( propRespPtr );
    oParams.RemoveProperty( propReqPtr );

    return iRet;
}

// reset the irp expire timer, should only be called
// when not IsInProcess
gint32 CIfIoReqTask::ResetTimer()
{
    gint32 ret = 0;
    CStdRTMutex oTaskLock( GetLock() );
    do{
        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );

        IRP* pIrp = nullptr;
        ret = oCfg.GetPointer( propIrpPtr, pIrp );

        if( ERROR( ret ) )
            break;

        if( pIrp == nullptr )
        {
            ret = -EFAULT;
            break;
        }

#ifdef DEBUG
        DebugPrint( 0,
            "the irp timer is reset" );
#endif
        pIrp->ResetTimer();

    }while( 0 );

    return ret;
}

gint32 CIfIoReqTask::OnKeepAlive(
    guint32 dwContext )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );

        ObjPtr pObj;
        vector< LONGWORD > vecParams;

        ret = GetParamList(
            vecParams, propKAParamList );

        if( ERROR( ret ) )
            break;

        guint32 iKaFlag = ( guint32 )vecParams[ 1 ];

        if( iKaFlag == CRpcServices::KATerm ||
            iKaFlag == CRpcServices::KARelay )
        {
            DebugPrint( 0,
                "the timer is reset on the keep-alive signal" );
            ResetTimer();
        }

        if( iKaFlag == CRpcServices::KARelay )
        {
            ret = oCfg.GetObjPtr(
                propEventSink, pObj );

            if( ERROR( ret ) )
                break;

            TaskletPtr pTask;
            pTask = pObj;
            if( pTask.IsEmpty() )
            {
                ret = -EFAULT;
                break;
            }

            EnumEventId iEvent = ( EnumEventId )
                ( eventTryLock | eventKeepAlive );

            ret = pTask->OnEvent( iEvent,
                CRpcServices::KARelay,
                0, nullptr );
        }

    }while( 0 );

    // return STATUS_PENDING to avoid io task to
    // complete
    return STATUS_PENDING;
}

CIfInvokeMethodTask::CIfInvokeMethodTask(
    const IConfigDb* pCfg ) : super( pCfg ),
    m_iKeepAlive( 0 ),
    m_iTimeoutId( 0 )
{
    SetClassId( clsid( CIfInvokeMethodTask ) );
}
/**
* @name FilterMessage: filter the message to invoke
* @{ */
/**
 *  Parameters:
 *
 *  pReqMsg : the req message in DBusMessage format
 *
 *  pReqMsg2 : the req message in IConfigDb format.
 *
 *  only one of the two can be non-nullptr. otherwise
 *  error returns;
 * @} */

gint32 CIfInvokeMethodTask::FilterMessage( 
    DBusMessage* pReqMsg,
    IConfigDb* pReqMsg2 )
{
    gint32 ret = 0;

    if( pReqMsg == nullptr &&
        pReqMsg2 == nullptr )
        return -EINVAL;

    if( pReqMsg != nullptr &&
        pReqMsg2 != nullptr )
        return -EINVAL;

    do{
        bool bInvoke = true;

        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );

        CRpcServices* pIf = nullptr;
        
        ObjPtr pObj;
        ret = oCfg.GetObjPtr( propIfPtr, pObj );
        if( ERROR( ret ) )
            break;

        pIf = pObj;

        // filter the message
        if( pIf->IsServer() )
        {
            IEventSink* pFilter = nullptr;
            std::vector< LONGWORD > vecParams;

            ret = GetParamList( vecParams );
            if( ERROR( ret ) )
            {
                ret = 0;
            }
            else if( vecParams[ 0 ] == eventFilterComp )
            {
                guint32 dwCount = GetArgCount(
                    &IEventSink::OnEvent );

                if( vecParams.size() >= dwCount )
                {
                    ret = vecParams[ 1 ];
                    CObjBase* pObj =
                        ( CObjBase* )vecParams[ 3 ];
                    pFilter = static_cast
                        < IEventSink* >( pObj );
                }
            }
            else
            {
                // maybe a retry happens, not our call
                ret = 0;
                break;
            }

            if( ret == ERROR_PREMATURE )
                bInvoke = false;

            if( bInvoke )
            {
                if( pReqMsg != nullptr )
                {
                    ret = pIf->FilterMessage( this,
                        pReqMsg, true, pFilter ); 
                }
                else
                {
                    ret = pIf->FilterMessage( this,
                        pReqMsg2, true, pFilter ); 
                }
            }

            // filter is still working
            if( ret == STATUS_PENDING )
                break;

            if( ret == ERROR_PREMATURE )
            {
                // set the response
                CCfgOpener oCfg(
                    ( IConfigDb* )GetConfig() );

                CParamList oParams;
                oParams[ propReturnValue ] = ret;

                oCfg.SetObjPtr( propRespPtr,
                    oParams.GetCfg() );
            }
        }

    }while( 0 );

    return ret;
}

/**
* @name OnFiltercomp: event handler for the filter
* completion. The handler should be called by the
* message filter when it has done its work
* asynchronously. At this point, the eventTaskComp and
* eventIrpComp or eventTimeoutCancel cannot be called
* because none of them are activated yet.
* eventCancelTask could happen on admin commands.
* @{ */
/** 
 * The filter must call this function via
 * CIfInvokeMethodTask::OnEvent with:
 *
 *  iEvent = eventFilterComp
 *
 *  dwParam1 = return code of FilterMsgIncoming
 *
 *  dwParam2 = 0
 *
 *  pData = ( guint32* )pointer to CObjBase of the
 *  filter task
 *
 * @} */

gint32 CIfInvokeMethodTask::OnFilterComp()
{
    gint32 ret = 0;
    do{
        vector< LONGWORD > vecParams;
        ret = GetParamList( vecParams );
        if( ERROR( ret ) )
            break;

        gint32 iRet = ( gint32 )vecParams[ 1 ];

        if( iRet == ERROR_PREMATURE )
        {
            // set the response
            CCfgOpener oCfg(
                ( IConfigDb* )GetConfig() );

            CParamList oParams;
            oParams[ propReturnValue ] = iRet;

            oCfg.SetObjPtr( propRespPtr,
                oParams.GetCfg() );

            ret = iRet;
            break;
        }

        if( ERROR( iRet ) )
        {
            gint32 iType = 0;
            ret = GetConfig()->GetPropertyType(
                propMsgPtr, iType );

            if( ERROR( ret ) )
                break;

            CCfgOpener oCfg(
                ( IConfigDb* )GetConfig() );

            if( iType == typeDMsg )
            {
                DMsgPtr pMsg;
                ret = oCfg.GetMsgPtr( propMsgPtr, pMsg );
                if( ERROR( ret ) )
                    break;

                CObjBase* pObj =
                    ( CObjBase* )vecParams[ 3 ];

                IEventSink* pFilterTask = static_cast
                    < IEventSink* >( pObj );

                IMessageFilter* pMsgFilt = dynamic_cast
                    < IMessageFilter* >( pFilterTask );
                
                if( pMsgFilt == nullptr )
                {
                    ret = -EFAULT;
                    break;
                }

                EnumStrategy iStrat = stratIgnore;
                pMsgFilt->GetFilterStrategy(
                    this, pMsg, iStrat );

                if( iStrat == stratIgnore )
                {
                    ret = 0;
                }
                else
                {
                    ret = ERROR_PREMATURE;
                    break;
                }
            }
            else
            {

                ObjPtr pObj;
                ret = oCfg.GetObjPtr( propMsgPtr, pObj );
                if( ERROR( ret ) )
                    break;

                IConfigDb* pMsg = pObj;
                if( pMsg == nullptr )
                {
                    ret = -EFAULT;
                    break;
                }

                CObjBase* pObjbase =
                    ( CObjBase* )vecParams[ 3 ];

                IEventSink* pFilterTask = static_cast
                    < IEventSink* >( pObjbase );

                IMessageFilter* pMsgFilt = dynamic_cast
                    < IMessageFilter* >( pFilterTask );
                
                if( pMsgFilt == nullptr )
                {
                    ret = -EFAULT;
                    break;
                }
                EnumStrategy iStrat = stratIgnore;
                pMsgFilt->GetFilterStrategy(
                    this, pMsg, iStrat );

                if( iStrat == stratIgnore )
                {
                    ret = 0;
                }
                else
                {
                    ret = ERROR_PREMATURE;
                    break;
                }
            }
        }

        // continue to the task
        ret = RunTask();

    }while( 0 );

    if( ret != STATUS_PENDING )
        OnTaskComplete( ret );

    return ret;
}

gint32 CIfInvokeMethodTask::GetIid(
    EnumClsid& iid )
{
    // to get the interface id of the invoking method
    gint32 ret = 0;
    do{
        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );

        CObjBase* pObj;
        ret = oCfg.GetPointer(
            propMatchPtr, pObj );

        if( ERROR( ret ) )
            break;

        CCfgOpenerObj oMatchCfg( pObj );
        ret = oMatchCfg.GetIntProp(
            propIid, ( guint32& )iid );

    }while( 0 );

    return ret;
}

gint32 CIfInvokeMethodTask::Process(
    guint32 dwContext )
{
    gint32 ret = 0;
    switch( dwContext )
    {
    case eventFilterComp:
        {
            ret = OnFilterComp();
            break;
        }
    default:
        {
            ret = -ENOTSUP;
            break;
        }
    }

    if( ret == -ENOTSUP )
    {
        return super::Process( dwContext );
    }

    do{
        if( ret == STATUS_PENDING )
        {
            break;
        }
        else if( Retriable( ret ) )
        {
            ret = STATUS_MORE_PROCESS_NEEDED;
            break;
        }
        else if( ret == -EAGAIN )
            ret = ERROR_FAIL;

        OnComplete( ret );

    }while( 0 );

    if( ret == STATUS_PENDING ||
        ret == STATUS_MORE_PROCESS_NEEDED )
        MarkPending();

    return ret;
}

gint32 CIfInvokeMethodTask::RunTask()
{
    gint32 ret = 0;

    // servicing the incoming request/event
    // message
    do{
        CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
        CRpcServices* pIf = nullptr;
        
        ObjPtr pObj;
        ret = oCfg.GetObjPtr( propIfPtr, pObj );
        if( ERROR( ret ) )
            break;

        pIf = pObj;

        gint32 iType = 0;

        ret = GetConfig()->GetPropertyType(
            propMsgPtr, iType );

        if( ERROR( ret ) )
            break;

        if( iType == typeDMsg )
        
        {
            DMsgPtr pMsg;
            ret = oCfg.GetMsgPtr( propMsgPtr, pMsg );
            if( ERROR( ret ) )
                break;

            if( pIf == nullptr || pMsg.IsEmpty() )
            {
                ret = -EFAULT;
                break;
            }

            string strSender = pMsg.GetSender();
            if( !pIf->IsConnected( strSender.c_str() ) )
            {
                // NOTE: this is not a serious state
                // check
                ret = ERROR_STATE;
                break;
            }

            std::string strMethod = pMsg.GetMember();

            // set the interface name in case the
            // method be called via the handler map
            oCfg[ propIfName ] = pMsg.GetInterface();
            if( pIf->IsServer() )
            {
                CInterfaceServer* pSvr = static_cast
                    < CInterfaceServer* >( pIf );
                string strIfName = pMsg.GetInterface();
                if( pSvr->IsPaused( strIfName ) )
                {
                    ret = ERROR_PAUSED;
                    break;
                }
            }

            bool bInvoke = true;
            ret = FilterMessage( pMsg, nullptr );

            if( ret == STATUS_PENDING )
                break;

            if( ERROR( ret ) )
                bInvoke = false;

            // WOW, finally we are about to go to the
            // user code
            //
            // since it could be very slow in the
            // InvokeMethod, we run this method after
            // the StartRecvMsgTask. it is possible in
            // multi-processor environment, the Task
            // could be scheduled ahead of the
            // InvokeMethod
            if( bInvoke )
            {
                ret = pIf->InvokeMethod< DBusMessage >
                    ( ( DBusMessage* )pMsg, this );
            }
        }
        else
        {

            ObjPtr pObj;
            ret = oCfg.GetObjPtr( propMsgPtr, pObj );
            if( ERROR( ret ) )
                break;

            CfgPtr pMsg = pObj;
            if( pMsg.IsEmpty() )
            {
                ret = -EFAULT;
                break;
            }

            if( pIf == nullptr || pMsg.IsEmpty() )
            {
                ret = -EFAULT;
                break;
            }

            if( !pIf->IsConnected() )
            {
                // NOTE: this is not a serious state
                // check
                ret = ERROR_STATE;
                break;
            }

            IConfigDb* pReq = pMsg;
            bool bInvoke = true;
            ret = FilterMessage( nullptr, pReq );

            if( ret == STATUS_PENDING )
                break;

            if( ERROR( ret ) )
                bInvoke = false;

            oCfg.CopyProp( propIfName, pReq );

            if( bInvoke )
            {
                ret = pIf->InvokeMethod< IConfigDb >
                    ( pReq, this );
            }

            if( ret == STATUS_PENDING )
            {
                SET_RMT_TASKID( pReq, oCfg );
            }
        }

        if( ret == STATUS_PENDING )
            break;

        if( ret == -EAGAIN ||
            ret == STATUS_MORE_PROCESS_NEEDED )
        {
            ret = ERROR_FAIL;
            if( CanRetry() )
                ret = STATUS_MORE_PROCESS_NEEDED;
            // schedule a retry
            break;
        }

        OnTaskComplete( ret );

    }while( 0 );

    return ret;
}

gint32 CIfInvokeMethodTask::OnCancel(
    guint32 dwContext )
{
    gint32 ret = 0;

    super::OnCancel( dwContext );

    if( dwContext == eventTimeoutCancel )
    {
        // we are in the the timeout context, the
        // m_iTimoutId should be simply zeroed
        m_iTimeoutId = 0;
    }

    // cancel this task
    do{
        if( !IsPending() )
        {
            // how did we come here?
            // Because the lock on CIfParallelTask
            // is already acquired at this point.
            // it is not possible to be landing
            // here
            ret = ERROR_FAIL;
            break;
        }
        CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
        CRpcServices* pIf = nullptr;
        
        ObjPtr pObj;
        ret = oCfg.GetObjPtr( propIfPtr, pObj );
        if( ERROR( ret ) )
            break;

        pIf = pObj;
        gint32 iType = 0;
        ret = GetConfig()->GetPropertyType(
            propMsgPtr, iType );

        if( ERROR( ret ) )
            break;

        if( iType == typeDMsg )
        {
            DMsgPtr pMsg;
            ret = oCfg.GetMsgPtr( propMsgPtr, pMsg );
            if( ERROR( ret ) )
                break;

            if( pIf == nullptr || pMsg.IsEmpty() )
            {
                ret = -EFAULT;
                break;
            }

            ret = pIf->OnCancel( pMsg, this );
        }
        else if( iType == typeObj )
        {
            ObjPtr pObj;
            CfgPtr pMsg;
            ret = oCfg.GetObjPtr( propMsgPtr, pObj );
            if( ERROR( ret ) )
                break;

            pMsg = pObj;
            if( pIf == nullptr || pMsg.IsEmpty() )
            {
                ret = -EFAULT;
                break;
            }

            ret = pIf->OnCancel(
                ( IConfigDb* )pMsg, this );
        }
        else
        {
            ret = -ENOTSUP;
            break;
        }

        // NOTE: the error is not exactly the one
        // in the response. it is used internally.
        ret = ERROR_CANCEL;
        if( dwContext == eventTimeoutCancel )
            ret = -ETIMEDOUT;
        else if( dwContext == eventUserCancel )
            ret = ERROR_USER_CANCEL;

        OnTaskComplete( ret );

    }while( 0 );

    return ret;
}

gint32 CIfInvokeMethodTask::OnTaskComplete(
    gint32 iRetVal )
{
    gint32 ret = 0;

    do{
        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );

        ObjPtr pObj;
        ret = oCfg.GetObjPtr(
            propRespPtr, pObj );

        if( ERROR( ret ) )
            break;

        CfgPtr pResp;
        pResp = pObj;
        if( pResp.IsEmpty() )
        {
            ret = 0;
            break;
        }

        CRpcServices* pIf = nullptr;
        ret = oCfg.GetPointer( propIfPtr, pIf );
        if( ERROR( ret ) )
            break;

        bool bServer = pIf->IsServer();

        gint32 iType = 0;
        ret = GetConfig()->GetPropertyType(
            propMsgPtr, iType );

        if( ERROR( ret ) )
            break;

        if( iType == typeDMsg )
        {
            DMsgPtr pMsg;
            ret = oCfg.GetMsgPtr( propMsgPtr, pMsg );
            if( ERROR( ret ) )
                break;

            gint32 iType = pMsg.GetType();
            switch( iType )
            {
            case DBUS_MESSAGE_TYPE_SIGNAL:
                {
                    bServer ? ret = -EINVAL : 0;
                    break;
                }
            case DBUS_MESSAGE_TYPE_METHOD_CALL:
                {
                    if( !bServer )
                    {
                        ret = -EINVAL;
                        break;
                    }

                    CInterfaceServer *pServer =
                        ObjPtr( pIf );

                    SvrConnPtr pConnMgr =
                        pServer->GetConnMgr();

                    if( !pConnMgr.IsEmpty() )
                    {
                        ret = pConnMgr->CanResponse(
                            ( HANDLE )( ( CObjBase* )this ) );
                    }

                    if( ret != ERROR_FALSE )
                    {
                        // so far we don't support
                        // asynchronous filter on
                        // response
                        ret = pServer->FilterMessage(
                            this, ( IConfigDb* )pResp, false );

                        if( ret != ERROR_PREMATURE )
                        {
                            ret = pIf->SendResponse(
                                pMsg, pResp );
                        }
                    }

                    // the connection record can be removed
                    if( !pConnMgr.IsEmpty() )
                    {
                        pConnMgr->OnInvokeComplete(
                            ( HANDLE )( ( CObjBase* )this ) );
                    }
                    break;
                }
            default:
                {
                    ret = -EINVAL;
                    break;
                }
            }
        }
        else if( iType == typeObj )
        {
            IConfigDb* pMsg = nullptr;
            ret = oCfg.GetPointer(
                propMsgPtr, pMsg );

            if( ERROR( ret ) )
                break;

            CReqOpener oCfg( pMsg );
            ret = oCfg.GetReqType(
                ( guint32& )iType );

            if( ERROR( ret ) )
                break;

            switch( iType )
            {
            case DBUS_MESSAGE_TYPE_SIGNAL:
                {
                    bServer ? ret = -EINVAL : 0;
                    break;
                }
            case DBUS_MESSAGE_TYPE_METHOD_CALL:
                {
                    ret = pIf->SendResponse(
                        pMsg, pResp );
                    break;
                }
            default:
                {
                    ret = -EINVAL;
                    break;
                }
            }
        }

    }while( 0 );

    return 0;
}

gint32 CIfInvokeMethodTask::OnComplete(
    gint32 iRetVal )
{
    gint32 ret = 0;

    if( m_iKeepAlive > 0 )
    {
        RemoveTimer( m_iKeepAlive );
        m_iKeepAlive = 0;
    }
    if( m_iTimeoutId > 0 )
    {
        RemoveTimer( m_iTimeoutId );
        m_iTimeoutId = 0;
    }

    // servicing the next incoming request or
    // event message
    do{
        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );
        CRpcServices* pIf = nullptr;
        
        ObjPtr pObj;
        ret = oCfg.GetObjPtr( propIfPtr, pObj );
        if( ERROR( ret ) )
            break;

        pIf = pObj;
        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

    }while( 0 );

    ret = super::OnComplete( iRetVal );

    RemoveProperty( propMsgPtr );
    RemoveProperty( propCallFlags );
    RemoveProperty( propRespPtr );
    RemoveProperty( propReqPtr );
    RemoveProperty( propMatchPtr );

    return ret;
}

gint32 CIfInvokeMethodTask::OnKeepAlive(
    guint32 dwContext )
{
    do{
        if( dwContext == eventTimeout )
        {
            OnKeepAliveOrig();
        }
        else if( dwContext == eventKeepAlive )
        {
            gint32 ret = 0;
            CCfgOpener oCfg(
                ( IConfigDb*)GetConfig() );
            CIoManager* pMgr = nullptr;

            ret = GET_IOMGR( oCfg, pMgr );
            if( ERROR( ret ) )
                break;

            // there could be a chain of nested locks
            // of the tasks along the way, let's
            // schedule a task to lower the risk of
            // deadlock
            DEFER_CALL( pMgr, ObjPtr( this ),
                &CIfInvokeMethodTask::OnKeepAliveRelay );
        }

    }while( 0 );

    return STATUS_PENDING;
}

gint32 CIfInvokeMethodTask::OnKeepAliveOrig()
{
    gint32 ret = 0;
    // NOTE: this method should be called by the
    // timer only
    // Send the heartbeat, schedule new timer,
    // and return status_pending for the next
    do{
        ObjPtr pObj;
        CCfgOpener oTaskCfg(
            ( IConfigDb* )GetConfig() );
        guint32 dwTimeoutSec = 0;

        if( !IsKeepAlive() )
            break;

        ret = GetKeepAliveSec( dwTimeoutSec );
        if( ERROR( ret ) )
        {
            ret = 0;
            break;
        }

        ret = oTaskCfg.GetObjPtr(
            propIfPtr, pObj );

        if( ERROR( ret ) )
            break;

        // keep alive can only be sent out by
        // interface server
        CInterfaceServer* pIf = pObj;
        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ret = pIf->OnKeepAlive( this,
            CRpcServices::KAOrigin ); 

        if( ERROR( ret ) )
            break;

        DebugPrint( ret, "Schedule next keep-alive" );
        // schedule the next keep-alive event
        CIoManager* pMgr = pIf->GetIoMgr();
        CUtilities& oUtils = pMgr->GetUtils();

        CTimerService& oTimerSvc =
            oUtils.GetTimerSvc();

        if( dwTimeoutSec > 0 )
        {
            m_iKeepAlive = oTimerSvc.AddTimer(
                dwTimeoutSec, this,
                ( guint32 )eventKeepAlive );
        }

        if( m_iTimeoutId > 0 )
        {
            oTimerSvc.ResetTimer( m_iTimeoutId );
        }
        
    }while( 0 );

    // return status_pending to avoid the task
    // being completed
    return STATUS_PENDING;
}

// should be called when this task is not IsInProcess
// ResetTimer, to extend the expire time by rewinding
// the timer to zero.
gint32 CIfInvokeMethodTask::ResetTimer()
{
    gint32 ret = 0;
    CStdRTMutex oTaskLock( GetLock() );

    do{
        ObjPtr pObj;
        CCfgOpener oTaskCfg(
            ( IConfigDb* )GetConfig() );

        CInterfaceServer* pIf = nullptr;
        ret = oTaskCfg.GetPointer(
            propIfPtr, pIf );

        if( ERROR( ret ) )
            break;

        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        // schedule the next keep-alive event
        CIoManager* pMgr = pIf->GetIoMgr();
        CUtilities& oUtils = pMgr->GetUtils();

        CTimerService& oTimerSvc =
            oUtils.GetTimerSvc();

        if( m_iTimeoutId > 0 )
        {
            oTimerSvc.ResetTimer( m_iTimeoutId );
#ifdef DEBUG
            DebugPrint( ret,
                "Invoke Request timer is reset" );
#endif
        }

    }while( 0 );

    return ret;
}

gint32 CIfInvokeMethodTask::OnKeepAliveRelay()
{
    gint32 ret = 0;
    // NOTE: for the rest CIfInvokeMethodTask's
    // along the forward-request path, this method
    // is called
    do{
        CStdRTMutex oTaskLock( GetLock() );

        if( GetTaskState() == stateStopped )
            return ERROR_STATE;

        ObjPtr pObj;
        CCfgOpener oTaskCfg(
            ( IConfigDb* )GetConfig() );

        ret = oTaskCfg.GetObjPtr(
            propIfPtr, pObj );

        if( ERROR( ret ) )
            break;

        // keep alive can only be sent out by
        // interface server
        CInterfaceServer* pIf = pObj;
        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ret = pIf->OnKeepAlive(
            this, CRpcServices::KARelay );
                
        ResetTimer();

    }while( 0 );

    return ret;
}

gint32 CIfInvokeMethodTask::GetCallOptions(
    CfgPtr& pCfg ) const
{
    const IConfigDb* pcCfg = GetConfig();
    CCfgOpener oCfg( pcCfg );
    gint32 ret = 0;
    do{
        ObjPtr pObj;
        ret = oCfg.GetObjPtr( propReqPtr, pObj );
        if( ERROR( ret ) )
            break;

        IConfigDb* pReqCfg = pObj;
        CCfgOpener oReqCfg( pReqCfg );

        ret = oReqCfg.GetObjPtr(
            propCallOptions, pObj );

        if( ERROR( ret ) )
            break;

        CfgPtr pSrcCfg = pObj;
        if( pSrcCfg.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        // make the copy of the ptr
        pCfg = pSrcCfg;

    }while( 0 );

    return ret;
}

gint32 CIfInvokeMethodTask::GetCallFlags(
    guint32& dwMsgType ) const
{
    gint32 ret = 0;
    CfgPtr pCfg;
    do{
        ret = GetCallOptions( pCfg );
        if( ERROR( ret ) )
            break;

        CCfgOpener oCfg( ( IConfigDb* )pCfg );
        guint32 dwFlags = 0;
        ret = oCfg.GetIntProp(
            propCallFlags, dwFlags );

        if( ERROR( ret ) )
            break;

        dwMsgType = dwFlags;
        
    }while( 0 );

    return ret;
}

gint32 CIfInvokeMethodTask::GetTimeoutSec(
    guint32& dwTimeoutSec ) const 
{
    gint32 ret = 0;
    CfgPtr pCfg;
    do{
        ret = GetCallOptions( pCfg );
        if( ERROR( ret ) )
            break;

        CCfgOpener oCfg( ( IConfigDb* )pCfg );
        guint32 dwTimeout = 0;
        ret = oCfg.GetIntProp(
            propTimeoutSec, dwTimeout );

        if( ERROR( ret ) )
            break;

        dwTimeoutSec = dwTimeout;
        
    }while( 0 );

    return ret;
}

bool CIfInvokeMethodTask::IsKeepAlive() const
{
    guint32 dwFlags = 0;

    gint32 ret = GetCallFlags( dwFlags );

    // by default, no keep-alive
    if( ERROR( ret ) )
        return false;

    if( dwFlags & CF_KEEP_ALIVE )
        return true;

    return false;
}

gint32 CIfInvokeMethodTask::GetKeepAliveSec(
    guint32& dwTimeoutSec )  const
{
    CfgPtr pCfg;

    gint32 ret = GetCallOptions( pCfg );
    if( ERROR( ret ) )
        return ret;

    guint32 dwSeconds;
    CCfgOpener oCfg( ( IConfigDb* )pCfg );

    ret = oCfg.GetIntProp(
        propKeepAliveSec, dwSeconds );

    if( ERROR( ret ) )
        return ret;

    dwTimeoutSec = dwSeconds;

    return ret;
}

bool CIfInvokeMethodTask::HasReply() const
{
    guint32 dwFlags = 0;

    gint32 ret = GetCallFlags( dwFlags );

    // by default, no reply
    if( ERROR( ret ) )
        return false;

    if( dwFlags & CF_WITH_REPLY )
        return true;

    return false;
}

/**
* @name SetAsyncCall, set propCallFlags to tell the
* on-going method is whether a sync call or not.
* This flag is set when the pIf->InvokeMethod
* returns STATUS_PENDING. However, this method
* does more than just set a flag. if this method
* is an async call, the task will set the
* keep-alive timer and the request timeout timer
* too. The two timers are set only when the
* propKeepAliveSec or propTimeoutSec is set
* respectively.
* @{ */
/**  @} */

gint32 CIfInvokeMethodTask::SetAsyncCall(
    bool bAsync )
{
    CfgPtr pCfg;
    gint32 ret = 0;

    while( bAsync )
    {
        // let's schedule an expire timer if
        // required
        
        ObjPtr pObj;
        CCfgOpener oTaskCfg(
            ( IConfigDb* )GetConfig() );

        guint32 dwTimeoutSec = 0;

        ret = GetTimeoutSec( dwTimeoutSec );
        if( ERROR( ret ) )
        {
            // fine, we are ok with it
            ret = GetKeepAliveSec( dwTimeoutSec );
            if( ERROR( ret ) )
            {
                ret = 0;
                break;
            }
            // let's use 2 * KeepAliveSec as
            // the timeout value
            dwTimeoutSec *= 2;
        }

        ret = oTaskCfg.GetObjPtr(
            propIfPtr, pObj );

        if( ERROR( ret ) )
            break;

        CRpcServices* pIf = pObj;
        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        CIoManager* pMgr = pIf->GetIoMgr();
        CUtilities& oUtils = pMgr->GetUtils();
        CTimerService& oTimerSvc =
            oUtils.GetTimerSvc();

        m_iTimeoutId = oTimerSvc.AddTimer(
            dwTimeoutSec, this,
            ( guint32 )eventTimeoutCancel );

        if( !IsKeepAlive() )
            break;

        ret = GetKeepAliveSec( dwTimeoutSec );
        if( ERROR( ret ) )
        {
            ret = 0;
            break;
        }

        if( dwTimeoutSec == 0 )
            break;

        m_iKeepAlive = oTimerSvc.AddTimer(
            dwTimeoutSec, this,
            ( guint32 )eventKeepAlive );

#ifdef __DEBUG
        DebugPrint( ret,
            "KeepAliveTimer started in %d seconds",
            dwTimeoutSec );
#endif

        break;
    }
    
    return ret;
}

gint32 CopyFile( gint32 iFdSrc, gint32 iFdDest, ssize_t& iSize );

gint32 CIfFetchDataTask::RunTask()
{
    gint32 ret = 0;
    CParamList oCfg( ( IConfigDb* )GetConfig() );
    ObjPtr pObj;

    gint32 iFdSrc = -1;
    gint32 iFdDest = -1;
    
    do{
        guint32 dwOffset = 0;
        guint32 dwSize = 0;

        ret = oCfg.GetObjPtr( 0, pObj );
        if( ERROR( ret ) )
            break;

        CfgPtr pDesc = pObj;
        if( pDesc.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        CCfgOpener oDesc;
        *oDesc.GetCfg() = *pDesc;

        ret = oCfg.GetIntProp( 1, dwOffset );
        if( ERROR( ret ) )
            break;

        ret = oCfg.GetIntProp( 2, dwSize );
        if( ERROR( ret ) )
            break;

        guint32 dwVal;
        ret = oCfg.GetIntProp( 3, dwVal );
        if( ERROR( ret ) )
            break;

        iFdSrc = *( gint32* )&dwVal;

        ret = oCfg.GetIntProp( 4, dwVal );
        if( ERROR( ret ) )
            break;

        iFdDest = *( gint32* )&dwVal;

        ObjPtr pObj;
        ret = oCfg.GetObjPtr( propIfPtr, pObj );
        if( ERROR( ret ) )
            break;

        CInterfaceServer* pIf = pObj;
        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ret = oCfg.GetObjPtr( propEventSink, pObj );
        if( ERROR( ret ) )
            break;

        IEventSink* pCallback = pObj;
        if( pCallback == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ssize_t iSize = dwSize;
        ssize_t iBytes = 4 * MAX_BYTES_PER_TRANSFER > iSize
            ? iSize : 4 * MAX_BYTES_PER_TRANSFER;

        while( iSize > 0 && !m_bExit )
        {
            ret = CopyFile( iFdSrc, iFdDest, iBytes );
            if( iSize > iBytes )
            {
                iSize -= iBytes;
                if( iSize < iBytes )
                    iSize = iBytes;
            }
            else
            {
                iSize = 0;
                break;
            }
        }
        if( m_bExit.load( std::memory_order_acquire ) )
        {
            ret = -ECANCELED;
            break;
        }

        CParamList oResp;
        oResp.SetIntProp( propReturnValue, ret );

        if( SUCCEEDED( ret ) )
        {
            dwSize = iSize;
            oResp.Push( ObjPtr( oDesc.GetCfg() ) );
            oResp.Push( iFdDest ); // fd with the data
            oResp.Push( dwOffset ); // offset
            oResp.Push( dwSize );   // offset
        }

        ret = pIf->OnServiceComplete(
            oResp.GetCfg(), pCallback );

    }while( 0 );

    if( iFdSrc >= 0 )
        close( iFdSrc );
    
    if( iFdDest >= 0 )
        close( iFdDest );

    return ret;
}

gint32 CIfFetchDataTask::OnCancel(
    guint32 dwContext )
{
    m_bExit.store( true, std::memory_order_release );
    while( m_bEnded.load( std::memory_order_acquire ) )
        usleep( 100000 );

    return 0;
}

CIfDummyTask::CIfDummyTask( const IConfigDb* pCfg )
   :super( pCfg ) 
{
    SetClassId( clsid( CIfDummyTask ) );
}

gint32 CIfDummyTask::RunTask()
{
    DebugPrint( 0, "CIfDummyTask is called" );
    return 0;
}

gint32 CIfDummyTask::operator()(
    guint32 dwContext )
{ return 0; }

gint32 CIfDummyTask::OnEvent(
    EnumEventId iEvent,
    LONGWORD dwParam1,
    LONGWORD dwParam2,
    LONGWORD* pData )
{
    switch( iEvent )
    {
    case eventIrpComp:
        {
            RemoveProperty( propIrpPtr );
            break;
        }
    default:
        {
            break;
        }
    }
    return 0;
}

gint32 CIoReqSyncCallback::operator()(
    guint32 dwContext )
{
    gint32 ret = 0;
    switch( ( EnumEventId )dwContext )
    {
    case eventTaskComp:
        {
            std::vector< LONGWORD > vecParams;
            do{
                ret = GetParamList(
                    vecParams, propParamList );

                if( ERROR( ret ) )
                    break;

                // the result of the io request
                ret = vecParams[ 1 ];
                if( ERROR( ret ) )
                    break;

                // the task could be the CIfIoReqTask
                CObjBase* pTask =
                    ( CObjBase* ) vecParams[ 3 ];

                if( pTask == nullptr )
                    break;

                TaskletPtr pT =
                    static_cast< CTasklet* >( pTask );

                if( pT.IsEmpty() )
                    break;

                CParamList oParams(
                    ( IConfigDb* )pT->GetConfig() );

                if( !oParams.exist( propRespPtr ) )
                {
                    DebugPrint( -ENOENT, "no Resp ptr" );
                    ret = -EINVAL;
                    break;
                }

                IConfigDb* pCfg = nullptr;
                ret = oParams.GetPointer(
                    propRespPtr, pCfg );

                if( ERROR( ret ) )
                    break;

                CCfgOpener oCfg( pCfg );
                if( !oCfg.exist( propReturnValue ) )
                {
                    ret = -EINVAL;
                    break;
                }

                CCfgOpener oTaskCfg(
                    (IConfigDb*)GetConfig() );

                oTaskCfg[ propRespPtr ] =
                    ObjPtr( pCfg );

                ret = 0;

            }while( 0 );

            SetError( ret );
            Sem_Post( &m_semWait );
            break;
        }
    default:
        {
            ret = -ENOTSUP;
            SetError( ret );
            Sem_Post( &m_semWait );
            DebugPrint( ret,
                "CIoReqSyncCallback completed with error %d",
                 dwContext );
            break;
        }
    }

    return ret;
}

gint32 CIfDeferredHandler::RunTask()
{
    EventPtr pEvt;
    gint32 ret = GetInterceptTask( pEvt );
    if( SUCCEEDED( ret ) )
    {
        CCfgOpenerObj oCfg( this );
        // oCfg.CopyProp( propRespPtr, pEvt );
        oCfg.CopyProp( propMsgPtr, pEvt );
        oCfg.CopyProp( propReqPtr, pEvt );
    }

    if( m_pDeferCall.IsEmpty() )
        return 0;

    ret = ( *m_pDeferCall )( 0 );
    if( ret == STATUS_PENDING )
        return ret;

    OnTaskComplete( ret );
    return ret;
}

gint32 CIfDeferredHandler::UpdateParamAt(
    guint32 i, BufPtr pBuf )
{
    CDeferredCallBase< CTasklet >* pTask =
        m_pDeferCall;
    if( pTask == nullptr )
        return -EINVAL;

    return pTask->UpdateParamAt( i, pBuf );
}

gint32 CIfDeferredHandler::OnTaskComplete( gint32 iRet ) 
{
    gint32 ret = 0;
    do{
        // set the response data
        EventPtr pEvtPtr;
        ret = GetInterceptTask( pEvtPtr );
        if( ERROR( ret ) )
        {
            if( SUCCEEDED( iRet ) )
                iRet = ret;
        }

        if( ERROR( iRet ) ) 
        {
            CParamList oParams;
            oParams[ propReturnValue ] = iRet;
            CInterfaceServer* pIf = nullptr;

            CCfgOpenerObj oTaskCfg( this );
            ret = oTaskCfg.GetPointer(
                propIfPtr, pIf );
            if( ERROR( ret ) )
                break;

            pIf->SetResponse( pEvtPtr,
                oParams.GetCfg() );
        }
        else
        {
            CCfgOpenerObj oTaskCfg(
                ( CObjBase* ) pEvtPtr );    

            ret = oTaskCfg.MoveProp(
                propRespPtr, this );

            if( ERROR( ret ) )
                iRet = ret;
        }

    }while( 0 );

    return iRet;
}

gint32 CIfDeferredHandler::OnComplete( gint32 iRetVal )
{
    gint32 ret = super::OnComplete( iRetVal );
    m_pDeferCall.Clear();
    RemoveProperty( propRespPtr );
    RemoveProperty( propMsgPtr );
    RemoveProperty( propReqPtr );
    return ret;
}

gint32 CIfResponseHandler::OnCancel(
    guint32 dwContext )
{
    gint32 ret = super::OnCancel( dwContext );
    m_pDeferCall.Clear();
    ClearClientNotify();
    return ret;
}

gint32 CIfResponseHandler::OnTaskComplete( gint32 iRet ) 
{
    gint32 ret = 0;
    do{
        // try best to make the response data
        // available in parameter 1
        BufPtr pBuf( true );
        std::vector< LONGWORD > vecParams;
        ret = GetParamList( vecParams );
        if( SUCCEEDED( ret ) )
        {
            // comes from a eventTaskComp
            if( vecParams.size() < 
                GetArgCount( &IEventSink::OnEvent ) )
            {
                ret = -EINVAL;
            }
            else
            {
                CObjBase* pIoReq =
                    ( CObjBase* )vecParams[ 3 ];

                if( pIoReq != nullptr )
                {
                    CCfgOpenerObj oReqCfg( pIoReq );
                    IConfigDb* pResp = nullptr;

                    ret = oReqCfg.GetPointer(
                        propRespPtr, pResp );

                    if( SUCCEEDED( ret ) )
                        *pBuf = ObjPtr( pIoReq );
                }
                else
                {
                    // the caller does not
                    // return response
                    ret = -EFAULT;
                }
            }
        }
        else
        {
            // comes from a immediate return
            CCfgOpener oCfg(
                ( IConfigDb* )GetConfig() );
            if( oCfg.exist( propRespPtr ) )
            {
                *pBuf = ObjPtr( this );
                ret = 0;
            }
        }

        if( ERROR( ret ) )
        {
            CParamList oResp;
            oResp[ propReturnValue ] = iRet;

            CParamList oParams;
            oParams.SetPointer( propRespPtr,
                ( CObjBase* )oResp.GetCfg() );
            TaskletPtr pTask;
            pTask.NewObj(
                clsid( CIfDummyTask ),
                oParams.GetCfg() );
            *pBuf = ObjPtr( pTask );
        }
        UpdateParamAt( 1, pBuf );

    }while( 0 );

    if( m_pDeferCall.IsEmpty() )
        return 0;

    ret = ( *m_pDeferCall )( 0 );
    ClearClientNotify();

    return ret;
}

gint32 CIfIoCallTask::RunTask()
{
    if( m_pMajorCall.IsEmpty() )
        return super::RunTask();

    gint32 ret = ( *m_pMajorCall )( eventZero );
    if( ret == STATUS_PENDING )
        return ret;

    return OnTaskComplete( ret );
}

gint32 CIfIoCallTask::OnTaskComplete(
    gint32 iRet )
{
    gint32 ret = super::OnTaskComplete( iRet );
    // the deferred call cannot be called twice.
    // this task should retire.
    if( ret == STATUS_PENDING )
        ret = 0;

    m_pMajorCall.Clear();
    m_pDeferCall.Clear();

    return ret;
}

gint32 CIfIoCallTask::OnCancel(
    guint32 dwContext )
{
    gint32 ret = super::OnCancel( dwContext );
    m_pMajorCall.Clear();
    return ret;
}

gint32 CBusPortStopSingleChildTask::OnComplete(
    gint32 iRetVal )
{
    gint32 ret = super::OnComplete( iRetVal );
    return ret;
}

gint32 CBusPortStopSingleChildTask::OnIrpComplete(
    PIRP pIrp )
{
    gint32 ret = 0;
    CParamList oCfg(
        ( IConfigDb* )GetConfig() );
    do{
        CIoManager* pMgr = nullptr;
        ret = oCfg.GetPointer(
            propIoMgr, pMgr );

        if( ERROR( ret ) )
            break;

        IPort* pPort = nullptr;
        ret = oCfg.GetPointer( 0, pPort );
        if( ERROR( ret ) )
            break;

        CPnpManager& oPnpMgr = pMgr->GetPnpMgr();
        ret = oPnpMgr.DestroyPortStack( pPort );
        if( ret == STATUS_PENDING )
        {
            DebugPrint( ERROR_FAIL,
            "Unable to stop the port gracefully "
            "proceed to complete the master irp "
            "anyway" );
        }

        IEventSink* pCallback = nullptr;
        ret = oCfg.GetPointer( 2, pCallback );
        if( ERROR( ret ) )
            break;

        LONGWORD dwParam2 = 0;

#if ( BUILD_64 == 0 )
        ret = oCfg.GetIntProp( 3, dwParam2 );
#else
        ret = oCfg.GetQwordProp( 3, dwParam2 );
#endif

        if( ERROR( ret ) )
            break;

        LONGWORD dwParam1 =
            ( LONGWORD )( ( IRP* )pIrp );

        // run the original callback
        pCallback->OnEvent( eventIrpComp,
            dwParam1, dwParam2, nullptr );

    }while( 0 );

    oCfg.ClearParams();
    return ret;
}

gint32 CBusPortStopSingleChildTask::RunTask()
{
    // this is a retriable task
    gint32 ret = 0;
    CParamList oCfg(
        ( IConfigDb* )GetConfig() );

    do{
        CIoManager* pMgr = nullptr;

        ret = oCfg.GetPointer(
            propIoMgr, pMgr );

        if( ERROR( ret ) )
            break;

        IPort* pPort = nullptr;
        ret = oCfg.GetPointer( 0, pPort );
        if( ERROR( ret ) )
            break;

        PIRP pIrp = nullptr; 
        ret = oCfg.GetPointer( 1, pIrp );
        if( ERROR( ret ) )
            break;

        CStdRMutex oIrpLock( pIrp->GetLock() );
        ret = pIrp->CanContinue( IRP_STATE_READY );
        if( ERROR( ret ) )
            break;

        EventPtr pIrpCb = pIrp->m_pCallback;
        LONGWORD dwContext = pIrp->m_dwContext;

        oCfg.Push( ObjPtr( pIrpCb ) );
        oCfg.Push( dwContext );

        // intercept the callback
        pIrp->SetCallback( this, 0 );

        CPnpManager& oPnpMgr = pMgr->GetPnpMgr();
        ret = oPnpMgr.StopPortStack(
            pPort, pIrp );

        // to avoid other contender to complete or
        // cancel the irp before we are done
        oIrpLock.Unlock();

        // the irp completion will be called
        if( SUCCEEDED( ret ) ||
            ret == STATUS_PENDING )
            break;

        ret = oPnpMgr.DestroyPortStack( pPort );
        if( ret == STATUS_PENDING )
        {
            DebugPrint( ret,
            "Unable to stop the port gracefully "
            "proceed to complete the master irp "
            "anyway" );
        }

        oIrpLock.Lock();
        ret = pIrp->CanContinue( IRP_STATE_READY );
        if( ERROR( ret ) )
        {
            ret = 0;
            break;
        }

        // recover the original callback
        pIrp->SetCallback( pIrpCb, dwContext );
        IrpCtxPtr& pIrpCtx =
            pIrp->GetTopStack();

        pIrpCtx->SetStatus( ret );
        oIrpLock.Unlock();

        pMgr->CompleteIrp( pIrp );
        break;

    }while( 0 );

    return ret;
}

gint32 CIfInterceptTask::OnKeepAlive(
    guint32 dwContext )
{
    EventPtr pEvt;
    gint32 ret = GetInterceptTask( pEvt );
    if( ERROR( ret ) )
        return ret;

    vector< LONGWORD > vecParams;
    ret = GetParamList(
        vecParams, propKAParamList );
    if( ERROR( ret ) )
        return ret;

    EnumEventId iEvent = ( EnumEventId )
        ( eventTryLock | eventKeepAlive );

    CCfgOpener oCfg(
        ( IConfigDb* )GetConfig() );
    CIoManager* pMgr = nullptr;
    ret = GET_IOMGR( oCfg, pMgr );
    if( ERROR( ret ) )
        return ret;

    return pEvt->OnEvent( iEvent,
        vecParams[ 1 ], 0, 0 );
}

gint32 CIfCallbackInterceptor::RunTask()
{ return STATUS_PENDING; }

gint32 CIfCallbackInterceptor::OnTaskComplete(
    gint32 iRet )
{
    if( ERROR( iRet ) )
        return iRet;

    if( m_pCallAhead.IsEmpty() )
        return iRet;

    gint32 ret =
        ( *m_pCallAhead )( eventZero );

    if( SUCCEEDED( iRet ) && ERROR( ret ) )
        iRet = ret;

    return iRet;
}

gint32 CIfCallbackInterceptor::OnComplete(
    gint32 iRet )
{
    super::OnComplete( iRet );

    if( !m_pCallAfter.IsEmpty() )
        ( *m_pCallAfter )( eventZero );

    return iRet;
}
