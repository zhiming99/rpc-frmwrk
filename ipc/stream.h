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

#define STM_MAX_BYTES_PER_BUF ( 32 * 1024 )

enum EnumPktToken : guint8
{
    tokInvalid = 255,
    tokData = 1,
    tokPing,
    tokPong,
    tokClose
};

// Packet format:
//
// Data Packet:
// tokData( guint8 ), size( guint32 ), <payload>
//
// command packets:
// [ tokPing | tokPong | tokClose ]

struct STM_PACKET
{
    guint8 byToken;
    guint32 dwSize;
    BufPtr pData;
    STM_PACKET() :
    byToken( tokInvalid ),
    dwSize( 0 )
    {}
};

class CRecvFilter
{
    int m_iFd;
    // the last byte readin in the tail buf of the
    // queue, each non-tail buffer has the size of 
    // STM_MAX_BYTES_PER_BUF
    guint32 m_dwBytesToRead;
    guint32  m_dwOffsetRead;
    guint32 m_dwBytesPending;
    std::deque< BufPtr > m_queBufRead;

    public:
    enum RecvState
    {
        recvTok,
        recvSize,
        recvData
    };
    // query the bytes waiting for reading
    CRecvFilter( int iFd ) :
        m_iFd( iFd ),
        m_dwBytesToRead( 0 ),
        m_dwOffsetRead( ( guint32 ) -1 ),
        m_dwBytesPending( 0 )
    {}

    RecvState GetRecvState() const;
    inline guint32 GetPendingRead() const
    { return m_dwBytesPending; }

    gint32 OnIoReady();
    gint32 ReadStream( BufPtr& pBuf );
};

class CSendQue
{
    int m_iFd;
    // the last bytes written in the head buf of the
    // queue.
    guint32 m_dwBytesToWrite;
    guint32 m_dwOffsetWrite;
    guint32 m_dwBytesPending;

    std::deque< STM_PACKET > m_queBufWrite;

    inline gint32 Send( int iFd,
        void* pBuf, guint32 dwSize )
    {
        return send( iFd, pBuf,
            dwSize, MSG_NOSIGNAL );
    }

    public:
    enum SendState
    {
        sendTok,
        sendSize,
        sendData
    };

    // query the bytes waiting for reading

    CSendQue( int iFd ) :
        m_iFd( iFd ),
        m_dwBytesToWrite( 0 ),
        m_dwOffsetWrite( ( guint32 )-1 ),
        m_dwBytesPending( 0 )
    {}

    SendState GetSendState() const;
    // query the bytes waiting for writing
    guint32 GetPendingWrite() const
    { return m_dwBytesPending; }

    gint32 OnIoReady();

    gint32 WriteStream( BufPtr& pBuf );
    gint32 SendPingPong( bool bPing = true );
    gint32 SendClose();
};

class IStream;

