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

std::map< STATEMAP_KEY, EnumIfState > m_mapState =
{
    STATEMAP_ENTRY( stateStarting,  eventPortStarted,       stateStarted ),
    STATEMAP_ENTRY( stateStarting,  eventPortStartFailed,   stateStopped ),
    STATEMAP_ENTRY( stateStarting,  eventPortStopping,      stateStopped ),
    STATEMAP_ENTRY( stateStarted,   eventPortStopping,      stateStopped ),
    STATEMAP_ENTRY( stateConnected, eventPortStopping,      stateStopped ),
    STATEMAP_ENTRY( stateRecovery,  eventPortStopping,      stateStopped ),
    STATEMAP_ENTRY( statePaused,    eventPortStopping,      stateStopped ),
    STATEMAP_ENTRY( stateConnected, eventModOffline,        stateRecovery ),
    STATEMAP_ENTRY( statePaused,    eventModOffline,        statePaused ),
    STATEMAP_ENTRY( stateRecovery,  eventModOnline,         stateConnected ),
    STATEMAP_ENTRY( statePaused,    eventModOnline,         statePaused ),
    STATEMAP_ENTRY( stateStarting,  eventDBusOffline,       stateStopped ),
    STATEMAP_ENTRY( stateStarted,   eventDBusOffline,       stateStopped ),
    STATEMAP_ENTRY( stateConnected, eventDBusOffline,       stateStopped ),
    STATEMAP_ENTRY( stateRecovery,  eventDBusOffline,       stateStopped ),
    STATEMAP_ENTRY( statePaused,    eventDBusOffline,       stateStopped ),
    STATEMAP_ENTRY( stateConnected, eventRmtModOffline,     stateRecovery ),
    STATEMAP_ENTRY( statePaused,    eventRmtModOffline,     statePaused ),
    STATEMAP_ENTRY( stateRecovery,  eventRmtModOnline,      stateConnected ),
    STATEMAP_ENTRY( statePaused,    eventRmtModOnline,      statePaused ),
    STATEMAP_ENTRY( stateStarting,  eventRmtSvrOffline,     stateStopped ),
    STATEMAP_ENTRY( stateStarted,   eventRmtSvrOffline,     stateStopped ),
    STATEMAP_ENTRY( stateConnected, eventRmtSvrOffline,     stateStopped ),
    STATEMAP_ENTRY( stateRecovery,  eventRmtSvrOffline,     stateStopped ),
    STATEMAP_ENTRY( statePaused,    eventRmtSvrOffline,     stateStopped ),
    STATEMAP_ENTRY( stateStopped,   cmdOpenPort,            stateStarting ),
    STATEMAP_ENTRY( stateStarting,  cmdClosePort,           stateStopped ),
    STATEMAP_ENTRY( stateStarted,   cmdClosePort,           stateStopped ),
    STATEMAP_ENTRY( stateConnected, cmdClosePort,           stateStopped ),
    STATEMAP_ENTRY( stateRecovery,  cmdClosePort,           stateStopped ),
    STATEMAP_ENTRY( statePaused,    cmdClosePort,           stateStopped ),
    STATEMAP_ENTRY( stateStarted,   cmdEnableEvent,         stateConnected ),
    STATEMAP_ENTRY( stateConnected, cmdDisableEvent,        stateStarted ),
    STATEMAP_ENTRY( stateRecovery,  cmdDisableEvent,        stateStarted ),
    STATEMAP_ENTRY( statePaused,    cmdDisableEvent,        stateStarted ),
    STATEMAP_ENTRY( stateStarting,  cmdShutdown,            stateStopped ),
    STATEMAP_ENTRY( stateStarted,   cmdShutdown,            stateStopped ),
    STATEMAP_ENTRY( stateConnected, cmdShutdown,            stateStopped ),
    STATEMAP_ENTRY( stateRecovery,  cmdShutdown,            stateStopped ),
    STATEMAP_ENTRY( statePaused,    cmdShutdown,            stateStopped ),
    STATEMAP_ENTRY( stateConnected, cmdPause,               statePaused ),

    // unknown means the value depends on the m_iResumeState
    STATEMAP_ENTRY( statePaused,    cmdSendData,            stateUnknown ), 
    STATEMAP_ENTRY( stateConnected, cmdFetchData,           stateConnected ),
    STATEMAP_ENTRY( stateConnected, cmdSendReq,             stateConnected )
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
            propObjPtr, pObj );

        if( ERROR( ret ) )
            break;

        m_pInterface = pObj;
        if( m_pInterface == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        m_pIfCfgDb->RemoveProperty( propIoMgr );
        m_pIfCfgDb->RemoveProperty( propObjPtr );

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

gint32 CInterfaceState::SetStateOnEvent(
    EnumEventId iEvent )
{
    // state switch table
    gint32 ret = ERROR_STATE;
    CStdRMutex oStateLock( GetLock() );

    switch( iEvent )
    {
    case cmdEnableEvent:
    case cmdRegSvr:
        {
            if( GetState() == stateStarted )
            {
                SetStateInternal( stateConnected );
                ret = 0;
            }
            break;
        }
    case cmdUnregSvr:
    case cmdDisableEvent:
        {
            EnumIfState iState = GetState();
            switch( iState )
            {
            case stateConnected:
            case stateRecovery:
            case statePaused:
                {
                    SetStateInternal( stateStarted );
                    ret = 0;
                    break;
                }
            default:
                break;
            }
            break;
        }
    default:
        {
            break;
        }
    }
    return ret;
}

bool CInterfaceState::IsMyPort(
    HANDLE hPort )
{
    if( hPort == 0 )
        return false;

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
        if( hPort == 0
            || iEvent == eventInvalid )
        {
            ret = -EINVAL;
            break;
        }

        CIoManager* pMgr = GetIoMgr();

        switch( iEvent )
        {
        case eventPortStartFailed:
            {
                if( true )
                {
                    CStdRMutex oStatLock( GetLock() );
                    if( GetState() != stateStarting )
                        break;

                    SetStateInternal( stateStopped );
                }

                break;
            }
        case eventPortStarted:
            {
                CRpcInterfaceBase* pIf = GetInterface();

                CStdRMutex oStatLock( GetLock() );

                // out of sync
                if( GetState() != stateStarting )
                    break;

                ret = pMgr->OpenPortByRef(
                    hPort, pIf );

                if( ERROR( ret ) )
                    break;

                m_hPort = hPort;

                SetStateInternal( stateStarted );

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

                CStdRMutex oStatLock( GetLock() );

                EnumIfState iState = GetState();
                switch( iState )
                {
                case stateStarted:
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
            {
                CStdRMutex oStatLock( GetLock() );
                // out of sync
                if( GetState() == stateConnected )
                {
                    // FIXME: should we schedule a
                    // task for more actions?
                    SetStateInternal( stateRecovery );
                    break;
                }
                else if( GetState() == statePaused )
                {
                    m_iResumeState = stateStarted;
                }
                else
                {
                    ret = ERROR_STATE;
                }

                // we don't have things to do
                break;
            }
        case eventRmtModOnline:
        case eventModOnline:
            {
                CStdRMutex oStatLock( GetLock() );
                EnumIfState iState = GetState();
                switch( iState )
                {
                case stateStarted:
                case stateRecovery:
                    {
                        // what else can we do?
                        SetStateInternal( stateConnected ); 
                        break;
                    }
                case statePaused:
                    {
                        m_iResumeState = stateConnected;
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
                CStdRMutex oStatLock( GetLock() );
                switch( GetState() )
                {
                case stateStarted:
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
                CStdRMutex oStatLock( GetLock() );
                switch( GetState() )
                {
                case stateStarted:
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
                    break;
                }

                break;
            }
        case cmdPause:
            {
                CStdRMutex oStatLock( GetLock() );

                switch( GetState() )
                {
                case stateConnected:
                    {
                        SetStateInternal( statePaused );
                        m_iResumeState = stateConnected;
                        break;
                    }
                default:
                    ret = ERROR_STATE;
                    break;
                }

                break;
            }
        case cmdResume:
            {
                if( true )
                {
                    CStdRMutex oStatLock( GetLock() );
                    switch( GetState() )
                    {
                    case statePaused:
                        {
                            if( m_iResumeState != stateInvalid )
                                SetStateInternal( m_iResumeState );
                            break;
                        }
                    default:
                        ret = ERROR_STATE;
                        break;
                    }
                }

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
    guint32 dwParam1,
    guint32 dwParam2,
    guint32* pData )
{
    gint32 ret = 0;

    switch( iEvent )
    {
    case eventPortStarted:
    case eventPortStartFailed:
    case eventPortStopped:
    case eventPortStopping:
        {
            ret = OnPortEvent( iEvent, dwParam1 );
            break;
        }
    case eventModOnline:
    case eventModOffline:
        {
            if( pData == nullptr )
                break;
            string strModName = ( char* )pData;
            ret = OnModEvent( iEvent, strModName );
            break;
        }
    case eventDBusOnline: 
    case eventDBusOffline:
        {
            ret = OnDBusEvent( iEvent );
            break;
        }
    case cmdShutdown:
    case cmdPause:
    case cmdResume:
        {
            if( pData == nullptr )
                break;

            string strParams = ( char* )pData;
            ret = OnAdminEvent( iEvent, strParams );
            break;
        }
    case eventRmtModOffline:
    case eventRmtModOnline:
        {
            if( pData == nullptr )
                break;
            string strIpAddr = ( char* )dwParam1;
            string strModName = ( char* )pData;
            ret = OnRmtModEvent(
                iEvent, strModName, strIpAddr );
            break;
        }
    case eventRmtSvrOffline:
    case eventRmtSvrOnline:
        {
            string strIpAddr = ( char* )dwParam1;
            ret = OnRmtSvrEvent(
                iEvent, strIpAddr );
            break;
        }
    default:
        break;
    }
    return ret;
}

gint32 CInterfaceState::OpenPortInternal(
    IEventSink* pCallback,  bool bRetry )
{
    gint32 ret = 0;

    if( pCallback == nullptr )
        return -EINVAL;

    do{
        CStdRMutex oStateLock( GetLock() );

        if( GetState() != stateStopped )
            return ERROR_STATE;

        ret = SubscribeEvents();

        if( ERROR( ret ) )
            return ret;

        CParamList oParams;

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

        SetStateInternal( stateStarting );

        oStateLock.Unlock();

        ret = GetIoMgr()->OpenPort(
            oParams.GetCfg(), m_hPort );

        oStateLock.Lock();

        if( GetState() != stateStarting )
        {
            ret = ERROR_STATE;
            break;
        }

        if( ret == STATUS_PENDING )
        {
            // let's wait the port to be created
            // in the event handler OnPortEvent
            break;
        }

        if( SUCCEEDED( ret ) )
        {
            SetStateInternal( stateStarted );
            break;
        }

        if( ret == -EAGAIN )
        {
            // retry will happen from outside
            // rollback the state
            SetStateInternal( stateStopped );
            UnsubscribeEvents();
            break;
        }

        // unrecoverable error
        
    }while( 0 );

    if( ERROR( ret ) )
    {
        UnsubscribeEvents();
    }

    return ret;
}

gint32 CInterfaceState::ClosePort()
{
    gint32 ret = 0;

    if( m_hPort != 0 )
    {
        ret = GetIoMgr()->ClosePort(
            m_hPort, GetInterface() );

        m_hPort = 0;
    }

    CStdRMutex oStateLock( GetLock() );
    SetStateInternal( stateStopped );
    return ret;
}


gint32 CInterfaceState::UnsubscribeEvents()
{
    gint32 ret = 0;

    CRpcInterfaceBase* pIf = GetInterface();
    EventPtr pEvent( pIf );

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
    UnsubscribeEvents();
    return ClosePort();
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
    gint32 ret = 0;

    CConnPointHelper oCpHelper( GetIoMgr() );

    CRpcInterfaceBase* pIf = GetInterface();
    EventPtr pEvent( pIf );

    vector< EnumPropId > vecEvents;

    do{
        ret = oCpHelper.SubscribeEvent( 
            propStartStopEvent, pEvent );

        if( ERROR( ret ) )
            break;

        vecEvents.push_back( propStartStopEvent );

        ret = oCpHelper.SubscribeEvent(
            propDBusModEvent, pEvent );

        if( ERROR( ret ) )
            break;

        vecEvents.push_back( propDBusModEvent );

        ret = oCpHelper.SubscribeEvent(
            propDBusSysEvent, pEvent );

        if( ERROR( ret ) )
            break;

        vecEvents.push_back( propDBusSysEvent );

        ret = oCpHelper.SubscribeEvent(
            propAdminEvent, pEvent );

        if( ERROR( ret ) )
            break;

        vecEvents.push_back( propAdminEvent );

    }while( 0 );

    CLEAR_SUBSCRIPTION_ON_ERROR();
    return ret;
}

gint32 CLocalProxyState::OpenPort(
    IEventSink* pCallback )
{
    // FIXME: not adaptable port path
    // generation
    gint32 ret = 0;

    // let's prepare the parameters
    ret = OpenPortInternal( pCallback );

    return ret;
}

CRemoteProxyState::CRemoteProxyState(
    const IConfigDb* pCfg )
    : super( pCfg )
{
    SetClassId( clsid( CRemoteProxyState ) );    
}

gint32 CRemoteProxyState::SubscribeEvents()
{
    gint32 ret = 0;

    CConnPointHelper oCpHelper( GetIoMgr() );

    CRpcInterfaceBase* pIf = GetInterface();
    EventPtr pEvent( pIf );

    vector< EnumPropId > vecEvents;

    do{
        ret = oCpHelper.SubscribeEvent( 
            propStartStopEvent, pEvent );

        if( ERROR( ret ) )
            break;

        vecEvents.push_back( propStartStopEvent );

        ret = oCpHelper.SubscribeEvent(
            propRmtModEvent, pEvent );

        if( ERROR( ret ) )
            break;

        vecEvents.push_back( propRmtModEvent );

        ret = oCpHelper.SubscribeEvent(
            propRmtSvrEvent, pEvent );

        if( ERROR( ret ) )
            break;

        vecEvents.push_back( propRmtSvrEvent );

        ret = oCpHelper.SubscribeEvent(
            propDBusSysEvent, pEvent );

        if( ERROR( ret ) )
            break;

        vecEvents.push_back( propDBusSysEvent );

        ret = oCpHelper.SubscribeEvent(
            propAdminEvent, pEvent );

        if( ERROR( ret ) )
            break;

        vecEvents.push_back( propAdminEvent );

    }while( 0 );

    CLEAR_SUBSCRIPTION_ON_ERROR();

    return ret;
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
    const string& strIpAddr )
{
    if( iEvent == eventRmtDBusOnline )
    {
        iEvent = eventRmtSvrOnline;
    }
    else if( iEvent == eventRmtDBusOffline )
    {
        iEvent = eventRmtSvrOffline;
    }

    return OnDBusEvent( iEvent );
}

bool CRemoteProxyState::IsMyDest(
    const std::string& strModName )
{
    bool bRet = super::IsMyDest( strModName );

    if( bRet )
        return bRet;

    // if the rpc router is down, we are done too
    string strDest =
        DBUS_DESTINATION( MODNAME_RPCROUTER );

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
    gint32 ret = 0;

    CConnPointHelper oCpHelper( GetIoMgr() );

    CRpcInterfaceBase* pIf = GetInterface();
    EventPtr pEvent( pIf );
    vector< EnumPropId > vecEvents;

    do{
        ret = oCpHelper.SubscribeEvent( 
            propStartStopEvent, pEvent );

        if( ERROR( ret ) )
            break;

        vecEvents.push_back( propStartStopEvent );

        ret = oCpHelper.SubscribeEvent(
            propDBusSysEvent, pEvent );

        if( ERROR( ret ) )
            break;

        vecEvents.push_back( propDBusSysEvent );

        ret = oCpHelper.SubscribeEvent(
            propAdminEvent, pEvent );

        if( ERROR( ret ) )
            break;

        vecEvents.push_back( propAdminEvent );

    }while( 0 );

    CLEAR_SUBSCRIPTION_ON_ERROR();

    return ret;
}

gint32 CIfServerState::OpenPort(
    IEventSink* pCallback )
{
    // FIXME: not adaptable port path
    // generation
    gint32 ret = 0;

    // let's prepare the parameters
    ret = OpenPortInternal( pCallback );

    return ret;
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
