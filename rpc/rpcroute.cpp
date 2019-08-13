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
 *      Copyright:  2019 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
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
    : super( pCfg ),
    m_dwRole( 1 )
{
    gint32 ret = 0;

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

        guint32 dwRole = 0;
        ret = oCfg.GetIntProp(
            propRouterRole, dwRole );

        if( ERROR( ret ) )
            break;

        if( SUCCEEDED( ret ) )
            m_dwRole = dwRole;

        CParamList oParams;
        oParams[ propIoMgr ] = 
            ObjPtr( GetIoMgr() );

        ret = m_pDBusSysMatch.NewObj(
            clsid( CDBusSysMatch ) );

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
    gint32 ret = 0;

    if( !HasReqForwarder() )
        return -ENOENT;    

    do{
        if( strIpAddr.empty() )
        {
            ret = -EINVAL;
            break;
        }

        std::map< std::string, InterfPtr >*
            pMap = &m_mapIp2BdgeProxies;
        
        CStdRMutex oRouterLock( GetLock() );
        std::map< std::string, InterfPtr >::iterator 
            itr = pMap->find( strIpAddr );

        if( itr == pMap->end() )
        {
            ret = -ENOENT;
            break;
        }

        pIf = itr->second;

    }while( 0 );

    return ret;
}

gint32 CRpcRouter::GetBridge(
    guint32 dwPortId,
    InterfPtr& pIf )
{
    gint32 ret = 0;

    if( !HasBridge() )
        return -ENOENT;

    do{
        std::map< guint32, InterfPtr >*
            pMap = &m_mapPortId2Bdge;
        
        CStdRMutex oRouterLock( GetLock() );
        std::map< guint32, InterfPtr >::iterator 
            itr = pMap->find( dwPortId );

        if( itr == pMap->end() )
        {
            ret = -ENOENT;
            break;
        }
        pIf = itr->second;

    }while( 0 );

    return ret;
}


gint32 CRpcRouter::AddBridgeProxy( 
    IGenericInterface* pIf )
{
    if( pIf == nullptr )
        return -EINVAL;

    if( !IsConnected() )
        return ERROR_STATE;

    gint32 ret = 0;
    do{
        std::map< std::string, InterfPtr >* pMap =
            &m_mapIp2BdgeProxies;
        
        std::string strIpAddr;
        CCfgOpenerObj oCfg( pIf );

        ret = oCfg.GetStrProp(
            propIpAddr, strIpAddr );

        CStdRMutex oRouterLock( GetLock() );
        std::map< std::string, InterfPtr >::iterator 
            itr = pMap->find( strIpAddr );

        if( itr != pMap->end() )
        {
            ret = -EEXIST;
            break;
        }

        ( *pMap )[ strIpAddr ] = InterfPtr( pIf );

    }while( 0 );

    return ret;
}

gint32 CRpcRouter::RemoveBridgeProxy( 
    IGenericInterface* pIf )
{
    if( pIf == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        std::map< std::string, InterfPtr >* pMap =
            &m_mapIp2BdgeProxies;
        
        std::string strIpAddr;
        CCfgOpenerObj oCfg( pIf );

        ret = oCfg.GetStrProp(
            propIpAddr, strIpAddr );

        CStdRMutex oRouterLock( GetLock() );
        std::map< std::string, InterfPtr >::iterator 
            itr = pMap->find( strIpAddr );

        if( itr == pMap->end() )
        {
            ret = -ENOENT;
            break;
        }
        pMap->erase( strIpAddr );

    }while( 0 );

    return ret;
}

gint32 CRpcRouter::AddBridge( 
    IGenericInterface* pIf )
{
    if( pIf == nullptr )
        return -EINVAL;

    if( !IsConnected() )
        return ERROR_STATE;

    gint32 ret = 0;

    do{
        std::map< guint32, InterfPtr >* pMap =
            &m_mapPortId2Bdge;

        guint32 dwPortId = 0;
        CCfgOpenerObj oCfg( pIf );

        ret = oCfg.GetIntProp(
            propPortId, dwPortId );

        if( ERROR( ret ) )
            break;

        CStdRMutex oRouterLock( GetLock() );
        std::map< guint32, InterfPtr >::iterator 
            itr = pMap->find( dwPortId );

        if( itr != pMap->end() )
        {
            ret = EEXIST;
            break;
        }

        ( *pMap )[ dwPortId ] = InterfPtr( pIf );

    }while( 0 );

    return ret;
}

gint32 CRpcRouter::RemoveBridge( 
    IGenericInterface* pIf )
{
    if( pIf == nullptr )
        return -EINVAL;

    gint32 ret = 0;

    do{
        std::map< guint32, InterfPtr >* pMap =
            &m_mapPortId2Bdge;

        guint32 dwPortId = 0;
        CCfgOpenerObj oCfg( pIf );

        ret = oCfg.GetIntProp(
            propPortId, dwPortId );

        if( ERROR( ret ) )
            break;

        CStdRMutex oRouterLock( GetLock() );
        std::map< guint32, InterfPtr >::iterator 
            itr = pMap->find( dwPortId );
        if( itr == pMap->end() )
        {
            ret = -ENOENT;
            break;
        }
        pMap->erase( itr );

    }while( 0 );

    return ret;
}

gint32 CRpcRouter::Start()
{
    // register this object
    gint32 ret = 0;
    try{
        do{
            ret = super::Start();
            if( ERROR( ret ) )
                break;

            InterfPtr pIf;
            if( HasReqForwarder() )
            {
                ret = StartReqFwdr( pIf, nullptr );
                if( ERROR( ret ) )
                    break;
            }

        } while( 0 );

        return ret;
    }
    catch( system_error& e )
    {
        ret = e.code().value();
    }
    return ret;
}

gint32 CRpcRouter::OnPreStop( IEventSink* pCallback )
{
    gint32 ret = 0;
    map< string, InterfPtr > mapIp2Proxies;
    map< guint32, InterfPtr > mapId2Server;
    std::map< std::string, InterfPtr > mapReqProxies;

    // FIXME: need a task to accomodate all the StopEx
    // completion and then let the pCallback to run
    do{
        CStdRMutex oRouterLock( GetLock() );
        mapIp2Proxies = m_mapIp2BdgeProxies;
        m_mapIp2BdgeProxies.clear();
        mapId2Server = m_mapPortId2Bdge;
        m_mapPortId2Bdge.clear();
        mapReqProxies = m_mapReqProxies;
        m_mapReqProxies.clear();

    }while( 0 );

    for( auto&& elem : mapIp2Proxies )
    {
        InterfPtr p;
        p = elem.second;
        p->Stop();
    }
    mapIp2Proxies.clear();

    for( auto&& elem : mapId2Server )
    {
        InterfPtr p;
        p = elem.second;
        p->Stop();
    }
    mapId2Server.clear(); 

    if( !m_pReqFwdr.IsEmpty() )
        ret = m_pReqFwdr->Stop();

    for( auto elem : mapReqProxies )
    {
        gint32 iRet = elem.second->Stop();
        if( ERROR( iRet ) )
            ret = iRet;
    }
    mapReqProxies.clear();

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
            propRouterPtr, ObjPtr( this ) );

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
        if( ret == -EEXIST )
        {
            // some other guy has created this
            // interface. the one held by pIfPtr will
            // be released silentely.
            ret = pRouter->GetReqFwdrProxy(
                pMatch, pIfPtr );
            if( ERROR( ret ) )
                break;
        }
        else if( ERROR( ret ) )
        {
            DebugPrint( ret,
                "CRouterStartReqFwdrProxyTask bug "
                "cannot add newly created proxy!" );
            break;
        }

        ret = pIf->AddInterface( pMatch );
        if( ret == EEXIST )
        {
            // something wrong, don't move further
            pIf->RemoveInterface( pMatch );
            ret = -ret;
            break;
        }
        else if( ERROR( ret ) )
        {
            break;
        }

        // in case the transaction failed
        TaskletPtr pRbTask;
        ret = pRouter->BuildStartStopReqFwdrProxy(
            pMatch, false, pRbTask );

        if( SUCCEEDED( ret ) )
        {
            // it won't run if all the whole
            // tasks succeeds
            AddRollbackTask( pRbTask );
        }

    }while( 0 );

    return ret;
}

