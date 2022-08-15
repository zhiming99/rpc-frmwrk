/*
 * =====================================================================================
 *
 *       Filename:  stmport.cpp
 *
 *    Description:  Definition of stream port class 
 *
 *        Version:  1.0
 *        Created:  08/14/2022 12:37:32 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2022 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */

#include <dbus/dbus.h>
#include <vector>
#include <string>
#include <regex>
#include "defines.h"
#include "port.h"
#include "dbusport.h"
#include "stlcont.h"
#include "emaphelp.h"
#include "ifhelper.h"
#include "stmport.h"
#include "fastrpc.h"

CDBusStreamPdo::CDBusStreamPdo(
    const IConfigDb* pCfg )
    : super( pCfg )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg( pCfg );
        ret = oCfg.GetObjPtr(
            propIfPtr, m_pIf );
        if( ERROR( ret ) )
            break;

        CRpcServices* pSvc = m_pIf;
        m_bServer = pSvc->IsServer();

        if( IsServer() )
        {
            CRpcReqStreamServer *pIf =
                GetStreamIf();
            pIf->SetPort( this );
        }
        else
        {
            CRpcReqStreamProxy *pIf =
                GetStreamIf();
            pIf->SetPort( this );
        }

    }while( 0 );

    if( ERROR( ret ) )
    {
        string strMsg = DebugMsg( ret,
            "Error in CDBusStreamPdo ctor" );
        throw std::runtime_error( strMsg );
    }
    return;
}

gint32 CDBusStreamPdo::SendDBusMsg(
    PIRP pIrp, DMsgPtr& pMsg )
{
    gint32 ret = 0;
    if( pIrp == nullptr || pMsg.IsEmpty() )
        return -EINVAL;

    do{
        BufPtr pExtBuf;
        IrpCtxPtr& pCtx = pIrp->GetCurCtx();
        pCtx->GetExtBuf( pExtBuf );
        if( unlikely( pExtBuf.IsEmpty() ) )
        {
            ret = -EFAULT;
            break;
        }
        IRPCTX_EXTDSP* pExt = pExtBuf->ptr();
        HANDLE hStream = pExt->m_hStream;

        BufPtr pBuf( true );
        ret = pMsg.Serialize( *pBuf );
        if( ERROR( ret ) )
            break;

        pExt->m_bWaitWrite = true;

        CCfgOpener oCtx;
        oCtx.SetPointer( propIrpPtr, pIrp );
        CRpcServices* pSvc = GetStreamIf();
        if( pSvc->IsServer() )
        {
            CStreamServerSync* pStm =
                GetStreamIf();
            ret = pStm->WriteStreamAsync(
                hStream, pBuf, oCtx.GetCfg() );
        }
        else
        {
            CStreamProxySync* pStm =
                GetStreamIf();
            ret = pStm->WriteStreamAsync(
                hStream, pBuf, oCtx.GetCfg() );
        }

    }while( 0 );

    return ret;
}

gint32 CDBusStreamPdo::BroadcastDBusMsg(
    PIRP pIrp, DMsgPtr& pMsg )
{
    gint32 ret = 0;
    if( pIrp == nullptr || pMsg.IsEmpty() )
        return -EINVAL;

    do{
        BufPtr pExtBuf;
        IrpCtxPtr& pCtx = pIrp->GetCurCtx();
        pCtx->GetExtBuf( pExtBuf );
        if( unlikely( pExtBuf.IsEmpty() ) )
        {
            ret = -EFAULT;
            break;
        }
        IRPCTX_EXTDSP* pExt = pExtBuf->ptr();
        pExt->m_bWaitWrite = true;

        HANDLE hStream = pExt->m_hStream;

        BufPtr pBuf( true );
        ret = pMsg.Serialize( *pBuf );
        if( ERROR( ret ) )
            break;

        CCfgOpener oCtx;
        oCtx.SetPointer( propIrpPtr, pIrp );
        CRpcServices* pSvc = GetStreamIf();
        if( pSvc->IsServer() )
        {
            CStreamServerSync* pStm =
                GetStreamIf();
            std::vector< HANDLE > vecUxStms;
            pStm->EnumStreams( vecUxStms );
            for( auto& elem : vecUxStms )
                pStm->WriteStreamAsync( elem,
                    pBuf, ( IConfigDb* )nullptr );
        }
        else
        {
            CStreamProxySync* pStm =
                GetStreamIf();
            std::vector< HANDLE > vecUxStms;
            pStm->EnumStreams( vecUxStms );
            for( auto& elem : vecUxStms )
                pStm->WriteStreamAsync( elem,
                    pBuf, ( IConfigDb* )nullptr );
        }

    }while( 0 );

    return ret;
}

