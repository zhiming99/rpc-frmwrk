/*
 * =====================================================================================
 *
 *       Filename:  gmsfido.cpp
 *
 *    Description:  functions and class implementations for GmSSL's fido for rpc-frmwrk 
 *
 *        Version:  1.0
 *        Created:  01/26/2023 05:21:11 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2023 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#include "gmsfido.h"

namespace rpcf
{

gint32 BufToIove( BufPtr& pSrc,
    PIOVE& pDest, bool bCopy, bool bTakeOwner )
{
    if( pSrc.IsEmpty() || pSrc->empty() )
        return -EINVAL;

    gint32 ret = 0;
    do{
        if( !pDest )
        {
            pDest = std::make_shared< AGMS_IOVE >
                ( new AGMS_IOVE );
        }
        if( bCopy )
        {
            pDest->clear_content();
            if( pDest->empty() )
            {
                ret = pDest->alloc( pSrc->size() );
                if( ERROR( ret ) )
                    break;
            }
            else
            {
                if( pDest->size() < pSrc->size() )
                {
                    ret = pDest->realloc(
                        pSrc->size() );
                    if( ERROR( ret ) )
                        break;
                }
            }
            memcpy( pDest->begin(),
                pSrc->ptr(), pSrc->size() );
            break;
        }
        else
        {
            char* pmem = nullptr;
            guint32 dwStart = 0;
            guint32 dwEnd = 0;
            guint32 dwSize = 0;
            if( bTakeOwner )
            {
                if( pSrc->IsNoFree() )
                {
                    ret = -EFAULT;
                    break;
                }
                ret = pSrc->Detach( pmem,
                    dwSize, dwStart, dwEnd );
                if( ret == -EACCES )
                {
                    ret = 0;
                    bCopy = true;
                    continue;
                }
                else if( ERROR( ret ) )
                    break;
            }
            else
            {
                pmem = pSrc->ptr();
                dwSize = pSrc->size();
                pDest->set_no_free();
            }

            ret = pDest->attach( pmem,
                dwSize, dwStart, dwEnd );
        }
        break;

    }while( 1 );

    return ret;
}

gint32 IoveToBuf( PIOVE& pSrc,
    BufPtr& pDest, bool bCopy, bool bTakeOwner )
{
    if( !pSrc || pSrc->empty() )
        return -EINVAL;

    gint32 ret = 0;
    do{
        if( pDest.IsEmpty() )
            pDest.NewObj();
        if( bCopy )
        {
            if( pDest->size() < pSrc->size() )
            {
                ret = pDest->Resize( pSrc->size() );
                if( ERROR( ret ) )
                    break;
            }

            memcpy( pDest->ptr(),
                pSrc->begin(), pSrc->size() );

            break;
        }
        else
        {
            uint8_t* pmem = nullptr;
            size_t dwStart = 0;
            size_t dwEnd = 0;
            size_t dwSize = 0;

            if( bTakeOwner )
            {
                pSrc->expose( &pmem,
                    dwSize, dwStart, dwEnd );
                ret = pDest->Attach( pmem,
                    dwSize, dwStart, dwEnd );
                if( ret == -EACCES )
                {
                    bCopy = true;
                    continue;
                }
                else if( ERROR( ret ) )
                    break;
                pSrc->set_no_free();
                pSrc->clear();
            }
            else
            {
                ret = pDest->Attach(
                    pSrc->begin(), pSrc->size() );
                if( ret == -EACCES )
                {
                    bCopy = true;
                    continue;
                }
                else if( ERROR( ret ) )
                    break;
                pDest->SetNoFree();
            }
        }
        break;

    }while( 1 );

    return ret;
}

gint32 CGmSSLHandshakeTask::OnTaskComplete(
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
        DebugPrint( iRet, "GmSSL Handshake done" );

    }while( 0 );

    oParams.RemoveProperty( propIrpPtr );
    oParams.RemoveProperty( propPortPtr );

    return ret;
}

gint32 CGmSSLHandshakeTask::OnIrpComplete(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CRpcGmSSLFido* pPort = nullptr;
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

gint32 CGmSSLHandshakeTask::RunTask()
{
    gint32 ret = 0;
    do{
        CRpcGmSSLFido* pPort = nullptr;
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

gint32 CRpcGmSSLFido::PostStart(
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

        if( !m_pSSL )
        {
            m_pSSL = std::move(
                std::unique_ptr( new TLS13 ) );
        }

        ret = m_pSSL->init( m_bClient );
        if( ERROR( ret ) )
            break;

        // hold the irp and start the handshake
        // task
        ret = StartSSLHandshake( pIrp );
        break;

    }while( 0 );

    return ret;
}

gint32 CRpcGmSSLFido::StartSSLHandshake(
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
            clsid( CGmSSLHandshakeTask ),
            oParams.GetCfg() );

        if( SUCCEEDED( ret ) )
            ret = STATUS_PENDING;

    }while( 0 );

    return ret;
}
gint32 CRpcGmSSLFido::DoHandshake(
     IEventSink* pCallback,
     BufPtr& pHandshake )
{

    DebugPrint( 0, "Start GmSSL handshake..." );
    return AdvanceHandshake(
        pCallback, pHandshake );
}

gint32 CRpcGmSSLFido::AdvanceHandshake(
     IEventSink* pCallback,
     BufPtr& pHandshake )
{
    return AdvanceHandshakeInternal(
        pCallback, pHandshake, false );
}

gint32 CRpcGmSSLFido::SendImmediateResp()
{
    gint32 ret = 0;
    do{
        CStdRMutex oLock( GetLock() );
        if( m_pSSL->write_bio.size() )
        {
            PIOVE piove;
            CParamList oParams;
            guint32 dwSize = 0;
            do{
                BufPtr pBuf;
                ret = m_pSSL->write_bio.read(
                    piove );
                if( ERROR( ret ) )
                {
                    ret = 0;
                    break;
                }

                // transfer the buffer ownership
                ret = IoveToBuf( piove, pBuf,
                    false, true );

                if( ERROR( ret ) )
                    break;

                dwSize += piove->size();
                oParams.Push( pBuf );

            }while( ( bool )piove )

            oLock.Unlock();

            if( ERROR( ret ) )
                break;

            if( dwSize > MAX_BYTES_PER_TRANSFER ||
                dwSize == 0 )
            {
                // should not be empty or too big
                ret = -ERANGE;
                break;
            }

            BufPtr pBuf( true );
            *pBuf = ObjPtr( oParams.GetCfg() );

            TaskletPtr pDummy;
            pDummy.NewObj( clsid( CIfDummyTask ) );
            ret = SendWriteReq( pDummy, pBuf );
        }

    }while( 0 );

    return ret;
}

gint32 CRpcGmSSLFido::AdvanceHandshakeInternal(
     IEventSink* pCallback,
     BufPtr& pHandshake,
     bool bShutdown )
{
    gint32 ret = 0;

    do{
        CStdRMutex oLock( GetLock() );

        if( !pHandshake.IsEmpty() &&
            !pHandshake->empty() )
        {
            ret = BufToIove( pHandshake,
                pInput, false, false );

            if( ERROR( ret ) )
                break;

            ret = m_pSSL->read_bio.write(
                pInput );

            if( ERROR( ret ) )
                break;
        }

        gint32 iRet = 0;

        if( bShutdown ) 
            iRet = m_pSSL->shutdown();
        else
            iRet = m_pSSL->handshake();

        oLock.Unlock();

        ret = SendImmediateResp();

        if( !ERROR( iRet ) && ERROR( ret ) )
            iRet = ret;

        if( ERROR( iRet ) )
        {
            ret = iRet;
            break;
        }

        if( SUCCEEDED( ret ) )
        {
            // handshake done
            break;
        }

        ret = SendListenReq(
            pCallback, pHandshake );

        if( ERROR( ret ) )
            break;

        if( ret == STATUS_PENDING )
            break;

        // handshake received, repeat
 
    }while( 1 );

    return ret;
}

gint32 CRpcGmSSLFido::PreStop(
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

gint32 CRpcGmSSLFido::AdvanceShutdown(
     IEventSink* pCallback,
     BufPtr& pHandshake )
{
    return AdvanceHandshakeInternal(
        pCallback, pHandshake, true );
    return ret;
}

gint32 CRpcGmSSLFido::DoShutdown(
     IEventSink* pCallback,
     BufPtr& pHandshake )
{
    DebugPrint( 0, "Start shutdown..." );
    return AdvanceShutdown(
        pCallback, pHandshake );
}

gint32 CGmSSLShutdownTask::RunTask()
{
    gint32 ret = 0;
    do{
        CRpcGmSSLFido* pPort = nullptr;
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
        OnTaskComplete( ret );

    return ret;
}

gint32 CGmSSLShutdownTask::OnIrpComplete(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CRpcGmSSLFido* pPort = nullptr;
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
        OnTaskComplete( ret );

    return ret;
}

gint32 CGmSSLShutdownTask::OnTaskComplete(
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
        DebugPrint( iRet, "GmSSL Shutdown done" );

    }while( 0 );

    oParams.RemoveProperty( propIrpPtr );
    oParams.RemoveProperty( propPortPtr );

    return ret;
}

gint32 CRpcGmSSLFido::StartSSLShutdown(
    PIRP pIrp )
{
    gint32 ret = 0;
    do{
        if( !m_pSSL->is_ready() )
            break;

        CParamList oParams;
        CIoManager* pMgr = GetIoMgr();
        oParams.SetPointer( propIoMgr, pMgr );
        oParams.SetPointer( propIrpPtr,  pIrp );
        oParams.SetPointer( propPortPtr, this );
        oParams.SetIntProp( propTimeoutSec,
            PORT_START_TIMEOUT_SEC / 2 );

        ret = pMgr->ScheduleTask(
            clsid( CGmSSLShutdownTask ),
            oParams.GetCfg() );

        if( SUCCEEDED( ret ) )
            ret = STATUS_PENDING;

    }while( 0 );

    return ret;
}

gint32 CRpcGmSSLFido::OnSubmitIrp(
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

gint32 CRpcGmSSLFido::CompleteListeningIrp(
    IRP* pIrp )
{
    gint32 ret = 0;
    bool bSSLErr = false;

    do{

        IrpCtxPtr pCtx = pIrp->GetCurCtx();
        IrpCtxPtr pTopCtx = pIrp->GetTopStack();

        BufPtr pInBuf = pTopCtx->m_pRespData;
        if( unlikely( ppInBuf.IsEmpty() ) )
        {
            pIrp->PopCtxStack();
            ret = -EFAULT;
            pCtx->SetStatus( ret );
            break;
        }

        pIrp->PopCtxStack();

        STREAM_SOCK_EVENT* psse =
        ( STREAM_SOCK_EVENT* )pInBuf->ptr();

        if( unlikely(
            psse->m_iEvent == sseError ) )
        {
            pCtx->SetRespData( pInBuf );
            pCtx->SetStatus( STATUS_SUCCESS );
            break;
        }
        else if( unlikely(
            psse->m_iEvent == sseRetWithFd ||
            psse->m_iEvent == sseInvalid ) )
        {
            psse->m_iEvent = sseError;
            psse->m_iData = -ENOTSUP;
            psse->m_iEvtSrc = GetClsid();
            pCtx->SetRespData( pInBuf );
            pCtx->SetStatus( STATUS_SUCCESS );
            break;
        }
        else if( unlikely(
            psse->m_iEvent != sseRetWithBuf ) )
        {
            psse->m_iEvent = sseError;
            psse->m_iData = -ENOTSUP;
            psse->m_iEvtSrc = GetClsid();
            pCtx->SetRespData( pInBuf );
            pCtx->SetStatus( STATUS_SUCCESS );
            break;
        }

        PIOVE piove;
        BufPtr pRespBuf = psse->m_pInBuf;

        bSSLErr = true;
        ret = BufToIove( pRespBuf,
            piove, false, false );

        if( ERROR( ret ) )
            break;

        IOV iov;

        CStdRMutex oPortLock( GetLock() );

        ret = m_pSSL->read_bio.write(
            piove );

        if( ERROR( ret ) )
            break;

        ret = m_pSSL->recv( iov );

        oPortLock.Unlock();

        SendImmediateResp();

        if( ERROR( ret ) )
            break;

        if( ret == STATUS_PENDING && iov.empty() )
            break;

        pRespBuf.Clear();
        pRespBuf.NewObj();
        for( auto& elem : iov )
        {
            if( pRespBuf->empty() )
            {
                ret = IoveToBuf( elem,
                    pRespBuf, no, true );

                if( ERROR( ret ) )
                    break;

                continue;
            }
            BufPtr pBuf;
            ret = IoveToBuf(
                elem, pBuf, no, true );
            if( ERROR( ret ) )
                break;
            ret = pRespBuf->Append( pBuf );
            if( ERROR( ret ) )
                break;
        }

        if( ERROR( ret ) )
            break;

        pInBuf.Clear();
        pInBuf.NewObj();
        pInBuf->Resize(
            sizeof( STREAM_SOCK_EVENT ) );

        psse = ( STREAM_SOCK_EVENT* )
            pInBuf->ptr();

        psse->m_iEvent = sseRetWithBuf;
        psse->m_pInBuf = pRespBuf;
        psse->m_iEvtSrc = GetClsid();

        pCtx->SetRespData( pInBuf );
        pCtx->SetStatus( ret );

    }while( 0 );

    if( ERROR( ret ) && bSSLErr )
        pCtx->SetStatus( ret );

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
                ret = CompleteListeningIrp( pIrp );
                while( ret == STATUS_PENDING )
                {
                    IrpCtxPtr pCtx =
                        pIrp->GetTopStack();

                    PortPtr pLowerPort = GetLowerPort();
                    ret = pIrp->AllocNextStack(
                        pLowerPort, IOSTACK_ALLOC_COPY );

                    if( ERROR( ret ) )
                        break;

                    IrpCtxPtr pTopCtx =
                        pIrp->GetTopStack();

                    IrpCtxPtr pTopCtx =
                        pIrp->GetTopStack();

                    pLowerPort->AllocIrpCtxExt(
                        pTopCtx, pIrp );

                    ret = pLowerPort->SubmitIrp( pIrp );
                    if( ret == STATUS_PENDING )
                        break;

                    if( ERROR( ret ) )
                    {
                        pIrp->PopCtxStack();
                        pCtx->SetStatus( ret );
                        break;
                    }
                }
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


gint32 CRpcGmSSLFido::CompleteFuncIrp( IRP* pIrp )
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

gint32 CRpcGmSSLFido::CompleteWriteIrp(
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

gint32 CRpcGmSSLFido::SubmitIoctlCmd(
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
            do{
                IrpCtxPtr pCtx =
                    pIrp->GetTopStack();

                PortPtr pLowerPort = GetLowerPort();
                ret = pIrp->AllocNextStack(
                    pLowerPort, IOSTACK_ALLOC_COPY );

                if( ERROR( ret ) )
                    break;

                IrpCtxPtr pTopCtx =
                    pIrp->GetTopStack();

                IrpCtxPtr pTopCtx =
                    pIrp->GetTopStack();

                pLowerPort->AllocIrpCtxExt(
                    pTopCtx, pIrp );

                ret = pLowerPort->SubmitIrp( pIrp );
                if( ret == STATUS_PENDING )
                    break;

                if( ERROR( ret ) )
                {
                    pIrp->PopCtxStack();
                    pCtx->SetStatus( ret );
                    break;
                }
                
                ret = CompleteListeningIrp( pIrp );
                if( ret == STATUS_PENDING )
                    continue;

                break;

            }while( 1 );

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

}
