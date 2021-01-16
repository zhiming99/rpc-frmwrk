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

namespace rpcfrmwrk
{

#define HAS_CALLBACK( _pIrp, _pCallback, _pBuf, _iRet ) \
({ \
    bool _ret = false; \
    do{ \
        if( _pIrp.IsEmpty() ) break;\
        BufPtr pExtBuf; \
        IrpCtxPtr pCtx; \
        if( _pIrp->GetStackSize() == 0 ) \
            break; \
        pCtx = _pIrp->GetTopStack(); \
        pCtx->GetExtBuf( pExtBuf ); \
        if( pExtBuf.IsEmpty() || \
            pExtBuf->empty() ) \
            break; \
        IRPCTX_EXT* pCtxExt = \
            ( IRPCTX_EXT* )pExtBuf->ptr(); \
        if( pCtxExt->pCallback.IsEmpty() ) \
            break; \
        _ret = true; \
        _pCallback = pCtxExt->pCallback; \
        gint32 iRet =  pCtx->GetStatus(); \
        if( ERROR( iRet ) ) \
            break; \
        guint32 dwDir = pCtx->GetIoDirection();\
        if( dwDir == IRP_DIR_IN ) {\
            _pBuf = pCtx->m_pRespData; \
        } \
        else {\
            _pBuf = pCtx->m_pReqData; \
        } \
    }while( 0 ); \
    _ret; \
})

#define INVOKE_CALLBACK( _pIrp_ ) \
({bool bRet = false;\
do{ \
    ObjPtr pObj; \
    BufPtr pBuf; \
    gint32 iRet = 0; \
    bool bHasCb = HAS_CALLBACK( \
        _pIrp_, pObj, pBuf, iRet ); \
    if( !bHasCb ) \
        break; \
    TaskletPtr pTask = pObj; \
    if( pTask.IsEmpty() ) \
        break; \
    CParamList oResp; \
    oResp[ propReturnValue ] = iRet; \
    if( SUCCEEDED( ret ) ) \
        oResp.Push( pBuf ); \
    TaskletPtr pDummy; \
    pDummy.NewObj( clsid( CIfDummyTask ) ); \
    CCfgOpener oCfg(\
        ( IConfigDb* )pDummy->GetConfig() ); \
    oCfg.SetPointer( propRespPtr, \
        ( IConfigDb* )oResp.GetCfg() );\
    LONGWORD* pData = \
        ( LONGWORD* )( ( CObjBase* )pDummy ); \
    pTask->OnEvent( \
        eventTaskComp, iRet, 0, pData ); \
    CLEAR_CALLBACK( _pIrp_, false );\
    bRet = true;\
}while( 0 ); \
bRet; \
})

#define CLEAR_CALLBACK( _pIrp, _bCancel ) \
({ \
    do{ \
        if( _pIrp.IsEmpty() ) break;\
        BufPtr pExtBuf; \
        IrpCtxPtr pCtx; \
        if( _pIrp->GetStackSize() == 0 ) \
            break; \
        pCtx = _pIrp->GetTopStack(); \
        pCtx->GetExtBuf( pExtBuf ); \
        if( pExtBuf.IsEmpty() || \
            pExtBuf->empty() ) \
            break; \
        IRPCTX_EXT* pCtxExt = \
            ( IRPCTX_EXT* )pExtBuf->ptr(); \
        if( pCtxExt->pCallback.IsEmpty() ) \
            break; \
        if( _bCancel ) {\
            TaskletPtr pCallback = pCtxExt->pCallback;\
            ( *pCallback )( eventCancelTask );\
        }\
        pCtxExt->pCallback.Clear(); \
    }while( 0 ); \
})

#define COMPLETE_SEND_IRP( _pIrp, _ret ) \
do{ \
    gint32 _iRet = ( gint32 )( _ret ); \
    CStdRMutex oIrpLock( _pIrp->GetLock() ); \
    if( _pIrp->GetState() == IRP_STATE_READY ) \
    { \
        _pIrp->RemoveTimer(); \
        _pIrp->RemoveCallback(); \
        _pIrp->SetStatus( ( _iRet ) ); \
        _pIrp->SetState( IRP_STATE_READY, \
            IRP_STATE_COMPLETED ); \
        if( INVOKE_CALLBACK( _pIrp ) ) break;\
        Sem_Post( &_pIrp->m_semWait ); \
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
        _pIrp->SetState( IRP_STATE_READY, \
            IRP_STATE_COMPLETED ); \
        if( INVOKE_CALLBACK( _pIrp ) ) break;\
        Sem_Post( &_pIrp->m_semWait ); \
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
    CCfgOpener _oParams( \
        ( const IConfigDb* )GetConfig() ); \
    CRpcServices* _pIf; \
    _ret = _oParams.GetPointer( \
        propIfPtr, _pIf ); \
    if( ERROR( _ret ) ) \
        break; \
    GET_STMPTR( _pIf, _pStm ); \
    if( _pStm == nullptr ) \
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
        InterfPtr pUxIf;
        ret = GetUxStream( pUxIf );
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
    CStdRTMutex oTaskLock( GetLock() );

    if( GetReqCount() > 0 )
        Resume();

    gint32 ret = 0;
    if( IsLoopStarted() )
    {
        POST_LOOP_EVENT( m_hChannel, 
            stmevtLifted, ret );
    }
    else
    {
        CRpcServices* pSvc = nullptr;
        GET_STMPTR2( pSvc, ret );
        if( ERROR( ret ) )
            return ret;

        HANDLE hChannel = m_hChannel;
        oTaskLock.Unlock();

        if( pSvc->IsServer() )
        {
            CStreamServerSync* pIf =
                ObjPtr( pSvc );
            pIf->OnWriteResumed( hChannel );
        }
        else
        {
            CStreamProxySync* pIf =
                ObjPtr( pSvc );
            pIf->OnWriteResumed( hChannel );
        }
    }
    return 0;
}

gint32 CIfStmReadWriteTask::PauseReading(
    bool bPause )
{
    gint32 ret = 0;
    do{
        InterfPtr pUxIf;
        ret = GetUxStream( pUxIf );
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
    bool bHead = false;
    std::deque< IrpPtr >::iterator
        itr = m_queRequests.begin();

    for( ; itr != m_queRequests.end(); ++itr )
    {
        if( *itr == IrpPtr( pIrp ) )
        {
            if( itr == m_queRequests.begin() )
                bHead = true;
            itr = m_queRequests.erase( itr );
            bFound = true;
            break;
        }
    }

    if( bFound )
    {
        pIrp->RemoveCallback();
        pIrp->RemoveTimer();
        Sem_Post( &pIrp->m_semWait ); \
    }

    if( !bHead )
        return STATUS_PENDING;

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

        BufPtr pBuf = pCurCtx->m_pReqData;
        if( pBuf.IsEmpty() ||
            pBuf->empty() )
        {
            ret = -EINVAL;
            break;
        }

        if( pBuf->size() < dwSize )
        {
            ret = -ERANGE;
            break;
        }

        pBuf->SetOffset(
            pBuf->offset() + dwSize );

        // recover the buffer
        BufPtr pExtBuf;
        pCurCtx->GetExtBuf( pExtBuf );
        IRPCTX_EXT* pExt =
            ( IRPCTX_EXT* )pExtBuf->ptr();

        bool bDone = false;
        if( pExt->dwTailOff ==
            pBuf->GetTailOff() )
                bDone = true;

        gint32 iRet = 0;
        if( bDone )
        {
            pBuf->SetOffset( pExt->dwOffset );
            pBuf->SetTailOff( pExt->dwTailOff );
            m_queRequests.pop_front();
            iRet = pCurIrp->GetStatus();
            COMPLETE_IRP( pCurIrp, iRet );
        }
        else
        {
            pCurIrp->ClearSubmited();
            iRet = AdjustSizeToWrite(
                pCurIrp, pBuf, true );

            if( ERROR( iRet ) )
            {
                if( iRet == -ENODATA )
                {
                    // the irp is done
                    iRet = 0;
                }
                pBuf->SetOffset( pExt->dwOffset );
                pBuf->SetTailOff( pExt->dwTailOff );
                m_queRequests.pop_front();
                COMPLETE_IRP( pCurIrp, iRet );
            }
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

gint32 CIfStmReadWriteTask::GetUxStream(
    InterfPtr& pIf ) const
{
    gint32 ret = 0;
    CStdRTMutex oTaskLock( GetLock() );
    IStream* pStm = nullptr;
    GET_STMPTR2( pStm, ret );
    if( ERROR( ret ) )
        return ret;

    HANDLE hChannel = m_hChannel;
    oTaskLock.Unlock();
    ret = pStm->GetUxStream( hChannel, pIf );
    if( ERROR( ret ) )
        return ret;

    return STATUS_SUCCESS;
}

gint32 CIfStmReadWriteTask::PeekStream(
    BufPtr& pBuf )
{
    gint32 ret = 0;

    if( !IsReading() )
        return -EINVAL;

    if( m_bDiscard )
        return ERROR_STATE;

    do{
        CStdRTMutex oTaskLock( GetLock() );
        EnumIfState dwState = GetTaskState();
        if( dwState == stateStopped )
        {
            ret = ERROR_STATE;
            break;
        }

        if( !m_queBufRead.empty() &&
            m_queRequests.empty() )
        {
            pBuf = m_queBufRead.front();
            ret = 0;
        }
        else if( m_queBufRead.empty() )
        {
            ret = -EAGAIN;
            break;
        }
        else
        {
            ret = ERROR_STATE;
            break;
        }

        bool bReport = IsReport( pBuf );
        if( !bReport )
            break;

        m_queBufRead.pop_front();
        oTaskLock.Unlock();

        SendProgress( pBuf );

    }while( 1 );

    return ret;
}

gint32 CIfStmReadWriteTask::ReadStreamInternal(
    IEventSink* pCallback,
    BufPtr& pBuf, bool bNoWait )
{
    gint32 ret = 0;
    IrpPtr pIrp( true );

    do{
        bool bMsg =
        ( pBuf.IsEmpty() || pBuf->empty() );

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

        if( !m_queBufRead.empty() )
        {
            BufPtr pRptBuf = m_queBufRead.front();
            if( IsReport( pRptBuf ) )
            {
                m_queBufRead.pop_front();
                oTaskLock.Unlock();
                SendProgress( pRptBuf );
                continue;
            }
            else if( m_queBufRead.size() >= 2 )
            {
                // make sure the progress packet
                // to be sent the same time the
                // packet ahead of it is out of
                // queue.
                BufPtr pRptBuf = m_queBufRead[ 1 ];
                if( IsReport( pRptBuf ) )
                {
                    m_queBufRead.erase(
                        m_queBufRead.begin() + 1 );
                    oTaskLock.Unlock();
                    SendProgress( pRptBuf );
                }
            }
        }

        if( bMsg && bNoWait )
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
                    // wakeup to resume receiving mesages
                    //
                    //POST_LOOP_EVENT( m_hChannel,
                    //    stmevtLifted, ret );
                    //
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
        else if( bNoWait )
        {
            ret = -EINVAL;
            break;
        }

        pIrp->AllocNextStack( nullptr );
        IrpCtxPtr& pIrpCtx = pIrp->GetTopStack(); 

        if( pCallback != nullptr )
        {
            IRPCTX_EXT oCtxExt;
            oCtxExt.pCallback = ObjPtr( pCallback );

            BufPtr pExtBuf( true );
            pExtBuf->Resize( sizeof( IRPCTX_EXT ) );
            new ( pExtBuf->ptr() )
                IRPCTX_EXT( oCtxExt );

            pIrpCtx->SetExtBuf( pExtBuf );
        }

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

        // this callback is for timer
        pIrp->SetCallback(
            this, ( intptr_t )this );

        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );

        CIoManager* pMgr = nullptr;
        ret = GET_IOMGR( oCfg, pMgr );
        if( ERROR( ret ) )
            break;

        guint32 dwTimeoutSec = 0;
        ret = oCfg.GetIntProp(
            propTimeoutSec, dwTimeoutSec );

        if( ERROR( ret ) )
            break;

        pIrp->SetTimer( dwTimeoutSec, pMgr );
        
        BufPtr pTempBuf;
        ret = CompleteReadIrp( pIrp, pTempBuf );
        if( ret == STATUS_PENDING )
            break;

        pIrp->RemoveTimer();
        if( SUCCEEDED( ret ) )
            pBuf = pTempBuf;

        break;

    }while( 1 );

    if( ret == STATUS_PENDING &&
        pCallback != nullptr )
        return ret;

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

    CLEAR_CALLBACK( pIrp, true );

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

            pDest->IncOffset( dwBytes );
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
                m_queBufRead.pop_front();
                if( !m_queBufRead.empty() )
                    continue;
                m_queRequests.push_back(
                    IrpPtr( pIrp ) );
            }
        }
        else if( ret == STATUS_MORE_PROCESS_NEEDED )
        {
            // leave the pBuf where it is, that
            // is, if the pbuf is in the queue,
            // stay there. if not, still not
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

        break;

    }while( 1 );

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

        guint32 dwCtrlCode =
            pIrpCtx->GetCtrlCode();

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
            AdjustSizeToWrite( pIrp, pBuf );
            BufPtr pExtBuf;
            pIrpCtx->GetExtBuf( pExtBuf );
            IRPCTX_EXT* pExt =
                ( IRPCTX_EXT* )pExtBuf->ptr();

            BufPtr& pTempBuf = pBuf;
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
            {
                pBuf->SetOffset( pExt->dwOffset );
                pBuf->SetTailOff( pExt->dwTailOff );
                m_queRequests.pop_front();
                COMPLETE_IRP( pIrp, ret );
                continue;
            }

            bool bDone = false;
            if( pExt->dwTailOff ==
                pBuf->GetTailOff() )
                    bDone = true;

            if( bDone )
            {
                pBuf->SetOffset( pExt->dwOffset );
                pBuf->SetTailOff( pExt->dwTailOff );
                m_queRequests.pop_front();
                COMPLETE_IRP( pIrp, ret );
                continue;
            }
            else
            {
                pIrp->ClearSubmited();
                ret = AdjustSizeToWrite(
                    pIrp, pBuf, true );

                if( SUCCEEDED( ret ) )
                    continue;

                if( ret == -ENODATA )
                    ret = 0;
                pBuf->SetOffset( pExt->dwOffset );
                pBuf->SetTailOff( pExt->dwTailOff );
                m_queRequests.pop_front();
                COMPLETE_IRP( pIrp, ret );
            }
        }

    }while( 1 );

