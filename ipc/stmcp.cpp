/*
 * =====================================================================================
 *
 *       Filename:  stmcp.cpp
 *
 *    Description:  Definitions of the classes as the connection point between 
 *                  proxy/server stream objects
 *
 *        Version:  1.0
 *        Created:  04/23/2023 05:56:34 PM
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
#include "stmcp.h"

void CStreamQueue::SetState(
    EnumTaskState dwState )
{
    CStdMutex oLock( this->GetLock() );
    m_dwState = dwState;
}

gint32 CStreamQueue::Start() 
{
    SetState( stateStarted );
    return 0;
}

gint32 CStreamQueue::Stop() 
{
    if( true )
    {
        CStdMutx oLock( this->GetLock() );
        if( GetState() == stateStopped )
            return ERROR_STATE;
    }
    SetState( stateStopping );
    CancelAllIrps();
    SetState( stateStopped );
    return 0;
}

gint32 CStreamQueue::QueuePacket( BufPtr& pBuf )
{
    gint32 ret = 0;
    do{
        CStdMutex oLock( GetLock() );
        if( GetState() != stateStarted )
        {
            ret = ERROR_STATE;
            break;
        }

        if( m_quePkts.size >= STM_MAX_QUEUE_SIZE )
        {
            ret = ERROR_QUEUE_FULL;
            break;
        }

        m_quePkts.push_back( pBuf );
        if( m_bInProcess )
            break;

        if( m_queIrps.empty() )
            break;

        TaslketPtr pTask;
        ret = DEFER_OBJCALL_NOSCHED(
            pTask, this,
            &CStreamQueue::ProcessIrps );
        if( ERROR( ret ) )
            break;

        m_bInProcess = true;
        CIoManager* pMgr = this->GetIoMgr();
        ret = pMgr->RescheduleTask( pTask );
        if( ERROR( ret ) )
            m_bInProcess = false;

    }while( 0 );

    return ret;
}

gint32 CStreamQueue::SubmitIrp( IrpPtr& pIrp )
{
    gint32 ret = 0;
    do{
        CStdMutex oLock( GetLock() );
        if( GetState() != stateStarted )
        {
            ret = ERROR_STATE;
            break;
        }

        if( m_bInProcess || m_quePkts.empty() )
        {
            m_queIrps.push_back( pIrp );
            ret = STATUS_PENDING;
            break;
        }

        if( m_queIrps.size() >= STM_MAX_QUEUE_SIZE )
        {
            ret = ERROR_QUEUE_FULL;
            break;
        }

        if( m_queIrps.size() )
        {
            m_queIrps.push_back( pIrp );
            TaslketPtr pTask;
            ret = DEFER_OBJCALL_NOSCHED(
                pTask, this,
                &CStreamQueue::ProcessIrps );
            if( ERROR( ret ) )
                break;

            CIoManager* pMgr = this->GetIoMgr();
            ret = pMgr->RescheduleTask( pTask );
            break;
        }

        BufPtr pResp = m_quePkts.front();
        m_quePkts.pop_front();
        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        pCtx->SetRespData( pResp );
        pCtx->SetStatus( STATUS_SUCCESS );
        if( m_quePkts.size() ==
            STM_MAX_QUEUE_SIZE - 1 )
        {
            oLock.Unlock();
            GetParent()->OnEvent( eventResumed,
                0, 0, ( guint32* )this );
        }

    }while( 0 );

    return ret;
}

gint32 CStreamQueue::ProcessIrps()
{
    gint32 ret = 0;
    do{
        CIoManager* pMgr = GetIoMgr();
        CStdMutex oLock( this->GetLock() );
        if( GetState() == stateStopped )
        {
            ret = ERROR_STATE;
            break;
        }
        if( !m_bInProcess )
            m_bInProcess = true;

        if( m_quePkts.empty() || m_queIrps.empty() )
        {
            m_bInProcess = false;
            break;
        }

        while( m_queIrps.size() &&
            m_quePkts.size() )
        {
            IrpPtr pIrp = m_queIrps.front();
            m_queIrps.pop_front( pIrp );
            BufPtr pResp = m_quePkts.front();
            m_quePkts.pop_front();
            oLock.Unlock();

            bool bPutBack = false;
            CStdRMutex oIrpLock( pIrp->GetLock() );
            if( pIrp->CanContinue( IRP_STATE_READY ) )
            {
                IrpCtxPtr& pCtx = pIrp->GetTopStack();
                pCtx->SetRespData( pResp );
                pCtx->SetStatus( STATUS_SUCCESS );
                oIrpLock.Unlock();
                pMgr->CompleteIrp( pIrp );
            }
            else
            {
                oIrpLock.Unlock();
                bPutBack = true;
            }
            oLock.Lock();
            if( bPutBack )
                m_quePkts.push_front( pResp );
            guint32 dwCount = m_quePkts.size();
            if( !bPutBack &&
                dwCount == STM_MAX_QUEUE_SIZE - 1 )
            {
                GetParent()->OnEvent( eventResumed,
                    0, 0, ( guint32* )this );
            }
            oLock.Lock();
        }
        m_bInProcess = false;

    }while( 0 );


    return ret;
}

gint32 CStreamQueue::CancelAllIrps(
    gint32 iError )
{
    gint32 ret = 0;
    do{
        CStdMutex oLock( this->GetLock() );
        if( GetState() == stateStopped )
        {
            ret = ERROR_STATE;
            break;
        }
        if( m_bInProcess )
        {
            ret = -EAGAIN;
            break;
        }

        std::vector< IrpPtr > vecIrps;
        vecIrps.insert( m_queIrps.begin(),
            m_queIrps.end() );
        m_queIrps.clear();
        m_quePkts.clear();
        oLock.Unlock();

        for( auto& elem : vecIrps )
        {
            GetIoMgr()->CancelIrp(
                elem, false, iError );
        }

    }while( 0 );

    return ret;
}

CStmConnPoint::CStmConnPoint(
    const IConfigDb* pCfg )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg( pCfg );
        ret = oCfg.GetPointer(
            propIoMgr, m_pIoMgr );

        gint32 iCount = sizeof( m_arrQues ) /
            sizeof( m_arrQues[ 0 ] )

        for( gint32 i = 0; i < iCount; i++ )
            m_arrQues[ i ].SetParent( this );

    }while( 0 );

    if( SUCCEEDED( ret ) )
        return;

    stdstr strMsg = DebugMsg( ret,
        "cannot find iomgr in "
        "CStmConnPoint's ctor" );
    throw std::runtime_error( strMsg );
}

gint32 CStmConnPoint::Start()
{
    gint32 ret = 0;

    gint32 iCount = sizeof( m_arrQues ) /
        sizeof( m_arrQues[ 0 ] )

    for( gint32 i = 0; i < iCount; i++ )
        m_arrQues[ i ].Start();

    return ret;
}

gint32 CStmConnPoint::Stop()
{
    gint32 ret = 0;
    do{
        gint32 iCount = sizeof( m_arrQues ) /
            sizeof( m_arrQues[ 0 ] )

        for( gint32 i = 0; i < iCount; i++ )
            m_arrQues[ i ].Stop();

    }while( 0 );
    return ret;
}

gint32 CStmConnPoint::OnEvent(
    EnumEventId iEvent,
    LONGWORD dwParam1,
    LONGWORD dwParam2,
    LONGWORD* pData )
{
    gint32 ret = 0;
    switch( iEvent )
    {
    case eventResumed:
        {
            auto pQue = reinterpret_cast
                < CStreamQueue* >( pData );

            guint32 dwIdx = ( pQue - m_arrQues ) /
                sizeof( *pQue );

            if( dwIdx >= 2 )
            {
                ret = -ERANGE;
                break;
            }

            dwIdx ^= 1;

            BufPtr pBuf( true );
            pBuf->Resize( 1 );
            *pBuf->ptr() = tokLift;
            m_arrQues[ dwIdx ].QueuePacket( pBuf );
            break;
        }
    default:
        break;
    }
    return ret;
}
