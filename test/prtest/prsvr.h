/*
 * =====================================================================================
 *
 *       Filename:  asyncsvr.h
 *
 *    Description:  asyncsvr and test declaration
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
#include <ifhelper.h>

#define METHOD_Echo         "Echo"
#define METHOD_EchoMany     "EchoMany"
#define METHOD_LongWait     "LongWait"
#define METHOD_EchoUnknown  "EchoUnknown"

#define MOD_SERVER_NAME "PauseResumeServer"
#define OBJNAME_ECHOSVR "CPauseResumeServer"               

#define MOD_CLIENT_NAME "PauseResumeClient"
#define OBJNAME_ECHOCLIENT "CPauseResumeClient"               

enum EnumMyClsid
{
    DECL_CLSID( MyStart ) = clsid( UserClsidStart ) + 601,
    DECL_CLSID( CPauseResumeServer ),
    DECL_CLSID( CPauseResumeClient )
};

class CPauseResumeServer :
    public CInterfaceServer
{ 
    public: 

    typedef CInterfaceServer super;

    CPauseResumeServer( const IConfigDb* pCfg );

    gint32 InitUserFuncs();

    gint32 OnLongWaitComplete(
        IEventSink* pCallback,
        const std::string& strText );

    gint32 LongWait(
        IEventSink* pCallback, // mandatory
        const std::string& strText ); // the input arg

    gint32 EchoUnknown(
        IEventSink* pCallback,  // mandatory
        const BufPtr& pBuf,     // the input arg
        BufPtr& pBufReply );    // the output arg

    gint32 Echo(
        IEventSink* pCallback, // mandatory
        const std::string& strText, // the input
        std::string& strReply ); // the output

    gint32 EchoMany(
        IEventSink* pCallback, // mandatory
        gint32 i1, gint16 i2, gint64 i3, float i4, double i5, const std::string& strText,
        gint32& o1, gint16& o2, gint64& o3, float& o4, double& o5, std::string& strReply );
        
};

#include "ifhelper.h"
BEGIN_DECL_PROXY_SYNC( CPauseResumeClient, CInterfaceProxy )

    DECL_PROXY_METHOD_SYNC( 1, Echo,
        const std::string& /* strEmit */,
        std::string& /* strReply */ );

    DECL_PROXY_METHOD_SYNC( 1, LongWait,
        const std::string& /* strEmit */,
        std::string& /* strReply */ );

    DECL_PROXY_METHOD_SYNC( 1, EchoUnknown,
        const BufPtr&, /* pText */
        BufPtr& );  /* pReply */

    DECL_PROXY_METHOD_SYNC( 6, EchoMany,
        gint32 /*i1*/, gint16 /*i2*/, gint64 /*i3*/, float /*i4*/, double /*i5*/, const std::string& /*strText*/,
        gint32& /*o1*/, gint16& /*o2*/, gint64& /*o3*/, float& /*o4*/, double& /*o5*/, std::string& /*strReply*/ );


END_DECL_PROXY_SYNC( CEchoClient );
