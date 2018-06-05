/*
 * =====================================================================================
 *
 *       Filename:  proxy.h
 *
 *    Description:  the proxy interface for dbus communication
 *
 *        Version:  1.0
 *        Created:  02/27/2016 11:06:12 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi(woodhead99@gmail.com )
 *   Organization:
 *
 * =====================================================================================
 */

#pragma once

#include <sys/time.h>
#include "defines.h"
#include "utils.h"
#include "configdb.h"
#include "msgmatch.h"
#include "ifstat.h"

#define ICT_MAX_IRPS 100
#define ICT_MAX_IFS 10

#define IFSTATE_OPENPORT_RETIRES        1
#define IFSTATE_OPENPORT_INTERVAL       180
#define IFSTATE_ENABLE_EVENT_TIMEOUT    IFSTATE_OPENPORT_INTERVAL
#define IFSTATE_DEFAULT_IOREQ_TIMEOUT   IFSTATE_OPENPORT_INTERVAL

// CallFlags for SubmitIoRequest & InvokeMethod
#define CF_MESSAGE_TYPE_MASK            0x07
#define CF_ASYNC_CALL                   0x08 
#define CF_WITH_REPLY                   0x10

// the server will send keep-alive heartbeat to
// the client at the specified interval specified
// by propIntervalSec
#define CF_KEEP_ALIVE                   0x20

// the request to send is not a dbus message,
// instead a ConfigDb
#define CF_NON_DBUS                     0x40

#define MAX_BYTES_PER_TRANSFER ( 1024 * 1024 )
#define MAX_BYTES_PER_FILE     ( 512 * 1024 * 1024 )

class IGenericInterface :
    public IServiceEx
{
    public:

    typedef IServiceEx super;

    virtual EnumIfState GetState() const = 0;

};

typedef CAutoPtr< clsid( Invalid ), IGenericInterface > InterfPtr;

#include "iftasks.h"

class IInterfaceCommands
{
    public:

    // pCallback is the callback if STATUS_PENDING
    // is returned
    //
    // Start and Stop will come from IServiceEx
    virtual gint32 Pause(
        IEventSink* pCallback = nullptr ) = 0;

    virtual gint32 Resume(
        IEventSink* pCallback = nullptr ) = 0;

    virtual gint32 Shutdown(
        IEventSink* pCallback = nullptr ) = 0;

    virtual gint32 Restart(
        IEventSink* pCallback = nullptr ) = 0;
};

class CGenericInterface :
    public IGenericInterface
{
    mutable stdrmutex       m_oLock;

    protected:
    IfStatPtr               m_pIfStat;

    public:

    typedef IGenericInterface   super;

    inline stdrmutex& GetLock() const
    { return m_oLock; }

    inline CIoManager* GetIoMgr() const
    {
        return ( ( CInterfaceState* )m_pIfStat )->GetIoMgr();
    }

    CGenericInterface( const IConfigDb* pCfg )
    {;}

    virtual gint32 GetProperty(
        gint32 iProp, CBuffer& pVal ) const
    {
        return m_pIfStat->GetProperty( iProp, pVal );
    }

    virtual gint32 SetProperty(
        gint32 iProp, const CBuffer& pVal )
    {
        return m_pIfStat->SetProperty( iProp, pVal );
    }

    gint32 RemoveProperty( gint32 iProp )
    {
        return m_pIfStat->RemoveProperty( iProp );
    }

    gint32 CopyProp( gint32 iProp, CObjBase* pObj )
    {
        return m_pIfStat->CopyProp( iProp, pObj );
    }

    gint32 SetStateOnEvent( EnumEventId iEvent )
    {
        return m_pIfStat->SetStateOnEvent( iEvent );
    }

    virtual EnumIfState GetState() const
    {
        return m_pIfStat->GetState();
    }

    inline guint32 GetPortHandle() const
    {
        return m_pIfStat->GetHandle();
    }

    inline IPort* GetPort() const
    {
        HANDLE hPort = GetPortHandle();
        IPort* pPort = HandleToPort( hPort );
        return pPort;
    }
    // Set the default request timeout for an
    // interface . The outgoing requests with
    // reply afterwards will all follow this
    // timeout value.
    gint32 SetTimeout( gint32 iTimeoutSec )
    {
        CCfgOpenerObj oCfg( ( CObjBase* )m_pIfStat );
        return oCfg.SetIntProp(
            propTimeoutSec, iTimeoutSec );
    }

    gint32 GetTimeout( gint32& iTimeoutSec )
    {
        CCfgOpenerObj oCfg( ( CObjBase* )m_pIfStat );
        return oCfg.GetIntProp( propTimeoutSec,
            ( guint32& )iTimeoutSec );
    }

    gint32 SetKeepAlive( gint32 iTimeoutSec  )
    {
        CCfgOpenerObj oCfg( ( CObjBase* )m_pIfStat );
        return oCfg.SetIntProp(
            propKeepAliveSec, iTimeoutSec );
    }

    gint32 GetKeepAlive( gint32& iTimeoutSec )
    {
        CCfgOpenerObj oCfg( ( CObjBase* )m_pIfStat );
        gint32 ret = oCfg.GetIntProp(
            propKeepAliveSec,
            ( guint32& )iTimeoutSec );

        if( SUCCEEDED( ret ) )
            return 0;

        return ERROR_FALSE;
    }
};

class CRpcBaseOperations :
    public CGenericInterface
{
    protected:

    // the default message match to register with
    // the port stack
    MatchPtr        m_pIfMatch;

    // the listening irp
    IrpPtr          m_pIrp;

    gint32 EnableEventInternal(
        IMessageMatch* pMatch,
        bool bEnable,
        IEventSink* pCompletion );

    gint32 PauseResumeInternal(
        EnumEventId iEvent,
        IEventSink* pCallback );

    public:
    typedef CGenericInterface super;
    CRpcBaseOperations( const IConfigDb* pCfg )
        : super( pCfg )
    {;}

    virtual gint32 OpenPort(
        IEventSink* pCallback );

    virtual gint32 ClosePort();

    // start listening the event from the
    // interface server
    virtual gint32 EnableEvent(
        IMessageMatch* pMatch,
        IEventSink* pCompletion );

    // stop listening the event from the interface
    // server
    virtual gint32 DisableEvent(
        IMessageMatch* pMatch,
        IEventSink* pCompletion );

    virtual gint32 StartRecvMsg(
        IEventSink* pCompletion,
        IMessageMatch* pMatch );

    gint32 CancelRecvMsg();

    gint32 DoPause(
        IEventSink* pCallback = nullptr );

    gint32 DoResume(
        IEventSink* pCallback = nullptr );

    gint32 DoModEvent(
        EnumEventId iEvent,
        const std::string& strModule );

    gint32 DoRmtModEvent(
        EnumEventId iEvent,
        const std::string& strModule,
        const std::string& strIpAddr );

    virtual gint32 SetupReqIrp( IRP* pIrp,
        IConfigDb* pReqCall,
        IEventSink* pCallback ) = 0;

    virtual gint32 BuildBufForIrp(
        BufPtr& pBuf,
        IConfigDb* pReqCall ) = 0;
};
/**
* @name CRpcInterfaceBase class will provide the
* the interface state mangement and task
* serializations related to state changing.
* it will interface with the events from the
* connection point and the request from client
* requests.
* 
* @{ */
/**  @} */

