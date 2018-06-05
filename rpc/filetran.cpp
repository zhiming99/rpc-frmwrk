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
 * =====================================================================================
 */
#include <unistd.h>
#include "frmwrk.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "filetran.h"

using namespace std;

gint32 CFileTransferProxy::UploadFile_Proxy(
    const std::string& strSrcFile,
    IEventSink* pCallback )
{
    gint32 ret = 0;
    CfgPtr pCfg( true );
    CCfgOpener oCfg( ( IConfigDb* )pCfg );
    oCfg.SetStrProp( propFilePath, strSrcFile );
    gint32 fd = 0;

    do{
        fd = open( strSrcFile.c_str(), O_RDONLY );
        if( fd <= 0 )
        {
            ret = -errno;
            break;
        }
        off_t iSize = lseek( fd, 0, SEEK_END );
        lseek( fd, 0, SEEK_SET );
        if( iSize == -1 )
        {
            ret = -errno;
            break;
        }
        if( pCallback != nullptr )
        {
            ret = SendData(
                pCfg, fd, 0, iSize, pCallback );
        }
        else
        {
            TaskletPtr pTask;
            ret = pTask.NewObj( clsid( CSyncCallback ) );
            if( ERROR( ret ) )
                break;

            ret = SendData(
                pCfg, fd, 0, iSize, pTask );

            if( ERROR( ret ) )
                break;

            ( ( CSyncCallback* )pTask )->WaitForComplete();
        }

    }while( 0 );

    if( fd > 0 )
    {
        close( fd );
        fd = 0;
    }
    return ret;
}

gint32 CFileTransferProxy::DownloadFile_Callback(
    gint32 iRet,
    gint32 fd,
    guint32 dwOffset,
    guint32 dwSize )
{
    // this is an example async callback function
    //
    // please make sure the parameters are
    // consistent with the response the server
    // side will return.
    return 0;
}

gint32 CFileTransferProxy::DownloadFile_Async(
    const std::string& strSrcFile,
    const std::string& strDestFile )
{

    gint32 ret = 0;
    gint32 fd = 0;

    do{
        CfgPtr pCfg( true );
        CCfgOpener oCfg( ( IConfigDb* )pCfg );

        ret = oCfg.SetStrProp(
            propFilePath, strSrcFile );

        if( ERROR( ret ) )
            break;

        guint32 dwOffset = 0;
        guint32 dwSize = 0;

        if( ERROR( ret ) )
            break;

        TaskletPtr pAsync;
        ret = BIND_CALLBACK( pAsync,
            &CFileTransferProxy::DownloadFile_Callback );

        if( ERROR( ret ) )
            break;

        ret = FetchData(
            pCfg, fd, dwOffset, dwSize, pAsync );

        if( ERROR( ret ) )
            break;

        // copy to the dest file.
        //
        // because we are not sure whether or not
        // the two files are on the same
        // filesystem, let's make a copy rather
        // than make a hard link.
        //
        off_t iOffset = lseek( fd, dwOffset, SEEK_SET );
        if( iOffset == -1 )
        {
            ret = -errno;
            break;
        }
        
        ret = CopyFile( fd, strDestFile );
            
    }while( 0 );

    return ret;
}

gint32 CFileTransferProxy::DownloadFile_Proxy(
    const std::string& strSrcFile,
    const std::string& strDestFile,
    IEventSink* pCallback )
{

    gint32 ret = 0;
    gint32 fd = 0;

    do{
        CfgPtr pCfg( true );
        CCfgOpener oCfg( ( IConfigDb* )pCfg );

        ret = oCfg.SetStrProp(
            propFilePath, strSrcFile );

        if( ERROR( ret ) )
            break;

        guint32 dwOffset = 0;
        guint32 dwSize = 0;
        if( ERROR( ret ) )
            break;

        if( pCallback )
        {
            ret = FetchData( pCfg,
                fd, dwOffset, dwSize, pCallback );

            if( ret == STATUS_PENDING )
                break;
        }
        else
        {
            TaskletPtr pTask;
            ret = pTask.NewObj( clsid( CSyncCallback ) );
            if( ERROR( ret ) )
                break;

            ret = FetchData(
                pCfg, fd, dwOffset, dwSize, pTask );

            if( ERROR( ret ) )
                break;

            ( ( CSyncCallback* )pTask )->WaitForComplete();
        }

        if( ERROR( ret ) )
            break;

        // copy to the dest file.
        //
        // because we are not sure whether or not
        // the two files are on the same
        // filesystem, let's make a copy rather
        // than make a hard link.
        //
        off_t iOffset = lseek( fd, dwOffset, SEEK_SET );
        if( iOffset == -1 )
        {
            ret = -errno;
            break;
        }
        
        ret = CopyFile( fd, strDestFile );
            
    }while( 0 );

    return ret;
}