gint32 CRouterStartReqFwdrProxyTask::
    OnTaskComplete( gint32 iRetVal )
{
    CParamList oTaskCfg(
        ( IConfigDb* )GetConfig() );
    bool bStart = ( bool& )oTaskCfg[ 0 ];

    if( bStart )
        return OnTaskCompleteStart( iRetVal );

    oTaskCfg.ClearParams();
    oTaskCfg.RemoveProperty( propIfPtr );
    oTaskCfg.RemoveProperty( propRouterPtr );
    oTaskCfg.RemoveProperty( propMatchPtr );

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

        IMessageMatch* pMatch = nullptr;
        bool bStart = ( bool& )oParams[ 0 ];

        ret = oParams.GetPointer( 1, pMatch );
        if( ERROR( ret ) )
            break;

        CRpcRouter* pRouter = pIf->GetParent();

        InterfPtr pProxy;
        ret = pRouter->GetReqFwdrProxy(
            pMatch, pProxy );

        //already exist
        if( SUCCEEDED( ret ) && bStart )
        {
            ret = EEXIST;
            break;
        }
        else if( ERROR( ret ) && !bStart )
        {
            ret = -ENOENT;
            break;
        }

        if( bStart )
        {
            ret = pIf->StartEx( this );
        }
        else
        {
            CRpcRouter* pRouter =
                pIf->GetParent();

            CStdRMutex oIfLock( pIf->GetLock() );
            CStdRMutex oRouterLock(
                pRouter->GetLock() );

            pIf->RemoveInterface( pMatch );
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
                oIfLock.Unlock();

                ret = pIf->SetStateOnEvent(
                    cmdShutdown );
                if( ERROR( ret ) )
                    break;

                ret = pIf->StopEx( this );
            }
            else
            {
                ret = 0;
            }
        }

    }while( 0 );

    if( SUCCEEDED( ret ) || ret == EEXIST )
        OnTaskComplete( ret );

    return ret;
}

gint32 CRpcRouter::BuildStartStopReqFwdrProxy( 
    IMessageMatch* pMatch,
    bool bStart,
    TaskletPtr& pTask )
{
    gint32 ret = 0;
    if( pMatch == nullptr )
        return -EINVAL;

    do{
        CParamList oIfParams;
        oIfParams.SetPointer( propIoMgr, GetIoMgr() );
        CParamList oTaskCfg;

        InterfPtr pIf;
        CInterfaceProxy* pProxy = nullptr;
        // this call is probably to succeed, and
        // the task to build will check it again.
        ret = GetReqFwdrProxy( pMatch, pIf );
        if( !bStart )
        {
            if( ERROR( ret ) )
                break;
            oTaskCfg[ propIfPtr ] = ObjPtr( pIf );
        }
        else
        {
            if( SUCCEEDED( ret ) )
            {
                pProxy = pIf;
                if( pProxy == nullptr )
                {
                    ret = -EFAULT;
                    break;
                }

                if( !pProxy->IsConnected() &&
                    pProxy->GetState() != stateRecovery )
                {
                    ret = ERROR_STATE;
                    break;
                }
                // although the interface is
                // already started, we still need
                // to add the match to the
                // interface before the
                // EnableEvent can happen, so the
                // task still needs to run
                oTaskCfg[ propIfPtr ] = ObjPtr( pIf );
            }
            else
            {
                ret = CRpcServices::LoadObjDesc(
                    ROUTER_OBJ_DESC,
                    OBJNAME_REQFWDR,
                    false,
                    oIfParams.GetCfg() );

                if( ERROR( ret ) )
                    break;

                oIfParams.SetObjPtr(
                    propRouterPtr, ObjPtr( this ) );

                ret = oIfParams.CopyProp(
                    propObjPath, pMatch );

                if( ERROR( ret ) )
                    break;

                ret = oIfParams.CopyProp(
                    propDestDBusName, pMatch );

                if( ERROR( ret ) )
                    break;

                EnumClsid iClsid =
                    clsid( CRpcReqForwarderProxyImpl );

                ret = pIf.NewObj( iClsid,
                    oIfParams.GetCfg() );

                if( ERROR( ret ) )
                    break;

                oTaskCfg[ propIfPtr ] = ObjPtr( pIf );
            }
        }

        oTaskCfg.SetPointer(
            propIoMgr, GetIoMgr() );

        oTaskCfg.Push( bStart );
        oTaskCfg.Push( ObjPtr( pMatch ) );

        ret = pTask.NewObj(
            clsid( CRouterStartReqFwdrProxyTask ),
            oTaskCfg.GetCfg() );

        if( ERROR( ret ) )
            break;

    }while( 0 );

    return ret;
}


