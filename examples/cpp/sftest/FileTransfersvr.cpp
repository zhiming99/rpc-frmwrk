/****BACKUP YOUR CODE BEFORE RUNNING RIDLC***/
// Implement the following methods
// to get the RPC proxy/server work
#include "rpc.h"
using namespace rpcf;
#include "sftest.h"
#include "FileTransfersvr.h"
#include <regex>
#include <sys/stat.h>

CFileTransfer_SvrImpl::CFileTransfer_SvrImpl(
    const IConfigDb* pCfg ) :
    super::virtbase( pCfg ), super( pCfg ),
    m_oTransCtx( this )
{
    gint32 ret = 0;
    m_oTransCtx.AddRef();
    do{
        if( !IsDirExist( m_strRootDir ) )
        {
            ret = MakeDir( m_strRootDir );
            if( ret < 0 )
            {
                throw std::runtime_error(
                   DebugMsg( -errno,
                       "failed to create directory %s",
                       m_strRootDir.c_str() ) );
            }
        }
        const char* szLogin = getenv( "USER" );
        if( szLogin == nullptr )
        {
            uid_t uid = geteuid();
            if( uid != 0 )
            {
                throw std::runtime_error(
                   DebugMsg( -ENOENT,
                       "no user name!" ) );
            }
            szLogin = "root";
        }
        m_strRootDir += szLogin;
        if( !IsDirExist( m_strRootDir ) )
        {
            ret = MakeDir( m_strRootDir );
            if( ret < 0 )
            {
                throw std::runtime_error(
                   DebugMsg( -errno,
                       "failed to create directory %s",
                       m_strRootDir.c_str() ) );
            }
        }
    }while( 0 );
}

// IFileTransfer Server
/* Sync Req */
gint32 CFileTransfer_SvrImpl::StartUpload(
    const std::string& strFile /*[ In ]*/,
    HANDLE hChannel_h /*[ In ]*/,
    guint64 qwOffset /*[ In ]*/,
    guint64 qwSize /*[ In ]*/ )
{
    if( !IsFileNameValid( strFile ) )
        return -EINVAL;

    TransFileContext o( strFile );
    o.m_cDirection = 'u';
    o.m_iSize = qwSize;
    o.m_iOffset = qwOffset;
    o.m_bServer = true;
    o.m_strPath = m_strRootDir + "/" + strFile;

    gint32 ret = 0;
    do{
        ret = o.OpenFile();
        if( ERROR( ret ) )
            break;

        ret = m_oTransCtx.AddContext(
            hChannel_h, o );
        if( ERROR( ret ) )
            break;

        ret = DEFER_CALL( GetIoMgr(),
            ObjPtr( this ),
            &CFileTransfer_SvrImpl::WriteFileAndRecv,
            hChannel_h, BufPtr() );

    }while( 0 );

    return ret;
}

/* Sync Req */
gint32 CFileTransfer_SvrImpl::StartDownload(
    const std::string& strFile /*[ In ]*/,
    HANDLE hChannel_h /*[ In ]*/,
    guint64 qwOffset /*[ In ]*/,
    guint64 qwSize /*[ In ]*/ )
{
    if( !IsFileNameValid( strFile ) )
        return -EINVAL;

    TransFileContext o( strFile );
    o.m_cDirection = 'd';
    o.m_strPath = m_strRootDir + "/" + strFile;
    o.m_iSize = qwSize;
    o.m_iOffset = qwOffset;
    o.m_bServer = true;
    gint32 ret = 0;
    do{
        ret = o.OpenFile();
        if( ret < 0 )
            break;

        ret = m_oTransCtx.AddContext(
            hChannel_h, o );
        if( ERROR( ret ) )
            break;
        ret = DEFER_CALL(
            GetIoMgr(), ObjPtr( this ),
            &CFileTransfer_SvrImpl::ReadFileAndSend,
            hChannel_h );

    }while( 0 );

    return ret;
}

/* Sync Req */
gint32 CFileTransfer_SvrImpl::GetFileInfo(
    const std::string& strFile /*[ In ]*/,
    FileInfo& fi /*[ Out ]*/ )
{
    if( !IsFileNameValid( strFile ) )
        return -EINVAL;

    gint32 ret = 0;
    do{
        stdstr strPath =
            m_strRootDir + "/" + strFile;
        fi.strFile = strFile;
        if( !IsFileExist( strPath ) )
        {
            ret = -ENOENT;
            break;
        }
        struct stat info;
        if( stat( strPath.c_str(), &info ) != 0 )
        {
            ret = -errno;
            break;
        }
        stdstr strAccess;
        guint32 dwFlags =
            ( info.st_mode & S_IRWXU );
        if( dwFlags & S_IRUSR )
            fi.strAccess += "r";
        else
            fi.strAccess += "-"; 

        if( dwFlags & S_IWUSR )
            fi.strAccess += "w";
        else
            fi.strAccess += "-"; 

        if( dwFlags & S_IXUSR )
            fi.strAccess += "x";
        else
            fi.strAccess += "-"; 
        fi.qwSize = info.st_size;
        break;

    }while( 0 );

    return ret;
}

/* Sync Req */
gint32 CFileTransfer_SvrImpl::RemoveFile(
    const std::string& strFile /*[ In ]*/ )
{
    if( !IsFileNameValid( strFile ) )
        return -EINVAL;

    gint32 ret = 0;
    do{
        stdstr strPath =
            m_strRootDir + "/" + strFile;
        if( !IsFileExist( strPath ) )
        {
            ret = -ENOENT;
            break;
        }
        ret = unlink( strPath.c_str() );
        if( ret < 0 )
            ret = -errno;

    }while( 0 );

    return ret;
}

gint32 CFileTransfer_SvrImpl::OnWriteStreamComplete(
    HANDLE hChannel, gint32 iRet,
    BufPtr& pBuf, IConfigDb* pCtx )
{
    if( hChannel == INVALID_HANDLE )
        return -EINVAL;
    return m_oTransCtx.OnWriteStreamComplete(
        hChannel, iRet, pBuf, pCtx );
}

gint32 CFileTransfer_SvrImpl::OnReadStreamComplete(
    HANDLE hChannel, gint32 iRet,
    BufPtr& pBuf, IConfigDb* pCtx )
{
    if( hChannel == INVALID_HANDLE )
        return -EINVAL;
    return m_oTransCtx.OnReadStreamComplete(
        hChannel, iRet, pBuf, pCtx );
}

gint32 CFileTransfer_SvrImpl::OnStreamReady(
    HANDLE hChannel )
{
    super::OnStreamReady( hChannel );
    BufPtr pBuf( true );
    stdstr strToken = "rdy";
    *pBuf = strToken;
    return DEFER_CALL( GetIoMgr(), ObjPtr( this ),
        &CFileTransfer_SvrImpl::SendToken,
        hChannel, pBuf );
} 

gint32 CFileTransfer_SvrImpl::OnStmClosing(
    HANDLE hChannel )
{
    m_oTransCtx.OnTransferDone(
        hChannel, -ECONNABORTED );
    // must be called to release the resources
    return super::OnStmClosing( hChannel );
}
