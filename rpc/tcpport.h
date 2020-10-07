/*
 * =====================================================================================
 *
 *       Filename:  tcpport.h
 *
 *    Description:  the tcp port and socket declaration for the rpc router
 *
 *        Version:  1.0
 *        Created:  04/11/2016 10:11:26 PM
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
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "portex.h"

#define RPC_MAX_BYTES_PACKET    ( 65536 - sizeof( CPacketHeader ) - 32 )

#define TCP_CONN_DEFAULT_CMD    0
#define TCP_CONN_DEFAULT_STM    1
#define TCP_CONN_SESS_MGR       2

#define STMSOCK_STMID_FLOOR 0x400

#define IsReserveStm( _iStmId ) \
( ( _iStmId ) >= 0 && ( _iStmId ) < STMSOCK_STMID_FLOOR )

#define RETRIABLE( iRet_ ) \
    ( iRet_ == EWOULDBLOCK || iRet_ == EAGAIN )

enum EnumProtoId
{
    protoDBusRelay,
    protoStream,
    protoControl
};

enum EnumSockState
{
    sockInvalid = 0,
    // the initial state
    sockInit,
    // the connection is estabilished and the port
    // is started
    sockStarted,
    // the connection is losted, but not stopped
    // yet
    sockDisconnected,
    sockStopped,
};

#define RPC_PACKET_MAGIC    ( *( ( guint32* )"PHdr" ) )
#define RPC_PACKET_FLG_COMPRESS       0x01

#define RPC_PACKET_FLG_SECMASK        0x06

#define RPC_PACKET_FLG_CLEARTXT       0x00
#define RPC_PACKET_FLG_SIGNED         0x02
#define RPC_PACKET_FLG_ENCRYPTED      0x04

#define ENC_TYPE( _flag ) \
    ( ( _flag ) & RPC_PACKET_FLG_SECMASK )

#define SET_ENC_TYPE( _flag, _type ) \
    ( ( ( _flag ) & ~RPC_PACKET_FLG_SECMASK ) | ( _type ) )

struct CPacketHeader
{
    guint32     m_dwMagic;

    // byte count of the payload after this header
    guint32     m_dwSize;

    // the sender's stream id
    gint32      m_iStmId;

    // the receiver's stream id
    gint32      m_iPeerStmId;
    guint16     m_wFlags;
    guint16     m_wProtoId;
    guint32     m_dwSessId;
    guint32     m_dwSeqNo;

    CPacketHeader() :
        m_dwSize( 0 ),
        m_iStmId( 0 ),
        m_iPeerStmId( 0 ),
        m_wFlags( 0 ),
        m_wProtoId( 0 ),
        m_dwSessId( 0 ),
        m_dwSeqNo( 0 )
    {
        m_dwMagic = RPC_PACKET_MAGIC;
    }

    void ntoh()
    {
        m_dwMagic = ntohl( m_dwMagic );
        m_dwSize = ntohl( m_dwSize );
        m_iStmId = ntohl( m_iStmId );
        m_iPeerStmId = ntohl( m_iPeerStmId );
        m_wFlags = ntohs( m_wFlags );
        m_wProtoId = ntohs( m_wProtoId );
        m_dwSessId = ntohl( m_dwSessId );
        m_dwSeqNo = ntohl( m_dwSeqNo );
    }

    void hton()
    {
        m_dwMagic = htonl( m_dwMagic );
        m_dwSize = htonl( m_dwSize );
        m_iStmId = htonl( m_iStmId );
        m_iPeerStmId = htonl( m_iPeerStmId );
        m_wFlags = htons( m_wFlags );
        m_wProtoId = htons( m_wProtoId );
        m_dwSessId = htonl( m_dwSessId );
        m_dwSeqNo = htonl( m_dwSeqNo );
    }

    gint32 Deserialize(
        const char* pBuf, guint32 dwSize )
    {
        const CPacketHeader* pHeader =
            reinterpret_cast < const CPacketHeader* >( pBuf );

        if( pHeader == nullptr
            || dwSize < sizeof( CPacketHeader ) )
            return -EPROTO;

        if( ntohl( pHeader->m_dwMagic ) != RPC_PACKET_MAGIC )
            return -EPROTO;

        m_dwMagic       = pHeader->m_dwMagic;
        m_dwSize        = pHeader->m_dwSize;
        m_wFlags        = pHeader->m_wFlags;
        m_iStmId        = pHeader->m_iStmId;
        m_iPeerStmId    = pHeader->m_iPeerStmId;
        m_wProtoId      = pHeader->m_wProtoId;
        m_dwSeqNo       =  pHeader->m_dwSeqNo;
        m_dwSessId      = pHeader->m_dwSessId;

        ntoh();

        return 0;
    }

    gint32 Serialize( CBuffer& oBuf )
    {
        oBuf.Resize( sizeof( CPacketHeader ) );

        CPacketHeader* pHeader =
            ( CPacketHeader* )oBuf.ptr();

        if( pHeader == nullptr )
            return -EFAULT;

        pHeader->m_dwMagic       = m_dwMagic;
        pHeader->m_dwSize        = m_dwSize;
        pHeader->m_wFlags        = m_wFlags;
        pHeader->m_iStmId        = m_iStmId;
        pHeader->m_iPeerStmId    = m_iPeerStmId;
        pHeader->m_wProtoId      = m_wProtoId;
        pHeader->m_dwSeqNo       = m_dwSeqNo;
        pHeader->m_dwSessId      = m_dwSessId;

        pHeader->hton();
        return 0;
    }
};

class CCarrierPacket :
    public CObjBase
{
    // provide the storage for the underlying
    // pointer m_pPayload
    protected:

    BufPtr          m_pBuf;
    CPacketHeader   m_oHeader;

    public:

    typedef CObjBase super;

    CCarrierPacket();
    ~CCarrierPacket();

    inline void ntoh()
    {  m_oHeader.ntoh(); }

    inline void hton()
    {  m_oHeader.hton(); }

    inline gint32 GetStreamId(
        gint32& iStmId ) const
    {
        iStmId = m_oHeader.m_iStmId;
        return 0;
    }

    inline gint32 GetPeerStmId(
        gint32& iStmId ) const
    {
        iStmId = m_oHeader.m_iPeerStmId;
        return 0;
    }

    inline gint32 GetProtoId( gint32 iProtoId ) const
    {
        iProtoId = ( gint32 )m_oHeader.m_wProtoId;
        return 0;
    }

    inline gint32 GetSeqNo( guint32& dwSeqNo ) const
    {
        dwSeqNo = m_oHeader.m_dwSeqNo;
        return 0;
    }

    inline gint32 GetPayload( BufPtr& pBuf )
    {
        if( m_pBuf.IsEmpty() )
            return -EFAULT;
        pBuf = m_pBuf;
        m_pBuf.Clear();
        return 0;
    }

    inline void SetPayload( BufPtr& pBuf )
    {
        m_pBuf = pBuf;
    }

    inline guint32 GetFlags()
    { return m_oHeader.m_wFlags; }

    inline const CPacketHeader& GetHeader() const
    { return m_oHeader; }
};

typedef CAutoPtr< Clsid_Invalid, CCarrierPacket > PacketPtr;

class COutgoingPacket :
    public CCarrierPacket
{
    // this object must be guaranteed to have its
    // life cycle contained within the irp's life
    // cycle
    IRP*        m_pIrp;
    guint32     m_dwBytesToSend;
    guint32     m_dwOffset;
    gint32      GetBytesToSend();

    public:
    typedef CCarrierPacket super;

    COutgoingPacket()
        : super(),
        m_pIrp( nullptr ),
        m_dwBytesToSend( 0 ),
        m_dwOffset( 0 )
    { SetClassId( clsid( COutgoingPacket ) ); }
    gint32 GetIrp( IrpPtr& pIrp ) const
    {
        if( m_pIrp == nullptr )
            return -EFAULT;
        pIrp = m_pIrp;
        return 0;
    }
    gint32 SetIrp( IRP* pIrp );
    gint32 GetIrp( IrpPtr& pIrp )
    {
        pIrp = m_pIrp;
        return 0;
    }
    gint32 SetBufToSend( CBuffer* pBuf );
    gint32 SetHeader( CPacketHeader* pHeader );
    gint32 StartSend( gint32 iFd );
    bool IsSending()
    { return ( m_dwOffset > 0 ); }

};

class CIncomingPacket:
    public CCarrierPacket
{
    guint32     m_dwBytesToRecv;
    guint32     m_dwOffset;

    public:
    typedef CCarrierPacket super;

    CIncomingPacket();
    gint32 FillPacket( gint32 iFd );
    gint32 FillPacket( BufPtr& pBuf );
};

class CRpcSocketBase;
class CRpcStreamSock;

#define STM_MAX_RECV_PACKETS    100
class CRpcStream :
    public CObjBase
{
    friend class CRpcStreamSock;

    protected:
    // the queue of outgoing packets, waiting for
    // socket ready to send
    gint32                      m_iStmId;
    gint32                      m_iPeerStmId;
    gint16                      m_wProtoId;

    std::deque< PacketPtr >     m_queBufToSend;

    // the queue of incoming packets, waiting for
    // reading operation
    std::deque< PacketPtr >     m_queBufToRecv;
    std::deque< IrpPtr >        m_queReadIrps;
    CIoManager                  *m_pMgr;

    CRpcStreamSock              *m_pStmSock;

    // sequence number for outgoing packet
    std::atomic< guint32 >      m_atmSeqNo;
    std::atomic< guint32 >      m_dwAgeSec;

    public:

    typedef CObjBase super;
    CRpcStream( const IConfigDb* pCfg );

    bool HasBufToSend() const
    { return !m_queBufToSend.empty(); }

    inline gint32 GetStreamId() const
    { return m_iStmId; }

    inline guint16 GetProtoId() const
    { return m_wProtoId; }

    inline gint32 SetStmId(
        gint32 iStmId, gint16 wProtoId )
    {
        m_iStmId = iStmId;
        m_wProtoId = wProtoId;
        return 0;
    }

    inline gint32 SetPeerStmId(
        gint32 iPeerId )
    {
        m_iPeerStmId = iPeerId;
        return 0;
    }

    inline void AgeStream( guint32 dwSeconds )
    {
        if( IsReserveStm( m_iStmId ) )
            return;
        m_dwAgeSec += dwSeconds;
    }

    inline void Refresh( )
    { m_dwAgeSec = 0; }

    inline gint32 GetPeerStmId(
        gint32& iPeerId ) const
    {
        iPeerId = m_iPeerStmId;
        return 0;
    }

    virtual void ClearAllQueuedIrps()
    {
        m_queReadIrps.clear();
        m_queBufToSend.clear();
    }

    CIoManager* GetIoMgr()
    { return m_pMgr; }

    guint32 GetSeqNo()
    { return m_atmSeqNo++; }

    // return 0 if the packet is sent,
    // return STATUS_PENDING if the packet is not
    // sent completely
    // only when StartSend complete the packet,
    // can we move to the next stream or next
    // packet.
    // the irp is the one to complete if the
    // packet is sent.
    virtual gint32 StartSend( gint32 iFd, IrpPtr& pIrp );

    gint32 QueuePacketOutgoing(
        CCarrierPacket* pPacket );

    virtual gint32 RecvPacket(
        CCarrierPacket* pPacketToRecv );

    virtual gint32 SetRespReadIrp(
        IRP* pIrp, CBuffer* pBuf );

    virtual gint32 GetReadIrpsToComp(
        std::vector< std::pair< IrpPtr, BufPtr > >& );

    virtual gint32 HandleReadIrp(
        IRP* pIrp );

    virtual gint32 HandleWriteIrp( IRP* pIrp,
        BufPtr& pPayload, guint32 dwSeqNo = 0 );

    virtual gint32 RemoveIrpFromMap(
        IRP* pIrp );

    gint32 DispatchDataIncoming(
        CRpcSocketBase* pParentSock );

    virtual gint32 GetAllIrpsQueued(
        std::vector< IrpPtr >& vecIrps );
};

typedef CAutoPtr< clsid( CRpcStream ), CRpcStream > StmPtr;

class CRpcControlStream :
    public CRpcStream
{
    friend class CRpcStreamSock;

    // the queue of outgoing packets, waiting for
    // socket ready to send
    std::map< gint32, IrpPtr >  m_mapIrpsForResp;

    // we added the evt queue for proxy waiting on
    // the event.
    std::deque< IrpPtr >        m_queReadIrpsEvt;
    std::deque< CfgPtr >        m_queBufToRecv2;

    gint32 RemoveIoctlRequest( IRP* pIrp );

    MatchPtr                    m_pStmMatch;

    public:

    typedef CRpcStream super;

    CRpcControlStream( const IConfigDb* pCfg );

    gint32 SetRespReadIrp(
        IRP* pIrp, CBuffer* pBuf );

    gint32 GetReadIrpsToComp(
        std::vector< std::pair< IrpPtr, BufPtr > >& );

    gint32 RemoveIrpFromMap( IRP* pIrp );

    gint32 HandleListening( IRP* pIrp );

    virtual gint32 HandleReadIrp( IRP* pIrp )
    { return -ENOTSUP; }

    virtual gint32 HandleWriteIrp( IRP* pIrp,
        BufPtr& pPayload, guint32 dwSeqNo = 0 )
    { return -ENOTSUP; }

    gint32 HandleIoctlIrp( IRP* pIrp );
    gint32 QueueIrpForResp( IRP* pIrp );

    virtual gint32 GetAllIrpsQueued(
        std::vector< IrpPtr >& vecIrps );

    virtual void ClearAllQueuedIrps()
    {
        super::ClearAllQueuedIrps();
        m_queReadIrpsEvt.clear();
    }

    virtual gint32 RecvPacket(
        CCarrierPacket* pPacketToRecv );
};

class CRpcSockWatchCallback : public CTasklet
{
    public:

    typedef CTasklet super;
    CRpcSockWatchCallback( const IConfigDb* pCfg )
        : super( pCfg )
    {
        SetClassId( clsid( CRpcSockWatchCallback ) );
    }
    gint32 operator()( guint32 dwContext );
};

#define SOCK_TIMER_SEC      100
#define SOCK_LIFE_SEC       1800

class CRpcSocketBase : public IService
{
    // socket options
    protected:
    CfgPtr              m_pCfg;
    EnumSockState       m_iState;
    gint32              m_iFd;
    CIoManager*         m_pMgr;
    guint32             m_dwRtt;
    mutable stdrmutex    m_oLock;
    std::atomic< guint32 > m_dwAgeSec;
    gint32              m_iTimerId;
    PacketPtr           m_pPackReceiving;
    IPort*              m_pParentPort;

    // watch for receiving
    HANDLE              m_hIoRWatch;
    // watch for sending
    HANDLE              m_hIoWWatch;
    HANDLE              m_hErrWatch;

    gint32 AttachMainloop();
    gint32 DetachMainloop();
    gint32 UpdateRttTimeMs();
    gint32 StartTimer();
    gint32 StopTimer();
    gint32 CloseSocket();

    virtual gint32 DispatchSockEvent(
        GIOCondition ) = 0;


    public:

    typedef IService super;
    // properties:
    // propSrcIpAddr
    // propDestIpAddr
    // propIoMgr
    // propPortPtr
    //
    // propFd if passive open
    //
    CRpcSocketBase(
        const IConfigDb* pCfg = nullptr );

    ~CRpcSocketBase()
    {
    }
    
    // commands
    // gint32 Start()
    gint32 Stop();

    gint32 GetSockFd( gint32& iFd ) const
    {
        if( m_iFd < 0 )
            return ERROR_STATE;

        iFd = m_iFd;
        return 0;
    }

    static gint32 GetAddrInfo(
        const std::string& strIpAddr,
        guint32 dwPortNum,
        addrinfo*& res );

    gint32 GetAsyncErr() const;
    virtual gint32 SetProperty(
        gint32 iProp, const CBuffer& oBuf )
    {
        CStdRMutex oSockLock( GetLock() );
        if( m_pCfg.IsEmpty() )
            return -EFAULT;

        return m_pCfg->SetProperty(
            iProp, oBuf );
    }

    virtual gint32 GetProperty(
        gint32 iProp, CBuffer& oBuf ) const
    {
        CStdRMutex oSockLock( GetLock() );
        if( m_pCfg.IsEmpty() )
            return -EFAULT;

        return m_pCfg->GetProperty(
            iProp, oBuf );
    }

    virtual gint32 RemoveProperty(
        gint32 iProp )
    {
        CStdRMutex oSockLock( GetLock() );
        return m_pCfg->RemoveProperty( iProp );
    }

    stdrmutex& GetLock() const
    { return m_oLock; }

    inline CIoManager* GetIoMgr() const
    { return m_pMgr; }

    EnumSockState GetState() const;
    gint32 SetState( EnumSockState iState );

    // the tcp-level keep-alive
    gint32 SetKeepAlive( bool bKeep );
    bool IsKeepAlive() const;

    gint32 GetRttTimeMs( guint32 dwMilSec);

    // active connect/disconnect
    bool IsConnected() const;
    gint32 Send( CCarrierPacket* pBuf );

    // events from the socket
    virtual gint32 OnError( gint32 ret ) = 0;
    virtual gint32 OnConnected() = 0;

    virtual gint32 OnSendReady()
    { return -ENOTSUP; }

    virtual gint32 OnReceive()
    { return -ENOTSUP; }

    gint32 ResetSocket();

    // event dispatcher
    gint32 OnEvent(
        EnumEventId iEvent,
        LONGWORD dwParam1,
        LONGWORD dwParam2,
        LONGWORD* pData );

    // Callback for socket events. Note that the
    // 3rd param, `data' must be a pointer to
    // CRpcSocketBase, otherwise unexpect result
    static gboolean SockEventCallback(
        GIOChannel* source,
        GIOCondition condition,
        gpointer data);

    virtual gint32 Connect() = 0;

    gint32 StartWatch( bool bWrite = true );
    gint32 StopWatch( bool bWrite = true );
};

typedef CAutoPtr< Clsid_Invalid, CRpcSocketBase > SockPtr;

class CRpcListeningSock :
    public CRpcSocketBase
{

    virtual gint32 DispatchSockEvent(
        GIOCondition );

    public:

    typedef CRpcSocketBase super;
    CRpcListeningSock( const IConfigDb* pCfg );

    // commands
    gint32 Start();

    // new connection comes
    virtual gint32 OnConnected();
    virtual gint32 OnError( gint32 ret );
    gint32 Connect();
};

class CRpcStreamSock :
    public CRpcSocketBase
{
    std::map< gint32, StmPtr > m_mapStreams;
    std::map< gint32, StmPtr >::iterator m_itrCurStm;
    gint32  m_iCurSendStm;
    gint32  m_iStmCounter;

    TaskletPtr m_pStartTask;

    virtual gint32 DispatchSockEvent(
        GIOCondition );


    gint32 GetCtrlStmFromIrp(
        IRP* pIrp, StmPtr& pStm );

	// virtual gint32 CancelStartIrp(
    //     IRP* pIrp, bool bForce = true );

    public:

    typedef CRpcSocketBase super;

    // properties in pCfg
    //
    // refer to CRpcSocketBase for the rest
    // properties
    CRpcStreamSock(
        const IConfigDb* pCfg );

    gint32 Start_bh();
    gint32 ActiveConnect();

    virtual gint32 Start();
    virtual gint32 Stop();

    virtual gint32 OnSendReady();
    virtual gint32 OnReceive();
    virtual gint32 OnConnected();
    gint32 OnDisconnected();

    gint32 Connect();
    gint32 Disconnect();

    gint32 GetStream( gint32 iIndex,
        StmPtr& pStream )
    {
        if( m_mapStreams.find( iIndex )
            == m_mapStreams.end() )
            return -ENOENT;

        pStream = m_mapStreams[ iIndex ];
        return 0;
    }

    inline gint32 GetStmCount() const
    { return m_mapStreams.size(); }

    gint32 GetStreamByPeerId(
        gint32 iPeerId, StmPtr& pStream );

    gint32 AgeStreams( guint32 dwSeconds )
    {
        for( auto& oPair : m_mapStreams )
        {
            oPair.second->
                AgeStream( dwSeconds );
        }
        return 0;
    }

    gint32 RefreshStream( gint32 iStmId )
    {
        StmPtr pStream;
        gint32 ret = GetStream( iStmId, pStream );
        if( ERROR( ret ) )
            return ret;

        pStream->Refresh();
        return 0;
    }

    gint32 ClearIdleStreams()
    { return 0; }

    gint32 StartSend( IRP* pIrp = nullptr );
    gint32 HandleReadIrp( IRP* pIrp );
    gint32 HandleWriteIrp( IRP* pIrp );

    gint32 HandleIoctlIrp( IRP* pIrp );
    // for ioctl irp
    gint32 QueueIrpForResp( IRP* pIrp );

    gint32 CancelAllIrps( gint32 iErrno );
    gint32 GetStreamFromIrp(
        IRP* pIrp, StmPtr& pStm );

    // stream management
    gint32 AddStream(
        gint32 iStream,
        guint16 wProtoId,
        gint32 iPeerStmId );

    gint32 RemoveStream( gint32 iStream );
    
    // select the eligible stream to send data
    // across all the streams
    gint32 GetStreamToSend( StmPtr& pStm );

    gint32 GetSeedStmId()
    {
        gint32 ret = m_iStmCounter++;
        if( m_iStmCounter < 0 )
        {
            m_iStmCounter = STMSOCK_STMID_FLOOR;
        }
        return ret;
    }

    gint32 RemoveIrpFromMap( IRP* pIrp );
    gint32 NewStreamId( gint32& iStream );
    gint32 ExistStream( gint32 iStream );
    virtual gint32 OnError( gint32 ret );

    // event dispatcher
    gint32 OnEvent(
        EnumEventId iEvent,
        LONGWORD dwParam1,
        LONGWORD dwParam2,
        LONGWORD* pData );

};

struct FIDO_IRP_EXT
{
    FIDO_IRP_EXT()
    {
        iStmId = -1;
        iPeerStmId = -1;
    }

    gint32 iStmId;
    gint32 iPeerStmId;
};

/**
* @name CRpcTcpFido
* A filter port for dbus-related request handling.
* @{ */
/**
* It is a FILTER port and all the stream port
* related requests can pass through to the
* underlying CTcpStreamPdo. And it will handle all
* the dbus related commands and transactions.
* Actually it has all the pdo functionality.
* @} */

