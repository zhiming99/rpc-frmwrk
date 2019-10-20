/*
 * =====================================================================================
 *
 *       Filename:  streamex.cpp
 *
 *    Description:  Implementation of CStreamServerEx and CStreamProxyEx and the
 *                  worker task
 *
 *        Version:  1.0
 *        Created:  08/29/2019 06:41:02 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *
 *      Copyright:  2019 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#include "uxstream.h"
#include "streamex.h"

#define COMPLETE_SEND_IRP( _pIrp, _ret ) \
do{ \
    gint32 _iRet = ( gint32 )( _ret ); \
    CStdRMutex oIrpLock( _pIrp->GetLock() ); \
    if( _pIrp->GetState() == IRP_STATE_READY ) \
    { \
        _pIrp->RemoveTimer(); \
        _pIrp->RemoveCallback(); \
        _pIrp->SetStatus( ( _iRet ) ); \
        Sem_Post( &_pIrp->m_semWait ); \
        _pIrp->SetState( IRP_STATE_READY, \
            IRP_STATE_COMPLETED ); \
        bool bNoWait = \
            ( ( guint32 )-1 == _pIrp->m_dwTimeoutSec ); \
        if( IsLoopStarted() && bNoWait ) \
            POST_SEND_DONE_EVT( m_hChannel, _iRet ); \
    } \
}while( 0 )

#define COMPLETE_RECV_IRP( _pIrp, _ret ) \
do{ \
    gint32 _iRet = ( gint32 )( _ret ); \
    CStdRMutex oIrpLock( _pIrp->GetLock() ); \
    if( _pIrp->GetState() == IRP_STATE_READY ) \
    { \
        _pIrp->RemoveTimer(); \
        _pIrp->RemoveCallback(); \
        _pIrp->SetStatus( ( _iRet ) ); \
        Sem_Post( &_pIrp->m_semWait ); \
        _pIrp->SetState( IRP_STATE_READY, \
            IRP_STATE_COMPLETED ); \
    } \
}while( 0 )

#define COMPLETE_IRP( _pIrp, _ret ) \
do{ \
    if( IsReading() ) \
        COMPLETE_RECV_IRP( _pIrp, _ret ); \
    else \
        COMPLETE_SEND_IRP( _pIrp, _ret ); \
}while( 0 )

#define GET_STMPTR( _pIf, _pStm ) \
do{ \
    if( _pIf->IsServer() ) \
    { \
        CStreamServer* pSvr = ObjPtr( _pIf ); \
        _pStm = pSvr; \
    } \
    else \
    { \
        CStreamProxy* pProxy = ObjPtr( _pIf ); \
        _pStm = pProxy; \
    } \
}while( 0 )

#define GET_STMPTR2( _pStm, _ret ) \
do{ \
    CParamList _oParams( \
        ( IConfigDb* )GetConfig() ); \
    CRpcServices* _pIf; \
    _ret = _oParams.GetPointer( \
        propIfPtr, _pIf ); \
    if( ERROR( _ret ) ) \
        break; \
    GET_STMPTR( _pIf, _pStm ); \
    if( pStm == nullptr ) \
    { \
        _ret = -EFAULT; \
        break; \
    } \
}while( 0 )

CIfStmReadWriteTask::CIfStmReadWriteTask(
    const IConfigDb* pCfg )
    : super( pCfg )
{
    SetClassId( clsid( CIfStmReadWriteTask ) );
    gint32 ret = 0;

    CParamList oParams(
        ( IConfigDb* )GetConfig() );
    do{
        CRpcServices* pIf;
        ret = oParams.GetPointer(
            propIfPtr, pIf );

        if( ERROR( ret ) )
            break;

        m_bIn = ( bool& )oParams[ 0 ];
        m_hChannel = ( intptr_t& )oParams[ 1 ];

        oParams.ClearParams();

    }while( 0 );

    if( ERROR( ret ) )
    {
        std::string strMsg = DebugMsg( ret,
            "Error in CIfUxSockTransRelayTask ctor" );
        throw std::runtime_error( strMsg );
    }
}

bool CIfStmReadWriteTask::CanSend()
{
    gint32 ret = 0;
    bool bRet = false;
    do{
        IStream* pStm = nullptr;
        GET_STMPTR2( pStm, ret );
        if( ERROR( ret ) )
            break;

        InterfPtr pUxIf;
        ret = pStm->GetUxStream(
            m_hChannel, pUxIf );

        if( ERROR( ret ) )
            break;

        if( pUxIf->IsServer() )
        {
            CUnixSockStmServer* pSvr = pUxIf;
            bRet = pSvr->CanSend();
        }
        else
        {
            CUnixSockStmProxy* pProxy = pUxIf;
            bRet = pProxy->CanSend();
        }

    }while( 0 );

    return bRet;
}

gint32 CIfStmReadWriteTask::OnFCLifted()
{
    if( GetReqCount() > 0 )
        Resume();

    gint32 ret = 0;
    POST_LOOP_EVENT( m_hChannel, 
        stmevtLifted, ret );
    return 0;
}

gint32 CIfStmReadWriteTask::PauseReading(
    bool bPause )
{
    gint32 ret = 0;
    do{
        IStream* pStm = nullptr;
        GET_STMPTR2( pStm, ret );
        if( ERROR( ret ) )
            break;

        InterfPtr pUxIf;
        ret = pStm->GetUxStream(
            m_hChannel, pUxIf );

        if( ERROR( ret ) )
            break;

        if( pUxIf->IsServer() )
        {
            CUnixSockStmServer* pSvr = pUxIf;
            ret = pSvr->PauseReading( bPause );
        }
        else
        {
            CUnixSockStmProxy* pProxy = pUxIf;
            ret = pProxy->PauseReading( bPause );
        }

    }while( 0 );

    return ret;
}

gint32 CIfStmReadWriteTask::OnWorkerIrpComplete(
    IRP* pIrp )
{
    if( pIrp == nullptr )
        return -EINVAL;

    bool bFound = false;
    std::deque< IrpPtr >::iterator
        itr = m_queRequests.begin();

    for( ; itr != m_queRequests.end(); ++itr )
    {
        if( *itr == IrpPtr( pIrp ) )
        {
            itr = m_queRequests.erase( itr );
            bFound = true;
        }
    }

    if( bFound )
    {
        pIrp->RemoveCallback();
        pIrp->RemoveTimer();
        Sem_Post( &pIrp->m_semWait ); \
    }

    gint32 ret = ReRun();
    if( SUCCEEDED( ret ) )
        ret = STATUS_PENDING;

    return ret;
}

bool CIfStmReadWriteTask::IsLoopStarted()
{
    gint32 ret = 0;
    do{
        IStream* pStm = nullptr;
        GET_STMPTR2( pStm, ret );
        if( ERROR( ret ) )
            return false;

        CRpcServices* pSvc =
            pStm->GetInterface();

        if( pSvc->IsServer() )
        {
            CStreamServerSync* pSvr =
                ObjPtr( pSvc );
            return pSvr->IsLoopStarted();
        }
        else
        {
            CStreamProxySync* pProxy =
                ObjPtr( pSvc );
            return pProxy->IsLoopStarted();
        }

    }while( 0 );

    return false;
}

gint32 CIfStmReadWriteTask::PostLoopEvent(
    BufPtr& pBuf )
{
    gint32 ret = 0;
    do{
        IStream* pStm = nullptr;
        GET_STMPTR2( pStm, ret );
        if( ERROR( ret ) )
            break;

        CRpcServices* pSvc =
            pStm->GetInterface();

        if( pSvc->IsServer() )
        {
            CStreamServerSync* pSvr =
                ObjPtr( pSvc );
            ret = pSvr->PostLoopEvent( pBuf );
        }
        else
        {
            CStreamProxySync* pProxy =
                ObjPtr( pSvc );
            ret = pProxy->PostLoopEvent( pBuf );
        }

    }while( 0 );

    return ret;
}

gint32 CIfStmReadWriteTask::OnIoIrpComplete(
    IRP* pIrp )
{
    if( pIrp == nullptr )
        return -EINVAL;

    gint32 ret = 0;

    do{
        ret = pIrp->GetStatus();
        if( ERROR( ret ) )
            break;

        // we should not be here
        if( IsReading() )
        {
            ret = ERROR_STATE;
            break;
        }

        IrpCtxPtr pCtx = pIrp->GetTopStack();
        BufPtr pWritten = pCtx->m_pReqData;
        if( pWritten.IsEmpty() ||
            pWritten->empty() )
        {
            ret = -EINVAL;
            break;
        }

        guint32 dwSize = pWritten->size();
        if( m_queRequests.empty() )
        {
            ret = ERROR_STATE;
            break;
        }

        IrpPtr pCurIrp = m_queRequests.front();
        IrpCtxPtr& pCurCtx =
            pCurIrp->GetTopStack();

        BufPtr pReqBuf = pCurCtx->m_pReqData;
        if( pReqBuf.IsEmpty() ||
            pReqBuf->empty() )
        {
            ret = -EINVAL;
            break;
        }

        if( pReqBuf->size() < dwSize )
        {
            ret = -ERANGE;
            break;
        }

        pReqBuf->SetOffset(
            pReqBuf->offset() + dwSize );

        if( pReqBuf->size() == 0 )
        {
            // recover the buffer
            pReqBuf->SetOffset( 0 );
            m_queRequests.pop_front();

            gint32 iRet = pIrp->GetStatus();
            COMPLETE_IRP( pCurIrp, iRet );
        }

        ret = ReRun();
        if( SUCCEEDED( ret ) )
            ret = STATUS_PENDING;

    }while( 0 );

    return ret;
}

gint32 CIfStmReadWriteTask::OnIrpComplete(
    IRP* pIrp )
{
    gint32 ret = 0;

    if( pIrp->IsCbOnly() )
        ret = OnWorkerIrpComplete( pIrp );
    else
        ret = OnIoIrpComplete( pIrp );

    return ret;
}

gint32 CIfStmReadWriteTask::ReadStreamInternal(
    bool bMsg, BufPtr& pBuf,
    guint32 dwTimeoutSec )
{
    gint32 ret = 0;
    IrpPtr pIrp( true );

    do{
        CStdRTMutex oTaskLock( GetLock() );
        EnumIfState dwState = GetTaskState();
        if( dwState == stateStopped )
        {
            ret = ERROR_STATE;
            break;
        }

        if( m_bDiscard )
        {
            ret = ERROR_STATE;
            break;
        }

        if( bMsg && dwTimeoutSec == ( guint32 )-1 )
        {
            if( !m_queBufRead.empty() &&
                m_queRequests.empty() )
            {
                bool bFull = ( m_queBufRead.size() >=
                    SYNC_STM_MAX_PENDING_PKTS );

                pBuf = m_queBufRead.front();
                m_queBufRead.pop_front();
                ret = 0;

                bool bFree = ( m_queBufRead.size() < 
                    SYNC_STM_MAX_PENDING_PKTS );

                if( bFull && bFree )
                {
                    // wakeup to fill the m_queBufRead
                    // queue
                    POST_LOOP_EVENT( m_hChannel,
                        stmevtLifted, ret );
                    Resume();
                }
            }
            else if( m_queBufRead.empty() )
            {
                ret = -EAGAIN;
            }
            else
            {
                ret = ERROR_STATE;
            }
            return ret;
        }

        pIrp->AllocNextStack( nullptr );
        IrpCtxPtr& pIrpCtx = pIrp->GetTopStack(); 

        // stop the child port
        pIrpCtx->SetMajorCmd( IRP_MJ_FUNC );
        pIrpCtx->SetMinorCmd( IRP_MN_IOCTL );
        pIrpCtx->SetIoDirection( IRP_DIR_IN );
        if( bMsg )
        {
            pIrpCtx->SetCtrlCode(
                CTRLCODE_READMSG );
        }
        else
        {
            if( pBuf.IsEmpty() || pBuf->empty() )
            {
                ret = -EINVAL;
                break;
            }
            pIrpCtx->SetCtrlCode(
                CTRLCODE_READBLK );
            pIrpCtx->m_pRespData = pBuf;
        }

        // this irp is for callback only
        pIrp->SetCbOnly( true );

        // send out notification on irp
        // completion.
        pIrp->MarkPending();
        pIrp->SetCallback(
            this, ( intptr_t )this );

        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );

        CIoManager* pMgr = nullptr;
        ret = GET_IOMGR( oCfg, pMgr );
        if( SUCCEEDED( ret ) && dwTimeoutSec != 0 )
        {
            pIrp->SetTimer(
                dwTimeoutSec, pMgr );
        }
        
        BufPtr pTempBuf;
        ret = CompleteReadIrp(
            pIrp, pTempBuf );

        if( ret == STATUS_PENDING )
            break;

        pIrp->RemoveTimer();
        if( SUCCEEDED( ret ) )
            pBuf = pTempBuf;

    }while( 0 );

    if( ret == STATUS_PENDING )
    {
        pIrp->WaitForComplete();
        ret = pIrp->GetStatus();
        if( SUCCEEDED( ret ) )
        {
            IrpCtxPtr& pCtx =
                pIrp->GetTopStack();
            pBuf = pCtx->m_pRespData;
        }
    }
    else if( ERROR( ret ) )
    {
       pIrp->RemoveTimer();
    }

    return ret;
}

gint32 CIfStmReadWriteTask::SetRdRespForIrp(
    PIRP pIrp, BufPtr& pBuf )
{
    if( pIrp == nullptr || pBuf.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;
    do{

        if( pIrp->GetStackSize() == 0 )
        {
            ret = -EINVAL;
            break;
        }

        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        if( pCtx->GetMajorCmd() != IRP_MJ_FUNC &&
            pCtx->GetMinorCmd() != IRP_MN_IOCTL )
        {
            ret = -EINVAL;
            break;
        }

        guint32 dwCtrlCode = pCtx->GetCtrlCode();
        if( dwCtrlCode == CTRLCODE_READMSG )
        {
            pCtx->SetRespData( pBuf );
            pCtx->SetStatus( 0 );
            pIrp->SetStatus( 0 );
        }
        else if( dwCtrlCode == CTRLCODE_READBLK )
        {
            if( pCtx->m_pRespData.IsEmpty() )
            {
                ret = -EINVAL;
                break;
            }

            BufPtr pDest = pCtx->m_pRespData;
            guint32 dwBytes = std::min(
                pDest->size(), pBuf->size() );

            memcpy( pDest->ptr(),
                pBuf->ptr(), dwBytes );

            pDest->SetOffset(
                pDest->offset() + dwBytes );

            if( pDest->size() > 0 )
            {
                ret = STATUS_PENDING;
                break;
            }

            // the request is done
            pDest->SetOffset( 0 );
            if( dwBytes < pBuf->size() )
            {
                pBuf->SetOffset(
                    pBuf->offset() + dwBytes );

                // more process for the pBuf
                ret = STATUS_MORE_PROCESS_NEEDED;
            }
            else
            {
                ret = 0;
            }
        }

        // no need of the callback
        pIrp->RemoveCallback();

    }while( 0 );

    return ret;
}

gint32 CIfStmReadWriteTask::CompleteReadIrp(
    IrpPtr& pIrp, BufPtr& pBuf )
{
    gint32 ret = 0;

    if( ( pIrp.IsEmpty() && pBuf.IsEmpty() ) ||
        ( !pIrp.IsEmpty() && !pBuf.IsEmpty() ) )
        return -EINVAL;

    bool bNewReq = true;
    if( pIrp.IsEmpty() )
        bNewReq = false;

    do{
        if( m_queRequests.size() &&
            m_queBufRead.size() )
        {
            ret = ERROR_STATE;
            break;
        }

        bool bNoReq = m_queRequests.empty();
        bool bNoData = m_queBufRead.empty();

        if( bNoData && bNewReq )
        {
            m_queRequests.push_back( pIrp );
            ret = STATUS_PENDING;
            break;
        }
        else if( bNoReq && !bNewReq )
        {
            m_queBufRead.push_back( pBuf );
            ret = STATUS_PENDING;
            break;
        }

        if( bNewReq )
            pBuf = m_queBufRead.front();
        else
            pIrp = m_queRequests.front();

        CStdRMutex oIrpLock( pIrp->GetLock() );
        if( pIrp->GetState() != IRP_STATE_READY )
        {
            ret = ERROR_STATE;
            break;
        }

        ret = SetRdRespForIrp( pIrp, pBuf );

        IrpCtxPtr& pIrpCtx = pIrp->GetTopStack();
        if( ret == 0 )
        {
            if( bNewReq )
                m_queBufRead.pop_front();
            else
                m_queRequests.pop_front(); 

            pIrp->SetStatus( ret );
            pIrpCtx->SetStatus( ret );

            if( !bNewReq )
                break;

            bool bWithinLimit = (
                m_queBufRead.size() < 
                SYNC_STM_MAX_PENDING_PKTS );

            if( bWithinLimit && IsPaused() )
            {
                // wakeup to fill the m_queBufRead
                // queue
                DebugMsg( m_queBufRead.size(),
                    "Reading resumed..." );
                Resume();
            }
        }
        else if( ret == STATUS_PENDING )
        {
            // waiting in the queue
            if( bNewReq )
            {
                m_queRequests.push_back(
                    IrpPtr( pIrp ) );
            }
        }
        else if( ret == STATUS_MORE_PROCESS_NEEDED )
        {
            // leave the pBuf where it is,
            // that is, if the pbuf is in the
            // queue, stay there. if not,
            // still not
            if( !bNewReq )
                m_queRequests.pop_front(); 

            // the irp is done
            pIrp->SetStatus( 0 );
            pIrpCtx->SetStatus( 0 );
            if( bNewReq )
                ret = 0;
        }
        else if( ERROR( ret ) )
        {
            pIrp->SetStatus( ret );
            pIrpCtx->SetStatus( ret );
        }

    }while( 0 );

    return ret;
}

gint32 CIfStmReadWriteTask::OnCancel(
    guint32 dwContext )
{
    gint32 ret = -ECANCELED;
    do{
        super::OnCancel( dwContext );

        std::deque< IrpPtr > queIrps;
        queIrps = m_queRequests;
        m_queRequests.clear();

        CIoManager* pMgr = nullptr;
        CParamList oCfg(
            ( IConfigDb* )GetConfig() );

        ret = GET_IOMGR( oCfg, pMgr );
        if( ERROR( ret ) )
            break;

        for( auto elem : queIrps )
        {
            // manually complete the irp
            // Cannot use CompleteIrp here,
            // because the task will be stopped
            // before the OnIrpComplete is able to
            // be called.
            COMPLETE_IRP( elem, -ECANCELED );
        }

    }while( 0 ); 

    return ret;
}

gint32 CIfStmReadWriteTask::RunTask()
{
    if( IsReading() )
        return STATUS_PENDING;

    gint32 ret = 0;

    do{
        if( m_queRequests.empty() )
        {
            Pause();
            ret = STATUS_PENDING;
            break;
        }

        IrpPtr pIrp = m_queRequests.front();
        CStdRMutex oIrpLock( pIrp->GetLock() );
        if( pIrp->GetState() != IRP_STATE_READY )
        {
            m_queRequests.pop_front();
            continue;
        }
        if( pIrp->IsSubmited() )
        {
            // the irp is still in processing
            DebugPrint( 0, "writing unfinished" );
            ret = STATUS_PENDING;
            break;
        }

        IrpCtxPtr& pIrpCtx = pIrp->GetTopStack();

        guint32 dwMajorCmd =
            pIrpCtx->GetMajorCmd();

        guint32 dwMinorCmd =
            pIrpCtx->GetMinorCmd();

        if( dwMajorCmd != IRP_MJ_FUNC ||
            dwMinorCmd != IRP_MN_IOCTL )
        {
            m_queRequests.pop_front();
            COMPLETE_IRP( pIrp, -EINVAL );
            continue;
        }
        guint32 dwCtrlCode =
            pIrpCtx->GetCtrlCode();

        if( dwCtrlCode != CTRLCODE_WRITEBLK &&
            dwCtrlCode != CTRLCODE_WRITEMSG )
        {
            m_queRequests.pop_front();
            COMPLETE_IRP( pIrp, -EINVAL );
            continue;
        }

        BufPtr pBuf = pIrpCtx->m_pReqData;

        IStream* pStm = nullptr;
        GET_STMPTR2( pStm, ret );
        if( ERROR( ret ) )
            break;

        if( dwCtrlCode == CTRLCODE_WRITEMSG )
        {
            pIrp->SetSubmited();
            ret = pStm->WriteStream(
                m_hChannel, pBuf, this );

            if( ret == STATUS_PENDING )
                break;

            if( ret == ERROR_QUEUE_FULL )
            {
                // clear the flag to re-visit the
                // irp later.
                pIrp->ClearSubmited();
                Pause();
                ret = STATUS_PENDING;
                break;
            }

            if( ERROR( ret ) )
                break;

            m_queRequests.pop_front();
            COMPLETE_IRP( pIrp, ret );
            // move on to the next
        }
        else
        {
            BufPtr pTempBuf;
            guint32 dwSize = pBuf->size();
            if( dwSize > STM_MAX_BYTES_PER_BUF )
            {
                dwSize = STM_MAX_BYTES_PER_BUF;
                ret = pTempBuf.NewObj();
                if( ERROR( ret ) )
                    break;

                pTempBuf->Resize( dwSize );
                memcpy( pTempBuf->ptr(),
                    pBuf->ptr(), dwSize );

            }
            else
            {
                pTempBuf = pBuf;
            }

            pIrp->SetSubmited();
            ret = pStm->WriteStream(
                m_hChannel, pTempBuf, this );

            if( ret == STATUS_PENDING )
                break;

            if( ret == ERROR_QUEUE_FULL )
            {
                pIrp->ClearSubmited();
                Pause();
                ret = STATUS_PENDING;
                break;
            }

            if( ERROR( ret ) )
                break;

            bool bDone = false;
            if( pTempBuf == pBuf ||
                dwSize == pBuf->size() )
                bDone = true;

            if( bDone )
            {
                m_queRequests.pop_front();
                pBuf->SetOffset( 0 );
                COMPLETE_IRP( pIrp, ret );
            }
            else
            {
                pIrp->ClearSubmited();
                pBuf->SetOffset(
                    pBuf->offset() + dwSize );
            }
        }

    }while( 1 );

    if( ERROR( ret ) )
        return ret;

    return ret;
}

gint32 CIfStmReadWriteTask::WriteStreamInternal(
    bool bMsg, BufPtr& pBuf,
    guint32 dwTimeoutSec )
{
    if( pBuf.IsEmpty() || pBuf->empty() )
        return -EINVAL;
    
    gint32 ret = 0;
    IrpPtr pIrp( true );

    bool bNoWait = false;
    if( dwTimeoutSec == ( guint32 )-1 )
        bNoWait = true;

    do{
        CStdRTMutex oTaskLock( GetLock() );
        EnumIfState dwState = GetTaskState();
        if( dwState == stateStopped )
        {
            ret = ERROR_STATE;
            break;
        }

        if( IsLoopStarted() )
        {
            // NOTE: check if a flow-control from
            // the uxstream is going on. The
            // transfer will get interrupted
            // without this check, for the reason
            // large amount of incoming data
            // packets blocks the entrance for
            // progress report packet.
            if( !CanSend()  )
            {
                // uxstream's flow control
                ret = ERROR_QUEUE_FULL;
                break;
            }
        }

        if( m_queRequests.size() >=
            SYNC_STM_MAX_PENDING_PKTS )
        {
            ret = ERROR_QUEUE_FULL;
            break;
        }

        pIrp->AllocNextStack( nullptr );
        IrpCtxPtr& pIrpCtx = pIrp->GetTopStack(); 

        // stop the child port
        pIrpCtx->SetMajorCmd( IRP_MJ_FUNC );
        pIrpCtx->SetMinorCmd( IRP_MN_IOCTL );
        pIrpCtx->SetIoDirection( IRP_DIR_OUT );
        pIrpCtx->m_pReqData = pBuf;
        if( bMsg )
        {
            if( pBuf->size() >
                STM_MAX_BYTES_PER_BUF )
            {
                ret = -EINVAL;
                break;
            }
            pIrpCtx->SetCtrlCode(
                CTRLCODE_WRITEMSG );
        }
        else
        {
            if( pBuf.IsEmpty() || pBuf->empty() )
            {
                ret = -EINVAL;
                break;
            }
            pIrpCtx->SetCtrlCode(
                CTRLCODE_WRITEBLK );
        }

        CIoManager* pMgr = nullptr;
        CParamList oCfg(
            ( IConfigDb* )GetConfig() );

        ret = GET_IOMGR( oCfg, pMgr );
        if( SUCCEEDED( ret ) &&
            dwTimeoutSec != 0 )
        {
            // for dwTimeoutSec == -1, the
            // notification will be sent to the
            // handler OnSendDone
            if( dwTimeoutSec == ( guint32 )-1 )
                pMgr = nullptr;

            pIrp->SetTimer(
                dwTimeoutSec, pMgr );
        }

        pIrp->SetTimer(
            dwTimeoutSec, nullptr );

        // this irp is for callback only
        pIrp->SetCbOnly( true );

        // send out notification on irp
        // completion.
        pIrp->MarkPending();
        pIrp->SetCallback(
            this, ( intptr_t )this );

        if( m_queRequests.empty() )
        {
            m_queRequests.push_back( pIrp );
            if( IsPaused() )
                Resume();
        }
        else
        {
            m_queRequests.push_back( pIrp );
        }

        ret = STATUS_PENDING;

    }while( 0 );

    if( ret == STATUS_PENDING && !bNoWait )
    {
        pIrp->WaitForComplete();
        ret = pIrp->GetStatus();
    }

    return ret;
}

gint32 CIfStmReadWriteTask::OnStmRecv(
    BufPtr& pBuf )
{
    if( pBuf.IsEmpty() || pBuf->empty() )
        return -EINVAL;

    if( !IsReading() )
        return -EINVAL;

    if( m_bDiscard )
        return 0;

    gint32 ret = 0;
    do{
        CStdRTMutex oTaskLock( GetLock() );
        if( m_queRequests.empty() )
        {
            m_queBufRead.push_back( pBuf );
            if( m_queBufRead.size() >=
                SYNC_STM_MAX_PENDING_PKTS )
            {
                Pause();
            }
            else if( m_queBufRead.size() == 1 )
            {
                ret = STATUS_MORE_PROCESS_NEEDED;
            }
            break;
        }

        // assuming the incoming packet
        // must not span across two irps
        gint32 bContinue = true;
        std::vector< IrpPtr > vecIrps;
        IrpPtr pIrp;
        while( bContinue )
        {
            ret = CompleteReadIrp(
                pIrp, pBuf );

            // NOTE: this
            // STATUS_MORE_PROCESS_NEEDED cannot
            // be returned as OnStmRecv's return
            // value
            if( ret !=
                STATUS_MORE_PROCESS_NEEDED )
                bContinue = false;

            if( ret == STATUS_PENDING )
                break;

            vecIrps.push_back( pIrp );
            pIrp.Clear();
        }

        if( ret != STATUS_PENDING )
        {
            // the buffer needs further
            // processing
            oTaskLock.Unlock();

            for( auto elem : vecIrps )
            {
                gint32 iRet = elem->GetStatus();
                COMPLETE_IRP( elem, iRet );
            }
        }

    }while( 0 );

    return ret;
}

gint32 CReadWriteWatchTask::operator()(
    guint32 dwContext )
{
    gint32 ret = 0;
    do{
        std::vector< LONGWORD > vecParams;
        ret = GetParamList( vecParams );
        if( ERROR( ret ) )
        {
            ret = G_SOURCE_REMOVE;
            break;
        }

        dwContext = vecParams[ 1 ];
        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );

        CRpcServices* pStream = nullptr;

        ret = oCfg.GetPointer(
            propIfPtr, pStream );

        if( ERROR( ret ) )
        {
            ret = G_SOURCE_REMOVE;
            break;
        }

        GIOCondition dwCond =
            ( GIOCondition )dwContext;

        ret = pStream->OnEvent(
            eventIoWatch, dwCond, 0, 0 );

        if( ERROR( ret ) )
            ret = G_SOURCE_REMOVE;
        else
            ret = G_SOURCE_CONTINUE;

    }while( 0 );

    return SetError( ret );
}
