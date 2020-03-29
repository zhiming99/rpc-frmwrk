/*
 * =====================================================================================
 *
 *       Filename:  rpcroute.h
 *
 *    Description:  implementation RPC related stuffs
 *
 *        Version:  1.0
 *        Created:  04/06/2016 09:59:50 PM
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
#include "defines.h"
#include "port.h"
#include "ifhelper.h"
#include "iftasks.h"
#include "counters.h"
#include "tractgrp.h"

#define ROUTER_OBJ_DESC             "./router.json"

struct IRpcReqProxyAsync
{

    // all the methods here are asynchronous
    //
    // open the listening port if not existing and
    // add an irp to listen to the strObjPath
    // usually called on start of the pdo config
    // db including `ip addr', `dst dbus name'
    // `obj name'
    virtual gint32 OpenRemotePort(
        IEventSink* pCallback,
        const IConfigDb* pCfg ) = 0;

    // config db including `ip addr', `dst dbus
    // name' called on interface demand
    virtual gint32 CloseRemotePort(
        IEventSink* pCallback,
        const IConfigDb* pCfg ) = 0;

    // modified by the pdo on call to interface's
    // enable event and redirect to the router
    // config db including `ip addr', `event dbus
    // name' `obj name' `if name'
    virtual gint32 EnableRemoteEvent(
        IEventSink* pCallback,
        IMessageMatch* pMatch ) = 0;

    // modified by the pdo and redirect to the
    // router
    //
    // config db including `ip addr', `event dbus
    // name' `obj name' `if name'
    virtual gint32 DisableRemoteEvent(
        IEventSink* pCallback,
        IMessageMatch* pMatch ) = 0;

    // called by the pdo to redirect a method call
    // to the router
    //
    // Response message layout:
    //
    // the propReturnValue contains the return
    // value
    //
    // 0 is the first [out] param
    //
    // 1 is the second [out] param ...
    // and so on
    //
    // the pRespMsg is returned only if the method
    // succeeded, if the method returns error,
    // the pRespMsg is empty.
    //
    // if STATUS_PENDING is returned,
    // the pRespMsg is not valid. And the msg will
    // be returned as the first arg in the
    // pCallback's propRespPtr.

    virtual gint32 ForwardRequest(
        IConfigDb* pReqCtx,
        DBusMessage* pReqMsg,           // [ in ]
        DMsgPtr& pRespMsg,              // [ out ]
        IEventSink* pCallback ) = 0;

};

class IRpcReqServerAsync
    : public IRpcReqProxyAsync
{
    typedef IRpcReqProxyAsync super;
    // IMPORTANT NOTE: that all the methods in
    // this interface ( including those methods
    // inherited from super's members ) cannot be
    // accessed randomly even if you have the
    // pointer to the interface, because they are
    // wrapped heavily with synchronization and
    // interception mechanism, and calling them
    // from outside the mechanism will cause
    // unexpected reasult
};

class IRpcEventRelay
{
    public:
    // the event message which comes from remote
    // server at strSrcIrp. The host Msg sent out
    // by this method has a layout as the
    // following
    //
    // 1. the first argument must be the string of
    // source ip addr of the event, [required by
    // the CProxyMsgMatch]. So if you don't have
    // an ip address related message, just put
    // '0.0.0.0' as a placeholder.
    //
    // 2. if there is dbus message across the
    // network, the dbus message from remote
    // server is serialized in byte array
    //
    // 3. we don't support array of variable
    // lenght element for now. All you have to do
    // is to serialize your data to a byte array
    // before adding it to the dbus message
    //
    // 4. IMPORTANT: It is a proxy-style method,
    // and can be accessed from all threads within
    // the process

    virtual gint32 ForwardEvent(
        IConfigDb* pEvtCtx,
        DBusMessage* pEventMsg,
        IEventSink* pCallback ) = 0;

};

class CRpcRouter;

class CRegisteredObject
    : public CConfigDb
{
    public:
    typedef CConfigDb super;

    CRegisteredObject( const IConfigDb* pCfg )
        : super( nullptr )
    { SetClassId( clsid( CRegisteredObject ) ); }

    gint32 SetProperties(
        const std::string& strSrcDBusName,
        const std::string& strSrcUniqName,
        guint32 dwPortId )
    {
        CCfgOpener oCfg( this );

        oCfg.SetStrProp(
            propSrcUniqName, strSrcUniqName );

        oCfg.SetStrProp(
            propSrcDBusName, strSrcDBusName );

        oCfg.SetIntProp(
            propPrxyPortId, dwPortId );

        return 0;
    }

    guint32 GetPortId()
    {
        CCfgOpener oCfg( this );
        guint32 dwPortId =
            oCfg[ propPrxyPortId ];
        return dwPortId;
    }

    std::string GetUniqName()
    {
        std::string strName;
        CCfgOpener oCfg( this );
        strName = oCfg[ propSrcUniqName ];
        return strName;
    }

    // test if the match belongs to this
    // registered interface
    gint32 IsMyMatch( IMessageMatch* pMatch );
    bool operator<( CRegisteredObject& rhs ) const;
};

typedef CAutoPtr< clsid( CRegisteredObject ), CRegisteredObject > RegObjPtr;

namespace std {

    template<>
    struct less<RegObjPtr>
    {
        bool operator()(const RegObjPtr& k1, const RegObjPtr& k2) const
        {
             return ( *k1 < *k2 );
        }
    };
}

class CRpcInterfaceProxy :
    virtual public CAggInterfaceProxy,
    public IRpcReqProxyAsync,
    public IRpcEventRelay
{
    protected:
    CRpcRouter  *m_pParent;

    gint32 OnKeepAliveRelay(
        IEventSink* pTask );

    //gint32 OnKeepAliveTerm(
     //   IEventSink* pTask );

    virtual gint32 OnKeepAliveOrig(
        IEventSink* pTask );

    public:
    typedef CAggInterfaceProxy super;
    CRpcInterfaceProxy( const IConfigDb* pCfg ) :
        super( pCfg )
    {;}

    virtual gint32 OnKeepAlive(
        IEventSink* pInvokeTask,
        EnumKAPhase bOrigin );

    CRpcRouter* GetParent() const
    { return m_pParent; }

    virtual bool IsConnected(
        const char* szAddr = nullptr );
};

class CRpcInterfaceServer :
    virtual public CAggInterfaceServer,
    public IRpcReqServerAsync,
    public IRpcEventRelay
{

    // common methods in both the CRpcReqForwarder
    // and CRpcTcpBridge 
    protected:
    CRpcRouter  *m_pParent;

    virtual gint32 BuildBufForIrpFwrdEvt(
        BufPtr& pBuf,
        IConfigDb* pReqCall ) = 0;

    virtual gint32 SetupReqIrpFwrdEvt(
        IRP* pIrp,
        IConfigDb* pReqCall,
        IEventSink* pCallback ) = 0;

    virtual gint32 OnKeepAliveRelay(
        IEventSink* pInvokeTask ) = 0;

    gint32 ValidateRequest_SendData(
        DBusMessage* pReqMsg,
        IConfigDb* pDataDesc );

    public:

    typedef CAggInterfaceServer super;
    CRpcInterfaceServer( const IConfigDb* pCfg ) :
        super( pCfg )
    {;}

    virtual bool IsConnected(
        const char* szAddr = nullptr );

    virtual gint32 DoInvoke(
        DBusMessage* pReqMsg,
        IEventSink* pCallback );

    CRpcRouter* GetParent() const
    { return m_pParent; }

    virtual gint32 SetupReqIrp(
        IRP* pIrp,
        IConfigDb* pReqCall,
        IEventSink* pCallback );

    virtual gint32 BuildBufForIrp(
        BufPtr& pBuf,
        IConfigDb* pReqCall );

    virtual gint32 OnKeepAlive(
        IEventSink* pInvokeTask,
        EnumKAPhase bOrigin );
};

class CRpcReqForwarder :
     public CRpcInterfaceServer
{
    protected:

    // the cfgptr contains the following
    // properties
    //
    // propPortId
    // propSrcUniqName
    // propSrcDBusName
    // check if the match is valid before it is
    // registered
    gint32 CheckMatch( IMessageMatch* pMatch );

    virtual gint32 BuildBufForIrpFwrdEvt(
        BufPtr& pBuf,
        IConfigDb* pReqCall );

    virtual gint32 OnKeepAliveRelay(
        IEventSink* pInvokeTask );

    gint32 EnableDisableEvent(
        IEventSink* pCallback,
        IMessageMatch* pMatch,
        bool& bEnable );

    gint32 OnRmtSvrOnline(
        IConfigDb* pEvtCtx,
        HANDLE hPort );

    gint32 OnRmtSvrOffline(
        IConfigDb* pEvtCtx,
        HANDLE hPort );

    gint32 BuildBufForIrpRmtSvrEvent(
        BufPtr& pBuf, IConfigDb* pReqCall );

    virtual gint32 OnRmtSvrEvent(
        EnumEventId iEvent,
        IConfigDb* pEvtCtx,
        HANDLE hPort );

    virtual gint32 BuildBufForIrp(
        BufPtr& pBuf, IConfigDb* pReqCall );

    gint32 OpenRemotePortInternal(
        IEventSink* pCallback,
        IConfigDb* pCfg );

    gint32 CloseRemotePortInternal(
        IEventSink* pCallback,
        IConfigDb* pCfg );

    virtual gint32 SetupReqIrpFwrdEvt(
        IRP* pIrp,
        IConfigDb* pReqCall,
        IEventSink* pCallback );

    gint32 StopBridgeProxy(
        IEventSink* pCallback,
        guint32 dwPortId,
        const std::string& strSrcUniqName,
        const std::string& strSrcDBusName );

    gint32 OnModOfflineInternal(
        IEventSink* pCallback,
        EnumEventId iEvent,
        std::string strUniqName );

    gint32 BuildKeepAliveMessage(
        IConfigDb* pCfg, DMsgPtr& pkaMsg );

    // CRpcInterfaceServer
    gint32 SendFetch_Server(
        IConfigDb* pDataDesc,           // [in, out]
        gint32& fd,                     // [out]
        guint32& dwOffset,              // [in,out]
        guint32& dwSize,                // [in,out]
        IEventSink* pCallback );

    bool SupportIid( EnumClsid iIfId ) const
    {
        if( iIfId == iid( IStream ) )
            return true;
        if( iIfId == iid( IStreamMH ) )
            return true;
        return false;
    }

    gint32 OnCheckRouterPathComplete(
        IEventSink* pCallback, 
        IEventSink* pIoReq,
        IConfigDb* pReqCtx );

    public:

    typedef CRpcInterfaceServer super;

    CRpcReqForwarder(
        const IConfigDb* pCfg );

    gint32 InitUserFuncs();

    gint32 AddRefCount(
        guint32 dwPortId,
        const std::string& strSrcUniqName,
        const std::string& strSrcDBusName );

    gint32 DecRefCount(
        guint32 dwPortId,
        const std::string& strSrcUniqName,
        const std::string& strSrcDBusName );

    gint32 ClearRefCountByPortId(
        guint32 dwPortId,
        std::vector< std::string >& vecUniqNames );

    gint32 ClearRefCountByUniqName(
        const std::string& strUniqName,
        std::set< guint32 >& setPortIds );

    gint32 GetRefCountByPortId(
        guint32 dwPortId );

    gint32 GetRefCountByUniqName(
        const std::string& strUniqName );

    const EnumClsid GetIid() const
    { return iid( CRpcReqForwarder ); }
    //
    // called by the bus port to create the remote
    // connection before the proxy pdo is created
    //
    // we have to call it from the proxy pdo to
    // utilize the support for synchronous call
    // across the process border
    //
    // make a connection to the remote target
    //
    // propConnParams: the remote address the
    // connection will be made
    //
    // propSrcDBusName: the local source who send
    // out the request
    //
    // propObjPath
    // propIfName
    // 
    virtual gint32 OpenRemotePort(
        IEventSink* pCallback,
        const IConfigDb* pCfg );

    // active disconnecting
    virtual gint32 CloseRemotePort(
        IEventSink* pCallback,
        const IConfigDb* pCfg );

    // mandatory properties within the pMatch
    //
    // propRouterPath: address of the remote server
    // propIfName: interface name
    // propObjPath: object path
    // propDestDBusName: destination dbus name
    //
    virtual gint32 EnableRemoteEvent(
        IEventSink* pCallback,
        IMessageMatch* pMatch );

    virtual gint32 DisableRemoteEvent(
        IEventSink* pCallback,
        IMessageMatch* pMatch );

    virtual gint32 ForwardEvent(
        IConfigDb* pEvtCtx,
        DBusMessage* pEventMsg,
        IEventSink* pCallback );

    gint32 CheckReqToFwrd(
        IConfigDb* pReqCtx,
        DMsgPtr& pMsg,
        MatchPtr& pMatchHit );

    // methods of CRpcBaseOperations
    virtual gint32 OnModEvent(
        EnumEventId iEvent,
        const std::string& strModule );

    gint32 ForwardRequest(
        IConfigDb* pReqCtx,
        DBusMessage* pFwdrMsg,
        DMsgPtr& pRespMsg,
        IEventSink* pCallback );

    virtual gint32 FetchData_Server(
        IConfigDb* pDataDesc,           // [in]
        gint32 fd,                      // [in]
        guint32 dwOffset,               // [in]
        guint32 dwSize,                 // [in]
        IEventSink* pCallback )
    {
        return SendFetch_Server( pDataDesc,
            fd, dwOffset, dwSize, pCallback );
    }

    gint32 CheckRouterPath(
        IEventSink* pCallback,
        IConfigDb* pReqCtx );

}; // CRpcReqForwarder

class CRpcRfpForwardEventTask
    : public CIfParallelTask
{
    public:
    typedef CIfParallelTask super;
    CRpcRfpForwardEventTask ( const IConfigDb* pCfg = nullptr )
        : CIfParallelTask( pCfg )
    {
        SetClassId( clsid( CRpcRfpForwardEventTask ) );
    }
    virtual gint32 RunTask();
};

class CRfpModEventRespTask :
    public CIfInterceptTaskProxy
{
    public:
    typedef CIfInterceptTaskProxy super;

    CRfpModEventRespTask ( const IConfigDb* pCfg )
        : super( pCfg )
    { SetClassId( clsid( CRfpModEventRespTask  ) ); }

    virtual gint32 RunTask()
    { return STATUS_PENDING; }

    virtual gint32 OnTaskComplete( gint32 iRetVal );
};

class CRpcReqForwarderProxy :
    public CRpcInterfaceProxy
{
    protected:
    // tcp port handle and the related
    // informations
    gint32 SetupReqIrpEnableEvt( IRP* pIrp,
        IConfigDb* pReqCall,
        IEventSink* pCallback );

    gint32 SetupReqIrpFwrdReq( IRP* pIrp,
        IConfigDb* pReqCall,
        IEventSink* pCallback );

    virtual gint32 RebuildMatches();

    using IFREF = std::pair< gint32, gint32 >;
    std::map< MatchPtr, std::pair< gint32, gint32 > > m_mapMatchRefs;

    public:

    typedef CRpcInterfaceProxy super;
    CRpcReqForwarderProxy( const IConfigDb* pCfg );
    ~CRpcReqForwarderProxy();

    gint32 InitUserFuncs();

    const EnumClsid GetIid() const
    { return iid( CRpcReqForwarder ); }

    virtual gint32 SetupReqIrp( IRP* pIrp,
        IConfigDb* pReqCall,
        IEventSink* pCallback );

    virtual gint32 OpenRemotePort(
        IEventSink* pCallback,
        const IConfigDb* pCfg )
    { return -ENOTSUP; }

    // active disconnecting
    virtual gint32 CloseRemotePort(
        IEventSink* pCallback,
        const IConfigDb* pCfg )
    { return -ENOTSUP; }

    virtual gint32 EnableRemoteEvent(
        IEventSink* pCallback,
        IMessageMatch* pMatch )
    { return -ENOTSUP; }

    virtual gint32 DisableRemoteEvent(
        IEventSink* pCallback,
        IMessageMatch* pMatch )
    { return -ENOTSUP; }

    virtual gint32 ForwardRequest(
        IConfigDb* pReqCtx,
        DBusMessage* pReqMsg,           // [ in ]
        DMsgPtr& pRespMsg,              // [ out ]
        IEventSink* pCallback );

    virtual gint32 ForwardEvent(
        IConfigDb* pEvtCtx,
        DBusMessage* pEventMsg,
        IEventSink* pCallback );

    virtual gint32 CustomizeRequest(
        IConfigDb* pReqCfg,
        IEventSink* pCallback );

    gint32 FillRespData(
        IRP* pIrp, CfgPtr& pResp );

    gint32 DoInvoke(
        DBusMessage* pEvtMsg,
        IEventSink* pCallback );

    gint32 NormalizeMatch(
        IMessageMatch* pMatchExternal,
        MatchPtr& pMatchOut );

    gint32 AddInterface(
        IMessageMatch* pMatch );

    gint32 RemoveInterface(
        IMessageMatch* pMatch );

    gint32 GetActiveIfCount()
    {
        gint32 iCount = 0;

        CStdRMutex oIfLock( GetLock() );
        for( auto elem : m_vecMatches )
        {
            IMessageMatch* pMatch = elem; 
            CCfgOpenerObj oMatch( pMatch );
            bool bDummy = false;

            gint32 ret = oMatch.GetBoolProp(
                propDummyMatch, bDummy );

            if( SUCCEEDED( ret ) && bDummy )
                continue;

            ++iCount;
        }
        return iCount;
    }

    gint32 OnRmtSvrEvent(
        EnumEventId iEvent,
        IConfigDb* pEvtCtx,
        HANDLE hPort );

    protected:

    gint32 OnModEvent(
        EnumEventId iEvent,
        const std::string& strModule );

    gint32 OnRmtSvrOffline(
        IConfigDb* pEvtCtx,
        HANDLE hPort );

    gint32 OnKeepAliveOrig(
        IEventSink* pTask );

}; // CRpcReqForwarderProxy 

struct CRpcTcpBridgeShared
{
    CRpcTcpBridgeShared( CRpcServices* pIf )
    { m_pParentIf = pIf; }

    gint32 OpenLocalStream( IPort* pPort,
        gint32 iStreamId,
        guint16 wProtoId,
        gint32& iStmIdOpened );

    gint32 CloseLocalStream(
        IPort* pPort, gint32 iStreamId );

    InterfPtr GetIfPtr() const
    {
        return InterfPtr( const_cast
        < CRpcServices* >( m_pParentIf ) );
    }

    // local commands
    gint32 ReadStream( gint32 iStreamId,
        BufPtr& pDestBuf,
        guint32 dwSize,
        IEventSink* pCallback );

    gint32 WriteStream( gint32 iStreamId,
        CBuffer* pSrcBuf,
        guint32 dwSize,
        IEventSink* pCallback );

    gint32 IoControlStream( gint32 iStreamId,
        CBuffer* pReq,
        BufPtr& pResp,
        IEventSink* pCallback );

    gint32 RegMatchCtrlStream(
        gint32 iStreamId,
        MatchPtr& pMatch,
        bool bReg );

    protected:
    CRpcServices* m_pParentIf;

    gint32 ReadWriteStream(
        gint32 iStreamId,
        BufPtr& pSrcBuf,
        guint32 dwSize,
        IEventSink* pCallback,
        bool bRead );

    gint32 SetupReqIrpListeningShared(
        IRP* pIrp,
        IConfigDb* pReqCall,
        IEventSink* pCallback );

    gint32 OnPostStartShared(
        IEventSink* pContext,
        MatchPtr& pMatch );

    guint32 GetPortToSubmitShared(
        CObjBase* pCfg,
        PortPtr& pPort,
        bool& bPdo );
};

class CRpcTcpBridge :
    public CRpcInterfaceServer,
    public CRpcTcpBridgeShared
{
    friend class CRpcTcpBridgeShared;

    gint32 EnableRemoteEventInternal(
        IEventSink* pCallback,
        IMessageMatch* pMatch,
        bool bEnable );

    gint32 EnableRemoteEventInternalMH(
        IEventSink* pCallback,
        IMessageMatch* pMatch,
        bool bEnable );

    virtual gint32 BuildBufForIrpFwrdEvt(
        BufPtr& pBuf,
        IConfigDb* pReqCall );

    virtual gint32 OnKeepAliveRelay(
        IEventSink* pTask );

    gint32 SetupReqIrpListening( IRP* pIrp,
        IConfigDb* pReqCall,
        IEventSink* pCallback )
    {
        return SetupReqIrpListeningShared(
            pIrp, pReqCall, pCallback );
    }

    gint32 OpenStream_Server(
        IEventSink* pCallback,
        gint32 iPeerStm,
        guint32 wProtocol );

    gint32 CloseStream_Server(
        IEventSink* pCallback,
        gint32 iStreamId );

    gint32 OnCheckRouterPathComplete(
        IEventSink* pCallback, 
        IEventSink* pIoReq,
        IConfigDb* pReqCtx );

    gint32 ClearRemoteEventsLocal(
        IEventSink* pCallback,
        ObjPtr& pvecMatches );

    gint32 OnClearRemoteEventsComplete(
        IEventSink* pCallback,
        IEventSink*  pIoReq,
        IConfigDb* pReqCtx );

    protected:
    virtual gint32 SetupReqIrpFwrdEvt(
        IRP* pIrp,
        IConfigDb* pReqCall,
        IEventSink* pCallback );

    gint32 CheckRouterPathAgain(
        IEventSink* pCallback,
        IConfigDb* pReqCtx );

    public:

    virtual gint32 OnPostStart(
        IEventSink* pContext );

    typedef CRpcInterfaceServer super;
    CRpcTcpBridge( const IConfigDb* pCfg );
    ~CRpcTcpBridge();

    const EnumClsid GetIid() const
    { return iid( CRpcTcpBridge ); }

    virtual gint32 OpenRemotePort(
        IEventSink* pCallback,
        const IConfigDb* pCfg )
    { return -ENOTSUP; }

    virtual gint32 SetupReqIrp( IRP* pIrp,
        IConfigDb* pReqCall,
        IEventSink* pCallback );

    // active disconnecting
    virtual gint32 CloseRemotePort(
        IEventSink* pCallback,
        const IConfigDb* pCfg )
    { return -ENOTSUP; }

    virtual gint32 EnableRemoteEvent(
        IEventSink* pCallback,
        IMessageMatch* pMatch );

    virtual gint32 DisableRemoteEvent(
        IEventSink* pCallback,
        IMessageMatch* pMatch );

    virtual gint32 ForwardEvent(
        IConfigDb* pEvtCtx,
        DBusMessage* pEventMsg,
        IEventSink* pCallback );

    virtual gint32 CheckReqToRelay(
        IConfigDb* pReqCtx,
        DMsgPtr& pMsg,
        MatchPtr& pMatchHit );

    virtual gint32 InitUserFuncs();

    // bridge and stream specific methods
    gint32 OpenStream(
        IEventSink* pCallback,  
        gint32 iPeerStm,
        guint32 wProtocol )
    {
        return OpenStream_Server(
            pCallback,
            iPeerStm,
            wProtocol );
    }

    gint32 CloseStream(
        IEventSink* pCallback,
        gint32 iStreamId )
    {
        return CloseStream_Server(
            pCallback, iStreamId );
    }

    virtual gint32 SendResponse(
        IConfigDb* pReqMsg,
        CfgPtr& pRespData );

    // called to set the response to send back
    virtual gint32 SetResponse(
        IEventSink* pTask,
        CfgPtr& pRespData );

    virtual gint32 OnPreStart(
        IEventSink* pCallback )
    {
        gint32 ret =
            super::OnPreStart( pCallback );

        if( ERROR( ret ) )
            return ret;

        // we don't need connect manager
        m_pConnMgr.Clear();
        return 0;
    }

    gint32 Pause_Server(
        IEventSink* pCallback )
    {
        // uncomment me if necessary.
        if( pCallback == nullptr )
            return -EINVAL;

        FillAndSetResponse(
            pCallback, -ENOTSUP );         
        return -ENOTSUP;
    }

    gint32 Resume_Server(
        IEventSink* pCallback )
    {
        if( pCallback == nullptr )
            return -EINVAL;

        FillAndSetResponse(
            pCallback, -ENOTSUP );         
        return -ENOTSUP;
    }

    gint32 ClearRemoteEvents(
        IEventSink* pCallback,
        ObjPtr& pVecMatches );

    gint32 CheckRouterPath(
        IEventSink* pCallback,
        IConfigDb* pReqCtx );

    using CRpcBaseOperations::GetPortToSubmit;

    virtual guint32 GetPortToSubmit(
        CObjBase* pCfg,
        PortPtr& pPort,
        bool& bPdo )
    {
        return GetPortToSubmitShared(
            pCfg, pPort, bPdo );
    }

    gint32 ForwardRequest(
        IConfigDb* pReqCtx,
        DBusMessage* pFwdrMsg,
        DMsgPtr& pRespMsg,
        IEventSink* pCallback );
}; // CRpcTcpBridge

class CRpcTcpBridgeProxy :
    public CRpcInterfaceProxy,
    public CRpcTcpBridgeShared
{
    friend class CRpcTcpBridgeShared;

    gint32 EnableRemoteEventInternal(
        IEventSink* pCallback,
        IMessageMatch* pMatch,
        bool bEnable );

    gint32 FillRespDataFwrdReq(
        IRP* pIrp, CfgPtr& pResp );

    gint32 BuildBufForIrpFwrdReq(
        BufPtr& pBuf,
        IConfigDb* pReqCall );

    gint32 SetupReqIrpListening(
        IRP* pIrp,
        IConfigDb* pReqCall,
        IEventSink* pCallback )
    {
        return SetupReqIrpListeningShared(
            pIrp, pReqCall, pCallback );
    }

    gint32 OpenStream_Proxy(
        guint32 wProtocol,
        gint32& iStreamId,
        IEventSink* pCallback );

    gint32 CloseStream_Proxy( gint32 iStreamId,
        IEventSink* pCallback );

    gint32 SendFetch_TcpProxy(
        IConfigDb* pDataDesc,           // [in, out]
        gint32& fd,                     // [out]
        guint32& dwOffset,              // [out]
        guint32& dwSize,                // [out]
        IEventSink* pCallback );

    gint32 CustomizeRequest(
        IConfigDb* pReqCfg,
        IEventSink* pCallback );

    gint32 OnRmtSvrEvent(
        IEventSink* pCallback,
        IConfigDb* pEvtCtx,
        guint32 dwEventId );

    public:

    typedef CRpcInterfaceProxy super;

    CRpcTcpBridgeProxy(
        const IConfigDb* pCfg );

    virtual gint32 SetupReqIrp( IRP* pIrp,
        IConfigDb* pReqCall,
        IEventSink* pCallback );

    virtual gint32 OpenRemotePort(
        IEventSink* pCallback,
        const IConfigDb* pCfg )
    { return -ENOTSUP; }

    const EnumClsid GetIid() const
    { return iid( CRpcTcpBridge ); }

    // RPC methods
    // active disconnecting
    virtual gint32 CloseRemotePort(
        IEventSink* pCallback,
        const IConfigDb* pCfg )
    { return -ENOTSUP; }

    // the method to do cleanup when the client is
    // down
    gint32 ClearRemoteEvents(
        ObjPtr& pVecMatches , // [ in ]
        IEventSink* pCallback );

    gint32 CheckRouterPath(
        IConfigDb* pReqCtx,
        IEventSink* pCallback );

    virtual gint32 EnableRemoteEvent(
        IEventSink* pCallback,
        IMessageMatch* pMatch );

    virtual gint32 DisableRemoteEvent(
        IEventSink* pCallback,
        IMessageMatch* pMatch );

    virtual gint32 ForwardRequest(
        IConfigDb* pReqCtx,
        DBusMessage* pReqMsg,           // [ in ]
        DMsgPtr& pRespMsg,              // [ out ]
        IEventSink* pCallback );

    virtual gint32 ForwardEvent(
        IConfigDb* pEvtCtx,
        DBusMessage* pEventMsg,
        IEventSink* pCallback );

    virtual gint32 FillRespData(
        IRP* pIrp, CfgPtr& pResp );

    virtual gint32 BuildBufForIrp(
        BufPtr& pBuf,
        IConfigDb* pReqCall );

    virtual gint32 DoInvoke(
        DBusMessage* pEvtMsg,
        IEventSink* pCallback );

    // bridge and stream specific methods
    gint32 OpenStream(
        guint32 wProtocol,
        gint32& iStreamId,
        IEventSink* pCallback )
    {
        return OpenStream_Proxy(
            wProtocol, iStreamId,
            pCallback );
    }

    gint32 CloseStream( gint32 iStreamId,
        IEventSink* pCallback )
    {
        return CloseStream_Proxy( 
            iStreamId, pCallback );
    }

    virtual gint32 InitUserFuncs();

    gint32 OnInvalidStreamId(
        IEventSink* pCallback,
        gint32 iStreamId,
        gint32 iPeerStreamId,
        guint32 dwSeqNo,
        guint32 dwProtocol );

    virtual gint32 OnPostStart(
        IEventSink* pContext );

    using CRpcBaseOperations::GetPortToSubmit;
    virtual guint32 GetPortToSubmit(
        CObjBase* pCfg,
        PortPtr& pPort,
        bool& bPdo )
    {
        return GetPortToSubmitShared(
            pCfg, pPort, bPdo );
    }

}; // CRpcTcpBridgeProxy

class CRouterStopBridgeProxyTask
    : public CIfInterceptTaskProxy
{
    public:
    typedef CIfInterceptTaskProxy super;
    CRouterStopBridgeProxyTask( const IConfigDb* pCfg )
        : CIfInterceptTaskProxy( pCfg )
    {
        SetClassId( clsid( CRouterStopBridgeProxyTask ) );
    }
    virtual gint32 RunTask();
    virtual gint32 OnTaskComplete( gint32 iRetVal );
    gint32 OnCancel( guint32 dwContext )
    {  return OnTaskComplete( -ECANCELED ); }
};

class CRouterStopBridgeProxyTask2
    : public CIfInterceptTaskProxy
{
    public:
    typedef CIfInterceptTaskProxy super;
    CRouterStopBridgeProxyTask2( const IConfigDb* pCfg )
        : CIfInterceptTaskProxy( pCfg )
    {
        SetClassId( clsid( CRouterStopBridgeProxyTask2 ) );
    }
    virtual gint32 RunTask();
    virtual gint32 OnTaskComplete( gint32 iRetVal );
    gint32 OnCancel( guint32 dwContext )
    {  return OnTaskComplete( -ECANCELED ); }
};

class CRouterStopBridgeTask
    : public CIfInterceptTaskProxy
{
    gint32 DisableRemoteEventsMH(
        IEventSink* pCallback,
        std::vector< MatchPtr >& vecMatches );

    public:
    typedef CIfInterceptTaskProxy super;
    CRouterStopBridgeTask( const IConfigDb* pCfg )
        : CIfInterceptTaskProxy( pCfg )
    {
        SetClassId( clsid( CRouterStopBridgeTask ) );
    }
    virtual gint32 RunTask();
    virtual gint32 OnTaskComplete( gint32 iRetVal );

    gint32 OnCancel( guint32 dwContext )
    {  return OnTaskComplete( -ECANCELED ); }

};

class CIfRouterState : public CIfServerState
{
    public:
    typedef CIfServerState super;
    CIfRouterState( const IConfigDb* pCfg );
    virtual gint32 SubscribeEvents();
};

class CRpcRouterManager;
class CRpcRouter :
    public virtual CAggInterfaceServer
{
    // the key is the peer ip-addr and the value is the
    // pair of the tcp bridge and tcp bridge proxy

    mutable stdrmutex   m_oLock;
    std::vector< EnumPropId >   m_vecTopicList;
    CRpcRouter*         m_pParent;

    protected:
    std::map< guint32, InterfPtr > m_mapPid2BdgeProxies;

    public:

    typedef CAggInterfaceServer super;

    CRpcRouter( const IConfigDb* pCfg );
    ~CRpcRouter();

    CRpcRouter* GetParent() const
    { return m_pParent; }

    CRpcRouterManager* GetRouterMgr() const;

    virtual bool HasReqForwarder() const
    { return false; }

    virtual bool HasBridge() const
    { return false; }

    gint32 AddRemoveMatch(
        IMessageMatch* pMatch, bool bAdd,
        std::map< MatchPtr, gint32 >* plm );

    gint32 GetMatchToAdd(
        IMessageMatch* pMatch,
        bool bRemote,
        MatchPtr& pMatchAdd ) const;

    inline gint32 GetMatchToRemove(
        IMessageMatch* pMatch,
        bool bRemote,
        MatchPtr& pMatchRemove )
    {
        return GetMatchToAdd(
            pMatch, bRemote, pMatchRemove );
    }

    gint32 GetBridgeProxy(
        const IConfigDb* pConnParams,
        InterfPtr& pIf );

    gint32 GetBridgeProxy(
        guint32 dwPortId,
        InterfPtr& pIf );

    gint32 AddBridgeProxy( 
        IGenericInterface* pIf );
        
    gint32 RemoveBridgeProxy( 
        IGenericInterface* pIf );

    gint32 SubscribeEvents();
    gint32 UnsubscribeEvents();

    protected:
    virtual gint32 OnRmtSvrOffline(
        IEventSink* pCallback,
        IConfigDb* pEvtCtx,
        HANDLE hPort )
    { return -ENOTSUP; }

    virtual gint32 OnRmtSvrEvent(
        EnumEventId iEvent,
        IConfigDb* pEvtCtx,
        HANDLE hPort )
    { return -ENOTSUP; }

    virtual gint32 OnRmtSvrOnline(
        IEventSink* pCallback,
        IConfigDb* pEvtCtx,
        HANDLE hPort )
    { return -ENOTSUP; }

    public:
    virtual gint32 OnPreStopLongWait(
        IEventSink* pCallback )
    { return -ENOTSUP; }

    gint32 OnPostStart( IEventSink* pContext );
    gint32 OnPreStop( IEventSink* pCallback );

    static gint32 GetNodeName(
        const std::string& strPath,
        std::string& strNode );

    gint32 GetPortId( HANDLE hPort,
        guint32 dwPortId ) const;
};

class CReqFwdrCloseRmtPortTask
    : public CIfInterceptTaskProxy
{
    public:
    typedef CIfInterceptTaskProxy super;
    CReqFwdrCloseRmtPortTask( const IConfigDb* pCfg )
        : CIfInterceptTaskProxy( pCfg )
    {
        SetClassId( clsid( CReqFwdrCloseRmtPortTask ) );
    }
    virtual gint32 RunTask();
    virtual gint32 OnTaskComplete( gint32 iRetVal );
    gint32 OnCancel( guint32 dwContext )
    {  return 0; }
};

class CReqFwdrOpenRmtPortTask 
    : public CIfInterceptTaskProxy
{

    protected:
    typedef enum{
        stateInitialized,
        stateStartBridgeProxy,
        stateStartBridgeServer,
        stateDone
    } TaskState;

    TaskState   m_iState;
    InterfPtr   m_pProxy;
    InterfPtr   m_pServer;

    // actively create a CRpcTcpPdo
    virtual gint32 CreateInterface( InterfPtr& pIf );
    virtual gint32 RunTaskInternal( gint32 iRetVal );

    // will call the CInterfaceServer's
    // OnServiceComplete to finish the async call
    virtual gint32 OnServiceComplete( gint32 iRetVal );

    public:
    typedef CIfInterceptTaskProxy super;

    CReqFwdrOpenRmtPortTask ( const IConfigDb* pCfg )
        : CIfInterceptTaskProxy( pCfg )
    {
        SetClassId( clsid( CReqFwdrOpenRmtPortTask ) );
        m_iState = stateInitialized;
    }

    virtual gint32 AdvanceState();
    virtual gint32 RunTask();
    virtual gint32 OnTaskComplete( gint32 iRetVal );
    gint32 OnCancel( guint32 dwContext );
};
/**
* @name CRouterOpenBdgePortTask
* @{ the task tries to create a
* CRpcTcpBridgeImpl or CRpcTcpBridgeProxyImpl
* object, and on success, register the object with
* the router  */
/** 
 * Parameters:
 *      0: bServer[ bool ], whether to create a
 *      proxy or a server.
 *      propRouterPtr: pointer to the router
 *      object to create this object.
 *
 *      propPortId: the port id for the
 *      CRpcTcpBridgeImpl to create, or the
 *      CRpcTcpBridgeImpl object, who requests to
 *      create the CRpcTcpBridgeProxyImpl if
 *      bServer is false.
 *
 *      propNodeName: the node name for the
 *      CRpcTcpBridgeProxyImpl to create when
 *      bServer is false.
 *
 *      propConnParams: the connection parameters
 *      to create the CRpcTcpBridgeProxyImpl.
 *
 *
 * @} */

