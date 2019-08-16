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
#include <stream.h>

#define METHOD_Echo         "Echo"

// this is for object path
#define MOD_SERVER_NAME "StreamingServer"
#define OBJNAME_ECHOSVR "CStreamingServer"               

#define MOD_CLIENT_NAME "StreamingClient"
#define OBJNAME_ECHOCLIENT "CStreamingClient"               

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
class CEchoClient :
    public virtual CAggInterfaceProxy
{
    public:
    typedef CAggInterfaceProxy super;

    const EnumClsid GetIid() const
    { return iid( CEchoServer ); }

    CEchoClient( const IConfigDb* pCfg );
    gint32 InitUserFuncs();
    gint32 Echo( const std::string& strEmit,
        std::string& strReply );
};

class CMyStreamProxy :
    public CStreamProxy
{
    sem_t m_semSync;

    public:
    typedef CStreamProxy super;
    CMyStreamProxy( const IConfigDb* pCfg ) :
        CAggInterfaceProxy( pCfg ), super( pCfg )
    {
        Sem_Init( &m_semSync, 0, 0 );
    }

    gint32 OnFCLifted( HANDLE hChannel )
    { return Sem_Post( &m_semSync ); }

    // Note: Just for demo purpose. There should be a
    // map for binding of channel and semaphore
    gint32 WaitForWriteAllowed( HANDLE hChannel )
    { return Sem_Wait( &m_semSync ); }

    virtual gint32 OnClose(
        HANDLE hChannel,
        IEventSink* pCallback = nullptr )
    {
        gint32 ret = super::OnClose(
            hChannel, pCallback );
        // in case the client is still waiting for flow
        // control to lift
        Sem_Post( &m_semSync );
        return ret;
    }

    // mandatory, otherwise, the proxy map for this
    // interface may not be initialized
    gint32 InitUserFuncs()
    { return super::InitUserFuncs(); }

    gint32 OnConnected( HANDLE hChannel );

    // mandatory
    // data is ready for reading
    gint32 OnStmRecv(
        HANDLE hChannel, BufPtr& pBuf );
};

class CMyStreamServer :
    public CStreamServer
{
    public:
    typedef CStreamServer super;
    CMyStreamServer( const IConfigDb* pCfg ) :
        CAggInterfaceServer( pCfg ), super( pCfg )
    {}

    // mandatory, otherwise, the proxy map for this
    // interface may not be initialized
    gint32 InitUserFuncs()
    { return super::InitUserFuncs(); }

    // mandatory
    gint32 OnConnected( HANDLE hChannel );

    virtual gint32 OnClose(
        HANDLE hChannel,
        IEventSink* pCallback = nullptr );

    // mandatory
    // data is ready for reading
    gint32 OnStmRecv(
        HANDLE hChannel, BufPtr& pBuf );
};

// Declare the major server/proxy objects. Please note
// that, the first class is the class to declare, and
// the second class to the last one are implementations
// of the different interfaces
DECLARE_AGGREGATED_PROXY(
    CStreamingClient,
    CEchoClient,
    CMyStreamProxy );

DECLARE_AGGREGATED_SERVER(
    CStreamingServer,
    CEchoServer,
    CMyStreamServer );

