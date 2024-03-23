//Generated by ridlc
//Your task is to implement the following classes
//to get your rpc server work
#pragma once
#include "sftest.h"
#include "transctx.h"

class CFileTransfer_SvrImpl
    : public CFileTransfer_SvrSkel
{
    std::string m_strRootDir = "/tmp/sfsvr-root/";

    public:
    TransferContext< CStreamServerAsync > m_oTransCtx;
    typedef CFileTransfer_SvrSkel super;
    CFileTransfer_SvrImpl( const IConfigDb* pCfg );

    /* The following 3 methods are important for */
    /* streaming transfer. rewrite them if necessary */
    gint32 OnStreamReady( HANDLE hChannel ) override;
    
    gint32 OnStmClosing( HANDLE hChannel ) override;
    
    gint32 AcceptNewStream(
        IEventSink* pCb, IConfigDb* pDataDesc ) override
    { return STATUS_SUCCESS; }
    
    // IFileTransfer
    virtual gint32 StartUpload(
        const std::string& strFile /*[ In ]*/,
        HANDLE hChannel_h /*[ In ]*/,
        guint64 qwOffset /*[ In ]*/,
        guint64 qwSize /*[ In ]*/ );
    
    virtual gint32 StartDownload(
        const std::string& strFile /*[ In ]*/,
        HANDLE hChannel_h /*[ In ]*/,
        guint64 qwOffset /*[ In ]*/,
        guint64 qwSize /*[ In ]*/ );
    
    virtual gint32 GetFileInfo(
        const std::string& strFile /*[ In ]*/,
        FileInfo& fi /*[ Out ]*/ );
    
    virtual gint32 RemoveFile(
        const std::string& strFile /*[ In ]*/ );

    gint32 ReadFileAndSend( HANDLE hChannel )
    { return m_oTransCtx.ReadFileAndSend( hChannel ); }

    gint32 WriteFileAndRecv( HANDLE hChannel, BufPtr& pBuf )
    { return m_oTransCtx.WriteFileAndRecv( hChannel, pBuf ); }

    gint32 OnWriteStreamComplete(
        HANDLE hChannel, gint32 iRet,
        BufPtr& pBuf, IConfigDb* pCtx );

    gint32 OnReadStreamComplete(
        HANDLE hChannel, gint32 iRet,
        BufPtr& pBuf, IConfigDb* pCtx );

    gint32 OnWriteResumed( HANDLE hChannel ) override
    { return m_oTransCtx.OnWriteResumed( hChannel ); }

    gint32 SendToken( HANDLE hChannel, BufPtr& pBuf )
    { return m_oTransCtx.SendToken( hChannel, pBuf ); }
};