class CRouterOpenBdgePortTask :
    public CReqFwdrOpenRmtPortTask
{
    gint32 AdvanceState();
    public:
    // passively open a CRpcTcpPdo, when the port has
    // already created.
    typedef CReqFwdrOpenRmtPortTask super;
    CRouterOpenBdgePortTask( const IConfigDb* pCfg )
        : super( pCfg )
    {
        SetClassId( clsid( CRouterOpenBdgePortTask ) ); 
    }

    virtual gint32 CreateInterface( InterfPtr& pIf );
    virtual gint32 RunTaskInternal( gint32 iRetVal );
    virtual gint32 OnServiceComplete( gint32 iRetVal )
    { return 0; }
};

class CIfRollbackableTask :
    public CIfInterceptTaskProxy
{
    public:
    typedef CIfInterceptTaskProxy super;
    CIfRollbackableTask( const IConfigDb* pCfg )
        : super( pCfg )
    {}

    gint32 GetTractGrp( CIfTransactGroup*& pTractGrp )
    {
        CCfgOpener oTaskCfg(
            ( IConfigDb* ) GetConfig() );

        guint32* intptr = nullptr;
        gint32 ret = oTaskCfg.GetIntPtr(
            propTransGrpPtr, intptr );

        if( ERROR( ret ) )
            return ret;

        pTractGrp = reinterpret_cast
            < CIfTransactGroup* >( intptr );

        if( pTractGrp == nullptr )
            return -EFAULT;

        return 0;
    }

    gint32 AddRollbackTask(
        TaskletPtr& pTask, bool bBack = false )
    {
        CIfTransactGroup* pTractGrp = nullptr;
        gint32 ret = GetTractGrp( pTractGrp );
        if( ERROR( ret ) )
            return ret;

        if( pTractGrp == nullptr )
            return -EFAULT;

        return pTractGrp->AddRollback(
            pTask, bBack );
    }

    gint32 ChangeRelation( EnumLogicOp iOp )
    {
        CIfTransactGroup* pTractGrp = nullptr;
        gint32 ret = GetTractGrp( pTractGrp );
        if( ERROR( ret ) )
            return ret;

        if( pTractGrp == nullptr )
            return -EFAULT;

        return pTractGrp->SetTaskRelation( iOp );
    }
    
    gint32 OnCancel( guint32 dwContext )
    { return OnTaskComplete( -ECANCELED ); }
};

