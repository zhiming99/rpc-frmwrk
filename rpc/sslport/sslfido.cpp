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
#include "ifhelper.h"
#include "dbusport.h"
#include "iftasks.h"
#include "sslfido.h"
#include "uxport.h"
#include "jsondef.h"

#define PROP_TASK_QUEUED 10

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
    TaskletPtr& pTask, PIRP pIrp, CfgPtr pCfg,
    gint32& iIdx, guint32& dwTotal,
    guint32& dwOffset )
{
    gint32 ret = 0;

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
            ret = oParams.GetProperty( iIdx, pBuf );
            if( ERROR( ret ) )
                break;


            gint32 n = 0;
            n = SSL_write( m_pSSL,
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
                CCfgOpenerObj oTaskCfg(
                    ( CObjBase* )pTask );
                oTaskCfg.SetIntProp( 0, iIdx );
                oTaskCfg.SetIntProp( 1, dwOffset );
                oTaskCfg.SetIntProp( 2, dwTotal );

                oTaskCfg.SetBoolProp(
                    PROP_TASK_QUEUED, true );

                if( m_queWriteTasks.empty() ||
                    m_queWriteTasks.front() != pTask )
                    m_queWriteTasks.push_back( pTask );

                ret = STATUS_PENDING;
                break;
            }
            else if( ret == SSL_ERROR_WANT_WRITE )
            {
                // unknown state
                ret = ERROR_STATE;
                break;
            }

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

                // next buffer
                ret = oParams.GetProperty( iIdx, pBuf );
                if( ERROR( ret ) )
                    break;

                dwOffset = 0;
                n = 0;
                dwTotal += pBuf->size();
            }
            else if( n + dwOffset < pBuf->size() )
            {
                // possible if partial write is
                // enabled
                dwOffset += n;
            }

        }while( 1 );
        oPortLock.Unlock();

        if( ret == STATUS_PENDING )
        {
            break;
        }
        else
        {
            CStdRMutex oPortLock( GetLock() );
            if( !( pTask ==
                m_queWriteTasks.front() ) )
            {
                ret = ERROR_STATE;
                break;
            }
            m_queWriteTasks.pop_front();
        }


        // the least size
        guint32 dwExpSize = std::min(
            ( dwTotal + 256 ),
            ( guint32 )STM_MAX_BYTES_PER_BUF );

        BufPtr pOutBuf( true );
        pOutBuf->Resize( dwExpSize );
        guint32 dwOutOff = 0;

        do{
            ret = BIO_read( m_pwbio,
                pOutBuf->ptr() + dwOutOff,
                pOutBuf->size() - dwOutOff );
            if( ret > 0 )
            {
                dwOutOff += ret;
                if( pOutBuf->size() == dwOutOff )
                {
                    // expand the buffer for
                    // next read
                    pOutBuf->Resize(
                        dwOutOff + PAGE_SIZE );
                }
            }
            else if( !BIO_should_retry( m_pwbio ) )
            {
                // error occurs
                ret = ERROR_FAIL;
                break;
            }

            // so much this time
            ret = 0;
            break;

        }while( 1 );

        if( ERROR( ret ) )
            break;

        // send down the irp
        PortPtr pLowerPort = GetLowerPort();
        pIrp->AllocNextStack(
            pLowerPort, IOSTACK_ALLOC_COPY  );

        CParamList oNewParam;
        oNewParam.Push( pOutBuf );
        IrpCtxPtr pCtx = pIrp->GetTopStack();
        BufPtr pNewBuf( true );
        *pNewBuf = ObjPtr( oNewParam.GetCfg() );
        // overwrite the clear text data with the
        // encrypted copy
        pCtx->SetReqData( pNewBuf );
        ret = pLowerPort->SubmitIrp( pIrp );

        if( ret == STATUS_PENDING )
            break;

        if( ERROR( ret ) )
        {
            pIrp->PopCtxStack();
            IrpCtxPtr pCtx = pIrp->GetTopStack();
            pCtx->SetStatus( ret );
            break;
        }
        
        BufPtr pRespBuf = pCtx->m_pRespData;
        pIrp->PopCtxStack();
        pCtx = pIrp->GetTopStack();
        pCtx->m_pRespData = pRespBuf;
        pCtx->SetStatus( ret );

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

        if( iCount <= 0 )
        {
            ret = -EINVAL;
            break;
        }

        gint32 i = 0;
        guint32 dwOffset = 0;
        guint32 dwTotal = 0;

        CParamList oTaskCfg;
        // save the context and wait for
        // the listening irp to complete
        oTaskCfg.SetPointer(
            propIrpPtr, pIrp );

        oTaskCfg.Push( i );
        oTaskCfg.Push( dwOffset );
        oTaskCfg.Push( dwTotal );

        oTaskCfg.SetPointer(
            propIoMgr, GetIoMgr() );

        oTaskCfg.SetPointer(
            propPortPtr, this );

        TaskletPtr pNewTask;
        ret = pNewTask.NewObj(
            clsid( COpenSSLResumeWriteTask ),
            oTaskCfg.GetCfg() );

        if( ERROR( ret ) )
            break;

        Sem_Wait( &m_semWriteSync );

        CStdRMutex oPortLock( GetLock() );
        if( m_dwRenegStat == rngstatNormal )
        {
            if( m_queWriteTasks.size() > 0 )
            {
                ret = ERROR_STATE;
                break;
            }
            m_queWriteTasks.push_back( pNewTask );
            ret = STATUS_PENDING; 
        }
        oPortLock.Unlock();

        ret = EncryptAndSend( pNewTask, pIrp,
            pCfg, i, dwTotal, dwOffset );

        Sem_Post( &m_semWriteSync );
        if( ret == STATUS_PENDING )
            break;

        COpenSSLResumeWriteTask*
            pResume = pNewTask;
        pResume->RemoveIrp();
        ( *pResume )( eventCancelTask );

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
            // NOTE: the default behavor for
            // filter driver is to
            // pass unknown irps on to the
            // lower driver
            PortPtr pLowerPort = GetLowerPort();
            ret = pIrp->AllocNextStack(
                pLowerPort, IOSTACK_ALLOC_COPY );

            if( ERROR( ret ) )
                break;

            ret = pLowerPort->SubmitIrp( pIrp );
            if( ret == STATUS_PENDING )
                break;

            IrpCtxPtr& pTopCtx =
                pIrp->GetTopStack();

            IrpCtxPtr& pCtx =
                pIrp->GetCurCtx();

            if( ERROR( ret ) )
            {
                pCtx->SetStatus( ret );
                break;
            }

            BufPtr pBuf = pTopCtx->m_pRespData;
            if( unlikely( pBuf.IsEmpty() ) )
            {
                ret = -EFAULT;
                break;
            }
            STREAM_SOCK_EVENT* psse =
            ( STREAM_SOCK_EVENT* )pBuf->ptr();
            if( psse->m_iEvent == sseError )
            {
                pCtx->SetRespData( pBuf );
                pCtx->SetStatus( ret );
            }
            else if( psse->m_iEvent ==
                sseRetWithFd )
            {
                ret = -ENOTSUP;
                pCtx->SetStatus( ret );
            }
            else
            {
                ret = CompleteListeningIrp(
                    pIrp );
            }
            break;
        }
    default:
        {
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

        ret = GetIoMgr()->ScheduleTask(
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

        IrpCtxPtr pCtx = pIrp->GetTopStack(); 
        pCtx->SetMajorCmd( IRP_MJ_FUNC );
        pCtx->SetMinorCmd( IRP_MN_IOCTL );
        pCtx->SetCtrlCode( CTRLCODE_IS_CLIENT );

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

    gint32 ret = StartSSLShutdown( pIrp );

    if( ret == STATUS_PENDING )
        return ret;

    return super::PreStop( pIrp );
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
    gint32 ret = 0;
    do{
        IrpCtxPtr pCtx = pIrp->GetCurCtx();
        IrpCtxPtr pTopCtx = pIrp->GetTopStack();

        BufPtr& pRespBuf = pTopCtx->m_pRespData;
        STREAM_SOCK_EVENT* psse =
            ( STREAM_SOCK_EVENT* )pRespBuf->ptr();

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

        guint32 dwExpSize = std::min(
            ( pEncrypted->size() + 256 ),
            ( guint32 )STM_MAX_BYTES_PER_BUF );

        pDecrypted->Resize( dwExpSize );

        do{
            // NOTE: no locking here since there
            // is one and only one read at any
            // time. Remember to add lock if
            // SSL_read could be called
            // concurrently
            ret = SSL_read( m_pSSL,
                pDecrypted->ptr() + dwOffset,
                pDecrypted->size() - dwOffset );

            if( ret <= 0 )
                break;

            dwOffset += ret;
            if( pDecrypted->size() < 
                dwOffset + 1024 )
            {
                pDecrypted->Resize(
                    pDecrypted->size() +
                    STM_MAX_BYTES_PER_BUF );

                if( pDecrypted->size() <
                    dwOffset + 1024 )
                {
                    // realloc failed
                    ret = ERROR_FAIL;
                    break;
                }
            }

        }while( 1 );

        if( ret == ERROR_FAIL )
        {
            ret = -ENOMEM;
            pCtx->SetStatus( ret );
            break;
        }

        ret = GetSSLError( m_pSSL, ret );
        if( ERROR( ret ) )
        {
            pCtx->SetStatus( ret );
            break;
        }

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
                break;
            }
            else
            {
                // deliver the received data up
                // and let the next irp to read
                // the remaining bytes.
            }
            ret = 0;
        }
        else if( ret == SSL_ERROR_WANT_WRITE )
        {
            CParamList oTaskCfg;

            oTaskCfg.Push( 0 );
            oTaskCfg.Push( 0 );
            oTaskCfg.Push( 0 );

            IrpPtr pIrp( true );
            pIrp->AllocNextStack(
                GetLowerPort() );
            IrpCtxPtr& pCtx = pIrp->GetTopStack();
            pCtx->SetMajorCmd( IRP_MJ_FUNC );
            pCtx->SetMinorCmd( IRP_MN_WRITE );

            CParamList oEmptyCfg;

            BufPtr pReqBuf( true );
            *pReqBuf = ObjPtr(
                oEmptyCfg.GetCfg() );
            pCtx->SetReqData( pReqBuf );

            TaskletPtr pTask;
            ret = pTask.NewObj(
                clsid( COpenSSLResumeWriteTask ),
                oTaskCfg.GetCfg() );

            if( ERROR( ret ) )
                break;

            CStdRMutex oPortLock( GetLock() );
            if( m_dwRenegStat == rngstatNormal )
                m_dwRenegStat = rngstatWantRead;

            if( m_queWriteTasks.empty() )
                m_queWriteTasks.push_back( pTask );

        }

        if( ERROR( ret ) )
            break;

        if( dwOffset > 0 )
        {
            if( dwOffset < pDecrypted->size() )
                pDecrypted->Resize( dwOffset );

            psse->m_pInBuf = pDecrypted;
            pCtx->SetRespData( pRespBuf );
            pCtx->SetStatus( ret );
        }

        CStdRMutex oPortLock( GetLock() );
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

    }while( 0 );

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
    if( pCallback == nullptr ||
        pBuf.IsEmpty() || pBuf->empty() )
        return -EINVAL;

    IrpPtr pIrp( true );
    PortPtr pLowerPort = GetLowerPort();
    pIrp->AllocNextStack( pLowerPort );
    IrpCtxPtr& pCtx = pIrp->GetTopStack();

    pCtx->SetMajorCmd( IRP_MJ_FUNC );
    pCtx->SetMinorCmd( IRP_MN_WRITE );
    pCtx->SetIoDirection( IRP_DIR_OUT );
    pCtx->SetReqData( pBuf );
    pIrp->SetCallback(
        ObjPtr( pCallback ), 0 );

    CParamList oParams;
    oParams.Push( pBuf );
    BufPtr pReqBuf( true );
    *pReqBuf = ObjPtr( oParams.GetCfg() );

    return GetIoMgr()->SubmitIrpInternal(
        pLowerPort, pIrp, false );
}

