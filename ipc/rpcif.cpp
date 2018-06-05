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
#include "proxy.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

CInterfaceProxy::CInterfaceProxy(
    const IConfigDb* pCfg ) :
    super( pCfg )
{
    gint32 ret = 0;

    do{
        CCfgOpener oCfg( pCfg );
        string strPortClass;

        ret = oCfg.GetStrProp(
            propPortClass, strPortClass );

        if( ERROR( ret ) )
            break;

        EnumClsid iStateClass =
            clsid( CLocalProxyState );

        if( strPortClass == PORT_CLASS_DBUS_PROXY_PDO ||
            strPortClass == PORT_CLASS_TCP_STREAM_PDO )
        {
            // CRemoteProxyState will have an ip
            // address to match
            iStateClass = clsid( CRemoteProxyState );
        }

        ret = m_pIfStat.NewObj(
            iStateClass, pCfg );

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

gint32 CRpcBaseOperations::OpenPort(
    IEventSink* pCallback )
{
    return m_pIfStat->OpenPort( pCallback );
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

        if( iState != stateStarted )
        {
            ret = ERROR_STATE;
            break;
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

        if( pCallback != nullptr )
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

        if( SUCCEEDED( ret ) )
        {
            // run the EnableEventCompletion
            // immediately
            IRP* ptrIrp = pIrp;
            if( ptrIrp == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            // use this to tell the task group to
            // move on to next task
            pCallback->OnEvent( eventIrpComp,
                ( guint32 )ptrIrp, 0, nullptr );
        }
        
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
    {
        pMatch = m_pIfMatch;
    }

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
    {
        pMatch = m_pIfMatch;
    }

    gint32 ret = EnableEventInternal(
        pMatch, false, pCallback );

    return ret;
}

gint32 CRpcBaseOperations::StartRecvMsg(
    IEventSink* pCompletion,
    IMessageMatch* pMatch )
{
    if( pCompletion != nullptr )
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

        BufPtr pBuf( true );
        ObjPtr pObj = ( CObjBase* )m_pIfMatch;

        if( pMatch != nullptr )
            pObj = pMatch;

        *pBuf = pObj;

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

        CInterfaceState* pIfStat = m_pIfStat;
        if( pIfStat == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        // make a copy of the irp for canceling
        pObj = pIrp;
        ret = oTaskCfg.SetObjPtr(
            propIrpPtr, pObj );

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

        // clear the irp reference
        pObj = pCompletion;
        TaskletPtr pTask;
        pTask =  pObj;
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

        CIrpCompThread* pIrpThrd = pThrd;
        if( ERROR( ret ) )
        {
            pIrp->RemoveCallback();
            break;
        }

        // complete the irp in the irp completion
        // thread
        ret = pIrpThrd->AddIrp( pIrp );
        if( ERROR( ret ) )
        {
            pIrp->RemoveCallback();
            break;
        }

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
    guint32 ret = 0;
    do{
        ret = pCallback.NewObj( clsid( CSyncCallback ) );
        if( ERROR( ret ) )
            break;

        ret = StartEx( pCallback );
        if( ERROR( ret ) )
            break;

        CSyncCallback* pSyncCallback = pCallback;
        if( pSyncCallback == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        pSyncCallback->WaitForComplete();

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

        if( m_pIfStat->GetState() != stateStopped )
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

        oParams.SetObjPtr( propMatchPtr,
            ObjPtr( m_pIfMatch ) );

        ret = oParams.Push( true );
        if( ERROR( ret ) )
            break;

        ret = pEnableEvtTask.NewObj(
            clsid( CIfEnableEventTask ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        bool bEnable = false;
        oParams.Pop( bEnable );

        ret = pRecvMsgTask.NewObj(
            clsid( CIfStartRecvMsgTask ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        pTaskGrp->AppendTask( pOpenPortTask );
        pTaskGrp->AppendTask( pEnableEvtTask );
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

        // this task will run later when the first
        // two have done
        ret = AddAndRun( pRecvMsgTask );
        if( ERROR( ret ) )
            break;

        ret = pRecvMsgTask->GetError();

    }while( 0 );

    if( ret == STATUS_MORE_PROCESS_NEEDED )
        ret = STATUS_PENDING;

    if( ret == STATUS_PENDING
        && pCallback == nullptr )
    {
        ret = pTaskGrp->WaitForComplete();    
    }

    return ret;
}

gint32 CRpcInterfaceBase::Stop()
{
    return StopEx( nullptr );
}

gint32 CRpcInterfaceBase::StopEx(
    IEventSink* pCallback )
{
    EnumIfState iState =
        m_pIfStat->GetState();

    if( iState == stateStopped )
        return ERROR_STATE;

    gint32 ret = 0;
    TaskGrpPtr pTaskGrp;

    do{
        TaskletPtr pClosePortTask;
        TaskletPtr pDisableEvtTask;
        TaskletPtr pStopRecvTask;

        if( m_pIfStat->GetState() != stateStopped )
        {
            ret = ERROR_STATE;
            break;
        }

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

        bool bEnable = false;

        ret = oParams.Push( bEnable );
        if( ERROR( ret ) )
            break;

        ret = pDisableEvtTask.NewObj(
            clsid( CIfEnableEventTask ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        ret = oParams.Pop( bEnable );
        if( ERROR( ret ) )
            break;

        ret = pStopRecvTask.NewObj(
            clsid( CIfStopRecvMsgTask ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        pTaskGrp->AppendTask( pStopRecvTask );
        pTaskGrp->AppendTask( pDisableEvtTask );
        pTaskGrp->AppendTask( pClosePortTask );
        pTaskGrp->SetRelation( logicOR );

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
            ret = oCompCfg.SetObjPtr(
                propEventSink, ObjPtr( pCallback) );

            if( ERROR( ret ) )
                break;
        }

        TaskletPtr pCompletion;
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
        if( ret != STATUS_PENDING
            && ret != STATUS_MORE_PROCESS_NEEDED )
        {
            // pCompletion->OnEvent(
            //     eventTaskComp, ret, 0, 0 );
        }

    }while( 0 );


    if( ret == STATUS_MORE_PROCESS_NEEDED )
    {
        ret = STATUS_PENDING;
    }

    if( ret == STATUS_PENDING &&
        pCallback == nullptr )
    {
        ret = pTaskGrp->WaitForComplete();    
    }
    else
    {
        if( m_pRootTaskGroup->GetTaskCount() > 0 )
            m_pRootTaskGroup->Clear();
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
    return StopEx( pCallback );
}

gint32 CRpcInterfaceBase::OnPortEvent(
    EnumEventId iEvent,
    HANDLE hPort )
{
    gint32 ret = 0;

    if( !IsMyPort( hPort ) )
        return -EINVAL;

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
    case eventPortStartFailed:
    case eventPortStarted:
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
    case cmdResume:
        {
            TaskletPtr pTask;

            // to avoid synchronous call
            // we put  dumb task there
            ret = pTask.NewObj(
                clsid( CIfDummyTask ) );

            if( ERROR( ret ) )
                break;

            if( iEvent == cmdPause )
            {
                ret = Pause( pTask );
            }
            else
            {
                ret = Resume( pTask );
            }
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

            string strIpAddr =
                reinterpret_cast< char* >( dwParam1 );

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
    default:
        {
            ret = -ENOTSUP;
            break;
        }
    }

#ifdef DEBUG
    if( ERROR( ret ) )
    {
        string strMsg = DebugMsg( ret,
            "Error occured in CRpcInterfaceBase::OnEvent" );
        printf( strMsg.c_str() );
    }
#endif
    return ret;
}

gint32 CRpcInterfaceBase::DoCleanUp()
{
    if( GetState() != stateStopped )
        return ERROR_STATE;

    m_pRootTaskGroup->OnEvent(
        eventCancelTask, 0, 0, 0 );

    m_pRootTaskGroup.Clear();

    return 0;
}

gint32 CRpcInterfaceBase::AppendAndRun(
    TaskletPtr& pTask )
{
    if( pTask.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;

    // synchronize with all the client calls
    CStdRMutex oIfLock( GetLock() );
    if( m_pRootTaskGroup.IsEmpty() )
        return ERROR_STATE;

    TaskletPtr pRootTask = m_pRootTaskGroup;
    
    do{
        ret = m_pRootTaskGroup->
            AppendTask( pTask );

        if( ERROR( ret ) )
            break;

        guint32 dwCount = 
            m_pRootTaskGroup->GetTaskCount();

        if( dwCount == 0 )
        {
            ret = ERROR_FAIL;
            break;
        }
        // we are the first one
        if( dwCount == 1 )
        {
            oIfLock.Unlock();
            // FIXME: race condition?
            // run immediately
            ( *pRootTask )( eventZero );
            // no need to lock again for destructor
            // oIfLock.Lock();
        }
        else
        {
            // set the status to STATUS_PENDING if
            // the task is blocked in the queue
            pTask->SetError( STATUS_PENDING );
        }

    }while( 0 );

    return ret;
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
            if( pParaGrp != nullptr )
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

            if( ERROR( ret ) )
                break;

            ret = STATUS_PENDING;
        }

    }while( 0 );

    // don't rely on `ret' to determine the return
    // value from the task pIoTask, you need to
    // get the return value from
    // pIoTask->GetError() in case the task is
    // scheduled to run later
    return ret;
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

CReqBuilder::CReqBuilder(
    CRpcBaseOperations* pIf )
    : CParamList()
{
    if( pIf == nullptr )
        return;

    gint32 ret = 0;
    do{
        m_pIf = pIf;

        ObjPtr pObj = pIf;

        CopyProp( propIfName, pIf );
        CopyProp( propObjPath, pIf );
        CopyProp( propSrcDBusName, pIf );

        CInterfaceProxy* pProxy = pObj;
        if( pProxy != nullptr )
        {
            // a mandatory parameter for proxy
            ret = CopyProp( propDestDBusName, pIf );
            if( ERROR( ret ) )
                break;
            CopyProp( propUserName, pIf );
        }

        CopyProp( propTimeoutSec, pIf );
        CopyProp( propKeepAliveSec, pIf );
        CopyProp( propIpAddr, pIf );

        CIoManager* pMgr = pIf->GetIoMgr();
        string strModName = pMgr->GetModName();

        string strSrcDBusName =
            DBUS_DESTINATION( strModName );

        SetSender( strSrcDBusName );

    }while( 0 );
}

CReqBuilder::CReqBuilder(
    const IConfigDb* pCfg )
    : CParamList()
{
    if( pCfg == nullptr )
        return;

    try{
        CopyProp( propIfName, pCfg );
        CopyProp( propObjPath, pCfg );
        CopyProp( propDestDBusName, pCfg );

        CopyProp( propTimeoutSec, pCfg );
        CopyProp( propKeepAliveSec, pCfg );
        CopyProp( propIpAddr, pCfg );
        CopyProp( propUserName, pCfg );
        CopyProp( propSrcDBusName, pCfg );

        CopyProp( propMethodName, pCfg );
        CopyProp( propCallOptions, pCfg );
        CopyProp( propTaskId, pCfg );

        CopyParams( pCfg );
    }
    catch( std::invalid_argument& e )
    {

    }
    return;
}

CReqBuilder::CReqBuilder(
    IConfigDb* pCfg )
    : CParamList( pCfg )
{ return; }

gint32 CReqBuilder::SetIfName(
    const std::string& strIfName )
{
    return SetStrProp(
        propIfName, strIfName );
}

gint32 CReqBuilder::SetObjPath(
    const std::string& strObjPath ) 
{
    return SetStrProp(
        propObjPath, strObjPath );
}
gint32 CReqBuilder::SetDestination(
    const std::string& strDestination ) 
{
    return SetStrProp(
        propDestDBusName, strDestination );
}

gint32 CReqBuilder::SetSender(
    const std::string& strSender ) 
{
    return SetStrProp(
        propSrcDBusName, strSender );
}

gint32 CReqBuilder::SetMethodName(
    const std::string& strMethodName ) 
{
    return SetStrProp(
        propMethodName, strMethodName );
}

gint32 CReqBuilder::SetKeepAliveSec(
    guint32 dwTimeoutSec )
{
    CfgPtr pCfg;
    gint32 ret = GetCallOptions( pCfg );
    if( ERROR( ret ) )
        return ret;

    CCfgOpener oCfg( ( IConfigDb* )pCfg );

    return oCfg.SetIntProp(
        propKeepAliveSec, dwTimeoutSec );
}

gint32 CReqBuilder::SetTimeoutSec(
    guint32 dwTimeoutSec)
{

    CfgPtr pCfg;
    gint32 ret = GetCallOptions( pCfg );
    if( ERROR( ret ) )
        return ret;

    CCfgOpener oCfg( ( IConfigDb* )pCfg );

    return oCfg.SetIntProp(
        propTimeoutSec, dwTimeoutSec );
}

gint32 CReqBuilder::SetCallFlags(
    guint32 dwFlags )
{
    CfgPtr pCfg;
    gint32 ret = GetCallOptions( pCfg );
    if( ERROR( ret ) )
        return ret;

    CCfgOpener oCfg( ( IConfigDb* )pCfg );

    return oCfg.SetIntProp(
        propCallFlags, dwFlags );
}

gint32 CReqBuilder::SetReturnValue(
    gint32 iRet )
{
    return SetIntProp(
        propReturnValue, iRet );
}

gint32 CReqBuilder::SetIpAddr(
    const string& strIpAddr, bool bSrc )
{
    EnumPropId iProp = propIpAddr;
    return SetStrProp( iProp, strIpAddr );
}

gint32 CReqBuilder::GetCallOptions(
    CfgPtr& pCfg )
{
    gint32 ret = 0;
    do{
        ObjPtr pObj;
        ret = GetObjPtr(
            propCallOptions, pObj );

        if( ERROR( ret ) )
        {
            ret = pCfg.NewObj();
            if( ERROR( ret ) )
                break;

            pObj = pCfg;
            if( pObj.IsEmpty() )
            {
                ret = -EFAULT;
                break;
            }
            ret = SetObjPtr(
                propCallOptions, pObj );

            if( ERROR( ret ) )
                break;

            break;
        }

        CfgPtr pSrcCfg = pObj;
        if( pSrcCfg.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        pCfg = pSrcCfg;

    }while( 0 );

    return ret;
}

gint32 CReqBuilder::SetTaskId(
    guint64 dwTaskId )
{
    return SetQwordProp(
        propTaskId, dwTaskId );
}

gint32 CReqOpener::GetIfName(
    std::string& strIfName ) const
{
    return GetStrProp(
        propIfName, strIfName );
}
gint32 CReqOpener::GetObjPath(
    std::string& strObjPath ) const 
{
    return GetStrProp(
        propObjPath, strObjPath );
}

gint32 CReqOpener::GetDestination(
    std::string& strDestination ) const 
{
    return GetStrProp(
        propDestDBusName, strDestination );
}

gint32 CReqOpener::GetMethodName(
    std::string& strMethodName ) const
{
    return GetStrProp(
        propMethodName, strMethodName );
}

gint32 CReqOpener::GetSender(
    std::string& strSender ) const
{
    return GetStrProp(
        propSrcDBusName, strSender );
}

gint32 CReqOpener::GetKeepAliveSec(
    guint32& dwTimeoutSec ) const
{
    CfgPtr pCfg;
    gint32 ret = GetCallOptions( pCfg ); 
    if( ERROR( ret ) )
        return ret;

    CCfgOpener oCallOption(
        ( IConfigDb* ) pCfg );

    return oCallOption.GetIntProp(
        propKeepAliveSec, dwTimeoutSec );

}

gint32 CReqOpener::GetTimeoutSec(
    guint32& dwTimeoutSec) const 
{
    CfgPtr pCfg;
    gint32 ret = GetCallOptions( pCfg ); 
    if( ERROR( ret ) )
        return ret;

    CCfgOpener oCallOption(
        ( IConfigDb* ) pCfg );

    return oCallOption.GetIntProp(
        propTimeoutSec, dwTimeoutSec );
}

gint32 CReqOpener::GetCallFlags(
    guint32& dwFlags ) const
{
    CfgPtr pCfg;
    gint32 ret = GetCallOptions( pCfg ); 
    if( ERROR( ret ) )
        return ret;

    CCfgOpener oCallOption(
        ( IConfigDb* ) pCfg );

    return oCallOption.GetIntProp(
        propCallFlags, dwFlags );
}

gint32 CReqOpener::GetReturnValue(
    gint32& iRet ) const
{
    guint32 dwRet = ( guint32 ) iRet;
    return GetIntProp(
        propReturnValue, dwRet );
}

gint32 CReqOpener::GetIpAddr(
    string& strIpAddr, bool bSrc ) const
{
    gint32 iProp = propIpAddr;
    return GetStrProp( iProp, strIpAddr );
}

bool CReqOpener::HasReply() const
{
    guint32 dwFlags;
    gint32 ret = GetCallFlags( dwFlags );
    if( ERROR( ret ) )
    {
        // default to have a replay
        return true;
    }
    bool bRet =
        ( ( dwFlags & CF_WITH_REPLY ) != 0 );

    return bRet;
}

bool CReqOpener::IsKeepAlive() const
{
    guint32 dwFlags;
    gint32 ret = GetCallFlags( dwFlags );
    if( ERROR( ret ) )
    {
        // default to no keep alive
        return false;
    }
    bool bRet =
        ( ( dwFlags & CF_KEEP_ALIVE ) != 0 );

    return bRet;
}

gint32 CReqOpener::GetReqType(
    guint32& dwType ) const
{
    guint32 dwFlags;
    gint32 ret = GetCallFlags( dwFlags );
    if( ERROR( ret ) )
        return ret;

    dwType = ( dwFlags & 7 );
    return 0;
}

gint32 CReqOpener::GetCallOptions(
    CfgPtr& pCfg ) const
{
    gint32 ret = 0;
    do{
        ObjPtr pObj;
        ret = GetObjPtr(
            propCallOptions, pObj );

        if( ERROR( ret ) )
            break;

        CfgPtr pSrcCfg = pObj;
        if( pSrcCfg.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        pCfg = pSrcCfg;

    }while( 0 );

    return ret;
}

gint32 CReqOpener::GetTaskId(
    guint64& dwTaskId ) const
{
    return GetQwordProp(
        propTaskId, dwTaskId );
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
        ret = pMsg.NewObj();

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

        ret = pMsg.SetDestination( strDest );
        if( ERROR( ret ) && bRequest )
            break;

        string strSender = DBUS_DESTINATION(
            GetIoMgr()->GetModName() );

        ret = pMsg.SetSender( strSender );
        if( ERROR( ret ) ) 
            break;

        ret = pMsg.SetMember( strMethod );
        if( ERROR( ret ) )
            break;

        BufPtr pReqBuf( true );
        ret = pReqCall->Serialize( *pReqBuf );
        if( ERROR( ret ) )
            break;

        if( !dbus_message_append_args( pMsg,
            DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
            pReqBuf->ptr(), pReqBuf->size(),
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
    IRP* pIrp, IConfigDb* pResp )
{
    gint32 ret = 0;
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    if( pResp == nullptr )
        return -EINVAL;

    // retrieve the data from the irp
    IrpCtxPtr& pCtx = pIrp->GetTopStack();

    guint32 dwDirection = pCtx->GetIoDirection();
    if( dwDirection == IRP_DIR_OUT )
        return -ENOTSUP;

    if( pCtx->IsNonDBusReq() )
    {
        CfgPtr pMsg = nullptr;
        ret = pCtx->GetRespAsCfg( pMsg );
        if( ERROR( ret ) )
            return ret;

        *pResp = *pMsg;
        ret = 0;
    }
    else
    {
        DMsgPtr pMsg = *pCtx->m_pRespData;
        if( pMsg.IsEmpty() )
            return -EINVAL;

        ret = FillRespData( pMsg, pResp );
    }

    if( !pResp->exist( propReturnValue ) )
    {
        gint32 iRet = pIrp->GetStatus();
        CParamList oParams( pResp );
        oParams.SetIntProp(
            propReturnValue, iRet );
    }
    return ret;
}

gint32 CRpcServices::FillRespData(
    DMsgPtr& pMsg, IConfigDb* pResp )
{
    gint32 ret = 0;
    if( pMsg.IsEmpty() || pResp == nullptr )
        return -EINVAL;

    if( pMsg.GetType() !=
        DBUS_MESSAGE_TYPE_METHOD_RETURN )
    {
        ret = -EBADMSG;
        CCfgOpener oCfg( pResp );
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

            CCfgOpener oCfg( pResp );
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

            CCfgOpener oCfg( pResp );
            ret = oCfg.SetIntProp(
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
            ret = pMsg.GetObjArgAt( 0, pObj );
            if( ERROR( ret ) )
                break;

            IConfigDb* pCfg = pObj;
            if( pCfg == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            ret = pResp->Clone( *pCfg );
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
                    CTRLCODE_REG_MATCH );
            }
            else if( strMethod ==
                IF_METHOD_ENABLEEVT )
            {
                pIrpCtx->SetCtrlCode(
                    CTRLCODE_UNREG_MATCH );
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
        ret = GetDelegate(
            strMethod, pTask );

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
                pSyncCallback->WaitForComplete();
            }
        }

        if( ERROR( ret ) )
            break;

        CReqOpener oResp( ( IConfigDb* )pResp );
        ret = oResp.GetReturnValue( iRet );

        if( ERROR( ret ) )
            break;

        ret = iRet;

    }while( 0 );

    return ret;
}

gint32 CInterfaceProxy::InitUserFuncs()
{
    BEGIN_PROXY_MAP( false );
    ADD_PROXY_METHOD( SYS_METHOD_USERCANCELREQ, guint64 );
    END_PROXY_MAP

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

    // a sample code snippet
    return CallProxyFunc( pCallback,
		qwThisTaskId,
        SYS_METHOD( __func__ ),
		qwTaskToCancel );
}

CInterfaceServer::CInterfaceServer(
    const IConfigDb* pCfg )
    : super( pCfg )
{
    gint32 ret = 0;

    do{
        CCfgOpener oCfg( pCfg );
        string strPortClass;

        ret = oCfg.GetStrProp(
            propPortClass, strPortClass );

        if( ERROR( ret ) )
            break;

        EnumClsid iStateClass =
            clsid( CIfServerState );

        ret = m_pIfStat.NewObj(
            iStateClass, pCfg );

        if( ERROR( ret ) )
            break;

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

// called to send back the response
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

    // set the response value
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
            ret = oReq.GetCallFlags( dwFlags );

            if( dwFlags & CF_WITH_REPLY )
                bResp = true;

            if( ret == STATUS_PENDING )
                SET_RMT_TASKID( pCfg, oCfg );

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
                DBUS_TYPE_UINT32, iRet ,
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
                DBUS_TYPE_UINT32, iRet,
                DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
                pBuf->ptr(), pBuf->size(),
                iFdType, iFd,
                DBUS_TYPE_UINT32, dwOffset,
                DBUS_TYPE_UINT32, dwSize,
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
                    DBUS_TYPE_UINT32, iRet,
                    DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
                    pBuf->ptr(), pBuf->size(),
                    DBUS_TYPE_INVALID ) )
                {
                    ret = -ENOMEM;
                    break;
                }
            }
            else
            {
                if( !dbus_message_append_args( pRespMsg,
                    DBUS_TYPE_UINT32, iRet,
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
                DBUS_TYPE_UINT32, iRet,
                DBUS_TYPE_INVALID ) )
            {
                ret = -ENOMEM;
                break;
            }
        }
        else
        {
            ret = -ENOTSUP;
            break;
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

    BEGIN_HANDLER_MAP;

    ADD_SERVICE_HANDLER(
        CInterfaceServer::UserCancelRequest,
        SYS_METHOD_USERCANCELREQ );

    END_HANDLER_MAP;

    return ret;
}

gint32 CInterfaceServer::UserCancelRequest(
    IEventSink* pCallback,
    guint64 qwIoTaskId )
{
    // let's search through the tasks to find the
    // specified task to cancel
    gint32 ret = -ENOENT;
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
            ret = oCfg.GetCfg()->GetPropertyType(
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
            ret = OnServiceComplete(
                pResp, pInvTask );
        }

    }while( 0 );

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

        unlink( szDestFile );

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