class CRpcInterfaceBase :
    public CRpcBaseOperations,
    public IInterfaceEvents,
    public IInterfaceCommands
{

    TaskGrpPtr      m_pRootTaskGroup;

    gint32 PauseResume( bool bResume,
        IEventSink* pCallback );

    public:

    typedef CRpcBaseOperations super;

    CRpcInterfaceBase( const IConfigDb* pCfg )
        :super( pCfg )
    {;}

    gint32 DoCleanUp();

    TaskGrpPtr& GetTaskGroup()
    { return m_pRootTaskGroup; }

    bool IsMyPort( HANDLE hPort )
    {
        return static_cast< CInterfaceState* >
            ( m_pIfStat )->IsMyPort( hPort );
    }

    bool IsMyDest( const std::string& strMod )
    {
        return static_cast< CInterfaceState* >
            ( m_pIfStat )->IsMyDest( strMod );
    }

    // events from the connection points
    virtual gint32 OnPortEvent(
        EnumEventId iEvent,
        HANDLE hPort );

    virtual gint32 OnModEvent(
        EnumEventId iEvent,
        const std::string& strModule );

    virtual gint32 OnDBusEvent(
        EnumEventId iEvent );

    virtual gint32 OnRmtModEvent(
        EnumEventId iEvent,
        const std::string& strModule,
        const std::string& strIpAddr );

    virtual gint32 OnRmtSvrEvent(
        EnumEventId iEvent,
        const std::string& strIpAddr );

    virtual gint32 OnAdminEvent(
        EnumEventId iEvent,
        const std::string& strParams );

    // IServiceEx methods
    // start the proxy, called by the interface
    // owner
    virtual gint32 Start();
    virtual gint32 StartEx( IEventSink* pCallback );

    // stop the proxy, called by the interface
    // owner
    virtual gint32 Stop();
    virtual gint32 StopEx( IEventSink* pCallback );

    virtual gint32 Pause(
        IEventSink* pCallback = nullptr );

    virtual gint32 Resume(
        IEventSink* pCallback = nullptr );

    virtual gint32 Shutdown(
        IEventSink* pCallback = nullptr );

    virtual gint32 Restart(
        IEventSink* pCallback = nullptr )
    { return -ENOTSUP; }

    gint32 AppendAndRun( TaskletPtr& pTask );
    gint32 AddAndRun( TaskletPtr& pParallelTask );

    virtual gint32 OnEvent( EnumEventId iEvent,
        guint32 dwParam1 = 0,
        guint32 dwParam2 = 0,
        guint32* pData = NULL  );
};

struct IRpcNonDBusIf
{
    virtual gint32 DoInvoke(
        IConfigDb* pReqMsg,
        IEventSink* pCallback = nullptr ) = 0;

    virtual gint32 OnCancel(
        IConfigDb* pReqCall,
        IEventSink* pCallback = nullptr )
    { return -ENOTSUP; }

    virtual gint32 SendResponse(
        IConfigDb* pReqMsg,
        CfgPtr& pRespData ) = 0;

};

typedef std::map< std::string, TaskletPtr > FUNC_MAP;
extern std::map< gint32, FUNC_MAP > g_mapFuncs;

