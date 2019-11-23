/*
 * =====================================================================================
 *
 *       Filename:  port.h
 *
 *    Description:  declaration of the port, port driver, and related stuffs
 *
 *        Version:  1.0
 *        Created:  01/19/2016 08:14:43 PM
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
#include "frmwrk.h"
#include "portdrv.h"

#define PORT_MAX_GENERATIONS         4
#define PORT_START_TIMEOUT_SEC       120

// port type mask
#define PORTFLG_TYPE_MASK               ( ( guint32 )0x07 )

// physical port
#define PORTFLG_TYPE_PDO                ( ( guint32 )0x01 )

// function port
#define PORTFLG_TYPE_FDO                ( ( guint32 )0x02 )

// filter port
#define PORTFLG_TYPE_FIDO               ( ( guint32 )0x03 )

// filter port
#define PORTFLG_TYPE_INVALID            ( ( guint32 )0x07 )
// bus port
#define PORTFLG_TYPE_BUS                ( ( guint32 )0x08 )

#define PortType( flag )                ( flag & PORTFLG_TYPE_MASK )
#define IsBusPort( flag )               ( flag & PORTFLG_TYPE_BUS )

// the port is attached to the bus, but not started
// it can receive the pnp event, but can handle
// pnp request only
#define PORT_STATE_INVALID              ( ( guint32 )0xFFFFFFFF )

#define PORT_STATE_READY                ( ( guint32 )0x00  )

#define PORT_STATE_ATTACHED             ( ( guint32 )0x01 )

// the port is being started and ready to accept all the
// user requests
#define PORT_STATE_STARTING             ( ( guint32 )0x02 )

// the port is being stopped, and can only handle pnp request.
#define PORT_STATE_STOPPING             ( ( guint32 )0x03 )

// still in busy state but can not enter busy state any more
// via new request
#define PORT_STATE_STOPPED              ( ( guint32 )0x04 )

// the port is being removed, waiting for reference owner to
// release the reference count
#define PORT_STATE_REMOVED              ( ( guint32 )0x05 )

// the port is busy with something. it is a transiant 
// state, will return to STARTED after done. This is an
// exclusive state, to allow one IRP at one time
#define PORT_STATE_BUSY                 ( ( guint32 )0x06 )

// this is a shared state mutual exclusive
// for stopping command, but shared between IRP_MJ_FUNC
#define PORT_STATE_BUSY_SHARED          ( ( guint32 )0x07 )

#define PORT_STATE_EXCLUDE_PENDING      ( ( guint32 )0x08 )

// two transiant state 
#define PORT_STATE_STOP_RESUME          ( ( guint32 )0x09 )
#define PORT_STATE_BUSY_RESUME          ( ( guint32 )0x0a )


#define PORT_STATE_MASK                 ( ( guint32 )0x0F )

// pnp states for start/stop port 
#define PNP_STATE_START_LOWER           ( ( guint32 )0x01 )
#define PNP_STATE_START_PRE_SELF        ( ( guint32 )0x02 )
#define PNP_STATE_START_SELF            ( ( guint32 )0x03 )
#define PNP_STATE_START_POST_SELF       ( ( guint32 )0x04 )
#define PNP_STATE_START_CHILD           ( ( guint32 )0x05 )
#define PNP_STATE_START_PORT_RDY        ( ( guint32 )0x06 )

#define PNP_STATE_STOP_PRELOCK          ( ( guint32 )0x08 )
#define PNP_STATE_STOP_BEGIN            ( ( guint32 )0x00 )
#define PNP_STATE_STOP_PRE              ( ( guint32 )0x03 )
#define PNP_STATE_STOP_SELF             ( ( guint32 )0x04 )
#define PNP_STATE_STOP_POST             ( ( guint32 )0x05 )
#define PNP_STATE_STOP_LOWER            ( ( guint32 )0x06 )

class CPort;

typedef gint32 ( CPort::* ReSubmitPtr )( IRP* );

#define PORTSTAT_MAP_LEN        11
class CPortState : public CObjBase
{
    IrpPtr                      m_pExclIrp;
    std::vector< guint8 >       m_vecStates;
    stdrmutex                   m_oLock;
    static const gint8          m_arrStatMap[ PORTSTAT_MAP_LEN ][ PORTSTAT_MAP_LEN ];
    sem_t                       m_semWaitForReady;
    IPort*                      m_pPort;

    TaskletPtr                  m_pResume;

    bool IsValidSubmit( guint32 dwNewState ) const;
    guint32 PushState( guint32 dwState, bool bCheckLimit = true );

    public:

    CPortState( const IConfigDb* pCfg );

    ~CPortState();

    guint32 GetPortState() const;
    stdrmutex& GetLock() const;


	gint32 CanContinue( IRP* pIrp,
        guint32 dwNewState = PORT_STATE_INVALID,
        guint32* pdwOldState = nullptr );

    void SetResumePoint( TaskletPtr& pFunc );
    gint32 SetPortState( guint32 state );

    // return the poped state
    gint32 PopState( guint32& dwState );

	bool SetPortStateWake(
		guint32 dwCurState,
        guint32 dwNewState,
        bool bWakeAll = false );

    bool IsPortReady();

    gint32 ScheduleResumeTask(
        TaskletPtr& pTask, IrpPtr& pIrp );

    gint32 HandleReady( IRP* pIrp,
        guint32 dwNewState,
        guint32* pdwOldState );

    gint32 HandleAttached( IRP* pIrp,
        guint32 dwNewState,
        guint32* pdwOldState );

    gint32 HandleStarting( IRP* pIrp,
        guint32 dwNewState,
        guint32* pdwOldState );

    gint32 HandleStopping( IRP* pIrp,
        guint32 dwNewState,
        guint32* pdwOldState );

    gint32 HandleStopped( IRP* pIrp,
        guint32 dwNewState,
        guint32* pdwOldState );

    gint32 HandleRemoved( IRP* pIrp,
        guint32 dwNewState,
        guint32* pdwOldState );

    gint32 HandleBusy( IRP* pIrp,
        guint32 dwNewState,
        guint32* pdwOldState );

    gint32 HandleBusyShared( IRP* pIrp,
        guint32 dwNewState,
        guint32* pdwOldState );

    gint32 HandleExclPending( IRP* pIrp,
        guint32 dwNewState,
        guint32* pdwOldState );

    gint32 HandleStopResume( IRP* pIrp,
        guint32 dwNewState,
        guint32* pdwOldState );

    gint32 HandleBusyResume( IRP* pIrp,
        guint32 dwNewState,
        guint32* pdwOldState );
};

typedef CAutoPtr< clsid( CPortState ), CPortState > PortStatPtr;

/**
* @name How to remove a Port
* @{ */
/** The moments to stop and removal a port are the
 * following:
 *
 * 1.   The physical disconnect of a port or a
 * disconnection of network socket, for example,
 * the tcp connection lost and one of the
 * `RpcTcpConn' will be lost.
 *      
 * 2.   The system admin command to restart/remove
 * a port
 *
 * 3.   There are no client activities on that
 * port for 30 minutes, for example, the last
 * interface proxy disconnected from the port and
 * no new interface hooks up, to exit and release
 * the resources.
 * 
 * 4.   A remote event message from rpc server
 * arrives the rpc proxy indicating the remote
 * server is going to shutdown.  In this
 * situation, the pnp manager will remove the
 * proxy port. This could happen across all the
 * processes that has connection to this server
 *
 * 5.   A remote interface server ( process ) is
 * down. And the router on both side need to peel
 * off all subscribers to that server. and for
 * those single subscriber, the connection will
 * stop.
 * @} */

