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
 * =====================================================================================
 */
#include "defines.h"
#include "port.h"
#include "ifhelper.h"
#include "iftasks.h"
#include "counters.h"

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
        const IConfigDb* pCfg,
        IEventSink* pCallback ) = 0;

    // config db including `ip addr', `dst dbus
    // name' called on interface demand
    virtual gint32 CloseRemotePort(
        const IConfigDb* pCfg,
        IEventSink* pCallback ) = 0;

    // modified by the pdo on call to interface's
    // enable event and redirect to the router
    // config db including `ip addr', `event dbus
    // name' `obj name' `if name'
    virtual gint32 EnableRemoteEvent(
        IMessageMatch* pMatch,
        IEventSink* pCallback ) = 0;

    // modified by the pdo and redirect to the
    // router
    //
    // config db including `ip addr', `event dbus
    // name' `obj name' `if name'
    virtual gint32 DisableRemoteEvent(
        IMessageMatch* pMatch,
        IEventSink* pCallback ) = 0;

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
        const std::string& strDestIp,   // [ in ]
        DBusMessage* pReqMsg,           // [ in ]
        DMsgPtr& pRespMsg,              // [ out ]
        IEventSink* pCallback ) = 0;

    // client provide the following info in the
    //
    // pDataDesc: at least file name. and
    // operation commands if necessary
    //
    // pDataDesc, before going into the port
    // stack, includes information for
    // DBusMessage:
    //
    // propIfName
    // propObjPath
    // propDestDBusName
    // propSrcDBusName
    //
    // They are provided by the underlying
    // interface support methods.
    //
    /*virtual gint32 SendData(
        IConfigDb* pDataDesc,           // [in]
        gint32 fd,                      // [in]
        guint32 dwOffset,               // [in]
        guint32 dwSize,                 // [in]
        IEventSink* pCallback ) = 0;

    virtual gint32 FetchData(
        IConfigDb* pDataDesc,           // [in, out]
        gint32& fd,                     // [out]
        guint32& dwOffset,              // [out]
        guint32& dwSize,                // [out]
        IEventSink* pCallback ) = 0; */
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
        const std::string& strSrcIp,
        DBusMessage* pEventMsg,
        IEventSink* pCallback ) = 0;

};

class CRpcRouter;

class CRegisteredModule
    : public CConfigDb
{
    public:
    typedef CConfigDb super;

    CRegisteredModule( const IConfigDb* pCfg )
        : super( nullptr )
    { SetClassId( clsid( CRegisteredModule ) ); }

    gint32 SetProperties(
        const std::string& strSrcDBusName,
        const std::string& strSrcUniqName,
        const std::string& strIpAddr )
    {
        CCfgOpener oCfg( this );

        oCfg.SetStrProp(
            propSrcUniqName, strSrcUniqName );

        oCfg.SetStrProp(
            propSrcDBusName, strSrcDBusName );

        oCfg.SetStrProp(
            propIpAddr, strIpAddr );

        return 0;
    }

    // test if the match belongs to this
    // registered interface
    gint32 IsMyMatch( IMessageMatch* pMatch );
    bool operator<( CRegisteredModule& rhs ) const;
};

typedef CAutoPtr< clsid( CRegisteredModule ), CRegisteredModule > RegModPtr;

namespace std {

    template<>
    struct less<RegModPtr>
    {
        bool operator()(const RegModPtr& k1, const RegModPtr& k2) const
        {
             return ( *k1 < *k2 );
        }
    };
}

