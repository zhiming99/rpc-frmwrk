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

#define MOD_SERVER_NAME "HelloWorldServer"
#define OBJNAME_ECHOSVR "CEchoServer"               

#define MOD_CLIENT_NAME "HelloWorldClient"
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
};

// declare the proxy class
#include "ifhelper.h"
BEGIN_DECL_PROXY_SYNC( CEchoClient, CInterfaceProxy )

/**
* @name Ping
* @{
* @brief the simpliest method to get the
* return value from the server. It can detect
* if the server is still alive.
* @0: zero input parameter
* @Mehtod name: Ping
* @Method Parameters:
*   none
* @return 
*   STATUS_SUCCESS if the request succeeds.
*   otherwise an error code is returned.
* */
/**  @} */
    
    DECL_PROXY_METHOD_SYNC( 0, Ping );

/**
* @name Echo
* @{
* @brief Send a string and receive the replay from
* the server. The request and reply string should
* be the same.
* @1: 1 input parameter
* @method name: Echo
* @parameters:
*   strEmit [ in ]: string to send
*   strReply[ out ]: string to hold the reply text.
* @return 
*   STATUS_SUCCESS if the request succeeds.
*   otherwise an error code is returned.
* */
/**  @} */
    DECL_PROXY_METHOD_SYNC( 1, Echo,
        const std::string& /* strEmit */,
        std::string& /* strReply */ );
/**
* @name EchoPtr
* @{
* @brief Send a string pointed to by szText, and
* receive a string contained in szReply. it is a
* test if the char* can send/receive successfully.
*
* @1: 1 input parameter
* @method name: EchoPtr
* @parameters:
*   szEmit [ in ]: string to send
*   szReply[ out ]: string to hold the reply text.
* @return 
*   STATUS_SUCCESS if the request succeeds.
*   otherwise an error code is returned.
* */
/**  @} */

    DECL_PROXY_METHOD_SYNC( 1, EchoPtr,
        const char*, /*szText*/
        const char*& ); /* szReply */
/**
* @name EchoCfg
* @{ 
* @brief Send a property map to the server and
* receive a property map from the server. also
* send a 32bit integer in iCount, and expect the
* server to send it back in the iCountReply
* @2 : there are two input parameters
* @Method name: EchoCfg
* @Parameters:
*   iCount [ in ]: a 32bit integer to send
*   pCfg [ in ] : a property map to send
*   iCountReply[ out ]: a 32bit integer to receive
*   pCfgReply [ out ] : a property map to receive.
*
* @return 
*   STATUS_SUCCESS if the request succeeds.
*   otherwise an error code is returned.
*
* */
/**  @} */

    DECL_PROXY_METHOD_SYNC( 2, EchoCfg,
        gint32, /* iCount */
        const CfgPtr&, /* pCfg */
        gint32&, /* iCountReply */
        CfgPtr& ); /* pCfgReply */
/**
* @name EchoUnknown
* @{
* @brief Send a byte block to the server and
* receive the copy in response. It is suitable for
* data types unable to transfer directly, for
* example, a plain C++ object, or a CObjBase object
* without Serialize/Deserialize.
*
* @1: 1 input parameter
* @Method name: EchoUnknown
* @parameters:
*   pText[ in ]: the byte block to send
*   pReply[ out ] : the byte block reponse from server
*
* @return 
*   STATUS_SUCCESS if the request succeeds.
*   otherwise an error code is returned.
* */
/**  @} */

    DECL_PROXY_METHOD_SYNC( 1, EchoUnknown,
        const BufPtr&, /* pText */
        BufPtr& );  /* pReply */

END_DECL_PROXY_SYNC( CEchoClient );