gint32 CDBusStreamPdo::HandleSendReq( IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        IrpCtxPtr& pCtx = pIrp->GetCurCtx();

        // client side I/O
        guint32 dwSerial = 0;
        guint32 dwIoDir = pCtx->GetIoDirection();

        ret = -EINVAL;

        if( dwIoDir == IRP_DIR_INOUT ||
            dwIoDir == IRP_DIR_OUT )
        {
            DMsgPtr pMsg = *pCtx->m_pReqData;
            CDBusBusPort *pBusPort = static_cast
                < CDBusBusPort* >( m_pBusPort );

            dwSerial =
                pBusPort->LabelMessage( pMsg );

            if( dwIoDir == IRP_DIR_INOUT )
            {
                // NOTE: there are slim chances,
                // that the response message
                // arrives before the irp enters
                // m_mapSerial2Resp after sending,
                // so better add the irp to
                // m_mapSerial2Resp ahead of
                // sending.
                CStdRMutex oPortLock( GetLock() );
                m_mapSerial2Resp[ dwSerial ] =
                    IrpPtr( pIrp );
            }
            else
            {
                pMsg.SetNoReply( true );
            }

            ret = SendDBusMsg( pIrp, pMsg );

            if( SUCCEEDED( ret ) &&
                dwIoDir == IRP_DIR_INOUT )
                ret = STATUS_PENDING;
        }

        if( ERROR( ret ) &&
            dwIoDir == IRP_DIR_INOUT &&
            dwSerial != 0 )
        {
            CStdRMutex oPortLock( GetLock() );
            m_mapSerial2Resp.erase( dwSerial );
        }

    }while( 0 );

    return ret;
}

gint32 CDBusStreamPdo::HandleSendEvent( IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        if( !IsServer() )
        {
            ret = -ENOTSUP;
            break;
        }

        IrpCtxPtr& pCtx = pIrp->GetCurCtx();

        // client side I/O
        guint32 dwSerial = 0;
        guint32 dwIoDir = pCtx->GetIoDirection();

        ret = -EINVAL;

        if( dwIoDir == IRP_DIR_OUT )
        {
            DMsgPtr pMsg = *pCtx->m_pReqData;
            if( pMsg.GetType() !=
                DBUS_MESSAGE_TYPE_SIGNAL )
            {
                ret = -EINVAL;
                break;
            }

            if( ERROR( ret ) )
                break;

            CDBusBusPort *pBusPort = static_cast
                < CDBusBusPort* >( m_pBusPort );

            dwSerial =
                pBusPort->LabelMessage( pMsg );

            pMsg.SetNoReply( true );
            ret = BroadcastDBusMsg( pIrp, pMsg );
        }
        else
        {
            ret = -EINVAL;
        }

    }while( 0 );

    return ret;
}

gint32 CDBusStreamPdo::AllocIrpCtxExt(
    IrpCtxPtr& pIrpCtx,
    void* pContext ) const
{
    gint32 ret = 0;
    switch( pIrpCtx->GetMajorCmd() )
    {
    case IRP_MJ_FUNC:
        {
            BufPtr pBuf( true );

             pBuf->Resize( 
                sizeof( IRPCTX_EXTDSP ) );

            if( ERROR( ret ) )
                break;

            pExt = new ( pBuf->ptr() )
                IRPCTX_EXTDSP();

            if( pExt == nullptr )
                ret = -ENOMEM;

            break;
        }
    default:
        {
            ret = super::AllocIrpCtxExt(
                pIrpCtx, pContext );
            break;
        }
    }
    return ret;
}

gint32 CDBusStreamPdo::CompleteSendReq(
    IRP* pIrp )
{

    gint32 ret = -EINVAL;
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
    {
        return ret;
    }

    do{
        // let's process the func irps
        IrpCtxPtr pCtx = pIrp->GetTopStack();
        guint32 dwIoDir = pCtx->GetIoDirection();
        
        if( dwIoDir != IRP_DIR_INOUT )
            break;

        ret = pCtx->GetStatus();
        if( ERROR( ret ) )
            break;

        BufPtr pExtBuf;
        pCtx->GetExtBuf( pExtBuf );
        if( unlikely( pExtBuf.IsEmpty() ) )
        {
            ret = -EFAULT;
            break;
        }
        IRPCTX_EXTDSP* pExt = pExtBuf->ptr();

        if( pExt->m_bWaitWrite )
        {
            // write operation completed

            pExt->m_bWaitWrite = false;
            DMsgPtr pMsg = *pCtx->m_pReqData;
            guint32 dwSerial = 0;
            ret = pMsg->GetSerial( &dwSerial );
            if( ERROR( ret ) )
                break;

            if( dwSerial == 0 )
            {
                ret = -EINVAL;
                break;
            }

            // put the irp to the reading queue for
            // the response we need to check if the
            // port is still in the valid state for
            // io request
            guint32 dwOldState = 0;
            CStdRMutex oPortLock( GetLock() );

            ret = CanContinue( pIrp,
                PORT_STATE_BUSY_SHARED, &dwOldState );

            if( SUCCEEDED( ret ) )
            {
                auto itr =
                    m_mapRespMsgs.find( dwSerial );
                if( itr == m_mapRespMsgs.end() )
                {
                    m_mapSerial2Resp[ dwSerial ] = pIrp;
                    ret = STATUS_PENDING;
                }
                else
                {
                    // the message arrives before
                    // the irp arrives here
                    DMsgPtr pRespMsg = itr->second;
                    m_mapRespMsgs.erase( itr );

                    DMsgPtr& pReqMsg = *pCtx->m_pReqData;
                    guint32 dwOldSeqNo = 0;
                    pReqMsg.GetSerial( dwOldSeqNo );
                    pRespMsg.SetReplySerial( dwOldSeqNo );

                    BufPtr pRespBuf( true );
                    *pRespBuf = pRespMsg;
                    pCtx->SetRespData( pRespBuf );
                    pCtx->SetStatus( ret );
                }
            }
            PopState( dwOldState );
            break;
        }
        else
        {
            ret = pCtx->GetStatus();
            // the response data 
            if( ERROR( ret ) )
                break;

            BufPtr& pBuf = pCtx->m_pRespData;
            if( pBuf.IsEmpty() )
            {
                ret = -ENODATA;
                pCtx->SetStatus( ret );
            }
        }

    }while( 0 );

    return ret;
}

