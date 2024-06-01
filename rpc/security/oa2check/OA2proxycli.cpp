// GENERATED BY RIDLC, MAKE SURE TO BACKUP BEFORE RUNNING RIDLC AGAIN
// Copyright (C) 2024  zhiming <woodhead99@gmail.com>
// This program can be distributed under the terms of the GNU GPLv3.
// ridlc -O . ../oaproxy.ridl 
// Implement the following methods
// to get the RPC proxy/server work
#include "rpc.h"
using namespace rpcf;
#include "oa2check.h"
#include "OA2proxycli.h"

FactoryPtr OA2CheckClassFactory()
{
    BEGIN_FACTORY_MAPS;

    INIT_MAP_ENTRYCFG( COA2proxy_CliImpl );
    
    INIT_MAP_ENTRY( TIMESTAMP );
    INIT_MAP_ENTRY( USER_INFO );
    INIT_MAP_ENTRY( OA2EVENT );
    
    END_FACTORY_MAPS;
}

COA2proxy_CliImpl( const IConfigDb* pCfg ) :
    super::virtbase( pCfg ), super( pCfg )
{
    gint32 ret = 0;
    do{
        SetClassId( clsid(COA2proxy_CliImpl ) );
        CCfgOpener oCfg( pCfg );
        CRpcServices* pSvc = nullptr;
        ret = oCfg.GetPointer(
            propRouterPtr, pSvc );
        if( ERROR( ret ) )
            break;
        m_pRouter = pSvc;

    }while( 0 );
    if( ERROR( ret ) )
    {
        stdstr strMsg = DebugMsg( ret,
            "Error in COA2proxy_CliImpl ctor" );
        throw new std::runtime_error( strMsg );
    }
}

// OAuth2Proxy Proxy
/* Async Req Complete Handler*/
gint32 COA2proxy_CliImpl::DoLoginCallback(
    IConfigDb* context, gint32 iRet,
    USER_INFO& ui /*[ In ]*/ )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCtx( context );
        ret = oCtx.GetPointer(
            propEventSink, pCb );
        if( ERROR( ret ) )
            break;
        CParamList oResp;
        oResp.SetIntProp(
            propReturnValue, iRet );
        if( SUCCEEDED( iRet ) )
        {
            ObjPtr pObj;
            pObj.NewObj( clsid( USER_INFO ) );
            USER_INFO* pui = pObj;
            // ui is allocated on the stack cannot be
            // passed on, make a copy on the heap
            *pui = ui;
            oResp.Push( pObj );
            pObj = oResp.GetCfg();
            Variant oVar( pObj );
            pCb->SetProperty(
                propRespPtr, oVar );
        }

        pCb->OnEvent( eventTaskComp,
            iRet, 0, nullptr );
    }while( 0 );
    return 0;
}

/* Async Req Complete Handler*/
gint32 COA2proxy_CliImpl::GetUserInfoCallback(
    IConfigDb* context, gint32 iRet,
    USER_INFO& ui /*[ In ]*/ )
{
    // TODO: Process the server response here
    // return code ignored
    return 0;
}

/* Async Req Complete Handler*/
gint32 COA2proxy_CliImpl::RevokeUserCallback(
    IConfigDb* context, gint32 iRet )
{
    // TODO: Process the server response here
    // return code ignored
    return 0;
}

/* Event Handler*/
gint32 COA2proxy_CliImpl::OnOA2Event(
    OA2EVENT& oEvent /*[ In ]*/ )
{
    // TODO: Processing the event here
    // return code ignored
    return 0;
}

gint32 COA2proxy_CliImpl::AddSession(
    guint32 dwPortId, 
    const std::string& strSess )
{
    m_mapPortId2Sess[ dwPortId ] = strSess;
    m_mapSess2PortId[ strSess ] = dwPortId;
    return STATUS_SUCCESS;
}

