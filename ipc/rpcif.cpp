/*
 * =====================================================================================
 *
 *       Filename:  rpcif.cpp
 *
 *    Description:  implementation of the rpc interface classes
 *
 *        Version:  1.0
 *        Created:  02/04/2017 05:01:32 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
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
#include "ifhelper.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "jsondef.h"

using namespace std;

CGenericInterface::CGenericInterface(
    const IConfigDb* pCfg )
{
    CCfgOpener oNewCfg;
    *oNewCfg.GetCfg() = *pCfg;

    if( !oNewCfg.exist( propIfStateClass ) )
    {
        throw std::invalid_argument(
            DebugMsg( -EINVAL, "Error occurs" ) );
    }

    guint32 iStateClass = 0;
    if( !oNewCfg.exist( propIfPtr ) )
        oNewCfg[ propIfPtr ] = this;

    gint32 ret = oNewCfg.GetIntProp(
        propIfStateClass, iStateClass ) ;
        
    ret = m_pIfStat.NewObj( 
        ( EnumClsid )iStateClass,
        oNewCfg.GetCfg() );

    if( ERROR( ret ) )
    {
        throw std::runtime_error( 
            DebugMsg( ret, "Error occurs" ) );
    }
}

static CfgPtr InitIfProxyCfg(
    const IConfigDb * pCfg )
{
    CCfgOpener oNewCfg;
    *oNewCfg.GetCfg() = *pCfg;

    string strPortClass;
    gint32 ret = oNewCfg.GetStrProp(
        propPortClass, strPortClass );

    if( ERROR( ret ) )
    {
        throw std::invalid_argument(
            DebugMsg( -EINVAL, "Error occurs" ) );
    }

    EnumClsid iStateClass =
        clsid( CLocalProxyState );

    if( strPortClass == PORT_CLASS_DBUS_PROXY_PDO ||
        strPortClass == PORT_CLASS_TCP_STREAM_PDO )
    {
        // CRemoteProxyState will have an extra ip
        // address to match
        iStateClass = clsid( CRemoteProxyState );
    }

    oNewCfg.SetIntProp(
        propIfStateClass,
        iStateClass );

    return oNewCfg.GetCfg();
}

CInterfaceProxy::CInterfaceProxy(
    const IConfigDb* pCfg ) :
    super( InitIfProxyCfg( pCfg ) )
{
    gint32 ret = 0;

    do{
        CCfgOpener oCfg( pCfg );
        string strPortClass;

        ret = oCfg.GetStrProp(
            propPortClass, strPortClass );

        if( ERROR( ret ) )
            break;

        ret = m_pIfMatch.NewObj(
            clsid( CMessageMatch ), pCfg );

        if( ERROR( ret ) )
            break;

        CCfgOpenerObj matchCfg(
            ( IMessageMatch* ) m_pIfMatch );

        ret = matchCfg.CopyProp(
            propDestDBusName, pCfg );

        if( ERROR( ret ) )
            break;

        ret = matchCfg.CopyProp(
            propIfName, pCfg );

        if( ERROR( ret ) )
            break;

        ret = matchCfg.CopyProp(
            propObjPath, pCfg );

        if( ERROR( ret ) )
            break;

        ret = matchCfg.SetIntProp(
            propMatchType, ( guint32 )matchClient );

        if( ERROR( ret ) )
            break;

    }while( 0 );

    if( ERROR( ret ) )
    {
        string strMsg = DebugMsg( ret,
            "Error in CInterfaceProxy ctor" );
        throw std::runtime_error( strMsg );
    }
}

CInterfaceProxy::~CInterfaceProxy()
{
    if( !m_pIfStat.IsEmpty() )
        m_pIfStat.Clear();

    if( !m_pIfMatch.IsEmpty() )
        m_pIfMatch.Clear();
}

gint32 CRpcBaseOperations::AddPortProps()
{
    if( !IsServer() )
    {
        // for proxy, we get a sender name from the
        // port
        BufPtr pBuf( true );
        gint32 ret = GetIoMgr()->GetPortProp(
            GetPortHandle(),
            propSrcDBusName,
            pBuf );

        if( SUCCEEDED( ret ) )
        {
            CInterfaceState* pStat = m_pIfStat;
            if( pStat->exist( propSrcDBusName ) )
                return 0;

            // at least the proxy will have a
            // meaningful sender name.  on server side,
            // the propSrcDBusName should be obtained
            // from the server name and the object name
            SetProperty(
                propSrcDBusName, *pBuf );
        }
    }
    return 0;
}

gint32 CRpcBaseOperations::OpenPort(
    IEventSink* pCallback )
{
    gint32 ret =
        m_pIfStat->OpenPort( pCallback );

    if( SUCCEEDED( ret ) )
        AddPortProps();

    return ret;
}

gint32 CRpcBaseOperations::ClosePort()
{
    return m_pIfStat->ClosePort();
}

// start listening the event from the interface
// server
gint32 CRpcBaseOperations::EnableEventInternal(
    IMessageMatch* pMatch,
    bool bEnable,
    IEventSink* pCallback )
{
    if( pMatch == nullptr )
        return -EINVAL;

    gint32 ret = 0;

    do{
        EnumIfState iState =
            m_pIfStat->GetState();

        if( bEnable )
        {
            if( iState != stateStarted &&
                iState != stateRecovery )
            {
                ret = ERROR_STATE;
                break;
            }
        }
        else
        {
            if( iState != stateStopping )
            {
                ret = ERROR_STATE;
                break;
            }
        }
       
        CIoManager* pMgr = GetIoMgr();
        IrpPtr pIrp;

        ret = pIrp.NewObj();
        if( ERROR( ret ) )
            break;

        ret = pIrp->AllocNextStack( nullptr );
        if( ERROR( ret ) )
            break;

        CReqBuilder oReq( this );

        string strMethod = IF_METHOD_ENABLEEVT;

        if( !bEnable )
            strMethod = IF_METHOD_DISABLEEVT;

        oReq.SetMethodName( strMethod );

        oReq.SetCallFlags( CF_WITH_REPLY |
           DBUS_MESSAGE_TYPE_METHOD_CALL );

        oReq.SetObjPtr( propMatchPtr,
            ObjPtr( pMatch ) );

        ret = SetupReqIrp( pIrp,
            oReq.GetCfg(), pCallback );

        if( ERROR( ret ) )
            break;

        CInterfaceState* pIfState = m_pIfStat;
        if( pIfState == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        if( pCallback == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        CCfgOpenerObj oTaskCfg(
            ( CObjBase* ) pCallback );

        // for task canceling purpose
        ret = oTaskCfg.SetObjPtr(
            propIrpPtr, ObjPtr( pIrp ) );

        ret = pMgr->SubmitIrp(
            GetPortHandle(), pIrp );

        if( ret == STATUS_PENDING )
            break;

        oTaskCfg.RemoveProperty(
            propIrpPtr );

        pIrp->RemoveTimer();
        pIrp->RemoveCallback();

    }while( 0 );

    return ret;
}

gint32 CRpcBaseOperations::EnableEvent(
    IMessageMatch* pMatch,
    IEventSink* pCallback )
{
    // stop listening the event from the interface
    // server
    if( pMatch == nullptr )
        return -EINVAL;

    gint32 ret = EnableEventInternal(
        pMatch, true, pCallback );

    return ret;
}

gint32 CRpcBaseOperations::DisableEvent(
    IMessageMatch* pMatch,
    IEventSink* pCallback )
{
    // stop listening the event from the interface
    // server
    if( pMatch == nullptr )
        return -EINVAL;

    gint32 ret = EnableEventInternal(
        pMatch, false, pCallback );

    return ret;
}

gint32 CRpcBaseOperations::StartRecvMsg(
    IEventSink* pCompletion,
    IMessageMatch* pMatch )
{
    if( pCompletion == nullptr ||
        pMatch == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{

        IrpPtr pIrp;
        if( m_pIfStat->GetState() != stateConnected )
        {
            ret = ERROR_STATE;
            break;
        }

        CIoManager* pMgr = GetIoMgr();

        CCfgOpenerObj oTaskCfg(
            ( CObjBase* ) pCompletion );

        //NOTE: complicated logic alert
        ret = pIrp.NewObj();
        if( ERROR( ret ) )
            break;

        ret = pIrp->AllocNextStack( nullptr );
        if( ERROR( ret ) )
            break;

        CCfgOpenerObj oMatchCfg( pMatch );

        string strIfName;
        ret = oMatchCfg.GetStrProp(
            propIfName, strIfName );

        if( ERROR( ret ) )
            break;

        if( strIfName == IFNAME_INVALID )
        {
            // this match is not for message
            // filtering
            ret = 0;
            break;
        }
        
        string strObjPath;
        string strObjPath2;

        ret = oMatchCfg.GetStrProp(
            propObjPath, strObjPath );

        if( ERROR( ret ) )
            break;

        strObjPath2 = DBUS_OBJ_PATH(
            pMgr->GetModName(), OBJNAME_INVALID );

        // an invalid obj path
        if( strObjPath == strObjPath2 )
        {
            // this match is not for message
            // filtering
            ret = 0;
            break;
        }

        CReqBuilder oReq( this );
        oReq.SetMethodName( IF_METHOD_LISTENING );

        oReq.SetCallFlags( CF_WITH_REPLY |
            CF_ASYNC_CALL |
            DBUS_MESSAGE_TYPE_METHOD_CALL );

        oReq.SetObjPtr(
            propMatchPtr, ObjPtr( pMatch ) );

        ret = SetupReqIrp( pIrp,
            oReq.GetCfg(), pCompletion );

        if( ERROR( ret ) )
            break;

        // set the completion thread
        pIrp->SetIrpThread( pMgr ); 
        pIrp->SetSyncCall( false );
        pIrp->RemoveTimer();

        CInterfaceState* pIfStat = m_pIfStat;
        if( pIfStat == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        // make a copy of the irp for canceling
        ret = oTaskCfg.SetObjPtr(
            propIrpPtr, ObjPtr( pIrp ) );

        HANDLE hPort = GetPortHandle();

        bool bPdo = false;
        ret = oTaskCfg.GetBoolProp(
            propSubmitPdo, bPdo );

        if( ERROR( ret ) || !bPdo )
        {
            ret = pMgr->SubmitIrp( hPort, pIrp );
        }
        else
        {
            IPort* pPort = HandleToPort( hPort );
            IPort* pPdo = pPort->GetBottomPort();
            ret = pMgr->SubmitIrpInternal(
                pPdo, pIrp, false );
        }

        if( ret == STATUS_PENDING )
            break;

        if( ERROR( ret ) )
            break;

        // clear the irp reference
        TaskletPtr pTask;
        pTask = ObjPtr( pCompletion );
        if( !pTask.IsEmpty() )
        {
            pTask->RemoveProperty(
                propIrpPtr );
        }

        ThreadPtr pThrd;
        ret = pIrp->GetIrpThread( pThrd );
        if( ERROR( ret ) )
        {
            pIrp->RemoveCallback();
            ret = -EFAULT;
            break;
        }

        pIrp->ClearIrpThread();

        CIrpCompThread* pIrpThrd = pThrd;
        // complete the irp in the irp completion
        // thread
        ret = pIrpThrd->AddIrp( pIrp );
        if( ERROR( ret ) )
        {
            pIrp->RemoveCallback();
            break;
        }

        // tell the tasklet to wait for the irp
        // completion
        ret = STATUS_PENDING;

    }while( 0 );

    return ret;
}

gint32 CRpcBaseOperations::CancelRecvMsg()
{
    // not a serious cancel if more than one
    // pending irps
    if( m_pIrp.IsEmpty() )
        return 0;

    gint32 ret = GetIoMgr()->CancelIrp(
        m_pIrp, true, -ECANCELED );

    return ret;
}

gint32 CRpcBaseOperations::PauseResumeInternal(
    EnumEventId iEvent, IEventSink* pCallback )
{
    return m_pIfStat->OnAdminEvent(
        iEvent, string( "" ) );
}

gint32 CRpcBaseOperations::DoModEvent(
    EnumEventId iEvent,
    const std::string& strModule )
{
    return m_pIfStat->OnModEvent(
        iEvent, strModule );
}

gint32 CRpcBaseOperations::DoRmtModEvent(
    EnumEventId iEvent,
    const std::string& strModule,
    const std::string& strIpAddr )
{

    return m_pIfStat->OnRmtModEvent(
        iEvent, strModule, strIpAddr );
}

gint32 CRpcBaseOperations::DoPause(
    IEventSink* pCallback )
{
    return PauseResumeInternal(
        cmdPause, pCallback );
}

gint32 CRpcBaseOperations::DoResume(
    IEventSink* pCallback )
{
    return PauseResumeInternal(
        cmdResume, pCallback );
}

gint32 CRpcInterfaceBase::Start()
{
    TaskletPtr pCallback;
    gint32 ret = 0;
    do{
        ret = SetStateOnEvent( cmdOpenPort );
        if( ERROR( ret ) )
            break;

        ret = pCallback.NewObj(
            clsid( CSyncCallback ) );

        if( ERROR( ret ) )
            break;

        ret = StartEx( pCallback );
        if( ERROR( ret ) )
            break;

        if( ret == STATUS_PENDING )
        {
            CSyncCallback* pSyncCb = pCallback;
            if( pSyncCb == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            pSyncCb->WaitForComplete();
        }

    }while( 0 );

    return ret;
}

gint32 CRpcInterfaceBase::StartEx(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    TaskGrpPtr pTaskGrp;

    if( pCallback == nullptr )
        return -EINVAL;

    do{
        TaskletPtr pOpenPortTask;
        TaskletPtr pEnableEvtTask;
        TaskletPtr pRecvMsgTask;
        EnumIfState iState =
            m_pIfStat->GetState();

        if( iState != stateStarting )
        {
            ret = ERROR_STATE;
            break;
        }

        CParamList oParams;

        ret = oParams.SetObjPtr(
            propIfPtr, ObjPtr( this ) );

        if( ERROR( ret ) )
            break;

        ret = m_pRootTaskGroup.NewObj(
            clsid( CIfRootTaskGroup ),
            oParams.GetCfg() ); 

        if( ERROR( ret ) )
            break;

        ret = pTaskGrp.NewObj(
            clsid( CIfTaskGroup ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        ret = pOpenPortTask.NewObj(
            clsid( CIfOpenPortTask ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        pTaskGrp->AppendTask( pOpenPortTask );

        // enable or disable event
        oParams.Push( true );

        // Change Interface state or not
        oParams.Push( false );

        for( auto pMatch : m_vecMatches )
        {
            oParams[ propMatchPtr ] =
                ObjPtr( pMatch );

            // the last one
            if( pMatch == m_pIfMatch )
            {
                // make sure to change Interface state
                // to connected
                oParams[ 1 ] = true;
            }

            ret = pEnableEvtTask.NewObj(
                clsid( CIfEnableEventTask ),
                oParams.GetCfg() );

            if( ERROR( ret ) )
                break;

            pTaskGrp->AppendTask( pEnableEvtTask );
        }
        pTaskGrp->SetRelation( logicAND );

        CCfgOpenerObj oCfg( ( CObjBase* )pTaskGrp );

        ret = oCfg.SetBoolProp(
            propNotifyClient, true );

        if( ERROR( ret ) )
            break;

        if( pCallback != nullptr )
        {
            ret = oCfg.SetObjPtr(
                propEventSink, ObjPtr( pCallback) );

            if( ERROR( ret ) )
                break;
        }

        TaskletPtr pTask = pTaskGrp;
        if( pTask.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        ret = AppendAndRun( pTask );
        if( ERROR( ret ) )
            break;

        bool bConnected = false;
        ret = pTask->GetError();

        if( SUCCEEDED( ret ) && 
            GetState() == stateConnected )
        {
            bConnected = true;
        }

        if( bConnected )
        {
            bool bEnable = false;
            oParams.Pop( bEnable );
            oParams.Pop( bEnable );

            // we cannot add the recvtask by AddAndRun
            // at this point, because it requires the
            // interface to be in the connected state,
            // while we are still in the stopped state
            CCfgOpener oRecvCfg;

            ret = oRecvCfg.SetObjPtr(
                propIfPtr, ObjPtr( this ) );

            if( ERROR( ret  ) )
                break;

            TaskletPtr pParaTaskGrp;
            ret = pParaTaskGrp.NewObj(   
                clsid( CIfParallelTaskGrp ),
                oRecvCfg.GetCfg() );

            if( ERROR( ret ) )
                break;
                
            CIfParallelTaskGrp* pTaskGrp
                = pParaTaskGrp;

            for( auto pMatch : m_vecMatches )
            {
                oParams[ propMatchPtr ] =
                    ObjPtr( pMatch );

                ret = pRecvMsgTask.NewObj(
                    clsid( CIfStartRecvMsgTask ),
                    oParams.GetCfg() );

                if( ERROR( ret ) )
                    break;

                ret = pTaskGrp->AppendTask(
                    pRecvMsgTask );

                if( ERROR( ret ) )
                    break;
            }

            if( ERROR( ret ) )
                break;

            ret = AppendAndRun( pParaTaskGrp );
            if( ERROR( ret ) )
                break;

            // the ret does not necessarily indicate
            // the return value of the receive task. we
            // need to get the status code from the
            // task.
            ret = pRecvMsgTask->GetError();
            if( ret == STATUS_PENDING )
                ret = 0;
        }

    }while( 0 );

    if( ret == STATUS_MORE_PROCESS_NEEDED )
        ret = STATUS_PENDING;

    return ret;
}

gint32 CRpcInterfaceBase::Stop()
{
    gint32 ret = 0;
    do{
        ret = SetStateOnEvent( cmdShutdown );
        if( ERROR( ret ) )
            break;

        TaskletPtr pTask;
        ret = pTask.NewObj(
            clsid( CSyncCallback ) );

        if( ERROR( ret ) )
            break;

        IEventSink* pCallback = pTask;
        ret = StopEx( pCallback );

        if( ret == STATUS_PENDING )
        {
            CSyncCallback* pSyncTask = pTask;
            ret = pSyncTask->WaitForComplete();
        }

    }while( 0 );
    
    return ret;
}

gint32 CRpcInterfaceBase::StopEx(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    TaskGrpPtr pTaskGrp;
    TaskletPtr pCompletion;

    if( pCallback == nullptr )
        return -EINVAL;

    do{
        TaskletPtr pClosePortTask;
        TaskletPtr pDisableEvtTask;

        if( stateStopping != GetState() )
            return ERROR_STATE;

        ClearActiveTasks();

        CParamList oParams;
        ret = oParams.SetObjPtr(
            propIfPtr, ObjPtr( this ) );

        if( ERROR( ret ) )
            break;

        ret = pTaskGrp.NewObj(
            clsid( CIfTaskGroup ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        ret = pClosePortTask.NewObj(
            clsid( CIfStopTask ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        // enable or disable event
        oParams.Push( false );
        // Change Interface state or not
        oParams.Push( false );

        for( auto pMatch : m_vecMatches )
        {
            oParams[ propMatchPtr ] =
                ObjPtr( pMatch );

            if( pMatch == m_pIfMatch )
                oParams[ 1 ] = true;

            ret = pDisableEvtTask.NewObj(
                clsid( CIfEnableEventTask ),
                oParams.GetCfg() );

            if( ERROR( ret ) )
                break;

            pTaskGrp->AppendTask( pDisableEvtTask );
        }

        if( ERROR( ret ) )
            break;

        pTaskGrp->AppendTask( pClosePortTask );
        pTaskGrp->SetRelation( logicNONE );

        CCfgOpenerObj oCfg( ( CObjBase* )pTaskGrp );

        ret = oCfg.SetBoolProp(
            propNotifyClient, true );

        if( ERROR( ret ) )
            break;

        CCfgOpener oCompCfg;

        ret = oCompCfg.SetObjPtr(
            propIfPtr, ObjPtr( this ) );

        if( ERROR( ret ) )
            break;

        if( pCallback )
        {
            oCompCfg[ propNotifyClient ] = true;
            oCompCfg[ propEventSink ] =
                ObjPtr( pCallback);
        }

        ret = pCompletion.NewObj(
            clsid( CIfCleanupTask ),
            oCompCfg.GetCfg() );

        if( ERROR( ret ) )
            break;

        ret = oCfg.SetObjPtr(
            propEventSink, pCompletion );

        if( ERROR( ret ) )
            break;

        TaskletPtr pTask = pTaskGrp;

        if( pTask.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        ret = AppendAndRun( pTask );
        if( ERROR( ret ) )
            break;

        ret = pTask->GetError();

        if( ret != STATUS_PENDING )
        {
            ( *pCompletion )( eventZero );
        }

    }while( 0 );


    if( ret == STATUS_MORE_PROCESS_NEEDED )
    {
        ret = STATUS_PENDING;
    }

    return ret;
}

gint32 CRpcInterfaceBase::PauseResume(
    bool bResume, IEventSink* pCallback )
{
    gint32 ret = 0;
    TaskletPtr pTask;

    do{
        CParamList oParams;
        oParams.Push( false );

        ret = oParams.SetObjPtr(
            propIfPtr, ObjPtr( this ) );

        if( ERROR( ret ) )
            break;

        ret = oParams.SetBoolProp(
            propNotifyClient, bResume );

        if( ERROR( ret ))
            break;

        ret = oParams.SetObjPtr(
            propEventSink, ObjPtr( pCallback ) );

        if( ERROR( ret ) )
            break;
        
        ret = pTask.NewObj(
            clsid( CIfPauseResumeTask ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        ret = AppendAndRun( pTask );
        if( ERROR( ret ) )
            break;

        ret = pTask->GetError();

    }while( 0 );

    if( ret == STATUS_MORE_PROCESS_NEEDED )
        ret = STATUS_PENDING;

    if( ret == STATUS_PENDING
        && pCallback == nullptr )
    {
        CIfRetryTask* pRetryTask = pTask;

        if( pRetryTask != nullptr )
            pRetryTask->WaitForComplete();
    }

    return ret;
}

gint32 CRpcInterfaceBase::Resume(
    IEventSink* pCallback )
{
    return PauseResume( true, pCallback );
}

gint32 CRpcInterfaceBase::Pause(
    IEventSink* pCallback )
{
    return PauseResume( false, pCallback );
}

gint32 CRpcInterfaceBase::Shutdown(
    IEventSink* pCallback )
{
    gint32 ret =
        SetStateOnEvent( cmdShutdown );

    if( ERROR( ret ) )
        return ret;

    return StopEx( pCallback );
}

gint32 CRpcInterfaceBase::OnPortEvent(
    EnumEventId iEvent,
    HANDLE hPort )
{
    gint32 ret = 0;

    if( !IsMyPort( hPort ) )
        return -EINVAL;

    ret = SetStateOnEvent( iEvent );
    if( ERROR( ret ) )
        return ret;

    switch( iEvent )
    {
    case eventPortStopping:
        {
            TaskletPtr pTask;
            ret = pTask.NewObj(
                clsid( CIfDummyTask ) );

            // we need an asynchronous call
            ret = StopEx( pTask );
            break;
        }
    case eventPortStarted:
        {
            // called from CIfOpenPortTask
            ret = m_pIfStat->OnPortEvent(
                iEvent, hPort );

            if( ERROR( ret ) )
                break;

            AddPortProps();
            break;
        }
    case eventPortStartFailed:
    case eventPortStopped:
    default:
        {
            ret = m_pIfStat->OnPortEvent(
                iEvent, hPort );
            break;
        }
    }

    return ret;
}

gint32 CRpcInterfaceBase::OnModEvent(
    EnumEventId iEvent,
    const std::string& strModule )
{
    gint32 ret = 0;

    do{
        TaskletPtr pTask;

        if( !IsMyDest( strModule ) )
            break;

        if( TestSetState( iEvent )
            == ERROR_STATE )
            break;

        CParamList oParams;
        ret = oParams.Push( iEvent );

        if( ERROR( ret ) )
            break;

        oParams.Push( strModule );

        if( ERROR( ret ) )
            break;

        ret = oParams.SetObjPtr(
            propIfPtr, ObjPtr( this ) );

        if( ERROR( ret  ) )
            break;

        TaskletPtr pDummyTask;
        ret = pDummyTask.NewObj(
            clsid( CIfDummyTask ) );

        if( ERROR( ret ) )
            break;

        ObjPtr pObj = pDummyTask;
        ret = oParams.SetObjPtr(
            propEventSink, pObj );

        if( ERROR( ret ) )
            break;

        oParams.SetBoolProp(
            propNotifyClient, true );

        if( ERROR( ret ) )
            break;

        ret = pTask.NewObj(
            clsid( CIfCpEventTask ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        ret = AppendAndRun( pTask );

    }while( 0 );

    return ret;
}

gint32 CRpcInterfaceBase::OnRmtModEvent(
    EnumEventId iEvent,
    const std::string& strModule,
    const std::string& strIpAddr )
{
    CCfgOpenerObj oCfg( this );
    string strDestIp;

    gint32 ret = oCfg.GetStrProp(
        propIpAddr, strDestIp );

    if( ERROR( ret ) )
        return ret;

    if( strDestIp != strIpAddr )
        return ERROR_FAIL;

    return OnModEvent( iEvent, strModule );
}

gint32 CRpcInterfaceBase::OnDBusEvent(
    EnumEventId iEvent )
{
    gint32 ret = 0;

    ret = SetStateOnEvent( iEvent );
    if( ERROR( ret ) )
        return ret;

    switch( iEvent )
    {
    case eventDBusOffline:
        {
            TaskletPtr pTask;
            ret = pTask.NewObj(
                clsid( CIfDummyTask ) );

            if( ERROR( ret ) )
                break;

            ret = StopEx( pTask );
            break;
        }
    case eventDBusOnline:
        {
            // no effect
            break;
        }
    default:
        {
            ret = -ENOTSUP;
        }
    }
    return ret;
}


gint32 CRpcInterfaceBase::OnRmtSvrEvent(
    EnumEventId iEvent,
    const string& strIpAddr )
{
    gint32 ret = 0;

    CCfgOpenerObj oCfg( this );
    string strDestIp;

    ret = oCfg.GetStrProp(
        propIpAddr, strDestIp );

    if( ERROR( ret ) )
        return ret;

    if( strDestIp != strIpAddr )
        return ERROR_ADDRESS;

    switch( iEvent )
    {
    case eventRmtDBusOffline:
    case eventRmtSvrOffline:
        {
            OnDBusEvent( eventDBusOffline );
            break;
        }
    case eventRmtDBusOnline:
    case eventRmtSvrOnline:
        {
            OnDBusEvent( eventDBusOnline );
            break;
        }
    default:
        {
            ret = -ENOTSUP;
            break;
        }
    }

    return ret;
}

gint32 CRpcInterfaceBase::OnAdminEvent(
    EnumEventId iEvent,
    const std::string& strParams )
{
    gint32 ret = 0;

    ret = SetStateOnEvent( iEvent );

    if( ERROR( ret ) )
        return ret;

    switch( iEvent )
    {
    case cmdShutdown:
        {
            TaskletPtr pTask;

            ret = pTask.NewObj(
                clsid( CIfDummyTask ) );

            if( ERROR( ret ) )
                break;

            ret = StopEx( pTask );
            break;
        }
    case cmdPause:
        {
            TaskletPtr pTask;

            // to avoid synchronous call
            // we put  dumb task there
            ret = pTask.NewObj(
                clsid( CIfDummyTask ) );

            if( ERROR( ret ) )
                break;

            ClearActiveTasks();
            ret = Pause( pTask );
            break;
        }
    case cmdResume:
        {
            TaskletPtr pTask;

            // to avoid synchronous call
            // we put  dumb task there
            ret = pTask.NewObj(
                clsid( CIfDummyTask ) );

            if( ERROR( ret ) )
                break;

            ret = Resume( pTask );
            break;
        }
    default:
        ret = -ENOTSUP;

    }
    return ret;
}

gint32 CRpcInterfaceBase::OnEvent(
    EnumEventId iEvent,
    guint32 dwParam1,
    guint32 dwParam2,
    guint32* pData )
{
    gint32 ret = 0;

    switch( iEvent )
    {
    case eventPortStarted:
    case eventPortStopping:
    case eventPortStartFailed:
        {
            ret = OnPortEvent( iEvent, dwParam1 );
#ifdef DEBUG
            if( ERROR( ret ) )
            {
                HANDLE hPort = dwParam1;
                IPort* pPort = HandleToPort( hPort );
                if( pPort != nullptr )
                {
                    string strDump;
                    pPort->Dump( strDump );
                    DebugPrint( ret, strDump );
                }
            }
#endif
            break;
        }
    case eventPortStopped:
        {
            ret = ERROR_STATE;
            break;
        }
    case eventModOnline:
    case eventModOffline:
        {
            string strMod =
                reinterpret_cast< char* >( pData );

            ret = OnModEvent( iEvent, strMod ); 
            break;
        }
    case eventRmtModOnline:
    case eventRmtModOffline:
        {
            string strMod =
                reinterpret_cast< char* >( pData );

            string strIpAddr =
                reinterpret_cast< char* >( dwParam1 );

            ret = OnRmtModEvent(
                iEvent, strMod, strIpAddr ); 

            break;
        }
    case eventDBusOnline:
    case eventDBusOffline:
        {
            ret = OnDBusEvent( iEvent );
            break;
        }
    case eventRmtDBusOffline:
    case eventRmtDBusOnline:
    case eventRmtSvrOnline:
    case eventRmtSvrOffline:
        {
            HANDLE hPort = ( guint32 )pData;
            string strIpAddr = reinterpret_cast
                < char* >( dwParam1 );

            CInterfaceState* pIfStat = m_pIfStat;
            if( pIfStat == nullptr )
                break;

            if( hPort != GetPortHandle() )
            {
                ret = ERROR_ADDRESS;
                break;
            }

            ret = OnRmtSvrEvent(
                iEvent, strIpAddr ); 

            break;
        }
    case cmdPause:
    case cmdResume:
    case cmdShutdown:
        {
            ret = OnAdminEvent(
                iEvent, string( "" ) );
            break;
        }
    default:
        {
            ret = -ENOTSUP;
            break;
        }
    }

#ifdef DEBUG
    if( ERROR( ret ) )
    {
        DebugPrint( ret,
            "Error occured on Event %d", iEvent );
    }
#endif
    return ret;
}

gint32 CRpcInterfaceBase::ClearActiveTasks()
{
    gint32 ret = 0;

    if( !m_pRootTaskGroup.IsEmpty() )
    {
        ret = ( *m_pRootTaskGroup )(
            eventCancelTask );
    }

    return ret;
}

gint32 CRpcInterfaceBase::AppendAndRun(
    TaskletPtr& pTask )
{
    if( pTask.IsEmpty() )
        return -EINVAL;

    return m_pRootTaskGroup->
        AppendAndRun( pTask );
}

gint32 CRpcInterfaceBase::AddAndRun(
    TaskletPtr& pIoTask )
{
    // usually I/O tasks are the task that will
    // run concurrently parallelly
    if( pIoTask.IsEmpty() )
        return -EINVAL;

    TaskGrpPtr pRootGrp;
    CIfRootTaskGroup* pRootTaskGroup = nullptr;
    if( true )
    {
        CStdRMutex oIfLock( GetLock() );
        if( GetState() != stateConnected )
        {
            return ERROR_STATE;
        }
        pRootTaskGroup = GetTaskGroup();

        if( m_pRootTaskGroup.IsEmpty() )
            return -EFAULT;

        // concurrent tasks are only allowed when the
        // state is stateConnected 
        if( GetState() != stateConnected )
            return ERROR_STATE;

        if( pRootTaskGroup == nullptr )
            return -EFAULT;

        // as a guard to the root group
        pRootGrp = pRootTaskGroup;
    }

    gint32 ret = 0;

    do{
        TaskletPtr pTail;
        bool bTail = true;

        CStdRTMutex oTaskLock(
            pRootTaskGroup->GetLock() );

        guint32 dwCount =
            pRootTaskGroup->GetTaskCount();

        ret = pRootTaskGroup->
            GetTailTask( pTail );

        if( SUCCEEDED( ret ) )
        {
            CIfParallelTaskGrp* pParaGrp = pTail;
            if( pParaGrp == nullptr )
                bTail = false;
        }

        TaskletPtr pParaTask;
        if( bTail )
        {
            pParaTask = pTail;
        }
        else if( !bTail || dwCount == 0 )
        {
            // add a new parallel task group
            CStdRMutex oIfLock( GetLock() );
            if( GetState() != stateConnected )
            {
                ret = ERROR_STATE;
                break;
            }
            CCfgOpener oCfg;

            ret = oCfg.SetObjPtr(
                propIfPtr, ObjPtr( this ) );

            if( ERROR( ret  ) )
                break;

            ret = pParaTask.NewObj(   
                clsid( CIfParallelTaskGrp ),
                oCfg.GetCfg() );

            if( ERROR( ret ) )
                break;
                
            pRootTaskGroup->
                AppendTask( pParaTask );

            pTail = pParaTask;
        }

        CIfParallelTaskGrp* pParaTaskGrp =
            pParaTask;

        if( pParaTaskGrp == nullptr )
        {
            // rollback
            pRootTaskGroup->
                RemoveTask( pParaTask );
            ret = -EFAULT;
            break;
        }

        // add the task to the pending queue or
        // run it immediately
        ret = pParaTaskGrp->AppendTask( pIoTask );
        bool bRunning = pParaTaskGrp->IsRunning();

        oTaskLock.Unlock();

        if( ERROR( ret ) )
            break;

        if( dwCount == 0 || bRunning )
        {
            // run the root task if the Parallel
            // task group is not running yet
            ( *pRootTaskGroup )( eventZero );
            ret = pRootTaskGroup->GetError();
        }
        else
        {

            DebugPrint( GetTid(),
                "root task not run immediately, dwCount=%d, bRunning=%d",
                dwCount, bRunning );
        }

    }while( 0 );

    // don't rely on `ret' to determine the return
    // value from the task pIoTask, you need to
    // get the return value from
    // pIoTask->GetError() in case the task is
    // scheduled to run later
    return ret;
}

gint32 CRpcServices::RebuildMatches()
{
    gint32 ret = 0;
    do{
        // add interface id to all the matches
        for( auto pMatch : m_vecMatches )
        {
            CCfgOpenerObj oMatchCfg(
                ( CObjBase* )pMatch );

            string strIfName;
            ret = oMatchCfg.GetStrProp(
                propIfName, strIfName );

            if( ERROR( ret ) )
                continue;
            
            string strIfName2 =
                IF_NAME_FROM_DBUS( strIfName );

            ToInternalName( strIfName );
            EnumClsid iid =
                CoGetIidFromIfName( strIfName2 );

            if( iid == clsid( Invalid ) )
                continue;

            oMatchCfg.SetIntProp( propIid, iid );
        }

        CCfgOpenerObj oMatchCfg(
            ( CObjBase* )m_pIfMatch );

        oMatchCfg.SetIntProp( propIid,
            GetClsid() );

        // also append the match for master interface
        m_vecMatches.push_back( m_pIfMatch );

    }while( 0 );

    return ret;
}

gint32 CRpcServices::StartEx(
    IEventSink* pCallback )
{
    gint32 ret = OnPreStart( pCallback );
    if( ERROR( ret ) )
        return ret;

    return super::StartEx( pCallback );
}

gint32 CRpcServices::OnPreStart(
    IEventSink* pCallback )
{
    gint32 ret = InitUserFuncs();

    if( ERROR( ret ) )
        return ret;

    return RebuildMatches();
}

gint32 CRpcServices::OnPostStop(
    IEventSink* pCallback )
{
    CStdRMutex oIfLock( GetLock() );
    m_vecMatches.clear();
    m_mapFuncs.clear();
    m_mapProxyFuncs.clear();
    m_pRootTaskGroup.Clear();

    return 0;
}

typedef std::pair< gint32, BufPtr > ARG_ENTRY;

bool CRpcServices::IsQueuedReq()
{

    CStdRMutex oIfLock( GetLock() );
    bool bQueuedReq = false;

    CCfgOpenerObj oCfg( this );
    gint32 ret = oCfg.GetBoolProp(
        propQueuedReq, bQueuedReq );

    if( ERROR( ret ) )
        return false;

    return bQueuedReq;
}

guint32 CRpcServices::GetQueueSize() const
{
    CStdRMutex oIfLock( GetLock() );
    return m_queTasks.size();
}

/**
* @name InvokeMethod to invoke the specified
* method in the pReqMsg. if pReqMsg is nullptr and
* propQueuedReq is set, the method will aussume
* the caller knows the current task is done and
* pop the first entry and proceed to the next
*
* parameter:
*   pReqMsg: the incoming method call or the event
*
*   pCallback: the callback object. It is required
*   to call this callback when the method is done
*   asynchronously, with eventTaskComp and return
*   value as the input parameters. Note that you
*   CANNOT call this callback from within the
*   context of InvokeMethod.
* @{ */
/**  @} */

