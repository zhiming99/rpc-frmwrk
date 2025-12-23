/*
 * =====================================================================================
 *
 *       Filename:  regfschk.cpp
 *
 *    Description:  a regfs tool the check and repair the errors and
 *                  remove the corrupted content from the regfs.
 *
 *        Version:  1.0
 *        Created:  12/16/2025 01:39:30 PM
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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <iostream>
#ifdef __FreeBSD__
#include <sys/socket.h>
#include <sys/un.h>
#endif
#include <sys/time.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif
#include "rpc.h"
using namespace rpcf;
#include "blkalloc.h"

extern EnumLogLvl rpcf::g_dwLogLevel;
static guint64 g_qwSize = 0;
static RegFsPtr g_pRegfs;
static stdstr g_strRegFsFile;
static bool g_bVerbose = false;

gint32 InitContext()
{
    gint32 ret = CoInitialize( 0 );
    if( ERROR( ret ) )
        return ret;
    do{
        stdstr strPath;
        ret = GetLibPath( strPath );
        if( ERROR( ret ) )
            break;

        strPath += "/libregfs.so";
        ret = CoLoadClassFactory(
            strPath.c_str() );

    }while( 0 );

    if( ERROR( ret ) )
    {
        CoUninitialize();
    }
    return ret;
}

gint32 DestroyContext()
{
    CoUninitialize();
    DebugPrintEx( logErr, 0,
        "#Leaked objects is %d",
        CObjBase::GetActCount() );
    return STATUS_SUCCESS;
}

gint32 ScanBlocks( RegFsPtr& pFs )
{
    return 0;
}

gint32 GetPathFromImg( FImgSPtr& pFile,
    stdstr& strPath )
{
    if( pFile.IsEmpty() )
        return -EINVAL;
    gint32 ret = 0;
    CFileImage* pNode = pFile;
    do{
        stdstr strName = pNode->GetName();
        if( strName == "/" )
        {
            strPath = strName + strPath;
            break;
        }
        else if( strPath.size() )
            strPath = strName + "/" + strPath;
        else
            strPath = strName;
        pNode = pNode->GetParentDir();
        if( pNode == nullptr )
            break;
    }while( 1 );
    return ret;
}

typedef enum {
    errOpen = 1,
    errSize,
    errDirEnt,
    errLoad,
    errInode,
    errChild,
    errBlkTbl,
} EnumErrType;

static stdstr GetErrorString( EnumErrType iErr )
{
    switch( iErr )
    {
    case errOpen:
        return stdstr( "error open" );
    case errSize:
        return stdstr( "error size" );
    case errDirEnt:
        return stdstr( "error dirent" );
    case errLoad:
        return stdstr( "error load object" );
    case errInode:
        return stdstr( "error inode" );
    case errBlkTbl:
        return stdstr( "error block table" );
    case errChild:
        return stdstr( "error child" );
    }
    return stdstr( "none" );
}

gint32 CheckInode( RegFsPtr& pFs,
    FImgSPtr pFile,
    std::unordered_set< guint32 >& setBlocks )
{
    gint32 ret = 0;
    if( pFs.IsEmpty() || pFile.IsEmpty() )
        return -EINVAL;

    stdstr strPath;
    GetPathFromImg( pFile, strPath );

    RegFSInode& oInode = pFile->m_oInodeStore;
    gint32 iSize = pFile->GetSize();
    if( BEYOND_MAX_LIMIT( iSize ) )
    {
        OutputMsg( iSize, "Error file size is invalid %s",
            strPath.c_str() );
        return errSize;
    }

    if( g_bVerbose )
        OutputMsg( iSize,
            "Checking inode %s", strPath.c_str() );

    // try to load all the block table
    guint8 buf[ 1024 * 1024 ];
    guint32 dwRead = 0;
    while( iSize > 0 )
    {
        guint32 dwSize = sizeof( buf );
        ret = pFile->ReadFile(
            dwRead, dwSize, buf );
        if( ret < 0 )
        {
            OutputMsg( iSize, "Error file block table is corrupted %s",
                strPath.c_str() );
            return errBlkTbl;
        }
        iSize -= ret;
        dwRead += ret;
    }

    iSize = pFile->GetSize();

    std::vector< guint32 > fileBlocks;
    std::vector< guint32 > metaBlocks;

    guint32 dwBlkCount = ( (
        iSize + BLOCK_SIZE - 1 ) >> BLOCK_SHIFT );

    if( dwBlkCount != 0 )
    {
        bool bAddInd = false, bAddSec = false;
        for( guint32 i = 0;i < dwBlkCount; i++ )
        {
            guint32 dwOff = ( i << BLOCK_SHIFT );
            if( WITHIN_DIRECT_BLOCK( dwOff ) )
            {
                guint32 dwIdx = oInode.m_arrBlocks[ i ];
                auto itr = setBlocks.find( dwIdx );
                if( itr != setBlocks.end() )
                {
                    OutputMsg( i, "Error block allocated "
                        "more than once %s",
                        strPath.c_str() );
                    return errBlkTbl;
                }
                fileBlocks.push_back( dwIdx );
            }
            else if( WITHIN_INDIRECT_BLOCK( dwOff ) )
            {
                if( pFile->m_pBitBlk.IsEmpty() )
                {
                    OutputMsg( dwOff,
                        "Error indirect block "
                        "bitmap is empty %s",
                        strPath.c_str() );
                    return errBlkTbl;
                }
                if( !bAddInd )
                {
                    guint32 dwIdx = 
                        oInode.m_arrBlocks[ BIT_IDX ];
                    auto itr = setBlocks.find( dwIdx );
                    if( itr != setBlocks.end() )
                    {
                        OutputMsg( i, "Error block allocated "
                            "more than once %s",
                            strPath.c_str() );
                        return errBlkTbl;
                    }
                    metaBlocks.push_back( dwIdx );
                    bAddInd = true;
                }

                guint32* pbis = ( guint32* )
                    pFile->m_pBitBlk->ptr();

                guint32 dwBlkIdx = ntohl(
                    pbis[ INDIRECT_BLK_IDX( dwOff ) ] );
                if( dwBlkIdx == 0 )
                {
                    OutputMsg( dwBlkIdx,
                        "Error indirect block "
                        "entry is empty %s",
                        strPath.c_str() );
                    return errBlkTbl;
                }
                auto itr = setBlocks.find( dwBlkIdx );
                if( itr != setBlocks.end() )
                {
                    OutputMsg( i, "Error block allocated "
                        "more than once %s",
                        strPath.c_str() );
                    return errBlkTbl;
                }
                fileBlocks.push_back( dwBlkIdx );
            }
            else if( WITHIN_SEC_INDIRECT_BLOCK( dwOff ) )
            {
                if( pFile->m_pBitdBlk.IsEmpty() )
                {
                    OutputMsg( dwOff,
                        "Error secondary indirect "
                        "block bitmap is empty %s",
                        strPath.c_str() );
                    return errBlkTbl;
                }
                if( !bAddSec )
                {
                    guint32 dwIdx = 
                        oInode.m_arrBlocks[ BITD_IDX ];
                    auto itr = setBlocks.find( dwIdx );
                    if( itr != setBlocks.end() )
                    {
                        OutputMsg( i, "Error block allocated "
                            "more than once %s",
                            strPath.c_str() );
                        return errBlkTbl;
                    }
                    metaBlocks.push_back( dwIdx );
                    guint32 dwMaxBd = BLKIDX_PER_TABLE;
                    guint32* pBitd = ( guint32* )
                        pFile->m_pBitdBlk->ptr();
                    std::vector< guint32 > vecBitBlks;
                    for( int j = 0; j < dwMaxBd; j++ ) 
                    {
                        guint32 dwBitBlkIdx =
                            ntohl( pBitd[ j ] );
                        if( dwBitBlkIdx == 0 )
                            break;
                        auto itr = setBlocks.find( dwIdx );
                        if( itr != setBlocks.end() )
                        {
                            OutputMsg( i, "Error block allocated "
                                "more than once %s",
                                strPath.c_str() );
                            return errBlkTbl;
                        }
                        vecBitBlks.push_back( dwBitBlkIdx );
                    }
                    metaBlocks.insert( metaBlocks.end(),
                        vecBitBlks.begin(), vecBitBlks.end() );
                    bAddSec = true;
                }

                guint32 dwBitIdx =
                    SEC_BIT_IDX( dwOff );
                auto p = ( guint32* )
                    pFile->m_pBitdBlk->ptr();
                guint32 dwBlkIdxIdx =
                    ntohl( p[ dwBitIdx ] );
                if( dwBlkIdxIdx == 0 )
                {
                    OutputMsg( i, "Error block table "
                        "directory damaged %s",
                        strPath.c_str() );
                    return errBlkTbl;
                }
                guint32 dwIdx2 =
                    SEC_BLKIDX_IDX( dwOff );
                auto itr = pFile->m_mapSecBitBlks.find(
                    dwBitIdx );
                if( itr == pFile->m_mapSecBitBlks.end() )
                {
                    OutputMsg( i, "Error secondary block "
                        "table damaged %s",
                        strPath.c_str() );
                    return errBlkTbl;
                }
                p = ( guint32* )itr->second->ptr();
                if( p == nullptr )
                {
                    OutputMsg( i, "Error secondary block "
                        "table not allocated %s",
                        strPath.c_str() );
                    return errBlkTbl;
                }
                guint32 dwPayloadBlk = ntohl( p[ dwIdx2 ]);
                if( dwPayloadBlk == 0 )
                {
                    OutputMsg( dwBlkCount, "Error data block "
                        "not allocated %s at block %d",
                        strPath.c_str(), i );
                    return errBlkTbl;
                }
                fileBlocks.push_back( dwPayloadBlk );
            }
        }
        if( fileBlocks.size() * BLOCK_SIZE - iSize > BLOCK_SIZE )
        {
            OutputMsg( iSize, "Error data block "
                "does not match the size %s",
                strPath.c_str() );
        }

        if( metaBlocks.size() )
            setBlocks.insert( metaBlocks.begin(),
                metaBlocks.end() );

        if( fileBlocks.size() )
            setBlocks.insert( fileBlocks.begin(),
                fileBlocks.end() );
    }
    setBlocks.insert(
        pFile->GetInodeIdx() );

    if( g_bVerbose )
    {
        OutputMsg( fileBlocks.size(),
            "%s File Blocks", strPath.c_str() );
        OutputMsg( metaBlocks.size(),
            "%s Meta Blocks", strPath.c_str() );
    }

    return 0;
}

template< typename T >
guint32 CountBits( T n ) {
    int count = 0;
    while( n ) {
        n &= (n - 1);
        count++;
    }
    return count;
}

gint32 CheckGroupBitmap( RegFsPtr& pFs,
    std::unordered_set< guint32 >& setBlocks )
{
    AllocPtr pAlloc = pFs->GetAllocator();
    if( pAlloc.IsEmpty() )
        return -EINVAL;
    CGroupBitmap* pgbmp = pAlloc->GetGroupBitmap();
    if( pgbmp == nullptr )
        return -EFAULT;
    gint32 ret = 0;
    do{
        OutputMsg( 0, "Checking group bitmap..." );
        guint32 dwFree = pgbmp->GetFreeCount();
        guint32 dwOffset =
            GRPBMP_SIZE - sizeof( guint16 );
        auto p =
            ( pgbmp->m_arrBytes + dwOffset ) - 1;
        auto a = *p;
        auto pend = pgbmp->m_arrBytes;

        while( a == 0 )
        {
            --p;
            if( p < pend )
                break;
            a = *p;
        }
        if( a == 0 )
        {
            OutputMsg( 0,
                "Error group bitmap is empty, "
                "which should at least have "
                "one block group allocated" );
            ret = -ENODATA;
            break;
        }

        guint32 dwLast = a;
        gint32 iStep = sizeof( guint32 ) * BYTE_BITS;
        gint32 iMaxAlloced =
            iStep * ( p - pend ) +
            ffs( dwLast );

        struct stat stBuf;
        fstat( pAlloc->GetFd(), &stBuf );
        if( stBuf.st_size - iMaxAlloced * GROUP_SIZE >
            ( SUPER_BLOCK_SIZE + GRPBMP_SIZE ) )
        {
            guint64 qwGrps = iMaxAlloced * GROUP_SIZE;
            OutputMsg( 0,
                "Warning registry occupied more "
                "space than needed, allocated bytes "
                "%lld, reg size %lld, ",
                qwGrps, stBuf.st_size );
            
            OutputMsg( 0, 
                "Shrink the registry? (y/n)" );
            stdstr strInput;
            std::cin >> strInput;
            if( strInput == "y" || strInput == "Y" )
            {
                ret = ftruncate( pAlloc->GetFd(),
                    qwGrps + SUPER_BLOCK_SIZE +
                    GRPBMP_SIZE );
                if( SUCCEEDED( ret ) )
                    OutputMsg( 0,
                        "registry is shrinked successfully" );
            }

        }
        else if( iMaxAlloced * GROUP_SIZE - stBuf.st_size >
            GROUP_SIZE - SUPER_BLOCK_SIZE - GRPBMP_SIZE )
        {
            guint64 qwGrps = iMaxAlloced * GROUP_SIZE;
            OutputMsg( 0,
                "Warning registry size is too "
                "small to hold all the block "
                "groups %lld, file size %lld",
                qwGrps, stBuf.st_size );
        }

        // count the allocated groups
        guint32 dwAlloced =
            pgbmp->GetAllocCount();

        auto pdw = ( guint32* )pgbmp->m_arrBytes;
        auto pdwend = pdw +
            GRPBMP_SIZE / sizeof( guint32 ) - 1;

        guint32 dwCount = 0;
        while( pdw < pdwend )
            dwCount += CountBits( *pdw++ );
        guint16* ps = ( guint16* )pdwend;
        dwCount += CountBits( *ps );
        if( dwCount != dwAlloced )
            OutputMsg( 0,
                "Error the actual number of allocated "
                "groups is not aggree with the "
                "group bitmap's counter" );
        OutputMsg( 0, "Done Checking group bitmap" );

    }while( 0 );
    return ret;
}

using ELEM_BADFILE=std::tuple< stdstr, guint32, gint32 >;
using ELEM_GOODFILE=std::tuple< stdstr, guint32, size_t >;

gint32 FindBadFilesDir( RegFsPtr& pFs,
    FImgSPtr ptrDir, RFHANDLE hDir,
    std::vector< ELEM_BADFILE >& vecBadFiles,
    std::vector< ELEM_GOODFILE  >& vecGoodFiles,
    std::unordered_set< guint32 >& setBlocks )
{
    gint32 ret = 0;
    if( ptrDir.IsEmpty() )
        return -EINVAL;
    stdstr strPath;
    GetPathFromImg( ptrDir, strPath );
    CDirImage* pDir = ptrDir;
    bool bCorrupted = false;
    std::vector< stdstr > vecNames;
    pFs->Namei( strPath, vecNames );

    do{
        std::vector< KEYPTR_SLOT > vecDirEnt;
        ret = pDir->ListDir( vecDirEnt );

        if( ERROR( ret ) )
            bCorrupted = true;
        if( bCorrupted && vecDirEnt.empty() )
        {
            if( strPath.size() )
                OutputMsg( ret,
                    "Error list directory %s",
                    strPath.c_str() );
            else
                OutputMsg( ret,
                    "Error list directory" );
            vecBadFiles.push_back(
                { strPath, errDirEnt, ftDirectory} );
            break;
        }

        stdstr strIndent;
        strIndent.insert( 0, vecNames.size() * 4, ' ' );
        for( auto& ks : vecDirEnt )
        {
            if( ks.oLeaf.byFileType != ftDirectory )
                continue;
            if( g_bVerbose )
                OutputMsg( 0, "%s%s",
                    strIndent.c_str(), ks.szKey );
            if( strnlen( ks.szKey, REGFS_NAME_LENGTH ) == 0 )
            {
                stdstr strName = strPath + "/unkdir_";
                if( strPath == "/" )
                    strName.erase( 0, 1 );

                strName += std::to_string(
                    ks.oLeaf.dwInodeIdx );

                EnumErrType iErr = errOpen;
                if( ks.oLeaf.dwInodeIdx == 0 )
                    iErr = errDirEnt;

                vecBadFiles.push_back( { strName,
                    iErr, ks.oLeaf.byFileType} );
                continue;
            }

            stdstr strChild = strPath + "/" + ks.szKey;
            if( strPath == "/" )
                strChild.erase( 0, 1 );

            FImgSPtr pChildDir;
            ret = pDir->Search( ks.szKey, pChildDir );
            if( ERROR( ret ) )
            {
                // name is damaged
                vecBadFiles.push_back( { strChild,
                    errDirEnt, ks.oLeaf.byFileType} );
                continue;
            }

            RFHANDLE hChildDir = INVALID_HANDLE;
            ret = pFs->OpenDir( strChild,
                O_RDONLY, hChildDir, nullptr );
            if( ERROR( ret ) )
            {
                vecBadFiles.push_back( { strChild,
                    errOpen, ks.oLeaf.byFileType} );
                bCorrupted = true;
                continue;
            }
            CFileHandle ofh( pFs, hChildDir );
            ret = pFs->GetDirImage(
                hChildDir, pChildDir );
            if( ERROR( ret ) )
            {
                vecBadFiles.push_back( {strChild,
                    errInode, ks.oLeaf.byFileType} );
                bCorrupted = true;
                continue;
            }
            FindBadFilesDir( pFs,
                pChildDir, hChildDir, vecBadFiles,
                vecGoodFiles,
                setBlocks );
        }

        for( auto& ks : vecDirEnt )
        {
            if( ks.oLeaf.byFileType == ftDirectory )
                continue;
            if( strnlen( ks.szKey, REGFS_NAME_LENGTH ) == 0 )
            {
                stdstr strName = strPath + "/unkfile_";
                if( strPath == "/" )
                    strName.erase( 0, 1 );

                strName += std::to_string(
                    ks.oLeaf.dwInodeIdx );

                EnumErrType iErr = errOpen;
                if( ks.oLeaf.dwInodeIdx == 0 )
                    iErr = errDirEnt;

                vecBadFiles.push_back( { strName,
                    iErr, ks.oLeaf.byFileType} );
                bCorrupted = true;
                continue;
            }

            stdstr strFile = strPath + "/" + ks.szKey;
            if( strPath == "/" )
                strFile.erase( 0, 1 );

            FImgSPtr pFile;
            ret = pDir->Search( ks.szKey, pFile );
            if( ERROR( ret ) )
            {
                // name is damaged
                vecBadFiles.push_back( { strFile,
                    errDirEnt, ks.oLeaf.byFileType} );
                continue;
            }
            
            ret = pFs->GetFileImage( hDir, ks.szKey,
                pFile, 0, nullptr );
            if( ERROR( ret ) )
            {
                vecBadFiles.push_back( {strFile,
                    errInode, ks.oLeaf.byFileType} );
                bCorrupted = true;
                continue;
            }
            struct stat stBuf;
            ret = pFile->GetAttr( stBuf );
            if( ERROR( ret ) )
            {
                vecBadFiles.push_back( { strFile,
                    errInode, ks.oLeaf.byFileType} );
                continue;
            }
            if( stBuf.st_size >= MAX_FILE_SIZE )
            {
                vecBadFiles.push_back( { strFile,
                    errSize, ks.oLeaf.byFileType} );
                continue;
            }

            ret = CheckInode( pFs, pFile, setBlocks );
            if( ret != 0 )
            {
                vecBadFiles.push_back( { strFile,
                    ret, ks.oLeaf.byFileType} );
                continue;
            }


            if( g_bVerbose )
                OutputMsg( stBuf.st_size, "%s%s",
                    strIndent.c_str(), ks.szKey );
            vecGoodFiles.push_back( { strFile,
                ks.oLeaf.byFileType, stBuf.st_size } );
        }

        if( bCorrupted )
        {
            vecBadFiles.push_back(
                { strPath, errChild, ftDirectory } );
            break;
        }

        struct stat stBuf;
        ret = pDir->GetAttr( stBuf );
        if( ERROR( ret ) )
        {
            bCorrupted = true;
            vecBadFiles.push_back(
                { strPath, errInode, ftDirectory } );
            break;
        }

        ret = CheckInode( pFs, pDir, setBlocks );
        if( ret != 0 )
        {
            bCorrupted = true;
            vecBadFiles.push_back(
                { strPath, ret, ftDirectory } );
            break;
        }
        vecGoodFiles.push_back( { strPath,
            ftDirectory, stBuf.st_size } );
                
    }while( 0 );

    if( bCorrupted )
    {
        OutputMsg( 0, "Corrupted dir %s", strPath.c_str() );
        pDir->PrintBNode();
    }
    return ret;
}

gint32 FindBadFiles( RegFsPtr& pFs,
    std::vector< ELEM_BADFILE >& vecBadFiles,
    std::vector< ELEM_GOODFILE  >& vecGoodFiles,
    std::unordered_set< guint32 >& setBlocks )
{
    gint32 ret = 0;
    if( pFs.IsEmpty() ) 
        return -EINVAL;
    do{
        CheckGroupBitmap( pFs, setBlocks );
        RFHANDLE hRoot = INVALID_HANDLE;
        ret = pFs->OpenDir(
            "/", O_RDONLY, hRoot, nullptr );
        if( ERROR( ret ) )
        {
            OutputMsg( ret,
                "Error unable to open "
                "root directory" );
            break;
        }
        CFileHandle orh( pFs, hRoot );
        FImgSPtr pRootDir;
        ret = pFs->GetDirImage(
            hRoot, pRootDir );
        if( ERROR( ret ) )
            break;
        ret = FindBadFilesDir( pFs,
            pRootDir, hRoot,
            vecBadFiles, vecGoodFiles,
            setBlocks );
        if( vecBadFiles.empty() )
        {
            OutputMsg( ret,
                "Nothing bad found" );
            break;
        }

        OutputMsg( vecBadFiles.size(),
            "Found corrupted files:" );

        for( auto& elem : vecBadFiles )
        {
            stdstr strError = GetErrorString(
                ( EnumErrType )std::get<1>(elem) );
            auto iType = std::get<2>(elem);
            stdstr strType;
            if( iType == ftDirectory )
                strType = "dir";
            else if( iType == ftLink )
                strType = "link";
            else if( iType == ftRegular )
                strType = "file";
            else
                strType = stdstr( "unk" ) +
                    std::to_string( iType );
            stdstr strPath =
                std::get<0>(elem).c_str();
            OutputMsg( 0, "%s: (%s)%s",
                strPath.c_str(),
                strType.c_str(), strError.c_str() );
        }

        OutputMsg( 0,
            "Healthy files: %d", vecGoodFiles.size() );
        if( g_bVerbose )
        {
            for( auto& elem : vecGoodFiles )
            {
                stdstr strType;
                auto iType = std::get<1>( elem );
                if( iType == ftDirectory )
                    strType = "dir";
                else if( iType == ftLink )
                    strType = "link";
                else if( iType == ftRegular )
                    strType = "file";
                else
                    strType = stdstr( "unk" ) +
                        std::to_string( iType );

                auto& strPath = std::get<0>( elem );
                OutputMsg( std::get<2>(elem), "%s: %s",
                    strPath.c_str(), strType.c_str() );
            }
        }

    }while( 0 );
    return ret;
}

gint32 BuildBadTree( RegFsPtr& pFs,
    const std::vector< ELEM_BADFILE >& vecBadFiles,
    std::map< stdstr, EnumErrType >& map )
{
    return 0;
}

gint32 RemoveBadDir( RegFsPtr& pFs,
    const std::vector< ELEM_BADFILE >& vecBadFiles )
{
    return 0;    
}

gint32 ClaimOrphanBlocks( RegFsPtr& pFs )
{
    return 0;
}

static void Usage( const char* szName )
{
    fprintf( stderr,
        "Usage: %s [OPTIONS] <regfs path> [<mount point>]\n"
        "\t [ <regfs path> the path to the file containing regfs  ]\n"
        "\t [ -h this help ]\n", szName );
}

int _main()
{
    gint32 ret = 0;
    do{ 
        rpcf::g_dwLogLevel = logEmerg;
        CParamList oParams;
        if( g_strRegFsFile.empty() )
        {
            ret = -EINVAL;
            OutputMsg( ret, "Error no "
                "registry path specified" );
            Usage( "regfsmnt" );
            break;
        }
        oParams.Push( false );
        oParams.SetStrProp(
            propConfigPath, g_strRegFsFile );
        ret = g_pRegfs.NewObj(
            clsid( CRegistryFs ),
            oParams.GetCfg() );
        if( ERROR( ret ) )
            break;

        CRegistryFs* pFs = g_pRegfs;
        ret = pFs->Start();
        if( ERROR( ret ) )
            break;
        std::vector< ELEM_BADFILE > vecBadFiles;
        std::vector< ELEM_GOODFILE > vecGoodFiles;
        std::unordered_set< guint32 > setBlocks;

        FindBadFiles( g_pRegfs,
            vecBadFiles, vecGoodFiles, setBlocks );
        OutputMsg( 0, "Summary:" );
        OutputMsg( 0, "Corrupted Files: %d", vecBadFiles.size() );
        OutputMsg( 0, "Healthy Files: %d", vecGoodFiles.size() );
        OutputMsg( 0, "Valid Blocks: %d", setBlocks.size() );

        pFs->Stop();
        g_pRegfs.Clear();

    }while( 0 );
    if( ERROR( ret ) )
    {
        OutputMsg( ret, "Warning, regfs "
            "quitting with error" );
    }

    return ret;
}

int main( int argc, char** argv)
{
    bool bDaemon = false;
    bool bDebug = false;
    bool bKernelCheck = false;
    int opt = 0;
    int ret = 0;
    do{
        while( ( opt = getopt( argc, argv, "hv" ) ) != -1 )
        {
            switch( opt )
            {
                case 'v':
                    g_bVerbose = true;
                    break;
                case 'h':
                default:
                    { Usage( argv[ 0 ] ); exit( 0 ); }
            }
        }
        if( ERROR( ret ) )
            break;

        if( optind >= argc )
        {
            ret = -EINVAL;
            OutputMsg( ret, "Error missing "
                "'regfs path' and 'mount point'" );
            break;
        }

        g_strRegFsFile = argv[ optind ];
        if( g_strRegFsFile.size() > REG_MAX_PATH - 1 )
        {
            OutputMsg( ret,
                "Error file name too long" );
            ret = -ENAMETOOLONG;
            break;
        }

        ret = access( g_strRegFsFile.c_str(),
            R_OK | W_OK );
        if( ret == -1 )
        {
            ret = -errno;
            OutputMsg( ret,
                "Error invalid registry file" );
            break;
        }
        struct stat st;
        ret = stat( g_strRegFsFile.c_str(), &st );
        if( ERROR( ret ) )
        {
            ret = -errno;
            break;
        }
        g_qwSize = st.st_size;
        if( g_qwSize > MAX_FS_SIZE ) 
        {
            OutputMsg( -ERANGE,
                "Warning, found invalid filesystem size" );
        }

        ret = InitContext();
        if( ERROR( ret ) ) 
        {
            DestroyContext();
            OutputMsg( ret,
                "Error failed to start regfschk, "
                "abort..." );
            break;
        }
        ret = _main();
        DestroyContext();
    }while( 0 );
    return ret;
}
