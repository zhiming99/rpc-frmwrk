/*
 * =====================================================================================
 *
 *       Filename:  secfido.cpp
 *
 *    Description:  Implementation of security filter port and the driver
 *
 *        Version:  1.0
 *        Created:  07/14/2020 07:16:49 PM
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
#include "secfido.h"
#include <algorithm>
#include "security.h"

bool CRpcSecFido::HasSecCtx() const
{
    CCfgOpenerObj oPortCfg( this );
    ObjPtr pObj;

    gint32 ret = oPortCfg.GetObjPtr(
        propSecCtx, pObj );

    if( ERROR( ret ) )
        return false;

    return true;
}

gint32 CRpcSecFido::GetSecCtx(
    ObjPtr& pObj ) const
{
    CCfgOpenerObj oPortCfg( this );

    ObjPtr pVal;
    gint32 ret = oPortCfg.GetObjPtr(
        propSecCtx, pVal );

    if( ERROR( ret ) )
        return ret;

    pObj = pVal;

    return 0;
}

gint32 CRpcSecFido::IsClient() const
{
    ObjPtr pObj;
    gint32 ret = GetSecCtx( pObj );
    if( ERROR( ret ) )
        return ret;

    bool bServer = true;
    CCfgOpener oSecCtx(
        ( IConfigDb* )pObj );

    ret = oSecCtx.GetBoolProp(
        propIsServer, bServer );

    if( ERROR( ret ) )
        return ret;

    if( bServer )
        return 0;

    return ERROR_FALSE;
}

CRpcSecFido::CRpcSecFido(
    const IConfigDb* pCfg )
    :super( pCfg )
{
    SetClassId( clsid( CRpcSecFido ) );
    m_dwFlags &= ~PORTFLG_TYPE_MASK;
    m_dwFlags |= PORTFLG_TYPE_FIDO;
}

gint32 CRpcSecFido::OnSubmitIrp(
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

gint32 CRpcSecFido::EncEnabled()
{
    if( !HasSecCtx() )
        return ERROR_FALSE;

    gint32 ret = 0;
    do{
        ObjPtr pObj;
        ret = GetSecCtx( pObj );
        if( ERROR( ret ) )
        {
            ret = ERROR_FALSE;
            break;
        }
        CCfgOpener oSecCtx(
            ( IConfigDb* )pObj );
        bool bNoEnc = false;
        ret = oSecCtx.GetBoolProp(
            propNoEnc, bNoEnc );
        if( ERROR( ret ) )
        {
            ret = ERROR_FALSE;
            break;
        }

        if( bNoEnc )
            ret = ERROR_FALSE;

    }while( 0 );

    return ret;
}

gint32 CRpcSecFido::DoEncrypt(
    PIRP pIrp, BufPtr& pOutBuf, bool bEncrypt)
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    if( pOutBuf.IsEmpty() )
    {
        ret = pOutBuf.NewObj();
        if( ERROR( ret ) )
            return ret;
    }

    IrpCtxPtr pCtx = pIrp->GetTopStack();

    do{
        const guint32& dwHdrSize =
            sizeof( SEC_PACKET_HEADER );

        // encrypt the payload
        CfgPtr pCfg;
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

        if( unlikely( iCount <= 0 ) )
        {
            ret = -EINVAL;
            break;
        }

        BufPtr pInBuf( true );
        pInBuf->Resize( dwHdrSize );

        for( int i = 0; i < iCount; ++i )
        {
            BufPtr pPayload;
            ret = oParams.GetProperty(
                i, pPayload );
            if( ERROR( ret ) )
                break;

            pInBuf->Append( pPayload->ptr(),
                pPayload->size() );
        }

        if( pInBuf->size() >
            MAX_BYTES_PER_TRANSFER )
        {
            ret = -ERANGE;
            break;
        }
        if( pInBuf->size() == dwHdrSize )
        {
            ret = -ENOMEM;
            break;
        }

        ObjPtr pObj;
        if( bEncrypt )
        {
            ret = GetSecCtx( pObj );
            if( ERROR( ret ) )
                break;

            CCfgOpener oSecCtx(
                ( IConfigDb* )pObj );

            CObjBase* pAuthObj = nullptr;
            ret = oSecCtx.GetPointer(
                propObjPtr, pAuthObj );
            if( ERROR( ret ) )
                break;

            ret = IsClient();
            if( SUCCEEDED( ret ) )
            {
                pInBuf->SetOffset(
                pInBuf->offset() + dwHdrSize );

                IAuthenticateProxy* pAuth =
                dynamic_cast< IAuthenticateProxy* >
                    ( pAuthObj );

                if( unlikely( pAuth == nullptr ) )
                {
                    ret = -EFAULT;
                    break;
                }

                pOutBuf->Resize(
                    sizeof( dwHdrSize ) );

                ret = pAuth->WrapMessage(
                    pInBuf, pOutBuf );

                if( ERROR( ret ) )
                    break;
            }
            else if( ret == ERROR_FALSE )
            {
                pInBuf->SetOffset(
                    pInBuf->offset() + dwHdrSize );

                IAuthenticateServer* pAuth =
                dynamic_cast< IAuthenticateServer* >
                    ( pAuthObj );

                if( unlikely( pAuth == nullptr ) )
                {
                    ret = -EFAULT;
                    break;
                }

                std::string strHash;
                ret = oSecCtx.GetStrProp(
                    propSessHash, strHash );
                if( ERROR( ret ) )
                    break;

                pOutBuf->Resize( dwHdrSize );
                ret = pAuth->WrapMessage(
                    strHash, pInBuf, pOutBuf );

                if( ERROR( ret ) )
                    break;
            }
            else
            {
                // unexpected error
                break;
            }
        }
        else
        {
            pOutBuf = pInBuf;
        }

        guint32 dwSize =
            pOutBuf->size() - dwHdrSize;

        ( ( guint32* )pOutBuf->ptr() )[ 1 ] =
            htonl( dwSize );

        if( bEncrypt )
        {
            ( ( guint32* )pOutBuf->ptr() )[ 0 ] =
                htonl( ENC_PACKET_MAGIC );
        }
        else
        {
            ( ( guint32* )pOutBuf->ptr() )[ 0 ] =
                htonl( CLEAR_PACKET_MAGIC );
        }

    }while( 0 );

    return ret;
}

gint32 CRpcSecFido::SubmitWriteIrp(
    PIRP pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    IrpCtxPtr pCtx = pIrp->GetTopStack();

    do{
        bool bEncrypt = true;
        ret = EncEnabled();
        if( ERROR( ret ) )
            bEncrypt = false;

        BufPtr pOutBuf( true );
        ret = DoEncrypt( pIrp,
            pOutBuf, bEncrypt );

        if( ERROR( ret ) )
            break;

        CParamList oParams;
        oParams.Push( pOutBuf );
        BufPtr pReqBuf( true );
        *pReqBuf = ObjPtr( oParams.GetCfg() );

        PortPtr pLowerPort = GetLowerPort();
        ret = pIrp->AllocNextStack(
            pLowerPort, IOSTACK_ALLOC_COPY );

        if( ERROR( ret ) )
            break;

        IrpCtxPtr pTopCtx =
            pIrp->GetTopStack();

        pLowerPort->AllocIrpCtxExt(
            pTopCtx, pIrp );

        pTopCtx->SetReqData( pReqBuf );
        ret = pLowerPort->SubmitIrp( pIrp );

    }while( 0 );

    if( ret != STATUS_PENDING )
        pCtx->SetStatus( ret );

    return ret;
}

gint32 CRpcSecFido::SubmitIoctlCmd(
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

            if( unlikely( psse->m_iEvent ==
                sseError ) )
            {
                pCtx->SetRespData( pRespBuf );
                pCtx->SetStatus( ret );
            }
            else if( unlikely( psse->m_iEvent ==
                sseRetWithFd ) )
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

gint32 CRpcSecFido::CompleteListeningIrp(
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
        const guint32& dwHdrSize =
            sizeof( SEC_PACKET_HEADER );

        BufPtr pSecPacket = psse->m_pInBuf;
        if( pSecPacket.IsEmpty() ||
            pSecPacket->empty() )
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
            m_pInBuf = pSecPacket;
        }
        else
        {
            ret = m_pInBuf->Append(
                pSecPacket->ptr(),
                pSecPacket->size() );
            if( ERROR( ret ) )
                break;
        }

        char* pPayload = m_pInBuf->ptr();
        if( m_pInBuf->size() < dwHdrSize )
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
        guint32 dwMagic = ntohl(
            ( ( guint32* )pPayload )[ 0 ] );

        bool bEncrypted;
        if( dwMagic == ENC_PACKET_MAGIC )
            bEncrypted = true;
        else if( dwMagic == CLEAR_PACKET_MAGIC )
            bEncrypted = false;
        else
        {
            ret = -EPROTO;
            break;
        }

        guint32 dwSize = ntohl(
            ( ( guint32* )pPayload )[ 1 ] );

        BufPtr pInBuf ;
        if( dwSize ==
            m_pInBuf->size() - dwHdrSize )
        {
            pInBuf = m_pInBuf;
        }
        else if( dwSize <
            m_pInBuf->size() - dwHdrSize )
        {
            ret = pInBuf.NewObj();
            if( ERROR( ret ) )
                break;

            guint32 dwPktSize =
                dwSize + dwHdrSize;

            pInBuf->Append(
                m_pInBuf->ptr() + dwPktSize, 
                m_pInBuf->size() - dwPktSize );

            m_pInBuf->Resize( dwPktSize );
            std::swap( m_pInBuf, pInBuf );
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

        if( !bEncrypted )
        {
            pInBuf->SetOffset( dwHdrSize );
            psse->m_pInBuf = pInBuf;
            pCtx->SetRespData( pRespBuf );
            break;
        }

        ObjPtr pObj;
        ret = GetSecCtx( pObj );
        if( ERROR( ret ) )
            break;

        CCfgOpener oSecCtx(
            ( IConfigDb* )pObj );

        CObjBase* pAuthObj = nullptr;
        ret = oSecCtx.GetPointer(
            propObjPtr, pAuthObj );
        if( ERROR( ret ) )
            break;

        BufPtr pOutBuf;
        ret = IsClient();
        if( SUCCEEDED( ret ) )
        {
            pInBuf->SetOffset(
                pInBuf->offset() + dwHdrSize );

            IAuthenticateProxy* pAuth =
            dynamic_cast< IAuthenticateProxy* >
                ( pAuthObj );

            if( unlikely( pAuth == nullptr ) )
            {
                ret = -EFAULT;
                break;
            }

            ret = pAuth->UnwrapMessage(
                pInBuf, pOutBuf );

            if( ERROR( ret ) )
                break;

        }
        else if( ret == ERROR_FALSE )
        {
            pInBuf->SetOffset(
                pInBuf->offset() + dwHdrSize );

            IAuthenticateServer* pAuth =
            dynamic_cast< IAuthenticateServer* >
                ( pAuthObj );

            if( unlikely( pAuth == nullptr ) )
            {
                ret = -EFAULT;
                break;
            }

            std::string strHash;
            ret = oSecCtx.GetStrProp(
                propSessHash, strHash );
            if( ERROR( ret ) )
                break;

            ret = pAuth->UnwrapMessage(
                strHash, pInBuf, pOutBuf );

            if( ERROR( ret ) )
                break;
        }
        else
        {
            break;
        }

        psse->m_pInBuf = pOutBuf;
        pCtx->SetRespData( pRespBuf );
        break;

    }while( 1 );

    if( ret == STATUS_PENDING )
        return ret;

    if( ERROR( ret ) )
    {
        psse->m_iEvent = sseError;
        psse->m_iData = ret;
        pCtx->m_pRespData = pRespBuf;
    }

    pCtx->SetStatus( ret );

    if( !pIrp->IsIrpHolder() )
        pIrp->PopCtxStack();

    return ret;
}

gint32 CRpcSecFido::CompleteIoctlIrp(
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

gint32 CRpcSecFido::CompleteFuncIrp( IRP* pIrp )
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

gint32 CRpcSecFido::CompleteWriteIrp(
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

CRpcSecFidoDrv::CRpcSecFidoDrv(
    const IConfigDb* pCfg ) :
    super( pCfg )
{ SetClassId( clsid( CRpcSecFidoDrv ) ); }

gint32 CRpcSecFidoDrv::Probe(
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

        IConfigDb* pConnParams = nullptr;
        ret = oPdoPort.GetPointer(
            propConnParams, pConnParams );
        if( ERROR( ret ) )
            break;

        bool bAuth = false;
        CConnParams oConn( pConnParams );
        bAuth = oConn.HasAuth();
        if( !bAuth )
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
            PORT_CLASS_SEC_FIDO;

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
