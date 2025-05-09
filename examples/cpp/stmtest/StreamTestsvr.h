//Generated by ridlc
//Your task is to implement the following classes
//to get your rpc server work
#pragma once
#include "stmtest.h"
#include "commdefs.h"
#include "IStreamTestsvr.h"

DECLARE_AGGREGATED_SERVER(
    CStreamTest_SvrSkel,
    CStatCountersServer,
    CStreamServerAsync,
    IIStreamTest_SImpl );

struct TransferContext
{
    gint32 m_iCounter = 0;
    gint32 m_iError = 0;

    inline void IncCounter()
    { m_iCounter++; }
    inline gint32 GetCounter()
    { return m_iCounter; }

    inline void SetError( gint32 iError )
    { m_iError = iError; }

    inline gint32 GetError()
    { return m_iError; }
};

class CStreamTest_SvrImpl
    : public CStreamTest_SvrSkel
{
    public:
    typedef CStreamTest_SvrSkel super;
    CStreamTest_SvrImpl( const IConfigDb* pCfg ) :
        super::virtbase( pCfg ), super( pCfg )
    {}

    /* The following 3 methods are important for */
    /* streaming transfer. rewrite them if necessary */
    gint32 OnStreamReady( HANDLE hChannel ) override;
    
    gint32 OnStmClosing( HANDLE hChannel ) override;
    
    gint32 AcceptNewStream(
        IEventSink* pCb, IConfigDb* pDataDesc ) override
    { return STATUS_SUCCESS; }
    
    virtual gint32 OnReadStreamComplete(
        HANDLE hChannel, gint32 iRet,
        BufPtr& pBuf, IConfigDb* pCtx ) override;

    virtual gint32 OnWriteStreamComplete(
        HANDLE hChannel, gint32 iRet,
        BufPtr& pBuf, IConfigDb* pCtx ) override;

    // IStreamTest
    virtual gint32 Echo(
        const std::string& i0 /*[ In ]*/,
        std::string& i0r /*[ Out ]*/ );
    
    gint32 ReadAndReply( HANDLE hChannel );
    gint32 WriteAndReceive( HANDLE hChannel );

    gint32 RunReadWriteTasks( HANDLE hChannel );

    protected:
    gint32 BuildAsyncTask( HANDLE hChannel,
        bool bRead, TaskletPtr& pTask );
};

