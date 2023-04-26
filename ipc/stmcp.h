/*
 * =====================================================================================
 *
 *       Filename:  stmcp.h
 *
 *    Description:  Declarations of the classes as the connection point between 
 *                  proxy/server stream objects
 *
 *        Version:  1.0
 *        Created:  04/22/2023 08:59:27 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *
 *      Copyright:  2023 Ming Zhi( woodhead99@gmail.com )
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

namespace rpcf
{

class CStreamQueue
{
    std::deque< IrpPtr > m_queIrps;
    std::deque< BufPtr > m_quePkts;
    CStmConnPoint* m_pParent = nullptr;

    // server side lock
    stdmutex m_oLock;
    bool m_bInProcess = false;
    EnumTaskState m_dwState = stateStarting;
    gint32 ProcessIrps();

public:

    CStreamQueue()
    {}
    ~CStreamQueue()
    {}

    stdmutex& GetLock()
    { return m_oLock; }

    void SetParent( CStmConnPoint* pParent )
    { m_pParent = pParent; }

    inline CStmConnPoint* GetParent()
    { return m_pParent; }

    inline CIoManager* GetIoMgr()
    {
        if( m_pParent == nullptr )
            return nullptr;
        return m_pParent->GetIoMgr();
    }

    gint32 QueuePacket( BufPtr& pBuf );
    gint32 SubmitIrp( IrpPtr& pIrp );

    // cancel all the irps held by this conn
    gint32 CancelAllIrps( gint32 iError );

    EnumTaskState GetState() const
    { return m_dwState; }
    
    void SetState( EnumTaskState dwState );

    gint32 Start();
    gint32 Stop();
};

class CStmConnPoint : IService
{
    CStreamQueue m_arrQues[ 2 ];

    CStreamQueue& m_oQue2Cli = m_arrQues[ 0 ];
    CStreamQueue& m_oQue2Svr = m_arrQues[ 1 ];
    CIoManager* m_pMgr;

    public:
    CStmConnPoint( const IConfigDb* pCfg );
    inline CStreamQueue& GetQueue( bool bProxy )
    { return bProxy ? m_oQue2Cli : m_oQue2Svr; }

    inline CIoManager* GetIoMgr()
    { return m_pIoMgr; }

    gint32 Start() override;
    gint32 Stop() override;

    gint32 OnEvent( EnumEventId iEvent,
        LONGWORD dwParam1,
        LONGWORD dwParam2,
        LONGWORD* pData ) override;
};

class CStmConnPointHelper
{
    ObjPtr& m_pObj;
    CStmConnPoint& m_oStmCp;
    CStreamQueue& m_oRecvQue;
    CStreamQueue& m_oSendQue;

    public:
    CStmConnPointHelper(
        ObjPtr& pStmCp, bool bProxy )
        m_pObj( pStmCp ),
        m_oStmCp( ( CStmConnPoint& )m_pObj ),
        m_oRecvQue(
            m_oStmCp.GetQueue( bProxy ) ),
        m_oSendQue(
            m_oStmCp.GetQueue( !bProxy ) ),
    {}

    ~CStmCConnPointT( ObjPtr pStmCp )
    {}

    gint32 Send( BufPtr& pBuf )
    { return m_oSendQue.QueuePacket( pBuf ); }

    gint32 SubmitListeningIrp( PIRP pIrp )
    { return m_oRecvQue.SubmitIrp( pIrp ); };

    inline gint32 Stop()
    { return m_oRecvQue.Stop(); }
};

class CStmCpPdo : public CPort
{
    protected:

    gint32 SubmitReadIrp( IRP* pIrp )
    { return -ENOTSUP; }
    gint32 SubmitWriteIrp( IRP* pIrp );
    gint32 SubmitIoctlCmd( IRP* pIrp );
    gint32 HandleListening( IRP* pIrp );
    gint32 HandleStreamCommand( IRP* pIrp );

    ObjPtr m_pStmCp;
    bool   m_bStarter;

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
    CStmCpPdo( const IConfigDb* pCfg );
    ~CStmCpPdo();

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
    {}

    gint32 SendNotify(
        guint8 byToken, BufPtr& pBuf );

    gint32 OnPortReady( IRP* pIrp );

    bool IsStarter()
    { return m_bStarter; }
};

}
