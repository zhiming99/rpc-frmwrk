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

CBytesWriter::CBytesWriter(
    CRateLimiterFido* pPdo )
{
    if( pPdo == nullptr )
        return;
    m_pPort = ( IPort* )pPdo;
}

gint32 CBytesWriter::SetIrpToSend(
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

gint32 CBytesWriter::SetSendDone(
    gint32 iRet )
{
    CRateLimiterFido* pFido = m_pPort;
    if( pFido == nullptr )
        return -EFAULT;

    CStdRMutex oPortLock( pFido->GetLock() );
    if( m_pIrp.IsEmpty() )
        return 0;
    IrpPtr pIrp = m_pIrp;
    m_pIrp.Clear();
    m_iBufIdx = -1;
    m_dwOffset = 0;
    oPortLock.Unlock();

    if( pIrp.IsEmpty() ||
        pIrp->GetStackSize() == 0 )
        return 0;

    CStdRMutex oIrpLock( pIrp->GetLock() );

    gint32 ret = pIrp->CanContinue(
        IRP_STATE_READY );

    if( ERROR( ret ) )
        return 0;

    IrpCtxPtr& pCtx = pIrp->GetTopStack();
    pCtx->SetStatus( iRet );
    oIrpLock.Unlock();
    CIoManager* pMgr = pFido->GetIoMgr();
    pMgr->CompleteIrp( pIrp );

    return 0;
}

gint32 CBytesWriter::CancelSend(
    const bool& bCancelIrp )
{
    CRateLimiterFido* pFido = m_pPort;
    if( pFido == nullptr )
        return -EFAULT;

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

gint32 CBytesWriter::SendImmediate(
    PIRP pIrpLocked )
{
    // caller should guarantee no irp ahead of
    // pIrpLocked 
    gint32 ret = 0;
    do{
        IrpPtr& pIrp = pIrpLocked;
        CRateLimiterFido* pPort = m_pPort;
        if( unlikely( pPort == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }
        CStdRMutex oPortLock( pPort->GetLock() );

        guint32 dwState = GetPortState();
        if( dwState == PORT_STATE_STOPPING ||
            dwState == PORT_STATE_STOPPED ||
            dwState == PORT_STATE_REMOVED )
        {
            ret = ERROR_STATE;
            break;
        }

        if( IsSendDone() )
            return ERROR_NOT_HANDLED;

        TaskletPtr pTokenTask =
            pPort->GetTokenTask( m_bWrite );
        oPortLock.Unlock();

        CIfRetryTask* prt = pTokenTask;
        CStdRTMutex oTaskLock( prt->GetLock() );
        oPortLock.Lock();

        guint32 dwState = GetPortState();
        if( dwState == PORT_STATE_STOPPING ||
            dwState == PORT_STATE_STOPPED ||
            dwState == PORT_STATE_REMOVED )
        {
            ret = ERROR_STATE;
            break;
        }

        IrpCtxPtr& pCtx = m_pIrp->GetTopStack();
        CfgPtr pCfg;
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

        if( m_iBufIdx >= iCount )
        {
            // send done
            break;
        }

        guint32 dwMaxBytes = m_dwBytesToSend;
        ret = pPort->AllocToken( dwMaxBytes ) ;
        if( ret == -ENOENT )
        {
            ret = STATUS_PENDING;
            break;
        }
        oTaskLock.Unlock();

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
        do{
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
                pDest->Append(
                    pBuf->ptr() + m_dwOffset,
                    dwSize );
            }
            else
            {
                break;
            }

            dwMaxBytes -= dwSize;
            oDest.Push( pDest );

            guint32 dwMaxSize = pBuf->size();
            guint32 dwNewOff = m_dwOffset + dwSize;

            ret = 0;
            if( dwMaxSize == dwNewOff )
            {
                // done with this buf
                m_dwOffset = 0;
                ++m_iBufIdx;

                if( m_iBufIdx == iCount ) 
                {
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
                    break;

                continue;
            }
            else if( dwMaxSize < dwNewOff )
            {
                ret = -EOVERFLOW;
                break;
            }

            m_dwOffset = dwNewOff;

        }while( 1 );

        if( oDest.GetCount() == 0 )
        {
            ret = -ENODATA;
            break;
        }

        bool bDone = false;
        if( m_dwBytesToSend == 0 )
        {
            bDone = true;
            SetSendDone( ret );
        }

        BufPtr pOutBuf( true );
        *pOutBuf = oDest.GetCfg();

        PortPtr pLower = this->GetLowerPort();
        oPortLock.Unlock();

        pIrp->AllocNextStack(
            pLower, IOSTACK_ALLOC_COPY )
        IrpCtx& pCtx = pIrp->GetTopStack();
        pCtx->SetReqData( pOutBuf );
        ret = pLower->SubmitIrp( pIrp );
        if( ret == STATUS_PENDING )
            break;

        if( bDone )
            break;

    }while( 1 );

    return ret;
}

gint32 CBytesWriter::OnSendReady()
{
    gint32 ret = 0;
    do{
        IrpPtr pIrp;
        CRateLimiterFido* pPort = m_pPort;
        if( unlikely( pPort == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }
        CStdRMutex oPortLock( pPort->GetLock() );
        if( IsSendDone() )
            return ERROR_NOT_HANDLED;

        pIrp = m_pIrp;
        oPortLock.Unlock();

        CStdRMutex oIrpLock( pIrp->GetLock() );
        if( !( pIrp == m_pIrp ) )
            continue;
        ret = m_pIrp->CanContinue(
            IRP_STATE_READY );
        if( ERROR( ret ) )
            break;

        ret = SendImmediate( pIrp );
        break;

    }while( 1 );

    return ret;
}

bool CBytesWriter::IsSendDone() const
{
    if( m_pIrp.IsEmpty() )
        return true;

    return false;
}

gint32 CBytesWriter::Clear()
{
    PortPtr pPort = m_pPort;
    if( pPort.IsEmpty() )
        return 0;

    CPort* ptr = pPort;
    CStdRMutex oPortLock(
        ptr->GetLock() );
    if( !m_pIrp.IsEmpty() )
        return ERROR_STATE;

    m_pPort.Clear();

    return 0;
}

}