gint32 CRpcOpenSSLFido::SendListenReq(
    IEventSink* pCallback, BufPtr& pBuf )
{
    if( pCallback == nullptr )
        return -EINVAL;

    IrpPtr pIrp( true );
    PortPtr pLowerPort = GetLowerPort();
    pIrp->AllocNextStack( pLowerPort );
    IrpCtxPtr& pCtx = pIrp->GetTopStack();

    pCtx->SetMajorCmd( IRP_MJ_FUNC );
    pCtx->SetMinorCmd( IRP_MN_IOCTL );
    pCtx->SetCtrlCode( CTRLCODE_LISTENING );
    pCtx->SetIoDirection( IRP_DIR_IN );
    pIrp->SetCallback(
        ObjPtr( pCallback ), 0 );

    gint32 ret = GetIoMgr()->SubmitIrpInternal(
        pLowerPort, pIrp, false );

    if( SUCCEEDED( ret ) )
    {
        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        pBuf = pCtx->m_pRespData;
        if( pBuf.IsEmpty() || pBuf->empty() )
            return -EFAULT;

        STREAM_SOCK_EVENT* psse = 
            ( STREAM_SOCK_EVENT* ) pBuf->ptr();

        if( psse->m_iEvent == sseRetWithBuf )
        {
            pBuf = psse->m_pInBuf;
            psse->m_pInBuf.Clear();
            return 0;
        }

        if( psse->m_iEvent == sseError )
            return psse->m_iData;
    }

    return ret;
}

