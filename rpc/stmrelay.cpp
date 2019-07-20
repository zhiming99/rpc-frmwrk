/*
 * =====================================================================================
 *
 *       Filename:  stmrelay.cpp
 *
 *    Description:  Implementation of CStreamProxyRelay and CStreamServerRelay
 *
 *        Version:  1.0
 *        Created:  07/10/2019 04:22:31 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *
 *      Copyright:  2019 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */

#include "rpcroute.h"
#include "stmrelay.h"

CRpcRouter* CStreamServerRelay::GetParent() const
{
    CRpcInterfaceServer* pMainIf =
        ObjPtr( ( CObjBase* )this );

    if( pMainIf == nullptr )
        return nullptr;

    return pMainIf->GetParent();
}

gint32 CStreamServerRelay::FetchData_Server(
    IConfigDb* pDataDesc,           // [in]
    gint32& fd,                     // [out]
    guint32& dwOffset,              // [in, out]
    guint32& dwSize,                // [in, out]
    IEventSink* pCallback )
{
    if( pDataDesc == nullptr ||
        pCallback == nullptr )
        return -EINVAL;

    if( fd <= 0 )
        return -EINVAL;

    gint32 ret = 0;

    do{
        std::string strDest;
        CCfgOpener oDataDesc( pDataDesc );
        ret = oDataDesc.GetStrProp(
            propDestDBusName, strDest );
        if( ERROR( ret ) )
            break;

        CRpcRouter* pRouter = GetParent();
        if( unlikely( pRouter == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }

        InterfPtr pIf;
        ret= pRouter->GetReqFwdrProxy(
            strDest, pIf );

        if( ERROR( ret ) )
            break;

        CRpcReqForwarderProxy* pProxy = pIf;
        if( unlikely( pProxy == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }

        // save the stream id for use later
        gint32 iStmId = fd;
        ret = IsTcpStmExist( iStmId );
        if( SUCCEEDED( ret ) )
        {
            // already exist. don't go further
            ret = -EEXIST;
            break;
        }

        CParamList oContext;
        oContext.Push( iStmId );
        oContext.Push( ObjPtr( pDataDesc ) );

        fd = -1;
        TaskletPtr pWrapper;
        ret = NEW_PROXY_RESP_HANDLER(
            pWrapper, ObjPtr( this ),
            &CStreamServerRelay::OnFetchDataComplete,
            pCallback, oContext.GetCfg() );

        if( ERROR( ret ) )
            break;

        oContext.Push( ObjPtr( pWrapper ) );

        ( *pWrapper )( eventZero );

        // give a valid value just to pass parameter
        // validation
        dwOffset = 0;
        dwSize = 0x20;

        ret = pProxy->FetchData( pDataDesc,
            fd, dwOffset, dwSize, pWrapper );

    }while( 0 );

    return ret;
}

gint32 CStreamServerRelay::OnFetchDataComplete(
    IEventSink* pCallback,
    IEventSink* pIoReq,
    IConfigDb* pContext )
{
    if( pCallback == nullptr ||
        pIoReq == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    gint32 iFd = -1;
    gint32 iStmId = -1;

    do{
        if( pContext == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        CCfgOpenerObj oReq( pIoReq );
        IConfigDb* pResp = nullptr;
        ret = oReq.GetPointer(
            propRespPtr, pResp );
        if( ERROR( ret ) )
            break;

        CCfgOpener oResp( pResp );
        gint32 iRet = oResp[ propReturnValue ];
        if( ERROR( iRet ) )
        {
            ret = iRet;
            break;
        }

        ret = oResp.GetIntProp(
            1, ( guint32& )iFd );
        if( ERROR( ret ) )
            break;

        CCfgOpener oCtx( pContext );
        IConfigDb* pDataDesc = nullptr;
        ret = oCtx.GetPointer( 1, pDataDesc );
        if( ERROR( ret ) )
            break;

        ret = oCtx.GetIntProp( 0,
            ( guint32& )iStmId );
        if( ERROR( ret ) )
            break;

        // we need to do the following things before
        // reponse to the remote client
        //
        // 1. create and start the uxstream proxy
        InterfPtr pUxIf;
        ret = CreateUxStream(
            pDataDesc, iFd,
            clsid( CUnixSockStmProxyRelay ),
            false, pUxIf );

        if( ERROR( ret ) )
            break;

        CCfgOpenerObj oUxIf(
            ( CObjBase*) pUxIf );

        oUxIf.SetIntProp( propStreamId,
            ( guint32& )iStmId );

        CParamList oParams;
        oParams[ propIfPtr ] = ObjPtr( this );
        oParams.Push( ObjPtr( pUxIf ) );
        oParams.Push( ObjPtr( pDataDesc ) );
        oParams.Push( iStmId );

        oParams[ propEventSink ] =
            ObjPtr( pCallback );

        TaskletPtr pStartTask;
        ret = pStartTask.NewObj(
            clsid( CIfStartUxSockStmRelayTask ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        ( *pStartTask )( 0 );
        ret = this->AddSeqTask( pStartTask );
        //
        // 2. bind the uxport and the the tcp stream
        //
        // 3. start the double-direction listening
        // tasks
        //
        // 4. or rollback all the above operations on
        // error, and close the local tcp stream
        //
        // 5. send the response to the remote client
        

    }while( 0 );
    
    if( ERROR( ret ) )
    {
        CParamList oResp;
        oResp[ propReturnValue ] = ret;
        OnServiceComplete(
            oResp.GetCfg(), pCallback );

        if( iFd > 0 )
        {
            // close the fd to notify the failure
            close( iFd );
            iFd = -1;
        }
        if( iStmId > 0 )
        {
            CRpcTcpBridge* pBridge = ObjPtr( this );
            if( likely( pBridge != nullptr ) )
            {
                pBridge->CloseLocalStream(
                    nullptr, iStmId );
            }
        }
    }

    if( pContext )
    {
        CParamList oContext( pContext );
        oContext.ClearParams();
    }

    return ret;
}

CRpcRouter* CStreamProxyRelay::GetParent() const
{
    CRpcInterfaceProxy* pMainIf =
        ObjPtr( ( CObjBase* )this );
    if( pMainIf == nullptr )
        return nullptr;
    return pMainIf->GetParent();
}

gint32 CStreamProxyRelay::OnFetchDataComplete(
    IEventSink* pCallback,
    IEventSink* pIoReqTask,
    IConfigDb* pContext )
{

    // we need to do the following things before
    // reponse to the remote client
    //
    // 1. create and start the uxport
    //
    // 2. bind the uxport and the the tcp stream
    //
    // 3. start the double-direction listening
    // tasks
    //
    // 4. or rollback all the above operations and
    // close the local tcp stream on error
    //
    // 5. send the response to the local client
    if( pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    gint32 iStmId = -1;
    gint32 iLocal = -1;
    gint32 iRemote = -1;

    do{
        if( pIoReqTask == nullptr ||
            pContext == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        CCfgOpenerObj oIoReq( pIoReqTask );
        IConfigDb* pResp = nullptr;
        ret = oIoReq.GetPointer(
            propRespPtr, pResp );
        if( ERROR( ret ) )
            break;

        CCfgOpener oResp( pResp );
        ret = oResp[ propReturnValue ];
        if( ERROR( ret ) )
            break;

        // time to create uxstream object
        iStmId = oResp[ 1 ];
        ret = CreateSocket( iLocal, iRemote );
        if( ERROR( ret ) )
            break;

        CParamList oContext( pContext );
        IConfigDb* pDataDesc = nullptr;

        ret = oContext.GetPointer(
            0, pDataDesc );

        if( ERROR( ret ) )
            break;

        InterfPtr pUxIf;
        ret = CreateUxStream( pDataDesc, iLocal,
            clsid( CUnixSockStmServerRelay ),
            true, pUxIf );

        if( ERROR( ret ) )
            break;

        CCfgOpenerObj oUxIf(
            ( CObjBase*) pUxIf );

        oUxIf.SetIntProp( propStreamId,
            ( guint32& )iStmId );

        CParamList oParams;
        oParams[ propIsServer ] = ( bool )true;
        oParams[ propIfPtr ] = ObjPtr( this );
        oParams.Push( ObjPtr( pUxIf ) );
        oParams.Push( ObjPtr( pDataDesc ) );
        oParams.Push( iStmId );
        oParams.Push( iRemote );

        oParams[ propEventSink ] =
            ObjPtr( pCallback );

        TaskletPtr pStartTask;
        ret = pStartTask.NewObj(
            clsid( CIfStartUxSockStmRelayTask ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;
        
    }while( 0 );

    if( ERROR( ret ) )
    {
        CParamList oResp;
        oResp[ propReturnValue ] = ret;

        TaskletPtr pDummy;
        pDummy.NewObj(
            clsid( CIfDummyTask ) );

        CCfgOpenerObj oTaskCfg(
            ( CObjBase* )pDummy );

        oTaskCfg.SetObjPtr( propRespPtr,
            ObjPtr( oResp.GetCfg() ) );

        guint32* pParams = ( guint32* )
            ( ( CObjBase* )pDummy );

        if( iStmId > 0 )
        {
            CRpcTcpBridgeProxy* pProxy =
                ObjPtr( this );

            pProxy->CloseLocalStream(
                nullptr, iStmId );
        }
        pCallback->OnEvent( eventTaskComp,
            ret, 0, pParams );

        if( iLocal >= 0 )
        {
            close( iLocal );
            iLocal = -1;
        }
        if( iRemote > 0 )
        {
            close( iRemote );
            iRemote = -1;
        }
    }

    return ret;
}

gint32 CStreamProxyRelay::OnOpenStreamComplete(
    IEventSink* pCallback,
    IEventSink* pIoReqTask,
    IConfigDb* pContext )
{
    if( pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    gint32 iStmId = -1;

    do{
        if( pIoReqTask == nullptr ||
            pContext == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        CCfgOpenerObj oIoReq( pIoReqTask );
        IConfigDb* pResp = nullptr;
        ret = oIoReq.GetPointer(
            propRespPtr, pResp );
        if( ERROR( ret ) )
            break;

        CCfgOpener oResp( pResp );
        ret = oResp[ propReturnValue ];
        if( ERROR( ret ) )
            break;

        iStmId = oResp[ 0 ];
        guint32 dwProtocol = oResp[ 1 ];
        if( dwProtocol != protoStream )
        {
            ret = -EPROTO;
            break;
        }

        CRpcTcpBridgeProxy* pProxy =
            ObjPtr( this );

        CParamList oContext( pContext );
        IConfigDb* pDataDesc = nullptr;

        ret = oContext.GetPointer(
            0, pDataDesc );

        if( ERROR( ret ) )
            break;

        oContext.Push( iStmId );

        TaskletPtr pWrapper;
        ret = NEW_PROXY_RESP_HANDLER(
            pWrapper, ObjPtr( this ),
            &CStreamProxyRelay::OnFetchDataComplete,
            pCallback, oContext.GetCfg() );

        guint32 dwSize = 0x20;
        guint32 dwOffset = 0;
        ret = pProxy->super::FetchData_Proxy(
            pDataDesc, iStmId, dwOffset, dwSize,
            pWrapper );
        
    }while( 0 );

    if( ERROR( ret ) )
    {
        CParamList oResp;
        oResp[ propReturnValue ] = ret;

        TaskletPtr pDummy;
        pDummy.NewObj(
            clsid( CIfDummyTask ) );

        CCfgOpenerObj oTaskCfg(
            ( CObjBase* )pDummy );

        oTaskCfg.SetObjPtr( propRespPtr,
            ObjPtr( oResp.GetCfg() ) );

        guint32* pParams = ( guint32* )
            ( ( CObjBase* )pDummy );

        if( iStmId > 0 )
        {
            CRpcTcpBridgeProxy* pProxy =
                ObjPtr( this );

            pProxy->CloseLocalStream(
                nullptr, iStmId );
        }
        pCallback->OnEvent( eventTaskComp,
            ret, 0, pParams );
    }

    return ret;
}

gint32 CStreamProxyRelay::FetchData_Proxy(
    IConfigDb* pDataDesc,           // [in]
    gint32& fd,                     // [out]
    guint32& dwOffset,              // [in, out]
    guint32& dwSize,                // [in, out]
    IEventSink* pCallback )
{
    // do the following tasks.
    // 1. open a tcp stream
    // 2. send the fetch_data request

    if( pDataDesc == nullptr ||
        pCallback == nullptr )
        return -EINVAL;

    if( fd <= 0 )
        return -EINVAL;

    gint32 ret = 0;
    CParamList oContext;

    do{
        CRpcTcpBridgeProxy* pProxy = ObjPtr( this );
        if( unlikely( pProxy == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }

        // save the stream id for use later
        gint32 iStmId = -1;
        oContext.Push( ObjPtr( pDataDesc ) );

        TaskletPtr pWrapper;
        ret = NEW_PROXY_RESP_HANDLER(
            pWrapper, ObjPtr( this ),
            &CStreamProxyRelay::OnOpenStreamComplete,
            pCallback, oContext.GetCfg() );

        if( ERROR( ret ) )
            break;

        ret = pProxy->OpenStream(
            protoStream, iStmId, pWrapper );

    }while( 0 );

    if( ret != STATUS_PENDING )
        oContext.ClearParams();

    return ret;
}

gint32 CIfStartUxSockStmRelayTask::CloseTcpStream(
    CRpcServices* pSvc, gint32 iStmId )
{
    if( pSvc == nullptr ||
        iStmId < 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        if( pSvc->IsServer() )
        {
            CStreamServerRelay* pSvr =
                ObjPtr( pSvc );
            ret = pSvr->CloseTcpStream( iStmId );
        }
        else
        {
            CStreamServerRelay* pProxy =
                ObjPtr( pSvc );
            ret = pProxy->CloseTcpStream( iStmId );
        }

    }while( 0 );

    return ret;
}

gint32 CIfStartUxSockStmRelayTask::OnTaskComplete(
    gint32 iRet )
{
    gint32 ret = 0;

    CParamList oResp;
    CParamList oParams(
        ( IConfigDb* )GetConfig() );

    CRpcServices* pUxSvc = nullptr;
    gint32 iStmId = -1;
    gint32 iRemote = -1;

    InterfPtr pParentIf;

    do{
        if( ERROR( iRet ) )
        {
            ret = iRet;
            break;
        }

        ObjPtr pIf;
        ret = oParams.GetObjPtr( propIfPtr, pIf );
        if( ERROR( ret ) )
            break;
        
        pParentIf = pIf;
        CRpcServices* pParent = pIf;
        IStream* pStream = 
            dynamic_cast< IStream* >( pParent );

        if( pStream == nullptr ||
            pParent == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        bool bBdgSvr = false;
        if( pParent->IsServer() )
            bBdgSvr = true;

        ret = oParams.GetObjPtr( 0, pIf );
        if( ERROR( ret ) )
            break;

        pUxSvc = pIf;
        if( pUxSvc == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        HANDLE hChannel = ( HANDLE )pUxSvc;
        InterfPtr pChanlIf( pIf );
        ret = pStream->AddUxStream(
            hChannel, pChanlIf );

        if( ERROR( ret ) )
            break;

        oParams.GetIntProp(
            2, ( guint32& )iStmId ) ;

        if( bBdgSvr )
        {
            CStreamServerRelay* pStmRly = pParentIf;
            ret = pStmRly->BindUxTcpStream( 
                hChannel, iStmId );
        }
        else
        {
            CStreamProxyRelay* pStmRly = pParentIf;
            ret = pStmRly->BindUxTcpStream( 
                hChannel, iStmId );
        }

        if( ERROR( ret ) )
        {
            break;
        }

        // set the response for FETCH_DATA request
        oResp[ propReturnValue ] = STATUS_SUCCESS;
        // push the pDataDesc
        oResp.Push( ( ObjPtr& ) oParams[ 1 ] );
        if( bBdgSvr )
        {
            // push the tcp stream id
            oResp.Push( ( guint32 )iStmId );
        }
        else
        {
            // push ux sock fd to return
            iRemote = oParams[ 3 ];
            oResp.Push( ( guint32& )iRemote );
        }

        // placeholders
        oResp.Push( 0 );
        oResp.Push( 0x20 );

        // set the response for OpenChannel
        TaskletPtr pConnTask;
        ret = DEFER_IFCALL_NOSCHED( pConnTask,
            ObjPtr( pParent ),
            &IStream::OnConnected,
            ( HANDLE )pUxSvc );

        if( ERROR( ret ) )
            break;

        pParent->RunManagedTask( pConnTask );

    }while( 0 );

    while( ERROR( ret ) )
    {
        if( pUxSvc == nullptr || 
            pParentIf.IsEmpty() )
            break;

        if( iStmId < 0 )
            break;

        CRpcServices* pParent = pParentIf;
        if( pParent->IsServer() )
        {
            CStreamServerRelay* pStream =
                ObjPtr( pParent );

            if( unlikely( pStream == nullptr ) )
                break;

            pStream->RemoveBinding(
                ( HANDLE )pUxSvc, iStmId );

            pStream->RemoveUxStream(
                ( HANDLE )pUxSvc );

            InterfPtr pIf( pUxSvc );
            pStream->CloseUxStream( pIf );

            CRpcTcpBridge* pShared = pParentIf;
            pShared->CloseLocalStream(
                nullptr, iStmId );
        }
        else
        {
            CStreamProxyRelay* pStream =
                ObjPtr( pParent );

            if( unlikely( pStream == nullptr ) )
                break;

            pStream->RemoveBinding(
                ( HANDLE )pUxSvc, iStmId );

            pStream->RemoveUxStream(
                ( HANDLE )pUxSvc );

            InterfPtr pIf( pUxSvc );
            pStream->CloseUxStream( pIf );

            CloseTcpStream( pParent, iStmId );
        }

        break;
    }

    if( ERROR( ret ) )
        oResp[ propReturnValue ] = ret;

    EventPtr pEvt;
    iRet = GetInterceptTask( pEvt );
    if( SUCCEEDED( iRet ) )
    {
        CCfgOpenerObj oCfg( ( IEventSink* )pEvt );
        oCfg.SetPointer( propRespPtr,
            ( CObjBase* )oResp.GetCfg() );

        pEvt->OnEvent( eventTaskComp, ret, 0, 0 );
    }

    if( iRemote > 0 )
    {
        close( iRemote );
        iRemote = -1;
    }
    oParams.ClearParams();
    ClearClientNotify();

    return iRet;
}

gint32 CIfUxListeningRelayTask::RunTask()
{
    gint32 ret = 0;
    do{
        CRpcServices* pIf = nullptr;
        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );

        ret = oCfg.GetPointer( propIfPtr, pIf );
        if( ERROR( ret ) )
            break;

        ObjPtr pObj( pIf );
        CIfUxRelayTaskHelper oHelper( this );
        ret = oHelper.StartListening( this );

        if( ret == STATUS_PENDING )
            break;

        if( ERROR( ret ) )
            break;

        ret = OnTaskComplete( ret );
        if( ret == STATUS_PENDING )
            break;

    }while( 1 );

    return ret;
}

gint32 CIfUxListeningRelayTask::OnTaskComplete(
    gint32 iRet )
{
    gint32 ret = 0;
    if( ERROR( iRet ) )
        return iRet;

    do{
        IConfigDb* pCfg = GetConfig();
        CCfgOpener oCfg( pCfg );

        ObjPtr pIf;
        ret = oCfg.GetObjPtr( propIfPtr, pIf );
        if( ERROR( ret ) )
            break;

        IConfigDb* pResp = nullptr;
        ret = oCfg.GetPointer(
            propRespPtr, pResp );

        if( ERROR( ret ) )
            break;

        CParamList oResp( pResp );
        BufPtr pPayload;
        oResp.GetProperty( 0, pPayload );
        if( ERROR( ret ) )
            break;
        
        if( pPayload.IsEmpty() ||
            pPayload->empty() )
        {
            ret = -EFAULT;
            break;
        }

        ret = PostEvent( pPayload );

    }while( 0 );

    return ret;
}

gint32 CIfUxListeningRelayTask::PostEvent(
    BufPtr& pBuf )
{
    if( pBuf.IsEmpty() ||
        pBuf->empty() )
        return -EINVAL;

    CRpcServices* pIf = nullptr;
    CCfgOpener oCfg(
        ( IConfigDb* )GetConfig() );

    gint32 ret = oCfg.GetPointer(
        propIfPtr, pIf );

    if( ERROR( ret ) )
        return ret;

    CIfUxRelayTaskHelper oHelper( this );

    guint8 byToken = pBuf->ptr()[ 0 ];
    BufPtr pNewBuf;
    switch( byToken )
    {
    case tokPing:
    case tokPong:
        {
            BufPtr pEmptyBuf;
            ret = oHelper.ForwardToRemote(
                byToken, pEmptyBuf );
            break;
        }
    case tokProgress:
        {
            guint32 dwSize;
            memcpy( &dwSize, pBuf->ptr() + 1,
                sizeof( guint32 ) );
            dwSize = ntohl( dwSize );
            BufPtr pNewBuf;
            pNewBuf->Resize( dwSize );
            if( ERROR( ret ) )
                break;

            if( dwSize > pBuf->size()- 1 - sizeof( guint32 ) )
            {
                ret = -EBADMSG;
                break;
            }

            memcpy( pNewBuf->ptr(),
                pBuf->ptr() + 1 + sizeof( guint32 ),
                dwSize );

            ret = oHelper.ForwardToRemote(
                byToken, pNewBuf );

            break;
        }
    case tokError:
        {
            gint32 iError;
            memcpy( &iError, pBuf->ptr() + 1,
                sizeof( gint32 ) );

            iError = ntohl( iError );
            BufPtr pNewBuf;
            pNewBuf->Resize( sizeof( gint32 ) );
            memcpy( pNewBuf->ptr(),
                &iError, sizeof( iError ) );
            break;
        }
    case tokClose:
    case tokFlowCtrl:
    case tokLift:
        {
            break;
        }
    default:
        {
            ret = -EINVAL;
            break;
        }
    }

    if( ret == ERROR_QUEUE_FULL )
    {
        ret = STATUS_PENDING;
        Pause();
    }

    if( ERROR( ret ) )
        return ret;

    ret = oHelper.PostUxSockEvent(
        byToken, pNewBuf );

    return ret;
}

gint32 CIfUxListeningRelayTask::OnIrpComplete(
    IRP* pIrp )
{
    gint32 ret = 0;

    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    do{
        IConfigDb* pCfg = GetConfig();
        CCfgOpener oCfg( pCfg );
        CRpcServices* pIf = nullptr;
        ret = oCfg.GetPointer( propIfPtr, pIf );
        if( ERROR( ret ) )
            break;

        ret = pIrp->GetStatus();
        if( ERROR( ret ) )
        {
            BufPtr pCloseBuf( true );
            *pCloseBuf = tokClose;
            ret = PostEvent( pCloseBuf );
            break;
        }
        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        BufPtr pPayload = pCtx->m_pRespData;
        if( pPayload.IsEmpty() ||
            pPayload->empty() )
        {
            ret = -EFAULT;
            break;
        }

        ret = PostEvent( pCtx->m_pRespData );
        if( ERROR( ret ) )
            break;

        oCfg.RemoveProperty( propIrpPtr );
        oCfg.RemoveProperty( propRespPtr );

        ret = ReRun();
        if( ERROR( ret ) )
            break;

        ret = STATUS_PENDING;

    }while( 0 );

    return ret;
}

CIfUxSockTransRelayTask::CIfUxSockTransRelayTask(
    const IConfigDb* pCfg ) :
    super( pCfg )
{
    SetClassId( clsid( CIfUxSockTransRelayTask ) );

    gint32 ret = 0;

    CParamList oParams(
        ( IConfigDb* )GetConfig() );
    do{
        CRpcServices* pIf;
        ret = oParams.GetPointer(
            propIfPtr, pIf );

        if( ERROR( ret ) )
            break;
        m_pUxStream = pIf;

        ret = oParams.GetIntProp(
            propTimeoutSec,
            m_dwTimeoutSec );

        if( ERROR( ret ) )
            break;

        ret = oParams.GetIntProp(
            propKeepAliveSec,
            m_dwKeepAliveSec );

        if( ERROR( ret ) )
            break;

        m_bIn = ( bool& )oParams[ 0 ];

    }while( 0 );

    if( ERROR( ret ) )
    {
        std::string strMsg = DebugMsg( ret,
            "Error in CIfUxSockTransRelayTask ctor" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CIfUxSockTransRelayTask::OnTaskComplete(
    gint32 iRet )
{
    gint32 ret = 0;
    if( ERROR( iRet ) )
        return iRet;

    CCfgOpenerObj oCallback( this );
    do{
        IConfigDb* pResp = nullptr;
        ret = oCallback.GetPointer(
            propRespPtr, pResp );

        if( ERROR( ret ) )
            break;

        CParamList oResp( pResp );
        BufPtr pPayload;
        oResp.GetProperty( 0, pPayload );
        if( ERROR( ret ) )
            break;
        
        if( pPayload.IsEmpty() ||
            pPayload->empty() )
        {
            ret = -EFAULT;
            break;
        }

        CIfUxRelayTaskHelper oHelper( this );
        if( IsReading() )
        {
            ret = oHelper.ForwardToRemote(
                tokData, pPayload );
            if( ret == ERROR_QUEUE_FULL )
            {
                ret = STATUS_PENDING;
                Pause();
            }

            oHelper.PostUxSockEvent(
                tokData, pPayload );
        }

    }while( 0 );

    return ret;
}

gint32 CIfUxSockTransRelayTask::HandleIrpResp(
    IRP* pIrp )
{
    gint32 ret = 0;

    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    ret = pIrp->GetStatus();
    if( ERROR( ret ) )
        return ret;

    if( !IsReading() )
        return ret;

    CIfUxRelayTaskHelper oHelper( this );
    do{
        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        BufPtr pPayload = pCtx->m_pRespData;

        ret = oHelper.ForwardToRemote(
            tokData, pPayload );

        if( ret == ERROR_QUEUE_FULL )
        {
            ret = STATUS_PENDING;
            Pause();
        }

        oHelper.PostUxSockEvent(
            tokData, pPayload );

    }while( 0 );

    return ret;
}

gint32 CIfUxSockTransRelayTask::OnIrpComplete(
    IRP* pIrp )
{
    gint32 ret = 0;

    do{
        RemoveProperty( propIrpPtr );
        RemoveProperty( propRespPtr );

        ret = HandleIrpResp( pIrp );
        if( ERROR( ret ) )
            break;

        if( IsPaused() )
            break;

        ret = ReRun();
        if( ERROR( ret ) )
            break;

        ret = STATUS_PENDING;

    }while( 0 );

    return ret;
}

gint32 CIfUxSockTransRelayTask::OnComplete( gint32 iRet )
{
    super::OnComplete( iRet );
    m_pUxStream.Clear();
    return iRet;
}

gint32 CIfUxSockTransRelayTask::RunTask()
{
    gint32 ret = 0;
    do{
        CIfUxRelayTaskHelper oHelper( this );
        if( IsReading() )
        {
            ret = oHelper.StartReading( this );
            if( ret == STATUS_PENDING )
                break;

            if( ERROR( ret ) )
                break;

            ret = OnTaskComplete( ret );
        }
        else
        {
            guint8 byToken = 0;
            BufPtr pPayload;
            ret = oHelper.GetReqToLocal(
                byToken, pPayload );

            if( ret == -ENOENT )
            {
                ret = STATUS_PENDING;
                Pause();
                break;
            }

            if( byToken == tokData )
            {
                ret = oHelper.WriteStream(
                    pPayload, this );
            }
            else
            {

            }
        }

        if( ERROR( ret ) )
            break;

        if( ret == STATUS_PENDING )
            break;

    }while( 1 );

    return ret;
}



CIfTcpStmTransTask::CIfTcpStmTransTask(
    const IConfigDb* pCfg ) :
    super( pCfg )
{
    SetClassId( clsid( CIfTcpStmTransTask ) );

    gint32 ret = 0;
    CParamList oParams(
        ( IConfigDb* )GetConfig() );
    do{
        CRpcServices* pIf;
        ret = oParams.GetPointer(
            propIfPtr, pIf );

        if( ERROR( ret ) )
            break;
        m_pUxStream = pIf;

        ret = oParams.GetIntProp(
            propTimeoutSec,
            m_dwTimeoutSec );

        if( ERROR( ret ) )
            break;

        ret = oParams.GetIntProp(
            propKeepAliveSec,
            m_dwKeepAliveSec );

        if( ERROR( ret ) )
            break;

        m_bIn = ( bool& )oParams[ 0 ];

    }while( 0 );

    if( ERROR( ret ) )
    {
        std::string strMsg = DebugMsg( ret,
            "Error in CIfTcpStmTransTask ctor" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CIfTcpStmTransTask::HandleIrpResp(
    IRP* pIrp )
{
    gint32 ret = 0;

    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    ret = pIrp->GetStatus();
    if( ERROR( ret ) )
        return ret;

    if( !IsReading() )
        return ret;

    IrpCtxPtr& pCtx = pIrp->GetTopStack();
    BufPtr pPayload = pCtx->m_pRespData;
    ret = PostEvent( pPayload );

    return ret;
}

gint32 CIfTcpStmTransTask::OnIrpComplete(
    IRP* pIrp )
{
    gint32 ret = 0;

    do{
        RemoveProperty( propIrpPtr );
        RemoveProperty( propRespPtr );

        ret = HandleIrpResp( pIrp );
        if( ERROR( ret ) )
            break;

        if( IsPaused() )
            break;

        ret = ReRun();
        if( ERROR( ret ) )
            break;

        ret = STATUS_PENDING;

    }while( 0 );

    return ret;
}

gint32 CIfTcpStmTransTask::OnComplete( gint32 iRet )
{
    super::OnComplete( iRet );
    m_pUxStream.Clear();
    return iRet;
}

gint32 CIfTcpStmTransTask::RunTask()
{
    gint32 ret = 0;
    do{
        CIfUxRelayTaskHelper oHelper( this );

        CCfgOpenerObj oCfg( this );

        gint32 iStmId = -1;
        ret = oCfg.GetIntProp( propStreamId,
            ( guint32& )iStmId );

        if( ERROR( ret ) )
            break;

        BufPtr pNewBuf;

        if( IsReading() )
        {
            BufPtr pBuf( true );
            ret = oHelper.ReadTcpStream(
                iStmId, pBuf, 20, this );

            if( ret == STATUS_PENDING )
                break;

            if( ERROR( ret ) )
                break;

            ret = PostEvent( pBuf );
        }
        else
        {
            guint8 byToken = 0;
            BufPtr pPayload;
            ret = oHelper.GetReqToRemote(
                byToken, pPayload );

            if( ret == -ENOENT )
            {
                ret = STATUS_PENDING;
                Pause();
                break;
            }
            
            ret = LocalToRemote( byToken,
                pPayload, pNewBuf );

            if( ERROR( ret ) )
                break;

            ret = oHelper.WriteTcpStream(
                iStmId, pNewBuf,
                pNewBuf->size(),
                this );
        }

        if( ERROR( ret ) )
            break;

        if( ret == STATUS_PENDING )
            break;

    }while( 1 );

    return ret;
}

gint32 CIfTcpStmTransTask::LocalToRemote(
    guint8 byToken,
    BufPtr& pLocal,
    BufPtr& pRemote )
{
    gint32 ret = 0;
    if( pLocal.IsEmpty() ||
        pLocal->empty() )
        return -EINVAL;

    BufPtr pBuf = pLocal;
    guint32 dwHdrSize =
        sizeof( guint8 ) + sizeof( guint32 );

    switch( byToken )
    {
    case tokData:
    case tokProgress:
        {
            guint32 dwSize = htonl( pBuf->size() );
            if( pBuf->offset() >= dwHdrSize )
            {
                // add the header ahead of the payload
                pBuf->SetOffset( pBuf->offset() - dwHdrSize );
                pBuf->ptr()[ 0 ] = byToken;
                memcpy( pBuf->ptr() + 1,
                    &dwSize, sizeof( dwSize ) );
            }
            else
            {
                // rebuild the packet
                BufPtr pNewBuf( true );
                pNewBuf->Resize(
                    pBuf->size() + dwHdrSize );
                memcpy( pNewBuf->ptr() + dwHdrSize,
                    pBuf->ptr(), pBuf->size() );
                pNewBuf->ptr()[ 0 ] = byToken;
                memcpy( pNewBuf->ptr() + 1,
                    &dwSize, sizeof( dwSize ) );
                pBuf = pNewBuf;
            }
            pRemote = pBuf;
            break;
        }
    case tokPing:
    case tokPong:
    case tokClose:
    case tokFlowCtrl:
    case tokLift:
        {
            if( pRemote.IsEmpty() )
            {
                ret = pRemote.NewObj();
                if( ERROR( ret ) )
                    break;
            }

            *pRemote = byToken;
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

gint32 CIfTcpStmTransTask::RemoteToLocal(
    BufPtr& pRemote,
    guint8& byToken,
    BufPtr& pLocal )
{

    gint32 ret = 0;
    if( pRemote.IsEmpty() ||
        pRemote->empty() )
        return -EINVAL;

    BufPtr pBuf = pRemote;
    byToken = pBuf->ptr()[ 0 ];

    guint32 dwHdrSize =
        sizeof( guint8 ) + sizeof( guint32 );

    switch( byToken )
    {
    case tokData:
    case tokProgress:
        {
            ret = pBuf->SetOffset(
                pBuf->offset() + dwHdrSize );

            if( ERROR( ret ) )
                break;

            pLocal = pBuf;
            break;
        }
    case tokPing:
    case tokPong:
    case tokClose:
    case tokFlowCtrl:
    case tokLift:
        {
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

gint32 CIfTcpStmTransTask::PostEvent(
    BufPtr& pBuf )
{
    if( pBuf.IsEmpty() ||
        pBuf->empty() )
        return -EINVAL;

    CRpcServices* pIf = nullptr;
    CCfgOpener oCfg(
        ( IConfigDb* )GetConfig() );

    gint32 ret = oCfg.GetPointer(
        propIfPtr, pIf );

    if( ERROR( ret ) )
        return ret;

    guint8 byToken = 0;
    BufPtr pNewBuf( true );

    ret = RemoteToLocal( pBuf,
        byToken, pNewBuf );

    if( ERROR( ret ) )
        return ret;

    CIfUxRelayTaskHelper oHelper( this );

    switch( byToken )
    {
    case tokPing:
    case tokPong:
        {
            ret = oHelper.ForwardToLocal(
                byToken, pNewBuf );
            break;
        }
    case tokData:
    case tokProgress:
        {
            guint32 dwHdrSize = sizeof( guint8 ) +
                sizeof( guint32 );

            pBuf->SetOffset(
                pBuf->offset() + dwHdrSize );

            ret = oHelper.ForwardToLocal(
                byToken, pBuf );

            break;
        }
    case tokClose:
    case tokFlowCtrl:
    case tokLift:
        {
            break;
        }
    default:
        {
            ret = -EINVAL;
            break;
        }
    }

    if( ret == ERROR_QUEUE_FULL )
    {
        ret = STATUS_PENDING;
        Pause();
    }

    if( ERROR( ret ) )
        return ret;

    oHelper.PostTcpStmEvent(
        byToken, pBuf );

    return ret;
}

CIfUxRelayTaskHelper::CIfUxRelayTaskHelper(
    IEventSink* pTask )
{
    SetTask( pTask );
    if( m_pSvc == nullptr )
    {
        std::string strMsg = DebugMsg( -EFAULT,
            "Error in CIfUxRelayTaskHelper ctor" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CIfUxRelayTaskHelper::SetTask(
    IEventSink* pTask )
{
    if( pTask == nullptr )
        return -EINVAL;

    ObjPtr pIf;
    CCfgOpenerObj oCfg( pTask );
    gint32 ret = oCfg.GetObjPtr(
        propIfPtr, pIf );

    if( ERROR( ret ) )
        return ret;

    m_pSvc = pIf;
    if( m_pSvc == nullptr )
        return -EFAULT;

    return STATUS_SUCCESS;
}

gint32 CIfUxRelayTaskHelper::ForwardToRemote(
    guint8 byToken, BufPtr& pBuf )
{
    gint32 ret = 0;
    if( m_pSvc->IsServer() )
    {
        CUnixSockStmServerRelay*
            pUxStm = ObjPtr( m_pSvc );
        ret = pUxStm->ForwardToRemote(
            byToken, pBuf );
    }
    else
    {
        CUnixSockStmProxyRelay*
            pUxStm = ObjPtr( m_pSvc );
        ret = pUxStm->ForwardToRemote(
            byToken, pBuf );
    }

    return ret;
}

gint32 CIfUxRelayTaskHelper::ForwardToLocal(
    guint8 byToken, BufPtr& pBuf )
{
    gint32 ret = 0;
    if( m_pSvc->IsServer() )
    {
        CUnixSockStmServerRelay*
            pUxStm = ObjPtr( m_pSvc );
        ret = pUxStm->ForwardToLocal(
            byToken, pBuf );
    }
    else
    {
        CUnixSockStmProxyRelay*
            pUxStm = ObjPtr( m_pSvc );
        ret = pUxStm->ForwardToLocal(
            byToken, pBuf );
    }

    return ret;
}

gint32 CIfUxRelayTaskHelper::GetReqToRemote(
    guint8& byToken, BufPtr& pBuf )
{
    gint32 ret = 0;
    if( m_pSvc->IsServer() )
    {
        CUnixSockStmServerRelay*
            pUxStm = ObjPtr( m_pSvc );
        ret = pUxStm->GetReqToRemote(
            byToken, pBuf );
    }
    else
    {
        CUnixSockStmProxyRelay*
            pUxStm = ObjPtr( m_pSvc );
        ret = pUxStm->GetReqToRemote(
            byToken, pBuf );
    }

    return ret;
}

gint32 CIfUxRelayTaskHelper::GetReqToLocal(
    guint8& byToken, BufPtr& pBuf )
{
    gint32 ret = 0;
    if( m_pSvc->IsServer() )
    {
        CUnixSockStmServerRelay*
            pUxStm = ObjPtr( m_pSvc );
        ret = pUxStm->GetReqToLocal(
            byToken, pBuf );
    }
    else
    {
        CUnixSockStmProxyRelay*
            pUxStm = ObjPtr( m_pSvc );
        ret = pUxStm->GetReqToLocal(
            byToken, pBuf );
    }

    return ret;
}

gint32 CIfUxRelayTaskHelper::PostUxSockEvent(
    guint8 byToken, BufPtr& pBuf )
{
    gint32 ret = 0;
    if( m_pSvc->IsServer() )
    {
        CUnixSockStmServerRelay*
            pUxStm = ObjPtr( m_pSvc );
        ret = pUxStm->PostUxSockEvent(
            byToken, pBuf );
    }
    else
    {
        CUnixSockStmProxyRelay*
            pUxStm = ObjPtr( m_pSvc );
        ret = pUxStm->PostUxSockEvent(
            byToken, pBuf );
    }

    return ret;
}

gint32 CIfUxRelayTaskHelper::PostTcpStmEvent(
    guint8 byToken, BufPtr& pBuf )
{
    gint32 ret = 0;
    if( m_pSvc->IsServer() )
    {
        CUnixSockStmServerRelay*
            pUxStm = ObjPtr( m_pSvc );
        ret = pUxStm->PostTcpStmEvent(
            byToken, pBuf );
    }
    else
    {
        CUnixSockStmProxyRelay*
            pUxStm = ObjPtr( m_pSvc );
        ret = pUxStm->PostTcpStmEvent(
            byToken, pBuf );
    }
    return ret;
}


gint32 CIfUxRelayTaskHelper::StartListening(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    if( m_pSvc->IsServer() )
    {
        CUnixSockStmServerRelay*
            pUxStm = ObjPtr( m_pSvc );
        ret = pUxStm->StartListening(
            pCallback );
    }
    else
    {
        CUnixSockStmProxyRelay*
            pUxStm = ObjPtr( m_pSvc );
        ret = pUxStm->StartListening(
            pCallback );
    }
    return ret;
}

gint32 CIfUxRelayTaskHelper::StartReading(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    if( m_pSvc->IsServer() )
    {
        CUnixSockStmServerRelay*
            pUxStm = ObjPtr( m_pSvc );
        ret = pUxStm->StartReading(
            pCallback );
    }
    else
    {
        CUnixSockStmProxyRelay*
            pUxStm = ObjPtr( m_pSvc );
        ret = pUxStm->StartReading(
            pCallback );
    }
    return ret;
}

gint32 CIfUxRelayTaskHelper::WriteStream(
    BufPtr& pBuf, IEventSink* pCallback )
{
    gint32 ret = 0;
    if( m_pSvc->IsServer() )
    {
        CUnixSockStmServerRelay*
            pUxStm = ObjPtr( m_pSvc );
        ret = pUxStm->WriteStream(
            pBuf, pCallback );
    }
    else
    {
        CUnixSockStmProxyRelay*
            pUxStm = ObjPtr( m_pSvc );
        ret = pUxStm->WriteStream(
            pBuf, pCallback );
    }
    return ret;
}

gint32 CIfUxRelayTaskHelper::GetBridgeIf(
    InterfPtr& pIf )
{
    IStream* pStream = nullptr;
    if( m_pSvc->IsServer() )
    {
        CUnixSockStmServerRelay*
            pUxStm = ObjPtr( m_pSvc );
        pStream = pUxStm->GetParent();
    }
    else
    {
        CUnixSockStmProxyRelay*
            pUxStm = ObjPtr( m_pSvc );
        pStream = pUxStm->GetParent();
    }

    CRpcServices* pBridge =
        pStream->GetInterface();

    pIf = ObjPtr( pBridge );

    return 0;
}

gint32 CIfUxRelayTaskHelper::WriteTcpStream(
    gint32 iStreamId,
    CBuffer* pSrcBuf,
    guint32 dwSizeToWrite,
    IEventSink* pCallback )
{
    InterfPtr pIf; 
    gint32 ret = GetBridgeIf( pIf );
    if( ERROR( ret ) )
        return ret;

    if( pIf.IsEmpty() )
        return -EFAULT;

    if( pIf->IsServer() )
    {
        CRpcTcpBridge*
            pBdge = ObjPtr( pIf );

        ret = pBdge->WriteStream(
            iStreamId, pSrcBuf,
            dwSizeToWrite, pCallback );
    }
    else
    {
        CRpcTcpBridgeProxy*
            pBdge = ObjPtr( pIf );

        ret = pBdge->WriteStream(
            iStreamId, pSrcBuf,
            dwSizeToWrite, pCallback );
    }

    return ret;
}

gint32 CIfUxRelayTaskHelper::ReadTcpStream(
    gint32 iStreamId,
    CBuffer* pSrcBuf,
    guint32 dwSizeToWrite,
    IEventSink* pCallback )
{
    InterfPtr pIf; 
    gint32 ret = GetBridgeIf( pIf );
    if( ERROR( ret ) )
        return ret;

    if( pIf.IsEmpty() )
        return -EFAULT;

    BufPtr pBuf( pSrcBuf );
    if( pIf->IsServer() )
    {
        CRpcTcpBridge*
            pBdge = ObjPtr( pIf );

        ret = pBdge->ReadStream(
            iStreamId, pBuf,
            dwSizeToWrite, pCallback );
    }
    else
    {
        CRpcTcpBridgeProxy*
            pBdge = ObjPtr( pIf );

        ret = pBdge->ReadStream(
            iStreamId, pBuf,
            dwSizeToWrite, pCallback );
    }

    return ret;
}

gint32 CIfUxRelayTaskHelper::PauseReading(
    bool bPause )
{
    gint32 ret = 0;
    if( m_pSvc->IsServer() )
    {
        CUnixSockStmServerRelay*
            pUxStm = ObjPtr( m_pSvc );
        ret = pUxStm->PauseReading( bPause );
    }
    else
    {
        CUnixSockStmProxyRelay*
            pUxStm = ObjPtr( m_pSvc );
        ret = pUxStm->PauseReading( bPause );
    }
    return ret;
}
