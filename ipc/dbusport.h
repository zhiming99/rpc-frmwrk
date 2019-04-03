/*
 * =====================================================================================
 *
 *       Filename:  dbusport.h
 *
 *    Description:  dbus bus port and bus driver implementation
 *
 *        Version:  1.0
 *        Created:  03/18/2016 11:38:24 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 * =====================================================================================
 */
#pragma once

#include <dbus/dbus.h>
#include "defines.h"
#include "port.h"
#include "autoptr.h"
#include "msgmatch.h"

#define DBUS_HANDLER_RESULT_HALT    ( ( DBusHandlerResult )100 )

gint32 NormalizeIpAddr(
    gint32 dwProto,
    const std::string strIn,
    std::string& strOut );

gint32 Ip4AddrToByteStr(
    const std::string& strIpAddr,
    std::string& strRet );

gint32 Ip4AddrToBytes(
    const std::string& strIpAddr,
    guint8* pBytes,
    guint32& dwSize );

gint32 BytesToString(
    const guint8* bytes,
    guint32 dwSize,
    std::string& strRet );

class CRpcBasePort : public CPort
{
    protected:

    struct MATCH_ENTRY
    {
        // irps waiting for the response or event
        std::deque<IrpPtr>      m_queWaitingIrps;

        // in case the incoming message is not being
        // handled in time
        std::deque<DMsgPtr>     m_quePendingMsgs;

        // we use a refcount to keep track of count
        // of interfaces referencing the same match
        gint32                  m_iRefCount;
        bool                    m_bConnected;

        MATCH_ENTRY()
            : m_iRefCount( 0 ),
            m_bConnected( false )
        {;}

        MATCH_ENTRY( const MATCH_ENTRY& rhs )
        {
            m_iRefCount = rhs.m_iRefCount;
            m_bConnected = rhs.m_bConnected;
            m_queWaitingIrps = rhs.m_queWaitingIrps;
            m_quePendingMsgs = rhs.m_quePendingMsgs;
        }

        ~MATCH_ENTRY()
        {
            m_queWaitingIrps.clear();
            m_quePendingMsgs.clear();
        }

        int AddRef()
        {
            ++m_iRefCount;
            return m_iRefCount;
        }
        int Release()
        {
            --m_iRefCount;
            return m_iRefCount;
        }
        int RefCount() const
        { return m_iRefCount; }

        bool IsConnected() const
        {
            return m_bConnected;
        }

        void SetConnected( bool bConnected )
        {
            m_bConnected = bConnected;
        }
    };

    IrpPtr m_pTestIrp;
    //
	// event/request listening irps

    // routing table for incoming event/signal
    // messages
    typedef std::map< MatchPtr, MATCH_ENTRY > MatchMap;

    MatchMap m_mapEvtTable;


	// the map for responst of the outgoing request IRPs 
	std::map< guint32, IrpPtr >     m_mapSerial2Resp;

    gint32 FindIrpForResp(
        DBusMessage* pMsg, IrpPtr& pIrp );

    gint32 CancelAllIrps( gint32 iErrno );
    
    protected:

    virtual gint32 DispatchSignalMsg(
        DBusMessage* pMsg );

    virtual gint32 DispatchRespMsg(
        DBusMessage* pMsg );

    gint32 DispatchKeepAliveMsg(
        DBusMessage* pMsg );

    gint32 ClearMatchMapInternal(
        MatchMap& oMap,
        std::vector< IrpPtr >& vecPendingIrps );

    virtual gint32 ClearDBusSetting(
        IMessageMatch* pMatch ) = 0;

    virtual gint32 SetupDBusSetting(
        IMessageMatch* pMatch ) = 0;

    virtual gint32 ClearMatchMap(
        std::vector< IrpPtr >& vecPendingIrps );

    gint32 RemoveIrpFromMapInternal(
        IRP* pIrp, MatchMap& oMap );

    virtual gint32 RemoveIrpFromMap( IRP* pIrp );

    virtual gint32 HandleRegMatch( IRP* pIrp );

    virtual gint32 HandleUnregMatch( IRP* pIrp );

    virtual gint32 HandleListening( IRP* pIrp );

    virtual gint32 HandleSendReq( IRP* pIrp ) = 0;

    virtual gint32 HandleSendEvent( IRP* pIrp ) = 0;

    virtual gint32 HandleSendData( IRP* pIrp ) = 0;

    // virtual gint32 HandleFetchData( IRP* pIrp );

    gint32 AddMatch( MatchMap& oMap, MatchPtr& pMatch );

    gint32 RemoveMatch( MatchMap& oMap, MatchPtr& pMatch );