    if( ERROR( ret ) )
        return ret;

    return ret;
}

gint32 CIfStmReadWriteTask::AdjustSizeToWrite(
    IrpPtr& pIrp, BufPtr& pBuf, bool bSlide )
{
    if( pBuf.IsEmpty() || pBuf->empty() )
        return -EINVAL;

    gint32 ret = 0;
    do{
        guint32 dwSize =
            STM_MAX_BYTES_PER_BUF;

        if( bSlide )
        {
            if( pBuf->GetShadowSize() == 0 )
                ret = -ENODATA;
            else
            {
                IrpCtxPtr& pCtx =
                    pIrp->GetTopStack();
                BufPtr pExt;
                pCtx->GetExtBuf( pExt );
                if( unlikely ( pExt.IsEmpty() ||
                    pExt->empty() ) )
                {
                    ret = pBuf->
                        SlideWindow( dwSize );
                    break;
                }

                IRPCTX_EXT* pCtxExt =
                    ( IRPCTX_EXT* )pExt->ptr();

                guint32 dwOrigTail =
                    pCtxExt->dwTailOff;

                guint32 dwTailOff =
                    pBuf->GetTailOff();

                if( dwSize <=
                    dwOrigTail - dwTailOff )
                {
                    pBuf->SlideWindow( dwSize );
                }
                else if( dwOrigTail > dwTailOff )
                {
                    dwSize = dwOrigTail - dwTailOff;
                    pBuf->SlideWindow( dwSize );
                }
                else if( dwOrigTail == dwTailOff )
                {
                    ret = -ENODATA;
                    break;
                }
                else if( dwOrigTail < dwTailOff )
                {
                    ret = -ERANGE;
                    break;
                }
            }
            break;
        }

        if( pBuf->size() > dwSize )
            pBuf->SetWinSize( dwSize );

    }while( 0 );

    return ret;
}

