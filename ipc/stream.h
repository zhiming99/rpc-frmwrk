/*
 * =====================================================================================
 *
 *       Filename:  stream.h
 *
 *    Description:  Declaration of stream related classes
 *
 *        Version:  1.0
 *        Created:  11/22/2018 10:08:34 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:  
 *
 * =====================================================================================
 */

#pragma once

#include "configdb.h"
#include "defines.h"
#include "ifhelper.h"
#include <exception>

#include "uxport.h"

struct IStream
{
    protected:
    gint32 OnUxStmEvtWrapper(
        HANDLE hChannel,
        IConfigDb* pCfg );

    public:
    typedef std::map< HANDLE, InterfPtr > UXSTREAM_MAP;
    UXSTREAM_MAP m_mapUxStreams;

    virtual gint32 CanContinue() = 0;

    inline CRpcServices* GetInterface()
    { return dynamic_cast< CRpcServices* >( this ); }

    inline gint32 GetUxStream(
        HANDLE hChannel, InterfPtr& pIf )
    {
        CRpcServices* pThis =
            dynamic_cast< CRpcServices* >( this );

        CStdRMutex oIfLock( pThis->GetLock() );
        UXSTREAM_MAP::iterator itr =
            m_mapUxStreams.find( hChannel );

        if( itr == m_mapUxStreams.end() )
            return -ENOENT;
        pIf = itr->second;

        return STATUS_SUCCESS;
    }

    inline gint32 AddUxStream(
        HANDLE hChannel, InterfPtr& pIf )
    {
        CRpcServices* pThis =
            dynamic_cast< CRpcServices* >( this );

        if( pIf.IsEmpty() )
            return -EFAULT;

        CStdRMutex oIfLock( pThis->GetLock() );

        UXSTREAM_MAP::iterator itr =
            m_mapUxStreams.find( hChannel );

        if( itr != m_mapUxStreams.end() )
            return -EEXIST;

        m_mapUxStreams.insert(
            std::pair< HANDLE, InterfPtr >
                ( hChannel, pIf ) );

        return STATUS_SUCCESS;
    }

    inline gint32 RemoveUxStream(
        HANDLE hChannel )
    {
        CRpcServices* pThis =
            dynamic_cast< CRpcServices* >( this );

        CStdRMutex oIfLock( pThis->GetLock() );

        UXSTREAM_MAP::iterator itr =
            m_mapUxStreams.find( hChannel );

        if( itr != m_mapUxStreams.end() )
            return -EEXIST;

        m_mapUxStreams.erase( itr );

        return STATUS_SUCCESS;
    }

    virtual gint32 OpenChannel(
        IConfigDb* pDataDesc,
        int& fd, HANDLE& hChannel,
        IEventSink* pCallback ) = 0; 

    virtual gint32 CloseChannel(
        HANDLE hChannel,
        IEventSink* pCallback );

    gint32 WriteStream( HANDLE hChannel,
        BufPtr& pBuf );

    gint32 WriteStream( HANDLE hChannel,
        BufPtr& pBuf, IEventSink* pCallback );

    gint32 SendPingPong( HANDLE hChannel,
        bool bPing = true );

    gint32 SendPingPong(
        HANDLE hChannel, bool bPing,
        IEventSink* pCallback );

    // data is ready for reading
    virtual gint32 OnStmRecv(
        HANDLE hChannel, BufPtr& pBuf ) = 0;

    virtual gint32 OnPingPong(
        HANDLE hChannel,
        bool bPing = true )
    { return 0; }

    // callback from the watch task when a fatal error
    // occurs during I/O
    virtual gint32 OnChannelError(
        HANDLE hChannel,
        gint32 iError ) = 0;

    // interface to notify the server/proxy, it is OK
    // to send data to the peer. Override this method
    // for the desired action.
    virtual gint32 OnConnected(
        HANDLE hChannel ) = 0;

    virtual gint32 OnUxStreamEvent(
        HANDLE hChannel,
        guint8 byToken,
        CBuffer* pBuf );

    // callback when a close token is received
    virtual gint32 OnClose(
        HANDLE hChannel );
};