class CIoWatchTask:
    public CIfParallelTask
{
    protected:

    int m_iFd;
    HANDLE m_hReadWatch;
    HANDLE m_hWriteWatch;

    IStream*    m_pStream;
    CRecvFilter m_oRecvFilter;
    CSendQue    m_oSendQue;
    guint64     m_qwBytesRead;
    guint64     m_qwBytesWrite;

    // using interface state for stream state three
    // states used:
    //
    // stateStarting, connecting in process, the server
    // will send a  handshake via the stream to notify
    // the connection is ok
    //
    // stateConnected, connected
    //
    // stateStopped, the stream is closed
    std::atomic< EnumIfState > m_dwState;

    gint32 StartStopWatch(
        bool bStop, bool bWrite );

    public:
    typedef CIfParallelTask super;
    CIoWatchTask( const IConfigDb* pCfg ) :
    super( pCfg ),
    m_iFd( -1 ),
    m_hReadWatch( 0 ),
    m_hWriteWatch( 0 ),
    m_pStream( nullptr ),
    m_oRecvFilter( GetFd( pCfg ) ),
    m_oSendQue( GetFd( pCfg ) ),
    m_qwBytesRead( 0 ),
    m_qwBytesWrite( 0 ),
    m_dwState( stateStarting )
    {}

    static int GetFd( const IConfigDb* pCfg )
    {
        gint32 ret = 0;
        CCfgOpener oCfg( pCfg );
        guint32 dwFd = 0;
        ret = oCfg.GetIntProp( 0, dwFd );
        if( ERROR( ret ) )
            return -1;
        return dwFd;
    }

    // watch handler for error
    inline gint32 OnIoError( guint32 revent )
    {
        OnError( -EIO );
        // return error to remove the watch immediately
        // from the mainloop
        return -EIO;
    }

    // watch handler for I/O events
    gint32 OnIoReady( guint32 revent );

    // new data arrived
    gint32 OnDataReady( BufPtr pBuf );

    gint32 OnSendReady();

    // ping/pong packets
    virtual gint32 OnPingPong( bool bPing );

    // received by server to notify a client initiated
    // close
    gint32 OnClose();

    void OnError( gint32 iError );

    inline gint32 StartWatch( bool bWrite = true )
    { return StartStopWatch( false, bWrite ); }

    inline gint32 StopWatch( bool bWrite = true )
    { return StartStopWatch( true, bWrite ); }

    // CIfParallelTask's methods
    virtual gint32 OnComplete( gint32 iRet )
    {
        ReleaseChannel();
        return super::OnComplete( iRet );
    }

    virtual gint32 RunTask();

    // we have some special processing for eventIoWatch
    // to bypass the normal CIfParallelTask's process
    // flow.
    virtual gint32 OnEvent(
        EnumEventId iEvent,
        guint32 dwParam1,
        guint32 dwParam2,
        guint32* pData );

    gint32 operator()( guint32 dwContext );
    gint32 OnIoWatchEvent( guint32 dwContext );

    // query the bytes waiting for writing
    guint32 GetPendingWrite()
    { return m_oSendQue.GetPendingWrite(); }

    // query the bytes waiting for reading
    guint32 GetPendingRead()
    { return m_oRecvFilter.GetPendingRead(); }

    virtual gint32 WriteStream( BufPtr& pBuf )
    {
        m_qwBytesWrite += pBuf->size();
        return m_oSendQue.WriteStream( pBuf );
    }

    // release the watches and fds
    virtual gint32 ReleaseChannel();

    inline gint32 SendPingPong( bool bPing = true )
    { return m_oSendQue.SendPingPong( bPing ); }

    inline gint32 SendClose()
    { return m_oSendQue.SendClose(); }

    inline guint64 GetBytesRead()
    { return m_qwBytesRead; }

    inline guint64 GetBytesWrite()
    { return m_qwBytesWrite; }
};

class CIoWatchTaskProxy :
    public CIoWatchTask
{
    TaskletPtr m_pIoTask;

    // semaphore for proxy to wait for the IoTask to
    // complete
    sem_t m_semWait;
    gint32 m_iTimerId;

    public:

    typedef CIoWatchTask super;
    CIoWatchTaskProxy( const IConfigDb* pCfg )
        : super( pCfg ), m_iTimerId( 0 )
    {
        SetClassId( clsid( CIoWatchTaskProxy ) );
        Sem_Init( &m_semWait, 0, 0 );
    }

    ~CIoWatchTaskProxy()
    { sem_destroy( &m_semWait ); }

    gint32 OnConnected();

    inline void SetReqTask( TaskletPtr pTask )
    { m_pIoTask = pTask; }

    inline TaskletPtr GetReqTask() const
    { return m_pIoTask; }

    virtual gint32 WriteStream( BufPtr& pBuf )
    {
        if( m_dwState != stateConnected )
            return -EAGAIN;
        return super::WriteStream( pBuf );
    }

    virtual gint32 OnTaskComplete( gint32 iRet )
    {
        Sem_Post( &m_semWait );
        return 0;
    }

    virtual gint32 ReleaseChannel();

    virtual gint32 CloseChannel()
    { return SendClose(); }

    virtual gint32 OnPingPong( bool bPing );

    inline gint32 WaitForComplete()
    { return Sem_Wait( &m_semWait ); }

    gint32 RunTask();
    gint32 OnKeepAlive( guint32 dwContext );
};

