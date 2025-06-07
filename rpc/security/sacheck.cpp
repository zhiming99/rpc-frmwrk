/*
 * =====================================================================================
 *
 *       Filename:  sacheck.cpp
 *
 *    Description:  Implementation of SimpAuth checker to accept login request, 
 *                  verify the login token, and provide session info.
 *
 *        Version:  1.0
 *        Created:  05/15/2025 09:00:01 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:  
 *
 *      Copyright:  2025 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#include "rpc.h"
using namespace rpcf;
#include "stmport.h"
#include "fastrpc.h"
#include "AppManagercli.h"
#include "security.h"
#include "sacheck.h"

namespace rpcf
{

gint32 CSimpleAuthCliWrapper::Create(
    CIoManager* pMgr,
    IEventSink* pCallback,
    IConfigDb* pCfg )
{
    gint32 ret = 0;
    TaskGrpPtr pGrp;
    do{
        CCfgOpener oCfg;
        oCfg.SetPointer( propIoMgr, pMgr );
        ret = pGrp.NewObj( clsid( CIfTaskGroup ),
            oCfg.GetCfg() );
        if( ERROR( ret ) )
            break;
        pGrp->SetRelation( logicNONE );
        TaskletPtr pDestroy;

        ret = NEW_FUNCCALL_TASKEX2( 1, pDestroy,
            pMgr, DestroySimpleAuthcli, pMgr,
            ( IEventSink* )nullptr, false );
        if( ERROR( ret ) )
            break;

        pGrp->AppendTask( pDestroy );

        gint32 (*func)( IEventSink*, IEventSink* ) =
        ([]( IEventSink* pTask, IEventSink* pCb )->gint32
        {
            gint32 ret = 0;
            IConfigDb* pResp = nullptr;
            CCfgOpener or2;
            do{
                CCfgOpenerObj oCfg( pTask );
                ret = oCfg.GetPointer(
                    propRespPtr, pResp );
                if( ERROR( ret ) )
                    break;
                CCfgOpener oResp( pResp );
                gint32 iRet = 0;
                ret = oResp.GetIntProp(
                    propReturnValue,
                    ( guint32& )iRet );
                if( ERROR( ret ) )
                    break;
                if( ERROR( iRet ) )
                {
                    ret = iRet;
                    break;
                }
            }while( 0 );
            if( pResp == nullptr )
            {
                or2.SetIntProp( propReturnValue, ret );
                pResp = or2.GetCfg();
            }
            Variant oVar = ObjPtr( pResp );
            pCb->SetProperty( propRespPtr, oVar );
            pCb->OnEvent( eventTaskComp,
                ret, 0, nullptr );
            return 0;
        });

        TaskletPtr pComp;
        ret = NEW_COMPLETE_FUNCALL( 0, pComp, pMgr,
            func, nullptr, pCallback );
        if( ERROR( ret ) )
            break;

        TaskletPtr pCreate;
        ret = NEW_FUNCCALL_TASKEX( pCreate, pMgr,
            CreateSimpleAuthcli, pMgr,
            clsid( CSimpleAuthCliWrapper ),
            pComp, pCfg );
        if( ERROR( ret ) )
            break;

        CIfRetryTask* pRetry = pComp;
        pRetry->SetClientNotify( pCreate );
        pCreate->MarkPending();

        pGrp->AppendTask( pCreate );
        TaskletPtr pTask = pGrp;
        ret = pMgr->AddAndRun( pTask, false );
        if( SUCCEEDED( ret ) )
            ret = STATUS_PENDING;
        else if( ERROR( ret ) )
        {
            pRetry->ClearClientNotify();
            ( *pComp )( eventCancelTask );
        }

    }while( 0 );
    if( ERROR( ret ) && !pGrp.IsEmpty() )
        ( *pGrp )( eventCancelTask );

    return ret;
}

gint32 CSimpleAuthCliWrapper::Destroy(
    CIoManager* pMgr,
    IEventSink* pCallback )
{
    TaskletPtr pDummy;
    if( pCallback == nullptr )
    {
        pDummy.NewObj( clsid( CIfDummyTask ) );    
        pCallback = pDummy;
    }
    return DestroySimpleAuthcli(
        pMgr, pCallback, false );
}

CSimpleAuthCliWrapper::CSimpleAuthCliWrapper(
    const IConfigDb* pCfg ) :
    super::virtbase( pCfg ), super( pCfg )
{
    gint32 ret = 0;
    do{
        SetClassId( clsid(CSimpleAuthCliWrapper ) );

        CCfgOpener oCfg( pCfg );
        CRpcServices* pSvc = nullptr;
        ret = oCfg.GetPointer(
            propRouterPtr, pSvc );
        if( ERROR( ret ) )
            break;
        m_pRouter = pSvc;

        PSAACBS psaacbs;
        psaacbs.reset( new CSacwCallbacks() );
        auto pcbs = static_cast< CSacwCallbacks* >
            ( psaacbs.get() );
        pcbs->m_pThis = this;

        CfgPtr pEmpty;
        SetAsyncCallbacks( psaacbs, pEmpty );

    }while( 0 );
    if( ERROR( ret ) )
    {
        stdstr strMsg = DebugMsg( ret,
            "Error in CSimpleAuthCliWrapper ctor" );
        throw new std::runtime_error( strMsg );
    }
}

gint32 CSacwCallbacks::CheckClientTokenCallback( 
    IConfigDb* context, gint32 iRet,
    ObjPtr& pInfo /*[ In ]*/ )
{
    gint32 ret = 0;
    if( context == nullptr )
        return -EINVAL;

    CCfgOpener oCtx( context );
    CParamList oResp;
    IEventSink* pCb = nullptr;
    ret = oCtx.GetPointer( propEventSink, pCb );
    if( ERROR( ret ) )
        return ret;
    do{
        if( ERROR( iRet ) )
        {
            ret = iRet;
            break;
        }
        oCtx.RemoveProperty( propReqPtr );
        oCtx.RemoveProperty( propGmSSL );

        stdstr strUser;
        ret = oCtx.GetStrProp(
            propUserName, strUser );
        if( ERROR( ret ) )
            break;

        guint32 dwPortId = 0;
        ret = oCtx.GetIntProp(
            propConnHandle, dwPortId );
        if( ERROR( ret ) )
            break;

        stdstr strSess;
        CSimpleAuthCliWrapper* psac = m_pThis;
        ret = psac->GenSessHash( strUser,
            dwPortId, strSess );

        if( ERROR( ret ) )
        {
            oCtx.Clear();
            break;
        }

        oResp[ propSessHash ] = strSess;

        timespec ts;
        clock_gettime( CLOCK_REALTIME, &ts );
        guint64 qwTs = ts.tv_sec;
        qwTs <<= 32;
        qwTs += ts.tv_nsec;
        oCtx.SetQwordProp( propTimestamp, qwTs );

        oResp.CopyProp( propSalt,
            ( IConfigDb* )oCtx.GetCfg() );
        CStdRMutex oIfLock( psac->GetLock() );
        CfgPtr pSess( context );
        psac->AddSession(
            dwPortId, strSess, pSess );
        oResp.Push( pInfo );

    }while( 0 );
    oResp[ propReturnValue ] = ret;
    Variant oVar( ObjPtr( oResp.GetCfg() ) );
    pCb->SetProperty( propRespPtr, oVar );
    pCb->OnEvent( eventTaskComp,
        ret, 0, nullptr );
    oCtx.RemoveProperty( propEventSink );
    return ret;
}