class CRpcTcpFido: public CRpcBasePortEx
{
    protected:

    static std::atomic< guint32 > m_atmSeqNo;
    TaskletPtr  m_pListenTask;
    bool    m_bAuth = false;

    std::hashmap< guint32, DMsgPtr > m_mapRespMsgs;
    virtual gint32 OnRespMsgNoIrp(
        DBusMessage* pDBusMsg );

    public:

    typedef CRpcBasePortEx super;

    CRpcTcpFido(
        const IConfigDb* pCfg = nullptr );

    ~CRpcTcpFido();

    virtual gint32 ClearDBusSetting(
        IMessageMatch* pMatch )
    { return 0; }

    virtual gint32 SetupDBusSetting(
        IMessageMatch* pMatch )
    { return 0; }


    gint32 HandleReadWriteReq( IRP* pIrp );
    gint32 CompleteReadWriteReq( IRP* pIrp );

    virtual gint32 HandleSendReq( IRP* pIrp );
    gint32 CompleteSendReq( IRP* pIrp );

    gint32 HandleSendResp( IRP* pIrp );
    gint32 HandleSendEvent( IRP* pIrp );

    gint32 HandleListening( IRP* pIrp );

    virtual gint32 HandleSendData( IRP* pIrp );

    gint32 LoopbackTest( DMsgPtr& pReqMsg );
    public:

