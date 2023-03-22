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
#include "jsondef.h"
extern "C"{
#include <gmssl/rand.h>
#include <gmssl/x509.h>
#include <gmssl/error.h>
#include <gmssl/sm2.h>
#include <gmssl/sm3.h>
#include <gmssl/sm4.h>
#include <gmssl/pem.h>
#include <gmssl/tls.h>
#include <gmssl/digest.h>
#include <gmssl/gcm.h>
#include <gmssl/hmac.h>
#include <gmssl/hkdf.h>
#include <gmssl/mem.h>
}
#include "agmsapi.h"
#include "gmsfido.h"
using namespace gmssl;

#define DETACH_SAFE( pBuf_, pMem_, dwSize_, dwOff_, dwTailOff_ ) \
do{ \
    ret = pBuf_->Detach( pMem_, dwSize_, dwOff_, dwTailOff_ ); \
    if( ret == -EACCES ) \
    { \
        pMem_ = ( decltype( pMem_ ) )malloc( pBuf_->size() ); \
        memcpy( pMem_, pBuf_->ptr(), pBuf_->size() ); \
        dwSize_ = pBuf_->size(); \
        dwOff_ = 0; \
        dwTailOff_ = dwSize_; \
        pBuf_->Resize( 0 ); \
        ret = 0;\
        break; \
    } \
}while( 0 )

namespace gmssl{
extern int init_ctx( AGMS_CTX* ctx, bool is_client );
}

extern char g_szKeyPass[ SSL_PASS_MAX + 1 ];
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
            pDest = std::shared_ptr< AGMS_IOVE >(
                new AGMS_IOVE );
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

                DETACH_SAFE( pSrc, pmem,
                    dwSize, dwStart, dwEnd );

                if( ERROR( ret ) )
                    break;

                ret = pDest->attach(
                    ( guint8* )pmem,
                    dwSize, dwStart, dwEnd );
            }
            else
            {
                pmem = pSrc->ptr();
                dwSize = pSrc->size();

                ret = pDest->attach(
                    ( guint8* )pmem,
                    dwSize, dwStart, dwEnd );
                if( SUCCEEDED( ret ) ) 
                    pDest->set_no_free();
            }

        }

    }while( 0 );

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
                ret = pDest->Attach(
                    ( char* )pmem, dwSize,
                    dwStart, dwEnd );
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
                    ( char* )pSrc->begin(), pSrc->size() );
                if( ret == -EACCES )
                {
                    bCopy = true;
                    continue;
                }
                else if( ERROR( ret ) )
                    break;
                pDest->SetNoFree( true );
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

CRpcGmSSLFido::CRpcGmSSLFido(
    const IConfigDb* pCfg ) :
    super( pCfg )
{
    gint32 ret = 0;
    do{
        m_dwFlags &= ~PORTFLG_TYPE_MASK;
        m_dwFlags |= PORTFLG_TYPE_FIDO;
        SetClassId( clsid( CRpcGmSSLFido ) );

        CCfgOpener oCfg( pCfg );
        guint32* ictx;
        ret = oCfg.GetIntPtr(
            propContext, ictx );
        if( ERROR( ret ) )
            break;
        AGMS_CTX* pCtx = ( AGMS_CTX* )ictx;
        m_pSSLCtx = pCtx;
        m_bClient = pCtx->is_client;

    }while( 0 );

    if( ERROR( ret ) )
    {
        std::string strMsg = DebugMsg( ret,
            "Error occurs in CRpcGmSSLFido ctor" );  
        throw std::runtime_error( strMsg );
    }
}

gint32 CRpcGmSSLFido::PostStart(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        if( !m_pSSL )
        {
            m_pSSL = std::move(
                std::unique_ptr< AGMS >
                    ( new TLS13 ) );
        }

        m_pSSL->gms_ctx = m_pSSLCtx;
        ret = m_pSSL->init( m_bClient );
        if( ERROR( ret ) )
            break;

        // hold the irp and start the handshake
        // task
        m_pSSL->set_state( STAT_START_HANDSHAKE );
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

gint32 CRpcGmSSLFido::SubmitWriteIrpSingle(
    PIRP pIrp, CfgPtr& pBufs )
{
    gint32 ret = 0;
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

        pLowerPort->AllocIrpCtxExt(
            pTopCtx, pIrp );

        CParamList oParams( pBufs );
        BufPtr pBuf;
        if( oParams.GetCount() == 1 )
        {
            oParams.GetBufPtr( 0, pBuf );
        }
        else
        {
            pBuf.NewObj();
            *pBuf = ObjPtr( pBufs );
        }

        pTopCtx->SetReqData( pBuf );
        ret = pLowerPort->SubmitIrp( pIrp );
        if( ret == STATUS_PENDING )
            break;

        pIrp->PopCtxStack();
        pCtx->SetStatus( ret );

    }while( 0 );

    return ret;
}