gint32 CRpcOpenSSLFido::DoHandshake(
     IEventSink* pCallback,
     BufPtr& pHandshake )
{
    if( SSL_is_init_finished( m_pSSL ) )
        return 0;

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
        ret = SSL_do_handshake( m_pSSL );
        ret = GetSSLError( m_pSSL, ret );
        if( ERROR( ret ) )
            break;

        if( SUCCEEDED( ret ) )
            break;

        if( ret == SSL_ERROR_WANT_READ )
        {
            ret = SendListenReq(
                pCallback, pHandshake );

            if( ret == STATUS_PENDING )
                break;

            if( ERROR( ret ) )
                break;

            ret = BIO_write( m_prbio,
                pHandshake->ptr(),
                pHandshake->size() );
            if( ret < 0 )
            {
                ret = ERROR_FAIL;
                break;
            }
            continue;
        }
        else if( ret != SSL_ERROR_WANT_WRITE )
        {
            // error
            ret = -ret;
            break;
        }

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
                    continue;
                }
            }
            else if( !BIO_should_retry( m_pwbio ) )
            {
                ret = ERROR_FAIL;
                break;
            }

            pBuf->Resize( dwOffset );
            ret = 0;
            break;

        }while( 1 );

        if( ERROR( ret ) )
            break;

        ret = SendWriteReq( pCallback, pBuf );
        if( ret == STATUS_PENDING )
            break;

        if( ERROR( ret ) )
            break;

    }while( 1 );

    return ret;
}

