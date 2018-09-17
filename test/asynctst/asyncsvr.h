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
#define METHOD_LongWaitNoParam "LongWaitNoParam"

#define MOD_SERVER_NAME "AsyncServer"
#define OBJNAME_ECHOSVR "CAsyncServer"               

#define MOD_CLIENT_NAME "AsyncClient"
#define OBJNAME_ECHOCLIENT "CAsyncClient"               

enum EnumMyClsid
{
    DECL_CLSID( MyStart ) = clsid( ReservedClsidEnd ) + 101,
    DECL_CLSID( CAsyncServer ),
    DECL_CLSID( CAsyncClient )
};

class CAsyncServer :
    public CInterfaceServer
{ 
    public: 

    typedef CInterfaceServer super;

    CAsyncServer( const IConfigDb* pCfg );

    gint32 InitUserFuncs();

    gint32 OnLongWaitComplete(
        IEventSink* pCallback,
        const std::string& strText );

    gint32 LongWait(
        IEventSink* pCallback,
        const std::string& strText );

    gint32 LongWaitNoParam(
        IEventSink* pCallback );
};

class CAsyncClient :
    public CInterfaceProxy
{
    sem_t m_semWait;

    public:
    typedef CInterfaceProxy super;
    CAsyncClient( const IConfigDb* pCfg );

    gint32 InitUserFuncs();

    gint32 LongWait(
        const std::string& strText,
        std::string& strReply );

    gint32 OnLongWaitNoParamComplete(
        IEventSink* pCallback,
        gint32 iRet );

    gint32 LongWaitNoParam();

    gint32 WaitForComplete();
};
