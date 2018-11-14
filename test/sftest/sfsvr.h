/*
 * =====================================================================================
 *
 *       Filename:  sfsvr.h
 *
 *    Description:  Declaration of CSendFetchServer/Proxy and related unit tests
 *
 *        Version:  1.0
 *        Created:  08/11/2018 14:32:00 AM
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
#include <ifhelper.h>
#include <filetran.h>

#define METHOD_Echo         "Echo"

#define MOD_SERVER_NAME "SendFetchServer"
#define OBJNAME_ECHOSVR "CSendFetchServer"               

#define MOD_CLIENT_NAME "SendFetchClient"
#define OBJNAME_ECHOCLIENT "CSendFetchClient"               

enum EnumMyClsid
{
    DECL_CLSID( MyStart ) = clsid( UserClsidStart ) + 611,
    DECL_CLSID( CSendFetchServer ),
    DECL_CLSID( CSendFetchClient ),

    DECL_IID( MyStart ) = clsid( ReservedIidStart ) + 100,
    DECL_IID( CEchoServer )
};

class CEchoServer :
    public virtual CInterfaceServer
{ 
    public: 
    typedef CInterfaceServer super;

    CEchoServer(
        const IConfigDb* pCfg )
    : super( pCfg )
    {}

    gint32 InitUserFuncs();
    gint32 Echo(
        IEventSink* pCallback,
        const std::string& strText,
        std::string& strReply );
};

class CEchoClient :
    public virtual CInterfaceProxy
{
    public:
    typedef CInterfaceProxy super;

    CEchoClient( const IConfigDb* pCfg );
    gint32 InitUserFuncs();
    gint32 Echo( const std::string& strEmit,
        std::string& strReply );
};



DECLARE_AGGREGATED_PROXY(
    CSendFetchClient,
    CFileTransferProxy,
    CEchoClient );

DECLARE_AGGREGATED_SERVER(
    CSendFetchServer,
    CFileTransferServer,
    CEchoServer );