gint32 CDBusStreamPdo::CompleteIoctlIrp(
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

        if( pIrp->MajorCmd() != IRP_MJ_FUNC &&
            pIrp->MinorCmd() != IRP_MN_IOCTL )
        {
            ret = -EINVAL;
            break;
        }
        
        IrpCtxPtr pCtx = pIrp->GetCurCtx();

        switch( pIrp->CtrlCode() )
        {
        case CTRLCODE_SEND_REQ:
        case CTRLCODE_SEND_RESP:
        case CTRLCODE_SEND_EVENT:
            {
                ret = CompleteSendReq( pIrp );
                break;
            }
        case CTRLCODE_LISTENING:
            {
                // copy the response data to the upper
                // stack for unknown irps
                ret = CompleteListening( pIrp );
                break;
            }
        default:
            ret = -EINVAL;
            break;
        }

    }while( 0 );

    return ret;
}

gint32 CDBusStreamPdo::HandleListening(
    IRP* pIrp )
{

    // the irp carry a buffer to receive, it is
    // assumed to be a DMsgPtr
    gint32 ret = 0;
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 
        super::HandleListening( pIrp );

    if( ERROR( ret ) )
        return ret;

    if( IsServer() )
    {
        CRpcReqStreamServer *pIf =
            GetStreamIf();
        pIf->AddListeningReq();
    }
    else
    {
        CRpcReqStreamProxy *pIf =
            GetStreamIf();
        pIf->AddListeningReq();
    }
    
    return ret;
}

gint32 CDBusStreamPdo::CompleteListening(
    IRP* pIrp )
{
    gint32 ret = 0;
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    IrpCtxPtr& pCtx = pIrp->GetTopStack();
    do{
        ret = pCtx->GetStatus();
        if( ERROR( ret ) )
            break;

        DMsgPtr& pRespMsg =
            *pCtx->m_pRespData;

        stdstr strSender = pRespMsg.GetSender();
        if( strSender.empty() ||
            strSender[ 0 ] != ':' )
        {
            ret = -EBADMSG;
            break;
        }

        stdstr strStream = strSender.substr( 1 );
        if( strStream.empty() )
        {
            ret = -EBADMSG;
            break;
        }

        HANDLE hStream = 0;
#if BUILD_64 == 1
        hStream = strtoll(
            strStream.c_str(), nullptr, 10 );
#else
        hStream = strtol(
            strStream.c_str(), nullptr, 10 );
#endif

        stdstr strHash;
        if( IsServer() )
        {
            CRpcReqStreamServer *pIf =
                GetStreamIf();
            ret = pIf->GetSessHash(
                hStream, strHash );
            if( ERROR( ret ) )
                break;
        }
        else
        {
            CRpcReqStreamProxy *pIf =
                GetStreamIf();
            ret = pIf->GetSessHash(
                hStream, strHash );
            if( ERROR( ret ) )
                break;
        }
        
        BufPtr pExtBuf;
        IrpCtxPtr& pCtx = pIrp->GetCurCtx();
        pCtx->GetExtBuf( pExtBuf );
        if( unlikely( pExtBuf.IsEmpty() ) )
        {
            ret = -EFAULT;
            break;
        }

        IRPCTX_EXTDSP* pExt = pExtBuf->ptr();
        pExt->m_hStream = hStream;

        strncpy( pExt->m_szHash,
            strHash.c_str(), SESS_HASH_SIZE );

    }while( 0 );

    if( ERROR( ret ) )
    {
        pCtx->SetStatus( ret );
        pCtx->m_pRespData.clear();
    }

    return ret;
}