    virtual gint32 IsIfSvrOnline( const DMsgPtr& pMsg );

    gint32 ClearReqIrpsOnDest(
        const std::string& strDestination,
        std::vector< IrpPtr >& vecIrpsToCancel );

    gint32 OnModOnOffline( DMsgPtr& pMsg, bool bOnline,
        const std::string& strModName );

    virtual gint32 OnModOnOffline( DBusMessage* pMsg );

    virtual DBusHandlerResult  DispatchDBusSysMsg(
        DBusMessage* pMsg );

    virtual gint32 ScheduleModOnOfflineTask(
        const std::string strModName, guint32 dwFlags );

    gint32 BuildSendDataReq(
        IConfigDb* pParams, DMsgPtr& pMsg );

    public:

    typedef CPort super;

    CRpcBasePort( const IConfigDb* pCfg = nullptr );

    virtual gint32 OnSubmitIrp( IRP* pIrp );

    virtual gint32 PreStop( IRP *pIrp );

    virtual gint32 SubmitIoctlCmd( IRP* pIrp );

    virtual gint32 GetMatchMap(
        IRP* pIrp, MatchMap*& pMap );

	virtual gint32 CancelFuncIrp(
        IRP* pIrp, bool bForce = true );

    virtual gint32 DispatchData( CBuffer* pData );

    virtual DBusHandlerResult PreDispatchMsg(
        gint32 iMsgType, DBusMessage* pMsg );

    virtual DBusHandlerResult DispatchMsg(
        gint32 iMsgType, DBusMessage* pMsg );

    virtual void PostDispatchMsg(
        gint32 iMsgType, DBusMessage* pMsg )
    {;}

    virtual gint32 Start()
    { return 0; }

    virtual gint32 BuildSendDataMsg(
        IRP* pIrp, DMsgPtr& pMsg );

};
/**
* @name CRpcBasePortEx, an extension of
* CRpcBasePort with the requst map for incoming
* requests 
* @{ */
/**  @} */

class CRpcBasePortEx : public CRpcBasePort
{
    protected:
    // routing table for incoming request messages
    MatchMap    m_mapReqTable;

    virtual gint32 ClearMatchMap(
        std::vector< IrpPtr >& vecPendingIrps );

    virtual gint32 RemoveIrpFromMap( IRP* pIrp );

    gint32 GetMatchMap( IMessageMatch* pMatch,
        MatchMap*& pMap );

    public:

    typedef  CRpcBasePort super;

    CRpcBasePortEx( const IConfigDb* pCfg )
        :CRpcBasePort( pCfg )
    {;}

    gint32 GetMatchMap( IRP* pIrp,
        MatchMap*& pMap );

    gint32 DispatchReqMsg( DBusMessage* pMsg );

    // dispatch data
    virtual DBusHandlerResult DispatchMsg(
        gint32 iMsgType, DBusMessage* pMsg );

};

class IRpcPdoPort : public CRpcBasePortEx
{
    // a tag class to mark this object as a pdo port
    public:
    typedef  CRpcBasePortEx super;

    IRpcPdoPort( const IConfigDb* pCfg )
        :CRpcBasePortEx( pCfg )
    { m_dwFlags = PORTFLG_TYPE_PDO; }
};

class CRpcPdoPort : public IRpcPdoPort
{
    protected:

    DBusConnection* m_pDBusConn;

    virtual gint32 ClearDBusSetting(
        IMessageMatch* pMatch );

    virtual gint32 SetupDBusSetting(
        IMessageMatch* pMatch );

    virtual gint32 HandleSendReq( IRP* pIrp );

    virtual gint32 HandleSendEvent( IRP* pIrp );

    virtual gint32 HandleSendData( IRP* pIrp );

    gint32 ScheduleModOnOfflineTask(
        const std::string strModName, guint32 dwFlags );

    public:

    typedef IRpcPdoPort super;

    CRpcPdoPort( const IConfigDb* pCfg = nullptr );

    virtual gint32 SendDBusMsg(
        DBusMessage* pMsg, guint32* pdwSerial );

    // IEventSink method
    virtual gint32 OnEvent(
        EnumEventId iEvent,
        guint32 dwParam1 = 0,
        guint32 dwParam2 = 0,
        guint32* pData = NULL  );

    
    gint32 SubmitIoctlCmd( IRP* pIrp );
    gint32 HandleSetReqQueSize( IRP* pIrp );
};

class CDBusLocalPdo : public CRpcPdoPort
{
    // pdo for local rpc via dbus
    //
    // the following two queues are for
    // server interfaces
    //

