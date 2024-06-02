// GENERATED BY RIDLC, MAKE SURE TO BACKUP BEFORE RUNNING RIDLC AGAIN
// Copyright (C) 2024  zhiming <woodhead99@gmail.com>
// This program can be distributed under the terms of the GNU GPLv3.
// ridlc -O . ../oaproxy.ridl 
// Your task is to implement the following classes
// to get your rpc server work
#pragma once
#include "oa2check.h"
#include "../security.h"

class COA2proxy_CliImpl :
    public COA2proxy_CliSkel,
    public IAuthenticateServer
{
    std::map< std::string, guint32 > m_mapSess2PortId;
    std::map< guint32, std::string > m_mapPortId2Sess;
    std::map< guint32, ObjPtr > m_mapSessions; 
    InterfPtr m_pRouter;

    public:
    typedef COA2proxy_CliSkel super;
    COA2proxy_CliImpl( const IConfigDb* pCfg );

    // OAuth2Proxy
    virtual gint32 DoLoginCallback(
        IConfigDb* context, gint32 iRet,
        USER_INFO& ui /*[ In ]*/ );
    
    virtual gint32 GetUserInfoCallback(
        IConfigDb* context, gint32 iRet,
        USER_INFO& ui /*[ In ]*/ );
    
    virtual gint32 RevokeUserCallback(
        IConfigDb* context, gint32 iRet );
    
    virtual gint32 OnOA2Event(
        OA2EVENT& oEvent /*[ In ]*/ );

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
    {
        return 0;
    }

    gint32 AddSession( guint32 dwPortId, 
        const std::string& strSess );

    gint32 RemoveSession( guint32 dwPortId );

    gint32 RemoveSession(
        const std::string& strSess ) override;

    gint32 GetSess(
        guint32 dwPortId,
        std::string& strSess ) override;

    gint32 IsNoEnc(
        const std::string& strSess ) override
    { return 0; }

    gint32 IsSessExpired(
        const std::string& strSess ) override;

    gint32 InquireSess(
        const std::string& strSess,
        CfgPtr& pInfo ) override;

    // IAuthenticate methods
    gint32 Login(
        IEventSink* pCallback,
        IConfigDb* pInfo, /*[ in ]*/
        CfgPtr& pResp /*[ out ]*/
        );

    gint32 MechSpecReq(
        IEventSink* pCallback,
        IConfigDb* pReq,/*[ in ]*/
        CfgPtr& pResp /*[ out ]*/
        ) override 
    { return -ENOTSUP; }

    gint32 GenSessHash(
        stdstr strToken,
        guint32 dwPortId,
        std::string& strSess );

    InterfPtr GetRouter() const
    { return m_pRouter; }

    gint32 OnPostStop(
        IEventSink* pCb ) override
    {
        m_pRouter.Clear();
        return super::OnPostStop( pCb );
    }

    gint32 BuildLoginResp(
        IEventSink* pInv, gint32 iRet,
        const Variant& oToken,
        IConfigDb* pResp );
};

