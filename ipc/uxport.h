/*
 * =====================================================================================
 *
 *       Filename:  uxport.h
 *
 *    Description:  declaration of unix stream sock bus port and pdo
 *
 *        Version:  1.0
 *        Created:  06/11/2019 12:14:56 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi(woodhead99@gmail.com )
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
#include "mainloop.h"
#include "dbusport.h"
#include "frmwrk.h"

#define STM_MAX_BYTES_PER_BUF ( 63 * 1024 )
#define STM_MAX_PENDING_WRITE ( 1 * 1024 * 1024 )
#define STM_MAX_PACKETS_REPORT 32

#define STM_MAX_QUEUE_SIZE  128
#define UXPKT_HEADER_SIZE   ( sizeof( guint32 ) + sizeof( guint8 ) )
#define UXBUF_OVERHEAD      8

enum EnumPktToken : guint8
{
    tokInvalid = 255,

    // the user's data payload
    tokData = 1,
    // the command is initiated by the proxy side with
    // tokPing, and the server side must respond with
    // tokPong. the interval of each ping/pong is
    // specified by propKeepAliveSec in the
    // FETCH_DATA's request.
    tokPing,
    tokPong,
    // this command appears only as a event to the
    // upper interface, to indicate the unix socket is
    // shutdown.
    tokClose,
    // tokProgress is for progress event, as is sent by
    // the receiver to the sender as a feedback for the
    // current transfer. it is not response for a
    // specific block data received. the token is
    // followed by a IConfigDb
    tokProgress,
    // flow control token goes only through the TCP
    // connection, or as the parameter for
    // CTRLCODE_STREAM_CMD
    tokFlowCtrl,
    tokLift,
    // the token error is to report error happens in
    // the socket. it should not appear on the wire, so
    // far.
    tokError,
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
    guint8  m_byToken;
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

    // for tokData only
    guint32 m_dwRestBytes;
    guint32 m_dwBlockSent;

    std::deque< STM_PACKET > m_queBufWrite;

    public:
    enum SendState
    {
        sendTok,
        sendSize,
        sendPayload,
    };

    // query the bytes waiting for reading

    CSendQue( int iFd ) :
        m_iFd( iFd ),
        m_dwBytesToWrite( 0 ),
        m_dwOffsetWrite( ( guint32 )-1 ),
        m_dwBytesPending( 0 ),
        m_dwRestBytes( 0 ),
        m_dwBlockSent( 0 )
    {}

    SendState GetSendState() const;
    // query the bytes waiting for writing
    guint32 GetPendingWrite() const
    { return m_dwBytesPending; }

    gint32 OnIoReady();

    gint32 WriteStream( BufPtr& pBuf,
        guint8 byToken = tokData );

    gint32 SendPingPong( bool bPing = true );
    gint32 SendClose();
};

class CIoWatchTask:
    public CIfParallelTask
{
    int m_iFd;
    HANDLE m_hReadWatch;
    HANDLE m_hWriteWatch;
    HANDLE m_hErrWatch;

    PortPtr     m_pPort;
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
    gint32 m_iWatTimerId;

    protected:

    gint32 StartStopTimer( bool bStart );
    gint32 StartStopWatch(
        bool bStop, bool bWrite );

    public:
    typedef CIfParallelTask super;

    CIoWatchTask( const IConfigDb* pCfg );

    inline CIoManager* GetIoMgr() const
    {
        CCfgOpenerObj oCfg( this );
        CIoManager* pMgr = nullptr;
        gint32 ret = oCfg.GetPointer(
            propIoMgr, pMgr );
        if( ERROR( ret ) )
            return nullptr;
        return pMgr;
    }

    static int GetFd( const IConfigDb* pCfg )
    {
        gint32 ret = 0;
        CCfgOpener oCfg( pCfg );
        gint32 iFd = 0;
        ret = oCfg.GetIntProp( propFd,
            ( guint32& )iFd );
        if( ERROR( ret ) )
            return -1;
        return iFd;
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

    gint32 OnProgress( BufPtr& pBuf );

    // received by server to notify a client initiated
    // close
    gint32 OnClose();

    void OnError( gint32 iError );

    inline gint32 StartWatch( bool bWrite = true )
    { return StartStopWatch( false, bWrite ); }

    inline gint32 StopWatch( bool bWrite = true )
    { return StartStopWatch( true, bWrite ); }

    inline gint32 StartTimer()
    { return StartStopTimer( true ); }

    inline gint32 StopTimer()
    { return StartStopTimer( false ); }

    // CIfParallelTask's methods
    virtual gint32 OnComplete( gint32 iRet )
    {
        ReleaseChannel();
        gint32 ret = super::OnComplete( iRet );
        m_pPort.Clear();
        return ret;
    }

    virtual gint32 OnTaskComplete( gint32 iRet )
    { return iRet; }

    virtual gint32 RunTask();

    // we have some special processing for eventIoWatch
    // to bypass the normal CIfParallelTask's process
    // flow.
    virtual gint32 OnEvent(
        EnumEventId iEvent,
        LONGWORD dwParam1,
        LONGWORD dwParam2,
        LONGWORD* pData );

    gint32 operator()( guint32 dwContext );
    gint32 OnIoWatchEvent( guint32 dwContext );

    // query the bytes waiting for writing
    guint32 GetPendingWrite() const
    { return m_oSendQue.GetPendingWrite(); }

    // query the bytes waiting for reading
    guint32 GetPendingRead()
    { return m_oRecvFilter.GetPendingRead(); }

    virtual gint32 WriteStream( BufPtr& pBuf,
        guint8 byToken = tokData )
    {
        m_qwBytesWrite += pBuf->size();
        gint32 ret = m_oSendQue.WriteStream(
            pBuf, byToken );
        return ret;
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

class CUnixSockStmPdo : public CPort
{
    bool    m_bStopReady = false;
    // flag to allow or deny incoming data
    bool    m_bFlowCtrl = false;
    bool    m_bListenOnly = false;

    protected:

    gint32 SubmitReadIrp( IRP* pIrp );
    gint32 SubmitWriteIrp( IRP* pIrp );
    gint32 SubmitIoctlCmd( IRP* pIrp );
    gint32 HandleListening( IRP* pIrp );
    gint32 HandleStreamCommand( IRP* pIrp );

    // event queues
    std::deque< BufPtr > m_queEventPackets;
    std::deque< IrpPtr > m_queListeningIrps;
 
    // data queues
    std::deque< BufPtr > m_queDataPackets;
    std::deque< IrpPtr > m_queReadingIrps;

    // writing queues
    std::deque< IrpPtr > m_queWritingIrps;

    TaskletPtr m_pIoWatch;

    public:

    typedef CPort super;

    // the configuration must includes the
    // following properties
    //
    // propIoMgr
    //
    // propBusPortPtr
    //
    // 
    // propPortId
    //
    // optional properties:
    //
    // propPortClass
    // propPortName
    //
    // ;there must be one of the following two options
    // propFd for unamed sock creation
    // propPath for named sock connection
    //
    CUnixSockStmPdo ( const IConfigDb* pCfg );
    ~CUnixSockStmPdo()
    {;}

    virtual gint32 CancelFuncIrp(
        IRP* pIrp, bool bForce );
    gint32 RemoveIrpFromMap( IRP* pIrp );
    virtual gint32 OnSubmitIrp( IRP* pIrp );

    // make the active connection if not connected
    // yet. Note that, within the PostStart, the
    // eventPortStarted is not sent yet
    virtual gint32 PostStart( IRP* pIrp );

	virtual gint32 PreStop( IRP* pIrp );

    // events from the watch task
    // data received from local sock
    gint32 OnStmRecv( BufPtr& pBuf );

    gint32 OnUxSockEvent(
        guint8 byToken, BufPtr& pBuf );

    // the local sock is ready for sending
    gint32 OnSendReady();
    inline bool IsConnected()
    {
        if( m_pIoWatch.IsEmpty() )
            return false;
        CIfParallelTask* pTask = m_pIoWatch;
        if( pTask == nullptr )
            return false;

        return stateStarted ==
            pTask->GetTaskState();
    }

    gint32 SendNotify(
        guint8 byToken, BufPtr& pBuf );

    gint32 OnPortReady( IRP* pIrp );
};

class CUnixSockBusPort :
    public CGenericBusPort
{
    gint32 CreateUxStreamPdo(
        IConfigDb* pConfig,
        PortPtr& pNewPort );

    public:
    typedef CGenericBusPort super;
    CUnixSockBusPort( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CUnixSockBusPort ) ); }

    virtual gint32 BuildPdoPortName(
        IConfigDb* pCfg,
        std::string& strPortName );

    virtual gint32 CreatePdoPort(
        IConfigDb* pConfig,
        PortPtr& pNewPort );
};

class CUnixSockBusDriver :
    public CGenBusDriver
{
    public:
    typedef CGenBusDriver super;

    CUnixSockBusDriver(
        const IConfigDb* pCfg = nullptr );

	gint32 Probe( IPort* pLowerPort,
        PortPtr& pNewPort,
        const IConfigDb* pConfig = NULL );
};