    // match table for dbus sys message
    MatchPtr                m_matchDBus;

    // registered objects on the dbus
    std::map< std::string, gint32 > m_mapRegObjs;

    protected:

    virtual gint32 ClearDBusSetting(
        IMessageMatch* pMatch );

    virtual gint32 SetupDBusSetting(
        IMessageMatch* pMatch );


    gint32 HandleSendResp( IRP* pIrp );

    public:

    typedef CRpcPdoPort super;

    CDBusLocalPdo(
        const IConfigDb* pCfg = NULL );

    ~CDBusLocalPdo();

    virtual bool Unloadable()
    { return false; }

    virtual gint32 PostStart( IRP* pIrp );


    virtual gint32 Stop( IRP *pIrp );

    gint32 SubmitIoctlCmd( IRP* pIrp );

    virtual DBusHandlerResult PreDispatchMsg(
        gint32 iMsgType, DBusMessage* pMsg );

    virtual DBusHandlerResult DispatchDBusSysMsg(
        DBusMessage* pMsg );

    virtual gint32 OnModOnOffline(
        DBusMessage* pMsg );
};

typedef CAutoPtr< clsid( CDBusLocalPdo ), CDBusLocalPdo > LocPdoPtr;

class CDBusLoopbackPdo : public CRpcPdoPort
{
    // this pdo will enable the in-process call to
    // a server from a proxy interface on the other
    // thread
    protected:

    virtual gint32 ClearDBusSetting(
        IMessageMatch* pMatch )
    { return 0; }

    virtual gint32 SetupDBusSetting(
        IMessageMatch* pMatch );

    // match table for dbus sys message
    MatchPtr                m_matchDBus;

    public:

    typedef CRpcPdoPort super;

    CDBusLoopbackPdo (
        const IConfigDb* pCfg );

    ~CDBusLoopbackPdo();

    virtual gint32 SendDBusMsg(
        DBusMessage* pMsg, guint32* pdwSerial );

    virtual DBusHandlerResult PreDispatchMsg(
        gint32 iMsgType, DBusMessage* pMsg );

    virtual bool Unloadable()
    { return false; }
};

struct CStartStopPdoCtx
{
    std::map< gint32, gint32 > m_mapIdToRes;
};

class CDBusBusPort : public CGenericBusPort
{
    // the pending calls waiting for dispatching
    // only calls to registered interface will
    // enter this queue. others are discarded
    std::deque<DMsgPtr> m_quePendingMsgs;

    // the dbus connection
    DBusConnection      *m_pDBusConn;
    TaskletPtr          m_pFlushTask;

    // local port attached to this bus
    gint32              m_iLocalPortId;
    gint32              m_iLpbkPortId;

    // proxy port attached to this bus
    std::vector<gint32> m_vecProxyPortIds;

    MatchPtr            m_pMatchDisconn;

    MatchPtr            m_pMatchLpbkProxy;
    MatchPtr            m_pMatchLpbkServer;

    std::map< std::string, gint32 > m_mapRules;

    using ADDRID_MAP =
        std::map< std::string, guint32 >;

    std::map< std::string, guint32 > m_mapAddrToId;

    gint32 CreateLocalDBusPdo(
        const IConfigDb* pConfig,
        PortPtr& pNewPort );

    gint32 CreateRpcProxyPdo(
        const IConfigDb* pConfig,
        PortPtr& pNewPort );

    gint32 CreateLoopbackPdo(
        const IConfigDb* pConfig,
        PortPtr& pNewPort );

    gint32 ReplyWithError(
        DBusMessage* pMsg );

    virtual gint32 GetChildPorts(
        std::vector< PortPtr >& vecChildPdo );

    void ReleaseDBus();

    gint32 SchedulePortsAttachNotifTask(
        std::vector< PortPtr >& vecChildPdo,
        guint32 dwEventId,
        IRP* pMasterIrp = nullptr );

    gint32 AddRemoveBusNameLpbk(
        const std::string& strName,
        bool bRemove );

    gint32 AddBusNameLpbk(
        const std::string& strName );

    gint32 RemoveBusNameLpbk(
        const std::string& strName );

    gint32 BuildModOnlineMsgLpbk(
        const std::string& strModName,
        bool bOnline, DMsgPtr& pMsg );

    gint32 AsyncStopDBusConn(
        IRP* pIrp, gint32 iPos );

    public:

    typedef CGenericBusPort super;

    CDBusBusPort(
        const IConfigDb* pConfig );

    ~CDBusBusPort();

    // this is our dbus message dispatcher
    // pData points to this object
    static DBusHandlerResult DBusMessageCallback(
         DBusConnection *pConn,
         DBusMessage *pMsg,
         void *pData);

