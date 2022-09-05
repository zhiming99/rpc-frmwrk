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

namespace rpcf
{

static gint32 FireRmtSvrEvent(
    IPort* pPort, EnumEventId iEvent, HANDLE hStream )
{
    // this is an event detected by the
    // socket
    gint32 ret = 0;

    switch( iEvent )
    {
    case eventRmtSvrOffline:
    case eventRmtSvrOnline:
        {
            CCfgOpenerObj oCfg( pPort );
            auto pcPort = dynamic_cast
                < CDBusStreamPdo* >( pPort );
            if( pcPort == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            Variant var;
            pcPort->GetProperty(
                propBusPortPtr, var );
            CPort* pBus = ( ObjPtr& )var;
            guint32 dwPortId = 0;
            ret = oCfg.GetIntProp(
                propPortId, dwPortId );

            CCfgOpener oEvtCtx;
            oEvtCtx.SetIntPtr( propStmHandle,
                ( guint32* )hStream );
            oEvtCtx[ propPortId ] = dwPortId;

            IConfigDb* pEvtExt =
                oEvtCtx.GetCfg();

            // pass on this event to the pnp
            // manager
            CEventMapHelper< CPort >
                oEvtHelper( pBus );

            HANDLE hPort =
                PortToHandle( pPort );

            oEvtHelper.BroadcastEvent(
                eventConnPoint, iEvent,
                ( LONGWORD )pEvtExt,
                ( LONGWORD* )hPort );

            break;
        }
    default:
        {
            ret = -EINVAL;
            break;
        }
    }
    return ret;
}

CDBusStreamPdo::CDBusStreamPdo(
    const IConfigDb* pCfg )
    : super( pCfg )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg( pCfg );
        ret = oCfg.GetBoolProp(
            propIsServer, m_bServer );
        if( ERROR( ret ) )
            break;

        if( IsServer() )
        {
            HANDLE hstm;
            ret = oCfg.GetIntPtr( propStmHandle,
                ( guint32*& )hstm );
            if( ERROR( ret ) )
                break;

            SetStream( hstm );
        }

        Sem_Init( &m_semFireSync, 0, 0 );
        SetClassId( clsid( CDBusStreamPdo ) );
        m_dwDisconned = 0;

    }while( 0 );