gint32 CRpcGmSSLFido::SubmitWriteIrpGroup(
    PIRP pIrp, ObjVecPtr& pvecBufs )
{
    gint32 ret = 0;
    std::vector< ObjPtr >& vecBufs =
        ( *pvecBufs )();

    if( vecBufs.empty() )
        return -EINVAL;

    IrpCtxPtr pCtx = pIrp->GetTopStack();
    do{
        if( vecBufs.empty() )
        {
            pCtx->SetStatus( ret );
            break;
        }

        auto elem = vecBufs.front();
        vecBufs.erase( vecBufs.begin() );

        BufPtr pBuf( true );
        *pBuf = elem;

        if( vecBufs.size() )
        {
            BufPtr pExtBuf;
            pExtBuf.NewObj();
            *pExtBuf = ObjPtr( pvecBufs );
            pCtx->SetExtBuf( pExtBuf );
        }

        PortPtr pLowerPort = GetLowerPort();
        pIrp->AllocNextStack( pLowerPort );
        IrpCtxPtr pTopCtx = 
            pIrp->GetTopStack();

        pTopCtx->SetMajorCmd( IRP_MJ_FUNC );
        pTopCtx->SetMinorCmd( IRP_MN_WRITE );
        pTopCtx->SetReqData( pBuf );
        pTopCtx->SetIoDirection( IRP_DIR_OUT ); 
        pLowerPort->AllocIrpCtxExt( pTopCtx, pIrp );
        ret = pLowerPort->SubmitIrp( pIrp );
        if( ret == STATUS_PENDING )
            break;

        pIrp->PopCtxStack();
        if( ERROR( ret ) )
        {
            pCtx->SetStatus( ret );
            break;
        }
        pCtx->ClearExtBuf();

    }while( 1 );

    return ret;
}

gint32 CRpcGmSSLFido::SubmitWriteIrpIn(
    IRP* pIrp )
{
    gint32 ret = 0;
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    IrpCtxPtr pCtx = pIrp->GetTopStack();
    do{
        PIOVE piove;
        BufPtr pBuf;
        CfgPtr pCfg;
        CStdRMutex oLock( this->GetLock() );
        ret = pCtx->GetReqAsCfg( pCfg );
        if( ERROR( ret ) )
        { 
            pBuf = pCtx->m_pReqData;
            ret = BufToIove(
                pBuf, piove, false, false );
            if( ERROR( ret ) )
                break;

            ret = m_pSSL->send( piove );
            if( ERROR( ret ) )
                break;
        }
        else
        {
            CParamList oParams(
                ( IConfigDb* )pCfg );
            guint32 dwCount = oParams.GetCount();
            if( dwCount == 0 )
            {
                ret = -ENODATA;
                break;
            }
            std::vector< BufPtr > vecBufs;
            for( guint32 i = 0; i < dwCount; ++i )
            {
                ret = oParams.GetBufPtr( i, pBuf );
                if( ERROR( ret ) )
                    break;
                vecBufs.push_back( pBuf );
            }
            for( auto& elem : vecBufs )
            {
                ret = BufToIove(
                    elem, piove, false, false );
                if( ERROR( ret ) )
                    break;

                ret = m_pSSL->send( piove );
                if( ERROR( ret ) )
                    break;
            }
            if( ERROR( ret ) )
                break;
        }

        //----------------

        ObjVecPtr pvecBufs( true );
        std::vector< ObjPtr >& vecBufs =
            ( *pvecBufs )();

        guint32 dwSize = 0;
        CfgPtr pBufs( true );
        while( m_pSSL->write_bio.size() )
        {
            PIOVE piove;
            CParamList oParams( pBufs );
            ret = m_pSSL->write_bio.read( piove );
            if( ERROR( ret ) )
                break;

            // transfer the buffer ownership
            BufPtr pBuf;
            ret = IoveToBuf(
                piove, pBuf, false, true );

            if( ERROR( ret ) )
                break;

            if( dwSize + pBuf->size() <
                MAX_BYTES_PER_TRANSFER )
            {
                dwSize += pBuf->size();
                oParams.Push( pBuf );
                continue;
            }

            vecBufs.push_back( ObjPtr( pBufs ) );
            pBufs.Clear();
            pBufs.NewObj();
            CParamList oParams2( pBufs );
            oParams2.Push( pBuf );
            dwSize = pBuf->size();
        }

        oLock.Unlock();

        if( ERROR( ret ) )
        {
            pCtx->SetStatus( ret );
            break;
        }

        CParamList oParams( pBufs );
        if( oParams.GetCount() > 0 )
        {
            vecBufs.push_back(
                ObjPtr( pBufs ) );
        }

        if( vecBufs.empty() )
        {
            IrpCtxPtr pCtx = pIrp->GetTopStack();
            pCtx->SetStatus( ret );
            break;
        }

        pCtx->ClearExtBuf();
        if( vecBufs.size() == 1 )
        {
            ret = SubmitWriteIrpSingle( pIrp, pBufs );
        }
        else
        {
            ret = SubmitWriteIrpGroup( pIrp, pvecBufs );
        }

    }while( 0 );

    if( ERROR( ret ) )
        pCtx->SetStatus( ret );

    return ret;
}