template< typename T1, 
    typename T = typename std::enable_if<
        std::is_same<T1, DBusMessage>::value ||
        std::is_same<T1, IConfigDb>::value, T1 >::type >
gint32 CRpcServices::InvokeMethod(
    T* pReqMsg, IEventSink* pCallback )
{
    gint32 ret = 0;
    gint32 ret1 = 0;
    CCfgOpenerObj oCfg( this );

    bool bQueuedReq = IsQueuedReq();
    if( pReqMsg == nullptr ||
        pCallback == nullptr )
        return -EINVAL;

    do{
        do{

            if( bQueuedReq )
            {
                CStdRMutex oIfLock( GetLock() );
                bool bWait = true;
                if( m_queTasks.size() > 20 )
                {
                    ret1 = -ENOMEM;
                    break;
                }
                else if( m_queTasks.size() > 0 )
                {

                    if( ( ( IEventSink* )m_queTasks.front() )
                        == pCallback )
                    {
                        // already the first one
                        // to run, no need to wait
                        bWait = false;
                    }
                    else
                    {
                        // some other task is
                        // running, and wait in
                        // the queue
                        m_queTasks.push_back(
                            EventPtr( pCallback ) );
                    }
                }
                else
                {
                    // this msg is the first to
                    // service, go on to run it
                    // and no wait
                    m_queTasks.push_back(
                        EventPtr( pCallback ) );
                    bWait = false;
                }

                if( bWait )
                {
                    // let's wait in the queue
                    ret1 = STATUS_PENDING;
                    break;
                }
            }
            ret = DoInvoke( pReqMsg, pCallback );

        }while( 0 );

        if( ret1 == STATUS_PENDING )
        {
            // the task is not activated, so we
            // skip SetAsyncCall().
            // Note that the timer for this task
            // is not activated too, so probably
            // not a good idea.
            ret = ret1;
            break;
        }
        else if( ERROR( ret1 ) )
        {
            ret = ret1;
        }

        if( ret == STATUS_PENDING )
        {
            // this is an async call
            CIfInvokeMethodTask* pInvokeTask = 
                ObjPtr( pCallback );

            if( pInvokeTask != nullptr )
                pInvokeTask->SetAsyncCall();

            break;
        }

    }while( 0 );

    return ret;
}

