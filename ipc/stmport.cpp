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

static gint32 FireRmtSvrEvent2(
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
            if( ERROR( ret ) )
                break;

            CCfgOpener oEvtCtx;
            oEvtCtx.SetIntPtr( propStmHandle,
                ( guint32* )hStream );
            oEvtCtx[ propPortId ] = dwPortId;

            IConfigDb* pEvtExt = oEvtCtx.GetCfg();

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
    : super( pCfg ),
    m_dwDisconned( 0 ),
    m_bSkelReady( false )
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

        SetClassId( clsid( CDBusStreamPdo ) );

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
    PIRP pIrp, IConfigDb* pMsg )
{
    gint32 ret = 0;
    if( pIrp == nullptr || pMsg == nullptr )
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
        FASTRPC_MSG ofrmsg;
        ofrmsg.m_pCfg = pMsg;
        ret = ofrmsg.Serialize( *pBuf );
        if( ERROR( ret ) )
            break;

        pExt->m_bWaitWrite = true;

        CCfgOpener oCtx;
        oCtx.SetPointer( propIrpPtr, pIrp );
        CRpcServices* pSvc = GetStreamIf();

        guint32 dwIoDir = pCtx->GetIoDirection();
        if( dwIoDir == IRP_DIR_OUT )
        {
            // the downstream request handler has its
            // own timer to cover the write operation.
            pIrp->RemoveTimer();
        }

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
                // pointer to the request to send
                ret = HandleSendReq( pIrp );
                break;
            }
        case CTRLCODE_SKEL_READY:
            {
                ret = HandleSkelReady( pIrp );
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
            IConfigDb* pMsg = *pCtx->m_pReqData;
            auto *pBusPort = static_cast
                < CDBusStreamBusPort* >( m_pBusPort );

            CCfgOpener oMsg( pMsg );
            ret = oMsg.GetIntProp(
                propSeqNo, dwSerial );
            if( ERROR( ret ) )
                break;

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
                m_mapSerial2Resp.insert(
                    { dwSerial, IrpPtr( pIrp )} );
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
            IConfigDb* pMsg = *pCtx->m_pReqData;
            CReqOpener oMsg( pMsg );
            guint32 dwFlags = 0;
            ret = oMsg.GetCallFlags( dwFlags );
            if( ERROR( ret ) )
                break;
            dwFlags &= CF_MESSAGE_TYPE_MASK;
            if( dwFlags !=
                DBUS_MESSAGE_TYPE_SIGNAL )
            {
                ret = -EINVAL;
                break;
            }

            if( ERROR( ret ) )
                break;

            ret = SendDBusMsg( pIrp, pMsg );
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
    IConfigDb* pRespMsg )
{
    // this method is guarded by the port lock
    //
    // return -ENOTSUP or ENOENT will tell the dispatch
    // routine to move on to next port
    if( pRespMsg == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CCfgOpener oResp( pRespMsg );
        guint32 dwSerial = 0;
        ret = oResp.GetIntProp(
            propSeqNo2, dwSerial );
        if( ERROR( ret ) )
            break;

        m_mapRespMsgs[ dwSerial ] =
            CfgPtr( pRespMsg );

    }while( 0 );

    return ret;
}

