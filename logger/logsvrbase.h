/*
 * =====================================================================================
 *
 *       Filename:  logsvrbase.h
 *
 *    Description:  Declaration of the interface class ILogSvc_SImpl as the base class
 *                  of the CLogService_SvrImpl.
 *
 *        Version:  1.0
 *        Created:  07/03/2024 09:28:35 PM
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
class ILogSvc_SImpl
    : public virtual CAggInterfaceServer
{
    public:
    typedef CAggInterfaceServer super;
    ILogSvc_SImpl( const IConfigDb* pCfg ) :
        super( pCfg )
        {}
    gint32 InitUserFuncs();
    
    const EnumClsid GetIid() const override
    { return iid( LogSvc ); }

    //RPC Sync Req Handler Wrapper
    gint32 DebugLogWrapper(
        IEventSink* pCallback, BufPtr& pBuf_ );
    
    //RPC Sync Req Handler
    //TODO: implement me
    virtual gint32 DebugLog(
        const std::string& strMsg ) = 0;
    
    //RPC Sync Req Handler Wrapper
    gint32 LogMessageWrapper(
        IEventSink* pCallback, BufPtr& pBuf_ );
    
    //RPC Sync Req Handler
    //TODO: implement me
    virtual gint32 LogMessage(
        const std::string& strMsg ) = 0;
    
    //RPC Async Req Handler wrapper
    gint32 LogCritMsgWrapper(
        IEventSink* pCallback, BufPtr& pBuf_ );
    
    gint32 LogCritMsgCancelWrapper(
        IEventSink* pCallback,
        gint32 iRet,
        IConfigDb* pReqCtx_, BufPtr& pBuf_ );
    
    //RPC Async Req Cancel Handler
    virtual gint32 OnLogCritMsgCanceled(
        IConfigDb* pReqCtx_, gint32 iRet,
        const std::string& strMsg ) = 0;
    
    //RPC Async Req Callback
    //Call this method when you have
    //finished the async operation
    //with all the return value set
    //or an error code
    virtual gint32 LogCritMsgComplete( 
        IConfigDb* pReqCtx_, gint32 iRet );
    
    //RPC Async Req Handler
    //TODO: adding code to emit your async
    //operation, keep a copy of pCallback and
    //return STATUS_PENDING
    virtual gint32 LogCritMsg(
        IConfigDb* pReqCtx_,
        const std::string& strMsg ) = 0;
    
};

DECLARE_AGGREGATED_SERVER(
    CLogService_SvrSkel,
    CStatCountersServer,
    ILogSvc_SImpl );
}