template
gint32 CRpcServices::InvokeMethod<DBusMessage>(
    DBusMessage* pReqMsg, IEventSink* pCallback );

template
gint32 CRpcServices::InvokeMethod<IConfigDb>(
    IConfigDb* pReqMsg, IEventSink* pCallback );

gint32 CRpcServices::InvokeNextMethod()
{
    gint32 ret = 0;
    bool bQueuedReq = IsQueuedReq();

    do{
        if( !bQueuedReq )
            break;

        CStdRMutex oIfLock( GetLock() );
        if( GetState() != stateConnected )
        {
            m_queTasks.clear();
            ret = ERROR_STATE;
            break;
        }

        m_queTasks.pop_front();
        if( m_queTasks.size() )
        {
            TaskletPtr pTask =
                ObjPtr( m_queTasks.front() );

            if( pTask.IsEmpty() )
            {
                // next message
                continue;
            }
            oIfLock.Unlock();
            ret = ( *pTask )( eventZero );
            oIfLock.Lock();

            if( ret == STATUS_PENDING )
                break;

            // exhaust the msg queue
            continue;
        }
        else
        {
            ret = -ENOENT;
        }

        break;

    }while( 1 );

    return ret;
}

/**
* @name RunIoTask
* Schedule a CIfIoReqTask with the parameters in
* pReqCall, and response data in pRespData. Every
* CIfIoReqTask will fill the response data
* OnComplete whether sync or async.
* @{ */
/** 
 * 
 *  You should extract the return value of the io
 *  task from the response data on success.
 *
 *  If error code returned, it means the operation
 *  is prematured and no response data is
 *  returned.
 *  
 *  The status for this io task is not returnred
 *  from this method, because we don't know if the
 *  return value in the response data have
 *  different protocol to interpret the error code
 *
 * @} */

                