class CRouterEnableEventRelayTask :
    public CIfRollbackableTask
{
    public:
    typedef CIfRollbackableTask super;

    // we will utilize the
    // CIfRetryTask::OnComplete's notification
    // support for the client notification
    //
    // parameters:
    // 0: bEnable
    // 1: pointer to a IMessageMatch object
    // propIfPtr
    // propEventSink
    // propIoMgr
    CRouterEnableEventRelayTask( const IConfigDb* pCfg )
        : super( pCfg )
    { SetClassId( clsid( CRouterEnableEventRelayTask  ) ); }

    virtual gint32 RunTask();
    virtual gint32 OnTaskComplete( gint32 iRetVal );
};

class CRouterStartRecvTask :
    public CIfInterceptTaskProxy
{
    public:
    typedef CIfInterceptTaskProxy super;
    CRouterStartRecvTask ( const IConfigDb* pCfg )
        : super( pCfg )
    { SetClassId( clsid( CRouterStartRecvTask ) ); }

    virtual gint32 RunTask();
};

class CRouterStartReqFwdrProxyTask :
    public CIfRollbackableTask
{
    gint32 OnTaskCompleteStart( gint32 iRetVal );
    public:
    typedef CIfRollbackableTask super;

    // we will utilize the
    // CIfRetryTask::OnComplete's notification
    // support for the client notification
    //
    // parameters:
    // 0: bEnable
    // 1: pointer to a IMessageMatch object
    // propIfPtr
    // propEventSink
    // propIoMgr
    CRouterStartReqFwdrProxyTask( const IConfigDb* pCfg )
        : super( pCfg )
    { SetClassId( clsid( CRouterStartReqFwdrProxyTask  ) ); }

    virtual gint32 RunTask();
    virtual gint32 OnTaskComplete( gint32 iRetVal );
};

