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

#include "defines.h"
#include "frmwrk.h"
#include "portdrv.h"
#include "port.h"
#include "emaphelp.h"
#include "iftasks.h"
#include "ifhelper.h"
#include "uxport.h"

namespace rpcf
{

class CStmConnPoint;
class CStreamQueue
{
    std::deque< IrpPtr > m_queIrps;
    std::deque< BufPtr > m_quePkts;
    CStmConnPoint* m_pParent = nullptr;

    // server side lock
    mutable stdmutex m_oLock;
    bool m_bInProcess = false;
    EnumTaskState m_dwState = stateStarting;
    MloopPtr m_pLoop;

    guint64  m_qwBytesIn = 0;
    guint64  m_qwBytesOut = 0;
    guint64  m_qwPktsIn = 0;
    guint64  m_qwPktsOut = 0;
    gint8    m_byStoppedBy = -1;

public:

    CStreamQueue()
    {}
    ~CStreamQueue()
    {}

    inline stdmutex& GetLock() const
    { return m_oLock; }

    void SetParent( CStmConnPoint* pParent )
    { m_pParent = pParent; }

    inline CStmConnPoint* GetParent()
    { return m_pParent; }

    CIoManager* GetIoMgr();
    gint32 QueuePacket( BufPtr& pBuf );
    gint32 SubmitIrp( PIRP pIrp );

    // cancel all the irps held by this conn
    gint32 CancelAllIrps( gint32 iError );
    gint32 RemoveIrpFromMap( PIRP pIrp );

    EnumTaskState GetState() const
    { return m_dwState; }
    
    void SetState( EnumTaskState dwState );

    gint32 Start();
    gint32 Stop( bool bStarter );
    gint32 ProcessIrps();

    inline void SetLoop( MloopPtr pLoop )
    { m_pLoop = pLoop; }

    gint32 RescheduleTask( TaskletPtr& pTask );
};

class CStmConnPoint : public IService
{
    CStreamQueue m_arrQues[ 2 ];

    CIoManager* m_pMgr;

    static stdmutex m_oLock;
    static std::hashmap< HANDLE, ObjPtr > m_mapConnPts;

    public:
    CStmConnPoint( const IConfigDb* pCfg );
    inline CStreamQueue& GetSendQueue( bool bStarter )
    { return bStarter ? m_arrQues[ 0 ] : m_arrQues[ 1 ]; }

    inline CStreamQueue& GetRecvQueue( bool bStarter )
    { return bStarter ? m_arrQues[ 1 ] : m_arrQues[ 0 ]; }

    inline CIoManager* GetIoMgr()
    { return m_pMgr; }

    gint32 Start() override;
    gint32 Stop() override
    { return 0; }
    gint32 Stop( bool bStarter );

    gint32 OnEvent( EnumEventId iEvent,
        LONGWORD dwParam1,
        LONGWORD dwParam2,
        LONGWORD* pData ) override;

    void SetLoop( MloopPtr pLoop, bool bStarter )
    {
        if( bStarter )
            GetSendQueue( bStarter ).SetLoop( pLoop );
        else
            GetRecvQueue( bStarter ).SetLoop( pLoop );
    }

    static stdmutex& GetLock()
    { return m_oLock; }

    gint32 RegForTransfer();
    static gint32 RetrieveAndUnreg(
        HANDLE hConn, ObjPtr& pConn );
};

class CStmConnPointHelper
{
    ObjPtr& m_pObj;
    CStmConnPoint& m_oStmCp;
    CStreamQueue& m_oRecvQue;
    CStreamQueue& m_oSendQue;

    public:
    CStmConnPointHelper(
        ObjPtr& pStmCp, bool bStarter ) :
        m_pObj( pStmCp ),
        m_oStmCp( *( CStmConnPoint* )m_pObj ),
        m_oRecvQue(
            m_oStmCp.GetRecvQueue( bStarter ) ),
        m_oSendQue(
            m_oStmCp.GetSendQueue( bStarter ) )
    {}

    ~CStmConnPointHelper()
    {}

    gint32 Send( BufPtr& pBuf )
    { return m_oSendQue.QueuePacket( pBuf ); }

    gint32 SubmitListeningIrp( PIRP pIrp )
    { return m_oRecvQue.SubmitIrp( pIrp ); };

    inline gint32 Stop( bool bStarter )
    { return m_oRecvQue.Stop( bStarter ); }

    gint32 RemoveIrpFromMap( PIRP pIrp )
    { return m_oRecvQue.RemoveIrpFromMap( pIrp ); }
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
    bool   m_bStarter = false;
    MloopPtr m_pLoop;

    public:

    typedef CPort super;

    // the configuration must includes the
    // following properties
    //
    // propIoMgr
    // propBusPortPtr
    // propPortId
    //
    // optional properties:
    // propPortClass
    // propPortName
    //
    CStmCpPdo( const IConfigDb* pCfg );
    ~CStmCpPdo()
    {}

    inline MloopPtr& GetMainLoop()
    { return m_pLoop; }

    inline void SetMainLoop( MloopPtr pLoop )
    {
        m_pLoop = pLoop;
        CStmConnPoint* pStmcp = m_pStmCp;
        pStmcp->SetLoop(
            pLoop, this->IsStarter() );
    }

    virtual gint32 CancelFuncIrp(
        IRP* pIrp, bool bForce );
    gint32 RemoveIrpFromMap( IRP* pIrp );
    virtual gint32 OnSubmitIrp( IRP* pIrp );

    gint32 PostStart( IRP* pIrp ) override;
    gint32 PreStop( IRP* pIrp ) override;

    bool IsStarter()
    { return m_bStarter; }
};

}
