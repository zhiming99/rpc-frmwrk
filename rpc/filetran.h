/*
 * =====================================================================================
 *
 *       Filename:  filetran.h
 *
 *    Description:  file transfer interface
 *
 *        Version:  1.0
 *        Created:  03/22/2016 08:01:53 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 * =====================================================================================
 */

#include "configdb.h"
#include "defines.h"
#include "proxy.h"
#include <exception>

gint32 CopyFile( gint32 iFdDest,
    const std::string& strSrcFile );

class IFileTransfer
{
    public:

    virtual gint32 UploadFile(
        const std::string& strSrcFile ) = 0;

    virtual gint32 DownloadFile(
        const std::string& strSrcFile,
        const std::string& strDestFile ) = 0;
};

class IStream
{
    public:
    gint32 OpenStream( IConfigDb* pStmCfg, gint32 fd );
    gint32 CloseStream( gint32 fd );
    gint32 Read( gint32 fd, BufPtr& pBuf );
    gint32 Write( gint32 fd, const BufPtr& pBuf );
};

class CFileTransferProxy :
    public CInterfaceProxy,
    public IFileTransfer
{

    public:

    gint32 UploadFile_Proxy(
        const std::string& strSrcFile,
        IEventSink* pCallback = nullptr );

    gint32 DownloadFile_Async(
        const std::string& strSrcFile,
        const std::string& strDestFile );

    // note that the parameters for a callback
    // are limited to EnumTypeId as defined in
    // configdb.h. For complex object, please make
    // it serializable, an subclass of CObject or
    // a dbus message
    gint32 DownloadFile_Callback(
        gint32 iRet,
        gint32 fd,
        guint32 dwOffset,
        guint32 dwSize );

    gint32 DownloadFile_Proxy(
        const std::string& strSrcFile,
        const std::string& strDestFile,
        IEventSink* pCallback = nullptr );

    virtual gint32 UploadFile(
        const std::string& strSrcFile )
    {
        return UploadFile_Proxy( strSrcFile );
    }

    virtual gint32 DownloadFile(
        const std::string& strSrcFile,
        const std::string& strDestFile )
    {
        return DownloadFile_Proxy(
            strSrcFile, strDestFile );
    }
};