void CSacwCallbacks::OnSvrOffline(
    IConfigDb* context,
    CSimpleAuth_CliImpl* pIf )
{
    return;
}
gint32 CSimpleAuthCliWrapper::OnPostStop(
    IEventSink* pCb )
{
    CAuthentServer* prt = this->GetRouter();
    if( prt == nullptr )
        return ERROR_STATE;
    gint32 (*func)( CAuthentServer* ) =
    ([]( CAuthentServer* prt )
    {
        ObjPtr pAuth;
        // a probe to check whether to reconnect.
        prt->GetAuthImpl( pAuth );
        return 0;
    });
    CIoManager* pMgr = GetIoMgr();
    TaskletPtr pReconn;
    gint32 ret = NEW_FUNCCALL_TASKEX( pReconn,
        pMgr, func, prt );
    if( SUCCEEDED( ret ) )
        pMgr->AddSeqTask( pReconn );
    ret = super::OnPostStop( pCb );
    m_pRouter.Clear();
    return ret;
}

gint32 CSimpleAuthCliWrapper::Login(
    IEventSink* pCallback,
    IConfigDb* pInfo, /*[ in ]*/
    CfgPtr& pResp ) /*[ out ]*/
{
    gint32 ret = 0;
    if( pCallback == nullptr ||
        pInfo == nullptr )
        return -EINVAL;
    do{
        CCfgOpener oInfo( pInfo );

        bool bGmSSL = false;
        ret = oInfo.GetBoolProp(
            propGmSSL, bGmSSL );

        if( ERROR( ret ) )
            break;

        BufPtr pToken;
        ret = oInfo.GetBufPtr( 0, pToken );
        if( ERROR( ret ) )
            break;

        DMsgPtr pMsg;
        CCfgOpenerObj oInvCfg( pCallback );
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

        CRpcRouterBridge* prt = GetRouter();
        InterfPtr pIf;
        ret = prt->GetBridge( dwPortId, pIf );
        if( ERROR( ret ) )
            break;

        Variant oVar;
        stdstr strNAHash;
        ret = pIf->GetProperty(
            propSessHash, oVar );
        if( ERROR( ret ) )
            break;
        strNAHash = ( stdstr& )oVar;

        stdstr strUser;
        ret = oInfo.GetStrProp(
            propUserName, strUser );
        if( ERROR( ret ) )
            break;

        CRpcTcpBridge* pBdge = pIf;
        guint64 qwTs = pBdge->GetStartSec();

        CCfgOpener oLocalInfo;
        oLocalInfo.SetStrProp(
            propSessHash, strNAHash );
        oLocalInfo.SetQwordProp(
            propTimestamp, qwTs );

        ObjPtr pRespTok;
        CCfgOpener oCtx;
        oCtx.SetPointer(
            propEventSink, pCallback );
        oCtx.SetBoolProp(
            propGmSSL, bGmSSL );
        oCtx.SetStrProp(
            propUserName, strUser );
        oCtx.SetPointer( propReqPtr,
            ( IConfigDb* )oLocalInfo.GetCfg() );
        oCtx.SetIntProp(
            propConnHandle, dwPortId );
        oCtx.SetQwordProp(
            propSalt, pIf->GetObjId() );

        ObjPtr pci( pInfo );
        ObjPtr psi( oLocalInfo.GetCfg() );
        ret = this->CheckClientToken(
            oCtx.GetCfg(), pci, psi,
            pRespTok ); 

        CParamList oResp( pResp );
        if( ret == STATUS_PENDING )
            break;

        if( SUCCEEDED( ret ) )
        do{
            stdstr strSess;
            ret = GenSessHash( strUser,
                dwPortId, strSess );

            if( ERROR( ret ) )
                break;

            oResp[ propSessHash ] = strSess;

            oCtx.RemoveProperty( propReqPtr );
            oCtx.RemoveProperty( propEventSink );
            oCtx.RemoveProperty( propGmSSL );
            timespec ts;
            clock_gettime( CLOCK_REALTIME, &ts );
            qwTs = ts.tv_sec;
            qwTs <<= 32;
            qwTs += ts.tv_nsec;
            oCtx.SetQwordProp( propTimestamp, qwTs );

            oResp.SetQwordProp(
                propSalt, pIf->GetObjId() );
            oResp.Push( ObjPtr( pResp ) );
            CStdRMutex oIfLock( this->GetLock() );
            AddSession( dwPortId,
                strSess, oCtx.GetCfg() );
        }while( 0 );

        oResp[ propReturnValue ] = ret;
        if( ERROR( ret ) )
            oCtx.Clear();
    }while( 0 );
    return ret;
}

