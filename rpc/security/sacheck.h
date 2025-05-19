/*
 * =====================================================================================
 *
 *       Filename:  sacheck.cpp
 *
 *    Description:  Declaration of SimpAuth checker to accept login request, 
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
#pragma once
#include "SimpleAuthcli.h"

namespace rpcf
{

struct CSacwCallbacks : IAsyncSACallbacks
{
    InterfPtr m_pThis;
    gint32 CheckClientTokenCallback( 
        IConfigDb* context, gint32 iRet,
        ObjPtr& oInfo /*[ In ]*/ ) override;
    void OnSvrOffline( IConfigDb*,
        CSimpleAuth_CliImpl* ) override;
};

class CSimpleAuthCliWrapper :
    public CSimpleAuth_CliImpl,
    public IAuthenticateServer
{
    InterfPtr m_pRouter;

    std::map< std::string, guint32 > m_mapSess2PortId;
    std::map< guint32, std::string > m_mapPortId2Sess;
    std::map< guint32, ObjPtr > m_mapSessions; 

    public:
    typedef CSimpleAuth_CliImpl super;
    CSimpleAuthCliWrapper( const IConfigDb* pCfg );

    static gint32 Create( CIoManager* pMgr,
        IEventSink* pCallback, IConfigDb* pCfg );

    static gint32 Destroy( CIoManager* pMgr,
        IEventSink* pCallback );

    gint32 Login(
        IEventSink* pCallback,
        IConfigDb* pInfo, /*[ in ]*/
        CfgPtr& pResp ) override; /*[ out ]*/

    gint32 MechSpecReq(
        IEventSink* pCallback,
        IConfigDb* pReq,/*[ in ]*/
        CfgPtr& pResp ) override/*[ out ]*/
    { return -ENOTSUP; }

    gint32 WrapMessage(
        const std::string& strSess,
        BufPtr& pInMsg,
        BufPtr& pOutMsg ) override
    {
        pOutMsg = pInMsg;
        return 0;
    }
        
    gint32 UnwrapMessage(
        const std::string& strSess,
        BufPtr& pInMsg,
        BufPtr& pOutMsg ) override
    {
        pOutMsg = pInMsg;
        return 0;
    }

    gint32 GetMicMsg(
        const std::string& strSess,
        BufPtr& pInMsg,
        BufPtr& pOutMsg ) override
    {
        pOutMsg = pInMsg;
        return 0;
    }
        
    gint32 VerifyMicMsg(
        const std::string& strSess,
        BufPtr& pInMsg,
        BufPtr& pInMic ) override
    { return 0; }

    gint32 IsNoEnc(
        const std::string& strSess ) override
    { return ERROR_FALSE; }

    gint32 IsSessExpired(
        const std::string& strSess ) override;

    gint32 AddSession( guint32 dwPortId, 
        const std::string& strSess,
        CfgPtr& pSessInfo );

    gint32 InquireSess(
        const std::string& strSess,
        CfgPtr& pInfo ) override;

    gint32 GenSessHash(
        const stdstr& strToken,
        guint32 dwPortId,
        std::string& strSess );

    gint32 RemoveSession(
        const stdstr& strSess ) override;

    gint32 RemoveSession( guint32 dwPortId );

    gint32 GetSess(
        guint32 dwPortId,
        std::string& strSess ) override;

    InterfPtr GetRouter() const
    { return m_pRouter; }

    gint32 OnPostStop(
        IEventSink* pCb ) override
    {
        m_pRouter.Clear();
        return super::OnPostStop( pCb );
    }
};

}
