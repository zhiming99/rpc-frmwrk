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

#define METHOD_Echo     "Echo"
#define METHOD_LongWait "LongWait"

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
        const BufPtr& pBuf );   // the input arg

    gint32 Echo( IEventSink* pCallback, // mandatory
        const std::string& strString ); // the input arg
};

class CPauseResumeClient :
    public CInterfaceProxy
{
    sem_t m_semWait;

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
};
