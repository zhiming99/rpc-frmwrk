/*
 * =====================================================================================
 *
 *       Filename:  rpcroute.cpp
 *
 *    Description:  implementation of CRpcRouter
 *
 *        Version:  1.0
 *        Created:  07/11/2017 11:34:37 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi(woodhead99@gmail.com )
 *   Organization:
 *
 * =====================================================================================
 */

#include "configdb.h"
#include "defines.h"
#include "proxy.h"
#include "rpcroute.h"
#include "dbusport.h"
#include "connhelp.h"

using namespace std;

CRpcRouter::CRpcRouter(
    const IConfigDb* pCfg )
{
    gint32 ret = 0;
    SetClassId( clsid( CRpcRouter ) );

    do{
        if( pCfg == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        CCfgOpener oCfg( pCfg );
        ret = oCfg.GetPointer( propIoMgr, m_pIoMgr );

        if( ERROR( ret ) )
            break;

    }while( 0 );

    if( ERROR( ret ) )
    {
        string strMsg = DebugMsg(
            ret, "Error occurs in CRpcRouter's ctor" );
        throw runtime_error( strMsg );
    }
}

CRpcRouter::~CRpcRouter()
{
}

gint32 CRpcRouter::GetBridgeProxy(
    const std::string& strIpAddr,
    InterfPtr& pIf )
{
    return GetBridgeIfInternal(
        strIpAddr, pIf, true );
}

gint32 CRpcRouter::GetBridge(
    const std::string& strIpAddr,
    InterfPtr& pIf )
{
    return GetBridgeIfInternal(
        strIpAddr, pIf, false );
}

gint32 CRpcRouter::GetBridgeIfInternal(
    const std::string& strIpAddr,
    InterfPtr& pIf,
    bool bProxy )
{

    if( strIpAddr.empty() )
        return -EINVAL;

    CStdRMutex oRouterLock( GetLock() );
    if( m_mapIp2NetIfs.find( strIpAddr ) ==
        m_mapIp2NetIfs.end() )
        return -ENOENT;

    // get proxy ptr
    if( bProxy )
        pIf = m_mapIp2NetIfs[ strIpAddr ].first;
    else
        pIf = m_mapIp2NetIfs[ strIpAddr ].second;

    return 0;
}

gint32 CRpcRouter::AddBridgePair( 
    const std::string& strIpAddr,
    IGenericInterface* pProxy,
    IGenericInterface* pServer )
{
    if( pProxy == nullptr ||
        pServer == nullptr )
    {
        return -EINVAL;
    }

    CStdRMutex oRouterLock( GetLock() );
    if( m_mapIp2NetIfs.find( strIpAddr ) !=
        m_mapIp2NetIfs.end() )
        return -EEXIST;

    NetIfPair oPair( pProxy, pServer );
    m_mapIp2NetIfs[ strIpAddr ] = oPair;
    return 0;
}

gint32 CRpcRouter::Start()
{
    // register this object
    gint32 ret = 0;
    try{
        do{
            InterfPtr pIf;
            ret = StartReqFwdr( pIf, nullptr );
            if( ERROR( ret ) )
                break;

        } while( 0 );

        return ret;
    }
    catch( system_error& e )
    {
        ret = e.code().value();
    }
    return ret;
}

gint32 CRpcRouter::Stop()
{
    gint32 ret = 0;
    map< string, NetIfPair > mapIp2NetIfs;

    //FIXME: need more convincable processing
    do{
        CStdRMutex oRouterLock( GetLock() );

        mapIp2NetIfs = m_mapIp2NetIfs;
        m_mapIp2NetIfs.clear();

    }while( 0 );

    for( auto&& oPairs : mapIp2NetIfs )
    {
        InterfPtr p;
        if( !oPairs.second.first.IsEmpty() )
            oPairs.second.first->Stop();
        if( !oPairs.second.second.IsEmpty() )
            oPairs.second.second->Stop();
    }
    mapIp2NetIfs.clear();
    
    ret = m_pReqFwdr->Stop();
    for( auto elem : m_mapReqProxies )
    {
        gint32 iRet = elem.second->Stop();
        if( ERROR( iRet ) )
            ret = iRet;
    }

    return ret;
}

gint32 CRpcRouter::StartReqFwdr( 
    InterfPtr& pIf, IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        CParamList oParams;
        oParams.SetPointer( propIoMgr, GetIoMgr() );

        EnumClsid iClsid =
            clsid( CRpcReqForwarderImpl );

        ret = CRpcServices::LoadObjDesc(
            ROUTER_OBJ_DESC,
            OBJNAME_REQFWDR,
            true,
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        oParams.SetObjPtr(
            propParentPtr, ObjPtr( this ) );

        ret = pIf.NewObj(
            iClsid, oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        m_pReqFwdr = pIf;
        ret = pIf->Start();

    }while( 0 );

    return ret;
}

gint32 CRouterStartReqFwdrProxyTask::
    OnTaskCompleteStart( gint32 iRetVal )
{
    gint32 ret = 0;
    do{
        if( ERROR( iRetVal ) )
            break;

        CRouterRemoteMatch* pMatch = nullptr;
        CParamList oTaskCfg(
            ( IConfigDb* )GetConfig() );

        ret = oTaskCfg.GetPointer( 1, pMatch );
        if( ERROR( ret ) )
            break;

        CRpcReqForwarderProxy* pIf = nullptr;
        ret = oTaskCfg.GetPointer( propIfPtr, pIf );
        if( ERROR( ret ) )
            break;

        CRpcRouter* pRouter = pIf->GetParent();

        InterfPtr pIfPtr( pIf );
        ret = pRouter->AddProxy( pMatch, pIfPtr );

        if( ERROR( ret ) )
        {
            // duplicated, stop/remove the proxy as
            // below, this could happen if two `enable
            // Event' request arrives at the same time
            // for the same object
            TaskletPtr pDummyTask;
            pDummyTask.NewObj(
                clsid( CIfDummyTask ) );

            DEFER_CALL( pIf->GetIoMgr(),
                ObjPtr( pRouter ),
                &CRpcRouter::StopReqFwdrProxy,
                pIf, pMatch, pDummyTask );

            ret =pRouter->GetReqFwdrProxy(
                pMatch, pIfPtr );

            if( ERROR( ret ) )
                break;

            pIf = pIfPtr;
            if( pIf == nullptr )
            {
                ret = -EFAULT;
                break;
            }
        }

        // move on to enable the event
        // listening for the match
        EventPtr pEvt;
        GetInterceptTask( pEvt );
        ret = pRouter->ScheduleEventRelayTask(
            pMatch, pEvt, pIf, true );

        if( ret == STATUS_PENDING )
        {
            // nasty, we have transfered the
            // callback to another task
            oTaskCfg.RemoveProperty(
                propEventSink );

            oTaskCfg.RemoveProperty(
                propNotifyClient );

            // this task is completed.
            ret = 0;
        }

    }while( 0 );

    return ret;
}

gint32 CRouterStartReqFwdrProxyTask::
    OnTaskComplete( gint32 iRetVal )
{

    CParamList oTaskCfg(
        ( IConfigDb* )GetConfig() );
    bool bStart = oTaskCfg[ 0 ];

    if( bStart )
        return OnTaskCompleteStart( iRetVal );

    return iRetVal;
}

gint32 CRouterStartReqFwdrProxyTask::RunTask()
{
    gint32 ret = 0;
    do{
        CParamList oParams(
            ( IConfigDb* )GetConfig() );

        CRpcReqForwarderProxy* pIf = nullptr;
        ret = oParams.GetPointer( propIfPtr, pIf );
        if( ERROR( ret ) )
            break;

        bool bStart = oParams[ 0 ];
        if( bStart )
        {
            ret = pIf->StartEx( this );
        }
        else
        {
            CRpcRouter* pRouter =
                pIf->GetParent();

            CStdRMutex oRouterLock(
                pRouter->GetLock() );

            // nested locking
            if( pIf->GetActiveIfCount() == 0 )
            {
                CRouterRemoteMatch*
                    pMatch = nullptr;

                ret = oParams.GetPointer(
                    1, pMatch );

                if( ERROR( ret ) )
                    break;

                InterfPtr pIfPtr( pIf );
                ret = pRouter->RemoveProxy(
                    pMatch, pIfPtr );

                oRouterLock.Unlock();
                ret = pIf->StopEx( this );
            }
            else
                ret = ERROR_FAIL;
        }

    }while( 0 );

    return ret;
}

gint32 CRpcRouter::StartReqFwdrProxy( 
    IMessageMatch* pMatch,
    IEventSink* pCallback )
{
    ObjPtr pObj;
    return StartStopReqFwdrProxy(
        pObj, pMatch, pCallback, true );
}

gint32 CRpcRouter::StopReqFwdrProxy( 
    CRpcReqForwarderProxy* pIf,
    IMessageMatch* pMatch,
    IEventSink* pCallback )
{
    ObjPtr pObj( pIf );
    return StartStopReqFwdrProxy(
        pObj, pMatch, pCallback, false );
}

gint32 CRpcRouter::StartStopReqFwdrProxy( 
    ObjPtr& pIfObj,
    IMessageMatch* pMatch,
    IEventSink* pCallback,
    bool bStart )
{
    gint32 ret = 0;
    if( pMatch == nullptr ||
        pCallback == nullptr )
        return -EINVAL;

    if( !bStart && pIfObj.IsEmpty() )
        return -EINVAL;

    do{
        CParamList oParams;
        oParams.SetPointer( propIoMgr, GetIoMgr() );

        TaskletPtr pTask;
        CParamList oTaskCfg;

        InterfPtr pIf;

        if( bStart )
        {
            EnumClsid iClsid =
                clsid( CRpcReqForwarderProxyImpl );

            ret = CRpcServices::LoadObjDesc(
                ROUTER_OBJ_DESC,
                OBJNAME_REQFWDR,
                false,
                oParams.GetCfg() );

            if( ERROR( ret ) )
                break;

            oParams.SetObjPtr(
                propParentPtr, ObjPtr( this ) );

            ret = oParams.CopyProp(
                propObjPath, pMatch );

            if( ERROR( ret ) )
                break;

            ret = oParams.CopyProp(
                propDestDBusName, pMatch );

            if( ERROR( ret ) )
                break;

            ret = pIf.NewObj(
                iClsid, oParams.GetCfg() );

            if( ERROR( ret ) )
                break;

            oTaskCfg[ propIfPtr ] = ObjPtr( pIf );
        }
        else
        {
            oTaskCfg[ propIfPtr ] = pIfObj;
        }

        oTaskCfg[ propEventSink ] = ObjPtr( pCallback );
        oTaskCfg.Push( bStart );
        oTaskCfg.Push( ObjPtr( pMatch ) );

        ret = pTask.NewObj(
            clsid( CRouterStartReqFwdrProxyTask ),
            oTaskCfg.GetCfg() );

        if( ERROR( ret ) )
            break;

        ret = ( *pTask )( eventZero );

    }while( 0 );

    return ret;
}

gint32 CRpcRouter::UnsubscribeEvents()
{
    gint32 ret = 0;

    EventPtr pEvent( this );

    CConnPointHelper oCpHelper( GetIoMgr() );
    vector< EnumPropId > vecEvents;

    if( true )
    {
        CStdRMutex oRouterLock( GetLock() );
        vecEvents = m_vecTopicList;
        m_vecTopicList.clear();
    }

    for( guint32 i = 0; i < vecEvents.size(); ++i )
    {
        oCpHelper.UnsubscribeEvent(
            vecEvents[ i ], pEvent );
    }

    return ret;
}

gint32 CRpcRouter::SubscribeEvents()
{
    gint32 ret = 0;

    CConnPointHelper oCpHelper( GetIoMgr() );
    EventPtr pEvent( this );

    vector< EnumPropId > vecEvents;

    do{
        ret = oCpHelper.SubscribeEvent(
            propRmtSvrEvent, pEvent );

        if( ERROR( ret ) )
            break;

        vecEvents.push_back( propRmtSvrEvent );

        ret = oCpHelper.SubscribeEvent(
            propAdminEvent, pEvent );

        if( ERROR( ret ) )
            break;

        vecEvents.push_back( propAdminEvent );

        ret = oCpHelper.SubscribeEvent(
            propRmtModEvent, pEvent );

        if( ERROR( ret ) )
            break;

        vecEvents.push_back( propRmtModEvent );

    }while( 0 );

    do{
        if( ERROR( ret ) )
        {
            for( guint32 i = 0; i < vecEvents.size(); i++ )
            {
                oCpHelper.UnsubscribeEvent(
                    vecEvents[ i ], pEvent );
            }
            vecEvents.clear();
        }
        else 
        {
            CStdRMutex oRouterLock( GetLock() );
            m_vecTopicList = vecEvents;
        }
    }while( 0 );

    return ret;
}

gint32 CRpcRouter::OnEvent(
    EnumEventId iEvent,
    guint32 dwParam1,
    guint32 dwParam2,
    guint32* pData )
{
    gint32 ret = 0;
    switch( iEvent )
    {
    case eventRmtSvrOnline:
        {
            HANDLE hPort = ( guint32 )pData;

            string strIpAddr =
                reinterpret_cast< char* >( dwParam1 );

            ret = OnRmtSvrOnline( iEvent,
                strIpAddr, HandleToPort( hPort ) ); 

            break;
        }
    case eventRmtSvrOffline:
    default:
        {
            ret = -ENOTSUP;
            break;
        }
    }

    return ret;
}

gint32 CRpcRouter::OnRmtSvrOnline(
    EnumEventId iEvent,
    const string& strIpAddr,
    IPort* pPort )
{
    if( pPort == nullptr ||
        strIpAddr.empty() )
        return -EINVAL;

    gint32 ret = 0;

    do{
        InterfPtr pIf;
        ret = GetBridgeProxy( strIpAddr, pIf );
        if( SUCCEEDED( ret ) )
            break;

        if( ret == -ENOENT )
        {
            // passive bridge creation
            CParamList oParams;

            oParams.SetStrProp( propIpAddr, strIpAddr );
            oParams.SetPointer( propIoMgr, GetIoMgr() );

            oParams.SetObjPtr(
                propRouterPtr, ObjPtr( this ) );

            TaskletPtr pTask;

            ret = pTask.NewObj(
                clsid( CRouterOpenRmtPortTask  ),
                oParams.GetCfg() );

            if( ERROR( ret ) )
                break;

            // run this task immediately
            ( *pTask )( eventZero );
            ret = pTask->GetError();
        }
        break;

    }while( 0 );

    return ret;
}

gint32 CRpcRouter::OnRmtSvrOffline(
    EnumEventId iEvent,
    const string& strIpAddr,
    IPort* pPort )
{
    return 0;
}

gint32 CRpcRouter::CheckReqToRelay(
    const std::string &strIpAddr,
    DMsgPtr& pMsg,
    MatchPtr& pMatchHit )
{
    gint32 ret = ERROR_FALSE;
    MatchPtr pMatch;
    if( pMsg.IsEmpty() ||
        strIpAddr.empty() )
        return -EINVAL;

    CStdRMutex oRouterLock( GetLock() );
    for( auto&& oPair : m_mapRmtMatches )
    {
        CRouterRemoteMatch* pMatch =
            oPair.first;

        if( pMatch == nullptr )
            continue;

        ret = pMatch->IsMyReqToForward(
            strIpAddr, pMsg );

        if( SUCCEEDED( ret ) )
        {
            pMatchHit = pMatch;
            break;
        }
    }

    return ret;
}

gint32 CRpcRouter::CheckReqToFwrd(
    const string &strIpAddr,
    DMsgPtr& pMsg,
    MatchPtr& pMatchHit )
{
    gint32 ret = ERROR_FALSE;

    CStdRMutex oRouterLock( GetLock() );
    for( auto&& oPair : m_mapLocMatches )
    {
        CRouterLocalMatch* pMatch =
            oPair.first;

        if( pMatch == nullptr )
            continue;

        ret = pMatch->IsMyReqToForward(
            strIpAddr, pMsg );

        if( SUCCEEDED( ret ) )
        {
            pMatchHit = pMatch;
            break;
        }
    }

    return ret;
}

gint32 CRpcRouter::CheckEvtToFwrd(
    const string &strIpAddr,
    DMsgPtr& pMsg,
    vector< MatchPtr > vecMatches )
{
    gint32 ret = ERROR_FALSE;

    CStdRMutex oRouterLock( GetLock() );
    for( auto&& oPair : m_mapLocMatches )
    {
        CRouterLocalMatch* pMatch =
            oPair.first;

        if( pMatch == nullptr )
            continue;

        ret = pMatch->IsMyEvtToForward(
            strIpAddr, pMsg );

        if( SUCCEEDED( ret ) )
        {
            vecMatches.push_back( pMatch );
            break;
        }
    }

    return ret;
}

gint32 CRpcRouter::ScheduleEventRelayTask(
    IMessageMatch* pMatch,
    IEventSink* pCallback,
    CRpcReqForwarderProxy* pProxy,
    bool bAdd )
{
    gint32 ret = 0;
    do{
        CParamList oParams;
        CIoManager* pMgr = GetIoMgr();

        oParams.Push( bAdd );
        oParams.Push( ObjPtr( pMatch ) );

        oParams[ propIfPtr ] = ObjPtr( pProxy );
        oParams[ propIoMgr ] = ObjPtr( pMgr );
        oParams[ propEventSink ] = ObjPtr( pCallback );

        // create a task for this async call
        TaskletPtr pTask;
        ret = pTask.NewObj(
            clsid( CRouterEnableEventRelayTask ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        ret = pMgr->RescheduleTask( pTask );
        if( SUCCEEDED( ret ) )
            ret = STATUS_PENDING;

    }while( 0 );

    return ret;
}

gint32 CRpcRouter::GetMatchToAdd(
    IMessageMatch* pMatch,
    bool bRemote,
    MatchPtr& pMatchAdd )
{
    gint32 ret = 0;
    CRouterRemoteMatch* prlm = nullptr;
    MatchPtr ptrMatch;
    do{
        if( bRemote )
        {
            ret = ptrMatch.NewObj(
                clsid( CRouterRemoteMatch ) );
        }
        else
        {
            ret = ptrMatch.NewObj(
                clsid( CRouterLocalMatch ) );
        }

        if( ERROR( ret ) )
            break;

        prlm = ptrMatch;
        ret = prlm->CopyMatch( pMatch );

        if( ERROR( ret ) )
            break;

        pMatchAdd = ptrMatch;

    }while( 0 );

    return ret;
}

gint32 CRpcRouter::AddLocalMatch(
    IMessageMatch* pMatch )
{
    MatchPtr ptrMatch;
    gint32 ret = GetMatchToAdd(
        pMatch, false, ptrMatch );

    if( ERROR( ret ) )
        return ret;

    return AddRemoveMatch(
        pMatch, true, false, nullptr );
}

gint32 CRpcRouter::RemoveLocalMatch(
    IMessageMatch* pMatch )
{
    MatchPtr ptrMatch;
    gint32 ret = GetMatchToRemove(
        pMatch, false, ptrMatch );

    if( ERROR( ret ) )
        return ret;

    return AddRemoveMatch(
        pMatch, false, false, nullptr );
}

gint32 CRpcRouter::AddRemoveMatch(
    IMessageMatch* pMatch,
    bool bAdd, bool bRemote,
    IEventSink* pCallback )
{
    if( pMatch == nullptr ||
        pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    MatchPtr ptrMatch( pMatch );
    map< MatchPtr, gint32 >* plm =
        &m_mapRmtMatches;

    if( !bRemote )
        plm = &m_mapLocMatches;

    do{
        bool bFound = false;
        CStdRMutex oRouterLock( GetLock() );
        map< MatchPtr, gint32 >::iterator
            itr = plm->find( ptrMatch );

        if( itr != plm->end() )
            bFound = true;

        if( bAdd )
        {
            if( !bFound )
            {
                ( *plm )[ ptrMatch ] = 1;
            }
            else
            {
                ++( itr->second );
            }
        }
        else
        {
            if( bFound )
            {
                --( itr->second );
                if( itr->second <= 0 )
                {
                    plm->erase( itr );
                }
            }
        }

    }while( 0 );

    return ret;
}

gint32 CRpcRouter::RemoveRemoteMatch(
    IMessageMatch* pMatch,
    IEventSink* pCallback )
{
    if( pMatch == nullptr ||
        pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        ret = AddRemoveMatch(
            pMatch, false, true, nullptr );

        if( ERROR( ret ) )
            break;
        
        InterfPtr pIf;
        ret = GetReqFwdrProxy( pMatch, pIf );
        if( ERROR( ret ) )
            break;

        CRpcReqForwarderProxy* pProxy = pIf;
        if( unlikely( pProxy == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }
        // disable the relay
        ret = ScheduleEventRelayTask(
            pMatch, pCallback, pProxy, false );

    }while( 0 );

    return ret;
}

gint32 CRpcRouter::AddRemoteMatch(
    IMessageMatch* pMatch,
    IEventSink* pCallback )
{
    if( pMatch == nullptr ||
        pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        bool bStart = false;
        InterfPtr pProxy;
        ret = GetReqFwdrProxy( pMatch, pProxy );
        if( ERROR( ret ) )
            bStart = true;
        
        if( bStart )
        {
            // BUGBUG: there could be rare condition if
            // more than one enableEvent requests for
            // the proxy with the same match surges
            // in, which could cause many proxy objects
            // creations and removals in a very short
            // time
            ret = StartReqFwdrProxy(
                pMatch, pCallback );

            if( ret == STATUS_PENDING ||
                ERROR( ret ) )
                break;

            ret = GetReqFwdrProxy(
                pMatch, pProxy );

            if( ERROR( ret ) )
                break;
        }

        ret = ScheduleEventRelayTask(
            pMatch, pCallback, pProxy, true );

        if( ERROR( ret ) )
            break;

    }while( 0 );

    return ret;
}

gint32 CRouterEnableEventRelayTask::RunTask()
{
    gint32 ret = 0;
    do{
        CParamList oParams;
        CParamList oTaskCfg(
            ( IConfigDb* )GetConfig() );

        bool bEnable = false;
        ret = oTaskCfg.GetBoolProp( 0, bEnable );
        if( ERROR( ret ) )
            break;

        IMessageMatch* pMatch = nullptr;
        ret = oTaskCfg.GetPointer( 1, pMatch );
        if( ERROR( ret ) )
            break;

        CRpcReqForwarderProxy* pIf = nullptr;
        ret = oTaskCfg.GetPointer( propIfPtr, pIf );
        if( ERROR( ret ) )
            break;

        // enable or disable event
        oParams.Push( bEnable );

        oParams[ propIfPtr ] = ObjPtr( pIf );
        oParams[ propMatchPtr ] = ObjPtr( pMatch );
        oParams[ propEventSink ] = ObjPtr( this );
        oParams[ propNotifyClient ] = true;

        TaskletPtr pEnableEvtTask;
        ret = pEnableEvtTask.NewObj(
            clsid( CIfEnableEventTask ),
            oParams.GetCfg() );

        ret = pIf->AddAndRun( pEnableEvtTask );

        if( ERROR( ret ) )
            break;

        ret = pEnableEvtTask->GetError();

    }while( 0 );

    if( ret != STATUS_PENDING ||
        ret != STATUS_MORE_PROCESS_NEEDED )
    {
        ret = OnTaskComplete( ret );
    }

    return ret;
}

gint32 CRouterEnableEventRelayTask::OnTaskCompleteEnable(
    gint32 iRetVal )
{
    // this task will run when the CIfIoReqTask
    // created in EnableEventRelay finishes
    gint32 ret = 0;

    do{
        CParamList oParams(
            ( IConfigDb* )GetConfig() );

        CRpcReqForwarderProxy* pProxy = nullptr;
        ret = oParams.GetPointer(
            propIfPtr, pProxy );

        if( ERROR( ret ) )
            break;

        CRpcRouter* pRouter = pProxy->GetParent();
        if( unlikely( pRouter == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }

        IMessageMatch* pMatch = nullptr;
        ret = oParams.GetPointer( 1, pMatch );
        if( ERROR( ret ) )
            break;

        if( iRetVal == -ENOTCONN )
        {
            pProxy->AddInterface( pMatch );
            // the server is not up, but the match is
            // ready. move on to add the match
            pRouter->AddRemoveMatch(
                pMatch, true, true, nullptr );

            ret = 0;
            break;
        }
        else if( SUCCEEDED( iRetVal ) )
        {
            pProxy->AddInterface( pMatch );
            pRouter->AddRemoveMatch(
                pMatch, true, true, nullptr );

            // Start listening on this match
            TaskletPtr pRecvMsgTask;
            CParamList oStartParams;

            ret = oStartParams.SetPointer(
                propIfPtr, pProxy );

            ret = oStartParams.SetPointer(
                propMatchPtr, pMatch );

            ret = pRecvMsgTask.NewObj(
                clsid( CIfStartRecvMsgTask ),
                oStartParams.GetCfg() );

            if( ERROR( ret ) )
                break;

            ret = pProxy->AddAndRun( pRecvMsgTask );
            // we don't care whatever returns
        }

    }while( 0 );

    if( ret != STATUS_PENDING &&
        ret != STATUS_MORE_PROCESS_NEEDED )
    {
        // clear the objects
        CParamList oParams( m_pCtx );
        oParams.ClearParams();
    }

    return ret;
}

gint32 CRouterEnableEventRelayTask::OnTaskCompleteDisable(
    gint32 iRetVal )
{
    gint32 ret = 0;
    do{
        CParamList oTaskCfg(
            ( IConfigDb* )GetConfig() );

        CRpcReqForwarderProxy* pProxy = nullptr;
        ret = oTaskCfg.GetPointer(
            propIfPtr, pProxy );

        if( ERROR( ret ) )
            break;

        IMessageMatch* pMatch = nullptr;
        ret = oTaskCfg.GetPointer( 1, pMatch );
        if( ERROR( ret ) )
            break;

        ret = pProxy->RemoveInterface( pMatch );
        if( pProxy->GetActiveIfCount() == 0 )
        {
            CRpcRouter* pRouter = pProxy->GetParent();

            EventPtr pEvt;
            GetInterceptTask( pEvt );

            ret = pRouter->StopReqFwdrProxy(
                ObjPtr( pProxy ),
                pMatch, pEvt );

            if( ret == STATUS_PENDING )
            {
                // nasty, we have transfered the
                // callback to another task
                ClearClientNotify();

                // this task is completed.
                ret = 0;
            }
        }

    }while( 0 );

    return ret;
}

gint32 CRouterEnableEventRelayTask::OnTaskComplete(
    gint32 iRetVal )
{
    gint32 ret = 0;

    do{
        CParamList oParams(
            ( IConfigDb* )GetConfig() );

        bool bEnable = false;
        ret = oParams.GetBoolProp( 0, bEnable );
        if( ERROR( ret ) )
            break;

        if( bEnable )
            ret = OnTaskCompleteEnable( iRetVal );
        else
            ret = OnTaskCompleteDisable( iRetVal );

    }while( 0 );

    return ret;
}