class CRpcInterfaceProxy :
    virtual public CInterfaceProxy,
    public IRpcReqProxyAsync,
    public IRpcEventRelay
{
    protected:
    CRpcRouter  *m_pParent;

    gint32 OnKeepAliveRelay(
        IEventSink* pTask );

    //gint32 OnKeepAliveTerm(
     //   IEventSink* pTask );

    gint32 OnKeepAliveOrig(
        IEventSink* pTask );

    public:
    typedef CInterfaceProxy super;
    CRpcInterfaceProxy( const IConfigDb* pCfg ) :
        super( pCfg )
    {;}

    virtual gint32 OnKeepAlive(
        IEventSink* pInvokeTask,
        EnumKAPhase bOrigin );

    CRpcRouter* GetParent() const
    { return m_pParent; }
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

    virtual gint32 CheckReqToFwrd(
        CRpcRouter* pRouter,
        const std::string& strIpAddr,
        DMsgPtr& pMsg,
        MatchPtr& pMatchHit ) = 0;

    virtual gint32 CheckSendDataToFwrd(
        IConfigDb* pDataDesc ) = 0;

    inline gint32 CheckFetchDatToFwrd(
        IConfigDb* pDataDesc )
    { return CheckSendDataToFwrd( pDataDesc ); }

    virtual gint32 BuildBufForIrpFwrdEvt(
        BufPtr& pBuf,
        IConfigDb* pReqCall ) = 0;

    gint32 SetupReqIrpFwrdEvt(
        IRP* pIrp,
        IConfigDb* pReqCall,
        IEventSink* pCallback );

    virtual gint32 OnKeepAliveRelay(
        IEventSink* pInvokeTask ) = 0;

    // CRpcInterfaceServer
    gint32 SendFetch_Server(
        IConfigDb* pDataDesc,           // [in, out]
        gint32& fd,                     // [out]
        guint32& dwOffset,              // [in,out]
        guint32& dwSize,                // [in,out]
        IEventSink* pCallback );

    public:

    typedef CAggInterfaceServer super;
    CRpcInterfaceServer( const IConfigDb* pCfg ) :
        super( pCfg )
    {;}

    virtual gint32 DoInvoke(
        DBusMessage* pReqMsg,
        IEventSink* pCallback );

    gint32 ForwardRequest(
        const std::string& strIpAddr,
        DBusMessage* pFwdrMsg,
        DMsgPtr& pRespMsg,
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

    // CRpcInterfaceServer
    virtual gint32 SendData_Server(
        IConfigDb* pDataDesc,           // [in]
        gint32 fd,                      // [in]
        guint32 dwOffset,               // [in]
        guint32 dwSize,                 // [in]
        IEventSink* pCallback )
    {
        return SendFetch_Server( pDataDesc,
            fd, dwOffset, dwSize, pCallback );
    }

    // CRpcInterfaceServer
    virtual gint32 FetchData_Server(
        IConfigDb* pDataDesc,           // [in, out]
        gint32& fd,                     // [out]
        guint32& dwOffset,              // [in,out]
        guint32& dwSize,                // [in,out]
        IEventSink* pCallback )
    {
        return SendFetch_Server( pDataDesc,
            fd, dwOffset, dwSize, pCallback );
    }

    // CRpcInterfaceServer
    virtual gint32 SendData(
        IConfigDb* pDataDesc,           // [in]
        gint32 fd,                      // [in]
        guint32 dwOffset,               // [in]
        guint32 dwSize,                 // [in]
        IEventSink* pCallback = nullptr )
    {
        if( pDataDesc == nullptr )
            return -EINVAL;

        CCfgOpener oCfg( pDataDesc );

        oCfg[ propMethodName ] =
            std::string( SYS_METHOD_SENDDATA );

        return SendData_Server( pDataDesc,
            fd, dwOffset, dwSize, pCallback );
    }

    // CRpcInterfaceServer
    virtual gint32 FetchData(
        IConfigDb* pDataDesc,           // [in, out]
        gint32& fd,                     // [out]
        guint32& dwOffset,              // [in,out]
        guint32& dwSize,                // [in,out]
        IEventSink* pCallback = nullptr )
    {
        if( pDataDesc == nullptr )
            return -EINVAL;

        CCfgOpener oCfg( pDataDesc );

        oCfg[ propMethodName ] =
            std::string( SYS_METHOD_FETCHDATA );

        return FetchData_Server( pDataDesc,
            fd, dwOffset, dwSize, pCallback );
    }
};

class CRpcReqForwarder :
     public CRpcInterfaceServer
{
    protected:

    // the cfgptr contains the following
    // properties
    // propIpAddr
    // propSrcUniqName
    // propSrcDBusName
    std::map< RegModPtr, guint32 > m_mapRefCount;

    // check if the match is valid before it is
    // registered
    gint32 CheckMatch( IMessageMatch* pMatch );

    virtual gint32 BuildBufForIrpFwrdEvt(
        BufPtr& pBuf,
        IConfigDb* pReqCall );

    virtual gint32 CheckSendDataToFwrd(
        IConfigDb* pDataDesc );

    virtual gint32 OnKeepAliveRelay(
        IEventSink* pInvokeTask );

    gint32 EnableDisableEvent(
        IMessageMatch* pMatch,
        IEventSink* pCallback,
        bool bEnable );

    public:

    static CfgPtr InitCfg( const IConfigDb* pCfg );
    typedef CRpcInterfaceServer super;

    CRpcReqForwarder(
        const IConfigDb* pCfg );

    gint32 InitUserFuncs();

    gint32 AddRefCount(
        const std::string& strIpAddr,
        const std::string& strSrcUniqName,
        const std::string& strSrcDBusName );

    gint32 DecRefCount(
        const std::string& strIpAddr,
        const std::string& strSrcUniqName,
        const std::string& strSrcDBusName );

    virtual const EnumClsid GetIid() const
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
    // propIpAddr: the remote address the
    // connection will be made
    //
    // propSrcDBusName: the local source who send
    // out the request
    //
    // propObjPath
    // propIfName
    // 
    virtual gint32 OpenRemotePort(
        const IConfigDb* pCfg,
        IEventSink* pCallback );

    // active disconnecting
    virtual gint32 CloseRemotePort(
        const IConfigDb* pCfg,
        IEventSink* pCallback )
    { return -ENOTSUP; }

    // mandatory properties within the pMatch
    //
    // propIpAddr: address of the remote server
    // propIfName: interface name
    // propObjPath: object path
    // propDestDBusName: destination dbus name
    //
    virtual gint32 EnableRemoteEvent(
        IMessageMatch* pMatch,
        IEventSink* pCallback );

    virtual gint32 DisableRemoteEvent(
        IMessageMatch* pMatch,
        IEventSink* pCallback );

    virtual gint32 ForwardEvent(
        const std::string& strSrcIp,
        DBusMessage* pEventMsg,
        IEventSink* pCallback );

    virtual gint32 CheckReqToFwrd(
        CRpcRouter* pRouter,
        const std::string& strIpAddr,
        DMsgPtr& pMsg,
        MatchPtr& pMatchHit );

    gint32 SendData_Server(
        IConfigDb* pDataDesc,           // [in]
        gint32 fd,                      // [in]
        guint32 dwOffset,               // [in]
        guint32 dwSize,                 // [in]
        IEventSink* pCallback );
};

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

class CRpcReqForwarderProxy :
    public CRpcInterfaceProxy
{
    protected:
    // tcp port handle and the related
    // informations
    static CfgPtr InitCfg( const IConfigDb* pCfg );

    gint32 SetupReqIrpEnableEvt( IRP* pIrp,
        IConfigDb* pReqCall,
        IEventSink* pCallback );

    gint32 SetupReqIrpFwrdReq( IRP* pIrp,
        IConfigDb* pReqCall,
        IEventSink* pCallback );

    public:

    typedef CRpcInterfaceProxy super;
    CRpcReqForwarderProxy( const IConfigDb* pCfg );
    ~CRpcReqForwarderProxy();

    gint32 InitUserFuncs();

    virtual gint32 SetupReqIrp( IRP* pIrp,
        IConfigDb* pReqCall,
        IEventSink* pCallback );

    virtual gint32 OpenRemotePort(
        const IConfigDb* pCfg,
        IEventSink* pCallback )
    { return -ENOTSUP; }

    // active disconnecting
    virtual gint32 CloseRemotePort(
        const IConfigDb* pCfg,
        IEventSink* pCallback )
    { return -ENOTSUP; }

    virtual gint32 EnableRemoteEvent(
        IMessageMatch* pMatch,
        IEventSink* pCallback )
    { return -ENOTSUP; }

    virtual gint32 DisableRemoteEvent(
        IMessageMatch* pMatch,
        IEventSink* pCallback )
    { return -ENOTSUP; }

    virtual gint32 ForwardRequest(
        const std::string& strDestIp,   // [ in ]
        DBusMessage* pReqMsg,           // [ in ]
        DMsgPtr& pRespMsg,              // [ out ]
        IEventSink* pCallback );

    virtual gint32 ForwardEvent(
        const std::string& strSrcIp,
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
};

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

    protected:
    CRpcServices* m_pParentIf;

    gint32 ReadWriteStream(
        gint32 iStreamId,
        CBuffer* pSrcBuf,
        guint32 dwSize,
        IEventSink* pCallback,
        bool bRead );

};

class CRpcTcpBridge:
    public CRpcInterfaceServer,
    public CRpcTcpBridgeShared
{
    gint32 EnableRemoteEventInternal(
        IMessageMatch* pMatch,
        IEventSink* pCallback,
        bool bEnable );

    virtual gint32 BuildBufForIrpFwrdEvt(
        BufPtr& pBuf,
        IConfigDb* pReqCall );

    virtual gint32 OnKeepAliveRelay(
        IEventSink* pTask );

    gint32 SetupReqIrpListening( IRP* pIrp,
        IConfigDb* pReqCall,
        IEventSink* pCallback );

    gint32 SetupReqIrpOnProgress( IRP* pIrp,
        IConfigDb* pReqCall,
        IEventSink* pCallback );

    gint32 OpenStream_Server(
        IEventSink* pCallback,
        gint32 iPeerStm,
        guint32 wProtocol );

    gint32 CloseStream_Server(
        IEventSink* pCallback,
        gint32 iStreamId );

    gint32 CheckSendDataToRelay(
        IConfigDb* pDataDesc );

    // CRpcTcpBridge
    gint32 SendFetch_Server(
        IConfigDb* pDataDesc,           // [in, out]
        gint32& fd,                     // [out]
        guint32& dwOffset,              // [in,out]
        guint32& dwSize,                // [in,out]
        IEventSink* pCallback );

    public:
    typedef CRpcInterfaceServer super;
    CRpcTcpBridge( const IConfigDb* pCfg );
    ~CRpcTcpBridge();

    virtual const EnumClsid GetIid() const
    { return iid( CRpcTcpBridge ); }

    virtual gint32 OpenRemotePort(
        const IConfigDb* pCfg,
        IEventSink* pCallback )
    { return -ENOTSUP; }

    virtual gint32 SetupReqIrp( IRP* pIrp,
        IConfigDb* pReqCall,
        IEventSink* pCallback );

    // active disconnecting
    virtual gint32 CloseRemotePort(
        const IConfigDb* pCfg,
        IEventSink* pCallback )
    { return -ENOTSUP; }

    virtual gint32 EnableRemoteEvent(
        IMessageMatch* pMatch,
        IEventSink* pCallback );

    virtual gint32 DisableRemoteEvent(
        IMessageMatch* pMatch,
        IEventSink* pCallback );

    virtual gint32 ForwardEvent(
        const std::string& strSrcIp,
        DBusMessage* pEventMsg,
        IEventSink* pCallback );

    virtual gint32 CheckReqToFwrd(
        CRpcRouter* pRouter,
        const std::string& strIpAddr,
        DMsgPtr& pMsg,
        MatchPtr& pMatchHit );

    virtual gint32 CheckSendDataToFwrd(
        IConfigDb* pDataDesc );

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

    // CRpcInterfaceBase methods
    virtual gint32 StartEx(
        IEventSink* pCallback );

    virtual gint32 SendResponse(
        IConfigDb* pReqMsg,
        CfgPtr& pRespData );

    // called to set the response to send back
    virtual gint32 SetResponse(
        IEventSink* pTask,
        CfgPtr& pRespData );

    // to send out a progress hint
    gint32 OnProgressNotify(
        guint32 dwProgress,
        guint64 iTaskId );

    // CRpcTcpBridge
    virtual gint32 SendData_Server(
        IConfigDb* pDataDesc,           // [in]
        gint32 fd,                      // [in]
        guint32 dwOffset,               // [in]
        guint32 dwSize,                 // [in]
        IEventSink* pCallback )
    {
        return SendFetch_Server( pDataDesc,
            fd, dwOffset, dwSize, pCallback );
    }

    // CRpcTcpBridge
    virtual gint32 FetchData_Server(
        IConfigDb* pDataDesc,           // [in, out]
        gint32& fd,                     // [out]
        guint32& dwOffset,              // [in,out]
        guint32& dwSize,                // [in,out]
        IEventSink* pCallback )
    {
        return SendFetch_Server( pDataDesc,
            fd, dwOffset, dwSize, pCallback );
    }

};

class CRpcTcpBridgeProxy :
    public CRpcInterfaceProxy,
    public CRpcTcpBridgeShared
{
    gint32 EnableRemoteEventInternal(
        IMessageMatch* pMatch,
        IEventSink* pCallback,
        bool bEnable );

    gint32 FillRespDataFwrdReq(
        IRP* pIrp, CfgPtr& pResp );

    gint32 BuildBufForIrpFwrdReq(
        BufPtr& pBuf,
        IConfigDb* pReqCall );

    gint32 SetupReqIrpListening(
        IRP* pIrp,
        IConfigDb* pReqCall,
        IEventSink* pCallback );

    gint32 OpenStream_Proxy(
        guint32 wProtocol,
        gint32& iStreamId,
        IEventSink* pCallback );

    gint32 CloseStream_Proxy( gint32 iStreamId,
        IEventSink* pCallback );

    gint32 SendFetch_Proxy(
        IConfigDb* pDataDesc,           // [in, out]
        gint32& fd,                     // [out]
        guint32& dwOffset,              // [out]
        guint32& dwSize,                // [out]
        IEventSink* pCallback );

    public:

    typedef CRpcInterfaceProxy super;

    CRpcTcpBridgeProxy(
        const IConfigDb* pCfg );

    virtual gint32 SetupReqIrp( IRP* pIrp,
        IConfigDb* pReqCall,
        IEventSink* pCallback );

    virtual gint32 OpenRemotePort(
        const IConfigDb* pCfg,
        IEventSink* pCallback )
    { return -ENOTSUP; }

    // RPC methods
    // active disconnecting
    virtual gint32 CloseRemotePort(
        const IConfigDb* pCfg,
        IEventSink* pCallback )
    { return -ENOTSUP; }

    virtual gint32 EnableRemoteEvent(
        IMessageMatch* pMatch,
        IEventSink* pCallback );

    virtual gint32 DisableRemoteEvent(
        IMessageMatch* pMatch,
        IEventSink* pCallback );

    virtual gint32 ForwardRequest(
        const std::string& strDestIp,   // [ in ]
        DBusMessage* pReqMsg,           // [ in ]
        DMsgPtr& pRespMsg,              // [ out ]
        IEventSink* pCallback );

    virtual gint32 ForwardEvent(
        const std::string& strSrcIp,
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

    // CRpcInterfaceBase methods
    virtual gint32 StartEx(
        IEventSink* pCallback );

    virtual gint32 SendData_Proxy(
        IConfigDb* pDataDesc,           // [in]
        gint32 fd,                      // [in]
        guint32 dwOffset,               // [in]
        guint32 dwSize,                 // [in]
        IEventSink* pCallback );

    virtual gint32 FetchData_Proxy(
        IConfigDb* pDataDesc,           // [in, out]
        gint32& fd,                     // [out]
        guint32& dwOffset,              // [out]
        guint32& dwSize,                // [out]
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

    gint32 OnRmtSvrOffline( IEventSink* pCallback );
    gint32 OnRmtModOffline( IEventSink* pCallback,
        const std::string& strModName );

    gint32 OnInvalidStreamId( IEventSink* pCallback,
        gint32 iStreamId,
        gint32 iPeerStreamId,
        guint32 dwSeqNo,
        guint32 dwProtocol );

    // to respond to a progress hint notification from
    // server
    gint32 OnProgressNotify(
        guint32 dwProgress,
        guint64 iTaskId );
};

class CRpcRouter : public IService
{
    // pair::first is the Proxy interface over the
    // CTcpStreamPdo
    //
    // pair::second is the server interface over
    // the CTcpStreamPdo
    typedef std::pair< InterfPtr, InterfPtr > NetIfPair;

    // pair::first is the ip-addr, and the
    // pair::second is the reference count
    typedef std::pair< std::string, gint32 > IfReference;

    // the key is the peer ip-addr and the value is the
    // pair of the tcp bridge and tcp bridge proxy
    std::map< std::string, NetIfPair > m_mapIp2NetIfs;

    // local registered matches
    // to check the validity of the outgoing requests
    // the key is the matchptr 
    // the value is reference count
    std::map< MatchPtr, gint32 > m_mapLocMatches;

    // remote registered matches
    // to route the outgoing events to the subscriber
    // the key is the matchptr 
    // the value is the reference count
    std::map< MatchPtr, gint32 > m_mapRmtMatches;

    // this service listening on dbus to receive
    // the req from the client and send the
    // messages via tcp socket
    InterfPtr           m_pReqFwdr;

    // this interface listening on the dbus of
    // server side to relay the request from
    // remote client to the remote server object
    InterfPtr           m_pReqFwdrProxy;

    CIoManager*         m_pIoMgr;
    mutable stdrmutex   m_oLock;
    std::vector< EnumPropId >   m_vecTopicList;

    gint32 AddRemoveMatch(
        IMessageMatch* pMatch,
        bool bAdd,
        bool bRemote,
        IEventSink* pCallback );

    gint32 GetBridgeIfInternal(
        const std::string& strIpAddr,
        InterfPtr& pIf,
        bool bProxy );

    public:

    typedef std::pair< std::multimap<MatchPtr, std::string>::iterator,
        std::multimap<MatchPtr, std::string>::iterator> MIPAIR;

    typedef IServiceEx super;

    CRpcRouter( const IConfigDb* pCfg );
    ~CRpcRouter();

    inline CIoManager* GetIoMgr() const
    { return m_pIoMgr; }

    stdrmutex& GetLock() const
    { return m_oLock; }

    gint32 AddLocalMatch(
        IMessageMatch* pMatch );

    gint32 RemoveLocalMatch(
        IMessageMatch* pMatch );

    gint32 AddRemoteMatch(
        IMessageMatch* pMatch,
        IEventSink* pCallback );

    gint32 RemoveRemoteMatch(
        IMessageMatch* pMatch,
        IEventSink* pCallback );

    gint32 GetBridge(
        const std::string& strIpAddr,
        InterfPtr& pIf );

    gint32 GetBridgeProxy(
        const std::string& strIpAddr,
        InterfPtr& pIf );

    inline gint32 GetReqFwdrProxy(
        InterfPtr& pIf )
    {
        pIf = m_pReqFwdrProxy;
        return 0;
    }

    inline gint32 GetReqFwdr(
        InterfPtr& pIf )
    {
        pIf = m_pReqFwdr;
        return 0;
    }

    gint32 AddBridgePair( 
        const std::string& strIpAddr,
        IGenericInterface* pProxy,
        IGenericInterface* pServer );
        
    gint32 StartReqFwder(
        InterfPtr& pReqFwder,
        IEventSink* pCallback,
        bool bProxy );

    gint32 SubscribeEvents();
    gint32 UnsubscribeEvents();

    gint32 Start();
    gint32 Stop();

    gint32 Start(
        IEventSink* pCallback );

    gint32 Stop(
        IEventSink* pCallback );

    gint32 OnEvent( EnumEventId iEvent,
        guint32 dwParam1,
        guint32 dwParam2,
        guint32* pData );

    gint32 OnRmtSvrOnline(
        EnumEventId iEvent,
        const std::string& strIpAddr,
        IPort* pPort );

    gint32 OnRmtSvrOffline(
        EnumEventId iEvent,
        const std::string& strIpAddr,
        IPort* pPort );

    // filter the request with the remote side
    // match table. The req cannot make to the
    // destination if it failes the tests through
    // the match table
    gint32 CheckReqToRelay(
        const std::string &strIpAddr,
        DMsgPtr& pMsg,
        MatchPtr& pMatchHit );

    // filter the request with the local side
    // match table. The req cannot make to the
    // destination if it failes the tests through
    // the match table
    gint32 CheckReqToFwrd(
        const std::string &strIpAddr,
        DMsgPtr& pMsg,
        MatchPtr& pMatch );

    // filter the event with the local side
    // match table. The req cannot make to the
    // destination if it failes the tests through
    // the match table
    gint32 CheckEvtToFwrd(
        const std::string& strIpAddr,
        DMsgPtr& pMsg,
        std::vector< MatchPtr > vecMatches );
};

class CReqFwdrOpenRmtPortTask 
    : public CIfRetryTask
{

    typedef enum{
        stateInitialized,
        stateStartBridgeProxy,
        stateStartBridgeServer,
        stateDone
    } TaskState;

    TaskState m_iState;
    InterfPtr   m_pProxy;
    InterfPtr   m_pServer;

    // actively create a CRpcTcpPdo
    gint32 CreateInterface( InterfPtr& pIf );
    gint32 RunTaskInternal( gint32 iRetVal );

    // will call the CInterfaceServer's
    // OnServiceComplete to finish the async call
    virtual gint32 OnServiceComplete( gint32 iRetVal );

    public:
    typedef CIfRetryTask super;

    CReqFwdrOpenRmtPortTask ( const IConfigDb* pCfg )
        : CIfRetryTask( pCfg )
    {
        SetClassId( clsid( CReqFwdrOpenRmtPortTask ) );
        m_iState = stateInitialized;
    }

    gint32 AdvanceState()
    {
        gint32 ret = 0;
        switch( m_iState )
        {
        case stateInitialized:
            {
                m_iState = stateStartBridgeProxy;
                break;
            }
        case stateStartBridgeProxy:
            {
                m_iState = stateStartBridgeServer;
                break;
            }
        case stateStartBridgeServer:
            {
                m_iState = stateDone;
                break;
            }
        default:
            {
                ret = ERROR_STATE;
                break;
            }
        }
        return ret;
    }
    virtual gint32 RunTask();
    virtual gint32 OnTaskComplete( gint32 iRetVal );
};

class CRouterOpenRmtPortTask :
    public CReqFwdrOpenRmtPortTask
{
    public:

    // passively open a CRpcTcpPdo, when the port has
    // already created.
    typedef CReqFwdrOpenRmtPortTask super;
    CRouterOpenRmtPortTask( const IConfigDb* pCfg )
        : super( pCfg )
    {
        SetClassId( clsid( CRouterOpenRmtPortTask ) ); 
    }
    virtual gint32 OnServiceComplete( gint32 iRetVal )
    { return 0; }
};

class CRouterEnableEventRelayTask :
    public CIfInterceptTaskProxy
{
    public:
    typedef CIfInterceptTaskProxy super;

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

    gint32 EnableEventRelay(
        CRpcServices* pIf,
        IMessageMatch* pMatch,
        bool bEnable );
};

class CReqFwdrEnableRmtEventTask :
    public CIfInterceptTask
{
    public:
    typedef CIfInterceptTask super;

    CReqFwdrEnableRmtEventTask( const IConfigDb* pCfg )
        : super( pCfg )
    { SetClassId( clsid( CReqFwdrEnableRmtEventTask ) ); }

    virtual gint32 RunTask();
    virtual gint32 OnTaskComplete( gint32 iRetVal );
};

class CBridgeAddRemoteMatchTask :
    public CIfInterceptTask
{
    public:
    typedef CIfInterceptTask super;

    // parameters:
    // 0: bAdd
    // 1: pointer to a IMessageMatch object
    // propIfPtr
    // propEventSink
    // propIoMgr
    CBridgeAddRemoteMatchTask( const IConfigDb* pCfg )
        : super( pCfg )
    { SetClassId( clsid( CBridgeAddRemoteMatchTask ) ); }

    virtual gint32 RunTask();
    virtual gint32 OnTaskComplete( gint32 iRetVal );
    gint32 AddRemoteMatchInternal(
        CRpcRouter* pRouter,
        IMessageMatch* pMatch,
        bool bEnable );
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

class CReqFwdrSendDataTask :
    public CReqFwdrForwardRequestTask
{
    public:
    typedef CReqFwdrForwardRequestTask super;

    CReqFwdrSendDataTask( const IConfigDb* pCfg )
        : super( pCfg )
    { SetClassId( clsid( CReqFwdrSendDataTask ) ); }

    virtual gint32 RunTask();
    virtual gint32 OnTaskComplete( gint32 iRetVal );
    gint32 OnServiceComplete( gint32 iRetVal );
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


// SEND_DATA tasks on the proxy side
// the first task in SendData_Proxy's call
class CBdgeProxyOpenStreamTask :
    public CIfInterceptTask
{
    public:
    typedef CIfInterceptTask super;
    // parameters:
    // propIfPtr
    // propEventSink: the pointer to the CReqFwdrSendDataTask to intercept
    // propIoMgr
    // propReqPtr: contains the SendData_Proxy's input parameters
    CBdgeProxyOpenStreamTask( const IConfigDb* pCfg )
        : super( pCfg )
    { SetClassId( clsid( CBdgeProxyOpenStreamTask ) ); }

    virtual gint32 RunTask();
    virtual gint32 OnTaskComplete( gint32 iRetVal );
    gint32 EmitSendDataRequest( gint32 iStmId );
};

#define TMP_FILE_PREFIX "/tmp/SendData-"

class CBdgeReadWriteStreamTask :
    public CIfInterceptTask
{

    protected:
    guint32 m_dwSize;
    guint32 m_dwCurSize;
    guint32 m_dwCurOffset;
    guint32 m_dwOffset;
    gint32  m_iFd;
    gint32  m_iStmId;
    CfgPtr  m_pDataDesc;
    std::string  m_strFile;
    bool    m_bTempFile;

    public:
    typedef CIfInterceptTask super;

    CBdgeReadWriteStreamTask( const IConfigDb* pCfg )
        :super( pCfg ),
        m_dwSize( 0 ),
        m_dwCurSize( 0 ),
        m_dwCurOffset( 0 ),
        m_dwOffset( 0 ),
        m_iFd( -1 ),
        m_iStmId( -1 ),
        m_bTempFile( false )
    {}

    ~CBdgeReadWriteStreamTask()
    {
        if( m_iFd >= 0 )
            close( m_iFd );

        if( m_bTempFile )
            unlink( m_strFile.c_str() );
    }

    gint32 ReadStream( bool bFinished );
    gint32 WriteStream( bool bFinished );
    gint32 OpenTempFile();
};

class CBdgeProxyStartSendTask :
    public CBdgeReadWriteStreamTask 
{
    typedef enum{
        statInit,
        statSendReq,
        statWaitNotify,
        statWriteStream,
        statReadStream,
        statWaitResp,
        statComplete,

    } EnumTaskStat;

    EnumTaskStat m_dwTaskState;

    ObjPtr  m_pCallback;

    inline EnumTaskStat GetState() const
    { return m_dwTaskState; }

    inline void SetState( EnumTaskStat iState )
    { m_dwTaskState = iState; }

    public:
    typedef CBdgeReadWriteStreamTask super;
    // parameters:
    // propIfPtr
    // propEventSink: the pointer to the CReqFwdrSendDataTask to intercept
    // propIoMgr
    // propReqPtr: contains the SendData_Proxy's input parameters
    CBdgeProxyStartSendTask( const IConfigDb* pCfg )
        : super( pCfg ),
        m_dwTaskState( statInit )
    {SetClassId( clsid( CBdgeProxyStartSendTask  ) ); }

    virtual gint32 RunTask();
    virtual gint32 OnTaskComplete( gint32 iRetVal );

    gint32 CloseStream();

    virtual gint32 OnNotify( guint32 dwEvent,
        guint32 dwParam1,
        guint32 dwParam2,
        guint32* pData );

    // to handle the completion from the stream
    // read
    virtual gint32 OnIrpComplete( PIRP pIrp );
    gint32 OnTaskCompleteWithError( gint32 iRetVal );
};

class CBdgeStartRecvDataTask :
    public CBdgeReadWriteStreamTask 
{
    typedef enum{
        statInit,
        statReadStream,
        statRelayReq,
        statComplete,

    } EnumTaskStat;

    ObjPtr  m_pCallback;
    guint64 m_qwTaskId;
    guint32 m_dwProgress;
    EnumTaskStat m_dwTaskState;

    // parameters:
    // propIfPtr
    // propEventSink: the pointer to the CReqFwdrSendDataTask to intercept
    // propIoMgr
    // propReqPtr: contains the SendData_Server's input parameters
    // propTaskId:
    public:
    typedef CBdgeReadWriteStreamTask super;
    CBdgeStartRecvDataTask( const IConfigDb* pCfg )
        : super( pCfg ),
        m_qwTaskId( 0 ),
        m_dwProgress( 0 ),
        m_dwTaskState( statInit )
    { SetClassId( clsid( CBdgeStartRecvDataTask ) ); }

    ~CBdgeStartRecvDataTask()
    {
        if( m_iFd != -1 )
        {
            close( m_iFd );
            m_iFd = -1;
        }
    }

    virtual gint32 RunTask();
    virtual gint32 OnTaskComplete( gint32 iRetVal );
    gint32 OnTaskCompleteWithError( gint32 iRetVal );

    virtual gint32 OnIrpComplete( PIRP pIrp );
    gint32 RelaySendData();
    gint32 SendProgressNotify();
    inline EnumTaskStat GetState() const
    { return m_dwTaskState; }

    inline void SetState( EnumTaskStat iState )
    { m_dwTaskState = iState; }
};

class CBdgeStartFetchDataTask :
    public CBdgeReadWriteStreamTask  
{
    protected:

    typedef enum{
        statInit,
        statRelayReq,
        statWriteStream,
        statComplete,

    } EnumTaskStat;

    ObjPtr  m_pCallback;
    guint64 m_qwTaskId;
    guint32 m_dwProgress;
    EnumTaskStat m_dwTaskState;

    public:

    typedef CBdgeReadWriteStreamTask super;
    // parameters:
    // propIfPtr
    // propEventSink: the pointer to the CReqFwdrSendDataTask to intercept
    // propIoMgr
    // propReqPtr: contains the SendData_Server's input parameters
    CBdgeStartFetchDataTask( const IConfigDb* pCfg )
        : super( pCfg )
    { SetClassId( clsid( CBdgeStartFetchDataTask ) ); }

    virtual gint32 RunTask();
    virtual gint32 OnTaskComplete( gint32 iRetVal );
    virtual gint32 OnIrpComplete( PIRP pIrp );
    gint32 StartSendbackData();

    gint32 SendProgressNotify();

    inline EnumTaskStat GetState() const
    { return m_dwTaskState; }

    inline void SetState( EnumTaskStat iState )
    { m_dwTaskState = iState; }
    gint32 RelayFetchData();
};

DECLARE_AGGREGATED_SERVER(
    CRpcReqForwarderImpl,
    CRpcReqForwarder,
    CStatCountersServer ); 

DECLARE_AGGREGATED_SERVER(
    CRpcTcpBridgeImpl,
    CRpcTcpBridge,
    CStatCountersServer ); 

DECLARE_AGGREGATED_PROXY(
    CRpcReqForwarderProxyImpl,
    CRpcReqForwarderProxy,
    CStatCountersProxy );

DECLARE_AGGREGATED_PROXY(
    CRpcTcpBridgeProxyImpl,
    CRpcTcpBridgeProxy,
    CStatCountersProxy );