typedef std::map< std::string, ObjPtr > PROXY_MAP;
extern std::map< gint32, PROXY_MAP > g_mapProxyFuncs;
/**
* @name CRpcServices as an interface to provide
* the transport layer interface for an rpc
* connection.
* @{ */
/**  @} */
class CRpcServices : 
    public CRpcInterfaceBase,
    public IRpcNonDBusIf
{

    friend class CIfServiceNextMsgTask;

    protected:

    std::deque< EventPtr > m_queTasks;
    gint32 InvokeNextMethod();

    virtual gint32 SetupReqIrp( IRP* pIrp,
        IConfigDb* pReqCall, IEventSink* pCallback );

    gint32 BuildBufForIrpIfMethod(
        BufPtr& pBuf,
        IConfigDb* pReqCall );

    virtual gint32 BuildBufForIrp(
        BufPtr& pBuf,
        IConfigDb* pReqCall );

    gint32 FillRespData(
        DMsgPtr& pMsg, IConfigDb* pResp );


    gint32 SetFuncMap( FUNC_MAP& oMap )
    {
        g_mapFuncs[ GetClsid() ] = oMap;
        return 0;
    }

    FUNC_MAP* GetFuncMap()
    {
        try{
            FUNC_MAP& oFuncMap =
                g_mapFuncs.at( GetClsid() );
            return &oFuncMap;
        }
        catch( const std::out_of_range& e )
        {
        }
        return nullptr;
    }

    PROXY_MAP* GetProxyMap()
    {
        try{
            PROXY_MAP& oProxyMap =
                g_mapProxyFuncs.at( GetClsid() );
            return &oProxyMap;
        }
        catch( const std::out_of_range& e )
        {
        }
        return nullptr;
    }

    gint32 SetProxyMap( PROXY_MAP& oMap )
    {
        g_mapProxyFuncs[ GetClsid() ] = oMap;
        return 0;
    }

    public:

    gint32 GetDelegate(
        const std::string& strFunc,
        TaskletPtr& pFunc )
    {
        gint32 ret = 0;
        try{
            pFunc = g_mapFuncs.
                at( GetClsid() ).at( strFunc );
        }
        catch( const std::out_of_range& e )
        {
            ret = -ERANGE;
        }
        return ret;
    }

    gint32 GetProxy(
        const std::string& strFunc,
        ObjPtr& pProxy )
    {
        gint32 ret = 0;
        try{
            pProxy = g_mapProxyFuncs.
                at( GetClsid() ).at( strFunc );
        }
        catch( const std::out_of_range& e )
        {
            ret = -ERANGE;
        }
        return ret;
    }

    typedef CRpcInterfaceBase super;

    // bring overloaded methods from base class to
    // current scope
    using IRpcNonDBusIf::DoInvoke;
    using IRpcNonDBusIf::OnCancel;
    using IRpcNonDBusIf::SendResponse;
    /*  
        // InitUserFuncs function should be called
        // from the topmost level of your
        // interface's constructor
        //
        // the content for the server interface
        // should be like the following:
        //
        // call the base class's InitUserFuncs
        super::InitUserFuncs();
        // declare your server side methods you
        // want to expose for remote users to call
    
        BEGIN_HANDLER_MAP;
        
        ADD_SERVICE_HANDLER(
            CInterfaceServer::UserCancelRequest,
            SYS_METHOD_USERCANCELREQ );
    
        END_HANDLER_MAP;
    
        // for proxy interface, should be like the
        // following:
    
        super::InitUserFuncs();
        // declare your proxy side event handlers
        // which will handle the incoming events
        // from the server
        BEGIN_HANDLER_MAP;

        ADD_EVENT_HANDLER(
            CRpcTcpBridgeProxy::OnRmtSvrOffline,
            BRIDGE_EVENT_SVROFF );

        ADD_EVENT_HANDLER(
            CRpcTcpBridgeProxy::OnRmtModOffline,
            BRIDGE_EVENT_MODOFF );

        ADD_EVENT_HANDLER(
            CRpcTcpBridgeProxy::OnInvalidStreamId,
            BRIDGE_EVENT_INVALIDSTM );

        END_HANDLER_MAP;

        // declare your proxy side method calls,
        // which will be forward to the server
        // side
        BEGIN_PROXY_MAP( false );

        ADD_PROXY_METHOD( SYS_METHOD_USERCANCELREQ, guint64 );

        END_PROXY_MAP
    */
 
    virtual gint32 InitUserFuncs() 
    { return 0; }

    CRpcServices( const IConfigDb* pCfg );

    virtual gint32 SendData(
        IConfigDb* pDataDesc,           // [in]
        gint32 fd,                      // [in]
        guint32 dwOffset,               // [in]
        guint32 dwSize,                 // [in]
        IEventSink* pCallback = nullptr ) = 0;

    virtual gint32 FetchData(
        IConfigDb* pDataDesc,           // [in, out]
        gint32& fd,                     // [out]
        guint32& dwOffset,              // [in,out]
        guint32& dwSize,                // [in,out]
        IEventSink* pCallback = nullptr ) = 0;

    // The method will allocate the irp and send
    // it down to the port stack from user thread.
    // If it is a synchronous call, it will wait
    // for the response till it is completed. For
    // an asyn call, the callback will be called
    // by from within a worker thread.
    // note that: both proxy and server stub needs
    // this methods
    virtual gint32 SendMethodCall(
        IConfigDb* pReqCall,
        CfgPtr& pRespData,
        IEventSink* pCallback = nullptr );

    // invoke an event henadler on the subclass if
    // event is enabled. called by OnEvent on the
    // async thread
    //
    // Interface properties for InvokeMethod:
    //
    // propQueuedReq: true if the incoming
    // requests will be serviced one after the
    // other.  Otherwise, the requests are
    // serviced concurrently, which means the
    // client code should handle the concurrency
    // and reentrancy on its own
    //
    // we have two version of InvokeMethod for
    // dbus and Non-dbus invoke of the method
    // non-dbus is currently existing only between
    // CRpcTcpBridgeProxy and CRpcTcpBridge
    //
    template< typename T1, 
        typename T = typename std::enable_if<
            std::is_same<T1, DBusMessage>::value ||
            std::is_same<T1, IConfigDb>::value, T1 >::type >
    gint32 InvokeMethod( T* pReqMsg,
        IEventSink* pCallback = nullptr );

    virtual gint32 DoInvoke(
        DBusMessage* pReqMsg,
        IEventSink* pCallback = nullptr ) = 0;

    virtual gint32 InvokeUserMethod(
        IConfigDb* pParams,
        IEventSink* pCallback = nullptr );

    // a method for user to override on method
    // canceling
    virtual gint32 OnCancel(
        DBusMessage* pReqMsg,
        IEventSink* pCallback = nullptr )
    { return -ENOTSUP; }

    bool IsQueuedReq();
    guint32 GetQueueSize() const;
    
    // put the request to the task queue and run
    // it immediately if possible
    // main entry from user code into a proxy/stub
    // context
    gint32 RunIoTask(
        IConfigDb* pReqCall,
        CfgPtr& pRespData,
        IEventSink* pCallback,
        guint64* pTaskId = nullptr );

    virtual gint32 FillRespData(
        IRP* pIrp, IConfigDb* pResp );

    virtual gint32 SendResponse(
        DBusMessage* pReqMsg,
        CfgPtr& pRespData ) = 0;

    enum EnumKAPhase{
        KAOrigin, // the KA relay start point
        KARelay,
        KATerm, // the last KA relay node
    };

    virtual gint32 OnKeepAlive(
        IEventSink* pInvokeTask,
        EnumKAPhase bOrigin ) = 0;
};

class CReqBuilder : public CParamList
{
    CRpcBaseOperations* m_pIf;
    gint32 GetCallOptions( CfgPtr& pCfg );

    public:
    typedef CParamList super;
    CReqBuilder( CRpcBaseOperations* pIf );
    CReqBuilder( const IConfigDb* pReq = nullptr ); // copy constructor
    CReqBuilder( IConfigDb* pReq );
    gint32 SetIfName( const std::string& strIfName );
    gint32 SetObjPath( const std::string& strObjPath ); 
    gint32 SetDestination( const std::string& strDestination ); 
    gint32 SetMethodName( const std::string& strMethodName ); 
    gint32 SetSender( const std::string& strSender );
    gint32 SetKeepAliveSec( guint32 dwTimeoutSec ); 
    gint32 SetTimeoutSec( guint32 dwTimeoutSec); 
    gint32 SetCallFlags( guint32 dwFlags );
    gint32 SetReturnValue( gint32 iRet );
    gint32 SetIpAddr(
        const std::string& strIpAddr, bool bSrc );
    gint32 SetTaskId( guint64 dwTaskid );
};

