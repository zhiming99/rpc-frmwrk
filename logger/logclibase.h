/*
 * =====================================================================================
 *
 *       Filename:  logclibase.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/03/2024 09:26:58 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
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