    DBusHandlerResult OnMessageArrival(
        DBusMessage *pMsg );

    gint32 SendLpbkMsg(
        DBusMessage *pMsg, guint32* pdwSerial );

    gint32 SendDBusMsg(
        DBusMessage *pMsg, guint32* pdwSerial );

    virtual gint32 BuildPdoPortName(
        IConfigDb* pCfg,
        std::string& strPortName );

    virtual gint32 CreatePdoPort(
        IConfigDb* pConfig,
        PortPtr& pNewPort );

    virtual void RemovePdoPort(
            guint32 iPortId );

    // the only exit of dbus message out of
    // this application
    gint32 SendData( const BufPtr& pMsg );

    // IPort methods
    gint32 Start( IRP* pIrp );
    gint32 Stop( IRP* pIrp );

    // IEventSink method
    gint32 OnEvent( EnumEventId iEvent,
            guint32 dwParam1 = 0,
            guint32 dwParam2 = 0,
            guint32* pData = NULL  );

    // handler of the func irps
    gint32 OnSubmitIrp( IRP* pIrp )
    { return -ENOTSUP; }

    gint32 PostStart( IRP* pIrp );
    gint32 OnPortReady( IRP* pIrp );
    void   OnPortStartFailed(
        IRP* pIrp, gint32 ret );

    gint32 HandleDBusDisconn();

    // loopback related methods
    gint32 InitLpbkMatch();
    gint32 FilterLpbkMsg(
        DBusMessage* pMsg, guint32* pdwSerial );

    gint32 ScheduleLpbkTask(
        MatchPtr& pFilter,
        DBusMessage *pDBusMsg,
        guint32* pdwSerial );

    DBusHandlerResult OnLpbkMsgArrival(
        DBusMessage* pMsg );

    // methods from CObjBase
    gint32 GetProperty(
            gint32 iProp,
            CBuffer& oBuf ) const;

    gint32 SetProperty(
            gint32 iProp,
            const CBuffer& oBuf );

    gint32 EnumProperties(
        std::vector< gint32 >& vecProps ) const;

    virtual gint32 AllocIrpCtxExt(
        IrpCtxPtr& pIrpCtx,
        void* pContext = nullptr ) const;

    virtual gint32 ReleaseIrpCtxExt(
        IrpCtxPtr& pIrpCtx,
        void* pContext = nullptr );

    virtual gint32 OnCompleteSlaveIrp(
        IRP* pMaster, IRP* pSlave );

    gint32 RegBusName(
        const std::string& strName,
        guint32 dwFlags );

    gint32 ReleaseBusName(
        const std::string& strName );

    gint32 AddRules(
        const std::string& strRules );

    gint32 RemoveRules(
        const std::string& strRules );

    gint32 IsDBusSvrOnline(
        const std::string& strDest );

	virtual gint32 PreStop( IRP* pIrp );
};

class CDBusConnFlushTask 
    : public CTasklet
{
    CIoManager* m_pMgr;
    CDBusBusPort* m_pBusPort;
    DBusConnection* m_pDBusConn;
    guint32     m_dwIntervalMs;
    HANDLE      m_hTimer;

    public:
    typedef CTasklet super;
    CDBusConnFlushTask( const IConfigDb* pCfg );
    ~CDBusConnFlushTask();
    gint32 operator()( guint32 dwContext );
    gint32 Stop();
};


class CGenBusDriver : public IBusDriver
{
    protected:
    std::atomic<guint32> m_atmBusId;

    public:
    typedef IBusDriver super;
    CGenBusDriver( const IConfigDb* pCfg );

    // the Start() method will create the non-pnp bus
    // ports from the configs
    gint32 Start();
    gint32 Stop();

    gint32 NewBusId()
    { return m_atmBusId++; }

    // create the fdo bus port, one instance
    // for now
    gint32 CreatePort(
        PortPtr& pNewPort,
        const IConfigDb* pConfig = NULL);

    // IEventSink method
    gint32 OnEvent( EnumEventId iEvent,
        guint32 dwParam1 = 0,
        guint32 dwParam2 = 0,
        guint32* pData = NULL  )
    { return 0;}
};

class CDBusBusDriver : public CGenBusDriver
{

    public:
    typedef CGenBusDriver super;
    CDBusBusDriver( const IConfigDb* pCfg );
	gint32 Probe( IPort* pLowerPort,
        PortPtr& pNewPort,
        const IConfigDb* pConfig = NULL );
};

typedef CAutoPtr< clsid( CDBusBusPort ), CDBusBusPort > DBPortPtr;