gint32 CIfStmReadWriteTask::WriteStreamInternal(
    IEventSink* pCallback,
    BufPtr& pBuf, bool bNoWait )
{
    if( pBuf.IsEmpty() || pBuf->empty() )
        return -EINVAL;

    if( bNoWait && pCallback != nullptr )
        return -EINVAL;
    
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

        if( m_queRequests.size() >=
            STM_MAX_PACKETS_REPORT )
        {
            ret = ERROR_QUEUE_FULL;
            break;
        }

        pIrp->AllocNextStack( nullptr );
        IrpCtxPtr& pIrpCtx = pIrp->GetTopStack(); 

        pIrpCtx->SetMajorCmd( IRP_MJ_FUNC );
        pIrpCtx->SetMinorCmd( IRP_MN_IOCTL );
        pIrpCtx->SetIoDirection( IRP_DIR_OUT );
        pIrpCtx->m_pReqData = pBuf;

        IRPCTX_EXT oCtxExt;
        oCtxExt.dwOffset = pBuf->offset();
        oCtxExt.dwTailOff = pBuf->GetTailOff();

        if( pCallback != nullptr )
            oCtxExt.pCallback = pCallback;

        BufPtr pExtBuf( true );
        pExtBuf->Resize( sizeof( IRPCTX_EXT ) );

        new ( pExtBuf->ptr() )
            IRPCTX_EXT( oCtxExt );

        pIrpCtx->SetExtBuf( pExtBuf );

        if( bNoWait && ( pBuf->size() >
            STM_MAX_BYTES_PER_BUF ) )
        {
            ret = -EINVAL;
            break;
        }

        if( bNoWait && ( pBuf->size() <= 
            STM_MAX_BYTES_PER_BUF ) )
        {
            pIrpCtx->SetCtrlCode(
                CTRLCODE_WRITEMSG );
        }
        else
        {
            pIrpCtx->SetCtrlCode(
                CTRLCODE_WRITEBLK );
        }

        CIoManager* pMgr = nullptr;
        CParamList oCfg(
            ( IConfigDb* )GetConfig() );

        ret = GET_IOMGR( oCfg, pMgr );
        if( ERROR( ret ) )
            break;

        guint32 dwTimeoutSec = 0;
        ret = oCfg.GetIntProp(
            propTimeoutSec, dwTimeoutSec );

        if( ERROR( ret ) )
            break;

        if( bNoWait )
            pIrp->SetTimer( -1, nullptr );
        else
            pIrp->SetTimer( dwTimeoutSec, pMgr );

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

    if( ret == STATUS_PENDING && bNoWait )
        return ret;
 
    if( ret == STATUS_PENDING &&
        pCallback == nullptr )
    {
        pIrp->WaitForComplete();
        ret = pIrp->GetStatus();
    }
    else if( ret == STATUS_PENDING )
        return ret;

    CLEAR_CALLBACK( pIrp, true );

    return ret;
}

