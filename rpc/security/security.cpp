/*
 * =====================================================================================
 *
 *       Filename:  security.cpp
 *
 *    Description:  implementation of authentication related classes 
 *
 *        Version:  1.0
 *        Created:  06/10/2020 11:32:32 PM
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

#include "configdb.h"
#include "defines.h"
#include "proxy.h"
#include "rpcroute.h"
#include "security.h"
#include "k5proxy.h"
#include <regex>

#include "blkalloc.h"
using namespace rpcf;
#ifdef OA2
#include "oa2check/OA2proxycli.h"
#define CLI_REG "clientreg.dat"
#include "oa2proxy.h"
#endif
#include "saproxy.h"

#include "fastrpc.h"
#include "stmport.h"
#include "sacheck.h"

namespace rpcf
{

gint32 CRpcTcpBridgeAuth::OnLoginTimeout(
    IEventSink* pCallback,
    IEventSink* pIoReq,
    IConfigDb* pReqCtx )
{
    if( pIoReq == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CCfgOpenerObj oReq( pIoReq );
        IConfigDb* pResp = nullptr;
        ret = oReq.GetPointer(
            propRespPtr, pResp );

        if( ERROR( ret ) )
            break;

        CCfgOpener oResp( pResp );
        gint32 iRet = 0;
        ret = oResp.GetIntProp(
            propReturnValue,
            ( guint32& ) iRet );

        if( ERROR( ret ) )
            break;

        if( iRet != ERROR_TIMEOUT )
        {
            ret = iRet;
            break;
        }

        OnLoginFailed( pCallback, -ETIMEDOUT );

    }while( 0 );

    return ret;
}

gint32 CRpcTcpBridgeAuth::StartLoginTimer(
    IEventSink* pCallback,
    guint32 dwTimeoutSec )
{
    gint32 ret = 0;
    do{
        CIfDeferCallTaskEx* pTask =
            ObjPtr( pCallback );
        if( pTask == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ret = pTask->EnableTimer( dwTimeoutSec );
        if( ERROR( ret ) )
            break;

        ret = STATUS_PENDING;

    }while( 0 );

    return ret;
}

gint32 CRpcTcpBridgeAuth::OnPostStart(
    IEventSink* pCallback )
{
    if( pCallback == nullptr )    
        return -EINVAL;

    gint32 ret = 0;
    do{
        TaskletPtr pRespCb;
        ret = NEW_PROXY_RESP_HANDLER2(
            pRespCb, ObjPtr( this ),
            &CRpcTcpBridgeAuth::OnLoginTimeout,
            nullptr, nullptr );
        if( ERROR( ret ) )
            break;

        TaskletPtr pTask;
        ret = DEFER_IFCALLEX_NOSCHED2(
            0, pTask, ObjPtr( this ),
            &CRpcTcpBridgeAuth::StartLoginTimer,
            nullptr, 0 );

        if( ERROR( ret ) )
        {
            ( *pRespCb )( eventCancelTask );
            break;
        }

        CIfRetryTask* pRetryTask = pTask;
        pRetryTask->SetClientNotify( pRespCb );
        ret = RunManagedTask( pTask );
        if( SUCCEEDED( ret ) )
            ret = pTask->GetError();

        if( ERROR( ret ) )
        {
            ( *pTask )( eventCancelTask );
            ( *pRespCb )( eventCancelTask );
        }

        if( ret == STATUS_PENDING )
        {
            m_pLoginTimer = pTask;
            ret = 0;
        }

    }while( 0 );

    return ret;
}

gint32 CRpcTcpBridgeAuth::OnEnableComplete(
    IEventSink* pCallback,
    IEventSink* pIoReq,
    IConfigDb* pReqCtx )
{
    if( pIoReq == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CCfgOpenerObj oReq( pIoReq );
        IConfigDb* pResp = nullptr;
        ret = oReq.GetPointer(
            propRespPtr, pResp );

        if( ERROR( ret ) )
            break;

        CCfgOpener oResp( pResp );
        gint32 iRet = 0;
        ret = oResp.GetIntProp(
            propReturnValue,
            ( guint32& ) iRet );

        if( ERROR( ret ) )
            break;

        if( ERROR( iRet ) )
        {
            ret = iRet;
            break;
        }

        CCfgOpener oReqCtx( pReqCtx );
        CStlObjVector* pVec;
        ret = oReqCtx.GetPointer( 0, pVec );
        if( ERROR( ret ) )
            break;

        std::vector< MatchPtr > vecMatches;
        for( auto elem : ( *pVec )() )
        {
            MatchPtr pMatch = elem;
            vecMatches.push_back( pMatch );
        }

        ret = StartRecvTasks( vecMatches );

    }while( 0 );

    return ret;
}

gint32 CRpcTcpBridgeAuth::EnableInterfaces()
{
    gint32 ret = 0;
    TaskGrpPtr pTaskGrp;
    TaskletPtr pRespCb;
    do{
        if( IsKdcChannel() )
        {
            ret = 0;
            break;
        }
        std::vector< MatchPtr > vecMatches;
        const char* szName = CoGetClassName(
            this->GetClsid() );

        stdstr strClass = DBUS_IF_NAME( szName );
        for( auto& elem : m_vecMatches )
        {
            CCfgOpenerObj oMatch(
                ( CObjBase* )elem );
            bool bDummy = true;
            ret = oMatch.GetBoolProp(
                propDummyMatch, bDummy );
            if( ERROR( ret ) )
                continue;

            if( !bDummy )
                continue;

            std::string strIfName;
            if( strClass == strIfName )
            {
                // this is a true dummy interface
                continue;
            }

            ret = oMatch.GetStrProp(
                propIfName, strIfName );
            if( ERROR( ret ) )
                continue;

            oMatch.RemoveProperty(
                propDummyMatch );

            vecMatches.push_back( elem );
        }

        if( vecMatches.empty() )
            break;

        CParamList oParams;            
        oParams[ propIfPtr ] = ObjPtr( this );

        ret = pTaskGrp.NewObj( 
            clsid( CIfTaskGroup ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        pTaskGrp->SetRelation( logicAND );

        ObjVecPtr pObjVec;
        ret = pObjVec.NewObj(
            clsid( CStlObjVector ) );

        if( ERROR( ret ) )
            break;

        oParams.Push( true );
        for( auto elem : vecMatches )
        {
            oParams[ propMatchPtr ] =
                ObjPtr( elem );

            TaskletPtr pEnableEvtTask;
            ret = pEnableEvtTask.NewObj(
                clsid( CIfEnableEventTask ),
                oParams.GetCfg() );

            if( ERROR( ret ) )
                break;

            pTaskGrp->AppendTask(
                pEnableEvtTask );

            ( *pObjVec )().push_back(
                ObjPtr( elem ) );
        }

        CParamList oReqCtx;
        oReqCtx.Push( ObjPtr( pObjVec ) );
        ret = NEW_PROXY_RESP_HANDLER2(
            pRespCb, ObjPtr( this ),
            &CRpcTcpBridgeAuth::OnEnableComplete,
            nullptr,
            ( IConfigDb* )oReqCtx.GetCfg() );

        if( ERROR( ret ) )
            break;

        pTaskGrp->SetClientNotify(
            ( IEventSink* )pRespCb );

        TaskletPtr pTask = pTaskGrp;
        ( *pTask )( eventZero );
        ret = pTask->GetError();
        if( ret == STATUS_PENDING )
            break;

        if( SUCCEEDED( ret ) )
        {
            ( *pRespCb )( eventCancelTask );

            CParamList oParms;
            oParams[ propReturnValue ] = 0;

            TaskletPtr pDummy;
            pDummy.NewObj(
                clsid( CIfDummyTask ) );
            CCfgOpener oCfg( ( IConfigDb* )
                pDummy->GetConfig() );
            oCfg.SetPointer( propRespPtr,
                ( IConfigDb* )oParams.GetCfg() );

            ret = OnEnableComplete( nullptr,
                pDummy, oReqCtx.GetCfg() );
        }

    }while( 0 );

    return ret;
};

gint32 CRpcTcpBridgeAuth::OnPostStop(
    IEventSink* pCallback )
{
    SetSessHash( "", false );
    return super::OnPostStop( pCallback );
}

gint32 CRpcTcpBridgeAuth::SetSessHash(
    const std::string& strHash,
    bool bNoEnc )
{
    // notify the bridge the authentication
    // context is setup.
    gint32 ret = 0;
    do{
        CRpcRouterBridgeAuthImpl* pRouter =
            ObjPtr( GetParent() );

        std::string strHash1;
        CCfgOpenerObj oIfCfg( this );
        ret = oIfCfg.GetStrProp(
            propSessHash, strHash1 );
        if( SUCCEEDED( ret ) &&
            strHash1.substr( 0, 2 ) == "AU" )
        { 
            if( !strHash.empty() )
            {
                ret = -EEXIST;
                break;
            }

            oIfCfg.RemoveProperty( propSessHash );
            m_oSecCtx.Clear();

            // clear the the session
            CAuthentServer* pAs;
            pAs = ObjPtr( pRouter );

            pAs->RemoveSession( strHash1 );
            if( !m_pLoginTimer.IsEmpty() )
            {
                // at this point, the
                // m_pLoginTimer is already
                // canceled.
                m_pLoginTimer.Clear();
            }
            if( !m_pSessChecker.IsEmpty() )
                ( *m_pSessChecker )( eventCancelTask );

            break;
        }

        ret = 0;
        if( strHash.empty() )
            break;

        ObjPtr pAuthImpl;
        ret = pRouter->GetAuthImpl( pAuthImpl );
        if( ERROR( ret ) )
        {
            ret = 0;
            break;
        }

        CCfgOpener& oCtx = m_oSecCtx;

        IConfigDb* pAuthInfo = nullptr;

        CCfgOpenerObj oRtCfg( pRouter );
        oRtCfg.GetPointer(
            propAuthInfo, pAuthInfo );

        if( ERROR( ret ) )
            break;

        Variant oVar;
        ret = pAuthInfo->GetProperty(
            propAuthMech, oVar );
        if( ERROR( ret ) )
            break;

        bool bKrb5 = false;
        if( ( stdstr& )oVar == "krb5" )
            bKrb5 = true;

        if( !bNoEnc && bKrb5 )
        {
            oCtx.SetObjPtr(
                propObjPtr, pAuthImpl );
            oCtx.CopyProp(
                propSignMsg, pAuthInfo );
        }


        oCtx.SetBoolProp( propNoEnc, bNoEnc );
        oCtx.SetBoolProp( propIsServer, true );
        oCtx.SetStrProp( propSessHash, strHash );

        PortPtr pPort;
        GET_TARGET_PORT( pPort );
        if( ERROR( ret ) )
            break;

        BufPtr pBuf( true );
        bool bFound = false;
        while( !pPort.IsEmpty() && bKrb5 )
        {
            ret = GetPortProp( pPort,
                propLowerPortPtr, pBuf );    
            if( ERROR( ret ) )
            {
                ret = 0;
                break;
            }

            pPort = ( ObjPtr& )*pBuf;
            if( pPort.IsEmpty() )
                break;

            ret = GetPortProp( pPort,
                propPortClass, pBuf );
            if( ERROR( ret ) )
            {
                ret = 0;
                continue;
            }
            std::string strClass = *pBuf;
            if( strClass != PORT_CLASS_SEC_FIDO )
                continue;

            bFound = true;
            break;
        }

        if( !bFound && bKrb5 )
        {
            oCtx.Clear();
            break;
        }

        oIfCfg.SetStrProp( propSessHash, strHash);

        *pBuf = ObjPtr( m_oSecCtx.GetCfg() );
        SetPortProp( pPort, propSecCtx, pBuf );

        // stop the login timer
        if( !m_pLoginTimer.IsEmpty() )
        {
            ( *m_pLoginTimer )( eventCancelTask );
            m_pLoginTimer.Clear();
        }

        // start all the rest interfaces,
        // espacially the stream interfaces

        if( IsKdcChannel() )
        {
            ret = 0;
            break;
        }

        // set the session checker
        ret = DEFER_IFCALLEX_NOSCHED(
            m_pSessChecker, ObjPtr( this ),
            &CRpcTcpBridgeAuth::CheckSessExpired,
            strHash );

        if( ERROR( ret ) )
            break;

        CIfRetryTask* pRetryTask = m_pSessChecker;
        if( unlikely( pRetryTask == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }

        CCfgOpenerObj oTaskCfg( pRetryTask );

        oTaskCfg.SetIntProp(
            propRetries, MAX_NUM_CHECK );

        oTaskCfg.SetIntProp(
            propIntervalSec, 360 );

        this->AddAndRun( m_pSessChecker );

    }while( 0 );

    return ret;
}


gint32 CRpcTcpBridgeAuth::IsAuthRequest(
    IConfigDb* pReqCtx,
    DBusMessage* pDMsg )
{
    if( pReqCtx == nullptr || pDMsg == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        DMsgPtr pMsg( pDMsg );
        std::string strVal = CoGetIfNameFromIid(
            iid( IAuthenticate ), "s" );
        strVal = DBUS_IF_NAME( strVal );
        stdstr strIf =
            std::move( pMsg.GetInterface() );

        if( strIf != strVal )
        {
            ret = ERROR_FALSE;
            break;
        }

        strVal = AUTH_DEST( this );
        if( strVal != pMsg.GetDestination() )
        {
            ret = -EACCES;
            break;
        }

        CCfgOpener oReqCtx( pReqCtx );
        ret = oReqCtx.GetStrProp(
            propRouterPath, strVal );
        if( ERROR( ret ) )
        {
            ret = ERROR_FALSE;
            break;
        }

        if( strVal != "/" )
        {
            ret = -EACCES;
            break;
        }

    }while( 0 );

    return ret;
}

gint32 CRpcTcpBridgeAuth::OnLoginFailed(
    IEventSink* pCallback, gint32 iRet )
{
    gint32 ret = 0;
    do{

        stdstr strMsg = "Error Login: ";
        DumpConnParams( strMsg );
        strMsg += "Login=false";
        LOGERR( GetIoMgr(), iRet, strMsg );
        DebugPrintEx( logErr, iRet, "Login failed,"
            " connection will be reset" );
        CRpcRouter* pRouter = GetParent();
        if( !m_pLoginTimer.IsEmpty() )
            ( *m_pLoginTimer )( eventCancelTask );

        PostDisconnEvent();

    }while( 0 );

    return ret;
}

gint32 CRpcTcpBridgeAuth::DumpConnParams(
    stdstr& strMsg )
{
    gint32 ret = 0;
    do{
        IConfigDb* pConnParams;
        CCfgOpenerObj oIfCfg( this );
        ret = oIfCfg.GetPointer(
            propConnParams, pConnParams );
        if( ERROR( ret ) )
            break;
        CConnParams oConn( pConnParams );
        stdstr strVal = oConn.GetSrcIpAddr();
        strMsg += stdstr( "SrcIp=" ) +
            strVal + ",";
        guint32 dwVal = oConn.GetSrcPortNum();
        strMsg += stdstr( "SrcPort=" ) +
                std::to_string( dwVal ) + ",";
        strVal = oConn.GetDestIpAddr();
        strMsg += stdstr( "DestIp=" ) +
            strVal + ",";
        dwVal = oConn.GetDestPortNum();
        strMsg += stdstr( "DestPort=" ) +
                std::to_string( dwVal ) + ",";
        strVal = oConn.GetRouterPath();
        strMsg += stdstr( "RouterPath=" ) +
            strVal + ",";
        bool bVal = oConn.IsSSL();
        if( bVal )
            strMsg += stdstr( "SSL=true," );
        else
            strMsg += stdstr( "SSL=false," );
        bVal = oConn.IsWebSocket();
        if( bVal )
            strMsg += stdstr( "WebSock=true," );
        else
            strMsg += stdstr( "WebSock=false," );
        if( bVal )
        {
            strVal = oConn.GetUrl();
            strMsg += stdstr( "Url=" ) +
                strVal + ",";
        }

    }while( 0 );
    return 0;
}

gint32 CRpcTcpBridgeAuth::LogSuccessfuleLogin(
    const stdstr& strSess,
    IConfigDb* pUserInfo )
{
    gint32 ret = 0;
    do{
        stdstr strMsg = "Session ";
        strMsg += strSess + ": ";
        CCfgOpener oInfo( pUserInfo );
        stdstr strVal;
        ret = oInfo.GetStrProp(
            propUserName, strVal );
        if( SUCCEEDED( ret ) )
            strMsg += stdstr( "UserName=" ) +
                strVal + ",";
        guint32 dwVal = 0;
        ret = oInfo.GetIntProp(
            propTimeoutSec, dwVal );
        if( SUCCEEDED( ret ) )
            strMsg += stdstr( "Lifetime=" ) +
                std::to_string( dwVal ) + ",";
        if( strSess.substr( 0, 4 ) == "AUoa" )
            strMsg += stdstr( "Auth=OAuth2," );
        if( strSess.substr( 0, 4 ) == "AUsa" )
            strMsg += stdstr( "Auth=SimpAuth," );
        else if( strSess.substr( 0, 2 ) == "AU" )
            strMsg += stdstr( "Auth=krb5," );
        else
            strMsg += stdstr( "Auth=none," );
        strVal.clear();
        DumpConnParams( strVal );
        if( strVal.size() )
            strMsg += strVal;
        strMsg += "Login=true";
        LOGINFO( GetIoMgr(), 0, strMsg );
    }while( 0 );
    return ret;
}

gint32 CRpcTcpBridgeAuth::OnLoginComplete(
    IEventSink* pCallback,
    IEventSink* pIoReq,
    IConfigDb* pReqCtx )
{
    if( pIoReq == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    IConfigDb* pResp = nullptr;
    do{
        if( !IsConnected() )
        {
            ret = ERROR_STATE;
            break;
        }

        CCfgOpenerObj oReq( pIoReq );
        ret = oReq.GetPointer(
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
            OnServiceComplete( pResp, pCallback );
            ret = iRet;
            break;
        }

        // this is a forward-request, we need to
        // unwrap the response message first.
        DMsgPtr pRespMsg;
        ret = oResp.GetMsgPtr( 0, pRespMsg );
        if( ERROR( ret ) )
            break;

        ObjPtr pObj;
        ret = pRespMsg.GetObjArgAt( 1, pObj );
        if( ERROR( ret ) )
            break;

        IConfigDb* pLoginResp = pObj;
        if( pLoginResp == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        CCfgOpener oLoginResp( pLoginResp );
        ret = oLoginResp.GetIntProp(
            propReturnValue, ( guint32& )iRet );
        if( ERROR( ret ) )
            break;

        if( ERROR( iRet ) )
        {
            OnServiceComplete( pResp, pCallback );
            ret = iRet;
            break;
        }

        IConfigDb* pLoginTok = nullptr;
        ret = oLoginResp.GetPointer(
            0, pLoginTok );
        if( ERROR( ret ) )
            break;

        CCfgOpener oLoginTok( pLoginTok );
        bool bContinue = false;
        ret = oLoginTok.GetBoolProp(
            propContinue, bContinue );
        if( ERROR( ret ) )
            break;

        // the login is not complete yet.
        if( bContinue )
        {
            OnServiceComplete( pResp, pCallback );
            break;
        }

        EnableInterfaces();

        gint32 (*func)( IEventSink*,
            CRpcTcpBridgeAuth* ) = ([](
            IEventSink* pCb,
            CRpcTcpBridgeAuth* pIf )->gint32
        {
            gint32 ret = 0;
            do{
                CCfgOpenerObj oIfCfg( pIf );
                guint32 dwPortId = 0;
                ret = oIfCfg.GetIntProp(
                    propPortId, dwPortId );
                if( ERROR( ret ) )
                    break;

                CAuthentServer* pAuth =
                    ObjPtr( pIf->GetParent() );

                if( unlikely( pAuth == nullptr ) )
                {
                    ret = -EFAULT;
                    break;
                }

                std::string strSess;
                ret = pAuth->GetSess( dwPortId, strSess );
                if( ERROR( ret ) )
                    break;
         
                bool bNoEnc = false;
                ret = pAuth->IsNoEnc( strSess );
                if( SUCCEEDED( ret ) )
                    bNoEnc = true;

                ret = pIf->SetSessHash( strSess, bNoEnc );
                if( ERROR( ret ) )
                    break;

                CCfgOpener oInfo;
                ret = pAuth->InquireSess(
                    strSess, oInfo.GetCfg() );
                if( SUCCEEDED( ret ) )
                {
                    ObjPtr pObj( oInfo.GetCfg() );
                    pIf->LogSuccessfuleLogin(
                        strSess, pObj );
                    Variant oVar( pObj );
                    pIf->SetProperty(
                        propLoginInfo, oVar );
                }

                TaskletPtr pDummy;
                ret = pDummy.NewObj( 
                    clsid( CIfDummyTask ) );
                if( ERROR( ret ) )
                    break;

                // to open the default control stream
                ret = pIf->super::OnPostStart( pDummy );

            }while( 0 );

            if( ERROR( ret ) )
                pIf->OnLoginFailed( nullptr, ret );

            return 0;
        });

        TaskletPtr pSendNotify;
        ret = NEW_COMPLETE_FUNCALL( 0, 
            pSendNotify, this->GetIoMgr(),
            func, nullptr, this );
        if( ERROR( ret ) )
            break;

        oResp.SetPointer( propEventSink,
            ( IEventSink* )pSendNotify );

        OnServiceComplete( pResp, pCallback );

    }while( 0 );

    if( ERROR( ret ) )
    {
        auto pMgr = GetIoMgr();
        DEFER_CALL_DELAY( pMgr, 1, 
            ObjPtr( this ),
            &CRpcTcpBridgeAuth::OnLoginFailed,
            nullptr, ret );
    }

    return ret;
}

// add a session hash to the pFwdrMsg
gint32 CRpcTcpBridgeAuth::ForwardRequest(
    IConfigDb* pReqCtx,
    DBusMessage* pFwdrMsg,
    DMsgPtr& pRespMsg,
    IEventSink* pCallback )
{
    if( pReqCtx == nullptr ||
        pFwdrMsg == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CCfgOpenerObj oIfCfg( this );
        DMsgPtr pMsg( pFwdrMsg );
        std::string strHash;
        bool bSeqTask = false;
        bool bAuthed = false;

        ret = oIfCfg.GetStrProp(
            propSessHash, strHash );

        if( SUCCEEDED( ret ) &&
            strHash.substr( 0, 2 ) == "AU" )
            bAuthed = true;

        ret = IsAuthRequest(
            pReqCtx, pFwdrMsg );

        if( ret == -EACCES )
            break;

        if( SUCCEEDED( ret ) )
        {
            bool bLogin = false;
            TaskletPtr pRespCb;
            std::string strMember =
                SYS_METHOD( AUTH_METHOD_LOGIN );

            std::string strReq = pMsg.GetMember();

            if( pMsg.GetMember() == strMember )
            {
                bLogin = true;
            }
            else if( strReq != SYS_METHOD(
                AUTH_METHOD_MECHSPECREQ ) )
            {
                ret = -EACCES;
                break;
            }

            if( !bAuthed )
            {
                if( bLogin )
                {
                    ret = NEW_PROXY_RESP_HANDLER2(
                        pRespCb, ObjPtr( this ),
                        &CRpcTcpBridgeAuth::OnLoginComplete,
                        pCallback, pReqCtx );

                    if( ERROR( ret ) )
                        break;

                    CCfgOpenerObj oResp(
                        ( CObjBase*)pRespCb );

                    ret = oResp.CopyProp(
                        propMsgPtr, pCallback );

                    if( ERROR( ret ) )
                        break;

                    bSeqTask = true;
                    pCallback = pRespCb;
                }
            }
            else
            {
                if( bLogin )
                {
                    // already login, disconn to
                    // login again
                    ret = -EACCES;
                    break;
                }
            }

            // NOTE: made the login task to be a
            // seq task, to prevent
            // eventRmtSvrOffline to execute
            // within the same period.
            ret = super::ForwardRequestInternal(
                pReqCtx, pFwdrMsg,
                pRespMsg, pCallback, bSeqTask );

            if( ERROR( ret ) &&
                !pRespCb.IsEmpty() )
            {
                ( *pRespCb )( eventCancelTask );
            }
            break;
        }

        // not an authentication request
        if( !bAuthed )
        {
            ret = -EACCES;
            break;
        }
        CCfgOpener oReqCtx( pReqCtx );
        oReqCtx[ propSessHash ] = strHash;
        ret = super::ForwardRequest(
                pReqCtx, pFwdrMsg,
                pRespMsg, pCallback );

    }while( 0 );

    return ret;
}

gint32 CRpcTcpBridgeAuth::DoInvoke(
    DBusMessage* pReqMsg,
    IEventSink* pCallback )
{
    gint32 ret = 0;    
    do{
        if( !IsKdcChannel() )
        {
            ret = super::DoInvoke(
                pReqMsg, pCallback );
            break;
        }

        DMsgPtr pMsg( pReqMsg );
        CParamList oResp;
        std::string strMethod = pMsg.GetMember();
        if( strMethod != SYS_METHOD_FORWARDREQ )
        {
            ret = -EACCES;
            break;
        }

        // further filter will be done in
        // ForwardRequest
        ret = super::DoInvoke(
            pReqMsg, pCallback );

    }while( 0 );

    return ret;
}

gint32 CRpcTcpBridgeAuth::DoInvoke(
    IConfigDb* pEvtMsg,
    IEventSink* pCallback )
{
    if( IsKdcChannel() )
        return -EACCES;

    return CInterfaceServer::DoInvoke(
        pEvtMsg, pCallback );
}

gint32 CRpcTcpBridgeAuth::FetchData_Filter(
    IConfigDb* pDataDesc,           // [in]
    gint32& fd,                     // [out]
    guint32& dwOffset,              // [in, out]
    guint32& dwSize,                // [in, out]
    IEventSink* pCallback )
{
    if( pDataDesc == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CCfgOpener oDataDesc( pDataDesc );
        IConfigDb* pTransCtx = nullptr;
        ret = oDataDesc.GetPointer(
            propTransCtx, pTransCtx );
        if( ERROR( ret ) )
            break;

        CCfgOpener oTransCtx( pTransCtx );
        ret = oTransCtx.CopyProp(
            propSessHash, this );
        if( ERROR( ret ) )
            break;

        ret = oTransCtx.CopyProp(
            propLoginInfo, this );
        if( ERROR( ret ) )
            break;
        // ret = super::FetchData_Server(
        //     pDataDesc, fd, dwOffset,
        //     dwSize, pCallback );

    }while( 0 );

    return ret;
}

bool CRpcTcpBridgeAuth::IsKdcChannel()
{
    bool bNoEnc = false;
    gint32 ret = m_oSecCtx.GetBoolProp(
        propNoEnc, bNoEnc );

    if( ERROR( ret ) )
        return false;

    return bNoEnc;
}

gint32 CRpcTcpBridgeAuth::CheckSessExpired(
    const std::string& strSess )
{
    gint32 ret = 0;
    do{
        CCfgOpenerObj oCfg(
            ( CObjBase* )m_pSessChecker );

        guint32 dwRetries;
        ret = oCfg.GetIntProp(
            propRetries, dwRetries );
        if( ERROR( ret ) )
            break;

        if( dwRetries == MAX_NUM_CHECK )
        {
            ret = STATUS_MORE_PROCESS_NEEDED;
            break;
        }

        CAuthentServer* pAuth =
            ObjPtr( GetParent() );

        if( unlikely( pAuth == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }

        ret = pAuth->IsSessExpired( strSess );
        if( SUCCEEDED( ret ) || ret == ERROR_FAIL )
        {
            if( ret == STATUS_SUCCESS )
                ret = -EKEYEXPIRED;
            OnLoginFailed( nullptr, ret );
            ret = 0;
        }
        else if( ret == ERROR_FALSE )
        {
            ret = STATUS_MORE_PROCESS_NEEDED;
        }
        else if( ret == -ENOENT )
        {
            // no such a session
            break;
        }

    }while( 0 );

    return ret;
}

gint32 CRpcReqForwarderProxyAuth::BuildNewMsgToFwrd(
    IConfigDb* pReqCtx,
    DMsgPtr& pFwrdMsg,
    DMsgPtr& pNewMsg ) 
{

    if( pFwrdMsg.GetMember() ==
        SYS_METHOD( AUTH_METHOD_LOGIN ) )
    {
        pNewMsg = pFwrdMsg;
        return 0;
    }
    return super::BuildNewMsgToFwrd(
        pReqCtx, pFwrdMsg, pNewMsg );
}

gint32 CRpcReqForwarderProxyAuth::ForwardLogin(
    IConfigDb* pReqCtx,
    DBusMessage* pMsgRaw,
    DMsgPtr& pRespMsg,
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        DMsgPtr pNewMsg;
        DMsgPtr pOldMsg( pMsgRaw );
        ret = pNewMsg.CopyHeader( pMsgRaw );
        if( ERROR( ret ) )
            break;

        BufPtr pReqBuf( true );
        gint32 iType = 0;
        ret = pOldMsg.GetArgAt( 0, pReqBuf, iType );
        if( ERROR( ret ) )
            break;

        // insert a the propConnHandle, which
        // will be a component to make up the
        // session id.
        CCfgOpener oNewCtx;
        ret = oNewCtx.CopyProp(
            propConnHandle, pReqCtx );
        if( ERROR( ret ) )
            break;

        BufPtr pCtxBuf( true );
        ret = oNewCtx.Serialize( *pCtxBuf );
        if( ERROR( ret ) )
            break;

        const char* pData = pReqBuf->ptr();
        const char* pCtx = pCtxBuf->ptr();
        if( !dbus_message_append_args( pNewMsg,
            DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
            &pData, pReqBuf->size(),
            DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
            &pCtx, pCtxBuf->size(),
            DBUS_TYPE_INVALID ) )
        {
            ret = -ENOMEM;
            break;
        }

        return super::ForwardRequest( pReqCtx,
            pNewMsg, pRespMsg, pCallback );

    }while( 0 );

    return ret;
}

gint32 CRpcReqForwarderProxyAuth::ForwardRequest(
    IConfigDb* pReqCtx,
    DBusMessage* pMsgRaw,
    DMsgPtr& pRespMsg,
    IEventSink* pCallback )
{
    if( pReqCtx == nullptr ||
        pMsgRaw == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    DMsgPtr pMsg( pMsgRaw );
    CRpcRouter* pRouter = GetParent();
    std::string strDest = AUTH_DEST( pRouter );
    do{
        stdstr strIfName =
            std::move( pMsg.GetInterface() );

        bool bAuthIf = ( strIfName ==
            DBUS_IF_NAME( "IAuthenticate" ) );

        if( strDest != pMsg.GetDestination() )
        {
            if( bAuthIf )
            {
                DebugPrintEx( logErr, ret,
                    "Error auth Message" );
                ret = -EACCES;
            }
            break;
        }

        if( !pRouter->HasAuth() )
        {
            ret = -EBADMSG;
            break;
        }

        if( !bAuthIf )
        {
            ret = -EINVAL;
            break;
        }

        std::string strPath;
        CCfgOpener oReqCtx( pReqCtx );
        ret = oReqCtx.GetStrProp(
            propRouterPath, strPath );
        if( ERROR( ret ) )
            break;

        if( strPath != "/" )
        {
            ret = -EINVAL;
            break;
        }
        
        if( pMsg.GetMember() !=
            SYS_METHOD( AUTH_METHOD_LOGIN ) )
            break;

        return ForwardLogin( pReqCtx,
            pMsgRaw, pRespMsg, pCallback );

    }while( 0 );

    if( ERROR( ret ) )
        return ret;

    return super::ForwardRequest( pReqCtx,
        pMsgRaw, pRespMsg, pCallback );
}

gint32 CRpcReqForwarderAuth::LocalLogin(
    IEventSink* pCallback,
    const IConfigDb* pcfg )
{
    if( pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        TaskletPtr pDeferTask;

        IConfigDb* pCfg = 
            const_cast< IConfigDb* >( pcfg );

        ret = DEFER_IFCALLEX_NOSCHED2(
            0, pDeferTask, ObjPtr( this ),
            &CRpcReqForwarderAuth::LocalLoginInternal,
            nullptr, pCfg, pCallback );

        if( ERROR( ret ) )
            break;

        // queue the task in a seq taskgroup
        ret = AddLoginSeqTask(
            pDeferTask, false );

        if( SUCCEEDED( ret ) )
            ret = STATUS_PENDING;

    }while( 0 );

    return ret;
}

gint32 CRpcReqForwarderAuth::OpenRemotePort(
    IEventSink* pCallback,
    const IConfigDb* pCfg )
{
    if( pCfg == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CCfgOpener oCfg( pCfg );

        IConfigDb* pConnParams = nullptr;
        ret = oCfg.GetPointer(
            propConnParams, pConnParams );
        if( ERROR( ret ) )
            break;

        CConnParamsProxy oConn( pConnParams );
        if( !oConn.HasAuth() )
        {
            ret = -EINVAL;
            break;
        }

        ret = super::OpenRemotePort(
            pCallback, pCfg );

    }while( 0 );

    return ret;
}

#ifdef OA2
gint32 CRpcReqForwarderAuth::GetLatestHash(
    ObjPtr& pRegistry,
    stdstr& strHash )
{
    gint32 ret = 0;
    RFHANDLE hDir = 0;
    bool bClose = false;
    CRegistryFs* pRegfs = pRegistry;
    do{
        stdstr strPath = "/cookies";
        ret = pRegfs->OpenDir(
            strPath, R_OK, hDir );
        if( ERROR( ret ) )
            break;
        bClose = true;
        std::vector< KEYPTR_SLOT > vecDirEnt;
        ret = pRegfs->ReadDir( hDir, vecDirEnt );
        if( ERROR( ret ) )
            break;
        if( vecDirEnt.empty() )
        {
            ret = -ENOENT;
            break;
        }

        stdstr strVal;
        if( vecDirEnt.size() == 1 )
        {
            strVal = vecDirEnt[ 0 ].szKey;
            break;
        }

        struct timespec tv = { 0, 0 };
        for( auto& elem : vecDirEnt )
        {
            struct stat stBuf;
            stdstr strFile =
                strPath + "/" + elem.szKey;
            ret = pRegfs->GetAttr(
                strFile, stBuf ); 

            if( ERROR( ret ) )
                continue;

            if( stBuf.st_mtim.tv_sec > tv.tv_sec )
            {
                tv = stBuf.st_mtim;
                strVal = elem.szKey;
            }
            else if( stBuf.st_mtim.tv_sec == tv.tv_sec &&
                stBuf.st_mtim.tv_nsec > tv.tv_nsec )
            {
                strVal = elem.szKey;
                tv = stBuf.st_mtim;
            }
        }
        if( tv.tv_sec == 0 )
        {
            ret = -ENOENT;
            break;
        }
        strHash = strVal;

    }while( 0 );
    if( ERROR( ret ) )
        OutputMsg( ret, "Error cannot find valid "
            "credential to login" );
    if( bClose )
        pRegfs->CloseFile( hDir );

    return ret;
}

gint32 CRpcReqForwarderAuth::GetCookieByHash(
    IConfigDb* pConnParams,
    const stdstr& strHashOrigin )
{
    gint32 ret = 0;
    RegFsPtr pRegfs;
    bool bStop = false;
    do{
        CParamList oParams;
        stdstr strHome = GetHomeDir();
        strHome += "/.rpcf/";
        oParams.SetStrProp(
            propConfigPath, strHome + CLI_REG );
        ret = pRegfs.NewObj(
            clsid( CRegistryFs ),
            oParams.GetCfg() );
        if( ERROR( ret ) )
            break;
        ret = pRegfs->Start();
        if( ERROR( ret ) )
            break;
        bStop = true;
        RFHANDLE hFile;
        stdstr strPath = "/cookies/";
        bool bTryEnc = true;
        stdstr strHash = strHashOrigin;
        if( strHash.empty() )
        {
            ObjPtr pReg = pRegfs;
            ret = GetLatestHash( pReg, strHash );
            if( ERROR( ret ) )
                break;
            bTryEnc = false;
        }
        stdstr strFile = strPath + strHash;
        struct stat stBuf;
        ret = pRegfs->GetAttr( strFile, stBuf );
        if( ERROR( ret ) && !bTryEnc )
        {
            break;
        }
        else if( ERROR( ret ) && bTryEnc )
        {
            strFile = strPath + "z" + strHash;
            ret = pRegfs->GetAttr( strFile, stBuf );
            if( ERROR( ret ) )
                break;
        }

        ret = pRegfs->OpenFile(
            strFile, R_OK, hFile );
        if( ERROR( ret ) )
            break;
        BufPtr pBuf( true );
        ret = pBuf->Resize( stBuf.st_size );
        if( ERROR( ret ) )
            break;
        guint32 dwSize = stBuf.st_size;
        ret = pRegfs->ReadFile(
            hFile, pBuf->ptr(), dwSize, 0 );
        if( ERROR( ret ) )
            break;

        Json::Value valConfig;
        Json::CharReaderBuilder oBuilder;
        Json::CharReader* pReader = nullptr;
        pReader = oBuilder.newCharReader();
        if( pReader == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        if( !pReader->parse( pBuf->ptr(),
            pBuf->ptr() + pBuf->size(),
            &valConfig, nullptr ) )
        {
            ret = -EBADMSG;
        }
        delete pReader;
        if( !valConfig.isMember( "oCookie" ) ||
            !valConfig[ "oCookie" ].isObject() )
        {
            ret = -ENOENT;
            break;
        }
        Json::Value& oCookie =
            valConfig[ "oCookie" ];

        if( !oCookie.isMember( "name" ) ||
            !oCookie[ "name" ].isString() )
        {
            ret = -ENOENT;
            break;
        }
        if( oCookie[ "name" ].asString() !=
            "rpcf_code" )
        {
            ret = -ENOENT;
            break;
        }
        if( !oCookie.isMember( "value" ) )
        {
            ret = -ENOENT;
            break;
        }
 
        Variant oVar;
        pConnParams->GetProperty(
            propAuthInfo, oVar );
        IConfigDb* pAuthInfo = ( ObjPtr& )oVar;
        oVar =
            oCookie[ "value" ].asString();
        if( strHash[ 0 ] != 'z' )
        {
            pAuthInfo->SetProperty(
                propCookie, oVar );
            break;
        }
        else
        {
            pAuthInfo->SetProperty(
                propEncCookie, oVar );
        }
    }while( 0 );
    if( bStop )
        pRegfs->Stop();
    return ret;
}

gint32 CRpcReqForwarderAuth::CheckOAuth2Params(
    IConfigDb* pConnParams )
{
    gint32 ret = 0;
    do{
        Variant oVar;
        pConnParams->GetProperty(
            propAuthInfo, oVar );
        IConfigDb* pCfg = ( ObjPtr& )oVar;
        pCfg->GetProperty(
            propAuthMech, oVar );
        if( ( ( stdstr& )oVar ) != "OAuth2" )
        {
            ret = ERROR_FALSE;
            break;
        }
        stdstr strAuthUrl;

        CCfgOpener oai( pCfg );
        gint32 ret1 = oai.GetStrProp(
            propAuthUrl, strAuthUrl );

        if( ERROR( ret1 ) )
        {
            ret = GetCookieByHash(
                pConnParams, "" );
        }
        else
        {
            OutputMsg( 0, "Login with credential "
                "from '%s'",
                strAuthUrl.c_str() );
            stdstr strVal = strAuthUrl;
            stdstr strHash;
            GenShaHash( strVal.c_str(),
                strVal.size(), strHash );
             ret = GetCookieByHash(
                pConnParams, strHash );   
        }
        if( ERROR( ret ) )
        {
            OutputMsg( ret, "Error cannot find valid "
                "credential to login" );
        }

    }while( 0 );
    return ret;
}
#endif

gint32 CRpcReqForwarderAuth::LocalLoginInternal(
    IEventSink* pCallback,
    IConfigDb* pCfg,
    IEventSink* pInvTask )
{
    if( pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    bool bExist = false;
    CCfgOpener oReqCtx;
    CCfgOpener oResp;

    do{
        if( pCfg == nullptr ||
            pCallback == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        guint32 dwPortId = 0;
        CCfgOpener oCfg( pCfg );

        IConfigDb* pConnParams = nullptr;
        ret = oCfg.GetPointer(
            propConnParams, pConnParams );
        if( ERROR( ret ) )
            break;

        CConnParamsProxy oConn( pConnParams );
        if( !oConn.HasAuth() )
        {
            ret = -EINVAL;
            break;
        }

#ifdef OA2
        ret = CheckOAuth2Params( pConnParams );
        if( ret == ERROR_FALSE )
            ret = 0;
        if( ERROR( ret ) )
            break;
#endif

        std::string strSender;
        ret = oCfg.GetStrProp(
            propSrcDBusName, strSender );

        if( ERROR( ret ) )
            break;

        std::string strSrcUniqName;
        ret = oCfg.GetStrProp(
            propSrcUniqName, strSrcUniqName );

        if( ERROR( ret ) )
            break;

        if( IsSepConns() )
        {
            CCfgOpener oConnCfg( pConnParams );
            oConnCfg[ propSrcUniqName ] =
                strSrcUniqName;
        }
        std::string strRouterPath;
        ret = oCfg.GetStrProp(
            propRouterPath, strRouterPath );
        if( ERROR( ret ) )
            break;

        InterfPtr pIf;
        CRpcRouter* pRouter = GetParent();
        ret = pRouter->GetBridgeProxy(
            pConnParams, pIf );

        if( SUCCEEDED( ret ) )
        {
            CRpcServices* pSvc = pIf;
            PortPtr pPort = pSvc->GetPort();
            CCfgOpenerObj oPortCfg(
                ( CObjBase* )pPort );

            ret = oPortCfg.GetIntProp(
                propPdoId, dwPortId );
            
            if( ERROR( ret ) )
                break;

            CCfgOpenerObj oIfCfg(
                ( CObjBase* )pIf );

            oIfCfg.SetIntProp(
                propPortId, dwPortId );

            ret = AddRefCount( dwPortId,
                strSrcUniqName, strSender );

            if( ERROR( ret ) )
                break;

            if( ret == 1 && IsRfcEnabled() )
            {
                ret = CreateGrpRfc( dwPortId,
                    strSrcUniqName, strSender );
                if( ERROR( ret ) )
                    break;
            }
            // schedule a checkrouter path task
            oReqCtx.SetIntProp(
                propConnHandle, dwPortId );

            oReqCtx.SetStrProp(
                propRouterPath, strRouterPath );

            oReqCtx.CopyProp( propConnParams,
                ( CObjBase* )pIf );

            // for rollback
            oReqCtx.SetStrProp(
                propSrcUniqName, strSrcUniqName );

            oReqCtx.SetStrProp(
                propSrcDBusName, strSender );

            bExist = true;

            if( strRouterPath == "/" )
            {
                oResp.SetIntProp(
                    propConnHandle, dwPortId );

                oResp.CopyProp( propConnParams,
                    ( CObjBase* )pIf );

                oResp.SetStrProp(
                    propRouterPath, "/" );

                DebugPrint( ret,
                    "The bridge proxy already"
                    " exists..., portid=%d,"
                    " uniqName=%s, sender=%s",
                    dwPortId,
                    strSrcUniqName.c_str(),
                    strSender.c_str() );

                ret = 0;
                break;
            }

            oReqCtx.CopyProp( propSessHash,
                ( CObjBase* )pIf );

            // schedule a checkrouterpath task
            TaskletPtr pChkRt;
            ret = DEFER_HANDLER_NOSCHED(
                pChkRt, ObjPtr( this ),
                &CRpcReqForwarder::CheckRouterPath,
                pInvTask, oReqCtx.GetCfg() );
            if( ERROR( ret ) )
                break;

            // schedule a new task to release the
            // login seq task queue
            CIoManager* pMgr = pSvc->GetIoMgr();
            ret = pMgr->RescheduleTask( pChkRt );
            if( SUCCEEDED( ret ) )
                ret = STATUS_MORE_PROCESS_NEEDED;
        }
        else if( ret == -ENOENT )
        {
            // create the authenticate proxy which
            // will finally create the bridge
            // proxy on success.
            ret = CreateBridgeProxyAuth(
                pCallback, pCfg, pInvTask );
        }
        break;

    }while( 0 );

    if( ret == STATUS_MORE_PROCESS_NEEDED )
    {
        // release the login queue only
        return STATUS_SUCCESS;
    }

    if( ret != STATUS_PENDING )
    {
        oResp[ propReturnValue ] = ret;

        if( ERROR( ret ) )
        {
            oResp.RemoveProperty(
                propConnHandle );

            oResp.RemoveProperty(
                propConnParams );

            if( bExist )
            {
                SchedToStopBridgeProxy(
                    oReqCtx.GetCfg() );
            }
        }

        OnServiceComplete(
            oResp.GetCfg(), pInvTask );
    }

    return ret;
}

gint32 CRpcReqForwarderAuth::InitUserFuncs()
{
    gint32 ret = super::InitUserFuncs();
    if( ERROR( ret ) )
        return ret;

    BEGIN_IFHANDLER_MAP( CRpcReqForwarderAuth );

    ADD_SERVICE_HANDLER(
        CRpcReqForwarderAuth::LocalLogin,
        SYS_METHOD_LOCALLOGIN );

    END_IFHANDLER_MAP;

    return 0;
}

gint32 CAuthentProxy::BuildLoginTask(
    IEventSink* pIf,
    IEventSink* pCallback,
    TaskletPtr& pTask )
{
    if( pIf == nullptr || pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        std::string strMech = GET_MECH( pIf );

        if( strMech == "krb5" )
        {
#ifdef KRB5
            TaskletPtr pLoginTask;
            ret = DEFER_IFCALLEX2_NOSCHED2(
                0, pLoginTask, ObjPtr( pIf ),
                &CK5AuthProxy::DoLogin,
                nullptr );

            if( ERROR( ret ) )
                break;

            CIfRetryTask* pRetryTask = pLoginTask;
            pRetryTask->SetClientNotify( pCallback );

            pTask = pLoginTask;
            break;
#endif
        }
        else if( strMech == "OAuth2" )
        {
#ifdef OA2
            TaskletPtr pLoginTask;
            ret = DEFER_IFCALLEX2_NOSCHED2(
                0, pLoginTask, ObjPtr( pIf ),
                &COAuth2LoginProxy::DoLogin,
                nullptr );

            if( ERROR( ret ) )
                break;

            CIfRetryTask* pRetryTask = pLoginTask;
            pRetryTask->SetClientNotify( pCallback );

            pTask = pLoginTask;
            break;

#endif
        }
        else if( strMech == "SimpAuth" )
        {
            TaskletPtr pLoginTask;
            ret = DEFER_IFCALLEX2_NOSCHED2(
                0, pLoginTask, ObjPtr( pIf ),
                &CSimpAuthLoginProxyImpl::DoLogin,
                nullptr );

            if( ERROR( ret ) )
                break;

            CIfRetryTask* pRetryTask = pLoginTask;
            pRetryTask->SetClientNotify( pCallback );

            pTask = pLoginTask;
            break;
        }


        ret = -ENOTSUP;

    }while( 0 );

    return ret;
}

gint32 CAuthentProxy::CorrectInstName(
    CIoManager* pMgr,
    IConfigDb* pCfg )
{
    gint32 ret = 0;
    do{
        stdstr strName;

        ret = pMgr->GetCmdLineOpt(
            propSvrInstName, strName );
        if( ERROR( ret ) )
            break;

        // no more correction
        if( strName == MODNAME_RPCROUTER )
            break;

        CCfgOpener oCfg( pCfg );
        stdstr strPath = oCfg[ propObjPath ];
        stdstr strDest = oCfg[ propDestDBusName ];

        std::regex e ( MODNAME_RPCROUTER );
        strPath = std::regex_replace(
            strPath, e, strName,
            std::regex_constants::format_first_only );

        strDest = std::regex_replace(
            strDest, e, strName,
            std::regex_constants::format_first_only );

        oCfg[ propObjPath ] = strPath;
        oCfg[ propDestDBusName ] = strDest;

        if( oCfg.exist( propObjList ) )
        {
            ObjPtr pObj;
            ret = oCfg.GetObjPtr( propObjList, pObj );
            if( ERROR( ret ) )
                break;

            ObjVecPtr pObjVec( pObj );
            if( pObjVec.IsEmpty() )
            {
                ret = -EFAULT;
                break;
            }
            for( auto pElem : ( *pObjVec )() )
            {
                CMessageMatch* pMatch = pElem;
                pMatch->SetObjPath( strPath );
                Variant oVar = strDest;
                pMatch->SetProperty(
                    propDestDBusName, oVar );
            }
        }
    }while( 0 );

    return ret;
}

gint32 CAuthentProxy::CreateSessImpl(
    const IConfigDb* pConnParams,
    CRpcRouter* pRouter,
    InterfPtr& pImpl )
{
    if( pConnParams == nullptr ||
        pRouter == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CCfgOpener oConn;
        ret = oConn.GetCfg()->Clone( *pConnParams );
        if( ERROR( ret ) )
            break;

        // force the router path to be the root
        // oConn.SetStrProp( propRouterPath, "/" );

        IConfigDb* pAuth = nullptr;
        ret = oConn.GetPointer(
            propAuthInfo, pAuth );
        if( ERROR( ret ) )
            break;

        CCfgOpener oAuth( pAuth );
        std::string strMech;
        ret = oAuth.GetStrProp(
            propAuthMech, strMech );

        if( ERROR( ret ) )
            break;

        std::string strObjName;
        if( strMech == "krb5" )
        {
            strObjName = OBJNAME_AUTHSVR;
        }
        else if( strMech == "OAuth2" )
        {
            strObjName = "OAuth2LoginServer";
        }
        else if( strMech == "SimpAuth" )
        {
            strObjName = "SimpAuthLoginServer";
        }
        else
        {
            ret = -ENOTSUP;
            break;
        }

        CCfgOpener oCfg;
        CIoManager* pMgr = pRouter->GetIoMgr();

        oCfg.SetPointer( propIoMgr, pMgr );

        ret = CRpcServices::LoadObjDesc(
            DESC_FILE, strObjName,
            false, oCfg.GetCfg() );

        if( ERROR( ret ) )
            break;

        // replace the conn params from the desc
        // file with what we have.
        oCfg.SetObjPtr( propConnParams,
            ObjPtr( oConn.GetCfg() ) );

        oCfg.SetPointer(
            propRouterPtr, pRouter );

        ret = CorrectInstName(
            pMgr, oCfg.GetCfg() );

        oCfg.SetIntProp( propIfStateClass,
            clsid( CRemoteProxyStateAuth ) );

        InterfPtr pIf;
        if( strMech == "krb5" )
        {
#ifdef KRB5
            // use a special ifstate
            ret = pIf.NewObj(
                clsid( CAuthentProxyK5Impl ),
                oCfg.GetCfg() );

            if( SUCCEEDED( ret ) )
                pImpl = pIf;
#endif
        }
        else if( strMech == "OAuth2" )
        {
#ifdef OA2
            ret = pIf.NewObj(
                clsid( COAuth2LoginProxyImpl ),
                oCfg.GetCfg() );
            if( SUCCEEDED( ret ) )
                pImpl = pIf;
#endif
        }
        else if( strMech == "SimpAuth" )
        {
            ret = pIf.NewObj(
                clsid( CSimpAuthLoginProxyImpl ),
                oCfg.GetCfg() );
            if( SUCCEEDED( ret ) )
                pImpl = pIf;
        }
        else
        {
            ret = -ENOTSUP;
        }

    }while( 0 );

    return ret;
}

gint32 CAuthentProxy::StopSessImpl(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        ObjPtr pSessImpl;
        ret = GetSessImpl( pSessImpl );
        if( ERROR( ret ) )
            break;

        CParamList oParams;
        oParams.SetObjPtr(
            propIfPtr, pSessImpl );

        TaskletPtr pDummy;
        ret = pDummy.NewObj( 
            clsid( CIfDummyTask ) );
        if( ERROR( ret ) )
            break;

        CRpcServices* pSvc = pSessImpl;
        ret = pSvc->TestSetState( cmdShutdown );
        if( ERROR( ret ) )
        {
            ret = 0;
            break;
        }
        ret = DEFER_CALL( GetIoMgr(),
            ObjPtr( pSvc ),
            &CRpcServices::Shutdown,
            pCallback == nullptr ?
                ( IEventSink* )pDummy : pCallback );
        if( SUCCEEDED( ret ) &&
            pCallback != nullptr )
            ret = STATUS_PENDING;

    }while( 0 );

    return ret;
}

gint32 CAuthentProxy::OnPreStop(
    IEventSink* pCallback )
{
    // only place to stop the auth proxy 
    return StopSessImpl( pCallback );
}

gint32 CAuthentProxy::OnPostStop(
    IEventSink* pCallback )
{
    m_pSessImpl.Clear();
    return 0;
}

gint32 CAuthentProxy::Login(
    IEventSink* pCallback,
    IConfigDb* pReq,
    CfgPtr& pResp )
{
    gint32 ret = 0;
    do{
        CRpcServices* pSvc = m_pSessImpl;
        if( pSvc == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        
        IAuthenticateProxy* pProxy =
        dynamic_cast< IAuthenticateProxy* >
            ( pSvc );

        if( pProxy == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ret = pProxy->Login(
            pCallback, pReq, pResp );

    }while( 0 );

    return ret;
}

gint32 CAuthentProxy::MechSpecReq(
    IEventSink* pCallback,
    IConfigDb* pReq,
    CfgPtr& pResp )
{
    gint32 ret = 0;
    do{
        CRpcServices* pSvc = m_pSessImpl;
        if( pSvc == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        
        IAuthenticateProxy* pProxy =
        dynamic_cast< IAuthenticateProxy* >
            ( pSvc );

        if( pProxy == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ret = pProxy->MechSpecReq(
            pCallback, pReq, pResp );

    }while( 0 );

    return ret;
}

gint32 CAuthentProxy::GetSess(
    std::string& strSess ) const
{
    gint32 ret = 0;
    do{
        CRpcServices* pSvc = m_pSessImpl;
        if( pSvc == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        
        IAuthenticateProxy* pProxy =
        dynamic_cast< IAuthenticateProxy* >
            ( pSvc );

        if( pProxy == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ret =  pProxy->GetSess( strSess );

    }while( 0 );

    return ret;
}

gint32 CAuthentProxy::WrapMessage(
    BufPtr& pInMsg,
    BufPtr& pOutMsg )
{
    gint32 ret = 0;
    do{
        CRpcServices* pSvc = m_pSessImpl;
        if( pSvc == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        
        IAuthenticateProxy* pProxy =
        dynamic_cast< IAuthenticateProxy* >
            ( pSvc );

        if( pProxy == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ret =  pProxy->WrapMessage(
            pInMsg, pOutMsg );

    }while( 0 );

    return ret;
}

gint32 CAuthentProxy::UnwrapMessage(
    BufPtr& pInMsg,
    BufPtr& pOutMsg )
{
    gint32 ret = 0;
    do{
        CRpcServices* pSvc = m_pSessImpl;
        if( pSvc == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        
        IAuthenticateProxy* pProxy =
        dynamic_cast< IAuthenticateProxy* >
            ( pSvc );

        if( pProxy == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ret =  pProxy->UnwrapMessage(
            pInMsg, pOutMsg );

    }while( 0 );

    return ret;
}

gint32 CAuthentProxy::GetMicMsg(
    BufPtr& pInMsg,/* [ in ] */
    BufPtr& pOutMic )/* [ out ] */
{
    gint32 ret = 0;
    do{
        CRpcServices* pSvc = m_pSessImpl;
        if( pSvc == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        
        IAuthenticateProxy* pProxy =
        dynamic_cast< IAuthenticateProxy* >
            ( pSvc );

        if( pProxy == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ret =  pProxy->GetMicMsg(
            pInMsg, pOutMic );

    }while( 0 );

    return ret;
}

gint32 CAuthentProxy::VerifyMicMsg(
    BufPtr& pInMsg,/* [ in ] */
    BufPtr& pInMic )/* [ in ] */
{
    gint32 ret = 0;
    do{
        CRpcServices* pSvc = m_pSessImpl;
        if( pSvc == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        
        IAuthenticateProxy* pProxy =
        dynamic_cast< IAuthenticateProxy* >
            ( pSvc );

        if( pProxy == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ret =  pProxy->VerifyMicMsg(
            pInMsg, pInMic );

    }while( 0 );

    return ret;
}

gint32 CAuthentProxy::InitEnvRouter(
    CIoManager* pMgr )
{
#ifdef KRB5
    return CK5AuthProxy::InitEnvRouter( pMgr );
#endif
    return 0;
}

void CAuthentProxy::DestroyEnvRouter(
    CIoManager* pMgr )
{
#ifdef KRB5
    CK5AuthProxy::DestroyEnvRouter( pMgr );
#endif
}

gint32 CRpcTcpBridgeProxyAuth::OnEnableComplete(
    IEventSink* pCallback,
    IEventSink* pIoReq,
    IConfigDb* pReqCtx )
{
    if( pIoReq == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CCfgOpenerObj oReq( pIoReq );
        IConfigDb* pResp = nullptr;
        ret = oReq.GetPointer(
            propRespPtr, pResp );

        if( ERROR( ret ) )
            break;

        CCfgOpener oResp( pResp );
        gint32 iRet = 0;
        ret = oResp.GetIntProp(
            propReturnValue,
            ( guint32& ) iRet );

        if( ERROR( ret ) )
            break;

        if( ERROR( iRet ) )
        {
            ret = iRet;
            break;
        }

        CCfgOpener oReqCtx( pReqCtx );
        CStlObjVector* pVec;
        ret = oReqCtx.GetPointer( 0, pVec );
        if( ERROR( ret ) )
            break;

        std::vector< MatchPtr > vecMatches;
        for( auto elem : ( *pVec )() )
        {
            MatchPtr pMatch = elem;
            vecMatches.push_back( pMatch );
        }

        ret = StartRecvTasks( vecMatches );

    }while( 0 );

    return ret;
}

gint32 CRpcTcpBridgeProxyAuth::EnableInterfaces()
{
    gint32 ret = 0;
    TaskGrpPtr pTaskGrp;
    TaskletPtr pRespCb;
    do{
        if( IsKdcChannel() )
        {
            ret = 0;
            break;
        }
        std::vector< MatchPtr > vecMatches;
        for( auto& elem : m_vecMatches )
        {
            CCfgOpenerObj oMatch(
                ( CObjBase* )elem );
            std::string strIfName;
            bool bDummy = true;
            ret = oMatch.GetBoolProp(
                propDummyMatch, bDummy );
            if( ERROR( ret ) )
                continue;

            if( bDummy )
            {
                oMatch.RemoveProperty(
                    propDummyMatch );
                vecMatches.push_back( elem );
            }
        }

        if( vecMatches.empty() )
            break;

        CParamList oParams;            
        oParams[ propIfPtr ] = ObjPtr( this );

        ret = pTaskGrp.NewObj( 
            clsid( CIfTaskGroup ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        pTaskGrp->SetRelation( logicAND );

        ObjVecPtr pObjVec;
        ret = pObjVec.NewObj(
            clsid( CStlObjVector ) );

        if( ERROR( ret ) )
            break;

        oParams.Push( true );
        for( auto elem : vecMatches )
        {
            oParams[ propMatchPtr ] =
                ObjPtr( elem );

            TaskletPtr pEnableEvtTask;
            ret = pEnableEvtTask.NewObj(
                clsid( CIfEnableEventTask ),
                oParams.GetCfg() );

            if( ERROR( ret ) )
                break;

            pTaskGrp->AppendTask(
                pEnableEvtTask );

            ( *pObjVec )().push_back(
                ObjPtr( elem ) );
        }

        CParamList oReqCtx;
        oReqCtx.Push( ObjPtr( pObjVec ) );
        ret = NEW_PROXY_RESP_HANDLER2(
            pRespCb, ObjPtr( this ),
            &CRpcTcpBridgeProxyAuth::OnEnableComplete,
            nullptr,
            ( IConfigDb* )oReqCtx.GetCfg() );

        if( ERROR( ret ) )
            break;

        pTaskGrp->SetClientNotify(
            ( IEventSink* )pRespCb );

        TaskletPtr pTask = pTaskGrp;
        ret = AddAndRun( pTask );
        if( SUCCEEDED( ret ) )
        {
            ret = pTask->GetError();
            if( ERROR( ret ) ||
                ret == STATUS_PENDING )
                break;

            CParamList oParms;
            oParams[ propReturnValue ] = 0;

            TaskletPtr pDummy;
            pDummy.NewObj(
                clsid( CIfDummyTask ) );
            CCfgOpener oCfg( ( IConfigDb* )
                pDummy->GetConfig() );
            oCfg.SetPointer( propRespPtr,
                ( IConfigDb* )oParams.GetCfg() );

            ret = OnEnableComplete( nullptr,
                pDummy, oReqCtx.GetCfg() );
        }

    }while( 0 );

    if( !ERROR( ret ) )
        return ret;

    if( !pTaskGrp.IsEmpty() )
        ( *pTaskGrp )( eventCancelTask );

    if( !pRespCb.IsEmpty() )
        ( *pRespCb )( eventCancelTask );

    return ret;
};

gint32 CRpcTcpBridgeProxyAuth::OnPostStart(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        CRpcRouter* pRouter = GetParent();
        CRpcRouterReqFwdrAuth* pRtAuth =
            ObjPtr( pRouter );

        if( unlikely( pRtAuth == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }

        // bind the bridge proxy and the auth
        // proxy
        // this is the only valid place we can
        // bind the two object, because the call
        // is serialized by the seqtask, and if
        // the bridge proxy goes offline
        // immediately, it will not happen till
        // this call ends.
        InterfPtr pIf;
        CRpcReqForwarderAuth* pReqFwdr = nullptr;
        ret = pRtAuth->GetReqFwdr( pIf );
        if( ERROR( ret ) )
            break;

        pReqFwdr = pIf;
        if( unlikely( pReqFwdr == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }

        IConfigDb* pReqCtx = nullptr;
        ret = pReqFwdr->GetStartCtx( pReqCtx );
        if( ERROR( ret ) )
            break;

        CCfgOpener oReqCtx( pReqCtx );
        CRpcServices* pSvc = nullptr;        
        ret = oReqCtx.GetPointer(
            propIfPtr, pSvc );
        if( ERROR( ret ) )
            break;

        InterfPtr pAuthObj = pSvc;
        CAuthentProxy* pap = ObjPtr( this );
        pap->SetSessImpl( pAuthObj );

        IAuthenticateProxy* pAuth =
        dynamic_cast < IAuthenticateProxy* >
                ( pSvc );

        if( unlikely( pAuth == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }
        pAuth->SetParent( this );

    }while( 0 );

    return ret;
}

gint32 CRpcTcpBridgeProxyAuth::OnPostStop(
    IEventSink* pCallback )
{
    SetSessHash( "", false );
    return super::OnPostStop( pCallback );
}

gint32 CRpcTcpBridgeProxyAuth::SetSessHash(
    const std::string& strHash, bool bNoEnc )
{
    // notify the bridge the authentication
    // context is setup.
    gint32 ret = 0;
    do{
        std::string strHash1;
        CCfgOpenerObj oIfCfg( this );
        ret = oIfCfg.GetStrProp(
            propSessHash, strHash1 );
        if( SUCCEEDED( ret ) &&
            strHash1.substr( 0, 2 ) == "AU" )
        { 
            if( strHash.empty() )
            {
                // clear the hash
                oIfCfg.RemoveProperty(
                    propSessHash );
                m_oSecCtx.Clear();
                ret = 0;
            }
            else
            {
                ret = -EEXIST;
            }
            break;
        }

        ret = 0;
        if( strHash.empty() )
            break;

        if( strHash.substr( 0, 4 ) == "AUoa" ||
            strHash.substr( 0, 4 ) == "AUsa" )
        {
            // OAuth2 or SimpAuth
            EnableInterfaces();
            break;
        }

        CAuthentProxy* psp = ObjPtr( this );
        ObjPtr pSessImpl;
        ret = psp->GetSessImpl( pSessImpl );
        if( ERROR( ret ) )
            break;

        CCfgOpener& oCtx = m_oSecCtx;
        if( !bNoEnc )
        {
            oCtx.SetObjPtr(
                propObjPtr, pSessImpl );

            IConfigDb* pAuthInfo =
                GET_AUTH( ( CObjBase* )pSessImpl );

            if( ERROR( ret ) )
                break;

            oCtx.CopyProp(
                propSignMsg, pAuthInfo );
        }

        oIfCfg.SetStrProp(
            propSessHash, strHash );

        oCtx.SetBoolProp(
            propNoEnc, bNoEnc );

        oCtx.SetStrProp(
            propSessHash, strHash );

        oCtx.SetBoolProp(
            propIsServer, false );

        PortPtr pPort;
        GET_TARGET_PORT( pPort );
        if( ERROR( ret ) )
            break;

        BufPtr pBuf( true );
        bool bFound = false;
        while( !pPort.IsEmpty() )
        {
            ret = GetPortProp( pPort,
                propLowerPortPtr, pBuf );    
            if( ERROR( ret ) )
            {
                ret = 0;
                break;
            }

            pPort = ( ObjPtr& )*pBuf;
            if( pPort.IsEmpty() )
                break;

            ret = GetPortProp( pPort,
                propPortClass, pBuf );
            if( ERROR( ret ) )
            {
                ret = 0;
                continue;
            }
            std::string strClass = *pBuf;
            if( strClass != PORT_CLASS_SEC_FIDO )
                continue;

            bFound = true;
            break;
        }

        if( !bFound )
        {
            ret = ERROR_FAIL;
            break;
        }

        *pBuf = ObjPtr( m_oSecCtx.GetCfg() );
        SetPortProp( pPort, propSecCtx, pBuf );

        EnableInterfaces();


    }while( 0 );

    return ret;
}

bool CRpcTcpBridgeProxyAuth::IsKdcChannel()
{
    bool bNoEnc = false;
    gint32 ret = 0;
    ret = m_oSecCtx.GetBoolProp(
        propNoEnc, bNoEnc );
    if( ERROR( ret ) )
        return false;
    return bNoEnc;
}

gint32 CRpcReqForwarderAuth::OnSessImplStarted(
    IEventSink* pInvTask,
    IEventSink* pIoReq,
    IConfigDb* pReqCtx )
{
    if( pInvTask == nullptr ||
        pIoReq == nullptr ||
        pReqCtx == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    CParamList oParams;
    CCfgOpener oReqCtx( pReqCtx );

    do{
        SetStartCtx( nullptr );
        CCfgOpenerObj oReq( pIoReq );
        IConfigDb* pResp = nullptr;
        ret = oReq.GetPointer(
            propRespPtr, pResp );
        if( ERROR( ret ) )
            break;

        CCfgOpener oResp( pResp );
        gint32 iRet = 0;
        ret = oResp.GetIntProp(
            propReturnValue,
            ( guint32& ) iRet );

        if( ERROR( ret ) )
            break;

        if( ERROR( iRet ) )
        {
            ret = iRet;
            break;
        }

    }while( 0 );

    if( ERROR( ret ) )
    {
        oParams[ propReturnValue ] = ret;
        OnServiceComplete(
            oParams.GetCfg(), pInvTask );
    }

    if( ret == STATUS_PENDING )
        ret = 0;

    return ret;
}

gint32 CRpcReqForwarderAuth::OnSessImplLoginCompleteSafe(
    IEventSink* pInvTask,
    IEventSink* pCallback,
    IConfigDb* pReqCtx )
{
    if( pInvTask == nullptr ||
        pCallback == nullptr ||
        pReqCtx == nullptr )
        return -EINVAL;

    TaskletPtr pSeqTask;
    gint32 ret = DEFER_IFCALLEX_NOSCHED(
        pSeqTask, ObjPtr( this ),
        &CRpcReqForwarderAuth::OnSessImplLoginComplete,
        pInvTask, pReqCtx );
    if( ERROR( ret ) )
        return ret;

    CIfRetryTask* pTask = pSeqTask;
    pTask->SetClientNotify( pCallback );
    CRpcRouter* pRouter = GetParent();

    // by adding this task to the router's seq
    // task, we want to prevent the potential
    // contention with the eventRmtSvrOffline
    ret = pRouter->AddSeqTask( pSeqTask );
    if( SUCCEEDED( ret ) )
        ret = STATUS_PENDING;

    return ret;
}
    
gint32 CRpcReqForwarderAuth::OnSessImplLoginComplete(
    IEventSink* pInvTask,
    IConfigDb* pReqCtx )
{
    if( pInvTask == nullptr ||
        pReqCtx == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    CParamList oParams;
    CCfgOpener oReqCtx( pReqCtx );

    do{
        CRpcServices* pap = nullptr;
        ret = oReqCtx.GetPointer(
            propIfPtr, pap );
        if( ERROR( ret ) )
            break;

        IAuthenticateProxy* pAuth =
            dynamic_cast < IAuthenticateProxy* >
            ( pap );

        if( unlikely( pAuth == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }

        CRpcServices* pbp = pAuth->GetParent();
        if( unlikely( pbp == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }

        if( !pbp->IsConnected() )
        {
            ret = ERROR_STATE;
            break;
        }

        guint32 dwTimeoutSec = 0;
        ret = oReqCtx.GetIntProp(
            propTimeoutSec, dwTimeoutSec );
        if( ERROR( ret ) )
            break;

        guint64 qwts = 0;
        ret = oReqCtx.GetQwordProp(
            propTimestamp, qwts );
        if( ERROR( ret ) )
            break;

        timespec tv;
        clock_gettime( CLOCK_REALTIME, &tv );
        guint64 qwNow = tv.tv_sec;
        guint32 dwDiff = abs( (
            ( gint64 )qwNow - ( gint64 )qwts ) );

        if( dwDiff > dwTimeoutSec )
        {
            ret = -ETIMEDOUT;
            DebugPrintEx( logErr, ret,
                "Error login timeout" );
            break;
        }
        
        // at this moment, the auth proxy should
        // have the correct connhandle in place.
        guint32 dwPortId = 0;
        CCfgOpenerObj oapCfg( pap );
        ret = oapCfg.GetIntProp(
            propConnHandle, dwPortId );
        if( ERROR( ret ) )
            break;

        ret = oReqCtx.SetIntProp(
            propConnHandle, dwPortId );
        if( ERROR( ret ) )
            break;

        bool bNoEnc;
        ret = oapCfg.GetBoolProp(
            propNoEnc, bNoEnc );
        if( ERROR( ret ) )
            bNoEnc = false;

        std::string strHash;
        pAuth->GetSess( strHash );

        CRpcTcpBridgeProxyAuth* pProxy =
            ObjPtr( pbp );

        ret = pProxy->
            SetSessHash( strHash, bNoEnc );

        if( ERROR( ret ) )
            break;

        ret = oReqCtx.CopyProp(
            propSessHash, ( CObjBase* )pbp );
        if( ERROR( ret ) )
            break;

        if( true )
        {
            std::string strSender;
            std::string strUniqName;

            ret = oReqCtx.GetStrProp(
                propSrcDBusName, strSender );
            if( ERROR( ret ) )
                break;

            ret = oReqCtx.GetStrProp(
                propSrcUniqName, strUniqName );
            if( ERROR( ret ) )
                break;

            CCfgOpenerObj oIfCfg(
                ( CObjBase* )pProxy );

            guint32 dwPortId = 0;
            ret = oIfCfg.GetIntProp(
                propPortId, dwPortId );
            
            if( ERROR( ret ) )
                break;

            ret = AddRefCount( dwPortId,
                strUniqName, strSender );

            if( ERROR( ret ) )
                break;

            if( ret == 1 && IsRfcEnabled() )
            {
                ret = CreateGrpRfc( dwPortId,
                    strUniqName, strSender );
                if( ERROR( ret ) )
                    break;
            }
        }
        // update the connparams with the newest
        // one from the bridge proxy.
        oReqCtx.CopyProp(
            propConnParams, pbp );

        std::string strPath;
        ret = oReqCtx.GetStrProp(
            propRouterPath, strPath );

        if( ERROR( ret ) )
            break;

        // ok the LocalLogin request is done
        if( strPath == "/" )
            break;

        // schedule a checkrouterpath task
        TaskletPtr pChkRt;
        ret = DEFER_HANDLER_NOSCHED(
            pChkRt, ObjPtr( this ),
            &CRpcReqForwarder::CheckRouterPath,
            pInvTask, pReqCtx );
        if( ERROR( ret ) )
            break;

        TaskletPtr pDummy;
        ret = pDummy.NewObj( 
            clsid( CIfDummyTask ) );
        if( ERROR( ret ) )
            break;

        ret = pProxy->CRpcTcpBridgeProxy::
            OnPostStart( pDummy );
        if( ERROR( ret ) )
            break;

        // a new task to release the seq task
        // queue
        CAuthentProxy* pabp =
            dynamic_cast< CAuthentProxy* >
                ( pProxy );

        CIoManager* pMgr = pabp->GetIoMgr();
        ret = pMgr->RescheduleTask( pChkRt );
        if( SUCCEEDED( ret ) )
            ret = STATUS_PENDING;

    }while( 0 );

    oReqCtx.RemoveProperty( propIfPtr );
    oParams[ propReturnValue ] = ret;
    if( SUCCEEDED( ret ) )
    {
        oParams.CopyProp(
            propConnHandle, pReqCtx );

        oParams.CopyProp(
            propConnParams, pReqCtx );

        oParams.CopyProp(
            propRouterPath, pReqCtx );
    }

    if( ret != STATUS_PENDING )
    {
        OnServiceComplete(
            oParams.GetCfg(), pInvTask );
    }

    if( ret == STATUS_PENDING )
    {
        // release the login task queue the rest
        // part, CheckRouterPath can run freely.
        ret = 0;
    }

    return ret;
}

gint32 CRpcReqForwarderAuth::OnPostStop(
    IEventSink* pCallback )
{ return super::OnPostStop( pCallback ); }

gint32 CRpcReqForwarderAuth::BuildStartAuthProxyTask(
    IEventSink* pInvTask,
    IConfigDb* pCfg,
    TaskletPtr& pTask )
{
    if( pInvTask == nullptr ||
        pCfg == nullptr )
        return -EINVAL;

    gint32 ret = 0;

    TaskGrpPtr pTaskGrp;
    TaskletPtr pRespCb;
    TaskletPtr pRespCb1;
    do{
        CParamList oParams;
        oParams[ propIfPtr ] = ObjPtr( this );
        ret = pTaskGrp.NewObj(
            clsid( CIfTransactGroup ),
            oParams.GetCfg() );
        if( ERROR( ret ) )
            break;

        CIfTransactGroup* pTransGrp = pTaskGrp;
        pTransGrp->SetTaskRelation( logicAND );

        IConfigDb* pConnParams = nullptr;
        CCfgOpener oCfg( pCfg );

        ret = oCfg.GetPointer(
            propConnParams, pConnParams );
        if( ERROR( ret ) )
            break;

        InterfPtr pIf;
        CRpcRouter* pRouter = GetParent();
        ret = CAuthentProxy::CreateSessImpl(
            pConnParams, pRouter, pIf );
        if( ERROR( ret ) )
            break;

        CParamList oReqCtx;

        oReqCtx.SetObjPtr( propIfPtr, pIf );

        ret = oReqCtx.SetPointer(
            propConnParams, pConnParams );
        if( ERROR( ret ) )
            break;

        ret = oReqCtx.CopyProp(
            propRouterPath, pCfg );
        if( ERROR( ret ) )
            break;

        ret = oReqCtx.CopyProp(
            propSrcDBusName, pCfg );
        if( ERROR( ret ) )
            break;

        ret = oReqCtx.CopyProp(
            propSrcUniqName, pCfg );
        if( ERROR( ret ) )
            break;

        ret = oReqCtx.CopyProp(
            propTimestamp, pCfg );
        if( ERROR( ret ) )
            break;

        ret = oReqCtx.CopyProp(
            propTimeoutSec, pCfg );
        if( ERROR( ret ) )
            break;

        IConfigDb* pReqCtx = oReqCtx.GetCfg();

        ret = NEW_PROXY_RESP_HANDLER2(
            pRespCb, ObjPtr( this ),
            &CRpcReqForwarderAuth::OnSessImplStarted,
            pInvTask, pReqCtx );
        if( ERROR( ret ) )
            break;

        TaskletPtr pStartIfTask;
        ret = DEFER_IFCALLEX2_NOSCHED2(
            0, pStartIfTask, ObjPtr( pIf ),
            &CRpcServices::StartEx,
            nullptr );

        if( ERROR( ret ) )
            break;

        CIfRetryTask* pStart = pStartIfTask;
        pStart->SetClientNotify( pRespCb );
        pTaskGrp->AppendTask( pStartIfTask );
        // force to call the callback all the time
        pStart->MarkPending();

        // just to check if the token exchange is
        // done. if failed, the invoke task will
        // be completed here.
        gint32 (*pLoginCb)( IEventSink*,
            CRpcReqForwarderAuth*,
            CRpcServices*,
            IEventSink*, IConfigDb* ) =
        ([](IEventSink* pCb,
            CRpcReqForwarderAuth* pFwdr,
            CRpcServices* pAuthImpl,
            IEventSink* pInvTask,
            IConfigDb* pReqCtx )->gint32
        {
            if( pInvTask == nullptr ||
                pReqCtx == nullptr )
                return -EINVAL;

            gint32 ret = 0;
            CParamList oParams;
            CCfgOpener oReqCtx( pReqCtx );
            do{
                pFwdr->SetStartCtx( nullptr );
                CCfgOpenerObj oReq( pCb );
                IConfigDb* pResp = nullptr;
                ret = oReq.GetPointer(
                    propRespPtr, pResp );
                if( ERROR( ret ) )
                    break;

                CCfgOpener oResp( pResp );
                gint32 iRet = 0;
                ret = oResp.GetIntProp(
                    propReturnValue,
                    ( guint32& ) iRet );

                if( ERROR( ret ) )
                    break;

                if( ERROR( iRet ) )
                {
                    ret = iRet;
                    break;
                }

            }while( 0 );

            if( ERROR( ret ) )
            do{
                // on error, actively disconnect to
                // avoid reusing this connection by
                // other login activities, before this
                // login returns.
                gint32 iRet = 0;
                IConfigDb* pConn;
                iRet = oReqCtx.GetPointer(
                    propConnParams, pConn );
                if( ERROR( iRet ) )
                    break;

                CRpcRouterReqFwdr* pRouter =
                    static_cast< CRpcRouterReqFwdr* >
                        ( pFwdr->GetParent() );
                InterfPtr pProxy;
                iRet = pRouter->GetBridgeProxy(
                    pConn, pProxy );
                if( ERROR( iRet ) )
                    break;
                CRpcServices* pSvc = pProxy;
                PortPtr pPort = pSvc->GetPort();
                Variant oVar;
                iRet = pPort->GetProperty(
                    propPdoId, oVar );
                if( ERROR( iRet ) )
                    break;
                guint32 dwPortId = oVar;
                pPort = pAuthImpl->GetPort();
                iRet = pPort->GetProperty(
                    propSrcDBusName, oVar );
                if( ERROR( iRet ) )
                    break;
                stdstr strSender = oVar;
                iRet = pPort->GetProperty(
                    propSrcUniqName, oVar );
                if( ERROR( iRet ) )
                    break;
                stdstr strUniq = oVar;
                TaskletPtr pDummy;
                pDummy.NewObj( clsid(CIfDummyTask)  );
                iRet = pFwdr->StopBridgeProxy( pDummy,
                    dwPortId, strUniq, strSender );
                if( ERROR( iRet ) )
                {
                    DebugPrint( iRet,
                        "Active disconnection failed" );
                    break;
                }

            }while( 0 );

            if( ERROR( ret ) )
            {
                oParams[ propReturnValue ] = ret;
                pFwdr->OnServiceComplete(
                    oParams.GetCfg(), pInvTask );
            }

            return ret;
        });

        TaskletPtr pCheckResp;
        ret = NEW_COMPLETE_FUNCALL( 0, pCheckResp,
            GetIoMgr(), pLoginCb, nullptr,
            this, pIf, pInvTask, pReqCtx );

        TaskletPtr pLoginTask;
        ret = CAuthentProxy::BuildLoginTask(
            pIf, pCheckResp, pLoginTask );

        if( ERROR( ret ) )
            break;

        pTaskGrp->AppendTask( pLoginTask );
        // force to call the callback all the time
        pLoginTask->MarkPending();

        TaskletPtr pLoginComp;
        ret = DEFER_IFCALLEX2_NOSCHED2(
            1, pLoginComp, ObjPtr( this ),
            &CRpcReqForwarderAuth::OnSessImplLoginCompleteSafe,
            pInvTask, nullptr, pReqCtx );
        if( ERROR( ret ) )
            break;

        pTaskGrp->AppendTask( pLoginComp );

        TaskletPtr pStopIf;
        ret = DEFER_IFCALLEX2_NOSCHED2(
            0, pStopIf, ObjPtr( pIf ),
            &CRpcServices::Shutdown,
            nullptr );

        if( ERROR( ret ) )    
            break;

        CIfTransactGroup* pTractGrp = pTaskGrp; 
        pTractGrp->AddRollback( pStopIf );
        pTask = ObjPtr( pTaskGrp );
        SetStartCtx( pReqCtx );

    }while( 0 );

    if( ERROR( ret ) )
    {
        if( !pRespCb.IsEmpty() )
            ( *pRespCb )( eventCancelTask );

        if( !pRespCb1.IsEmpty() )
            ( *pRespCb1 )( eventCancelTask );

        if( !pTaskGrp.IsEmpty() )
            ( *pTaskGrp )( eventCancelTask );
    }

    return ret;
}

gint32 CRpcReqForwarderAuth::CreateBridgeProxyAuth(
    IEventSink* pSeqTask,
    IConfigDb* pCfg,
    IEventSink* pInvTask )
{
    if( pSeqTask == nullptr ||
        pCfg == nullptr ||
        pInvTask == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        TaskletPtr pApTask;
        ret = BuildStartAuthProxyTask(
            pInvTask, pCfg, pApTask );
        if( ERROR( ret ) )
            break;

        CIfRetryTask* pTask = pApTask;
        pTask->SetClientNotify( pSeqTask );

        CIoManager* pMgr = GetIoMgr();
        ret = pMgr->RescheduleTask( pApTask );
        if( SUCCEEDED( ret ) )
            ret = STATUS_PENDING;

    }while( 0 );

    return ret;
}


gint32 CRpcRouterReqFwdrAuth::IsEqualConn(
    const IConfigDb* pConn1,
    const IConfigDb* pConn2 )
{
    gint32 ret = super::IsEqualConn(
        pConn1, pConn2 );
    if( ERROR( ret ) )
        return ret;

    do{
        CCfgOpener oConn1( pConn1 );
        IConfigDb* pAuth1 = nullptr;
        ret = oConn1.GetPointer(
            propAuthInfo, pAuth1 );
        if( ERROR( ret ) )
            break;

        IConfigDb* pAuth2 = nullptr;
        CCfgOpener oConn2( pConn2 );
        ret = oConn2.GetPointer(
            propAuthInfo, pAuth2 );
        if( ERROR( ret ) )
            break;

        CCfgOpener oAuth1( pAuth1 );
        CCfgOpener oAuth2( pAuth2 );
        std::string strMech;
        ret = oAuth1.GetStrProp(
            propAuthMech, strMech );
        if( ERROR( ret ) )
            break;

        ret = oAuth2.IsEqual(
            propAuthMech, strMech );
        if( ERROR( ret ) )
        {
            ret = ERROR_FALSE;
            break;
        }

        if( strMech == "krb5" )
        {
            ret = oAuth1.IsEqualProp(
                propUserName, pAuth2 );
            if( ERROR( ret ) )
                break;

            ret = oAuth1.IsEqualProp(
                propServiceName, pAuth2 );
            if( ERROR( ret ) )
                break;

            ret = oAuth1.IsEqualProp(
                propRealm, pAuth2 );
            if( ERROR( ret ) )
                break;
                
            bool bSignMsg1 = false;
            bool bSignMsg2 = false;

            oAuth1.GetBoolProp(
                propSignMsg, bSignMsg1 );

            oAuth2.GetBoolProp(
                propSignMsg, bSignMsg2 );

            if( bSignMsg1 != bSignMsg2 )
            {
                ret = ERROR_FALSE;
                break;
            }
        }
        else if( strMech == "OAuth2" )
        {
            ret = oAuth1.IsEqualProp(
                propAuthUrl, pAuth2 );
            if( ERROR( ret ) )
                break;
        }

        ret = 0;

    }while( 0 );

    if( ERROR( ret ) )
        ret = ERROR_FALSE;

    return ret;
}

gint32 CRpcRouterReqFwdrAuth::GetBridgeProxy(
    const IConfigDb* pConnParams,
    InterfPtr& pIf )
{
    if( pConnParams == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CCfgOpener oConn( pConnParams );
        IConfigDb* pAuth = nullptr;
        ret = oConn.GetPointer(
            propAuthInfo, pAuth );
        if( ERROR( ret ) )
            break;

        ret = super::GetBridgeProxy(
            pConnParams, pIf );

    }while( 0 );

    return ret;
}

gint32 CRpcRouterReqFwdrAuth::StopProxyNoRef(
    guint32 dwPortId )
{
    gint32 ret = 0;

    do{
        RegObjPtr pReg;
        CStdRMutex oRouterLock( GetLock() );
        for( auto elem : m_mapRefCount )
        {
            if( elem.first->GetPortId() == dwPortId )
            {
                pReg = elem.first;
                break;
            }
        }
        if( pReg.IsEmpty() )
        { 
            ret = -EFAULT;
            break;
        }

        // notify the authprxy to stop
        InterfPtr pIf;
        ret = super::GetBridgeProxy( dwPortId, pIf );
        if( ERROR( ret ) )
            break;

        CAuthentProxy* pspp = pIf;
        oRouterLock.Unlock();

        pspp->StopSessImpl( nullptr );

    }while( 0 );

    return ret;
}

gint32 CRpcRouterReqFwdrAuth::DecRefCount(
        guint32 dwPortId,
        const std::string& strSrcUniqName,
        const std::string& strSrcDBusName )
{
    gint32 ret = super::DecRefCount( dwPortId,
        strSrcUniqName, strSrcDBusName );

    if( ERROR( ret ) )
        return ret;

    if( ret == 1 )
    {
        std::vector< stdstr > vecNames;
        ClearRefCountByPortId( dwPortId, vecNames );
        ret = 0;
        // the last is held by the CK5AuthProxy
    }

    return ret;
}

CRpcRouterReqFwdrAuth::CRpcRouterReqFwdrAuth(
    const IConfigDb* pCfg ) :
    CAggInterfaceServer( pCfg ), super( pCfg ),
    CRpcRouterAuthShared( this )
{
    bool bKProxy = false;
    CIoManager* pMgr = GetIoMgr();
    gint32 ret = pMgr->GetCmdLineOpt(
        propKProxy, bKProxy );
    if( SUCCEEDED( ret ) )
        m_bKProxy = bKProxy;
}

gint32 CRpcRouterReqFwdrAuth::CheckReqToFwrd(
    IConfigDb* pTransCtx,
    DMsgPtr& pMsg,
    MatchPtr& pMatchHit )
{
    if( pTransCtx == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        ret = super::CheckReqToFwrd(
            pTransCtx, pMsg, pMatchHit );
        if( SUCCEEDED( ret ) )
            break;

        std::string strVal;
        CMessageMatch* pAuthMatch =
            ( CMessageMatch* )m_pAuthMatch;
        pAuthMatch->GetIfName( strVal );
        stdstr strIf = std::move(
            pMsg.GetInterface() );
        if( strIf != strVal )
        {
            ret = ERROR_FALSE;
            break;
        }

        CCfgOpener oTransCtx( pTransCtx );
        std::string strPath;
        ret = oTransCtx.GetStrProp(
            propRouterPath, strPath );
        if( ERROR( ret ) )
            break;

        if( strPath != "/" )
        {
            ret = ERROR_FALSE;
            break;
        }

        guint32 dwPortId = 0;
        ret = oTransCtx.GetIntProp(
            propConnHandle, dwPortId );
        if( ERROR( ret ) )
            break;

        InterfPtr pIf;
        ret = GetBridgeProxy( dwPortId, pIf );
        if( ERROR( ret ) )
            break;

        if( pMsg.GetType() !=
            DBUS_MESSAGE_TYPE_METHOD_CALL )
        {
            ret = ERROR_FALSE;
            break;
        }

        CCfgOpenerObj oIfCfg( ( CObjBase*) pIf );
        ret = oIfCfg.GetStrProp(
            propObjPath, strVal );
        if( ERROR( ret ) )
            break;

        size_t pos = strVal.find_last_of( '/' );
        if( pos == std::string::npos )
        {
            ret = ERROR_FALSE;
            break;
        }

        std::string strDir =
            strVal.substr( 0, pos + 1 );

        strDir += OBJNAME_ROUTER_BRIDGE_AUTH;
        stdstr strInPath = DBUS_OBJ_PATH(
            MODNAME_RPCROUTER,
            OBJNAME_ROUTER_BRIDGE_AUTH );

        if( m_bKProxy &&
            pMsg.GetPath() == strInPath )
        {
            pMsg.SetPath( strDir );
            stdstr strDest = strDir;
            std::replace( strDest.begin(),
                strDest.end(), '/', '.' );

            if( strDest[ 0 ] == '.' )
                strDest.erase( strDest.begin() );
            pMsg.SetDestination( strDest );
        }
        else
        {
            if( strDir != pMsg.GetPath() )
            {
                ret = ERROR_FALSE;
                DebugPrintEx( logErr, ret,
                    "Error check message" );
                break;
            }

            std::replace( strDir.begin(),
                strDir.end(), '/', '.' );

            if( strDir[ 0 ] == '.' )
                strDir.erase( strDir.begin() );

            if( strDir != pMsg.GetDestination() )
            {
                ret = ERROR_FALSE;
                break;
            }
        }

        ret = pMatchHit.NewObj(
            clsid( CRouterRemoteMatch ) );
        if( ERROR( ret ) )
            break;

        CRouterRemoteMatch* prrm = pMatchHit;
        prrm->CopyMatch( m_pAuthMatch );

        CCfgOpenerObj oMatch( prrm );

        oMatch.SetStrProp(
            propObjPath, pMsg.GetPath() );

        oMatch.SetStrProp( propDestDBusName,
            pMsg.GetDestination() );
  
        oMatch.SetIntProp(
            propIid, iid( IAuthenticate ) );

        oMatch.SetIntProp(
            propPortId, 0xffffffff );

    }while( 0 );

    return ret;
}

gint32 CRpcReqForwarderAuth::ClearRefCountByPortId(
    guint32 dwPortId,
    std::vector< std::string >& vecUniqNames )
{
    gint32 ret = super::ClearRefCountByPortId(
        dwPortId, vecUniqNames );

    if( ret < 2 )
        return ret;

    PortPtr pPort = GetPort();
    if( pPort.IsEmpty() )
        return -EINVAL;

    CCfgOpenerObj oPortCfg(
        ( CObjBase* )pPort );

    std::string strUniqName;
    ret = oPortCfg.GetStrProp(
        propSrcUniqName, strUniqName );

    if( ERROR( ret ) )
        return ret;

    std::vector< std::string >::iterator itr =
        vecUniqNames.begin();

    while( itr != vecUniqNames.end() )
    {
        // remove the local one, to avoid authprxy
        // receiving the eventRmtSvrOnline event.
        if( *itr == strUniqName )
        {
            vecUniqNames.erase( itr );
            break;
        }
        ++itr;
    }

    return vecUniqNames.size();
}

gint32 CRpcReqForwarderAuth::OnPostStart(
    IEventSink* pCallback )
{
    return super::OnPostStart( pCallback );
}

gint32 CRpcReqForwarderAuth::OnPreStop(
    IEventSink* pCallback )
{
    return super::OnPreStop( pCallback );
}

CRpcReqForwarderAuth::~CRpcReqForwarderAuth()
{
    CAuthentProxy::DestroyEnvRouter( GetIoMgr() );
}

gint32 CAuthentServer::InitUserFuncs()
{
    BEGIN_IFHANDLER_MAP( IAuthenticate );
    
    ADD_SERVICE_HANDLER_EX( 1,
       CAuthentServer::Login,
       AUTH_METHOD_LOGIN );

    ADD_SERVICE_HANDLER_EX( 1,
       CAuthentServer::MechSpecReq,
       AUTH_METHOD_MECHSPECREQ );

    END_IFHANDLER_MAP;
    return 0;
}

gint32 CAuthentServer::SetAuthImpl(
    ObjPtr pAuthImpl )
{
    guint32 ret = 0;
    do{
        CStdRMutex oLock( this->GetLock() );
        m_pAuthImpl = pAuthImpl;
    }while( 0 );
    return ret;
}

static gint32 RetryStartAuthImpl(
    CAuthentServer* pSvr,
    InterfPtr& pOldIf )
{
    gint32 ret = 0;

    gint32 (*sn)( CAuthentServer*, InterfPtr& ) =
    ([]( CAuthentServer* pSvr, InterfPtr& pOldIf  )->gint32
    {
        CIoManager* pMgr = pSvr->GetIoMgr();
        CStdRMutex oLock( pSvr->GetLock() );
        if( pSvr->GetState() != stateConnected ||
            pMgr->IsStopping() )
        {
            TaskGrpPtr pg =
                pSvr->GetPendingLogins();
            if( !pg.IsEmpty() )
                ( *pg )( eventCancelTask );
            return 0;
        }
        oLock.Unlock();
        pSvr->StartNewAuthImpl( pOldIf );
        return 0;
    });
    TaskletPtr pTimer;
    ret = ADD_TIMER_FUNC( pTimer, 10,
        pSvr->GetIoMgr(), sn, pSvr, pOldIf );
    return ret;
}

gint32 CAuthentServer::StartNewAuthImpl(
    InterfPtr& pOldIf )
{
    gint32 ret = 0;
    do{
        int (*func)( IEventSink*,
            CAuthentServer* ) =
        ([]( IEventSink* pCb,
            CAuthentServer* pSvr )->gint32
        {
            gint32 ret = 0;
            gint32 iRet = 0;
            do{
                CCfgOpenerObj oCfg( pCb );
                IConfigDb* pResp;
                ret = oCfg.GetPointer(
                    propRespPtr, pResp );
                if( ERROR( ret ) )
                    break;
                CCfgOpener oResp( pResp );
                ret = oResp.GetIntProp(
                    propReturnValue,
                    ( guint32& )iRet );
                if( ERROR( ret ) )
                    break;
                if( ERROR( iRet ) )
                {
                    break;
                }
            }while( 0 );
            do{
                CStdRMutex oLock( pSvr->GetLock() );
                if( pSvr->GetState() != stateConnected &&
                    SUCCEEDED( ret ) )
                    ret = ERROR_STATE;
                TaskGrpPtr pg =
                    pSvr->GetPendingLogins();

                bool bHasLogin = ( !pg.IsEmpty() &&
                    !pg->IsStopped( pg->GetTaskState() ) );

                ObjPtr pImpl = pSvr->GetAuthImplNoLock();
                oLock.Unlock();
                if( ERROR( ret ) && bHasLogin )
                {
                    ( *pg )( eventCancelTask );
                    break;
                }
                else if( ERROR( iRet ) )
                {
                    // retry is already scheduled.
                    break;
                }
                else if( bHasLogin )
                {
                    IEventSink* pTask = pg;
                    CRpcServices* pSvc = pImpl;
                    if( pSvc )
                        ret = pSvc->RunManagedTask2(
                            pTask );
                    else
                        ret = -EFAULT;
                    if( ERROR( ret ) )
                        ( *pg )( eventCancelTask );
                }
            }while( 0 );
            return 0;
        });

        TaskletPtr pCompTask;
        ret = NEW_COMPLETE_FUNCALL( 0, 
            pCompTask, this->GetIoMgr(),
            func, nullptr, this ); 
        if( ERROR( ret ) )
            break;
        stdstr strMech = GetAuthMech();
        if( strMech == "OAuth2" )
            ret = StartOA2Checker( pCompTask );
        else if( strMech == "SimpAuth" )
            ret = StartSimpAuthCli(
                pCompTask, pOldIf );
        if( ERROR( ret ) )
            ( *pCompTask )( eventCancelTask );
    }while( 0 );
    return ret;
}

gint32 CAuthentServer::GetAuthImpl(
    ObjPtr& pAuthImpl )
{
    guint32 ret = 0;
    do{
        InterfPtr pOldIf;
        stdstr strMech = GetAuthMech();
        if( strMech == "krb5" )
        {
            pAuthImpl = m_pAuthImpl;
            break;
        }
        CStdRMutex oLock( this->GetLock() );
        if( GetState() != stateConnected )
        {
            ret = ERROR_STATE;
            break;
        }
        if( m_pAuthImpl.IsEmpty() )
        {
            ret = -EAGAIN;
            break;
        }
        CRpcServices* pSvc = m_pAuthImpl;
        if( pSvc->GetState() != stateStopped )
        {
            pAuthImpl = m_pAuthImpl;
            break;
        }

        pOldIf = m_pAuthImpl;
        m_pAuthImpl.Clear();
        oLock.Unlock();

        ret = StartNewAuthImpl( pOldIf );

        // notify to queue the login task
        if( ret == STATUS_PENDING )
            ret = -EAGAIN;
    }while( 0 );
    return ret;
}

gint32 CAuthentServer::QueueLoginTask(
    TaskletPtr& pTask )
{
    guint32 ret = 0;
    do{
        CStdRMutex oLock( this->GetLock() );
        if( GetState() != stateConnected )
        {
            ret = ERROR_STATE;
            break;
        }
        if( !m_pAuthImpl.IsEmpty() )
        {
            CRpcServices* pImpl = m_pAuthImpl;
            ret = pImpl->RunManagedTask2( pTask );
            if( SUCCEEDED( ret ) )
                ret = STATUS_PENDING;
            break;
        }
        CIfTaskGroup* pg = m_pLoginTasks;
        if( pg == nullptr ||
            pg->IsStopped( pg->GetTaskState() ) )
        {
            CCfgOpener oParams;
            oParams.SetPointer(
                propIfPtr, this );
            ret = m_pLoginTasks.NewObj(
                clsid( CIfParallelTaskGrp ),
                oParams.GetCfg() );
            if( ERROR( ret ) )
                break;
            m_pLoginTasks->AppendTask( pTask );
            ret = STATUS_PENDING;
            break;
        }
        else
        {
            if( m_pLoginTasks->GetTaskCount() > 20 )
            {
                ret = -ENOMEM;
                break;
            }
            ret = m_pLoginTasks->AppendTask( pTask );
            if( ERROR( ret ) )
                break;
            ret = STATUS_PENDING;
        }
        
    }while( 0 );
    return ret;
}

gint32 CAuthentServer::Login(
    IEventSink* pCallback,
    IConfigDb* pInfo, /*[ in ]*/
    CfgPtr& pResp )
{
    gint32 ret = 0;
    do{
        ObjPtr pObj;
        ret = this->GetAuthImpl( pObj );
        if( ret == -EAGAIN )
        {
            stdstr strMech = GetAuthMech();
            gint32 dwMech = 0;
            if( strMech == "OAuth2" )
                dwMech = 1;
            else if( strMech == "SimpAuth" )
                dwMech = 2;
            if( dwMech )
            {
                // restart the checker, OAuth2 checker
                // may drop offline if inactivity lasts
                // over two minutes.
                if( dwMech == 1 )
                    DebugPrintEx( logErr, 0,
                        "Reconnect the oa2checker" );
                else
                    DebugPrintEx( logErr, 0,
                        "Reconnect the saclient" );

                gint32 (*pLogin)(
                    CAuthentServer*,
                    IEventSink*, IConfigDb* ) =
                ([]( CAuthentServer* pSvr,
                    IEventSink* pCb,
                    IConfigDb* pInfo )->gint32
                {
                    gint32 ret = 0;
                    CCfgOpener oResp;
                    CParamList oParams;
                    ObjPtr pObj;
                    ret = pSvr->GetAuthImpl( pObj );
                    if( ERROR( ret ) )
                        return ret;
#ifdef OA2
                    COA2proxy_CliImpl* pAuth1 = pObj;
                    CSimpleAuthCliWrapper* pAuth2 = pObj;
                    IAuthenticateServer* pAuth; 
                    if( pAuth1 != nullptr )
                        pAuth = pAuth1;
                    else
                        pAuth = pAuth2;
#else
                    CSimpleAuthCliWrapper* pAuth2 = pObj;
                    IAuthenticateServer* pAuth = pAuth2;
#endif
                    ret = pAuth->Login(
                        pCb, pInfo, oResp.GetCfg() );
                    if( ret == STATUS_PENDING )
                        return 0;
                    oParams.SetIntProp(
                        propReturnValue, ret );
                    if( SUCCEEDED( ret ) )
                    {
                        ObjPtr pObj = oResp.GetCfg();
                        oParams.Push( pObj );
                    }
                    pSvr->OnServiceComplete(
                        oParams.GetCfg(), pCb );
                    return 0;
                });
                TaskletPtr pLoginTask;
                ret = NEW_FUNCCALL_TASKEX(
                    pLoginTask, this->GetIoMgr(),
                    pLogin, this, pCallback, pInfo );
                if( ERROR( ret ) )
                    break;
                ret = QueueLoginTask( pLoginTask );
                if( ERROR( ret ) )
                {
                    ( *pLoginTask )( eventCancelTask );
                    ret = -EACCES;
                }
                break;
            }
            else
            {
                ret = ERROR_STATE;
                break;
            }
        }

        CRpcServices* pSvc = pObj;
        IAuthenticate* pAuth =
        dynamic_cast< IAuthenticate* >
                ( pSvc );

        if( unlikely( pAuth == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }
        ret = pAuth->Login(
            pCallback, pInfo, pResp );

    }while( 0 );

    return ret;
}

gint32 CAuthentServer::IsNoEnc(
    const std::string& strSess )
{
    gint32 ret = 0;
    do{
        ObjPtr pIf;
        ret = GetAuthImpl( pIf );
        if( ERROR( ret ) )
            break;

        CRpcServices* pObj = pIf;
        if( pObj == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        IAuthenticateServer* pAuth =
        dynamic_cast< IAuthenticateServer* >
                ( pObj );

        if( unlikely( pAuth == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }
        ret = pAuth->IsNoEnc( strSess );

    }while( 0 );

    return ret;
}

gint32 CAuthentServer::GetSess(
    guint32 dwPortId,
    std::string& strSess )
{
    gint32 ret = 0;
    do{
        ObjPtr pIf;
        ret = GetAuthImpl( pIf );
        if( ERROR( ret ) )
            break;

        CRpcServices* pObj = pIf;
        if( pObj == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        IAuthenticateServer* pAuth =
        dynamic_cast< IAuthenticateServer* >
                ( pObj );

        if( unlikely( pAuth == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }
        ret = pAuth->GetSess(
            dwPortId, strSess );

    }while( 0 );

    return ret;
}

gint32 CAuthentServer::MechSpecReq(
    IEventSink* pCallback,
    IConfigDb* pInfo,/*[ in ]*/
    CfgPtr& pResp )/*[ out ]*/
{
    gint32 ret = 0;
    do{
        CRpcServices* pObj = m_pAuthImpl;
        if( pObj == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        IAuthenticate* pAuth =
        dynamic_cast< IAuthenticate* >
                ( pObj );

        if( unlikely( pAuth == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }
        ret = pAuth->MechSpecReq(
            pCallback, pInfo, pResp );

    }while( 0 );

    return ret;
}

gint32 CAuthentServer::WrapMessage(
    const std::string& strSess,
    BufPtr& pInMsg,
    BufPtr& pOutMsg )
{
    gint32 ret = 0;
    do{
        CRpcServices* pObj = m_pAuthImpl;
        if( pObj == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        IAuthenticateServer* pAuth =
        dynamic_cast< IAuthenticateServer* >
                ( pObj );

        if( unlikely( pAuth == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }
        ret = pAuth->WrapMessage(
            strSess, pInMsg, pOutMsg );

    }while( 0 );

    return ret;
}
    
gint32 CAuthentServer::UnwrapMessage(
    const std::string& strSess,
    BufPtr& pInMsg,
    BufPtr& pOutMsg )
{
    gint32 ret = 0;
    do{
        CRpcServices* pObj = m_pAuthImpl;
        if( pObj == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        IAuthenticateServer* pAuth =
        dynamic_cast< IAuthenticateServer* >
                ( pObj );

        if( unlikely( pAuth == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }
        ret = pAuth->UnwrapMessage(
            strSess, pInMsg, pOutMsg );

    }while( 0 );

    return ret;
}

gint32 CAuthentServer::GetMicMsg(
    const std::string& strSess,
    BufPtr& pInMsg,
    BufPtr& pOutMic )
{
    gint32 ret = 0;
    do{
        CRpcServices* pObj = m_pAuthImpl;
        if( pObj == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        IAuthenticateServer* pAuth =
        dynamic_cast< IAuthenticateServer* >
                ( pObj );

        if( unlikely( pAuth == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }
        ret = pAuth->GetMicMsg(
            strSess, pInMsg, pOutMic );

    }while( 0 );

    return ret;
}   

gint32 CAuthentServer::VerifyMicMsg(
    const std::string& strSess,
    BufPtr& pInMsg,
    BufPtr& pInMic )
{
    gint32 ret = 0;
    do{
        CRpcServices* pObj = m_pAuthImpl;
        if( pObj == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        IAuthenticateServer* pAuth =
        dynamic_cast< IAuthenticateServer* >
                ( pObj );

        if( unlikely( pAuth == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }
        ret = pAuth->VerifyMicMsg(
            strSess, pInMsg, pInMic );

    }while( 0 );

    return ret;
}

gint32 CAuthentServer::InquireSess(
    const std::string& strSess,
    CfgPtr& pInfo )
{
    gint32 ret = 0;
    do{
        ObjPtr pIf;
        ret = GetAuthImpl( pIf );
        if( ERROR( ret ) )
            break;

        CRpcServices* pObj = pIf;
        if( pObj == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        IAuthenticateServer* pAuth =
        dynamic_cast< IAuthenticateServer* >
                ( pObj );

        if( unlikely( pAuth == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }
        ret = pAuth->InquireSess(
            strSess, pInfo );

    }while( 0 );

    return ret;
}

gint32 CAuthentServer::OnStartAuthImplComplete(
    IEventSink* pCallback,
    IEventSink* pIoReq,
    IConfigDb* pReqCtx )
{
    if( pCallback == nullptr ||
        pIoReq == nullptr ||
        pReqCtx == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    CParamList oParams;
    CCfgOpener oReqCtx( pReqCtx );

    do{
        CCfgOpenerObj oReq( pIoReq );
        IConfigDb* pResp = nullptr;
        ret = oReq.GetPointer(
            propRespPtr, pResp );
        if( ERROR( ret ) )
            break;

        CCfgOpener oResp( pResp );
        gint32 iRet = 0;
        ret = oResp.GetIntProp(
            propReturnValue,
            ( guint32& ) iRet );

        if( ERROR( ret ) )
            break;

        if( ERROR( iRet ) )
        {
            ret = iRet;
            break;
        }

        CRpcServices* pSvc;
        ret = oReqCtx.GetPointer(
            propIfPtr, pSvc );
        if( ERROR( ret ) )
            break;

        SetAuthImpl( ObjPtr( pSvc ) );

    }while( 0 );

    pCallback->OnEvent( eventTaskComp,
        ret, 0, ( LONGWORD* )this );

    return ret;
}

gint32 CAuthentServer::StartAuthImpl(
    IEventSink* pCallback )
{
    if( pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        //FIXME: just mechanism is krb5 only
        CCfgOpener oCfg;
        CIoManager* pMgr = GetIoMgr();
        oCfg.SetPointer( propIoMgr, pMgr );

        ret = CRpcServices::LoadObjDesc(
            DESC_FILE, "K5AuthServer",
            true, oCfg.GetCfg() );

        if( ERROR( ret ) )
            break;

        CRpcRouter* pRouter = ObjPtr( this );

        oCfg.SetPointer(
            propRouterPtr, pRouter );

        ret = oCfg.CopyProp(
            propAuthInfo, this );
        if( ERROR( ret ) )
            break;

        InterfPtr pIf;
        ret = pIf.NewObj(
            clsid( CK5AuthServer ),
            oCfg.GetCfg() );

        if( ERROR( ret ) )
            break;

        CCfgOpener oReqCtx;
        oReqCtx[ propIfPtr ] = ObjPtr( pIf );
        TaskletPtr pRespCb;
        ret = NEW_PROXY_RESP_HANDLER2(
            pRespCb, ObjPtr( this ),
            &CAuthentServer::OnStartAuthImplComplete,
            pCallback,
            ( IConfigDb* )oReqCtx.GetCfg() );
        if( ERROR( ret ) )
            break;

        m_pAuthImpl = pIf;
        ret = pIf->StartEx( pRespCb );
        if( ret != STATUS_PENDING )
            ( *pRespCb )( eventCancelTask );

    }while( 0 );

    return ret;
}

gint32 CAuthentServer::OnStartOA2CheckerComplete(
    IEventSink* pCallback,
    IEventSink* pIoReq,
    IConfigDb* pReqCtx )
{
    if( pCallback == nullptr ||
        pIoReq == nullptr ||
        pReqCtx == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    CParamList oParams;
    CCfgOpener oReqCtx( pReqCtx );

    do{
        CCfgOpenerObj oReq( pIoReq );
        IConfigDb* pResp = nullptr;
        ret = oReq.GetPointer(
            propRespPtr, pResp );
        if( ERROR( ret ) )
            break;

        CCfgOpener oResp( pResp );
        gint32 iRet = 0;
        ret = oResp.GetIntProp(
            propReturnValue,
            ( guint32& ) iRet );

        if( ERROR( ret ) )
            break;

        if( ERROR( iRet ) )
        {
            ret = iRet;
            break;
        }

        CRpcServices* pSvc;
        ret = oReqCtx.GetPointer(
            propIfPtr, pSvc );
        if( ERROR( ret ) )
            break;

        SetAuthImpl( ObjPtr( pSvc ) );

    }while( 0 );
    if( ERROR( ret ) )
    {
        LOGERR( this->GetIoMgr(), ret,
        "Error, failed to connect to oa2checker, "
        "login stop working" );

        DebugPrintEx( logErr, ret,
            "Error, failed to connect to oa2checker, " 
            "login stop working" );
    }

    pCallback->OnEvent( eventTaskComp,
        ret, 0, ( LONGWORD* )pIoReq );

    return ret;
}

gint32 CAuthentServer::StartOA2Checker(
    IEventSink* pCallback )
{
    if( pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;
#ifdef OA2
    do{
        CCfgOpener oCfg;
        CIoManager* pMgr = GetIoMgr();
        oCfg.SetPointer( propIoMgr, pMgr );

        ret = CRpcServices::LoadObjDesc(
            "./oa2checkdesc.json", "OA2proxy",
            false, oCfg.GetCfg() );

        if( ERROR( ret ) )
            break;

        CRpcRouter* pRouter = ObjPtr( this );

        oCfg.SetPointer(
            propRouterPtr, pRouter );

        ret = oCfg.CopyProp(
            propAuthInfo, this );
        if( ERROR( ret ) )
            break;

        InterfPtr pIf;
        ret = pIf.NewObj(
            clsid( COA2proxy_CliImpl ),
            oCfg.GetCfg() );

        if( ERROR( ret ) )
            break;

        CCfgOpener oReqCtx;
        oReqCtx[ propIfPtr ] = ObjPtr( pIf );
        TaskletPtr pRespCb;
        ret = NEW_PROXY_RESP_HANDLER2(
            pRespCb, ObjPtr( this ),
            &CAuthentServer::OnStartOA2CheckerComplete,
            pCallback,
            ( IConfigDb* )oReqCtx.GetCfg() );
        if( ERROR( ret ) )
            break;

        ret = pIf->StartEx( pRespCb );
        if( ret != STATUS_PENDING )
            ( *pRespCb )( eventCancelTask );

    }while( 0 );
#endif
    return ret;
}

gint32 CAuthentServer::OnStartSimpAuthCliComplete(
    IEventSink* pCallback,
    IEventSink* pIoReq,
    IConfigDb* pReqCtx )
{
    if( pCallback == nullptr ||
        pIoReq == nullptr ||
        pReqCtx == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    CParamList oParams;
    CCfgOpener oReqCtx( pReqCtx );
    InterfPtr pIf;

    do{
        GetSimpleAuthcli( pIf );
        CCfgOpenerObj oReq( pIoReq );
        IConfigDb* pResp = nullptr;
        ret = oReq.GetPointer(
            propRespPtr, pResp );
        if( ERROR( ret ) )
            break;

        CCfgOpener oResp( pResp );
        gint32 iRet = 0;
        ret = oResp.GetIntProp(
            propReturnValue,
            ( guint32& ) iRet );

        if( ERROR( ret ) )
            break;

        if( ERROR( iRet ) )
        {
            ret = iRet;
            break;
        }

        if( pIf.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }
        SetAuthImpl( ObjPtr( pIf ) );

    }while( 0 );
    if( ERROR( ret ) )
    {
        LOGERR( this->GetIoMgr(), ret,
        "Warning, failed to connect to SimpleAuthServer, "
        "retry is scheduled..." );

        DebugPrintEx( logErr, ret,
            "Warning, failed to connect to SimpleAuthServer, " 
            "retry is scheduled..." );
        RetryStartAuthImpl( this, pIf );
    }

    pCallback->OnEvent( eventTaskComp,
        ret, 0, ( LONGWORD* )pIoReq );

    return ret;
}

gint32 CAuthentServer::StartSimpAuthCli(
    IEventSink* pCallback,
    InterfPtr& pOldIf )
{
    if( pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;

    do{
        CCfgOpener oCfg;
        CIoManager* pMgr = GetIoMgr();
        CRpcRouter* pRouter = ObjPtr( this );

        oCfg.SetPointer(
            propRouterPtr, pRouter );

        ret = oCfg.CopyProp(
            propAuthInfo, this );
        if( ERROR( ret ) )
            break;

        CCfgOpener oReqCtx;
        TaskletPtr pRespCb;
        ret = NEW_PROXY_RESP_HANDLER2(
            pRespCb, ObjPtr( this ),
            &CAuthentServer::OnStartSimpAuthCliComplete,
            pCallback,
            ( IConfigDb* )oReqCtx.GetCfg() );
        if( ERROR( ret ) )
            break;
        ret = CSimpleAuthCliWrapper::Create(
            pMgr, pRespCb, oCfg.GetCfg(), pOldIf );

        if( ret != STATUS_PENDING )
            ( *pRespCb )( eventCancelTask );

    }while( 0 );
    return ret;
}

gint32 CAuthentServer::OnPostStart(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        // replace the service name with the
        // service name from the command line
        // if specified.
        IConfigDb* pAuth = nullptr;
        CCfgOpenerObj oCfg( this );

        ret = oCfg.GetPointer(
            propAuthInfo, pAuth );
        if( ERROR( ret ) )
            break;

        CCfgOpener oAuth( pAuth );
        std::string strSvcName;
        CIoManager* pMgr = GetIoMgr();

        ret = pMgr->GetCmdLineOpt(
            propServiceName, strSvcName );
        if( SUCCEEDED( ret ) )
        {
            oAuth.SetStrProp(
                propServiceName, strSvcName );
        }
        stdstr strMech;
        ret = oAuth.GetStrProp(
            propAuthMech, strMech );\
        if( ERROR( ret ) )
            break;

        m_strMech = strMech;

        gint32 (*func)(
            IEventSink*, IEventSink* ) =
        ([]( IEventSink* pTask,
            IEventSink* pCb )->gint32
        {
            // ignore if failure occur and retry
            // restarting in the background
            pCb->OnEvent(
                eventTaskComp, 0, 0, nullptr );
            return 0;
        });

#ifdef KRB5
        if( strMech == "krb5" )
        {
            ret = StartAuthImpl( pCallback );
            break;
        }
#endif
#ifdef OA2
        if( strMech == "OAuth2" )
        {
            TaskletPtr pCb;
            ret = NEW_COMPLETE_FUNCALL( 0, 
                pCb, this->GetIoMgr(),
                func, nullptr, pCallback );
            if( ERROR( ret ) )
                break;

            ret = StartOA2Checker( pCb );
            break;
        }
#endif
        if( strMech == "SimpAuth" )
        {
            pMgr->TryLoadClassFactory(
                "./libappmancli.so" );

            TaskletPtr pCb;
            ret = NEW_COMPLETE_FUNCALL( 0, 
                pCb, this->GetIoMgr(),
                func, nullptr, pCallback );
            if( ERROR( ret ) )
                break;
            InterfPtr pEmpty;
            ret = StartSimpAuthCli( pCb, pEmpty );
            break;
        }
        ret = -ENOTSUP;

    }while( 0 );

    return ret;
}

gint32 CAuthentServer::OnPreStopComplete(
    IEventSink* pCallback,
    IEventSink* pIoReq,
    IConfigDb* pReqCtx )
{
    if( pCallback == nullptr ||
        pIoReq == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    CParamList oParams;
    UNREFERENCED( pReqCtx );

    do{
        if( !m_pAuthImpl.IsEmpty() )
            m_pAuthImpl.Clear();

        CCfgOpenerObj oReq( pIoReq );
        IConfigDb* pResp = nullptr;
        ret = oReq.GetPointer(
            propRespPtr, pResp );
        if( ERROR( ret ) )
            break;

        CCfgOpener oResp( pResp );
        gint32 iRet = 0;
        ret = oResp.GetIntProp(
            propReturnValue,
            ( guint32& ) iRet );

        if( ERROR( ret ) )
            break;

        if( ERROR( iRet ) )
        {
            ret = iRet;
            break;
        }

    }while( 0 );

    pCallback->OnEvent( eventTaskComp,
        0, 0, ( LONGWORD* )this );

    return ret;
}

gint32 CAuthentServer::OnPreStop(
    IEventSink* pCallback )
{
    gint32 ret = 0;

    TaskletPtr pRespCb;
    ret = NEW_PROXY_RESP_HANDLER2(
        pRespCb, ObjPtr( this ),
        &CAuthentServer::OnPreStopComplete,
        pCallback, nullptr );
    if( ERROR( ret ) )
        return ret;

    stdstr strMech = GetAuthMech();
    if( strMech == "krb5" )
    {
        if( m_pAuthImpl.IsEmpty() )
            return -EINVAL;

        CRpcServices* pSvc = m_pAuthImpl;
        CStdRMutex oLock( pSvc->GetLock() );
        if( pSvc->GetState() != stateStopped &&
            pSvc->GetState() != stateStopping )
        {
            oLock.Unlock();
            ret = pSvc->Shutdown( pRespCb );
            if( ret != STATUS_PENDING )
                ( *pRespCb )( eventCancelTask );
        }

        if( ret != STATUS_PENDING )
            m_pAuthImpl.Clear();
        return ret;
    }
    else if( strMech == "OAuth2" )
    do{
        ObjPtr pImpl;
        CStdRMutex oLock( GetLock() );
        pImpl = m_pAuthImpl;
        CRpcServices* pSvc = pImpl;
        if( pSvc == nullptr )
        {
            TaskGrpPtr pg = GetPendingLogins();
            if( pg.IsEmpty()  ||
                pg->IsStopped( pg->GetTaskState() ) )
            {
                break;
            }
            else if( !pg.IsEmpty() &&
                !pg->IsStopped( pg->GetTaskState() ) )
            {
                oLock.Unlock();
                ( *pg )( eventCancelTask );
                break;
            }
        }
        CStdRMutex oIfLock( pSvc->GetLock() );
        if( pSvc->GetState() != stateStopped &&
            pSvc->GetState() != stateStopping )
        {
            oIfLock.Unlock();
            oLock.Unlock();
            ret = pSvc->Shutdown( pRespCb );
            if( ret != STATUS_PENDING )
                ( *pRespCb )( eventCancelTask );
            break;
        }
    }while( 0 );
    else if( strMech == "SimpAuth" )
    {
        ObjPtr pEmpty;
        SetAuthImpl( pEmpty );
        ret = CSimpleAuthCliWrapper::Destroy(
            GetIoMgr(), pCallback );
    }
    return ret;
}

gint32 CAuthentServer::RemoveSession(
    const std::string& strSess )
{
    CRpcServices* pSvc = m_pAuthImpl;

    IAuthenticateServer* pAuth =
    dynamic_cast< IAuthenticateServer* >
            ( pSvc );

    if( pAuth == nullptr )
        return -EFAULT;

    return pAuth->RemoveSession( strSess );
}

gint32 CAuthentServer::IsSessExpired(
    const std::string& strSess )
{
    ObjPtr pIf;
    gint32 ret = GetAuthImpl( pIf );
    if( ERROR( ret ) )
    {
        // yes sess expired
        return STATUS_SUCCESS;
    }

    CRpcServices* pSvc = pIf;
    IAuthenticateServer* pAuth =
    dynamic_cast< IAuthenticateServer* >
            ( pSvc );

    if( pAuth == nullptr )
        return -EFAULT;

    return pAuth->IsSessExpired( strSess );
}

gint32 CRpcRouterBridgeAuth::OnStartRfpaComplete(
    IEventSink* pCallback,
    IEventSink* pIoReq,
    IConfigDb* pReqCtx )
{
    if( pIoReq == nullptr ||
        pReqCtx == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CCfgOpenerObj oReq( pIoReq );
        IConfigDb* pResp = nullptr;
        ret = oReq.GetPointer(
            propRespPtr, pResp );

        if( ERROR( ret ) )
            break;

        CCfgOpener oResp( pResp );
        gint32 iRet = 0;
        ret = oResp.GetIntProp(
            propReturnValue,
            ( guint32& ) iRet );

        if( ERROR( ret ) )
            break;

        TaskletPtr pStartRecv;
        ret = pStartRecv.NewObj( 
            clsid( CIfStartRecvMsgTask ),
            pReqCtx );

        if( ERROR( ret ) )
            break;

        ret = AddAndRun( pStartRecv );
        if( ret == STATUS_PENDING )
            ret = 0;

    }while( 0 );

    pCallback->OnEvent( eventTaskComp,
        ret, 0, ( LONGWORD* )this );

    return ret;
}

gint32 CRpcRouterBridgeAuth::OnPostStart(
    IEventSink* pCallback )
{
    gint32 ret = super::OnPostStart(
        pCallback );

    if( ERROR( ret ) )
        return ret;
    do{
        TaskGrpPtr pTransGrp;
        CParamList oParams;
        oParams[ propIfPtr ] = ObjPtr( this );
        ret = pTransGrp.NewObj(
            clsid( CIfTransactGroup ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        ret = LoadBlockList();
        if( ERROR( ret ) )
            break;
        // NOTE:this match does not have
        // propPortId, so cannot be enabled to
        // receive events on.  For the reqfwdr
        // proxy, it is OK to deliver the auth
        // request to the AuthentServer without
        // the match
        MatchPtr& pMatch = m_pAuthMatch;

        TaskletPtr pStartTask;
        ret = BuildStartStopReqFwdrProxy(
            pMatch, true, pStartTask );
        if( ERROR( ret ) )
            break;

        pTransGrp->AppendTask( pStartTask );

        TaskletPtr pEnableEvtTask;

        oParams.Push( true );
        oParams.SetPointer(
            propMatchPtr, ( CObjBase* )pMatch );
        
        ret = pEnableEvtTask.NewObj(
            clsid( CIfEnableEventTask ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        pTransGrp->AppendTask( pEnableEvtTask );

        TaskletPtr pRespCb;
        ret = NEW_PROXY_RESP_HANDLER2(
            pRespCb, ObjPtr( this ),
            &CRpcRouterBridgeAuth::OnStartRfpaComplete,
            pCallback,
            ( IConfigDb* )oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        CIfRetryTask* pGrp = pTransGrp;
        pGrp->SetClientNotify( pRespCb );
        TaskletPtr pTask( pGrp );
        ret = GetIoMgr()->RescheduleTask( pTask );
        if( SUCCEEDED( ret ) )
            ret = STATUS_PENDING;
        
    }while( 0 );

    return ret;
}

#include "jsondef.h"
gint32 CRpcRouterBridgeAuth::LoadBlockList()
{
    gint32 ret = 0;
    do{
        stdstr strPath;
        auto pMgr = GetIoMgr();
        ret = pMgr->TryFindDescFile(
            "invalidpath/blist.json",
            strPath );
        if( ERROR( ret ) )
            break;
        Json::Value blist;
        ret = ReadJsonCfgFile( strPath, blist );
        if( ERROR( ret ) )
            break;
        if( !blist.isObject() ||
            !blist.isMember( "BlockList" ) )
        {
            ret = -EINVAL;
            break;
        }
        Json::Value arrAddrs = blist[ "BlockList" ];
        for( int i = 0; i < arrAddrs.size(); i++ )
        {
            Json::Value& oElem = arrAddrs[ i ];
            if( !oElem.isObject() ||
                !oElem.isMember( JSON_ATTR_OBJNAME ) || 
                !oElem.isMember( JSON_ATTR_MODNAME ) ||
                !oElem.isMember( JSON_ATTR_ROUTER_PATH ) )
            {
               ret = -EINVAL;
               break;
            }
            m_vecBlockList.push_back(
                { oElem[ JSON_ATTR_ROUTER_PATH ].asString(),
                oElem[ JSON_ATTR_MODNAME ].asString(),
                oElem[ JSON_ATTR_OBJNAME ].asString() } );
        }
    }while( 0 );
    if( ERROR( ret ) )
        OutputMsg( ret, "Error loading block-list file" );
    return ret;
}
    
#include <stdio.h>
gint32 CRpcRouterBridgeAuth::FilterMatchBlockList(
    IMessageMatch* pMatch )
{
    gint32 ret = 0;
    do{
        Variant oVar;
        ret = pMatch->GetProperty( propObjPath, oVar );
        if( ERROR( ret ) )
            break;
        constexpr auto dwPrefix =
            sizeof( DBUS_NAME_PREFIX );
        stdstr& strObjPath = ( stdstr& )oVar;
        if( strObjPath.substr( 0, dwPrefix ) !=
            "/org/rpcf/" )
        {
            ret = -EINVAL;
            break;
        }
        auto strPath =
            ( ( stdstr& )oVar ).substr( dwPrefix );
        auto pos1 = strPath.find_first_of( '/' );
        if( pos1 == stdstr::npos )
        {
            ret = -EINVAL;
            break;
        }
        stdstr strMod =
            strPath.substr( 0, pos1 );

        pos1 = strPath.find_first_not_of( '/', pos1 );
        if( pos1 == stdstr::npos )
        {
            ret = -EINVAL;
            break;
        }
        auto pos2 = strPath.find_first_of(
            '/', pos1 );
        if( pos2 == stdstr::npos )
        {
            ret = -EINVAL;
            break;
        }
        if( strPath.substr( pos1, pos2 - pos1 ) != "objs" )
        {
            ret = -EINVAL;
            break;
        }
        pos1 = strPath.find_first_not_of( '/', pos2 );
        if( pos1 == stdstr::npos )
        {
            ret = -EINVAL;
            break;
        }
        stdstr strObj = strPath.substr( pos1 );
        for( auto& elem : m_vecBlockList )
        {
            if( strMod != std::get<1>( elem ) )
                continue;
            auto& strBlkObj = std::get<2>( elem );
            if( strObj.size() < strBlkObj.size() )
                continue;
            if( strBlkObj != strObj.substr(
                0, strBlkObj.size() ) )
                continue;
            ret = -EACCES;
            break;
        }
    }while( 0 );
    return ret;
}

gint32 CRpcRouterBridgeAuth::RunEnableEventTask(
    IEventSink* pCallback,
    IMessageMatch* pMatch )
{
    gint32 ret = 0;
    do{
        ret = FilterMatchBlockList( pMatch );
        if( ERROR( ret ) )
            break;
        ret = super::RunEnableEventTask(
            pCallback, pMatch );

    }while( 0 );
    return ret;
}

}