gint32 CSimpleAuthCliWrapper::IsSessExpired(
    const std::string& strSess )
{
    CStdRMutex oGssLock( this->GetLock() );
    auto itr = 
        m_mapSess2PortId.find( strSess );
    if( itr == m_mapSess2PortId.end() )
        return STATUS_SUCCESS;

    guint32 dwPortId = itr->second;
    auto it2 = m_mapSessions.find( dwPortId );
    if( it2 == m_mapSessions.end() )
        return ERROR_FAIL;
    
    IConfigDb* pui = it2->second;
    if( pui == nullptr )
        return ERROR_FAIL;
    timespec tv;
    clock_gettime( CLOCK_REALTIME, &tv );
    CCfgOpener oCtx( pui );
    guint64 qwTs;
    oCtx.GetQwordProp( propTimestamp, qwTs );
    if( tv.tv_sec >=
        ( ( qwTs >> 32 ) + 3600 * 24 ) )
        return STATUS_SUCCESS;
    return ERROR_FALSE;
}

gint32 CSimpleAuthCliWrapper::InquireSess(
    const std::string& strSess,
    CfgPtr& pInfo )
{
    CCfgOpener oInfo( ( IConfigDb* )pInfo );
    CStdRMutex oGssLock( this->GetLock() );
    auto itr = 
        m_mapSess2PortId.find( strSess );
    if( itr == m_mapSess2PortId.end() )
        return ERROR_FAIL;

    guint32 dwPortId = itr->second;
    auto it2 = m_mapSessions.find( dwPortId );
    if( it2 == m_mapSessions.end() )
        return ERROR_FAIL;
    
    IConfigDb* pui = it2->second;
    if( pui == nullptr )
        return ERROR_FAIL;

    oInfo.CopyProp( propUserName, pui );
    oInfo.SetStrProp( propAuthMech, "SimpAuth" );
    oInfo.SetStrProp( propSessHash, strSess );
    oInfo.CopyProp( propConnHandle, pui );
    oInfo.CopyProp( propTimestamp, pui );
    return STATUS_SUCCESS;
}

gint32 CSimpleAuthCliWrapper::GenSessHash(
    const stdstr& strToken,
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

        strSess = "AUsa";
        strSess += strRet;

        DebugPrint( 0, "Sess hash is %s",
            strSess.c_str() );

    }while( 0 );

    return ret;
}

gint32 CSimpleAuthCliWrapper::AddSession(
    guint32 dwPortId, 
    const std::string& strSess,
    CfgPtr& pSessInfo )
{
    m_mapPortId2Sess[ dwPortId ] = strSess;
    m_mapSess2PortId[ strSess ] = dwPortId;
    m_mapSessions[ dwPortId ] = ObjPtr( pSessInfo );
    return STATUS_SUCCESS;
}

gint32 CSimpleAuthCliWrapper::RemoveSession(
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

gint32 CSimpleAuthCliWrapper::RemoveSession(
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

gint32 CSimpleAuthCliWrapper::GetSess(
    guint32 dwPortId,
    std::string& strSess )
{
    CStdRMutex oGssLock( this->GetLock() );
    auto itr =
        m_mapPortId2Sess.find( dwPortId );
    if( itr == m_mapPortId2Sess.end() )
        return -ENOENT;

    strSess = itr->second;
    return STATUS_SUCCESS;
}

}
