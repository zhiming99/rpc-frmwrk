/*
 * =====================================================================================
 *
 *       Filename:  regfs.cpp
 *
 *    Description:  implementation of registry fs related classes. 
 *
 *        Version:  1.0
 *        Created:  09/18/2024 09:20:11 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi(woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2024 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */

#include "blkalloc.h"
#include <fcntl.h>

#define VALIDATE_NAME( _strName_ ) \
    ( _strName_.size() <= REGFS_NAME_LENGTH - 1 )

namespace rpcf
{

CRegistryFs::CRegistryFs(
    const IConfigDb* pCfg )
{
    gint32 ret = 0;
    do{
        SetClassId( clsid( CRegistryFs ) );
        ret = m_pAlloc.NewObj(
            clsid( CBlockAllocator ), pCfg );
        if( ERROR( ret ) )
            break;

        Variant oVar;
        ret = pCfg->GetProperty( 0, oVar );
        if( SUCCEEDED( ret ) )
            m_bFormat = oVar;
        ret = 0;

    }while( 0 );
    if( ERROR( ret ) )
    {
        stdstr strMsg = DebugMsg( ret,
            "Error in CRegistryFs ctor" );
        throw std::runtime_error( strMsg );
    }
}

CRegistryFs::~CRegistryFs()
{
}

gint32 CRegistryFs::CreateRootDir()
{
    // can only be called within the call of Format.
    gint32 ret = 0;
    do{
        guint32 dwBlkIdx = 0;
        gint32 ret = m_pAlloc->AllocBlocks(
            &dwBlkIdx, 1 );
        if( ERROR( ret ) )
            break;
        if( dwBlkIdx != ROOT_INODE_BLKIDX )
        {
            ret = ERROR_STATE;
            break;
        }
        ret = CFileImage::Create( ftDirectory,
            m_pRootImg, m_pAlloc,
            ROOT_INODE_BLKIDX, 0,
            ObjPtr() );
        ret = m_pRootImg->Format();
        if( ERROR( ret ) )
            break;
        // ret = m_pRootImg->Flush();
        ret = COpenFileEntry::Create( 
            ftDirectory, m_pRootDir,
            m_pRootImg, m_pAlloc,
            stdstr( "/" ) );
        if( ERROR( ret ) )
            break;

    }while( 0 );
    return ret;
}

gint32 CRegistryFs::OpenRootDir()
{
    gint32 ret = 0;
    do{
        ret = CFileImage::Create( ftDirectory,
            m_pRootImg, m_pAlloc,
            ROOT_INODE_BLKIDX, 0,
            ObjPtr() );
        if( ERROR( ret ) )
            break;
        ret = m_pRootImg->Reload();
        if( ERROR( ret ) )
            break;

        ret = COpenFileEntry::Create( 
            ftDirectory, m_pRootDir,
            m_pRootImg, m_pAlloc,
            stdstr( "/" ) );
        if( ERROR( ret ) )
            break;

        ret = m_pRootDir->Open(
            O_RDONLY, nullptr );

        RFHANDLE hFile = m_pRootDir->GetObjId();
        m_mapOpenFiles[ hFile ] = m_pRootDir;

    }while( 0 );
    return ret;
}

gint32 CRegistryFs::Format()
{
    gint32 ret = 0;
    do{
        ret = m_pAlloc->Format();
        if( ERROR( ret ) )
            break;
        ret = CreateRootDir();
    }while( 0 );
    return ret;
}

gint32 CRegistryFs::Reload()
{
    gint32 ret = 0;
    do{
        ret = m_pAlloc->Reload();
        if( ERROR( ret ) )
        {
            OutputMsg( ret, "Error, Loading "
                "Registry filesystem" );
            break;
        }
        ret = OpenRootDir();
    }while( 0 );
    return ret;
}

gint32 CRegistryFs::Flush( guint32 dwFlags )
{
    gint32 ret = 0;
    do{
        ret = m_pRootImg->Flush( dwFlags );
        if( ERROR( ret ) )
            break;
        ret = m_pAlloc->Flush( dwFlags );
        if( ERROR( ret ) )
            break;
    }while( 0 );
    return ret;
}    

gint32 CRegistryFs::Start()
{
    gint32 ret = 0;
    do{
        WRITE_LOCK( this );
        if( unlikely( m_bFormat ) )
            ret = Format();
        else
            ret = Reload();
        if( ERROR( ret ) )
            break;
        SetState( stateStarted );
    }while( 0 );
    return ret;
}

gint32 CRegistryFs::Stop()
{
    gint32 ret = 0;
    do{
        WRITE_LOCK( this );
        if( m_mapOpenFiles.size() )
            DebugPrint( m_mapOpenFiles.size(),
                "there are some open files to close" );
        for( auto& elem : m_mapOpenFiles )
        {
            FileSPtr pOpen = elem.second;
            pOpen->Close();
        }
        m_pRootDir->Close();
        m_mapOpenFiles.clear();
        ret = Flush( FLAG_FLUSH_CHILD |
            FLAG_FLUSH_DEFAULT );
        if( ERROR( ret ) )
            break;
        SetState( stateStopped );
    }while( 0 );
    return ret;
}

gint32 CRegistryFs::Namei(
    const stdstr& strPath,
    std::vector<stdstr>& vecNames )
{
    gint32 ret = 0;
    do{
        ret = CRegistry::Namei(
            strPath, vecNames );

        if( ERROR( ret ) )
            break;
        for( auto& elem : vecNames )
        {
            if( !VALIDATE_NAME( elem ) )
            {
                ret = -EINVAL;
                DebugPrint( ret, "Error, "
                    "path name '%s' too long",
                    elem.c_str() );
                break;
            }
        }
        
    }while( 0 );
    return ret;
}

gint32 CRegistryFs::GetParentDir(
    const stdstr& strPath, FImgSPtr& pDir,
    CAccessContext* pac )
{
    gint32 ret = 0;
    do{
        std::vector< stdstr > vecNames;
        ret = Namei( strPath, vecNames );
        if( ERROR( ret ) )
            break;
        if( vecNames[ 0 ] != "/" )
        {
            ret = -EINVAL;
            DebugPrint( ret, "Error, relative "
                "path '%s' not allowed",
                strPath.c_str() );
            break;
        }
        gint32 idx = 1;
        CDirImage* pCurDir = m_pRootImg;
        std::vector< FImgSPtr > vecDirs;
        vecDirs.push_back( m_pRootImg );
        FImgSPtr curDir = m_pRootImg;
        for( ; idx < vecNames.size() - 1; idx++ )
        {
            stdstr& strCurDir = vecNames[ idx ];
            if( strCurDir == "." )
                continue;
            if( strCurDir == ".." )
            {
                if( vecDirs.empty() )
                {
                    ret = -ENOTDIR;
                    DebugPrint( ret, "Error, "
                        "invalid path '%s'",
                        strPath.c_str() );
                    break;
                }
                curDir = vecDirs.back();
                pCurDir = curDir;
                vecDirs.pop_back();
                continue;
            }
            if( pac != nullptr )
            {
                READ_LOCK( pCurDir );
                ret = pCurDir->CheckAccess(
                    X_OK, pac );
                if( ERROR( ret ) )
                    break;
            }
            FImgSPtr pFile;
            ret = pCurDir->Search(
                strCurDir.c_str(), pFile );
            if( ERROR( ret ) )
                break;
            if( pCurDir == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            vecDirs.push_back( curDir );
            pCurDir = pFile;
            curDir = pFile;
        }

        vecDirs.clear();
        if( ERROR( ret ) )
            break;
        pDir = curDir;
    }while( 0 );
    return ret;
}

gint32 CRegistryFs::CreateFile(
    const stdstr& strPath,
    mode_t dwMode, guint32 dwFlags,
    RFHANDLE& hFile,
    CAccessContext* pac )
{
    gint32 ret = 0;
    do{
        READ_LOCK( this );
        FImgSPtr dirPtr;
        ret = GetParentDir(
            strPath, dirPtr, pac );
        if( ERROR( ret ) )
            break;
        CDirImage* pDir = dirPtr;
        std::string strFile =
            basename( strPath.c_str() );
        FImgSPtr pFile;
        ret = pDir->CreateFile(
            strFile.c_str(), dwMode, pFile );
        if( ERROR( ret ) )
            break;

        FileSPtr pOpenFile;
        ret = COpenFileEntry::Create( 
            ftRegular, pOpenFile,
            pFile, m_pAlloc, strPath );
        if( ERROR( ret ) )
            break;

        CAccessContext oac;
        if( pac == nullptr )
        {
            // to check conflict between dwMode and dwFlags
            oac.dwUid = pFile->GetUid();
            oac.dwGid = pFile->GetGid();
            pac = &oac;
        }

        ret = pOpenFile->Open( dwFlags, pac );
        if( ERROR( ret ) )
            break;

        CStdRMutex oLock( this->GetExclLock() );
        pOpenFile->SetFlags( dwFlags );
        hFile = pOpenFile->GetObjId();
        m_mapOpenFiles[ hFile ] = pOpenFile;

    }while( 0 );
    if( ERROR( ret ) )
    {
        DebugPrint( ret, "Error create file '%s'",
            strPath.c_str() ); 
    }
    return ret;
}

gint32 CRegistryFs::OpenFile(
    const stdstr& strPath,
    guint32 dwFlags,
    RFHANDLE& hFile,
    CAccessContext* pac )
{
    gint32 ret = 0;
    do{
        READ_LOCK( this );
        FImgSPtr dirPtr;
        ret = GetParentDir(
            strPath, dirPtr, pac );
        if( ERROR( ret ) )
            break;

        std::string strFile =
            basename( strPath.c_str() );

        CDirImage* pDir = dirPtr;
        FImgSPtr pFile;
        ret = pDir->Search(
            strFile.c_str(), pFile );
        if( ERROR( ret ) )
            break;

        FileSPtr pOpenFile;
        ret = COpenFileEntry::Create( 
            ftRegular, pOpenFile,
            pFile, m_pAlloc, strPath );
        if( ERROR( ret ) )
            break;

        ret = pOpenFile->Open( dwFlags, pac );
        if( ERROR( ret ) )
            break;

        if( dwFlags & O_TRUNC )
            pFile->Truncate( 0 );

        CStdRMutex oLock( this->GetExclLock() );
        pOpenFile->SetFlags( dwFlags );
        hFile = pOpenFile->GetObjId();
        m_mapOpenFiles[ hFile ] = pOpenFile;
        
    }while( 0 );
    if( ERROR( ret ) )
    {
        DebugPrint( ret, "Error create file '%s'",
            strPath.c_str() ); 
    }
    return ret;
}

gint32 CRegistryFs::CloseFileNoLock(
    RFHANDLE hFile )
{
    if( hFile == INVALID_HANDLE )
        return -EINVAL;
    gint32 ret = 0;
    do{
        CStdRMutex oLock( this->GetExclLock() );
        auto itr = m_mapOpenFiles.find( hFile );
        if( itr == m_mapOpenFiles.end() )
        {
            ret = -ENOENT;
            break;
        }
        FileSPtr pFile = itr->second;
        m_mapOpenFiles.erase( itr );
        oLock.Unlock();
        ret = pFile->Close();
        if( ret != STATUS_MORE_PROCESS_NEEDED )
            break;

        ret = 0;
        const stdstr& strPath = pFile->GetPath();
        FImgSPtr dirPtr;
        FImgSPtr pImg = pFile->GetImage();
        CDirImage* pDir = pImg->GetParentDir();
        if( pDir == nullptr )
        {
            ret = GetParentDir( strPath, dirPtr );
            if( ERROR( ret ) )
                break;
            pDir = dirPtr;
        }

        std::string strFile =
            basename( strPath.c_str() );
        pDir->UnloadFile( strFile.c_str() );

    }while( 0 );
    return ret;
}

gint32 CRegistryFs::CloseFile(
    RFHANDLE hFile )
{
    gint32 ret = 0;
    do{
        READ_LOCK( this );
        ret = CloseFileNoLock( hFile );
    }while( 0 );
    return ret;
}

gint32 CRegistryFs::RemoveFile(
    const stdstr& strPath,
    CAccessContext* pac )
{
    gint32 ret = 0;
    do{
        READ_LOCK( this );
        FImgSPtr dirPtr;
        ret = GetParentDir(
            strPath, dirPtr, pac );
        if( ERROR( ret ) )
            break;

        std::string strFile =
            basename( strPath.c_str() );

        CDirImage* pDir = dirPtr;
        ret = pDir->RemoveFile(
            strFile.c_str() );
        
    }while( 0 );
    if( ERROR( ret ) )
    {
        DebugPrint( ret,
            "Error removing file '%s'",
            strPath.c_str() ); 
    }
    return ret;
}

gint32 CRegistryFs::ReadFile( RFHANDLE hFile,
    char* buffer, guint32& dwSize, guint32 dwOff )
{
    gint32 ret = 0;
    do{
        READ_LOCK( this );
        CStdRMutex oLock( GetExclLock() );
        FileSPtr pFile;
        auto itr = m_mapOpenFiles.find( hFile );
        if( itr == m_mapOpenFiles.end() )
        {
            ret = -ENOENT;
            break;
        }
        pFile = itr->second;
        oLock.Unlock();
        ret = pFile->ReadFile(
            dwSize, ( guint8* )buffer, dwOff );
        if( ERROR( ret ) )
            break;
        ret = dwSize;
    }while( 0 );
    return 0;
}

gint32 CRegistryFs::WriteFile( RFHANDLE hFile,
    const char* buffer, guint32& dwSize,
    guint32 dwOff )
{
    gint32 ret = 0;
    do{
        READ_LOCK( this );
        CStdRMutex oLock( GetExclLock() );
        FileSPtr pFile;
        auto itr = m_mapOpenFiles.find( hFile );
        if( itr == m_mapOpenFiles.end() )
        {
            ret = -ENOENT;
            break;
        }
        pFile = itr->second;
        oLock.Unlock();
        char* pBuf =
            const_cast< char* >( buffer );
        ret = pFile->WriteFile(
            dwSize, ( guint8* )pBuf, dwOff );
    }while( 0 );
    return 0;
}

gint32 CRegistryFs::Truncate(
    RFHANDLE hFile, guint32 dwOff )
{
    gint32 ret = 0;
    do{
        READ_LOCK( this );
        FileSPtr pFile;
        CStdRMutex oLock( GetExclLock() );
        auto itr = m_mapOpenFiles.find( hFile );
        if( itr == m_mapOpenFiles.end() )
        {
            ret = -ENOENT;
            break;
        }
        pFile = itr->second;
        oLock.Unlock();
        ret = pFile->Truncate( dwOff );
    }while( 0 );
    return 0;
}

gint32 CRegistryFs::RemoveDir(
    const stdstr& strPath,
    CAccessContext* pac )
{
    gint32 ret = 0; 
    do{
        READ_LOCK( this );
        FImgSPtr dirPtr;
        ret = GetParentDir(
            strPath, dirPtr, pac );
        if( ERROR( ret ) )
            break;

        std::string strDir =
            basename( strPath.c_str() );

        CDirImage* pDir = dirPtr;
        FImgSPtr pFile;
        ret = pDir->Search(
            strDir.c_str(), pFile );
        if( ERROR( ret ) )
            break;

        if( pac )
        {
            READ_LOCK( pDir );
            ret = pDir->CheckAccess( W_OK, pac );
            if( ERROR( ret ) )
                break;
        }

        ret = pDir->RemoveFile( strDir.c_str() );

    }while( 0 );
    return ret;
}

gint32 CRegistryFs::SetGid(
    const stdstr& strPath, gid_t wGid,
    CAccessContext* pac )
{
    gint32 ret = 0;
    do{
        READ_LOCK( this );
        if( strPath == "/" )
        {
            m_pRootImg->SetGid( wGid );
            break;
        }
        FImgSPtr dirPtr;
        ret = GetParentDir(
            strPath, dirPtr, pac );
        if( ERROR( ret ) )
            break;

        std::string strFile =
            basename( strPath.c_str() );

        CDirImage* pDir = dirPtr;
        FImgSPtr pFile;
        ret = pDir->Search(
            strFile.c_str(), pFile );
        if( ERROR( ret ) )
            break;

        if( pac )
        {
            READ_LOCK( pFile );
            ret = pFile->CheckAccess( W_OK, pac );
            if( ERROR( ret ) )
                break;
        }
        pFile->SetGid( wGid );

    }while( 0 );
    return ret;
}

gint32 CRegistryFs::SetUid(
    const stdstr& strPath, uid_t wUid,
    CAccessContext* pac )
{
    gint32 ret = 0;
    do{
        READ_LOCK( this );
        if( strPath == "/" )
        {
            m_pRootImg->SetUid( wUid );
            break;
        }
        FImgSPtr dirPtr;
        ret = GetParentDir(
            strPath, dirPtr, pac );
        if( ERROR( ret ) )
            break;

        std::string strFile =
            basename( strPath.c_str() );

        CDirImage* pDir = dirPtr;
        FImgSPtr pFile;
        ret = pDir->Search(
            strFile.c_str(), pFile );
        if( ERROR( ret ) )
            break;

        if( pac )
        {
            READ_LOCK( pFile );
            ret = pFile->CheckAccess( W_OK, pac );
            if( ERROR( ret ) )
                break;
        }
        pFile->SetUid( wUid );

    }while( 0 );
    return ret;
}

gint32 CRegistryFs::GetGid(
    const stdstr& strPath, gid_t& gid,
    CAccessContext* pac )
{
    gint32 ret = 0;
    do{
        READ_LOCK( this );
        if( strPath == "/" )
        {
            gid = m_pRootImg->GetUid();
            break;
        }
        FImgSPtr dirPtr;
        ret = GetParentDir(
            strPath, dirPtr, pac );
        if( ERROR( ret ) )
            break;

        std::string strFile =
            basename( strPath.c_str() );

        CDirImage* pDir = dirPtr;
        FImgSPtr pFile;
        ret = pDir->Search(
            strFile.c_str(), pFile );
        if( ERROR( ret ) )
            break;
        if( pac )
        {
            READ_LOCK( pFile );
            ret = pFile->CheckAccess( R_OK, pac );
            if( ERROR( ret ) )
                break;
        }
        gid = pFile->GetGid();
    }while( 0 );
    return ret;
}

gint32 CRegistryFs::GetUid(
    const stdstr& strPath, uid_t& uid,
    CAccessContext* pac )
{
    gint32 ret = 0;
    do{
        READ_LOCK( this );
        if( strPath == "/" )
        {
            uid = m_pRootImg->GetUid();
            break;
        }
        FImgSPtr dirPtr;
        ret = GetParentDir(
            strPath, dirPtr, pac );
        if( ERROR( ret ) )
            break;

        std::string strFile =
            basename( strPath.c_str() );

        CDirImage* pDir = dirPtr;
        FImgSPtr pFile;
        ret = pDir->Search(
            strFile.c_str(), pFile );
        if( ERROR( ret ) )
            break;
        if( pac )
        {
            READ_LOCK( pFile );
            ret = pFile->CheckAccess( R_OK, pac );
            if( ERROR( ret ) )
                break;
        }
        uid = pFile->GetUid();
    }while( 0 );
    return ret;
}

gint32 CRegistryFs::GetValue(
   const stdstr& strPath, Variant& oVar,
    CAccessContext* pac )
{
    gint32 ret = 0;
    do{
        READ_LOCK( this );
        FImgSPtr dirPtr;
        ret = GetParentDir(
            strPath, dirPtr, pac );
        if( ERROR( ret ) )
            break;

        std::string strFile =
            basename( strPath.c_str() );

        CDirImage* pDir = dirPtr;
        FImgSPtr pFile;
        ret = pDir->Search(
            strFile.c_str(), pFile );
        if( ERROR( ret ) )
            break;
        if( pac )
        {
            READ_LOCK( pFile );
            ret = pFile->CheckAccess( R_OK, pac );
            if( ERROR( ret ) )
                break;
        }
        ret = pFile->ReadValue( oVar );
    }while( 0 );
    return ret;
}

gint32 CRegistryFs::SetValue(
   const stdstr& strPath, Variant& oVar,
   CAccessContext* pac )
{
    gint32 ret = 0;
    do{
        READ_LOCK( this );
        FImgSPtr dirPtr;
        ret = GetParentDir(
            strPath, dirPtr, pac );
        if( ERROR( ret ) )
            break;

        std::string strFile =
            basename( strPath.c_str() );

        CDirImage* pDir = dirPtr;
        FImgSPtr pFile;
        ret = pDir->Search(
            strFile.c_str(), pFile );
        if( ERROR( ret ) )
            break;
        if( pac )
        {
            READ_LOCK( pFile );
            ret = pFile->CheckAccess( W_OK, pac );
            if( ERROR( ret ) )
                break;
        }
        ret = pFile->WriteValue( oVar );
    }while( 0 );
    return ret;
}

gint32 CRegistryFs::SymLink(
    const stdstr& strTarget,
    const stdstr& strLinkPath,
    CAccessContext* pac )
{
    gint32 ret = 0;
    do{
        READ_LOCK( this );
        FImgSPtr dirPtr;
        ret = GetParentDir(
            strLinkPath, dirPtr, pac );
        if( ERROR( ret ) )
            break;

        std::string strFile =
            basename( strLinkPath.c_str() );

        CDirImage* pDir = dirPtr;
        FImgSPtr pFile;
        ret = pDir->Search(
            strFile.c_str(), pFile );
        if( SUCCEEDED( ret ) )
        {
            ret = -EEXIST;
            DebugPrint( ret, "Error, "
                "%s already exists",
                strLinkPath.c_str() );
            break;
        }
        FImgSPtr pLink;
        if( pac )
        {
            READ_LOCK( pDir );
            ret = pDir->CheckAccess(
                W_OK, pac );
            if( ERROR( ret ) )
                break;
        }
        ret = pDir->CreateLink(
            strFile.c_str(),
            strTarget.c_str(), pLink );

    }while( 0 );
    return ret;
}

gint32 CRegistryFs::ReadLink(
    const stdstr& strLink,
    char* buf, guint32& dwSize,
    CAccessContext* pac )
{
    gint32 ret = 0;
    do{
        READ_LOCK( this );
        FImgSPtr dirPtr;
        ret = GetParentDir(
            strLink, dirPtr, pac );
        if( ERROR( ret ) )
            break;

        std::string strFile =
            basename( strLink.c_str() );

        CDirImage* pDir = dirPtr;
        FImgSPtr pFile;
        ret = pDir->Search(
            strFile.c_str(), pFile );
        if( ERROR( ret ) )
        {
            ret = -ENOENT;
            break;
        }
        CLinkImage* pLink = pFile;
        if( pLink == nullptr )
        {
            ret = -EINVAL;
            break;
        }
        if( pac )
        {
            READ_LOCK( pLink );
            ret = pLink->CheckAccess(
                R_OK, pac );
            if( ERROR( ret ) )
                break;
        }
        ret = pLink->ReadLink(
            ( guint8* )buf, dwSize );
    }while( 0 );
    return ret;
}

gint32 CRegistryFs::MakeDir(
    const stdstr& strPath, mode_t dwMode,
    CAccessContext* pac )
{
    gint32 ret = 0;
    do{
        READ_LOCK( this );
        FImgSPtr dirPtr;
        ret = GetParentDir(
            strPath, dirPtr, pac );
        if( ERROR( ret ) )
            break;

        std::string strFile =
            basename( strPath.c_str() );

        CDirImage* pDir = dirPtr;
        FImgSPtr pFile;
        ret = pDir->Search(
            strFile.c_str(), pFile );
        if( SUCCEEDED( ret ) )
        {
            ret = -EEXIST;
            break;
        }
        if( pac )
        {
            READ_LOCK( pDir );
            ret = pDir->CheckAccess(
                W_OK, pac );
            if( ERROR( ret ) )
                break;
        }
        ret = pDir->CreateDir(
            strFile.c_str(), dwMode, pFile );
    }while( 0 );
    return ret;
}

gint32 CRegistryFs::Chmod(
    const stdstr& strPath, mode_t dwMode,
    CAccessContext* pac )
{
    gint32 ret = 0;
    do{
        READ_LOCK( this );
        FImgSPtr dirPtr;
        ret = GetParentDir(
            strPath, dirPtr, pac );
        if( ERROR( ret ) )
            break;

        std::string strFile =
            basename( strPath.c_str() );

        CDirImage* pDir = dirPtr;
        FImgSPtr pFile;
        ret = pDir->Search(
            strFile.c_str(), pFile );
        if( ERROR( ret ) )
        {
            ret = -ENOENT;
            break;
        }
        if( pac )
        {
            READ_LOCK( pFile );
            ret = pFile->CheckAccess(
                W_OK, pac );
            if( ERROR( ret ) )
                break;
        }
        pFile->SetMode( dwMode );

    }while( 0 );
    return ret;
}

gint32 CRegistryFs::Chown(
    const stdstr& strPath,
    uid_t dwUid, gid_t dwGid,
    CAccessContext* pac )
{
    gint32 ret = 0;
    do{
        READ_LOCK( this );
        FImgSPtr dirPtr;
        ret = GetParentDir(
            strPath, dirPtr, pac );
        if( ERROR( ret ) )
            break;

        std::string strFile =
            basename( strPath.c_str() );

        CDirImage* pDir = dirPtr;
        FImgSPtr pFile;
        ret = pDir->Search(
            strFile.c_str(), pFile );
        if( ERROR( ret ) )
        {
            ret = -ENOENT;
            break;
        }
        if( pac )
        {
            READ_LOCK( pFile );
            ret = pFile->CheckAccess( W_OK, pac );
            if( ERROR( ret ) )
                break;
        }
        pFile->SetUid( dwUid );
        pFile->SetGid( dwGid );

    }while( 0 );
    return ret;
}

gint32 CRegistryFs::Rename(
    const stdstr& strFrom, const stdstr& strTo,
    CAccessContext* pac )
{
    gint32 ret = 0;
    do{
        READ_LOCK( this );
        FImgSPtr dirPtr;
        ret = GetParentDir(
            strFrom, dirPtr, pac );
        if( ERROR( ret ) )
            break;

        std::string strFile =
            basename( strFrom.c_str() );
        stdstr strDirName = 
            GetDirName( strFrom );
        stdstr strDstDirName = 
            GetDirName( strTo );
        stdstr strDstFile = 
            basename( strTo.c_str() );
        CDirImage* pDir = dirPtr;
        FImgSPtr pFile;
        ret = pDir->Search(
            strFile.c_str(), pFile );
        if( ERROR( ret ) )
        {
            ret = -ENOENT;
            break;
        }
        if( pac )
        {
            READ_LOCK( pDir );
            ret = pDir->CheckAccess( W_OK, pac );
            if( ERROR( ret ) )
                break;
        }
        if( strDstDirName == strDirName )
        {
            ret = pDir->Rename(
            strFile.c_str(), strDstFile.c_str() );
            break;
        }

        FImgSPtr dstDirPtr;
        ret = GetParentDir(
            strTo, dstDirPtr, pac );
        if( ERROR( ret ) )
            break;

        CDirImage* pDstDir = dstDirPtr;
        if( pac )
        {
            READ_LOCK( pDstDir );
            ret = pDstDir->CheckAccess( W_OK, pac );
            if( ERROR( ret ) )
                break;
        }
        FImgSPtr pDstFile;
        ret = pDir->RemoveFileNoFree(
            strFile.c_str(), pDstFile );
        if( ERROR( ret ) )
            break;
        ret = pDstDir->InsertFile(
            strDstFile.c_str(), pDstFile );
        if( SUCCEEDED( ret ) )
            break;
        if( ERROR( ret ) && ret != -EEXIST )
            break;
        ret = pDstDir->RemoveFile(
            strDstFile.c_str() );
        if( ERROR( ret ) )
            break;
        pDstFile->SetState( stateStarted );
        ret = pDstDir->InsertFile(
            strDstFile.c_str(), pDstFile );

    }while( 0 );
    return ret;
}

gint32 CRegistryFs::GetFile(
    RFHANDLE hFile, FileSPtr& pFile )
{
    if( hFile == INVALID_HANDLE )
        return -EINVAL;
    gint32 ret = 0;
    do{
        CStdRMutex oLock( this->GetExclLock() );
        auto itr = m_mapOpenFiles.find( hFile );
        if( itr == m_mapOpenFiles.end() )
        {
            ret = -ENOENT;
            break;
        }
        pFile = itr->second;

    }while( 0 );
    return ret;
}

gint32 CRegistryFs::Access(
    const stdstr& strPath, guint32 dwFlags,
    CAccessContext* pac )
{
    gint32 ret = 0;
    do{
        READ_LOCK( this );
        FImgSPtr dirPtr;
        if( strPath == "/" )
        {
            if( dwFlags == F_OK )
                break;
            READ_LOCK( m_pRootImg );
            ret = m_pRootImg->CheckAccess(
                dwFlags, pac );
            break;
        }
        ret = GetParentDir(
            strPath, dirPtr, pac );
        if( ERROR( ret ) )
            break;

        std::string strFile =
            basename( strPath.c_str() );

        CDirImage* pDir = dirPtr;
        FImgSPtr pFile;
        ret = pDir->Search(
            strFile.c_str(), pFile );
        if( ERROR( ret ) )
        {
            ret = -ENOENT;
            break;
        }
        if( dwFlags == F_OK )
            break;
        {
            READ_LOCK( pFile );
            ret = pFile->CheckAccess(
                dwFlags, pac );
        }

    }while( 0 );
    return ret;
}

gint32 CRegistryFs::OpenDir(
    const stdstr& strPath,
    guint32 dwFlags, RFHANDLE& hFile,
    CAccessContext* pac )
{
    gint32 ret = 0;
    do{
        READ_LOCK( this );
        FImgSPtr pFile;
        stdstr strFile; 
        if( strPath == "/" )
        {
            pFile = m_pRootImg;
            strFile = "/";
        }
        else
        {
            FImgSPtr dirPtr;
            ret = GetParentDir(
                strPath, dirPtr, pac );
            if( ERROR( ret ) )
                break;

            strFile = basename( strPath.c_str() );

            CDirImage* pDir = dirPtr;
            ret = pDir->Search(
                strFile.c_str(), pFile );
            if( ERROR( ret ) )
                break;
        }

        FileSPtr pOpenFile;
        ret = COpenFileEntry::Create( 
            ftDirectory, pOpenFile,
            pFile, m_pAlloc, strPath );
        if( ERROR( ret ) )
            break;

        ret = pOpenFile->Open( O_RDONLY, pac );
        if( ERROR( ret ) )
            break;

        CStdRMutex oLock( this->GetExclLock() );
        hFile = pOpenFile->GetObjId();
        m_mapOpenFiles[ hFile ] = pOpenFile;
        
    }while( 0 );
    if( ERROR( ret ) )
    {
        DebugPrint( ret, "Error create file '%s'",
            strPath.c_str() ); 
    }
    return ret;
}

gint32 CRegistryFs::ReadDir( RFHANDLE hDir,
    std::vector< KEYPTR_SLOT >& vecDirEnt )
{
    gint32 ret = 0;
    do{
        if( hDir == INVALID_HANDLE )
        {
            ret = -EINVAL;
            break;
        }
        READ_LOCK( this );
        CStdRMutex oLock( GetExclLock() );
        FileSPtr pFile;
        auto itr = m_mapOpenFiles.find( hDir );
        if( itr == m_mapOpenFiles.end() )
        {
            ret = -ENOENT;
            break;
        }
        pFile = itr->second;
        oLock.Unlock();
        CDirFileEntry* pDir = pFile;
        if( pDir == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        ret = pDir->ListDir( vecDirEnt );
        if( ERROR( ret ) )
            break;
    }while( 0 );
    return 0;
}

gint32 CRegistryFs::GetAttr(
    const stdstr& strPath,
    struct stat& stBuf,
    CAccessContext* pac )
{
    gint32 ret = 0;
    do{
        READ_LOCK( this );
        if( strPath == "/" )
        {
            ret = m_pRootImg->GetAttr( stBuf );
            break;
        }
        FImgSPtr dirPtr;
        ret = GetParentDir(
            strPath, dirPtr, pac );
        if( ERROR( ret ) )
            break;

        std::string strFile =
            basename( strPath.c_str() );

        CDirImage* pDir = dirPtr;
        FImgSPtr pFile;
        ret = pDir->Search(
            strFile.c_str(), pFile );
        if( ERROR( ret ) )
        {
            ret = -ENOENT;
            break;
        }

        if( pac )
        {
            READ_LOCK( pDir );
            ret = pFile->CheckAccess(
                R_OK, pac );
            if( ERROR( ret ) )
                break;
        }
        ret = pFile->GetAttr( stBuf );

    }while( 0 );
    return ret;
}

gint32 CRegistryFs::GetOpenFile(
    RFHANDLE hFile, FileSPtr& pFile )
{
    gint32 ret = 0;
    do{
        CStdRMutex oLock( GetExclLock() );
        auto itr = m_mapOpenFiles.find( hFile );
        if( itr == m_mapOpenFiles.end() )
        {
            ret = -EBADF;
            break;
        }
        pFile = itr->second;
    }while( 0 );
    return ret;
}

gint32 CRegistryFs::GetSize(
    RFHANDLE hFile, guint32 dwSize ) const
{
    gint32 ret = 0;
    do{
        FileSPtr pFile;
        CStdRMutex oLock( GetExclLock() );
        auto itr = m_mapOpenFiles.find( hFile );
        if( itr == m_mapOpenFiles.end() )
        {
            ret = -EBADF;
            break;
        }
        pFile = itr->second;
        oLock.Unlock();
        FImgSPtr pImg = pFile->GetImage();
        dwSize = pImg->GetSize();
    }while( 0 );
    return ret;
}

}