    virtual gint32 OnSubmitIrp( IRP* pIrp );
    virtual gint32 SubmitIoctlCmd( IRP* pIrp );
    gint32 SubmitReadWriteCmd( IRP* pIrp );

    virtual gint32 CompleteFuncIrp( IRP* pIrp );
    virtual gint32 CompleteIoctlIrp( IRP* pIrp );
    gint32 CompleteListening( IRP* pIrp );

    virtual gint32 DispatchData( CBuffer* pData );

    gint32 CancelFuncIrp( IRP* pIrp, bool bForce );

    virtual gint32 OnPortReady( IRP* pIrp );

    gint32 ScheduleRecvDataTask(
        BufVecPtr& pVecBuf );

    virtual gint32 BuildSendDataMsg(
        IRP* pIrp, DMsgPtr& pMsg );

    gint32 GetPeerStmId( gint32 iStmId,
        gint32& iPeerId );

    gint32 AllocIrpCtxExt(
        IrpCtxPtr& pIrpCtx,
        void* pContext = nullptr ) const;

    virtual gint32 PostStop( IRP* irp );
};

class CTcpFidoListenTask
    : public CTaskletRetriable
{
    public:
    typedef CTasklet super;
    CTcpFidoListenTask( const IConfigDb* pCfg = nullptr )
        : CTaskletRetriable( pCfg )
    {
        SetClassId( clsid( CTcpFidoListenTask ) );
    }
    virtual gint32 Process( guint32 dwContext );
};

