/*
 * =====================================================================================
 *
 *       Filename:  tcportex.h
 *
 *    Description:  the enhanced tcp port and socket declaration for the rpc router
 *
 *        Version:  1.0
 *        Created:  11/04/2019 07:37:04 PM
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
#include "tcpport.h"
#include "frmwrk.h"

class CRpcNativeProtoFdo;

class CRpcStream2 :
    public CObjBase
{
    protected:
    // the queue of outgoing packets, waiting for
    // socket ready to send
    gint32                      m_iStmId;
    gint32                      m_iPeerStmId;
    gint16                      m_wProtoId;

    // the queue of incoming packets, waiting for
    // reading operation
    std::deque< PacketPtr >     m_queBufToRecv;
    std::deque< IrpPtr >        m_queReadIrps;
    std::deque< IrpPtr >        m_queWriteIrps;
    CIoManager                  *m_pMgr;

    ObjPtr                      m_pParent;
    CRpcNativeProtoFdo*         m_pParentPort;

    // sequence number for outgoing packet
    mutable std::atomic< guint32 > m_atmSeqNo;
    std::atomic< guint32 >      m_dwAgeSec;

    gint32 StartSendDeferred( PIRP pIrp );
    gint32 SetupIrpForLowerPort(
        IPort* pLowerPort, PIRP pIrp ) const;

    public:

    typedef CObjBase super;
    CRpcStream2( const IConfigDb* pCfg );

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

    virtual void ClearAllIrpsQueued()
    {
        m_queReadIrps.clear();
        m_queWriteIrps.clear();
    }

    CIoManager* GetIoMgr()
    { return m_pMgr; }

    guint32 GetSeqNo() const
    { return m_atmSeqNo++; }

    virtual gint32 RecvPacket(
        CCarrierPacket* pPacketToRecv );

    virtual gint32 SetRespReadIrp(
        IRP* pIrp, CBuffer* pBuf );

    virtual gint32 GetReadIrpsToComp(
        std::vector< std::pair< IrpPtr, BufPtr > >& );

    // accept just two request, read and write
    virtual gint32 HandleReadIrp( IRP* pIrp );
    virtual gint32 HandleWriteIrp( IRP* pIrp );

    virtual gint32 RemoveIrpFromMap(
        IRP* pIrp );

    gint32 DispatchDataIncoming();

    virtual gint32 GetAllIrpsQueued(
        std::vector< IrpPtr >& vecIrps );

    inline guint32 GetQueSizePending()
    { return m_queReadIrps.size(); }

    inline guint32 GetQueSizeSend()
    { return m_queWriteIrps.size(); }

    gint32 QueueIrpOutgoing( IRP* pIrp )
    {
        if( pIrp == nullptr ||
            pIrp->GetStackSize() == 0 )
            return -EINVAL;

        m_queWriteIrps.push_back( pIrp );
        return 0;
    }

    gint32 StartSend( IrpPtr& pIrpToComp );

};

typedef CAutoPtr< clsid( CRpcStream2 ), CRpcStream2 > Stm2Ptr;

class CRpcControlStream2 :
    public CRpcStream2
{
    // the queue of outgoing packets, waiting for
    // socket ready to send
    std::map< gint32, IrpPtr >  m_mapIrpsForResp;

    // the evt queue for the proxy waiting on the
    // event.
    std::deque< IrpPtr >        m_queReadIrpsEvt;
    std::deque< CfgPtr >        m_queBufToRecv2;

    gint32 RemoveIoctlRequest( IRP* pIrp );

    MatchPtr                    m_pStmMatch;

    public:

    typedef CRpcStream2 super;

    CRpcControlStream2( const IConfigDb* pCfg );

    gint32 SetRespReadIrp(
        IRP* pIrp, CBuffer* pBuf );

    gint32 GetReadIrpsToComp(
        std::vector< std::pair< IrpPtr, BufPtr > >& );

    gint32 RemoveIrpFromMap( IRP* pIrp );

    gint32 HandleListening( IRP* pIrp );

    virtual gint32 HandleReadIrp( IRP* pIrp )
    { return -ENOTSUP; }

    virtual gint32 HandleWriteIrp( IRP* pIrp )
    { return -ENOTSUP; }

    gint32 HandleIoctlIrp( IRP* pIrp );
    gint32 QueueIrpForResp( IRP* pIrp );

    virtual gint32 GetAllIrpsQueued(
        std::vector< IrpPtr >& vecIrps );

    virtual void ClearAllIrpsQueued()
    {
        super::ClearAllIrpsQueued();
        m_queReadIrpsEvt.clear();
    }

    virtual gint32 RecvPacket(
        CCarrierPacket* pPacketToRecv );
};
typedef enum : gint32
{
    sseInvalid,
    sseRetWithBuf,
    sseRetWithFd,
    sseError,

} EnumStmSockEvt;

struct STREAM_SOCK_EVENT
{
    EnumStmSockEvt  m_iEvent;
    // m_iData is an fd if m_iEvent is
    // sseRetWithFd or an error code if m_iEvent
    // is sseError
    gint32          m_iData;

    // m_pInBuf is a data block if m_iEvent is
    // sseRetWithBuf
    BufPtr          m_pInBuf;

    STREAM_SOCK_EVENT()
    {
        m_iEvent = sseInvalid;
        m_iData = 0;
    }

    STREAM_SOCK_EVENT(
        const STREAM_SOCK_EVENT& rhs )
    {
        m_iEvent = rhs.m_iEvent;
        m_iData = rhs.m_iData;
        m_pInBuf = rhs.m_pInBuf;
    }
};

class CRpcNativeProtoFdo: public CPort
{
    protected:
    std::map< gint32, Stm2Ptr > m_mapStreams;

    // the stream whose irp for sending is in process
    bool    m_bStopReady = false;
    bool    m_bSending = false;
    bool    m_bCompress = false;

    Stm2Ptr m_pCurStm;
    PacketPtr m_pPackReceiving;

    std::deque< Stm2Ptr >  m_cycQueSend;
    gint32  m_iStmCounter;
    TaskletPtr m_pListeningTask;


    // completion handler for irp from tcpfido
    gint32 CompleteOpenStmIrp( IRP* pIrp );
    gint32 CompleteCloseStmIrp( IRP* pIrp );
    gint32 CompleteInvalStmIrp( IRP* pIrp );

    // completion handler for irp from pdo
    gint32 CompleteWriteIrp( IRP* pIrp );
    gint32 CompleteReadIrp( IRP* pIrp );
    gint32 CompleteIoctlIrp( IRP* pIrp );
    gint32 CompleteFuncIrp( IRP* pIrp );

    gint32 SubmitReadIrp( IRP* pIrp );
    gint32 SubmitWriteIrp( IRP* pIrp );
    gint32 SubmitIoctlCmd( IRP* pIrp );

    gint32 HandleIoctlIrp( IRP* pIrp );
    gint32 QueueIrpForResp( IRP* pIrp );

    gint32 CancelFuncIrp(
        IRP* pIrp, bool bForce );

    gint32 AddStmToSendQue( Stm2Ptr& pStm );
    virtual gint32 PreStop( IRP* pIrp );

    public:

    typedef CPort super;

    CRpcNativeProtoFdo( const IConfigDb* pCfg );
    bool IsImmediateReq( IRP* pIrp );
    gint32 RemoveIrpFromMap( IRP* pIrp );

    gint32 GetStream( gint32 iIndex,
        Stm2Ptr& pStream )
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
        gint32 iPeerId, Stm2Ptr& pStream );

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
        Stm2Ptr pStream;
        gint32 ret = GetStream( iStmId, pStream );
        if( ERROR( ret ) )
            return ret;

        pStream->Refresh();
        return 0;
    }

    gint32 ClearIdleStreams()
    { return 0; }

    static gint32 GetIoctlReqState(
        IRP* pIrp, EnumIoctlStat& iState );

    static gint32 SetIoctlReqState(
        IRP* pIrp, EnumIoctlStat iState );

    gint32 StartListeningTask();
    gint32 StopListeningTask();
    gint32 OnPortStackReady( IRP* pIrp );

    gint32 GetStreamFromIrp(
        IRP* pIrp, Stm2Ptr& pStm );

    // stream management
    gint32 AddStream(
        gint32 iStream,
        guint16 wProtoId,
        gint32 iPeerStmId );

    gint32 RemoveStream( gint32 iStream );
    
    // select the eligible stream to send data
    // across all the streams
    gint32 GetStreamToSend( Stm2Ptr& pStm );
    gint32 NewStreamId( gint32& iStream );
    gint32 GetSeedStmId();

    gint32 GetCtrlStmFromIrp(
        IRP* pIrp, Stm2Ptr& pStm );

    virtual gint32 PostStart(
        IRP* pIrp );

	virtual gint32 OnQueryStop(
        IRP* pIrp );

    virtual gint32 Stop( IRP* pIrp );

    gint32 CancelAllIrps( gint32 iErrno );

    inline void SetSending( bool bSending )
    { m_bSending = bSending; }

    inline bool IsSending() const
    { return m_bSending; }

    gint32 StartSend( IRP* pIrpLocked );

    virtual gint32 OnSubmitIrp( IRP* pIrp );

    gint32 OnReceive(
        gint32 iFd, BufPtr& pBuf );

    gint32 AllocIrpCtxExt(
        IrpCtxPtr& pIrpCtx,
        void* pContext ) const;

    inline bool IsCompress() const
    { return m_bCompress; }

    gint32 SetListeningTask(
        TaskletPtr& pTask );
};

class CFdoListeningTask :
    public CIfParallelTask
{
    public:
    typedef CIfParallelTask super;

    CFdoListeningTask( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CFdoListeningTask ) ); }

    gint32 RunTask();
    gint32 HandleIrpResp( IRP* pIrp );
    gint32 OnIrpComplete( IRP* pIrp );
    gint32 OnTaskComplete( gint32 iRet );
};

class CRpcNatProtoFdoDrv : public CRpcTcpFidoDrv
{
    public:
    typedef CRpcTcpFidoDrv super;

    CRpcNatProtoFdoDrv(
        const IConfigDb* pCfg = nullptr );

	gint32 Probe( IPort* pLowerPort,
        PortPtr& pNewPort,
        const IConfigDb* pConfig = NULL );
};


class CRpcConnSock :
    public CRpcSocketBase
{
    TaskletPtr m_pStartTask;
    bool m_bClient = false;

    public:

    typedef CRpcSocketBase super;

    // properties in pCfg
    //
    // refer to CRpcSocketBase for the rest
    // properties
    CRpcConnSock(
        const IConfigDb* pCfg );

    gint32 Start_bh();

    gint32 ActiveConnect(
        const std::string& strIpAddr );

    virtual gint32 DispatchSockEvent(
        GIOCondition );

    virtual gint32 Start();
    virtual gint32 Stop();

    virtual gint32 OnSendReady();
    virtual gint32 OnReceive();
    virtual gint32 OnConnected()
    {
        // we will use StartTask's OnConnected
        // instead of this one
        return -ENOTSUP;
    }

    gint32 OnDisconnected();

    gint32 Connect();
    gint32 Disconnect();

    inline bool IsClient() const
    { return m_bClient; }

    gint32 StartSend( IRP* pIrp = nullptr );
    virtual gint32 OnError( gint32 ret );

    // event dispatcher
    gint32 OnEvent(
        EnumEventId iEvent,
        LONGWORD dwParam1,
        LONGWORD dwParam2,
        LONGWORD* pData );
};

#include "conntask.h"
class CStmSockConnectTask2 :
    public CStmSockConnectTaskBase< CRpcConnSock >
{
    public:
    typedef CStmSockConnectTaskBase< CRpcConnSock > super;
    CStmSockConnectTask2( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CStmSockConnectTask2 ) ); }
};

class CTcpStreamPdo2;

class CBytesSender
{
    IrpPtr m_pIrp;
    gint32 m_iBufIdx = -1;
    guint32 m_dwOffset = 0;
    PortPtr m_pPort;

    public:

    CBytesSender( CTcpStreamPdo2* pPdo )
    {
        if( pPdo == nullptr )
            return;
        m_pPort = ( IPort* )pPdo;
    }

    gint32 SetIrpToSend( IRP* pIrp )
    {
        if( !m_pIrp.IsEmpty() )
            return ERROR_STATE;

        m_pIrp = ObjPtr( pIrp );
        m_iBufIdx = 0;
        m_dwOffset = 0;
        return 0;
    }

    gint32 SendImmediate( gint32 iFd, PIRP pIrp );
    gint32 SetSendDone( gint32 iRet = 0 );
    bool IsSendDone() const;
    IrpPtr GetIrp() const
    {
        CPort* pPort = m_pPort;
        if( pPort == nullptr )
            return -EFAULT;
        return m_pIrp;
    }

    gint32 OnSendReady( gint32 iFd );
    gint32 CancelSend( const bool& bCancelIrp );

    gint32 Clear()
    {
        PortPtr pPort = m_pPort;
        if( pPort.IsEmpty() )
            return 0;

        CPort* ptr = pPort;
        CStdRMutex oPortLock(
            ptr->GetLock() );
        if( !m_pIrp.IsEmpty() )
            return ERROR_STATE;

        m_pPort.Clear();

        return 0;
    }
};

// port specific ctrl code
#define CTRLCODE_IS_CLIENT 0x31
/**
* @name CTcpStreamPdo2 
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

class CTcpStreamPdo2 : public CPort
{

    bool        m_bStopReady = false;
    bool        m_bSvrOnline = false;

    gint32 SubmitWriteIrp( IRP* pIrp );
    gint32 SubmitIoctlCmd( IRP* pIrp );
    gint32 SubmitListeningCmd( IRP* pIrp );

    // gint32 CompleteWriteIrp( IRP* pIrp );
    // gint32 CompleteFuncIrp( IRP* pIrp );
    gint32 CompleteIoctlIrp( IRP* pIrp );
    gint32 StartSend( IRP* pIrpLocked  );
    gint32 CancelAllIrps( gint32 iErrno );

    SockPtr m_pConnSock;

    std::deque< IrpPtr > m_queWriteIrps;
    std::deque< IrpPtr >  m_queListeningIrps;
    std::deque< STREAM_SOCK_EVENT > m_queEvtToRecv;

    CBytesSender m_oSender;

    gint32 SendImmediate( gint32 iFd, PIRP pIrp );

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
    CTcpStreamPdo2( const IConfigDb* pCfg );
    ~CTcpStreamPdo2();

    gint32 RemoveIrpFromMap(
        IRP* pIrp );

    virtual gint32 CancelFuncIrp(
        IRP* pIrp, bool bForce );

    virtual gint32 OnSubmitIrp( IRP* pIrp );

    // make the active connection if not connected
    // yet. Note that, within the PostStart, the
    // eventPortStarted is not sent yet
    virtual gint32 PostStart( IRP* pIrp );
    virtual gint32 PreStop( IRP* pIrp );
    virtual gint32 Stop( IRP* pIrp );

    gint32 AllocIrpCtxExt(
        IrpCtxPtr& pIrpCtx,
        void* pContext = nullptr ) const;

    gint32 OnReceive( gint32 iFd );
    gint32 OnSendReady( gint32 iFd );

    gint32 OnStmSockEvent(
        STREAM_SOCK_EVENT& sse );

    gint32 OnDisconnected();
};
