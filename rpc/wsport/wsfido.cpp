/*
 * =====================================================================================
 *
 *       Filename:  wsfido.cpp
 *
 *    Description:  Implementation of WebSocket filter port and related classes 
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
#include "wsfido.h"
#include "uxport.h"
#include "jsondef.h"
#include "tcportex.h"

namespace rpcf
{

CConnParams GetConnParams2(
    const CObjBase* pObj )
{
    CCfgOpenerObj oCfg( pObj );
    IConfigDb* pConnParams = nullptr;

    oCfg.GetPointer(
        propConnParams, pConnParams );

    return CConnParams( pConnParams );
}

CRpcWebSockFido::CRpcWebSockFido(
    const IConfigDb* pCfg ) :
    super( pCfg ),
    m_bCloseSent( false )
{
    SetClassId( clsid( CRpcWebSockFido ) );
    m_dwFlags &= ~PORTFLG_TYPE_MASK;
    m_dwFlags |= PORTFLG_TYPE_FIDO;

    gint32 ret = 0;
    do{
        ret = m_pCurFrame.NewObj();
        m_vecTasks.resize( 3 );

    }while( 0 );
    if( ERROR( ret ) )
    {
        std::string strMsg = DebugMsg( ret,
            "Error occurs in CRpcWebSockFido ctor" );  
        throw std::runtime_error( strMsg );
    }
}

CRpcWebSockFido::~CRpcWebSockFido()
{
}

gint32 CRpcWebSockFido::EncodeAndSend(
    PIRP pIrp, CfgPtr pCfg )
{
    gint32 ret = 0;
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    if( pCfg.IsEmpty() )
        return -EINVAL;

    do{
        gint32 iCount = 0;
        guint32& dwCount = ( guint32& )iCount;
        CParamList oParams( pCfg );
        ret = oParams.GetSize( dwCount );
        if( ERROR( ret ) )
            break;

        CParamList oNewParams;
        BufPtr pOutBuf( true );
        oNewParams.Push( pOutBuf );

        guint32 dwSize = 0;
        gint32 iIdx = 0;
        for( ; iIdx < iCount; iIdx++ )
        {
            BufPtr pBuf;
            ret = oParams.GetProperty(
                iIdx, pBuf );
            if( ERROR( ret ) )
                break;
            dwSize += pBuf->size();
            oNewParams.Push( pBuf );
        }

        if( ERROR( ret ) )
            break;

        pOutBuf->Resize( 2 );
        guint8* pFrmHdr =
            ( guint8* )pOutBuf->ptr();
        if( dwSize < 126 )
        {
            pFrmHdr[ 0 ] = BINARY_FRAME;
            pFrmHdr[ 1 ] = ( guint8 )dwSize;
        }
        else if( dwSize < 65536 )
        {
            pOutBuf->Resize( 4 );
            pFrmHdr[ 0 ] = BINARY_FRAME;
            pFrmHdr[ 1 ] = 126;
            pFrmHdr[ 2 ] =
                ( ( dwSize >> 8 ) & 0xFF );
            pFrmHdr[ 3 ] =
                ( dwSize & 0xFF );
        }
        else
        {
            pOutBuf->Resize( 10 );
            pFrmHdr[ 0 ] = BINARY_FRAME;
            pFrmHdr[ 1 ] = 126;
            ( ( guint16* )pFrmHdr )[ 1 ] = 0;
            ( ( guint16* )pFrmHdr )[ 2 ] = 0;
            pFrmHdr[ 6 ] =
                ( ( dwSize >> 24 ) & 0xFF );
            pFrmHdr[ 7 ] =
                ( ( dwSize >> 16 ) & 0xFF );
            pFrmHdr[ 8 ] =
                ( ( dwSize >> 8 ) & 0xFF );
            pFrmHdr[ 9 ] =
                ( dwSize  & 0xFF );
        }

        // send the encrypted copy down 
        PortPtr pLowerPort = GetLowerPort();
        pIrp->AllocNextStack(
            pLowerPort, IOSTACK_ALLOC_COPY  );

        IrpCtxPtr pCtx = pIrp->GetTopStack();
        BufPtr pNewBuf( true );
        *pNewBuf = ObjPtr( oNewParams.GetCfg() );
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

gint32 CRpcWebSockFido::ReceiveData(
    const BufPtr& pBuf )
{
    gint32 ret = 0;
    CStdRMutex oPortLock( GetLock() );
    if( m_pCurFrame->empty() )
    {
        m_pCurFrame = pBuf;
    }
    else
    {
        ret = m_pCurFrame->Append(
            ( guint8*)pBuf->ptr(),
            pBuf->size() );
    }
    return ret;
}

gint32 CRpcWebSockFido::SubmitWriteIrp(
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
        ret = EncodeAndSend( pIrp, pCfg );

    }while( 0 );

    return ret;
}

gint32 CRpcWebSockFido::SubmitIoctlCmd(
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

            if( psse->m_iEvent == sseError )
            {
                pCtx->SetRespData( pRespBuf );
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

gint32 CRpcWebSockFido::OnSubmitIrp(
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

gint32 CRpcWebSockFido::StartWsHandshake(
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

        if( IsClient() )
        {
            oParams.SetBoolProp(
                propHsSent, false );
        }
        else
        {
            oParams.SetBoolProp(
                propHsReceived, false );
        }

        ret = m_vecTasks[ enumHsTask ].NewObj(
            clsid( CWsHandshakeTask ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        CIoManager* pMgr = GetIoMgr();

        ret = pMgr->RescheduleTask(
            m_vecTasks[ enumHsTask ] );

        if( SUCCEEDED( ret ) )
            ret = STATUS_PENDING;

    }while( 0 );

    return ret;
}

gint32 CRpcWebSockFido::PostStart(
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

        // hold the irp and start the handshake
        // task
        ret = StartWsHandshake( pIrp );
        break;

    }while( 0 );

    return ret;
}

gint32 CRpcWebSockFido::ClearTask( gint32 iIdx )
{

    CStdRMutex oPortLock( GetLock() );
    if( iIdx < 0 ||
        ( ( guint32 )iIdx ) >= m_vecTasks.size() )
        return -EINVAL;

    if( m_vecTasks[ iIdx ].IsEmpty() )
        return 0;

    CIfParallelTask* pTask = m_vecTasks[ iIdx ];
    m_vecTasks[ iIdx ].Clear();

    oPortLock.Unlock();

    EnumTaskState iState =
        pTask->GetTaskState();

    if( pTask->IsStopped( iState ) )
        return 0;

    ( *pTask )( eventCancelTask );
    return -ECANCELED;
}

gint32 CRpcWebSockFido::PreStop(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = ClearTask( enumHsTask );
    if( ERROR( ret ) )
        return super::PreStop( pIrp );

    bool bExpected = false;
    if( m_bCloseSent.compare_exchange_strong(
        bExpected, true ) )
    {
        ret = ScheduleCloseTask( pIrp,
            ERROR_PORT_STOPPED, true );
        if( SUCCEEDED( ret ) )
            return STATUS_PENDING;
    }
    return super::PreStop( pIrp );
}

gint32 CRpcWebSockFido::Stop( IRP* pIrp )
{
    ClearTask( enumCloseTask );
    return super::Stop( pIrp );
}

gint32 CRpcWebSockFido::ScheduleCloseTask(
    PIRP pIrp, gint32 dwReason, bool bStart )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CParamList oParams;
        oParams.Push( dwReason );
        oParams.Push( bStart );

        oParams.SetPointer( propIrpPtr, pIrp );
        oParams.SetPointer( propPortPtr, this );
        // we have 20 seconds of patience on it
        oParams.SetIntProp( propTimeoutSec, 20 );
        oParams.SetPointer( propIoMgr, GetIoMgr() );

        ret = m_vecTasks[ enumCloseTask ].NewObj(
            clsid( CWsCloseTask ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        GetIoMgr()->RescheduleTask(
            m_vecTasks[ enumCloseTask ] );

    }while( 0 );

    return ret;
}

gint32 CRpcWebSockFido::SchedulePongTask(
    BufPtr& pDecrypted )
{
    gint32 ret = 0;
    do{
        CParamList oParams;
        oParams.Push( pDecrypted );

        oParams.SetPointer( propPortPtr, this );
        oParams.SetIntProp( propTimeoutSec, 30 );

        ret = GetIoMgr()->ScheduleTask(
            clsid( CWsPingPongTask ),
            oParams.GetCfg() );

    }while( 0 );

    return ret;
}

gint32 CRpcWebSockFido::CompleteListeningIrp(
    IRP* pIrp )
{
    gint32 ret = 0;
    IrpCtxPtr pCtx;
    pCtx = pIrp->GetCurCtx();
    IrpCtxPtr pTopCtx = pIrp->GetTopStack();

    BufPtr pRespBuf = pTopCtx->m_pRespData;
    STREAM_SOCK_EVENT* psse =
    ( STREAM_SOCK_EVENT* )pRespBuf->ptr();

    BufPtr pBuf = psse->m_pInBuf;
    psse->m_pInBuf.Clear();
    if( pBuf.IsEmpty() || pBuf->empty() )
    {
        pCtx->SetStatus( -EBADMSG );
        if( !pIrp->IsIrpHolder() )
            pIrp->PopCtxStack();
        return -EBADMSG;
    }

    ret = ReceiveData( pBuf );
    if( ERROR( ret ) )
    {
        pCtx->SetStatus( -EBADMSG );
        if( !pIrp->IsIrpHolder() )
            pIrp->PopCtxStack();
        return ret;
    }

    BufPtr pDecrypted( true );
    do{
        BufPtr& pCurFrame = GetCurFrame();
        WebSocketFrameType ret1;

        {
            CStdRMutex oPortLock( GetLock() );
            ret1 = m_oWebSock.getFrame(
                pCurFrame, pDecrypted );
        }

        if( ret1 == INCOMPLETE_TEXT_FRAME ||
            ret1 == INCOMPLETE_BINARY_FRAME ||
            ret1 == INCOMPLETE_FRAME )
        {
            if( pDecrypted.IsEmpty() ||
                pDecrypted->empty() )
            {
                PortPtr pLowerPort = GetLowerPort();
                pTopCtx->m_pRespData.Clear();
                pTopCtx->SetStatus( STATUS_PENDING );

                ret = pLowerPort->SubmitIrp( pIrp );
                if( ret == STATUS_PENDING )
                    break;

                if( ERROR( ret ) )
                    break;

                pRespBuf = pTopCtx->m_pRespData;
                psse = ( STREAM_SOCK_EVENT* )
                    pRespBuf->ptr();

                EnumStmSockEvt iEvent =
                    psse->m_iEvent;

                if( iEvent == sseError )
                {
                    pCtx->SetRespData( pRespBuf );
                    pCtx->SetStatus(
                        pTopCtx->GetStatus() );
                    break;
                }
                else if( iEvent == sseRetWithFd )
                {
                    ret = -ENOTSUP;
                    break;
                }

                pBuf = psse->m_pInBuf;
                psse->m_pInBuf.Clear();
                if( pBuf.IsEmpty() || pBuf->empty() )
                {
                    ret = -EBADMSG;
                    break;
                }
                ret = ReceiveData( pBuf );
                if( ERROR( ret ) )
                    break;

                continue;
            }

            // there are frames received
            psse->m_pInBuf = pDecrypted;
            pCtx->SetRespData( pRespBuf );
            ret = 0;
            break;
        }
        else if( ret1 == TEXT_FRAME ||
            ret1 == BINARY_FRAME )
        {
            ret = 0;
            // the remaider if any, will be
            // returned on next read request
            CStdRMutex oPortLock( GetLock() );
            if( pCurFrame->empty() )
            {
                pCurFrame->Resize( 0 );
                psse->m_pInBuf = pDecrypted;
                pCtx->SetRespData( pRespBuf );
                ret = 0;
                break;
            }
            continue;
        }
        else if( ret1 == CLOSE_FRAME )
        {
            if( !pIrp->IsIrpHolder() )
                pIrp->PopCtxStack();

            bool bExpected = false;
            if( m_bCloseSent.compare_exchange_strong(
                bExpected, true ) )
            {
                ret = ScheduleCloseTask(
                    pIrp, 0, false );

                if( SUCCEEDED( ret ) )
                {
                    ret = STATUS_PENDING; 
                    break;
                }
            }

            psse->m_iEvent = sseError;
            psse->m_iData = -ENOTCONN;
            psse->m_iEvtSrc = GetClsid();
            pCtx->SetRespData( pRespBuf );
            pCtx->SetStatus( STATUS_SUCCESS );
            break;
        }
        else if( ret1 == PING_FRAME )
        {
            SchedulePongTask( pDecrypted );
            continue;
        }
        else if( ret1 == PONG_FRAME )
        {
            // ignore pong frames
            continue;
        }

        ret = ERROR_FAIL;
        break;

    }while( 1 );


    if( ret == STATUS_PENDING )
        return ret;

    if( ERROR( ret ) )
        pCtx->SetStatus( ret );

    if( !pIrp->IsIrpHolder() )
        pIrp->PopCtxStack();

    return ret;
}

gint32 CRpcWebSockFido::CompleteIoctlIrp(
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

gint32 CRpcWebSockFido::CompleteFuncIrp( IRP* pIrp )
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

gint32 CRpcWebSockFido::CompleteWriteIrp(
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

gint32 CRpcWebSockFido::SendWriteReq(
    IEventSink* pCallback, BufPtr& pBuf )
{
    if( pCallback == nullptr ||
        pBuf.IsEmpty() || pBuf->empty() )
        return -EINVAL;

    CCfgOpenerObj oTaskCfg( pCallback );
    guint32 dwTimeoutSec = 0;

    gint32 ret = oTaskCfg.GetIntProp(
        propTimeoutSec, dwTimeoutSec );

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

    pLowerPort->AllocIrpCtxExt(
        pCtx, ( PIRP )pIrp );

    CIoManager* pMgr = GetIoMgr();
    if( dwTimeoutSec > 0 && dwTimeoutSec < 180 )
        pIrp->SetTimer( dwTimeoutSec, pMgr );

    CParamList oParams;
    oParams.Push( pBuf );
    BufPtr pReqBuf( true );
    *pReqBuf = ObjPtr( oParams.GetCfg() );

    oTaskCfg.SetPointer(
        propIrpPtr1, ( PIRP )pIrp ); 

    ret = GetIoMgr()->SubmitIrpInternal(
        pLowerPort, pIrp, false );

    if( ret != STATUS_PENDING )
    {
        pIrp->RemoveTimer();
        oTaskCfg.RemoveProperty( propIrpPtr1 );
    }

    return ret;
}

gint32 CRpcWebSockFido::SendListenReq(
    IEventSink* pCallback, BufPtr& pBuf )
{
    if( pCallback == nullptr )
        return -EINVAL;

    CCfgOpenerObj oTaskCfg( pCallback );
    guint32 dwTimeoutSec = 0;
    gint32 ret = oTaskCfg.GetIntProp(
        propTimeoutSec, dwTimeoutSec );

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

    pLowerPort->AllocIrpCtxExt(
        pCtx, ( PIRP )pIrp );

    CIoManager* pMgr = GetIoMgr();
    if( dwTimeoutSec > 0 && dwTimeoutSec < 180 )
        pIrp->SetTimer( dwTimeoutSec, pMgr );

    oTaskCfg.SetPointer(
        propIrpPtr1, ( PIRP )pIrp );

    ret = pMgr->SubmitIrpInternal(
        pLowerPort, pIrp, false );

    if( ret != STATUS_PENDING )
    {
        pIrp->RemoveTimer();
        oTaskCfg.RemoveProperty( propIrpPtr1 );
    }

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

gint32 CRpcWebSockFido::AdvanceHandshake(
     IEventSink* pCallback )
{
    gint32 ret = 0;

    if( IsClient() )
        ret = AdvanceHandshakeClient( pCallback );
    else
        ret = AdvanceHandshakeServer( pCallback );

    return ret;
}

gint32 CRpcWebSockFido::AdvanceHandshakeServer(
     IEventSink* pCallback )
{
    gint32 ret = 0;
    if( pCallback == nullptr )
        return -EINVAL;
    do{
        CCfgOpenerObj oTaskCfg( pCallback );
        bool bHsReceived = false;
        BufPtr& pHandshake = GetCurFrame();
        ret = oTaskCfg.GetBoolProp(
            propHsReceived, bHsReceived );
        if( ERROR( ret ) )
            break;

        int header_size = 0;
        if( !bHsReceived &&
            ( pHandshake.IsEmpty() ||
            pHandshake->empty() ) )
        {
            BufPtr pBuf;
            ret = SendListenReq(
                pCallback, pBuf );
            if( ERROR( ret ) )
                break;

            if( ret == STATUS_PENDING )
                break;

            ReceiveData( pBuf );
        }

        if( !bHsReceived )
        {
            WebSocketFrameType ret1 =
                m_oWebSock.parseHandshake(
                    ( guint8* )pHandshake->ptr(),
                    pHandshake->size(),
                    header_size );

            if( ret1 == INCOMPLETE_FRAME )
            {
                BufPtr pBuf;
                ret = SendListenReq(
                    pCallback, pBuf );
                if( ERROR( ret ) )
                    break;

                if( ret == STATUS_PENDING )
                    break;

                ReceiveData( pBuf );
                continue;
            }
            else if( ret1 != OPENING_FRAME )
            {
                ret = -EBADMSG;
                break;
            }

            ret = pHandshake->SetOffset(
                header_size );

            if( ERROR( ret ) )
                break;

            // the remaining data will be consumed
            // after the port becomes ready
            pHandshake->Resize(
                pHandshake->size() );

            oTaskCfg.SetBoolProp(
                propHsReceived, true );

            std::string strResp( 
                m_oWebSock.answerHandshake() );

            if( strResp.empty() )
            {
                ret = ERROR_FAIL;
                break;
            }

            BufPtr pResp( true );
            pResp->Resize( strResp.size() );

            memcpy( pResp->ptr(),
                strResp.c_str(), pResp->size() );

            ret = SendWriteReq(
                pCallback, pResp );
        }

        break;

    }while( 1 );

    return ret;
}

gint32 CRpcWebSockFido::AdvanceHandshakeClient(
     IEventSink* pCallback )
{
    gint32 ret = 0;
    if( pCallback == nullptr )
        return -EINVAL;

    do{
        int header_size = 0;
        BufPtr pBuf;
        BufPtr& pHandshake = GetCurFrame();
        if( pHandshake.IsEmpty() ||
            pHandshake->empty() )
        {
            CCfgOpenerObj oTaskCfg( pCallback );
            bool bSent = false;
            ret = oTaskCfg.GetBoolProp(
                propHsSent, bSent );
            if( ERROR( ret ) )
                break;

            if( !bSent )
            {
                // check if there is something to write
                BufPtr pBuf( true );

                CConnParams oConn =
                    GetConnParams2( this );

                std::string strUrl = oConn.GetUrl();
                if( strUrl.empty() )
                    strUrl = "https://www.example.com/chat";

                std::string strRet;

                ret = m_oWebSock.makeHandshake(
                    GetSecKey(), strUrl.c_str(), strRet );

                if( ERROR( ret ) )
                    break;

                pBuf->Resize( strRet.size() );

                memcpy( pBuf->ptr(),
                    strRet.c_str(), pBuf->size() );

                if( !pBuf->empty() )
                {
                    ret = SendWriteReq(
                        pCallback, pBuf );
                    if( ret == STATUS_PENDING )
                        break;

                    if( ERROR( ret ) )
                        break;
                }
            }

            ret = SendListenReq(
                pCallback, pBuf );

            if( ERROR( ret ) )
                break;

            if( ret == STATUS_PENDING )
                break;

            ret = ReceiveData( pBuf );
            if( ERROR( ret ) )
                break;

            pHandshake = GetCurFrame();

        }

        WebSocketFrameType ret1 =
            m_oWebSock.checkHandshakeResp(
                ( guint8* )pHandshake->ptr(),
                pHandshake->size(),
                header_size );

        if( ret1 == OPENING_FRAME )
        {
            ret = pHandshake->SetOffset(
                header_size );
            if( ERROR( ret ) )    
            {
                ret = -EBADMSG;
                break;
            }
            pHandshake->Resize(
                pHandshake->size() );
            // handshake is done
            break;
        }
        else if( ret1 != INCOMPLETE_FRAME )
        {
            ret = -EBADMSG;
            break;
        }

        // continue to receive the remainder of
        // the frame
        ret = SendListenReq( pCallback, pBuf );
        if( ERROR( ret ) )
            break;

        if( ret == STATUS_PENDING )
            break;

        ret = ReceiveData( pBuf );
        if( ERROR( ret ) )
            break;

    }while( 1 );

    return ret;
}

gint32 CRpcWebSockFido::DoHandshake(
     IEventSink* pCallback, bool bFirst )
{
    if( bFirst )
    {
        DebugPrint( 0, "Start WebSocket handshake..." );
    }
    else
    {
        DebugPrint( 0, "Continue WebSocket handshake..." );
    }

    return AdvanceHandshake( pCallback );
}

gint32 CRpcWebSockFido::AllocIrpCtxExt(
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

gint32 CRpcWebSockFidoDrv::Probe(
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
            PORT_CLASS_WEBSOCK_FIDO;

        oNewCfg.SetIntProp(
            propPortId, NewPortId() );

        ret = oNewCfg.CopyProp(
            propConnParams, pLowerPort );
        if( ERROR( ret ) )
            break;
        
        IConfigDb* pConnParams = nullptr;
        ret = oNewCfg.GetPointer(
            propConnParams, pConnParams );
        if( ERROR( ret ) )
            break;

        CConnParams oConn( pConnParams );
        bool bEnableWs = oConn.IsWebSocket();
        if( !bEnableWs )
        {
            // websocket is not enabled
            pNewPort = pLowerPort;
            break;
        }

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

CRpcWebSockFidoDrv::CRpcWebSockFidoDrv(
    const IConfigDb* pCfg ) :
    super( pCfg )
{
    SetClassId( clsid( CRpcWebSockFidoDrv ) );
}

CRpcWebSockFidoDrv::~CRpcWebSockFidoDrv()
{
}

gint32 CWsTaskBase::OnTaskComplete(
    gint32 iRet )
{
    gint32 ret = 0;

    CParamList oParams(
        ( IConfigDb* )GetConfig() );

    CRpcWebSockFido* pPort = nullptr;
    do{
        IRP* pMasterIrp = nullptr;        

        // the irp pending on poststart
        ret = oParams.GetPointer(
            propIrpPtr, pMasterIrp );

        if( ERROR( ret ) )
            break;

        ret = oParams.GetPointer(
            propPortPtr, pPort );
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

    if( pPort != nullptr )
    {
        gint32 iIdx = -1;
        EnumClsid iClsid = GetClsid();

        if( iClsid == clsid( CWsHandshakeTask ) )
            iIdx = CRpcWebSockFido::enumHsTask;

        else if( iClsid == clsid( CWsCloseTask ) )
            iIdx = CRpcWebSockFido::enumCloseTask;

        if( iIdx >= 0 )
            pPort->ClearTask( iIdx );
    }

    oParams.ClearParams();
    oParams.RemoveProperty( propIrpPtr );
    oParams.RemoveProperty( propIrpPtr1 );
    oParams.RemoveProperty( propPortPtr );

    return ret;
}

gint32 CWsTaskBase::OnCancel(
    guint32 iEvent )
{
    gint32 ret = 0;
    do{
        CParamList oParams(
            ( IConfigDb* )GetConfig() );

        CRpcWebSockFido* pPort = nullptr;
        ret = oParams.GetPointer(
            propPortPtr, pPort );
        if( ERROR( ret ) )
            break;

        gint32 iRet = 0;
        if( iEvent == eventTimeout )
            iRet = -ETIMEDOUT;
        else if( iEvent == eventCancelTask )
            iRet = -ECANCELED;

        IRP* pIrp = nullptr;
        ret = oParams.GetPointer(
            propIrpPtr1, pIrp );

        if( SUCCEEDED( ret ) )
        {
            CIoManager* pMgr =
                pPort->GetIoMgr();

            CStdRMutex oIrpLock(
                pIrp->GetLock() );

            ret = pIrp->CanContinue(
                IRP_STATE_READY );

            if( SUCCEEDED( ret ) )
            {
                pIrp->RemoveCallback();
                oIrpLock.Unlock();

                pMgr->CancelIrp(
                    pIrp, true, iRet );
            }

            ret = iRet;
        }

        OnTaskComplete( ret );

    }while( 0 );

    return ret;
}

gint32 CWsHandshakeTask::OnIrpComplete(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CRpcWebSockFido* pPort = nullptr;
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
                pPort->ReceiveData(
                    psse->m_pInBuf );

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
        else if( dwIoDir == IRP_DIR_OUT )
        {
            if( pPort->IsClient() )
            {
                oParams.SetBoolProp(
                    propHsSent, true ); 
            }
        }

        ret = pPort->DoHandshake( this, false );

    }while( 0 );

    if( ret != STATUS_PENDING )
        ret = OnTaskComplete( ret );

    if( SUCCEEDED( ret ) )
        DebugPrintEx( logInfo, ret,
            "WebSocket handshake complete" );
    else if( ERROR( ret ) )
        DebugPrintEx( logErr, ret,
            "WebSocket handshake failed" );
    return ret;
}

gint32 CWsHandshakeTask::RunTask()
{
    gint32 ret = 0;
    do{
        CParamList oParams(
            ( IConfigDb* )GetConfig() );

        CRpcWebSockFido* pPort = nullptr;
        ret = oParams.GetPointer(
            propPortPtr, pPort );

        if( ERROR( ret ) )
            break;

        ret = pPort->DoHandshake( this, true );

    }while( 0 );

    if( ret != STATUS_PENDING )
        ret = OnTaskComplete( ret );

    if( SUCCEEDED( ret ) )
        DebugPrintEx( logInfo, ret,
            "WebSocket handshake complete" );
    else if( ERROR( ret ) )
        DebugPrintEx( logErr, ret,
            "WebSocket handshake failed" );
    return ret;
}

gint32 CWsPingPongTask::OnIrpComplete(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = pIrp->GetStatus();
    OnTaskComplete( ret );
    return ret;
}

gint32 CWsPingPongTask::RunTask()
{
    gint32 ret = 0;
    do{
        CRpcWebSockFido* pPort = nullptr;
        CParamList oParams(
            ( IConfigDb* )GetConfig() );

        ret = oParams.GetPointer(
            propPortPtr, pPort );

        if( ERROR( ret ) )
            break;

        BufPtr pPayload;
        ret = oParams.GetProperty(
            0, pPayload );
        if( ERROR( ret ) )
            break;

        BufPtr pOutBuf( true );
        ret = pPort->MakeFrame(
            PONG_FRAME,
            ( guint8* )pPayload->ptr(),
            pPayload->size(),
            pOutBuf );

        if( ERROR( ret ) )
            break;

        ret = pPort->SendWriteReq(
            this, pOutBuf );

    }while( 0 );

    if( ret != STATUS_PENDING )
        ret = OnTaskComplete( ret );

    return ret;
}

gint32 CWsCloseTask::RunTask()
{
    gint32 ret = 0;
    CParamList oParams(
        ( IConfigDb* )GetConfig() );
    do{
        guint32 dwReason = 0;
        ret = oParams.GetIntProp( 0, dwReason );
        if( ERROR( ret ) )
            break;

        CRpcWebSockFido* pPort = nullptr;
        ret = oParams.GetPointer(
            propPortPtr, pPort );

        if( ERROR( ret ) )
            break;

        BufPtr pPayload( true );
        if( ERROR( dwReason ) )
        {
            *pPayload = htons( 1011 );
            dwReason = htonl( dwReason );
            pPayload->Append( ( guint8* )&dwReason,
                sizeof( guint32 ) );
        }
        else
            *pPayload = htons( 1000 );
        BufPtr pOutBuf( true );

        ret = pPort->MakeFrame(
            CLOSE_FRAME,
            ( guint8* )pPayload->ptr(),
            pPayload->size(),
            pOutBuf );

        if( ERROR( ret ) )
            break;

        ret = pPort->SendWriteReq(
            this, pOutBuf );

    }while( 0 );

    if( ret != STATUS_PENDING )
        OnTaskComplete( ret );

    return ret;
}
gint32 CWsCloseTask::OnIrpComplete(
    IRP* pIrp )
{
    gint32 ret = 0;
    CParamList oParams(
        ( IConfigDb* )GetConfig() );
    do{
        bool bStart = false;
        ret = oParams.GetBoolProp(
            1, bStart );

        if( ERROR( ret ) )
            break;

        if( !bStart )
        {
            // the close response is sent
            break;
        }

        CRpcWebSockFido* pPort = nullptr;
        ret = oParams.GetPointer(
            propPortPtr, pPort );

        if( ERROR( ret ) )
            break;

        IrpCtxPtr& pTopCtx =
            pIrp->GetTopStack();

        guint32 dwIoDir = 
            pTopCtx->GetIoDirection();

        if( dwIoDir == IRP_DIR_IN )
        {
            // we are done with the close
            // sequence.
            ret = pIrp->GetStatus();
            break;
        }
        if( dwIoDir != IRP_DIR_OUT )
        {
            ret = ERROR_FAIL;
            break;
        }

        // FIXME: the protocol requires waiting for
        // the CLOSE_FRAME from the peer
        //
        // don't wait for the response
        //
        // BufPtr pBuf;
        // ret = pPort->SendListenReq(
        //     this, pBuf );

    }while( 0 );

    if( ret != STATUS_PENDING )
        OnTaskComplete( ret );

    return ret;
}

gint32 CWsCloseTask::OnTaskComplete(
    gint32 iRet )
{
    gint32 ret = 0;

    CParamList oParams(
        ( IConfigDb* )GetConfig() );

    CRpcWebSockFido* pPort = nullptr;
    do{
        IRP* pMasterIrp = nullptr;        

        // the irp pending on poststart
        ret = oParams.GetPointer(
            propIrpPtr, pMasterIrp );

        if( ERROR( ret ) )
            break;

        ret = oParams.GetPointer(
            propPortPtr, pPort );
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

        if( pCtx->GetMajorCmd() == IRP_MJ_PNP &&
            pCtx->GetMinorCmd() == IRP_MN_PNP_STOP )
        {
            ret = pPort->CPort::PreStop( pMasterIrp );
            pCtx->SetStatus( ret );
        }
        else if( pCtx->GetMajorCmd() == IRP_MJ_FUNC &&
            pCtx->GetMinorCmd() == IRP_MN_IOCTL &&
            pCtx->GetCtrlCode() == CTRLCODE_LISTENING )
        {
            BufPtr pRespBuf( true );
            ret = pRespBuf->Resize(
                sizeof( STREAM_SOCK_EVENT ) );
            if( ERROR( ret ) )
                break;

            STREAM_SOCK_EVENT* psse =
            ( STREAM_SOCK_EVENT* )pRespBuf->ptr();
            psse->m_iEvent = sseError;
            psse->m_iData = -ENOTCONN;
            psse->m_iEvtSrc = GetClsid();
            pCtx->SetStatus( STATUS_SUCCESS );
            pCtx->SetRespData( pRespBuf );
        }
        oIrpLock.Unlock();

        pMgr->CompleteIrp( pMasterIrp );

    }while( 0 );

    if( pPort != nullptr )
    {
        gint32 iIdx = 
            CRpcWebSockFido::enumCloseTask;
        pPort->ClearTask( iIdx );
    }

    oParams.ClearParams();
    oParams.RemoveProperty( propIrpPtr );
    oParams.RemoveProperty( propIrpPtr1 );
    oParams.RemoveProperty( propPortPtr );

    return ret;
}

}
