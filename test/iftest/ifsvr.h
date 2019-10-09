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

#define METHOD_Echo             "Echo"
#define METHOD_EchoPtr          "EchoPtr"
#define METHOD_EchoCfg          "EchoCfg"
#define METHOD_EchoUnknown      "EchoUnknown"
#define METHOD_Ping             "Ping"

#define MOD_SERVER_NAME "EchoServer"
#define OBJNAME_ECHOSVR "CEchoServer"               

#define MOD_CLIENT_NAME "EchoClient"
#define OBJNAME_ECHOCLIENT "CEchoClient"               

enum EnumMyClsid
{
    DECL_CLSID( MyStart ) = clsid( UserClsidStart ) + 1,
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

    gint32 Echo2( IEventSink* pCallback,
        gint32 i1, double f2, double& i3 );
};

#include "ifhelper.h"
// declare the proxy class
BEGIN_DECLARE_PROXY_CLASS_SYNC( CEchoClient )

    DECL_PROXY_METHOD_SYNC( 0, CEchoClient, Ping );

    DECL_PROXY_METHOD_SYNC( 1, CEchoClient, Echo,
        const std::string& /* strEmit */,
        std::string& /* strReply */ );

    DECL_PROXY_METHOD_SYNC( 1, CEchoClient, EchoPtr,
        const char*, /*szText*/
        const char*& ); /* szReply */

    DECL_PROXY_METHOD_SYNC( 2, CEchoClient, EchoCfg,
        gint32, /* iCount */
        const CfgPtr&, /* pCfg */
        gint32&, /* iCountReply */
        CfgPtr& ); /* pCfgReply */

    DECL_PROXY_METHOD_SYNC( 1, CEchoClient, EchoUnknown,
        const BufPtr&, /* pText */
        BufPtr& );  /* pReply */

    DECL_PROXY_METHOD_SYNC( 2, CEchoClient, Echo2,
        const gint32&, /* i1 */
        const double&, /* i2 */
        double& );     /* reply = i1+i2 */

END_DECLARE_PROXY_CLASS_SYNC( CEchoClient );
