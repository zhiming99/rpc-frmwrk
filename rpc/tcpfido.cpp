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
 * =====================================================================================
 */

#include "defines.h"
#include "stlcont.h"
#include "dbusport.h"
#include "tcpport.h"
#include "emaphelp.h"
#include "reqopen.h"
#include <byteswap.h>

using namespace std;

std::atomic< guint32 > CRpcTcpFido::m_atmSeqNo;

CRpcTcpFido::CRpcTcpFido(
    const IConfigDb* pCfg  )
    : super( pCfg )
{
    gint32 ret = 0;

    SetClassId( clsid( CRpcTcpFido ) );
    m_dwFlags &= ~PORTFLG_TYPE_MASK;
    m_dwFlags |= PORTFLG_TYPE_FIDO;

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
        if( pIrp == nullptr
            || pIrp->GetStackSize() == 0 )
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
    IrpCtxPtr& pCtx = pIrp->GetCurCtx();

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
    IrpCtxPtr& pCtx = pIrp->GetCurCtx();

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
                ret = 0;
                break;
            }
        case CTRLCODE_LISTENING:
            {
                ret = HandleListening( pIrp );
                break;
            }
        case CTRLCODE_SEND_DATA:
        case CTRLCODE_FETCH_DATA:
            {
                ret = HandleSendData( pIrp );
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
        IrpCtxPtr& pCtx = pIrp->GetCurCtx();
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
        IrpCtxPtr& pCtx = pIrp->GetCurCtx();
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

        if( dwCtrlCode == CTRLCODE_SEND_REQ )
        {
            dwSerial = m_atmSeqNo++;

            if( dwSerial == 0 )
                dwSerial = 1;

            ret = pReq.GetSerial( dwOldSerial );

            if( dwOldSerial == 0 )
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

        *pBuf = ObjPtr( oParams.GetCfg() );
        IrpCtxPtr& pNewCtx = pIrp->GetTopStack();
        pNewCtx->SetMajorCmd( IRP_MJ_FUNC );
        pNewCtx->SetMinorCmd( IRP_MN_WRITE );
        pNewCtx->SetIoDirection( IRP_DIR_OUT );
        pNewCtx->SetReqData( pBuf );

        ret = pLowerPort->SubmitIrp( pIrp );
        if(  ret == STATUS_PENDING )
            break;

        if( ERROR( ret ) )
        {
            pCtx->SetStatus( ret );
            break;
        }

        // move the irp to the waiting queue
        if( dwSerial != 0 &&
            dwIoDir == IRP_DIR_INOUT )
        {
            CStdRMutex oPortLock( GetLock() );
            m_mapSerial2Resp[ dwSerial ] = pIrp;
            ret = STATUS_PENDING;
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

        BufVecPtr pBufVec;
        pBufVec = pObj;
        if( pBufVec.IsEmpty() )
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

        for( auto&& oElem : ( *pBufVec )() )
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
        IrpCtxPtr& pCtx = pIrp->GetCurCtx();
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

        BufVecPtr pVecBuf( true );

        do{
            ret = pIrp->AllocNextStack( pLowerPort );
            if( ERROR( ret ) )
                break;

            IrpCtxPtr& pNewCtx = pIrp->GetTopStack();

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

            ( *pVecBuf )().push_back(
                pNewCtx->m_pRespData );

            pIrp->PopCtxStack();

            // iterate till we exhaust all the
            // cached messages

        }while( 1 );

        if( !( *pVecBuf )().empty() )
            ScheduleRecvDataTask( pVecBuf );

    }while( 0 );

    return ret;
}

gint32 CRpcTcpFido::ScheduleRecvDataTask(
    BufVecPtr& pVecBuf )
{
    if( pVecBuf.IsEmpty() )
        return 0;

    gint32 ret = 0;
    if( !( *pVecBuf )().empty() )
    {
        // schedule a task to dispatch the data
        CParamList oParams;
        ObjPtr pObj;
        pObj = pVecBuf;
        oParams.Push( pObj );
        oParams.SetPointer( propIoMgr, GetIoMgr() );

        oParams.SetObjPtr(
            propPortPtr, ObjPtr( this ) );

        ret = GetIoMgr()->ScheduleTask(
            clsid( CFidoRecvDataTask ),
            oParams.GetCfg() );
    }
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
        IrpCtxPtr& pCtx = pIrp->GetCurCtx();
        IrpCtxPtr& pTopCtx = pIrp->GetTopStack();
        BufVecPtr pVecBuf( true );

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
            BufPtr pRespData = pTopCtx->m_pRespData;
            if( !pRespData.IsEmpty() )
                ( *pVecBuf )().push_back( pRespData );
        }

        pIrp->PopCtxStack();

        ret = ScheduleRecvDataTask( pVecBuf );
        if( ERROR( ret ) )
            break;

        // we need to check if the port is in the
        // proper state before re-submit the irp
        guint32 dwOldState = PORT_STATE_INVALID;
        ret = CanContinue( pIrp,
            PORT_STATE_BUSY_SHARED, &dwOldState );

        if( ERROR( ret ) )
            return ret;

        ret = HandleListening( pIrp );
        PopState( dwOldState );

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
        IrpCtxPtr& pCtx = pIrp->GetCurCtx();
        guint32 dwIoDir = pCtx->GetIoDirection();

        if( !pIrp->IsIrpHolder() )
        {
            IrpCtxPtr& pTopCtx = pIrp->GetTopStack();
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
            if( dwIoDir == IRP_DIR_OUT )
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
                // put the irp to the reading
                // queue for the response
                // we need to check if the port is
                // still in the valid state for
                // io request
                guint32 dwOldState = 0;
                ret = CanContinue( pIrp,
                    PORT_STATE_BUSY_SHARED, &dwOldState );
                if( SUCCEEDED( ret ) )
                {
                    CStdRMutex oPortLock( GetLock() );
                    m_mapSerial2Resp[ dwSerial ] = pIrp;
                    ret = STATUS_PENDING;
                }
                PopState( dwOldState );
            }
            break;
        }
        else
        {
            ret = pCtx->GetStatus();

            // the response data 
            while( SUCCEEDED( ret ) )
            {
                if( dwIoDir != IRP_DIR_INOUT )
                {
                    // the response and event request
                    // goes here
                    // ret = -EFAULT;
                    break;
                }
                BufPtr& pBuf = pCtx->m_pRespData;
                if( pBuf.IsEmpty() )
                    break;

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
                break;
            }
        }

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

    return super::DispatchData( pData );
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
        IrpCtxPtr& pCtx = pIrp->GetCurCtx();
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

        IrpCtxPtr& pTopCtx =
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

        IrpCtxPtr& pNewCtx = pIrp->GetTopStack();
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
    IrpCtxPtr& pCtx = pIrp->GetCurCtx();

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

        ret = oParams.SetIntProp(
            1, ( guint32 )iPeerId );

        ret = BuildSendDataReq( pCfg, pMsg );
        if( ERROR( ret ) )
            break;

        string strPath = DBUS_OBJ_PATH(
            MODNAME_RPCROUTER, OBJNAME_TCP_BRIDGE );

        pMsg.SetPath( strPath );

        string strIfName =
            DBUS_IF_NAME( IFNAME_TCP_BRIDGE );

        pMsg.SetInterface( strIfName );

        string strDest =
            DBUS_DESTINATION( MODNAME_RPCROUTER );

        pMsg.SetDestination( strDest );

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

        IrpCtxPtr& pCtx = pIrp->GetCurCtx();
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

        if( SUCCEEDED( ret ) )
        {
            // cannot succeed at this moment
            ret = ERROR_FAIL;
        }

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
        IrpCtxPtr& pCtx = pIrp->GetCurCtx();
        IrpCtxPtr& pTopCtx = pIrp->GetTopStack();

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
        
        IrpCtxPtr& pCtx = pIrp->GetCurCtx();

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
            {
                // copy the response data to the upper
                // stack for unknown irps
                if( !pIrp->IsIrpHolder() )
                {
                    IrpCtxPtr& pTopCtx =
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
    gint32 ret = 0;
    do{
        if( GetUpperPort() != nullptr )
            return 0;

        ret = FireRmtSvrEvent(
            this, eventRmtSvrOnline );

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