    if( ERROR( ret ) )
    {
        stdstr strMsg = DebugMsg( ret,
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
        auto pExt =
            ( IRPCTX_EXTDSP* )pExtBuf->ptr();
        HANDLE hStream = GetStream();

        BufPtr pBuf( true );
        ret = pMsg.Serialize( *pBuf );
        if( ERROR( ret ) )
            break;

        pExt->m_bWaitWrite = true;

        CCfgOpener oCtx;
        oCtx.SetPointer( propIrpPtr, pIrp );
        CRpcServices* pSvc = GetStreamIf();
        if( IsServer() )
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

gint32 CDBusStreamBusPort::BroadcastDBusMsg(
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
        auto pExt =
            ( IRPCTX_EXTDSP* )pExtBuf->ptr();
        pExt->m_bWaitWrite = true;

        BufPtr pBuf( true );
        ret = pMsg.Serialize( *pBuf );
        if( ERROR( ret ) )
            break;

        CCfgOpener oCtx;
        oCtx.SetPointer( propIrpPtr, pIrp );
        CRpcServices* pSvc = GetStreamIf();
        if( IsServer() )
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

gint32 CDBusStreamPdo::SubmitIoctlCmd(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
    {
        return -EINVAL;
    }

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
        case CTRLCODE_SEND_RESP:
            {
                // the m_pReqData contains a
                // pointer to DMsgPtr
                ret = HandleSendReq( pIrp );
                break;
            }
        default:
            {
                ret = super::SubmitIoctlCmd( pIrp );
                break;
            }
        }
    }while( 0 );

    if( ret != STATUS_PENDING )
        pCtx->SetStatus( ret );

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
            auto *pBusPort = static_cast
                < CDBusStreamBusPort* >( m_pBusPort );

            dwSerial =
               CDBusBusPort::LabelMessage( pMsg );

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

ObjPtr& CDBusStreamPdo::GetStreamIf()
{
    auto pPort = static_cast
        < CDBusStreamBusPort* >( m_pBusPort );
    return pPort->GetStreamIf();
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

            auto *pBusPort = static_cast
                < CDBusStreamBusPort* >( m_pBusPort );

            dwSerial =
                CDBusBusPort::LabelMessage( pMsg );

            pMsg.SetNoReply( true );
            ret = pBusPort->BroadcastDBusMsg(
                pIrp, pMsg );
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

            auto pExt = new ( pBuf->ptr() )
                IRPCTX_EXTDSP();

            if( pExt == nullptr )
                ret = -ENOMEM;

            pIrpCtx->SetExtBuf( pBuf );

            break;
        }
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
                ret = super::AllocIrpCtxExt(
                    pIrpCtx, pContext );
                break;
            }
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

gint32 CDBusStreamPdo::OnRespMsgNoIrp(
    DBusMessage* pDBusMsg )
{
    // this method is guarded by the port lock
    //
    // return -ENOTSUP or ENOENT will tell the dispatch
    // routine to move on to next port
    if( pDBusMsg == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        DMsgPtr pMsg( pDBusMsg );
        guint32 dwSerial = 0;
        ret = pMsg.GetReplySerial( dwSerial );
        if( ERROR( ret ) )
            break;

        if( dwSerial == 0 )
        {
            ret = -EBADMSG;
            break;
        }

        m_mapRespMsgs[ dwSerial ] = pMsg;

    }while( 0 );

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

        auto pExt =
            ( IRPCTX_EXTDSP* )pExtBuf->ptr();

        if( pExt->m_bWaitWrite )
        {
            // write operation completed

            pExt->m_bWaitWrite = false;
            DMsgPtr& pMsg = *pCtx->m_pReqData;
            guint32 dwSerial = 0;
            ret = pMsg.GetSerial( dwSerial );
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
            CStdRMutex oPortLock( GetLock() );

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

            oPortLock.Unlock();
            if( ERROR( ret ) )
                break;

            if( IsServer() )
            {
                CRpcStreamChannelSvr *pIf =
                    GetStreamIf();
                pIf->AddRecvReq( GetStream() );
            }
            else
            {
                CRpcStreamChannelCli *pIf =
                    GetStreamIf();
                pIf->AddRecvReq( GetStream() );
            }
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

gint32 CDBusStreamPdo::IsIfSvrOnline(
    const DMsgPtr& pMsg )
{
    auto *pBusPort = static_cast
        < CDBusStreamBusPort* >( m_pBusPort );
    CRpcServices* pSvc = pBusPort->GetStreamIf();
    if( pSvc == nullptr ||
        pSvc->GetState() != stateConnected );
        return -ENOTCONN;
    return STATUS_SUCCESS;
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

    ret = super::HandleListening( pIrp );

    if( ERROR( ret ) )
        return ret;

    if( IsServer() )
    {
        CRpcStreamChannelSvr *pIf =
            GetStreamIf();
        pIf->AddRecvReq( GetStream() );
    }
    else
    {
        CRpcStreamChannelCli *pIf =
            GetStreamIf();
        pIf->AddRecvReq( GetStream() );
    }
    
    return ret;
}

gint32 CDBusStreamPdo::CompleteListening(
    IRP* pIrp )
{
    return STATUS_SUCCESS;
}

gint32 GetPreStopStep(
    PIRP pIrp, guint32& dwStepNo )
{
    BufPtr pBuf;
    pIrp->GetCurCtx()->GetExtBuf( pBuf );
    if( pBuf.IsEmpty() || pBuf->empty() )
        return -EINVAL;

    PORT_START_STOP_EXT* psse =
        ( PORT_START_STOP_EXT* )pBuf->ptr();

    dwStepNo = *( guint32* )&psse[ 1 ];
    return 0;
}

gint32 SetPreStopStep(
    PIRP pIrp, guint32 dwStepNo )
{
    BufPtr pBuf;
    pIrp->GetCurCtx()->GetExtBuf( pBuf );
    if( pBuf.IsEmpty() || pBuf->empty() )
        return -EINVAL;

    PORT_START_STOP_EXT* psse =
        ( PORT_START_STOP_EXT* )pBuf->ptr();

    *( guint32* )&psse[ 1 ] = dwStepNo;
    return 0;
}

gint32 CDBusStreamPdo::PreStop(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    guint32 dwStepNo = 0;
    do{
        ret = GetPreStopStep( pIrp, dwStepNo );
        while( dwStepNo == 0 )
        {
            HANDLE hstm = GetStream();
            auto *pBusPort = static_cast
            < CDBusStreamBusPort* >( m_pBusPort );
            pBusPort->RemoveBinding( hstm );
            SetPreStopStep( pIrp, 1 );

            CCfgOpener oReqCtx;
            oReqCtx[ propIrpPtr ] = ObjPtr( pIrp );
            oReqCtx[ propPortPtr ] = ObjPtr( this );

            CRpcServices* pSvc = GetStreamIf();
            if( pSvc->GetState() != stateConnected )
            {
                SetPreStopStep( pIrp, 1 );
                break;
            }

            TaskletPtr pStopCb;
            if( IsServer() )
            {
                CRpcStmChanSvr* pstm = GetStreamIf();
                ret = NEW_PROXY_RESP_HANDLER2(
                    pStopCb, GetStreamIf(), 
                    &CRpcStmChanSvr::OnStopStmComplete,
                    ( IEventSink* )nullptr,
                    ( IConfigDb* )oReqCtx.GetCfg() );
                if( SUCCEEDED( ret ) )
                {
                    ret = pstm->ActiveClose(
                        hstm, pStopCb );
                }
            }
            else
            {
                CRpcStmChanCli* pstm = GetStreamIf();
                    ret = NEW_PROXY_RESP_HANDLER2(
                        pStopCb, GetStreamIf(), 
                        &CRpcStmChanCli::OnStopStmComplete,
                        ( IEventSink* )nullptr,
                        ( IConfigDb* )oReqCtx.GetCfg() );
                if( SUCCEEDED( ret ) )
                {
                    ret = pstm->ActiveClose(
                        hstm, pStopCb );
                }
            }
            if( ret == STATUS_PENDING )
            {
                ret = STATUS_MORE_PROCESS_NEEDED;
                break;
            }
            if( !pStopCb.IsEmpty() )
                ( *pStopCb )( eventCancelTask );
            SetPreStopStep( pIrp, 1 );
            break;
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

gint32 CDBusStreamPdo::PostStart(
    IRP* pIrp )
{
    gint32 ret = 0;
    if( IsServer() )
        return STATUS_SUCCESS;
    do{
        CStreamProxySync* pstm = GetStreamIf();
        if( pstm == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        CCfgOpener oReqCtx;
        oReqCtx[ propIrpPtr ] = ObjPtr( pIrp );
        oReqCtx[ propPortPtr ] = ObjPtr( this );

        TaskletPtr pRespCb;
        ret = NEW_PROXY_RESP_HANDLER2(
            pRespCb, GetStreamIf(), 
            &CRpcStreamChannelCli::OnStartStmComplete,
            ( IEventSink* )nullptr,
            ( IConfigDb* )oReqCtx.GetCfg() );
        if( ERROR( ret ) )
            break;

        CCfgOpener oDesc;
        HANDLE hstm = INVALID_HANDLE;

        ret = pstm->StartStream(
            hstm, nullptr, pRespCb );

        if( ret == STATUS_PENDING )
            break;

        if( SUCCEEDED( ret ) )
            SetStream( hstm );

        ( *pRespCb )( eventCancelTask );
        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        pCtx->SetStatus( ret );

    }while( 0 );

    return ret;
}

gint32 CDBusStreamPdo::OnPortReady( IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        ret = super::OnPortReady( pIrp );
        if( ERROR( ret ) )
            break;

        auto *pBusPort = static_cast
            < CDBusStreamBusPort* >( m_pBusPort );

        HANDLE hStream = GetStream();
        pBusPort->BindStreamPort(
            hStream, PortPtr( this ) );

        if( IsServer() )
        {
            FireRmtSvrEvent( this,
                eventRmtSvrOnline, hStream );
            Sem_Post( &m_semFireSync );
        }

    }while( 0 );

    pIrp->GetCurCtx()->SetStatus( ret );
    return ret;
}

gint32 CDBusStreamPdo::OnEvent(
    EnumEventId iEvent,
    LONGWORD dwParam1,
    LONGWORD dwParam2,
    LONGWORD* data )
{
    gint32 ret = 0;
    switch( iEvent )
    {
    case eventDisconn:
        {
            // passive disconnection detected
            if( m_dwDisconned++ > 0 )
            {
                DebugPrint( m_dwDisconned,
                    "Repeated closing of "
                    "StreamPdo 0x%llx",
                    ( HANDLE )this );
                break;
            }
            if( IsServer() )
            {
                Sem_Wait( &m_semFireSync );
            }
            ret = FireRmtSvrEvent( this,
                eventRmtSvrOffline,
                ( HANDLE )dwParam1 );
            break;
        }
    default:
        ret = super::OnEvent( iEvent,
            dwParam1, dwParam2, data );
        break;
    }

    return ret;
}

CDBusStreamBusPort::CDBusStreamBusPort(
    const IConfigDb* pCfg ) :
    super( pCfg )
{
    gint32 ret = 0;
    do{
        SetClassId( clsid( CDBusStreamBusPort ) );
        auto pMgr = GetIoMgr();
        if( pMgr == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        ret = pMgr->GetCmdLineOpt(
            propIsServer, m_bServer );
        if( ERROR( ret ) )
            break;

    }while( 0 );
    if( ERROR( ret ) )
    {
        throw std::runtime_error( 
            DebugMsg( ret, "Error occurs in ctor "
                "of CDBusStreamBusPort" ) );
    }
}

gint32 CDBusStreamBusPort::PostStart(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CCfgOpener oCfg;
        oCfg[ propIsServer ] = IsServer();
        CIoManager* pMgr = GetIoMgr();
        stdstr strDesc;
        ret = pMgr->GetCmdLineOpt(
            propObjDescPath, strDesc );
        if( ERROR( ret ) )
            break;

        stdstr strObjName =
            OBJNAME_RPC_STREAMCHAN_SVR;

        oCfg[ propIoMgr ] = ObjPtr( pMgr );
        ret = CRpcServices::LoadObjDesc(
            strDesc, strObjName, IsServer(),
            oCfg.GetCfg() );
        if( ERROR( ret ) )
            break;

        InterfPtr pIf;
        if( IsServer() )
            ret = pIf.NewObj(
                clsid( CRpcStreamChannelSvr ),
                oCfg.GetCfg() );
        else
            ret = pIf.NewObj(
                clsid( CRpcStreamChannelCli ),
                oCfg.GetCfg() );

        if( ERROR( ret ) )
            break;

        CRpcServices* pSvc = pIf;
        if( IsServer() )
        {
            ret = pSvc->Start();
            if( SUCCEEDED( ret ) )
            {
                SetStreamIf( pIf );
                CRpcStreamChannelSvr* pSvr = pIf;
                pSvr->SetPort( this );
            }
        }
        else
        {
            CRpcStreamChannelCli* pCli = pIf;
            TaskletPtr pStartTask;
            ret = DEFER_IFCALLEX_NOSCHED2(
                0, pStartTask, pSvc, 
                &CRpcServices::StartEx,
                ( IEventSink* )nullptr );
            if( ERROR( ret ) )
                break;

            CCfgOpener oReqCtx;
            oReqCtx.SetPointer( propIrpPtr, pIrp );
            oReqCtx.SetPointer( propPortPtr, this );

            TaskletPtr pRespCb;
            ret = NEW_PROXY_RESP_HANDLER2(
                pRespCb, ObjPtr( pSvc ), 
                &CRpcStreamChannelCli::OnStartStopComplete,
                ( IEventSink* )nullptr,
                ( IConfigDb* )oReqCtx.GetCfg() );
            if( ERROR( ret ) )
            {
                ( *pStartTask )( eventCancelTask );
                break;
            }
            CIfRetryTask* pRetry = pStartTask;
            pRetry->SetClientNotify( pRespCb );
            auto pMgr = this->GetIoMgr();
            ret = pMgr->RescheduleTask(
                pStartTask );
            if( ERROR( ret ) )
            {
                ( *pStartTask )( eventCancelTask );
                ( *pRespCb )( eventCancelTask );
            }
            else
            {
                ret = STATUS_PENDING;
            }
        }

    }while( 0 );

    return ret;
}

gint32 CDBusStreamBusPort::StopStreamChan(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        ObjPtr pIf = GetStreamIf();
        TaskletPtr pStopTask;
        ret = DEFER_IFCALLEX_NOSCHED2(
            0, pStopTask, pIf, 
            &CRpcServices::StopEx,
            ( IEventSink* )nullptr );
        if( ERROR( ret ) )
            break;

        CCfgOpener oReqCtx;
        oReqCtx.SetPointer( propIrpPtr, pIrp );
        oReqCtx.SetPointer( propPortPtr, this );
        TaskletPtr pRespCb;
        if( IsServer() )
        {
            ret = NEW_PROXY_RESP_HANDLER2(
                pRespCb, pIf, 
                &CRpcStreamChannelSvr::OnStartStopComplete,
                ( IEventSink* )nullptr,
                ( IConfigDb* )oReqCtx.GetCfg() );
        }
        else
        {
            ret = NEW_PROXY_RESP_HANDLER2(
                pRespCb, pIf, 
                &CRpcStreamChannelCli::OnStartStopComplete,
                ( IEventSink* )nullptr,
                ( IConfigDb* )oReqCtx.GetCfg() );
        }
        if( ERROR( ret ) )
        {
            ( *pStopTask )( eventCancelTask );
            break;
        }
        CIfRetryTask* pRetry = pStopTask;
        pRetry->SetClientNotify( pRespCb );
        auto pMgr = this->GetIoMgr();
        ret = pMgr->RescheduleTask(
            pStopTask );
        if( ERROR( ret ) )
        {
            ( *pStopTask )( eventCancelTask );
            ( *pRespCb )( eventCancelTask );
        }
        else
        {
            ret = STATUS_PENDING;
        }

    }while( 0 );

    return ret;
}

gint32 CDBusStreamBusPort::Stop( PIRP pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    return StopStreamChan( pIrp );
}

void CDBusStreamBusPort::BindStreamPort(
    HANDLE hStream, PortPtr pPort )
{
    if( hStream == INVALID_HANDLE ||
        pPort.IsEmpty() )
        return;
    CStdRMutex oPortLock( GetLock() );
    m_mapStm2Port[ hStream ] = pPort;
}

void CDBusStreamBusPort::RemoveBinding(
    HANDLE hStream )
{
    CStdRMutex oPortLock( GetLock() );
    m_mapStm2Port.erase( hStream );
}

gint32 CDBusStreamBusPort::GetStreamPort(
    HANDLE hStream, PortPtr& pPort )
{
    if( hStream == INVALID_HANDLE )
        return -EINVAL;;
    CStdRMutex oPortLock( GetLock() );
    auto itr = m_mapStm2Port.find( hStream );
    if( itr == m_mapStm2Port.end() )
        return -ENOENT;
    pPort = itr->second;
    return STATUS_SUCCESS;
}

CDBusStreamBusDrv::CDBusStreamBusDrv(
    const IConfigDb* pCfg ): 
    super( pCfg )
{
    SetClassId( clsid( CDBusStreamBusDrv ) );
}

gint32 CDBusStreamBusDrv::Probe(
        IPort* pLowerPort,
        PortPtr& pNewPort,
        const IConfigDb* pConfig )
{
    gint32 ret = 0;

    do{
        // we don't have dynamic bus yet
        CfgPtr pCfg( true );

        // make a copy of the input args
        if( pConfig != nullptr )
            *pCfg = *pConfig;

        CCfgOpener oCfg( ( IConfigDb* )pCfg );

        ret = oCfg.SetStrProp( propPortClass,
            PORT_CLASS_DBUS_STREAM_BUS );

        if( ERROR( ret ) )
            break;

        ret = oCfg.SetIntProp( propClsid,
            clsid( CDBusStreamBusPort ) );

        // FIXME: we need a better task to receive the
        // notification of the port start events
        TaskletPtr pTask;
        ret = pTask.NewObj( clsid( CSyncCallback ) );
        if( ERROR( ret ) )
            break;

        // we don't care the event sink to bind
        ret = oCfg.SetObjPtr( propEventSink,
            ObjPtr( pTask ) );

        if( ERROR( ret ) )
            break;

        ret = CreatePort( pNewPort, pCfg );
        if( ERROR( ret ) )
            break;

        if( pLowerPort != nullptr )
        {
            ret = pNewPort->AttachToPort( pLowerPort );
        }

        if( ERROR( ret ) )
            break;

        ret = NotifyPortAttached( pNewPort );
        if( ret != STATUS_PENDING )
            break;

        // waiting for the start to complete
        CSyncCallback* pSyncTask = pTask;
        if( pSyncTask != nullptr )
        {
            ret = pSyncTask->WaitForComplete();
            if( SUCCEEDED( ret ) )
                ret = pSyncTask->GetError();
        }

    }while( 0 );

    return ret;
}

gint32 CDBusStreamBusPort::CreatePdoPort(
    IConfigDb* pCfg,
    PortPtr& pNewPort )
{
    gint32 ret = 0;

    // Note that we are within a port lock
    do{
        if( pCfg == nullptr )
            return -EINVAL;

        if( !pCfg->exist( propPortClass ) )
        {
            ret = -EINVAL;
            break;
        }

        std::string strPortClass;
        CCfgOpener oExtCfg( ( IConfigDb* )pCfg );

        ret = oExtCfg.GetStrProp(
            propPortClass, strPortClass );

        // by default, we creaete the tcp stream
        // pdo, and the only pdo we support
        if( ERROR( ret ) )
        {
            strPortClass =
                PORT_CLASS_DBUS_STREAM_PDO;
            ret = 0;
        }

        if( strPortClass
            == PORT_CLASS_DBUS_STREAM_PDO )
        {
            ret = CreateDBusStreamPdo(
                pCfg, pNewPort );
        }
        else
        {
            ret = -ENOTSUP;
        }

    }while( 0 );

    return ret;
}

gint32 CDBusStreamBusPort::CreateDBusStreamPdo(
    IConfigDb* pCfg,
    PortPtr& pNewPort )
{
    if( pCfg == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        std::string strPortName;
        guint32 dwPortId = 0;

        CCfgOpener oExtCfg(
            ( IConfigDb* )pCfg );

        ret = oExtCfg.GetIntProp(
            propPortId, dwPortId );

        if( ERROR( ret ) )
            break;

        // verify if the port already exists
        if( this->PortExist( dwPortId ) )
        {
            ret = -EEXIST;
            break;
        }

        oExtCfg.SetBoolProp(
            propIsServer, IsServer() );

        ret = pNewPort.NewObj(
            clsid( CDBusStreamPdo ), pCfg );

        // the pdo port `Start()' will be deferred
        // till the complete port stack is built.
        //
        // And well, we are done here

    }while( 0 );

    return ret;
}

gint32 CDBusStreamBusPort::BuildPdoPortName(
    IConfigDb* pCfg,
    std::string& strPortName )
{
    if( pCfg == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CCfgOpener oCfg( pCfg );

        std::string strClass;
        ret = oCfg.GetStrProp(
            propPortClass, strClass );

        if( ERROR( ret ) )
            break;

        // port name is composed in the following
        // format:
        // <PortClassName> + "_" + <PortId>

        if( strClass !=
            PORT_CLASS_DBUS_STREAM_PDO )
        {
            ret = -EINVAL;
            break;
        }

        guint32 dwPortId = 0;

        ret = oCfg.GetIntProp(
            propPortId, dwPortId );
        if( ERROR( ret ) )
        {
            ret = 0;
            dwPortId = NewPdoId();
            oCfg.SetIntProp(
                propPortId, dwPortId );
        }

        strPortName = strClass +
            std::string( "_" )
            + std::to_string( dwPortId );

    }while( 0 );

    return ret;
}

gint32 CDBusStreamBusPort::OnNewConnection(
    HANDLE hStream, PortPtr& pPort )
{
    gint32 ret = 0; 
    do{
        CCfgOpener oCfg;
        oCfg[ propIsServer ] = IsServer();

        oCfg[ propPortClass ] =
            PORT_CLASS_DBUS_STREAM_PDO;

        stdstr strBus =
            PORT_CLASS_DBUS_STREAM_BUS;
        strBus += "_0";
        oCfg[ propBusName ] = strBus;

        oCfg.SetIntPtr( propStmHandle,
            ( guint32*& )hStream );

        ret = OpenPdoPort(
            oCfg.GetCfg(), pPort );
        if( ret == STATUS_PENDING )
            ret = STATUS_SUCCESS;

    }while( 0 );

    return ret;
}

gint32 CDBusStreamBusPort::OnEvent(
    EnumEventId iEvent,
    LONGWORD dwParam1,
    LONGWORD dwParam2,
    LONGWORD* data )
{
    gint32 ret = 0;
    switch( iEvent )
    {
    case eventNewConn:
        {
            // passive disconnection detected
            PortPtr pPort;
            ret = OnNewConnection(
                ( HANDLE )dwParam1, pPort );
            if( ERROR( ret ) )
                break;

            auto pctx = ( IConfigDb* )data;
            CCfgOpener oCtx( pctx );
            oCtx[ propPortPtr ] = ObjPtr( pPort );
            break;
        }
    default:
        ret = super::OnEvent( iEvent,
            dwParam1, dwParam2, data );
        break;
    }
    return ret;
}

}
