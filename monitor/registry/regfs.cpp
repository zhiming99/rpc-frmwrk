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

gint32 CRegistryFs::GetDirectory(
    const stdstr& strPath, FileSPtr& pDir )
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
        for( ; idx < vecNames.size(); idx++ )
        {
            stdstr& strCurDir = vecNames[ idx ];
            m_pRootDir

        }
    }while( 0 );
    return ret;
}

gint32 CRegistryFs::CreateFile(
    const stdstr& strPath,
    CAccessContext* pac = nullptr )
{
    gint32 ret = 0;
    do{

    }while( 0 );
    return ret;
}

}