class CIoWatchTaskServer :
    public CIoWatchTask
{
    TaskletPtr m_pInvTask;
    CfgPtr m_pDataDesc;

    public:
    typedef CIoWatchTask super;
    CIoWatchTaskServer( const IConfigDb* pCfg )
        : super( pCfg )
    {
        SetClassId( clsid( CIoWatchTaskServer ) );
        // set to connected at the very beginning
        m_dwState = stateConnected;
    }

    gint32 ReleaseChannel()
    {
        gint32 ret = super::ReleaseChannel();
        m_pInvTask.Clear();
        m_pDataDesc.Clear();
        return ret;
    }

    virtual gint32 RunTask();

    inline TaskletPtr& GetInvokeTask() const
    { return const_cast< TaskletPtr& >( m_pInvTask ); }

    gint32 OnPingPong( bool bPing );
};

class CIoWatchTaskRelay :
    public CIoWatchTask
{
    TaskletPtr m_pInvTask;
};

struct IStream
{
    typedef std::map< guint32, TaskletPtr > WATCH_MAP;
    WATCH_MAP m_mapWatches;

    virtual gint32 CanContinue() = 0;

    gint32 GetWatchTask( HANDLE hChannel,
        TaskletPtr& pTask ) const;

    virtual gint32 OpenChannel(
        IConfigDb* pDataDesc,
        int& fd, HANDLE& hChannel,
        IEventSink* pCallback ) = 0; 

    virtual gint32 CloseChannel(
        HANDLE hChannel ) = 0;

    gint32 RemoveChannel( HANDLE hChannel );

    gint32 WriteStream(
        HANDLE hChannel, BufPtr& pBuf );

    gint32 SendPingPong( HANDLE hChannel,
        bool bPing = true );

    // data is ready for reading
    virtual gint32 OnStmRecv(
        HANDLE hChannel, BufPtr& pBuf ) = 0;

    virtual gint32 OnPingPong(
        HANDLE hChannel,
        bool bPing = true )
    { return 0; }

    virtual gint32 OnSendReady(
        HANDLE hChannel )
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
};

struct IStreamServer : public IStream
{
    // callback when a close token is received
    virtual gint32 OnClose(
        HANDLE hChannel ) = 0;
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

    gint32 CreateSocket(
        int& fdlocal, int& fdRemote );

    // send the FETCH_DATA request to the server
    gint32 SendRequest(
        IConfigDb* pDataDesc,
        int fd, IEventSink* pCallback );

    public:
    typedef CInterfaceProxy super;

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

    // call this method when the proxy want to end the
    // stream actively
    virtual gint32 CloseChannel(
        HANDLE hChannel );

    gint32 CancelReqTask( IEventSink* pTask,
        IEventSink* pWatchTask );

    // call this method when the proxy encounters
    // fatal error
    gint32 CancelChannel( HANDLE hChannel );

    // call this helper to start a stream channel
    gint32 StartStream( HANDLE& hChannel,
        TaskletPtr& pSyncTask );

};

class CStreamServer :
    public virtual CInterfaceServer,
    public IStreamServer
{
    gint32 CanContinue();

    gint32 CompleteInvokeTask(
        IConfigDb* pResp, IEventSink* pInvTask )
    {
        return this->OnServiceComplete(
            pResp, pInvTask );
    }

    public:
    typedef CInterfaceServer super;
    CStreamServer( const IConfigDb* pCfg )
        :super( pCfg )
    {}
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

    gint32 OnClose( HANDLE hChannel );
    gint32 CloseChannel( HANDLE hChannel );

};

class CStreamServerRelay :
    public CStreamServer
{
    public:
    virtual gint32 CloseChannel( HANDLE hChannel );
};

class CStreamProxyRelay :
    public CStreamProxy
{
    public:
    virtual gint32 CloseChannel( HANDLE hChannel );
};