enum EnumIoctlStat
{
    reqStatInvalid = 0,
    reqStatOut,
    reqStatIn,
    reqStatDone,
    reqStatMax,
};

struct STMPDO_IRP_EXT
{
    EnumIoctlStat m_iState;
    STMPDO_IRP_EXT()
    {
        m_iState = reqStatInvalid;
    }
};
/**
* @name CTcpStreamPdo 
* @{ */
/**
* This port has double identities, pdo and bus
* port at the same time
*
* And this port does not manage the req-resp
* transaction, and it perform only Send/Recv task
* and as the result, the IRP to it has only
* IRP_DIR_IN or IRP_DIR_OUT. IRP_DIR_INOUT will be
* ignoered.
*
* The default channel 0 is open throughout the
* lifecycle of this object.
*
* The port will perform the following tasks
*   1. asynchronous send/recv
*
*   2.  data split on send and data merge on recv
*
*   3.  load balance on different outgoing queue
*
*   4.  open new stream or close the existing
*       stream.
*
*   5.  stream management, including verify the
*       stream validity, and related working set
*       cleanup on close.
*
*   6.  manage the lifecycle of CRpcTcpPdo, as its
*       upper port
*
*   7.  Start/Stop the child CRpcTcpPdo
*
*   8.  implement CancelFuncIrp to remove the irp
*       from the waiting queue
*
*   9.  respond to the socket events and manage
*       its internal port state
*
*   10. interface with the GIO module.
* @} */