class CReqFwdrEnableRmtEventTask :
    public CIfInterceptTaskProxy
{
    public:
    typedef CIfInterceptTaskProxy super;

    CReqFwdrEnableRmtEventTask( const IConfigDb* pCfg )
        : super( pCfg )
    { SetClassId( clsid( CReqFwdrEnableRmtEventTask ) ); }

    virtual gint32 RunTask();
    virtual gint32 OnTaskComplete( gint32 iRetVal );
};

class CRouterAddRemoteMatchTask :
    public CIfRollbackableTask
{
    public:
    typedef CIfRollbackableTask super;

    // parameters:
    // 0: bAdd
    // 1: pointer to a IMessageMatch object
    // propIfPtr
    // propEventSink
    // propIoMgr
    CRouterAddRemoteMatchTask( const IConfigDb* pCfg )
        : super( pCfg )
    { SetClassId( clsid( CRouterAddRemoteMatchTask ) ); }

    virtual gint32 RunTask();
    gint32 AddRemoteMatchInternal(
        CRpcRouter* pRouter,
        IMessageMatch* pMatch,
        bool bEnable );
};

class CRouterEventRelayRespTask :
    public CIfInterceptTaskProxy
{
    bool m_bEnable = true;
    public:
    typedef CIfInterceptTaskProxy super;

    CRouterEventRelayRespTask( const IConfigDb* pCfg )
        : super( pCfg )
    { SetClassId( clsid( CRouterEventRelayRespTask ) ); }

    virtual gint32 RunTask()
    { return STATUS_PENDING; }

    virtual gint32 OnTaskComplete( gint32 iRetVal );
    void SetEnable( bool bEnable )
    { m_bEnable = bEnable; }

};