class CPort : public IPort
{
    friend class CIrpGateKeeper;

	protected:
	guint32	        m_dwFlags;

    // the upper filter and lower filter port we
    // don't hold the ref count because they have
    // the same life cycle
    IPort*          m_pUpperPort;
	IPort*          m_pLowerPort;

    // the bus port if this is a pdo port
    IPort*          m_pBusPort;

    // the limit of parallel requests in
    // processing
	guint32	        m_dwMaxCurReq;

    // for references count of busy threads
    // working on this port simutaneously
    guint32         m_dwBusyCount;

    // the port identification infomation
    // std::string m_strName;
    // std::string m_strPath;
    // std::string m_strPortClass;
    // guint32 m_dwPortId;
    CfgPtr          m_pCfgDb;

	IPortDriver*    m_pDriver;

    // port lock for concurrent access
    std::recursive_mutex       m_oLock;

    // the irps which is not finished yet
    std::map<IrpPtr, gint32> m_mapIrpIn;

    PortStatPtr      m_pPortState;

    CIoManager*     m_pIoMgr;

	gint32 SubmitStartIrp( IRP* pIrp );
	gint32 CompleteStartIrp( IRP* pIrp );

	gint32 SubmitStopIrp( IRP* pIrp );
	gint32 CompleteStopIrp( IRP* pIrp );

