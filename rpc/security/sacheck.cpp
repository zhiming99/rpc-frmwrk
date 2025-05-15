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
    return CreateSimpleAuthcli( pMgr,
        clsid( CSimpleAuthCliWrapper ),
        pCallback, pCfg );
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

gint32 CSimpleAuthCliWrapper::Login(
    IEventSink* pCallback,
    IConfigDb* pInfo, /*[ in ]*/
    CfgPtr& pResp ) /*[ out ]*/
{
    return ERROR_NOT_IMPL;
}

gint32 CSimpleAuthCliWrapper::InquireSess(
    const std::string& strSess,
    CfgPtr& pInfo )
{
    return ERROR_NOT_IMPL;
}

gint32 CSimpleAuthCliWrapper::GenSessHash(
    stdstr strToken,
    guint32 dwPortId,
    std::string& strSess )
{
    return ERROR_NOT_IMPL;
}

gint32 CSimpleAuthCliWrapper::RemoveSession(
    const std::string& strSess )
{
    return ERROR_NOT_IMPL;
}

gint32 CSimpleAuthCliWrapper::GetSess(
    guint32 dwPortId,
    std::string& strSess )
{
    return ERROR_NOT_IMPL;
}

}
