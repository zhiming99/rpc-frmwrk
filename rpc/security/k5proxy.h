/*
 * =====================================================================================
 *
 *       Filename:  k5proxy.h
 *
 *    Description:  declararation of auth implementation with Krb5-MIT
 *
 *        Version:  1.0
 *        Created:  05/24/2020 04:09:54 PM
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
#include "security.h"
#include "secclsid.h"
#include <krb5/locate_plugin.h>

#define DESC_FILE               "./authprxy.json"
#define PLUGIN_MODULE_NAME      "kdconn"
#define OBJNAME_AUTHSVR         "K5AuthServer"
#define OBJNAME_KDCCHANNEL      "KdcChannel"
#define USERNAME_NOLOGIN        "kdcclient"

namespace rpcf
{

gint32 gss_sess_hash_partial(
    gss_ctx_id_t gssctx, BufPtr& pBuf );

gint32 gen_sess_hash(
    BufPtr& pBuf,
    std::string& strSess );

gint32 AppendConnParams(
    IConfigDb* pConnParams,
    BufPtr& pBuf );

gint32 wrap_message(
    gss_ctx_id_t context,
    BufPtr& pInBuf,
    BufPtr& pOutBuf );

gint32 unwrap_message(
    gss_ctx_id_t context,
    BufPtr& pInBuf,
    BufPtr& pOutBuf );

gint32 get_mic(
    gss_ctx_id_t context,
    BufPtr& pInBuf,
    BufPtr& pOutBuf );

gint32 verify_message(
    gss_ctx_id_t context,
    BufPtr& pInBuf,
    BufPtr& pMicBuf );

gint32 AcquireCred(
    const std::string& strName,
    gss_OID& oNameType,
    gss_cred_id_t* pServerCred );

struct CKrb5InitHook : public CObjBase
{
    bool m_bInited = false;
    bool m_bInRouter = false;

    stdrmutex m_oLock;
    InterfPtr m_pProxy;
    ObjPtr m_pMgr;

    gint32 StartProxy();
    gint32 StopProxy();

    inline stdrmutex& GetLock()
    { return m_oLock; }

    typedef CObjBase super;

    CKrb5InitHook( const IConfigDb* pCfg );
    ~CKrb5InitHook()
    { StopProxy(); }

    CIoManager* GetIoMgr()
    { return ( CIoManager* )m_pMgr; }

    void SetIoMgr( CIoManager* pMgr )
    { m_pMgr = pMgr; }

    gint32 Start();

    CRpcServices* GetProxy()
    { return m_pProxy; }

    inline bool IsInited() 
    { return m_bInited; }

};

class CKdcChannelProxy :
    public CInterfaceProxy,
    public IAuthenticate
{
    public:
    typedef CInterfaceProxy super;

    CKdcChannelProxy( const IConfigDb* pCfg )
        : super( pCfg )
    { SetClassId( clsid( CKdcChannelProxy ) ); }

    virtual gint32 InitUserFuncs();
    gint32 MechSpecReq(
        IEventSink* pCallback,
        IConfigDb* pCtx,/*[ in ]  pCtx */
        CfgPtr& pResp ); /* [ out ] pResp */

    gint32 Login(
        IEventSink* pCallback,
        IConfigDb* pInfo, /*[ in ]*/
        CfgPtr& pResp );/*[ out ]*/

    gint32 RebuildMatches();
};

class CRemoteProxyStateAuth :
    public CRemoteProxyState 
{
    public:
    typedef CRemoteProxyState super;
    CRemoteProxyStateAuth(
        const IConfigDb* pCfg ) :
        super( pCfg )
    {
        SetClassId(
            clsid( CRemoteProxyStateAuth ) );
    }

    gint32 SetupOpenPortParams(
        IConfigDb* pCfg )
    {
        gint32 ret =
            super::SetupOpenPortParams( pCfg );
        if( ERROR( ret ) )
            return ret;

        // propOpenPort will tell the dbusprxy
        // pdo to send OPENPORT request rather
        // than local-login
        CCfgOpener oCfg( pCfg );
        oCfg.SetBoolProp( propOpenPort, true );

        return STATUS_SUCCESS;
    }

    gint32 SubscribeEvents()
    {
        std::vector< EnumPropId > vecEvtToSubscribe =
        {
        };

        return SubscribeEventsInternal(
            vecEvtToSubscribe );
    }
};

// Assuming we are dealing with just one kdc
class CInitHookMap
{
    stdrmutex m_oHookLock;
    std::map< guint32, ObjPtr > m_mapInitHooks;
    bool m_bInRouter = false;
    ObjPtr m_pMgr;

    gint32 CreateNewHook( ObjPtr& pHook );