gint32 FireRmtSvrEvent(
    IPort* pPort, EnumEventId iEvent );

class CTcpStreamPdo : public CPort
{

    SockPtr     m_pStmSock;
    bool        m_bStopReady = false;
    bool        m_bSvrOnline = false;

    gint32 SubmitReadIrp( IRP* pIrp );
    gint32 SubmitWriteIrp( IRP* pIrp );
    gint32 SubmitIoctlCmd( IRP* pIrp );

    gint32 CompleteReadIrp( IRP* pIrp );
    gint32 CompleteWriteIrp( IRP* pIrp );
    gint32 CompleteFuncIrp( IRP* pIrp );
    gint32 CompleteIoctlIrp( IRP* pIrp );

    gint32 CompleteOpenStmIrp( IRP* pIrp );
    gint32 CompleteCloseStmIrp( IRP* pIrp );

    gint32 CompleteInvalStmIrp( IRP* pIrp );

    gint32 OpenCloseStreamLocal( IRP* pIrp );
    gint32 CheckAndSend(
        IRP* pIrp, gint32 ret );

    sem_t   m_semFireSync;

    public:

    typedef CPort super;

    // the configuration must includes the
    // following properties
    //
    // propIoMgr
    //
    // propBusPortPtr
    // propDestIpAddr
    //
    // propPortId
    // propBusPortPtr
    //
    // optional properties:
    //
    // propPortClass
    // propPortName
    //
    // propSrcIpAddr. if not existing, the binding
    // will happen on 0.0.0.0
    //
    // propFd for the socket fd if passive
    // connection
    //
    CTcpStreamPdo( const IConfigDb* pCfg );
    ~CTcpStreamPdo();

