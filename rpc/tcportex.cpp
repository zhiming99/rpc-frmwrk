/*
 * =====================================================================================
 *
 *       Filename:  tcportex.cpp
 *
 *    Description:  the enhanced tcp port and socket declaration for the rpc router
 *
 *        Version:  1.0
 *        Created:  11/05/2019 12:22:47 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi(woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2019 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */

#include "defines.h"
#include "stlcont.h"
#include "dbusport.h"
#include "tcpport.h"
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "reqopen.h"
#include <fcntl.h>
#include "ifhelper.h"
#include "tcportex.h"
#include "emaphelp.h"

CRpcStream2::CRpcStream2(
    const IConfigDb* pCfg )
    : super(),
    m_atmSeqNo( ( guint32 )
        GetRandom() ),
    m_dwAgeSec( 0 )
{
    gint32 ret = 0;
    // parameters:
    // 0: an integer as Stream Id
    // 1: protocol id
    // propIoMgr: pointer to the io manager
    SetClassId( clsid( CRpcStream2 ) );
    do{
        if( pCfg == nullptr )
        {
            ret = -EINVAL;
            break;
        }
        CCfgOpener oCfg( pCfg );

        ret = oCfg.GetPointer(
            propIoMgr, m_pMgr );

        if( ERROR( ret ) )
            break;

        if( oCfg.exist( 0 ) )
        {
            ret = oCfg.GetIntProp( 0,
                ( guint32& )m_iStmId );

            if( ERROR( ret ) )
                break;

            if( IsReserveStm( m_iStmId ) )
            {
                m_iPeerStmId = m_iStmId;
            }
            else
            {
                m_iPeerStmId = -1;
            }
        }

        if( oCfg.exist( 1 ) )
        {
            guint32 dwVal = 0;

            ret = oCfg.GetIntProp( 1, dwVal );
            if( ERROR( ret ) )
                break;

            m_wProtoId = dwVal;
        }

        ObjPtr pObj;
        ret = oCfg.GetObjPtr(
            propPortPtr, pObj );

        if( ERROR( ret ) )
            break;

        m_pParentPort = pObj;
        if( m_pParentPort == nullptr )
        {
            ret = -EFAULT;
        }
        m_pParent = pObj;

    }while( 0 );

    if( ERROR( ret ) )
    {
        std::string strMsg = DebugMsg( ret, 
            "Error in CRpcStream ctor" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CRpcStream2::HandleReadIrp(
    IRP* pIrp )
{
    gint32 ret = 0;
    if( pIrp == nullptr )
        return -EINVAL;

    do{
        // NOTE: no locking of IRP
        IrpPtr ptrIrp( pIrp );
        if( m_queBufToRecv.empty() )
        {
            m_queReadIrps.push_back( pIrp );
            ret = STATUS_PENDING;
            break;
        }
        else if( m_queReadIrps.size() > 0 )
        {
            m_queReadIrps.push_back( pIrp );
            ret = STATUS_PENDING;
            break;
        }

        PacketPtr ptrPacket =
            m_queBufToRecv.front();

        m_queBufToRecv.pop_front();

        CIncomingPacket* pPacket = ptrPacket;
        if( pPacket == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        BufPtr pBuf;
        IrpCtxPtr& pCtx = pIrp->GetCurCtx();
        ret = pPacket->GetPayload( pBuf );
        if( ERROR( ret ) )
            break;

        // get the user required size from the irp
        CfgPtr pCfg;
        ret = pCtx->GetReqAsCfg( pCfg );
        if( ERROR( ret ) )
            break;

        CParamList oReq( ( IConfigDb* )pCfg );
        guint32 dwReqSize = 0;

        ret = oReq.GetIntProp(
            propByteCount, dwReqSize );

        if( ERROR( ret ) )
        {
            dwReqSize = pBuf->size();
            ret = 0;
        }

        if( dwReqSize < pBuf->size() )
        {
            ret = -EBADMSG;
            break;
        }

        if( pCtx->m_pRespData.IsEmpty() )
        {
            pCtx->SetRespData( pBuf );
        }
        else
        {
            pCtx->m_pRespData->Append(
                ( guint8* )pBuf->ptr(),
                pBuf->size() );
        }

        guint32 dwCurSize =
            pCtx->m_pRespData->size();

        if( dwCurSize > dwReqSize )
        {
            ret = -EBADMSG;
            break;
        }
        else if( dwCurSize == dwReqSize )
            break;

        continue;

    }while( 1 );

    return ret;
}

gint32 CRpcStream2::GetReadIrpsToComp(
    std::vector< std::pair< IrpPtr, BufPtr > >& vecIrps )
{
    gint32 ret = 0;

    do{
        CStdRMutex oPortLock(
            m_pParentPort->GetLock() );

        if( m_queReadIrps.empty() ||
            m_queBufToRecv.empty() )
            break;

        IrpPtr pIrp = m_queReadIrps.front();
        m_queReadIrps.pop_front();
        oPortLock.Unlock();

        CStdRMutex oIrpLock( pIrp->GetLock() );
        ret = pIrp->CanContinue( IRP_STATE_READY );

        // discard the irp
        if( ERROR( ret ) )
        {
            ret = 0;
            continue;
        }

        oPortLock.Lock();
        if( m_queBufToRecv.empty() )
            break;

        PacketPtr pPacket = m_queBufToRecv.front();
        m_queBufToRecv.pop_front();

        BufPtr pBuf;
        ret = pPacket->GetPayload( pBuf );
        if( ERROR( ret ) )
            break;

        CfgPtr pReq;
        IrpCtxPtr pCtx = pIrp->GetTopStack();
        ret = pCtx->GetReqAsCfg( pReq );
        if( ERROR( ret ) )
            break;

        CParamList oReq(
            ( IConfigDb* )pReq );

        guint32 dwReqSize = 0;
        ret = oReq.GetIntProp(
            propByteCount, dwReqSize );

        if( ERROR( ret ) )
        {
            // no size reqeust, just return the
            // current packet
            dwReqSize = pBuf->size();
            ret = 0;
        }

        guint32 dwRecvSize = pBuf->size();
        if( dwReqSize < dwRecvSize )
        {
            ret = -EBADMSG;
            break;
        }
        else if( dwReqSize == dwRecvSize )
        {
            // move on
        }
        else
        {
            // dwReqSize > dwRecvSize
            if( pCtx->m_pRespData.IsEmpty() )
            {
                pCtx->SetRespData( pBuf );
                m_queReadIrps.push_front( pIrp );
                continue;
            }

            guint32 dwSizeToRecv = dwReqSize -
                pCtx->m_pRespData->size();

            if( dwSizeToRecv >= dwRecvSize )
            {
                ret = pCtx->m_pRespData->Append(
                    ( guint8* )pBuf->ptr(),
                    pBuf->size() );

                if( ERROR( ret ) )
                    break;

                if( dwSizeToRecv == dwRecvSize )
                {
                    pBuf = pCtx->m_pRespData;
                    pCtx->m_pRespData.Clear();
                }
                else
                {
                    // put the irp back
                    m_queReadIrps.push_front( pIrp );
                    continue;
                }
            }
            else
            {
                // dwSizeToRecv < dwRecvSize,
                // split the recv buffer
                ret = pCtx->m_pRespData->Append(
                    ( guint8* )pBuf->ptr(),
                    dwRecvSize );

                if( ERROR( ret ) )
                    break;

                guint32 dwResidual = 
                    pBuf->size() - dwRecvSize;

                memmove( pBuf->ptr() + dwRecvSize,
                    pBuf->ptr(), dwResidual );

                pBuf->Resize( dwResidual );
                pPacket->SetPayload( pBuf );
                m_queBufToRecv.push_front( pPacket );

                pBuf = pCtx->m_pRespData;
                pCtx->m_pRespData.Clear();
            }
        }
        vecIrps.push_back(
            std::pair< IrpPtr, BufPtr >( pIrp, pBuf ) );

    }while( 1 );

    return ret;
}

gint32 CRpcStream2::RemoveIrpFromMap(
    IRP* pIrp )
{
    // NOTE: you should make sure the irp is
    // locked ahead of this call
    if( pIrp == nullptr )
        return -EINVAL;

    IrpCtxPtr pCtx = pIrp->GetTopStack();
    if( pCtx->GetMajorCmd() != IRP_MJ_FUNC )
        return -EINVAL;

    gint32 ret = 0;
    guint32 i = 0;

    switch( pCtx->GetMinorCmd() )
    {
    case IRP_MN_WRITE:
        {
            guint32 dwCount = m_queWriteIrps.size();
            for( i = 0; i < dwCount; ++i )
            {
                IrpPtr ptrIrp = m_queWriteIrps[ i ];
                if( pIrp == ptrIrp )
                {
                    m_queWriteIrps.erase(
                        m_queWriteIrps.begin() + i );
                    break;
                }
            }
            if( i == dwCount )
                ret = -ENOENT;
            break;
        }
    case IRP_MN_READ:
        {
            guint32 dwCount = m_queReadIrps.size();
            for( i = 0; i < dwCount; ++i )
            {
                IrpPtr ptrIrp = m_queReadIrps[ i ];
                if( pIrp == ptrIrp )
                {
                    m_queReadIrps.erase(
                        m_queReadIrps.begin() + i );
                    break;
                }
            }
            if( i == dwCount )
                ret = -ENOENT;
            break;
        }
    default:
        ret = -EINVAL; 
        break;
    }

    return ret;
}

gint32 CRpcStream2::GetAllIrpsQueued(
    std::vector< IrpPtr >& vecIrps )
{
    for( auto pIrp : m_queReadIrps )
    {
        vecIrps.push_back( pIrp );
    }
    for( auto pIrp : m_queWriteIrps )
    {
        vecIrps.push_back( pIrp );
    }
    return 0;
}

gint32 CRpcStream2::StartSendDeferred(
    PIRP pIrp )
{
    gint32 ret = 0;
    bool bPopState = false;
    guint32 dwOldState = 0;

    do{
        if( pIrp == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        CStdRMutex oIrpLock( pIrp->GetLock() );
        ret = pIrp->CanContinue( IRP_STATE_READY );
        if( ERROR( ret ) )
            break;

        if( m_pParentPort == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ret = m_pParentPort->CanContinue( pIrp,
            PORT_STATE_BUSY_SHARED, &dwOldState );

        if( ERROR( ret ) )
            break;

        bPopState = true;

        IPort* plowp =
            m_pParentPort->GetLowerPort();

        ret = SetupIrpForLowerPort( plowp, pIrp );
        if( ERROR( ret ) )
            break;

        // don't complete this irp from within the
        // submitIrp
        pIrp->SetNoComplete( true );

        ret = plowp->SubmitIrp( pIrp );

        pIrp->SetNoComplete( false );

        if( ret == STATUS_PENDING )
        {
            ret = 0;
            break;
        }

        if( SUCCEEDED( ret ) )
            Refresh();

        if( !pIrp->IsIrpHolder() )
        {
            IrpCtxPtr& pTopCtx =
                pIrp->GetTopStack();

            IrpCtxPtr& pCurCtx =
                pIrp->GetCurCtx();

            pCurCtx->SetStatus(
                pTopCtx->GetStatus() );

            pIrp->PopCtxStack();
        }

        oIrpLock.Unlock();

        m_pMgr->CompleteIrp( pIrp );

    }while( 0 );

    if( ret != STATUS_PENDING )
    {
        CStdRMutex oPortLock(
            m_pParentPort->GetLock() );
        m_pParentPort->SetSending( false );
    }

    if( bPopState )
        m_pParentPort->PopState( dwOldState );

    return ret;
}

gint32 CompressDataLZ4(
    const BufPtr& pPayload, BufPtr& pCompressed )
{
    gint32 ret = 0;
    if( pPayload.IsEmpty() || pPayload->empty() )
        return -EINVAL;

    do{
        if( pCompressed.IsEmpty() )
        {
            ret = pCompressed.NewObj();
            if( ERROR( ret ) )
                break;
        }

        guint32 dwMaxSize = 0;
        ret = pPayload->Compress(
            nullptr, dwMaxSize );

        if( ERROR( ret ) )
            break;

        pCompressed->Resize(
            sizeof( guint32 ) + dwMaxSize );

        guint32 dwActSize = dwMaxSize;
        guint8* pDest = ( guint8* )
            pCompressed->ptr();

        pDest += sizeof( guint32 );

        ret = pPayload->Compress(
            pDest, dwActSize );

        if( ERROR( ret ) )
            break;

        if( dwActSize < dwMaxSize )
        {
            guint32 dwFinSize =
                sizeof( guint32 ) + dwActSize;
            pCompressed->Resize( dwFinSize );
            *( ( guint32* )pCompressed->ptr() ) =
                htonl( pPayload->size() );
        }

    }while( 0 );

    return ret;
}

gint32 DecompressDataLZ4(
    BufPtr& pPayload, BufPtr& pDecompressed )
{
    gint32 ret = 0;
    if( pPayload.IsEmpty() || pPayload->empty() )
        return -EINVAL;

    do{
        guint32 dwOrigSize = 0;
        if( pDecompressed.IsEmpty() )
        {
            ret = pDecompressed.NewObj();
            if( ERROR( ret ) )
                break;
        }

        dwOrigSize = ntohl(
            *( guint32* )pPayload->ptr() );
        
        if( dwOrigSize == 0 ||
            dwOrigSize > MAX_BYTES_PER_TRANSFER )
        {
            ret = -EBADMSG;
            break;
        }

        pDecompressed->Resize( dwOrigSize );

        pPayload->IncOffset( sizeof( guint32 ) );

        ret = pPayload->Decompress( ( guint8* )
            pDecompressed->ptr(), dwOrigSize );

        pPayload->IncOffset( sizeof( guint32 ) );

    }while( 0 );

    return ret;
}

gint32 CRpcStream2::SetupIrpForLowerPort(
    IPort* pLowerPort, PIRP pIrp ) const
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    IrpCtxPtr pCtx = pIrp->GetCurCtx();

    do{
        guint32 dwMajorCmd = pCtx->GetMajorCmd();
        guint32 dwMinorCmd = pCtx->GetMinorCmd();
        guint32 dwIoDir = pCtx->GetIoDirection();

        if( dwMajorCmd != IRP_MJ_FUNC )
        {
            ret = -EINVAL;
            break;
        }

        if( dwMinorCmd != IRP_MN_IOCTL &&
            dwMinorCmd != IRP_MN_WRITE )
        {
            ret = -EINVAL;
            break;
        }

        if( dwMinorCmd == IRP_MN_WRITE &&
            dwIoDir != IRP_DIR_OUT )
        {
            ret = -EINVAL;
            break;
        }

        if( dwMinorCmd == IRP_MN_IOCTL &&
            ( dwIoDir != IRP_DIR_INOUT &&
              dwIoDir != IRP_DIR_OUT ) )
        {
            ret = -EINVAL;
            break;
        }

        CfgPtr pCfg;
        ret = pCtx->GetReqAsCfg( pCfg );
        if( ERROR( ret ) )
            break;

        CCfgOpener oCfg(
            ( IConfigDb* )pCfg );
       
        BufPtr pPayload;
        if( dwMinorCmd == IRP_MN_WRITE )
        {
            ret = oCfg.GetProperty( 0, pPayload );
            if( ERROR( ret ) )
                break;
        }
        else
        {
            ret = pPayload.NewObj();
            if( ERROR( ret ) )
                break;
            ret = pCfg->Serialize( *pPayload );
            if( ERROR( ret ) )
                break;
        }

        if( m_pParentPort->IsCompress() )
        {
            BufPtr pCompressed;
            ret = CompressDataLZ4(
                pPayload, pCompressed );
            if( ERROR( ret ) )
                break;
            pPayload = pCompressed;
        }

        guint32 dwSeqNo = 0;
        oCfg.GetIntProp( propSeqNo, dwSeqNo );
        
        CPacketHeader oHeader;
        oHeader.m_iStmId    = m_iStmId;
        oHeader.m_iPeerStmId = m_iPeerStmId;
        oHeader.m_wProtoId  = m_wProtoId;
        oHeader.m_dwSize    = pPayload->size();
        oHeader.m_dwSessId  = 0;

        if( m_pParentPort->IsCompress() )
        {
            oHeader.m_wFlags =
                RPC_PACKET_FLG_COMPRESS ;
        }

        if( dwSeqNo != 0 )
            oHeader.m_dwSeqNo = dwSeqNo;
        else
            oHeader.m_dwSeqNo = GetSeqNo();

        BufPtr pHeader( true );
        ret = oHeader.Serialize( *pHeader );
        if( ERROR( ret ) )
            break;

        ret = pIrp->AllocNextStack( pLowerPort );
        if( ERROR( ret ) )
            break;

        CParamList oParams;

        guint32 dwTotalBytes =
            pHeader->size() + pPayload->size();

        if( dwTotalBytes < ( guint32 )PAGE_SIZE )
        {
            ret = pHeader->Append(
                ( guint8* )pPayload->ptr(),
                pPayload->size() );

            if( ERROR( ret ) )
                break;

            // less network traffic
            oParams.Push( pHeader );
        }
        else
        {
            // less memory and copies
            // a scatter-gather list
            oParams.Push( pHeader );
            oParams.Push( pPayload );
        }

        BufPtr pBufNew( true );
        *pBufNew = ObjPtr( oParams.GetCfg() );

        IrpCtxPtr pNewCtx = pIrp->GetTopStack();
        pNewCtx->SetMajorCmd( IRP_MJ_FUNC );
        pNewCtx->SetMinorCmd( IRP_MN_WRITE );
        pNewCtx->SetIoDirection( IRP_DIR_OUT );
        pNewCtx->SetReqData( pBufNew );
        pLowerPort->AllocIrpCtxExt(
            pNewCtx, pIrp );

    }while( 0 );

    return ret;
}

gint32 CRpcStream2::StartSend(
    IrpPtr& pIrpToComp )
{
    gint32 ret = 0;
    do{
        IrpPtr pIrp;
        IrpPtr pIrpLocked = pIrpToComp;
        gint32 iQueSize = GetQueSizeSend();
        if( iQueSize <= 0 )
        {
            ret = -ENOENT;
            break;
        }
        else
        {
            pIrp = m_queWriteIrps.front();
            m_queWriteIrps.pop_front();
        }

        IPort* plowp =
            m_pParentPort->GetLowerPort();

        if( plowp == nullptr )
        {
            pIrpToComp = pIrp;
            ret = -EFAULT;
            break;
        }

        m_pParentPort->SetSending( true );
        if( pIrp == pIrpLocked )
        {
            // build the irp here instead of
            // HandleWriteIrp to avoid timeout or
            // canceling with unwanted context stack
            ret = SetupIrpForLowerPort( plowp, pIrp );
            if( SUCCEEDED( ret ) )
            {
                // irp is safe to access
                // move on to submit the irp
                ret = plowp->SubmitIrp( pIrp );
                if( SUCCEEDED( ret ) )
                    Refresh();
            }
        }
        else
        {
            // NOTE: the irp cannot be locked at
            // this moment, and we need to defer
            // sending the irp to the lower port
            // later when the irp can be locked.
            TaskletPtr pTask;
            ret = DEFER_CALL_NOSCHED( pTask,
                ObjPtr( this ),
                &CRpcStream2::StartSendDeferred,
                pIrp );

            if( SUCCEEDED( ret ) )
            {
                ret = m_pMgr->RescheduleTask(
                    pTask );
                if( SUCCEEDED( ret ) )
                {
                    ret = STATUS_PENDING;
                }
                else
                {
                    IrpCtxPtr pTopCtx =
                        pIrp->GetTopStack();
                    pTopCtx->SetStatus( ret );
                }
            }
        }

        if( ret != STATUS_PENDING )
        {
            // no pending write
            m_pParentPort->SetSending( false );

            if( !pIrp->IsIrpHolder() )
            {
                IrpCtxPtr& pTopCtx =
                    pIrp->GetTopStack();

                IrpCtxPtr& pCurCtx =
                    pIrp->GetCurCtx();

                pCurCtx->SetStatus(
                    pTopCtx->GetStatus() );

                ret = pCurCtx->GetStatus();
                pIrp->PopCtxStack();
            }
        }

        pIrpToComp = pIrp;

    }while( 0 );

    return ret;
}

gint32 CRpcStream2::SetRespReadIrp(
    IRP* pIrp, CBuffer* pBuf )
{
    gint32 ret = 0;
    do{
        CStdRMutex oIrpLock( pIrp->GetLock() );
        ret = pIrp->CanContinue( IRP_STATE_READY );
        if( ERROR( ret ) )
        {
            // no irp to complete
            break;
        }

        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        guint32 dwMajorCmd = pIrp->MajorCmd();
        guint32 dwMinorCmd = pIrp->MinorCmd();

        if( dwMajorCmd == IRP_MJ_FUNC &&
            dwMinorCmd == IRP_MN_READ )
        {
            if( pCtx->m_pRespData.IsEmpty() )
                pCtx->SetRespData( pBuf );
            else
            {
                if( pBuf->GetObjId() !=
                    pCtx->m_pRespData->GetObjId() )
                {
                    guint8* pData =
                        ( guint8* )pBuf->ptr();
                    pCtx->m_pRespData->Append(
                        pData, pBuf->size() );
                }
                else
                {
                    // the resp is already set in
                    // GetReadIrpsToComp
                }
            }
            pCtx->SetStatus( 0 );
        }
        else
        {
            pCtx->SetStatus( -ENOTSUP );
            ret = -ENOTSUP;
        }

    }while( 0 );

    return ret;
}

gint32 CRpcStream2::DispatchDataIncoming()
{
    gint32 ret = 0;
    do{
        CIoManager* pMgr = GetIoMgr();
        using ElemType =
            std::pair< IrpPtr, BufPtr >;

        std::vector< ElemType > vecIrps;
        if( true )
        {
            ret = GetReadIrpsToComp( vecIrps );
        }

        for( auto&& oPair : vecIrps )
        {
            ret = SetRespReadIrp(
                oPair.first, oPair.second );

            if( SUCCEEDED( ret ) )
                pMgr->CompleteIrp( oPair.first );
        }

    }while( 0 );

    return ret;
}

gint32 CRpcStream2::HandleWriteIrp(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    CStdRMutex oPortLock(
        m_pParentPort->GetLock() );
    return QueueIrpOutgoing( pIrp );
}

gint32 CRpcStream2::RecvPacket(
        CCarrierPacket* pPacketToRecv )
{
    if( pPacketToRecv == nullptr )
        return -EINVAL;

    PacketPtr pPacket( pPacketToRecv );
    m_queBufToRecv.push_back( pPacket );
    guint32 dwSize = m_queBufToRecv.size();

    m_atmSeqNo++;
    // we need to consider better flow control
    // method, like leaky bucket?
    if( dwSize > STM_MAX_RECV_PACKETS )
        m_queBufToRecv.pop_front();

    return 0;
}

gint32 CRpcControlStream2::GetReadIrpsToComp(
    std::vector< std::pair< IrpPtr, BufPtr > >& vecIrps )
{
    // this method override the base method, and
    // goes beyond READ irps, and it will also
    // scan the req map to find the irp to match
    // the incoming packet
    gint32 ret = 0;

    CStdRMutex oPortLock(
        m_pParentPort->GetLock() );

    if( m_queReadIrps.size() +
        m_queReadIrpsEvt.size() +
        m_mapIrpsForResp.size() == 0 ||
        m_queBufToRecv2.size() == 0 )
        return -ENOENT;

    std::deque< CfgPtr > quePktPutback;
    while( m_queBufToRecv2.size() != 0 )
    {
        CfgPtr pCfg = m_queBufToRecv2.front();
        m_queBufToRecv2.pop_front();

        CReqOpener oCfg( ( IConfigDb* )pCfg );

        guint32 dwType = 0;
        ret = oCfg.GetReqType( dwType );
        if( ERROR( ret ) )
            continue;

        IrpPtr pIrp;
        if( dwType == DBUS_MESSAGE_TYPE_METHOD_CALL )
        {
            if( m_queReadIrps.empty() )
            {
                quePktPutback.push_back( pCfg );
                continue;
            }

            pIrp = m_queReadIrps.front();
            m_queReadIrps.pop_front();
        }
        else if( dwType == DBUS_MESSAGE_TYPE_SIGNAL )
        {
            if( m_queReadIrpsEvt.empty() )
            {
                quePktPutback.push_back( pCfg );
                continue;
            }

            pIrp = m_queReadIrpsEvt.front();
            m_queReadIrpsEvt.pop_front();
        }
        else if( dwType == DBUS_MESSAGE_TYPE_METHOD_RETURN )
        {
            guint32 dwSeqNo = 0;

            ret = oCfg.GetIntProp(
                propSeqNo, dwSeqNo );

            // discard the packet
            if( ERROR( ret ) )
                continue;

            // maybe we have arrived very soon,
            // and right before the irp is put to
            // this map because the system is
            // heavily loaded
            if( m_mapIrpsForResp.find( dwSeqNo )
                == m_mapIrpsForResp.end() )
            {
                quePktPutback.push_back( pCfg );
                continue;
            }

            pIrp = m_mapIrpsForResp[ dwSeqNo ];
            m_mapIrpsForResp.erase( dwSeqNo );
        }

        BufPtr pDeserialized( true );
        *pDeserialized = ObjPtr( pCfg );
        vecIrps.push_back( std::pair< IrpPtr, BufPtr >
            ( pIrp, pDeserialized ) );
    }

    m_queBufToRecv2 = quePktPutback;

    return ret;
}

gint32 CRpcControlStream2::QueueIrpForResp(
    IRP* pIrp )
{
    // NOTE: you should make sure the irp is
    // locked from outside
    if( pIrp == nullptr )
        return -EINVAL;

    IrpCtxPtr pCtx = pIrp->GetTopStack();
    if( pCtx->GetMajorCmd() != IRP_MJ_FUNC ||
        pCtx->GetMinorCmd() != IRP_MN_IOCTL )
        return -EINVAL;

    guint32 dwCtrlCode = pCtx->GetCtrlCode();

    if( dwCtrlCode != CTRLCODE_CLOSE_STREAM_PDO &&
        dwCtrlCode != CTRLCODE_OPEN_STREAM_PDO &&
        dwCtrlCode != CTRLCODE_LISTENING )
        return -ENOTSUP;

    gint32 ret = 0;
    do{
        if( dwCtrlCode == CTRLCODE_LISTENING )
        {
            EnumMatchType dwType =
                m_pStmMatch->GetType();

            if( ERROR( ret ) )
                break;

            std::deque< IrpPtr >* pQueIrps =
                &m_queReadIrps;

            if( dwType == matchClient )
                pQueIrps = &m_queReadIrpsEvt;

            pQueIrps->push_back( pIrp );
            break;
        }

        CfgPtr pCfg;
        ret = pCtx->GetReqAsCfg( pCfg );
        if( ERROR( ret ) )
            break;

        CCfgOpener oCfg( ( IConfigDb* )pCfg );
        guint32 dwReqSeqNo = 0;
        ret = oCfg.GetIntProp(
            propSeqNo, dwReqSeqNo );

        if( ERROR( ret ) )
            break;

        bool bFound = false;
        for( auto pCfg : m_queBufToRecv2 )
        {
            // it is possible the reply arrives ahead
            // of this irp geting queued here
            CReqOpener oCfg( ( IConfigDb* )pCfg );

            guint32 dwType = 0;
            ret = oCfg.GetReqType( dwType );
            if( ERROR( ret ) )
            {
                ret = 0;
                continue;
            }

            if( dwType != DBUS_MESSAGE_TYPE_METHOD_RETURN )
                continue;

            guint32 dwRespSeqNo = 0;
            ret = oCfg.GetIntProp(
                propSeqNo, dwRespSeqNo );

            // discard the packet
            if( ERROR( ret ) )
            {
                ret = 0;
                continue;
            }

            if( dwRespSeqNo != dwReqSeqNo )
                continue;

            bFound = true;
            BufPtr pBuf( true );
            *pBuf = ObjPtr( pCfg );
            pCtx->SetRespData( pBuf );
            pCtx->SetStatus( 0 );
            break;
        }

        if( !bFound )
        {
            m_mapIrpsForResp[ dwReqSeqNo ] =
                IrpPtr( pIrp );
        }

    }while( 0 );

    return ret;
}

gint32 CRpcControlStream2::SetRespReadIrp(
    IRP* pIrp, CBuffer* pBuf )
{
    gint32 ret = 0;
    do{
        CStdRMutex oIrpLock( pIrp->GetLock() );
        ret = pIrp->CanContinue(
            IRP_STATE_READY );

        if( ERROR( ret ) )
            break;

        guint32 dwMajorCmd = pIrp->MajorCmd();
        guint32 dwMinorCmd = pIrp->MinorCmd();

        IrpCtxPtr& pCtx = pIrp->GetTopStack();

        if( dwMajorCmd != IRP_MJ_FUNC ||
            dwMinorCmd != IRP_MN_IOCTL )
        {
            ret = -EINVAL;
            break;
        }

        switch( pCtx->GetCtrlCode() )
        {
        case CTRLCODE_OPEN_STREAM_PDO:
        case CTRLCODE_CLOSE_STREAM_PDO:
        case CTRLCODE_LISTENING:
            {
                pCtx->SetRespData( pBuf );
                pCtx->SetStatus( 0 );
                break;
            }
        default:
            ret = -ENOTSUP;
            break;
        }

    }while( 0 );

    return ret;
}

gint32 CRpcControlStream2::RemoveIoctlRequest(
        IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        EnumIoctlStat iState = reqStatInvalid;

        ret = CTcpStreamPdo::GetIoctlReqState(
            pIrp, iState );

        if( ERROR( ret ) )
            break;

        if( iState == reqStatOut )
        {
            // let's find the irp in the sending
            // queue
            std::deque< IrpPtr >:: iterator itr =
                m_queWriteIrps.begin();

            bool bErased = false;
            while( itr != m_queWriteIrps.end() )
            {
                IrpPtr pIrpToRmv = *itr;

                if( pIrp == pIrpToRmv )
                {
                    m_queWriteIrps.erase( itr );
                    bErased = true;
                    break;
                }
                ++itr;
            }
            if( !bErased )
            {
                ret = -ENOENT;
                break;
            }
        }
        else if( iState == reqStatIn )
        {
            CfgPtr pCfg;
            ret = pCtx->GetReqAsCfg( pCfg );
            if( ERROR( ret ) )
                break;

            CCfgOpener oCfg( ( IConfigDb* )pCfg );

            guint32 dwSeqNo = 0;

            ret = oCfg.GetIntProp(
                propSeqNo, dwSeqNo );

            if( ERROR( ret ) )
                break;

            if( m_mapIrpsForResp.find( dwSeqNo ) !=
                m_mapIrpsForResp.end() )
            {
                m_mapIrpsForResp.erase( dwSeqNo );
            }
            else
            {
                ret = -ENOENT;
            }
        }
        else
        {
            ret = -EINVAL;
        }

    }while( 0 );

    return ret;
}

gint32 CRpcControlStream2::RemoveIrpFromMap(
        IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        switch( pCtx->GetCtrlCode() )
        {
        case CTRLCODE_OPEN_STREAM_PDO:
        case CTRLCODE_CLOSE_STREAM_PDO:
        case CTRLCODE_INVALID_STREAM_ID_PDO:
            {
                ret = RemoveIoctlRequest( pIrp );
                break;
            }
        case CTRLCODE_LISTENING:
            {
                EnumMatchType dwType =
                    m_pStmMatch->GetType();

                std::deque< IrpPtr >* pQueIrps =
                    &m_queReadIrps;

                if( dwType == matchClient )
                    pQueIrps = &m_queReadIrpsEvt;

                std::deque< IrpPtr >::iterator itr
                    = pQueIrps->begin();

                while( itr != pQueIrps->end() )
                {
                    if( pIrp == *itr )
                    {
                        pQueIrps->erase( itr );
                        break;
                    }
                    ++itr;
                }
                if( itr == pQueIrps->end() )
                    ret = -ENOENT;

                break;
            }
        default:
            {
                ret = -ENOTSUP;
                break;
            }
        }
    }while( 0 );

    return ret;
}

CRpcControlStream2::CRpcControlStream2(
    const IConfigDb* pCfg )
    : super( pCfg )
{
    SetClassId( clsid( CRpcControlStream2 ) );
}

gint32 CRpcControlStream2::RecvPacket(
        CCarrierPacket* pPacketToRecv )
{
    if( pPacketToRecv == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        PacketPtr pPacket( pPacketToRecv );

        BufPtr pBuf;
        ret = pPacket->GetPayload( pBuf );
        if( ERROR( ret ) )
            break;

        CfgPtr pCfg( true );
        ret = pCfg->Deserialize( *pBuf );
        if( ERROR( ret ) )
        {
            pBuf.Clear();
            break;
        }

        m_queBufToRecv2.push_back( pCfg );
        guint32 dwSize = m_queBufToRecv2.size();

        // queue overflow
        if( dwSize > STM_MAX_RECV_PACKETS )
            m_queBufToRecv2.pop_front();

    }while( 0 );

    return ret;
}

gint32 CRpcControlStream2::HandleListening(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    IrpCtxPtr& pCtx = pIrp->GetCurCtx();
    gint32 ret = 0;
    do{
        // NOTE: no locking of IRP
 
        CfgPtr pCfg;
        ret = pCtx->GetReqAsCfg( pCfg );
        if( ERROR( ret ) )
            break;

        CCfgOpener oCfg( ( IConfigDb* )pCfg );

        CMessageMatch* pMatch = nullptr;
        ret = oCfg.GetPointer(
            propMatchPtr, pMatch );

        if( ERROR( ret ) )
            break;
        
        if( pMatch->ToString() !=
            m_pStmMatch->ToString() )
        {
            ret = -EINVAL;
            break;
        }

        EnumMatchType dwMatchType =
            m_pStmMatch->GetType();

        std::deque< IrpPtr >* pQueIrp =
            &m_queReadIrps;

        guint32 dwReqType =
            DBUS_MESSAGE_TYPE_METHOD_CALL;

        if( dwMatchType == matchClient )
        {
            pQueIrp = &m_queReadIrpsEvt;
            dwReqType =
                DBUS_MESSAGE_TYPE_SIGNAL;
        }

        if( m_queBufToRecv2.empty() ||
            pQueIrp->size() )
        {
            pQueIrp->push_back( pIrp );
            ret = STATUS_PENDING;
            break;
        }

        std::deque< CfgPtr >::iterator itr =
            m_queBufToRecv2.begin();

        while( itr != m_queBufToRecv2.end() )
        {
            CfgPtr pCfg =
                m_queBufToRecv2.front();

            CReqOpener oReq( pCfg );
            guint32 dwInReqType = 0;
            ret = oReq.GetReqType( dwInReqType );
            if( ERROR( ret ) )
            {
                ++itr;
                continue;
            }

            if( dwReqType != dwInReqType )
            {
                ++itr;
                continue;
            }

            BufPtr pBuf( true );
            *pBuf = ObjPtr( pCfg );
            pCtx->SetRespData( pBuf );
            m_queBufToRecv2.erase( itr );
            ret = 0;
            break;
        }

        if( itr == m_queBufToRecv2.end() )
        {
            ret = STATUS_PENDING;
            pQueIrp->push_back( pIrp );
        }

    }while( 0 );

    if( ERROR( ret ) ||
        ret == STATUS_PENDING )
        return ret;

    return ret;
}

/**
* @name HandleIoctlIrp, to setup the parameters to
* send and put the packet to the sending queue
* @{ */
/**  @} */
gint32 CRpcControlStream2::HandleIoctlIrp(
    IRP* pIrp )
{
    if( pIrp == nullptr )
        return -EINVAL;

    IrpCtxPtr pCtx = pIrp->GetCurCtx();
    if( pCtx->GetMajorCmd() != IRP_MJ_FUNC ||
        pCtx->GetMinorCmd() != IRP_MN_IOCTL )
        return -EINVAL;

    guint32 dwCtrlCode = pCtx->GetCtrlCode();
    gint32 ret = 0;

    if( dwCtrlCode == CTRLCODE_LISTENING )
        return HandleListening( pIrp );

    // get the config
    CfgPtr pCfg;
    ret = pCtx->GetReqAsCfg( pCfg );
    if( ERROR( ret ) )
        return ret;

    CReqOpener oCfg( ( IConfigDb* )pCfg );
    guint32 dwReqType = 0;
    ret = oCfg.GetReqType( dwReqType );
    if( ERROR( ret ) )
        return ret;

    BufPtr pPayload( true );
    switch( dwCtrlCode )
    {
    case CTRLCODE_CLOSE_STREAM_PDO:
        {
            guint32 dwSeqNo = 0;

            if( dwReqType !=
                DBUS_MESSAGE_TYPE_METHOD_RETURN )
            {
                CReqBuilder oParams(
                    ( const IConfigDb* )pCfg ) ;

                dwSeqNo = GetSeqNo();
                ret = oParams.SetIntProp(
                    propSeqNo, dwSeqNo );

                // copy the sequence no to the
                // request cfg for reference in
                // the future. the SeqNo is only
                // generatated at this point
                ret = oCfg.SetIntProp(
                    propSeqNo, dwSeqNo );

                gint32 iPeerStm = 0;
                ret = oCfg.GetIntProp(
                    1, ( guint32& )iPeerStm );

                if( ERROR( ret ) )
                    break;

                oParams.Push( iPeerStm );
            }

            ret = super::HandleWriteIrp( pIrp );
            break;
        }
    case CTRLCODE_OPEN_STREAM_PDO:
        {
            guint32 dwSeqNo = 0;

            if( dwReqType !=
                DBUS_MESSAGE_TYPE_METHOD_RETURN )
            {
                dwSeqNo = GetSeqNo();

                // copy the sequence #NO to the
                // request cfg for reference in
                // the future. the SeqNo is only
                // generatated at this point
                ret = oCfg.SetIntProp(
                    propSeqNo, dwSeqNo );
            }

            ret = super::HandleWriteIrp( pIrp );
            break;
        }
    case CTRLCODE_INVALID_STREAM_ID_PDO:
        {
            ret = super::HandleWriteIrp( pIrp );
            break;
        }
    case CTRLCODE_RMTSVR_OFFLINE_PDO:
    case CTRLCODE_RMTMOD_OFFLINE_PDO:
        {
            guint32 dwSeqNo = GetSeqNo();

            ret = oCfg.SetIntProp(
                propSeqNo, dwSeqNo );

            ret = super::HandleWriteIrp( pIrp );
            break;
        }
    case CTRLCODE_REG_MATCH:
        {
            CStdRMutex oPortLock(
                m_pParentPort->GetLock() );

            if( !m_pStmMatch.IsEmpty() )
            {
                // unregister the old match first
                ret = -EEXIST;
                break;
            }

            ObjPtr pMatch;
            ret = oCfg.GetObjPtr(
                propMatchPtr, pMatch );

            if( ERROR( ret ) )
                break;

            m_pStmMatch = pMatch;
            if( m_pStmMatch.IsEmpty() )
                ret = -EINVAL;

            break;
        }
    case CTRLCODE_UNREG_MATCH:
        {
            CStdRMutex oPortLock(
                m_pParentPort->GetLock() );

            CMessageMatch* pMatch = nullptr;
            ret = oCfg.GetPointer(
                propMatchPtr, pMatch );

            if( ERROR( ret ) )
                break;

            
            if( pMatch->ToString() !=
                m_pStmMatch->ToString() )
            {
                ret = -EINVAL;
                break;
            }

            m_pStmMatch.Clear();
            break;
        }
    default:
        ret = -ENOTSUP;
        break;
    }

    return ret;
}

gint32 CRpcControlStream2::GetAllIrpsQueued(
    std::vector< IrpPtr >& vecIrps )
{
    super::GetAllIrpsQueued( vecIrps );
    for( auto&& oPair : m_mapIrpsForResp )
    {
        vecIrps.push_back( oPair.second );
    }
    for( auto&& pIrp : m_queReadIrpsEvt )
    {
        vecIrps.push_back( pIrp );
    }
    return 0;
}

CRpcNativeProtoFdo::CRpcNativeProtoFdo(
    const IConfigDb* pCfg )
    : super( pCfg ),
    m_iStmCounter( STMSOCK_STMID_FLOOR )
{
    do{
        SetClassId( clsid( CRpcNativeProtoFdo ) );
        m_dwFlags &= ~PORTFLG_TYPE_MASK;
        m_dwFlags |= PORTFLG_TYPE_FDO;

    }while( 0 );

    return;
}

gint32 CRpcNativeProtoFdo::AddStream(
    gint32 iStream,
    guint16 wProtoId,
    gint32 iPeerStmId )
{
    gint32 ret = 0;

    if( iStream < 0 )
        return -EINVAL;

    do{
        CStdRMutex oPortLock( GetLock() );
        if( m_mapStreams.find( iStream ) !=
            m_mapStreams.end() )
        {
            ret = -EEXIST;
            break;
        }

        CParamList oParams;

        guint32 dwVal = iStream;
        oParams.Push( dwVal );

        dwVal = wProtoId;
        oParams.Push( dwVal );

        ret = oParams.SetPointer(
            propIoMgr, GetIoMgr() );

        if( ERROR( ret ) )
            break;

        ret = oParams.SetObjPtr(
            propPortPtr, ObjPtr( this ) );

        if( ERROR( ret ) )
            break;

        Stm2Ptr pStm;
        if( wProtoId == protoStream ||
            wProtoId == protoDBusRelay )
        {
            ret = pStm.NewObj(
                clsid( CRpcStream2 ),
                oParams.GetCfg() );
        }
        else if( wProtoId == protoControl )
        {
            ret = pStm.NewObj(
                clsid( CRpcControlStream2 ),
                oParams.GetCfg() );
        }
        else
        {
            ret = -ENOTSUP;
        }

        if( ERROR( ret ) )
            break;

        m_mapStreams[ iStream ] = pStm;
        pStm->SetPeerStmId( iPeerStmId );

        if( pStm->GetQueSizeSend() > 0 )
            AddStmToSendQue( pStm );

    }while( 0 );

    return ret;
}

gint32 CRpcNativeProtoFdo::CompleteOpenStmIrp(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    IrpCtxPtr pCtx = pIrp->GetCurCtx();
    ret = pCtx->GetStatus();
    if( ERROR( ret ) )
        return ret;

    do{
        CfgPtr pReqCfg;
        ret = pCtx->GetReqAsCfg( pReqCfg );
        if( ERROR( ret ) )
            break;

        CCfgOpener oReqCfg( ( IConfigDb* )pReqCfg );

        gint32 iReqStmId = 0;

        ret = oReqCfg.GetIntProp( 0,
            ( guint32& )iReqStmId );

        if( ERROR( ret ) )
            break;

        CfgPtr pCfg;
        ret = pCtx->GetRespAsCfg( pCfg );
        if( ERROR( ret ) )
            break;

        gint32 iRet = 0;
        CReqOpener oCfg( ( IConfigDb* )pCfg );

        do{
            ret = oCfg.GetIntProp(
                propReturnValue, ( guint32& )iRet );

            if( ERROR( ret ) )
                break;

            if( ERROR( iRet ) )
            {
                ret = iRet;
                break;
            }

            gint32 iLocalId = 0;
            ret = oCfg.GetIntProp(
                2, ( guint32& )iLocalId );

            if( ERROR( ret ) )
                break;

            if( iReqStmId != iLocalId )
            {
                ret = -EINVAL;
                break;
            }

            guint32 dwProtoId = 0;
            ret = oCfg.GetIntProp( 1,
                ( guint32& )dwProtoId );

            if( ERROR( ret ) )
                break;

            guint32 dwProtoIdReq = 0;
            ret = oReqCfg.GetIntProp( 1,
                ( guint32& )dwProtoIdReq );

            if( ERROR( ret ) )
                break;

            if( dwProtoIdReq != dwProtoId )
            {
                ret = -EINVAL;
                break;
            }

            gint32 iPeerId = 0;
            ret = oCfg.GetIntProp(
                0, ( guint32& )iPeerId );

            if( ERROR( ret ) )
                break;

            ret = AddStream( iLocalId,
                dwProtoId, iPeerId );

            if( ERROR( ret ) )
                break;

            // this is the returned parameter
            // oCfg.ClearParams();
            // swap the local and peer id
            // that is, local is at 0, and peer at 2
            oCfg.SwapProp( 0, 2 );

        }while( 0 );

        oCfg.SetIntProp(
            propReturnValue, ret );
        
    }while( 0 );

    return ret;
}

gint32 CRpcNativeProtoFdo::CompleteCloseStmIrp(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    IrpCtxPtr& pCtx = pIrp->GetCurCtx();

    do{
        CfgPtr pReqCfg;
        ret = pCtx->GetReqAsCfg( pReqCfg );
        if( ERROR( ret ) )
            break;

        CCfgOpener oReqCfg( ( IConfigDb* )pReqCfg );

        gint32 iReqPeerId = 0;

        ret = oReqCfg.GetIntProp( 1,
            ( guint32& )iReqPeerId );

        if( ERROR( ret ) )
            break;

        CfgPtr pCfg;
        ret = pCtx->GetRespAsCfg( pCfg );
        if( ERROR( ret ) )
            break;

        gint32 iRet = 0;
        CReqOpener oCfg( ( IConfigDb* )pCfg );

        do{
            ret = oCfg.GetIntProp(
                propReturnValue, ( guint32& )iRet );

            if( ERROR( ret ) )
                break;

            if( ERROR( iRet ) )
            {
                ret = iRet;
            }

            gint32 iPeerId = 0;
            ret = oCfg.GetIntProp(
                0, ( guint32& )iPeerId );

            if( ERROR( ret ) )
                break;

            if( iReqPeerId != iPeerId )
            {
                ret = -EINVAL;
                break;
            }

            gint32 iLocalStmId = 0;
            ret = oReqCfg.GetIntProp( 0,
                ( guint32& )iLocalStmId );

            if( ERROR( ret ) )
                break;

            ret = RemoveStream( iLocalStmId );

        }while( 0 );

        oCfg.ClearParams();
        oCfg.SetIntProp(
            propReturnValue, ret );
        
    }while( 0 );

    return ret;
}

gint32 CRpcNativeProtoFdo::GetStreamByPeerId(
    gint32 iPeerId, Stm2Ptr& pStm )
{
    if( iPeerId < 0 )
        return -EINVAL;

    if( IsReserveStm( iPeerId ) )
    {
        return GetStream( iPeerId, pStm );
    }

    for( auto&& oElem : m_mapStreams )
    {
        gint32 iStmId;
        gint32 ret = ( oElem.second )->
            GetPeerStmId( iStmId );

        if( ERROR( ret ) )
            continue;

        pStm = oElem.second;
        return 0;
    }
    return -ENOENT;
}

gint32 CRpcNativeProtoFdo::GetStreamFromIrp(
    IRP* pIrp, Stm2Ptr& pStm )
{
    if( pIrp == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        guint32 dwMajorCmd = pCtx->GetMajorCmd();
        guint32 dwMinorCmd = pCtx->GetMinorCmd();

        if( dwMajorCmd == IRP_MJ_FUNC &&
            ( dwMinorCmd == IRP_MN_READ ||
              dwMinorCmd == IRP_MN_WRITE ) )
        {
            CfgPtr pCfg;
            ret = pCtx->GetReqAsCfg( pCfg );
            if( ERROR( ret ) )
                break;

            CCfgOpener oCfg(
                ( IConfigDb* )pCfg );
           
            gint32 iStmId;
            ret = oCfg.GetIntProp(
                propStreamId, ( guint32& )iStmId );

            if( ERROR( ret ) )
                break;
            
            ret = GetStream( iStmId, pStm );
            if( ERROR( ret ) )
                break;
        }
        else
        {
            ret = GetCtrlStmFromIrp( pIrp, pStm );
        }

    }while( 0 );

    return ret;
}

gint32 CRpcNativeProtoFdo::GetCtrlStmFromIrp(
    IRP* pIrp, Stm2Ptr& pStm )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;

    do{
        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        if( pCtx->GetMajorCmd() != IRP_MJ_FUNC ||
            pCtx->GetMinorCmd() != IRP_MN_IOCTL )
        {
            ret = -ENOTSUP;
            break;
        }

        switch( pCtx->GetCtrlCode() )
        {
        case CTRLCODE_OPEN_STREAM_PDO:
        case CTRLCODE_CLOSE_STREAM_PDO:
        case CTRLCODE_LISTENING:
        case CTRLCODE_REG_MATCH:
        case CTRLCODE_UNREG_MATCH:
            {
                CfgPtr pCfg;
                ret = pCtx->GetReqAsCfg( pCfg );
                if( ERROR( ret ) )
                    break;

                CCfgOpener oCfg(
                    ( IConfigDb* )pCfg );
               
                gint32 iStmId = 0;
                ret = oCfg.GetIntProp( propStreamId,
                    ( guint32& )iStmId );

                if( ERROR( ret ) )
                    break;
                
                ret = GetStream( iStmId, pStm );
                break;
            }
        default:
            {
                ret = -ENOTSUP;
                break;
            }
        }
    }while( 0 );

    return ret;
}

gint32 CRpcNativeProtoFdo::RemoveIrpFromMap(
    IRP* pIrp )
{
    gint32 ret = 0;

    do{
        CStdRMutex oPortLock( GetLock() );

        Stm2Ptr pStm;
        ret = GetStreamFromIrp( pIrp, pStm );
        if( ERROR( ret ) )
            break;

        ret = pStm->RemoveIrpFromMap( pIrp );

    }while( 0 );

    return ret;
}

gint32 CRpcNativeProtoFdo::OnPortStackReady(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    IPort* pPdo = GetBottomPort();

    gint32 ret = StartListeningTask();
    if( ERROR( ret ) )
        return ret;

    FireRmtSvrEvent(
        pPdo, eventRmtSvrOnline );

    pIrp->GetCurCtx()->SetStatus( ret );

    return 0;
}

gint32 CRpcNativeProtoFdo::RemoveStream(
    gint32 iStream )
{
   if( iStream < 0 )
       return -EINVAL;

    gint32 ret = 0;
    CStdRMutex oPortLock( GetLock() );
    do{

        if( m_mapStreams.find( iStream )
            == m_mapStreams.end() )
        {
            ret = -ENOENT;
            break;
        }

        Stm2Ptr pStm = m_mapStreams[ iStream ];
        m_mapStreams.erase( iStream );

        std::deque< Stm2Ptr >::iterator itr =
            m_cycQueSend.begin();

        Stm2Ptr pCurStm;
        if( IsSending() )
            pCurStm = m_pCurStm;

        while( itr != m_cycQueSend.end() )
        {
            if( !( *itr == pStm ) )
            {
                ++itr;
                continue;
            }

            itr = m_cycQueSend.erase( itr );

            if( m_cycQueSend.empty() )
                SetSending( false );

            if( !IsSending() )
                break;

            if( pCurStm == pStm )
            {
                // assign -1 to restart the send
                // on other streams.
                SetSending( false );
                break;
            }

            break;
        }

    }while( 0 );

    if( !IsSending() )
        m_pCurStm.Clear();

    return ret;
}

gint32 CRpcNativeProtoFdo::GetStreamToSend(
    Stm2Ptr& pStm )
{
    // some write request is in process
    if( IsSending() )
        return -EAGAIN;

    m_pCurStm.Clear();
    while( !m_cycQueSend.empty() )  
    {
        pStm = m_cycQueSend.front();
        if( pStm->GetQueSizeSend() > 0 )
        {
            if( m_cycQueSend.size() > 1 )
            {
                // rotate
                m_cycQueSend.pop_front();
                m_cycQueSend.push_back( pStm );
            }
            m_pCurStm = pStm;
            break;
        }
        m_cycQueSend.pop_front();
    }

    if( m_cycQueSend.empty() )
        return -ENOENT;

    return 0;
}

/**
* @name StartSend
* @brief To select the most eligible buffer to
* send among the streams. The strategy is to
* round-robin the streams with buffers to send.
* an irp from a stream at a time. On done, rotate
* to the next stream
* @{ */
/** 
 * Parameters:
 *  pIrpLocked: the irp whose lock is held and can
 *  access freely. Usually this means the call is
 *  from SubmitIrp. If it can be a nullptr, which
 *  indicates the caller is irp completion routine
 *  or an asynchronous task.
 * @} */

gint32 CRpcNativeProtoFdo::StartSend(
    IRP* pIrpLocked )
{
    gint32 ret = 0;
    gint32 retLocked = STATUS_PENDING;

    IrpVec2Ptr pIrpVec( true );
    std::vector< CStlIrpVector2::ElemType >&
        vecIrpComplete = ( *pIrpVec )();

    CIoManager* pMgr = GetIoMgr();
    if( pMgr == nullptr )
        return -EFAULT;

    IrpCtxPtr pCtx;
    bool bHandled = false;
    bool bControl = false;

    if( pIrpLocked != nullptr )
    {
        pCtx = pIrpLocked->GetCurCtx();
        guint32 dwMinorCmd = pCtx->GetMinorCmd();
        if( dwMinorCmd == IRP_MN_IOCTL )
            bControl = true;
    }

    // ALERT: complicated section ahead
    do{
        Stm2Ptr pStream;
        CStdRMutex oPortLock( GetLock() );
        ret = GetStreamToSend( pStream );
        if( ret == -EAGAIN )
        {
            // There is another request going on
            // possibly this irp is grabbed by the
            // completion handler. Let them move
            // on.
            ret = STATUS_PENDING;
            DebugPrint( 0, "stream is busy" );
            break;
        }
        else if( ret == -ENOENT )
        {
            // this irp disappers in the thin air
            // should be impossible. the irp is
            // toxic, and don't touch it any more.
            // DebugPrint( 0, "no stream to send" );
            ret = STATUS_PENDING;
            break;
        }
        else if( ERROR( ret ) )
        {
            // weird errors.
            if( pIrpLocked != nullptr )
            {
                pCtx->SetStatus( ret );
                retLocked = ret;
            }
            break;
        }

        IrpPtr pIrp = pIrpLocked;
        // NOTE: pIrp is not necessarily the irp
        // to send, just a hint for the stream as
        // the irp is currently locked. The
        // returned pIrp is the one currently
        // being send out
        ret = pStream->StartSend( pIrp );
        if( ret == STATUS_PENDING )
            break;

        if( pIrp.IsEmpty() )
        {
            // find next stream to send
            ret = 0;
            continue;
        }

        if( pIrpLocked == pIrp )
        {
            pCtx->SetStatus( ret );
            retLocked = ret;
            bHandled = true;
        }

        CStlIrpVector2::ElemType e( ret, pIrp );
        vecIrpComplete.push_back( e );

        // fatal error
        if( ERROR( ret ) )
            break;

    }while( 1 );

    do{
        if( vecIrpComplete.empty() )
            break;

        bool bSync = ( pIrpLocked == nullptr );
        if( !bHandled )
        {
            goto LBL_SCHEDCOMP;
        }
        else if( pIrpLocked !=
            vecIrpComplete.front().second )
        {
            goto LBL_SCHEDCOMP;
        }
        else if( vecIrpComplete.size() > 1 )
        {
            goto LBL_SCHEDCOMP;
        }
        else if( !bControl )
        {
            ret = retLocked;
            break;
        }
        else if( SUCCEEDED( retLocked ) )
        {
            goto LBL_SCHEDCOMP;
        }

        // bControl && ( pending or error )
        ret = retLocked;
        break;

LBL_SCHEDCOMP:
        ret = ScheduleCompleteIrpTask(
            GetIoMgr(), pIrpVec, bSync );
        if( SUCCEEDED( ret ) )
            ret = STATUS_PENDING;

        break;

    }while( 0 );

    return ret;
}

gint32 CRpcNativeProtoFdo::SubmitReadIrp(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CStdRMutex oPortLock( GetLock() );
        IrpCtxPtr& pCtx = pIrp->GetCurCtx();
        guint32 dwMajorCmd = pCtx->GetMajorCmd();
        guint32 dwMinorCmd = pCtx->GetMinorCmd();
        if( dwMajorCmd == IRP_MJ_FUNC && 
            dwMinorCmd == IRP_MN_READ )
        {
            CfgPtr pCfg;
            ret = pCtx->GetReqAsCfg( pCfg );
            if( ERROR( ret ) )
                break;

            CCfgOpener oCfg(
                ( IConfigDb* )pCfg );
           
            gint32 iStmId;
            ret = oCfg.GetIntProp(
                propStreamId, ( guint32& )iStmId );

            if( ERROR( ret ) )
                break;
            
            Stm2Ptr pStm;
            ret = GetStream( iStmId, pStm );
            if( ERROR( ret ) )
                break;

            ret = pStm->HandleReadIrp( pIrp );
        }
        else
        {
            ret = -ENOTSUP;
            break;
        }

    }while( 0 );

    return ret;
}

gint32 CRpcNativeProtoFdo::OnSubmitIrp(
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
        case IRP_MN_READ:
            {
                // the parameter is a IConfigDb*
                // and contains
                // 0: stream id
                ret = SubmitReadIrp( pIrp );
                break;
            }
        case IRP_MN_WRITE:
            {
                // the parameter is a IConfigDb*
                // and contains
                // 0: stream id
                // 1: the payload buffer
                ret = SubmitWriteIrp( pIrp );
                break;
            }
        case IRP_MN_IOCTL:
            {
                switch( pIrp->CtrlCode() )
                {
                case CTRLCODE_INVALID_STREAM_ID_PDO:
                case CTRLCODE_OPEN_STREAM_PDO:
                case CTRLCODE_CLOSE_STREAM_PDO:
                case CTRLCODE_RMTSVR_OFFLINE_PDO:
                case CTRLCODE_RMTMOD_OFFLINE_PDO:
                case CTRLCODE_LISTENING:
                case CTRLCODE_REG_MATCH:
                case CTRLCODE_UNREG_MATCH:
                case CTRLCODE_OPEN_STREAM_LOCAL_PDO:
                case CTRLCODE_CLOSE_STREAM_LOCAL_PDO:
                case CTRLCODE_GET_LOCAL_STMID:
                case CTRLCODE_GET_RMT_STMID:
                case CTRLCODE_RESET_CONNECTION:
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
                break;
            }
        default:
            {
                ret = PassUnknownIrp( pIrp );
                break;
            }
        }
    }while( 0 );

    if( ret != STATUS_PENDING )
    {
        pIrp->GetCurCtx()->SetStatus( ret );
    }

    return ret;
}

