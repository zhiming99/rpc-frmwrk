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
 *      Copyright:  2019 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */

#pragma once

#include "configdb.h"
#include "defines.h"
#include "ifhelper.h"
#include <exception>

#include "uxport.h"

namespace rpcfrmwrk
{

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

    inline CRpcServices* GetInterface() const
    { 
        return dynamic_cast< CRpcServices* >
        ( const_cast< IStream* >( this ) );
    }

    inline gint32 GetUxStream(
        HANDLE hChannel, InterfPtr& pIf ) const
    {
        CRpcServices* pThis = GetInterface();
        CStdRMutex oIfLock( pThis->GetLock() );
        UXSTREAM_MAP::const_iterator itr =
            m_mapUxStreams.find( hChannel );

        if( itr == m_mapUxStreams.end() )
            return -ENOENT;
        pIf = itr->second;

        return STATUS_SUCCESS;
    }

    inline gint32 AddUxStream(
        HANDLE hChannel, InterfPtr& pIf )
    {
        CRpcServices* pThis = GetInterface();
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
        CRpcServices* pThis = GetInterface();
        CStdRMutex oIfLock( pThis->GetLock() );

        UXSTREAM_MAP::iterator itr =
            m_mapUxStreams.find( hChannel );

        if( itr == m_mapUxStreams.end() )
            return -ENOENT;

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

    gint32 GetDataDesc( HANDLE hChannel,
        CfgPtr& pDataDesc ) const;

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

    // flow control
    virtual gint32 OnFCLifted( HANDLE hChannel ) = 0;

    virtual gint32 OnFlowControl( HANDLE hChannel )
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
        HANDLE hChannel,
        IEventSink* pCallback = nullptr );

    virtual gint32 CreateUxStream(
        IConfigDb* pDataDesc,
        gint32 iFd,
        EnumClsid iClsid,
        bool bServer,
        InterfPtr& pIf );

    // create the ux socket pair
    gint32 CreateSocket(
        int& fdlocal, int& fdRemote );

    gint32 OnPreStopShared(
        IEventSink* pCallback );

    bool CanSend( HANDLE hChannel );
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
    public virtual CAggInterfaceProxy,
    public IStreamProxy
{

    // send the FETCH_DATA request to the server
    gint32 SendSetupReq(
        IConfigDb* pDataDesc,
        int fd, IEventSink* pCallback );

    public:
    typedef CAggInterfaceProxy super;
    typedef CAggInterfaceProxy _MyVirtBase;

    using IStream::OnUxStreamEvent;
    using IStream::OnStmRecv;
    using IStream::OnChannelError;
    using IStream::OnFCLifted;
    using IStream::OnClose;

    CStreamProxy( const IConfigDb* pCfg )
        :super( pCfg )
    {}

    const EnumClsid GetIid() const
    { return iid( IStream ); }

    virtual gint32 CanContinue()
    { return 0; }

    virtual gint32 InitUserFuncs()
    {
        BEGIN_IFPROXY_MAP( IStream, false );
        END_IFPROXY_MAP;
        return 0;
    }

    gint32 OnFCLifted( HANDLE hChannel )
    { return 0; }

    virtual gint32 OnPreStop( IEventSink* pCallback )
    {
        return OnPreStopShared( pCallback );
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
    gint32 StartStream( HANDLE& hChannel,
        IConfigDb* pDesc = nullptr );

    gint32 OnUxStreamEvent(
        HANDLE hChannel,
        guint8 byToken,
        CBuffer* pBuf )
    {
        return IStream::OnUxStreamEvent(
            hChannel, byToken, pBuf );
    }

    virtual gint32 OnClose(
        HANDLE hChannel,
        IEventSink* pCallback = nullptr )
    {
        return IStream::OnClose(
            hChannel, pCallback );
    }

};

class CStreamServer :
    public virtual CAggInterfaceServer,
    public IStreamServer
{
    gint32 CanContinue();

    public:
    typedef CAggInterfaceServer super;
    typedef CAggInterfaceServer _MyVirtBase;

    using IStream::OnUxStreamEvent;
    using IStream::OnStmRecv;
    using IStream::CanSend;
    using IStream::OnFCLifted;
    using IStream::OnClose;

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

    gint32 OnFCLifted( HANDLE hChannel )
    { return 0; }

    virtual gint32 OnPreStop( IEventSink* pCallback )
    {
        return OnPreStopShared( pCallback );
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

    virtual gint32 OnClose(
        HANDLE hChannel,
        IEventSink* pCallback = nullptr )
    {
        return IStream::OnClose(
            hChannel, pCallback );
    }
};

class CIfCreateUxSockStmTask :
    public CIfInterceptTaskProxy
{
    bool m_bServer = false;
    public:
    typedef CIfInterceptTaskProxy super;

    CIfCreateUxSockStmTask( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CIfCreateUxSockStmTask ) ); }

    gint32 RunTask();
    gint32 OnTaskComplete( gint32 iRet );
    gint32 GetResponse();
};

class CIfStartUxSockStmTask :
    public CIfInterceptTaskProxy
{
    bool m_bServer = false;
    public:
    typedef CIfInterceptTaskProxy super;

    CIfStartUxSockStmTask( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CIfStartUxSockStmTask ) ); }

    gint32 RunTask();
    gint32 OnTaskComplete( gint32 iRet );
};

class CIfStopUxSockStmTask :
    public CIfInterceptTaskProxy
{
    bool m_bServer = false;

    public:
    typedef CIfInterceptTaskProxy super;

    CIfStopUxSockStmTask( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CIfStopUxSockStmTask ) ); }

    gint32 RunTask();
    gint32 OnTaskComplete( gint32 iRet );
};

extern gint32 GetObjIdHash(
    guint64 qwObjId, guint64& qwHash );

}
