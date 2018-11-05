/*
 * =====================================================================================
 *
 *       Filename:  ifsvr.h
 *
 *    Description:  ifsvr and test declaration
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

#define METHOD_Echo     "Echo"
#define METHOD_EchoPtr  "EchoPtr"
#define METHOD_EchoCfg  "EchoCfg"
#define METHOD_Ping     "Ping"

#define MOD_SERVER_NAME "EchoServer"
#define OBJNAME_ECHOSVR "CEchoServer"               

#define MOD_CLIENT_NAME "EchoClient"
#define OBJNAME_ECHOCLIENT "CEchoClient"               

enum EnumMyClsid
{
    DECL_CLSID( MyStart ) = clsid( ReservedClsidEnd ) + 1,
    DECL_CLSID( CEchoServer ),
    DECL_CLSID( CEchoClient )
};

class CEchoServer :
    public CInterfaceServer
{ 
    public: 

    typedef CInterfaceServer super;

    CEchoServer( const IConfigDb* pCfg );

    gint32 InitUserFuncs();

    gint32 Echo(
        IEventSink* pCallback,
        const std::string& strText,
        std::string& strReply );

    gint32 EchoPtr(
        IEventSink* pCallback,
        const char* szText );

    gint32 EchoCfg(
        IEventSink* pCallback,
        gint32 iCount,
        const CfgPtr& pCfg,
        gint32& iCountReply,
        CfgPtr& pCfgReply );

    gint32 EchoUnknown(
        IEventSink* pCallback,
        const BufPtr& pBuf );

    gint32 Ping( IEventSink* pCallback );
};

class CEchoClient :
    public CInterfaceProxy
{
    public:
    typedef CInterfaceProxy super;
    CEchoClient( const IConfigDb* pCfg );
    gint32 InitUserFuncs();
    gint32 Echo( const std::string& strEmit,
        std::string& strReply );

    gint32 EchoPtr(
        const char* szText,
        const char*& szReply );

    gint32 EchoCfg(
        gint32 iCount,
        const CfgPtr& pCfg,
        gint32& iCountReply,
        CfgPtr& pCfgReply );

    gint32 EchoUnknown(
        const BufPtr& pText,
        BufPtr& pReply );

    gint32 Ping();
};