    bool IsImmediateReq( IRP* pIrp );
    gint32 RemoveIrpFromMap(
        IRP* pIrp );

    virtual gint32 CancelFuncIrp(
        IRP* pIrp, bool bForce );

    virtual gint32 OnSubmitIrp(
        IRP* pIrp );

    // make the active connection if not connected
    // yet. Note that, within the PostStart, the
    // eventPortStarted is not sent yet
    virtual gint32 PostStart(
        IRP* pIrp );

    virtual gint32 PreStop(
        IRP* pIrp );

	virtual gint32 OnQueryStop(
        IRP* pIrp );

    virtual gint32 OnPortReady(
        IRP* pIrp );
    // main entry of DispatchData for incoming
    // packets
    virtual gint32 OnEvent(
        EnumEventId iEvent,
        LONGWORD dwParam1,
        LONGWORD dwParam2,
        LONGWORD* pData );

    gint32 AllocIrpCtxExt(
        IrpCtxPtr& pIrpCtx,
        void* pContext = nullptr ) const;

    static gint32 GetIoctlReqState(
        IRP* pIrp, EnumIoctlStat& iState );

    static gint32 SetIoctlReqState(
        IRP* pIrp, EnumIoctlStat iState );

    gint32 CancelStartIrp(
        IRP* pIrp, bool bForce );