	gint32 SubmitStopIrpEx( IRP* pIrp );
	gint32 CompleteStopIrpEx( IRP* pIrp );

	gint32 SubmitQueryStopIrp( IRP* pIrp );
	virtual gint32 CompleteQueryStopIrp( IRP* pIrp );

    gint32 CancelStartStopIrp(
        IRP* pIrp, bool bForce, bool bStart );

	gint32 SubmitPortStackIrp( IRP* pIrp );

    gint32 ScheduleStartStopNotifTask(
        guint32 dwEventId,
        bool bImmediately = false );

	gint32 SubmitGetPropIrp( IRP* pIrp );
    gint32 GetStopResumeTask(
        PIRP pIrp, TaskletPtr& pTask );

	public:
    typedef IPort   super;

    CPort( const IConfigDb* pCfg = nullptr );
    ~CPort();

	virtual gint32 SubmitIrp( IRP* pIrp );
	virtual gint32 SubmitPnpIrp( IRP* pIrp );
	virtual gint32 SubmitFuncIrp( IRP* pIrp );

	virtual gint32 CompleteIrp( IRP* pIrp );
	virtual gint32 CompletePnpIrp( IRP* pIrp );
	virtual gint32 CompleteFuncIrp( IRP* pIrp );
	virtual gint32 CompleteIoctlIrp( IRP* pIrp );

    virtual bool Unloadable()
    { return true; }
    // method for user code
	virtual gint32 OnSubmitIrp( IRP* pIrp )
    { return 0; }

    // notify the port a stop command is comming
	virtual gint32 OnQueryStop( IRP* pIrp )
    { return 0; }
    virtual gint32 OnPortStackDestroy( IRP* pIrp );
    virtual gint32 OnPortStackBuilt( IRP* pIrp );
    virtual gint32 OnPortStackReady( IRP* pIrp )
    { return 0; }

    virtual void OnPortStartFailed(
        IRP* pIrp, gint32 ret );

    virtual gint32 OnPortReady( IRP* pIrp );
    virtual void OnPortStopped();

    gint32 MakeAssocIrp(
        IRP* pMaster,
        IRP* pSlave );

    virtual gint32 CompleteAssocIrp(
        IRP* pMaster,
        IRP* pSlave );

    virtual gint32 OnCompleteSlaveIrp(
        IRP* pMaster, IRP* pSlave );

    bool CanCancelIrp( IRP* pIrp )
    { return pIrp->CanCancel(); }

	// provide a chance for caller to
	// cancel a request
	virtual gint32 CancelIrp(
        IRP* pIrp, bool bForce = true );

	gint32 CancelPnpIrp(
        IRP* pIrp, bool bForce = true );

	virtual gint32 CancelStartIrp(
        IRP* pIrp, bool bForce = true );

	gint32 CancelStopIrp(
        IRP* pIrp, bool bForce = true );

	virtual gint32 CancelFuncIrp(
        IRP* pIrp, bool bForce = true );

	virtual gint32 CanContinue( IRP* pIrp,
        guint32 dwNewState = PORT_STATE_INVALID,
        guint32* pdwOldState = nullptr );

