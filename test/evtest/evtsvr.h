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

#define EVENT_HelloWorld "OnHelloWorld"

#define MOD_SERVER_NAME "EventServer"
#define OBJNAME_ECHOSVR "CEventServer"               

#define MOD_CLIENT_NAME "EventClient"
#define OBJNAME_ECHOCLIENT "CEventClient"               

#define OBJNAME_SERVER  "CEventServer"               
#define OBJDESC_PATH    "./evtdesc.json"

enum EnumMyClsid
{
    DECL_CLSID( MyStart ) = clsid( UserClsidStart ) + 301,
    DECL_CLSID( CEventServer ),
    DECL_CLSID( CEventClient )
};

class CEventServer :
    public CInterfaceServer
{ 
    public: 

    typedef CInterfaceServer super;

    CEventServer( const IConfigDb* pCfg );
    gint32 InitUserFuncs();
    gint32 OnHelloWorld(
        const std::string& strEvent );
};

class CEventClient :
    public CInterfaceProxy
{
    public:

    typedef CInterfaceProxy super;
    CEventClient( const IConfigDb* pCfg );
    gint32 InitUserFuncs();

    gint32 OnHelloWorld(
        IEventSink* pContext,
        const std::string& strEvent );
};