gint32 CRpcRouter::OnRmtSvrEvent(
    EnumEventId iEvent,
    const std::string& strIpAddr,
    HANDLE hPort )
{
    gint32 ret = 0;
    if( hPort == 0 || strIpAddr.empty() )
        return -EINVAL;

    TaskletPtr pDeferTask;
    switch( iEvent )
    {
    case eventRmtSvrOnline:
        {
            ObjPtr pObj;
            ret = DEFER_IFCALL_NOSCHED(
                pDeferTask,
                ObjPtr( this ),
                &CRpcRouter::OnRmtSvrOnline,
                pObj, strIpAddr, hPort );

            if( ERROR( ret ) )
                break;

            CIfDeferCallTask* pTask = pDeferTask;
            BufPtr pBuf( true );
            *pBuf = ObjPtr( pTask );

            // fill the empty callback pointer to
            // this defer task
            pTask->UpdateParamAt( 0, pBuf );
            ret = AddSeqTask( pDeferTask, false );

            if( ERROR( ret ) )
                ( *pDeferTask )( eventCancelTask );

            break;
        }
    case eventRmtSvrOffline:
        {
            ObjPtr pObj;
            ret = DEFER_IFCALL_NOSCHED(
                pDeferTask,
                ObjPtr( this ),
                &CRpcRouter::OnRmtSvrOffline,
                pObj, strIpAddr, hPort );

            if( ERROR( ret ) )
                break;

            CIfDeferCallTask* pTask = pDeferTask;
            BufPtr pBuf( true );
            *pBuf = ObjPtr( pTask );

            // fill the empty callback pointer to
            // this defer task
            pTask->UpdateParamAt( 0, pBuf );
            ret = AddSeqTask( pDeferTask, false );
            if( ERROR( ret ) )
                ( *pDeferTask )( eventCancelTask );

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

gint32 CRpcRouter::OnRmtSvrOnline(
    IEventSink* pCallback,
    const string& strIpAddr,
    HANDLE hPort )
{
    if( hPort == INVALID_HANDLE || strIpAddr.empty() )
        return -EINVAL;

    gint32 ret = 0;

    do{
        InterfPtr pIf;
        if( !IsConnected() )
        {
            ret = ERROR_STATE;
            break;
        }

        if( !HasBridge() )
        {
            // odd situation
            ret = ERROR_FAIL;
            break;
        }

        PortPtr pPort;
        ret = GetIoMgr()->GetPortPtr(
            hPort, pPort );

        if( ERROR( ret ) )
            break;

        CCfgOpenerObj oPortCfg(
            ( CObjBase* )pPort );

        guint32 dwPortId = 0;
        ret = oPortCfg.GetIntProp(
            propPortId, dwPortId );

        if( ERROR( ret ) )
            break;

        bool bServer = false;
        ret = oPortCfg.GetBoolProp(
            propIsServer, bServer );
        if( ERROR( ret ) )
            break;

        // false call
        if( ( !bServer && m_dwRole == 0x02 ) ||
            ( bServer && m_dwRole == 0x01 ) )
            break;

        // we need to sequentially GetBridge.
        // it could be possible, two
        // OpenRemotePort requests for the same ip
        // connection arrive at the same time.
        ret = GetBridge( dwPortId, pIf );
        if( SUCCEEDED( ret ) )
            break;

        if( ret == -ENOENT )
        {
            // passive bridge creation
            CParamList oParams;

            oParams[ propEventSink ] =
                ObjPtr( pCallback );

            oParams.CopyProp( propIpAddr, pPort );
            oParams.CopyProp( propSrcTcpPort, pPort );
            oParams.CopyProp( propPortId, pPort );
            oParams.SetPointer( propIoMgr, GetIoMgr() );

            oParams.SetObjPtr(
                propRouterPtr, ObjPtr( this ) );

            TaskletPtr pTask;
            ret = pTask.NewObj(
                clsid( CRouterOpenRmtPortTask  ),
                oParams.GetCfg() );

            if( ERROR( ret ) )
                break;

            ( *pTask )( eventZero );
            ret = pTask->GetError();
        }
        break;

    }while( 0 );

    return ret;
}

gint32 CRouterStopBridgeProxyTask::RunTask()
{
    gint32 ret = 0;    
    do{
        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );

        CRpcInterfaceProxy* pProxy = nullptr;
        ret = oCfg.GetPointer( 0, pProxy );
        if( ERROR( ret ) )
            break;

        string strIpAddr;
        ret = oCfg.GetStrProp(
            propIpAddr, strIpAddr );
        if( ERROR( ret ) )
            break;

        CRpcRouter* pRouter =
            pProxy->GetParent();

        InterfPtr pIf;
        pRouter->GetReqFwdr( pIf );
        CRpcReqForwarder* pReqFwdr = pIf;

        if( true )
        {
            ret = pProxy->Shutdown( this );

            // stop the bridge proxy
            CStdRMutex oRouterLock(
                pRouter->GetLock() );

            pRouter->RemoveLocalMatchByAddr(
                strIpAddr );

            pRouter->RemoveBridgeProxy( pProxy );
        }
        HANDLE hPort =
            PortToHandle( pProxy->GetPort() );

        pReqFwdr->OnEvent(
            eventRmtSvrOffline,
            ( guint32 )strIpAddr.c_str(),
            0, ( guint32* )hPort );

    }while( 0 );

    if( ret != STATUS_PENDING )
        OnTaskComplete( ret );

    return ret;
}

gint32 CRouterStopBridgeProxyTask::OnTaskComplete(
    gint32 iRetVal )
{
    CParamList oTaskCfg(
        ( IConfigDb* )GetConfig() );

    oTaskCfg.ClearParams();
    oTaskCfg.RemoveProperty( propIfPtr );
    oTaskCfg.RemoveProperty( propRouterPtr );
    oTaskCfg.RemoveProperty( propMatchPtr );

    return iRetVal;
}

gint32 CRouterStopBridgeTask::RunTask()
{
    gint32 ret = 0;    
    do{
        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );

        CRpcTcpBridge* pBridge = nullptr;
        ret = oCfg.GetPointer( 0, pBridge );
        if( ERROR( ret ) )
            break;

        guint32 dwPortId = 0;
        CCfgOpenerObj oIfCfg( pBridge );

        ret = oIfCfg.GetIntProp(
            propPortId, dwPortId );
        if( ERROR( ret ) )
            break;

        string strIpAddr;

        ret = oIfCfg.GetStrProp(
            propIpAddr, strIpAddr );
        if( ERROR( ret ) )
            break;

        CRpcRouter* pRouter =
            pBridge->GetParent();

        // put the bridge to the stopping state to
        // prevent further incoming requests
        ret = pBridge->SetStateOnEvent(
            cmdShutdown );
        if( ERROR( ret ) )
            break;

        // remove all the remote matches
        // registered via this bridge
        vector< MatchPtr > vecMatches;

        CStdRMutex oRouterLock(
            pRouter->GetLock() );

        pRouter->RemoveBridge( pBridge );
        pRouter->RemoveRemoteMatchByPortId(
            dwPortId, vecMatches );

        oRouterLock.Unlock();

        // find all the proxies with matches
        // referring to this bridge
        std::set< string > setProxies;
        for( auto elem : vecMatches )
        {
            std::string strDest;
            ret = pRouter->GetObjAddr(
                elem, strDest );
            if( SUCCEEDED( ret ) )
                setProxies.insert( strDest );
        }

        // give those proxies a chance to cancel
        // the outstanding requests from the
        // bridge to shutdown. By doing so, all
        // the tasks along the task chain will be
        // notified to cancel.
        for( auto elem : setProxies )
        {
            InterfPtr pProxy;

            ret = pRouter->GetReqFwdrProxy(
                elem, pProxy );
            if( SUCCEEDED( ret ) )
            {
                CRpcServices* pIf =
                    ( CRpcServices* )pProxy;

                HANDLE hPort = PortToHandle(
                    pIf->GetPort() );

                guint32 dwIpAddr = ( guint32 )
                    ( guint32 )strIpAddr.c_str();

                pProxy->OnEvent(
                    eventRmtSvrOffline,
                    dwIpAddr, 0,
                    ( guint32* )hPort );
            }
        }
 
        for( auto elem : vecMatches )
        {
            TaskletPtr pDummy;

            pDummy.NewObj( 
                clsid( CIfDummyTask ) );

            pRouter->RunDisableEventTask(
                pDummy, elem );
        }

        ret = pBridge->StopEx( this );
        if( ERROR( ret ) )
        {
            DebugPrint( ret, "StopBridge failed" );
            break;
        }

    }while( 0 );

    if( ret != STATUS_PENDING )
        OnTaskComplete( ret );

    return ret;
}

gint32 CRouterStopBridgeTask::OnTaskComplete(
    gint32 iRetVal )
{
    CParamList oParams(
        ( IConfigDb* )GetConfig() );

    oParams.ClearParams();
    oParams.RemoveProperty( propIfPtr );
    oParams.RemoveProperty( propRouterPtr );
    oParams.RemoveProperty( propMatchPtr );

    return iRetVal;
}

gint32 CRpcRouter::OnRmtSvrOffline(
    IEventSink* pCallback,
    const string& strIpAddr,
    HANDLE hPort )
{
    if( hPort == INVALID_HANDLE ||
        strIpAddr.empty() )
        return -EINVAL;

    gint32 ret = 0;
    CIoManager* pMgr = GetIoMgr();
    do{
        InterfPtr pIf;
        if( !IsConnected() )
        {
            ret = ERROR_STATE;
            break;
        }

        PortPtr pPort;
        ret = pMgr->GetPortPtr(
            hPort, pPort );

        if( ERROR( ret ) )
            break;

        CCfgOpenerObj oPortCfg(
            ( CObjBase* ) pPort );

        guint32 dwPortId = 0;
        ret = oPortCfg.GetIntProp(
            propPortId, dwPortId );
        if( ERROR( ret ) )
            break;

        bool bBridge = false;
        if( HasBridge() )
        {
            ret = GetBridge( dwPortId, pIf );
        }
        else
        {
            ret = ERROR_FAIL;
        }

        if( SUCCEEDED( ret ) )
        {
            bBridge = true;
        }
        else if( HasReqForwarder() )
        {
            ret = GetBridgeProxy(
                strIpAddr, pIf );
        }
         
        if( ERROR( ret ) )
        {
            break;
        }

        TaskletPtr pTask;
        CParamList oParams;

        oParams[ propEventSink ] =
            ObjPtr( pCallback );

        oParams.Push( pIf );
        oParams[ propIfPtr ] =
            ObjPtr( this );

        if( bBridge )
        {
            oParams.CopyProp(
                propPortId, pPort );

            // stop the bridge
            ret = pTask.NewObj(
                clsid( CRouterStopBridgeTask ),
                oParams.GetCfg() );

            if( ERROR( ret ) )
                break;
        }
        else
        {
            oParams.SetStrProp(
                propIpAddr, strIpAddr );

            ret = pTask.NewObj(
                clsid( CRouterStopBridgeProxyTask  ),
                oParams.GetCfg() );

            if( ERROR( ret ) )
                break;
        }

        ret = ( *pTask )( eventZero );

    }while( 0 );

    if( ERROR( ret ) )
    {
        // close the port if exists
        pMgr->ClosePort( hPort, nullptr );
    }

    return ret;
}

gint32 CRpcRouter::OnModEvent(
    EnumEventId iEvent,
    const std::string& strModule )
{
    if( strModule.empty() )
        return -EINVAL;

    if( strModule[ 0 ] == ':' )
    {
        // uniq name, not our call
        return 0;
    }
    return ForwardModOnOfflineEvent(
        iEvent, strModule );
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
    vector< MatchPtr >& vecMatches )
{
    gint32 ret = ERROR_FALSE;

    do{
        CStdRMutex oRouterLock( GetLock() );
        ret = m_pDBusSysMatch->IsMyMsgIncoming( pMsg );
        if( SUCCEEDED( ret ) )
        {
            string strDest;
            string strMember;
            strMember = pMsg.GetMember();
            if( strMember != "NameOwnerChanged" )
            {
                // currently we don't support other
                // messages.
                break;
            }

            std::set< string > setSrcAddr;
            ret = pMsg.GetStrArgAt( 0, strDest );
            if( ERROR( ret ) )
                break;

            for( auto&& oPair : m_mapLocMatches )
            {
                CRouterLocalMatch* pMatch =
                    oPair.first;

                if( unlikely( pMatch == nullptr ) )
                    continue;

                CCfgOpener oMatch(
                    ( IConfigDb* )pMatch->GetCfg() );

                ret = oMatch.IsEqual(
                    propIpAddr, strIpAddr );

                if( ERROR( ret ) )
                    continue;

                ret = oMatch.IsEqual(
                    propDestDBusName, strDest );

                if( ERROR( ret ) )
                    continue;

                string strUniqName =
                    oMatch[ propSrcUniqName ];

                if( setSrcAddr.find( strUniqName ) ==
                    setSrcAddr.end() )
                {
                    setSrcAddr.insert( strUniqName );
                    vecMatches.push_back( pMatch );
                }
            }
            ret = 0;
            break;
        }

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
            }
            else
            {
                ret = 0;
            }
        }
    }while( 0 );

    return ret;
}