    // methods from CObjBase
    gint32 GetProperty(
            gint32 iProp,
            CBuffer& oBuf ) const;

    gint32 SetProperty(
            gint32 iProp,
            const CBuffer& oBuf );

    gint32 EnumProperties(
        std::vector< gint32 >& vecProps ) const;

    gint32 RemoveProperty( gint32 iProp );

    PortPtr GetUpperPort() const;
    PortPtr GetLowerPort() const;

    inline gint32 GetPdoPort(
        PortPtr& portPtr ) const
    {
        return FindPortByType(
            PORTFLG_TYPE_PDO, portPtr );
    }

    inline gint32 GetFdoPort(
        PortPtr& portPtr ) const
    {
        return FindPortByType(
            PORTFLG_TYPE_FDO, portPtr );
    }

    gint32 GetStackPos() const;

    gint32 StartEx( IRP* irp );
    gint32 PreStart( IRP* irp );
    virtual gint32 Start( IRP* irp )
    { return 0; }
    gint32 PostStart( IRP* irp );

    virtual gint32 OnPrepareStop( IRP* irp )
    { return 0; }

    gint32 StopEx( IRP* irp );
    gint32 PreStop( IRP* irp );
    gint32 PostStop( IRP* irp );
    virtual gint32 Stop( IRP* irp )
    { return 0; }

    gint32 StopLowerPort( IRP* irp );

    guint32 GetPortState() const
    { return m_pPortState->GetPortState(); }

    gint32 SetPortState( guint32 state )
    { return m_pPortState->SetPortState( state ); }

    // return the poped state
    gint32 PopState( guint32& dwState )
    { return m_pPortState->PopState( dwState ); }

    stdrmutex& GetLock() const
	{ return ( stdrmutex& )m_oLock; }

	bool SetPortStateWake(
		guint32 dwCurState, guint32 dwNewState, bool bWakeAll = false );

    inline bool IsPortReady()
    { return m_pPortState->IsPortReady(); }

    inline guint32 GetPortType()
    {
        CCfgOpenerObj oPortCfg( this );
        guint32 dwVal = PORTFLG_TYPE_INVALID;
        oPortCfg.GetIntProp(
            propPortType, dwVal );

        return dwVal;
    }

    gint32 FindPortByType(
        guint32 dwPortType,
        PortPtr& pPort ) const;

    gint32 AllocIrpCtxExt(
        IrpCtxPtr& pIrpCtx,
        void* pContext = nullptr ) const;

    gint32 ReleaseIrpCtxExt(
        IrpCtxPtr& pIrpCtx,
        void* pContext = nullptr )
    { return 0; }

    gint32 OnEvent( EnumEventId iEvent,
        LONGWORD dwParam1,
        LONGWORD dwParam2,
        LONGWORD* pData );

    gint32 AttachToPort( IPort* pLowerPort );
    gint32 DetachFromPort( IPort* pLowerPort );

    gint32 PassUnknownIrp( IRP* pIrp );

    PortPtr GetTopmostPort() const;
    PortPtr GetBottomPort() const;

    inline CIoManager* GetIoMgr() const
    { return m_pIoMgr; }

    static std::string ExClassToInClass(
        const std::string& strExternalName );

    static std::string InClassToExClass(
        const std::string& strInternalName );

    static bool CanAcceptMsg(
        guint32 dwPortState );

};

class CIrpGateKeeper 
{
    protected:
    CPort* m_pPort;

    public:
    CIrpGateKeeper( CPort* pPort )
    {
        if( pPort != nullptr )
            m_pPort = pPort;
    }

    bool IsTopPort( IRP* pIrp )
    {
        IPort* pPort =
           pIrp->m_vecCtxStack.front().first; 

        if( pPort == nullptr )
        {
            // the irp will not be
            // tracked
            return false;
        }

        if( m_pPort->GetObjId()
            == pPort->GetObjId() )
            return true;

        return false;
    }