gint32 COA2proxy_CliImpl::RemoveSession(
    guint32 dwPortId )
{
    // the caller makes sure the gss lock is
    // acquired.
    auto itr = m_mapSessions.find( dwPortId);
    if( itr != m_mapSessions.end() )
        m_mapSessions.erase( itr );

    auto itr1 =
        m_mapPortId2Sess.find( dwPortId );
    if( itr1 != m_mapPortId2Sess.end() ) 
    {
        std::string strSess = itr1->second;
        m_mapPortId2Sess.erase( itr1 );
        m_mapSess2PortId.erase( strSess );
    }

    return STATUS_SUCCESS;
}

gint32 COA2proxy_CliImpl::RemoveSession(
    const std::string& strSess )
{
    // the caller makes sure the gss lock is
    // acquired.
    CStdRMutex oGssLock( this->GetLock() );
    auto itr2 =
        m_mapSess2PortId.find( strSess );
    if( itr2 == m_mapSess2PortId.end() )
        return -ENOENT;

    guint32 dwPortId = itr2->second;
    return RemoveSession( dwPortId );
}

gint32 COA2proxy_CliImpl::IsSessExpired(
    IEventSink* pCallback,
    const std::string& strSess )
{
    CStdRMutex oGssLock( this->GetLock() );
    auto itr = 
        m_mapSess2PortId.find( strSess );
    if( itr == m_mapSess2PortId.end() )
        return STATUS_SUCCESS;

    guint32 dwPortId = itr2->second;
    auto it2 = m_mapSessions.find( dwPortId );
    if( it2 == m_mapSessions.end() )
        return ERROR_FAIL;
    
    USER_INFO* pui = it2->second;
    if( pui == nullptr )
        return ERROR_FAIL;
    timespec tv;
    clock_gettime( CLOCK_REALTIME, &tv );
    if( tv.tv_sec >= pui->tsExpireTime.tv_sec )
        return STATUS_SUCCESS;
    return ERROR_FALSE;
}

gint32 COA2proxy_CliImpl::InquireSess(
    const std::string& strSess,
    CfgPtr& pInfo )
{ return ERROR_NOT_IMPL; }


extern gint32 gen_sess_hash(
    BufPtr& pBuf,
    std::string& strSess );