gint32 CRpcRouter::BuildEventRelayTask(
    IMessageMatch* pMatch,
    bool bAdd,
    TaskletPtr& pTask )
{
    gint32 ret = 0;
    do{
        CParamList oParams;

        InterfPtr pIf;

        // this call could fail if the reqfwdr
        // proxy is not created yet. But the proxy
        // should be ready when the task starts to
        // run
        ret = GetReqFwdrProxy( pMatch, pIf );
        if( SUCCEEDED( ret ) )
        {
            oParams[ propIfPtr ] = ObjPtr( pIf );
        }
        else if( ERROR( ret ) && !bAdd )
        {
            // nothing to disable
            break;
        }

        CIoManager* pMgr = GetIoMgr();

        oParams.Push( bAdd );
        oParams.Push( ObjPtr( pMatch ) );

        oParams[ propRouterPtr ] = ObjPtr( this );
        oParams[ propIoMgr ] = ObjPtr( pMgr );

        ret = pTask.NewObj(
            clsid( CRouterEnableEventRelayTask ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

    }while( 0 );

    return ret;
}

gint32 CRpcRouter::GetMatchToAdd(
    IMessageMatch* pMatch,
    bool bRemote,
    MatchPtr& pMatchAdd ) const
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

gint32 CRpcRouter::IsMatchOnline(
    IMessageMatch* pMatch, bool& bOnline ) const
{
    MatchPtr ptrMatch;

    if( pMatch == nullptr )
        return -EINVAL;

    gint32 ret = GetMatchToAdd(
        pMatch, false, ptrMatch );

    if( ERROR( ret ) )
        return ret;

    const map< MatchPtr, gint32 >*
        plm = &m_mapLocMatches;

    do{
        CStdRMutex oRouterLock( GetLock() );
        map< MatchPtr, gint32 >::const_iterator
            itr = plm->find( ptrMatch );

        if( itr != plm->end() )
        {
            ret = -ENOENT;
            break;
        }

        CMessageMatch* pDstMat = itr->first;
        CCfgOpener oMatchCfg(
            ( IConfigDb* )pDstMat->GetCfg() );

        ret = oMatchCfg.GetBoolProp(
            propOnline, bOnline );

    }while( 0 );

    return ret;
}

gint32 CRpcRouter::SetMatchOnline(
    IMessageMatch* pMatch, bool bOnline )
{
    MatchPtr ptrMatch;
    if( pMatch == nullptr )
        return -EINVAL;

    CMessageMatch* pSrcMat =
        static_cast< CMessageMatch* >( pMatch );
    CCfgOpener oSrcMatch(
        ( IConfigDb* )pSrcMat->GetCfg() );

    gint32 ret = 0;
    do{
        std::string strIpAddr;
        ret = oSrcMatch.GetStrProp(
            propIpAddr, strIpAddr );
        if( ERROR( ret ) )
            break;

        std::string strDest;
        ret = oSrcMatch.GetStrProp(
            propDestDBusName, strDest );
        if( ERROR( ret ) )
            break;

        std::string strPath;
        ret = oSrcMatch.GetStrProp(
            propObjPath, strPath );
        if( ERROR( ret ) )
            break;

        CStdRMutex oRouterLock( GetLock() );
        for( auto oElem : m_mapLocMatches )
        {
            CMessageMatch* pDstMat = oElem.first;
            CCfgOpener oMatchCfg(
                ( IConfigDb* )pDstMat->GetCfg() );

            ret = oMatchCfg.IsEqual(
                propObjPath, strPath );

            if( ERROR( ret ) )
                continue;

            ret = oMatchCfg.IsEqual(
                propDestDBusName, strDest );

            if( ERROR( ret ) )
                continue;

            ret = oMatchCfg.IsEqual(
                propIpAddr, strIpAddr );

            if( ERROR( ret ) )
                continue;

            oMatchCfg.SetBoolProp(
                propOnline, bOnline );
        }

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
        ptrMatch, true, false, nullptr );
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
        ptrMatch, false, false, nullptr );
}

gint32 CRpcRouter::RemoveLocalMatchByAddr(
    const std::string& strIpAddr )
{
    gint32 ret = 0;
    gint32 iCount = 0;

    do{
        map< MatchPtr, gint32 >* plm =
            &m_mapLocMatches;

        CStdRMutex oRouterLock( GetLock() );
        map< MatchPtr, gint32 >::iterator
            itr = plm->begin();

        while( itr != plm->end() )
        {
            CCfgOpenerObj oMatch(
                ( CObjBase* )itr->first );
            std::string strVal;
            ret = oMatch.GetStrProp(
                propIpAddr, strVal );

            if( ERROR( ret ) )
            {
                itr++;
                continue;
            }

            if( strIpAddr == strVal )
            {
                itr = plm->erase( itr );
                iCount++;
                continue;
            }
            itr++;
        }

    }while( 0 );

    return iCount;
}

gint32 CRpcRouter::RemoveLocalMatchByUniqName(
    const std::string& strUniqName,
    std::vector< MatchPtr >& vecMatches )
{
    gint32 ret = 0;

    do{
        map< MatchPtr, gint32 >* plm =
            &m_mapLocMatches;

        CStdRMutex oRouterLock( GetLock() );
        map< MatchPtr, gint32 >::iterator
            itr = plm->begin();

        while( itr != plm->end() )
        {
            CCfgOpenerObj oMatch(
                ( CObjBase* )itr->first );

            std::string strVal;
            ret = oMatch.GetStrProp(
                propSrcUniqName, strVal );

            if( ERROR( ret ) )
            {
                ++itr;
                continue;
            }

            if( strUniqName == strVal )
            {
                vecMatches.push_back( itr->first );
                itr = plm->erase( itr );
            }
            else
            {
                ++itr;
            }
        }

    }while( 0 );

    return vecMatches.size();
}

gint32 CRpcRouter::AddRemoveMatch(
    IMessageMatch* pMatch,
    bool bAdd, bool bRemote,
    IEventSink* pCallback )
{
    if( pMatch == nullptr )
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
                ret = EEXIST;
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
                else
                {
                    ret = EEXIST;
                }
            }
        }

    }while( 0 );

    return ret;
}

