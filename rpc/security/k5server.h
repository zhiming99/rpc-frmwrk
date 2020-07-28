/*
 * =====================================================================================
 *
 *       Filename:  k5server.h
 *
 *    Description:  declaration of CK5AuthServer object and the tcp port for kdc
 *                  connection.
 *
 *        Version:  1.0
 *        Created:  07/15/2020 10:28:26 PM
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
#pragma once

#include "configdb.h"
#include "defines.h"
#include "ifhelper.h"
#include <krb5.h>
#include "secclsid.h"
#include "security.h"

#define KDCRELAY_DESC_FILE "./authprxy.json"

class CKdcRelayProxy : 
    public CInterfaceProxy
{
    public:

    typedef CInterfaceProxy super;

    CKdcRelayProxy( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CKdcRelayProxy ) ); }

    gint32 InitUserFuncs();
    gint32 SetupReqIrp( IRP* pIrp,
        IConfigDb* pReqCall,
        IEventSink* pCallback );

    gint32 FillRespData(
        IRP* pIrp, CfgPtr& pResp );

    gint32 MechSpecReq(
        IEventSink* pCallback,
        BufPtr& pReq,
        BufPtr& pResp ); /* [ out ] pResp */
};

class CK5AuthServer :
    public CInterfaceServer,
    public IAuthenticateServer
{
    // the sever is an interface hosted by the
    // CRpcRouterBridgeAuthImpl object session
    // map, SESSION_ID is a pair of `initiator
    // name' and a `port-id'
    typedef std::pair< std::string, guint32 > SESSION_ID;

    std::map< std::string, guint32 > m_mapSessToPortId;
    std::map< guint32, std::string > m_mapPortIdToSess;

    using ITRSP = std::map< std::string, guint32 >::iterator;
    using ITRPS = std::map< guint32, std::string >::iterator;

    // a map from session string to the gss_sec_context
    std::map< guint32, gss_ctx_id_t > m_mapSessions; 
    using ITRGSS = std::map< guint32, gss_ctx_id_t >::iterator;

    // a loop to send/receive kdc messages
    stdrmutex m_oAuthLock;

    InterfPtr   m_pKdcProxy;
    gss_cred_id_t m_pSvcCred = GSS_C_NO_CREDENTIAL;

    CRpcRouter* m_pRouter = nullptr;

    gint32 OnSendKdcRequestComplete(
        IEventSink* pCallback, 
        IEventSink* pIoReq,
        IConfigDb* pReqCtx );

    gint32 StartKdcProxy(
        IEventSink* pCallback );

    gint32 AddSession(
        guint32 dwPortId, 
        const std::string& strSess );

    gint32 RemoveSession(
        guint32 dwPortId );

    gint32 Krb5Login(
        IEventSink* pCallback,
        guint32 dwPortId,
        BufPtr& pToken,
        BufPtr& pRespTok );

    gint32 NoTokenLogin(
        IEventSink* pCallback,
        guint32 dwPortId );

    gint32 OnStartKdcProxyComplete(
        IEventSink* pInvTask, 
        IEventSink* pIoReq,
        IConfigDb* pReqCtx );

    gint32 SendKdcRequest(
        IEventSink* pInvTask,
        IConfigDb* pReq,
        bool bFirst ); /*[ in ]*/

    public:

    using IAuthenticate::Login;
    typedef CInterfaceServer super;

    CK5AuthServer( const IConfigDb* pCfg );

    inline CRpcRouter* GetRouter() const
    { return m_pRouter; }

    gint32 GenSessHash(
        gss_ctx_id_t gssctx,
        guint32 dwPortId,
        std::string& strSess );

    virtual gint32 Login(
        IEventSink* pCallback,
        IConfigDb* pInfo, /*[ in ]*/
        CfgPtr& pResp ); /*[ out ]*/

    // traffics between client and kdc
    virtual gint32 MechSpecReq(
        IEventSink* pCallback,
        IConfigDb* pReq, /*[ in ]*/
        IConfigDb* pResp ); /*[ out ]*/

    inline stdrmutex& GetGssLock()
    { return m_oAuthLock; }

    gint32 BuildSendReqTask(
        IEventSink* pInvTask,
        IConfigDb* pReq,
        bool bFirst,
        TaskletPtr& pTask );

    gint32 OnPostStart( IEventSink* pCallback );
    gint32 OnPreStop( IEventSink* pCallback );
    gint32 OnPostStop( IEventSink* pCallback );

    // for ForwardEvent
    // To obtain all the sessions on the port
    // `dwPortId'
    gint32 GetSessionByPortId(
        gint32 dwPortId,
        std::string& strSess );

    gint32 WrapMessage(
        const std::string& strSess,
        BufPtr& pInMsg,
        BufPtr& pOutMsg );
        
    gint32 UnwrapMessage(
        const std::string& strSess,
        BufPtr& pInMsg,
        BufPtr& pOutMsg );

    gint32 GetMicMsg(
        const std::string& strSess,
        BufPtr& pInMsg,
        BufPtr& pOutMic );
        
    gint32 VerifyMicMsg(
        const std::string& strSess,
        BufPtr& pInMsg,
        BufPtr& pInMic );

    gint32 GetPortId(
        const std::string& strSess,
        guint32& dwPortId );

    gint32 GetSess(
        guint32 dwPortId,
        std::string& strSess );

    gint32 GetGssCtx(
        const std::string& strSess,
        gss_ctx_id_t& context );

    gint32 RemoveSession(
        const std::string& strSess );

    gint32 IsNoEnc(
        const std::string& strSess );
};

