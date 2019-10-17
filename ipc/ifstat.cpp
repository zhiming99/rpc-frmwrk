/*
 * =====================================================================================
 *
 *       Filename:  ifstat.cpp
 *
 *    Description:  implementation of the interface state classes
 *
 *        Version:  1.0
 *        Created:  01/23/2017 10:09:32 PM
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
#include "defines.h"
#include "frmwrk.h"
#include "emaphelp.h"
#include "connhelp.h"
#include "proxy.h"

using namespace std;

#define STATEMAP_KEY std::pair< EnumIfState, EnumEventId >

#define STATEMAP_ENTRY( iState, iEvent, iNewState ) \
  { STATEMAP_KEY( iState, iEvent ), iNewState }

std::map< STATEMAP_KEY, EnumIfState > CInterfaceState::m_mapState =
{
    STATEMAP_ENTRY( stateStopped,   cmdOpenPort,            stateStarting ),
    STATEMAP_ENTRY( stateStarting,  eventPortStarted,       stateConnected ),
    STATEMAP_ENTRY( stateStarting,  eventPortStartFailed,   stateStartFailed ),
    STATEMAP_ENTRY( stateStarting,  eventPortStopping,      stateStopped ),
    STATEMAP_ENTRY( stateStarting,  eventDBusOffline,       stateStopped ),
    STATEMAP_ENTRY( stateStarting,  eventRmtSvrOffline,     stateStopped ),
    STATEMAP_ENTRY( stateStarting,  cmdShutdown,            stateStopped ),
    STATEMAP_ENTRY( stateConnected, eventPaused,            statePaused ),
    STATEMAP_ENTRY( stateConnected, eventPortStopping,      stateStopping ),
    STATEMAP_ENTRY( stateConnected, eventDBusOffline,       stateStopping ),
    STATEMAP_ENTRY( stateConnected, eventRmtSvrOffline,     stateStopping ),
    STATEMAP_ENTRY( stateConnected, cmdShutdown,            stateStopping ),
    STATEMAP_ENTRY( stateConnected, eventModOffline,        stateRecovery ),
    STATEMAP_ENTRY( stateConnected, eventRmtModOffline,     stateRecovery ),
    STATEMAP_ENTRY( stateConnected, cmdPause,               statePausing ),
    STATEMAP_ENTRY( stateRecovery,  eventPortStopping,      stateStopping ),
    STATEMAP_ENTRY( stateRecovery,  eventModOffline,        stateRecovery ),
    STATEMAP_ENTRY( stateRecovery,  cmdEnableEvent,         stateRecovery ),
    STATEMAP_ENTRY( stateRecovery,  eventModOnline,         stateUnknown ),
    STATEMAP_ENTRY( stateRecovery,  eventRmtModOnline,      stateUnknown ),
    STATEMAP_ENTRY( stateRecovery,  eventDBusOffline,       stateStopping ),
    STATEMAP_ENTRY( stateRecovery,  eventRmtSvrOffline,     stateStopping ),
    STATEMAP_ENTRY( stateRecovery,  cmdShutdown,            stateStopping ),
    STATEMAP_ENTRY( statePaused,    eventPortStopping,      stateStopping ),
    STATEMAP_ENTRY( statePaused,    eventModOffline,        stateRecovery ),
    STATEMAP_ENTRY( statePaused,    eventDBusOffline,       stateStopping ),
    STATEMAP_ENTRY( statePaused,    eventRmtModOffline,     stateRecovery ),
    STATEMAP_ENTRY( statePaused,    eventRmtSvrOffline,     stateStopped ),
    STATEMAP_ENTRY( statePaused,    cmdShutdown,            stateStopping ),
    STATEMAP_ENTRY( statePaused,    cmdResume,              stateResuming ),
    STATEMAP_ENTRY( statePausing,   eventPaused,            statePaused ),
    STATEMAP_ENTRY( stateStopping,  cmdClosePort,           stateStopped ),
    STATEMAP_ENTRY( stateStartFailed, cmdCleanup,           stateStopping ),
    STATEMAP_ENTRY( stateStartFailed, eventRetry,           stateStarting ),
    STATEMAP_ENTRY( stateResuming,  eventResumed,           stateConnected ),

    // unknown means the value depends on the m_iResumeState
};

CRpcInterfaceBase* CInterfaceState::GetInterface()
{
    return static_cast< CRpcInterfaceBase* >( m_pInterface );
}

CInterfaceState::CInterfaceState(
    const IConfigDb* pCfg )
{
    gint32 ret = 0;

    do{
        if( pCfg == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        ret = m_pIfCfgDb.NewObj();
        if( ERROR( ret ) )
            break;

        *m_pIfCfgDb = *pCfg;

        CCfgOpener oCfg( pCfg );
        ret = oCfg.GetPointer( propIoMgr, m_pIoMgr );
        if( ERROR( ret ) )
            break;

        ObjPtr pObj;
        ret = oCfg.GetObjPtr(
            propIfPtr, pObj );

        if( ERROR( ret ) )
            break;

        m_pInterface = pObj;
        if( m_pInterface == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        m_pIfCfgDb->RemoveProperty( propIoMgr );
        m_pIfCfgDb->RemoveProperty( propIfPtr );

        // mandatory properties
        //
        // propBusName
        // propPortClass
        // propPortId
        // 
        // propIfName
        // propObjPath
        // propDestDBusName
        // propSrcDBusName
        //
        // optional properties
        // propIpAddr if subclass is CRemoteProxyState

        m_hPort = 0;
        SetStateInternal( stateStopped );
        m_iResumeState = stateInvalid;

    }while( 0 );

    if( ERROR( ret ) )
    {
        string strMsg = DebugMsg( ret, 
            "fatal error, error in CInterface ctor " );
        throw runtime_error( strMsg );
    }
}

CInterfaceState::~CInterfaceState()
{
    ClosePort();
}

gint32 CInterfaceState::SubscribeEventsInternal(
    const std::vector< EnumPropId >& vecEvtToSubscribe )
{
    gint32 ret = 0;

    UnsubscribeEvents();

    CConnPointHelper oCpHelper( GetIoMgr() );
    CRpcInterfaceBase* pIf = GetInterface();
    EventPtr pEvent( pIf );

    vector< EnumPropId > vecEvents;

    for( auto elem : vecEvtToSubscribe )
    {
        ret = oCpHelper.SubscribeEvent(
            elem, pEvent );

        if( ERROR( ret ) )
            break;

        vecEvents.push_back( elem );
    }

    CLEAR_SUBSCRIPTION_ON_ERROR();

    return ret;
}

gint32 CInterfaceState::TestSetState(
    EnumEventId iEvent )
{
    CStdRMutex oStateLock( GetLock() );
    STATEMAP_KEY oKey( GetState(), iEvent );
    std::map< STATMAP_KEY, EnumIfState >::iterator
        itr = m_mapState.find( oKey ); 

    if( itr == m_mapState.end() )
        return ERROR_STATE;

    return 0;
}

gint32 CInterfaceState::SetStateOnEvent(
    EnumEventId iEvent )
{
    // state switch table
    gint32 ret = 0;
    CStdRMutex oStateLock( GetLock() );

    STATEMAP_KEY oKey( GetState(), iEvent );
    std::map< STATMAP_KEY, EnumIfState >::iterator
        itr = m_mapState.find( oKey ); 

    if( itr != m_mapState.end() )
    {
        switch( itr->second )
        {
        case stateUnknown:
            {
                SetStateInternal( m_iResumeState );
                ret = 0;
                break;
            }
        case stateRecovery:
            {
                if( GetState() != stateRecovery )
                    m_iResumeState = GetState();
                else
                    break;

                // fall through
            }
        default:
            {
                SetStateInternal( m_mapState[ oKey ] );
                ret = 0;
                break;
            }
        }
    }
    else
    {
        ret = ERROR_STATE;
    }
    return ret;
}

bool CInterfaceState::IsMyPort(
    HANDLE hPort )
{
    if( hPort == 0 )
        return false;

    if( m_hPort == hPort )
        return true;

    if( m_hPort != 0 )
        return false;

    // we are waiting for such a port
    gint32 ret = 0;
    CIoManager* pMgr = GetIoMgr();

    do{
        // check if the port is what we
        // are waiting for
        BufPtr pBuf( true );

        ret = pMgr->GetPortProp(
            hPort, propPortClass, pBuf );

        if( ERROR( ret ) )
            break;

        string strPortClass = *pBuf;
        string strDesiredClass;

        CCfgOpenerObj oCfg( this );

        ret = oCfg.GetStrProp(
            propPortClass, strDesiredClass );

        if( ERROR( ret ) )
            break;

        if( strPortClass != strDesiredClass )
        {
            ret = ERROR_FALSE;
            break;
        }

    }while( 0 );

    if( ERROR( ret ) )
        return false;

    return true;
}

gint32 CInterfaceState::OnPortEvent(
        EnumEventId iEvent,
        HANDLE hPort )
{
    gint32 ret = 0;

    do{
        if( hPort == 0 ||
            iEvent == eventInvalid )
        {
            ret = -EINVAL;
            break;
        }

        CIoManager* pMgr = GetIoMgr();

        switch( iEvent )
        {
        case eventPortStartFailed:
            {
                ret = SetStateOnEvent(
                    eventPortStartFailed );
                break;
            }
        case eventPortStarted:
            {
                CRpcInterfaceBase* pIf =
                    GetInterface();

                //BUGBUG: the lock relies on the
                //implementation of OpenPortByRef
                CStdRMutex oStateLock( GetLock() );
                ret = TestSetState( eventPortStarted );
                if( ERROR( ret ) )
                    break;

                ret = pMgr->OpenPortByRef(
                    hPort, pIf );

                if( ERROR( ret ) )
                {
                    ret = SetStateOnEvent(
                        eventPortStartFailed );
                    break;
                }

                m_hPort = hPort;
                ret = SetStateOnEvent(
                    eventPortStarted );

                if( ERROR( ret ) )
                    break;

                ret = SubscribeEvents();
                break;
            }
        case eventPortStopping:
            {
                if( !IsMyPort( hPort ) )
                {
                    // not our port
                    ret = -ENOENT;
                    break;
                }

                EnumIfState iState = GetState();
                switch( iState )
                {
                case stateConnected:
                case stateRecovery:
                case statePaused:
                case stateStarting:
                    {
                        ClosePort();
                        break;
                    }
                case stateStopped:
                default:
                    {
                        break;
                    }
                }

                break;
            }
        case eventPortStopped:
            {
                m_hPort = 0;
                UnsubscribeEvents();
                ret = SetStateOnEvent( cmdClosePort );
                break;
            }
        default:
            {
                break;
            }
        }

    }while( 0 );

    return ret;
}
  
gint32 CInterfaceState::OnModEvent(
    EnumEventId iEvent,
    const string& strModule )
{
    gint32 ret = 0;

    do{
        if( iEvent >= eventInvalid
            || strModule.empty() )
        {
            ret = -EINVAL;
            break;
        }

        switch( iEvent )
        {
        case eventRmtModOffline:
        case eventModOffline:
        case eventRmtModOnline:
        case eventModOnline:
            {
                SetStateOnEvent( iEvent );
                break;
            }
        default:
            break;
        }

    }while( 0 );

    return ret;
}

gint32 CInterfaceState::OnDBusEvent(
    EnumEventId iEvent )
{
    gint32 ret = 0;

    do{
        if( iEvent >= eventInvalid )
        {
            ret = -EINVAL;
            break;
        }

        switch( iEvent )
        {
        case eventRmtSvrOffline:
        case eventDBusOffline:
            {
                switch( GetState() )
                {
                case stateStopping:
                    {
                        ClosePort();
                        break;
                    }
                case stateStopped:
                default:
                    break;
                }

                break;
            }
        case eventRmtSvrOnline:
        case eventDBusOnline:
            {
                break;
            }
        default:
            break;

        }

    }while( 0 );

    return ret;
}

gint32 CInterfaceState::OnAdminEvent(
    EnumEventId iEvent,
    const std::string& strParams )
{
    gint32 ret = 0;

    do{

        if( iEvent >= eventInvalid )
        {
            ret = -EINVAL;
            break;
        }

        switch( iEvent )
        {
        case cmdShutdown:
            {
                switch( GetState() )
                {
                case stateStopping:
                    {
                        ClosePort();
                        break;
                    }
                default:
                    {
                        ret = ERROR_STATE;
                        break;
                    }
                }

                break;
            }
        case cmdPause:
            {
                ret = SetStateOnEvent(
                    eventPaused );
                break;
            }
        case cmdResume:
            {
                ret = SetStateOnEvent(
                    eventResumed );
                break;
            }
        default:
            break;
        }

    }while( 0 );

    return ret;
}

gint32 CInterfaceState::OnEvent(
    EnumEventId iEvent,
    LONGWORD dwParam1,
    LONGWORD dwParam2,
    LONGWORD* pData )
{
    return 0;
}

gint32 CInterfaceState::OpenPortInternal(
    IEventSink* pCallback, bool bRetry )
{
    gint32 ret = 0;

    if( pCallback == nullptr )
        return -EINVAL;

    do{
        if( true )
        {
            CStdRMutex oStateLock( GetLock() );
            if( GetState() != stateStarting )
            {
                ret = ERROR_STATE;
                break;
            }
        }

        CReqBuilder oParams( GetInterface() );
        // let's prepare the parameters
        ret = oParams.CopyProp(
            propBusName, this );

        if( ERROR( ret ) )
            break;

        ret = oParams.CopyProp(
            propPortClass, this );

        if( ERROR( ret ) )
            break;

        ret = oParams.CopyProp(
            propPortId, this );

        if( ERROR( ret ) )
            break;

        ret = oParams.SetObjPtr(
            propEventSink, ObjPtr( pCallback ) );

        if( ERROR( ret ) )
            break;

        // if the port is created, we will use this ptr
        // to register to the Handle map
        oParams[ propIfPtr ] =
            ObjPtr( GetInterface() );

        // give subclass a chance to customize the
        // parameters
        ret = SetupOpenPortParams(
            oParams.GetCfg() );
        if( ERROR( ret ) )
            break;

        ret = GetIoMgr()->OpenPort(
            oParams.GetCfg(), m_hPort );

        if( ret == STATUS_PENDING )
        {
            // let's wait the port to be created
            // in the event handler OnPortEvent
            break;
        }

        if( SUCCEEDED( ret ) )
        {
            ret = SetStateOnEvent(
                eventPortStarted );

            if( ERROR( ret ) )
                return ret;

            ret = SubscribeEvents();
            break;
        }

        if( ERROR( ret ) )
        {
            // return to stopped state
            // retry will happen from outside
            // rollback the state
            SetStateOnEvent(
                eventPortStartFailed );
            break;
        }
        // unrecoverable error
 
    }while( 0 );

    return ret;
}

gint32 CInterfaceState::ClosePort(
    IEventSink* pCallback )
{
    if( m_hPort == 0 )
        return -EINVAL;

    // cut off the feedback the underlying
    // port could emit on port unloading
    UnsubscribeEvents();
    return GetIoMgr()->ClosePort(
        m_hPort, GetInterface(), pCallback );
}

gint32 CInterfaceState::UnsubscribeEvents()
{
    gint32 ret = 0;

    CConnPointHelper oCpHelper( GetIoMgr() );
    vector< EnumPropId > vecEvents;

    if( true )
    {
        CStdRMutex oStateLock( GetLock() );
        vecEvents = m_vecTopicList;
        m_vecTopicList.clear();
    }

    // when there is no subscription, the
    // interface may not be valid, so better stop
    // here.
    if( vecEvents.empty() )
        return ret;

    CRpcInterfaceBase* pIf = GetInterface();
    EventPtr pEvent( pIf );

    for( auto elem : vecEvents )
    {
        oCpHelper.UnsubscribeEvent(
            elem, pEvent );
    }

    return ret;
}

gint32 CInterfaceState::Start(
    IEventSink* pCallback )
{
    gint32 ret = OpenPort( pCallback );
    if( ret == STATUS_PENDING )
        return ret;

    if( ERROR( ret ) )
        return ret;

    return ret;
}

gint32 CInterfaceState::Stop()
{
    return ClosePort();
}

gint32 CInterfaceState::EnumProperties(
    std::vector< gint32 >& vecProps ) const
{
    gint32 ret = super::EnumProperties( vecProps );
    if( ERROR( ret ) )
        return ret;

    return m_pIfCfgDb->EnumProperties( vecProps );
}

bool CInterfaceState::exist(
    gint32 iProp ) const
{
    std::vector< gint32 > vecProps; 
    EnumProperties( vecProps );
    for( auto i : vecProps )
    {
        if( iProp == i )
            return true;
    }
    return false;
}

gint32 CInterfaceState::GetProperty(
    gint32 iProp, CBuffer& pVal ) const
{
    gint32 ret = 0;
    CStdRMutex oStatLock( GetLock() );
    if( unlikely( iProp == propPortId && m_hPort != 0 ) )
    {
        guint32 dwPortId = ( guint32 )-1;
        ret = m_pIfCfgDb->GetProperty( iProp, pVal );
        if( SUCCEEDED( ret ) )
            dwPortId = pVal;

        if( dwPortId == ( guint32 )-1 )
        {
            PortPtr pPort;
            CIoManager* pMgr = GetIoMgr();
            ret = pMgr->GetPortPtr( m_hPort, pPort );
            if( ERROR( ret ) )
                return ret;

            CCfgOpenerObj oPortCfg(
                ( IPort* )pPort );

            ret = oPortCfg.GetIntProp(
                propPdoId, dwPortId );

            if( ERROR( ret ) )
                return ret;

            pVal = dwPortId;

            CCfgOpener oIfCfg(
                ( IConfigDb* )m_pIfCfgDb );

            oIfCfg[ propPortId ] = dwPortId;
        }
    }
    else
    {
        ret = m_pIfCfgDb->GetProperty( iProp, pVal );
    }
    return ret;
}

CLocalProxyState::CLocalProxyState(
    const IConfigDb* pCfg )
    : super( pCfg )
{
    SetClassId( clsid( CLocalProxyState ) );    
}

bool CLocalProxyState::IsMyDest(
    const string& strModName )
{
    gint32 ret = 0;

    do{
        CCfgOpenerObj oCfg( this );

        string strDestation;

        ret = oCfg.GetStrProp(
            propDestDBusName, strDestation );

        if( ERROR( ret ) )
            break;

        if( strModName == strDestation )
            break;

        string strObjPath;

        ret = oCfg.GetStrProp(
            propObjPath, strObjPath );

        if( strObjPath == strModName )
            break;

        ret = -EINVAL;

    }while( 0 );

    if( ERROR( ret ) )
        return false;

    return true;
}

gint32 CLocalProxyState::SubscribeEvents()
{
    vector< EnumPropId > vecEvtToSubscribe = {
        propStartStopEvent,
        propDBusModEvent,
        propDBusSysEvent,
        propAdminEvent,
    };

    return SubscribeEventsInternal(
        vecEvtToSubscribe );
}

gint32 CLocalProxyState::OpenPort(
    IEventSink* pCallback )
{
    // let's prepare the parameters
    return OpenPortInternal( pCallback );
}

CRemoteProxyState::CRemoteProxyState(
    const IConfigDb* pCfg )
    : super( pCfg )
{
    SetClassId( clsid( CRemoteProxyState ) );    
}

gint32 CRemoteProxyState::SubscribeEvents()
{
    vector< EnumPropId > vecEvtToSubscribe = {
        propStartStopEvent,
        propDBusModEvent,
        propRmtModEvent,
        propRmtSvrEvent,
        propDBusSysEvent,
        propAdminEvent
    };

    return SubscribeEventsInternal(
        vecEvtToSubscribe );
}

gint32 CRemoteProxyState::OnRmtModEvent(
    EnumEventId iEvent,
    const string& strModule,
    const string& strIpAddr )
{
    // Maybe in the future, we will split the 
    // OnModEvent to two different methods
    return OnModEvent( iEvent, strModule );
}

gint32 CRemoteProxyState::OnRmtSvrEvent(
        EnumEventId iEvent,
        const std::string& strIpAddr,
        HANDLE hPort )
{
    if( iEvent == eventRmtDBusOnline )
    {
        iEvent = eventRmtSvrOnline;
    }
    else if( iEvent == eventRmtDBusOffline )
    {
        iEvent = eventRmtSvrOffline;
    }

    return 0;
}

bool CRemoteProxyState::IsMyDest(
    const std::string& strModName )
{
    bool bRet = super::IsMyDest( strModName );

    if( bRet )
        return bRet;

    string strRtName;
    GetIoMgr()->GetRouterName( strRtName );

    // if the rpc router is down, we are done too
    string strDest =
        DBUS_DESTINATION( strRtName );

    if( strDest == strModName )
        return true;

    return false;
}

CIfServerState::CIfServerState(
    const IConfigDb* pCfg )
    : super( pCfg )
{
    SetClassId( clsid( CIfServerState ) );
}

gint32 CIfServerState::SubscribeEvents()
{
    vector< EnumPropId > vecEvtToSubscribe = {
        propDBusModEvent,
        propStartStopEvent,
        propDBusSysEvent,
        propAdminEvent
    };

    return SubscribeEventsInternal(
        vecEvtToSubscribe );
}

gint32 CIfServerState::OpenPort(
    IEventSink* pCallback )
{
    // let's prepare the parameters
    return OpenPortInternal( pCallback );
}

bool CIfServerState::IsMyDest(
    const string& strModName )
{
    gint32 ret = 0;

    do{
        CCfgOpenerObj oCfg( this );

        string strSrcDbusName;

        ret = oCfg.GetStrProp(
            propSrcDBusName, strSrcDbusName );

        if( ERROR( ret ) )
            break;

        if( strModName == strSrcDbusName )
            break;

        string strObjPath;

        ret = oCfg.GetStrProp(
            propObjPath, strObjPath );

        if( strObjPath == strModName )
            break;

        ret = -EINVAL;

    }while( 0 );

    if( ERROR( ret ) )
        return false;

    return true;
}

gint32 CTcpBdgePrxyState::SetupOpenPortParams(
    IConfigDb* pCfg )
{
    CCfgOpener oCfg( pCfg );

    oCfg.CopyProp( propDestTcpPort, this );
    oCfg.CopyProp( propSrcTcpPort, this );
    oCfg.CopyProp( propIsServer, this );

    return 0;
}

gint32 CUnixSockStmState::SetupOpenPortParams(
        IConfigDb* pCfg )
{
    CCfgOpener oCfg( pCfg );

    oCfg.CopyProp( propFd, this );
    oCfg.CopyProp( propTimeoutSec, this );
    oCfg.CopyProp( propKeepAliveSec, this );
    oCfg.CopyProp( propListenOnly, this );

    return 0;
}
