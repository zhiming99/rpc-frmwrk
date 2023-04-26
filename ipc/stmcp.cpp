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

gint32 CStmCpPdo::CancelFuncIrp(
    IRP* pIrp, bool bForce )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        ret = RemoveIrpFromMap( pIrp );
        if( ERROR( ret ) )
            break;

        CStdRMutex oIrpLock( pIrp->GetLock() );
        ret = pIrp->CanContinue( IRP_STATE_READY );
        if( ERROR( ret ) )
            break;

        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        pCtx->SetStatus( ret = ERROR_CANCEL );

        oIrpLock.Unlock();
        super::CancelFuncIrp( pIrp, bForce );

    }while( 0 );

    return ret;
}

gint32 CStmCpPdo::SubmitIoctlCmd(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    // let's process the func irps
    IrpCtxPtr pCtx = pIrp->GetCurCtx();
    do{
        if( pIrp->MajorCmd() != IRP_MJ_FUNC
            || pIrp->MinorCmd() != IRP_MN_IOCTL )
        {
            ret = -EINVAL;
            break;
        }

        switch( pIrp->CtrlCode() )
        {
        case CTRLCODE_LISTENING:
            {
                // server side I/O
                ret = HandleListening( pIrp );
                break;
            }
        case CTRLCODE_STREAM_CMD:
            {
                ret = HandleStreamCommand( pIrp );
                break;
            }
        default:
            {
                ret = -ENOTSUP;
                break;
            }
        }

    }while( 0 );

    if( ret != STATUS_PENDING )
        pCtx->SetStatus( ret );

    return ret;
}

gint32 CStmCpPdo::HandleListening(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;

    // let's process the func irps
    IrpCtxPtr& pCtx = pIrp->GetTopStack();
    do{
        CIoWatchTask* pTask = m_pIoWatch;
        CStdRMutex oPortLock( GetLock() );
        std::deque< BufPtr >& oEvtQue =
            m_queEventPackets;
        if( !oEvtQue.empty() )
        {
            BufPtr pBuf = oEvtQue.front();        
            pCtx->m_pRespData = pBuf;
            oEvtQue.pop_front();
            ret = STATUS_SUCCESS;
            if( m_bFlowCtrl )
                break;
            if( oEvtQue.size() ==
                STM_MAX_QUEUE_SIZE - 1 )
                pTask->StartWatch( false );
            break;
        }

        if( m_queListeningIrps.size() >
            STM_MAX_QUEUE_SIZE )
        {
            // why send read request so many times
            ret = ERROR_QUEUE_FULL;
            break;
        }
        m_queListeningIrps.push_back( pIrp );
        ret = STATUS_PENDING;

    }while( 0 );

    return ret;
}

gint32 CStmCpPdo::HandleStreamCommand(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    bool bNotify = false;
    CIoWatchTask* pTask = m_pIoWatch;
    if( pTask == nullptr )
        return -EFAULT;

    // let's process the func irps
    do{

        CStdRTMutex oTaskLock( pTask->GetLock() );
        CStdRMutex oPortLock( GetLock() );
        IrpCtxPtr& pCtx = pIrp->GetTopStack();

        bool bOut = true;
        if( pCtx->GetIoDirection() == IRP_DIR_IN )
            bOut = false;

        if( bOut && m_queWritingIrps.size() >=
            STM_MAX_QUEUE_SIZE )
            return ERROR_QUEUE_FULL;

        if( bOut && !m_queWritingIrps.empty() )
        {
            m_queWritingIrps.push_back( pIrp );
            if( m_queWritingIrps.size() ==
                STM_MAX_QUEUE_SIZE )
                bNotify = true;
            ret = STATUS_PENDING;
            break;
        }
            
        BufPtr pBuf = pCtx->m_pReqData;
        if( pBuf.IsEmpty() )
        {
            ret = -EINVAL;
            break;
        }

        guint8 cToken = pBuf->ptr()[ 0 ];
        switch( cToken )
        {
        case tokPong:
            ret = pTask->SendPingPong( false );
            break;

        case tokPing:
            ret = pTask->SendPingPong( true );
            break;

        case tokProgress:
            {
                pBuf->SetOffset( pBuf->offset() +
                    UXPKT_HEADER_SIZE );

                ret = pTask->WriteStream(
                    pBuf, tokProgress );

                // pBuf->SetOffset( pBuf->offset() -
                //     UXPKT_HEADER_SIZE );

                break;
            }
        case tokLift:
            {
                m_bFlowCtrl = false;
                if( m_queDataPackets.size() >=
                    STM_MAX_QUEUE_SIZE )
                {
                    // ret = ERROR_FAIL;
                    break;
                }
                pTask->StartWatch( false );
                break;
            }
        case tokFlowCtrl:
            {
                m_bFlowCtrl = true;
                pTask->StopWatch( false );
                break;
            }
        default:
            {
                ret = -EINVAL;
                break;
            }
        }

        if( ret == STATUS_PENDING )
        {
            m_queWritingIrps.push_back( pIrp );
            if( m_queWritingIrps.size() ==
                STM_MAX_QUEUE_SIZE )
                bNotify = true;
        }

    }while( 0 );
    if( ret == -EPIPE )
    {
        pTask->OnError( ret );
        return ret;
    }
    if( bNotify )
    {
        BufPtr pNullBuf;
        SendNotify( tokFlowCtrl, pNullBuf );
    }

    return ret;
}

gint32 CStmCpPdo::OnSubmitIrp(
    IRP* pIrp )
{

    gint32 ret = 0;
    do{
        if( pIrp == nullptr ||
            pIrp->GetStackSize() == 0 )
        {
            ret = -EINVAL;
            break;
        }

        // let's process the func irps
        if( pIrp->MajorCmd() != IRP_MJ_FUNC )
        {
            ret = -EINVAL;
            break;
        }

        switch( pIrp->MinorCmd() )
        {
        case IRP_MN_READ:
            {
                ret = SubmitReadIrp( pIrp );
                break;
            }
        case IRP_MN_WRITE:
            {
                ret = SubmitWriteIrp( pIrp );
                break;
            }
        case IRP_MN_IOCTL:
            {
                switch( pIrp->CtrlCode() )
                {
                case CTRLCODE_LISTENING:
                case CTRLCODE_STREAM_CMD:
                    {
                        ret = SubmitIoctlCmd( pIrp );
                        break;
                    }
                default:
                    {
                        ret = -ENOTSUP;
                        break;
                    }
                }

                if( ret != -ENOTSUP )
                    break;
                // fall through
            }
        default:
            {
                ret = PassUnknownIrp( pIrp );
                break;
            }
        }

    }while( 0 );

    if( ret != STATUS_PENDING )
    {
        pIrp->GetCurCtx()->SetStatus( ret );
    }

    return ret;
}

gint32 CStmCpPdo::SubmitWriteIrp(
    PIRP pIrp )
{
    gint32 ret = 0;
    bool bNotify = false;

    do{
        if( pIrp == nullptr ||
            pIrp->GetStackSize() == 0 )
        {
            ret = -EINVAL;
            break;
        }
        CStmConnPointHelper oh(
            m_pStmCp, IsStarter() );

    }while( 0 );

    if( ret == -EPIPE )
    {
        pTask->OnError( ret );
        return ret;
    }

    if( bNotify )
    {
        BufPtr pNullBuf( true );
        SendNotify( tokFlowCtrl, pNullBuf );
    }

    return ret;
}