gint32 CRpcRouter::RemoveRemoteMatchByPortId(
    guint32 dwPortId,
    std::vector< MatchPtr >& vecMatches )
{
    if( dwPortId == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        map< MatchPtr, gint32 >* plm =
            &m_mapRmtMatches;

        CStdRMutex oRouterLock( GetLock() );
        map< MatchPtr, gint32 >::iterator
            itr = plm->begin();
        while( itr != plm->end() )
        {
            CCfgOpenerObj oMatch(
                ( CObjBase* )itr->first );

            guint32 dwVal = 0;
            ret = oMatch.GetIntProp(
                propPortId, dwVal );
            if( ERROR( ret ) )
            {
                ++itr;
                continue;
            }

            if( dwPortId == dwVal )
            {
                vecMatches.push_back(
                    itr->first );
                itr = plm->erase( itr );
            }
            else
            {
                ++itr;
            }
        }

    }while( 0 );

    return vecMatches.size();
}

static guint32 dwAddCount = 0;
static guint32 dwRemoveCount = 0;

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

        CRpcRouter* pRouter = nullptr;
        ret = oTaskCfg.GetPointer(
            propRouterPtr, pRouter );
        if( ERROR( ret ) )
            break;

        CRpcReqForwarderProxy* pProxy = nullptr;

        ret = oTaskCfg.GetPointer(
            propIfPtr, pProxy );

        if( ERROR( ret ) && !bEnable )
        {
            break;
        }
        else if( ERROR( ret ) && bEnable )
        {
            InterfPtr pIf;
            ret = pRouter->GetReqFwdrProxy(
                pMatch, pIf );
            if( ERROR( ret ) )
                break;

            pProxy = pIf;
        }

        // enable or disable event
        oParams.Push( bEnable );

        oParams[ propIfPtr ] = ObjPtr( pProxy );
        oParams[ propMatchPtr ] = ObjPtr( pMatch );
        oParams[ propEventSink ] = ObjPtr( this );
        oParams[ propNotifyClient ] = true;

        TaskletPtr pEnableEvtTask;
        ret = pEnableEvtTask.NewObj(
            clsid( CIfEnableEventTask ),
            oParams.GetCfg() );

        ret = ( *pEnableEvtTask )( eventZero );
        if( !ERROR( ret ) )
        {
            bEnable ? dwAddCount++ : dwRemoveCount++;
        }

    }while( 0 );

    if( ret == STATUS_PENDING )
        return ret;

    if( Retriable( ret ) )
        return STATUS_MORE_PROCESS_NEEDED;

    ret = OnTaskComplete( ret );

    return ret;
}