class CReqFwdrForwardRequestTask :
    public CIfInterceptTask
{
    public:
    typedef CIfInterceptTask super;

    CReqFwdrForwardRequestTask( const IConfigDb* pCfg )
        : super( pCfg )
    { SetClassId( clsid( CReqFwdrForwardRequestTask ) ); }

    virtual gint32 RunTask();
    virtual gint32 OnTaskComplete( gint32 iRetVal );
};

class CBridgeForwardRequestTask :
    public CReqFwdrForwardRequestTask
{
    public:
    typedef CReqFwdrForwardRequestTask super;

    CBridgeForwardRequestTask( const IConfigDb* pCfg )
        : super( pCfg )
    { SetClassId( clsid( CBridgeForwardRequestTask ) ); }
    gint32 RunTask();
    gint32 ForwardRequestMH(
        IConfigDb* pReqCtx,
        DBusMessage* pReqMsg,           // [ in ]
        DMsgPtr& pRespMsg );            // [ out ]
};

class CReqFwdrFetchDataTask :
    public CIfInterceptTask
{
    public:
    typedef CIfInterceptTask super;

    CReqFwdrFetchDataTask( const IConfigDb* pCfg )
        : super( pCfg )
    { SetClassId( clsid( CReqFwdrFetchDataTask ) ); }

    virtual gint32 RunTask();
    virtual gint32 OnTaskComplete( gint32 iRetVal );
    gint32 OnServiceComplete( gint32 iRetVal );
};