class CReqOpener : public CParamList
{
    gint32 GetCallOptions( CfgPtr& pCfg ) const;
    public:
    typedef CParamList super;
    CReqOpener( IConfigDb* pCfg ) : super( pCfg )
    {;}
    gint32 GetIfName( std::string& strIfName ) const;
    gint32 GetObjPath( std::string& strObjPath ) const; 
    gint32 GetDestination( std::string& strDestination ) const; 
    gint32 GetMethodName( std::string& strMethodName ) const; 
    gint32 GetSender( std::string& strSender ) const;
    gint32 GetKeepAliveSec( guint32& dwTimeoutSec ) const; 
    gint32 GetTimeoutSec( guint32& dwTimeoutSec) const; 
    gint32 GetCallFlags( guint32& dwFlags ) const; 
    gint32 GetReturnValue( gint32& iRet ) const;
    gint32 GetIpAddr( std::string& strIpAddr,
        bool bSrc ) const;
    gint32 GetTaskId( guint64& dwTaskid ) const;

    bool HasReply() const;
    bool IsKeepAlive() const;
    gint32 GetReqType( guint32& dwType ) const;
};

template< typename ...Args>
class CMethodProxy;

class CInterfaceProxy :
    public CRpcServices
{
    protected:

    // invoke an event henadler on the subclass if
    // event is enabled. called by OnEvent on the
    // async thread
    virtual gint32 DoInvoke(
        DBusMessage* pEvtMsg,
        IEventSink* pCallback = nullptr );

    virtual gint32 DoInvoke(
        IConfigDb* pEvtMsg,
        IEventSink* pCallback );

    virtual gint32 OnCancel(
        IConfigDb* pReqCall,
        IEventSink* pCallback = nullptr )
    {
        return OnCancel_Proxy(
            pReqCall, pCallback );
    }

    gint32 OnKeepAliveTerm(
        IEventSink* pInvokeTask );

    virtual gint32 SendFetch_Proxy(
        IConfigDb* pDataDesc,           // [in, out]
        gint32& fd,                     // [out]
        guint32& dwOffset,              // [out]
        guint32& dwSize,                // [out]
        IEventSink* pCallback );

    public:

    typedef CRpcServices super;

    CInterfaceProxy( const IConfigDb* pCfg );
    ~CInterfaceProxy();

    virtual gint32 SendData(
        IConfigDb* pDataDesc,           // [in]
        gint32 fd,                      // [in]
        guint32 dwOffset,               // [in]
        guint32 dwSize,                 // [in]
        IEventSink* pCallback = nullptr )
    {
        if( pDataDesc == nullptr )
            return -EINVAL;

        return SendData_Proxy( pDataDesc,
            fd, dwOffset, dwSize, pCallback );
    }

    virtual gint32 FetchData(
        IConfigDb* pDataDesc,           // [in, out]
        gint32& fd,                     // [out]
        guint32& dwOffset,              // [in,out]
        guint32& dwSize,                // [in,out]
        IEventSink* pCallback = nullptr )
    {
        if( pDataDesc == nullptr )
            return -EINVAL;

        return SendData_Proxy( pDataDesc,
            fd, dwOffset, dwSize, pCallback );
    }

    virtual gint32 OnCancel_Proxy(
        IConfigDb* pReqCall,
        IEventSink* pCallback = nullptr );


    virtual gint32 SendData_Proxy(
        IConfigDb* pDataDesc,           // [in]
        gint32 fd,                      // [in]
        guint32 dwOffset,               // [in]
        guint32 dwSize,                 // [in]
        IEventSink* pCallback = nullptr );

    virtual gint32 FetchData_Proxy(
        IConfigDb* pDataDesc,           // [in, out]
        gint32& fd,                     // [out]
        guint32& dwOffset,              // [out]
        guint32& dwSize,                // [out]
        IEventSink* pCallback = nullptr );

    virtual gint32 SendResponse(
        DBusMessage* pReqMsg,
        CfgPtr& pRespData );

    virtual gint32 SendResponse(
        IConfigDb* pReqMsg,
        CfgPtr& pRespData );

    virtual gint32 OnKeepAlive(
        IEventSink* pInvokeTask,
        EnumKAPhase bTarget );

    virtual gint32 SendProxyReq(
        IEventSink* pCallback,
        bool bNonDBus,
        const std::string& strMethod,
        std::vector< BufPtr >& vecParams,
        guint64& qwIoTaskId );

    virtual gint32 CustomizeRequest(
        IConfigDb* pReqCfg,
        IEventSink* pCallback )
    { return 0; }

    template< typename ...Args >
    gint32 CallUserProxyFunc(
        IEventSink* pCallback,
        guint64& qwIoTaskId,
        std::string strMethod,
        Args... args )
    {
        std::string strName = USER_METHOD( strMethod );
        return CallProxyFunc( pCallback, qwIoTaskId, 
            strName, args... );
    }

    template< typename ...Args >
    gint32 CallProxyFunc(
        IEventSink* pCallback,
        guint64& qwIoTaskId,
        std::string strMethod,
        Args... args )
    {
        ObjPtr pObj;
        gint32 ret = GetProxy( strMethod, pObj );
        if( ERROR( ret ) )
            return ret;

        CMethodProxy< Args...>* pProxy = pObj;
        if( pProxy == nullptr )
            return -EFAULT;

        return ( *pProxy )( this, pCallback,
            qwIoTaskId, args... );
    }

    virtual gint32 InitUserFuncs();

    // a user-initialized cancel request
    gint32 UserCancelRequest(
        IEventSink* pCallback,
	guint64& qwThisTaksId,
        guint64 qwTaskToCancel );
};