gint32 CRpcOpenSSLFido::DoShutdown(
     IEventSink* pCallback )
{
    if( !SSL_is_init_finished( m_pSSL ) )
        return 0;

    // NOTE: at this moment, the lower port is
    // still in `READY' state, and able to accept
    // io requests.
    BufPtr pShutdownInfo;
    gint32 ret = 0;
    do{
        ret = SSL_shutdown( m_pSSL );
        ret = GetSSLError( m_pSSL, ret );
        if( ERROR( ret ) )
            break;

        if( SUCCEEDED( ret ) )
            break;

        if( ret == SSL_ERROR_WANT_READ )
        {
            ret = SendListenReq(
                pCallback, pShutdownInfo );

            if( ret == STATUS_PENDING )
                break;

            if( ERROR( ret ) )
                break;

            ret = BIO_write( m_prbio,
                pShutdownInfo->ptr(),
                pShutdownInfo->size() );
            if( ret < 0 )
            {
                ret = ERROR_FAIL;
                break;
            }
            continue;
        }
        else if( ret != SSL_ERROR_WANT_WRITE )
        {
            // error
            ret = -ret;
            break;
        }

        BufPtr pBuf( true );
        pBuf->Resize( 128 );
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
                    pBuf->Resize( dwOffset + 128 );
                    continue;
                }
            }
            else if( !BIO_should_retry( m_pwbio ) )
            {
                ret = ERROR_FAIL;
                break;
            }

            pBuf->Resize( dwOffset );
            ret = 0;
            break;

        }while( 1 );

        if( ERROR( ret ) )
            break;

        ret = SendWriteReq( pCallback, pBuf );
        if( ERROR( ret ) ||
            ret == STATUS_PENDING )
            break;

    }while( 1 );

    return ret;
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

        std::string strPdoClass;

        CCfgOpenerObj oCfg( pLowerPort );
        ret = oCfg.GetStrProp(
            propPdoClass, strPdoClass );

        if( ERROR( ret ) )
            break;

        if( strPdoClass !=
            PORT_CLASS_OPENSSL_FIDO )
        {
            // this is not a port we support
            ret = -ENOTSUP;
            break;
        }

        CParamList oNewCfg;
        oNewCfg[ propPortClass ] =
            PORT_CLASS_OPENSSL_FIDO;

        oNewCfg.CopyProp(
            propPortId, pLowerPort );

        // destination ip addr
        oNewCfg.CopyProp(
            propIpAddr, pLowerPort );

        oNewCfg.CopyProp(
            propDestTcpPort, pLowerPort );

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
                    break;
                }
                m_strKeyPath = strKeyFile;
            }
        }

    }while( 0 );

    return ret;
}

CRpcOpenSSLFidoDrv::CRpcOpenSSLFidoDrv(
    const IConfigDb* pCfg ) :
    super( pCfg )
{
    SetClassId( clsid( CRpcOpenSSLFidoDrv ) );

    LoadSSLSettings();
    InitSSLContext();
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
        ret = pPort->DoShutdown( this );

    }while( 0 );

    if( ret != STATUS_PENDING )
        ret = OnTaskComplete( ret );

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

        ret = pPort->DoShutdown( this );

    }while( 0 );

    if( ret != STATUS_PENDING )
        ret = OnTaskComplete( ret );

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

        TaskletPtr pTask( this );
        ret = pPort->EncryptAndSend( pTask,
            pIrp, pCfg, ( gint32& )dwIdx,
            dwTotal, dwOffset );

        CIoManager* pMgr = pPort->GetIoMgr();
        if( ret != STATUS_PENDING )
            pMgr->CompleteIrp( pIrp );

    }while( 0 );

    return ret;
}