class CBdgeProxyReadWriteComplete:
    public CIfInterceptTaskProxy
{
    public:
    typedef CIfInterceptTaskProxy super;

    // parameters:
    // propIfPtr
    // propEventSink
    // propIoMgr
    CBdgeProxyReadWriteComplete ( const IConfigDb* pCfg )
        : super( pCfg )
    { SetClassId( clsid( CBdgeProxyReadWriteComplete ) ); }
    virtual gint32 OnIrpComplete( PIRP pIrp );
    virtual gint32 RunTask()
    { return STATUS_PENDING; }
};

class CRpcRouterReqFwdr : public CRpcRouter
{
    // local registered matches
    // to check the validity of the outgoing requests
    // the key is the matchptr 
    // the value is reference count
    std::map< MatchPtr, gint32 > m_mapLocMatches;

    // this service listening on dbus to receive
    // the req from the client and send the
    // messages via tcp socket
    InterfPtr           m_pReqFwdr;
    protected:

    std::map< RegObjPtr, guint32 > m_mapRefCount;
    MatchPtr            m_pDBusSysMatch;

    gint32 OnPreStopLongWait(
        IEventSink* pCallback );

    gint32 OnRmtSvrOnline(
        IEventSink* pCallback,
        IConfigDb* pEvtCtx,
        HANDLE hPort )
    { return 0; }

    gint32 OnRmtSvrOffline(
        IEventSink* pCallback,
        IConfigDb* pEvtCtx,
        HANDLE hPort );

    public:
    typedef CRpcRouter super;
    CRpcRouterReqFwdr( const IConfigDb* pCfg );

    const EnumClsid GetIid() const
    { return iid( CRpcRouterReqFwdr ); }

    inline gint32 GetReqFwdr(
        InterfPtr& pIf )
    {
        pIf = m_pReqFwdr;
        return 0;
    }

    gint32 Start();

    bool HasReqForwarder() const
    { return true; }

    // check if the match is valid before it is
    // registered
    gint32 CheckMatch( IMessageMatch* pMatch );

    gint32 StartReqFwdr(
        InterfPtr& pReqFwder,
        IEventSink* pCallback );

    gint32 AddRefCount(
        guint32 dwPortId,
        const std::string& strSrcUniqName,
        const std::string& strSrcDBusName );

    gint32 DecRefCount(
        guint32 dwPortId,
        const std::string& strSrcUniqName,
        const std::string& strSrcDBusName );

    gint32 ClearRefCountByPortId(
        guint32 dwPortId,
        std::vector< std::string >& vecUniqNames );

    gint32 ClearRefCountByUniqName(
        const std::string& strUniqName,
        std::set< guint32 >& setPortIds );

    gint32 GetRefCountByPortId(
        guint32 dwPortId );

    gint32 GetRefCountByUniqName(
        const std::string& strUniqName );

    gint32 AddLocalMatch(
        IMessageMatch* pMatch );

    gint32 RemoveLocalMatch(
        IMessageMatch* pMatch );

