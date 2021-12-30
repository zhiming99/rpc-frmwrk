/****BACKUP YOUR CODE BEFORE RUNNING RIDLC***/
// Implement the following methods
// to get the RPC proxy/server work
#include "rpc.h"
using namespace rpcf;
#include "sftest.h"
#include "FileTransfercli.h"

gint32 CFileTransfer_CliImpl::UploadFile(
    HANDLE hChannel, const stdstr& strFile )
{
    if( hChannel == INVALID_HANDLE ||
        strFile.empty() )
        return -EINVAL;

    if( !IsFileExist( strFile ) )
        return -ENOENT;

    gint32 ret = 0;
    do{
        TransFileContext o( strFile );
        o.m_cDirection = 'u';
        o.m_strPath = strFile;
        o.m_bServer = false;
        ret = o.OpenFile();
        if( ERROR( ret ) )
            break;
        m_oTransCtx.AddContext( hChannel, o );
        BufPtr pBuf( true );
        *pBuf = strFile;
        stdstr strName = basename( pBuf->ptr() );

        ret = this->StartUpload(
            strName, hChannel, 0, o.m_iSize );
        if( ERROR( ret ) )
            break;

        ret = m_oTransCtx.ReadFileAndSend(
            hChannel );
    }while( 0 );

    return ret; 
}

gint32 CFileTransfer_CliImpl::DownloadFile(
    HANDLE hChannel, const stdstr& strFile,
    guint64 qwOffset, guint64 qwSize  )
{
    if( hChannel == INVALID_HANDLE ||
        strFile.empty() )
        return -EINVAL;

    if( !IsFileNameValid( strFile ) )
        return - EINVAL;

    gint32 ret = 0;
    do{
        TransFileContext o( strFile );
        o.m_cDirection = 'd';
        o.m_strPath = "./" + strFile + ".1";
        o.m_bServer = false;
        o.m_iOffset = qwOffset;
        o.m_iSize = qwSize;

        ret = o.OpenFile();
        if( ERROR( ret ) )
            break;
        ret = m_oTransCtx.AddContext(
            hChannel, o );
        if( ERROR( ret ) )
            break;

        ret = StartDownload( strFile,
            hChannel, qwOffset, qwSize );
        if( ERROR( ret ) )
            break;
        BufPtr pBuf;
        ret = m_oTransCtx.WriteFileAndRecv(
            hChannel, pBuf );

    }while( 0 );
    return ret;
}


