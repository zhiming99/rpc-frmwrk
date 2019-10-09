/*
 * =====================================================================================
 *
 *       Filename:  inprocsvr.h
 *
 *    Description:  CInProcServer and CInProcClient declaration
 *
 *        Version:  1.0
 *        Created:  07/15/2018 11:22:27 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com ), 
 *   Organization:  
 *
 * =====================================================================================
 */
#pragma once

#include <rpc.h>
#include <string>
#include <iostream>
#include <unistd.h>
#include <proxy.h>

#define EVENT_HelloWorld "OnHelloWorld"

#define MOD_SERVER_NAME "InProcSvr"

#define OBJNAME_CLIENT "CInProcClient"               

#define OBJNAME_SERVER  "CInProcServer"               
#define OBJDESC_PATH    "./inproc.json"

#define METHOD_Echo "Echo"

enum EnumMyClsid
{
    DECL_CLSID( MyStart ) = clsid( UserClsidStart ) + 401,
    DECL_CLSID( CInProcServer ),
    DECL_CLSID( CInProcClient )
};

class CInProcServer :
    public CInterfaceServer
{ 
    public: 

    typedef CInterfaceServer super;

    CInProcServer( const IConfigDb* pCfg );
    gint32 InitUserFuncs();

    // event broadcast
    gint32 OnHelloWorld(
        const std::string& strEvent );

    // handlers
    gint32 Echo(
        IEventSink* pCallback,
        const std::string& strText );

    gint32 EchoUnknown(
        IEventSink* pCallback,
        const BufPtr& pBuf );
};

#include "ifhelper.h"
BEGIN_DECL_PROXY_SYNC( CInProcClient, CInterfaceProxy )

    gint32 InitUserFuncs();

    // event handlers
    gint32 OnHelloWorld(
        IEventSink* pContext,
        const std::string& strEvent );

    DECL_PROXY_METHOD_SYNC( 1, Echo,
        const std::string& /* strEmit */,
        std::string& /* strReply */ );

    DECL_PROXY_METHOD_SYNC( 1, EchoUnknown,
        const BufPtr&, /* pText */
        BufPtr& );  /* pReply */

END_DECL_PROXY_SYNC( CInProcClient );
