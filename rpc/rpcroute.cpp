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
        CRegPreloadable( GetIoMgr(), this );

        do{
            InterfPtr pIf;
            CParamList oParams;
            oParams.SetPointer( propIoMgr, GetIoMgr() );

            ret = StartReqFwder(
                pIf, nullptr, true );

            if( ERROR( ret ) )
                break;

            ret = StartReqFwder(
                pIf, nullptr, false );

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
    ret = m_pReqFwdrProxy->Stop();

    // Unregister the router
    // NOTE: after this call, there is no known
    // global reference held. The caller should
    // make sure the reference count is not zero
    // if it still want to use this object
    CRegPreloadable( GetIoMgr(), this, false );
    return ret;
}

gint32 CRpcRouter::StartReqFwder( 
    InterfPtr& pIf, IEventSink* pCallback, bool bProxy )
{
    gint32 ret = 0;
    do{
        CParamList oParams;
        oParams.SetPointer( propIoMgr, GetIoMgr() );

        EnumClsid iClsid =
            clsid( CRpcReqForwarderProxyImpl );

        if( !bProxy )
            iClsid = clsid( CRpcReqForwarderImpl );

        ret = CRpcServices::LoadObjDesc(
            ROUTER_OBJ_DESC,
            OBJNAME_REQFWDR,
            !bProxy,
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        oParams.SetObjPtr(
            propParentPtr, ObjPtr( this ) );

        ret = pIf.NewObj(
            iClsid, oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        if( bProxy )
            m_pReqFwdrProxy = pIf;
        else
            m_pReqFwdr = pIf;

        ObjPtr pTask;
        ret = pTask.NewObj(
            clsid( CIfDummyTask ) );

        if( ERROR( ret ) )
            break;

        ret = pIf->StartEx( pTask );

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
        CStdRMutex oStateLock( GetLock() );
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
            CStdRMutex oStateLock( GetLock() );
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

gint32 CRpcRouter::AddLocalMatch(
    IMessageMatch* pMatch )
{
    return AddRemoveMatch(
        pMatch, true, false, nullptr );
}

gint32 CRpcRouter::RemoveLocalMatch(
    IMessageMatch* pMatch )
{
    return AddRemoveMatch(
        pMatch, false, false, nullptr );
}

gint32 CRpcRouter::AddRemoteMatch(
    IMessageMatch* pMatch,
    IEventSink* pCallback )
{
    return AddRemoveMatch(
        pMatch, true, true, pCallback );
}

gint32 CRpcRouter::RemoveRemoteMatch(
    IMessageMatch* pMatch,
    IEventSink* pCallback )
{
    return AddRemoveMatch(
        pMatch, false, true, pCallback );
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

    MatchPtr matchPtr( pMatch );

    map< MatchPtr, gint32 >* plm =
        &m_mapRmtMatches;

    if( !bRemote )
        plm = &m_mapLocMatches;

    bool bEnableRelay = false;
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

        CRouterRemoteMatch* prlm = ptrMatch;
        ret = prlm->CopyMatch( pMatch );
        if( ERROR( ret ) )
            break;

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

                if( bRemote )
                    bEnableRelay = true;
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
                if( itr->second < 1 )
                {
                    plm->erase( itr );
                }
            }
        }
    }while( 0 );

    while( bEnableRelay )
    {
        // NOTE: we use the original match for
        // dbus listening
        CParamList oParams;
        oParams.Push( bAdd );
        oParams.Push( ObjPtr( pMatch ) );

        ret = oParams.SetObjPtr( propIfPtr,
            ObjPtr( m_pReqFwdrProxy ) );

        if( ERROR( ret ) )
            break;

        ret = oParams.SetPointer( propIoMgr, GetIoMgr() );

        if( ERROR( ret ) )
            break;

        // create a task for this async call

        TaskletPtr pTask;
        ret = pTask.NewObj(
            clsid( CRouterEnableEventRelayTask ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        CRouterEnableEventRelayTask*
            pRelayTask = pTask;

        if( pRelayTask != nullptr )
        {
            pRelayTask->SetInterceptTask(
                pCallback );
        }

        ( *pTask )( eventZero );
        ret = pTask->GetError();

        break;
    }

    if( bEnableRelay && ERROR( ret ) )
    {
        if( !ptrMatch.IsEmpty() )
        {
            CStdRMutex oRouterLock( GetLock() );
            plm->erase( ptrMatch );
        }
    }

    return ret;
}

gint32 CRouterEnableEventRelayTask::EnableEventRelay(
    CRpcServices* pIf,
    IMessageMatch* pMatch,
    bool bEnable )
{
    gint32 ret = 0;

    if( pMatch == nullptr || 
        pIf == nullptr )
        return -EINVAL;

    do{
        CReqBuilder oBuilder( pIf );

        oBuilder.Push( bEnable );
        oBuilder.Push( ObjPtr( pMatch ) );

        if( bEnable )
        {
            oBuilder.SetMethodName(
                IF_METHOD_ENABLEEVT );
        }
        else
        {
            oBuilder.SetMethodName(
                IF_METHOD_DISABLEEVT );
        }

        oBuilder.SetCallFlags( CF_WITH_REPLY
           | DBUS_MESSAGE_TYPE_METHOD_CALL );

        EventPtr pEvent;
        ret = GetInterceptTask( pEvent );
        if( ERROR( ret ) )
            break;

        // retrieve the response ptr if exists on
        // the callback object
        CfgPtr pRespCfg;
        ObjPtr pObj;

        CCfgOpenerObj oTaskCfg(
            ( CObjBase* )pEvent );

        // use the resp data from the callback if
        // exists
        ret = oTaskCfg.GetObjPtr(
            propRespPtr, pObj );

        if( SUCCEEDED( ret ) )
        {
            pRespCfg = pObj;
        }
        else
        {
            CParamList oParams;
            pRespCfg = oParams.GetCfg();
        }

        // we need the response to pass back from
        // the RunIoTask if it is asynchronous
        // call
        this->SetRespData( pRespCfg );

        ret = pIf->RunIoTask( oBuilder.GetCfg(),
            pRespCfg, this );

        if( ret == STATUS_PENDING )
            break;

        CCfgOpener oRespCfg(
            ( IConfigDb* )pRespCfg );

        // if the RunIoTask returns immediately 
        if( SUCCEEDED( ret ) )
        {
            gint32 iRet = 0;

            ret = oRespCfg.GetIntProp(
                propReturnValue, ( guint32& )iRet );

            if( SUCCEEDED( ret ) )
                ret = iRet;
        }
        else
        {
             oRespCfg.SetIntProp(
                propReturnValue, ret );
        }

    }while( 0 );

    return ret;
}

gint32 CRouterEnableEventRelayTask::RunTask()
{
    gint32 ret = 0;
    do{

        CParamList oParams( m_pCtx );
        InterfPtr pIf;
        ObjPtr pObj;
        ret = oParams.GetObjPtr( propIfPtr, pObj );
        if( ERROR( ret ) )
            break;

        pIf = pObj;
        if( pIf.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        CRpcReqForwarderProxy* pProxy = pIf;
        if( pProxy == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        CRpcRouter* pRouter = pProxy->GetParent();
        if( pRouter == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        bool bEnable = false;
        MatchPtr pMatch;

        ret = oParams.GetBoolProp( 0, bEnable );
        if( ERROR( ret ) )
            break;

        ret = oParams.GetObjPtr( 1, pObj );
        if( ERROR( ret ) )
            break;
        pMatch = pObj;

        ret = EnableEventRelay(
            pProxy, pMatch, bEnable );

    }while( 0 );

    if( ret != STATUS_PENDING ||
        ret != STATUS_MORE_PROCESS_NEEDED )
    {
        ret = OnTaskComplete( ret );
    }

    return ret;
}

gint32 CRouterEnableEventRelayTask::OnTaskComplete(
    gint32 iRetVal )
{
    // this task will run when the CIfIoReqTask
    // created in EnableEventRelay finishes
    gint32 ret = 0;

    do{
        EventPtr pEvent;
        ret = GetInterceptTask( pEvent );
        if( ERROR( ret ) )
            break;

        CParamList oParams( m_pCtx );

        InterfPtr pIf;
        ObjPtr pObj;

        ret = oParams.GetObjPtr(
            propIfPtr, pObj );

        if( ERROR( ret ) )
            break;

        pIf = pObj;
        if( pIf.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        CRpcReqForwarderProxy* pProxy = pIf;
        if( pProxy == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        bool bEnable = false;
        MatchPtr pMatch;

        ret = oParams.GetBoolProp( 0, bEnable );
        if( ERROR( ret ) )
            break;

        ret = oParams.GetObjPtr( 1, pObj );
        if( ERROR( ret ) )
            break;

        pMatch = pObj;

        if( ERROR( iRetVal ) )
        {
            // rollback what we have done
            CRpcRouter* pRouter =
                pProxy->GetParent();

            if( pRouter == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            if( bEnable )
            {
                // this call must be a synchronous
                // call
                pRouter->RemoveRemoteMatch(
                    pMatch, pEvent );
            }
            else
            {
                // no add-back
            }
        }
        else
        {
            // Start listening on this match
            if( bEnable ) 
            {
                TaskletPtr pRecvMsgTask;
                CParamList oStartParams;

                ret = oStartParams.SetObjPtr(
                    propIfPtr, ObjPtr( pProxy ) );

                ret = oStartParams.SetObjPtr(
                    propMatchPtr, ObjPtr( pMatch ) );

                ret = pRecvMsgTask.NewObj(
                    clsid( CIfStartRecvMsgTask ),
                    oStartParams.GetCfg() );

                if( ERROR( ret ) )
                    break;

                // ignore the return even if it
                // returns
                pProxy->AddAndRun( pRecvMsgTask );
            }
            else
            {
                // do nothing and the
                // StartRecvMsgTask will be
                // completed by the irp canceled
                // during match removal in the
                // port
                
            }
        }

    }while( 0 );

    if( ret != STATUS_PENDING &&
        ret != STATUS_MORE_PROCESS_NEEDED )
    {
        // clear the objects
        CParamList oParams( m_pCtx );
        bool bEnable = false;
        oParams.Pop( bEnable );
        ObjPtr pObj;
        oParams.Pop( pObj );
    }

    return ret;
}
