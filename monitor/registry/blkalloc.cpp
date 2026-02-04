/*
 * =====================================================================================
 *
 *       Filename:  blkalloc.cpp
 *
 *    Description:  implementation of CBlockAllocator and related classes 
 *
 *        Version:  1.0
 *        Created:  09/13/2024 05:09:34 PM
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
#include <unistd.h>
#include <limits.h>
#include <sys/uio.h>
#include <stdlib.h>

namespace rpcf{

bool    g_bSafeMode = false;
guint32 g_dwBlockSize = DEFAULT_BLOCK_SIZE;
guint32 g_dwRegFsPageSize = DEFAULT_PAGE_SIZE;
guint32 g_dwCacheLife = 180;

static gint32 IsValidBlockSize( guint32 dwSize )
{
    if( dwSize != 512 &&
        dwSize != 1024 &&
        dwSize != 2048 &&
        dwSize != 4096 )
        return -EINVAL;
    return STATUS_SUCCESS;
}

static gint32 SetBlockSize( guint32 dwSize )
{
    gint32 ret = IsValidBlockSize( dwSize );
    if( ERROR( ret ) )
        return ret;
    g_dwBlockSize = dwSize;
    return STATUS_SUCCESS;
}

static gint32 IsValidPageSize( guint32 dwSize )
{
    guint32 dwShift = find_shift( dwSize );
    if( dwShift <= BLOCK_SHIFT || dwShift > 16 )
        return -EINVAL;
    return STATUS_SUCCESS;
}

static gint32 SetPageSize( guint32 dwSize )
{
    gint32 ret = IsValidPageSize( dwSize );
    if( ERROR( ret ) )
        return ret;
    g_dwRegFsPageSize = dwSize;
    return STATUS_SUCCESS;
}

gint32 CSuperBlock::Flush( guint32 dwFlags )
{
    gint32 ret = 0;
    do{
        BufPtr pBuf( true );
        pBuf->Resize( SUPER_BLOCK_SIZE );
        char *arrSb = pBuf->ptr();
        memset( arrSb, 0, SUPER_BLOCK_SIZE );
        guint32* p = ( guint32* )arrSb;

        p[ 0 ] = m_dwMagic;
        p[ 1 ] = htonl( m_dwVersion );
        p[ 2 ] = htonl( m_dwBlkSize );
        p[ 3 ] = htonl( m_dwPageSize );
        ret = m_pAlloc->SaveSuperBlock(
            arrSb, SUPER_BLOCK_SIZE );
    }while( 0 );
    return ret;
}

gint32 CSuperBlock::Format()
{
    gint32 ret = 0;
    do{
        SetBlockSize( m_dwBlkSize );
        SetPageSize( m_dwPageSize );
    }while( 0 );
    return ret;
}

gint32 CSuperBlock::Reload()
{
    gint32 ret = 0;
    do{
        BufPtr pBuf( true );
        pBuf->Resize( SUPER_BLOCK_SIZE );
        char *arrSb = pBuf->ptr();
        ret = m_pAlloc->LoadSuperBlock(
            arrSb, SUPER_BLOCK_SIZE );
        if( ERROR( ret ) )
        {
            DebugPrint( ret,
                "Error LoadSuperBlock" );
            break;
        }

        if( arrSb[ 0 ] != 'r' ||
            arrSb[ 1 ] != 'e' ||
            arrSb[ 2 ] != 'g' ||
            arrSb[ 3 ] != 'i' )
        {
            DebugPrint( ret,
                "Error invalid magic number '%s' "
                "in SuperBlock", arrSb );
            ret = -EINVAL;
            break;
        }
        guint32* p = ( guint32* )arrSb;
        m_dwVersion = ntohl( p[ 1 ] );
        if( m_dwVersion != 0x01 )
        {
            ret = -ENOTSUP;
            break;
        }

        guint32 dwBlkSize = ntohl( p[ 2 ] );
        ret = SetBlockSize( dwBlkSize );
        if( ERROR( ret ) )
            break;
        m_dwBlkSize = dwBlkSize;

        guint32 dwPageSize = htonl( p[ 3 ] );
        ret = SetPageSize( dwPageSize );
        if( ERROR( ret ) )
            break;
        m_dwPageSize = dwPageSize;

    }while( 0 );
    return ret;
}

CGroupBitmap::CGroupBitmap(
    CBlockAllocator* pAlloc ) :
    m_pAlloc( pAlloc )
{
    m_pBytes.NewObj();
    m_pBytes->Resize( GRPBMP_SIZE );
    m_arrBytes = ( guint8* )m_pBytes->ptr();
}

gint32 CGroupBitmap::FreeGroup( guint32 dwGrpIdx )
{
    gint32 ret = 0;
    do{
        if( m_wFreeCount == BLKGRP_NUMBER )
            break;
        guint8* pbmp = m_arrBytes;

        constexpr guint32 dwNumBits = BYTE_BITS;
        if( dwGrpIdx >= BLKGRP_NUMBER )
        {
            ret = -EINVAL;
            break;
        }

        guint32 shift = find_shift( dwNumBits );
        guint32 dwByteIdx = ( dwGrpIdx >> shift );
        guint32 dwOffset =
            dwGrpIdx & ( dwNumBits - 1 );

        guint8* a = pbmp + dwByteIdx;
        if( *a == 0 )
            break;

        guint8 bit = ( 1 << dwOffset );
        *a &= ( ~bit );
        m_wFreeCount++;
        Flush( 0 );

    }while( 0 );

    return ret;
}

gint32 CGroupBitmap::AllocGroup( guint32& dwGrpIdx )
{
    if( m_wFreeCount == 0 )
        return -ENOMEM;
    gint32 ret = 0;
    LONGWORD* pbmp = ( LONGWORD* )m_arrBytes;

    guint32 dwCount =
        GRPBMP_SIZE / sizeof( LONGWORD );

    bool bFound = false;
    guint8 bit = 1;
    constexpr guint32 dwNumBits = BYTE_BITS;
    for( guint32 i = 0; i < dwCount; i++ )
    {
        LONGWORD& b = *pbmp;
        guint32 dwByteIdx = i * sizeof( LONGWORD );
        guint32 j = dwByteIdx;
        guint32 dwEnd =
            dwByteIdx + sizeof( LONGWORD );
        if( i < dwCount - 1 )
        {
            if( b == ( ( LONGWORD )-1 ) )
            {
                pbmp++;
                continue;
            }
            for( ; j < dwEnd; j++ )
                if( m_arrBytes[ j ] != UCHAR_MAX )
                    break;
            if( j == dwEnd )
                break;
            dwByteIdx = j;
            auto& a = m_arrBytes[ dwByteIdx ];
            for( j = 0; j < dwNumBits; j++ )
            {
                if( ( a & bit ) == 0 )
                {
                    dwGrpIdx =
                        dwByteIdx * dwNumBits + j;
                    bFound = true;
                    a |= bit;
                    break;
                }
                bit <<= 1;
            }
            break;
        }
        dwEnd -= sizeof( guint16 );
        for(; j < dwEnd; j++ )
            if( m_arrBytes[ j ] != UCHAR_MAX )
                break;

        if( j == dwEnd )
            break;

        dwByteIdx = j;
        auto& a = m_arrBytes[ dwByteIdx ];
        if( a == UCHAR_MAX )
            break;
        for( j = 0; j < dwNumBits; j++ )
        {
            if( ( a & bit ) == 0 )
            {
                dwGrpIdx =
                    dwByteIdx * dwNumBits + j;
                bFound = true;
                a |= bit;
                break;
            }
            bit <<= 1;
        }
    }
    if( bFound )
    {
        m_wFreeCount--;
        Flush( 0 );
        return STATUS_SUCCESS;
    }
    return -ENOMEM;
}

gint32 CGroupBitmap::InitBitmap()
{
    guint32 dwOffset =
        GRPBMP_SIZE - sizeof( guint16 );
    memset( m_arrBytes, 0, dwOffset );
    guint16* pbmp = ( guint16* )m_arrBytes;
    m_wFreeCount = BLKGRP_NUMBER;
    pbmp[ dwOffset ] = htons( m_wFreeCount );
    return 0;
}

gint32 CGroupBitmap::IsBlockGroupFree(
    guint32 dwGrpIdx ) const
{
    gint32 ret = 0;
    do{
        if( m_wFreeCount == BLKGRP_NUMBER )
            break;

        guint8* pbmp = m_arrBytes;
        constexpr int iNumBits = BYTE_BITS;
        if( dwGrpIdx >= BLKGRP_NUMBER )
        {
            ret = -EINVAL;
            break;
        }

        guint32 shift = find_shift( iNumBits );
        guint32 dwByteIdx = ( dwGrpIdx >> shift );
        int offset = dwGrpIdx & ( iNumBits - 1 );

        guint8* a = pbmp + dwByteIdx;
        guint8 bit = ( 1 << offset );
        if( *a & bit )
            ret = ERROR_FALSE;

    }while( 0 );

    return ret;
}

gint32 CGroupBitmap::Flush( guint32 dwFlags )
{
    gint32 ret = 0;
    do{
        guint16* p = ( guint16*)
            ( m_arrBytes + GRPBMP_SIZE );
        p[ -1 ] = htons( m_wFreeCount );
        ret = m_pAlloc->SaveGroupBitmap(
            ( char* )m_arrBytes, GRPBMP_SIZE );

    }while( 0 );
    return ret;
}
gint32 CGroupBitmap::Format()
{
    gint32 ret = 0;
    do{
        m_wFreeCount = BLKGRP_NUMBER;
        memset( m_arrBytes, 0, GRPBMP_SIZE );
        guint16* p = ( guint16*)
            ( m_arrBytes + GRPBMP_SIZE );
        p[ -1 ] = htons(  m_wFreeCount );
        ret = m_pAlloc->SaveGroupBitmap(
            ( char* )m_arrBytes, GRPBMP_SIZE );
    }while( 0 );
    return ret;
}

gint32 CGroupBitmap::Reload()
{
    gint32 ret = 0;
    do{
        ret = m_pAlloc->LoadGroupBitmap(
            ( char* )m_arrBytes, GRPBMP_SIZE );
        if( ERROR( ret ) )
            break;
        guint16* p = ( guint16*)
            ( m_arrBytes + GRPBMP_SIZE );
        m_wFreeCount = ntohs( p[ -1 ] );
        if( m_wFreeCount > BLKGRP_NUMBER )
        {
            ret = -ERANGE;
            break;
        }
    }while( 0 );
    return ret;
}

gint32 CBlockBitmap::IsBlockFree(
    guint32 dwBlkIdx ) const
{
    gint32 ret = 0;
    if( dwBlkIdx == 0 )
        return -EINVAL;
    do{
        if( GetFreeCount() ==
            BLOCKS_PER_GROUP - BLKBMP_BLKNUM )
            break;

        auto pbmp = m_arrBytes;
        constexpr guint32 dwNumBits = BYTE_BITS;

        guint32 shift = find_shift( dwNumBits );
        guint32 dwByteIdx = ( dwBlkIdx >> shift );

        guint32 dwOffset =
            dwBlkIdx & ( dwNumBits - 1 );

        guint8* a = pbmp + dwByteIdx;
        guint8 bit = ( 1 << dwOffset );
        if( *a & bit )
            ret = ERROR_FALSE;

    }while( 0 );

    return ret;
}

gint32 CBlockBitmap::InitBitmap()
{
    guint32 dwOffset =
        BLKBMP_SIZE - sizeof( guint16 );
    memset( m_arrBytes, 0, dwOffset );
    gint32 dwBytes = BLKBMP_BLKNUM / 8;
    for( int i = 0; i < dwBytes; i++ )
        m_arrBytes[ i ] = 0xff;

    guint32 dwCount = ( BLKBMP_BLKNUM & 0x07 );
    if( dwCount )
    {
        guint8 bit = 1;
        guint8* byTail = m_arrBytes + dwBytes;
        while( dwCount-- )
        {
            *byTail |= bit;
            bit <<= 1;
        }
    }
    m_wFreeCount =
        BLOCKS_PER_GROUP - BLKBMP_BLKNUM;
    auto pbmp = ( guint16* )
        ( m_arrBytes + dwOffset );
    *pbmp = htons( m_wFreeCount );
    return 0;
}

gint32 CBlockBitmap::AllocBlocks(
    guint32* pvecBlocks,
    guint32& dwNumBlocks )
{
    if( pvecBlocks == nullptr ||
        dwNumBlocks == 0 )
        return -EINVAL;
    
    if( m_wFreeCount == 0 )
        return -ENOMEM;

    gint32 ret = 0;
    guint32 dwBlocks = 0;
    bool bFull = false;
    guint8 arrDirtyBlk[ BLKBMP_BLKNUM ] = {0};
    guint32 dwGrpIdx =
        ( m_dwGroupIdx << GROUP_INDEX_SHIFT );
    do{
        LONGWORD* pbmp = ( LONGWORD* )m_arrBytes;

        guint32 dwCount =
            BLKBMP_SIZE/sizeof( LONGWORD );

        guint32 dwBlkIdx = 0;
        constexpr guint32 dwNumBits = BYTE_BITS;

        for( guint32 i = 0;
            i < dwCount && dwBlocks < dwNumBlocks;
            i++ )
        {
            LONGWORD& b = *pbmp;
            guint8 bit = 1;

            guint32 dwByteIdx = i * sizeof( LONGWORD );
            guint32 j = dwByteIdx;
            guint32 dwEnd =
                dwByteIdx + sizeof( LONGWORD );

            if( i < dwCount - 1 )
            {
                if( b == ( ( LONGWORD )-1 ) )
                {
                    pbmp++;
                    continue;
                }
                for(; j < dwEnd; j++ )
                    if( m_arrBytes[ j ] != UCHAR_MAX )
                        break;
                if( j == dwEnd )
                    break;
                dwByteIdx = j;
                auto& a = m_arrBytes[ dwByteIdx ];
                for( j = 0; j < dwNumBits; j++ )
                {
                    if( ( a & bit ) == 0 )
                    {
                        dwBlkIdx =
                            dwByteIdx * dwNumBits + j;
                        pvecBlocks[ dwBlocks ] =
                            ( dwGrpIdx | dwBlkIdx );
                        dwBlocks++;
                        a |= bit;
                        if( IsSafeMode() )
                        {
                            arrDirtyBlk[ dwBlkIdx /
                                ( BLOCK_SIZE * BYTE_BITS ) ] = 1;
                        }
                        if( dwBlocks == dwNumBlocks )
                        {
                            bFull = true;
                            break;
                        }
                    }
                    bit <<= 1;
                }
                if( bFull )
                    break;
                pbmp++;
                continue;
            }

            dwEnd -= sizeof( guint16 );
            for(; j < dwEnd; j++ )
                if( m_arrBytes[ j ] != UCHAR_MAX )
                    break;

            if( j == dwEnd )
                break;

            dwByteIdx = j;
            auto& a = m_arrBytes[ dwByteIdx ];

            for( j = 0; j < dwNumBits; j++ )
            {
                if( ( a & bit ) == 0 )
                {
                    dwBlkIdx =
                        dwByteIdx * dwNumBits + j;
                    pvecBlocks[ dwBlocks ] =
                        ( dwGrpIdx | dwBlkIdx );
                    dwBlocks++;
                    a |= bit;
                    if( IsSafeMode() )
                    {
                        arrDirtyBlk[ dwBlkIdx /
                            ( BLOCK_SIZE * BYTE_BITS ) ] = 1;
                    }
                    if( dwBlocks == dwNumBlocks )
                    {
                        bFull = true;
                        break;
                    }
                }
                bit <<= 1;
            }
        }
    }while( 0 );

    if( m_wFreeCount < dwBlocks )
        return ERROR_FAIL;
    m_wFreeCount -= dwBlocks;
    if( dwBlocks && IsSafeMode() )
    {
        guint16* p = ( guint16* )
            ( m_arrBytes + BLKBMP_BLKNUM * BLOCK_SIZE );
        p[ -1 ] = htons( m_wFreeCount );
        arrDirtyBlk[ BLKBMP_BLKNUM - 1 ] = 1;
        for( int i = 0; i < BLKBMP_BLKNUM; i++ )
        {
            if( arrDirtyBlk[ i ] == 0 )
                continue;
            guint32 dwBlkIdx = ( dwGrpIdx | i );
            m_pAlloc->WriteBlock( dwBlkIdx,
                m_arrBytes + i * BLOCK_SIZE  );
        }
    }
    if( bFull )
        return STATUS_SUCCESS;
    dwNumBlocks = dwBlocks;
    return STATUS_MORE_PROCESS_NEEDED;
}

gint32 CBlockBitmap::FreeBlock(
    guint32 dwBlkIdx )
{
    if( dwBlkIdx == 0 )
        return -EINVAL;
    return FreeBlocks( &dwBlkIdx, 1 );
}
gint32 CBlockBitmap::FreeBlocks(
    const guint32* pvecBlocks,
    guint32 dwNumBlocks )
{
    gint32 ret = 0;
    if( pvecBlocks == nullptr ||
        dwNumBlocks == 0 )
        return -EINVAL;
    do{
        if( m_wFreeCount ==
            BLOCKS_PER_GROUP - BLKBMP_BLKNUM )
            break;

        LONGWORD* pbmp = ( LONGWORD* )m_arrBytes;
        constexpr guint32 dwNumBits = BYTE_BITS;
        guint8 arrDirtyBlk[ BLKBMP_BLKNUM ] = {0};
        guint32 dwCount = 0;

        guint32 shift = find_shift( dwNumBits );
        for( guint32 i = 0; i < dwNumBlocks; i++ )
        {
            guint32 dwBlkIdx =
            ( pvecBlocks[ i ] & BLOCK_IDX_MASK );
            if( dwBlkIdx == 0 )
                continue;

            guint32 dwByteIdx =
                ( dwBlkIdx >> shift );

            guint32 offset =
                dwBlkIdx & ( dwNumBits - 1 );

            guint8* a = m_arrBytes + dwByteIdx;
            if( *a == 0 )
                continue;

            guint8 bit = ( 1 << offset );
            *a &= ( ~bit );
            m_wFreeCount++;
            if( IsSafeMode() )
            {
                arrDirtyBlk[ dwBlkIdx /
                    ( BLOCK_SIZE * BYTE_BITS ) ] = 1;
                dwCount++;
            }
        }
        if( dwCount && IsSafeMode() )
        {
            guint32 dwGrpIdx =
                ( m_dwGroupIdx << GROUP_INDEX_SHIFT );
            guint16* p = ( guint16* )
                ( m_arrBytes + BLKBMP_BLKNUM * BLOCK_SIZE );
            p[ -1 ] = htons( m_wFreeCount );
            arrDirtyBlk[ BLKBMP_BLKNUM - 1 ] = 1;
            for( int i = 0; i < BLKBMP_BLKNUM; i++ )
            {
                if( arrDirtyBlk[ i ] == 0 )
                    continue;
                guint32 dwBlkIdx = ( dwGrpIdx | i );
                m_pAlloc->WriteBlock( dwBlkIdx,
                    m_arrBytes + i * BLOCK_SIZE  );
            }
        }
    }while( 0 );

    return ret;
}

gint32 CBlockBitmap::Flush( guint32 dwFlags )
{
    gint32 ret = 0;
    do{
        guint32 dwBlockIdx =
            ( m_dwGroupIdx << GROUP_INDEX_SHIFT );
        DECL_ARRAY2( guint32,
            arrBlks, BLKBMP_BLKNUM );
        guint32 i = 0;
        guint32 dwCount = BLKBMP_BLKNUM;
        guint32 dwSize =
            dwCount * BLOCK_SIZE;
        for( ; i < dwCount; i++ )
            arrBlks[ i ] = dwBlockIdx + i;
        guint16* p = ( guint16* )
            ( m_arrBytes + dwSize );
        p[ -1 ] = htons( m_wFreeCount );
        ret = m_pAlloc->WriteBlocks(
            arrBlks, dwCount, m_arrBytes, true );
        if( ERROR( ret ) )
            break;
        ret = 0;
    }while( 0 );

    return ret;
}

gint32 CBlockBitmap::Format()
{
    gint32 ret = InitBitmap();
    if( ERROR( ret ) )
        return ret;
    return Flush();
}

gint32 CBlockBitmap::Reload()
{
    gint32 ret = 0;
    do{
        guint32 dwBlockIdx =
            ( m_dwGroupIdx << GROUP_INDEX_SHIFT );
        DECL_ARRAY2( guint32,
            arrBlks, BLKBMP_BLKNUM );
        guint32 i = 0;
        guint32 dwCount = BLKBMP_BLKNUM;
        guint32 dwSize = dwCount * BLOCK_SIZE;
        for( ; i < dwCount; i++ )
            arrBlks[ i ] = dwBlockIdx + i;
        ret = m_pAlloc->ReadBlocks(
            arrBlks, dwCount, m_arrBytes, true );
        if( ERROR( ret ) )
            break;
        ret = 0;
        guint16* p = ( guint16* )
            &m_arrBytes[ dwSize ];
        m_wFreeCount = ntohs( p[ -1 ] );
    }while( 0 );

    return ret;
}

CBlockAllocator::CBlockAllocator(
    const IConfigDb* pCfg )
{
    gint32 ret = 0;
    do{
        SetClassId( clsid( CBlockAllocator ) );
        stdstr strPath;
        CCfgOpener oCfg( pCfg );
        ret = oCfg.GetStrProp(
            propConfigPath, strPath );
        if( ERROR( ret ) )
            break;
        bool bFormat = false;
        oCfg.GetBoolProp( 0, bFormat );
        guint32 dwFlags = O_RDWR;
        if( bFormat )
            dwFlags |= O_CREAT;
        ret = open( strPath.c_str(),
            dwFlags, 00600 );
        if( ERROR( ret ) )
        {
            ret = -errno;
            break;
        }
        DebugPrint( ret,
            "Info: registry path %s",
            strPath.c_str() );
        m_iFd = ret;
        m_pSuperBlock.reset(
            new CSuperBlock( this ) );
    }while( 0 );

    if( ERROR( ret ) )
    {
        stdstr strMsg = DebugMsg( ret,
            "Error in CBlockAllocator ctor" );
        throw std::runtime_error( strMsg );
    }
}

CBlockAllocator::~CBlockAllocator()
{
    if( m_iFd >= 0 )
    {
        close( m_iFd );
        m_iFd = -1;
    }
}

gint32 CBlockAllocator::ReadWriteFile(
    char* pBuf, guint32 dwSize,
    guint64 qwOff, bool bRead )
{
    gint32 ret = 0;
    do{
        if( pBuf == nullptr )
        {
            ret = -EINVAL;
            break;
        }
        CStdRMutex oLock( this->GetLock() );
        ret = lseek( m_iFd, qwOff, SEEK_SET );
        if( ERROR( ret ) )
        {
            ret = -errno;
            break;
        }
        if( bRead )
        {
            ret = read( m_iFd, pBuf, dwSize );
        }
        else
        {
            ret = write( m_iFd, pBuf, dwSize );
        }
        if( ERROR( ret ) )
        {
            ret = -errno;
            break;
        }

    }while( 0 );
    return ret;
}

gint32 CBlockAllocator::SaveSuperBlock(
    const char* pSb, guint32 dwSize )
{
    gint32 ret = 0;
    if( dwSize < SUPER_BLOCK_SIZE )
        return -EINVAL;
    if( IsSafeMode() )
    {
        guint32 dwBlkIdx = ( guint32 )-2;
        ret = CacheBlocks(
            &dwBlkIdx, 1, ( guint8* )pSb ); 
        if( ERROR( ret ) )
            return ret;
        if( ret == 0 )
            ret = -ENODATA;
    }
    else
    {
        ret = ReadWriteFile(
            const_cast< char* >( pSb ),
            SUPER_BLOCK_SIZE, 0, false );
        if( ERROR( ret ) )
            return ret;
        if( ret == 0 )
            ret = -ENODATA;
    }
    return ret;
}

gint32 CBlockAllocator::LoadSuperBlock(
    char* pSb, guint32 dwSize )
{
    gint32 ret = 0;
    if( dwSize < SUPER_BLOCK_SIZE )
        return -EINVAL;
    if( IsSafeMode() )
    {
        guint32 dwBlkIdx = ( guint32 )-2;
        ret = ReadCache( &dwBlkIdx,
            1, ( guint8* )pSb );
    }
    else
    {
        ret = ReadWriteFile(
            pSb, SUPER_BLOCK_SIZE, 0, true );
        if( ERROR( ret ) )
            return ret;
        if( ret == 0 )
            ret = -ENODATA;
    }
    return ret;
}

gint32 CBlockAllocator::SaveGroupBitmap(
    const char* pbmp, guint32 dwSize )
{
    if( dwSize < GRPBMP_SIZE )
        return -EINVAL;
    gint32 ret = 0;
    if( IsSafeMode() )
    {
        guint32 dwBlkIdx = ( guint32 )-4;
        ret = CacheBlocks(
            &dwBlkIdx, GRPBMP_BLKNUM,
            ( guint8* )pbmp ); 
        if( ERROR( ret ) )
            return ret;
        if( ret == 0 )
            ret = -ENODATA;
    }
    else
    {
        ret = ReadWriteFile(
            const_cast< char* >( pbmp ), 
            GRPBMP_SIZE, GRPBMP_START, false );
        if( ERROR( ret ) )
            return ret;
        if( ret == 0 )
            ret = -ENODATA;
    }
    return ret;
}

gint32 CBlockAllocator::LoadGroupBitmap(
    char* pbmp, guint32 dwSize )
{
    if( dwSize < GRPBMP_SIZE )
        return -EINVAL;

    gint32 ret = 0;
    if( IsSafeMode() )
    {
        guint32 dwBlkIdx = ( guint32 )-4;
        ret = ReadCache( &dwBlkIdx, 1, 
           ( guint8* )pbmp );
    }
    else
    {
        ret = ReadWriteFile(
            pbmp, GRPBMP_SIZE,
            GRPBMP_START, true );
        if( ERROR( ret ) )
            return ret;
        if( ret == 0 )
            ret = -ENODATA;
    }
    return ret;
}

static guint32 dwTotalFree = 0;
gint32 CBlockAllocator::FreeBlocks(
    const guint32* pvecBlocks,
    guint32 dwNumBlocks,
    bool bByteOrder )
{
    if( pvecBlocks == nullptr ||
        dwNumBlocks == 0 )
        return -EINVAL;
    gint32 ret = 0;
    do{
        CStdRMutex oLock( GetLock() );
        dwTotalFree+= dwNumBlocks;
        // search within the cache
        guint32 dwBlocks = 0;
        guint32 dwRest = dwNumBlocks;
        std::vector< CONTBLKS > vecBlks;
        ret = FindContigousBlocks(
            pvecBlocks, dwNumBlocks, vecBlks,
            bByteOrder );
        if( ERROR( ret ) )
            break;
        for( auto& elem : vecBlks )
        {
            guint32 dwBlkIdx = elem.first;
            if( dwBlkIdx == 0 )
                continue;
            DECL_ARRAY2( guint32,
                arrIdxs, elem.second );
            for( int i = 0; i < elem.second; i++ )
                arrIdxs[ i ] = dwBlkIdx + i;
            guint32 dwGrpIdx = GROUP_INDEX( dwBlkIdx );
            CGroupBitmap* p = m_pGroupBitmap.get();
            gint32 iRet = 
                p->IsBlockGroupFree( dwGrpIdx );
            if( SUCCEEDED( iRet ) )
                continue;
            auto itr = m_mapBlkGrps.find( dwGrpIdx );
            if( itr == m_mapBlkGrps.end() )
            {
                BlkGrpUPtr pbg(
                    new CBlockGroup( this, dwGrpIdx ) );
                ret = pbg->Reload();
                if( ERROR( ret ) )
                    break;
                ret = pbg->FreeBlocks(
                    arrIdxs, elem.second );
                if( ERROR( ret ) )
                    break;
                m_mapBlkGrps.insert(
                    {dwGrpIdx, std::move( pbg )});
            }
            else
            {
                auto pbg = itr->second.get();
                ret = pbg->FreeBlocks(
                    arrIdxs, elem.second );
            }
        }
    }while( 0 );
    return ret;
}

gint32 CBlockAllocator::AllocBlocks( 
    guint32* pvecBlocks,
    guint32 dwNumBlocks )
{
    if( pvecBlocks == nullptr ||
        dwNumBlocks == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CStdRMutex oLock( GetLock() );
        // search within the cache
        guint32 dwBlocks = 0;
        guint32* pCurBlk = pvecBlocks;
        guint32 dwRest = dwNumBlocks;
        while( dwBlocks < dwNumBlocks )
        {
            for( auto& elem : m_mapBlkGrps )
            {
                guint32 dwFree =
                    elem.second->GetFreeCount();
                if( dwFree == 0 )
                    continue;
                guint32 dwCount = 
                    std::min( dwFree, dwRest );
                ret = elem.second->AllocBlocks(
                    pCurBlk, dwCount );
                if( ERROR( ret ) )
                    break;
                dwBlocks += dwCount;
                dwRest -= dwCount;
                pCurBlk += dwCount;
                if( dwRest == 0 )
                    break;
            }
            if( dwRest == 0 )
                break;

            guint32 dwGrps =
                m_pGroupBitmap->GetAllocCount();

            guint32 dwNeed = dwRest;
            if( dwGrps > m_mapBlkGrps.size() )
            {
                for( int i = 0; i < BLKGRP_NUMBER; i++ )
                {
                    gint32 iRet = IsBlockGroupFree( i );
                    if( SUCCEEDED( iRet ) )
                        continue;
                    auto itr = m_mapBlkGrps.find( i );
                    if( itr != m_mapBlkGrps.end() )
                        continue;
                    BlkGrpUPtr pbg(
                        new CBlockGroup( this, i ) );
                    ret = pbg->Reload();
                    if( ERROR( ret ) )
                        break;
                    if( pbg->GetFreeCount() < dwNeed )
                        dwNeed -= pbg->GetFreeCount();
                    else
                        dwNeed = 0;
                    m_mapBlkGrps.insert(
                        {i, std::move( pbg )} );
                    if( dwNeed == 0 )
                        break;
                    if( m_mapBlkGrps.size() < 
                        m_pGroupBitmap->GetAllocCount() )
                        continue;
                    break;
                }
            }

            // allocate new blockgroup till dwNeed is met
            while( dwNeed > 0 )
            {
                // all are loaded 
                guint32 dwGrpIdx;
                ret = m_pGroupBitmap->AllocGroup(
                    dwGrpIdx );
                if( ERROR( ret ) )
                    break;
                BlkGrpUPtr pbg(
                    new CBlockGroup( this, dwGrpIdx ) );
                ret = pbg->Format();
                if( ERROR( ret ) )
                    break;
                if( pbg->GetFreeCount() >= dwNeed )
                {
                    dwNeed = 0;
                    m_mapBlkGrps.insert(
                        {dwGrpIdx, std::move( pbg )} );
                    break;
                }
                dwNeed -= pbg->GetFreeCount();
                m_mapBlkGrps.insert(
                    { dwGrpIdx, std::move( pbg ) } );
                continue;
            }
            if( ERROR( ret ) )
                break;
        }

    }while( 0 );

    return ret;
}

gint32 CBlockAllocator::ReadBlock(
    guint32 dwBlockIdx, guint8* pBuf )
{
    if( dwBlockIdx == INVALID_BLOCK_IDX ||
        pBuf == nullptr )
        return -EINVAL;

    guint32 arrBlks[ 1 ] = { dwBlockIdx };
    return ReadWriteBlocks( true,
        arrBlks, 1, pBuf, false );
}

gint32 CBlockAllocator::WriteBlock(
    guint32 dwBlockIdx, const guint8* pBuf )
{
    if( dwBlockIdx == INVALID_BLOCK_IDX ||
        pBuf == nullptr )
        return -EINVAL;

    guint32 arrBlks[ 1 ] = { dwBlockIdx };
    return ReadWriteBlocks(
        false, arrBlks, 1,
        const_cast< guint8* >( pBuf ),
        false );
}

gint32 CBlockAllocator::WriteBlocks(
    const guint32* pBlocks,
    guint32 dwNumBlocks, const guint8* pBuf,
    bool bContigous )
{
    return ReadWriteBlocks( false,
        pBlocks, dwNumBlocks,
        const_cast< guint8* >( pBuf ),
        bContigous );
}

gint32 CBlockAllocator::ReadBlocks(
    const guint32* pBlocks,
    guint32 dwNumBlocks, guint8* pBuf,
    bool bContigous )
{
    return ReadWriteBlocks( true, pBlocks,
        dwNumBlocks, pBuf, bContigous );
}

gint32 CBlockAllocator::FindContigousBlocks(
    const guint32* pBlocks, guint32 dwNumBlocks,
    std::vector< CONTBLKS >& vecBlocks,
    bool bByteOrder )
{
    if( pBlocks == nullptr || dwNumBlocks == 0 )
        return -EINVAL;

    guint32 dwPrev = pBlocks[ 0 ];

    if( bByteOrder )
        dwPrev = ntohl( dwPrev );

    guint32 dwBase = dwPrev;
    guint32 dwCount = 1;

    for( guint32 i = 1; i < dwNumBlocks; i++ )
    {
        guint32 dwBlkIdx = pBlocks[ i ];
        if( bByteOrder )
            dwBlkIdx = ntohl( dwBlkIdx );
        if( dwBlkIdx == dwPrev + 1 &&
            ( dwBlkIdx & BLOCK_IDX_MASK ) != 0 )
        {
            dwCount++;
            dwPrev += 1;
        }
        else if( dwBlkIdx == INVALID_BLOCK_IDX &&
            dwBase == INVALID_BLOCK_IDX )
        {
            // a hole
            dwCount++;
            dwPrev = dwBlkIdx;   
        }
        else
        {
            vecBlocks.push_back(
                { dwBase, dwCount } );
            dwBase = dwPrev = dwBlkIdx;
            dwCount = 1;
        }
    }
    vecBlocks.push_back( { dwBase, dwCount } );
    return STATUS_SUCCESS;
}

gint32 CBlockAllocator::ReadWriteBlocks2(
    std::vector< CONTBLKS >& vecBlocks,
    guint8* pBuf, bool bRead )
{
    gint32 ret = 0;
    guint32 dwOff = 0;
    do{
        for( auto& elem : vecBlocks )
        {
            guint32 dwBlkIdx = elem.first;

            if( BLKIDX_BEYOND_MAX( dwBlkIdx ) )
            {
                ret = -ERANGE;
                break;
            }

            guint32 dwIdx =
                ( dwBlkIdx & BLOCK_IDX_MASK );
            guint64 off = ABS_ADDR(
                GROUP_START( dwBlkIdx ) +
                dwIdx * BLOCK_SIZE );
            size_t qs = BLOCK_SIZE * elem.second;
            if( qs > MAX_FILE_SIZE )
            {
                ret = -ERANGE;
                break;
            }
            if( dwBlkIdx == INVALID_BLOCK_IDX &&
                bRead )
            {
                memset( pBuf + dwOff, 0, qs );
                dwOff += qs;
                continue;
            }
            else if( dwBlkIdx == INVALID_BLOCK_IDX &&
                !bRead )
            {
                ret = -ENODATA;
                break;
            }
            ret = ReadWriteFile(
                ( char* )pBuf + dwOff,
                BLOCK_SIZE * elem.second,
                off, bRead );

            if( ERROR( ret ) )
                break;

            dwOff += qs;
        }
    }while( 0 );
    if( ERROR( ret ) )
        return ret;
    return dwOff;
}
    
gint32 CBlockAllocator::ReadWriteBlocks(
    bool bRead, const guint32* pBlocks,
    guint32 dwNumBlocks, guint8* pBuf,
    bool bContigous )
{
    if( pBlocks == nullptr ||
        pBuf == nullptr ||
        dwNumBlocks == 0 )
        return -EINVAL;
    gint32 ret = 0;
    do{
        CStdRMutex oLock( this->GetLock() );
        if( IsSafeMode() )
        {
            if( bRead )
                ret = ReadCache( pBlocks,
                    dwNumBlocks, pBuf, bContigous );
            else
                ret = CacheBlocks( pBlocks,
                    dwNumBlocks, pBuf, bContigous );
            break;
        }

        if( bContigous )
        {
            guint32 dwDist =
                pBlocks[ dwNumBlocks - 1 ] -
                pBlocks[ 0 ];
            if( dwDist + 1 != dwNumBlocks )
            {
                ret = -EINVAL;
                break;
            }
            const guint32* pLast =
                pBlocks + dwNumBlocks - 1;
            if( GROUP_INDEX( pBlocks[ 0 ] ) ==
                GROUP_INDEX( pLast[ 0 ] ) )
            {
                guint32 dwBlkIdx = pBlocks[ 0 ];
                guint32 dwIdx =
                    ( dwBlkIdx & BLOCK_IDX_MASK );
                guint64 off = ABS_ADDR(
                    GROUP_START( dwBlkIdx ) +
                    dwIdx * BLOCK_SIZE );
                ret = ReadWriteFile(
                    ( char* )pBuf,
                    BLOCK_SIZE * dwNumBlocks, 
                    off, bRead );
                break;
            }
            // spanning the block groups
        }
        std::vector< CONTBLKS > vecBlocks;
        ret = FindContigousBlocks(
            pBlocks, dwNumBlocks, vecBlocks );
        if( ERROR( ret ) )
            break;
        ret = ReadWriteBlocks2(
            vecBlocks, pBuf, bRead );
    }while( 0 );
    return ret;
}

gint32 CBlockAllocator::IsBlockFree(
    guint32 dwBlkIdx )
{
    gint32 ret = 0;
    do{
        CStdRMutex oLock( this->GetLock() );
        guint32 dwGrpIdx =
            GROUP_INDEX( dwBlkIdx );
        gint32 iRet = this->IsBlockGroupFree(
            dwGrpIdx );
        if( SUCCEEDED( iRet ) )
            break;
        if( iRet != ERROR_FALSE )
        {
            ret = -ERANGE;
            break;
        }
        guint32 dwIdx =
            ( dwBlkIdx & BLOCK_IDX_MASK );
        auto itr = m_mapBlkGrps.find( dwGrpIdx );
        if( itr != m_mapBlkGrps.end() )
        {
            CBlockGroup* pGrp = itr->second.get();
            if( pGrp == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            ret = pGrp->IsBlockFree( dwIdx );
        }
        else
        {
            BlkGrpUPtr pbg( new CBlockGroup(
                this, dwGrpIdx ) );
            ret = pbg->Reload();
            if( ERROR( ret ) )
                break;
            ret = pbg->IsBlockFree( dwIdx );
            m_mapBlkGrps.insert(
                { dwGrpIdx, std::move( pbg ) } );
        }

    }while( 0 );
    return ret;
}

gint32 CBlockAllocator::Format()
{
    gint32 ret = 0;
    do{
        CStdRMutex oLock( this->GetLock() );
        ret = ftruncate( m_iFd, 0 );
        if( ret < 0 )
        {
            ret = -errno;
            break;
        }
        this->m_pSuperBlock->Format();
        m_pGroupBitmap.reset(
            new CGroupBitmap( this ) );
        ret = m_pGroupBitmap->Format();
        if( ERROR( ret ) )
            break;
        guint32 dwGrpIdx = 0xFFFFFFFF;
        ret = m_pGroupBitmap->AllocGroup(
            dwGrpIdx );
        if( ERROR( ret ) )
            break;
        BlkGrpUPtr pbg(
            new CBlockGroup( this, dwGrpIdx ) );
        ret = pbg->Format();
        if( ERROR( ret ) )
            break;
        m_mapBlkGrps.insert(
            { 0, std::move( pbg ) } );
    }while( 0 );
    return ret;
}

gint32 CBlockAllocator::Flush( guint32 dwFlags )
{
    gint32 ret = 0;
    do{
        CStdRMutex oLock( this->GetLock() );
        ret = this->m_pSuperBlock->Flush( dwFlags );
        if( ERROR( ret ) )
            break;
        ret = m_pGroupBitmap->Flush( dwFlags );
        if( ERROR( ret ) )
            break;
        for( auto& elem : m_mapBlkGrps )
        {
            ret = elem.second->Flush( dwFlags );
            if( ERROR( ret ) )
                break;
        }
        if( ERROR( ret ) )
            break;
        ret = fsync( m_iFd );
        if( ret < 0 )
            ret = -errno;
    }while( 0 );
    return ret;
}

gint32 CBlockAllocator::Reload()
{
    gint32 ret = 0;
    do{
        CStdRMutex oLock( this->GetLock() );
        ret = this->m_pSuperBlock->Reload();
        if( ERROR( ret ) )
            break;
        m_pGroupBitmap.reset(
            new CGroupBitmap( this ) );
        ret = m_pGroupBitmap->Reload();
        if( ERROR( ret ) )
            break;

        CGroupBitmap* pgb = m_pGroupBitmap.get();
        for( int i = 0; i < BLKGRP_NUMBER; i++ )
        {
            gint32 iRet =
                pgb->IsBlockGroupFree( i );
            if( SUCCEEDED( iRet ) )
                continue;
            BlkGrpUPtr pbg(
                new CBlockGroup( this, i ) );
            ret = pbg->Reload();
            if( ERROR( ret ) )
                break;
            m_mapBlkGrps.insert(
                { i, std::move( pbg ) } );
        }
    }while( 0 );
    return ret;
}


#include <algorithm>

using INTERVAL=std::pair< guint32, guint32 >;
using INTERVALS=std::vector< CONTBLKS >;

// every interval is right-open [first, second)
// intervals1 is sorted and non-overlapping
// intervals2 is an ordered map and non-overlapping

INTERVALS IntersectIntervals(
    const INTERVALS& intervals1,
    const DirtyBlks& intervals2)
{
    INTERVALS result;
    guint32 i = 0, j = 0;
    guint32 n1 = intervals1.size();
    guint32 n2 = intervals2.size();
    if( n2 == 0 || n1 == 0 )
        return result;

    auto jter = intervals2.lower_bound(
        intervals1[i].first );
    if( jter != intervals2.begin() )
        jter--;

    while (i < n1 && jter != intervals2.end()) {
        guint32 start = std::max(
            intervals1[i].first, jter->first);

        guint32 end2 = jter->first +
            jter->second->size() / BLOCK_SIZE;

        guint32 end = std::min(
            intervals1[i].second, end2 );

        if (start < end) {
            guint32 dwBufOff = intervals1[i].dwOff;
            guint32 dwOff = (
                start == intervals1[i].first
                ?  dwBufOff : dwBufOff + 
                start - intervals1[i].first );
            result.emplace_back(
                start, end, dwOff, jter->first );
        }
        if (intervals1[i].second < end2 )
            ++i;
        else
            ++jter;
    }
    return result;
}

INTERVALS SubtractIntervals(
    const INTERVALS& intervals1,
    const INTERVALS& intervals2)
{
    INTERVALS result;
    guint32 i = 0, j = 0;
    guint32 n1 = intervals1.size();
    guint32 n2 = intervals2.size();
    if( n2 == 0 )
    {
        result.insert( result.end(),
            intervals1.begin(),
            intervals1.end());
        return result;
    }
    if( n1 == 0 )
        return result;

    while( i < n1 )
    {
        guint32 start1 = intervals1[i].first;
        guint32 end1 = intervals1[i].second;

        while( j < n2 && intervals2[j].second <= start1 )
            ++j;

        guint32 currentStart = start1;

        while( j < n2 && intervals2[j].first < end1 )
        {
            if( intervals2[j].first > currentStart )
            {
                result.emplace_back( currentStart,
                    intervals2[j].first,
                    intervals1[i].dwOff + currentStart -
                        intervals1[i].first );
            }
            currentStart = std::max(
                currentStart, intervals2[j].second);
            ++j;
        }

        if( currentStart < end1 )
        {
            result.emplace_back( currentStart, end1,
            intervals1[i].dwOff + currentStart -
                intervals1[i].first );
        }
        ++i;
    }
    return result;
}

gint32 CBlockAllocator::CacheBlocks(
    const guint32* pBlocks,
    guint32 dwNumBlocks, guint8* pBuf,
    bool bContigous )
{
    gint32 ret = 0;
    do{
        guint32 dwBytesWritten = 0;
        CStdRMutex oLock( this->GetLock() );
        if( m_mapDirtyBlks.empty() )
        {
            ret = clock_gettime(
                CLOCK_REALTIME, &m_oStartTime );
            if( ret < 0 )
            {
                ret = -errno;
                break;
            }
        }

        // handle the special cases
        if( *pBlocks == ( guint32 )-2 )
        {
            BufPtr pBufCopy( true );
            ret = pBufCopy->Append(
                pBuf, SUPER_BLOCK_SIZE );
            if( ERROR( ret ) )
                break;
            m_mapDirtyBlks[ *pBlocks ] = pBufCopy;
            dwBytesWritten += 512;
            ret = dwBytesWritten;
            continue;
        }
        else if( *pBlocks == ( guint32 )-4 )
        {
            BufPtr pBufCopy( true );
            ret = pBufCopy->Append(
                pBuf, GRPBMP_SIZE );
            if( ERROR( ret ) )
                break;
            m_mapDirtyBlks[ *pBlocks ] = pBufCopy;
            dwBytesWritten += 512;
            ret = dwBytesWritten;
            continue;
        }

        std::vector< CONTBLKS > vecBlks;
        if( !bContigous )
        {
            FindContigousBlocks( pBlocks,
                dwNumBlocks, vecBlks );
            guint32 dwOff = 0;
            for( auto& elem : vecBlks )
            {
                elem.third = dwOff;
                dwOff += elem.second;
                elem.second += elem.first;
                // block offset in the pBuf
            }
            std::sort( vecBlks.begin(), vecBlks.end(),
                ( []( const CONTBLKS& a1, const CONTBLKS& a2 )
                { return a1.dwBlkIdx <= a2.dwBlkIdx; } ) );
        }
        else
        {
            vecBlks.push_back(
                { *pBlocks, *pBlocks + dwNumBlocks } );
        }

        INTERVALS oCommonBlks = IntersectIntervals(
            vecBlks, this->m_mapDirtyBlks );
        INTERVALS oNewBlks = SubtractIntervals(
            vecBlks, oCommonBlks );
        for( auto& elem : oCommonBlks )
        {
            if( elem.first == INVALID_BLOCK_IDX )
            {
                OutputMsg(ret, "Cache internal error, "
                "aborting to avoid data corruption.");
                abort();
            };
            guint32 dwSize = BLOCK_SIZE *
                ( elem.second - elem.first );
            if( elem.first == ( guint32 )-2 ||
                elem.first == ( guint32 )-4 )
            {
                auto iter = m_mapDirtyBlks.find(
                    elem.first );
                if( iter == m_mapDirtyBlks.end() )
                    continue;
                memcpy( iter->second->ptr(),
                    pBuf + elem.dwOff * BLOCK_SIZE,
                    dwSize );
                dwBytesWritten += dwSize;
                continue;
            }
            auto iter = m_mapDirtyBlks.find(
                elem.dwKey );
            if( iter == m_mapDirtyBlks.end() )
                continue;

            guint32 dwOff = BLOCK_SIZE *
                ( elem.first - iter->first );
            if( dwOff > iter->second->size() )
                continue;

            char* pDst =
                iter->second->ptr() + dwOff;
            auto pSrc =
                pBuf + elem.dwOff * BLOCK_SIZE;
            memcpy( pDst, pSrc, dwSize );
            dwBytesWritten += dwSize;
        }

        for( auto& elem : oNewBlks )
        {
            guint32 dwSize = BLOCK_SIZE *
                ( elem.second - elem.first );
            BufPtr pBufNew( true );
            ret = pBufNew->Resize( dwSize );
            if( ERROR( ret ) )
                break;
            memcpy( pBufNew->ptr(),
                pBuf + elem.dwOff * BLOCK_SIZE,
                pBufNew->size() );
            m_mapDirtyBlks.emplace(
                elem.first, pBufNew );
            m_dwDirtyBlkCount +=
                ( dwSize >> BLOCK_SHIFT );
            dwBytesWritten += dwSize;
        }

        if( SUCCEEDED( ret ) )
            ret = dwBytesWritten;

    }while( 0 );
    return ret;
}

gint32 CBlockAllocator::ReadCache(
    const guint32* pBlocks,
    guint32 dwNumBlocks, guint8* pBuf,
    bool bContigous )
{
    gint32 ret = 0;
    do{
        CStdRMutex oLock( this->GetLock() );

        // handle the special cases
        if( *pBlocks == ( guint32 )-2 ||
            *pBlocks == ( guint32 )-4 )
        {
            auto iter = m_mapDirtyBlks.find( *pBlocks );
            if( iter != m_mapDirtyBlks.end() )
            {
                memcpy( pBuf,
                    iter->second->ptr(),
                    std::min( dwNumBlocks * BLOCK_SIZE,
                        iter->second->size() ) );
                break;
            }
            size_t dwOff = 0;
            if( *pBlocks == -4 )
                dwOff = SUPER_BLOCK_SIZE;
            ret = ReadWriteFile( ( char* )pBuf,
                BLOCK_SIZE * dwNumBlocks,
                dwOff, true );
            break;
        }

        std::vector< CONTBLKS > vecBlks;
        if( !bContigous )
        {
            FindContigousBlocks( pBlocks,
                dwNumBlocks, vecBlks );
            guint32 dwOff = 0;
            for( auto& elem : vecBlks )
            {
                // convert 'second' from block count
                // to 'tail block idx'
                elem.third = dwOff;
                dwOff += elem.second;
                elem.second += elem.first;
                // block offset in the pBuf
            }
            std::sort( vecBlks.begin(), vecBlks.end(),
                ( []( const CONTBLKS& a1,
                    const CONTBLKS& a2 )
                {
                    if( a1.first < a2.first )
                        return true;
                    else if( a1.first == a2.first )
                    {
                        if( a1.second < a2.second )
                            return true;
                        else if( a1.second == a2.second )
                        {
                            if( a1.third <= a2.third )
                                return true;
                            return false;
                        }
                        return false;
                    }
                    return false;
                } ) );

            auto itr = vecBlks.begin();
            while( itr != vecBlks.end() )
            {
                if( itr->first < INVALID_BLOCK_IDX )
                {
                    itr++;
                    continue;
                }
                if( itr->first == INVALID_BLOCK_IDX )
                {
                    // fill the holes with zero
                    guint32 dwBlocks = 
                        itr->second - itr->first;
                    memset( pBuf + itr->dwOff * BLOCK_SIZE,
                        0,  dwBlocks * BLOCK_SIZE );
                    itr = vecBlks.erase( itr );
                    continue;
                }
                break;
            }
        }
        else
        {
            vecBlks.push_back( { *pBlocks,
                *pBlocks + dwNumBlocks } );
        }

        INTERVALS oCommonBlks = IntersectIntervals(
            vecBlks, this->m_mapDirtyBlks );
        INTERVALS oNewBlks = SubtractIntervals(
            vecBlks, oCommonBlks );

        //cache hit
        guint32 dwBytesRead = 0;
        for( auto& elem : oCommonBlks )
        {
            auto iter = m_mapDirtyBlks.find(
                elem.dwKey );
            if( iter == m_mapDirtyBlks.end() )
                continue;
            guint32 dwOff = ( elem.first -
                iter->first ) * BLOCK_SIZE;
            if( dwOff > iter->second->size() )
                continue;
            guint32 dwSize = BLOCK_SIZE *
                ( elem.second - elem.first );
            memcpy( pBuf + elem.dwOff * BLOCK_SIZE,
                iter->second->ptr() + dwOff,
                dwSize );
            dwBytesRead += dwSize;
        }

        //cache missed
        for( auto& elem : oNewBlks )
        {
            guint32 dwSize = BLOCK_SIZE *
                ( elem.second - elem.first );

            guint64 qwOff =
                BLOCK_ABS( elem.first );

            ret = ReadWriteFile( (char* )pBuf +
                elem.dwOff * BLOCK_SIZE,
                dwSize, qwOff, true );
            if( ERROR( ret ) )
                break;
            dwBytesRead += dwSize;
        }

        if( SUCCEEDED( ret ) )
            ret = dwBytesRead;

    }while( 0 );
    return ret;
}

gint32 MergeBlocks(
    const DirtyBlks& mapBlocks,
    DirtyBlks& mergedBlocks )
{
    if (mapBlocks.empty())
        return 0;

    gint32 ret = 0;
    auto itr = mapBlocks.begin();
    guint32 currentBlockIndex = itr->first;

    // Copy the first buffer
    BufPtr currentBuffer = itr->second;
    for( ++itr; itr != mapBlocks.end(); ++itr )
    {
        guint32 nextBlockIndex = itr->first;
        guint32 currentBlockCount =
            currentBuffer->size() / BLOCK_SIZE;

        if( nextBlockIndex ==
            currentBlockIndex + currentBlockCount )
        {
            // Merge the buffers
            if( currentBuffer->size() +
                itr->second->size() <=
                MAX_BYTES_PER_BUFFER )
            {
                ret = currentBuffer->Append(
                    itr->second->ptr(),
                    itr->second->size());
                if( ERROR( ret ) )
                {
                    DebugPrint( ret,
                        "Error cannot append "
                        "buffer during merge" );
                    return ret;
                }
            }
            else
            {
                // CBuffer has an upper limit of 16MB
                mergedBlocks.emplace(
                    currentBlockIndex,
                    currentBuffer );

                // Start a new block
                currentBlockIndex = nextBlockIndex;
                currentBuffer = itr->second;
            }
        }
        else
        {
            // Add the current merged block to the
            // result
            mergedBlocks.emplace(
                currentBlockIndex,
                currentBuffer );

            // Start a new block
            currentBlockIndex = nextBlockIndex;
            currentBuffer = itr->second;
        }
    }

    // Add the last merged block to the result
    mergedBlocks.emplace(
        currentBlockIndex, currentBuffer );

    // DebugPrint( 0, "Merged blocks %d: ",
    //     mapBlocks.size() - mergedBlocks.size() );
    return ret;
}

#if __linux__ && (__KERNEL__ >= 501)
#include <liburing.h>
static gint32 WriteBlocksWithIoUring(
    int fd, const DirtyBlks& mapBlocks )
{
    struct io_uring ring;
    const int QUEUE_DEPTH = 64;
    gint32 ret = io_uring_queue_init(
        QUEUE_DEPTH, &ring, 0);
    if( ret < 0 )
    {
        DebugPrint( -errno, "io_uring_queue_init");
        return -errno;
    }

    gint32 iTotalBytes = 0;
    auto itr = mapBlocks.begin();
    do{
        guint32 dwQueued = 0;
        for( int i = 0; i < QUEUE_DEPTH &&
            itr != mapBlocks.end(); ++i, ++itr )
        {
            guint32 dwBlkIdx = itr->first;
            guint64 offset;
            if( unlikely( dwBlkIdx == -2 ) )
                offset = 0;
            else if( unlikely( dwBlkIdx == -4 ) )
                offset = SUPER_BLOCK_SIZE;
            else
                offset = BLOCK_ABS(dwBlkIdx);

            auto dwSize = itr->second->size();

            struct io_uring_sqe* sqe =
                io_uring_get_sqe(&ring);
            if (!sqe) {
                ret = -EAGAIN;
                DebugPrint( ret,
                    "Error failed to get sqe" );
                break;
            }

            ++dwQueued;
            io_uring_prep_write(sqe, fd,
                itr->second->ptr(), dwSize, offset);
            iTotalBytes += dwSize;
        }
        if( ERROR( ret ) )
            break;

        if( dwQueued == 0 )
            break;

        ret = io_uring_submit(&ring);
        if (ret < 0) {
            DebugPrint(
                -errno, "io_uring_submit");
            ret = -errno;
            break;
        }

        struct io_uring_cqe* cqe;
        for (size_t i = 0; i < dwQueued; ++i) {
            ret = io_uring_wait_cqe(&ring, &cqe);
            if (ret < 0) {
                DebugPrint( -errno,
                    "io_uring_wait_cqe");
                ret = -errno;
                break;
            }

            if (cqe->res < 0) {
                DebugPrint( cqe->res,
                    "I/O error: %s\n",
                    strerror(-cqe->res));
                io_uring_cqe_seen(&ring, cqe);
                ret = cqe->res;
                break;
            }
            io_uring_cqe_seen(&ring, cqe);
        }
        if( ERROR( ret ) )
            break;
    }while( itr != mapBlocks.end() );

    io_uring_queue_exit(&ring);
    if( ERROR( ret ) )
        return ret;
    return iTotalBytes; // Return total bytes processed
}
#endif

gint32 CBlockAllocator::MergeBlocks()
{
    gint32 ret = 0;
    do{
        CStdRMutex oLock( this->GetLock() );
        DirtyBlks mergedBlocks;
        ret = rpcf::MergeBlocks(
            m_mapDirtyBlks, mergedBlocks );
        if( ERROR( ret ) )
        {
            DebugPrint( ret,
                "Error merging blocks" );
            break;
        }
        this->m_mapDirtyBlks =
            std::move( mergedBlocks );
    } while (0);
    return ret; 
}

gint32 CBlockAllocator::CommitCache()
{
    gint32 ret = 0;
    do{
        CStdRMutex oLock( this->GetLock() );
        DebugPrint( 0, "Committing %d dirty blocks...",
            m_dwDirtyBlkCount );
#if __linux__ && (__KERNEL__ >= 501)
        ret = WriteBlocksWithIoUring(
            m_iFd, m_mapDirtyBlks );
        if( ERROR( ret ) )
            break;
#else
        for( auto& elem : m_mapDirtyBlks )
        {
            guint64 qwOff;
            if( elem.first == -2 )
                qwOff = 0;
            else if( elem.first == -4 )
                qwOff = SUPER_BLOCK_SIZE;
            else
                qwOff = BLOCK_ABS( elem.first );
            ret = ReadWriteFile( elem.second->ptr(),
                elem.second->size(), qwOff, false );
            if( ERROR( ret ) )
                break;
            ret = 0;
        }
#endif
        if( SUCCEEDED( ret ) )
        {
            if( IsStopped() )
            {
                OutputMsg(m_mapDirtyBlks.size(),
                    "Info successfully committed %d"
                    "blocks ", m_dwDirtyBlkCount );
            }
            m_mapDirtyBlks.clear();
            m_dwDirtyBlkCount = 0;
            m_dwCacheAge = 0;
        }
        else
        {
            OutputMsg(ret, "Error committing block "
                "cache, aborting to avoid data "
                "corruption.");
            abort();
        }
    }while( 0 );
    return ret;
}

gint32 CBlockAllocator::CheckAndCommit( bool bEntering )
{
    gint32 ret = 0;
    do{
        // already locked here
        if( m_dwTransCount != 0 )
        {
            ret = -EAGAIN;
            break;
        }
        if( m_mapDirtyBlks.empty() )
            break;

        bool bCommit = false;
        if( unlikely( IsStopped() ) )
            bCommit = true;
        else if( m_dwDirtyBlkCount >=
            CACHE_MAX_BLOCKS )
            bCommit = true;
        else
        {
            timespec oNow;
            ret = clock_gettime(
                CLOCK_REALTIME, &oNow );
            if( ret < 0 )
            {
                ret = -errno;
                break;
            }
            if( oNow.tv_sec - m_oStartTime.tv_sec >=
                GetCacheLifeCycle() && bEntering )
                bCommit = true;
        }

        if( !bCommit )
        {
            ret = -EAGAIN;
            break;
        }

        ret = CommitCache();

    }while( 0 );
    return ret;
}

}
