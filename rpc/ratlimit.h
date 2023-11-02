/*
 * =====================================================================================
 *
 *       Filename:  ratlimit.h
 *
 *    Description:  declaration of QOS related classes 
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

class CTimerWatchCallback2 :
    public CTimerWatchCallback
{
    TaskletPtr m_pCallback;
    guint32 m_dwEvent = 0;
    guint32 m_dwContext = 0;
    bool  m_bRepeat = false;
    public:
    typedef CTasklet super;
    CTimerWatchCallback2( const IConfigDb* pCfg )
        : super( pCfg )
    {
        gint32 ret = 0;
        SetClassId( clsid( CTimerWatchCallback2 ) );
        do{
            CCfgOpener oCfg( pCfg );
            oCfg.GetIntProp( propEventId, m_dwEvent );
            oCfg.GetIntProp( propContext, m_dwContext );
            ObjPtr pObj;
            ret = oCfg.GetObjPtr( propObjPtr, pObj );
            if( SUCCEEDED( ret ) )
                m_pCallback = pObj;
            oCfg.GetBoolProp( 1, m_bRepeat );
        }while( 0 );
        return;
    }

    gint32 operator()( guint32 dwContext ) override
    {
        gint32 ret = 0;
        if( !m_pCallback.IsEmpty() )
        {
            ret = ( *m_pCallback )( m_dwEvent,
                m_dwContext, 0, nullptr );
        }
        if( !bRepeat || ERROR( ret ) )
            return G_SOURCE_REMOVE;
        return G_SOURCE_CONTINUE;
    }
};

struct CTokenTimerTask :
    public CIfParallelTask
{
    protected:
    timespec    m_tsTimestamp;
    guint32     m_dwMaxTokens = 0;
    guint32     m_dwTokens = 0;
    PortPtr     m_pPort;

    public:
    typedef CIfParallelTask super;

    CTokenTimerTask( const IConfigDb* pCfg );
    gint32 RunTask() override
    { return STATUS_PENDING; }

    bool IsDisabled()
    { return m_dwMaxTokens == ( guint32 )-1; }

    void Disable()
    { SetMaxTokens( ( guint32 )-1 ); }

    gint32 AllocToken( guint32& dwNumReq )
    {
        CStdRTMutex oLock( GetLock() );
        if( GetTaskState() != stateReady )
            return ERROR_STATE;

        if( IsDisabled() )
            return 0;

        if( m_dwTokens == 0 )
            return -ENOENT;

        if( dwNumReq >= m_dwTokens )
        {
            dwNumReq = m_dwTokens;
            m_dwTokens = 0;
        }
        else
        {
            m_dwTokens -= dwNumReq;
        }
        return 0;
    }

    gint32 FreeToken( guint32 dwNumReq )
    {
        CStdRTMutex oLock( GetLock() );
        if( GetTaskState() != stateReady )
            return ERROR_STATE;

        if( IsDisabled() )
            return 0;

        if( m_dwTokens == m_dwMaxTokens )
            return ERROR_QUEUE_FULL;

        m_dwTokens = std::min(
            m_dwTokens + dwNumReq,
            m_dwMaxTokens );

        return 0;
    }

    gint32 OnRetry() override
    {
        if( IsDisabled() )
            return 0;

        do{
            gint32 ret = FreeToken( m_dwMaxTokens );
            if( ERROR( ret ) )
                break;

            CIoManager* pMgr = GetIoMgr();
            gint32 (*func)( CPort*, IEventSink* ) =
            ([]( CPort* pPort,
                IEventSink* pTokenTask )->gint32
            {
                pPort->OnEvent( eventResumed,
                    0, 0, ( HANDLE )pTokenTask );
                return 0;
            }

            TaskletPtr pTask;
            ret = NEW_FUNCCALL_TASK( pTask,
                pMgr, func, pPort, this );
            if( ERROR( ret ) )
                break;
            pMgr->RescheduleTask( pTask );
        }while( 0 );
        return STATUS_PENDING;
    }

    gint32 GetMaxTokens(
        guint32& dwMaxTokens ) const
    {
        CStdRTMutex oLock( GetLock() );
        if( GetTaskState() != stateReady )
            return ERROR_STATE;
        dwMaxTokens = m_dwMaxTokens;
        return 0;
    }

    gint32 SetMaxTokens(
        guint32 dwMaxTokens )
    {
        CStdRTMutex oLock( GetLock() );
        if( GetTaskState() != stateReady )
            return ERROR_STATE;
        m_dwMaxTokens = dwMaxTokens;
        return 0;
    }
    gint32 OnCancel(
        guint32 dwContext ) override;
};

class CRateLimiterFido : public CPort
{
    std::deque< IrpPtr >  m_queOutIrp;
    std::deque< IrpPtr >  m_queInIrp;
    TaskletPtr  m_pOutTimer;
    TaskletPtr  m_pInTimer;

    HANDLE      m_hInWatch;
    HANDLE      m_hOutWatch;

    public:
    typedef CPort super;
    CRateLimiterFido( const IConfigDb* pCfg );
    gint32 PostStart( PIRP pIrp ) override;
    gint32 PreStop( PIRP pIrp ) override;
    gint32 CompleteFuncIrp( PIRP pIrp );
    gint32 OnSubmitIrp( PIRP pIrp );

    gint32 OnEvent( EnumEventId iEvent,
            LONGWORD dwParam1 = 0,
            LONGWORD dwParam2 = 0,
            LONGWORD* pData = NULL  ) override;
};