gint32 COA2proxy_CliImpl::GenSessHash(
    stdstr strToken,
    guint32 dwPortId,
    std::string& strSess )
{
    if( dwPortId == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        BufPtr pBuf( true );
        ret = pBuf->Append( strToken.c_str(),
            strToken.size() );
        if( ERROR( ret ) )
            break;

        CRpcRouterBridge* pRouter =
            static_cast< CRpcRouterBridge* >
                ( GetRouter() );
        if( unlikely( pRouter == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }

        InterfPtr pIf;
        ret = pRouter->GetBridge( dwPortId, pIf );
        if( ERROR( ret ) )
            break;

        CCfgOpenerObj oIfCfg( ( CObjBase* )pIf );
        IConfigDb* pConn;
        ret = oIfCfg.GetPointer(
            propConnParams, pConn );
        if( ERROR( ret ) )
            break;

        ret = AppendConnParams( pConn, pBuf );
        if( ERROR( ret ) )
            break;

        guint64 qwSalt = pIf->GetObjId();
        qwSalt = htonll( qwSalt );

        pBuf->Append( ( char* )&qwSalt,
            sizeof( qwSalt ) );

        stdstr strRet;
        ret = gen_sess_hash( pBuf, strRet );
        if( ERROR( ret ) )
            break;

        strSess = "AUoa";
        strSess += strRet;

        DebugPrint( 0, "Sess hash is %s",
            strSess.c_str() );

    }while( 0 );

    return ret;
}

gint32 COA2proxy_CliImpl::BuildLoginResp(
    IEventSink* pInv, gint32 iRet,
    const Variant& oToken,
    CfgPtr& pResp )
{
    gint32 ret = 0;
    do{
        gint32 iRet = 0;
        CParamList oResp( pResp );
        ret = oResp.SetIntProp(
            propReturnValue, iRet );
        if( ERROR( ret ) )
            break;

        if( ERROR( iRet ) )
        {
            ret = iRet;
            break;
        }

        DMsgPtr pMsg;
        CCfgOpenerObj oInvCfg( pInv );
        ret = oInvCfg.GetMsgPtr(
            propMsgPtr, pMsg );
        if( ERROR( ret ) )
            break;

        ObjPtr pObj;
        ret = pMsg.GetObjArgAt( 1, pObj );
        if( ERROR( ret ) )
            break;

        guint32 dwPortId = 0;
        CCfgOpener oReqCtx(
            ( IConfigDb* )pObj );
        ret = oReqCtx.GetIntProp(
            propConnHandle, dwPortId );
        if( ERROR( ret ) )
            break;

        ret = pIf->GenSessHash(
            oToken, dwPortId, strSess );
        if( ERROR( ret ) )
            break;

        ObjPtr pui;
        ret = oResp.Pop( pui );
        if( ERROR( ret ) )
            break;

        oResp.SetStrProp(
            propSessHash, strSess );

        CRpcRouterBridge* pRouter =
            static_cast< CRpcRouterBridge* >
                ( GetRouter() );
        if( unlikely( pRouter == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }

        InterfPtr pBridge;
        ret = pRouter->GetBridge(
            dwPortId, pBridge );

        if( ERROR( ret ) )
            break;

        oResp.SetQwordProp(
            propSalt, pBridge->GetObjId() );

        CStdRMutex oIfLock( pIf->GetLock() );
        pIf->AddSession( dwPortId, strSess );
        pIf->m_mapSessions[ dwPortId ] = pui;

    }while( 0 );
    return ret;
}

gint32 COA2proxy_CliImpl::Login(
    IEventSink* pCallback,
    IConfigDb* pInfo,
    CfgPtr& pResp
    )
{
    gint32 ret = 0;
    do{
        CParamList oContext;
        oContext.SetPointer(
            propEventSink, pCallback );
        Variant oToken;
        ret = pInfo->GetProperty( 0, oToken );
        if( ERROR( ret ) )
            break;

        oContext.SetProperty( 0, oToken );
        bool bValid = false;

        gint32 (*func)( IEventSink*,
            COA2proxy_CliImpl*, IConfigDb* ) =
        ([]( IEventSink* pTask,
            COA2proxy_CliImpl* pIf,
            IConfigDb* pContext )->gint32
        {
            gint32 ret = 0;
            IConfigDb* pResp = nullptr;
            IEventSink* pInv = nullptr;
            do{
                CCfgOpener oCtx( pContext );
                oCtx.GetPointer(
                    propEventSink, pInv );

                CCfgOpener oCfg(
                    pTask->GetConfig() );

                ret = oCfg.GetPointer(
                    propRespPtr, pResp );

                if( ERROR( ret ) )
                    break;

                Variant oToken;
                ret = oCtx.GetProperty( 0, oToken );
                if( ERROR( ret ) )
                    break;

                gint32 iRet = 0;
                CParamList oResp( pResp );
                ret = oResp.GetIntProp(
                    propReturnValue, iRet );
                if( ERROR( ret ) )
                    break;

                if( ERROR( iRet ) )
                {
                    ret = iRet;
                    break;
                }

                ret = this->BuildLoginResp(
                    pInv, iRet, oToken,  pResp );

            }while( 0 );

            CCfgOpener oResp;
            if( ERROR( ret ) && pResp == nullptr )
            {
                oResp.SetIntProp(
                    propReturnValue, ret );
                pResp = oResp.GetCfg();
            }

            if( pInv == nullptr )
                return 0;

            pIf->SetResponse( pInv, pResp );
            pInv->OnEvent(
                eventTaskComp, ret, 0, nullptr );

            return ret;
        });

        TaskletPtr plcc;
        ret = NEW_COMPLETE_FUNCALL( 0, plcc,
            this->GetIoMgr(), func,
            this, oContext.GetCfg() );

        if( ERROR( ret ) )
            break;

        CCfgOpener oCtx2;
        oCtx2.SetPointer( propEventSink,
            ( CObjBase* )plcc );

        USER_INFO ui;
        ret = this->DoLogin( oCtx2.GetCfg(),
            ( stdstr& )oToken, ui );
        if( ret == STATUS_PENDING )
            break;

        ( *plcc )( eventCancelTask );
        this->BuildLoginResp(
            pCallback, ret, oToken, pResp );

    }while( 0 );
    return ret;
}