gint32 CRpcNativeProtoFdo::CompleteReadIrp(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    IrpCtxPtr& pCtx = pIrp->GetTopStack();
    return pCtx->GetStatus();;
}

gint32 CRpcNativeProtoFdo::CompleteWriteIrp(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() < 2 )
        return -EINVAL;

    IrpCtxPtr& pCtx = pIrp->GetTopStack();
    gint32 ret = pCtx->GetStatus();;
    IrpCtxPtr& pCurCtx = pIrp->GetCurCtx();
    pCurCtx->SetStatus( ret );
    if( !pIrp->IsIrpHolder() )
        pIrp->PopCtxStack();

    if( SUCCEEDED( ret ) )
    {
        CStdRMutex oPortLock( GetLock() ); 
        Stm2Ptr pStm;
        ret = GetStreamFromIrp( pIrp, pStm );
        if( SUCCEEDED( ret ) )
            pStm->Refresh();
        SetSending( false );
        oPortLock.Unlock();

        StartSend( nullptr );
    }
    return ret;
}

gint32 CRpcNativeProtoFdo::AddStmToSendQue(
    Stm2Ptr& pStm )
{
    if( pStm.IsEmpty() )
        return -EINVAL;

    for( auto elem : m_cycQueSend )
    {
        if( pStm == elem )
            return 0;
    }

    m_cycQueSend.push_back( pStm );

    return 0;
}

