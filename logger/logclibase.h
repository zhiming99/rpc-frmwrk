/*
 * =====================================================================================
 *
 *       Filename:  logclibase.h
 *
 *    Description:  Declaration of the interface class ILogSvc_PImpl as the base class
 *                  of the CLogService_CliImpl.
 *
 *        Version:  1.0
 *        Created:  07/03/2024 09:26:58 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:  
 *
 *      Copyright:  2024 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#pragma once

#include "logger.h"

namespace rpcf
{
class ILogSvc_PImpl
    : public virtual CAggInterfaceProxy
{
    public:
    typedef CAggInterfaceProxy super;
    ILogSvc_PImpl( const IConfigDb* pCfg ) :
        super( pCfg )
        {}

    gint32 InitUserFuncs()
    {
        BEGIN_IFPROXY_MAP( LogSvc, false );

        ADD_USER_PROXY_METHOD_EX( 1,
            ILogSvc_PImpl::DebugLogDummy,
            "DebugLog" );

        ADD_USER_PROXY_METHOD_EX( 1,
            ILogSvc_PImpl::LogMessageDummy,
            "LogMessage" );

        ADD_USER_PROXY_METHOD_EX( 1,
            ILogSvc_PImpl::LogCritMsgDummy,
            "LogCritMsg" );

        END_IFPROXY_MAP;
        
        return STATUS_SUCCESS;
    }
    
    const EnumClsid GetIid() const override
    { return iid( LogSvc ); }

    gint32 DebugLogDummy( BufPtr& pBuf_ )
    { return STATUS_SUCCESS; }
    
    gint32 LogMessageDummy( BufPtr& pBuf_ )
    { return STATUS_SUCCESS; }
    
    gint32 LogCritMsgDummy( BufPtr& pBuf_ )
    { return STATUS_SUCCESS; }
    
    //RPC Sync Req Sender
    virtual gint32 DebugLog(
        const std::string& strMsg ) = 0;
    
    //RPC Sync Req Sender
    virtual gint32 LogCritMsg(
        const std::string& strMsg ) = 0;
    
    //RPC Sync Req Sender
    virtual gint32 LogMessage(
        const std::string& strMsg ) = 0;
};

}