gint32 CIfStmReadWriteTask::ReschedRead()
{
    if( !IsReading() )
        return -EINVAL;

    if( m_bDiscard )
        return 0;

    gint32 ret = 0;
    do{
        CStdRTMutex oTaskLock( GetLock() );
        if( m_queRequests.empty() ||
            m_queBufRead.empty() )
            break;

        BufPtr pBuf = m_queBufRead.front();
        m_queBufRead.pop_front();
        // assuming the incoming packet
        // must not span across two irps
        gint32 bContinue = true;
        std::vector< IrpPtr > vecIrps;
        IrpPtr pIrp;
        while( bContinue )
        {
            ret = CompleteReadIrp( pIrp, pBuf );

            // NOTE: this
            // STATUS_MORE_PROCESS_NEEDED cannot
            // be as OnStmRecv's return value
            if( ret !=
                STATUS_MORE_PROCESS_NEEDED )
                bContinue = false;

            if( ret == STATUS_PENDING )
                break;

            vecIrps.push_back( pIrp );
            pIrp.Clear();
        }

        // the buffer needs further
        // processing
        for( auto elem : vecIrps )
        {
            gint32 iRet = elem->GetStatus();
            COMPLETE_IRP( elem, iRet );
        }

    }while( 0 );

    return ret;
}