gint32 CRouterEnableEventRelayTask::OnTaskComplete(
    gint32 iRetVal )
{
    gint32 ret = 0;
    CParamList oParams(
        ( IConfigDb* )GetConfig() );

    do{
        bool bEnable = false;

        ret = oParams.GetBoolProp( 0, bEnable );
        if( ERROR( ret ) )
            break;

        if( ERROR( iRetVal ) )
            break;

        if( !bEnable )
            break;

        IMessageMatch* pMatch = nullptr;
        ret = oParams.GetPointer( 1, pMatch );
        if( ERROR( ret ) )
            break;

        CRpcRouter* pRouter = nullptr;
        ret = oParams.GetPointer(
            propRouterPtr, pRouter );
        if( ERROR( ret ) )
            break;

        CRpcReqForwarderProxy* pProxy = nullptr;
        ret = oParams.GetPointer(
            propIfPtr, pProxy );

        if( ERROR( ret ) )
        {
            InterfPtr pIf;
            ret = pRouter->GetReqFwdrProxy(
                pMatch, pIf );
            if( ERROR( ret ) )
                break;
            pProxy = pIf;
        }

        // in case somewhere the transaction failed,
        // add a rollback task
        TaskletPtr pRbTask;
        ret = pRouter->BuildEventRelayTask(
            pMatch, false, pRbTask );

        if( SUCCEEDED( ret ) )
            AddRollbackTask( pRbTask );

    }while( 0 );

    oParams.ClearParams();
    oParams.RemoveProperty( propIfPtr );
    oParams.RemoveProperty( propRouterPtr );
    oParams.RemoveProperty( propMatchPtr );

    return iRetVal;
}


gint32 CRouterAddRemoteMatchTask::AddRemoteMatchInternal(
    CRpcRouter* pRouter,
    IMessageMatch* pMatch,
    bool bEnable )
{
    gint32 ret = 0;
    do{
        if( pRouter == nullptr ||
            pMatch == nullptr )
            return false;

        if( bEnable )
        {
            ret = pRouter->AddRemoveMatch(
                pMatch, true, true, nullptr );

            if( ret == EEXIST )
            {
                ret = 0;
                InterfPtr pIf;
                ret = pRouter->GetReqFwdrProxy(
                    pMatch, pIf );

                if( ERROR( ret ) )
                    break;

                if( pIf->GetState() == stateRecovery )
                    ret = ENOTCONN;
                // stop further actions in this
                // transaction
                ChangeRelation( logicOR );
                break;
            }
            else if( ERROR( ret ) )
                break;

            // in case the rest transaction failed
            // add a rollback task
            TaskletPtr pRbTask;
            ret = pRouter->BuildAddMatchTask(
                pMatch, false, pRbTask );

            if( SUCCEEDED( ret ) )
                AddRollbackTask( pRbTask );
        }
        else
        {
            // a DisableEvent request
            ret = pRouter->AddRemoveMatch(
                pMatch, false, true, nullptr );

            if( ret == EEXIST )
            {
                ret = 0;
                ChangeRelation( logicOR );
                break;
            }
        }

    }while( 0 );

    return ret;
}

// this task as a precondition of the following
// tasks
gint32 CRouterAddRemoteMatchTask::RunTask()
{
    gint32 ret = 0;
    do{
        CParamList oParams( m_pCtx );

        CRpcRouter* pRouter = nullptr;
        ret = oParams.GetPointer(
            propIfPtr, pRouter );

        if( ERROR( ret ) )
            break;

        bool bEnable = false;
        IMessageMatch* pMatch;

        ret = oParams.GetBoolProp( 0, bEnable );
        if( ERROR( ret ) )
            break;

        ret = oParams.GetPointer( 1, pMatch );
        if( ERROR( ret ) )
            break;

        ret = AddRemoteMatchInternal(
            pRouter, pMatch, bEnable );

    }while( 0 );

    return ret;
}

