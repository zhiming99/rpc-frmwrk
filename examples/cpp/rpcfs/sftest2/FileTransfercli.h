// Generated by ridlc
// ridlc -s -O . ../../sftest.ridl 
// Your task is to implement the following classes
// to get your rpc server work
#pragma once
#include "sftest.h"
#include "transctx.h"

#define Clsid_CFileTransfer_CliBase    Clsid_Invalid

DECLARE_AGGREGATED_PROXY(
    CFileTransfer_CliBase,
    CStreamProxyAsync,
    IIFileTransfer_CliApi,
    CFastRpcProxyBase );

class CFileTransfer_CliImpl
    : public CFileTransfer_CliBase
{
    public:
    TransferContextCli< CStreamProxyAsync > m_oTransCtx;

    typedef CFileTransfer_CliBase super;
    CFileTransfer_CliImpl( const IConfigDb* pCfg ) :
        super::virtbase( pCfg ), super( pCfg ),
        m_oTransCtx( this )
    {
        m_oTransCtx.AddRef();
 	SetClassId( clsid(CFileTransfer_CliImpl ) );
    }

    gint32 UploadFile(
        HANDLE hChannel, const stdstr& strFile );

    gint32 DownloadFile(
        HANDLE hChannel, const stdstr& strFile,
        guint64 qwOffset, guint64 qwSize  );

    /* The following 2 methods are important for */
    /* streaming transfer. rewrite them if necessary */
    gint32 OnStreamReady( HANDLE hChannel ) override
    { return super::OnStreamReady( hChannel ); } 
    
    gint32 OnStmClosing( HANDLE hChannel ) override
    {
        m_oTransCtx.OnTransferDone(
            hChannel, -ECONNABORTED );
        return super::OnStmClosing( hChannel );
    }

    gint32 CreateStmSkel(
        InterfPtr& pIf ) override;
    
    gint32 OnPreStart(
        IEventSink* pCallback ) override;

    // IFileTransfer
    gint32 OnReadStreamComplete(
        HANDLE hChannel, gint32 iRet,
        BufPtr& pBuf, IConfigDb* pCtx )
    {
        if( iRet < 0 )
        {
            OutputMsg( iRet,
                "ReadStreamAsync failed with error" );
        }
        return m_oTransCtx.OnReadStreamComplete(
            hChannel, iRet, pBuf, pCtx );
    }
    
    gint32 OnWriteStreamComplete(
        HANDLE hChannel, gint32 iRet,
        BufPtr& pBuf, IConfigDb* pCtx )
    {
        if( iRet < 0 )
        {
            OutputMsg( iRet,
                "WriteStreamAsync failed with error" );
        }
        return m_oTransCtx.OnWriteStreamComplete(
            hChannel, iRet, pBuf, pCtx );
    }
};

class CFileTransfer_ChannelCli
    : public CRpcStreamChannelCli
{
    public:
    typedef CRpcStreamChannelCli super;
    CFileTransfer_ChannelCli(
        const IConfigDb* pCfg ) :
        super::virtbase( pCfg ), super( pCfg )
    { SetClassId( clsid(
        CFileTransfer_ChannelCli ) ); }
};