class CInterfaceServer :
    public CRpcServices
{

    protected:
    // Typically one port per interface. sometimes
    // you can open more than one port if you are
    // aware of what you are doing

    virtual gint32 SendData_Server(
        IConfigDb* pDataDesc,           // [in]
        gint32 fd,                      // [in]
        guint32 dwOffset,               // [in]
        guint32 dwSize,                 // [in]
        IEventSink* pCallback = nullptr ) = 0;

    virtual gint32 FetchData_Server(
        IConfigDb* pDataDesc,           // [in, out]
        gint32& fd,                     // [out]
        guint32& dwOffset,              // [out]
        guint32& dwSize,                // [out]
        IEventSink* pCallback = nullptr ) = 0;

    // invoke a method on subclass via the method
    // name
    virtual gint32 DoInvoke(
        DBusMessage* pReqMsg,
        IEventSink* pCallback = nullptr );

    virtual gint32 DoInvoke(
        IConfigDb* pEvtMsg,
        IEventSink* pCallback );

    // OnCancel is called when the request will be
    // canceled due to some reasons. This call
    // should only be effective when it is an
    // asynchronous service of message
    virtual gint32 OnCancel(
        DBusMessage* pReqMsg,
        IEventSink* pCallback = nullptr )
    {
        return OnCancel_Server(
            pReqMsg, pCallback );
    }

    gint32 OnKeepAliveOrig(
        IEventSink* pTask );

    gint32 OnKeepAliveRelay(
        IEventSink* pTask );

    public:

    typedef CRpcServices super;

    CInterfaceServer( const IConfigDb* pCfg );
    ~CInterfaceServer();

    // called to set the response to send back
    virtual gint32 SetResponse(
        IEventSink* pTask,
        CfgPtr& pRespData );

    virtual gint32 SendResponse(
        DBusMessage* pReqMsg,
        CfgPtr& pRespData );

    virtual gint32 SendResponse(
        IConfigDb* pReqMsg,
        CfgPtr& pRespData );

    // send a dbus signal, called from the user
    // code
    gint32 BroadcastEvent(
        IConfigDb* pEvent,
        IEventSink* pCallback = nullptr );

    // server side SendData
    virtual gint32 SendData(
        IConfigDb* pDataDesc,           // [in]
        gint32 fd,                      // [in]
        guint32 dwOffset,               // [in]
        guint32 dwSize,                 // [in]
        IEventSink* pCallback = nullptr )
    {
        return SendData_Server( pDataDesc,
            fd, dwOffset, dwSize, pCallback );
    }

    // server side FetchData
    virtual gint32 FetchData(
        IConfigDb* pDataDesc,           // [in, out]
        gint32& fd,                     // [out]
        guint32& dwOffset,              // [in,out]
        guint32& dwSize,                // [in,out]
        IEventSink* pCallback = nullptr )
    {
        return FetchData_Server( pDataDesc,
            fd, dwOffset, dwSize, pCallback );
    }

    virtual gint32 OnCancel_Server(
        DBusMessage* pReqMsg,
        IEventSink* pCallback )
    {
        return super::OnCancel( pReqMsg, pCallback );
    }

    // if the SendData_Server and FetchData_Server
    // return STATUS_PENDING, you should call
    // OnServiceComplete later sometime to
    // complete the task.  This callback is called
    // from user code the pResp point to the
    // response information
    gint32 OnServiceComplete(
        IConfigDb* pResp,
        IEventSink* pCallback );
 
    virtual gint32 OnKeepAlive(
        IEventSink* pInvokeTask,
        EnumKAPhase  bOrigin );


    virtual gint32 InitUserFuncs();

    // a user-initialized cancel request
    gint32 UserCancelRequest(
        IEventSink* pInvokeTask,
        guint64 qwIoTaskId );
};

// callback definitions for sync/async interface
// calls
class CGenericCallback :
    public CTasklet
{

    public:
    typedef CTasklet   super; 

    CGenericCallback()
        : CTasklet( nullptr )
    {;}

    gint32 NormalizeParams( bool bResp, IConfigDb* pResp,
        std::vector< BufPtr >& oRes )
    {
        CParamList oResp( pResp );
        gint32 ret = 0;
        do{
            if( bResp )
            {
                gint32 iRet = 0;
                ret = oResp.GetIntProp( propReturnValue,
                    ( guint32& )iRet );

                if( ERROR( ret ) )
                    break;

                BufPtr pBuf( true );

                *pBuf = ( guint32 )iRet;
                oRes.push_back( pBuf );
            }

            guint32 dwSize = 0;
            ret = oResp.GetSize( dwSize );
            if( ERROR( ret )  )
                break;

            // NOTE: if iRet is an error code, the
            // dwSize could be zero, and we need
            // to handle this
            for( guint32 i = 0; i < dwSize; i++ )
            {
                BufPtr pBuf;
                ret = oResp.GetProperty( i, pBuf );
                if( ERROR( ret ) )
                    break;

                oRes.push_back( pBuf );
            }
        }while( 0 );

        if( ERROR( ret ) )
            oRes.clear();

        return ret;
    }
};

class CSyncCallback :
    public CGenericCallback
{
    sem_t m_semWait;

    public:
    typedef CGenericCallback super;

    CSyncCallback( const IConfigDb* pCfg = nullptr)
        : CGenericCallback() 
    {
        SetClassId( clsid( CSyncCallback ) );
        sem_init( &m_semWait, 0, 0 );
    }

    ~CSyncCallback()
    {
        sem_destroy( &m_semWait );
    }

    gint32 operator()( guint32 dwContext = 0 )
    {
        if( dwContext != eventTaskComp )
            return -ENOTSUP;
        sem_post( &m_semWait );
        return 0;
    }

    gint32 WaitForComplete()
    {
        sem_wait( &m_semWait );
        return 0;
    }


};

class CAsyncCallbackBase :
    public CGenericCallback
{

    public:
    typedef CGenericCallback super;

    CAsyncCallbackBase( const IConfigDb* pCfg = nullptr)
        : CGenericCallback() 

    {;}

    virtual gint32 Callback(
        std::vector< BufPtr >& vecParams ) = 0;

    gint32 operator()( guint32 dwContext = 0 )
    {
        if( dwContext != eventTaskComp )
            return -ENOTSUP;

        gint32 ret = 0;
        do{
            std::vector< guint32 > vecParams;

            gint32 ret = GetParamList( vecParams );

            if( ERROR( ret ) )
                break;

            if( vecParams.size() < 4 )
            {
                ret = -EINVAL;
                break;
            }

            if( ERROR( vecParams[ 1 ] ) )
            {
                ret = vecParams[ 1 ];
                break;
            }

            CObjBase* pObjBase =
                reinterpret_cast< CObjBase* >( vecParams[ 3 ] );

            if( pObjBase == nullptr )
                break;

            CCfgOpenerObj oCfg( pObjBase );
            ObjPtr pObj;

            ret = oCfg.GetObjPtr( propRespPtr, pObj );
            if( ERROR( ret ) )
                break;

            CfgPtr pResp = pObj;
            if( pResp.IsEmpty()  )
            {
                ret = -EFAULT;
                break;
            }
            std::vector< BufPtr > vecRespParams;
            ret = NormalizeParams( true, pResp, vecRespParams );
            if( ERROR( ret ) )
                break;

            ret = Callback( vecRespParams );

        }while( 0 );

        return ret;
    }
};

template< typename T >
T CastTo( BufPtr& i )
{
    return ( T )*i;
}

// remove the reference or const modifiers from
// the type list
#define DFirst typename std::decay< First >::type

template< typename First >
auto VecToTupleHelper( std::vector< BufPtr >& vec ) -> std::tuple<First>
{
    BufPtr& i = vec.back();
    return std::tuple< DFirst >(
            std::forward< DFirst >( CastTo< DFirst >( i ) ) );
}

