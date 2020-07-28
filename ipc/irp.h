/*
 * =====================================================================================
 *
 *       Filename:  irp.h
 *
 *    Description:  declaration of irp related classes
 *
 *        Version:  1.0
 *        Created:  01/11/2018 11:01:38 PM
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
#include "stlcont.h"

// the events, which will go to the IEventSink
// the peer connection to a SS/E2 ready
#define EVENT_CONN_READY        1

// the peer connection to a SS/E2 lost
#define EVENT_CONN_LOST         2

// the session is timeout
#define EVENT_SESS_TIMEOUT      3
//
// a new port connected
#define EVENT_PNP_PORT_CONN     4

// a port disconnected
#define EVENT_PNP_PORT_DISCONN  5

// the IRP is a request packet, that is, after
// request is sent, the irp will waiting for the
// response if the m_dwRespSize is not zero
// this request will usually have a timer associated
// with it
#define IRP_FLAG_REQUEST

// the IRP is waiting for a notification, that is, 
// the irp will waiting for a notification. This irp
// does not require to have a timeout associated.
#define IRP_FLAG_NOTIFIY

// the data won't be converted and will go through
// without changes
#define IRP_FLAG_PASSTHROUGH

// So far we don't provide synchronous call in irp
// for the PnP issue.
// but you can still do it in OnEvent callback
// #define IRP_FLAG_SYNC

#define IRP_MAJOR_MASK  ( ( guint32 )0xFF )
#define IRP_MINOR_MASK  ( ( guint32 )0xFF )
#define IRP_CTRLCODE_MASK ( ( guint32 )0xFFFF )

// the io control code for the m_dwMajorCmd
#define IRP_MJ_PNP                      0x01
#define IRP_MJ_FUNC                     0x02
#define IRP_MJ_POWER                    0x03
#define IRP_MJ_ASSOC                    0x04

// the dwMinorCmd for GETPROP is the handle 
// to the port where this irp is destined.
// and the m_pReqData contains a CfgPtr which
// carries the parameters, with propMinorCmd
// ( mandatory ) to indicate which command to
// perform. And the rest property is command
// specific.
// This must be a synchronous request
#define IRP_MJ_GETPROP                  0x05
#define IRP_MJ_SETPROP                  0x06

#define IRP_MJ_INVALID                  0xFF


// IRP_MJ_FUNC's minor command
// listening req/event
#define IRP_MN_READ                     0x01
// send req/event
#define IRP_MN_WRITE                    0x02
// send req/recv resp
#define IRP_MN_READWRITE                0x03
// io control
#define IRP_MN_IOCTL                    0x04

#define IRP_MN_INVALID                  0xFF

// register the msg match interface
#define CTRLCODE_REG_MATCH              0x01
#define CTRLCODE_UNREG_MATCH            0x03

// adjust the sizeo of request queue 
#define CTRLCODE_SET_REQUE_SIZE         0x04

#define CTRLCODE_LISTENING              0x05
#define CTRLCODE_SEND_REQ               0x07
#define CTRLCODE_SEND_RESP              0x08
#define CTRLCODE_SEND_EVENT             0x09
#define CTRLCODE_CONNECT                0x0a
#define CTRLCODE_DISCONN                0x0b
#define CTRLCODE_FORWARD_REQ            0x0e
#define CTRLCODE_LISTENING_FDO          0x0f
#define CTRLCODE_RMT_REG_MATCH          0x10
#define CTRLCODE_RMT_UNREG_MATCH        0x11
#define CTRLCODE_SEND_DATA              0x12
#define CTRLCODE_FETCH_DATA             0x13

// a multi-purpose request
#define CTRLCODE_SEND_CMD               0x14

// ioctl code for CTcpStreamPdo
#define CTRLCODE_OPEN_STREAM_PDO        0x15
#define CTRLCODE_CLOSE_STREAM_PDO       0x16
#define CTRLCODE_OPEN_STREAM_LOCAL_PDO  0x17
#define CTRLCODE_CLOSE_STREAM_LOCAL_PDO 0x18
#define CTRLCODE_INVALID_STREAM_ID_PDO  0x1a
#define CTRLCODE_RMTSVR_OFFLINE_PDO     0x1b
#define CTRLCODE_RMTMOD_OFFLINE_PDO     0x1c
#define CTRLCODE_GET_LOCAL_STMID        0x1e
#define CTRLCODE_GET_RMT_STMID          0x1f
#define CTRLCODE_RESET_CONNECTION       0x21           

// ioctl code for CUnixSockStmPdo
#define CTRLCODE_STREAM_CMD             0x20


#define CTRLCODE_INVALID                0xFFFF

// for association irps
#define IRP_MN_ASSOC_RUN                0x01

#define IRP_MN_PNP_START                0x01
#define IRP_MN_PNP_STOP                 0x02
#define IRP_MN_PNP_QUERY_STOP           0x03

// immdiate commands
// port stack management
#define IRP_MN_PNP_STACK_BUILT          0x04
// the last command before the
// port is removed
#define IRP_MN_PNP_STACK_DESTROY        0x05

// stop a single child pdo port stack of the bus port
#define IRP_MN_PNP_STOP_CHILD           0x06
#define IRP_MN_PNP_REATTACH             0x07
#define IRP_MN_PNP_STACK_READY          0x08

#define IRP_DIR_IN                      0x00
#define IRP_DIR_OUT                     0x01
#define IRP_DIR_INOUT                   0x02
#define IRP_DIR_MASK                    0x03

// the caller will wait the completion signal
#define IRP_SYNC_CALL                   0x04

// The irp is marked pending if the caller get the
// STATUS_PENDING on the return of SubmitIrp
#define IRP_PENDING                     0x08

// don't complet the irp for now
#define IRP_NO_COMPLETE                 0x10

// This is a requests not from dbus, indicating
// the request and response are in IConfigDb
#define IRP_NON_DBUS                    0x20

// the flag to tell CompleteIrp to call the
// callback only. no need to go through the port
// stack
#define IRP_CALLBACK_ONLY               0x40
#define IRP_SUBMITED                    0x80

// state flags of the irp
#define IRP_STATE_READY                 0x01
#define IRP_STATE_COMPLETING            0x02
#define IRP_STATE_COMPLETED             0x03
#define IRP_STATE_CANCELLING            0x04
#define IRP_STATE_CANCELLED             0x05
#define IRP_STATE_CANCEL_SLAVE          0x06
#define IRP_STATE_BUSY                  0x07
#define IRP_STATE_COMPLETING_ASSOC      0x08
class IRP_CONTEXT;

typedef CAutoPtr< clsid( IRP_CONTEXT ), IRP_CONTEXT > IrpCtxPtr;

class IPort;
typedef CAutoPtr< Clsid_Invalid, IPort > PortPtr;

class IPort : public IEventSink
{
	public:
    typedef super   IEventSink;

    virtual gint32 PreStart( IRP* irp ) = 0;
    virtual gint32 Start( IRP* irp ) = 0;
    virtual gint32 PostStart( IRP* irp ) = 0;
    virtual gint32 PreStop( IRP* irp ) = 0;
    virtual gint32 Stop(IRP* irp ) = 0;
    virtual gint32 PostStop( IRP* irp ) = 0;
	virtual gint32 SubmitIrp( IRP* irp ) = 0;
	virtual gint32 CompleteIrp( IRP* irp ) = 0;
	virtual gint32 CancelIrp( IRP* irp, bool bForce = true ) = 0;
	virtual bool CanCancelIrp( IRP* pIrp ) = 0;

    virtual gint32 AttachToPort( IPort* pLowerPort ) = 0 ;

    virtual gint32 AllocIrpCtxExt(
        IrpCtxPtr& pIrpCtx,
        void* pContext = nullptr ) const = 0;

    virtual gint32 ReleaseIrpCtxExt(
        IrpCtxPtr& pIrpCtx,
        void* pContext = nullptr ) = 0;

	virtual guint32 GetPortState() const = 0;
    virtual PortPtr GetUpperPort() const = 0;
    virtual PortPtr GetLowerPort() const = 0;
    virtual PortPtr GetTopmostPort() const = 0;
    virtual PortPtr GetBottomPort() const = 0;

    virtual gint32 CompleteAssocIrp(
        IRP* pMaster, IRP* pSlave ) = 0;
};

class CStlPortVector : public CStlVector< PortPtr >
{

    public:
    typedef CStlVector<PortPtr> super;
    CStlPortVector()
        :CStlVector<PortPtr>()
    {
        SetClassId( clsid( CStlPortVector  ) );
    }
};

typedef CAutoPtr< clsid( CStlPortVector ), CStlPortVector > PortVecPtr;

struct IRP_CONTEXT : CObjBase
{
    // IRP_DIR_IN
    // IRP_DIR_OUT
    // IRP_DIR_INOUT
    guint32             m_dwFlags;

    // the status returned by the next lower port
    gint32              m_iStatus;

    // the major control code
    guint32             m_dwMajorCmd;

    // the minor control code
    guint32             m_dwMinorCmd;

    // the io control code
    guint32             m_dwCtrlCmd;

    // the pointer of the request data
    BufPtr              m_pReqData;

    // the output data buffer, 
    // the caller alloc/frees this buffer
    BufPtr              m_pRespData;

    // context extension
    BufPtr              m_pExtBuf;

    IRP_CONTEXT( IRP_CONTEXT* pRhs = nullptr );

    IRP_CONTEXT& operator=(
        const IRP_CONTEXT& oIrpCtx );

    ~IRP_CONTEXT();

    guint32 GetIoDirection() const;
    void SetIoDirection( guint32 dwDirection );

    bool IsNonDBusReq() const;
    void SetNonDBusReq( bool bNonDBus );

    guint32 GetMajorCmd() const;
    guint32 GetMinorCmd() const;
    guint32 GetCtrlCode() const;

    void SetMajorCmd( guint32 dwMajorCmd );
    void SetMinorCmd( guint32 dwMinorCmd );
    void SetCtrlCode( guint32 dwCtrlCode );

    gint32 GetStatus() const;
    void SetStatus( gint32 dwStatus );
    gint32 SetExtBuf( const BufPtr& bufPtr );
    void GetExtBuf( BufPtr& bufPtr ) const;

    gint32 SetRespData( const BufPtr& bufPtr );
    gint32 SetReqData( const BufPtr& bufPtr );

    gint32 GetRespAsCfg( CfgPtr& pCfg );
    gint32 GetReqAsCfg( CfgPtr& pCfg );
};


#define IOSTACK_ALLOC_NEW    1
#define IOSTACK_ALLOC_COPY   2

class CIoManager;
typedef CAutoPtr< clsid( CIrpCompThread ), IThread >   IrpThrdPtr;

typedef CAutoPtr< clsid( IoRequestPacket ), IoRequestPacket > IrpPtr;
struct IoRequestPacket : public IEventSink
{
    //
    // the major status of this IO command
    // if the status code is negative,
    // an error occurs.
    //
    // if the status code is no less than 0,
    // the status is OK, or waiting for
    // something 
    //
    gint32              m_iStatus;

    // the io direction flags
    // IRP_DIR_IN
    // IRP_DIR_OUT
    // IRP_DIR_INOUT

    // the sync/async call flag
    // IRP_SYNC_CALL
    // IRP_PENDING
    guint32             m_dwFlags;

    std::atomic< gint32 >  m_atmState;

    // the callback function from irp allocator
    EventPtr            m_pCallback;

    // the parameter for m_pCallback
    LONGWORD            m_dwContext;

    // dedicated irp completion thread if any
    IrpThrdPtr          m_IrpThrdPtr;

    // the context each port has
    // since we cannot make sure the irp life cycle contained
    // within the minimum lifecycle of all the IPort* in the
    // context stack, we use portPtr. But we should also mutual
    // reference issue
    std::vector< std::pair< PortPtr, IrpCtxPtr > > m_vecCtxStack;

    // semaphore for synchronous calls from outside the mainloop
    sem_t               m_semWait;

    // lock for concurrent access of this irp
    stdrmutex           m_oLock;

    // life time before canceling in seconds
    guint32             m_dwTimeoutSec;

    gint32              m_iTimerId;


    CIoManager*         m_pMgr;

    // used for cancel and completion to indicate where
    // the current completion entry is in the m_vecCtxStack
    guint32              m_dwCurPos;

    // tid of the thread where this irp is submitted
    guint32              m_dwTid;
    // for associated irps
    // on cancel, all the slave irps will be canceled
    // by providing associated irps, we can support
    // calling from one driver stack to another
    // or we can fork the irp to multiple requests
    IrpPtr               m_pMasterIrp;
    std::map< IoRequestPacket*, int > m_mapSlaveIrps;

    // minimum slave irps to associate before the
    // master irp can be completed
    //
    // NOTE: it combines the m_mapSlaveIrps to decide
    // wether to complete the master irp
    guint16             m_wMinSlaves;

    IoRequestPacket();

    IoRequestPacket(
            const IoRequestPacket& rhs );

    ~IoRequestPacket();

    IoRequestPacket& operator=(
            const IoRequestPacket& rhs );

    // irp stack operations
    void PopCtxStack();

    gint32 AllocNextStack(
            IPort* pPort,
            gint32 iAlloc = IOSTACK_ALLOC_NEW );

    IrpCtxPtr& GetTopStack() const;
    IPort* GetTopPort() const;
    guint32 GetStackSize() const;

    guint32 GetCurPos() const;
    bool SetCurPos( guint32 dwIdx );
    bool IsIrpHolder() const;

    inline IrpCtxPtr& GetCurCtx() const
    {
         return GetCtxAt( GetCurPos() );
    }

    inline IPort* GetCurPort() const
    {
        return GetPortAt( GetCurPos() ); 
    }
    IrpCtxPtr& GetCtxAt( guint32 dwPos ) const;
    IPort* GetPortAt( guint32 dwPos ) const;

    gint32 CanContinue( gint32 iNewState );
    inline gint32 GetStatus()
    { return m_iStatus; }

    bool IsSyncCall() const;
    void SetSyncCall( bool bSync = true ) ;

    bool IsPending() const;
    void MarkPending();
    void RemovePending();

    inline void SetSubmited()
    { m_dwFlags |= IRP_SUBMITED; }

    inline void ClearSubmited()
    { m_dwFlags &= ~IRP_SUBMITED; }

    inline bool IsSubmited() const
    { return ( ( m_dwFlags & IRP_SUBMITED ) != 0 ); }

    void SetCbOnly( bool bCbOnly );

    inline bool IsCbOnly() const
    { return ( ( m_dwFlags & IRP_CALLBACK_ONLY ) != 0 ); }

    void SetNoComplete( bool bNoComplete );

    inline bool IsNoComplete() const
    { return ( ( m_dwFlags & IRP_NO_COMPLETE ) != 0 ); }

    bool SetState( gint32 iCurState, gint32 iNewState );

    gint32 GetState() const
    { return m_atmState; }

    bool HasTimer() const { return m_iTimerId > 0; };

    void SetTimer( guint32 dwTimeoutSec, CIoManager* pMgr );
    void RemoveTimer();
    void ResetTimer();

    void SetMasterIrp( IRP* pMaster );
    IRP* GetMasterIrp() const;
    void SetMinSlaves( guint16 wCount );
    void AddSlaveIrp( IRP* pSlave );
    bool FindSlaveIrp( IRP* pSlave ) const;
    guint32 GetSlaveCount() const;
    void RemoveSlaveIrp( IRP* pSlave );
    void RemoveAllSlaves();
    stdrmutex& GetLock() const
    { return ( stdrmutex& )m_oLock; }

    gint32 OnEvent( EnumEventId iEvent,
            LONGWORD dwParam1 = 0,
            LONGWORD dwParam2 = 0,
            LONGWORD* pData = NULL  );
    
    gint32 WaitForComplete();
    void SetStatus( gint32 iStatus )
    { m_iStatus = iStatus; }

    void SetCallback(
        const EventPtr& pEvent,
        LONGWORD dwContext );

    gint32 GetIrpThread( ThreadPtr& pthrd );
    gint32 SetIrpThread( CIoManager* pMgr );
    void ClearIrpThread();

    void RemoveCallback();

    bool CanComplete();
	bool CanCancel();


    guint32 MajorCmd() const;
    guint32 MinorCmd() const;
    guint32 CtrlCode() const;
    guint32 IoDirection() const;
};

namespace std
{
    template<>
    struct less<PortPtr>
    {
        bool operator()(const PortPtr& k1, const PortPtr& k2) const
            {
              return ( reinterpret_cast< unsigned long >( ( IPort* )k1  )
                        < reinterpret_cast< unsigned long >( ( IPort* )k2 ) );
            }
    };

    template<>
    struct less<IrpPtr>
    {
        bool operator()(const IrpPtr& k1, const IrpPtr& k2) const
            {
              return ( reinterpret_cast< unsigned long >( ( IRP* )k1  )
                        < reinterpret_cast< unsigned long >( ( IRP* )k2 ) );
            }
    };
}

class CStlIrpVector : public CStlVector< IrpPtr >
{

    public:
    typedef IrpPtr ElemType;
    typedef CStlVector< IrpPtr > super;

    CStlIrpVector() : CStlVector< ElemType >()
    {
        SetClassId( clsid( CStlIrpVector ) );
    }
};

typedef CAutoPtr< clsid( CStlIrpVector ), CStlIrpVector > IrpVecPtr;

class CStlIrpVector2 : public CStlVector< std::pair< gint32, IrpPtr > >
{

    public:
    typedef std::pair< gint32, IrpPtr > ElemType;
    typedef CStlVector< ElemType > super;

    CStlIrpVector2() : CStlVector< ElemType >()
    {
        SetClassId( clsid( CStlIrpVector2 ) );
    }
};

typedef CAutoPtr< clsid( CStlIrpVector2 ), CStlIrpVector2 > IrpVec2Ptr;

