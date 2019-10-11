/*
 * =====================================================================================
 *
 *       Filename:  stmsvr.h
 *
 *    Description:  Declaration of CStreamingServer/Proxy and related unit tests
 *
 *        Version:  1.0
 *        Created:  12/04/2018 10:37:00 AM
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
#include <filetran.h>
#include <streamex.h>

#define METHOD_Echo         "Echo"

// this is for object path
#define MOD_SERVER_NAME "StreamingServer"
#define OBJNAME_ECHOSVR "CStreamingServer"               

#define MOD_CLIENT_NAME "StreamingClient"
#define OBJNAME_ECHOCLIENT "CStreamingClient"               

#define LOOP_COUNT  30000
// Declare the class id for this object and declare the
// iid for the interfaces we have implemented for this
// object.
// Note that, CFileTransferServer and IInterfaceServer
// are built-in interfaces
enum EnumMyClsid
{
    DECL_CLSID( MyStart ) = clsid( UserClsidStart ) + 711,
    DECL_CLSID( CStreamingServer ),
    DECL_CLSID( CStreamingClient ),

    DECL_IID( MyStart ) = clsid( ReservedIidStart ) + 200,
    DECL_IID( CEchoServer )
};

// Declare the interface class
//
// Note that unlike the CEchoServer in Helloworld test.
// this class inherit from a virtual base
// CAggInterfaceServer. The reason is simple, multiple
// interfaces share the same instance of
// CAggInterfaceServer.
class CEchoServer :
    public virtual CAggInterfaceServer
{ 
    public: 
    typedef CAggInterfaceServer super;

    CEchoServer( const IConfigDb* pCfg )
    : super( pCfg )
    {}

    const EnumClsid GetIid() const
    { return iid( CEchoServer ); }

    gint32 InitUserFuncs();
    gint32 Echo(
        IEventSink* pCallback,
        const std::string& strText,
        std::string& strReply );
};

// Declare the interface class
#include "ifhelper.h"
BEGIN_DECL_IF_PROXY_SYNC( CEchoServer, CEchoClient )

    DECL_IF_PROXY_METHOD_SYNC( 1,
        CEchoServer, Echo,
        const std::string& /* strEmit */,
        std::string& /* strReply */ );

END_DECL_IF_PROXY_SYNC( CEchoClient );

class CMyStreamProxy :
    public CStreamProxySync
{
    public:
    typedef CStreamProxySync super;
    CMyStreamProxy( const IConfigDb* pCfg ) :
        super::_MyVirtBase( pCfg ), super( pCfg )
    {
    }

    gint32 SendMessage( HANDLE hChannel );
    /**
    * @name OnWriteEnabled_Loop
    * @{ an event handler called when the channel
    * `hChannel' is ready for writing. it could be
    * temporarily blocked if the flowcontrol
    * happens. The first event to the channel is
    * also a WiteEnabled event after the channel
    * is established for read/write. When the flow
    * control happens, the WriteXXX will return
    * ERROR_QUEUE_FULL, and you cannot write more
    * messages to the stream channel.
    * */
    /**  @} */
    
    virtual gint32 OnWriteEnabled_Loop(
        HANDLE hChannel );
    /**
    * @name OnRecvData_Loop
    * @{ an event handler called when there is
    * data arrives to the channel `hChannel'.
    * */
    /**  @} */

    virtual gint32 OnRecvData_Loop(
        HANDLE hChannel, gint32 iRet );

    /**
    * @name OnSendDone_Loop
    * @{ an event handler called when the last
    * pending write is done on the channel.
    * */
    /**  @} */

    virtual gint32 OnSendDone_Loop(
        HANDLE hChannel, gint32 iRet );

    virtual gint32 OnStart_Loop();

    virtual gint32 OnCloseChannel_Loop(
        HANDLE hChannel );
};

class CMyStreamServer :
    public CStreamServerSync
{
    public:
    typedef CStreamServerSync super;
    CMyStreamServer( const IConfigDb* pCfg ) :
        super::_MyVirtBase( pCfg ), super( pCfg )
    {}

    virtual gint32 OnRecvData_Loop(
        HANDLE hChannel, gint32 iRet );

    virtual gint32 OnSendDone_Loop(
        HANDLE hChannel, gint32 iRet );

    virtual gint32 OnWriteEnabled_Loop(
        HANDLE hChannel );

    virtual gint32 OnCloseChannel_Loop(
        HANDLE hChannel )
    { return 0; }

    virtual gint32 OnStart_Loop()
    { return 0; }
};

// Declare the major server/proxy objects. Please
// note that, the first class is the class to
// declare, and the second class to the last one
// are implementations of the different interfaces
DECLARE_AGGREGATED_PROXY(
    CStreamingClient,
    CEchoClient,
    CMyStreamProxy );

DECLARE_AGGREGATED_SERVER(
    CStreamingServer,
    CEchoServer,
    CMyStreamServer );