struct IStreamServer : public IStream
{
};

struct IStreamProxy : public IStream
{
    virtual gint32 CancelChannel(
        HANDLE hChannel ) = 0;
};

class CStreamProxy :
    public virtual CInterfaceProxy,
    public IStreamProxy
{

    // send the FETCH_DATA request to the server
    gint32 SendRequest(
        IConfigDb* pDataDesc,
        int fd, IEventSink* pCallback );

    public:
    typedef CInterfaceProxy super;
    typedef CInterfaceProxy _MyVirtBase;

    using IStream::OnUxStreamEvent;
    using IStream::OnStmRecv;
    using IStream::OnChannelError;

    CStreamProxy( const IConfigDb* pCfg )
        :super( pCfg )
    {}

    virtual gint32 CanContinue()
    { return 0; }

    virtual gint32 InitUserFuncs()
    {
        BEGIN_IFPROXY_MAP( IStream, false );
        END_IFPROXY_MAP;
        return 0;
    }

    // note the fd is always -1 for proxy version of
    // implementation and the fd will be created in
    // this method
    gint32 OpenChannel(
        IConfigDb* pDataDesc,
        int& fd, HANDLE& hChannel,
        IEventSink* pCallback );

    // callback from the watch task on the I/O error
    virtual gint32 OnChannelError(
        HANDLE hChannel, gint32 iError );

    // call this method when the proxy encounters
    // fatal error
    gint32 CancelChannel( HANDLE hChannel );

    // call this helper to start a stream channel
    gint32 StartStream( HANDLE& hChannel );

    gint32 OnUxStreamEvent(
        HANDLE hChannel,
        guint8 byToken,
        CBuffer* pBuf )
    {
        return IStream::OnUxStreamEvent(
            hChannel, byToken, pBuf );
    }
};

class CStreamServer :
    public virtual CAggInterfaceServer,
    public IStreamServer
{
    gint32 CanContinue();

    gint32 CreateSocket(
        int& fdlocal, int& fdRemote );

    public:
    typedef CAggInterfaceServer super;
    typedef CAggInterfaceServer _MyVirtBase;

    using IStream::OnUxStreamEvent;
    using IStream::OnStmRecv;

    CStreamServer( const IConfigDb* pCfg )
        :super( pCfg )
    {}

    const EnumClsid GetIid() const
    { return iid( IStream ); }

    virtual gint32 InitUserFuncs()
    {
        // we are using built-in handlers, so empty map
        // is fine
        BEGIN_IFHANDLER_MAP( IStream );
        END_IFHANDLER_MAP;
        return 0;
    }

    // called by FetchData_Server on new stream request
    // note the fd is from the FETCH_DATA request
    gint32 OpenChannel(
        IConfigDb* pDataDesc,
        int& fd, HANDLE& hChannel,
        IEventSink* pCallback );

    gint32 FetchData_Server(
        IConfigDb* pDataDesc,           // [in]
        gint32& fd,                     // [out]
        guint32& dwOffset,              // [in, out]
        guint32& dwSize,                // [in, out]
        IEventSink* pCallback );

    virtual gint32 OnChannelError(
        HANDLE hChannel, gint32 iError );

    gint32 OnUxStreamEvent(
        HANDLE hChannel,
        guint8 byToken,
        CBuffer* pBuf )
    {
        return IStream::OnUxStreamEvent(
            hChannel, byToken, pBuf );
    }
};