gint32 CRpcNativeProtoFdo::SubmitWriteIrp(
    IRP* pIrp )
{
    if( pIrp == nullptr
        || pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        if( pIrp->MinorCmd() != IRP_MN_WRITE )
        {
            ret = -ENOTSUP;
            break;
        }

        Stm2Ptr pStm;
        CStdRMutex oPortLock( GetLock() );
        ret = GetStreamFromIrp( pIrp, pStm );
        if( ERROR( ret ) )
            break;

        ret = pStm->HandleWriteIrp( pIrp );
        if( ERROR( ret ) )
            break;

        if( pStm->GetQueSizeSend() > 0 )
            AddStmToSendQue( pStm );

        oPortLock.Unlock();

        // don't check the return code here
        ret = StartSend( pIrp );

    }while( 0 );

    return ret;
}

bool CRpcNativeProtoFdo::IsImmediateReq(
    IRP* pIrp )
{
    if( pIrp == nullptr
        || pIrp->GetStackSize() == 0 )
        return true;

    bool ret = false;
    do{
        IrpCtxPtr& pCtx = pIrp->GetCurCtx();
        guint32 dwCtrlCode = pCtx->GetCtrlCode();
        if( dwCtrlCode == CTRLCODE_CLOSE_STREAM_LOCAL_PDO ||
            dwCtrlCode == CTRLCODE_OPEN_STREAM_LOCAL_PDO ||
            dwCtrlCode == CTRLCODE_GET_RMT_STMID || 
            dwCtrlCode == CTRLCODE_GET_LOCAL_STMID ||
            dwCtrlCode == CTRLCODE_REG_MATCH ||
            dwCtrlCode == CTRLCODE_UNREG_MATCH ||
            dwCtrlCode == CTRLCODE_RESET_CONNECTION )
            ret = true;
    }while( 0 );

    return ret;
}