    gint32 OnPortStackReady( IRP* pIrp );

};

class CRpcTcpBusPort :
    public CGenericBusPortEx
{
    SockPtr m_pListenSock;
    std::vector< SockPtr > m_vecListenSocks;

    using PDOADDR = CConnParams;
    std::map< guint32, PDOADDR > m_mapIdToAddr;
    std::map< PDOADDR, guint32 > m_mapAddrToId;

    gint32 CreateTcpStreamPdo(
        IConfigDb* pConfig,
        PortPtr& pNewPort,
        const EnumClsid& iClsid ) const;

    gint32 OnNewConnection( gint32 iSockFd,
        CRpcListeningSock* pSock );

    gint32 LoadPortOptions(
        IConfigDb* pCfg );

    gint32 GetPdoAddr(
        guint32 dwPortId, PDOADDR& oAddr );

    gint32 GetPortId(
        PDOADDR& oAddr, guint32& dwPortId );

    gint32 RemovePortId( guint32 dwPortId );
    gint32 RemovePortAddr( PDOADDR& oAddr );

    public:
    typedef CGenericBusPortEx super;

    gint32 BindPortIdAndAddr(
        guint32 dwPortId, PDOADDR oAddr );

    virtual void RemovePdoPort(
        guint32 iPortId );

    CRpcTcpBusPort(
        const IConfigDb* pCfg );

    virtual gint32 PostStart(
        IRP* pIrp );

    virtual gint32 PreStop( IRP* pIrp );

    virtual gint32 BuildPdoPortName(
        IConfigDb* pCfg,
        std::string& strPortName );

    virtual gint32 CreatePdoPort(
        IConfigDb* pConfig,
        PortPtr& pNewPort );

    virtual gint32 OnEvent(
        EnumEventId iEvent,
        LONGWORD dwParam1,
        LONGWORD dwParam2,
        LONGWORD* pData );

    // methods from CObjBase
    gint32 GetProperty(
            gint32 iProp,
            CBuffer& oBuf ) const;

    gint32 SetProperty(
            gint32 iProp,
            const CBuffer& oBuf );

};

