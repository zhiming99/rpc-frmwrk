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
            oPortLock.Unlock();
            if( ERROR( ret ) )
                break;

            if( IsServer() )
            {
                CRpcReqStreamServer *pIf =
                    GetStreamIf();
                pIf->AddRecvReq();
            }
            else
            {
                CRpcReqStreamProxy *pIf =
                    GetStreamIf();
                pIf->AddRecvReq();
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
        pIf->AddRecvReq();
    }
    else
    {
        CRpcReqStreamProxy *pIf =
            GetStreamIf();
        pIf->AddRecvReq();
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

gint32 CDBusStreamPdo::PostStart(
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
        stdstr strDesc = "./";
        strDesc = pMgr->GetModName();
        strDesc += "desc.json";

        stdstr strObjName =
            OBJNAME_RPC_STREAMCHAN_SVR;

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
            ret = pSvc->Start();
        else
        {
            CRpcStreamChannelCli* pCli = pIf;
            TaskletPtr pStartTask;
            ret = DEFER_IFCALLEX_NOSCHED2(
                0, pTask, pSvc, 
                &CRpcServices::StartEx,
                ( IEventSink* )nullptr );
            if( ERROR( ret ) )
                break;

            CCfgOpener oReqCtx;
            oReqCtx.SetPointer( propIrpPtr, pIrp );
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

gint32 CDBusStreamPdo::StopStreamChan(
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
            0, pTask, pIf, 
            &CRpcServices::StopEx,
            ( IEventSink* )nullptr );
        if( ERROR( ret ) )
            break;

        CCfgOpener oReqCtx;
        oReqCtx.SetPointer( propIrpPtr, pIrp );
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

gint32 CDBusStreamPdo::PreStop(
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
            ret = StopStreamChan( pIrp );
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

        ret = pNewPort.NewObj(
            clsid( CDBusStreamPdo ), pCfg );

        // the pdo port `Start()' will be deferred
        // till the complete port stack is built.
        //
        // And well, we are done here

    }while( 0 );

    return ret;
}

}
