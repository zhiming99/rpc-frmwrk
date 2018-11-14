/*
 * =====================================================================================
 *
 *       Filename:  actcsvr.h
 *
 *    Description:  active cancel svr/client and test declaration
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

#define MOD_SERVER_NAME "ActcServer"
#define OBJNAME_SERVER  "CActcServer"               

#define MOD_CLIENT_NAME "ActcClient"
#define OBJNAME_CLIENT "CActcClient"               

#define OBJDESC_PATH    "./actcdesc.json"

enum EnumMyClsid
{
    DECL_CLSID( MyStart ) = clsid( UserClsidStart ) + 201,
    DECL_CLSID( CActcServer ),
    DECL_CLSID( CActcClient )
};

class CActcServer :
    public CInterfaceServer
{ 
    public: 

    typedef CInterfaceServer super;

    CActcServer( const IConfigDb* pCfg );

    gint32 InitUserFuncs();

    gint32 OnLongWaitComplete(
        IEventSink* pCallback,
        const std::string& strText );

    gint32 LongWait(
        IEventSink* pCallback,
        const std::string& strText );

    gint32 OnLongWaitCanceled(
        IEventSink* pCallback,
        gint32 iTimerId );
};

class CActcClient :
    public CInterfaceProxy
{
    sem_t m_semWait;

    public:

    typedef CInterfaceProxy super;

    CActcClient( const IConfigDb* pCfg );
    ~CActcClient();

    gint32 InitUserFuncs();

    gint32 OnLongWaitComplete(
        IEventSink* pCallback,
        gint32 iRet,
        const std::string& strReply );

    gint32 LongWait(
        CfgPtr& pResp,
        const std::string& strText,
        std::string& strReply );

    gint32 WaitForComplete();
};
