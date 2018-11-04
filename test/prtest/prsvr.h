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

#define MOD_SERVER_NAME "PauseResumeServer"
#define OBJNAME_ECHOSVR "CPauseResumeServer"               

#define MOD_CLIENT_NAME "PauseResumeClient"
#define OBJNAME_ECHOCLIENT "CPauseResumeClient"               

enum EnumMyClsid
{
    DECL_CLSID( MyStart ) = clsid( ReservedClsidEnd ) + 601,
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

class CPauseResumeClient :
    public CInterfaceProxy
{
    public:
    typedef CInterfaceProxy super;
    CPauseResumeClient( const IConfigDb* pCfg );

    gint32 InitUserFuncs();

    gint32 LongWait(
        const std::string& strText,
        std::string& strReply );

    gint32 EchoUnknown(
        const BufPtr& pBufToSend, // buf to echo
        BufPtr& pRemoteReply );     // buf echoed

    gint32 Echo(
        const std::string& strEmit, // string to echo
        std::string& strReply );    // string echoed

    gint32 EchoMany(
        gint32 i1, gint16 i2, gint64 i3, float i4, double i5, const std::string& strText,
        gint32& o1, gint16& o2, gint64& o3, float& o4, double& o5, std::string& strReply );
};