// generate a tuple from the vector with the
// proper type set for each element
template< typename First, typename Second, typename ...Types >
auto VecToTupleHelper( std::vector< BufPtr >& vec ) -> std::tuple< First, Second, Types... >
{
    BufPtr& i = vec[ vec.size() - sizeof...( Types ) + 2 ];

    // to assign right type to the element in the
    // tuple
    return std::tuple_cat(
        std::tuple< DFirst >(
            std::forward< DFirst >( CastTo< DFirst >( i ) ) ),
        VecToTupleHelper< Second, Types... >( vec ) );
}

template< typename ...Types >
auto VecToTuple( std::vector< BufPtr >& vec ) -> std::tuple< Types... >
{
    return VecToTupleHelper< Types...>( vec );
}

template<>
auto VecToTuple<>( std::vector< BufPtr >& vec ) -> std::tuple<>; 

// parameter pack generator
template< int ... >
struct NumberSequence
{};

template<>
struct NumberSequence<>
{};

template< int N, int ...S >
struct GenSequence : GenSequence< N - 1, N - 1, S... > {};

template< int ...S >
struct GenSequence< 0, S... >
{
    typedef NumberSequence< S... > type;
};

template<typename ClassName, typename ...Args>
class CAsyncCallback :
    public CAsyncCallbackBase
{
    typedef gint32 ( ClassName::* FuncType)( Args... ) ;

    template< int ...S>
    gint32 CallUserFunc( std::tuple< Args... >& oTuple, NumberSequence< S... > )
    {
        if( m_pObj == nullptr || m_pUserFunc == nullptr )
            return -EINVAL;

        gint32 ret = ( m_pObj->*m_pUserFunc )(
            std::get< S >( oTuple ) ... );

        return ret;
    }
    //---------

    FuncType m_pUserFunc;
    ClassName* m_pObj;

    public:
    CAsyncCallback( ClassName* pObj, FuncType pFunc )
    {
        // NOTE that the class doesn't have a
        // class id associated because this is a
        // template class
        m_pUserFunc = pFunc;
        m_pObj = pObj;
    }

    // well, this is a virtual function in the
    // template class. 
    gint32 Callback( std::vector< BufPtr >& vecParams )
    {
        if( vecParams.empty() )
            return -EINVAL;

        if( vecParams.size() != sizeof...( Args ) )
        {
            return -EINVAL;
        }

        std::vector< BufPtr > vecResp;

        vecResp.insert( vecResp.begin(),
             vecParams.begin(), vecParams.end() );

        std::tuple<Args...> oTuple =
            VecToTuple< Args... >( vecResp );

        CallUserFunc( oTuple,
            typename GenSequence< sizeof...( Args ) >::type() );

        return 0;
    }
};

/**
* @name NewCallback
* @{ */
/**
  * helper function to ease the developer's effort
  * on instanciation of the the CAsyncCallback via
  * implicit instanciation.
  *@} */

