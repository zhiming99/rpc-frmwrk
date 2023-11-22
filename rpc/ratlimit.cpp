/*
 * =====================================================================================
 *
 *       Filename:  ratlimit.cpp
 *
 *    Description:  Implementations of rate limiter related classes 
 *
 *        Version:  1.0
 *        Created:  11/05/2023 08:44:53 PM
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
 *
 * =====================================================================================
 */
#include "ratlimit.h"
namespace rpcf
{

IrpPtr CRateLimiterFido::CBytesWriter::GetIrp() const
{ return m_pIrp; }

CRateLimiterFido::CBytesWriter::CBytesWriter(
    CRateLimiterFido* pPdo )
{
    if( pPdo == nullptr )
        return;
    m_pPort = pPdo;
}

gint32 CRateLimiterFido::CBytesWriter::SetIrpToSend(
    IRP* pIrp, guint32 dwSize )
{
    if( !m_pIrp.IsEmpty() )
        return ERROR_STATE;

    m_pIrp = ObjPtr( pIrp );
    m_iBufIdx = 0;
    m_dwOffset = 0;
    m_dwBytesToSend = dwSize;
    return 0;
}

gint32 CRateLimiterFido::CBytesWriter::SetSendDone()
{
    CRateLimiterFido* pFido = m_pPort;
    CStdRMutex oPortLock( pFido->GetLock() );
    if( m_pIrp.IsEmpty() )
        return 0;
    m_pIrp.Clear();
    m_iBufIdx = -1;
    m_dwOffset = 0;

    return 0;
}

gint32 CRateLimiterFido::CBytesWriter::CancelSend(
    const bool& bCancelIrp )
{
    CRateLimiterFido* pFido = m_pPort;

    CStdRMutex oPortLock( pFido->GetLock() );
    if( m_pIrp.IsEmpty() )
        return -ENOENT;

    IrpPtr pIrp = m_pIrp;
    m_pIrp.Clear();
    m_iBufIdx = -1;
    m_dwOffset = 0;
    oPortLock.Unlock();

    if( !bCancelIrp )
        return 0;

    CIoManager* pMgr = pFido->GetIoMgr();
    CStdRMutex oIrpLock( pIrp->GetLock() );
    gint32 ret = pIrp->CanContinue(
        IRP_STATE_READY );

    if( ERROR( ret ) )
        return ret;

    IrpCtxPtr& pCtx = pIrp->GetTopStack();
    pCtx->SetStatus( ERROR_CANCEL );
    oIrpLock.Unlock();
    pMgr->CompleteIrp( pIrp );

    return 0;
}

#define STMPDO2_MAX_IDX_PER_REQ 1024
#define PORT_INVALID( pPort ) \
({ bool bValid = true; \
    guint32 dwState = pPort->GetPortState(); \
    if( dwState == PORT_STATE_STOPPING || \
        dwState == PORT_STATE_STOPPED || \
        dwState == PORT_STATE_REMOVED ) \
        bValid = false; \
    bValid; })

gint32 CRateLimiterFido::CBytesWriter::SendImmediate(
    PIRP pIrpLocked )
{
    // caller should guarantee no irp is still in
    // process pIrpLocked 
    gint32 ret = 0;
    do{
        IrpPtr& pIrp = pIrpLocked;
        CRateLimiterFido* pPort = m_pPort;

        CStdRMutex oPortLock( pPort->GetLock() );
        TaskletPtr pTokenTask =
            pPort->GetTokenTask( m_bWrite );
        if( pTokenTask.IsEmpty() )
        {
            ret = ERROR_STATE;
            break;
        }
        oPortLock.Unlock();

        CTokenBucketTask* prt = pTokenTask;
        CStdRTMutex oTaskLock( prt->GetLock() );
        oPortLock.Lock();

        if( PORT_INVALID( pPort ) )
        {
            ret = ERROR_STATE;
            break;
        }

        if( !IsSendDone() && m_pIrp != pIrp )
        {
            ret = ERROR_STATE;
            break;
        }

        EnumTaskState iState =
            prt->GetTaskState();
        if( prt->IsStopped( iState ) )
        {
            ret = ERROR_STATE;
            break;
        }

        CfgPtr pCfg;
        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        ret = pCtx->GetReqAsCfg( pCfg );
        if( ERROR( ret ) )
        {
            BufPtr pPayload = pCtx->m_pReqData;
            if( pPayload.IsEmpty() ||
                pPayload->empty() )
            {
                ret = -EFAULT;
                break;
            }

            // a single buffer to send
            CParamList oParams;
            oParams.Push( pPayload );
            pCfg = oParams.GetCfg();
        }

        CParamList oParams( pCfg );
        gint32 iCount = 0;
        guint32& dwCount = ( guint32& )iCount;
        ret = oParams.GetSize( dwCount );
        if( ERROR( ret ) )
            break;

        if( unlikely( dwCount == 0  ||
            dwCount > STMPDO2_MAX_IDX_PER_REQ ) )
        {
            ret = -EINVAL;
            break;
        }

        if( IsSendDone() )
        {
            guint32 dwBytes = 0;
            for( int i = 0; i < iCount; i++ )
            {
                BufPtr pBuf =
                    ( BufPtr& )oParams[ i ];
                dwBytes += pBuf->size();
            }
            if( dwBytes == 0 )
            {
                ret = -EINVAL;
                break;
            }
            SetIrpToSend( pIrp, dwBytes );
        }

        if( m_iBufIdx == iCount )
            break;

        if( m_dwBytesToSend == 0 )
        {
            ret = ERROR_STATE;
            SetSendDone();
            break;
        }

        guint32 dwMaxBytes = m_dwBytesToSend;
        ret = prt->AllocToken( dwMaxBytes ) ;
        if( ret == -ENOENT )
        {
            ret = STATUS_PENDING;
            break;
        }
        oTaskLock.Unlock();

        bool bSplit = true;
        if( m_dwBytesToSend == dwMaxBytes &&
            m_iBufIdx == 0 &&
            m_dwOffset == 0 )
            bSplit = false;

        BufPtr pBuf;
        ret = oParams.GetProperty(
            m_iBufIdx, pBuf );

        if( ERROR( ret ) )
            break;

        if( unlikely( m_dwOffset >=
            pBuf->size() ) )
        {
            ret = -ERANGE;
            break;
        }

        m_dwBytesToSend -= dwMaxBytes;
        CParamList oDest;
        while( bSplit )
        {
            guint32 dwSize = std::min(
                pBuf->size() - m_dwOffset,
                dwMaxBytes );

            BufPtr pDest( true );
            ret = pDest->Attach(
                pBuf->ptr() + m_dwOffset,
                dwSize );

            if( SUCCEEDED( ret ) )
            {
                pDest->SetNoFree();
            }
            else if( ret == -EACCES )
            {
                ret = pDest->Append(
                    pBuf->ptr() + m_dwOffset,
                    dwSize );
            }

            if( ERROR( ret ) )
                break;

            dwMaxBytes -= dwSize;
            oDest.Push( pDest );

            guint32 dwMaxSize = pBuf->size();
            m_dwOffset += dwSize;

            if( dwMaxBytes == 0 )
                break;

            if( dwMaxSize > m_dwOffset )
                continue;

            if( dwMaxSize == m_dwOffset )
            {
                // done with this buf
                m_dwOffset = 0;
                ++m_iBufIdx;

                if( m_iBufIdx == iCount ) 
                {
                    ret = -ENODATA;
                    break;
                }
                else if( unlikely(
                    m_iBufIdx > iCount ) )
                {
                    ret = -EOVERFLOW;
                    break;
                }

                ret = oParams.GetProperty(
                    m_iBufIdx, pBuf );

                if( ERROR( ret ) )
                {
                    ret = -ENODATA;
                    break;
                }

                continue;
            }

            // dwMaxSize < m_dwOffset
            ret = -EOVERFLOW;
            break;
        }

        if( bSplit && oDest.GetCount() == 0 )
        {
            ret = -ENODATA;
            break;
        }

        if( m_dwBytesToSend == 0 )
        {
            SetSendDone();
        }

        BufPtr pOutBuf( true );
        if( pSplit )
        {
            *pOutBuf = oDest.GetCfg();
        }
        else
        {
            *pOutBuf = oParams.GetCfg();
        }

        PortPtr pLower = this->GetLowerPort();
        oPortLock.Unlock();

        pIrp->AllocNextStack(
            pLower, IOSTACK_ALLOC_COPY )
        IrpCtx& pCtx = pIrp->GetTopStack();
        pCtx->SetReqData( pOutBuf );

        pLowerPort->AllocIrpCtxExt(
            pCtx, nullptr );

        ret = pLower->SubmitIrp( pIrp );
        if( ret == STATUS_PENDING )
            break;

        if( ERROR( ret ) )
            break;

        if( IsSendDone() )
            break;

    }while( 1 );

    return ret;
}

gint32 CRateLimiterFido::CBytesWriter::OnSendReady()
{
    gint32 ret = 0;
    CRateLimiterFido* pPort = m_pPort;
    if( unlikely( pPort == nullptr ) )
        return -EFAULT;
    auto pMgr = pPort->GetIoMgr();

    do{
        IrpPtr pIrp;
        CStdRMutex oPortLock( pPort->GetLock() );
        if( IsSendDone() )
        {
            if( pPort->m_queWriteIrps.empty() )
            {
                ret = -ENOENT;
                break;
            }
            pIrp = pPort->m_queWriteIrps.front();
            pPort->m_queWriteIrps.pop_front();
        }
        else
        {
            pIrp = m_pIrp;
        }
        if( pIrp.IsEmpty() )
        {
            ret = -ENOENT;
            break;
        }
        oPortLock.Unlock();

        CStdRMutex oIrpLock( pIrp->GetLock() );
        ret = pIrp->CanContinue(
            IRP_STATE_READY );
        if( ERROR( ret ) )
            break;

        ret = SendImmediate( pIrp );
        if( ret == STATUS_PENDING )
            break;
        oIrpLock.Unlock();
        pMgr->CompleteIrp( pIrp );

    }while( 1 );

    return ret;
}

bool CRateLimiterFido::CBytesWriter::IsSendDone() const
{
    if( m_pIrp.IsEmpty() )
        return true;
    return false;
}

gint32 CRateLimiterFido::CBytesWriter::Clear()
{ return 0; }

gint32 CRateLimiterFido::CompleteIoctlIrp(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;

    if( pIrp->IsIrpHolder() )
    {
        // unknown error
        ret = ERROR_FAIL;
        pIrp->SetStatus( ret );
        return ret;
    }

    IrpCtxPtr pCtx = pIrp->GetCurCtx();
    IrpCtxPtr pTopCtx = pIrp->GetTopStack();
    guint32 dwCtrlCode = pTopCtx->GetCtrlCode();

    switch( dwCtrlCode )
    {
    case CTRLCODE_LISTENING:
        {
            ret = pTopCtx->GetStatus();
            if( ERROR( ret ) )
            {
                pCtx->SetStatus( ret );
                break;
            }

            BufPtr& pBuf = pTopCtx->m_pRespData;
            if( pBuf.IsEmpty() || pBuf->empty() )
            {
                ret = -EBADMSG;
                pCtx->SetStatus( ret );
            }
            else
            {
                STREAM_SOCK_EVENT* psse =
                ( STREAM_SOCK_EVENT* )pBuf->ptr();
                if( psse->m_iEvent == sseError )
                {
                    pCtx->SetRespData( pBuf );
                    pCtx->SetStatus(
                        pTopCtx->GetStatus() );
                    break;
                }
                else if( psse->m_iEvent == sseRetWithFd )
                {
                    ret = -ENOTSUP;
                    pCtx->SetStatus( ret );
                    break;
                }
                ret = CompleteListeningIrp( pIrp );
            }
            break;
        }
    default:
        {
            ret = pTopCtx->GetStatus();
            pCtx->SetStatus( ret );
            break;
        }
    }

    return ret;
}

gint32 CRateLimiterFido::CompleteFuncIrp( IRP* pIrp )
{
    gint32 ret = 0;

    do{
        if( pIrp == nullptr ||
            pIrp->GetStackSize() == 0 )
        {
            ret = -EINVAL;
            break;
        }

        if( pIrp->MajorCmd() != IRP_MJ_FUNC )
        {
            ret = -EINVAL;
            break;
        }
        
        switch( pIrp->MinorCmd() )
        {
        case IRP_MN_IOCTL:
            {
                ret = CompleteIoctlIrp( pIrp );
                break;
            }
        case IRP_MN_WRITE:
            {
                ret = CompleteWriteIrp( pIrp );
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
    {
        IrpCtxPtr& pCtx =
            pIrp->GetCurCtx();

        if( !pIrp->IsIrpHolder() )
        {
            IrpCtxPtr& pCtxLower = pIrp->GetTopStack();

            ret = pCtxLower->GetStatus();
            pCtx->SetStatus( ret );
            pIrp->PopCtxStack();
        }
        else
        {
            pCtx->SetStatus( ret );
        }
    }
    return ret;
}

gint32 CRateLimiterFido::CompleteWriteIrp(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    if( !pIrp->IsIrpHolder() )
    {
        IrpCtxPtr& pTopCtx = pIrp->GetTopStack();
        IrpCtxPtr& pCtx = pIrp->GetCurCtx();
        ret = pTopCtx->GetStatus();
        pCtx->SetStatus( ret );
        pIrp->PopCtxStack();
    }
    if( ERROR( ret ) )
        return ret;

    do{
        CStdRMutex oLock( GetLock() );
        if( m_oWriter.IsSendDone() )
        {
            if( m_queWriteIrps.empty() )
                break;
            gint32 (*func)( CRateLimiterFido* ) =
            ([]( CRateLimiterFido* pPort )->gint32
            {
                CStdRMutex oLock( GetLock() );
                if( PORT_INVALID( pPort ) )
                    return 0;

                CBytesWriter& oWriter =
                    pPort->m_oWriter;
                oLock.Unlock();
                oWriter.OnSendReady();
                return 0;
            });

            auto pMgr = this->GetIoMgr();
            TaskletPtr pTask;
            ret = NEW_FUNCCALL_TASKEX( pTask,
                pMgr, func, this );
            if( ERROR( ret ) )
                break;
            ret = pMgr->RescheduleTask( pTask );
            break;
        }

        // another irp
        if( m_oWriter.m_pIrp != pIrp )
        {
            ret = ERROR_STATE;
            break;
        }
        oLock.Unlock();

        ret = m_oWriter.SendImmediate();
        if( ret == STATUS_PENDING )
            break;

        if( ERROR( ret ) )
            break;

    }while( 1 );

    return ret;
}

CRateLimiterFido::CRateLimiterFido(
    const IConfigDb* pCfg )
    : super( pCfg )
{
    SetClassId( clsid( CRateLimiterFido ) );
    m_dwFlags &= ~PORTFLG_TYPE_MASK;
    m_dwFlags |= PORTFLG_TYPE_FIDO;
}

TaskletPtr CRateLimiterFido::GetTokenTask(
    bool bWrite )
{
    if( bWrite )
        return m_pWriteTb;
    else
        return m_pReadTb;
}

gint32 CRateLimiterFido::PostStart(
    PIRP pIrp )
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
        {
            ( *m_pReadTb )( eventCancelTask );
            m_pReadTb.Clear();
        }
        if( !m_pWriteTb.IsEmpty() )
        {
            ( *m_pWriteTb )( eventCancelTask );
            m_pWriteTb.Clear();
        }
    }
    return ret;
}

gint32 CRateLimiterFido::PreStop( PIRP pIrp )
{
    if( !m_pWriteTb.IsEmpty() )
    {
        ( *m_pWriteTb )( eventCancelTask );
    }
    if( !m_pReadTb.IsEmpty() )
    {
        ( *m_pReadTb )( eventCancelTask );
    }

    gint32 ret = super::PreStop( pIrp );
    CStdRMutex oPortLock( GetLock() );
    m_pReadTb.Clear();
    m_pWriteTb.Clear();
    // the irps will be released in super class's
    // PreStop
    m_queWriteIrp.clear();
    m_queReadIrp.clear();
    m_pReadIrp.Clear();
    m_pWriteIrp.Clear();
    m_pLoop.Clear();
    m_oWriter.Clear();
    return ret;
}

gint32 CRateLimiterFido::ResumeWrite()
{ return m_oWriter.OnSendReady(); }

gint32 CRateLimiterFido::OnSubmitIrp(
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
        case IRP_MN_WRITE:
            {
                // direct stream access
                ret = SubmitWriteIrp( pIrp );
                break;
            }
        case IRP_MN_IOCTL:
            {
                ret = SubmitIoctlCmd( pIrp );
                break;
            }
        default:
            {
                ret = PassUnknownIrp( pIrp );
                break;
            }
        }
    }while( 0 );

    return ret;
}
 
gint32 CRateLimiterFido::OnPortReady(
    PIRP pIrp )
{
    
}

gint32 CRateLimiterFido::OnEvent(
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
            CStdRMutex oPortLock( GetLock() );
            if( PORT_INVALID( this ) )
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

}
