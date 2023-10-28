/*
 * =====================================================================================
 *
 *       Filename:  ifstat.h
 *
 *    Description:  Declaration of the family of CInterfaceState classes
 *
 *        Version:  1.0
 *        Created:  02/12/2017 08:44:58 PM
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
#pragma once
#include <sys/time.h>
#include "defines.h"
#include "utils.h"
#include "configdb.h"
#include "msgmatch.h"

namespace rpcf
{

#define CLEAR_SUBSCRIPTION_ON_ERROR() \
do{ \
    if( ERROR( ret ) ) \
    { \
        for( guint32 i = 0; i < vecEvents.size(); i++ ) \
        { \
            oCpHelper.UnsubscribeEvent( \
                vecEvents[ i ], pEvent ); \
        } \
        vecEvents.clear(); \
    } \
    else \
    { \
        CStdRMutex oStateLock( GetLock() ); \
        m_vecTopicList = vecEvents; \
    } \
}while( 0 ) 

typedef enum : guint32
{
    stateStopped = 0,
    stateStarting,
    stateStarted,
    stateConnected,
    stateRecovery,
    statePaused,
    stateUnknown,
    stateStopping,
    statePausing,
    stateResuming,
    stateInvalid,
    stateIoDone,    // for the task state
    stateStartFailed,

}EnumIfState;

class IInterfaceEvents
{
    public:

    // events from the connection points
    virtual gint32 OnPortEvent(
        EnumEventId iEvent,
        HANDLE hPort ) = 0;

    virtual gint32 OnModEvent(
        EnumEventId iEvent,
        const std::string& strModule ) = 0;

    virtual gint32 OnDBusEvent(
        EnumEventId iEvent ) = 0;

    virtual gint32 OnRmtModEvent(
        EnumEventId iEvent,
        const std::string& strModule,
        IConfigDb* pEvtCtx ) = 0;

    virtual gint32 OnRmtSvrEvent(
        EnumEventId iEvent,
        IConfigDb* pEvtCtx,
        HANDLE hPort ) = 0;

    virtual gint32 OnAdminEvent(
        EnumEventId iEvent,
        const std::string& strParams ) = 0;

};

class IInterfaceState :
    public IEventSink,
    public IInterfaceEvents
{

    public:

    typedef IEventSink super;

    virtual EnumIfState GetState() const = 0;

    virtual gint32 SubscribeEvents() = 0;

    virtual gint32 UnsubscribeEvents() = 0;

    virtual gint32 OpenPort(
        IEventSink* pCallback ) = 0;

    virtual gint32 ClosePort(
        IEventSink* pCallback ) = 0;

    virtual gint32 SetStateOnEvent(
        EnumEventId iEvent ) = 0;

    virtual gint32 TestSetState(
        EnumEventId iEvent ) = 0;

    virtual gint32 CopyProp( gint32 iProp,
        CObjBase* pObj ) = 0;

    virtual HANDLE GetHandle() const = 0;
};

typedef CAutoPtr< clsid( Invalid ), IInterfaceState > IfStatPtr;

class CRpcInterfaceBase;

class CInterfaceState : public IInterfaceState
{
    public:
    typedef IInterfaceState super;

    private:
    CIoManager                  *m_pIoMgr;

    HANDLE                      m_hPort;
    IGenericInterface           *m_pInterface;

    // std::string              m_strModName;
    // std::string              m_strIfPath;
    // bool                     m_bEventEnabled;
    // timeval                  m_tvReqTimeout;
    // std::string              m_strPortPath;
    CfgPtr                      m_pIfCfgDb;
    // mutable stdrmutex           m_oLock;
    std::atomic< EnumIfState >  m_iState;
    EnumIfState                 m_iResumeState;

    void SetStateInternal( gint32 iState )
    { m_iState = ( EnumIfState )iState; }

    protected:
    std::vector< EnumPropId >   m_vecTopicList;

    typedef std::pair< EnumIfState, EnumEventId >
        STATMAP_KEY;

    static std::map< STATMAP_KEY, EnumIfState >
        m_mapState;

    gint32 SubscribeEventsInternal(
        const std::vector< EnumPropId >& vecEvtToSubscribe );

    public:

    inline CIoManager* GetIoMgr() const
    { return m_pIoMgr; }

    HANDLE GetHandle() const
    { return m_hPort; }

    CRpcInterfaceBase* GetInterface();

    stdrmutex& GetLock() const;

    CInterfaceState(
        const IConfigDb* pCfg );

    ~CInterfaceState();

    // the rpc channel consists of the proxy and
    // the port stack this method will trigger to
    // build the port stack if necessary and bind
    // this interface to the port stack the
    // content in the config db is as follows

    gint32 OpenPortInternal(
        IEventSink* pCallback,
        bool bRetry = true );

    virtual gint32 SetupOpenPortParams(
        IConfigDb* pcfg )
    { return 0; }

    virtual gint32 OnPortEvent(
        EnumEventId iEvent,
        HANDLE hPort );

    virtual gint32 OnModEvent(
        EnumEventId iEvent,
        const std::string& strModule );

    virtual gint32 OnDBusEvent(
        EnumEventId iEvent );

    virtual gint32 OnAdminEvent(
        EnumEventId iEvent,
        const std::string& strParams );

    virtual gint32 ClosePort(
        IEventSink* pCallback = nullptr );

    virtual gint32 GetProperty(
        gint32 iProp, Variant& oVar ) const override;

    virtual gint32 SetProperty(
        gint32 iProp, const Variant& oVar ) override
    {
        CStdRMutex oStatLock( GetLock() );
        return m_pIfCfgDb->SetProperty( iProp, oVar );
    }

    gint32 RemoveProperty( gint32 iProp )
    {
        CStdRMutex oStatLock( GetLock() );
        return m_pIfCfgDb->RemoveProperty( iProp );
    }

    gint32 CopyProp( gint32 iProp, CObjBase* pObj )
    {
        // CStdRMutex oStatLock( GetLock() );
        CCfgOpenerObj oCfg( this );
        return oCfg.CopyProp( iProp, pObj );
    }

    gint32 EnumProperties(
        std::vector< gint32 >& vecProps ) const;

    bool exist( gint32 iProp ) const;

    virtual gint32 OnEvent( EnumEventId iEvent,
                LONGWORD dwParam1,
                LONGWORD dwParam2,
                LONGWORD* pData );

    virtual EnumIfState GetState() const
    { return m_iState; }

    virtual gint32 SetStateOnEvent(
        EnumEventId iEvent );

    gint32 TestSetState(
        EnumEventId iEvent );

    gint32 ScheduleOpenPortTask(
        IConfigDb* pCfg );

    virtual bool IsMyPort( HANDLE hPort );

    virtual bool IsMyDest(
        const std::string& strModName ) = 0;

    virtual gint32 UnsubscribeEvents();

    virtual gint32 OnRmtModEvent(
        EnumEventId iEvent,
        const std::string& strModule,
        IConfigDb* pEvtCtx )
    {  return -ENOTSUP; }

    virtual gint32 OnRmtSvrEvent(
        EnumEventId iEvent,
        IConfigDb* pEvtCtx,
        HANDLE hPort )
    {  return -ENOTSUP; }

    virtual gint32 Start(
        IEventSink* pCallback );

    virtual gint32 Stop();
};

class CLocalProxyState : public CInterfaceState
{
    public:

    typedef CInterfaceState super;

    CLocalProxyState( const IConfigDb* pCfg );

    virtual gint32 OpenPort(
        IEventSink* pCallback );

    // subscribe a connection point event
    virtual gint32 SubscribeEvents();

    virtual bool IsMyDest(
        const std::string& strModName );
};

class CRemoteProxyState : public CLocalProxyState
{
    public:
    typedef CLocalProxyState super;

    CRemoteProxyState( const IConfigDb* pCfg );

    virtual bool IsMyDest(
        const std::string& strModName );

    virtual gint32 SubscribeEvents();

    virtual gint32 OnRmtModEvent(
        EnumEventId iEvent,
        const std::string& strModule,
        IConfigDb* pEvtCtx );

    virtual gint32 OnRmtSvrEvent(
        EnumEventId iEvent,
        IConfigDb* pEvtCtx,
        HANDLE hPort );

    virtual gint32 SetupOpenPortParams(
        IConfigDb* pCfg );

    virtual gint32 OpenPort(
        IEventSink* pCallback );

    virtual gint32 OnPortEvent(
        EnumEventId iEvent,
        HANDLE hPort );
};

class CIfServerState : public CInterfaceState
{

    public:
    typedef CInterfaceState super;

    CIfServerState( const IConfigDb* pCfg );

    virtual gint32 OpenPort(
        IEventSink* pCallback );

    virtual gint32 SubscribeEvents();

    virtual bool IsMyDest(
        const std::string& strModName );

};

class CTcpBdgePrxyState : public CLocalProxyState
{
    public:
    typedef CLocalProxyState super;
    CTcpBdgePrxyState( const IConfigDb* pCfg )
        :super( pCfg )
    {
        SetClassId( clsid( CTcpBdgePrxyState ) );    
    }
    
    virtual gint32 SetupOpenPortParams(
        IConfigDb* pCfg );

    virtual gint32 OpenPort(
        IEventSink* pCallback );

    virtual gint32 OnPortEvent(
        EnumEventId iEvent,
        HANDLE hPort );

    virtual gint32 SubscribeEvents();
};

class CIfTcpBridgeState : public CIfServerState
{
    public:
    typedef CIfServerState super;
    CIfTcpBridgeState( const IConfigDb* pCfg )
        :super( pCfg )
    { SetClassId( clsid( CIfTcpBridgeState ) ); }
    virtual gint32 SubscribeEvents();
};

class CIfReqFwdrState : public CIfServerState
{
    public:
    typedef CIfServerState super;
    CIfReqFwdrState( const IConfigDb* pCfg )
        :super( pCfg )
    { SetClassId( clsid( CIfReqFwdrState ) ); }
    virtual gint32 SubscribeEvents();
};

class CIfReqFwdrPrxyState : public CLocalProxyState
{
    public:
    typedef CLocalProxyState super;
    CIfReqFwdrPrxyState( const IConfigDb* pCfg )
        :super( pCfg )
    { SetClassId( clsid( CIfReqFwdrPrxyState ) ); }
    gint32 SubscribeEvents() override;
    gint32 SetupOpenPortParams(
        IConfigDb* pCfg ) override;
};

class CUnixSockStmState : public CLocalProxyState
{
    public:
    typedef CLocalProxyState super;
    CUnixSockStmState( const IConfigDb* pCfg )
        :super( pCfg )
    {
        SetClassId( clsid( CUnixSockStmState ) );    
    }
    
    gint32 SetupOpenPortParams(
        IConfigDb* pCfg );

    gint32 SubscribeEvents()
    { return 0; }
};

class CStmCpState : public CLocalProxyState
{
    public:
    typedef CLocalProxyState super;
    CStmCpState( const IConfigDb* pCfg )
        :super( pCfg )
    { SetClassId( clsid( CStmCpState ) ); }
    
    gint32 SetupOpenPortParams(
        IConfigDb* pCfg );
    gint32 SubscribeEvents()
    { return 0; }
};

class CDummyInterfaceState : public CLocalProxyState
{
    public:
    typedef CLocalProxyState super;
    CDummyInterfaceState( const IConfigDb* pCfg )
        : super( pCfg )
    { SetClassId( clsid( CDummyInterfaceState ) ); }
    // subscribe a connection point event
    virtual gint32 SubscribeEvents()
    { return 0; }
};

}