gint32 CIfStmReadWriteTask::SendProgress(
    BufPtr& pBuf ) const
{
    if( !IsReading() )
        return -ENOTSUP;

    if( pBuf.IsEmpty() || pBuf->empty() )
        return -EINVAL;

    gint32 ret = 0;

    do{
        InterfPtr pIf;
        ret = GetUxStream( pIf );
        if( ERROR( ret ) )
            break;

        ObjPtr& pObj = *pBuf;
        IConfigDb* pRpt = pObj;
        if( pRpt == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        BufPtr pRptBuf( true );
        ret = pRpt->Serialize( *pRptBuf );
        if( ERROR( ret ) )
            break;

        TaskletPtr pCallback;
        pCallback.NewObj(
            clsid( CIfDummyTask ) );

        if( pIf->IsServer() )
        {
            CUnixSockStmServer* pSvc = pIf;
            if( pSvc == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            pSvc->SendProgress(
                pRptBuf, pCallback );
        }
        else
        {
            CUnixSockStmProxy* pSvc = pIf;
            if( pSvc == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            pSvc->SendProgress(
                pRptBuf, pCallback );
        }

    }while( 0 );

    return ret;
}

gint32 CIfStmReadWriteTask::OnStmRecv(
    BufPtr& pBuf )
{
    if( pBuf.IsEmpty() || pBuf->empty() )
        return -EINVAL;

    if( !IsReading() )
        return -EINVAL;

    gint32 ret = 0;
    do{
        bool bReport = IsReport( pBuf );
        CStdRTMutex oTaskLock( GetLock() );
        if( bReport )
        {
            if( m_queBufRead.empty() || m_bDiscard )
            {
                oTaskLock.Unlock();
                ret = SendProgress( pBuf );
                break;
            }
            m_queBufRead.push_back( pBuf );
            break;
        }

        if( m_bDiscard )
            break;;

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
            ret = CompleteReadIrp( pIrp, pBuf );

            // NOTE: this
            // STATUS_MORE_PROCESS_NEEDED cannot
            // be as OnStmRecv's return value
            if( ret !=
                STATUS_MORE_PROCESS_NEEDED )
                bContinue = false;

            if( ret == STATUS_PENDING )
                break;

            vecIrps.push_back( pIrp );
            pIrp.Clear();
        }

        // the buffer needs further
        // processing
        for( auto elem : vecIrps )
        {
            gint32 iRet = elem->GetStatus();
            COMPLETE_IRP( elem, iRet );
        }

    }while( 0 );

    return ret;
}

gint32 CReadWriteWatchTask::operator()(
    guint32 dwContext )
{
    gint32 ret = 0;
    gint32 iRet = G_SOURCE_CONTINUE;
    do{
        std::vector< LONGWORD > vecParams;
        ret = GetParamList( vecParams );
        if( ERROR( ret ) )
        {
            iRet = G_SOURCE_REMOVE;
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
            iRet = G_SOURCE_REMOVE;
            break;
        }

        GIOCondition dwCond =
            ( GIOCondition )dwContext;

        ret = pStream->OnEvent(
            eventIoWatch, dwCond, 0, 0 );

        if( ERROR( ret ) )
            iRet = G_SOURCE_REMOVE;
        else
            iRet = G_SOURCE_CONTINUE;

    }while( 0 );

    return SetError( iRet );
}

gint32 CStreamServerSync::OnConnected(
    HANDLE hCfg )
{
    if( hCfg == INVALID_HANDLE )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CStdRMutex oIfLock( this->GetLock() );
        if( !this->IsConnected( nullptr ) )
        {
            ret = ERROR_STATE;
            break;
        }

        IConfigDb* pResp = reinterpret_cast
            < IConfigDb* >( hCfg );
        if( pResp == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        CCfgOpener oResp( pResp );
        HANDLE hChannel = ( HANDLE& )oResp[ 1 ];

        InterfPtr pIf;
        ret = GetUxStream( hChannel, pIf );
        if( ERROR( ret ) )
            break;

        guint64 qwId = pIf->GetObjId();
        guint64 qwHash = 0;
        ret = GetObjIdHash( qwId, qwHash );
        if( ERROR( ret ) )
            break;

        m_mapIdHashToHandle[ qwHash ] = hChannel;
        oIfLock.Unlock();

        ret = super::OnConnected( hCfg );
        if( ERROR( ret ) )
        {
            oIfLock.Lock();
            m_mapIdHashToHandle.erase( qwHash );
        }

    }while( 0 );

    return ret;
}

gint32 CStreamServerSync::StopWorkers(
    HANDLE hChannel )
{
    if( hChannel == INVALID_HANDLE )
        return -EINVAL;

    gint32 ret = 0;
    do{
        InterfPtr pIf;
        ret = super::StopWorkers( hChannel );
        CStdRMutex oIfLock( this->GetLock() );
        gint32 r = GetUxStream( hChannel, pIf );
        if( ERROR( r ) )
            break;

        guint64 qwId = pIf->GetObjId();
        guint64 qwHash = 0;
        ret = GetObjIdHash( qwId, qwHash );
        if( ERROR( ret ) )
            break;

        m_mapIdHashToHandle.erase( qwHash );

    }while( 0 );

    return ret;
}

HANDLE CStreamServerSync::GetChanByIdHash(
    guint64 qwIdHash ) const
{
    if( qwIdHash == 0 )
        return INVALID_HANDLE;

    CStdRMutex oIfLock( this->GetLock() );
    std::map< guint64, HANDLE >::const_iterator
        itr = m_mapIdHashToHandle.find( qwIdHash );
    if( itr == m_mapIdHashToHandle.cend() )
        return INVALID_HANDLE;

    return itr->second;
}

gint32 CStreamProxySync::StartStream(
    HANDLE& hChannel,
    IConfigDb* pDesc,
    IEventSink* pCallback )
{
    gint32 ret = 0;
    hChannel = INVALID_HANDLE;
    do{
        if( !IsConnected() )
        {
            ret = ERROR_STATE;
            break;
        }
        ret = super::StartStream(
            hChannel, pDesc, pCallback );

    }while( 0 );

    return ret;
}

gint32 CStreamProxySync::GetPeerObjId(
    HANDLE hChannel, guint64& qwPeerObjId )
{
    // a place to store channel specific data.
    if( hChannel == INVALID_HANDLE )
        return -EINVAL;

    CStdRMutex oIfLock( this->GetLock() );
    typename WORKER_MAP::iterator itr = 
        m_mapStmWorkers.find( hChannel );

    if( itr == m_mapStmWorkers.end() )
        return -ENOENT;

    IConfigDb* pCtx = itr->second.pContext;
    CCfgOpener oCtx( pCtx );
    return oCtx.GetQwordProp(
        propPeerObjId, qwPeerObjId );
}

}
