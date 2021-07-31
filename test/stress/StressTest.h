#pragma once
#include <string>
#include "rpc.h"
#include "ifhelper.h"
#include "streamex.h"

#define DECLPTRO( _type, _name ) \
    ObjPtr p##_name;\
    p##_name.NewObj( clsid( _type ) );\
    _type& _name=*(_type*)p##_name;

enum EnumMyClsid
{
    DECL_CLSID( CStressSvc_CliSkel ) = 480072123,
    DECL_CLSID( CStressSvc_SvrSkel ),
    DECL_CLSID( CStressSvc_CliImpl ),
    DECL_CLSID( CStressSvc_SvrImpl ),
    
    DECL_IID( IEchoThings ),
    
    
};

class IIEchoThings_PImpl
    : public virtual CAggInterfaceProxy
{
    public:
    typedef CAggInterfaceProxy super;
    IIEchoThings_PImpl( const IConfigDb* pCfg ) :
        super( pCfg )
        {}
    gint32 InitUserFuncs();
    
    const EnumClsid GetIid() const override
    { return iid( IEchoThings ); }

    //RPC Async Req Sender
    gint32 Echo( 
        IConfigDb* context, 
        const std::string& strText );

    gint32 EchoDummy( BufPtr& pBuf_ )
    { return STATUS_SUCCESS; }
    
    //Async callback wrapper
    gint32 EchoCbWrapper( 
        IEventSink* pCallback, 
        IEventSink* pIoReq,
        IConfigDb* pReqCtx );
    
    //RPC Async Req Callback
    //TODO: implement me by adding
    //response processing code
    virtual gint32 EchoCallback(
        IConfigDb* context, 
        gint32 iRet,const std::string& strResp ) = 0;
    
    //RPC Sync Req Sender
    gint32 EchoUnknown(
        BufPtr& pBuf,
        BufPtr& pResp );
    
    gint32 EchoUnknownDummy( BufPtr& pBuf_ )
    { return STATUS_SUCCESS; }
    
    //RPC Sync Req Sender
    gint32 Ping(
        const std::string& strCount );
    
    gint32 PingDummy( BufPtr& pBuf_ )
    { return STATUS_SUCCESS; }
    
    //RPC event handler 'OnHelloWorld'
    //TODO: implement me
    virtual gint32 OnHelloWorld(
        const std::string& strMsg ) = 0;
    
    //RPC event handler wrapper
    gint32 OnHelloWorldWrapper(
        IEventSink* pCallback, BufPtr& pBuf_ );
    
};

class IIEchoThings_SImpl
    : public virtual CAggInterfaceServer
{
    public:
    typedef CAggInterfaceServer super;
    IIEchoThings_SImpl( const IConfigDb* pCfg ) :
        super( pCfg )
        {}
    gint32 InitUserFuncs();
    
    const EnumClsid GetIid() const override
    { return iid( IEchoThings ); }

    //RPC Sync Req Handler Wrapper
    gint32 EchoWrapper(
        IEventSink* pCallback, BufPtr& pBuf_ );
    
    //RPC Sync Req Handler
    //TODO: implement me
    virtual gint32 Echo(
        const std::string& strText,
        std::string& strResp ) = 0;
    
    //RPC Sync Req Handler Wrapper
    gint32 EchoUnknownWrapper(
        IEventSink* pCallback, BufPtr& pBuf_ );
    
    //RPC Sync Req Handler
    //TODO: implement me
    virtual gint32 EchoUnknown(
        BufPtr& pBuf,
        BufPtr& pResp ) = 0;
    
    //RPC Sync Req Handler Wrapper
    gint32 PingWrapper(
        IEventSink* pCallback, BufPtr& pBuf_ );
    
    //RPC Sync Req Handler
    //TODO: implement me
    virtual gint32 Ping(
        const std::string& strCount ) = 0;
    
    //RPC event sender
    gint32 OnHelloWorld(
        const std::string& strMsg );
    
};

DECLARE_AGGREGATED_PROXY(
    CStressSvc_CliSkel,
    CStreamProxyAsync,
    IIEchoThings_PImpl );

DECLARE_AGGREGATED_SERVER(
    CStressSvc_SvrSkel,
    CStreamServerAsync,
    IIEchoThings_SImpl );

