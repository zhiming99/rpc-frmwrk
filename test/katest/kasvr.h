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

#define METHOD_LongWait "LongWait"

#define MOD_SERVER_NAME "KeepAliveServer"
#define OBJNAME_ECHOSVR "CKeepAliveServer"               

#define MOD_CLIENT_NAME "KeepAliveClient"
#define OBJNAME_ECHOCLIENT "CKeepAliveClient"               

enum EnumMyClsid
{
    DECL_CLSID( MyStart ) = clsid( UserClsidStart ) + 501,
    DECL_CLSID( CKeepAliveServer ),
    DECL_CLSID( CKeepAliveClient )
};

class CKeepAliveServer :
    public CInterfaceServer
{ 
    public: 

    typedef CInterfaceServer super;

    CKeepAliveServer( const IConfigDb* pCfg );

    gint32 InitUserFuncs();

    gint32 OnLongWaitComplete(
        IEventSink* pCallback,
        const std::string& strText );

    gint32 LongWait(
        IEventSink* pCallback,
        const std::string& strText );

};

class CKeepAliveClient :
    public CInterfaceProxy
{
    sem_t m_semWait;

    public:
    typedef CInterfaceProxy super;
    CKeepAliveClient( const IConfigDb* pCfg );

    gint32 InitUserFuncs();

    gint32 LongWait(
        const std::string& strText,
        std::string& strReply );

};