gint32 CDBusStreamPdo::CompleteSendReq(
    IRP* pIrp )
{

    gint32 ret = -EINVAL;
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return ret;

    do{
        // let's process the func irps
        IrpCtxPtr pCtx = pIrp->GetTopStack();
        guint32 dwIoDir = pCtx->GetIoDirection();
        
        if( dwIoDir != IRP_DIR_INOUT )
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

        ret = pCtx->GetStatus();
        if( pExt->m_bWaitWrite )
        {
            // write operation failed
            if( ERROR( ret ) )
                break;

            // write operation completed
            pExt->m_bWaitWrite = false;
            IConfigDb* pReqMsg =
                ( ObjPtr& )*pCtx->m_pReqData;
            CCfgOpener oMsg( pReqMsg );
            guint32 dwSerial = 0;
            ret = oMsg.GetIntProp(
                propSeqNo, dwSerial );
            if( ERROR( ret ) )
                break;

            // put the irp to the reading queue for
            // the response we need to check if the
            // port is still in the valid state for
            // io request
            CStdRMutex oPortLock( GetLock() );

            auto itr =
                m_mapRespMsgs.find( dwSerial );
            if( itr == m_mapRespMsgs.end() )
            {
                m_mapSerial2Resp.insert(
                    { dwSerial, IrpPtr( pIrp )} );
                ret = STATUS_PENDING;
            }
            else
            {
                // the message arrives before
                // the irp arrives here
                IConfigDb* pRespMsg = itr->second;

                BufPtr pRespBuf( true );
                *pRespBuf = ObjPtr( pRespMsg );
                pCtx->SetRespData( pRespBuf );
                pCtx->SetStatus( ret );

                m_mapRespMsgs.erase( itr );
            }

            oPortLock.Unlock();

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
            if( SUCCEEDED( ret ) )
                break;

            // Error happens
            // remove the request from the irp map
            IConfigDb* pReqMsg =
                ( ObjPtr& )*pCtx->m_pReqData;
            CCfgOpener oMsg( pReqMsg );
            guint32 dwSerial = 0;
            ret = oMsg.GetIntProp(
                propSeqNo, dwSerial );
            if( ERROR( ret ) )
                break;

            CStdRMutex oPortLock( GetLock() );
            auto itr =
                m_mapRespMsgs.find( dwSerial );
            if( itr != m_mapRespMsgs.end() )
                m_mapRespMsgs.erase( itr );
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

gint32 CDBusStreamPdo::IsIfSvrOnline()
{
    auto *pBusPort = static_cast
        < CDBusStreamBusPort* >( m_pBusPort );
    CRpcServices* pSvc = pBusPort->GetStreamIf();
    if( pSvc == nullptr ||
        pSvc->GetState() != stateConnected );
        return -ENOTCONN;
    return STATUS_SUCCESS;
}

gint32 CDBusStreamPdo::HandleListeningInternal(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    IrpPtr pIrpLocked( pIrp );

    do{
        IrpCtxPtr& pCtx = pIrp->GetCurCtx();

        IMessageMatch* pMatch =
            *( ObjPtr* )( *pCtx->m_pReqData );

        if( pMatch == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        CStdRMutex oPortLock( GetLock() );

        MatchMap *pMap = nullptr;

        MatchPtr matchPtr( pMatch );

        ret = GetMatchMap( pIrp, pMap );
        if( ERROR( ret ) )
            break;

        MatchMap::iterator itr =
            pMap->find( matchPtr );

        if( itr != pMap->end() )
        {
            // first, check if there are pending
            // messages if there are, this irp can
            // return immediately with the first
            // pending message.
            MATCH_ENTRY& oMe = itr->second;

            if( !oMe.IsConnected() )
            {
                ret = -ENOTCONN;
                break;
            }
            if( oMe.GetMsgQue< CfgPtr >().size() &&
                oMe.m_queWaitingIrps.empty() )
            {
                BufPtr pBuf( true );
                *pBuf = oMe.GetMsgQue< CfgPtr >().front();
                pCtx->SetRespData( pBuf );

                oMe.GetMsgQue< CfgPtr >().pop_front();
                // immediate return
            }
            else if( oMe.GetMsgQue< CfgPtr >().size() &&
                !oMe.m_queWaitingIrps.empty() )
            {
                bool bFound = false;
                oMe.m_queWaitingIrps.push_back( pIrp );
                std::deque< IrpPtr > queIrps;
                std::deque< CfgPtr > queMsgs;
                while( oMe.m_queWaitingIrps.size() &&
                    oMe.GetMsgQue< CfgPtr >().size() )
                {
                    queIrps.push_back(
                        oMe.m_queWaitingIrps.front() );
                    queMsgs.push_back(
                        oMe.GetMsgQue< CfgPtr >().front() );
                    oMe.m_queWaitingIrps.pop_front();
                    oMe.GetMsgQue< CfgPtr >().pop_front();
                    if( queIrps.front() == pIrpLocked )
                        bFound = true;
                }
                oPortLock.Unlock();
                ret = SchedCompleteIrps(
                    queIrps, queMsgs );
                // BUGBUG:Discard all the remaining
                // messages.
                if( SUCCEEDED( ret ) )
                    ret = STATUS_PENDING;
                else if( ERROR( ret ) && !bFound )
                {
                    // try best to keep in shape
                    // on error
                    ret = STATUS_PENDING;
                }
            }
            else
            {
                // queue the irp
                oMe.m_queWaitingIrps.push_back(
                    IrpPtr( pIrp ) );
                ret = STATUS_PENDING;
            }
            pCtx->SetNonDBusReq( true );
        }
        else
        {
            // must be registered before listening
            ret = -ENOENT;
        }

    }while( 0 );

    return ret;
}

gint32 CDBusStreamPdo::HandleListening(
    IRP* pIrp )
{
    // the irp carry a buffer to receive, it is
    // assumed to be a CfgPtr
    gint32 ret = 0;
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    ret = HandleListeningInternal( pIrp );

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

gint32 CDBusStreamPdo::FindIrpForResp(
    IConfigDb* pMessage, IrpPtr& pReqIrp )
{
    gint32 ret = 0;

    if( pMessage == nullptr )
        return -EINVAL;

    CfgPtr pMsg( pMessage );

    do{
        IrpPtr pIrp;
        CCfgOpener oMsg( ( IConfigDb* )pMsg );

        if( true )
        {
            guint32 dwSerial = 0;
            ret = oMsg.GetIntProp(
                propSeqNo2, dwSerial );
            if( ERROR( ret ) )
                break;

            CStdRMutex oPortLock( GetLock() );
            guint32 dwPortState = GetPortState();
            if( !CanAcceptMsg( dwPortState ) )
                return ERROR_STATE;

            std::map< guint32, IrpPtr >::iterator itr =
                m_mapSerial2Resp.find( dwSerial );

            if( itr == m_mapSerial2Resp.end() )
            {
                OnRespMsgNoIrp( pMessage );
                break;
            }

            pIrp = itr->second;
            m_mapSerial2Resp.erase( itr );
        }

        if( !pIrp.IsEmpty() )
        {

            CStdRMutex oIrpLock( pIrp->GetLock() ) ;
            ret = pIrp->CanContinue( IRP_STATE_READY );
            if( ERROR( ret ) )
                break;

            IrpCtxPtr& pIrpCtx = pIrp->GetCurCtx();

            guint32 dwFlags = 0;

            IConfigDb* pOptions;
            ret = oMsg.GetPointer(
                propCallOptions, pOptions );
            if( ERROR( ret ) )
            {
                ret = -EBADMSG;
                pIrpCtx->SetStatus( ret );
                break;
            }
            CCfgOpener oOptions( pOptions );
            ret = oOptions.GetIntProp(
                propCallFlags, dwFlags );
            if( ERROR( ret ) )
            {
                ret = -EBADMSG;
                pIrpCtx->SetStatus( ret );
                break;
            }

            dwFlags &= CF_MESSAGE_TYPE_MASK;

            if( dwFlags ==
                DBUS_MESSAGE_TYPE_METHOD_RETURN )
            {
                BufPtr pBuf( true );
                *pBuf = pMsg;
                pIrpCtx->SetRespData( pBuf  );
                pIrpCtx->SetStatus( 0 );
            }
            else 
            {
                ret = -EBADMSG;
                pIrpCtx->SetStatus( ret );
            }

            pReqIrp = pIrp;
        }
        else
        {
            ret = -EFAULT;
        }

    }while( 0 );

    return ret;
}

gint32 CDBusStreamPdo::DispatchReqMsg(
    IConfigDb* pMsg )
{
    return DispatchReqMsgT( pMsg );
}

gint32 CDBusStreamPdo::DispatchSignalMsg(
    IConfigDb* pMsg )
{
    return DispatchSignalMsgT( pMsg );
}

gint32 CDBusStreamPdo::DispatchRespMsg(
    IConfigDb* pMsg )
{
    IrpPtr pIrp;
    if( pMsg == nullptr )
        return -EINVAL;

    gint32 ret = 0;

    ret = FindIrpForResp( pMsg, pIrp );
    if( ERROR( ret ) )
        return ret;

    ret = GetIoMgr()->CompleteIrp( pIrp );
    return ret;
}

DBusHandlerResult CDBusStreamPdo::DispatchMsg(
    gint32 iType, IConfigDb* pMsg )
{
    DBusHandlerResult ret =
        DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    switch( iType )
    {
    case DBUS_MESSAGE_TYPE_SIGNAL:
        {
            if( DispatchSignalMsg( pMsg ) != -ENOENT )
                ret = DBUS_HANDLER_RESULT_HANDLED;
            break;
        }
    case DBUS_MESSAGE_TYPE_ERROR:
    case DBUS_MESSAGE_TYPE_METHOD_RETURN:
        {
            if( DispatchRespMsg( pMsg ) != -ENOENT )
                ret = DBUS_HANDLER_RESULT_HANDLED;
            break;
        }
    case DBUS_MESSAGE_TYPE_METHOD_CALL:
        {
            if( DispatchReqMsg( pMsg ) != -ENOENT )
                ret = DBUS_HANDLER_RESULT_HANDLED;
            break;
        }
    default:
        break;
    }
    return ret;
}
gint32 CDBusStreamPdo::DispatchData(
    CBuffer* pData )
{
    gint32 ret = 0;

    if( pData == nullptr )
        return -EINVAL;

    if( m_pTestIrp.IsEmpty() )
    {
        m_pTestIrp.NewObj();
        m_pTestIrp->AllocNextStack( nullptr );
        IrpCtxPtr& pIrpCtx = m_pTestIrp->GetTopStack(); 
        pIrpCtx->SetMajorCmd( IRP_MJ_FUNC );
        pIrpCtx->SetMinorCmd( IRP_MN_IOCTL );
        pIrpCtx->SetCtrlCode( CTRLCODE_LISTENING );
    }

    guint32 dwFlags = 0;
    CfgPtr pMsg = ( ObjPtr& )*pData;
    CCfgOpener oMsg( ( IConfigDb* )pMsg );

    IConfigDb* pOptions;
    ret = oMsg.GetPointer(
        propCallOptions, pOptions );
    if( ERROR( ret ) )
        return -EBADMSG;

    CCfgOpener oOptions( pOptions );
    ret = oOptions.GetIntProp(
        propCallFlags, dwFlags );
    if( ERROR( ret ) )
        return -EBADMSG;

    gint32 iType = dwFlags & CF_MESSAGE_TYPE_MASK;

    DBusHandlerResult ret2; 
    guint32 dwPortState = 0;
    if( true )
    {
        CStdRMutex oPortLock( GetLock() );
        dwPortState = GetPortState();

        if( !( dwPortState == PORT_STATE_READY
             || dwPortState == PORT_STATE_BUSY_SHARED
             || dwPortState == PORT_STATE_BUSY
             || dwPortState == PORT_STATE_BUSY_RESUME ) )
        {
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }
    }

    guint32 dwOldState = 0;
    do{
        ret = CanContinue( m_pTestIrp,
            PORT_STATE_BUSY_SHARED, &dwOldState );

        if( ret == -EAGAIN )
        {
            usleep(100);
            continue;
        }
        break;

    }while( 1 );


    if( ERROR( ret ) )
    {
#ifdef DEBUG
        DebugPrint( ret, 
            "Error, dmsg cannot be processed by CDBusStreamPdo, "
            "curstate = %d, requested state =%d",
            dwOldState,
            PORT_STATE_BUSY_SHARED );
#endif

        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    do{

        ret2 = DispatchMsg( iType, pMsg );
        if( ret2 != DBUS_HANDLER_RESULT_NOT_YET_HANDLED )
            break;

    }while( 0 );

    PopState( dwOldState );

    if( iType == DBUS_MESSAGE_TYPE_SIGNAL
        && ret2 == DBUS_HANDLER_RESULT_HALT )
    {
        // a channel to stop processing the signal
        // further
        ret = ERROR_PREMATURE;
    }
    else if( ret2 == DBUS_HANDLER_RESULT_NOT_YET_HANDLED )
    {
        ret = -ENOENT;
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
        if( ERROR( ret ) )
            break;
        while( dwStepNo == 0 )
        {
            HANDLE hstm = GetStream();
            auto *pBusPort = static_cast
            < CDBusStreamBusPort* >( m_pBusPort );
            pBusPort->RemoveBinding( hstm );

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

            SetPreStopStep( pIrp, 1 );
            if( ret == STATUS_SUCCESS )
            {
                ret = STATUS_MORE_PROCESS_NEEDED;
                break;
            }

            if( !pStopCb.IsEmpty() )
                ( *pStopCb )( eventCancelTask );

            break;
        }

        if( ret == STATUS_MORE_PROCESS_NEEDED )
            break;

        GetPreStopStep( pIrp, dwStepNo );
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

        CRpcStreamChannelCli* pStmCli =
            GetStreamIf();

        TaskletPtr pRespCb;
        ret = NEW_PROXY_RESP_HANDLER2(
            pRespCb, pStmCli, 
            &CRpcStreamChannelCli::OnStartStmComplete,
            ( IEventSink* )nullptr,
            ( IConfigDb* )oReqCtx.GetCfg() );
        if( ERROR( ret ) )
            break;

        IConfigDb* pDesc = nullptr;
        CCfgOpener oDesc;
        HANDLE hstm = INVALID_HANDLE;
        if( pStmCli->IsLocalStream() )
        {
            CCfgOpener oTransCtx;
            stdstr strProps;
            stdstr strHash;
            Variant oVar;
            ret = pStmCli->GetProperty(
                propDestDBusName, oVar );
            if( SUCCEEDED( ret ) )
                strProps.append( oVar.m_strVal );

            ret = pStmCli->GetProperty(
                propObjPath, oVar );
            if( SUCCEEDED( ret ) )
                strProps.append( oVar.m_strVal );

            ret = pStmCli->GetProperty(
                propSrcUniqName, oVar );
            if( SUCCEEDED( ret ) )
                strProps.append( oVar.m_strVal );

            strProps.append(
                std::to_string( getpid() ) );

            ret = GenShaHash( strProps.c_str(),
                strProps.size(), strHash );
            if( ERROR( ret ) )
                break;
            strProps = "DBus";
            strProps += strHash;

            oTransCtx.SetStrProp(
                propSessHash, strProps );
            oTransCtx.SetStrProp(
                propRouterPath, "/" );
            oDesc.SetPointer( propTransCtx,
                ( IConfigDb* )oTransCtx.GetCfg() );
            pDesc = oDesc.GetCfg();
        }

        ret = pstm->StartStream(
            hstm, pDesc, pRespCb );

        if( ret == STATUS_PENDING )
        {
            // startstream has a new timer
            // and this irp's timer can retire.
            pIrp->RemoveTimer();
            break;
        }

        if( SUCCEEDED( ret ) )
            SetStream( hstm );

        ( *pRespCb )( eventCancelTask );
        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        pCtx->SetStatus( ret );

    }while( 0 );

    return ret;
}

gint32 CDBusStreamPdo::CheckExistance(
    ObjPtr& pUxIf )
{
    gint32 ret = 0;
    do{

        CCfgOpenerObj oIfCfg(
            ( CObjBase* )pUxIf );

        bool bOnline;
        ret = oIfCfg.GetBoolProp(
            propOnline, bOnline );
        if( ERROR( ret ) )
        {
            bOnline = true;
            ret = 0;
        }

        if( !bOnline )
            ret = -ENOTCONN;

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

        if( !IsServer() )
            break;

        FireRmtSvrEvent2( this,
            eventRmtSvrOnline, hStream );

    }while( 0 );

    pIrp->GetCurCtx()->SetStatus( ret );
    return ret;
}

gint32 CDBusStreamPdo::HandleSkelReady(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;


    gint32 ret = 0;
    HANDLE hStream = GetStream();

    do{
        if( !IsServer() )
            break;

        m_bSkelReady = true;

        InterfPtr pUxIf;
        CStreamServerSync* ps = GetStreamIf();
        ret = ps->GetUxStream( hStream, pUxIf );
        if( ERROR( ret ) )
            break;

        ObjPtr pIf = pUxIf;
        ret = CheckExistance( pIf );

    }while( 0 );

    if( ERROR( ret ) )
    {
        // still let the port start to success
        OnEvent( eventDisconn,
            hStream, 0, nullptr );
        ret = 0;
    }

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
            if( IsServer() && !m_bSkelReady )
                break;
            if( m_dwDisconned++ > 0 )
            {
                DebugPrint( m_dwDisconned,
                    "Repeated closing of "
                    "StreamPdo 0x%llx",
                    ( HANDLE )this );
                break;
            }
            ret = FireRmtSvrEvent2( this,
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
        SetClassId( clsid(
            CDBusStreamBusPort ) );

        CCfgOpener oCfg( pCfg );
        ret = oCfg.GetBoolProp(
            propIsServer, m_bServer );
        if( ERROR( ret ) )
            break;

    }while( 0 );
    if( ERROR( ret ) )
    {
        throw std::runtime_error( 
            DebugMsg( ret,
                "Error occurs in ctor "
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
        auto pMgr = GetIoMgr();
        stdstr strDesc;
        CCfgOpenerObj oPortCfg( this );
        IConfigDb* pContext = nullptr;
        ret = oPortCfg.GetPointer(
            propSkelCtx, pContext );
        if( ERROR( ret ) )
            break;

        CCfgOpener oCtx( pContext );
        ret = oCtx.GetStrProp(
            propObjDescPath, strDesc );
        if( ERROR( ret ) )
            break;

        stdstr strObjName = oCtx[ 0 ];
        guint32 iClsid = oCtx[ propClsid ];

        oCfg[ propIoMgr ] = ObjPtr( pMgr );
        ret = oCfg.CopyProp(
            propObjInstName, pContext ); 
        if( ERROR( ret ) )
            break;

        oCfg.CopyProp(
            propSvrInstName, pContext );

        ret = CRpcServices::LoadObjDesc(
            strDesc, strObjName, IsServer(),
            oCfg.GetCfg() );
        if( ERROR( ret ) )
            break;

        InterfPtr pIf;
        ret = pIf.NewObj( ( EnumClsid )iClsid,
            oCfg.GetCfg() );

        if( ERROR( ret ) )
        {
            DebugPrintEx( logErr, ret,
                "Unable to create object %s[0x%x]",
                strObjName.c_str(), iClsid );
            break;
        }

        // remove the timer as the StartEx has its
        // own timer, we don't need many timers.
        pIrp->RemoveTimer();

        CRpcServices* pSvc = pIf;
        if( IsServer() )
        {
            CCfgOpener oReqCtx;
            oReqCtx.SetPointer( propIrpPtr, pIrp );
            oReqCtx.SetPointer( propPortPtr, this );

            TaskletPtr pRespCb;
            ret = NEW_PROXY_RESP_HANDLER2(
                pRespCb, ObjPtr( pSvc ), 
                &CRpcStreamChannelSvr::OnStartStopComplete,
                ( IEventSink* )nullptr,
                ( IConfigDb* )oReqCtx.GetCfg() );
            if( ERROR( ret ) )
            {
                ( *pRespCb )( eventCancelTask );
                break;
            }
            ret = pSvc->StartEx( pRespCb );
            if( ERROR( ret ) )
                ( *pRespCb )( eventCancelTask );

            break;
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

gint32 CDBusStreamBusPort::OnPortReady(
    IRP* pIrp )
{
    gint32 ret = 0;
    do{
        ret = super::OnPortReady( pIrp );
        if( ERROR( ret ) )
            break;
        // copy the propFetchTimeout value from
        // channel proxy
        ObjPtr pIf = GetStreamIf();
        if( pIf.IsEmpty() )
            break;
        CRpcServices* pSvc = pIf;
        if( pSvc->IsServer() )
            break;

        Variant oVar;
        ret = pSvc->GetProperty(
            propFetchTimeout, oVar );
        if( ERROR( ret ) )
        {
            ret = 0;
            break;
        }
        this->SetProperty(
            propFetchTimeout, oVar );

    }while( 0 );

    pIrp->GetCurCtx()->SetStatus( ret );
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
        CRpcServices* pSvc = pIf;

        // FIXME: we should also check if the upstream
        // CFastRpcProxyBase or CFastRpcServerBase is
        // in stopping state or not.
        if( pSvc->GetState() == stateStopping ||
            pSvc->GetState() == stateStopped )
            break;

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
        ret = pMgr->RescheduleTask( pStopTask );
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

gint32 CDBusStreamBusDrv::Probe2(
        IPort* pLowerPort,
        PortPtr& pNewPort,
        CfgPtr& pContext,
        IEventSink* pCallback )
{
    gint32 ret = 0;
    if( pCallback == nullptr ||
        pContext.IsEmpty() )
        return -EINVAL;

    do{
        CCfgOpener oCtx( ( IConfigDb* )pContext );

        bool bIsServer = oCtx[ propIsServer ];
        stdstr strMasterIf = oCtx[ 1 ];

        if( !bIsServer )
        {
            CStdRMutex oDrvLock( GetLock() );
            auto itr =m_mapNameToBusId.find(
                strMasterIf );
            if( itr != m_mapNameToBusId.end() )
            {
                guint32 dwBusId = itr->second;

                ret = GetPortById(
                    dwBusId, pNewPort );

                if( ERROR( ret ) )
                    break;
                oCtx[ propBusId ] = dwBusId;
                break;
            }
        }
        else
        {
            CStdRMutex oDrvLock( GetLock() );
            auto itr = m_mapNameToBusId.find(
                strMasterIf );
            if( itr != m_mapNameToBusId.end() )
            {
                ret = -EEXIST;
                break;
            }
        }

        CCfgOpener oCfg;
        oCfg[ propIsServer ] = bIsServer;
        oCfg[ propPortClass ] =
            ( char* )PORT_CLASS_DBUS_STREAM_BUS;
        oCfg[ propObjInstName ] = strMasterIf;

        oCfg.SetIntProp( propClsid,
            clsid( CDBusStreamBusPort ) );

        oCfg.SetPointer(
            propEventSink, pCallback );

        ret = CreatePort( pNewPort, oCfg.GetCfg() );
        if( ERROR( ret ) )
            break;

        oCtx.CopyProp( propBusId, pNewPort );

        CCfgOpenerObj oPortCfg(
            ( IPort* )pNewPort );

        oPortCfg.SetPointer( propSkelCtx,
            ( IConfigDb* )pContext );

        ret = NotifyPortAttached( pNewPort );

    }while( 0 );

    return ret;
}

gint32 CDBusStreamBusDrv::BindNameBus(
    const stdstr& strName,
    guint32 dwBusId )
{
    gint32 ret = 0;
    do{
        CStdRMutex oDrvLock( GetLock() );
        auto itr =m_mapNameToBusId.find(
            strName );
        if( itr != m_mapNameToBusId.end() )
        {
            ret = -EEXIST;
            break;
        }
        m_mapNameToBusId[ strName ] = dwBusId;

    }while( 0 );

    return ret;
}

gint32 CDBusStreamBusDrv::RemoveBinding(
    const stdstr& strName )
{
    gint32 ret = 0;
    do{
        CStdRMutex oDrvLock( GetLock() );
        auto itr =m_mapNameToBusId.find(
            strName );
        if( itr == m_mapNameToBusId.end() )
        {
            ret = -ENOENT;
            break;
        }
        m_mapNameToBusId.erase( itr );

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
        if( ERROR( ret ) ||
            dwPortId == ( ( guint32 )-1 ) )
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
        strBus.push_back( '_' );

        Variant oVar;
        ret = this->GetProperty(
            propPortId, oVar );
        if( ERROR( ret ) )
            break;

        guint32 dwBusId = oVar;
        strBus += std::to_string( dwBusId );
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
            // new connection comes
            PortPtr pPort;
            ret = OnNewConnection(
                ( HANDLE )dwParam1, pPort );
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
