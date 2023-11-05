/*
 * =====================================================================================
 *
 *       Filename:  ratlimit.h
 *
 *    Description:  Declarations of rate limiter related classes 
 *
 *        Version:  1.0
 *        Created:  11/01/2023 03:27:05 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
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
#include "port.h"
#include "frmwrk.h"
#include "tcportex.h"
#include "iftasks.h"
namespace rpcf
{

typedef enum 
{
    DECL_CLSID( CRateLimiterDrv ) =
        clsid( ClassFactoryStart ) + 100 ,
    DECL_CLSID( CRateLimiterFido ),

} EnumRatLimitClsid;

class CRateLimiterDrv : public CRpcTcpFidoDrv
{
    public:
    typedef CRpcTcpFidoDrv super;

    CRateLimiterDrv(
        const IConfigDb* pCfg = nullptr );

	gint32 Probe( IPort* pLowerPort,
        PortPtr& pNewPort,
        const IConfigDb* pConfig = NULL );
};

class CRateLimiterFido : public CPort
{
    std::deque< IrpPtr >  m_queWriteIrp;
    std::deque< IrpPtr >  m_queReadIrp;

    TaskletPtr  m_pReadTb;
    TaskletPtr  m_pWriteTb;

    IrpPtr      m_pReadIrp;
    IrpPtr      m_pWriteIrp;
    MloopPtr    m_pLoop;

    public:
    typedef CPort super;
    CRateLimiterFido( const IConfigDb* pCfg )
        : super( pCfg )
    {
        SetClassId( clsid( CRateLimiterFido ) );
        m_dwFlags &= ~PORTFLG_TYPE_MASK;
        m_dwFlags |= PORTFLG_TYPE_FIDO;
    }

    gint32 PostStart( PIRP pIrp ) override
    {
        gint32 ret = 0;
        do{
            PortPtr pPdo;
            ret = this->GetPdoPort( pPdo );
            if( ERROR( ret ) )
                break;
            CTcpStreamPdo2* pStmPdo = pPdo;
            CMainIoLoop* pLoop =
                pStmPdo->GetMainLoop();
            if( pLoop == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            m_pLoop = pLoop;
            CParamList oParams;
            oParams.SetPointer(
                propObjPtr, pLoop );
            oParams.SetPointer(
                propParentPtr, this );
            oParams.SetIntProp(
                propTimeoutSec, 1 );
            oParams.CopyProp(
                0, propReadBps, this );

            ret = m_pReadTb.NewObj(
                clsid( CTokenBucketTask ),
                oParams.GetCfg();
            if( ERROR( ret ) )
                break;

            oParams.CopyProp(
                0, propWriteBps, this );

            ret = m_pWriteTb.NewObj(
                clsid( CTokenBucketTask ),
                oParams.GetCfg();
            if( ERROR( ret ) )
                break;

            ret = ( *m_pReadTb )( eventZero );
            if( ERROR( ret ) )
                break;
            ret = ( *m_pWriteTb )( eventZero );
            if( ERROR( ret ) )
                break;

        }while( 0 );
        if( ERROR( ret ) )
        {
            if( !m_pReadTb.IsEmpty() )
                ( *m_pReadTb )( eventCancelTask );
            if( !m_pWriteTb.IsEmpty() )
                ( *m_pReadTb )( eventCancelTask );
        }
        return ret;
    }

    gint32 PreStop( PIRP pIrp ) override
    {
        if( !m_pWriteTb.IsEmpty() )
        {
            ( *m_pWriteTb )( eventCancelTask );
            m_pWriteTb.Clear();
        }
        if( !m_pReadTb.IsEmpty() )
        {
            ( *m_pReadTb )( eventCancelTask );
            m_pReadTb.Clear();
        }

        // the irps will be released in super class's
        // PreStop
        m_queWriteIrp.clear();
        m_queReadIrp.clear();
        m_pReadIrp.Clear();
        m_pWriteIrp.Clear();
        m_pLoop.Clear();
        return super::PreStop( pIrp );
    }

    gint32 ResumeWrite();
    gint32 ResumeRead();

    gint32 CompleteFuncIrp( PIRP pIrp );
    gint32 OnSubmitIrp( PIRP pIrp );

    gint32 OnEvent( EnumEventId iEvent,
            LONGWORD dwParam1 = 0,
            LONGWORD dwParam2 = 0,
            LONGWORD* pData = NULL  ) override
    {
        gint32 ret = 0;
        switch( iEvent )
        {
        case eventResumed:
            {
                CStdRMutex oPortLock( GetLock() );
                guint32 dwState = GetPortState();
                if( dwState == PORT_STATE_STOPPING ||
                    dwState == PORT_STATE_STOPPED ||
                    dwState == PORT_STATE_REMOVED )
                {
                    ret = ERROR_STATE;
                    break;
                }
                auto pTask = reinterpret_cast
                    < CTokenBucketTask* >( pData );
                guint64 qwObjId =
                    pTask->GetObjId();
                if( qwObjId ==
                    m_pReadTb->GetObjId() )
                {
                    oPortLock.Unlock();
                    ret = ResumeRead();
                }
                else if( qwObjId ==
                    m_pWriteTb->GetObjId() )
                {
                    oPortLock.Unlock();
                    ret = ResumeWrite();
                }
                break;
            }
        default:
            {
                ret = super::OnEvent(
                    iEvent, dwParam1,
                    dwParam2, pData );
                break;
            }
        }
        return ret;
    }
};

}
