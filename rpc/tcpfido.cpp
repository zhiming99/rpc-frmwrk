/*
 * =====================================================================================
 *
 *       Filename:  tcpfido.cpp
 *
 *    Description:  implementation of the CRpcTcpFido class and the related functionalities
 *
 *        Version:  1.0
 *        Created:  06/11/2017 05:23:35 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
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
#include "emaphelp.h"
#include "reqopen.h"
#include <byteswap.h>

namespace rpcf
{

using namespace std;

#define BRIDGE_NAME \
({ \
    std::string strVal; \
    if( m_bAuth ) \
        strVal = OBJNAME_TCP_BRIDGE_AUTH; \
    else \
        strVal = OBJNAME_TCP_BRIDGE; \
    strVal; \
})

extern CConnParamsProxy GetConnParams(
    const CObjBase* pObj );

std::atomic< guint32 > CRpcTcpFido::m_atmSeqNo(
    ( guint32 )GetRandom() );

static gint32 GetBdgeIrpStmId(
    PIRP pIrp, gint32& iStmId )
{
    if( pIrp == nullptr )
        return -EINVAL;

    IrpCtxPtr& pCtx = pIrp->GetTopStack();
    BufPtr pExtBuf;
    pCtx->GetExtBuf( pExtBuf );
    if( pExtBuf.IsEmpty() )
        return -EINVAL;

    FIDO_IRP_EXT* pExt =
        ( FIDO_IRP_EXT* )pExtBuf->ptr();    

    iStmId = pExt->iStmId;
    return 0;
}

gint32 SetBdgeIrpStmId( 
    PIRP pIrp, gint32 iStmId )
{
    if( pIrp == nullptr )
        return -EINVAL;

    if( iStmId < 0 )
        return -EINVAL;

    IrpCtxPtr& pCtx = pIrp->GetTopStack();
    BufPtr pExtBuf;
    pCtx->GetExtBuf( pExtBuf );
    if( pExtBuf.IsEmpty() )
        return -EINVAL;

    FIDO_IRP_EXT* pExt =
        ( FIDO_IRP_EXT* )pExtBuf->ptr();    

    pExt->iStmId = iStmId;
    return 0;
}

CRpcTcpFido::CRpcTcpFido(
    const IConfigDb* pCfg  )
    : super( pCfg )
{
    gint32 ret = 0;

    SetClassId( clsid( CRpcTcpFido ) );
    m_dwFlags &= ~PORTFLG_TYPE_MASK;

    // actually this port serves an fdo function,
    // not a filter anymore
    m_dwFlags |= PORTFLG_TYPE_FDO;

    do{
        timespec ts;
        ret = clock_gettime( CLOCK_MONOTONIC, &ts );
        if( ret == -1 )
        {
            ret = -errno;
            break;
        }

        m_atmSeqNo = bswap_32( ts.tv_nsec );

    }while( 0 );

    if( ERROR( ret ) )
    {
        string strMsg = DebugMsg(
            ret, "Error in CRpcTcpFido's ctor" );

        throw runtime_error( strMsg );
    }
}

CRpcTcpFido::~CRpcTcpFido()
{
}

gint32 CRpcTcpFido::OnSubmitIrp(
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
        case IRP_MN_WRITE:
            {
                // direct stream access
                ret = SubmitReadWriteCmd( pIrp );
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

gint32 CRpcTcpFido::SubmitReadWriteCmd(
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

    ret = HandleReadWriteReq( pIrp );

    if( ret != STATUS_PENDING )
        pCtx->SetStatus( ret );

    return ret;
}

gint32 CRpcTcpFido::SubmitIoctlCmd(
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
                // server side I/O
                //
                // the m_pReqData contains a
                // pointer to DMsgPtr
                ret = HandleSendResp( pIrp );
                break;
            }
        case CTRLCODE_SEND_EVENT:
            {
                // server side I/O
                //
                // the m_pReqData contains a
                // pointer to DMsgPtr
                ret = HandleSendEvent( pIrp );
                break;
            }
        case CTRLCODE_SEND_REQ:
            {
                // the m_pReqData contains a
                // pointer to DMsgPtr
                ret = HandleSendReq( pIrp );
                break;
            }
        case CTRLCODE_REG_MATCH:
        case CTRLCODE_UNREG_MATCH:
            {
                // succeed the
                // EnableEvent/DisableEvent
                // immediately from the interface
                ret = HandleRegMatch( pIrp );
                break;
            }
        case CTRLCODE_LISTENING:
            {
                if( m_pListenTask.IsEmpty() )
                {
                    CParamList oCfg;
                    ret = oCfg.SetPointer(
                        propIoMgr, GetIoMgr() );

                    if( ERROR( ret ) )
                        break;

                    ret = oCfg.SetObjPtr(
                        propPortPtr, ObjPtr( this ) );

                    if( ERROR( ret ) )
                        break;

                    ret = m_pListenTask.NewObj(
                        clsid( CTcpFidoListenTask ),
                        oCfg.GetCfg() );

                    if( ERROR( ret ) )
                        break;

                    ret = GetIoMgr()->RescheduleTask(
                        m_pListenTask );

                    if( ERROR( ret ) )
                        break;
                }
                ret = super::HandleListening( pIrp );
                break;
            }
        case CTRLCODE_SEND_DATA:
        case CTRLCODE_FETCH_DATA:
            {
                ret = HandleSendData( pIrp );
                break;
            }
        case CTRLCODE_LISTENING_FDO:
            {
                ret = HandleListening( pIrp );
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
        pCtx->SetStatus( ret );

    return ret;
}

gint32 CRpcTcpFido::HandleSendEvent(
    IRP* pIrp )
{
    return HandleSendResp( pIrp );
}

gint32 CRpcTcpFido::HandleSendResp(
    IRP* pIrp )
{
    gint32 ret = 0;
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
    {
        ret = -EINVAL;
        return ret;
    }

    do{

        // let's process the func irps
        IrpCtxPtr pCtx = pIrp->GetCurCtx();
        guint32 dwIoDir = pCtx->GetIoDirection();

        if( dwIoDir != IRP_DIR_OUT )
        {
            ret = -EINVAL;
            break;
        }
        ret = HandleSendReq( pIrp );
    }while( 0 );

    return ret;
}

gint32 CRpcTcpFido::HandleSendReq(
    IRP* pIrp )
{
    // the irp carry a buffer to send, it is
    // assumed to be a DMsgPtr
    gint32 ret = 0;
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
    {
        ret = -EINVAL;
        return ret;
    }

    do{
        // let's process the func irps
        IrpCtxPtr pCtx = pIrp->GetCurCtx();
        guint32 dwIoDir = pCtx->GetIoDirection();

        if( dwIoDir == IRP_DIR_IN )
        {
            // a listening request
            ret = -EINVAL;
            break;
        }

        BufPtr pBuf( true );
        DMsgPtr& pReq = *pCtx->m_pReqData;
        if( pReq.IsEmpty() )
        {
            ret = -EINVAL;
            break;
        }

        guint32 dwSerial = 0; 
        guint32 dwOldSerial = 0;
        guint32 dwCtrlCode = pCtx->GetCtrlCode();

        dwSerial = m_atmSeqNo++;
        if( dwSerial == 0 )
            dwSerial = 1;

        if( dwCtrlCode == CTRLCODE_SEND_REQ )
        {
            ret = pReq.GetSerial( dwOldSerial );
            if( dwOldSerial == 0 )
                pReq.SetSerial( dwSerial );
        }
        else
        {
            pReq.SetSerial( dwSerial );
        }

        ret = pReq.Serialize( pBuf );
        if( ERROR( ret ) )
            break;

        // restore the old one
        if( dwCtrlCode == CTRLCODE_SEND_REQ )
            pReq.SetSerial( dwOldSerial );

        IPort* pLowerPort = GetLowerPort();
        if( pLowerPort == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        ret = pIrp->AllocNextStack( pLowerPort );
        if( ERROR( ret ) )
            break;

        CParamList oParams;
        // Default stream for ioctl commands
        // transport
        oParams.Push( pBuf );
        // keep the old serial for response
        // message
        gint32 iStmId = TCP_CONN_DEFAULT_STM;
        GetBdgeIrpStmId( pIrp, iStmId );
        oParams.SetIntProp( propStreamId, iStmId );

        ret = oParams.SetIntProp(
            propSeqNo2, dwSerial );

        BufPtr pBufNew( true );
        *pBufNew = ObjPtr( oParams.GetCfg() );
        IrpCtxPtr pNewCtx = pIrp->GetTopStack();
        pNewCtx->SetMajorCmd( IRP_MJ_FUNC );
        pNewCtx->SetMinorCmd( IRP_MN_WRITE );
        pNewCtx->SetIoDirection( IRP_DIR_OUT );
        pNewCtx->SetReqData( pBufNew );

        ret = pLowerPort->SubmitIrp( pIrp );
        if(  ret == STATUS_PENDING )
            break;

        if( dwIoDir == IRP_DIR_OUT )
        {
            pCtx->SetStatus( 
                pNewCtx->GetStatus() );
        }

        // the response is yet to come
        pIrp->PopCtxStack();

        if( ERROR( ret ) )
            break;

        // move the irp to the waiting queue
        if( dwSerial != 0 &&
            dwIoDir == IRP_DIR_INOUT )
        {
            CStdRMutex oPortLock( GetLock() );
            std::hashmap< guint32, DMsgPtr >::iterator
                itr = m_mapRespMsgs.find( dwSerial );
            if( itr == m_mapRespMsgs.end() )
            {
                m_mapSerial2Resp[ dwSerial ] = pIrp;
                ret = STATUS_PENDING;
            }
            else
            {
                // the message arrives before
                // the irp arrives here
                DebugPrint( ret, "respmsg arrives before reqmsg" );
                DMsgPtr pRespMsg = itr->second;
                m_mapRespMsgs.erase( itr );

                pRespMsg.SetReplySerial( dwOldSerial );

                BufPtr pRespBuf( true );
                *pRespBuf = pRespMsg;
                pCtx->SetRespData( pRespBuf );
                pCtx->SetStatus( ret );
            }
        }
    }while( 0 );

    return ret;
}

gint32 CFidoRecvDataTask::Process(
    guint32 dwContext ) 
{
    gint32 ret = 0;
    do{
        CParamList oParams( m_pCtx );
        ObjPtr pObj;

        ret = oParams.Pop( pObj );
        if( ERROR( ret ) )
            break;

        BufVecPtr pVecBuf;
        pVecBuf = pObj;
        if( pVecBuf.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        ret = oParams.GetObjPtr(
            propPortPtr, pObj );

        if( ERROR( ret ) )
            break;

        CRpcTcpFido* pPort = pObj;
        if( pPort == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        for( auto&& oElem : ( *pVecBuf )() )
            pPort->DispatchData( oElem );

    }while( 0 );

    return ret;
}

gint32 CRpcTcpFido::HandleListening(
    IRP* pIrp )
{

    // the irp carry a buffer to receive, it is
    // assumed to be a DMsgPtr
    gint32 ret = 0;
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
    {
        ret = -EINVAL;
        return ret;
    }

    do{
        // let's process the func irps
        IrpCtxPtr pCtx = pIrp->GetCurCtx();
        guint32 dwIoDir = pCtx->GetIoDirection();

        if( dwIoDir != IRP_DIR_IN )
        {
            // a listening request
            ret = -EINVAL;
            break;
        }

        IPort* pLowerPort = GetLowerPort();
        if( pLowerPort == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ret = pIrp->AllocNextStack( pLowerPort );
        if( ERROR( ret ) )
            break;

        IrpCtxPtr pNewCtx = pIrp->GetTopStack();

        pNewCtx->SetMajorCmd( IRP_MJ_FUNC );
        pNewCtx->SetMinorCmd( IRP_MN_READ );
        pNewCtx->SetIoDirection( IRP_DIR_IN );
        pLowerPort->AllocIrpCtxExt( pNewCtx );

        if( true )
        {
            // set the stream id to read from
            gint32 iStmId = TCP_CONN_DEFAULT_STM;
            GetBdgeIrpStmId( pIrp, iStmId );
            CCfgOpener oReadReq;
            oReadReq.SetIntProp(
                propStreamId, iStmId );

            BufPtr pBuf( true );
            *pBuf = ObjPtr( oReadReq.GetCfg() );

            pNewCtx->SetReqData( pBuf );
        }

        ret = pLowerPort->SubmitIrp( pIrp );
        if(  ret == STATUS_PENDING )
            break;

        if( ERROR( ret ) )
        {
            pCtx->SetStatus( ret );
            break;
        }

        pCtx->SetRespData(
            pNewCtx->m_pRespData );

        pCtx->SetStatus( ret );
        pIrp->PopCtxStack();

    }while( 0 );

    return ret;
}

gint32 CRpcTcpFido::ScheduleRecvDataTask(
    BufVecPtr& pVecBuf )
{
    if( pVecBuf.IsEmpty() )
        return 0;

    gint32 ret = 0;
    if( ( *pVecBuf )().empty() )
        return 0;

    // DebugPrint( 0, "probe: Schedule dispatch" );
    do{
        // schedule a task to dispatch the data
        CParamList oParams;
        ObjPtr pObj;
        pObj = pVecBuf;
        oParams.Push( pObj );
        oParams.SetPointer( propIoMgr, GetIoMgr() );

        oParams.SetObjPtr(
            propPortPtr, ObjPtr( this ) );

        TaskletPtr pTask;
        ret = pTask.NewObj( 
            clsid( CFidoRecvDataTask ),
            oParams.GetCfg() );
        if( ERROR( ret ) )
            break;

        ( *pTask )( eventZero );

    }while( 0 );

    return ret;
}

gint32 CRpcTcpFido::CompleteListening(
    IRP* pIrp )
{

    // the irp carry a buffer to send, it is
    // assumed to be a DMsgPtr
    gint32 ret = 0;
    if( pIrp == nullptr ||
        pIrp->GetStackSize() < 2 )
    {
        ret = -EINVAL;
        return ret;
    }

    // this requests will never complete till
    // error occurs
    do{
        // let's process the func irps
        IrpCtxPtr pCtx = pIrp->GetCurCtx();
        IrpCtxPtr pTopCtx = pIrp->GetTopStack();

        ret = pTopCtx->GetStatus();
        if( ERROR( ret ) )
        {
            if( ret != -EAGAIN  )
            {
                pCtx->SetStatus( ret );
                break;
            }
        }
        else
        {
            BufPtr pRespData =
                pTopCtx->m_pRespData;

            pCtx->SetRespData( pRespData );
            pCtx->SetStatus( ret );
        }
        pIrp->PopCtxStack();

    }while( 0 );

    return ret;
}

gint32 CRpcTcpFido::CompleteSendReq(
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
        IrpCtxPtr pCtx = pIrp->GetCurCtx();
        guint32 dwIoDir = pCtx->GetIoDirection();

        if( !pIrp->IsIrpHolder() )
        {
            IrpCtxPtr pTopCtx = pIrp->GetTopStack();
            if( pTopCtx == pCtx )
            {
                // unknown situation
                ret = -ERROR_FAIL;
                break;
            }

            ret = pTopCtx->GetStatus();
            pCtx->SetStatus( ret );
            pIrp->PopCtxStack();

            // well, we are done
            if( dwIoDir != IRP_DIR_INOUT )
                break;

            if( ERROR( ret ) )
                break;

            ObjPtr& pReq = *pTopCtx->m_pReqData;
            if( pReq.IsEmpty() )
            {
                ret = -EINVAL;
                break;
            }
            CfgPtr pCfg;
            pCfg = pReq;
            if( pCfg.IsEmpty() )
            {
                ret = -EFAULT;
                break;
            }

            guint32 dwSerial = 0;
            CCfgOpener oCfg( ( IConfigDb* )pCfg );

            ret = oCfg.GetIntProp(
                propSeqNo2, dwSerial );

            if( ERROR( ret ) )
                break;

            if( dwSerial != 0 )
            {
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
                    std::hashmap< guint32, DMsgPtr >::iterator
                        itr = m_mapRespMsgs.find( dwSerial );
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
            }
            break;
        }
        else
        {
            ret = pCtx->GetStatus();
            // the response data 
            if( ERROR( ret ) )
                break;

            do{
                if( dwIoDir != IRP_DIR_INOUT )
                {
                    // the response and event request
                    // goes here
                    // ret = -EFAULT;
                    break;
                }
                BufPtr& pBuf = pCtx->m_pRespData;
                if( pBuf.IsEmpty() )
                {
                    ret = -ENODATA;
                    pCtx->SetStatus( ret );
                    break;
                }

                DMsgPtr pMsg = *pBuf;
                DMsgPtr& pReqMsg = *pCtx->m_pReqData;

                guint32 dwOldSeqNo = 0;

                ret = pReqMsg.GetSerial( dwOldSeqNo );
                if( SUCCEEDED( ret ) )
                {
                    if( dwOldSeqNo != 0 )
                    {
                        // recover the old seq number
                        pMsg.SetReplySerial( dwOldSeqNo );
                    }
                }
                BufPtr pRespBuf( true );
                *pRespBuf = pMsg;
                pCtx->SetRespData( pRespBuf );

            }while( 0 );
        }

    }while( 0 );

    return ret;
}

gint32 CRpcTcpFido::LoopbackTest(
    DMsgPtr& pReqMsg )
{
    gint32 ret = 0;
    do{
        DMsgPtr pRespMsg;
        if( pReqMsg.GetType() !=
            DBUS_MESSAGE_TYPE_METHOD_CALL )
        {
            DebugPrint( 0, "unexpected messages received:\n"
            "%s", pReqMsg.DumpMsg().c_str() );
            return 0;
        }

        ret = pRespMsg.NewResp( pReqMsg );

        if( ERROR( ret ) )
            break;

        CParamList oParams;
        oParams[ propReturnValue ] = 0;

        BufPtr pBuf( true );
        ret = oParams.GetCfg()->Serialize( *pBuf );

        const char* pData = pBuf->ptr();
        dbus_message_append_args( pRespMsg,
            DBUS_TYPE_UINT32, &ret,
            DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
            &pData, pBuf->size(),
            DBUS_TYPE_INVALID );

        IrpPtr pIrp( true );
        pIrp->AllocNextStack( this );

        IrpCtxPtr pCtx = pIrp->GetTopStack();
        BufPtr pReqBuf( true );
        *pReqBuf = pRespMsg;

        pCtx->SetMajorCmd( IRP_MJ_FUNC );
        pCtx->SetMinorCmd( IRP_MN_IOCTL );
        pCtx->SetCtrlCode( CTRLCODE_SEND_RESP );

        // the stream id to listen to is in.
        this->AllocIrpCtxExt( pCtx );

        pCtx->SetReqData( pReqBuf );

        pCtx->SetIoDirection( IRP_DIR_OUT ); 
        pIrp->SetSyncCall( false );

        // NOTE: no timer here
        //
        // set a callback
        TaskletPtr pTask;
        pTask.NewObj( clsid( CIfDummyTask ) );
        pIrp->SetCallback( pTask, 0 );

        ret = GetIoMgr()->SubmitIrp(
            PortToHandle( this ),
            pIrp, false );

    }while( 0 );

    return ret;
}

gint32 CRpcTcpFido::DispatchData(
    CBuffer* pData )
{
    if( pData == nullptr )
        return -EINVAL;

    DMsgPtr pMsg;
    gint32 ret = pMsg.Deserialize( pData );
    if( ERROR( ret ) )
        return ret;

    BufPtr pBuf( true );
    *pBuf = pMsg;

#ifdef LOOP_TEST
    return LoopbackTest( pMsg );
#else
    return super::DispatchData( pBuf );
#endif
}

gint32 CRpcTcpFido::HandleReadWriteReq(
    IRP* pIrp )
{
    // the caller need to make sure the m_pReqData
    // contains a cfgptr with
    //
    // propStreamId: the stream id ( for IRP_MN_READ and
    // IRP_MN_WRITE )
    //
    // 0: the payload ( for IRP_MN_WRITE )
    //
    gint32 ret = -EINVAL;
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
    {
        return ret;
    }

    do{
        // let's process the func irps
        IrpCtxPtr pCtx = pIrp->GetCurCtx();
        guint32 dwIoDir = pCtx->GetIoDirection();

        if( dwIoDir != IRP_DIR_IN &&
            dwIoDir != IRP_DIR_OUT )
        {
            ret = -EINVAL;
            break;
        }

        IPort* pPort = GetLowerPort();

        if( pPort == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        // pass through
        ret = pIrp->AllocNextStack(
            pPort, IOSTACK_ALLOC_COPY );

        if( ERROR( ret ) )
            break;

        ret = pPort->SubmitIrp( pIrp );
        if( ret == STATUS_PENDING )
            break;

        IrpCtxPtr pTopCtx =
            pIrp->GetTopStack();

        ret = pTopCtx->GetStatus();
        pCtx->SetStatus( ret );

        if( ERROR( ret ) )
        {
            pIrp->PopCtxStack();
            break;
        }

        dwIoDir =
            pTopCtx->GetIoDirection();

        if( dwIoDir == IRP_DIR_IN &&
            SUCCEEDED( ret ) )
        {
            pCtx->SetRespData(
                pTopCtx->m_pRespData );
        }
        pIrp->PopCtxStack();

    }while( 0 );

    return ret;
}

gint32 CRpcTcpFido::GetPeerStmId(
    gint32 iStmId, gint32& iPeerId )
{
    gint32 ret = 0;
    if( IsReserveStm( iStmId ) )
    {
        iPeerId = iStmId;
        return ret;
    }

    do{
        IPort* pLowerPort = GetLowerPort();
        if( pLowerPort == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        IrpPtr pIrp( true );
        ret = pIrp->AllocNextStack( pLowerPort );
        if( ERROR( ret ) )
            break;

        IrpCtxPtr pNewCtx = pIrp->GetTopStack();
        pNewCtx->SetMajorCmd( IRP_MJ_FUNC );
        pNewCtx->SetMinorCmd( IRP_MN_IOCTL );
        pNewCtx->SetCtrlCode( CTRLCODE_GET_RMT_STMID );
        pNewCtx->SetIoDirection( IRP_DIR_INOUT );

        CParamList oParams;
        oParams.Push( iStmId );
        BufPtr pBuf( true );
        *pBuf = ObjPtr( oParams.GetCfg() );
        pNewCtx->SetReqData( pBuf );

        pLowerPort->AllocIrpCtxExt( pNewCtx );
        ret = pLowerPort->SubmitIrp( pIrp );
        if( ERROR( ret ) )
            break;

        CfgPtr pResp;
        ret = pNewCtx->GetRespAsCfg( pResp );
        if( ERROR( ret ) )
            break;

        CParamList oResp( ( IConfigDb* )pResp );
        guint32 *pdwPeerId = ( guint32* )&iPeerId;
        ret = oResp.GetIntProp( 0, *pdwPeerId );

    }while( 0 );

    return ret;
}

gint32 CRpcTcpFido::BuildSendDataMsg(
    IRP* pIrp, DMsgPtr& pMsg )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    if( !pMsg.IsEmpty() )
        pMsg.Clear();

    gint32 ret = 0;
    IrpCtxPtr pCtx = pIrp->GetCurCtx();

    do{
        // there is a CfgPtr in the buffer
        ObjPtr& pObj = *pCtx->m_pReqData; 
        CfgPtr pCfg = pObj;
        if( pCfg.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        CReqOpener oParams( ( IConfigDb* )pCfg );
        guint32 dwCallFlags = 0;

        ret = oParams.GetCallFlags( dwCallFlags );
        if( ERROR( ret ) )
            break;

        guint32 dwMsgType =
            dwCallFlags & CF_MESSAGE_TYPE_MASK;

        if( dwMsgType
            != DBUS_MESSAGE_TYPE_METHOD_CALL )
        {
            ret = -ENOTSUP;
            break;
        }

        oParams.SetBoolProp( propNonFd, true );

        gint32 iStmId = 0;
        guint32* pdwStmId = ( guint32* )&iStmId;
        ret = oParams.GetIntProp( 1, *pdwStmId );
        if( ERROR( ret ) )
            break;

        gint32 iPeerId = 0;
        ret = GetPeerStmId( iStmId, iPeerId );
        if( ERROR( ret ) )
            break;

        // the pDataDesc will be removed after
        // BuildSendDataReq.
        IConfigDb* pDataDesc = nullptr;
        ret = oParams.GetPointer( 0, pDataDesc );
        if( ERROR( ret ) )
            break;

        string strIfName;
        CCfgOpener oDataDesc( pDataDesc );
        ret = oDataDesc.GetStrProp(
            propIfName, strIfName );
        if( ERROR( ret ) )
            break;

        string strRtName;
        GetIoMgr()->GetRouterName( strRtName );

        string strDest = DBUS_DESTINATION2( 
            strRtName, BRIDGE_NAME );

        pMsg.SetDestination( strDest );
        ret = oParams.SetIntProp(
            1, ( guint32 )iPeerId );

        ret = BuildSendDataReq( pCfg, pMsg );
        if( ERROR( ret ) )
            break;

        string strPath = DBUS_OBJ_PATH(
            strRtName, BRIDGE_NAME );

        pMsg.SetPath( strPath );
        pMsg.SetInterface( strIfName );

    }while( 0 );

    return ret;
}

gint32 CRpcTcpFido::HandleSendData(
    IRP* pIrp )
{
    if( pIrp == nullptr
        || pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;

    do{
         
        DMsgPtr pMsg;

        ret = BuildSendDataMsg( pIrp, pMsg );
        if( ERROR( ret ) )
            break;

        IrpCtxPtr pCtx = pIrp->GetCurCtx();
        BufPtr pBuf( true );
        *pBuf = pMsg;

        // NOTE: We have overwritten the incoming
        // parameter with a new buffer ptr
        pCtx->SetReqData( pBuf );

        ret = HandleSendReq( pIrp );
        if( ret == STATUS_PENDING )
            break;

        if( ERROR( ret ) )
            break;

    }while( 0 );

    return ret;
}

gint32 CRpcTcpFido::CompleteReadWriteReq(
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
        IrpCtxPtr pCtx = pIrp->GetCurCtx();
        IrpCtxPtr pTopCtx = pIrp->GetTopStack();

        ret = pTopCtx->GetStatus();
        pCtx->SetStatus( ret );

        if( ERROR( ret ) )
        {
            pIrp->PopCtxStack();
            break;
        }

        guint32 dwIoDir =
            pTopCtx->GetIoDirection();

        if( dwIoDir == IRP_DIR_IN &&
            SUCCEEDED( ret ) )
        {
            pCtx->SetRespData(
                pTopCtx->m_pRespData );
        }
        pIrp->PopCtxStack();

    }while( 0 );

    return ret;
}

gint32 CRpcTcpFido::CompleteIoctlIrp(
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
        case CTRLCODE_SEND_DATA:
        case CTRLCODE_FETCH_DATA:
            {
                ret = CompleteSendReq( pIrp );
                break;
            }
        case CTRLCODE_LISTENING_FDO:
            {
                // copy the response data to the upper
                // stack for unknown irps
                ret = CompleteListening( pIrp );
                break;
            }
        default:
            {
                // copy the response data to the upper
                // stack for unknown irps
                if( !pIrp->IsIrpHolder() )
                {
                    IrpCtxPtr pTopCtx =
                        pIrp->GetTopStack();

                    ret = pTopCtx->GetStatus();
                    pCtx->SetStatus( ret );

                    if( ERROR( ret ) )
                        break;

                    guint32 dwIoDir =
                        pTopCtx->GetIoDirection();

                    BufPtr pResp;

                    if( dwIoDir == IRP_DIR_IN ||
                        dwIoDir == IRP_DIR_INOUT )
                    {
                        pResp = pTopCtx->m_pRespData;
                    }

                    if( pResp.IsEmpty() )
                        break;

                    dwIoDir =
                        pCtx->GetIoDirection();

                    if( dwIoDir == IRP_DIR_IN ||
                        dwIoDir == IRP_DIR_INOUT )
                    {
                        pCtx->SetRespData( pResp );
                    }
                    else
                    {
                        pCtx->m_pRespData.Clear();
                    }
                }
                else
                {
                    ret = pCtx->GetStatus();
                }
                break;
            }
        }

    }while( 0 );

    return ret;
}

gint32 CRpcTcpFido::CompleteFuncIrp(
    IRP* pIrp )
{
    gint32 ret = 0;
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
    {
        return -EINVAL;
    }

    if( pIrp->MajorCmd() != IRP_MJ_FUNC )
    {
        return -EINVAL;
    }
    
    switch( pIrp->MinorCmd() )
    {
    case IRP_MN_READ:
    case IRP_MN_WRITE:
        {
            ret = CompleteReadWriteReq( pIrp );
            break;
        }
    default:
        {
            ret = super::CompleteFuncIrp( pIrp );
            break;
        }
    }

    return ret;
}

gint32 CRpcTcpFido::CancelFuncIrp(
    IRP* pIrp, bool bForce )
{
    gint32 ret = 0;

    do{
        if( pIrp == nullptr ||
            pIrp->GetStackSize() == 0 )
        {
            ret = -EINVAL;
            break;
        }

        if( pIrp->MajorCmd() != IRP_MJ_FUNC )
        {
            ret = -EINVAL;
            break;
        }

        if( pIrp->IsIrpHolder() )
        {
            // remove irp from the internal map
            // with super::CancelFuncIrp
            ret = super::CancelFuncIrp(
                pIrp, bForce );
            break;
        }
        else
        {
            // this irp is queued in the lower
            // port, no need to remove from the
            // internal irp map
            ret = CPort::CancelFuncIrp(
                pIrp, bForce );
        }
    }while( 0 );

    return ret;
}

gint32 CRpcTcpFido::OnPortReady(
    IRP* pIrp )
{
    gint32 ret = super::OnPortReady( pIrp );
    if( ERROR( ret ) )
        return ret;

    do{
        // we are done
        if( !GetUpperPort().IsEmpty() )
            break;

        // notify the stack all the ports are
        // ready
        if( !pIrp->IsIrpHolder() )
            pIrp->PopCtxStack();

        IPort* pLowerPort = GetLowerPort();
        if( pLowerPort == nullptr )
            break;
            
        pIrp->AllocNextStack( pLowerPort );
        IrpCtxPtr pCtx = pIrp->GetTopStack();
        pCtx->SetMajorCmd( IRP_MJ_PNP );
        pCtx->SetMinorCmd( IRP_MN_PNP_STACK_READY );
        pLowerPort->AllocIrpCtxExt( pCtx );

        pCtx->SetIoDirection( IRP_DIR_OUT ); 
        pIrp->SetSyncCall( false );

        ret = pLowerPort->SubmitIrp( pIrp );
        pIrp->PopCtxStack();

        if(  ret == STATUS_PENDING )
        {
            // pending is not allowed
            ret = ERROR_FAIL;
            break;
        }

        pCtx = pIrp->GetCurCtx();
        pCtx->SetStatus( ret );

        CConnParamsProxy oConn =
            GetConnParams( this );

        m_bAuth = oConn.HasAuth();

    }while( 0 );

    return ret;
}

gint32 CRpcTcpFido::AllocIrpCtxExt(
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
                    FIDO_IRP_EXT oExt; 
                    oExt.iStmId = TCP_CONN_DEFAULT_STM;
                    oExt.iPeerStmId = TCP_CONN_DEFAULT_STM;

                    BufPtr pBuf( true );
                    if( ERROR( ret ) )
                        break;

                    *pBuf = oExt;
                    pIrpCtx->SetExtBuf( pBuf );
                    break;
                }
            default:
                break;
            }
            break;
        }
    default:
        {
            ret = super::AllocIrpCtxExt(
                pIrpCtx, pContext );
        }
        break;
    }
    return ret;
}

gint32 CTcpFidoListenTask::Process(
    guint32 dwContext )
{
    CCfgOpener oParams(
        ( IConfigDb* )GetConfig() );
    gint32 ret = 0;

    BufVecPtr pVecBuf( true );
    do{
        if( dwContext == eventIrpComp )
        {
            IrpPtr pIrp;
            ret = GetIrpFromParams( pIrp );
            if( ERROR( ret ) )
                break;

            if( pIrp.IsEmpty() ||
                pIrp->GetStackSize() == 0 )   
                break;

            ret = pIrp->GetStatus();
            if( ret == -EAGAIN ||
                ret == STATUS_MORE_PROCESS_NEEDED )
            {
                if( CanRetry() )
                    ret = STATUS_MORE_PROCESS_NEEDED;
                else if( ret == STATUS_MORE_PROCESS_NEEDED ) 
                    ret = -ETIMEDOUT;
                break;
            }
            if( ERROR( ret ) )
                break;

            IrpCtxPtr pCtx = pIrp->GetTopStack();
            BufPtr pRespBuf = pCtx->m_pRespData;
            ( *pVecBuf )().push_back( pRespBuf );

            // continue to get next event message
            dwContext = eventZero;
        }

        CIoManager* pMgr;

        ret = oParams.GetPointer(
            propIoMgr, pMgr );

        if( ERROR( ret ) )
            break;

        ObjPtr portPtr;
        ret = oParams.GetObjPtr(
            propPortPtr, portPtr );

        if( ERROR( ret ) )
            break;

        IPort* pPort = portPtr;
        if( pPort == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        IrpPtr pIrp( true );
        pIrp->AllocNextStack( pPort );

        IrpCtxPtr pCtx = pIrp->GetTopStack();

        pCtx->SetMajorCmd( IRP_MJ_FUNC );
        pCtx->SetMinorCmd( IRP_MN_IOCTL );
        pCtx->SetCtrlCode( CTRLCODE_LISTENING_FDO );

        // the stream id to listen to is in.
        pPort->AllocIrpCtxExt( pCtx );

        pCtx->SetIoDirection( IRP_DIR_IN ); 
        pIrp->SetSyncCall( false );
        pIrp->SetCompleteInPlace( true );

        // NOTE: no timer here
        //
        // set a callback
        pIrp->SetCallback( this, 0 );

        ret = pMgr->SubmitIrp(
            PortToHandle( pPort ),
            pIrp, false );

        if( ret != STATUS_PENDING )
            pIrp->RemoveCallback();

        // continue to get next event message
        if( SUCCEEDED( ret ) )
        {
            BufPtr pRespBuf = pCtx->m_pRespData;
            ( *pVecBuf )().push_back( pRespBuf );
            continue;
        }

        CRpcTcpFido* pFido = portPtr;
        if( pFido == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        switch( ret )
        {
        case STATUS_PENDING:
            {
                if( ( *pVecBuf )().empty() )
                    break;

                pFido->ScheduleRecvDataTask(
                    pVecBuf );

                ( *pVecBuf )().clear();
                break;
            }

        case -ETIMEDOUT:
        case -ENOTCONN:
            {
                DebugPrint( 0,
                    "The server is not online?,"
                    " retry scheduled..." );
                // fall through
            }
        case -EAGAIN:
            {
                if( ( *pVecBuf )().empty() )
                    break;

                pFido->ScheduleRecvDataTask(
                    pVecBuf );

                ( *pVecBuf )().clear();
                ret = STATUS_MORE_PROCESS_NEEDED;
                break;
            }

        case ERROR_PORT_STOPPED:
        case -ECANCELED:
            {
                // if canceled, or disconned, no
                // need to continue
                break;
            }

        // otherwise something must be wrong and
        // stop emitting irps
        }

        break;

    }while( 1 );

    if( ret != STATUS_PENDING &&
        ret != STATUS_MORE_PROCESS_NEEDED )
    {
        oParams.RemoveProperty(
            propPortPtr );
    }

    return SetError( ret );
}

gint32 CRpcTcpFido::PostStop(
    IRP* pIrp )
{
    if( !m_pListenTask.IsEmpty() )
        m_pListenTask.Clear();

    return super::PostStop( pIrp );
}

gint32 CRpcTcpFido::OnRespMsgNoIrp(
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

}
