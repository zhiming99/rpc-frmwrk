/*
 * =====================================================================================
 *
 *       Filename:  filetran.h
 *
 *    Description:  declaration of interface for file/blob transfer 
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

#pragma once

#include "configdb.h"
#include "defines.h"
#include "ifhelper.h"
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

class CFileTransferProxy :
    public virtual CInterfaceProxy,
    public IFileTransfer
{
    protected:

    gint32 UploadFile_Callback(
        IEventSink* pContext,
        gint32 iRet );

    gint32 UploadFile_Async(
        const std::string& strSrcFile,
        IEventSink* pCallback );

    gint32 DownloadFile_Async(
        const std::string& strSrcFile,
        const std::string& strDestFile,
        IEventSink* pCallback = nullptr );

    // note that the parameters for a callback should
    // be exactly the same as the response of the
    // request, except the first mandatory IEventSink* 
    gint32 DownloadFile_Callback(
        IEventSink* pContext,
        gint32 iRet,
        ObjPtr& pDataDesc,
        gint32 fd,
        guint32 dwOffset,
        guint32 dwSize );

    public:

    virtual gint32 InitUserFuncs();
    typedef CInterfaceProxy super;

    CFileTransferProxy( const IConfigDb* pCfg )
        :super( pCfg )
    {}

    gint32 UploadFile_Proxy(
        const std::string& strSrcFile,
        IEventSink* pCallback = nullptr );


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

    // override this method for your own business
    // logics
    virtual gint32 OnFileDownloaded(
        IEventSink* pContext,
        gint32 iRet,
        ObjPtr& pDataDesc,
        gint32 fd,
        guint32 dwOffset,
        guint32 dwSize );

    virtual gint32 OnFileUploaded(
        IEventSink* pContext,
        gint32 iFd );
};

class CFileTransferServer :
    public virtual CInterfaceServer
{
    protected:

    gint32 SendData_Server(
        IConfigDb* pDataDesc,           // [in]
        gint32 fd,                      // [in]
        guint32 dwOffset,               // [in]
        guint32 dwSize,                 // [in]
        IEventSink* pCallback );

    gint32 FetchData_Server(
        IConfigDb* pDataDesc,           // [in]
        gint32& fd,                     // [out]
        guint32& dwOffset,              // [in, out]
        guint32& dwSize,                // [in, out]
        IEventSink* pCallback );

    public:

    typedef CInterfaceServer super;
    CFileTransferServer( const IConfigDb* pCfg )
        :super( pCfg )
    {}
    virtual gint32 InitUserFuncs();

    // override this method for your own purpose
    virtual gint32 HandleIncomingData( gint32 fd );

    virtual gint32 HandleStreaming(
        IConfigDb* pDataDesc,           // [in]
        gint32 fd,                      // [in]
        guint32 dwOffset,               // [in]
        guint32 dwSize,                 // [in]
        IEventSink* pCallback )
    { return -ENOTSUP; }
};