template< class T >
class CStreamRelayBase :
    public T
{
    protected:

    // the parent object, the CRpcTcpBridgeProxy or
    // CRpcTcpBridge
    InterfPtr m_pParent;

    // maps between the stream id and the stream handle
    std::map< HANDLE, gint32 > m_mapHandleToStmId;
    std::map< gint32, HANDLE > m_mapStmIdToHandle;

    // the irps to receive the packet from a stream
    std::map< gint32, guint32 > m_mapStmPendingBytes;


    public:

    typedef enum{
        stmevtFdClosed,
        stmevtStmClosed,
        stmevtKATimeout,
        stmevtPipeError,     // internal error
        stmevtNetError,      // internal error

    }EnumStmEvt;

    typedef T super;
    CStreamRelayBase( const IConfigDb* pCfg )
        : super::_MyVirtBase( pCfg ), super( pCfg )
    {}

    // stream over unix sock
    guint32 GetPendingWrite(
        HANDLE hWatch ) const;

    // data received from local sock
    virtual gint32 OnStmRecv(
        HANDLE hChannel, BufPtr& pBuf );

    // the local sock is closed
    virtual gint32 OnClose(
        HANDLE hChannel );

    // error happens on the local sock 
    virtual gint32 OnChannelError(
        HANDLE hChannel, gint32 iError );

    // data received from remote( tcp ) sock
    gint32 OnStmRecv(
        gint32 iStmId, BufPtr& pBuf );

    // the remote( tcp ) sock is closed
    gint32 OnClose(
        gint32 iStmId );

    // error happens on the remote( tcp ) sock 
    gint32 OnChannelError(
        gint32 iStmId, gint32 iError );

    gint32 OpenChannel(
        IConfigDb* pDataDesc,
        int& fd, HANDLE& hChannel,
        IEventSink* pCallback );

    // stream over tcp sock
    guint32 GetPendingWrite(
        gint32 iStmId ) const
    {
        CStdRMutex oIfLock( this->GetLock() );
        std::map< gint32, guint32 >::iterator
            itr = m_mapStmPendingBytes.find( iStmId );
        if( itr != m_mapStmPendingBytes.end() )
            return itr->second;
        return 0;
    }

    // stream over tcp sock
    gint32 AddPendingWrite(
        gint32 iStmId, guint32 dwBytes )
    {
        CStdRMutex oIfLock( this->GetLock() );
        std::map< gint32, guint32 >::iterator
            itr = m_mapStmPendingBytes.find( iStmId );
        if( itr == m_mapStmPendingBytes.end() )
            return -ENOENT;
        itr->second += dwBytes;
        return 0;
    }

    gint32 SubPendingWrite(
        gint32 iStmId, guint32 dwBytes )
    {
        CStdRMutex oIfLock( this->GetLock() );
        std::map< gint32, guint32 >::iterator
            itr = m_mapStmPendingBytes.find( iStmId );
        if( itr == m_mapStmPendingBytes.end() )
            return -ENOENT;
        itr->second -= dwBytes;
        return 0;
    }

    gint32 OnStreamEvent(
        EnumEventId iEvent,
        guint32 dwStmId,
        guint32 dwContext,
        guint32* pData );

    using IStream::OnConnected;
    gint32 OnConnected( HANDLE hChannel )
    { return 0; }
};

// this interface will be hosted by
// CRpcTcpBridgeImpl
class CStreamServerRelay :
    public CStreamRelayBase< CStreamServer >
{
    protected:
    gint32 FetchData_Server(
        IConfigDb* pDataDesc,           // [in]
        gint32& fd,                     // [out]
        guint32& dwOffset,              // [in, out]
        guint32& dwSize,                // [in, out]
        IEventSink* pCallback );

    public:

    using IStream::OnPingPong;

    typedef CStreamRelayBase super;
    CStreamServerRelay( const IConfigDb* pCfg )
        : super::_MyVirtBase( pCfg ), super( pCfg )
    {;}

    virtual gint32 OnPingPong(
        HANDLE hChannel,
        bool bPing = true );
};

// this interface will be hosted by
// CRpcTcpBridgeProxyImpl
class CStreamProxyRelay :
    public CStreamRelayBase< CStreamProxy >
{
    protected:
    gint32 FetchData_Proxy(
        IConfigDb* pDataDesc,           // [in, out]
        gint32& fd,                     // [out]
        guint32& dwOffset,              // [in, out]
        guint32& dwSize,                // [in, out]
        IEventSink* pCallback );

    public:

    typedef CStreamRelayBase< CStreamProxy > super;
    CStreamProxyRelay( const IConfigDb* pCfg )
        : super::_MyVirtBase( pCfg ), super( pCfg )
    {;}

    virtual gint32 OnPingPong(
        HANDLE hChannel,
        bool bPing = true );
};