gint32 CRpcGmSSLFido::SubmitWriteIrpOut( PIRP pIrp )
{
    gint32 ret = 0;
    do{
        IrpCtxPtr pCtx = pIrp->GetTopStack();
        CStdRMutex oLock( GetLock() );

    }while( 0 );

    return ret;
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
                ret = m_pSSL->write_bio.read(
                    piove );
                if( ERROR( ret ) )
                {
                    ret = 0;
                    break;
                }

                // transfer the buffer ownership
                BufPtr pBuf;
                ret = IoveToBuf( piove, pBuf,
                    false, true );

                if( ERROR( ret ) )
                    break;

                dwSize += pBuf->size();
                oParams.Push( pBuf );

            }while( ( bool )piove );

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
            PIOVE pInput;
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

        if( ERROR( iRet ) )
        {
            ret = iRet;
            break;
        }

        if( ERROR( ret ) )
            break;

        if( SUCCEEDED( iRet ) )
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
                ret = SubmitWriteIrpIn( pIrp );
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
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    bool bSSLErr = false;
    IrpCtxPtr pCtx = pIrp->GetCurCtx();

    do{
        IrpCtxPtr pTopCtx = pIrp->GetTopStack();

        BufPtr pInBuf = pTopCtx->m_pRespData;
        pIrp->PopCtxStack();

        if( unlikely( pInBuf.IsEmpty() ) )
        {
            ret = -EFAULT;
            pCtx->SetStatus( ret );
            break;
        }

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
        BufPtr pRespBuf1 = psse->m_pInBuf;
        psse->m_pInBuf.Clear();

        bSSLErr = true;
        ret = BufToIove( pRespBuf1,
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

        BufPtr pBufLast;
        BufPtr pRespBuf;
        CParamList oResp;
        for( auto& elem : iov )
        {
            BufPtr pBuf;
            ret = IoveToBuf(
                elem, pBuf, false, true );
            if( ERROR( ret ) )
                break;
            oResp.Push( pBuf );
            pBufLast = pBuf;
        }

        if( ERROR( ret ) )
            break;

        if( oResp.GetCount() == 1 )
        {
            pRespBuf = pBufLast;
        }
        else
        {
            pRespBuf.NewObj();
            *pRespBuf = oResp.GetCfg();
        }

        pInBuf.NewObj();
        pInBuf->Resize(
            sizeof( STREAM_SOCK_EVENT ) );

        psse = new ( pInBuf->ptr() )
            STREAM_SOCK_EVENT;

        psse->m_iEvent = sseRetWithBuf;
        psse->m_pInBuf = pRespBuf;
        psse->m_iEvtSrc = GetClsid();

        pCtx->SetRespData( pInBuf );
        pCtx->SetStatus( ret );

    }while( 0 );

    if( ERROR( ret ) && bSSLErr )
    {
        BufPtr pInBuf( true );
        pInBuf->Resize(
            sizeof( STREAM_SOCK_EVENT ) );

        STREAM_SOCK_EVENT* psse = 
            new ( pInBuf->ptr() ) STREAM_SOCK_EVENT;

        psse->m_iEvent = sseError;
        psse->m_iData = ret;
        psse->m_iEvtSrc = GetClsid();

        pCtx->SetRespData( pInBuf );
        pCtx->SetStatus( 0 );
        ret = 0;
    }

    return ret;
}