    void IrpIn( IRP* pIrp )
    {
        if( nullptr == pIrp )
            return;

        CStdRMutex oPortLock( m_pPort->GetLock() );
        if( !IsTopPort( pIrp ) )
            return;

        guint32 dwMajorCmd =
            pIrp->GetTopStack()->GetMajorCmd();


        guint32 dwMinorCmd =
            pIrp->GetTopStack()->GetMinorCmd();

        // let `pnp stop' to pass through
        if( dwMajorCmd == IRP_MJ_PNP
            && dwMinorCmd == IRP_MN_PNP_STOP )
            return;

        IrpPtr irpPtr( pIrp );
        m_pPort->m_mapIrpIn[ irpPtr ] = 1;
        return;
    }

    void IrpOut( IRP* pIrp )
    {
        if( nullptr == pIrp )
            return;

        CStdRMutex oPortLock( m_pPort->GetLock() );

        if( !IsTopPort( pIrp ) )
            return;

        IrpPtr irpPtr( pIrp );
        m_pPort->m_mapIrpIn.erase( irpPtr );
        return;
    }

    guint32 GetCount() const
    {
        CStdRMutex oPortLock( m_pPort->GetLock() );
        return m_pPort->m_mapIrpIn.size();
    }

    void GetAllIrps(
        std::map< IrpPtr, gint32 >& mapIrps )
    {
        CStdRMutex oPortLock( m_pPort->GetLock() );
        mapIrps = m_pPort->m_mapIrpIn;
        m_pPort->m_mapIrpIn.clear();
        return;
    }
};


struct PORT_START_STOP_EXT
{
    guint32 m_dwState;
};

struct CGenBusPnpExt :
    public PORT_START_STOP_EXT
{
    guint32 m_dwExtState;
};

#define BUSPORT_STOP_RETRY_SEC     1
#define BUSPORT_STOP_RETRY_TIMES   3

class CGenericBusPort : public CPort
{
    protected:

    // auto-increment id counter
    std::atomic<guint32> m_atmPdoId;

    // map from port id to port pointer
    // the reference here should be the last
    // before the pdo get removed
	std::map<guint32, PortPtr> m_mapId2Pdo;

    public:

    typedef CPort super;
    CGenericBusPort( const IConfigDb* pCfg );

	virtual gint32 SubmitPnpIrp( IRP* pIrp );
	virtual gint32 CompletePnpIrp( IRP* pIrp );

    virtual bool Unloadable()
    { return false; }

    // start point of a driver stack buiding
    virtual gint32 OpenPdoPort(
            const IConfigDb* pConfig,
            PortPtr& pNewPort );

    virtual gint32 ClosePdoPort(
            PIRP pMasterIrp,
            IPort* pPdoPort );

    virtual gint32 EnumPdoPorts(
            std::vector<PortPtr>& vecPorts );

    virtual gint32 BuildPdoPortName(
            IConfigDb* pCfg,
            std::string& strPortName ) = 0;

    virtual gint32 CreatePdoPort(
        IConfigDb* pConfig,
        PortPtr& pNewPort ) = 0;

    inline guint32  NewPdoId()
    { return m_atmPdoId++; }

    virtual void AddPdoPort(
            guint32 iPortId,
            PortPtr& portPtr );

    virtual void RemovePdoPort(
            guint32 iPortId );

    gint32 GetPdoPort(
            guint32 iPortId, PortPtr& portPtr ) const;

    virtual gint32 GetChildPorts(
            std::vector< PortPtr >& vecChildPdo );

    bool PortExist( guint32 iPortId ) const;

    virtual gint32 OnPrepareStop( IRP* irp );

    virtual gint32 StopChild( IRP* pIrp );

    virtual gint32 PreStop( IRP* pIrp );

    // handler on port destroy
    gint32 OnPortStackDestroy( IRP* pIrp );

    gint32 SchedulePortAttachNotifTask(
        IPort* pNewPort,
        guint32 dwEventId,
        IRP* pMasterIrp = nullptr,
        bool bImmediately = false );

    gint32 AllocIrpCtxExt(
        IrpCtxPtr& pIrpCtx,
        void* pContext = nullptr ) const;
};

typedef CAutoPtr< Clsid_Invalid, CGenericBusPort > BusPortPtr;