gint32 CRpcServices::RunIoTask(
    IConfigDb* pReqCall,
    CfgPtr& pRespData,
    IEventSink* pCallback,
    guint64* pIoTaskId )
{
    gint32 ret = 0;
    do{
        CParamList oParams;
        ret = oParams.SetObjPtr(
            propIfPtr, ObjPtr( this ) );

        if( ERROR( ret ) )
            break;

        ret = oParams.SetBoolProp(
            propNotifyClient, true );

        if( ERROR( ret ) )
            break;

        if( pCallback != nullptr )
        {
            ret = oParams.SetObjPtr(
                propEventSink, pCallback );

            if( ERROR( ret ) )
                break;
        }

        TaskletPtr pTask;   
        ret = pTask.NewObj(
            clsid( CIfIoReqTask ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        CIfIoReqTask* pIoTask = pTask;
        if( pIoTask == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        // set the req and the resp
        pIoTask->SetReqCall( pReqCall );
        pIoTask->SetRespData( pRespData );

        guint64 qwTaskId = pTask->GetObjId();
        CReqBuilder oCfg( pReqCall );

        // propTaskId is for KEEP_ALIVE, and the
        // event notification
        oCfg.SetTaskId( qwTaskId );

        if( pIoTaskId != nullptr )
            *pIoTaskId = qwTaskId;

        // schedule the task or run it immediately
        ret = AddAndRun( pTask );

        if( ERROR( ret ) )
            break;

        ret = pTask->GetError();

        if( ret == STATUS_PENDING &&
            pCallback == nullptr )
        {
            CIfIoReqTask* pIoTask = pTask;
            if( pIoTask == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            ret = pIoTask->WaitForComplete();
            if( SUCCEEDED( ret ) )
            {
                // the response data should
                // already in the pRespData.
                //
                // no more action here
            }
        }

    }while( 0 );

    return ret;
}

gint32 CInterfaceProxy::SendData_Proxy(
    IConfigDb* pDataDesc,           // [in]
    gint32 fd,                      // [in]
    guint32 dwOffset,               // [in]
    guint32 dwSize,                 // [in]
    IEventSink* pCallback )
{
    CCfgOpener oCfg( pDataDesc );
    if( !oCfg.exist( propMethodName ) )
        oCfg[ propMethodName ] = 
            string( SYS_METHOD_SENDDATA );

    return SendFetch_Proxy( pDataDesc,
        fd, dwOffset, dwSize, pCallback );
}

gint32 CInterfaceProxy::FetchData_Proxy(
    IConfigDb* pDataDesc,           // [in, out]
    gint32& fd,                     // [out]
    guint32& dwOffset,              // [in, out]
    guint32& dwSize,                // [in, out]
    IEventSink* pCallback )
{
    if( pDataDesc == nullptr ||
        pCallback == nullptr )
        return -EINVAL;

    CCfgOpener oCfg( pDataDesc );
    if( !oCfg.exist( propMethodName ) )
        oCfg[ propMethodName ] = 
            string( SYS_METHOD_FETCHDATA );

    return SendFetch_Proxy( pDataDesc,
        fd, dwOffset, dwSize, pCallback );
}

gint32 CInterfaceProxy::SendFetch_Proxy(
    IConfigDb* pDataDesc,           // [in, out]
    gint32& fd,                     // [out]
    guint32& dwOffset,              // [in, out]
    guint32& dwSize,                // [in, out]
    IEventSink* pCallback )
{

    gint32 ret = 0;
    if( pDataDesc == nullptr ||
        pCallback == nullptr ||
        dwSize == 0 )
        return -EINVAL;

    try{
        do{
            CParamList oResp;
            bool bFetch = false;

            CReqBuilder oDesc( this );
            oDesc.Append( pDataDesc );

            if( string( SYS_METHOD_FETCHDATA ) ==
                string( oDesc[ propMethodName ] ) )
                bFetch = true;

            CReqBuilder oBuilder( this );
            oBuilder.SetMethodName(
                oDesc[ propMethodName ] );

            oBuilder.SetCallFlags( CF_WITH_REPLY
               | DBUS_MESSAGE_TYPE_METHOD_CALL 
               | CF_ASYNC_CALL );

            oBuilder.SetTimeoutSec(
                IFSTATE_DEFAULT_IOREQ_TIMEOUT ); 

            oBuilder.SetKeepAliveSec(
                IFSTATE_DEFAULT_IOREQ_TIMEOUT / 2 ); 

            oBuilder.CopyProp( propNonFd, pDataDesc );

            oBuilder.Push( ObjPtr( oDesc.GetCfg() ) );
            oBuilder.Push( fd );
            oBuilder.Push( dwOffset );
            oBuilder.Push( dwSize );

            CustomizeRequest(
                oBuilder.GetCfg(), pCallback );

            gint32 ret = RunIoTask( oBuilder.GetCfg(),
                oResp.GetCfg(), pCallback );

            if( ret == STATUS_PENDING )
                break;

            gint32 iRet = 0;
            ret = oResp.GetIntProp( propReturnValue,
                 ( guint32& )iRet );

            if( ERROR( ret ) )
                break;

            ret = iRet;

            if( SUCCEEDED( ret ) && bFetch )
            {
                oResp.Pop( dwSize );
                oResp.Pop( dwOffset );
                oResp.Pop( fd );
                ObjPtr pObj;
                oResp.Pop( pObj );

                IConfigDb* pRespDesc = pObj;
                if( pRespDesc != nullptr )
                {
                    // overwrite the pDataDesc
                    *pDataDesc = *pRespDesc;
                }
            }
        }while( 0 );
    }
    catch( std::runtime_error& e )
    {
        ret = -ENOENT;
    }

    return ret;
}

CRpcServices::CRpcServices( const IConfigDb* pCfg )
    : super( pCfg )
{
    do{
        gint32 ret = 0;
        CCfgOpenerObj oCfg( this );
        ret = oCfg.CopyProp( propTimeoutSec, pCfg );
        if( ERROR( ret ) )
        {
            oCfg.SetIntProp( propTimeoutSec,
                IFSTATE_DEFAULT_IOREQ_TIMEOUT );
        }
        ret = oCfg.CopyProp( propKeepAliveSec, pCfg );
        if( ERROR( ret ) )
        {
            oCfg.SetIntProp( propKeepAliveSec,
                IFSTATE_DEFAULT_IOREQ_TIMEOUT / 2 );
        }

        if( oCfg.exist( propObjList ) )
        {
            ObjPtr pObj;
            ret = oCfg.GetObjPtr( propObjList, pObj );
            if( ERROR( ret ) )
                break;

            ObjVecPtr pObjVec( pObj );
            if( pObjVec.IsEmpty() )
            {
                ret = -EFAULT;
                break;
            }
            for( auto pElem : ( *pObjVec )() )
            {
                MatchPtr pMatch( pElem );
                m_vecMatches.push_back( pMatch );
            }

            RemoveProperty( propObjList );
        }

    }while( 0 );

    return;
}

gint32 CRpcServices::BuildBufForIrpIfMethod(
    BufPtr& pBuf, IConfigDb* pReqCall )
{
    if( pReqCall == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        if( pBuf.IsEmpty() )
        {
            ret = pBuf.NewObj();
            if( ERROR( ret ) )
                break;
        }

        // the requests have a match object in the
        // req ptr
        CCfgOpener oCfg( pReqCall );
        ObjPtr pObj;
        ret = oCfg.GetObjPtr(
            propMatchPtr, pObj );

        if( ERROR( ret ) )
            break;

        *pBuf = pObj;

    }while( 0 );

    return ret;
}

gint32 CRpcServices::BuildBufForIrp(
    BufPtr& pBuf, IConfigDb* pReqCall )
{
    if( pReqCall == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        if( pBuf.IsEmpty() )
        {
            ret = pBuf.NewObj();
            if( ERROR( ret ) )
                break;
        }

        bool bRequest = false;

        CReqOpener oReq( pReqCall );
        gint32 iReqType;
        ret = oReq.GetReqType(
            ( guint32& )iReqType );

        if( ERROR( ret ) )
            break;

        if( iReqType == 
                DBUS_MESSAGE_TYPE_METHOD_RETURN )
        {
            ret = -EINVAL;
            break;
        }
        if( iReqType ==
            DBUS_MESSAGE_TYPE_METHOD_CALL )
        {
            bRequest = true;
        }

        string strMethod;
        ret = oReq.GetMethodName( strMethod );
        if( ERROR( ret ) )
            break;

        if( strMethod == SYS_METHOD_SENDDATA ||
            strMethod == SYS_METHOD_FETCHDATA )
        {
            ObjPtr pObj( pReqCall );
            *pBuf = pObj;
            break;
        }
        else if( strMethod == IF_METHOD_ENABLEEVT ||
            strMethod == IF_METHOD_DISABLEEVT ||
            strMethod == IF_METHOD_LISTENING )
        {
            // the m_reqData is an MatchPtr
            ret = BuildBufForIrpIfMethod(
                pBuf, pReqCall );
            break;
        }

        // let's build a message
        DMsgPtr pMsg;
        ret = pMsg.NewObj( ( EnumClsid )iReqType );

        string strObjPath;
        ret = oReq.GetObjPath( strObjPath );
        if( ERROR( ret ) )
            break;

        ret = pMsg.SetPath( strObjPath );
        if( ERROR( ret ) )
            break;

        string strIfName;
        ret = oReq.GetIfName( strIfName );
        if( ERROR( ret ) )
            break;

        ret = pMsg.SetInterface( strIfName );
        if( ERROR( ret ) )
            break;

        string strDest;
        ret = oReq.GetDestination( strDest );
        if( ERROR( ret ) && bRequest )
            break;

        if( SUCCEEDED( ret ) )
        {
            ret = pMsg.SetDestination( strDest );
            if( ERROR( ret ) && bRequest )
                break;
        }

        string strSender;
        ret = oReq.GetSender( strSender );

        if( SUCCEEDED( ret ) )
        {
            ret = pMsg.SetSender( strSender );
            if( ERROR( ret ) ) 
                break;
        }

        ret = pMsg.SetMember( strMethod );
        if( ERROR( ret ) )
            break;

        BufPtr pReqBuf( true );
        ret = pReqCall->Serialize( *pReqBuf );
        if( ERROR( ret ) )
            break;

        if( !dbus_message_append_args( pMsg,
            DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
            &pReqBuf->ptr(), pReqBuf->size(),
            DBUS_TYPE_INVALID ) )
        {
            ret = -ENOMEM;
            break;
        }
        *pBuf = pMsg;

    }while( 0 );

    return ret;
}

/**
* @name Fill pResp with the content in pIrp
* @{ */
/**  @} */

gint32 CRpcServices::FillRespData(
    IRP* pIrp, CfgPtr& pResp )
{
    gint32 ret = 0;
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    if( pResp.IsEmpty() )
        return -EINVAL;

    CParamList oParams( ( IConfigDb* )pResp );
    if( SUCCEEDED( pIrp->GetStatus() ) )
    {
        // retrieve the data from the irp
        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        guint32 dwDirection = pCtx->GetIoDirection();
        if( dwDirection == IRP_DIR_OUT )
            return -EINVAL;

        if( pCtx->IsNonDBusReq() )
        {
            CfgPtr pMsg = nullptr;
            ret = pCtx->GetRespAsCfg( pMsg );
            if( SUCCEEDED( ret ) )
            {
                pResp = pMsg;
            }
            else
            {
                oParams[ propReturnValue ] = ret;
            }
        }
        else
        {
            BufPtr& pBuf = pCtx->m_pRespData;
            if( !pBuf.IsEmpty() )
            {
                DMsgPtr pMsg = *pBuf;
                if( !pMsg.IsEmpty() )
                {
                    ret = FillRespData( pMsg, pResp );
                    if( ERROR( ret ) )
                    {
                        oParams[ propReturnValue ] = ret;
                    }
                }
            }
        }
    }

    if( !oParams.exist( propReturnValue ) )
    {
        // for those cancelled irp or bad response
        // returned
        oParams[ propReturnValue ] =
            pIrp->GetStatus();
    }
    return ret;
}

gint32 CRpcServices::FillRespData(
    DMsgPtr& pMsg, CfgPtr& pResp )
{
    gint32 ret = 0;
    if( pMsg.IsEmpty() || pResp.IsEmpty() )
        return -EINVAL;

    CCfgOpener oCfg( ( IConfigDb* )pResp );
    if( pMsg.GetType() !=
        DBUS_MESSAGE_TYPE_METHOD_RETURN )
    {
        ret = -EBADMSG;
        oCfg.SetIntProp(
            propReturnValue, ret );
        return ret;
    }

    do{
        string strMethod = pMsg.GetMember();
        if( strMethod == SYS_METHOD_SENDDATA )
        {
            gint32 iRet = 0;
            ret = pMsg.GetIntArgAt(
                0, ( guint32& )iRet );

            if( ERROR( ret ) )
                break;

            ret = oCfg.SetIntProp(
                propReturnValue, iRet );

            if( ERROR( ret ) )
                break;
        }
        else if( strMethod == SYS_METHOD_FETCHDATA )
        {
            
            gint32 iRet = 0;
            CParamList oParams( pResp );
            ret = pMsg.GetIntArgAt(
                0, ( guint32& )iRet );

            if( ERROR( ret ) )
                break;

            ret = oParams.SetIntProp(
                propReturnValue, iRet );

            if( ERROR( ret ) )
                break;

            gint32 iFd = 0;
            guint32 dwOffset = 0;
            guint32 dwSize = 0;
            ObjPtr pObj;

            ret = pMsg.GetObjArgAt( 0, pObj );
            if( ERROR( ret ) )
                break;

            oParams.Push( pObj );

            ret = pMsg.GetFdArgAt( 1, iFd );

            if( ERROR( ret ) )
            {
                ret = pMsg.GetIntArgAt( 1,
                    ( guint32& )iFd );
                break;
            }

            oParams.Push( iFd );

            ret = pMsg.GetIntArgAt( 2, dwOffset );
            if( ERROR( ret ) )
                break;
            oParams.Push( dwOffset );

            ret = pMsg.GetIntArgAt( 3, dwSize );
            if( ERROR( ret ) )
                break;

            oParams.Push( dwSize );
        }
        else
        {
            // for the default requests, a
            // configDb ptr is returned
            ObjPtr pObj;
            gint32 iRet = 0;
            ret = pMsg.GetIntArgAt( 0, ( guint32& )iRet );
            if( ERROR( ret ) )
                break;

            if( ERROR( iRet ) )
            {
                oCfg[ propReturnValue ] = iRet;
                break;
            }

            ret = pMsg.GetObjArgAt( 1, pObj );
            if( ERROR( ret ) )
                break;

            IConfigDb* pCfg = pObj;
            if( pCfg == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            /* Fixed a bug by copying the response data
             * from GetObjArgAt to CIfIoReqTask's
             * propRespPtr, which was so but changed to
             * copying ptr sometimes for performance
             * consideration. Now there are chances the
             * CIfIoReqTask can complete immediately,
             * that is the CRpcServices::AddAndRun can
             * return successfully some times, which
             * requires the the response data in the
             * CIfIoReqTask to be filled with
             * meaningful return value, instead of
             * replacing the given response data ptr
             * from the top request caller with a newly
             * allocated response data. So we have to
             * copy the content from the config db
             * created by DMsgPtr::GetObjArgAt to the
             * pResp held by the CIfIoReqTask.
            */
            *pResp = *pCfg;
        }

    }while( 0 );

    return ret;
}

gint32 CRpcServices::SetupReqIrp( IRP* pIrp,
    IConfigDb* pReqCall,
    IEventSink* pCallback )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;

    do{
        CReqOpener oReq( pReqCall );
        pIrp->SetIrpThread( GetIoMgr() );

        IrpCtxPtr& pIrpCtx = pIrp->GetTopStack(); 
        pIrpCtx->SetMajorCmd( IRP_MJ_FUNC );
        pIrpCtx->SetMinorCmd( IRP_MN_IOCTL );

        guint32 dwIoDir = IRP_DIR_OUT;
        guint32 dwCallFlags = 0;

        ret = oReq.GetCallFlags( dwCallFlags );
        if( SUCCEEDED( ret ) )
        {
            if( dwCallFlags & CF_WITH_REPLY )
                dwIoDir = IRP_DIR_INOUT;

            if( dwCallFlags & CF_NON_DBUS )
                pIrpCtx->SetNonDBusReq( true );
        }

        bool bSet = false;
        string strMethod;
        ret = oReq.GetMethodName( strMethod );
        if( SUCCEEDED( ret ) )
        {
            bSet = true;
            if( strMethod ==
                IF_METHOD_DISABLEEVT )
            {
                pIrpCtx->SetCtrlCode(
                    CTRLCODE_UNREG_MATCH );
            }
            else if( strMethod ==
                IF_METHOD_ENABLEEVT )
            {
                pIrpCtx->SetCtrlCode(
                    CTRLCODE_REG_MATCH );
            }
            else if( strMethod ==
                IF_METHOD_LISTENING )
            {
                pIrpCtx->SetCtrlCode(
                    CTRLCODE_LISTENING );
                dwIoDir = IRP_DIR_IN;
            }
            else if( strMethod ==
                SYS_METHOD_FETCHDATA )
            {
                pIrpCtx->SetCtrlCode(
                    CTRLCODE_FETCH_DATA );
            }
            else if( strMethod ==
                SYS_METHOD_SENDDATA )
            {
                pIrpCtx->SetCtrlCode(
                    CTRLCODE_SEND_DATA );
            }
            else
            {
                bSet = false;
            }
        }

        if( !bSet )
        {
            gint32 iReqType = 0;
            ret = oReq.GetReqType(
                ( guint32& )iReqType );

            if( ERROR( ret ) )
                break;

            if( iReqType ==
                DBUS_MESSAGE_TYPE_METHOD_CALL )
            {
                pIrpCtx->SetCtrlCode(
                    CTRLCODE_SEND_REQ );
            }
            else if( iReqType ==
                DBUS_MESSAGE_TYPE_SIGNAL )
            {
                pIrpCtx->SetCtrlCode(
                    CTRLCODE_SEND_EVENT );
            }
            else if( iReqType ==
                DBUS_MESSAGE_TYPE_METHOD_RETURN ||
                iReqType ==
                DBUS_MESSAGE_TYPE_ERROR )
            {
                pIrpCtx->SetCtrlCode(
                    CTRLCODE_SEND_RESP );
            }
            else
            {
                ret = -EINVAL;
                break;
            }
        }

        pIrpCtx->SetIoDirection( dwIoDir ); 

        BufPtr pBuf( true );
        ret = BuildBufForIrp( pBuf, pReqCall );
        if( ERROR( ret ) )
            break;

        pIrpCtx->SetReqData( pBuf );

        if( pCallback != nullptr )
            pIrp->SetCallback( pCallback, 0 );

        guint32 dwTimeoutSec = 0;
        ret = oReq.GetTimeoutSec( dwTimeoutSec );

        if( SUCCEEDED( ret ) &&
            dwTimeoutSec != 0 )
        {
            pIrp->SetTimer(
                dwTimeoutSec, GetIoMgr() );
        }

    }while( 0 );

    return ret;
}

gint32 CRpcServices::InvokeUserMethod(
    IConfigDb* pParams,
    IEventSink* pCallback )
{
    if( pParams == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CReqOpener oReq( pParams );
        string strMethod;
        ret = oReq.GetMethodName( strMethod );
        if( ERROR( ret ) )
            break;

        TaskletPtr pTask;

        CCfgOpenerObj oCfg( pCallback );
        string strIfName;

        ret = oCfg.GetStrProp(
            propIfName, strIfName );
        if( ERROR( ret ) ) 
            break;

        strIfName = IF_NAME_FROM_DBUS( strIfName );

        ToInternalName( strIfName );
        EnumClsid iid =
            CoGetIidFromIfName( strIfName );

        ret = GetDelegate(
            strMethod, pTask, iid );

        if( ERROR( ret ) )
            break;

        CDelegateBase* pDelegate = pTask;
        if( pDelegate == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ret = ( *pDelegate )(
            this, pCallback, pParams ); 

    }while( 0 );

    return ret;
}

gint32 CRpcServices::SendMethodCall(
    IConfigDb* pReqCall,
    CfgPtr& pRespData,
    IEventSink* pCallback )
{
    gint32 ret = 0;
    if( pCallback == nullptr ||
        pReqCall == nullptr )
        return -EINVAL;

    if( pRespData.IsEmpty() )
        return -EFAULT;

    do{
        CStdRMutex oIfLock( GetLock() );
        if( GetState() != stateConnected )
        {
            ret = ERROR_STATE;
            break;
        }

        IrpPtr pIrp( true );
        ret = pIrp->AllocNextStack( nullptr );
        if( ERROR( ret ) )
            break;

        ret = SetupReqIrp(
            pIrp, pReqCall, pCallback );

        if( ERROR( ret ) )
            break;

        CInterfaceState* pIfState = m_pIfStat;
        if( pIfState == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        guint32 hPort = GetPortHandle();
        if( hPort == 0 )
        {
            ret = -EFAULT;
            break;
        }

        CReqOpener oReq( pReqCall );

        // set an irp for canceling purpose
        CCfgOpenerObj oCfg( pCallback );
        oCfg.SetObjPtr( propIrpPtr,
            ObjPtr( ( IRP* )pIrp ) );

        if( !oReq.exist( propSubmitPdo ) )
        {
            ret = GetIoMgr()->SubmitIrp( hPort, pIrp );
        }
        else
        {
            ret = GetIoMgr()->SubmitIrpInternal(
                HandleToPort( hPort ), pIrp, false );
        }

        if( ret == STATUS_PENDING )
            break;

        oCfg.RemoveProperty( propIrpPtr );
        pIrp->RemoveCallback();
        pIrp->RemoveTimer();

        if( ret == -EAGAIN )
            break;

        if( oReq.HasReply() ) 
        {
            ret = FillRespData( pIrp, pRespData );
        }

    }while( 0 );

    return ret;
}

FUNC_MAP* CRpcServices::GetFuncMap(
    EnumClsid iIfId )
{
    EnumClsid iEffectiveId = iIfId;
    if( iIfId == clsid( Invalid ) )
        iEffectiveId = GetClsid();

    FUNC_MAP* pMap = nullptr;

    std::map< gint32, FUNC_MAP >::iterator itr = 
        m_mapFuncs.find( iEffectiveId );

    if( itr == m_mapFuncs.end() )
        return nullptr;

    pMap = &itr->second;
    return pMap;
}

gint32 CRpcServices::SetFuncMap(
    FUNC_MAP& oMap, EnumClsid iIfId )
{
    if( oMap.empty() )
        return -EINVAL;

    EnumClsid iEffectiveId = iIfId;
    if( iIfId == clsid( Invalid ) )
        iEffectiveId = GetClsid();

    FUNC_MAP* pMap = GetFuncMap( iEffectiveId );
    if( pMap == nullptr )
    {
        m_mapFuncs[ iEffectiveId ] = oMap;
    }
    else
    {
        // to add new or override the existing
        pMap = &m_mapFuncs[ iEffectiveId ];
        for( auto elem : oMap )
            ( *pMap )[ elem.first ] = elem.second;
    }
    return 0;
}

PROXY_MAP* CRpcServices::GetProxyMap(
    EnumClsid iIfId )
{
    EnumClsid iEffectiveId = iIfId;
    if( iIfId == clsid( Invalid ) )
        iEffectiveId = GetClsid();

    std::map< gint32, PROXY_MAP >::iterator itr =
        m_mapProxyFuncs.find( iEffectiveId );

    if( itr == m_mapProxyFuncs.end() )
        return nullptr;

    PROXY_MAP* pMap = &itr->second;
    return pMap;
}

gint32 CRpcServices::SetProxyMap(
    PROXY_MAP& oMap, EnumClsid iIfId )
{
    if( oMap.empty() )
        return -EINVAL;

    EnumClsid iEffectiveId = iIfId;
    if( iIfId == clsid( Invalid ) )
        iEffectiveId = GetClsid();

    PROXY_MAP* pMap = GetProxyMap( iEffectiveId );
    if( pMap == nullptr )
    {
        m_mapProxyFuncs[ iEffectiveId ] = oMap;
    }
    else
    {
        // to add new or override the existing
        pMap = &m_mapProxyFuncs[ iEffectiveId ];
        for( auto elem : oMap )
            ( *pMap )[ elem.first ] = elem.second;
    }
    return 0;
}

gint32 CRpcServices::GetDelegate(
    const std::string& strFunc,
    TaskletPtr& pFunc,
    EnumClsid iIfId )
{
    EnumClsid iEffectiveId = iIfId;
    if( iIfId == clsid( Invalid ) )
        iEffectiveId = GetClsid();

    FUNC_MAP* pMap =
        GetFuncMap( iEffectiveId );

    if( pMap == nullptr )
        return -ERANGE;

    FUNC_MAP::iterator itrFunc =
        pMap->find( strFunc );

    if( itrFunc == pMap->end() )
        return -ERANGE;

    pFunc = itrFunc->second;
    return 0;
}

gint32 CRpcServices::GetProxy(
    const std::string& strFunc,
    ObjPtr& pProxy,
    EnumClsid iIfId )
{
    EnumClsid iEffectiveId = iIfId;
    if( iIfId == clsid( Invalid ) )
        iEffectiveId = GetClsid();

    PROXY_MAP* pMap =
        GetProxyMap( iEffectiveId );

    if( pMap == nullptr )
        return -ERANGE;

    PROXY_MAP::iterator itrFunc =
        pMap->find( strFunc );

    if( itrFunc == pMap->end() )
        return -ERANGE;

    pProxy = itrFunc->second;
    return 0;
}

gint32 CRpcServices::LoadObjDesc(
    const std::string& strFile,
    const std::string& strObjName,
    bool bServer,
    CfgPtr& pCfg )
{
    gint32 ret = 0;

    if( strObjName.empty() ||
        strFile.empty() )
        return -EINVAL;

    do{
        CCfgOpener oCfg( ( IConfigDb* )pCfg );

        // Load the object decription file in json
        Json::Value valObjDesc;
        ret = ReadJsonCfg( strFile, valObjDesc );
        if( ERROR( ret ) )
            break;

        // get BusName
        Json::Value& oBusName =
            valObjDesc[ JSON_ATTR_BUSNAME ];

        if( oBusName == Json::Value::null )
        {
            ret = -ENOENT;
            break;
        }
        
        string strVal = oBusName.asString();
        strVal = strVal.substr( 0, 255 );
        oCfg[ propBusName ] = strVal;

        // get PortClass
        Json::Value& oPortClass =
            valObjDesc[ JSON_ATTR_PORTCLASS ];

        if( oPortClass == Json::Value::null )
        {
            ret = -ENOENT;
            break;
        }

        strVal = oPortClass.asString();
        strVal = strVal.substr( 0, 255 );
        oCfg[ propPortClass ] = strVal;

        // get PortId
        Json::Value& oPortId =
            valObjDesc[ JSON_ATTR_PORTID ];

        if( oPortId == Json::Value::null )
        {
            ret = -ENOENT;
            break;
        }
        
        strVal = oPortId.asString();
        strVal = strVal.substr( 0, 255 );
        oCfg[ propPortId ] = std::stoi( strVal );

        // get ipaddr
        Json::Value& oIpAddr =
            valObjDesc[ JSON_ATTR_IPADDR ];

        if( oIpAddr != Json::Value::null )
        {
            strVal = oIpAddr.asString();
            if( !strVal.empty() )
            {
                strVal = strVal.substr( 0, 16 );
                oCfg[ propIpAddr ] = strVal;
            }
        }
        
        // get ServerName
        Json::Value& oServerName =
            valObjDesc[ JSON_ATTR_SVRNAME ];

        if( oServerName == Json::Value::null )
        {
            ret = -ENOENT;
            break;
        }
        
        string strSvrName = oServerName.asString();
        strSvrName = strSvrName.substr( 0, 255 );

        // get object array
        Json::Value& oObjArray =
            valObjDesc[ JSON_ATTR_OBJARR ];

        if( oObjArray == Json::Value::null ||
            !oObjArray.isArray() ||
            oObjArray.empty() )
        {
            ret = -ENOENT;
            break;
        }

        size_t i = 0;
        for( ; i < oObjArray.size(); i++ )
        {
            if( oObjArray[ i ].empty() ||
                !oObjArray[ i ].isObject() )
                continue;

            Json::Value& oObjElem = oObjArray[ i ];
            if( !oObjElem.isMember( JSON_ATTR_OBJNAME ) )
                continue;

            // find the section with `ObjName' is
            // strObjName
            if( oObjElem[ JSON_ATTR_OBJNAME ].asString()
                != strObjName )
                continue;

            // get the RequestTimeoutSec
            if( oObjElem.isMember( JSON_ATTR_REQ_TIMEOUT ) &&
                oObjElem[ JSON_ATTR_REQ_TIMEOUT ].isString() )
            {
                strVal = oObjElem[ JSON_ATTR_REQ_TIMEOUT ].asString(); 
                oCfg[ propTimeoutSec ] = std::stoi( strVal );
            }

            // get the KeepAliveTimeoutSec
            if( oObjElem.isMember( JSON_ATTR_KA_TIMEOUT ) &&
                oObjElem[ JSON_ATTR_KA_TIMEOUT ].isString() )
            {
                strVal = oObjElem[ JSON_ATTR_KA_TIMEOUT ].asString(); 
                oCfg[ propKeepAliveSec ] = std::stoi( strVal );
            }

            // overwrite the global port class and port
            // id if PROXY_PORTCLASS and PROXY_PORTID
            // exist
            if( oObjElem.isMember( JSON_ATTR_PROXY_PORTCLASS ) &&
                oObjElem[ JSON_ATTR_PROXY_PORTCLASS ].isString() &&
                !bServer )
            {
                strVal = oObjElem[ JSON_ATTR_PROXY_PORTCLASS ].asString(); 
                oCfg[ propPortClass ] = strVal;
            }

            if( oObjElem.isMember( JSON_ATTR_PROXY_PORTID ) &&
                oObjElem[ JSON_ATTR_PROXY_PORTID ].isString() &&
                !bServer )
            {
                strVal = oObjElem[ JSON_ATTR_PROXY_PORTID ].asString(); 
                oCfg[ propPortId ] = std::stoi( strVal );
            }

            Json::Value& oIfArray = 
                oObjElem[ JSON_ATTR_IFARR ];            

            if( oIfArray == Json::Value::null ||
                !oIfArray.isArray() ||
                oIfArray.empty() )
                ret = -ENOENT;

            break;
        }

        if( ERROR( ret ) )
            break;

        if( i == oObjArray.size() )
        {
            ret = -ENOENT;
            break;
        }

        // Found the object decriptor
        //
        // dbus related information
        string strObjPath = DBUS_OBJ_PATH(
            strSvrName, strObjName );

        oCfg[ propObjPath ] = strObjPath;

        string strDest( strObjPath );

        std::replace( strDest.begin(),
            strDest.end(), '/', '.');

        if( strDest[ 0 ] == '.' )
            strDest = strDest.substr( 1 );

        EnumMatchType iType =
            bServer ? matchServer : matchClient;
        oCfg[ propMatchType ] = iType;

        // set the destination dbus name
        if( bServer )
        {
            // useful when sending out event
            oCfg[ propSrcDBusName ] = strDest;
        }
        else
        {
            // important for proxy
            oCfg[ propDestDBusName ] = strDest;
        }


        Json::Value& oIfArray =
            oObjArray[ i ][JSON_ATTR_IFARR ];
        
        i = 0;

        ObjVecPtr pObjVec( true );

        for( ; i < oIfArray.size(); i++ )
        {
            Json::Value& oIfDesc = oIfArray[ i ];

            // get the interface name
            Json::Value& oIfName =
                oIfDesc[ JSON_ATTR_IFNAME ];

            if( oIfName == Json::Value::null ||
                !oIfName.isString() )
                continue;
            
            if( oIfDesc.isMember( JSON_ATTR_BIND_CLSID ) &&
                oIfDesc[ JSON_ATTR_BIND_CLSID ].isString() &&
                oIfDesc[ JSON_ATTR_BIND_CLSID ].asString() == "true" )
            {
                // the master interface
                strVal = oIfName.asString().substr( 0, 255 );
                if( strVal.empty() )
                {
                    ret = -EINVAL;
                    break;
                }
                oCfg[ propIfName ] = 
                    DBUS_IF_NAME( oIfName.asString() );

                continue;
            }

            MatchPtr pMatch;
            pMatch.NewObj( clsid( CMessageMatch ) );

            CCfgOpenerObj oMatch(
                ( CObjBase* )pMatch );

            oMatch.SetStrProp(
                propObjPath, strObjPath );

            oMatch.SetIntProp( propMatchType, iType );

            oMatch.SetStrProp( propIfName,
                DBUS_IF_NAME( oIfName.asString() ) );

            if( bServer )
            {
                oMatch.SetStrProp(
                    propSrcDBusName, strDest );
            }
            else
            {
                // this will be required by the 
                // CRpcPdoPort::SetupDBusSetting
                oMatch.SetStrProp(
                    propDestDBusName, strDest );
            }

            ( *pObjVec )().push_back( pMatch );
        }

        if( ERROR( ret ) )
            break;

        if( ( *pObjVec )().size() )
        {
            ObjPtr pObj( pObjVec );
            oCfg.SetObjPtr( propObjList, pObj );
        }

    }while( 0 );

    return ret;
}

gint32 CRpcServices::PackEvent(
    EnumEventId iEvent,
    guint32 dwParam1,
    guint32 dwParam2,
    guint32* pData,
    CfgPtr& pDestCfg  )
{
    gint32 ret = 0;

    CParamList oParams;
    oParams.Push( ( guint32 )iEvent );

    switch( iEvent )
    {
    case eventPortStarted:
    case eventPortStopping:
    case eventPortStartFailed:
        {
            // push the port handle
            oParams.Push( dwParam1 );
            break;
        }
    case eventPortStopped:
        {
            break;
        }
    case eventModOnline:
    case eventModOffline:
        {
            oParams.Push( 0 );
            oParams.Push( 0 );

            string strMod =
                reinterpret_cast< char* >( pData );
            oParams.Push( strMod );

            break;
        }
    case eventRmtModOnline:
    case eventRmtModOffline:
        {

            string strIpAddr =
                reinterpret_cast< char* >( dwParam1 );

            oParams.Push( strIpAddr );
            oParams.Push( 0 );

            string strMod =
                reinterpret_cast< char* >( pData );
            oParams.Push( strMod );

            break;
        }
    case eventDBusOnline:
    case eventDBusOffline:
        {
            break;
        }
    case eventRmtDBusOffline:
    case eventRmtDBusOnline:
    case eventRmtSvrOnline:
    case eventRmtSvrOffline:
        {
            string strIpAddr = reinterpret_cast
                < char* >( dwParam1 );
            oParams.Push( strIpAddr );

            oParams.Push( 0 );

            HANDLE hPort = ( guint32 )pData;
            oParams.Push( hPort );
            break;
        }
    default:
        {
            ret = -ENOTSUP;
            break;
        }
    }

    if( SUCCEEDED( ret ) )
        pDestCfg = oParams.GetCfg();
    return ret;
}

gint32 CRpcServices::UnpackEvent(
    CfgPtr& pSrcCfg,
    EnumEventId& iEvent,
    guint32& dwParam1,
    guint32& dwParam2,
    guint32*& pData )
{
    gint32 ret = 0;

    CParamList oParams(
        ( IConfigDb*) pSrcCfg );

    iEvent = oParams[ 0 ];
    dwParam1 = 0;
    dwParam2 = 0;
    pData = nullptr;

    switch( iEvent )
    {
    case eventPortStarted:
    case eventPortStopping:
    case eventPortStartFailed:
        {
            // push the port handle
            dwParam1 = oParams[ 1 ];
            break;
        }
    case eventPortStopped:
        {
            break;
        }
    case eventModOnline:
    case eventModOffline:
        {
            BufPtr pBuf;
            ret = oParams.GetProperty( 3, pBuf );
            if( ERROR( ret ) )
                break;
            pData = ( guint32* )pBuf->ptr();

            break;
        }
    case eventRmtModOnline:
    case eventRmtModOffline:
        {
            string strMod =
                reinterpret_cast< char* >( pData );

            string strIpAddr =
                reinterpret_cast< char* >( dwParam1 );

            BufPtr pBuf;
            ret = oParams.GetProperty( 1, pBuf );
            if( ERROR( ret ) )
                break;
            dwParam1 = ( guint32 )pBuf->ptr();

            ret = oParams.GetProperty( 3, pBuf );
            if( ERROR( ret ) )
                break;

            pData = ( guint32* )pBuf->ptr();
            break;
        }
    case eventDBusOnline:
    case eventDBusOffline:
        {
            break;
        }
    case eventRmtDBusOffline:
    case eventRmtDBusOnline:
    case eventRmtSvrOnline:
    case eventRmtSvrOffline:
        {
            BufPtr pBuf;
            ret = oParams.GetProperty( 1, pBuf );
            if( ERROR( ret ) )
                break;
            dwParam1 = ( guint32 )pBuf->ptr();

            guint32 hPort = oParams[ 3 ];
            pData = ( guint32* )hPort;
            break;
        }
    default:
        {
            ret = -ENOTSUP;
            break;
        }
    }
    return ret;
}

gint32 CRpcServices::OnEvent(
    EnumEventId iEvent,
    guint32 dwParam1,
    guint32 dwParam2,
    guint32* pData  )
{
    gint32 ret = 0;

    bool bProcess = false;

    CStdRMutex oIfLock( GetLock() );
    if( m_queEvents.size() == 0 )
        bProcess = true;

    CfgPtr pParams;
    ret = PackEvent( iEvent, dwParam1,
        dwParam2, pData, pParams );

    if( ERROR( ret ) )
        return ret;

    m_queEvents.push_back( pParams );

    if( !bProcess )
        return ret;

    oIfLock.Unlock();

    do{
        ret = super::OnEvent( iEvent,
            dwParam1, dwParam2, pData );

        oIfLock.Lock();
        m_queEvents.pop_front();
        if( m_queEvents.empty() )
            break;

        CfgPtr pParams =
            m_queEvents.front();

        oIfLock.Unlock();

        ret = UnpackEvent( pParams, iEvent,
            dwParam1, dwParam2, pData );

        if( ERROR( ret ) )
            break;

    }while( 1 );

    return ret;
}

gint32 CRpcServices::RestartListening()
{
    gint32 ret = 0;
    CParamList oParams;
    TaskletPtr pRecvMsgTask;

    oParams[ propIfPtr ] = ObjPtr( this );
    for( auto pMatch : m_vecMatches )
    {
        oParams[ propMatchPtr ] =
            ObjPtr( pMatch );

        ret = pRecvMsgTask.NewObj(
            clsid( CIfStartRecvMsgTask ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        ret = AddAndRun( pRecvMsgTask );

        if( ERROR( ret ) )
            break;
    }

    return ret;
}

gint32 CRpcServices::DoModEvent(
    EnumEventId iEvent,
    const std::string& strModule )
{
    EnumIfState iOldState = GetState();

    gint32 ret = m_pIfStat->OnModEvent(
        iEvent, strModule );

    if( SUCCEEDED( ret ) )
    {
        EnumIfState iNewState = GetState();
        if( iNewState == stateConnected &&
            iOldState == stateRecovery )
        {
            // let's send new request to listen on the
            // events
            ret = RestartListening();
        }
    }
    return ret;
}

/**
* @name DoInvoke
* find a event handler for the event message
* pEvtMsg and handle it.
* This method is usually not expected to return
* STATUS_PENDING unless you are aware what you are
* doing
* @{ */
/** pEvtMsg [ in ]: the dbus message with the
 * event
 * pCallback [ in ]: the callback carrying the
 * related CIfInvokeMethodTask with the related
 * config parameters, and the callback in case the
 * call will complete asynchronously @} */
gint32 CInterfaceProxy::DoInvoke(
    DBusMessage* pEvtMsg,
    IEventSink* pCallback )
{
    gint32 ret = 0;

    do{
        if( pEvtMsg == nullptr ||
            pCallback == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        DMsgPtr pMsg( pEvtMsg );

        if( pMsg.GetType()
            != DBUS_MESSAGE_TYPE_SIGNAL )
        {
            ret = -EINVAL;
            break;
        }

        ObjPtr pObj;
        ret = pMsg.GetObjArgAt( 0, pObj );
        if( ERROR( ret ) )
            break;

        IConfigDb* pCfg = pObj;
        if( pCfg == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        string strMethod = pMsg.GetMember();
        if( strMethod.empty() )
        {
            ret = -EBADMSG;
            break;
        }
        else if( strMethod ==
            SYS_EVENT_KEEPALIVE )
        {
            ret = OnKeepAlive(
                pCallback, KATerm );
            break;
        }

        ret = InvokeUserMethod(
            pCfg, pCallback );

    }while( 0 );

    return ret;
}

gint32 CInterfaceProxy::DoInvoke(
    IConfigDb* pEvtMsg,
    IEventSink* pCallback )
{
    gint32 ret = 0;

    do{
        if( pEvtMsg == nullptr ||
            pCallback == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        CReqOpener oReq( pEvtMsg );
        gint32 iReqType = 0;

        ret = oReq.GetReqType(
            ( guint32& )iReqType ); 

        if( ERROR( ret ) )
            break;

        if( iReqType != DBUS_MESSAGE_TYPE_SIGNAL )
        {
            ret = -EINVAL;
            break;
        }

        string strMethod;
        ret = oReq.GetMethodName( strMethod );
        if( ERROR( ret ) )
            break;

        ret = InvokeUserMethod(
            pEvtMsg, pCallback );

    }while( 0 );

    return ret;
}

gint32 CInterfaceProxy::OnCancel_Proxy(
    IConfigDb* pReqMsg,
    IEventSink* pCallback )
{
    if( pReqMsg == nullptr ||
        pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        TaskletPtr pTasklet;
        pTasklet = ObjPtr( pCallback ); 
        CIfParallelTask* pTask = pTasklet;

        if( pTask == nullptr )
        {
            ret = -EFAULT;
            break;
        }

    }while( 0 );

    return ret;
}

gint32 CInterfaceProxy::OnKeepAlive(
    IEventSink* pTask, EnumKAPhase iPhase )
{
    if( pTask == nullptr )
        return -EINVAL;

    if( iPhase == KATerm )
    {
        return OnKeepAliveTerm( pTask );
    }

    return -EINVAL;
}

gint32 CInterfaceProxy::OnKeepAliveTerm(
    IEventSink* pTask )
{
    gint32 ret = 0;

    // servicing the incoming request/event
    // message
    do{
        DMsgPtr pMsg;
        CCfgOpenerObj oCfg( pTask );

        ret = oCfg.GetMsgPtr( propMsgPtr, pMsg );
        if( ERROR( ret ) )
            break;

        if( pMsg.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        if( GetState() != stateConnected )
        {
            // NOTE: this is not a serious state
            // check
            ret = ERROR_STATE;
            break;
        }

        ObjPtr pObj;
        ret = pMsg.GetObjArgAt( 0, pObj );
        if( ERROR( ret ) )
            break;

        
        IConfigDb* pCfg = pObj;
        if( pCfg == nullptr )
        {
            ret = -EFAULT;
            break;
        }

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
            if( GetState() != stateConnected )
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

        ret = pTaskToKa->OnEvent( iEvent,
            ( guint32 )KATerm, 0, nullptr );

    }while( 0 );

    return STATUS_PENDING;
}

gint32 CInterfaceProxy::SendResponse(
    DBusMessage* pReqMsg,
    CfgPtr& pRespData )
{
    // called to send back the response
    return 0;
}

gint32 CInterfaceProxy::SendResponse(
    IConfigDb* pReqMsg,
    CfgPtr& pRespData )
{
    // called to send back the response
    return 0;
}

// don't use this method unless you know what you are doing
gint32 CInterfaceProxy::SendProxyReq(
    IEventSink* pCallback,
    bool bNonDBus,
    const string& strMethod,
    std::vector< BufPtr >& vecParams,
    guint64& qwIoTaskId )
{
    gint32 ret = 0;
    bool bSync = false;

    if( pCallback == nullptr )
    {
        // we won't let the call afterwards
        // without a callback
        bSync = true;
        TaskletPtr pTask;
        ret = pTask.NewObj( clsid( CSyncCallback ) );
        if( ERROR( ret ) )
            return ret;
        pCallback = pTask;
    }

    do{
        CReqBuilder oReq( this );

        // overwrite the strIfName if exist in the
        // pCallback
        oReq.CopyProp( propIfName, pCallback );
        oReq.SetMethodName( strMethod );

        for( auto& pBuf: vecParams )
            oReq.Push( *pBuf );

        guint32 dwFlags = CF_ASYNC_CALL |
            DBUS_MESSAGE_TYPE_METHOD_CALL |
            CF_WITH_REPLY;

        if( bNonDBus )
        {
            dwFlags |= CF_NON_DBUS;
        }
        else
        {
            // FIXME: what does this means
            oReq.SetIntProp( propCmdId,
                CTRLCODE_SEND_REQ );
        }

        oReq.SetCallFlags( dwFlags );
        CustomizeRequest( oReq.GetCfg(), pCallback );

        CfgPtr pResp( true );
        gint32 iRet = 0;

        ret = RunIoTask( oReq.GetCfg(),
            pResp, pCallback, &qwIoTaskId );

        if( ret == STATUS_PENDING )
        {
            if( bSync )
            {
                CSyncCallback* pSyncCallback =
                    ObjPtr( pCallback );
                if( pSyncCallback == nullptr )
                {
                    ret = -EFAULT;
                    break;
                }
                ret = pSyncCallback->WaitForComplete();
            }
            else
            {
                break;
            }
        }

        if( ERROR( ret ) )
            break;

        CReqOpener oResp( ( IConfigDb* )pResp );
        ret = oResp.GetReturnValue( iRet );

        if( ERROR( ret ) )
            break;

        if( !bSync )
        {
            // sometimes we can land here successfully
            // in the highly concurrent environment
            CCfgOpenerObj oCallback( pCallback );
            oCallback.SetObjPtr( propRespPtr,
                ObjPtr( pResp ) );
        }

        ret = iRet;

    }while( 0 );

    return ret;
}

gint32 CInterfaceProxy::InitUserFuncs()
{
    gint32 ret = super::InitUserFuncs();
    if( ERROR( ret ) )
        return ret;

    // since we are contacting the remote
    // IInterfaceServer, so the interface name should
    // be IInterfaceServer
    BEGIN_IFPROXY_MAP( IInterfaceServer, false );

    ADD_PROXY_METHOD( SYS_METHOD_USERCANCELREQ, guint64 );

    END_IFPROXY_MAP;

    return 0;
}

// a user-initialized cancel request
gint32 CInterfaceProxy::UserCancelRequest(
	IEventSink* pCallback,
	guint64& qwThisTaskId,
	guint64 qwTaskToCancel )
{
	if( qwTaskToCancel == 0 )
		return -EINVAL;

    bool bRemove = false;
    CCfgOpenerObj oCfg( pCallback );
    if( !oCfg.exist( propIfName ) )
    {
        string strIfName = CoGetIfNameFromIid(
            iid( IInterfaceServer ) );

        if( strIfName.empty() )
            return ERROR_FAIL;

        ToPublicName( strIfName );

        // FIXME: not a good solution
        oCfg.SetStrProp( propIfName,
            DBUS_IF_NAME( strIfName ) );

        bRemove = true;
    }

    // a sample code snippet
    gint32 ret =  CallProxyFunc( pCallback,
		qwThisTaskId,
        SYS_METHOD( __func__ ),
		qwTaskToCancel );

    if( bRemove )
    {
        oCfg.RemoveProperty( propIfName );
    }
    return ret;
}

gint32 CInterfaceProxy::CancelRequest(
    guint64 qwTaskId )
{
    if( qwTaskId == 0 )
        return -EINVAL;

    gint32 ret = 0;

    do{
        TaskletPtr pTask; 
        ret = pTask.NewObj(
            clsid( CIoReqSyncCallback ) ); 

        if( ERROR( ret ) ) 
            break; 

        // this is a built-in request
        guint64 qwThisTask = 0;
        ret = UserCancelRequest(
            pTask, qwThisTask, qwTaskId );

        if( ERROR( ret ) )
            break;

        if( ret == STATUS_PENDING )
        {
            CIoReqSyncCallback* pSyncTask = pTask;
            pSyncTask->WaitForComplete();
            if( SUCCEEDED( ret ) )
                ret = pSyncTask->GetError();
        }
        if( ERROR( ret ) )
            break;

        ObjPtr pObj; 
        CCfgOpenerObj oTask( ( CObjBase* )pTask ); 
        ret = oTask.GetObjPtr( propRespPtr, pObj ); 
        if( ERROR( ret ) ) 
            break; 

        CfgPtr pResp = pObj; 
        if( pResp.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        CParamList oResp( ( IConfigDb* )pResp );
        if( !oResp.exist( propReturnValue ) )
        {
            ret = ERROR_FAIL;
            break;
        }

        ret = oResp[ propReturnValue ];

   }while( 0 ); 

   return ret;
}

static CfgPtr InitIfSvrCfg(
    const IConfigDb * pCfg )
{
    CCfgOpener oNewCfg;
    *oNewCfg.GetCfg() = *pCfg;

    oNewCfg.SetIntProp(
        propIfStateClass,
        clsid( CIfServerState ) );

    return oNewCfg.GetCfg();
}

CInterfaceServer::CInterfaceServer(
    const IConfigDb* pCfg )
    : super( InitIfSvrCfg( pCfg ) )
{
    gint32 ret = 0;

    do{
        CCfgOpener oCfg( pCfg );
        string strPortClass;

        if( !oCfg.exist( propPortClass ) )
        {
            ret = -EINVAL;
            break;
        }

        ret = m_pIfMatch.NewObj(
            clsid( CMessageMatch ), pCfg );

        if( ERROR( ret ) )
            break;

        CCfgOpenerObj matchCfg(
            ( IMessageMatch* ) m_pIfMatch );

        // no propDestDBusName
        ret = matchCfg.SetStrProp(
            propDestDBusName,
            DBUS_DESTINATION( MODNAME_INVALID ) );
        

        ret = matchCfg.CopyProp(
            propIfName, pCfg );

        if( ERROR( ret ) )
            break;

        ret = matchCfg.CopyProp(
            propObjPath, pCfg );

        if( ERROR( ret ) )
            break;

        ret = matchCfg.SetIntProp(
            propMatchType,
            ( guint32 )matchServer );

        if( ERROR( ret ) )
            break;

    }while( 0 );

    if( ERROR( ret ) )
    {
        string strMsg = DebugMsg( ret,
            "Error in CInterfaceServer ctor" );
        throw std::runtime_error( strMsg );
    }
}

CInterfaceServer::~CInterfaceServer()
{
    if( !m_pIfStat.IsEmpty() )
        m_pIfStat.Clear();

    if( !m_pIfMatch.IsEmpty() )
        m_pIfMatch.Clear();
}

// called to hook the response to the task for
// sending back
gint32 CInterfaceServer::SetResponse(
    IEventSink* pTask,
    CfgPtr& pRespData )
{
    gint32 ret = 0;
    if( pTask == nullptr ||
        pRespData.IsEmpty() )
        return -EINVAL;
    
    CCfgOpenerObj oTaskCfg( pTask );
    ObjPtr pObj;
    pObj = pRespData;
    if( pObj.IsEmpty() )
        return -EFAULT;

    // set the response pointer
    ret = oTaskCfg.SetObjPtr(
        propRespPtr, pObj );

    return ret;
}

gint32 CInterfaceServer::DoInvoke(
    DBusMessage* pReqMsg,
    IEventSink* pCallback )
{
    gint32 ret = 0;
    bool bCloseFile = false;
    gint32 iFd2Close = -1;

    do{
        bool bResp = true;
        CCfgOpenerObj oCfg( this );
        DMsgPtr pMsg( pReqMsg );
        CParamList oResp;

        string strMethod = pMsg.GetMember();
        if( strMethod.empty() )
        {
            ret = -EBADMSG;
            break;
        }

        vector< ARG_ENTRY > vecArgs;
        ret = pMsg.GetArgs( vecArgs );
        if( ERROR( ret ) )
            break;

        CfgPtr pDataDesc;
        do{
            if( strMethod == SYS_METHOD_SENDDATA )
            {
                guint32 iArgCount = GetArgCount(
                    &CInterfaceServer::SendData );

                if( vecArgs.empty()
                    || vecArgs.size() != iArgCount )
                {
                    ret = -EBADMSG;
                    break;
                }

                BufPtr pBuf( true );

                ret = pBuf->Deserialize(
                    *( CBuffer* )vecArgs[ 0 ].second );

                if( ERROR( ret ) )
                    break;

                pDataDesc = ObjPtr( pBuf );
                if( pDataDesc.IsEmpty() )
                {
                    ret = -EBADMSG;
                    break;
                }

                gint32 fd =
                    ( gint32& )*vecArgs[ 1 ].second;

                guint32 dwOffset =
                    ( guint32& )*vecArgs[ 2 ].second;

                guint32 dwSize =
                    ( guint32& )*vecArgs[ 3 ].second;

                if( pDataDesc->exist( propCallOptions ) )
                {
                    CfgPtr pCallOpt( true );
                    if( !pCallOpt.IsEmpty() )
                    {
                        ret = oCfg.CopyProp(
                            propCallOptions,
                            ( IConfigDb* )pDataDesc );
                    }
                }

                ret = SendData( pDataDesc, fd,
                    dwOffset, dwSize, pCallback );

                if( ret == STATUS_PENDING )
                {
                    CCfgOpenerObj oTaskCfg( pCallback );
                    SET_RMT_TASKID(
                        ( IConfigDb* )pDataDesc, oTaskCfg );
                    // the return value will be send
                    // in OnSendDataComplete
                    break;
                }
                ret = oResp.SetIntProp(
                    propReturnValue, ret );

                break;
            }
            else if( strMethod == SYS_METHOD_FETCHDATA )
            {

                guint32 iArgCount = GetArgCount(
                    &CInterfaceServer::FetchData );

                if( vecArgs.empty()
                    || vecArgs.size() != iArgCount )
                {
                    ret = -EBADMSG;
                    break;
                }

                BufPtr pBuf( true );

                ret = pBuf->Deserialize(
                    *( CBuffer* )vecArgs[ 0 ].second );

                if( ERROR( ret ) )
                    break;

                pDataDesc = ObjPtr( pBuf );
                if( pDataDesc.IsEmpty() )
                {
                    ret = -EBADMSG;
                    break;
                }

                if( pDataDesc->exist( propCallOptions ) )
                {
                    CfgPtr pCallOpt( true );
                    if( !pCallOpt.IsEmpty() )
                    {
                        ret = oCfg.CopyProp(
                            propCallOptions,
                            ( IConfigDb* )pDataDesc );
                    }
                }

                gint32 fd = -1;

                guint32 dwOffset =
                    ( guint32& )*vecArgs[ 2 ].second;

                guint32 dwSize =
                    ( guint32& )*vecArgs[ 3 ].second;

                ret = FetchData( pDataDesc, fd,
                    dwOffset, dwSize, pCallback );

                if( ret == STATUS_PENDING )
                {
                    // the return value will be send
                    // in OnFetchDataComplete
                    CCfgOpenerObj oTaskCfg( pCallback );
                    SET_RMT_TASKID(
                        ( IConfigDb* )pDataDesc, oTaskCfg );
                    break;
                }

                gint32 ret2 = oResp.SetIntProp(
                    propReturnValue, ret );

                if( SUCCEEDED( ret2 ) &&
                    SUCCEEDED( ret ) )
                {
                    oResp.Push( ObjPtr( pDataDesc ) );
                    oResp.Push( fd );
                    oResp.Push( dwOffset );
                    oResp.Push( dwSize );
                    bCloseFile = true;
                    iFd2Close = fd;
                    
                }
                else if( SUCCEEDED( ret ) )
                {
                    ret = ret2;
                }
                break;
            }
            else
            {
                ret = -ENOTSUP;
                break;
            }

        }while( 0 );

        if( ret == -ENOTSUP )
        {
            ret = 0;
            ObjPtr pObj;

            bResp = false;
            ret = pMsg.GetObjArgAt( 0, pObj );
            if( ERROR( ret ) )
                break;

            IConfigDb* pCfg = pObj;
            if( pCfg == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            ret = InvokeUserMethod(
                pCfg, pCallback );

            CReqOpener oReq( pCfg );

            guint32 dwFlags = 0;
            gint32 iRet = oReq.GetCallFlags( dwFlags );

            if( SUCCEEDED( iRet ) && 
                ( dwFlags & CF_WITH_REPLY ) )
                bResp = true;

            if( ret == STATUS_PENDING )
            {
                // for keep-alive purpose
                CCfgOpenerObj oTaskCfg( pCallback );
                SET_RMT_TASKID( pCfg, oTaskCfg );
            }
        }

        if( ret == STATUS_PENDING )
            break;

        if( ERROR( ret ) && bResp )
        {
            oResp.SetIntProp(
                propReturnValue, ret );

            SetResponse( pCallback,
                oResp.GetCfg() );
        }

    }while( 0 );

    if( bCloseFile )
        close( iFd2Close );

    return ret;
}

gint32 CInterfaceServer::DoInvoke(
    IConfigDb* pEvtMsg,
    IEventSink* pCallback )
{
    gint32 ret = 0;

    do{
        if( pEvtMsg == nullptr ||
            pCallback == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        CReqOpener oReq( pEvtMsg );
        gint32 iReqType = 0;

        ret = oReq.GetReqType(
            ( guint32& )iReqType ); 

        if( ERROR( ret ) )
            break;

        if( iReqType != DBUS_MESSAGE_TYPE_METHOD_CALL )
        {
            ret = -EINVAL;
            break;
        }

        string strMethod;
        ret = oReq.GetMethodName( strMethod );
        if( ERROR( ret ) )
            break;

        ret = InvokeUserMethod(
            pEvtMsg, pCallback );

    }while( 0 );

    return ret;
}

gint32 CInterfaceServer::OnServiceComplete(
    IConfigDb* pResp,
    IEventSink* pCallback )
{
    // this method is called from outside the
    // CIfInvokeMethodTask if the invoke method is
    // complete asynchronously
    if( pResp == nullptr ||
        pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    CfgPtr pRespPtr( pResp );
    ret = SetResponse(
        pCallback, pRespPtr );

    if( ERROR( ret ) )
        return ret;

    gint32 iRet = 0;
    CCfgOpener oCfg( pResp );

    ret = oCfg.GetIntProp(
        propReturnValue, ( guint32& )iRet );

    if( ERROR( ret ) )
        return ret;

    pCallback->OnEvent(
        eventTaskComp, iRet, 0, 0 );

    return ret;
}

gint32 CInterfaceServer::SendResponse(
    DBusMessage* pReqMsg,
    CfgPtr& pRespData )
{
    // called to send back the response
    if( pReqMsg == nullptr ||
        pRespData.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0; 
    do{
        DMsgPtr pMsg( pReqMsg );
        if( pMsg.GetType() ==
            DBUS_MESSAGE_TYPE_METHOD_RETURN )
        {
            ret = -EINVAL;
            break;
        }

        string strMethod = pMsg.GetMember( );
        DMsgPtr pRespMsg;
        ret = pRespMsg.NewResp( pReqMsg );
        if( ERROR( ret ) )
            break;

        CParamList oParams(
            ( IConfigDb* )pRespData );

        gint32 iRet = 0;
        ret = oParams.GetIntProp(
            propReturnValue, ( guint32& )iRet );

        if( ERROR( ret ) )
            break;

        if( strMethod == SYS_METHOD_SENDDATA )
        {
            
            if( !dbus_message_append_args( pRespMsg,
                DBUS_TYPE_UINT32, &iRet ,
                DBUS_TYPE_INVALID ) )
            {
                ret = -ENOMEM;
                break;
            }
        }
        else if( strMethod == SYS_METHOD_FETCHDATA )
        {
            guint32 dwSize = 0;
            guint32 dwOffset = 0;
            gint32 iFd = 0;
            ObjPtr pDataDesc;
            BufPtr pBuf( true );

            if( SUCCEEDED( iRet ) )
            {
                ret = oParams.Pop( dwSize );
                if( ERROR( ret ) )
                    break;

                ret = oParams.Pop( dwOffset );
                if( ERROR( ret ) )
                    break;

                ret = oParams.Pop( iFd );
                if( ERROR( ret ) )
                    break;

                ret = oParams.Pop( pDataDesc );
                if( ERROR( ret ) )
                    break;

                ret = pDataDesc->Serialize( *pBuf );
                if( ERROR( ret ) )
                    break;
            }

            gint32 iFdType = DBUS_TYPE_UNIX_FD;

#ifdef _WIN32
            if( iFd >= 0 )
            {
                // No fd transfer in the Windows
                // world yet
                ret = -ENOTSUP;
                break;
            }
#else
            if( iFd < 0 )
                iFdType = DBUS_TYPE_UINT32;
#endif

            if( !dbus_message_append_args( pRespMsg,
                DBUS_TYPE_UINT32, &iRet,
                DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
                &pBuf->ptr(), pBuf->size(),
                iFdType, &iFd,
                DBUS_TYPE_UINT32, &dwOffset,
                DBUS_TYPE_UINT32, &dwSize,
                DBUS_TYPE_INVALID ) )
            {
                ret = -ENOMEM;
                break;
            }
        }
        else if( strMethod == SYS_METHOD_FORWARDREQ )
        {
            DMsgPtr pMsg2Fwdr;
            BufPtr pBuf( true );

            ret = oParams.Pop( pMsg2Fwdr );
            if( SUCCEEDED( ret ) )
            {
                if( pMsg2Fwdr.IsEmpty() )
                {
                    ret = -EFAULT;
                    break;
                }
                ret = pMsg2Fwdr.Serialize( *pBuf );
                if( ERROR( ret ) )
                    break;
            }

            if( pBuf->size() )
            {
                if( !dbus_message_append_args( pRespMsg,
                    DBUS_TYPE_UINT32, &iRet,
                    DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
                    &pBuf->ptr(), pBuf->size(),
                    DBUS_TYPE_INVALID ) )
                {
                    ret = -ENOMEM;
                    break;
                }
            }
            else
            {
                if( !dbus_message_append_args( pRespMsg,
                    DBUS_TYPE_UINT32, &iRet,
                    DBUS_TYPE_INVALID ) )
                {
                    ret = -ENOMEM;
                    break;
                }
            }
        }
        else if( strMethod == SYS_METHOD_ENABLERMTEVT ||
            strMethod == SYS_METHOD_DISABLERMTEVT )
        {
            if( !dbus_message_append_args( pRespMsg,
                DBUS_TYPE_UINT32, &iRet,
                DBUS_TYPE_INVALID ) )
            {
                ret = -ENOMEM;
                break;
            }
        }
        else
        {
            TaskletPtr pTask;

            // NOTE: no need to check if the method is
            // valid or not. It has be done already
            BufPtr pBuf( true );
            ret = pRespData->Serialize( *pBuf );  
            if( ERROR( ret ) )
                break;

            if( !dbus_message_append_args( pRespMsg,
                DBUS_TYPE_UINT32, &iRet,
                DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
                &pBuf->ptr(), pBuf->size(),
                DBUS_TYPE_INVALID ) )
            {
                ret = -ENOMEM;
                break;
            }
        }

        CIoManager* pMgr = GetIoMgr();
        IrpPtr pIrp;

        ret = pIrp.NewObj();

        if( ERROR( ret ) )
            break;

        ret = pIrp->AllocNextStack( nullptr );
        if( ERROR( ret ) )
            break;

        guint32 dwCtrlCode = CTRLCODE_SEND_RESP;
        IrpCtxPtr& pIrpCtx = pIrp->GetTopStack(); 

        pIrpCtx->SetMajorCmd( IRP_MJ_FUNC );
        pIrpCtx->SetMinorCmd( IRP_MN_IOCTL );
        pIrpCtx->SetCtrlCode( dwCtrlCode );
        pIrpCtx->SetIoDirection( IRP_DIR_OUT ); 

        BufPtr pBuf( true );
        *pBuf = pRespMsg;
        pIrpCtx->SetReqData( pBuf );

        CInterfaceState* pIfState = m_pIfStat;
        if( pIfState == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        TaskletPtr pTask;
        ret = pTask.NewObj( clsid( CIfDummyTask ) );

        if( SUCCEEDED( ret ) )
        {
            EventPtr pEvent;
            pEvent = pTask;
            // NOTE: we have fed a dummy task here
            pIrp->SetCallback( pEvent, 0 );
        }

        pIrp->SetTimer(
            IFSTATE_ENABLE_EVENT_TIMEOUT, GetIoMgr() );

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

gint32 CInterfaceServer::SendResponse(
    IConfigDb* pReqMsg,
    CfgPtr& pRespData )
{
    // called to send back the response
    if( pReqMsg == nullptr ||
        pRespData.IsEmpty() )
        return -EINVAL;

    return -ENOTSUP;
}

// send a dbus signal, called from the user
// code
gint32 CInterfaceServer::BroadcastEvent(
    IConfigDb* pReqCall,
    IEventSink* pCallback )
{
    if( pReqCall == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{

        CParamList oRespCfg;

        CfgPtr pRespCfg = oRespCfg.GetCfg();
        if( pRespCfg.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        ret = RunIoTask( pReqCall,
            pRespCfg, pCallback );

        if( ret == STATUS_PENDING )
            break;

        if( SUCCEEDED( ret ) )
        {
            CCfgOpener oCfg(
                ( IConfigDb* )pRespCfg );

            gint32 iRet = 0;
            ret = oCfg.GetIntProp(
                propReturnValue,
                ( guint32& )iRet );

            if( ERROR( ret ) )
                break;

            ret = iRet;
        }

    }while( 0 );

    return ret;
}

gint32 CInterfaceServer::OnKeepAlive(
    IEventSink* pTask, EnumKAPhase iPhase )
{
    if( iPhase == KAOrigin )
    {
        return OnKeepAliveOrig( pTask );
    }

    return -EINVAL;
}

/**
* @name OnKeepAliveOrig
* a event handler when the keep-alive timer is out
* @{ */
/** 
 * this is where the keepalive event originates.
 * @} */
gint32 CInterfaceServer::OnKeepAliveOrig(
    IEventSink* pTask )
{
    if( pTask == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CCfgOpenerObj oCfg( pTask );
        DMsgPtr pMsg;
        ret = oCfg.GetMsgPtr( propMsgPtr, pMsg );
        if( ERROR( ret ) )
            break;

        if( pMsg.GetType() !=
            DBUS_MESSAGE_TYPE_METHOD_CALL )
        {
            ret = -ENOTSUP;
            break;
        }
        if( GetState() != stateConnected )
        {
            // NOTE: this is not a serious state
            // check
            ret = ERROR_STATE;
            break;
        }

        ObjPtr pObj;
        ret = pMsg.GetObjArgAt( 0, pObj );
        if( ERROR( ret ) )
            break;

        IConfigDb* pCfg = pObj;
        if( pCfg == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        CReqOpener oReq( pCfg );
        if( !oReq.IsKeepAlive() )
            break;

        guint64 iTaskId = 0;
        ret = oReq.GetTaskId( iTaskId );
        if( ERROR( ret ) )
            break;

        CReqBuilder okaReq( this );

        okaReq.SetMethodName(
            SYS_EVENT_KEEPALIVE );

        okaReq.SetCallFlags( 
           DBUS_MESSAGE_TYPE_SIGNAL
           | CF_ASYNC_CALL );

        string strVal = pMsg.GetDestination( );
        if( strVal.empty() )
        {
            ret = -EINVAL;
            break;
        }
        okaReq.SetSender( strVal );

        strVal = pMsg.GetSender( );
        okaReq.SetDestination( strVal );

        okaReq.Push( iTaskId );
        okaReq.Push( ( guint32 )KAOrigin );

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

gint32 CInterfaceServer::InitUserFuncs()
{
    gint32 ret = super::InitUserFuncs();
    if( ERROR( ret ) )
        return ret;

    BEGIN_IFHANDLER_MAP( IInterfaceServer );

    ADD_SERVICE_HANDLER(
        CInterfaceServer::UserCancelRequest,
        SYS_METHOD_USERCANCELREQ );

    END_IFHANDLER_MAP;

    return ret;
}

gint32 CInterfaceServer::UserCancelRequest(
    IEventSink* pCallback,
    guint64 qwIoTaskId )
{
    // let's search through the tasks to find the
    // specified task to cancel
    gint32 ret = 0;
    do{
        TaskGrpPtr pTaskGrp = GetTaskGroup();
        if( pTaskGrp.IsEmpty() )
            break;

        TaskletPtr pTask;
        ret = pTaskGrp->FindTaskByRmtId(
            qwIoTaskId, pTask );

        if( ERROR( ret ) )
            break;

        CfgPtr pResp;
        CIfInvokeMethodTask* pInvTask = pTask;

        if( pInvTask == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        else
        {
            CStdRTMutex oTaskLock(
                pInvTask->GetLock() );

            if( pInvTask->GetTaskState() ==
                stateStopped )
                return ERROR_STATE;
           
            CCfgOpenerObj oCfg( pInvTask ); 
            
            gint32 iType = 0;
            ret = pInvTask->GetPropertyType(
                propMsgPtr, iType );

            if( ERROR( ret ) )
                break;

            if( iType == typeObj )
            {
                ObjPtr pMsg;
                ret = oCfg.GetObjPtr( propMsgPtr, pMsg );
                if( ERROR( ret ) )
                    break;

                IConfigDb* pReqCfg = pMsg;
                if( pReqCfg == nullptr )
                {
                    ret = -EFAULT;
                    break;
                }

                CReqBuilder oResp( pReqCfg );

                oResp.ClearParams();
                ret = oResp.SwapProp(
                    propDestDBusName, propSrcDBusName );

                oResp.SetCallFlags( CF_ASYNC_CALL |
                    DBUS_MESSAGE_TYPE_METHOD_RETURN |
                    CF_NON_DBUS );

                oResp.SetReturnValue( -ECANCELED );
                pResp = oResp.GetCfg();
            }
            else
            {
                // typeDMsg
                CReqBuilder oResp( this );
                DMsgPtr pMsg;
                ret = oCfg.GetMsgPtr( propMsgPtr, pMsg );
                if( ERROR( ret ) )
                    break;

                string strVal; 
                oResp.SetMethodName(
                    pMsg.GetMember() );

                oResp.SetDestination(
                     pMsg.GetSender() );

                oResp.SetCallFlags( CF_ASYNC_CALL |
                    DBUS_MESSAGE_TYPE_METHOD_RETURN );

                oResp.SetReturnValue( -ECANCELED );
                pResp = oResp.GetCfg();
            }

            if( ERROR( ret ) )
                break;
        }

        if( !pResp.IsEmpty() )
        {
            // note we do not use OnServiceComplete to
            // complete the invoke task, force to
            // deliver the `cancel event' to the task.
            SetResponse( pInvTask, pResp );
            pInvTask->OnEvent( eventCancelTask,
                -ECANCELED, 0, 0 );
        }

    }while( 0 );

    // set the response for this cancel request
    CParamList oMyResp;
    oMyResp[ propReturnValue ] = ret;

    SetResponse( pCallback,
        oMyResp.GetCfg() );

    return ret;
}

gint32 CopyFile( gint32 iFdSrc, gint32 iFdDest, ssize_t& iSize )
{
    if( iFdSrc < 0 || iFdDest < 0 )
        return -EINVAL;

    gint32 ret = 0;
    ssize_t iBytes = 0;
    ssize_t iTransferred = 0;

    BufPtr pBuf(true);
    if( iSize == 0 || iSize > MAX_BYTES_PER_FILE )
    {
        pBuf->Resize( MAX_BYTES_PER_TRANSFER );
    }
    else
    {
        pBuf->Resize( iSize );
    }

    if( iSize == 0 )
        iSize = MAX_BYTES_PER_FILE;

    do{
        iBytes = read( iFdSrc, pBuf->ptr(), pBuf->size() );
        if( iBytes == -1 )
        {
            ret = -errno;
            break;
        }

        iBytes = write( iFdDest, pBuf->ptr(), iBytes );
        if( iBytes == -1 )
        {
            ret = -errno;
            break;
        }

        iTransferred += iBytes;
        if( iSize > iBytes )
        {
            iSize -= iBytes;
            if( iSize < MAX_BYTES_PER_TRANSFER )
            {
                pBuf->Resize( iSize );
            }
        }

    }while( iBytes == MAX_BYTES_PER_TRANSFER );

    if( SUCCEEDED( ret ) )
        iSize = iTransferred;

    return ret;
}

gint32 CopyFile( gint32 iFdSrc,
    const std::string& strDestFile )
{
    if( iFdSrc < 0 || strDestFile.empty() )
        return -EINVAL;

    gint32 ret = 0;

    gint32 iFdDest = open( strDestFile.c_str(),
        O_CREAT | O_WRONLY );

    if( iFdDest == -1 )
        return -errno;

    ssize_t iBytes = 0;
    ret = CopyFile( iFdSrc, iFdDest, iBytes );

    // rollback
    if( ERROR( ret ) )
        unlink( strDestFile.c_str() );

    close( iFdDest );
    return ret;
}

gint32 CopyFile( const std::string& strSrcFile,
    gint32 iFdDest )
{
    if( iFdDest < 0 || strSrcFile.empty() )
        return -EINVAL;

    gint32 iFdSrc = open(
        strSrcFile.c_str(), O_RDONLY );

    if( iFdSrc == -1 )
        return -errno;

    gint32 ret = 0;
    do{
        
        off_t iOffset =
            lseek( iFdDest, 0, SEEK_CUR );

        if( iOffset == -1 )
        {
            ret = -errno;
            break;
        }

        ssize_t iBytes = 0;

        ret = CopyFile( iFdSrc, iFdDest, iBytes );

        if( ERROR( ret ) )
        {
            // rollback
            iBytes = lseek( iFdDest, iOffset, SEEK_SET );
            if( iBytes != iOffset )
            {
                ret = -errno;
                break;
            }
            ftruncate( iFdDest, iOffset );
        }

    }while( 0 );

    close( iFdSrc );
    return ret;
}

gint32 CInterfaceServer::SendData_Server(
    IConfigDb* pDataDesc,           // [in]
    gint32 fd,                      // [in]
    guint32 dwOffset,               // [in]
    guint32 dwSize,                 // [in]
    IEventSink* pCallback )
{
    if( pDataDesc == nullptr
        || dwOffset > MAX_BYTES_PER_FILE
        || dwSize > MAX_BYTES_PER_FILE 
        || pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    bool bClose = false;
    do{
        if( fd < 0 )
        {
            string strSrcFile;
            CCfgOpener oCfg( pDataDesc );

            // FIXME: security loophole, no limit on
            // where to store the files
            ret = oCfg.GetStrProp(
                propFilePath, strSrcFile );

            if( ERROR( ret ) )
                break;

            BufPtr pBuf( true );

            pBuf->Resize( strSrcFile.size() + 16 );
            *pBuf = strSrcFile;

            // remove the dirname
            string strFileName = basename( pBuf->ptr() );
            fd = open( strFileName.c_str(), O_RDONLY );
            if( fd < 0 )
            {
                ret = -errno;
                break;
            }
            bClose = true;
        }
        // what to do with this file?

    }while( 0 );

    if( bClose )
        close( fd );

    return ret;
}

gint32 CInterfaceServer::FetchData_Server(
    IConfigDb* pDataDesc,           // [in]
    gint32& fd,                     // [out]
    guint32& dwOffset,              // [in, out]
    guint32& dwSize,                // [in, out]
    IEventSink* pCallback )
{
    if( pDataDesc == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    gint32 fdDest = -1;
    gint32 fdSrc = -1;

    do{
        string strSrcFile;
        CCfgOpener oCfg( pDataDesc );
        ret = oCfg.GetStrProp(
            propFilePath, strSrcFile );

        if( ERROR( ret ) )
            break;

        char szDestFile[] = "tmpXXXXXX";
        ret = mkstemp( szDestFile );
        if( ret == -1 )
        {
            ret = -errno;
            break;
        }

        gint32 fdDest = open( szDestFile, O_CREAT, 0600 );
        if( fdDest == -1 )
        {
            ret = -errno;
            break;
        }

        // delete on close
        unlink( szDestFile );

        // FIXME: Security issue here, no limit for
        // external access to all the files

        fdSrc = open( strSrcFile.c_str(), O_RDONLY );
        if( fdSrc == -1 )
        {
            ret = -errno;
            break;
        }

        ssize_t iSize = lseek( fdSrc, dwOffset, SEEK_END );
        if( iSize == -1 )
        {
            ret = -errno;
            break;
        }

        if ( dwSize > iSize - dwOffset )
            dwSize = iSize - dwOffset;

        if( lseek( fdSrc, dwOffset, SEEK_SET ) == -1 )
        {
            ret = -errno;
            break;
        }

        // FIXME: size is not the only factor for
        // transfer, and the RTT should also be
        // considered
        if( dwSize <= 8 * MAX_BYTES_PER_TRANSFER )
        {
            ssize_t iSize = dwSize;
            ret = CopyFile( fdSrc, fdDest, iSize );
            if( ERROR( ret ) )
                break;

            dwSize = iSize;
            ret = dup( fdDest );
            if( ret == -1 )
            {
                ret = -errno;
                break;
            }
            fd = ret;
        }
        else
        {
            // using a standalone thread for large data
            // transfer
            CParamList oParams;

            oParams.Push( ObjPtr( pDataDesc ) );
            oParams.Push( dwOffset );
            oParams.Push( dwSize );

            gint32 fdSrc1 = dup( fdSrc );
            if( fdSrc1 == -1 )
            {
                ret = -errno;
                break;
            }
            gint32 fdDest1 = dup( fdDest );
            if( fdDest1 == -1 )
            {
                ret = -errno;
                close( fdSrc1 );
                break;
            }

            oParams.Push( fdSrc1 );
            oParams.Push( fdDest1 );

            oParams.SetObjPtr(
                propIfPtr, ObjPtr( this ) );

            oParams.SetObjPtr(
                propEventSink,
                ObjPtr( pCallback ) );

            // scheduled a dedicate thread for this
            // task
            ret = GetIoMgr()->ScheduleTask(
                clsid( CIfFetchDataTask ), 
                oParams.GetCfg(), true );

            if( SUCCEEDED( ret ) )
                ret = STATUS_PENDING;
        }
        
    }while( 0 );

    if( fdDest >= 0 )
        close( fdDest );

    if( fdSrc >= 0 )
        close( fdSrc );

    return ret;
}