gint32 CRpcGmSSLFido::CompleteIoctlIrp(
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
            else do
            {
                ret = CompleteListeningIrp( pIrp );
                if( ret != STATUS_PENDING )
                    break;

                IrpCtxPtr pCtx =
                    pIrp->GetTopStack();

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

                if( ERROR( ret ) )
                {
                    pIrp->PopCtxStack();
                    pCtx->SetStatus( ret );
                    break;
                }

            }while( 1 );

            break;
        }
    default:
        {
            ret = pTopCtx->GetStatus();
            pCtx->SetRespData( pTopCtx->m_pRespData );
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
    IrpCtxPtr pCtx = pIrp->GetCurCtx();

    do{
        BufPtr pvecBufs; 
        IrpCtxPtr pTopCtx = pIrp->GetTopStack();
        ret = pTopCtx->GetStatus();
        pIrp->PopCtxStack();
        if( ERROR( ret ) )
            break;

        if( m_pSSL->get_state() != STAT_READY )
        {
            ret = ERROR_STATE;
            break;
        }

        pCtx->GetExtBuf( pvecBufs );
        if( pvecBufs.IsEmpty() )
        {
            pCtx->SetStatus( ret );
            break;
        }
        pCtx->ClearExtBuf();
        ObjVecPtr pvBufs = ( ObjPtr& )( *pvecBufs );
        if( pvBufs.IsEmpty() )
        {
            ret = -ENOENT;
            break;
        }

        ret = SubmitWriteIrpGroup(
            pIrp, pvBufs ); 

    }while( 0 );

    if( ret != STATUS_PENDING )
        pCtx->SetStatus( ret );

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
                if( ret != STATUS_PENDING )
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

CRpcGmSSLFidoDrv::CRpcGmSSLFidoDrv(
    const IConfigDb* pCfg )
    : super( pCfg )
{
    SetClassId( clsid( CRpcGmSSLFidoDrv ) );
    m_pGmsCtx = std::move(
        std::unique_ptr< AGMS_CTX >
            ( new AGMS_CTX ) );
}

CRpcGmSSLFidoDrv::~CRpcGmSSLFidoDrv()
{}

gint32 CRpcGmSSLFidoDrv::Start()
{
    gint32 ret1 =LoadSSLSettings();
    gint32 ret2 = InitSSLContext();

    if( ERROR( ret1 ) ) 
        return ret1;
    else if( ERROR( ret2 ) )
        return ret2;

    return super::Start();
}

gint32 CRpcGmSSLFidoDrv::Probe(
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
            PORT_CLASS_GMSSL_FIDO;

        oNewCfg.SetIntProp(
            propPortId, NewPortId() );

        oNewCfg.CopyProp(
            propConnParams, pLowerPort );

        oNewCfg.SetPointer(
            propIoMgr, GetIoMgr() );

        oNewCfg.SetPointer(
            propDrvPtr, this );

        oNewCfg.SetIntPtr( propContext,
            ( guint32* )m_pGmsCtx.get() );

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

gint32 CRpcGmSSLFidoDrv::LoadSSLSettings()
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
            if( strPortClass != PORT_CLASS_GMSSL_FIDO )
                continue;

            if( !( elem.isMember( JSON_ATTR_PARAMETERS ) &&
                elem[ JSON_ATTR_PARAMETERS ].isObject() ) )
            {
                ret = -ENOENT;
                break;
            }

            Json::Value& oParams =
                elem[ JSON_ATTR_PARAMETERS ];

            if( oParams.isMember( JSON_ATTR_VERIFY_PEER ) &&
                oParams[ JSON_ATTR_VERIFY_PEER ].isString() )
            {
                std::string strVal =
                    oParams[ JSON_ATTR_VERIFY_PEER ].asString();
                if( strVal == "true" )
                    m_bVerifyPeer = true;
            }

            // certificate file path
            if( oParams.isMember( JSON_ATTR_CERTFILE ) &&
                oParams[ JSON_ATTR_CERTFILE ].isString() )
            {
                std::string strCert =
                    oParams[ JSON_ATTR_CERTFILE ].asString();
                if( strCert.empty() )
                {
                    ret = -ENOENT;
                    break;
                }
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
                if( strKeyFile.empty() )
                {
                    ret = -ENOENT;
                    break;
                }

                ret = access( strKeyFile.c_str(), R_OK );
                if( ret == -1 )
                {
                    ret = -errno;
                    break;
                }
                m_strKeyPath = strKeyFile;
            }

            // certificate file path
            if( oParams.isMember( JSON_ATTR_CACERT ) &&
                oParams[ JSON_ATTR_CACERT ].isString() )
            {
                std::string strCAPath =
                    oParams[ JSON_ATTR_CACERT ].asString();
                if( strCAPath.size() )
                {
                    ret = access( strCAPath.c_str(), R_OK );
                    if( ret == -1 )
                    {
                        ret = -errno;
                        break;
                    }
                    m_strCAPath = strCAPath;
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
                    if( strPath == "console" )
                    {
                        m_strSecretPath = strPath;
                    }
                    else
                    {
                        ret = access( strPath.c_str(), R_OK );
                        if( ret == -1 )
                        {
                            ret = -errno;
                            break;
                        }
                        m_strSecretPath = strPath;
                    }
                }
            }
            break;
        }

    }while( 0 );

    return ret;
}