gint32 CRpcNativeProtoFdo::HandleIoctlIrp(
    IRP* pIrp )
{
    // NOTE: you should make sure the irp is
    // locked from outside
    //
    //
    // properties from irp's m_pReqData
    //
    // 0:               the stream id to open/close
    //
    // propStreamId:    For CloseStream and OpenStream, the stream
    //                  id via which the command is sent
    //
    //                  For OpenLocalStream, ignored
    //                  For CloseLocalStream, the stream to close 
    //
    // propPeerStmId:   Peer stream id 
    //
    // propProtoId:     the protocol of the stream
    //
    // propSeqNo:       added by the socket , not by the irp maker
    //
    // propReqType:     the request type, a request,
    //      a response or an event
    //
    // propCmdId:       the request id, or an
    //      event id,
    //

    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CStdRMutex oPortLock( GetLock() );
        IrpCtxPtr& pCtx = pIrp->GetCurCtx();
        guint32 dwCtrlCode = pCtx->GetCtrlCode();

        CfgPtr pCfg;
        if( dwCtrlCode != CTRLCODE_RESET_CONNECTION )
        {
            ret = pCtx->GetReqAsCfg( pCfg );
            if( ERROR( ret ) )
                break;
        }

        CReqOpener oCfg(
            ( IConfigDb* )pCfg );

        CRpcControlStream2* pCtrlStm = nullptr;
        Stm2Ptr pStream;

        if( !IsImmediateReq( pIrp ) )
        {
            // check stream existance
            ret = GetCtrlStmFromIrp(
                pIrp, pStream );

            pCtrlStm = pStream;
            if( pCtrlStm == nullptr )
            {
                // CRpcStream does not support
                // ioctrl code
                ret = -ENOTSUP;
                break;
            }
        }

        switch( dwCtrlCode )
        {
        case CTRLCODE_GET_RMT_STMID:
        case CTRLCODE_GET_LOCAL_STMID:
            {
                gint32 iStmId = 0;
                gint32 iResultId = 0;
                guint32 dwProtoId = 0;
                // check steam existance
                ret = oCfg.GetIntProp( 0,
                    ( guint32& )iStmId );

                if( ERROR( ret ) )
                    break;

                iResultId = iStmId;
                if( iStmId == TCP_CONN_DEFAULT_CMD )
                {
                    dwProtoId = protoControl;
                }
                else if( iStmId == TCP_CONN_DEFAULT_STM )
                {
                    dwProtoId = protoDBusRelay;
                }
                else if( dwCtrlCode == CTRLCODE_GET_LOCAL_STMID )
                {
                    Stm2Ptr pStream;
                    ret = GetStreamByPeerId( iStmId, pStream );

                    if( ERROR( ret ) )
                        break;

                    iStmId = pStream->GetStreamId();
                    if( ERROR( ret ) )
                        break;

                    dwProtoId = pStream->GetProtoId();
                }
                else if( dwCtrlCode == CTRLCODE_GET_RMT_STMID )
                {
                    Stm2Ptr pStream;
                    ret = GetStream( iStmId, pStream );

                    if( ERROR( ret ) )
                        break;

                    ret = pStream->GetPeerStmId( iResultId );
                    if( ERROR( ret ) )
                        break;

                    dwProtoId = pStream->GetProtoId();
                }

                CParamList oParams;
                oParams.SetIntProp( propReturnValue, 0 );
                oParams.Push( ( guint32 )iResultId );
                oParams.Push( dwProtoId );

                BufPtr pBuf( true );
                *pBuf = ObjPtr( oParams.GetCfg() );
                pCtx->SetRespData( pBuf );

                break;
            }
        case CTRLCODE_CLOSE_STREAM_LOCAL_PDO:
            {
                gint32 iStreamId = -1;
                ret = oCfg.GetIntProp( 0,
                    ( guint32& )iStreamId );

                if( ERROR( ret ) )
                    break;

                if( IsReserveStm( iStreamId ) )
                {
                    ret = -EINVAL;
                    break;
                }

                Stm2Ptr pStream;
                ret = GetStream( iStreamId, pStream );

                if( ERROR( ret ) )
                    break;

                gint32 iPeerStm = 0;

                pStream->GetPeerStmId( iPeerStm );
                guint32 dwProtoId =
                    pStream->GetProtoId();

                IrpVecPtr pIrpVec( true );
                ret = pStream->GetAllIrpsQueued(
                    ( *pIrpVec )() );

                if( ERROR( ret ) )
                    break;

                ret = RemoveStream( iStreamId );
                if( ERROR( ret ) )
                    break;

                // cancel all the contained irps
                CParamList oParams;
                ret = oParams.Push( ERROR_CANCEL );

                if( ERROR( ret ) )
                    break;

                ObjPtr pObj;
                if( !( *pIrpVec )().empty() )
                {
                    pObj = pIrpVec;
                    if( pObj.IsEmpty() )
                    {
                        ret = -EFAULT;
                        break;
                    }
                    ret = oParams.Push( pObj );
                    if( ERROR( ret ) )
                        break;

                    oParams.SetPointer(
                        propIoMgr, GetIoMgr() );

                    ret = GetIoMgr()->ScheduleTask(
                        clsid( CCancelIrpsTask ),
                        oParams.GetCfg() );
                }

                if( true )
                {
                    // set the response data
                    CfgPtr pRespCfg( true );

                    CParamList oRespCfg(
                        ( IConfigDb* )pRespCfg );

                    oRespCfg.Push( iStreamId );
                    oRespCfg.Push( dwProtoId );
                    oRespCfg.Push( iPeerStm );

                    oRespCfg.SetIntProp(
                        propReturnValue, ret );

                    BufPtr pBuf( true );
                    *pBuf = ObjPtr( pRespCfg );

                    pCtx->SetRespData( pBuf );
                    // let the client check the
                    // response to find the new
                    // stream id
                    ret = 0;
                }
                break;
            }
        case CTRLCODE_OPEN_STREAM_LOCAL_PDO:
            {
                gint32 iStreamId = -1;

                gint32 iRequiredStm = -1;
                ret = oCfg.GetIntProp( 0,
                    ( guint32& )iRequiredStm );

                if( ERROR( ret ) )
                    break;

                guint32 dwProtoId = 0;
                ret = oCfg.GetIntProp( 1,
                    ( guint32& )dwProtoId );

                if( ERROR( ret ) )
                    break;

                if( IsReserveStm( iRequiredStm ) )
                {
                    ret = -EINVAL;
                    break;
                }

                ret = NewStreamId( iStreamId );
                if( ERROR( ret ) )
                    break;

                ret = AddStream( iStreamId,
                    dwProtoId, iRequiredStm );

                if( ERROR( ret ) )
                    break;

                if( true )
                {
                    // set the response data
                    CfgPtr pRespCfg( true );

                    CParamList oRespCfg(
                        ( IConfigDb* )pRespCfg );

                    oRespCfg.Push( iStreamId );
                    oRespCfg.Push( dwProtoId );
                    oRespCfg.Push( iRequiredStm );

                    oRespCfg.SetIntProp(
                        propReturnValue, ret );

                    BufPtr pBuf( true );
                    *pBuf = ObjPtr( pRespCfg );

                    pCtx->SetRespData( pBuf );
                    // let the client check the
                    // response to find the new
                    // stream id
                    ret = 0;
                }
                break;
            }
            // non-local commands
        case CTRLCODE_CLOSE_STREAM_PDO:
            {   
                // this command happens only on
                // proxy side
                guint32 dwType =
                    DBUS_MESSAGE_TYPE_METHOD_CALL;

                ret = oCfg.GetReqType( dwType );
                if( ERROR( ret ) )
                    break;

                if( dwType ==
                    DBUS_MESSAGE_TYPE_METHOD_CALL )
                {
                    gint32 iLocalStm = 0;
                    ret = oCfg.GetIntProp( 0,
                        ( guint32& )iLocalStm );

                    if( ERROR( ret ) )
                        break;

                    Stm2Ptr pStm;
                    ret = GetStream( iLocalStm, pStm );
                    if( ERROR( ret ) )
                        break;

                    gint32 iPeerStmId = 0;
                    pStm->GetPeerStmId( iPeerStmId );
                    oCfg.Push( iPeerStmId );
                }

                ret = pCtrlStm->HandleIoctlIrp( pIrp );

                if( ERROR( ret ) )
                    break;

                if( pCtrlStm->GetQueSizeSend() > 0 )
                    AddStmToSendQue( pStream );

                break;
            }
        case CTRLCODE_INVALID_STREAM_ID_PDO:
            {
                guint32 dwSize = 0;
                ret = oCfg.GetSize( dwSize );
                if( ERROR( ret ) || dwSize < 4 )
                {
                    // at least for parameters
                    ret = -EINVAL;
                    break;
                }
                ret = pCtrlStm->HandleIoctlIrp( pIrp );

                if( ERROR( ret ) )
                    break;

                if( pCtrlStm->GetQueSizeSend() > 0 )
                    AddStmToSendQue( pStream );

                break;
            }
        case CTRLCODE_OPEN_STREAM_PDO:
            {
                // this command happens only on
                // proxy side
                // the new stream id will be
                // returned after this call
                // completes
                guint32 dwType =
                    DBUS_MESSAGE_TYPE_METHOD_CALL;

                ret = oCfg.GetReqType( dwType );
                if( ERROR( ret ) )
                    break;

                if( dwType == DBUS_MESSAGE_TYPE_METHOD_CALL )
                {
                    if( !oCfg.exist( 0 ) )
                    {
                        ret = -EINVAL;
                        break;
                    }
                    gint32 iStreamId = 0;
                    ret = NewStreamId( iStreamId );
                    guint32 dwProtocol = 0;

                    // adjust the params to send
                    oCfg.Pop( dwProtocol );
                    oCfg.Push( iStreamId );
                    oCfg.Push( dwProtocol );
                }

                ret = pCtrlStm->HandleIoctlIrp( pIrp );

                if( ERROR( ret ) )
                    break;

                if( pCtrlStm->GetQueSizeSend() > 0 )
                    AddStmToSendQue( pStream );

                break;
            }
        case CTRLCODE_LISTENING:
            {
                ret = pCtrlStm->HandleIoctlIrp( pIrp );
                break;
            }
        case CTRLCODE_REG_MATCH:
        case CTRLCODE_UNREG_MATCH:
            {
                Stm2Ptr pStream;
                ret = GetCtrlStmFromIrp(
                    pIrp, pStream );

                pCtrlStm = pStream;
                if( pCtrlStm == nullptr )
                {
                    ret = -EFAULT;
                    break;
                }
                ret = pCtrlStm->HandleIoctlIrp( pIrp );

                if( ERROR( ret ) )
                    break;

                if( pCtrlStm->GetQueSizeSend() > 0 )
                    AddStmToSendQue( pStream );

                break;
            }
        case CTRLCODE_RESET_CONNECTION:
            {
                IPort* pLowerPort =
                    GetLowerPort();
                if( pLowerPort == nullptr )
                {
                    ret = -EFAULT;
                    break;
                }
                pIrp->AllocNextStack( pLowerPort,
                    IOSTACK_ALLOC_COPY );
                ret = pLowerPort->SubmitIrp( pIrp );
                pIrp->PopCtxStack();
                break;
            }
        default:
            {
                ret = -ENOTSUP;
                break;
            }
        }

    }while( 0 );

    return ret;
}

