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
            ROOT_INODE_BLKIDX, 0 );
        ret = m_pRootImg->Format();
        if( ERROR( ret ) )
            break;
        ret = m_pRootImg->Flush();
        ret = COpenFileEntry::Create( 
            ftDirectory, m_pRootDir,
            m_pRootImg, m_pAlloc );
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
            ROOT_INODE_BLKIDX, 0 );
        if( ERROR( ret ) )
            break;
        ret = m_pRootImg->Reload();
        if( ERROR( ret ) )
            break;

        ret = COpenFileEntry::Create( 
            ftDirectory, m_pRootDir,
            m_pRootImg, m_pAlloc );
        if( ERROR( ret ) )
            break;

        FImgSPtr pEmpty;
        ret = m_pRootDir->Open( pEmpty );

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
            break;
        ret = OpenRootDir();
    }while( 0 );
    return ret;
}

gint32 CRegistryFs::Namei(
    const stdstr& strPath,
    std::vector<stdstr>& vecNames ) const
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
    const stdstr& strPath, FImgSPtr& pDir )
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
                elem.c_str() );
            break;
        }
        gint32 idx = 1;
        CDirImage* pCurDir = m_pRootImg;
        for( ; idx < vecNames.size() - 1; idx++ )
        {
            stdstr& strCurDir = vecNames[ idx ];
            FImgSPtr pFile;
            CBPlusNode* pNode;
            ret = pCurDir->Search(
                strCurDir.c_str(), pFile, pNode );
            if( ERROR( ret ) )
                break
            pCurDir = pFile;
            if( pCurDir == nullptr )
            {
                ret = -EFAULT;
                break;
            }
        }

        if( ERROR( ret ) )
            break;
        pDir = pCurDir;
    }while( 0 );
    return ret;
}

gint32 CRegistryFs::CreateFile(
    const stdstr& strPath,
    CAccessContext* pac = nullptr )
{
    gint32 ret = 0;
    do{
        FImgSPtr dirPtr;
        ret = GetParentDir( strPath, dirPtr );
        if( ERROR( ret ) )
            break;
        CDirImage* pDir = dirPtr;
        FImgSPtr pFile;
        ret = pDir->CreateFile(
            strPath.c_str(), pFile );
    }while( 0 );
    if( ERROR( ret ) )
    {
        DebugPrint( ret, "Error create file '%s'",
            strPath.c_str() ); 
    }
    return ret;
}

RFHANDLE CRegistryFs::OpenFile(
    const stdstr& strPath,
    CAccessContext* pac = nullptr )
{
    gint32 ret = 0;
    guint64 hFile = INVALID_HANDLE;
    do{
        FImgSPtr dirPtr;
        ret = GetParentDir( strPath, dirPtr );
        if( ERROR( ret ) )
            break;

        CDirImage* pDir = dirPtr;
        FImgSPtr pFile;
        CBPlusNode* pNode = nullptr;
        ret = pDir->Search(
            strPath.c_str(), pFile, pNode );
        if( ERROR( ret ) )
            break;

        FileSPtr pOpenFile;
        ret = COpenFileEntry::Create( 
            ftRegular, pOpenFile,
            pFile, m_pAlloc );
        if( ERROR( ret ) )
            break;

        ret = pOpenFile->Open( nullptr, pac );

        CStdRMutex oLock( this->GetLock() )
        hFile = pOpenFile->GetObjId();
        m_mapOpenFiles[ hFile ] = pOpenFile;
        
    }while( 0 );
    if( ERROR( ret ) )
    {
        DebugPrint( ret, "Error create file '%s'",
            strPath.c_str() ); 
    }
    return hFile;
}

gint32 CRegistryFs::CloseFile(
    RFHANDLE hFile )
{
    if( hFile == INVALID_HANDLE )
        return -EINVAL;
    gint32 ret = 0;
    do{
        CStdRMutex oLock( this->GetLock() )
        auto itr = m_mapOpenFiles.find( hFile );
        if( itr = m_mapOpenFiles.end() )
        {
            ret = -ENOENT;
            break;
        }
        FileSPtr pFile = itr->second;
        m_mapOpenFiles.erase( itr );
        oLock.Unlock();

        pFile->Close();

    }while( 0 );
    return ret;
}

gint32 CRegistryFs::RemoveFile(
    const stdstr& strPath )
{
    gint32 ret = 0;
    guint64 hFile = INVALID_HANDLE;
    do{
        FImgSPtr dirPtr;
        ret = GetParentDir( strPath, dirPtr );
        if( ERROR( ret ) )
            break;

        CDirImage* pDir = dirPtr;
        FImgSPtr pFile;
        CBPlusNode* pNode = nullptr;
        ret = pDir->Search(
            strPath.c_str(), pFile, pNode );
        if( ERROR( ret ) )
            break;

        std::string strFile =
            basename( strPath.c_str() );

        ret = pDir->RemoveFile( strPath.c_str() );
        
    }while( 0 );
    if( ERROR( ret ) )
    {
        DebugPrint( ret, "Error create file '%s'",
            strPath.c_str() ); 
    }
    return hFile;
}

gint32 CRegistryFs::ReadFile( RFHANDLE hFile,
    char* buffer, guint32 dwSize )
{
    gint32 ret = 0;
    do{
        CStdRMutex oLock( GetLock() );
        FileSPtr pFile;
        auto itr = m_mapOpenFiles.find( hFile );
        if( itr == m_mapOpenFiles.end() )
        {
            ret = -ENOENT;
            break;
        }
        pFile = itr->second;
        oLock->Unlock();
        ret = pFile->ReadFile( buffer, dwSize );
    }while( 0 );
    return 0;
}

}