gint32 CRpcGmSSLFidoDrv::InitSSLContext()
{
    gint32 ret = 0;
    do{
        guint32 dwRole = 0;
        ret = GetIoMgr()->GetCmdLineOpt(
            propRouterRole, dwRole );
        if( ERROR( ret ) )
            break;

        ret = 0;
        AGMS_CTX* ctx = m_pGmsCtx.get();
        TLS_CTX* tctx = ctx;
        memset(tctx, 0, sizeof(*tctx));

        if( m_strSecretPath.empty() )
        {
            ctx->password = "1234";
        }
        else if( m_strSecretPath == "console" )
        {
            if( g_szKeyPass[ 0 ] == 0 )
            {
                ret = -EACCES;
                break;
            }
            ctx->password = g_szKeyPass;
        }
        else
        {
            FILE* fp = fopen(
                m_strSecretPath.c_str(), "r" );
            if( fp == nullptr )
            {
                ret = -errno;
                break;
            }
            char* pszPass = nullptr;
            size_t len = 0;
            ret = getline( &pszPass, &len , fp );
            fclose( fp );
            fp = nullptr;
            if( ret == -1 )
            {
                if( pszPass )
                    free( pszPass );
                ret = -errno;
                break;
            }

            stdstr& pass = ctx->password;
            pass = pszPass;
            for( size_t i = 0; i < len; i++ )
                pszPass[ i ] = ' ';
            free( pszPass );
            pszPass = nullptr;

            if( pass.size() > SSL_PASS_MAX )
                ctx->password.erase( SSL_PASS_MAX );
            while( pass.back() == '\n' ||
                pass.back() == '\r' )
                pass.pop_back();
            if( pass.empty() )
            {
                ret = -EACCES;
                break;
            }
        }

        ctx->keyfile = m_strKeyPath;
        ctx->certfile = m_strCertPath;
        ctx->cacertfile = m_strCAPath; 
        ctx->verify_peer = m_bVerifyPeer;

        ret = gmssl::init_ctx( ctx, dwRole == 1 );

        ctx->password.replace(
            0, std::string::npos,
            ctx->password.size(), '0' );
        ctx->password.clear();

    }while( 0 );
    memset( g_szKeyPass, 0,
        sizeof( g_szKeyPass ) );
    return ret;
}

}