class CRpcTcpBusDriver : public CGenBusDriverEx
{
    public:
    typedef CGenBusDriverEx super;

    CRpcTcpBusDriver(
        const IConfigDb* pCfg = nullptr );

	gint32 Probe( IPort* pLowerPort,
        PortPtr& pNewPort,
        const IConfigDb* pConfig = NULL );

    gint32 GetTcpSettings(
        IConfigDb* pCfg ) const;
};

class CRpcTcpFidoDrv : public CPortDriver
{
    protected:
    gint32 CreatePort( PortPtr& pNewPort,
        const IConfigDb* pConfig );

    public:
    typedef CPortDriver super;

    CRpcTcpFidoDrv(
        const IConfigDb* pCfg = nullptr );

	gint32 Probe( IPort* pLowerPort,
        PortPtr& pNewPort,
        const IConfigDb* pConfig = NULL );

    virtual gint32 OnEvent( EnumEventId iEvent,
        LONGWORD dwParam1 = 0,
        LONGWORD dwParam2 = 0,
        LONGWORD* pData = NULL  )
    { return -ENOTSUP; }
};

#include "conntask.h"
class CStmSockConnectTask :
    public CStmSockConnectTaskBase< CRpcStreamSock >
{
    public:
    typedef CStmSockConnectTaskBase< CRpcStreamSock > super;
    CStmSockConnectTask( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CStmSockConnectTask ) ); }
};

gint32 SetBdgeIrpStmId( 
    PIRP pIrp, gint32 iStmId );

gint32 ScheduleCompleteIrpTask(
    CIoManager* pMgr,
    IrpVec2Ptr& pIrpVec,
    bool bSync );

gint32 GetPreStopStep(
    PIRP pIrp, guint32& dwStepNo );

gint32 SetPreStopStep(
    PIRP pIrp, guint32 dwStepNo );
