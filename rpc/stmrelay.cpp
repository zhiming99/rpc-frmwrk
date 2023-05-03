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
#include "stmcp.h"

namespace rpcf
{

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
        CRpcTcpBridge* pBdge = ObjPtr( this );
        ret = pBdge->FetchData_Filter(
            pDataDesc, fd, dwOffset,
            dwSize, pCallback );
        if( ERROR( ret ) )
            break;

        std::string strDest;
        CCfgOpener oDataDesc( pDataDesc );
        ret = oDataDesc.GetStrProp(
            propDestDBusName, strDest );
        if( ERROR( ret ) )
            break;

        CRpcRouterBridge* pRouter =
        static_cast< CRpcRouterBridge* >
                ( GetParent() );

        if( unlikely( pRouter == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }

        IConfigDb* pTransCtx = nullptr;
        ret = oDataDesc.GetPointer(
            propTransCtx, pTransCtx );
        if( ERROR( ret ) )
            break;

        CCfgOpener oTransCtx( pTransCtx );
        std::string strPath;
        ret = oTransCtx.GetStrProp(
            propRouterPath, strPath );
        if( ERROR( ret ) )
            break;

        if( strPath != "/" )
        {
            // should go to the interface
            // CStreamServerRelayMH
            ret = -ENOTSUP;
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
        if( ERROR( ret ) )
            break;

        CParamList oContext;
        oContext.Push( iStmId );
        oContext.Push( ObjPtr( pDataDesc ) );

        fd = -1;
        TaskletPtr pWrapper;
        ret = NEW_PROXY_RESP_HANDLER2(
            pWrapper, ObjPtr( this ),
            &CStreamServerRelay::OnFetchDataComplete,
            pCallback, oContext.GetCfg() );

        if( ERROR( ret ) )
            break;

        oContext.Push( ObjPtr( pWrapper ) );

        // give a valid value just to pass parameter
        // validation
        dwOffset = 0;
        dwSize = 0x20;

        ret = pProxy->FetchData( pDataDesc,
            fd, dwOffset, dwSize, pWrapper );

        if( ret == STATUS_PENDING )
            break;

        if( ERROR( ret ) )
        {
            ( *pWrapper )( eventCancelTask );
            break;
        }

        ret = pWrapper->GetError();
        if( ret != STATUS_PENDING )
        {
            // the wrapper task has been completed
            // somewhere else, the callback will be
            // called when we return.
            ret = STATUS_PENDING;
            break;
        }

        //how can we be here?
        ( *pWrapper )( eventCancelTask );
        oContext.ClearParams();
        oContext.Push( iStmId );
        oContext.Push( ObjPtr( pDataDesc ) );

        // the wrapper task has not run yet
        // though across the process boundary.
        CParamList oResp;
        oResp[ propReturnValue ] = 0;
        oResp.Push( ObjPtr( pDataDesc ) );
        oResp.Push( ( guint32 )fd );
        oResp.Push( dwOffset );
        oResp.Push( dwSize );

        TaskletPtr pDummy;
        pDummy.NewObj( clsid( CIfDummyTask ) );
        CCfgOpener oCfg(
            ( IConfigDb*)pDummy->GetConfig() );
        oCfg.SetPointer( propRespPtr,
            ( IConfigDb* )oResp.GetCfg() );

        TaskletPtr pTask;
        ret = DEFER_IFCALL_NOSCHED(
            pTask, ObjPtr( this ), 
            &CStreamServerRelay::OnFetchDataComplete,
            pCallback, ( IEventSink* )pDummy, 
            oContext.GetCfg() );
        if( ERROR( ret ) )
            break;

        CIoManager* pMgr = GetIoMgr();
        ret = pMgr->RescheduleTask( pTask );
        if( ret == 0 )
            ret = STATUS_PENDING;

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
    CParamList oContext( pContext );

    do{
        if( pContext == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ret = oContext.GetIntProp( 0,
            ( guint32& )iStmId );
        if( ERROR( ret ) )
            break;

        CCfgOpenerObj oReq( pIoReq );
        IConfigDb* pResp = nullptr;
        ret = oReq.GetPointer(
            propRespPtr, pResp );
        if( ERROR( ret ) )
            break;

        CCfgOpener oResp( pResp );
        gint32 iRet = 0;
        ret = oResp.GetIntProp( propReturnValue,
            ( guint32& ) iRet );

        if( ERROR( ret ) )
            break;

        if( ERROR( iRet ) )
        {
            ret = iRet;
            break;
        }

        ret = oResp.GetIntProp(
            1, ( guint32& )iFd );
        if( ERROR( ret ) )
            break;

        IConfigDb* pDataDesc = nullptr;
        ret = oResp.GetPointer( 0, pDataDesc );
        if( ERROR( ret ) )
            break;

        ObjPtr pscp;
        CStmConnPoint* pStmCp = nullptr;
        CCfgOpener oDesc( pDataDesc );
        if( pDataDesc->exist( propStmConnPt ) )
        {
            guint64 qwStmCp = 0;
            ret = oDesc.GetQwordProp(
                propStmConnPt, qwStmCp );
            if( ERROR( ret ) )
                break;
            pStmCp = reinterpret_cast
                < CStmConnPoint* >( qwStmCp );
            pscp = pStmCp;
            pStmCp->Release();
        }

        // we need to do the following things
        // before reponse to the remote client
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


        if( !pscp.IsEmpty() )
        {
            oDesc.RemoveProperty( propStmConnPt );
            oUxIf.SetObjPtr( propStmConnPt, pscp );
        }

        // use CIfUxListeningRelayTask to
        // receive incoming stream as well as
        // events.
        oUxIf.SetBoolProp( propListenOnly, true );

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
        oContext.ClearParams();

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

        CCfgOpener oCtx( pContext );
        ret = oCtx.GetIntProp(
            1, ( guint32& )iStmId );
        if( ERROR( ret ) )
            break;

        CCfgOpenerObj oIoReq( pIoReqTask );
        IConfigDb* pResp = nullptr;
        ret = oIoReq.GetPointer(
            propRespPtr, pResp );
        if( ERROR( ret ) )
            break;

        CCfgOpener oResp( pResp );
        gint32 iRet = 0;
        ret = oResp.GetIntProp( propReturnValue,
            ( guint32& )iRet );
        if( ERROR( ret ) )
            break;

        if( ERROR( iRet ) )
        {
            ret = iRet;
            break;
        }

        IConfigDb* pDataDesc = nullptr;
        ret = oResp.GetPointer( 0, pDataDesc );
        if( ERROR( ret ) )
            break;

        ObjPtr pStmCp;
        if( !pContext->exist( propStmConnPt ) )
        {
            // time to create uxstream object
            ret = CreateSocket( iLocal, iRemote );
            if( ERROR( ret ) )
                break;
        }
        else
        {
            ret = oCtx.GetObjPtr(
                propStmConnPt, pStmCp );
            if( ERROR( ret ) )
                break;
            CObjBase* pObj = pStmCp;
            CCfgOpener oDesc( pDataDesc );
            oDesc.SetQwordProp(
                propStmConnPt, ( guint64 )pObj );
            pObj->AddRef();
        }

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

        oUxIf.SetBoolProp( propListenOnly, true );
        if( !pStmCp.IsEmpty() )
            oUxIf.SetObjPtr( propStmConnPt, pStmCp );

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

        ret = this->AddSeqTask( pStartTask );
        
    }while( 0 );

    if( ERROR( ret ) )
    {
        CParamList oResp;
        oResp[ propReturnValue ] = ret;

        CCfgOpenerObj oTaskCfg(
            ( CObjBase* )pCallback );

        oTaskCfg.SetObjPtr( propRespPtr,
            ObjPtr( oResp.GetCfg() ) );

        if( iStmId > 0 )
        {
            CRpcTcpBridgeProxy* pProxy =
                ObjPtr( this );

            pProxy->CloseLocalStream(
                nullptr, iStmId );
        }
        // complete the invoke task
        pCallback->OnEvent( eventTaskComp,
            ret, 0, 0 );

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

        gint32 iRet = 0;
        CCfgOpener oResp( pResp );
        ret = oResp.GetIntProp(
            propReturnValue, ( guint32& )iRet );
        if( ERROR( ret ) )
            break;

        if( ERROR( iRet ) )
        {
            ret = iRet;
            break;
        }

        ret = oResp.GetIntProp(
            0, ( guint32& )iStmId );
        if( ERROR( ret ) )
            break;

        guint32 dwProtocol;
        ret = oResp.GetIntProp( 1, dwProtocol );
        if( ERROR( ret ) )
            break;

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
        ret = NEW_PROXY_RESP_HANDLER2(
            pWrapper, ObjPtr( this ),
            &CStreamProxyRelay::OnFetchDataComplete,
            pCallback, oContext.GetCfg() );

        if( ERROR( ret ) )
            break;

        guint32 dwSize = 0x20;
        guint32 dwOffset = 0;
        ret = pProxy->super::FetchData_Proxy(
            pDataDesc, iStmId, dwOffset, dwSize,
            pWrapper );

        if( ERROR( ret ) )
        {
            ( *pWrapper )( eventCancelTask );
            break;
        }

        if( ret == STATUS_PENDING )
        {
            // retire this task
            ret = 0;
            break;
        }

        // immediate return
        CIfParallelTask* pPara = pWrapper;
        ret = pPara->GetError();
        if( ret != STATUS_PENDING )
        {
            // already completed
            break;
        }

        // the response handler was not called
        // and lets' call it here
        CParamList oFdResp;
        oFdResp[ propReturnValue ] = 0;
        oFdResp.Push( ObjPtr( pDataDesc ) );
        oFdResp.Push( ( guint32 )iStmId );
        oFdResp.Push( dwOffset );
        oFdResp.Push( dwSize );

        TaskletPtr pDummy;
        pDummy.NewObj( clsid( CIfDummyTask ) );
        CCfgOpener oCfg(
            ( IConfigDb*)pDummy->GetConfig() );
        oCfg.SetPointer( propRespPtr,
            ( IConfigDb* )oFdResp.GetCfg() );

        IEventSink* pIoReq = pDummy;
        CIfResponseHandler* pHandler = pWrapper;
        pHandler->OnEvent( eventTaskComp,
            STATUS_SUCCESS, 0, ( LONGWORD* )pIoReq  );

        // retire this task
        ret = STATUS_SUCCESS;

    }while( 0 );

    if( ERROR( ret ) )
    {
        // complete the invoke task from the
        // reqfwdr
        CParamList oResp;
        oResp[ propReturnValue ] = ret;

        CCfgOpenerObj oTaskCfg(
            ( CObjBase* )pCallback );

        oTaskCfg.SetObjPtr( propRespPtr,
            ObjPtr( oResp.GetCfg() ) );

        if( iStmId > 0 )
        {
            CRpcTcpBridgeProxy* pProxy =
                ObjPtr( this );

            pProxy->CloseLocalStream(
                nullptr, iStmId );
        }
        pCallback->OnEvent( eventTaskComp,
            ret, 0, 0 );
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

    gint32 ret = 0;
    CParamList oContext;

    do{
        CRpcTcpBridgeProxy* pProxy = ObjPtr( this );
        if( unlikely( pProxy == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }

        ObjPtr pscp;
        if( pDataDesc->exist( propStmConnPt ) )
        {
            guint64 qwStmCp;
            CCfgOpener oDesc( pDataDesc );
            oDesc.GetQwordProp(
                propStmConnPt, qwStmCp );
            CStmConnPoint* pStmCp = reinterpret_cast
                < CStmConnPoint* >( qwStmCp );
            pscp = pStmCp;
            pStmCp->Release();
            oDesc.RemoveProperty( propStmConnPt );
        }

        gint32 iStmId = -1;
        oContext.Push( ObjPtr( pDataDesc ) );
        if( !pscp.IsEmpty() )
        {
            oContext.SetObjPtr(
                propStmConnPt, pscp );
        }

        TaskletPtr pWrapper;
        ret = NEW_PROXY_RESP_HANDLER2(
            pWrapper, ObjPtr( this ),
            &CStreamProxyRelay::OnOpenStreamComplete,
            pCallback, oContext.GetCfg() );

        if( ERROR( ret ) )
            break;

        ret = pProxy->OpenStream(
            protoStream, iStmId, pWrapper );

        if( ret == STATUS_PENDING )
            break;

        if( ERROR( ret ) )
        {
            ( *pWrapper )( eventCancelTask );
            break;
        }

        ret = pWrapper->GetError();
        if( ERROR( ret ) )
            break;

        if( ret != STATUS_PENDING )
        {
            // some other guy is doing the work
            ret = STATUS_PENDING;
            break;
        }

        ( *pWrapper )( eventCancelTask );
        // ghost event, sometimes, it can complete
        // immediately, though across the network. But
        // we cannot return status_success directly
        // since we have not create the local stream
        // sock yet.

        DebugPrintEx( logCrit, ret,
            "%s: catched an immediate return",
            __func__ );

        CParamList oResp;
        oResp[ propReturnValue ] = 0;
        oResp.Push( iStmId );
        oResp.Push( ( guint32 )protoStream );

        TaskletPtr pDummy;
        pDummy.NewObj( clsid( CIfDummyTask ) );
        CCfgOpener oCfg(
            ( IConfigDb*)pDummy->GetConfig() );
        oCfg.SetPointer( propRespPtr,
            ( IConfigDb* )oResp.GetCfg() );

        TaskletPtr pTask;
        ret = DEFER_IFCALL_NOSCHED(
            pTask, ObjPtr( this ), 
            &CStreamProxyRelay::OnOpenStreamComplete,
            pCallback, ( IEventSink* )pDummy, 
            oContext.GetCfg() );
        if( ERROR( ret ) )
            break;

        CIoManager* pMgr = GetIoMgr();
        ret = pMgr->RescheduleTask( pTask );
        if( SUCCEEDED( ret ) )
            ret = STATUS_PENDING;

    }while( 0 );

    if( ret != STATUS_PENDING )
        oContext.ClearParams();

    return ret;
}

gint32 CIfStartUxSockStmRelayTask::RunTask()
{
    gint32 ret = 0;

    do{
        ObjPtr pIf;
        CParamList oParams(
            ( IConfigDb* )GetConfig() );

        ret = oParams.GetObjPtr( 0, pIf );
        if( ERROR( ret ) )
            break;

        CRpcServices* pSvc = pIf;
        if( pSvc == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ret = pSvc->StartEx( this );
        if( ERROR( ret ) )
            break;

        if( ret == STATUS_PENDING )
            break;

    }while( 0 );

    if( ret == STATUS_PENDING )
        return ret;

    OnTaskComplete( ret );

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
        ObjPtr pIf;
        ret = oParams.GetObjPtr( propIfPtr, pIf );
        if( ERROR( ret ) )
            break;
        
        oParams.GetIntProp(
            2, ( guint32& )iStmId ) ;

        pParentIf = pIf;
        CRpcServices* pParent = pIf;

        IStream* pStream = nullptr;
        if( pParent->IsServer() )
        {
            CStreamServerRelay* pstm = pIf;
            pStream = pstm;
        }
        else
        {
            CStreamProxyRelay* pstm = pIf;
            pStream = pstm;
        }

        if( pStream == nullptr ||
            pParent == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        if( ERROR( iRet ) )
        {
            ret = iRet;
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
            oResp.SetBoolProp( propNonFd, true );
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

        // start the stream readers
        gint32 ( *func )( CRpcServices*, CRpcServices*, gint32 ) =
            ([]( CRpcServices* pUxIf,
                CRpcServices* pParent,
                gint32 iStmId )->gint32
        {
            gint32 ret = 0;
            if( pUxIf == nullptr || pParent == nullptr )
                return -EINVAL;
            do{
                ObjPtr pObj = pUxIf;
                if( pUxIf->IsServer() )
                {
                    CUnixSockStmServerRelay* pSvc = pObj;
                    ret = pSvc->OnPostStartDeferred( nullptr );
                }
                else
                {
                    CUnixSockStmProxyRelay* pSvc = pObj;
                    ret = pSvc->OnPostStartDeferred( nullptr );
                }
                if( SUCCEEDED( ret ) )
                    break;

                pObj = pParent;
                if( pParent->IsServer() )
                {
                    CStreamServerRelay* pSvc = pObj;
                    pSvc->OnChannelError( iStmId, ret );
                }
                else
                {
                    HANDLE hstm = ( HANDLE )pUxIf;
                    CStreamProxyRelay* pSvc = pObj;
                    pSvc->OnChannelError( hstm, ret );
                }

            }while( 0 );
            return ret;
        });

        auto pMgr = pParent->GetIoMgr();
        TaskletPtr pListenTask;
        ret = NEW_FUNCCALL_TASK( pListenTask,
            pMgr, func, pUxSvc, pParent, iStmId );
        if( ERROR( ret ) )
            break;

        ret = pMgr->RescheduleTask( pListenTask );
        if( ERROR( ret ) )
            ( *pListenTask )( eventCancelTask );

    }while( 0 );

    while( ERROR( ret ) )
    {
        if( pParentIf.IsEmpty() )
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

            pStream->OnClose( iStmId );
        }
        else
        {
            if( pUxSvc == nullptr )
                break;

            CStreamProxyRelay* pStream =
                ObjPtr( pParent );

            if( unlikely( pStream == nullptr ) )
                break;

            pStream->OnClose( ( HANDLE )pUxSvc );
        }

        break;
    }
    if( ERROR( iRet ) )
    {
        oResp[ propReturnValue ] = iRet;
        ret = iRet;
    }
    else if( ERROR( ret ) )
    {
        oResp[ propReturnValue ] = ret;
        iRet = ret;
    }

    bool bClosed = false;
    EventPtr pEvt;
    ret = GetInterceptTask( pEvt );
    if( SUCCEEDED( ret ) )
    {
        CCfgOpenerObj oCfg( ( IEventSink* )pEvt );
        oCfg.SetPointer( propRespPtr,
            ( CObjBase* )oResp.GetCfg() );

        pEvt->OnEvent( eventTaskComp, iRet, 0, 0 );
        if( SUCCEEDED( iRet ) )
            bClosed = true;
    }

    if( !bClosed && iRemote > 0 )
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
        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );

        CIfUxRelayTaskHelper oHelper( this );
        ret = oHelper.StartListening( this );

        if( ret == STATUS_PENDING )
            break;

        ret = OnTaskComplete( ret );
        if( ERROR( ret ) )
            break;

    }while( 1 );

    return ret;
}

gint32 CIfUxListeningRelayTask::OnTaskComplete(
    gint32 iRet )
{
    gint32 ret = 0;

    do{
        if( ERROR( iRet ) )
        {
            ret = iRet;
            break;
        }
        IConfigDb* pCfg = GetConfig();
        CCfgOpener oCfg( pCfg );

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

    if( ERROR( ret ) )
    {
        DebugPrint( ret, "Error, the relay "
            "channel will be closed from "
            "OnTaskComplete" );
        BufPtr pCloseBuf( true );
        *pCloseBuf = tokClose;
        PostEvent( pCloseBuf );
    }

    return ret;
}

gint32 CIfUxListeningRelayTask::PostEvent(
    BufPtr& pBuf )
{
    if( pBuf.IsEmpty() ||
        pBuf->empty() )
        return -EINVAL;

    CCfgOpener oCfg(
        ( IConfigDb* )GetConfig() );

    gint32 ret = 0;
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
    case tokData:
        {
            guint32 dwSize = pBuf->size() -
                UXPKT_HEADER_SIZE;
            oHelper.IncRxBytes( dwSize );
            // fall through
        }
    case tokProgress:
        {
            pBuf->SetOffset( pBuf->offset() +
                UXPKT_HEADER_SIZE ); 
            pNewBuf = pBuf;
            ret = oHelper.ForwardToRemote(
                byToken, pNewBuf );
            break;
        }
    case tokError:
        {
            pNewBuf = pBuf;
            pNewBuf->SetOffset(
                pBuf->offset() + 1 );
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

    oHelper.PostUxSockEvent(
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

        ret = pIrp->GetStatus();
        if( ERROR( ret ) )
        {
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

        if( ret == STATUS_PENDING )
            break;

        oCfg.RemoveProperty( propIrpPtr );
        oCfg.RemoveProperty( propRespPtr );

        ret = ReRun();
        if( ERROR( ret ) )
            break;

        ret = STATUS_PENDING;

    }while( 0 );
    if( ERROR( ret ) )
    {
        DebugPrint( ret, "Error, the relay "
            "channel will be closed from "
            "OnIrpComplete" );
        BufPtr pCloseBuf( true );
        *pCloseBuf = tokClose;
        PostEvent( pCloseBuf );
    }
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

        if( ret == STATUS_PENDING )
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

            ret = OnTaskComplete( ret );
        }
        else
        {
            guint8 byToken = 0;
            BufPtr pPayload;
            ret = oHelper.PeekReqToLocal(
                byToken, pPayload );

            if( ret == -ENOENT )
            {
                ret = STATUS_PENDING;
                Pause();
                break;
            }

            switch( byToken )
            {
            case tokData:
                {
                    ret = oHelper.WriteStream(
                        pPayload, this );

                    break;
                }
            case tokPing:
            case tokPong:
                {
                    ret = oHelper.SendPingPong(
                        byToken, this );
                    break;
                }
            case tokProgress:
                {
                    ret = oHelper.SendProgress(
                        pPayload, this );
                    break;
                }
            default:
                {
                    ret = -EBADMSG;
                    break;
                }
            }

            // note that this flow control comes from
            // uxport rather than the uxstream.
            if( ERROR_QUEUE_FULL == ret )
            {
                Pause();
                ret = STATUS_PENDING;
                break;
            }

            // remove the packet from the queue
            BufPtr pBuf;
            guint8 byToken2 = 0;
            gint32 iRet = oHelper.GetReqToLocal(
                byToken2, pBuf );

            if( ERROR( iRet ) && SUCCEEDED( ret ) )
                ret = iRet;

            else if( byToken != byToken2 )
                ret = -EBADMSG;

            else if( byToken == tokData ||
                byToken == tokProgress )
            {
                if( !( pBuf == pPayload ) )
                    ret = -EBADMSG;
            }
            if( ERROR( ret ) )
                break;
        }

        if( ERROR( ret ) )
            break;

        if( ret == STATUS_PENDING )
            break;

    }while( 1 );

    return ret;
}

gint32 CIfUxSockTransRelayTask::Pause()
{
    if( IsPaused() )
        return 0;

    CIfUxRelayTaskHelper oHelper( this );

    // stop watching the uxsock and uxsock's buffer
    // will do the rest work
    if( IsReading() )
        oHelper.PauseReading( true );
    else
    {
        // remove the irp which is still in the
        // port driver.
        ObjPtr pIrpObj;

        CStdRTMutex oTaskLock( GetLock() );
        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );

        gint32 ret = oCfg.GetObjPtr(
            propIrpPtr, pIrpObj );

        if( SUCCEEDED( ret ) )
        {
            oCfg.RemoveProperty( propIrpPtr );

            IrpPtr pIrp = pIrpObj;
            CStdRMutex oIrpLock( pIrp->GetLock() );
            ret = pIrp->CanContinue(
                IRP_STATE_READY );

            if( SUCCEEDED( ret ) )
            {
                // remove the callback of this task.
                pIrp->RemoveCallback();
            }
        }
    }

    return super::Pause();
}

gint32 CIfUxSockTransRelayTask::Resume()
{
    if( !IsPaused() )
        return 0;

    CIfUxRelayTaskHelper oHelper( this );
    // resume watching the uxsock
    if( IsReading() )
        oHelper.PauseReading( false );

    return super::Resume();
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

    if( !IsReading() )
        return ret;

    BufPtr pPayload( true );
    if( ERROR( ret ) )
    {
        // return error to close the stream
        if( ret == ERROR_PORT_STOPPED )
        {
            *pPayload = tokClose;
        }
        else
        {
            *pPayload = tokError;
            gint32 iRet = htonl( ret );
            memcpy( pPayload->ptr() + 1,
                &iRet, sizeof( iRet ) );
        }
    }
    else
    {
        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        pPayload = pCtx->m_pRespData;
    }

    ret = PostEvent( pPayload );

    return ret;
}

gint32 CIfTcpStmTransTask::OnIrpComplete(
    IRP* pIrp )
{
    gint32 ret = 0;

    do{
        if( !IsReading() && IsPaused() )
        {
            // keep the context and quit
            // immediately till the task is
            // resumed
            ret = STATUS_PENDING;
            break;
        }

        RemoveProperty( propIrpPtr );
        RemoveProperty( propRespPtr );

        ret = HandleIrpResp( pIrp );
        if( ERROR( ret ) )
            break;

        if( ret == STATUS_PENDING )
            break;

        ret = ReRun();
        if( ERROR( ret ) )
            break;

        ret = STATUS_PENDING;

    }while( 0 );

    return ret;
}

gint32 CIfTcpStmTransTask::OnComplete(
    gint32 iRet )
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
            BufPtr pBuf;
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
    BufPtr pBuf = pLocal;
    guint32 dwHdrSize = UXPKT_HEADER_SIZE;

    switch( byToken )
    {
    case tokData:
    case tokProgress:
        {
            if( pBuf.IsEmpty() ||
                pBuf->empty() )
            {
                ret = -EINVAL;
                break;
            }

            guint32 dwSize = htonl( pBuf->size() );
            if( pBuf->offset() >= dwHdrSize )
            {
                // add the header ahead of the payload
                pBuf->SetOffset(
                    pBuf->offset() - dwHdrSize );

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
            pRemote->Resize( sizeof( byToken ) );
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

    guint32 dwHdrSize = UXPKT_HEADER_SIZE;

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

    CCfgOpener oCfg(
        ( IConfigDb* )GetConfig() );

    guint8 byToken = 0;
    BufPtr pNewBuf( true );

    gint32 ret = RemoteToLocal(
        pBuf, byToken, pNewBuf );

    if( ERROR( ret ) )
        return ret;

    CIfUxRelayTaskHelper oHelper( this );

    switch( byToken )
    {
    case tokData:
        {
            guint32 dwSize = pNewBuf->size();
            oHelper.IncTxBytes( dwSize );
            ret = oHelper.ForwardToLocal(
                byToken, pNewBuf );
            break;
        }
    case tokPing:
    case tokPong:
    case tokProgress:
        {
            ret = oHelper.ForwardToLocal(
                byToken, pNewBuf );
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
        byToken, pNewBuf );

    return ret;
}

gint32 CIfTcpStmTransTask::Pause()
{
    gint32 ret = 0;
    if( IsPaused() )
        return ret;

    do{
        if( IsReading() )
        {
            // cancel the irp immediately if any
            ret = CIfUxTaskBase::Pause();
            break;
        }
        else
        {
            // let the ongoing irp to proceed
            // till it is completed
            CStdRTMutex oTaskLock( GetLock() );
            guint32 dwState = GetTaskState();
            if( dwState != stateStarted )
            {
                ret = ERROR_STATE;
                break;
            }

            if( IsPaused() )
                break;

            m_dwFCState = statePaused;

            // reset the task state
            SetTaskState( stateStarting );

            // don't cancel the irp
            CCfgOpener oCfg(
                ( IConfigDb* )GetConfig() );

            IRP* pIrp = nullptr;
            gint32 ret = oCfg.GetPointer(
                propIrpPtr, pIrp );

            if( ERROR( ret ) )
                break;

            CStdRMutex oIrpLock(
                pIrp->GetLock() );

            ret = pIrp->CanContinue(
                IRP_STATE_READY );

            if( SUCCEEDED( ret ) )
                pIrp->RemoveCallback();

            oCfg.RemoveProperty( propIrpPtr );
        }

    }while( 0 );

    return ret;
}

gint32 CIfTcpStmTransTask::ResumeWriting()
{
    gint32 ret = 0;
    do{
        // let the ongoing irp to proceed
        // till it is completed
        CStdRTMutex oTaskLock( GetLock() );
        EnumTaskState dwState = GetTaskState();
        if( IsStopped( dwState ) )
        {
            ret = ERROR_STATE;
            break;
        }

        if( !IsPaused() )
            break;

        m_dwFCState = stateStarted;

        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );

        CIoManager* pMgr = nullptr;
        ret = GET_IOMGR( oCfg, pMgr );
        if( ERROR( ret ) )
            break;

        PIRP pIrp = nullptr;
        gint32 ret = oCfg.GetPointer(
            propIrpPtr, pIrp );

        if( ERROR( ret ) )
        {
            TaskletPtr pTask( ( CTasklet* )this );
            // ret = pMgr->RescheduleTask( pTask );
            // the irp is completed but not
            // handled yet
            ret = DEFER_CALL( pMgr, this,
                &IEventSink::OnEvent,
                eventZero, 0, 0, nullptr );
        }
        else
        {
            LONGWORD irpPtr = ( LONGWORD )pIrp;
            CStdRMutex oIrpLock( pIrp->GetLock() );
            ret = pIrp->CanContinue(
                IRP_STATE_READY );
            if( SUCCEEDED( ret ) )
            {
                // the irp is still pending
                // nothing to do
                break;
            }

            // the irp is completed but not
            // handled yet
            ret = DEFER_CALL( pMgr, this,
                &IEventSink::OnEvent,
                eventIrpComp, irpPtr,
                0, nullptr );
        }

    }while( 0 );

    return ret;
}

gint32 CIfTcpStmTransTask::Resume()
{
    gint32 ret = 0;
    if( !IsPaused() )
        return 0;

    do{
        if( IsReading() )
        {
            ret = CIfUxTaskBase::Resume();
        }
        else
        {
            ret = ResumeWriting();
        }

    }while( 0 );

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

gint32 CIfUxRelayTaskHelper::PeekReqToLocal(
    guint8& byToken, BufPtr& pBuf )
{
    gint32 ret = 0;
    if( m_pSvc->IsServer() )
    {
        CUnixSockStmServerRelay*
            pUxStm = ObjPtr( m_pSvc );
        ret = pUxStm->PeekReqToLocal(
            byToken, pBuf );
    }
    else
    {
        CUnixSockStmProxyRelay*
            pUxStm = ObjPtr( m_pSvc );
        ret = pUxStm->PeekReqToLocal(
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
        ret = pUxStm->OnUxSockEvent(
            byToken, pBuf );
    }
    else
    {
        CUnixSockStmProxyRelay*
            pUxStm = ObjPtr( m_pSvc );
        ret = pUxStm->OnUxSockEvent(
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
        ret = pUxStm->OnTcpStmEvent(
            byToken, pBuf );
    }
    else
    {
        CUnixSockStmProxyRelay*
            pUxStm = ObjPtr( m_pSvc );
        ret = pUxStm->OnTcpStmEvent(
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

gint32 CIfUxRelayTaskHelper::SendProgress(
    CBuffer* pBuf, IEventSink* pCallback )
{
    gint32 ret = 0;
    if( m_pSvc->IsServer() )
    {
        CUnixSockStmServerRelay*
            pUxStm = ObjPtr( m_pSvc );
        ret = pUxStm->SendProgress(
            pBuf, pCallback );
    }
    else
    {
        CUnixSockStmProxyRelay*
            pUxStm = ObjPtr( m_pSvc );
        ret = pUxStm->SendProgress(
            pBuf, pCallback );
    }
    return ret;
}

gint32 CIfUxRelayTaskHelper::SendPingPong(
    guint8 byToken, IEventSink* pCallback )
{
    gint32 ret = 0;
    if( m_pSvc->IsServer() )
    {
        CUnixSockStmServerRelay*
            pUxStm = ObjPtr( m_pSvc );
        ret = pUxStm->SendPingPong(
            byToken, pCallback );
    }
    else
    {
        CUnixSockStmProxyRelay*
            pUxStm = ObjPtr( m_pSvc );
        ret = pUxStm->SendPingPong(
            byToken, pCallback );
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
    BufPtr& pSrcBuf,
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

        ret = pBdge->ReadStream(
            iStreamId, pSrcBuf,
            dwSizeToWrite, pCallback );
    }
    else
    {
        CRpcTcpBridgeProxy*
            pBdge = ObjPtr( pIf );

        ret = pBdge->ReadStream(
            iStreamId, pSrcBuf,
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

EnumFCState CIfUxRelayTaskHelper::IncRxBytes(
    guint32 dwSize )
{
    EnumFCState ret;
    if( m_pSvc->IsServer() )
    {
        CUnixSockStmServerRelay*
            pUxStm = ObjPtr( m_pSvc );
        ret = pUxStm->IncRxBytes( dwSize );
    }
    else
    {
        CUnixSockStmProxyRelay*
            pUxStm = ObjPtr( m_pSvc );
        ret = pUxStm->IncRxBytes( dwSize );
    }
    return ret;
}

EnumFCState CIfUxRelayTaskHelper::IncTxBytes(
    guint32 dwSize )
{
    EnumFCState ret;
    if( m_pSvc->IsServer() )
    {
        CUnixSockStmServerRelay*
            pUxStm = ObjPtr( m_pSvc );
        ret = pUxStm->IncTxBytes( dwSize );
    }
    else
    {
        CUnixSockStmProxyRelay*
            pUxStm = ObjPtr( m_pSvc );
        ret = pUxStm->IncTxBytes( dwSize );
    }
    return ret;
}

gint32 CUnixSockStmServerRelay::OnPingPong( bool bPing )
{
    BufPtr pBuf( true );
    return SendUxStreamEvent(
        bPing ? tokPing : tokPong,
        pBuf );
}

gint32 CUnixSockStmServerRelay::OnPingPongRemote( bool bPing )
{
    BufPtr pBuf( true );
    return SendBdgeStmEvent(
        bPing ? tokPing : tokPong,
        pBuf );
}

IStream* CUnixSockStmServerRelay::GetParent()
{
    CStreamProxyRelay* pIf =
        ObjPtr( m_pParent );
    return ( IStream* )pIf;
}

gint32 CUnixSockStmProxyRelay::OnPingPong( bool bPing )
{
    BufPtr pBuf( true );
    return SendUxStreamEvent(
        bPing ? tokPing : tokPong,
        pBuf );
}

gint32 CUnixSockStmProxyRelay::OnPingPongRemote( bool bPing )
{
    BufPtr pBuf( true );
    return SendBdgeStmEvent(
        bPing ? tokPing : tokPong,
        pBuf );
}

IStream* CUnixSockStmProxyRelay::GetParent()
{
    CStreamServerRelay* pIf =
        ObjPtr( m_pParent );
    return ( IStream* )pIf;
}

gint32 CUnixSockStmProxyRelay::OnDataReceivedRemote(
    CBuffer* pBuf )
{ 
    guint64 qwBytes = 0, qwPkts = 0;
    do{
        bool ret = m_oFlowCtrl.GetOvershoot(
            qwBytes, qwPkts );
        if( !ret )
            break;

        if( qwPkts == 0 && qwBytes == 0 )
            break;

        if( ( qwPkts > 0 &&
            qwPkts <= ( STM_MAX_PACKETS_REPORT >> 3 ) ) &&
            ( qwBytes > 0 &&
                qwBytes <= ( STM_MAX_PENDING_WRITE >> 8 ) ) )
            break;

        BufPtr ptrBuf( true );
        gint32 iRet = htonl( -EPROTO );
        *ptrBuf = ( guint8 )tokError;
        ptrBuf->Append( ( guint8* )&iRet, sizeof( iRet ) );
        PostUxStreamEvent( tokError, ptrBuf );
        DebugPrintEx( logErr, -EPROTO,
            "Error received unwanted packets/bytes" );

    }while( 0 );

    return 0;
}

}
