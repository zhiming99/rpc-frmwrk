/*
 * =====================================================================================
 *
 *       Filename:  oa2proy.h
 *
 *    Description:  declararation of OAuth2 proxy for non-js client login
 *
 *        Version:  1.0
 *        Created:  12/02/2024 02:54:26 PM
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
 * =====================================================================================
 */
#pragma once
#include "configdb.h"
#include "defines.h"
#include "ifhelper.h"
#include "security.h"

namespace rpcf
{

class COAuth2LoginProxy :
    public virtual CAggInterfaceProxy,
    public IAuthenticateProxy
{
    // the session hash
    std::string m_strSess;

    // the GSS-API context
    CRpcRouter* m_pRouter = nullptr;
    guint64 m_qwSalt = 0;

    gint32 StartLogin(
        IEventSink* pCallback );

    gint32 DecryptCookie(
        const stdstr& strHex,
        stdstr& strCookie );

    inline void SetSess(
        const stdstr& strSess )
    { m_strSess = strSess; }

    public:
    using IAuthenticate::Login;

    typedef CAggInterfaceProxy super;

    COAuth2LoginProxy( const IConfigDb* pCfg );

    const EnumClsid GetIid() const
    { return iid( IAuthenticate ); }

    virtual gint32 InitUserFuncs();

    gint32 DoLogin(
        IEventSink* pCallback );

    gint32 Login(
        IEventSink* pCallback,
        IConfigDb* pInfo, /*[ in ]*/
        CfgPtr& pResp );/*[ out ]*/

    gint32 OAuth2Login(
        IEventSink* pCallback,
        IConfigDb* pInfo );

    // a sync call since the krb hook is a sync
    // call, make sure this method is called from
    // a thread that can wait for long.
    gint32 MechSpecReq(
        IEventSink* pCallback,
        IConfigDb* pCtx,/*[ in ]  pCtx */
        CfgPtr& pResp ) /* [ out ] pResp */
    { return -ENOTSUP; }

    virtual gint32 GetSess(
        std::string& strSess ) const
    { strSess = m_strSess; return 0; }

    virtual gint32 RebuildMatches();

    gint32 WrapMessage(
        BufPtr& pInMsg,
        BufPtr& pOutMsg ) override 
    { return -ENOTSUP; }
        
    gint32 UnwrapMessage(
        BufPtr& pInMsg,
        BufPtr& pOutMsg ) override
    { return -ENOTSUP; }

    gint32 GetMicMsg(
        BufPtr& pInMsg,/* [ in ] */
        BufPtr& pOutMic ) override
    { return -ENOTSUP; }

    gint32 VerifyMicMsg(
        BufPtr& pInMsg,/* [ in ] */
        BufPtr& pInMic ) override
    { return -ENOTSUP; }
};

// auth/access control proxy implementation
// with K5
DECLARE_AGGREGATED_PROXY(
    COAuth2LoginProxyImpl,
    COAuth2LoginProxy );
}