template < typename C, typename ... Types>
inline gint32 NewCallback( TaskletPtr& pCallback,
    bool bAsync, C* pObj, gint32(C::*f)(Types ...) )
{
    if( pObj == nullptr || f == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    if( bAsync )
    {
        try{
            // we cannot use NewObj for this
            // template object
            pCallback = new CAsyncCallback< C, Types... >( pObj, f );
        }
        catch( const std::bad_alloc& e )
        {
            ret = -ENOMEM;
        }
        catch( const std::runtime_error& e )
        {
            ret = -EFAULT;
        }
        catch( const std::invalid_argument& e )
        {
            ret = -EINVAL;
        }
    }
    else
    {
        ret = pCallback.NewObj( clsid( CSyncCallback ) );
    }
    return ret;
}

/**
* @name BIND_CALLBACK 
* @{ */
/**
  * Helper macro for further effort saving note the
  * `this' pointer, which indicated you should use
  * the macro only in the context of the  member
  * method of the same class as pFunc.
  *
  * So the pTask is a TaskletPtr to receive the new
  * callback object, and the pFunc is the member method
  * as the callback for a specific request
  *
  * NOTE: please make sure the first parameter of the
  * pFunc must be the return value of the call
 * @} */

#define BIND_CALLBACK( pTask, pFunc ) \
    NewCallback( pTask, true, this, pFunc ) 

class CTaskWrapper :
    public CGenericCallback
{
    TaskletPtr m_pTask;
    public:
    typedef CGenericCallback super;

    CTaskWrapper()
    {
        SetClassId( clsid( CTaskWrapper ) );
    }

    gint32 SetTask( IEventSink* pTask )
    {
        if( pTask == nullptr )
            return 0;

        m_pTask = ObjPtr( pTask );
        return 0;
    }

    void TransferParams()
    {
        CCfgOpener oCfg( ( IConfigDb* )
            m_pTask->GetConfig() );

        oCfg.MoveProp( propRespPtr, this );
        oCfg.MoveProp( propMsgPtr, this );
        oCfg.MoveProp( propReqPtr, this );
    }

    virtual gint32 OnEvent( EnumEventId iEvent,
        guint32 dwParam1 = 0,
        guint32 dwParam2 = 0,
        guint32* pData = NULL  )
    {
        if( m_pTask.IsEmpty() )
            return 0;

        TransferParams();
        return m_pTask->OnEvent( iEvent,
            dwParam1, dwParam2, pData );
    }

    virtual gint32 operator()(
        guint32 dwContext )
    {
        if( m_pTask.IsEmpty() )
            return 0;

        TransferParams();
        return m_pTask->operator()( dwContext );
    }
};

class CDelegateBase :
    public CGenericCallback
{

    public:
    typedef CGenericCallback super;

    CDelegateBase( const IConfigDb* pCfg = nullptr)
        : CGenericCallback() 
    {;}

    virtual gint32 Delegate(
        CObjBase* pObj,
        IEventSink* pCallback,
        std::vector< BufPtr >& vecParams ) = 0;

    gint32 operator()( guint32 dwContext )
    {
        return 0;
    }

    gint32 operator()(
        CObjBase* pObj,
        IEventSink* pCallback,
        IConfigDb* pParams )
    {
        if( pCallback == nullptr ||
            pParams == nullptr ||
            pObj == nullptr )
            return -EINVAL;

        gint32 ret = 0;
        do{
            CfgPtr pReq = pParams;
            if( pReq.IsEmpty()  )
            {
                ret = -EFAULT;
                break;
            }
            std::vector< BufPtr > vecRespParams;

            ret = NormalizeParams(
                false, pReq, vecRespParams );

            if( ERROR( ret ) )
                break;

            ret = Delegate( pObj, pCallback,
                vecRespParams );

        }while( 0 );

        return ret;
    }
};

template<typename ClassName, typename ...Args>
class CMethodDelegate :
    public CDelegateBase
{
    typedef gint32 ( ClassName::* FuncType)(
        IEventSink* pCallback, Args... ) ;

    template< int ...S>
    gint32 CallUserFunc( ClassName* pObj, IEventSink* pCallback,
        std::tuple< Args... >& oTuple, NumberSequence< S... > )
    {
        if( pObj == nullptr || m_pUserFunc == nullptr )
            return -EINVAL;

        gint32 ret = ( pObj->*m_pUserFunc )( pCallback,
            std::get< S >( oTuple )... );

        return ret;
    }

    //---------

    FuncType m_pUserFunc;

    public:
    typedef CDelegateBase super;
    CMethodDelegate ( FuncType pFunc )
    {
        // NOTE that the class doesn't have a
        // class id associated because this is a
        // template class
        m_pUserFunc = pFunc;
    }

    // well, this is a virtual function in the
    // template class. weird? the joint part where
    // two types of Polymorphisms meet, virtual
    // functions and template classes
    gint32 Delegate( 
        CObjBase* pObj,
        IEventSink* pCallback,
        std::vector< BufPtr >& vecParams )
    {
        if( vecParams.empty() )
            return -EINVAL;

        if( pObj == nullptr ||
            pCallback == nullptr )
            return -EINVAL;

        ClassName* pClass = static_cast< ClassName* >( pObj );
        if( vecParams.size() != sizeof...( Args ) )
        {
            return -EINVAL;
        }

        std::vector< BufPtr > vecResp;
        for( auto& elem : vecParams )
            vecResp.push_back( elem );

        std::tuple<Args...> oTuple =
            VecToTuple< Args... >( vecResp );

        CallUserFunc( pClass, pCallback, oTuple,
            typename GenSequence< sizeof...( Args ) >::type() );

        return 0;
    }
};

template < typename C, typename ...Types>
inline gint32 NewDelegate( TaskletPtr& pDelegate,
    gint32(C::*f)(IEventSink*, Types ...) )
{
    if( f == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    try{
        // we cannot use NewObj for this
        // template object
        pDelegate = new CMethodDelegate< C, Types... >( f );
    }
    catch( const std::bad_alloc& e )
    {
        ret = -ENOMEM;
    }
    catch( const std::runtime_error& e )
    {
        ret = -EFAULT;
    }
    catch( const std::invalid_argument& e )
    {
        ret = -EINVAL;
    }
    return ret;
}

#define BEGIN_HANDLER_MAP \
do{\
    FUNC_MAP&& _oMap_ = FUNC_MAP(); \
    FUNC_MAP* _pMap_ = nullptr; \
    do{ \
        _pMap_ = GetFuncMap(); \
        if( _pMap_ != nullptr ) \
            _oMap_ = *_pMap_; \
    }while( 0 )

#define END_HANDLER_MAP \
    do{ \
        if( _pMap_ == nullptr && \
            _oMap_.size() > 0 ) \
            SetFuncMap( _oMap_ ); \
    }while( 0 ); \
}while( 0 )

// we have some limitation on the service handler,
// that is, output parameter is not supported so
// far, because no one care about it. for the
// service handler's output, better attach it to
// the propResp property of the incoming
// "pCallback"
#define ADD_SERVICE_HANDLER( f, fname ) \
do{ \
    if( _oMap_.size() > 0 && \
        _oMap_.find( fname ) != _oMap_.end() ) \
        break; \
    TaskletPtr pTask; \
    gint32 ret = NewDelegate( pTask,  &f ); \
    if( ERROR( ret ) ) \
    { \
        std::string strMsg = DebugMsg( ret, \
            "Error allocaate delegate" ); \
        throw std::runtime_error( strMsg ); \
    } \
    _oMap_.insert( \
        std::pair< std::string, TaskletPtr >( fname, pTask ) ); \
    if( ERROR( ret ) ) \
    { \
        std::string strMsg = DebugMsg( ret, \
            "Error allocaate delegate" ); \
        throw std::runtime_error( strMsg ); \
    } \
}while( 0 )

#define ADD_USER_SERVICE_HANDLER( f, fname ) \
do{ \
    std::string strHandler = USER_METHOD( fname ); \
    ADD_SERVICE_HANDLER( f, strHandler ); \
}while( 0 )

#define ADD_EVENT_HANDLER( f, fname ) \
	ADD_SERVICE_HANDLER( f, fname )

#define ADD_USER_EVENT_HANDLER( f, fname ) \
	ADD_USER_SERVICE_HANDLER( f, fname )

template< typename T >
BufPtr PackageTo( T& i )
{
    BufPtr pBuf( true );
    *pBuf = i;
    return pBuf;
}

template< typename ...Args>
class CMethodProxy :
    public CObjBase
{
    //---------
    std::string m_strMethod;
    bool m_bNonDBus;
    public:
    CMethodProxy ( bool bNonDBus,
        const std::string& strMethod )
        : m_strMethod( strMethod ),
        m_bNonDBus( bNonDBus )
    {;}

    template < int N >
    void TupleToVec( std::tuple< Args...>& oTuple,
        std::vector< BufPtr >& vec,
        NumberSequence< N > )
    {
        vec[ N ] = ( PackageTo< typename std::decay< decltype( std::get< N >( oTuple ) ) >::type >
            ( std::get< N >( oTuple ) ) );
    }

    template < int N, int M, int...S >
    void TupleToVec( std::tuple< Args...>& oTuple,
        std::vector< BufPtr >& vec,
        NumberSequence< N, M, S... > )
    {
        vec[ N ] = ( PackageTo< typename std::decay< decltype( std::get< N >( oTuple ) ) >::type >
            ( std::get< N >( oTuple ) ) );
        TupleToVec( oTuple, vec, NumberSequence<M, S...>() );
    }

    gint32 operator()(
        CInterfaceProxy* pIf,
        IEventSink* pCallback,
        guint64& qwIoTaskId,
        Args... args )
    {
        if( pIf == nullptr )
            return -EINVAL;

        auto oTuple = std::make_tuple( args ... );
        std::vector< BufPtr > vec;

        if( sizeof...( args ) )
        {
            TupleToVec( oTuple, vec,
                typename GenSequence< sizeof...( Args ) >::type() );
        }

        return pIf->SendProxyReq( pCallback,
            m_bNonDBus, m_strMethod, vec, qwIoTaskId );
    }
};

#define BEGIN_PROXY_MAP( bNonDBus ) \
do{ \
    PROXY_MAP&& _oMapProxies_ = PROXY_MAP(); \
    bool _bNonBus_ = bNonDBus; \
    PROXY_MAP* _pMap_ = nullptr; \
    do{ \
        _pMap_ = GetProxyMap(); \
        if( _pMap_ != nullptr ) \
            _oMapProxies_ = *_pMap_; \
    }while( 0 )

#define ADD_PROXY_METHOD( MethodName, ... ) \
do{ \
    const std::string strName( MethodName ); \
    if( _oMapProxies_.size() > 0 && \
        _oMapProxies_.find( strName ) != \
            _oMapProxies_.end() ) \
        break; \
    ObjPtr pObj = new CMethodProxy< __VA_ARGS__ >( \
        _bNonBus_, strName );\
    _oMapProxies_[ strName ] = pObj; \
}while( 0 )
    
#define ADD_USER_PROXY_METHOD( MethodName, ... ) \
do{ \
    std::string strName = USER_METHOD( MethodName );\
    ADD_PROXY_METHOD( strName, __VA_ARGS__ ); \
}while( 0 )

#define END_PROXY_MAP \
    do{ \
        if( _pMap_ == nullptr && \
            _oMapProxies_.size() > 0 ) \
            SetProxyMap( _oMapProxies_ ); \
    }while( 0 ); \
}while( 0 );

template<>
CObjBase* CastTo< CObjBase* >( BufPtr& i );

template<>
DBusMessage* CastTo< DBusMessage* >( BufPtr& i );

template<>
BufPtr CastTo< BufPtr >( BufPtr& i );

template<>
BufPtr PackageTo< DMsgPtr >( DMsgPtr& pMsg );

template<>
BufPtr PackageTo< ObjPtr >( ObjPtr& pObj );

template<>
BufPtr PackageTo< CObjBase* >( CObjBase*& pObj );

template<>
BufPtr PackageTo< DBusMessage* >( DBusMessage*& pMsg );

template<>
BufPtr PackageTo< stdstr >( stdstr& str );

template<>
BufPtr PackageTo< float >( float& pfloat );

#define SET_RMT_TASKID( pReq, oTaskCfg ) \
do{ \
    CCfgOpener oReq( pReq ); \
    guint64 qwTaskId; \
    ret = oReq.GetQwordProp( \
        propTaskId, qwTaskId ); \
    if( SUCCEEDED( ret ) ) \
    { \
        oTaskCfg.SetQwordProp( \
            propRmtTaskId, qwTaskId ); \
    } \
}while( 0 )

class CDeferredCallBase :
    public CGenericCallback
{

    public:
    typedef CGenericCallback super;

    CDeferredCallBase( const IConfigDb* pCfg = nullptr)
        : CGenericCallback()
    {;}

    virtual gint32 Delegate( CObjBase* pObj,
        std::vector< BufPtr >& vecParams ) = 0;

    ObjPtr m_pObj;
    std::vector< BufPtr > m_vecArgs;

    gint32 operator()( guint32 dwContext )
    {
        if( m_pObj.IsEmpty() ||
            m_vecArgs.empty() )
            return -EINVAL;

        return Delegate( m_pObj, m_vecArgs );
    }
};

template<typename ClassName, typename ...Args>
class CDeferredCall :
    public CDeferredCallBase
{
    typedef gint32 ( ClassName::* FuncType)( Args... ) ;

    template< int ...S>
    gint32 CallUserFunc( ClassName* pObj, 
        std::vector< BufPtr >& vecParams, NumberSequence< S... > )
    {
        if( pObj == nullptr || m_pUserFunc == nullptr )
            return -EINVAL;

        gint32 ret = ( pObj->*m_pUserFunc )( *vecParams[ S ]... );

        return ret;
    }

    template < int N >
    void TupleToVec( std::tuple< Args...>& oTuple,
        std::vector< BufPtr >& vec,
        NumberSequence< N > )
    {
        vec[ N ] = ( PackageTo< typename std::decay< decltype( std::get< N >( oTuple ) ) >::type >
            ( std::get< N >( oTuple ) ) );
    }

    template < int N, int M, int...S >
    void TupleToVec( std::tuple< Args...>& oTuple,
        std::vector< BufPtr >& vec,
        NumberSequence< N, M, S... > )
    {
        vec[ N ] = ( PackageTo< typename std::decay< decltype( std::get< N >( oTuple ) ) >::type >
            ( std::get< N >( oTuple ) ) );
        TupleToVec( oTuple, vec, NumberSequence<M, S...>() );
    }
    //---------

    FuncType m_pUserFunc;

    public:
    typedef CDelegateBase super;
    CDeferredCall( FuncType pFunc, ObjPtr& pObj, Args... args )
    {
        // NOTE that the class doesn't have a
        // class id associated because this is a
        // template class
        m_pObj = pObj;
        m_pUserFunc = pFunc;

        auto oTuple = std::make_tuple( args ... );
        std::vector< BufPtr > vec;

        if( sizeof...( args ) )
        {
            TupleToVec( oTuple, m_vecArgs,
                typename GenSequence< sizeof...( Args ) >::type() );
        }
    }

    // well, this is a virtual function in the
    // template class. weird? the joint part where
    // two types of Polymorphisms meet, virtual
    // functions and template classes
    gint32 Delegate( CObjBase* pObj,
        std::vector< BufPtr >& vecParams )
    {
        if( vecParams.empty() )
            return -EINVAL;

        if( pObj == nullptr )
            return -EINVAL;

        ClassName* pClass = dynamic_cast< ClassName* >( pObj );

        if( vecParams.size() != sizeof...( Args ) )
            return -EINVAL;

        CallUserFunc( pClass, vecParams,
            typename GenSequence< sizeof...( Args ) >::type() );

        return 0;
    }
};

template < typename C, typename ... Types>
inline gint32 NewDeferredCall( TaskletPtr& pCallback,
    ObjPtr pObj, gint32(C::*f)(Types ...), Types... args )
{
    CTasklet* pDeferredCall =
        new CDeferredCall< C, Types... >( f, pObj, args... );
    if( pDeferredCall == nullptr )
        return -EFAULT;
    pCallback = pDeferredCall;
    return 0;
}

#define DEFER_CALL( pMgr, pObj, func, ... ) \
({ \
    TaskletPtr pTask; \
    gint32 ret_ = NewDeferredCall( pTask, pObj, func, __VA_ARGS__ ); \
    if( SUCCEEDED( ret ) ) \
        ret_ = pMgr->RescheduleTask( pTask ); \
    ret_; \
})