    gint32 RemoveLocalMatchByPortId(
        guint32 dwPortId );

    gint32 RemoveLocalMatchByUniqName(
        const std::string& strUniqName,
        std::vector< MatchPtr >& vecMatches );

    // local match operation
    gint32 SetMatchOnline(
        IMessageMatch* pMatch,
        bool bOnline );

    gint32 IsMatchOnline(
        IMessageMatch* pMatch,
        bool& bOnline ) const;

    // filter the request with the local side
    // match table. The req cannot make to the
    // destination if it failes the tests against
    // the match table
    gint32 CheckReqToFwrd(
        IConfigDb* pReqCtx,
        DMsgPtr& pMsg,
        MatchPtr& pMatch );

    // filter the event with the local side
    // match table. The event cannot make to the
    // destination if it failes the tests against
    // the match tables
    gint32 CheckEvtToRelay(
        IConfigDb* pEvtCtx,
        DMsgPtr& pMsg,
        std::vector< MatchPtr >& vecMatches );

    gint32 OnRmtSvrEvent(
        EnumEventId iEvent,
        IConfigDb* pEvtCtx,
        HANDLE hPort );
};

class CRegObjectBridge
{
    guint32 m_dwPortId = 0;
    guint32 m_dwProxyPortId = 0;
    std::string m_strNodeName;

    public:
    CRegObjectBridge()
    {}

    CRegObjectBridge( const CRegObjectBridge& rhs )
    {
        m_dwProxyPortId = rhs.m_dwProxyPortId;
        m_dwPortId = rhs.m_dwPortId;
        m_strNodeName = rhs.m_strNodeName;
    }

    CRegObjectBridge( 
        const std::string& strNode,
        guint32 dwPortId,
        guint32 dwProxyId = 0 )
    {
        m_dwProxyPortId = dwProxyId;
        m_dwPortId = dwPortId;
        m_strNodeName = strNode;
    }

    gint32 SetProperties(
        const std::string& strNodeName,
        guint32 dwPortId,
        guint32 dwProxyId = 0 )
    {
        if( dwPortId == 0 ||
            strNodeName.empty() )
            return -EINVAL;

        m_dwPortId = dwPortId;
        m_dwProxyPortId = dwProxyId;
        return 0;
    }

    bool HasBridge() const
    { return true; }

    guint32 GetPortId() const
    { return m_dwPortId; }

    guint32 GetProxyPortId() const
    { return m_dwProxyPortId; }

    const std::string& GetNodeName() const
    { return m_strNodeName; }

    // test if the match belongs to this
    // registered interface
    gint32 IsMyMatch( IMessageMatch* pMatch )
    {
        gint32 ret = 0;
        CCfgOpenerObj oMatch( pMatch );
        do{
            ret = oMatch.IsEqual(
                propPortId, m_dwPortId );
            if( ERROR( ret ) )
                break;

            std::string strPath;
            ret = oMatch.GetStrProp(
                propRouterPath, strPath );
            if( ERROR( ret ) )
                break;

            std::string strNode;
            ret = CRpcRouter::GetNodeName(
                strPath, strNode );
            if( ERROR( ret ) )
                break;

            if( strNode!= m_strNodeName )
            {
                ret = ERROR_FALSE;
                break;
            }

        }while( 0 );

        return ret;
    }

    bool operator<( const CRegObjectBridge& rhs ) const
    {
        if( m_dwPortId < rhs.m_dwPortId )
            return true;

        if( m_strNodeName < rhs.m_strNodeName )
            return true;

        return false;
    }
};

class CRpcRouterBridge : public CRpcRouter
{
    // the key is the peer ip-addr plus peer port-number
    std::map< guint32, InterfPtr > m_mapPortId2Bdge;

    // remote registered matches
    // to route the outgoing events to the subscriber
    // the key is the matchptr 
    // the value is the reference count
    std::map< MatchPtr, gint32 > m_mapRmtMatches;

    // this interface listening on the dbus of
    // server side to relay the request from
    // remote client to the remote server object
    std::map< std::string, InterfPtr > m_mapReqProxies;

    gint32 OnRmtSvrOnline(
        IEventSink* pCallback,
        IConfigDb* pEvtCtx,
        HANDLE hPort );

    gint32 OnRmtSvrOfflineMH(
        IEventSink* pCallback,
        IConfigDb* pEvtCtx,
        HANDLE hPort );

    gint32 OnClearRemoteEventsComplete(
        IEventSink* pCallback,
        IEventSink*  pIoReq,
        IConfigDb* pReqCtx );

    gint32 BuildNodeMap();

    protected:

    std::map< std::string, guint32 > m_mapNode2Pid;
    std::map< std::string, CfgPtr > m_mapNode2ConnParams;

    std::map< CRegObjectBridge, gint32 > m_mapRefCount;

    gint32 ForwardModOnOfflineEvent(
        EnumEventId iEvent,
        const std::string& strModule );

    gint32 OnPreStopLongWait(
        IEventSink* pCallback );

    // methods of CRpcBaseOperations
    virtual gint32 OnModEvent(
        EnumEventId iEvent,
        const std::string& strModule );

    virtual gint32 OnRmtSvrOffline(
        IEventSink* pCallback,
        IConfigDb* pEvtCtx,
        HANDLE hPort );

    virtual gint32 OnPostStart(
        IEventSink* pContext );

    private:

    inline gint32 AddProxy(
        const std::string& strDest,
        InterfPtr& pIf )
    {
        if( !IsConnected() )
            return ERROR_STATE;

        CStdRMutex oRouterLock( GetLock() );
        std::map< std::string, InterfPtr >::iterator
            itr = m_mapReqProxies.find( strDest );
        if( itr == m_mapReqProxies.end() )
        {
            m_mapReqProxies[ strDest ] = pIf;
            return 0;
        }
        else
        {
            if( itr->second == pIf )
                return EEXIST;
        }
        return -EEXIST;
    }

    inline gint32 RemoveProxy(
        const std::string& strDest,
        const InterfPtr& pIf )
    {
        gint32 ret = -ENOENT;
        CStdRMutex oRouterLock( GetLock() );
        std::map< std::string, InterfPtr >::iterator
            itr = m_mapReqProxies.find( strDest );
        if( itr != m_mapReqProxies.end() )
        {
            if( pIf == itr->second )
            {
                m_mapReqProxies.erase( itr );
                ret = 0;
            }
        }
        return ret;
    }

    gint32 RemoveRemoteMatchByPath(
        const std::string& strUpstm,
        std::vector< MatchPtr >& vecMatches );

    public:

    typedef CRpcRouter super;
    CRpcRouterBridge( const IConfigDb* pCfg );

    using IFMAP_CITR =
        std::map< std::string, InterfPtr >::const_iterator;

    inline gint32 GetReqFwdrProxy(
        const std::string& strDest,
        InterfPtr& pIf ) const
    {
        CStdRMutex oRouterLock( GetLock() );
        IFMAP_CITR citr =
            m_mapReqProxies.find( strDest );

        if( citr == m_mapReqProxies.cend() )
            return -ENOENT;

        pIf = citr->second;
        return 0;
    }

    const EnumClsid GetIid() const
    { return iid( CRpcRouterBridge ); }

    gint32 AddRemoteMatch(
        IMessageMatch* pMatch );

    gint32 RemoveRemoteMatch(
        IMessageMatch* pMatch );

    gint32 RemoveRemoteMatchByPortId(
        guint32 dwPortId,
        std::vector< MatchPtr >& vecMatches );

    gint32 RemoveRemoteMatchByNodeName(
        const std::string& strNode,
        std::vector< MatchPtr >& vecMatches );

    inline gint32 GetObjAddr(
        IMessageMatch* pMatch,
        std::string& strDest )
    {
        if( pMatch == nullptr )
            return -EINVAL;

        CCfgOpenerObj oMatch( pMatch );
        gint32 ret = oMatch.GetStrProp(
            propDestDBusName, strDest );

        return ret;
    }