    public:
    CInitHookMap()
    {}

    ~CInitHookMap();

    stdrmutex& GetLock()
    { return m_oHookLock; }

    void SetIoMgr( CIoManager* pMgr )
    { m_pMgr = pMgr; }

    void SetInRouter( bool bIn = true )
    { m_bInRouter = bIn; }

    bool IsInRouter( bool bIn = true )
    { return m_bInRouter; }

    guint32 size() const
    { return m_mapInitHooks.size(); }

    bool empty() const
    { return m_mapInitHooks.empty(); }

    CKrb5InitHook* GetFirst() const
    {
        if( m_mapInitHooks.empty() )
            return nullptr;
        return m_mapInitHooks.begin()->second;
    }

    gint32 GetInitHook(
        CKrb5InitHook*& pHook );

    gint32 Start();
    gint32 Stop();

    gint32 SetInitHook(
        CKrb5InitHook* pHook );

    gint32 RemoveHook();

    // hooks with krb5 plugin
    static void OnKrb5CtxInit(
        krb5_context ctx, void* pobj );

    static krb5_error_code PluginInit(
        krb5_context ctx, void** );

    static void PluginFinish( void* )
    {}

    static krb5_error_code Lookup( void *,
        enum locate_service_type svc, const char *realm,
        int socktype, int family,
        int (*cbfunc)(void *,int,struct sockaddr *), 
        void *cbdata);

    // Make sure to run this blocking method on a
    // stand-alone thread.
    static krb5_error_code Krb5SendHook(
        krb5_context context, void *data,
        const krb5_data *realm,
        const krb5_data *message,
        krb5_data **new_message_out,
        krb5_data **reply_out);
};

class CK5AuthProxy :
    public virtual CAggInterfaceProxy,
    public IAuthenticateProxy
{
    // the session hash
    std::string m_strSess;

    // the GSS-API context
    gss_ctx_id_t m_gssctx = GSS_C_NO_CONTEXT;
    CRpcRouter* m_pRouter = nullptr;
    stdrmutex m_oGssLock;
    guint64 m_qwSalt = 0;

    gint32 StartLogin(
        IEventSink* pCallback );

    gint32 Krb5Login(
        const std::string& strUserName,
        const std::string& strSvcName );

    gint32 DeleteSecCtx();
    gint32 NoTokenLogin();

    public:
    using IAuthenticate::Login;

    typedef CAggInterfaceProxy super;

    CK5AuthProxy( const IConfigDb* pCfg );

    static gint32 InitEnvRouter(
        CIoManager* pMgr );

    inline stdrmutex& GetGssLock()
    { return m_oGssLock; }

    const EnumClsid GetIid() const
    { return iid( IAuthenticate ); }

    virtual bool IsConnected(
        const char* szDestAddr = nullptr );

    virtual gint32 InitUserFuncs();

    gint32 OnStartLoginComplete(
        IEventSink* pCallback, 
        IEventSink* pIoReq,
        IConfigDb* pReqCtx );

    gint32 DoLogin(
        IEventSink* pCallback );

    gint32 Login(
        IEventSink* pCallback,
        IConfigDb* pInfo, /*[ in ]*/
        CfgPtr& pResp );/*[ out ]*/

    // a sync call since the krb hook is a sync
    // call, make sure this method is called from
    // a thread that can wait for long.
    gint32 MechSpecReq(
        IEventSink* pCallback,
        IConfigDb* pCtx,/*[ in ]  pCtx */
        CfgPtr& pResp ); /* [ out ] pResp */

    virtual gint32 OnPostStop(
        IEventSink* pCallback );

    virtual gint32 OnPreStart(
        IEventSink* pCallback );

    virtual gint32 GetSess(
        std::string& strSess ) const
    { strSess = m_strSess; return 0; }

    virtual gint32 RebuildMatches();

    virtual gss_ctx_id_t GetGssCtx()
    { return m_gssctx; }

    virtual gint32 WrapMessage(
        BufPtr& pInMsg,
        BufPtr& pOutMsg );
        
    virtual gint32 UnwrapMessage(
        BufPtr& pInMsg,
        BufPtr& pOutMsg );

    virtual gint32 GetMicMsg(
        BufPtr& pInMsg,/* [ in ] */
        BufPtr& pOutMic );/* [ out ] */

    virtual gint32 VerifyMicMsg(
        BufPtr& pInMsg,/* [ in ] */
        BufPtr& pInMic );/* [ in ] */

    gint32 InitPluginHook();
};

// auth/access control proxy implementation
// with K5
DECLARE_AGGREGATED_PROXY(
    CAuthentProxyK5Impl,
    CK5AuthProxy );

}