gint32 CRpcNativeProtoFdo::CompleteInvalStmIrp(
    IRP* pIrp )
{
    // successfully send the the notification
    return 0;
}

gint32 CRpcNativeProtoFdo::CompleteIoctlIrp(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;

    IrpCtxPtr pCtx = pIrp->GetCurCtx();
    if( !pIrp->IsIrpHolder() )
    {
        IrpCtxPtr& pTopCtx = pIrp->GetTopStack();
        ret = pTopCtx->GetStatus();;
        pCtx->SetStatus( ret );
        pIrp->PopCtxStack();
    }

    EnumIoctlStat iState = reqStatInvalid;
    ret = CTcpStreamPdo::GetIoctlReqState(
        pIrp, iState );
    if( ERROR( ret ) )
        return ret;

    guint32 dwCtrlCode = pIrp->CtrlCode();
    switch( dwCtrlCode )
    {
    case CTRLCODE_INVALID_STREAM_ID_PDO:
    case CTRLCODE_OPEN_STREAM_PDO:
    case CTRLCODE_CLOSE_STREAM_PDO:
    case CTRLCODE_RMTSVR_OFFLINE_PDO:
    case CTRLCODE_RMTMOD_OFFLINE_PDO:
        {
            guint32 dwIoDir =
                pCtx->GetIoDirection();

            if( iState == reqStatOut &&
                dwIoDir == IRP_DIR_INOUT )
            {
                ret = CTcpStreamPdo::SetIoctlReqState(
                    pIrp, reqStatIn );

                if( ERROR( ret ) )
                    break;

                BufPtr pEmpty;

                pCtx->SetRespData( pEmpty );
                ret = QueueIrpForResp( pIrp );
                if( ERROR( ret ) )
                    break;

                if( !pCtx->m_pRespData.IsEmpty() &&
                    !pCtx->m_pRespData->empty() )
                {
                    // fantastic, the response has
                    // arrived
                    ret = pCtx->GetStatus();
                    iState = reqStatIn;
                }
                else
                {
                    // we need to continue waiting for
                    // the response
                    ret = STATUS_PENDING;
                }
            }
            else if( iState == reqStatOut &&
                dwIoDir == IRP_DIR_OUT )
            {
                // this should be a response
                CTcpStreamPdo::SetIoctlReqState(
                    pIrp, reqStatDone );

                if( dwCtrlCode ==
                    CTRLCODE_INVALID_STREAM_ID_PDO )
                {
                    ret = CompleteInvalStmIrp( pIrp );
                }
                else if( dwCtrlCode ==
                    CTRLCODE_RMTMOD_OFFLINE_PDO ||
                    dwCtrlCode ==
                    CTRLCODE_RMTSVR_OFFLINE_PDO )
                {
                    // do nothing
                }
                else if( dwCtrlCode ==
                    CTRLCODE_OPEN_STREAM_PDO ||
                    dwCtrlCode ==
                    CTRLCODE_CLOSE_STREAM_PDO )
                {
                    // this is the server side
                    // response
                    //
                    // do nothing
                }
            }

            if( iState == reqStatIn )
            {
                CTcpStreamPdo::SetIoctlReqState(
                    pIrp, reqStatDone );

                if( dwCtrlCode ==
                    CTRLCODE_OPEN_STREAM_PDO )
                {
                    // this is the proxy side
                    // received the response and
                    // about to complete
                    ret = CompleteOpenStmIrp( pIrp );
                }
                else if( dwCtrlCode ==
                    CTRLCODE_CLOSE_STREAM_PDO )
                {
                    ret = CompleteCloseStmIrp( pIrp );
                }
            }
            break;
        }
    case CTRLCODE_LISTENING:
        {
            ret = CTcpStreamPdo::SetIoctlReqState(
                pIrp, reqStatDone );
            break;
        }
    default:
        {
            ret = -ENOTSUP;
        }
    }

    if( iState == reqStatOut )
    {
        CStdRMutex oPortLock( GetLock() );
        Stm2Ptr pStm;
        gint32 iRet = GetStreamFromIrp(
            pIrp, pStm );

        if( SUCCEEDED( iRet ) )
            pStm->Refresh();

        SetSending( false );
        oPortLock.Unlock();

        StartSend( nullptr );
    }

    return ret;
}