    inline gint32 GetReqFwdrProxy(
        IMessageMatch* pMatch,
        InterfPtr& pIf )
    {
        if( pMatch == nullptr )
            return -EINVAL;

        std::string strDest;
        gint32 ret = GetObjAddr( pMatch, strDest );
        if( ERROR( ret ) )
            return ret;

        return GetReqFwdrProxy( strDest, pIf );
    }

    inline gint32 AddConnParamsByNodeName(
        const std::string& strNode,
        const CfgPtr& pConnParams )
    {
        if( strNode.empty() )
            return -EINVAL;
        
        CStdRMutex oRouterLock( GetLock() );
        std::map< std::string, CfgPtr >::const_iterator 
            itr = m_mapNode2ConnParams.find(
                strNode );
        if( itr != m_mapNode2ConnParams.end() )
            return -EEXIST;

        m_mapNode2ConnParams[ strNode ] = pConnParams;
        return 0;
    }

    inline gint32 RemoveConnParamsByNodeName(
        const std::string& strNode )
    {
        if( strNode.empty() )
            return -EINVAL;
        
        CStdRMutex oRouterLock( GetLock() );
        size_t ret =
            m_mapNode2ConnParams.erase( strNode );

        if( ret == 0 )
            ret = -ENOENT;

        return 0;
    }

    inline gint32 GetConnParamsByNodeName(
        const std::string& strNode,
        CfgPtr& pConnParams ) const
    {
        if( strNode.empty() )
            return -EINVAL;
        
        CStdRMutex oRouterLock( GetLock() );
        std::map< std::string, CfgPtr >::const_iterator 
            itr = m_mapNode2ConnParams.find(
                strNode );
        if( itr == m_mapNode2ConnParams.end() )
            return -ENOENT;

        pConnParams = itr->second;
        return 0;
    }

    inline gint32 AddProxy(
        IMessageMatch* pMatch,
        InterfPtr& pIf )
    {
        if( pMatch == nullptr )
            return -EINVAL;

        std::string strDest;
        gint32 ret =
            GetObjAddr( pMatch, strDest );

        if( ERROR( ret ) )
            return ret;

        return AddProxy( strDest, pIf );
    }

    inline gint32 RemoveProxy(
        IMessageMatch* pMatch,
        const InterfPtr& pIf )
    {
        if( pMatch == nullptr )
            return -EINVAL;

        std::string strDest;
        gint32 ret =
            GetObjAddr( pMatch, strDest );

        if( ERROR( ret ) )
            return ret;

        return RemoveProxy( strDest, pIf );
    }
    gint32 BuildStartRecvTask(
        IMessageMatch* pMatch,
        TaskletPtr& pTask );

    gint32 BuildStartStopReqFwdrProxy( 
        IMessageMatch* pMatch,
        bool bStart,
        TaskletPtr& pTask );

    gint32 BuildEventRelayTask(
        IMessageMatch* pMatch,
        bool bAdd,
        TaskletPtr& pTask );

    gint32 BuildAddMatchTask(
        IMessageMatch* pMatch,
        bool bEnable,
        TaskletPtr& pTask );

    gint32 RunEnableEventTask(
        IEventSink* pCallback,
        IMessageMatch* pMatch );

    gint32 BuildDisEvtTaskGrp(
        IEventSink* pCallback,
        IMessageMatch* pMatch,
        TaskletPtr& pTask );

    gint32 RunDisableEventTask(
        IEventSink* pCallback,
        IMessageMatch* pMatch );

    gint32 GetBridge(
        guint32 dwPortId,
        InterfPtr& pIf );

    gint32 AddBridge( 
        IGenericInterface* pIf );

    gint32 RemoveBridge( 
        IGenericInterface* pIf );

    // filter the request with the remote side
    // match table. The req cannot make to the
    // destination if it failes the tests against
    // the match table
    gint32 CheckReqToRelay(
        IConfigDb* pReqCtx,
        DMsgPtr& pMsg,
        MatchPtr& pMatchHit );

    // filter the event with the remote side
    // match table. The event cannot make to the
    // destination if it failes the tests against
    // the match table
    gint32 CheckEvtToFwrd(
        IConfigDb* pReqCtx,
        DMsgPtr& pMsg,
        std::set< guint32 >& setPortIds );

    gint32 AddRefCount(
        const std::string& strNodeName,
        guint32 dwPortId,
        guint32 dwProxyPortId );

    gint32 DecRefCount(
        const std::string& strNodeName,
        guint32 dwPortId );

    gint32 ClearRefCountByPortId(
        guint32 dwPortId,
        std::vector< std::string >& vecNodes );

    gint32 ClearRefCountByNodeName(
        const std::string& strUniqName,
        std::set< guint32 >& setPortIds );

    // get bridgeproxy's refcount by the bridge's
    // portid
    gint32 GetRefCountByPortId(
        guint32 dwPortId );

    gint32 GetRefCountByNodeName(
        const std::string& strNodeName );

    using CRpcRouter::GetBridgeProxy;

    gint32 GetBridgeProxy(
        const std::string& strNode,
        InterfPtr& pIf );

    gint32 GetBridgeProxyByPath(
        const std::string& strPath,
        InterfPtr& pIf );

    // get bridgeproxy's portid by node name
    gint32 GetProxyIdByNodeName(
        const std::string& strNode,
        guint32& dwPortId ) const;

    gint32 OnRmtSvrEvent(
        EnumEventId iEvent,
        IConfigDb* pEvtCtx,
        HANDLE hPort );

    gint32 ClearRemoteEventsMH(
        IEventSink* pCallback,
        ObjPtr& pVecMatches,
        bool bForceClear );

    gint32 BuildRmtSvrEventMH(
        EnumEventId iEvent,
        IConfigDb* pCtx,
        CfgPtr& pEvtReq );
};

class CIfRouterMgrState : public CIfServerState
{
    public:
    typedef CIfServerState super;
    CIfRouterMgrState ( const IConfigDb* pCfg )
        : super( pCfg )
    { SetClassId( clsid( CIfRouterMgrState ) ); }
    gint32 SubscribeEvents();
};

class CRpcRouterManager : public CRpcRouter
{
    std::vector< InterfPtr > m_vecRoutersBdge;
    InterfPtr m_pRouterReqFwdr;
    guint32   m_dwRole = 1;


    protected:
    gint32 RebuildMatches();

    public:
    typedef CRpcRouter super;

    CRpcRouterManager( const IConfigDb* pCfg );
    ~CRpcRouterManager();

    const EnumClsid GetIid() const
    { return iid( CRpcRouterManager ); }

    gint32 GetRouters(
        std::vector< InterfPtr >& vecRouters ) const;

    gint32 Start();
    gint32 OnPreStop(
        IEventSink* pCallback ); 
};

DECLARE_AGGREGATED_SERVER(
     CRpcRouterManagerImpl,
     CRpcRouterManager );

DECLARE_AGGREGATED_SERVER(
     CRpcRouterReqFwdrImpl,
     CRpcRouterReqFwdr,
     CStatCountersServer ); 

DECLARE_AGGREGATED_SERVER(
     CRpcRouterBridgeImpl,
     CRpcRouterBridge,
     CStatCountersServer ); 

DECLARE_AGGREGATED_SERVER(
    CRpcReqForwarderImpl,
    CRpcReqForwarder,
    CStatCountersServer ); 

DECLARE_AGGREGATED_PROXY(
    CRpcReqForwarderProxyImpl,
    CRpcReqForwarderProxy,
    CStatCountersProxy );

#include "stmrelay.h"
#include "streammh.h"

DECLARE_AGGREGATED_SERVER(
    CRpcTcpBridgeImpl,
    CRpcTcpBridge,
    CStreamServerRelay,
    CStreamServerRelayMH,
    CStatCountersServer ); 

DECLARE_AGGREGATED_PROXY(
    CRpcTcpBridgeProxyImpl,
    CStreamProxyRelay,
    CRpcTcpBridgeProxy,
    CStatCountersProxy );

