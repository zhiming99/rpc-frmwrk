/*
 * =====================================================================================
 *
 *       Filename:  saproxy.h
 *
 *    Description:  Declararation of SimpAuth proxy for non-js client login
 *
 *        Version:  1.0
 *        Created:  05/11/2025 10:20:58 AM
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

#pragma once
#include "configdb.h"
#include "defines.h"
#include "ifhelper.h"
#include "security.h"

namespace rpcf
{

class CSimpAuthLoginProxy :
    public virtual CAggInterfaceProxy,
    public IAuthenticateProxy
{
    // the GSS-API context
    CRpcRouter* m_pRouter = nullptr;
    stdstr m_strSess;

    gint32 StartLogin(
        IEventSink* pCallback );

    inline void SetSess(
        const stdstr& strSess )
    { m_strSess = strSess; }

    gint32 BuildLoginInfo(
        const stdstr& strUser, CfgPtr& pInfo );

    public:
    using IAuthenticate::Login;

    typedef CAggInterfaceProxy super;

    CSimpAuthLoginProxy( const IConfigDb* pCfg );

    const EnumClsid GetIid() const
    { return iid( IAuthenticate ); }

    virtual gint32 InitUserFuncs();

    gint32 DoLogin(
        IEventSink* pCallback );

    gint32 Login(
        IEventSink* pCallback,
        IConfigDb* pInfo, /*[ in ]*/
        CfgPtr& pResp );/*[ out ]*/

    gint32 SimpAuthLogin(
        IEventSink* pCallback,
        IConfigDb* pInfo );

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
// with simple authentication.
DECLARE_AGGREGATED_PROXY(
    CSimpAuthLoginProxyImpl,
    CSimpAuthLoginProxy );
}
