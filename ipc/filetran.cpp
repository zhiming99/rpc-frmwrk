/*
 * =====================================================================================
 *
 *       Filename:  filetran.cpp
 *
 *    Description:  file transfer implementation
 *
 *        Version:  1.0
 *        Created:  03/22/2016 11:25:08 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2019 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#include <unistd.h>
#include "frmwrk.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "filetran.h"
#include <algorithm>

namespace rpcf
{

gint32 CFileTransferProxy::InitUserFuncs()
{
    // Note: no need to call super::InitUserFuncs for
    // interface proxy
    //
    // And define an empty map, since the method is not
    // declared following the calling rules. but we
    // have this interface registered.
    BEGIN_IFPROXY_MAP( CFileTransferServer, false );
    END_IFPROXY_MAP;
    return 0;
}

gint32 CFileTransferProxy::OnFileUploaded(
    IEventSink* pTask, gint32 fd ) 
{
    if( fd < 0 )
        return 0;

    return 0;
}

gint32 CFileTransferProxy::UploadFile_Callback(
    IEventSink* pTask, gint32 iRet ) 
{

    if( pTask == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    IEventSink* pCallback = nullptr;
    CCfgOpenerObj oTaskCfg( pTask );
    gint32 iFd = -1;

    do{
        // NOTE: must get pCallback first to make sure
        // whether or not the req succeeded or not, the
        // callback will be called.
        ret = oTaskCfg.GetPointer(
            propEventSink, pCallback );

        if( ERROR( iRet ) )
        {
            OnFileUploaded( pTask, iFd );
            break;
        }

        ret = oTaskCfg.GetIntProp(
            propFd, ( guint32& )iFd );

        if( ERROR( ret ) )
            break;

        OnFileUploaded( pTask, iFd );

        if( iFd >= 0 )
            close( iFd );

    }while( 0 );

    if( pCallback != nullptr )
    {
        void* pData = ( CObjBase* )pTask;
        pCallback->OnEvent( eventTaskComp,
            iRet, 0, ( LONGWORD* )pData );
    }

    oTaskCfg.RemoveProperty( propObjPtr );
    oTaskCfg.RemoveProperty( propEventSink );

    return ret;
}

gint32 CFileTransferProxy::UploadFile_Async(
    const std::string& strSrcFile,
    IEventSink* pCallback )
{
    gint32 ret = 0;
    gint32 fd = -1;

    if( strSrcFile.empty() )
        return -EINVAL;

    do{
        fd = open( strSrcFile.c_str(), O_RDONLY );
        if( fd <= 0 )
        {
            ret = -errno;
            break;
        }

        off_t iSize = lseek( fd, 0, SEEK_END );
        lseek( fd, 0, SEEK_SET );
        if( iSize < 0 )
        {
            ret = -errno;
            break;
        }

        CParamList oDescCfg;
        oDescCfg[ propFilePath ] = strSrcFile;
        if( ERROR( ret ) )
            break;

        oDescCfg[ propIid ] =
            iid( CFileTransferServer );

        TaskletPtr pAsync;
        ret = BIND_CALLBACK( pAsync,
            &CFileTransferProxy::UploadFile_Callback );

        if( ERROR( ret ) )
            break;

        CCfgOpenerObj oTaskCfg(
            ( CObjBase* )pAsync );

        // set the desc ptr for DownloadFile_Callback
        oTaskCfg.SetObjPtr( propObjPtr,
            ObjPtr( oDescCfg.GetCfg() ) );

        oTaskCfg.SetIntProp( propFd, fd );

        if( pCallback != nullptr )
        {
            oTaskCfg.SetObjPtr( propEventSink,
                ObjPtr( pCallback ) );
        }

        ret = SendData( oDescCfg.GetCfg(),
            fd, 0, iSize, pAsync );

    }while( 0 );

    if( ret != STATUS_PENDING )
    {
        if( fd >= 0 )
        {
            close( fd );
            fd = -1;
        }
    }

    return ret;
}

gint32 CFileTransferProxy::UploadFile_Proxy(
    const std::string& strSrcFile,
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        if( pCallback != nullptr )
        {
            ret = UploadFile_Async(
                strSrcFile, pCallback );

            if( ret == STATUS_PENDING )
                break;
        }
        else
        {
            TaskletPtr pTask;
            ret = pTask.NewObj( clsid( CSyncCallback ) );
            if( ERROR( ret ) )
                break;

            ret = UploadFile_Async(
                strSrcFile, pTask );

            if( ret == STATUS_PENDING )
            {
                CSyncCallback* pSync = pTask;
                if( pSync == nullptr )
                {
                    ret = -EFAULT;
                    break;
                }
                ret = pSync->WaitForComplete();
                if( SUCCEEDED( ret ) )
                    ret = pSync->GetError();
            }
        }

    }while( 0 );

    return ret;
}

gint32 CFileTransferProxy::DownloadFile_Callback(
    IEventSink* pTask,
    gint32 iRet,
    ObjPtr& pDataDesc,
    gint32 fd, 
    guint32 dwOffset,
    guint32 dwSize )
{
    return OnFileDownloaded( pTask, iRet,
        pDataDesc, fd, dwOffset, dwSize );
}

// note that fd can be 0 if ERROR( iRet ).
gint32 CFileTransferProxy::OnFileDownloaded(
    IEventSink* pTask,
    gint32 iRet,
    ObjPtr& pDataDesc,
    gint32 fd, 
    guint32 dwOffset,
    guint32 dwSize )
{

    if( pTask == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    IEventSink* pCallback = nullptr;
    CCfgOpenerObj oTaskCfg( pTask );

    do{
        // must go first to get the callback pointer
        oTaskCfg.GetPointer(
            propEventSink, pCallback );

        if( ERROR( iRet ) )
            break;

        if( fd < 0 ||
            dwOffset + dwSize > MAX_BYTES_PER_FILE )
        {
            ret = -EINVAL;
            break;
        }

        // copy to the dest file.
        off_t iOffset = lseek(
            fd, dwOffset, SEEK_SET );

        if( iOffset == -1 )
        {
            ret = -errno;
            break;
        }

        IConfigDb* pCfg = nullptr;
        ret = oTaskCfg.GetPointer(
            propObjPtr, pCfg );

        if( ERROR( ret ) )
            break;

        std::string strDestFile;
        ret = oTaskCfg.GetStrProp(
            propFilePath1, strDestFile );

        if( ERROR( ret ) )
            break;

        ret = CopyFile( fd, strDestFile );

    }while( 0 );

    if( pCallback != nullptr )
    {
        void* pData = ( CObjBase* )pTask;
        pCallback->OnEvent( eventTaskComp,
            iRet, 0, ( LONGWORD* )pData );
    }

    if( fd >= 0 )
        close( fd );

    oTaskCfg.RemoveProperty( propObjPtr );
    oTaskCfg.RemoveProperty( propEventSink );

    return 0;
}

gint32 CFileTransferProxy::DownloadFile_Async(
    const std::string& strSrcFile,
    const std::string& strDestFile,
    IEventSink* pCallback )
{
    gint32 ret = 0;
    gint32 fd = -1;

    if( strSrcFile.empty() ||
        strDestFile.empty() )
        return -EINVAL;

    do{
        CParamList oDescCfg;

        oDescCfg[ propFilePath ] = strSrcFile;

        oDescCfg[ propIid ] =
            iid( CFileTransferServer );

        guint32 dwOffset = 0;
        // dwSize set to zero to read the whole file
        guint32 dwSize = 0;

        if( ERROR( ret ) )
            break;

        TaskletPtr pAsync;
        ret = BIND_CALLBACK( pAsync,
            &CFileTransferProxy::DownloadFile_Callback );

        if( ERROR( ret ) )
            break;

        CCfgOpenerObj oTaskCfg(
            ( CObjBase* )pAsync );

        // set the desc ptr for DownloadFile_Callback
        oTaskCfg.SetObjPtr( propObjPtr,
            ObjPtr( oDescCfg.GetCfg() ) );

        oTaskCfg.SetStrProp(
            propFilePath1, strDestFile );

        if( pCallback != nullptr )
        {
            oTaskCfg.SetObjPtr( propEventSink,
                ObjPtr( pCallback ) );
        }

        ret = FetchData( oDescCfg.GetCfg(),
            fd, dwOffset, dwSize, pAsync );

        if( ERROR( ret ) )
            break;

        if( ret == STATUS_PENDING )
            break;

        // copy to the dest file.
        off_t iOffset = lseek(
            fd, dwOffset, SEEK_SET );

        if( iOffset == -1 )
        {
            ret = -errno;
            break;
        }
        
        ret = CopyFile( fd, strDestFile );
            
    }while( 0 );

    if( fd >= 0 )
        close( fd );

    return ret;
}

gint32 CFileTransferProxy::DownloadFile_Proxy(
    const std::string& strSrcFile,
    const std::string& strDestFile,
    IEventSink* pCallback )
{
    gint32 ret = 0;

    do{
        if( pCallback )
        {
            ret = DownloadFile_Async( strSrcFile,
                strDestFile, pCallback );

            if( ret == STATUS_PENDING )
                break;
        }
        else
        {
            TaskletPtr pTask;
            ret = pTask.NewObj(
                clsid( CSyncCallback ) );

            if( ERROR( ret ) )
                break;

            ret = DownloadFile_Async( strSrcFile,
                strDestFile, pTask );

            if( ERROR( ret ) )
                break;

            if( ret == STATUS_PENDING )
            {
                CSyncCallback* pSync = pTask;
                if( pSync == nullptr )
                {
                    ret = -EFAULT;
                    break;
                }
                ret = pSync->WaitForComplete();
                if( SUCCEEDED( ret ) )
                    ret = pSync->GetError();
            }
        }

    }while( 0 );

    return ret;
}

gint32 CopyFile( gint32 iFdSrc, gint32 iFdDest, ssize_t& iSize )
{
    if( iFdSrc < 0 || iFdDest < 0 )
        return -EINVAL;

    gint32 ret = 0;
    ssize_t iBytes = 0;
    ssize_t iTransferred = 0;

    BufPtr pBuf(true);
    if( pBuf->size() == 0 )
        pBuf->Resize( 16 * 1024 );

    if( iSize == 0 )
        iSize = MAX_BYTES_PER_FILE;

    do{
        iBytes = read( iFdSrc, pBuf->ptr(), pBuf->size() );

        if( iBytes == 0 )
            break;

        if( iBytes == -1 )
        {
            ret = -errno;
            break;
        }

        iBytes = write( iFdDest, pBuf->ptr(), iBytes );
        if( iBytes == -1 )
        {
            ret = -errno;
            break;
        }

        iTransferred += iBytes;
        iSize -= iBytes;

        if( iSize <= 0 )
            break;

    }while( iBytes == ( ssize_t )pBuf->size() );

    if( SUCCEEDED( ret ) )
        iSize = iTransferred;
    else if( ERROR( ret ) )
        iSize = 0;

    return ret;
}

gint32 CopyFile( gint32 iFdSrc,
    const std::string& strDestFile )
{
    if( iFdSrc < 0 || strDestFile.empty() )
        return -EINVAL;

    gint32 ret = 0;

    gint32 iFdDest = open( strDestFile.c_str(),
        O_CREAT | O_WRONLY );

    if( iFdDest == -1 )
        return -errno;

    ssize_t iBytes = 0;
    ret = CopyFile( iFdSrc, iFdDest, iBytes );

    // rollback
    if( ERROR( ret ) )
        unlink( strDestFile.c_str() );

    close( iFdDest );
    return ret;
}

gint32 CopyFile( const std::string& strSrcFile,
    gint32 iFdDest )
{
    if( iFdDest < 0 || strSrcFile.empty() )
        return -EINVAL;

    gint32 iFdSrc = open(
        strSrcFile.c_str(), O_RDONLY );

    if( iFdSrc == -1 )
        return -errno;

    gint32 ret = 0;
    do{
        
        off_t iOffset =
            lseek( iFdDest, 0, SEEK_CUR );

        if( iOffset == -1 )
        {
            ret = -errno;
            break;
        }

        ssize_t iBytes = 0;
        ret = CopyFile( iFdSrc, iFdDest, iBytes );

        if( ERROR( ret ) )
        {
            // rollback
            iBytes = lseek( iFdDest, iOffset, SEEK_SET );
            if( iBytes != iOffset )
            {
                ret = -errno;
                break;
            }
            ftruncate( iFdDest, iOffset );
        }

    }while( 0 );

    close( iFdSrc );
    return ret;
}

gint32 CFileTransferServer::InitUserFuncs()
{
    BEGIN_IFHANDLER_MAP( CFileTransferServer );
    END_IFHANDLER_MAP;

    return 0;
}

gint32 CFileTransferServer::SendData_Server(
    IConfigDb* pDataDesc,           // [in]
    gint32 fd,                      // [in]
    guint32 dwOffset,               // [in]
    guint32 dwSize,                 // [in]
    IEventSink* pCallback )
{
    if( pDataDesc == nullptr ||
        dwOffset > MAX_BYTES_PER_FILE ||
        dwSize > MAX_BYTES_PER_FILE ||
        pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    bool bClose = false;
    do{
        if( fd < 0 )
        {
            BufPtr pBuf( true );
            std::string strSrcFile;
            CCfgOpener oCfg( pDataDesc );

            // FIXME: security loophole, no limit on
            // where to store the files
            ret = oCfg.GetStrProp(
                propFilePath, strSrcFile );

            if( ERROR( ret ) )
                break;

            pBuf->Resize( strSrcFile.size() + 16 );
            *pBuf = strSrcFile;

            // remove the dirname
            std::string strFileName = basename( pBuf->ptr() );
            fd = open( strFileName.c_str(), O_RDONLY );
            if( fd < 0 )
            {
                ret = -errno;
                break;
            }
            bClose = true;
        }
        else
        {
            bClose = true;
            ret = HandleIncomingData( fd );
        }
        // what to do with this file?

    }while( 0 );

    if( bClose )
        close( fd );

    return ret;
}

gint32 CFileTransferServer::HandleIncomingData( int fd )
{
    if( fd < 0 )
        return -EINVAL;
    do{
        char szDestFile[] = "tmpXXXXXX";
        gint32 ret = mkstemp( szDestFile );
        if( ret == -1 )
        {
            ret = -errno;
            break;
        }

        int fdDest = ret;

        ssize_t iSize = 0;
        ret = CopyFile( fd, fdDest, iSize );
        if( ERROR( ret ) )
        {
            close( fdDest );
            unlink( szDestFile );
            break;
        }

        char szCmd[ 32 ];
        snprintf( szCmd, sizeof( szCmd ),
            "xxd %s", szDestFile );
        //system( szCmd );
        close( fdDest );
        unlink( szDestFile );

    }while( 0 );

    return 0;
}

gint32 CFileTransferServer::FetchData_Server(
    IConfigDb* pDataDesc,           // [in]
    gint32& fd,                     // [out]
    guint32& dwOffset,              // [in, out]
    guint32& dwSize,                // [in, out]
    IEventSink* pCallback )
{
    if( pDataDesc == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    gint32 fdDest = -1;
    gint32 fdSrc = -1;
    char szDestFile[] = "tmpXXXXXX";

    do{
        std::string strSrcFile;
        CCfgOpener oCfg( pDataDesc );

        ret = oCfg.GetStrProp(
            propFilePath, strSrcFile );

        if( ERROR( ret ) )
            break;

        ret = mkstemp( szDestFile );
        if( ret == -1 )
        {
            ret = -errno;
            break;
        }

        fdDest = ret;
        if( fdDest == -1 )
        {
            ret = -errno;
            break;
        }

        // FIXME: Security issue here, no limit for
        // external access to all the files
        fdSrc = open( strSrcFile.c_str(), O_RDONLY );
        if( fdSrc == -1 )
        {
            ret = -errno;
            break;
        }

        ssize_t iSize = lseek( fdSrc, 0, SEEK_END );
        if( iSize == -1 )
        {
            ret = -errno;
            break;
        }

        if( iSize <= ( ssize_t )dwOffset )
        {
            ret = -EINVAL;
            break;
        }

        if( dwSize == 0 )
            dwSize = iSize - dwOffset;
        else if ( dwSize > iSize - dwOffset )
            dwSize = iSize - dwOffset;

        if( dwSize > MAX_BYTES_PER_FILE )
        {
            ret = -EINVAL;
            break;
        }

        if( lseek( fdSrc, dwOffset, SEEK_SET ) == -1 )
        {
            ret = -errno;
            break;
        }

        // FIXME: size is not the only factor for
        // transfer, and the RTT should also be
        // considered if over the internet
        if( dwSize <= MAX_BYTES_PER_TRANSFER )
        {
            ssize_t iSize = dwSize;
            ret = CopyFile( fdSrc, fdDest, iSize );
            if( ERROR( ret ) )
                break;

            dwSize = iSize;
            fd = fdDest;
        }
        else
        {
            // using a standalone thread for large data
            // transfer
            CParamList oParams;

            oParams.Push( ObjPtr( pDataDesc ) );
            oParams.Push( dwOffset );
            oParams.Push( dwSize );
            oParams.Push( fdSrc );
            oParams.Push( fdDest );

            oParams.SetObjPtr(
                propIfPtr, ObjPtr( this ) );

            oParams.SetObjPtr(
                propEventSink,
                ObjPtr( pCallback ) );

            // scheduled a dedicate thread for this
            // task
            ret = GetIoMgr()->ScheduleTask(
                clsid( CIfFetchDataTask ), 
                oParams.GetCfg(), true );

            if( SUCCEEDED( ret ) )
                ret = STATUS_PENDING;
        }
        
    }while( 0 );

    if( fdDest >= 0 && ERROR( ret ) )
        close( fdDest );

    // delete on close
    unlink( szDestFile );

    if( fdSrc >= 0 &&
        ret != STATUS_PENDING )
        close( fdSrc );

    return ret;
}

}