gint32 CRpcRouter::BuildAddMatchTask(
    IMessageMatch* pMatch,
    bool bEnable,
    TaskletPtr& pTask )
{
    if( pMatch == nullptr )
        return -EINVAL;

    gint32 ret = 0;

    do{
        // NOTE: we use the original match for
        // dbus listening
        CParamList oParams;

        oParams.Push( bEnable );
        oParams.Push( ObjPtr( pMatch ) );

        ret = oParams.SetObjPtr(
            propIfPtr, ObjPtr( this ) );

        if( ERROR( ret ) )
            break;

        oParams.SetObjPtr(
            propIoMgr, GetIoMgr() );

        ret = pTask.NewObj(
            clsid( CRouterAddRemoteMatchTask ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

    }while( 0 );

    return ret;
}

gint32 CRpcRouter::BuildStartRecvTask(
    IMessageMatch* pMatch,
    TaskletPtr& pTask )
{
    gint32 ret = 0;
    if( pMatch == nullptr )
        return -EINVAL;

    do{
        CParamList oParams;

        ret = oParams.SetPointer(
            propIoMgr, GetIoMgr() );

        InterfPtr pIf;
        ret = GetReqFwdrProxy( pMatch, pIf );
        if( SUCCEEDED( ret ) )
        {
            ret = oParams.SetObjPtr(
                propIfPtr, pIf );
        }

        ret = oParams.SetPointer(
            propRouterPtr, this );

        ret = oParams.SetPointer(
            propMatchPtr, pMatch );

        ret = pTask.NewObj(
            clsid( CRouterStartRecvTask ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

    }while( 0 );

    return ret;
}

gint32 CRouterStartRecvTask::RunTask()
{
    gint32 ret = 0;
    CParamList oParams(
        ( IConfigDb* )GetConfig() );

    do{
        IMessageMatch* pMatch = nullptr;
        ret = oParams.GetPointer(
            propMatchPtr, pMatch );
        if( ERROR( ret ) )
            break;
    
        CRpcReqForwarderProxy* pProxy = nullptr;
        ret = oParams.GetPointer(
            propIfPtr, pProxy );
        if( ERROR( ret ) )
        {
            CRpcRouter* pRouter = nullptr;
            ret = oParams.GetPointer(
                propRouterPtr, pRouter );
            if( ERROR( ret ) )
                break;

            InterfPtr pIf;
            ret = pRouter->GetReqFwdrProxy(
                pMatch, pIf );
            if( ERROR( ret ) )
                break;

            pProxy = pIf;
        }

        CParamList oRecvParams;
        oRecvParams[ propMatchPtr ] =
            ObjPtr( pMatch );

        oRecvParams[ propIfPtr ] =
            ObjPtr( pProxy );

        TaskletPtr pRecvTask;
        pRecvTask.NewObj(
            clsid( CIfStartRecvMsgTask ),
            oRecvParams.GetCfg() );

        if( pProxy->GetState() == stateRecovery )
        {
            // server is not up
            ret = ENOTCONN;
            break;
        }
        // transfer the control to the proxy
        ret = pProxy->AddAndRun( pRecvTask );
        if( ERROR( ret ) )
            break;

        ret = pRecvTask->GetError();
        if( ret == STATUS_PENDING )
        {
            ret = 0;
            break;
        }
        if( ret == -ENOTCONN )
        {
            // server is not up
            ret = ENOTCONN;
            break;
        }

    }while( 0 );

    oParams.ClearParams();
    oParams.RemoveProperty( propIfPtr );
    oParams.RemoveProperty( propRouterPtr );
    oParams.RemoveProperty( propMatchPtr );

    return ret;
}

gint32 CRouterEventRelayRespTask::OnTaskComplete(
    gint32 iRetVal )
{
    gint32 ret = 0;
    CParamList oResp;
    oResp[ propReturnValue ] = iRetVal;

    CParamList oParams(
        ( IConfigDb* )GetConfig() );

    CRpcRouter* pRouter = nullptr;
    ret = oParams.GetPointer(
        propIfPtr, pRouter );

    if( ERROR( ret ) )
        return ret;

    EventPtr pTask;
    ret = GetInterceptTask( pTask );
    if( ERROR( ret ) )
        return 0;

    return pRouter->SetResponse(
        pTask, oResp.GetCfg() );
}

gint32 CRpcRouter::RunEnableEventTask(
    IEventSink* pCallback,
    IMessageMatch* pMatch )
{
    gint32 ret = 0;
    if( pCallback == nullptr ||
        pMatch == nullptr )
        return -EINVAL;

    TaskletPtr pRespTask;
    TaskGrpPtr pTransGrp;

    do{
        CParamList oParams;
        oParams[ propIfPtr ] = ObjPtr( this );

        oParams[ propEventSink ] =
            ObjPtr( pCallback );

        ret = pRespTask.NewObj(
            clsid( CRouterEventRelayRespTask ),
            oParams.GetCfg() );

        CRouterEventRelayRespTask* prerrt =
            ( CRouterEventRelayRespTask* )pRespTask;
        prerrt->SetEnable( true );   

        // run the task to set it to a proper
        // state
        ( *pRespTask )( eventZero );

        oParams[ propEventSink ] =
            ObjPtr( pRespTask );
        oParams[ propNotifyClient ] = true;

        ret = pTransGrp.NewObj(
            clsid( CIfTransactGroup ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        TaskletPtr pAddMatchTask;
        ret = BuildAddMatchTask(
            pMatch, true, pAddMatchTask );
        if( ERROR( ret ) )
            break;

        TaskletPtr pStartProxyTask;
        ret = BuildStartStopReqFwdrProxy(
            pMatch, true, pStartProxyTask );
        if( ERROR( ret ) && ret != -EEXIST )
            break;

        TaskletPtr pEnableTask;
        ret = BuildEventRelayTask(
            pMatch, true, pEnableTask );
        if( ERROR( ret ) )
            break;

        TaskletPtr pRecvTask;
        ret = BuildStartRecvTask(
            pMatch, pRecvTask );

        if( ERROR( ret ) )
            break;

        pTransGrp->AppendTask( pAddMatchTask );
        pTransGrp->AppendTask( pStartProxyTask );
        pTransGrp->AppendTask( pEnableTask );
        pTransGrp->AppendTask( pRecvTask );

        TaskletPtr pGrpTask = pTransGrp;
        ret = AddSeqTask( pGrpTask, false );
        if( SUCCEEDED( ret ) )
            ret = pTransGrp->GetError();
        
    }while( 0 );

    if( ERROR( ret ) )
    {
        if( !pRespTask.IsEmpty() )
        {
            CIfInterceptTaskProxy* pIcpt = pRespTask;
            if( pIcpt != nullptr )
                pIcpt->SetInterceptTask( nullptr );
        }
        if( !pTransGrp.IsEmpty() )
        {
            ( *pTransGrp )( eventCancelTask );
        }
    }

    return ret;
}

gint32 CRpcRouter::BuildDisEvtTaskGrp(
    IEventSink* pCallback,
    IMessageMatch* pMatch,
    TaskletPtr& pTask )
{
    gint32 ret = 0;
    if( pMatch == nullptr )
        return -EINVAL;

    TaskGrpPtr pTaskGrp;
    do{
        TaskletPtr pRespTask;
        CParamList oParams;
        oParams[ propIfPtr ] = ObjPtr( this );

        if( pCallback != nullptr )
        {
            oParams[ propEventSink ] =
                ObjPtr( pCallback );

            ret = pRespTask.NewObj(
                clsid( CRouterEventRelayRespTask ),
                oParams.GetCfg() );

            if( ERROR( ret ) )
                break;
            CRouterEventRelayRespTask* prerrt =
                ( CRouterEventRelayRespTask* )pRespTask;
            prerrt->SetEnable( false );   

            // run the task to set it to a proper
            // state
            ( *pRespTask )( eventZero );

            oParams[ propNotifyClient ] = true;
            oParams[ propEventSink ] =
                ObjPtr( pRespTask );
        }

        ret = pTaskGrp.NewObj(
            clsid( CIfTransactGroup ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        TaskletPtr pAddMatchTask;
        ret = BuildAddMatchTask(
            pMatch, false, pAddMatchTask );
        if( ERROR( ret ) )
            break;

        TaskletPtr pStopProxyTask;
        ret = BuildStartStopReqFwdrProxy(
            pMatch, false, pStopProxyTask );
        if( ERROR( ret ) )
            break;

        TaskletPtr pDisableTask;
        ret = BuildEventRelayTask(
            pMatch, false, pDisableTask );
        if( ERROR( ret ) )
            break;

        pTaskGrp->SetRelation( logicNONE );
        pTaskGrp->AppendTask( pAddMatchTask );
        pTaskGrp->AppendTask( pDisableTask );
        pTaskGrp->AppendTask( pStopProxyTask );

        pTask = pTaskGrp;
        if( pTask.IsEmpty() )
            ret = -EFAULT;

    }while( 0 );

    if( pTask.IsEmpty() || ERROR( ret ) )
    {
        // free the resources the pTaskGrp claimed
        if( !pTaskGrp.IsEmpty() )
            ( *pTaskGrp )( eventCancelTask );

    }

    return ret;
}

gint32 CRpcRouter::RunDisableEventTask(
    IEventSink* pCallback,
    IMessageMatch* pMatch )
{
    gint32 ret = 0;
    if( pCallback == nullptr ||
        pMatch == nullptr )
        return -EINVAL;

    do{
        TaskletPtr pTask;

        ret = BuildDisEvtTaskGrp(
            pCallback, pMatch, pTask );

        if( ERROR( ret ) )
            break;

        ret = AddSeqTask( pTask, false );
        if( SUCCEEDED( ret ) )
            ret = pTask->GetError();
        
    }while( 0 );

    return ret;
}

gint32 CRpcRouter::ForwardModOnOfflineEvent(
    EnumEventId iEvent,
    const std::string& strModule )
{
    if( !HasBridge() )
        return 0;

    gint32 ret = 0;
    do{
        bool bOffline = false;
        if( iEvent == eventModOffline )
            bOffline = true;
        else if( iEvent != eventModOnline )
        {
            ret = -EINVAL;
            break;
        }

        // find all the interested bridges
        std::map< MatchPtr, gint32 >* plm =
            &m_mapRmtMatches;

        std::set< guint32 > setPortIds;
        CStdRMutex oRouterLock( GetLock() );
        for( auto elem : *plm )
        {
            IMessageMatch* pMatch = elem.first;
            if( unlikely( pMatch == nullptr ) )
                continue;

            CCfgOpenerObj oMatchCfg( pMatch );
            string strDest;
            ret = oMatchCfg.GetStrProp(
                propDestDBusName, strDest );
            if( ERROR( ret ) )
                continue;

            if( strModule == strDest )
            {
                guint32 dwPortId = 0;
                ret = oMatchCfg.GetIntProp(
                    propPortId, dwPortId );
                if( ERROR( ret ) )
                    continue;

                setPortIds.insert( dwPortId );
            }
        }
        oRouterLock.Unlock();

        if( setPortIds.empty() )
            break;

        DMsgPtr pMsg;
        ret = pMsg.NewObj(
            ( EnumClsid )DBUS_MESSAGE_TYPE_SIGNAL );
        if( ERROR( ret ) )
            break;

        ret = pMsg.SetPath(
            DBUS_PATH_DBUS );
        if( ERROR( ret ) )
            break;

        ret = pMsg.SetInterface(
            DBUS_INTERFACE_DBUS );
        if( ERROR( ret ) )
            break;

        ret = pMsg.SetMember(
            "NameOwnerChanged" );
        if( ERROR( ret ) )
            break;

        string strModName =
            GetIoMgr()->GetModName();

        ret = pMsg.SetSender(
            DBUS_DESTINATION( strModName ) );

        pMsg.SetSerial( iEvent );

        const char* szUnknown = "unknown";
        const char* szEmpty = "";
        const char* szModule = strModule.c_str();
        if( bOffline )
        {
            if( !dbus_message_append_args( pMsg,
                DBUS_TYPE_STRING, &szModule,
                DBUS_TYPE_STRING, &szUnknown,
                DBUS_TYPE_STRING, &szEmpty,
                DBUS_TYPE_INVALID ) )
            {
                ret = -ENOMEM;
                break;
            }
        }
        else
        {
            if( !dbus_message_append_args( pMsg,
                DBUS_TYPE_STRING, &szModule,
                DBUS_TYPE_STRING, &szEmpty,
                DBUS_TYPE_STRING, &szUnknown,
                DBUS_TYPE_INVALID ) )
            {
                ret = -ENOMEM;
                break;
            }
        }

        TaskletPtr pDummyTask;
        ret = pDummyTask.NewObj(
            clsid( CIfDummyTask ) );
        if( ERROR( ret ) )
            break;

        // forward the event
        for( auto elem : setPortIds )
        {
            InterfPtr pIf;
            ret = GetBridge( elem, pIf );
            if( ERROR( ret ) )
                continue;
            CRpcTcpBridge* pBridge = pIf;
            if( unlikely( pBridge == nullptr ) )
                continue;

            std::string strIpAddr;
            CCfgOpenerObj oIfCfg( pBridge );
            ret = oIfCfg.GetStrProp(
                propIpAddr, strIpAddr );

            if( ERROR( ret ) )
                continue;

            pBridge->ForwardEvent(
                strIpAddr, pMsg, pDummyTask );

        }

    }while( 0 );

    return ret;
}

gint32 CRpcRouter::ForwardDBusEvent(
    EnumEventId iEvent )
{
    gint32 ret = 0;
    DMsgPtr pMsg;

    if( !HasBridge() )
        return 0;

    std::string strMethod;
    if( iEvent == eventRmtDBusOnline )
        strMethod = "Connected";
    else
        strMethod = "Disconnected";
    do{
        ret = pMsg.NewObj(
            ( EnumClsid )DBUS_MESSAGE_TYPE_SIGNAL );
        if( ERROR( ret ) )
            break;

        ret = pMsg.SetPath(
            DBUS_SYS_OBJPATH );
        if( ERROR( ret ) )
            break;

        ret = pMsg.SetInterface(
            DBUS_SYS_INTERFACE );
        if( ERROR( ret ) )
            break;

        ret = pMsg.SetMember(
            strMethod.c_str() );
        if( ERROR( ret ) )
            break;

        // notify through all the tcp bridges
        std::vector< InterfPtr > vecBridges;
        CStdRMutex oRouterLock( GetLock() );
        std::map< guint32, InterfPtr >* pMap =
            &m_mapPortId2Bdge;
        for( auto elem : *pMap )
            vecBridges.push_back( elem.second );
        oRouterLock.Unlock();

        TaskletPtr pDummyTask;
        ret = pDummyTask.NewObj(
            clsid( CIfDummyTask ) );
        if( ERROR( ret ) )
            break;

        for( auto elem : vecBridges )
        {
            CRpcTcpBridge* pBridge = elem;
            if( unlikely( pBridge == nullptr ) )
                continue;

            string strIpAddr;
            CCfgOpenerObj oIfCfg( pBridge );
            ret = oIfCfg.GetStrProp(
                propIpAddr, strIpAddr );
            if( ERROR( ret ) )
                continue;

            // broadcast to all the connected
            // clients
            pBridge->ForwardEvent(
                strIpAddr, pMsg, pDummyTask );
        }

    }while( 0 );

    return ret;
}

CIfRouterState::CIfRouterState(
    const IConfigDb* pCfg )
    : super( pCfg )
{
    SetClassId( clsid( CIfRouterState ) );
}

gint32 CIfRouterState::SubscribeEvents()
{
    vector< EnumPropId > vecEvtToSubscribe = {
        propStartStopEvent ,
        propDBusModEvent,
        propDBusSysEvent,
        propRmtSvrEvent,
        propAdminEvent,
    };
    return SubscribeEventsInternal(
        vecEvtToSubscribe );
}

gint32 CRpcRouter::RebuildMatches()
{
    gint32 ret = super::RebuildMatches();
    if( ERROR( ret ) )
        return ret;

    do{
        CCfgOpenerObj oCfg( this );
        guint32 dwRole = 0;
        ret = oCfg.GetIntProp(
            propRouterRole, dwRole );
        if( ERROR( ret ) )
            break;
        
        std::string strSurffix;
        if( dwRole == 1 )
            strSurffix = "_1";
        else if( dwRole == 2 )
            strSurffix = "_2";

        std::string strObjPath;

        ret = oCfg.GetStrProp(
            propObjPath, strObjPath );
        if( ERROR( ret ) )
            break;

        strObjPath += strSurffix;
        string strDest;
        for( auto pMatch : m_vecMatches )
        {
            CCfgOpenerObj oMatch(
                ( CObjBase* )pMatch );

            oMatch.SetStrProp(
                propObjPath, strObjPath );

            ret = oMatch.GetStrProp(
                propSrcDBusName, strDest );
            if( SUCCEEDED( ret ) )
            {
                strDest += strSurffix;
                oMatch.SetStrProp(
                    propSrcDBusName, strDest );
            }
            else
            {
                ret = 0;
            }
        }

    }while( 0 );

    return ret;
}