gint32 CRpcNativeProtoFdo::CompleteFuncIrp(
    IRP* pIrp )
{

    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        if( pIrp->MajorCmd() != IRP_MJ_FUNC )
        {
            ret = -EINVAL;
            break;
        }

        IrpCtxPtr& pCtx = pIrp->GetCurCtx();
        ret = pCtx->GetStatus();
        if( ERROR( ret ) )
            break;

        switch( pCtx->GetMinorCmd() )
        {
        case IRP_MN_READ:
            {
                ret = CompleteReadIrp( pIrp );
                break;
            }
        case IRP_MN_WRITE:
            {
                ret = CompleteWriteIrp( pIrp );
                break;
            }
        case IRP_MN_IOCTL:
            {
                ret = CompleteIoctlIrp( pIrp );    
                break;
            }
        default:
            ret = -ENOTSUP;
            break;
        }

    }while( 0 );

    if( ret != STATUS_PENDING )
    {
        IrpCtxPtr& pCtx =
            pIrp->GetCurCtx();

        if( !pIrp->IsIrpHolder() )
        {
            IrpCtxPtr& pCtxLower = pIrp->GetCurCtx();

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

gint32 CRpcNativeProtoFdo::PostStart(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CCfgOpenerObj oPortCfg(
            ( CObjBase* )this );
        IConfigDb* pConnParams = nullptr;
        ret = oPortCfg.GetPointer(
            propConnParams, pConnParams );
        if( ERROR( ret ) )
            break;

        CConnParams oConn( pConnParams );
        m_bCompress = oConn.IsCompression();

        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        ret = pCtx->GetStatus();
        if( ERROR( ret ) )
            break;

        ret = AddStream( TCP_CONN_DEFAULT_STM,
            protoDBusRelay,
            TCP_CONN_DEFAULT_STM );

        if( ERROR( ret ) )
            break;
        
        ret = AddStream( TCP_CONN_DEFAULT_CMD,
            protoControl,
            TCP_CONN_DEFAULT_CMD );

    }while( 0 );

    return ret;
}

gint32 CRpcNativeProtoFdo::StartListeningTask()
{
    gint32 ret = 0;
    do{
        CIoManager* pMgr = GetIoMgr();
        CParamList oParams;
        oParams.SetPointer( propIoMgr, pMgr );
        oParams.SetPointer( propPortPtr, this );

        ret = m_pListeningTask.NewObj(
            clsid( CFdoListeningTask ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        pMgr->RescheduleTask( m_pListeningTask );

    }while( 0 );

    return ret;
}

gint32 CRpcNativeProtoFdo::StopListeningTask()
{
    CStdRMutex oPortLock( GetLock() );
    if( m_pListeningTask.IsEmpty() )
        return 0;

    TaskletPtr pTask = m_pListeningTask;
    m_pListeningTask.Clear();
    oPortLock.Unlock();

    ( *pTask )( eventCancelTask );
    return 0;
}

gint32 CRpcNativeProtoFdo::SetListeningTask(
    TaskletPtr& pTask )
{
    CStdRMutex oPortLock( GetLock() );

    guint32 dwPortState = GetPortState();
    if( dwPortState != PORT_STATE_BUSY_SHARED &&
        dwPortState != PORT_STATE_READY )
        return ERROR_STATE;

    m_pListeningTask = pTask;
    return 0;
}

gint32 CRpcNativeProtoFdo::OnQueryStop(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    if( true )
    {
        // query stop has be done once
        CStdRMutex oPortLock( GetLock() );
        if( m_bStopReady )
            return 0;
        m_bStopReady = true;
    }

    gint32 ret = 0;
    CParamList oParams;
    ret = oParams.SetPointer(
        propIoMgr, GetIoMgr() );

    ret = oParams.SetObjPtr(
        propPortPtr, ObjPtr( this ) );

    ret = oParams.SetObjPtr(
        propIrpPtr, ObjPtr( pIrp ) );

    ret = GetIoMgr()->ScheduleTask(
        clsid( CStmPdoSvrOfflineNotifyTask ),
        oParams.GetCfg() );

    if( ERROR( ret ) )
    {
        IrpCtxPtr& pCtx = pIrp->GetCurCtx();
        pCtx->SetStatus( ret );
    }
    else
    {
        ret = STATUS_PENDING;
    }
    return ret;
}

gint32 CRpcNativeProtoFdo::PreStop(
    IRP* pIrp )
{
    // remove all the pending irps. It is
    // preferable to do this here, to allow lower
    // port ( still ready ) to do wrapup work in a
    // clean environment
    StopListeningTask();
    CancelAllIrps( ERROR_PORT_STOPPED );
    return super::PreStop( pIrp );
}

gint32 CRpcNativeProtoFdo::CancelAllIrps(
    gint32 iErrno )
{
    gint32 ret = 0;

    CStdRMutex oPortLock( GetLock() );
    IrpVecPtr pIrpVec( true );
    CParamList oParams;
    std::vector< IrpPtr >& vecIrps =
        ( *pIrpVec )();

    for( auto&& oPair : m_mapStreams )
    {
        Stm2Ptr& pStm = oPair.second;
        pStm->GetAllIrpsQueued( vecIrps );
        pStm->ClearAllIrpsQueued();
    }

    CIoManager* pMgr = GetIoMgr();
    oPortLock.Unlock();

    while( !vecIrps.empty() )
    {
        ret = oParams.Push(
            *( guint32* )&iErrno );

        if( ERROR( ret ) )
            break;

        ObjPtr pObj;

        if( !vecIrps.empty() )
        {
            pObj = pIrpVec;
            ret = oParams.Push( pObj );
            if( ERROR( ret ) )
                break;
        }

        oParams.SetPointer( propIoMgr, pMgr );

        // NOTE: this is a double check of the
        // remaining IRPs before the STOP begins.
  
        TaskletPtr pTask;
        pTask.NewObj(
            clsid( CCancelIrpsTask ),
            oParams.GetCfg() );

        // NOTE: Assumes there is no pending
        // completion. And the irp's callback
        // is still yet to be called when this
        // method returns.
        ( *pTask )( eventZero );
        ret = pTask->GetError();
        break;
    }

    return ret;
}

gint32 CRpcNativeProtoFdo::Stop(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    std::vector< gint32 > vecStmIds;

    CStdRMutex oPortLock( GetLock() );
    for( auto elem : m_mapStreams )
        vecStmIds.push_back( elem.first );

    for( auto elem : vecStmIds )
        RemoveStream( elem );

    return 0;
}

gint32 CRpcNativeProtoFdo::AllocIrpCtxExt(
    IrpCtxPtr& pIrpCtx,
    void* pContext ) const
{
    gint32 ret = 0;
    switch( pIrpCtx->GetMajorCmd() )
    {
    case IRP_MJ_FUNC:
        {
            switch( pIrpCtx->GetMinorCmd() )
            {
            case IRP_MN_IOCTL:
                {
                    STMPDO_IRP_EXT oExt; 
                    guint32 dwCtrlCode =
                        pIrpCtx->GetCtrlCode();

                    switch( dwCtrlCode )
                    {
                    case CTRLCODE_OPEN_STREAM_PDO:
                    case CTRLCODE_CLOSE_STREAM_PDO:
                    case CTRLCODE_INVALID_STREAM_ID_PDO:
                        {
                            oExt.m_iState = reqStatOut;
                            break;
                        }
                    case CTRLCODE_LISTENING:
                        {
                            oExt.m_iState = reqStatIn;
                            break;
                        }
                    default:
                        ret = -ENOTSUP;
                        break;
                    }

                    if( ERROR( ret ) )
                        break;

                    BufPtr pBuf( true );
                    *pBuf = oExt;
                    pIrpCtx->SetExtBuf( pBuf );
                    break;
                }
            default:
                ret = -ENOTSUP;
                break;
            }
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

gint32 CRpcNativeProtoFdo::GetSeedStmId()
{
    gint32 ret = m_iStmCounter++;
    if( m_iStmCounter < 0 )
    {
        m_iStmCounter = STMSOCK_STMID_FLOOR;
    }
    return ret;
}

gint32 CRpcNativeProtoFdo::NewStreamId(
    gint32& iStream )
{
    iStream = GetSeedStmId();
    return 0;
}

gint32 CRpcNativeProtoFdo::OnReceive(
    gint32 iFd, BufPtr& pBuf )
{
    gint32 ret = 0;
    IrpPtr pIrp;
    guint32 dwOffsetBackup = 0;

    bool bEmptyBuf =
        ( pBuf.IsEmpty() || pBuf->empty() );

    if( ( iFd >= 0 && !bEmptyBuf ) ||
        ( iFd < 0 && bEmptyBuf ) )
        return -EINVAL;

    if( !bEmptyBuf )
        dwOffsetBackup = pBuf->offset();

    do{
        Stm2Ptr pStm;

        if( true )
        {
            CStdRMutex oPortLock( GetLock() );
            if( m_pPackReceiving.IsEmpty() )
            {
                ret = m_pPackReceiving.NewObj(
                    clsid( CIncomingPacket ) );

                if( ERROR( ret ) )
                    break;
            }

            CIncomingPacket* pIn =
                m_pPackReceiving;

            if( pIn == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            if( iFd >= 0 )
            {
                ret = pIn->FillPacket( iFd );
            }
            else if( !pBuf.IsEmpty() )
            {
                if( pBuf->empty() )
                    break;

                ret = pIn->FillPacket( pBuf );
            }
            else
            {
                ret = -EINVAL;
            }

            if( ret == STATUS_PENDING )
            {
                break;
            }
            else if( ERROR( ret ) )
            {
                // NOTE: we are entering a wrong
                // state cannot recover, we need to
                // close the stream
                m_pPackReceiving.Clear();
                break;
            }
            else if( SUCCEEDED( ret ) )
            do{
                gint32 iStmId = 0;
                pIn->GetPeerStmId( iStmId );

                gint32 iStmPeerId = 0;
                pIn->GetStreamId( iStmPeerId );

                guint32 dwProtoId = 0;
                pIn->GetProtoId( dwProtoId );

                guint32 dwSeqNo = 0;
                pIn->GetSeqNo( dwSeqNo );

                if( pIn->GetFlags() &
                    RPC_PACKET_FLG_COMPRESS )
                {
                    BufPtr pDecompressed;
                    BufPtr pBuf;
                    ret = pIn->GetPayload( pBuf );
                    if( ERROR( ret ) )
                        break;

                    ret = DecompressDataLZ4(
                        pBuf, pDecompressed );
                    if( ERROR( ret ) )
                        break;

                    pIn->SetPayload(
                        pDecompressed );
                }

                ret = GetStream( iStmId, pStm );
                if( ERROR( ret ) )
                {
                    // Fine, let's notify the
                    // caller the target stream
                    // does not exist
                    CParamList oParams;
                    ret = oParams.SetIntProp(
                        propStreamId, iStmId );

                    if( ERROR( ret ) )
                        break;

                    ret = oParams.SetIntProp(
                        propPeerStmId, iStmPeerId );

                    if( ERROR( ret ) )
                        break;

                    ret = oParams.SetIntProp(
                        propProtoId, dwProtoId );

                    if( ERROR( ret ) )
                        break;

                    ret = oParams.SetIntProp(
                        propSeqNo, dwSeqNo );

                    if( ERROR( ret ) )
                        break;
                    ret = oParams.SetPointer(
                        propIoMgr, GetIoMgr() );

                    if( ERROR( ret ) )
                        break;

                    ret = oParams.SetPointer(
                        propPortPtr, this );

                    if( ERROR( ret ) )
                        break;

                    GetIoMgr()->ScheduleTask(
                        clsid( CStmSockInvalStmNotifyTask ),
                        oParams.GetCfg() );

                    break;
                }

                gint32 iRemoteId;
                pStm->GetPeerStmId( iRemoteId );
                if( iRemoteId != iStmPeerId )
                {
                    // wrong destination
                    ret = -EINVAL;
                    break;
                }

                ret = pStm->RecvPacket(
                    m_pPackReceiving );

                m_pPackReceiving.Clear();
                if( ERROR( ret ) )
                    break;

            }while( 0 );

            if( ERROR( ret ) )
                m_pPackReceiving.Clear();
        }

        if( pStm.IsEmpty() )
            continue;

        pStm->Refresh();
        pStm->DispatchDataIncoming();

    }while( 1 );

    if( ERROR( ret ) )
    {
        DebugPrint( ret,
            "Error receiving packets" );
        m_pPackReceiving.Clear();
    }

    if( pBuf->size() > 0 )
    {
        if( SUCCEEDED( ret ) )
            return -EFAULT;
    }

    // recover the original offset
    if( !pBuf.IsEmpty() )
        pBuf->SetOffset( dwOffsetBackup );

    return ret;
}

gint32 CRpcNativeProtoFdo::QueueIrpForResp(
    IRP* pIrp )
{
    if( pIrp == nullptr
        || pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        Stm2Ptr pStm;
        CStdRMutex oPortLock( GetLock() );
        ret = GetCtrlStmFromIrp( pIrp, pStm );
        if( ERROR( ret ) )
            break;

        CRpcControlStream2* pCtrlStm = pStm;
        if( pCtrlStm == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        ret = pCtrlStm->QueueIrpForResp( pIrp );

    }while( 0 );

    return ret;
}

gint32 CRpcNativeProtoFdo::SubmitIoctlCmd(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        ret = HandleIoctlIrp( pIrp );

        if( ERROR( ret ) )
            break;

        if( IsImmediateReq( pIrp ) )
            break;

        IrpCtxPtr pCtx = pIrp->GetCurCtx();
        if( pCtx->GetIoDirection() == IRP_DIR_IN )
            break;

        // start to send the queued buffer
        ret = StartSend( pIrp );

    }while( 0 );

    return ret;
}

gint32 CRpcNativeProtoFdo::CancelFuncIrp(
    IRP* pIrp, bool bForce )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CStdRMutex oPortLock( GetLock() );
        RemoveIrpFromMap( pIrp );

    }while( 0 );

    if( true )
    {
        IrpCtxPtr& pCtx = pIrp->GetCurCtx();
        pCtx->SetStatus( ERROR_CANCEL );
        if( pIrp->MinorCmd() == IRP_MN_READ )
        {
            CfgPtr pCfg;
            ret = pCtx->GetReqAsCfg( pCfg );
            if( ERROR( ret ) )
                return ret;

            CCfgOpener oCfg(
                ( IConfigDb* )pCfg );

            gint32 iStreamId = 0;
            ret = oCfg.GetIntProp(
                0, ( guint32& )iStreamId );

            if( SUCCEEDED( ret ) )
            {
                // remove the callback on
                // stream iStreamId 
                pIrp->RemoveCallback();
            }
        }
        super::CancelFuncIrp( pIrp, bForce );
    }

    return ret;
}

gint32 CFdoListeningTask::HandleIrpResp(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CRpcNativeProtoFdo* pPort = nullptr;
        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );

        ret = oCfg.GetPointer(
            propPortPtr, pPort );

        if( ERROR( ret ) )
            break;

        gint32 iRet = pIrp->GetStatus();
        if( ERROR( iRet ) )
        {
            ret = iRet;
            break;
        }

        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        BufPtr pRespBuf = pCtx->m_pRespData;
        if( pRespBuf.IsEmpty() ||
            pRespBuf->empty() )
        {
            ret = -EFAULT;
            break;
        }

        if( pRespBuf->size() <
            sizeof( STREAM_SOCK_EVENT ) )
        {
            ret = -EINVAL;
            break;
        }

        STREAM_SOCK_EVENT* pEvt =
            ( STREAM_SOCK_EVENT* )pRespBuf->ptr();

        switch( pEvt->m_iEvent )
        {
        case sseRetWithFd:
            {
                BufPtr pNullBuf;
                ret = pPort->OnReceive(
                    pEvt->m_iData, pNullBuf );
                break;
            }
        case sseRetWithBuf:
            {
                ret = pPort->OnReceive(
                    -1, pEvt->m_pInBuf );
                pEvt->m_pInBuf.Clear();
                break;
            }
        case sseError:
            {
                ret = pEvt->m_iData;
                if( ret == -ENOTCONN ||
                    ERROR( ret ) )
                {
                    IPort* pdo =
                        pPort->GetBottomPort();
                    FireRmtSvrEvent(
                        pdo, eventRmtSvrOffline );
                }
                break;
            }
        default:
            {
                ret = -ENOTSUP;
                break;
            }
        }

    }while( 0 );

    return ret;
}

gint32 CFdoListeningTask::RunTask()
{
    gint32 ret = 0;
    do{
        CRpcNativeProtoFdo* pPort = nullptr;
        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );

        ret = oCfg.GetPointer(
            propPortPtr, pPort );

        if( ERROR( ret ) )
            break;

        IPort* pLowerPort =
            pPort->GetLowerPort();

        if( pLowerPort == nullptr )
            break;

        IrpPtr pIrp( true );
        pIrp->AllocNextStack( pLowerPort );

        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        pCtx->SetMajorCmd( IRP_MJ_FUNC );
        pCtx->SetMinorCmd( IRP_MN_IOCTL );
        pCtx->SetCtrlCode( CTRLCODE_LISTENING );

        CIoManager* pMgr = pPort->GetIoMgr();
        oCfg.SetPointer(
            propIrpPtr, ( PIRP )pIrp );

        pIrp->SetCallback( this, 0 );
        pIrp->SetIrpThread( pMgr );

        ret = pMgr->SubmitIrpInternal(
            pLowerPort, pIrp, false );

        if( ret == STATUS_PENDING )
            break;

        if( ERROR( ret ) )
        {
            DebugPrint( ret,
                "Error occurs in CFdoListeningTask" );
        }

        oCfg.RemoveProperty( propIrpPtr );
        ret = HandleIrpResp( pIrp );
        ret = OnTaskComplete( ret );

    }while( 0 );

    return ret;
}

gint32 CFdoListeningTask::OnTaskComplete(
    gint32 iRet )
{
    gint32 ret = 0;

    IConfigDb* pCfg = GetConfig();
    CCfgOpener oCfg( pCfg );

    CRpcNativeProtoFdo* pPort = nullptr;
    ret = oCfg.GetPointer(
        propPortPtr, pPort );
    if( ERROR( ret ) )
        return ret;

    if( ERROR( iRet ) )
    {
        if( iRet == ERROR_PORT_STOPPED )
            return iRet;

        DebugPrint( iRet,
            "CFdoListeningTask encounters error..." );

        if( iRet == -EPROTO )
        {
            // close the connection immediately
            // and the next listening will receive
            // the error code and fire the offline
            // event
            IrpPtr pIrp( true );
            pIrp->AllocNextStack( pPort );
            IrpCtxPtr& pCtx = pIrp->GetTopStack();
            pCtx->SetMajorCmd( IRP_MJ_FUNC );
            pCtx->SetMinorCmd( IRP_MN_IOCTL );
            pCtx->SetCtrlCode(
                CTRLCODE_RESET_CONNECTION );

            CIoManager* pMgr = pPort->GetIoMgr();
            pMgr->SubmitIrpInternal(
                pPort, pIrp, false );
        }
        else
        {
            return iRet;
        }
    }

    do{
        CParamList oParams;
        CIoManager* pMgr = pPort->GetIoMgr();
        oParams.SetPointer( propIoMgr, pMgr );
        oParams.SetPointer( propPortPtr, pPort );

        TaskletPtr pTask;
        ret = pTask.NewObj(
            clsid( CFdoListeningTask ),
            oParams.GetCfg() );
        if( ERROR( ret ) )
            break;

        ret = pPort->SetListeningTask( pTask );
        if( ERROR( ret ) )
            break;

        ret = pMgr->RescheduleTask( pTask );

    }while( 0 );

    oCfg.RemoveProperty( propPortPtr );

    return ret;
}

gint32 CFdoListeningTask::OnIrpComplete(
    IRP* pIrp )
{
    gint32 ret = 0;

    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    do{
        IConfigDb* pCfg = GetConfig();
        CCfgOpener oCfg( pCfg );

        ret = HandleIrpResp( pIrp );
        oCfg.RemoveProperty( propIrpPtr );
        ret = OnTaskComplete( ret );

    }while( 0 );

    return ret;
}

CRpcNatProtoFdoDrv::CRpcNatProtoFdoDrv(
    const IConfigDb* pCfg )
    : super( pCfg )
{ SetClassId( clsid( CRpcNatProtoFdoDrv ) ); }

gint32 CRpcNatProtoFdoDrv::Probe(
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
        std::string strClass =
            PORT_CLASS_TCP_STREAM_PDO2;

        PortPtr pPdoPort; 
        ret = ( ( CPort* )pLowerPort )->
            GetPdoPort( pPdoPort );
        if( ERROR( ret ) )
            break;

        CPort* pPdo = pPdoPort;
        CCfgOpenerObj oCfg( pPdo );
        ret = oCfg.GetStrProp(
            propPortClass, strPdoClass );

        if( ERROR( ret ) )
            break;

        if( strClass != strPdoClass )
        {
            // this is not a port we support
            ret = -ENOTSUP;
            break;
        }

        CParamList oNewCfg;
        oNewCfg[ propPortClass ] =
            PORT_CLASS_RPC_NATPROTO_FDO;

        guint32 dwPortId = NewPortId();
        oNewCfg.SetIntProp(
            propPortId, dwPortId );

        ret = oNewCfg.CopyProp( propConnParams,
            ( CObjBase* )pPdoPort );

        if( ERROR( ret ) )
            break;

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

