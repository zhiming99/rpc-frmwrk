/*
 * =====================================================================================
 *
 *       Filename:  sslfido.cpp
 *
 *    Description:  Implementation of OpenSSL filter port and related classes 
 *
 *        Version:  1.0
 *        Created:  11/26/2019 08:43:57 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2019 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#include "rpc.h"
#include "ifhelper.h"
#include "dbusport.h"
#include "sslfido.h"
#include "uxport.h"
#include "jsondef.h"

namespace rpcf
{

#define PROP_TASK_QUEUED 10
#define IsWantIo( n ) \
  ( n == SSL_ERROR_WANT_WRITE || \
    n == SSL_ERROR_WANT_READ )

#define SSL_ENCRYPT_OVERHEAD    128
extern gint32 GetSSLError( SSL* pssl, int n );

CRpcOpenSSLFido::CRpcOpenSSLFido(
    const IConfigDb* pCfg ) :
    super( pCfg )
{
    SetClassId( clsid( CRpcOpenSSLFido ) );
    m_dwFlags &= ~PORTFLG_TYPE_MASK;
    m_dwFlags |= PORTFLG_TYPE_FIDO;

    gint32 ret = 0;
    do{
        CCfgOpener oParams( pCfg );
        ret = oParams.GetIntPtr( propSSLCtx,
            ( guint32*& )m_pSSLCtx );

        if( ERROR( ret ) )
            break;
        Sem_Init( &m_semWriteSync, 0, 1 );
        Sem_Init( &m_semReadSync, 0, 1 );

        ret = m_pOutBuf.NewObj();
        if( ERROR( ret ) )
            break;

        m_pOutBuf->Resize(
            STM_MAX_BYTES_PER_BUF + 512 );

    }while( 0 );
    if( ERROR( ret ) )
    {
        std::string strMsg = DebugMsg( ret,
            "Error occurs in CRpcOpenSSLFido ctor" );  
        throw std::runtime_error( strMsg );
    }
}

CRpcOpenSSLFido::~CRpcOpenSSLFido()
{
    if( m_pSSLCtx != nullptr )
    {
        SSL_CTX_free( m_pSSLCtx );
        m_pSSLCtx = nullptr;
    }

    sem_destroy( &m_semWriteSync );
    sem_destroy( &m_semReadSync );
}

gint32 CRpcOpenSSLFido::EncryptAndSend(
    PIRP pIrp, CfgPtr pCfg,
    gint32 iIdx, guint32 dwTotal,
    guint32 dwOffset, bool bResume )
{
    gint32 ret = 0;
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    if( pCfg.IsEmpty() || iIdx < 0 )
        return -EINVAL;

    do{
        gint32 iCount = 0;
        guint32& dwCount = ( guint32& )iCount;
        CParamList oParams( pCfg );
        ret = oParams.GetSize( dwCount );
        if( ERROR( ret ) )
            break;

        CStdRMutex oPortLock( GetLock() );
        if( iIdx < iCount ) do{

            BufPtr pBuf;
            ret = oParams.GetProperty(
                iIdx, pBuf );

            if( ERROR( ret ) )
                break;

            if( pBuf->size() <= dwOffset )
            {
                ret = -EINVAL;
                break;
            }

            gint32 n = SSL_write( m_pSSL,
                pBuf->ptr() + dwOffset,
                pBuf->size() - dwOffset );

            ret = GetSSLError( m_pSSL, n );
            if( ERROR( ret ) )
                break;

            if( ret == SSL_ERROR_WANT_READ )
            {
                m_dwRenegStat = rngstatWantRead;
                // save the context and wait for
                // the listening irp to complete
                TaskletPtr pTask;
                if( bResume )
                {
                    if( m_queWriteTasks.empty() )
                    {
                        ret = ERROR_STATE;
                        break;
                    }

                    pTask = m_queWriteTasks.front();
                    CCfgOpenerObj oTaskCfg(
                        ( CObjBase* )pTask );
                    oTaskCfg.SetIntProp( 0, iIdx );
                    oTaskCfg.SetIntProp( 1, dwOffset );
                    oTaskCfg.SetIntProp( 2, dwTotal );
                    oTaskCfg.SetBoolProp(
                        PROP_TASK_QUEUED, true );
                }
                else
                {
                    ret = BuildResumeTask( pTask, pIrp,
                        iIdx, dwOffset, dwTotal );
                    if( ERROR( ret ) )
                        break;
                    m_queWriteTasks.push_back(
                        pTask );
                }

                ret = STATUS_PENDING;
                DebugPrint( ret,
                    "Waiting for incoming packets" );
                break;
            }
            else if(  ret == SSL_ERROR_WANT_WRITE )
            {
                // don't know how to handle
                ret = ERROR_STATE;
                DebugPrint( ret,
                    "Want write, what happens?" );
                break;
            }

            dwTotal += n;
            if( n + dwOffset == pBuf->size() )
            {
                ++iIdx;
                // we are done with all the
                // buffers.
                if( iIdx == iCount )
                {
                    ret = 0;
                    break;
                }

                // move to next buffer
                dwOffset = 0;
            }
            else if( n + dwOffset < pBuf->size() )
            {
                // possible if partial write is
                // enabled
                dwOffset += n;
            }

        }while( 1 );

        if( ret == STATUS_PENDING )
            break;

        if( ERROR( ret ) )
            break;

        // the least size
        guint32 dwExpSize = std::min(
            ( dwTotal + ( guint32 )PAGE_SIZE ),
            ( guint32 )STM_MAX_BYTES_PER_BUF );

        // make the tail align to the page
        // boundary
        dwExpSize &= ~( PAGE_SIZE - 1 );

        guint32 dwOutOff = 0;
        char* pHoldBuf = GetOutBuf();
        guint32 dwHoldSize = GetOutSize();

        do{
            ret = BIO_read( m_pwbio,
                pHoldBuf + dwOutOff,
                dwHoldSize - dwOutOff );

            if( ret > 0 )
            {
                dwOutOff += ret;
                if( dwOutOff == dwHoldSize )
                {
                    m_pOutBuf->Resize(
                        dwHoldSize * 2  );
                    dwHoldSize = GetOutSize();
                    pHoldBuf = GetOutBuf();
                }
                continue;
            }
            else if( !BIO_should_retry( m_pwbio ) )
            {
                // error occurs
                ret = ERROR_FAIL;
                break;
            }

            ret = 0;
            break;

        }while( 1 );

        if( ERROR( ret ) )
            break;

        if( dwOutOff == 0 )
        {
            // all the data has been held by the
            // SSL without output, complete this
            // irp and wait till next write.
            DebugPrint( ret,
                "Strange, all %d bytes has "\
                "been eaten by SSL without output",
                dwTotal );
            break;
        }

        BufPtr pOutBuf( true );
        pOutBuf->Resize( dwOutOff );
        memcpy( pOutBuf->ptr(), pHoldBuf, dwOutOff );

        oPortLock.Unlock();

        // send the encrypted copy down 
        PortPtr pLowerPort = GetLowerPort();
        pIrp->AllocNextStack(
            pLowerPort, IOSTACK_ALLOC_COPY  );

        CParamList oNewParam;
        oNewParam.Push( pOutBuf );
        IrpCtxPtr pCtx = pIrp->GetTopStack();
        BufPtr pNewBuf( true );
        *pNewBuf = ObjPtr( oNewParam.GetCfg() );
        pCtx->SetReqData( pNewBuf );
        pLowerPort->AllocIrpCtxExt(
            pCtx, ( PIRP )pIrp );

        ret = pLowerPort->SubmitIrp( pIrp );
        if( ret != STATUS_PENDING )
            pIrp->PopCtxStack();

    }while( 0 );

    if( ret != STATUS_PENDING )
    {
        IrpCtxPtr pCtx = pIrp->GetTopStack();
        pCtx->SetStatus( ret );
    }

    return ret;
}

gint32 CRpcOpenSSLFido::BuildResumeTask(
    TaskletPtr& pResumeTask, PIRP pIrp,
    gint32 iIdx, guint32 dwOffset,
    guint32 dwTotal )
{
    gint32 ret = 0;
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    do{
        CParamList oTaskCfg;
        // save the context and wait for
        // the listening irp to complete
        oTaskCfg.SetPointer(
            propIrpPtr, pIrp );

        oTaskCfg.Push( iIdx );
        oTaskCfg.Push( dwOffset );
        oTaskCfg.Push( dwTotal );

        oTaskCfg.SetPointer(
            propIoMgr, GetIoMgr() );

        oTaskCfg.SetPointer(
            propPortPtr, this );

        ret = pResumeTask.NewObj(
            clsid( COpenSSLResumeWriteTask ),
            oTaskCfg.GetCfg() );

        if( ERROR( ret ) )
            break;

    }while( 0 );

    return ret;
}

gint32 CRpcOpenSSLFido::SubmitWriteIrp(
    PIRP pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CfgPtr pCfg;
        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        ret = pCtx->GetReqAsCfg( pCfg );
        if( ERROR( ret ) )
        {
            BufPtr pPayload = pCtx->m_pReqData;
            if( pPayload.IsEmpty() ||
                pPayload->empty() )
            {
                ret = -EINVAL;
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

        if( iCount <= 0 )
        {
            ret = -EINVAL;
            break;
        }

        Sem_Wait( &m_semWriteSync );

        CStdRMutex oPortLock( GetLock() );
        if( m_dwRenegStat == rngstatNormal )
        {
            if( m_queWriteTasks.size() > 0 )
            {
                ret = ERROR_STATE;
                break;
            }
        }
        else
        {
            TaskletPtr pTask;
            ret = BuildResumeTask(
                pTask, pIrp, 0, 0, 0 );

            if( ERROR( ret ) )
                break;

            m_queWriteTasks.push_back( pTask );
            ret = STATUS_PENDING; 
            Sem_Post( &m_semWriteSync );
            break;
        }

        oPortLock.Unlock();

        ret = EncryptAndSend(
            pIrp, pCfg, 0, 0, 0, false );

        Sem_Post( &m_semWriteSync );

    }while( 0 );

    return ret;
}

gint32 CRpcOpenSSLFido::SubmitIoctlCmd(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    switch( pIrp->CtrlCode() )
    {
    case CTRLCODE_LISTENING:
        {
            guint32 dwOffset = 0;
            BufPtr pBuf( true );
            pBuf->Resize( PAGE_SIZE );

            CStdRMutex oPortLock( GetLock() );
            if( m_bFirstRead )do{
                m_bFirstRead = false;
                ret = SSL_read( m_pSSL,
                    pBuf->ptr() + dwOffset,
                    pBuf->size() - dwOffset );

                if( ret > 0 )
                {
                    DebugPrint( ret, "Catched something" );
                    dwOffset += ret;    
                    if( pBuf->size() == dwOffset )
                    {
                        gint32 dwRest =
                            SSL_pending( m_pSSL );

                        if( dwRest == 0 )
                            dwRest = PAGE_SIZE;

                        pBuf->Resize(
                            dwOffset + dwRest );
                    }
                    continue;
                }

                if( dwOffset > 0 )
                    pBuf->Resize( dwOffset );
                else
                    break;

                STREAM_SOCK_EVENT sse;
                BufPtr pRespBuf( true );
                pRespBuf->Resize( sizeof( sse ) );
                sse.m_iEvent = sseRetWithBuf;
                sse.m_pInBuf = pBuf;
                new ( pRespBuf->ptr() )
                    STREAM_SOCK_EVENT( sse );

                IrpCtxPtr& pCtx =
                    pIrp->GetTopStack();

                pCtx->SetRespData( pRespBuf );
                pCtx->SetStatus( 0 );
                break;

            }while( 1 );

            oPortLock.Unlock();
            // we have some thing to return
            if( dwOffset > 0 )
            {
                ret = 0;
                break;
            }

            PortPtr pLowerPort = GetLowerPort();
            ret = pIrp->AllocNextStack(
                pLowerPort, IOSTACK_ALLOC_COPY );

            if( ERROR( ret ) )
                break;

            IrpCtxPtr pTopCtx =
                pIrp->GetTopStack();

            pLowerPort->AllocIrpCtxExt(
                pTopCtx, pIrp );

            ret = pLowerPort->SubmitIrp( pIrp );
            if( ret == STATUS_PENDING )
                break;

            IrpCtxPtr& pCtx =
                pIrp->GetCurCtx();

            if( ERROR( ret ) )
            {
                pCtx->SetStatus( ret );
                break;
            }

            BufPtr pRespBuf =
                pTopCtx->m_pRespData;

            if( unlikely( pRespBuf.IsEmpty() ) )
            {
                ret = -EFAULT;
                break;
            }
            STREAM_SOCK_EVENT* psse =
            ( STREAM_SOCK_EVENT* )pRespBuf->ptr();

            if( unlikely(
                psse->m_iEvent == sseError ) )
            {
                pCtx->SetRespData( pRespBuf );
                pCtx->SetStatus( STATUS_SUCCESS );
            }
            else if( unlikely(
                psse->m_iEvent == sseRetWithFd ) )
            {
                psse->m_iEvent = sseError;
                psse->m_iData = -ENOTSUP;
                psse->m_iEvtSrc = GetClsid();
                pCtx->SetRespData( pRespBuf );
                pCtx->SetStatus( STATUS_SUCCESS );
            }
            else if( unlikely(
                psse->m_iEvent == sseInvalid ) )
            {
                psse->m_iEvent = sseError;
                psse->m_iData = -ENOTSUP;
                psse->m_iEvtSrc = GetClsid();
                pCtx->SetRespData( pRespBuf );
                pCtx->SetStatus( STATUS_SUCCESS );
            }
            else
            {
                ret = CompleteListeningIrp( pIrp );
            }
            break;
        }
    default:
        {
            // NOTE: the default behavior for a
            // filter driver is to pass unknown
            // irps on to the lower driver
            ret = PassUnknownIrp( pIrp );
            break;
        }
    }

    return ret;
}

gint32 CRpcOpenSSLFido::OnSubmitIrp(
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

gint32 CRpcOpenSSLFido::StartSSLHandshake(
    PIRP pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CParamList oParams;

        oParams.SetPointer(
            propPortPtr, this );

        oParams.SetPointer(
            propIoMgr, GetIoMgr() );

        oParams.SetPointer(
            propIrpPtr, pIrp );

        oParams.SetIntProp(
            propTimeoutSec,
            PORT_START_TIMEOUT_SEC - 20 );

        ret = GetIoMgr()->ScheduleTask(
            clsid( COpenSSLHandshakeTask ),
            oParams.GetCfg() );

        if( SUCCEEDED( ret ) )
            ret = STATUS_PENDING;

    }while( 0 );

    return ret;
}

gint32 CRpcOpenSSLFido::StartSSLShutdown(
    PIRP pIrp )
{
    gint32 ret = 0;
    do{
        CParamList oParams;
        CIoManager* pMgr = GetIoMgr();
        oParams.SetPointer( propIoMgr, pMgr );
        oParams.SetPointer( propIrpPtr,  pIrp );
        oParams.SetPointer( propPortPtr, this );
        oParams.SetIntProp( propTimeoutSec,
            PORT_START_TIMEOUT_SEC - 20 );

        ret = pMgr->ScheduleTask(
            clsid( COpenSSLShutdownTask ),
            oParams.GetCfg() );

        if( SUCCEEDED( ret ) )
            ret = STATUS_PENDING;

    }while( 0 );

    return ret;
}

gint32 CRpcOpenSSLFido::PostStart(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        PortPtr pPort = GetLowerPort();
        ret = pIrp->AllocNextStack( pPort );
        if( ERROR( ret ) )
            break;

        IrpCtxPtr& pCtx = pIrp->GetTopStack(); 
        pCtx->SetMajorCmd( IRP_MJ_FUNC );
        pCtx->SetMinorCmd( IRP_MN_IOCTL );
        pCtx->SetCtrlCode( CTRLCODE_IS_CLIENT );
        pPort->AllocIrpCtxExt( pCtx, pIrp );

        ret = pPort->SubmitIrp( pIrp );
        if( SUCCEEDED( ret ) )
        {
            m_bClient = true;
        }
        else if( ret == ERROR_FALSE )
        {
            m_bClient = false;
            ret = 0;
        }

        pIrp->PopCtxStack();
        if( ERROR( ret ) )
            break;

        ret = InitSSL();
        if( ERROR( ret ) )
            break;

        // strange, the handshake is done?
        if( SSL_is_init_finished( m_pSSL ) )
            break;

        // hold the irp and start the handshake
        // task
        ret = StartSSLHandshake( pIrp );
        break;

    }while( 0 );

    return ret;
}

gint32 CRpcOpenSSLFido::PreStop(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    guint32 dwStepNo = 0;
    do{
        ret = GetPreStopStep( pIrp, dwStepNo );
        if( dwStepNo == 0 )
        {
            SetPreStopStep( pIrp, 1 );
            ret = StartSSLShutdown( pIrp );
            if( ret == STATUS_PENDING )
            {
                ret = STATUS_MORE_PROCESS_NEEDED;
                break;
            }
        }

        ret = GetPreStopStep( pIrp, dwStepNo );
        if( dwStepNo == 1 )
        {
            SetPreStopStep( pIrp, 2 );
            ret = super::PreStop( pIrp );
        }

    }while( 0 );

    return ret;
}

gint32 CRpcOpenSSLFido::Stop( IRP* pIrp )
{
    if( m_pSSL != nullptr )
    {
        SSL_free( m_pSSL );
        m_pSSL = nullptr;
    }
    return super::Stop( pIrp );
}

gint32 CRpcOpenSSLFido::CompleteListeningIrp(
    IRP* pIrp )
{
    // listening request from the upper port
    if( pIrp == nullptr ||
        pIrp->GetStackSize() < 2 )
        return -EINVAL;

    if( pIrp->IsIrpHolder() )
        return -EINVAL;

    gint32 ret = 0;
    IrpCtxPtr pCtx;
    pCtx = pIrp->GetCurCtx();
    IrpCtxPtr pTopCtx = pIrp->GetTopStack();
    BufPtr pRespBuf = pTopCtx->m_pRespData;
    STREAM_SOCK_EVENT* psse =
        ( STREAM_SOCK_EVENT* )pRespBuf->ptr();
    do{
        BufPtr pEncrypted = psse->m_pInBuf;
        if( pEncrypted.IsEmpty() ||
            pEncrypted->empty() )
        {
            ret = -EBADMSG;
            break;
        }

        // explicitly release the buffer in the
        // STREAM_SOCK_EVENT
        psse->m_pInBuf.Clear();

        // NOTE: SSL is not thread-safe, locking
        // is required
        CStdRMutex oPortLock( GetLock() );
        ret = BIO_write( m_prbio,
            pEncrypted->ptr(),
            pEncrypted->size() );

        if( ret <= 0 )
        {
            ret = ERROR_FAIL;
            break;
        }

        BufPtr pDecrypted( true );
        guint32 dwOffset = 0;
        gint32 iNumRead = 0;

        pDecrypted->Resize( PAGE_SIZE );

        do{
            iNumRead = SSL_read( m_pSSL,
                pDecrypted->ptr() + dwOffset,
                pDecrypted->size() - dwOffset );

            if( iNumRead <= 0 )
                break;

            dwOffset += iNumRead;

            guint32 dwPending =
                SSL_pending( m_pSSL );

            guint32 dwOldSize =
                pDecrypted->size();

            guint32 dwNewSize =
                dwPending + dwOffset;

            if( dwNewSize > dwOldSize )
            {
                pDecrypted->Resize( dwNewSize );
                if( pDecrypted->size() ==
                    dwOldSize )
                {
                    // realloc failed
                    ret = -ENOMEM;
                    break;
                }
            }
            else if( dwOffset == dwOldSize )
            {
                pDecrypted->Resize(
                    dwOldSize + PAGE_SIZE );
                if( pDecrypted->size() ==
                    dwOldSize )
                {
                    // realloc failed
                    ret = -ENOMEM;
                    break;
                }
            }

        }while( 1 );

        gint32 iRet = ret;
        ret = GetSSLError( m_pSSL, iNumRead );
        if( ERROR( ret ) || ERROR( iRet ) )
        {
            if( SUCCEEDED( ret ) )
                ret = iRet;
            break;
        }

        oPortLock.Unlock();

        if( ret == SSL_ERROR_WANT_READ )
        {
            if( dwOffset == 0 )
            {
                // incoming data is not enough for
                // SSL to move on, resubmit the
                // irp
                pTopCtx->m_pRespData.Clear();
                IPort* pPort = GetLowerPort();
                ret = pPort->SubmitIrp( pIrp );
                if( SUCCEEDED( ret ) )
                {
                    pRespBuf = pTopCtx->m_pRespData;
                    psse = ( STREAM_SOCK_EVENT* )
                        pRespBuf->ptr();
                    continue;
                }

                // STATUS_PENDING goes here
            }
            else
            {
                // deliver the received data up
                // and leave the remaining bytes
                // still on the way to the next
                // irp
                ret = 0;
            }
        }
        else if( ret == SSL_ERROR_WANT_WRITE )
        {
            // insert a write request
            CStdRMutex oPortLock1( GetLock() );
            if( m_dwRenegStat == rngstatNormal )
                m_dwRenegStat = rngstatWantRead;

            if( m_queWriteTasks.empty() )
            {
                TaskletPtr pTask;
                IrpPtr pIrp( true );
                PortPtr pPort = GetLowerPort();
                pIrp->AllocNextStack( pPort );

                IrpCtxPtr& pCtx =
                    pIrp->GetTopStack();

                pCtx->SetMajorCmd( IRP_MJ_FUNC );
                pCtx->SetMinorCmd( IRP_MN_WRITE );
                pPort->AllocIrpCtxExt(
                    pCtx, ( PIRP )pIrp );

                CParamList oEmptyCfg;
                BufPtr pReqBuf( true );
                *pReqBuf = ObjPtr(
                    oEmptyCfg.GetCfg() );
                pCtx->SetReqData( pReqBuf );
                ret = BuildResumeTask(
                    pTask, pIrp, 0, 0, 0 );

                if( ERROR( ret ) )
                    break;

                m_queWriteTasks.push_back(
                    pTask );
            }
        }

        if( ERROR( ret ) )
            break;

        if( dwOffset > 0 )
        {
            // set the decrypted buffer to return
            if( dwOffset < pDecrypted->size() )
                pDecrypted->Resize( dwOffset );

            psse->m_pInBuf = pDecrypted;
            pCtx->SetRespData( pRespBuf );
            pCtx->SetStatus( ret );
        }
        else if( ret == STATUS_PENDING )
        {
            DebugPrint( 0,
                "Resubmit the irp..." );
        }

        // issue the write request if any
        oPortLock.Lock();
        if( m_dwRenegStat != rngstatWantRead )
            break;

        m_dwRenegStat = rngstatResumeWrite;
        if( m_queWriteTasks.empty() )
        {
            ret = ERROR_STATE;
            break;
        }
        DEFER_CALL( GetIoMgr(), this,
            &CRpcOpenSSLFido::ResumeWriteTasks );

        break;

    }while( 1 );

    if( ret == STATUS_PENDING )
        return ret;

    if( ERROR( ret ) )
    {
        psse->m_iEvent = sseError;
        psse->m_iData = ret;
        psse->m_pInBuf.Clear();
        psse->m_iEvtSrc = GetClsid();
        pCtx->SetRespData( pRespBuf );
        DebugPrint( ret, "SSLFido, error detected "
        "in CompleteListeningIrp" );
        ret = STATUS_SUCCESS;
    }
    pCtx->SetStatus( 0 );
    pIrp->PopCtxStack();

    return ret;
}

gint32 CRpcOpenSSLFido::CompleteIoctlIrp(
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
                else if( psse->m_iEvent != sseRetWithBuf )
                {
                    psse->m_iEvent = sseError;
                    psse->m_iData = -ENOTSUP;
                    psse->m_iEvtSrc = GetClsid();
                    pCtx->SetRespData( pBuf );
                    pCtx->SetStatus( STATUS_SUCCESS );
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

gint32 CRpcOpenSSLFido::CompleteFuncIrp( IRP* pIrp )
{
    gint32 ret = 0;

    do{
        if( pIrp == nullptr 
           || pIrp->GetStackSize() == 0 )
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
            ret = pCtx->GetStatus();
        }
    }
    return ret;
}

gint32 CRpcOpenSSLFido::CompleteWriteIrp(
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

    return ret;
}

gint32 CRpcOpenSSLFido::SendWriteReq(
    IEventSink* pCallback, BufPtr& pBuf )
{
    return rpcf::SendWriteReq(
        this, pCallback, pBuf );
}

gint32 CRpcOpenSSLFido::SendListenReq(
    IEventSink* pCallback, BufPtr& pBuf )
{
    return rpcf::SendListenReq(
        this, pCallback, pBuf );
}

gint32 CRpcOpenSSLFido::AdvanceHandshake(
     IEventSink* pCallback,
     BufPtr& pHandshake )
{
    gint32 ret = 0;

    if( !pHandshake.IsEmpty() &&
        !pHandshake->empty() )
    {
        ret = BIO_write( m_prbio,
            pHandshake->ptr(),
            pHandshake->size() );
        if( ret < 0 )
            ret = ERROR_FAIL;
    }

    if( ERROR( ret ) )
        return ret;

    do{
        gint32 ret1 = 0;

        ret1 = SSL_do_handshake( m_pSSL );
        ret = GetSSLError( m_pSSL, ret1 );
        if( ERROR( ret ) )
            break;

        if( SUCCEEDED( ret ) )
        {
            // the handshake is done
        }
        else if( !IsWantIo( ret ) )
        {
            if( ret > 0 )
            {
                ret = -ret;
                DebugPrint( ret,
                    "Unknown error code" );
            }
            break;
        }

        bool bRead = true;
        if( ret != SSL_ERROR_WANT_READ )
            bRead = false;

        // check if there is something to write
        BufPtr pBuf( true );
        pBuf->Resize( PAGE_SIZE );
        guint32 dwOffset = 0;

        do{
            ret = BIO_read( m_pwbio,
                pBuf->ptr() + dwOffset,
                pBuf->size() - dwOffset );

            if( ret > 0 )
            {
                dwOffset += ret;
                if( dwOffset == pBuf->size() )
                {
                    pBuf->Resize(
                        dwOffset + PAGE_SIZE );
                }
                continue;
            }
            else if( BIO_should_retry( m_pwbio ) )
            {
                // no data at this time
                ret = 0;
                pBuf->Resize( dwOffset );
                break;
            }
            ret = ERROR_FAIL;
            break;

        }while( 1 );

        if( ERROR( ret ) )
            break;

        if( !pBuf->empty() )
        {
            ret = SendWriteReq( pCallback, pBuf );
            if( ret == STATUS_PENDING )
                break;

            if( ERROR( ret ) )
                break;
        }

        if( !bRead )
            break;

        ret = SendListenReq(
            pCallback, pHandshake );

        if( ERROR( ret ) )
            break;

        if( ret == STATUS_PENDING )
            break;

        gint32 iRet = BIO_write( m_prbio,
            pHandshake->ptr(),
            pHandshake->size() );

        if( iRet < 0 )
        {
            ret = ERROR_FAIL;
            break;
        }

        // handshake is received, repeat
 
    }while( 1 );

    return ret;
}

gint32 CRpcOpenSSLFido::DoHandshake(
     IEventSink* pCallback,
     BufPtr& pHandshake )
{
    if( SSL_is_init_finished( m_pSSL ) )
        return 0;

    if( pHandshake.IsEmpty() )
        DebugPrint( 0, "Start SSL handshake..." );
    else
        DebugPrint( 0, "Continue SSL handshake..." );

    gint32 ret = AdvanceHandshake(
        pCallback, pHandshake );

    return ret;
}

gint32 CRpcOpenSSLFido::AdvanceShutdown(
     IEventSink* pCallback,
     BufPtr& pHandshake )
{
    gint32 ret = 0;

    if( !pHandshake.IsEmpty() &&
        !pHandshake->empty() )
    {
        ret = BIO_write( m_prbio,
            pHandshake->ptr(),
            pHandshake->size() );
        if( ret < 0 )
            ret = ERROR_FAIL;
    }

    if( ERROR( ret ) )
        return ret;

    gint32 iShutdown = 0;
    do{
        bool bRead = true;
        iShutdown = SSL_shutdown( m_pSSL );
        if( iShutdown < 0 )
        {
            ret = GetSSLError(
                m_pSSL, iShutdown );

            if( ERROR( ret ) )
                break;

            if( ret == SSL_ERROR_WANT_WRITE )
                bRead = false;
        }

        // check if there is something to write
        BufPtr pBuf( true );
        pBuf->Resize( PAGE_SIZE );
        guint32 dwOffset = 0;

        do{
            ret = BIO_read( m_pwbio,
                pBuf->ptr() + dwOffset,
                pBuf->size() - dwOffset );

            if( ret > 0 )
            {
                dwOffset += ret;
                if( dwOffset == pBuf->size() )
                {
                    pBuf->Resize(
                        dwOffset + PAGE_SIZE );
                }
                continue;
            }
            else if( BIO_should_retry( m_pwbio ) )
            {
                // no data or no more data
                ret = 0;
                if( dwOffset < pBuf->size() )
                    pBuf->Resize( dwOffset );
                break;
            }
            ret = ERROR_FAIL;
            break;

        }while( 1 );

        if( ERROR( ret ) )
            break;

        if( dwOffset > 0 )
        {
            ret = SendWriteReq( pCallback, pBuf );
            if( ret == STATUS_PENDING )
                break;

            if( ERROR( ret ) )
                break;
        }

        if( iShutdown == 1 )
        {
            // shutdown is done
            ret = 0;
            break;
        }

        if( iShutdown < 0 && !bRead )
            break;

        ret = SendListenReq(
            pCallback, pHandshake );

        if( ERROR( ret ) )
            break;

        if( ret == STATUS_PENDING )
            break;

        gint32 iRet = BIO_write( m_prbio,
            pHandshake->ptr(),
            pHandshake->size() );

        if( iRet < 0 )
        {
            ret = ERROR_FAIL;
            break;
        }

        // handshake is received, repeat
 
    }while( 1 );

    return ret;
}

gint32 CRpcOpenSSLFido::DoShutdown(
     IEventSink* pCallback,
     BufPtr& pHandshake )
{
    if( !SSL_is_init_finished( m_pSSL ) )
        return 0;

    DebugPrint( 0, "Start shutdown..." );
    // NOTE: at this moment, the lower port is
    // still in `READY' state, and able to accept
    // io requests.
    return AdvanceShutdown(
        pCallback, pHandshake );
}

gint32 CRpcOpenSSLFido::ResumeWriteTasks()
{
    gint32 ret = 0; 
    do{
        CStdRMutex oPortLock( GetLock() );
        if( m_dwRenegStat != rngstatResumeWrite )
            break;

        if( m_queWriteTasks.empty() )
        {
            ret = 0;
            m_dwRenegStat = rngstatNormal;
            break;
        }

        TaskletPtr pTask =
            m_queWriteTasks.front();

        if( unlikely( pTask.IsEmpty() ) )
        {
            m_queWriteTasks.pop_front();
            continue;
        }
        oPortLock.Unlock();

        CCfgOpenerObj oTaskCfg(
            ( CObjBase* )pTask );

        oTaskCfg.RemoveProperty(
            PROP_TASK_QUEUED ); 

        ret = ( *pTask )( eventZero );

        bool bQueued = false;
        oTaskCfg.GetBoolProp(
            PROP_TASK_QUEUED, bQueued ); 

        // the request has sent down, move on
        // to the next task
        oPortLock.Lock();

        if( bQueued && ret == STATUS_PENDING )
        {
            // waiting for another incoming events
            // to happen
            m_dwRenegStat = rngstatWantRead;
            ret = 0;
            break;
        }
        if( unlikely( pTask !=
            m_queWriteTasks.front() ) )
        {
            ret = ERROR_STATE;
            break;
        }

        m_queWriteTasks.pop_front();
        if( ret == STATUS_PENDING )
        {
            // remove the task manually if the irp
            // has sent down.
            CCfgOpenerObj oCfg(
                ( CObjBase* )pTask );
            oCfg.RemoveProperty( propIrpPtr );
            ( *pTask )( eventCancelTask );
            ret = 0;
        }

    }while( 1 );

    return ret;
}

gint32 CRpcOpenSSLFido::RemoveIrpFromMap(
    PIRP pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    CStdRMutex oPortLock( GetLock() );
    std::deque< TaskletPtr >::iterator itr =
        m_queWriteTasks.begin();

    COpenSSLResumeWriteTask* pTask = nullptr;
    for( ; itr != m_queWriteTasks.end(); ++itr )
    {
        COpenSSLResumeWriteTask* pTask = *itr;
        IrpPtr pPendingIrp;
        ret = pTask->GetIrp( pPendingIrp );
        if( ERROR( ret ) )
            continue;

        if( pIrp == pPendingIrp )
            break;
    }

    if( itr == m_queWriteTasks.end() )
        return -ENOENT;

    TaskletPtr pBackup = *itr;
    m_queWriteTasks.erase( itr );
    if( m_dwRenegStat != rngstatNormal &&
        m_queWriteTasks.empty() )
        m_dwRenegStat = rngstatNormal;

    oPortLock.Unlock();
    pTask->RemoveIrp();
    ( *pTask )( eventCancelTask );

    return 0;
}

gint32 CRpcOpenSSLFido::CancelFuncIrp(
    IRP* pIrp, bool bForce )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    do{
        CStdRMutex oPortLock( GetLock() );
        RemoveIrpFromMap( pIrp );

    }while( 0 );

    return super::CancelFuncIrp(
        pIrp, bForce );
}

gint32 CRpcOpenSSLFido::AllocIrpCtxExt(
    IrpCtxPtr& pIrpCtx,
    void* pContext ) const
{
    gint32 ret = 0;
    switch( pIrpCtx->GetMajorCmd() )
    {
    case IRP_MJ_PNP:
        {
            switch( pIrpCtx->GetMinorCmd() )
            {
            case IRP_MN_PNP_STOP:
                {
                    BufPtr pBuf( true );
                    if( ERROR( ret ) )
                        break;

                    pBuf->Resize( sizeof( guint32 ) +
                        sizeof( PORT_START_STOP_EXT ) );

                    memset( pBuf->ptr(),
                        0, pBuf->size() ); 

                    pIrpCtx->SetExtBuf( pBuf );
                    break;
                }
            default:
                ret = -ENOTSUP;
                break;
            }
            break;
        }
    default:
        {
            ret = -ENOTSUP;
            break;
        }
    }

    if( ret == -ENOTSUP )
    {
        // let's try it on CPort
        ret = super::AllocIrpCtxExt(
            pIrpCtx, pContext );
    }

    return ret;
}

gint32 CRpcOpenSSLFidoDrv::Start()
{
    gint32 ret = LoadSSLSettings();
    if( ERROR( ret ) )
        return ret;
    return super::Start();
}

gint32 CRpcOpenSSLFidoDrv::Probe(
    IPort* pLowerPort,
    PortPtr& pNewPort,
    const IConfigDb* pConfig )
{
    gint32 ret = 0;
    do{
        if( pLowerPort == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        bool bServer = false;

        CPort* pPort =
            static_cast< CPort* >( pLowerPort );

        PortPtr pPdoPort;
        ret = pPort->GetPdoPort( pPdoPort );
        if( ERROR( ret ) )
            break;

        CCfgOpenerObj oPdoPort(
            ( CObjBase* )pPdoPort );

        IConfigDb* pConnParams = nullptr;
        ret = oPdoPort.GetPointer(
            propConnParams, pConnParams );
        if( ERROR( ret ) )
            break;

        bool bEnableSSL = false;
        CConnParams oConn( pConnParams );
        bEnableSSL = oConn.IsSSL();
        if( !bEnableSSL )
        {
            ret = 0;
            pNewPort = pLowerPort;
            break;
        }

        bServer = oConn.IsServer();
        if( m_pSSLCtx == nullptr )
        {
            InitSSLContext( bServer );
        }

        std::string strPdoClass;
        CCfgOpenerObj oCfg( pLowerPort );
        ret = oCfg.GetStrProp(
            propPdoClass, strPdoClass );

        if( ERROR( ret ) )
            break;

        std::string strExpPdo =
            PORT_CLASS_TCP_STREAM_PDO2;

        if( strPdoClass != strExpPdo )
        {
            // this is not a port we support
            ret = -ENOTSUP;
            break;
        }

        CParamList oNewCfg;
        oNewCfg[ propPortClass ] =
            PORT_CLASS_OPENSSL_FIDO;

        oNewCfg.SetIntProp(
            propPortId, NewPortId() );

        oNewCfg.CopyProp(
            propConnParams, pLowerPort );

        oNewCfg.SetPointer(
            propIoMgr, GetIoMgr() );

        oNewCfg.SetPointer(
            propDrvPtr, this );

        SSL_CTX_up_ref( m_pSSLCtx );
        oNewCfg.SetIntPtr(
            propSSLCtx,
            ( guint32* )m_pSSLCtx );

        ret = CreatePort( pNewPort,
            oNewCfg.GetCfg() );

        if( ERROR( ret ) )
            break;

        ret = pNewPort->AttachToPort(
            pLowerPort );

        if( ERROR( ret ) )
            break;

    }while( 0 );

    return ret;
}

extern bool IsVerifyPeerEnabled(
    const Json::Value& oValue,
    const stdstr& strPortClass );

gint32 CRpcOpenSSLFidoDrv::LoadSSLSettings()
{
    CIoManager* pMgr = GetIoMgr();
    CDriverManager& oDrvMgr = pMgr->GetDrvMgr();
    Json::Value& ojc = oDrvMgr.GetJsonCfg();
    Json::Value& oPorts = ojc[ JSON_ATTR_PORTS ];

    if( oPorts == Json::Value::null )
        return -ENOENT;

    if( !oPorts.isArray() || oPorts.size() == 0 )
        return -ENOENT;

    gint32 ret = 0;
    do{
        for( guint32 i = 0; i < oPorts.size(); i++ )
        {
            Json::Value& elem = oPorts[ i ];
            if( elem == Json::Value::null )
                continue;

            std::string strPortClass =
                elem[ JSON_ATTR_PORTCLASS ].asString();
            if( strPortClass != PORT_CLASS_OPENSSL_FIDO )
                continue;


            if( !( elem.isMember( JSON_ATTR_PARAMETERS ) &&
                elem[ JSON_ATTR_PARAMETERS ].isObject() ) )
            {
                ret = -ENOENT;
                break;
            }

            Json::Value& oParams =
                elem[ JSON_ATTR_PARAMETERS ];

            // certificate file path
            if( oParams.isMember( JSON_ATTR_CERTFILE ) &&
                oParams[ JSON_ATTR_CERTFILE ].isString() )
            {
                std::string strCert =
                    oParams[ JSON_ATTR_CERTFILE ].asString();
                ret = access( strCert.c_str(), R_OK );
                if( ret == -1 )
                {
                    ret = -errno;
                    DebugPrintEx( logErr, ret,
                        "Error cannot find ssl certificate '%s'",
                        strCert.c_str() );
                    break;
                }
                m_strCertPath = strCert;
            }

            // private key file path
            if( oParams.isMember( JSON_ATTR_KEYFILE ) &&
                oParams[ JSON_ATTR_KEYFILE ].isString() )
            {
                std::string strKeyFile =
                    oParams[ JSON_ATTR_KEYFILE ].asString();

                ret = access( strKeyFile.c_str(), R_OK );
                if( ret == -1 )
                {
                    ret = -errno;
                    DebugPrintEx( logErr, ret,
                        "Error cannot find ssl key '%s'",
                        strKeyFile.c_str() );
                    break;
                }
                m_strKeyPath = strKeyFile;
            }

            // certificate file path
            if( oParams.isMember( JSON_ATTR_CACERT ) &&
                oParams[ JSON_ATTR_CACERT ].isString() )
            {
                std::string strCAFile =
                    oParams[ JSON_ATTR_CACERT ].asString();
                if( strCAFile.size() )
                {
                    ret = access( strCAFile.c_str(), R_OK );
                    if( ret == -1 )
                    {
                        ret = -errno;
                        DebugPrintEx( logErr, ret,
                            "Error cannot find ssl CA certificate '%s'",
                            strCAFile.c_str() );
                        break;
                    }
                    m_strCAFile = strCAFile;
                }
            }

            // secret file path
            if( oParams.isMember( JSON_ATTR_SECRET_FILE ) &&
                oParams[ JSON_ATTR_SECRET_FILE ].isString() )
            {
                // either specifying secret file or prompting password
                std::string strPath =
                    oParams[ JSON_ATTR_SECRET_FILE ].asString();

                if( strPath.empty() || strPath == "1234" )
                {
                    m_strSecretPath.clear(); 
                }
                else
                {
                    m_strSecretPath = strPath;
                    if( strPath != "console" )
                    {
                        ret = access( strPath.c_str(), R_OK );
                        if( ret == -1 )
                        {
                            ret = -errno;
                            DebugPrintEx( logErr, ret,
                                "Error cannot find the secret file '%s'",
                                strPath.c_str() );
                            break;
                        }
                        m_strSecretPath = strPath;
                    }
                }
            }
        }
        if( IsVerifyPeerEnabled(
            ojc, PORT_CLASS_OPENSSL_FIDO ) )
            m_bVerifyPeer = true;

    }while( 0 );

    return ret;
}

CRpcOpenSSLFidoDrv::CRpcOpenSSLFidoDrv(
    const IConfigDb* pCfg ) :
    super( pCfg )
{
    SetClassId( clsid( CRpcOpenSSLFidoDrv ) );
}

CRpcOpenSSLFidoDrv::~CRpcOpenSSLFidoDrv()
{
    if( m_pSSLCtx != nullptr )
    {
        SSL_CTX_free( m_pSSLCtx );
        m_pSSLCtx = nullptr;
    }
}

gint32 COpenSSLHandshakeTask::OnTaskComplete(
    gint32 iRet )
{
    gint32 ret = 0;

    CParamList oParams(
        ( IConfigDb* )GetConfig() );
    do{
        IRP* pMasterIrp = nullptr;        

        // the irp pending on poststart
        ret = oParams.GetPointer(
            propIrpPtr, pMasterIrp );

        if( ERROR( ret ) )
            break;

        CIoManager* pMgr = nullptr;
        ret = oParams.GetPointer(
            propIoMgr, pMgr );

        if( ERROR( ret ) )
            break;

        CStdRMutex oIrpLock(
            pMasterIrp->GetLock() );

        ret = pMasterIrp->CanContinue(
            IRP_STATE_READY );

        if( ERROR( ret ) )
            break;

        IrpCtxPtr& pCtx =
            pMasterIrp->GetTopStack();

        pCtx->SetStatus( iRet );
        oIrpLock.Unlock();

        pMgr->CompleteIrp( pMasterIrp );
        DebugPrint( iRet, "SSL Handshake done" );

    }while( 0 );

    oParams.RemoveProperty( propIrpPtr );
    oParams.RemoveProperty( propPortPtr );

    return ret;
}

gint32 COpenSSLHandshakeTask::OnIrpComplete(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CRpcOpenSSLFido* pPort = nullptr;
        CParamList oParams(
            ( IConfigDb* )GetConfig() );

        ret = oParams.GetPointer(
            propPortPtr, pPort );

        if( ERROR( ret ) )
            break;

        ret = pIrp->GetStatus();
        if( ERROR( ret ) )
            break;

        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        guint32 dwIoDir = pCtx->GetIoDirection();

        BufPtr pHandshake;
        if( dwIoDir == IRP_DIR_IN )
        {
            IrpCtxPtr& pCtx = pIrp->GetTopStack();
            BufPtr pBuf = pCtx->m_pRespData;
            if( pBuf.IsEmpty() || pBuf->empty() )
            {
                ret = -EFAULT;
                break;
            }

            STREAM_SOCK_EVENT* psse = 
                ( STREAM_SOCK_EVENT* ) pBuf->ptr();

            if( psse->m_iEvent == sseRetWithBuf )
            {
                pHandshake = psse->m_pInBuf;
                psse->m_pInBuf.Clear();
            }
            else if( psse->m_iEvent == sseError )
            {
                ret = psse->m_iData;
                if( ret == -ENOTCONN )
                    ret = -ECONNRESET;
                break;
            }
            else
            {
                ret = -ENOTSUP;
                break;
            }
        }

        ret = pPort->DoHandshake(
            this, pHandshake );

    }while( 0 );

    if( ret != STATUS_PENDING )
        ret = OnTaskComplete( ret );

    return ret;
}

gint32 COpenSSLHandshakeTask::RunTask()
{
    gint32 ret = 0;
    do{
        CRpcOpenSSLFido* pPort = nullptr;
        CParamList oParams(
            ( IConfigDb* )GetConfig() );

        ret = oParams.GetPointer(
            propPortPtr, pPort );

        if( ERROR( ret ) )
            break;

        BufPtr pEmptyBuf;
        ret = pPort->DoHandshake(
            this, pEmptyBuf );

    }while( 0 );

    if( ret != STATUS_PENDING )
        ret = OnTaskComplete( ret );

    return ret;
}

gint32 COpenSSLShutdownTask::RunTask()
{
    gint32 ret = 0;
    do{
        CRpcOpenSSLFido* pPort = nullptr;
        CParamList oParams(
            ( IConfigDb* )GetConfig() );

        ret = oParams.GetPointer(
            propPortPtr, pPort );

        if( ERROR( ret ) )
            break;

        BufPtr pEmptyBuf;
        ret = pPort->DoShutdown(
            this, pEmptyBuf );

    }while( 0 );

    if( ret != STATUS_PENDING )
        ret = OnTaskComplete( 0 );

    return ret;
}

gint32 COpenSSLShutdownTask::OnIrpComplete(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CRpcOpenSSLFido* pPort = nullptr;
        CParamList oParams(
            ( IConfigDb* )GetConfig() );

        ret = oParams.GetPointer(
            propPortPtr, pPort );

        if( ERROR( ret ) )
            break;

        ret = pIrp->GetStatus();
        if( ERROR( ret ) )
            break;

        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        guint32 dwIoDir = pCtx->GetIoDirection();

        BufPtr pHandshake;
        if( dwIoDir == IRP_DIR_IN )
        {
            IrpCtxPtr& pCtx = pIrp->GetTopStack();
            BufPtr pBuf = pCtx->m_pRespData;
            if( pBuf.IsEmpty() || pBuf->empty() )
            {
                ret = -EFAULT;
                break;
            }

            STREAM_SOCK_EVENT* psse = 
                ( STREAM_SOCK_EVENT* ) pBuf->ptr();

            if( psse->m_iEvent == sseRetWithBuf )
            {
                pHandshake = psse->m_pInBuf;
                psse->m_pInBuf.Clear();
            }
            else if( psse->m_iEvent == sseError )
            {
                ret = psse->m_iData;
                break;
            }
            else
            {
                ret = -ENOTSUP;
                break;
            }
        }

        ret = pPort->DoShutdown(
            this, pHandshake );

    }while( 0 );

    if( ret != STATUS_PENDING )
        ret = OnTaskComplete( 0 );

    return ret;
}

gint32 COpenSSLShutdownTask::OnTaskComplete(
    gint32 iRet )
{
    gint32 ret = 0;
    CParamList oParams(
        ( IConfigDb* )GetConfig() );
    do{
        IRP* pMasterIrp = nullptr;        

        // the irp pending on poststart
        ret = oParams.GetPointer(
            propIrpPtr, pMasterIrp );

        if( ERROR( ret ) )
            break;

        CIoManager* pMgr = nullptr;
        ret = oParams.GetPointer(
            propIoMgr, pMgr );

        if( ERROR( ret ) )
            break;

        CStdRMutex oIrpLock(
            pMasterIrp->GetLock() );

        ret = pMasterIrp->CanContinue(
            IRP_STATE_READY );

        if( ERROR( ret ) )
            break;

        IrpCtxPtr& pCtx =
            pMasterIrp->GetTopStack();

        pCtx->SetStatus( iRet );
        oIrpLock.Unlock();

        pMgr->CompleteIrp( pMasterIrp );

    }while( 0 );

    oParams.RemoveProperty( propIrpPtr );
    oParams.RemoveProperty( propPortPtr );

    return ret;
}

gint32 COpenSSLResumeWriteTask::GetIrp(
    IrpPtr& pIrp )
{
    ObjPtr pObj;
    CCfgOpenerObj oTaskCfg( this );
    gint32 ret = oTaskCfg.GetObjPtr(
        propIrpPtr, pObj );

    if( ERROR( ret ) )
        return ret;

    pIrp = pObj;
    if( pIrp.IsEmpty() )
        return -EFAULT;

    return 0;
}

gint32 COpenSSLResumeWriteTask::RemoveIrp()
{
    CCfgOpenerObj oTaskCfg( this );
    return oTaskCfg.RemoveProperty( propIrpPtr );
}
 
gint32 COpenSSLResumeWriteTask::RunTask()
{
    gint32 ret = 0;
    do{
        CParamList oTaskCfg(
            ( IConfigDb* )GetConfig() );

        guint32 dwIdx = 0;
        guint32 dwOffset = 0;
        guint32 dwTotal = 0;
        ret = oTaskCfg.GetIntProp( 0, dwIdx );
        if( ERROR( ret ) )
            break;

        ret = oTaskCfg.GetIntProp( 1, dwOffset );
        if( ERROR( ret ) )
            break;

        ret = oTaskCfg.GetIntProp( 2, dwTotal );
        if( ERROR( ret ) )
            break;

        CRpcOpenSSLFido* pPort = nullptr;
        ret = oTaskCfg.GetPointer(
            propPortPtr, pPort );
        if( ERROR( ret ) )
            break;

        IRP* pIrp = nullptr;
        ret = oTaskCfg.GetPointer(
            propIrpPtr, pIrp );
        if( ERROR( ret ) )
            break;

        CStdRMutex oIrpLock( pIrp->GetLock() );
        ret = pIrp->CanContinue( IRP_STATE_READY );
        if( ERROR( ret ) )
            break;

        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        CfgPtr pCfg;
        ret = pCtx->GetReqAsCfg( pCfg );
        if( ERROR( ret ) )
            break;

        DebugPrint( 0,
            "ResumeWriteTask is running..." );

        TaskletPtr pTask( this );
        ret = pPort->EncryptAndSend(
            pIrp, pCfg, ( gint32& )dwIdx,
            dwTotal, dwOffset, true );

        oIrpLock.Unlock();
        CIoManager* pMgr = pPort->GetIoMgr();
        if( ret != STATUS_PENDING )
            pMgr->CompleteIrp( pIrp );

    }while( 0 );

    return ret;
}

}
