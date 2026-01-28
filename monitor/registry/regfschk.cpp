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
static bool g_bRebuild = false;
static bool g_bClaimOrphans = false;

#ifdef RELEASE
#define OutputMsg2( ret, strFmt, ... ) \
do{ \
    printf( strFmt, ##__VA_ARGS__ ); \
    printf( "(%d)\n", ret ); \
}while( 0 )
#else
#define OutputMsg2 OutputMsg
#endif


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

std::unordered_map< stdstr, guint32> g_mapPath2Id;
std::unordered_map< guint32, stdstr > g_mapId2Path;
guint32 g_dwFileCount = 0;
gint32 GetFileId( const stdstr& strPath )
{
    auto itr = g_mapPath2Id.find( strPath );
    if( itr == g_mapPath2Id.end() )
        return -1;
    return itr->second;
}

stdstr GetFilePath( guint32 dwId )
{
    auto itr = g_mapId2Path.find( dwId );
    if( itr == g_mapId2Path.end() )
        return "";
    return itr->second;
}

void AddPath( const stdstr& strPath )
{
    g_mapPath2Id.emplace( strPath, g_dwFileCount );
    g_mapId2Path.emplace( g_dwFileCount, strPath );
    g_dwFileCount++;
}

gint32 CheckInode( RegFsPtr& pFs,
    FImgSPtr pFile,
    std::unordered_map< guint32, guint32 >& setBlocks )
{
    gint32 ret = 0;
    if( pFs.IsEmpty() || pFile.IsEmpty() )
        return -EINVAL;

    stdstr strPath;
    GetPathFromImg( pFile, strPath );
    AddPath( strPath );

    guint32 dwFileId = GetFileId( strPath );
    RegFSInode& oInode = pFile->m_oInodeStore;
    gint32 iSize = pFile->GetSize();
    if( iSize > MAX_FILE_SIZE )
    {
        OutputMsg2( iSize, "Error file size is invalid %s",
            strPath.c_str() );
        return errSize;
    }

    if( g_bVerbose )
        OutputMsg2( iSize,
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
            OutputMsg2( iSize, "Error file block table is corrupted %s",
                strPath.c_str() );
            return errBlkTbl;
        }
        iSize -= ret;
        dwRead += ret;
    }

    iSize = pFile->GetSize();

    std::vector< std::pair< guint32, guint32 >> fileBlocks;
    std::vector< std::pair<guint32, guint32 >> metaBlocks;

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
                    stdstr strOrig = GetFilePath(
                        itr->second );

                    OutputMsg2( i, "Error block %d "
                        "allocated to both %s and %s",
                        dwIdx,
                        strPath.c_str(),
                        strOrig.c_str() );
                    return errBlkTbl;
                }
                fileBlocks.emplace_back( dwIdx, dwFileId );
            }
            else if( WITHIN_INDIRECT_BLOCK( dwOff ) )
            {
                if( pFile->m_pBitBlk.IsEmpty() )
                {
                    OutputMsg2( dwOff,
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
                        OutputMsg2( i, "Error block allocated "
                            "more than once %s",
                            strPath.c_str() );
                        return errBlkTbl;
                    }
                    metaBlocks.emplace_back(
                        dwIdx, dwFileId );
                    bAddInd = true;
                }

                guint32* pbis = ( guint32* )
                    pFile->m_pBitBlk->ptr();

                guint32 dwBlkIdx = ntohl(
                    pbis[ INDIRECT_BLK_IDX( dwOff ) ] );
                if( dwBlkIdx == 0 )
                {
                    OutputMsg2( dwBlkIdx,
                        "Error indirect block "
                        "entry is empty %s",
                        strPath.c_str() );
                    return errBlkTbl;
                }
                auto itr = setBlocks.find( dwBlkIdx );
                if( itr != setBlocks.end() )
                {
                    OutputMsg2( i, "Error block allocated "
                        "more than once %s",
                        strPath.c_str() );
                    return errBlkTbl;
                }
                fileBlocks.emplace_back( dwBlkIdx, dwFileId );
            }
            else if( WITHIN_SEC_INDIRECT_BLOCK( dwOff ) )
            {
                if( pFile->m_pBitdBlk.IsEmpty() )
                {
                    OutputMsg2( dwOff,
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
                        OutputMsg2( i, "Error block allocated "
                            "more than once %s",
                            strPath.c_str() );
                        return errBlkTbl;
                    }
                    metaBlocks.emplace_back(
                        dwIdx, dwFileId );
                    guint32 dwMaxBd = BLKIDX_PER_TABLE;
                    guint32* pBitd = ( guint32* )
                        pFile->m_pBitdBlk->ptr();
                    decltype( metaBlocks ) vecBitBlks;
                    for( int j = 0; j < dwMaxBd; j++ ) 
                    {
                        guint32 dwBitBlkIdx =
                            ntohl( pBitd[ j ] );
                        if( dwBitBlkIdx == 0 )
                            break;
                        auto itr = setBlocks.find( dwIdx );
                        if( itr != setBlocks.end() )
                        {
                            OutputMsg2( i, "Error block allocated "
                                "more than once %s",
                                strPath.c_str() );
                            return errBlkTbl;
                        }
                        vecBitBlks.emplace_back(
                            dwBitBlkIdx, dwFileId );
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
                    OutputMsg2( i, "Error block table "
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
                    OutputMsg2( i, "Error secondary block "
                        "table damaged %s",
                        strPath.c_str() );
                    return errBlkTbl;
                }
                p = ( guint32* )itr->second->ptr();
                if( p == nullptr )
                {
                    OutputMsg2( i, "Error secondary block "
                        "table not allocated %s",
                        strPath.c_str() );
                    return errBlkTbl;
                }
                guint32 dwPayloadBlk = ntohl( p[ dwIdx2 ]);
                if( dwPayloadBlk == 0 )
                {
                    OutputMsg2( dwBlkCount, "Error data block "
                        "not allocated %s at block %d",
                        strPath.c_str(), i );
                    return errBlkTbl;
                }
                fileBlocks.emplace_back( dwPayloadBlk, dwFileId );
            }
        }
        if( fileBlocks.size() * BLOCK_SIZE - iSize > BLOCK_SIZE )
        {
            OutputMsg2( iSize, "Error data block "
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
    setBlocks.emplace(
        pFile->GetInodeIdx(), dwFileId );

    if( g_bVerbose )
    {
        OutputMsg2( fileBlocks.size(),
            "%s File Blocks", strPath.c_str() );
        OutputMsg2( metaBlocks.size(),
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

gint32 CollectBlockGroups( CGroupBitmap* pgbmp,
    std::vector< guint32 >& vecGrps )
{
    if( pgbmp == nullptr )
        return -EINVAL;

    guint32 dwOffset =
        GRPBMP_SIZE - sizeof( guint16 );
    for( guint32 i = 0; i < dwOffset; i++ )
    {
        gint32 ret =
            pgbmp->IsBlockGroupFree( i );
        if( SUCCEEDED( ret ) )
            continue;
        vecGrps.push_back( i );
    }
    return 0;
}

gint32 CheckGroupBitmap( RegFsPtr& pFs,
    std::unordered_map< guint32, guint32 >& setBlocks )
{
    AllocPtr pAlloc = pFs->GetAllocator();
    if( pAlloc.IsEmpty() )
        return -EINVAL;
    CGroupBitmap* pgbmp = pAlloc->GetGroupBitmap();
    gint32 ret = 0;
    do{
        OutputMsg2( 0, "Checking group bitmap..." );
        guint32 dwFree = pgbmp->GetFreeCount();

        std::vector< guint32 > vecGrps;
        CollectBlockGroups( pgbmp, vecGrps );
        guint32 dwMaxAlloced = vecGrps.back() + 1;

        struct stat stBuf;
        gint32 iFd = pAlloc->GetFd();
        fstat( iFd, &stBuf );
        if( stBuf.st_size - dwMaxAlloced * GROUP_SIZE >
            ( SUPER_BLOCK_SIZE + GRPBMP_SIZE ) )
        {
            guint64 qwGrps = dwMaxAlloced * GROUP_SIZE;
            OutputMsg2( 0,
                "Warning registry occupied more "
                "space than needed, allocated bytes "
                "%lld, reg size %lld, ",
                qwGrps, stBuf.st_size );
        }
        else if( dwMaxAlloced * GROUP_SIZE - stBuf.st_size >
            GROUP_SIZE - SUPER_BLOCK_SIZE - GRPBMP_SIZE )
        {
            guint64 qwGrps = dwMaxAlloced * GROUP_SIZE;
            OutputMsg2( 0,
                "Warning registry size is too "
                "small to hold all the block "
                "groups %lld, file size %lld",
                qwGrps, stBuf.st_size );
        }

        // count the allocated groups
        guint32 dwAlloced =
            pgbmp->GetAllocCount();
        if( dwAlloced != vecGrps.size() )
        {
            OutputMsg2( 0,
                "Warning counter of allocated groups ( %d ) does not "
                "agree with the number of allocated groups found (%d )",
                dwAlloced, vecGrps.size() );
        }

        auto pdw = ( guint32* )pgbmp->m_arrBytes;
        auto pdwend = pdw +
            GRPBMP_SIZE / sizeof( guint32 ) - 1;

        guint32 dwCount = 0;
        while( pdw < pdwend )
            dwCount += CountBits( *pdw++ );
        guint16* ps = ( guint16* )pdwend;
        dwCount += CountBits( *ps );
        if( dwCount != dwAlloced )
            OutputMsg2( 0,
                "Error the actual number of allocated "
                "groups is not aggree with the "
                "group bitmap's counter" );
        else
            OutputMsg2( 0,
                "groups allocated: %d", dwCount );
        OutputMsg2( 0, "Done Checking group bitmap" );

    }while( 0 );
    return ret;
}

using ELEM_GOODFILE=std::tuple< stdstr, guint32, size_t >;

gint32 RebuildFs( RegFsPtr& pOldFs, 
    std::vector< ELEM_GOODFILE >& vecGoodFiles )
{
    gint32 ret = 0;
    RegFsPtr pNewFs;
    do{
        CParamList oParams;
        // format a new registry
        oParams.Push( true );
        oParams.SetStrProp(
            propConfigPath,
            g_strRegFsFile + ".restore" );
        ret = pNewFs.NewObj(
            clsid( CRegistryFs ),
            oParams.GetCfg() );
        if( ERROR( ret ) )
        {
            OutputMsg2( ret,
                "Error creating new registry" );
            break;
        }
        ret = pNewFs->Start();
        if( ERROR( ret ) )
        {
            OutputMsg2( ret,
                "Error starting the new registry" );
            break;
        }

        guint8 buf[ 1024 * 128 ];
        for( auto& elem : vecGoodFiles )
        {
            auto& strPath = std::get<0>( elem );
            auto iType = std::get<1>( elem );
            auto dwSize = std::get<2>( elem );
            if( strPath == "/" )
                continue;
            if( iType == ftDirectory )
            {
                OutputMsg2( 0, "creating %s...", strPath.c_str() );
                ret = pNewFs->MakeDir( strPath, 0750 );
                if( ERROR( ret ) )
                {
                    OutputMsg2( ret,
                        "Error create directory %s",
                        strPath.c_str() );
                }
            }
            else if( iType == ftRegular )
            {
                OutputMsg2( 0, "creating %s...",
                    strPath.c_str() );
                RFHANDLE hOldFile, hFile;
                ret = pOldFs->OpenFile( strPath,
                    O_RDONLY, hOldFile, nullptr );
                if( ERROR( ret ) )
                    break;
                CFileHandle oOld( pOldFs, hOldFile );
                ret = pNewFs->CreateFile( strPath,
                    0600, O_WRONLY | O_TRUNC | O_CREAT,
                    hFile, nullptr );
                if( ERROR( ret ) )
                    break;
                CFileHandle oNew( pNewFs, hFile );
                guint32 dwCopied = 0;
                if( dwSize > 0 )do
                {
                    guint32 dwToRead = sizeof( buf );
                    ret = pOldFs->ReadFile(
                        hOldFile, ( char* )buf,
                        dwToRead, dwCopied );
                    if( ERROR( ret ) )
                        break;
                    guint32& dwToWrite = dwToRead;
                    ret = pNewFs->WriteFile( hFile,
                        ( const char* )buf, dwToWrite,
                        dwCopied );

                    if( ERROR( ret ) )
                        break;

                    dwCopied += dwToWrite;
                    if( dwToWrite < sizeof( buf ) )
                        break;
                }while( dwCopied < dwSize );
                if( ERROR( ret ) )
                {
                    OutputMsg2( ret,
                        "Error copying file %s",
                        strPath.c_str() );
                    break;
                }
            }
            else if( iType == ftLink )
            {
                OutputMsg2( 0, "creating %s...",
                    strPath.c_str() );
                RFHANDLE hOldFile, hFile;
                guint32 dwLinkSize = sizeof( buf ) - 1;
                ret = pOldFs->ReadLink( strPath,
                    ( char* )buf, dwLinkSize );
                if( ERROR( ret ) )
                    break;

                buf[ dwLinkSize ] = 0;
                ret = pNewFs->SymLink(
                    ( const char* )buf, strPath );
                if( ERROR( ret ) )
                    break;

                ret = pNewFs->OpenFile( strPath,
                    O_WRONLY | O_TRUNC | O_CREAT,
                    hFile, nullptr );
                if( ERROR( ret ) )
                    break;

                CFileHandle oNew( pNewFs, hFile );
                ret = pNewFs->WriteFile( hFile,
                    ( const char* )buf,
                    dwLinkSize, 0 );
                if( ERROR( ret ) )
                {
                    OutputMsg2( ret,
                        "Error copying file %s",
                        strPath.c_str() );
                    break;
                }
            }
            guint32 dwVal;
            ret = pOldFs->GetUid( strPath, dwVal );
            if( SUCCEEDED( ret ) )
                pNewFs->SetUid( strPath, dwVal );
            ret = pOldFs->GetGid( strPath, dwVal );
            if( SUCCEEDED( ret ) )
                pNewFs->SetGid( strPath, dwVal );
            Variant oVar;
            ret = pOldFs->GetValue( strPath, oVar );
            if( SUCCEEDED(ret ) )
                pNewFs->SetValue( strPath, oVar );
            struct stat stBuf;
            ret = pOldFs->GetAttr( strPath, stBuf );
            if( SUCCEEDED( ret ) )
            {
                pNewFs->Chmod( strPath, stBuf.st_mode );
                RFHANDLE hFile = INVALID_HANDLE;
                ret = pNewFs->OpenFile( strPath,
                    O_RDONLY, hFile, nullptr );
                if( ERROR( ret ) )
                    break;
                CFileHandle oNew( pNewFs, hFile );
                FileSPtr pOpen;
                ret = pNewFs->GetOpenFile( hFile, pOpen );
                if( SUCCEEDED( ret ) )
                {
                    struct timespec tv[ 2 ] =
                        {{0,0},{0,0}};
                    tv[ 0 ].tv_sec = stBuf.st_atime;
                    tv[ 1 ].tv_sec = stBuf.st_mtime;
                    pOpen->SetTimes( tv );
                }
            }
        }

    }while( 0 );
    if( !pNewFs.IsEmpty() )
        pNewFs->Stop();
    return ret;
}

#include <codecvt>
#include <locale>

bool IsValidUTF8(const std::string& str) {
    try {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
        std::wstring widestr = conv.from_bytes(str);
        return true;
    } catch (const std::range_error&) {
        return false;
    }
}

gint32 CheckLeaf(
    CDirImage* pDir, BNodeUPtr pLeaf )
{
    gint32 ret = 0;
    if( pDir == nullptr || !pLeaf )
        return -EINVAL;
    do{
        stdstr strPath;
        GetPathFromImg( pDir, strPath );
        guint32 dwCount = pLeaf->GetKeyCount();
        guint32 dwLimit = MIN_KEYS( pLeaf->IsRoot() );
        if( dwCount == 0 && pLeaf->IsRoot() )
            break;
        if( dwCount < dwLimit )
            OutputMsg2( dwCount,
                "Warning BNode keys is less than "
                "lower limit of %d, rebalancing "
                "needed for %s", dwLimit,
                strPath.c_str() );
        if( dwCount > MAX_KEYS_PER_NODE )
            OutputMsg2( dwCount,
                "Error BNode keys is has exceeded "
                "upper limit of %d, split "
                "needed for %s",
                MAX_KEYS_PER_NODE,
                strPath.c_str() );

        AllocPtr pAlloc = pDir->GetAlloc();
        for( int i = 0; i < dwCount; i++ )
        {
            auto pks = pLeaf->GetSlot( i );
            if( !IsValidUTF8( pks->szKey ) )
            {
                OutputMsg2( -EINVAL, "Error file entry "
                    "is not a valid UTF8 string @%d of "
                    "directory %s", i,
                    strPath.c_str() );
            }
            ret = pAlloc->IsBlockFree(
                pks->oLeaf.dwInodeIdx );
            if( ret != ERROR_FALSE )
               OutputMsg2( ret, "Error the "
               "file inode is not allocated a valid block@%d of "
               "directory %s", i,
               ( strPath + "/" + pks->szKey ).c_str() );
        }

    }while( 0 );
    return ret;
}

gint32 CheckNonLeaf(
    CDirImage* pDir, BNodeUPtr pNode )
{
    gint32 ret = 0;
    if( pDir == nullptr || !pNode )
        return -EINVAL;
    do{
        stdstr strPath;
        GetPathFromImg( pDir, strPath );
        guint32 dwCount = pNode->GetKeyCount();
        guint32 dwLimit = MIN_KEYS( pNode->IsRoot() );
        if( dwCount < dwLimit )
            OutputMsg2( dwCount,
                "Warning BNode keys is less than "
                "lower limit of %d, rebalancing "
                "needed for %s", dwLimit,
                strPath.c_str() );
        if( dwCount > MAX_KEYS_PER_NODE )
            OutputMsg2( dwCount,
                "Error BNode keys is has exceeded "
                "upper limit of %d, split "
                "needed for %s",
                MAX_KEYS_PER_NODE,
                strPath.c_str() );

        // OutputMsg2( dwCount,
        //     "Info BNode %d contains %d keys "
        //     "in directory %s",
        //     pNode->GetBNodeIndex(),
        //     dwCount, strPath.c_str() );
        AllocPtr pAlloc = pDir->GetAlloc();
        bool bDirty = false;
        for( int i = 0; i < dwCount + 1; i++ )
        {
            auto pks = pNode->GetSlot( i );
            if( pks->szKey[0] != 0 &&
                !IsValidUTF8( pks->szKey ) )
            {
                OutputMsg2( -EINVAL, "Error file entry "
                    "is not a valid UTF8 string @%d of "
                    "directory %s", i,
                    strPath.c_str() );
            }
            auto pChild = pNode->GetChild2( i );
            if( pChild )
                CheckNonLeaf( pDir, pChild );
            if( i < dwCount && !pNode->IsLeaf() )
            {
                auto pSucc = pNode->GetChild2( i + 1 );
                if( !pSucc )
                {
                    OutputMsg2( -EFAULT, "Error the child node"
                        "@%d is not available in directory %s",
                        i, strPath.c_str() );
                    continue;
                }
                auto pSuccKey = pSucc->GetSuccKey( 0 );
                if( pSuccKey &&
                    strcmp( pSuccKey, pks->szKey ) != 0 )
                {
                    OutputMsg2( -EINVAL, "Error the key "
                        "of non-leaf node@%d is an orphan key, "
                        "without a file object associated, "
                        "in directory %s, replaced with %s",
                        i, strPath.c_str(), pSuccKey );
                    COPY_KEY( pks->szKey, pSuccKey );
                    bDirty = true;
                }
            }
        }
        if( bDirty )
            pNode->Flush( FLAG_FLUSH_SINGLE_BNODE );

    }while( 0 );
    return ret;
}

gint32 CheckBNode( FImgSPtr ptrDir )
{
    gint32 ret = 0;
    do{
        CDirImage* pDir = ptrDir;
        if( pDir == nullptr )
        {
            ret = -EINVAL;
            break;
        }
        BNodeUPtr pNode =
            pDir->GetRootNode2();

        if( pNode )
        {
            if( pNode->IsLeaf() )
                CheckLeaf( pDir, pNode );
            else
                CheckNonLeaf( pDir, pNode );
        }

    }while( 0 );
    return ret;
}

using ELEM_BADFILE=std::tuple< stdstr, guint32, gint32 >;

gint32 FindBadFilesDir( RegFsPtr& pFs,
    FImgSPtr ptrDir, RFHANDLE hDir,
    std::vector< ELEM_BADFILE >& vecBadFiles,
    std::vector< ELEM_GOODFILE  >& vecGoodFiles,
    std::unordered_map< guint32, guint32 >& setBlocks )
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

    std::vector< ELEM_GOODFILE > vecGoodLocal;
    struct stat stBuf;

    do{
        std::vector< KEYPTR_SLOT > vecDirEnt;
        ret = pDir->ListDir( vecDirEnt );

        if( ERROR( ret ) )
            bCorrupted = true;
        if( bCorrupted && vecDirEnt.empty() )
        {
            if( strPath.size() )
                OutputMsg2( ret,
                    "Error list directory %s",
                    strPath.c_str() );
            else
                OutputMsg2( ret,
                    "Error list directory" );
            vecBadFiles.push_back(
                { strPath, errDirEnt, ftDirectory} );
            break;
        }

        CheckBNode( ptrDir );

        stdstr strIndent;
        strIndent.insert( 0, vecNames.size() * 4, ' ' );
        for( auto& ks : vecDirEnt )
        {
            if( ks.oLeaf.byFileType != ftDirectory )
                continue;
            if( g_bVerbose )
                OutputMsg2( 0, "%s%s",
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
                vecGoodLocal,
                setBlocks );
        }

        if( !g_bVerbose )
        {
            std::cout<<  "\033[2KChecking "
                << strPath << "\r"
                << std::flush;
        }

        for( auto& ks : vecDirEnt )
        {
            if( ks.oLeaf.byFileType == ftDirectory )
                continue;
            if( !g_bVerbose )
            {
                std::cout<<  "\033[2KChecking "
                    << strPath << "/" << ks.szKey << "\r"
                    << std::flush;
            }
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
            if( stBuf.st_size > MAX_FILE_SIZE )
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
                OutputMsg2( stBuf.st_size, "%s%s",
                    strIndent.c_str(), ks.szKey );
            vecGoodLocal.push_back( { strFile,
                ks.oLeaf.byFileType, stBuf.st_size } );
        }

        if( bCorrupted )
        {
            vecBadFiles.push_back(
                { strPath, errChild, ftDirectory } );
            break;
        }

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

        vecGoodFiles.push_back(
            { strPath, ftDirectory, stBuf.st_size } );

    }while( 0 );

    if( vecGoodLocal.size() )
    {
        if( bCorrupted )
        {
            // though the directory is corrupted,
            // the good ones under it requires its presence.
            vecGoodFiles.push_back(
                { strPath, ftDirectory, stBuf.st_size } );
        }
        vecGoodFiles.insert( vecGoodFiles.end(),
            vecGoodLocal.begin(), vecGoodLocal.end() );
    }
                
    if( bCorrupted )
    {
        OutputMsg2( 0, "Corrupted dir %s", strPath.c_str() );
        if( g_bVerbose )
            pDir->PrintBNode();
    }
    return ret;
}

gint32 FindBadFiles( RegFsPtr& pFs,
    std::vector< ELEM_BADFILE >& vecBadFiles,
    std::vector< ELEM_GOODFILE  >& vecGoodFiles,
    std::unordered_map< guint32, guint32 >& setBlocks )
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
            OutputMsg2( ret,
                "Error unable to open "
                "root directory" );
            break;
        }

        AddPath( "/" );

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
            OutputMsg2( ret,
                "Nothing bad found" );
            break;
        }

        OutputMsg2( vecBadFiles.size(),
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
            OutputMsg2( 0, "%s: (%s)%s",
                strPath.c_str(),
                strType.c_str(), strError.c_str() );
        }

        OutputMsg2( 0,
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
                OutputMsg2( std::get<2>(elem), "%s: %s",
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

gint32 ClaimOrphanBlocks( RegFsPtr& pFs,
    const std::unordered_map< guint32, guint32 >& setBlocks )
{
    gint32 ret = 0;
    do{
        AllocPtr pAlloc = pFs->GetAllocator();
        auto& mapBlkGrps = pAlloc->GetBlkGrps();    
        guint32 dwCount = 0;
        std::vector< guint32 > vecFreeGrps;
        std::vector< std::pair< guint32, guint32 > > vecBlks;
        for( auto& elem : mapBlkGrps )
        {
            CBlockGroup* pbg = elem.second.get();
            CBlockBitmap* pbit = pbg->GetBlkBmp();
            guint32 dwGrpIdx = pbit->GetGroupIndex();
            for( guint32 i = BLKBMP_BLKNUM;
                i < BLOCKS_PER_GROUP; i++ )
            {
                ret = pbit->IsBlockFree( i );
                if( SUCCEEDED( ret ) )
                    continue;
                guint32 dwBlkIdx = i |
                    dwGrpIdx << GROUP_INDEX_SHIFT;
                if( setBlocks.find( dwBlkIdx ) ==
                    setBlocks.end() )
                {
                    if( g_bClaimOrphans )
                        pbit->FreeBlock( i );
                    vecBlks.push_back( { dwBlkIdx,
                        BLOCK_ABS( dwBlkIdx ) } );
                    dwCount++;
                    continue;
                }
            }
            if( pbit->IsEmpty() )
                vecFreeGrps.push_back( dwGrpIdx );
        }
        if( g_bClaimOrphans )
            OutputMsg2( 0,
                "Orphan blocks claimed: %d", dwCount );
        else
            OutputMsg2( 0,
                "Orphan blocks: %d", dwCount );

        for( guint32 i = 0; i < dwCount; i+=4 )
        {
            OutputMsg2( 0, "{ %d, %d }, { %d, %d }, { %d, %d }, { %d, %d }",
                vecBlks[ i ].first, vecBlks[ i ].second, 
                i + 1 >= dwCount ? 0 : vecBlks[ i + 1 ].first,
                i + 1 >= dwCount ? 0 : vecBlks[ i + 1 ].second, 
                i + 2 >= dwCount ? 0 : vecBlks[ i + 2 ].first,
                i + 2 >= dwCount ? 0 : vecBlks[ i + 2 ].second, 
                i + 3 >= dwCount ? 0 : vecBlks[ i + 3 ].first,
                i + 3 >= dwCount ? 0 : vecBlks[ i + 3 ].second );
            if( i >= 20 )
            {
                OutputMsg2( 0, "..." );
                break;
            }
        }
        
        CGroupBitmap* pgbmp = pAlloc->GetGroupBitmap();
        for( auto elem : vecFreeGrps )
        {
            pgbmp->FreeGroup( elem );
            mapBlkGrps.erase( elem );
        }

        OutputMsg2( 0,
            "Freed empty block groups: %d",
            vecFreeGrps.size() );

        std::vector< guint32 > vecGrps;
        CollectBlockGroups( pgbmp, vecGrps );
        if( vecGrps.empty() )
            break;
        guint32 dwMaxAlloced = vecGrps.back() + 1;
        guint64 qwTail = dwMaxAlloced * GROUP_SIZE +
            SUPER_BLOCK_SIZE + GRPBMP_SIZE;
        gint32 iFd = pAlloc->GetFd();
        struct stat stBuf;
        fstat( iFd, &stBuf );

        if( stBuf.st_size > qwTail )
        {
            ftruncate( iFd , qwTail );
            OutputMsg2( 0,
                "Shrinked the registry size from %lld to %lld bytes",
                stBuf.st_size, qwTail );
        }
    }while( 0 );
    return ret;
}

static void Usage( const char* szName )
{
    fprintf( stderr,
        "Usage: %s [OPTIONS] <regfs path>\n"
        "\t [ <regfs path> the path to the file containing regfs  ]\n"
        "\t [ -c Claim the orphan blocks during checking ]\n"
        "\t [ -r Rebuild and defregrement the registry in a new registry "
            "file with the suffix '.restore' ]\n"
        "\t [ -h Display this help ]\n"
        "\t [ -v Output verbose information  ]\n", szName );
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
            OutputMsg2( ret, "Error no "
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
        std::unordered_map< guint32, guint32 > setBlocks;

        FindBadFiles( g_pRegfs,
            vecBadFiles, vecGoodFiles, setBlocks );
        OutputMsg2( 0, "Summary:" );
        OutputMsg2( 0, "Corrupted Files and Directories: %d",
            vecBadFiles.size() );
        OutputMsg2( 0, "Healthy Files and Directories: %d",
            vecGoodFiles.size() );
        OutputMsg2( 0, "Valid Blocks: %d",
            setBlocks.size() );

        OutputMsg2( 0, "Claiming orphan blocks..." );
        ClaimOrphanBlocks( g_pRegfs, setBlocks );
        if( g_bRebuild )
        {            
            // OutputMsg2( 0, "Do you want to remove "
            //     "the corrupted files and "
            //     "directories? (y/n)" );
            // stdstr strInput;
            // std::cin >> strInput;
            // if( strInput == "y" || strInput == "Y" )
            ret = RebuildFs( g_pRegfs, vecGoodFiles );
        }
        pFs->Stop();
        g_pRegfs.Clear();

    }while( 0 );
    if( ERROR( ret ) )
    {
        OutputMsg2( ret, "Warning, regfs "
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
        while( ( opt = getopt( argc, argv, "crvh" ) ) != -1 )
        {
            switch( opt )
            {
                case 'c':
                    g_bClaimOrphans = true;
                    break;
                case 'r':
                    g_bRebuild = true;
                    break;
                case 'v':
                    g_bVerbose = true;
                    break;
                case 'h':
                default:
                    { Usage( argv[ 0 ] ); exit( 0 ); }
            }
        }
        if( ERROR( ret ) )
        {
            Usage( argv[ 0 ] );
            break;
        }

        if( optind >= argc )
        {
            ret = -EINVAL;
            OutputMsg2( ret, "Error missing "
                "'registry file to check'" );
            Usage( argv[ 0 ] );
            break;
        }

        g_strRegFsFile = argv[ optind ];
        if( g_strRegFsFile.size() > REG_MAX_PATH - 1 )
        {
            OutputMsg2( ret,
                "Error file name too long" );
            ret = -ENAMETOOLONG;
            break;
        }

        ret = access( g_strRegFsFile.c_str(),
            R_OK | W_OK );
        if( ret == -1 )
        {
            ret = -errno;
            OutputMsg2( ret,
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
            OutputMsg2( -ERANGE,
                "Warning, found invalid filesystem "
                "size %lld, max is %lld",
                g_qwSize,
                MAX_FS_SIZE );
        }

        ret = InitContext();
        if( ERROR( ret ) ) 
        {
            DestroyContext();
            OutputMsg2( ret,
                "Error failed to start regfschk, "
                "abort..." );
            break;
        }
        ret = _main();
        DestroyContext();
    }while( 0 );
    return -ret;
}
