/*
 * =====================================================================================
 *
 *       Filename:  kdcfdo.cpp
 *
 *    Description:  declaration of the port dedicated for KDC traffic as a KDC client.
 *
 *        Version:  1.0
 *        Created:  07/15/2020 11:41:48 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:  
 *
 *      Copyright:  2020 Ming Zhi( woodhead99@gmail.com )
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
#include "jsondef.h"
#include "kdcfdo.h"
#include <algorithm>
#include "emaphelp.h"

namespace rpcfrmwrk
{

static gint32 FireConnErrEvent(
    IPort* pPort, EnumEventId iEvent );

gint32 CKdcRelayFdoDrv::Probe(
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

        // BUGBUG: we should have a better way to
        // determine if the underlying port is
        // client or server before the port is
        // started.
        CCfgOpenerObj oPdoPort(
            ( CObjBase* )pPdoPort );

        std::string strPdoClass;
        CCfgOpenerObj oCfg( pLowerPort );
        ret = oCfg.GetStrProp(
            propPdoClass, strPdoClass );

        if( ERROR( ret ) )
            break;

        std::string strExpPdo =
            PORT_CLASS_KDCRELAY_PDO;

        if( strPdoClass != strExpPdo )
        {
            // this is not a port we support
            ret = -ENOTSUP;
            break;
        }

        CParamList oNewCfg;
        oNewCfg[ propPortClass ] =
            PORT_CLASS_KDCRELAY_FDO;

        oNewCfg.SetIntProp(
            propPortId, NewPortId() );

        oNewCfg.CopyProp(
            propConnParams, pLowerPort );

        oNewCfg.SetPointer(
            propIoMgr, GetIoMgr() );

        oNewCfg.SetPointer(
            propDrvPtr, this );

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

CKdcRelayFdo::CKdcRelayFdo(
    const IConfigDb* pCfg )
    :super( pCfg )
{
    SetClassId( clsid( CKdcRelayFdo ) );
    m_dwFlags &= ~PORTFLG_TYPE_MASK;
    m_dwFlags |= PORTFLG_TYPE_FDO;
    return;
}

gint32 CKdcRelayFdo::OnSubmitIrp(
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

gint32 CKdcRelayFdo::CompleteWriteIrp(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    IrpCtxPtr pCtx = pIrp->GetCurCtx();

    do{
        IrpCtxPtr pTopCtx = pIrp->GetTopStack();
        ret = pTopCtx->GetStatus();
        if( ERROR( ret ) )
        {
            FireConnErrEvent(
                this, eventConnErr );
            break;
        }

        pIrp->PopCtxStack();
        PortPtr pLowerPort = GetLowerPort();
        ret = pIrp->AllocNextStack( pLowerPort );
        if( ERROR( ret ) )
            break;

        pTopCtx = pIrp->GetTopStack();
        pTopCtx->SetMajorCmd( IRP_MJ_FUNC );
        pTopCtx->SetMinorCmd( IRP_MN_IOCTL );
        pTopCtx->SetCtrlCode(CTRLCODE_LISTENING );

        pLowerPort->AllocIrpCtxExt(
            pTopCtx, pIrp );

        ret = pLowerPort->SubmitIrp( pIrp );
        if( ret == STATUS_PENDING )
            break;

        if( ERROR( ret ) )
            break;

        CIoManager* pMgr = GetIoMgr();
        ret = DEFER_CALL( pMgr,
            ObjPtr( pMgr ),
            &CIoManager::CompleteIrp,
            pIrp );

        if( SUCCEEDED( ret ) )
            ret = STATUS_PENDING;

    }while( 0 );

    if( ret != STATUS_PENDING )
        pCtx->SetStatus( ret );

    return ret;
}

gint32 CKdcRelayFdo::ResubmitIrp()
{
   gint32 ret = 0; 
   do{
       IrpPtr pIrp;
       CStdRMutex oPortLock( GetLock() );
       if( m_queWaitingIrps.empty() )
       {
           m_bResubmit = false;
           break;
       }

       pIrp = m_queWaitingIrps.front();
       oPortLock.Unlock();

       if( pIrp.IsEmpty() || 
           pIrp->GetStackSize() == 0 )
           break;

        CStdRMutex oIrpLock( pIrp->GetLock() );
        ret = pIrp->CanContinue( IRP_STATE_READY );
        if( ERROR( ret ) )
        {
            ret = 0;
            continue;
        }

        if( !pIrp->IsIrpHolder() )
            continue;

        IrpCtxPtr pCtx = pIrp->GetCurCtx();
        if( pCtx->GetMinorCmd() != IRP_MN_IOCTL )
        {
            ret = -EINVAL;
            break;
        }
        ret = this->SubmitIrp( pIrp );
        if( ret == STATUS_PENDING )
            break;

        oIrpLock.Unlock();
        GetIoMgr()->CompleteIrp( pIrp );

   }while( 1 );

   return ret;
}

gint32 CKdcRelayFdo::HandleNextIrp(
    PIRP pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    do{
        CStdRMutex oPortLock( GetLock() );

        // another resubmit is in process
        if( m_bResubmit )
            break;

        if( m_queWaitingIrps.empty() )
            break;

        IrpPtr pIrp1 = m_queWaitingIrps.front();
        if( pIrp != pIrp1 )
            break;

        m_queWaitingIrps.pop_front();
        if( m_queWaitingIrps.empty() )
            break;

        TaskletPtr pTask;
        gint32 iRet = DEFER_CALL_NOSCHED(
            pTask, ObjPtr( this ), 
            &CKdcRelayFdo::ResubmitIrp );

        if( ERROR( iRet ) )
            break;

        m_bResubmit = true;
        oPortLock.Unlock();

        CIoManager* pMgr = GetIoMgr();
        pMgr->RescheduleTask( pTask );

    }while( 0 );

    return 0;
}

gint32 CKdcRelayFdo::RemoveIrpFromMap( IRP* pIrp )
{
    // NOTE: you should make sure the irp is
    // locked from outside
    if( pIrp == nullptr )
        return -EINVAL;

    IrpCtxPtr pCtx = pIrp->GetTopStack();
    if( pCtx->GetMajorCmd() != IRP_MJ_FUNC ||
        pCtx->GetMinorCmd() != IRP_MN_IOCTL ||
        pCtx->GetCtrlCode() != CTRLCODE_SEND_REQ )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CStdRMutex oPortLock( GetLock() );
        std::deque< IrpPtr >* pQueue = nullptr;

        switch( pCtx->GetMinorCmd() )
        {
        case IRP_MN_IOCTL:
            {
                pQueue = &m_queWaitingIrps;
                break;
            }
        default:
            ret = -ENOTSUP; 
            break;
        }

        if( ERROR( ret ) )
            break;

        std::deque< IrpPtr >::iterator itr =
            pQueue->begin();

        ret = -ENOENT;
        while( itr != pQueue->end() )
        {
            if( pIrp->GetObjId() ==
                ( *itr )->GetObjId() )
            {
                pQueue->erase( itr );
                ret = 0;
                break;
            }
            ++itr;
        }

    }while( 0 );

    return ret;
}

gint32 CKdcRelayFdo::CancelFuncIrp(
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

gint32 CKdcRelayFdo::SubmitIoctlCmd(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;

    IrpCtxPtr pCurCtx = pIrp->GetCurCtx();

    switch( pIrp->CtrlCode() )
    {
    case CTRLCODE_SEND_REQ:
        {
            CStdRMutex oPortLock( GetLock() );
            if( !m_queWaitingIrps.empty() &&
                m_queWaitingIrps.size() <
                MAX_PENDING_MSG )
            {
                m_queWaitingIrps.push_back( pIrp );
                ret = STATUS_PENDING;
                break;
            }
            else if( !m_queWaitingIrps.empty() )
            {
                ret = ERROR_QUEUE_FULL;
                break;
            }
            m_queWaitingIrps.push_back( pIrp );
            oPortLock.Unlock();

            PortPtr pLowerPort = GetLowerPort();
            ret = pIrp->AllocNextStack(
                pLowerPort );
            if( ERROR( ret ) )
                break;

            IrpCtxPtr pTopCtx =
                pIrp->GetTopStack();

            pTopCtx->SetMajorCmd( IRP_MJ_FUNC );
            pTopCtx->SetMinorCmd( IRP_MN_WRITE );
            pTopCtx->SetIoDirection( IRP_DIR_OUT );

            pTopCtx->SetReqData(
                pCurCtx->m_pReqData );

            pLowerPort->AllocIrpCtxExt(
                pTopCtx, pIrp );

            ret = pLowerPort->SubmitIrp( pIrp );
            if( ret == STATUS_PENDING )
                break;

            ret = CompleteWriteIrp( pIrp );
            break;
        }
    default:
        {
            ret = -ENOTSUP;
            break;
        }
    }

    if( ret == STATUS_PENDING )
        return ret;

    if( ret != STATUS_PENDING )
    {
        pCurCtx->SetStatus( ret );
        HandleNextIrp( pIrp );
    }

    return ret;
}

gint32 CKdcRelayFdo::CompleteListeningIrp(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    // listening request from the upper port
    gint32 ret = 0;
    IrpCtxPtr pCtx = pIrp->GetCurCtx();
    IrpCtxPtr pTopCtx = pIrp->GetTopStack();
    BufPtr& pRespBuf = pTopCtx->m_pRespData;
    STREAM_SOCK_EVENT* psse =
        ( STREAM_SOCK_EVENT* )pRespBuf->ptr();

    do{

        BufPtr pRespBuf = psse->m_pInBuf;
        if( pRespBuf.IsEmpty() ||
            pRespBuf->empty() )
        {
            ret = -EBADMSG;
            break;
        }
        // explicitly release the buffer in the
        // STREAM_SOCK_EVENT
        psse->m_pInBuf.Clear();
        if( m_pInBuf.IsEmpty() ||
            m_pInBuf->empty() )
        {
            m_pInBuf = pRespBuf;
        }
        else
        {
            ret = m_pInBuf->Append(
                ( guint8* )pRespBuf->ptr(),
                pRespBuf->size() );
            if( ERROR( ret ) )
                break;
        }

        char* pPayload = m_pInBuf->ptr();
        if( unlikely( m_pInBuf->size() <
            sizeof( guint32 ) ) )
        {
            // accumulated data does not make-up a
            // complete packet, and re-submit the
            // listening irp
            pTopCtx->m_pRespData.Clear();
            IPort* pPort = GetLowerPort();
            ret = pPort->SubmitIrp( pIrp );
            if( SUCCEEDED( ret ) )
                continue;

            break;

        }
        guint32 dwSize = ntohl(
            ( ( guint32* )pPayload )[ 0 ] );

        BufPtr pInBuf ;
        if( dwSize == m_pInBuf->size() -
            sizeof( guint32 ) )
        {
            pInBuf = m_pInBuf;
            m_pInBuf.Clear();
        }
        else if( dwSize < m_pInBuf->size() -
            sizeof( guint32 ) )
        {
            ret = pInBuf.NewObj();
            if( ERROR( ret ) )
                break;

            guint32 dwPktSize = dwSize +
                sizeof( guint32 );

            pInBuf->Append(
                m_pInBuf->ptr() + dwPktSize, 
                m_pInBuf->size() - dwPktSize );

            m_pInBuf->Resize( dwPktSize );
            std::swap( pInBuf, m_pInBuf );
        }
        else
        {
            // incoming data is not a complete
            // packet, and re-submit the listening
            // irp
            pTopCtx->m_pRespData.Clear();
            IPort* pPort = GetLowerPort();
            ret = pPort->SubmitIrp( pIrp );
            if( SUCCEEDED( ret ) )
                continue;

            break;
        }

        pCtx->SetRespData( pInBuf );
        break;

    }while( 1 );

    return ret;
}

gint32 FireConnErrEvent(
    IPort* pPort, EnumEventId iEvent )
{
    // this is an event detected by the
    // socket
    gint32 ret = 0;
    do{
        CCfgOpenerObj oCfg( pPort );
        CPort* pcPort = static_cast
            < CPort* >( pPort );

        PortPtr pPdoPtr;
        ret = pcPort->GetPdoPort( pPdoPtr );
        if( ERROR( ret ) )
            break;

        CPort* pPdo = pPdoPtr;
        HANDLE hPort = PortToHandle( pPdo );

        // pass on this event to the pnp
        // manager
        CEventMapHelper< CPort >
            oEvtHelper( pPdo );

        oEvtHelper.BroadcastEvent(
            eventConnPoint, iEvent,
            ( LONGWORD )hPort, nullptr );

        break;

    }while( 0 );

    return ret;
}
gint32 CKdcRelayFdo::CompleteIoctlIrp(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;

    IrpCtxPtr pCtx = pIrp->GetCurCtx();

    if( pIrp->IsIrpHolder() )
    {
        // unknown error
        ret = ERROR_FAIL;
        pCtx->SetStatus( ret );
        return ret;
    }

    IrpCtxPtr pTopCtx = pIrp->GetTopStack();
    guint32 dwMinorCmd = pTopCtx->GetMinorCmd();
    guint32 dwCtrlCode = pTopCtx->GetCtrlCode();

    if( dwMinorCmd == IRP_MN_WRITE )
    {
        return CompleteWriteIrp( pIrp );
    }

    if( dwMinorCmd != IRP_MN_IOCTL )
    {
        ret = -ENOTSUP;
        pCtx->SetStatus( ret );
        return ret;
    }

    switch( dwCtrlCode )
    {
    case CTRLCODE_LISTENING:
        {
            ret = pTopCtx->GetStatus();
            if( ERROR( ret ) )
                break;

            BufPtr& pBuf = pTopCtx->m_pRespData;
            if( pBuf.IsEmpty() || pBuf->empty() )
            {
                ret = -EBADMSG;
                break;
            }

            STREAM_SOCK_EVENT* psse =
            ( STREAM_SOCK_EVENT* )pBuf->ptr();
            if( psse->m_iEvent == sseError )
            {
                ret = psse->m_iData;
                pCtx->SetStatus( ret );
                if( ret == -EAGAIN )
                    break;

                // FireConnErrEvent(
                //     this, eventConnErr );
                break;
            }
            else if( psse->m_iEvent ==
                sseRetWithFd )
            {
                ret = -ENOTSUP;
                pCtx->SetStatus( ret );
                break;
            }

            ret = CompleteListeningIrp( pIrp );
            break;
        }
    default:
        {
            ret = -ENOTSUP;
            break;
        }
    }

    if( ret != STATUS_PENDING )
    {
        pCtx->SetStatus( ret );
        HandleNextIrp( pIrp );
    }

    if( ERROR( ret ) )
        pIrp->PopCtxStack();

    return ret;
}

gint32 CKdcRelayPdo::OnStmSockEvent(
    STREAM_SOCK_EVENT& sse )
{
    gint32 ret = 0;
    do{
        CStdRMutex oPortLock( GetLock() );
        if( m_queListeningIrps.empty() )
        {
            if( sse.m_iEvent == sseError )
            {
                FireConnErrEvent(
                    this, eventConnErr );
                break;
            }
        }

        ret = super::OnStmSockEvent( sse );
        break;

    }while( 1 );

    return ret;
}

}
