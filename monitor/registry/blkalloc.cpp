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

namespace rpcf{

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

gint32 CSuperBlock::Flush()
{
    gint32 ret = 0;
    do{
        char arrSb[ SUPER_BLOCK_SIZE ];
        memset( arrSb, 0, sizeof( arrSb ) );
        guint32* p = ( guint32* )arrSb;

        p[ 0 ] = m_dwMagic;
        p[ 1 ] = htonl( m_dwVersion );
        p[ 2 ] = htonl( m_dwBlkSize );
        p[ 3 ] = htonl( m_dwGroupNum );
        ret = m_pAlloc->SaveSuperBlock(
            arrSb, sizeof( arrSb ) );
    }while( 0 );
    return ret;
}

gint32 CSuperBlock::Format()
{
    gint32 ret = 0;
    do{
        char arrSb[ SUPER_BLOCK_SIZE ];
        memset( arrSb, 0, sizeof( arrSb ) );
        guint32* p = ( guint32* )arrSb;

        p[ 0 ] = m_dwMagic;
        p[ 1 ] = htonl( m_dwVersion );
        p[ 2 ] = htonl( m_dwBlkSize );
        p[ 3 ] = htonl( m_dwGroupNum );
        ret = m_pAlloc->SaveSuperBlock(
            arrSb, sizeof( arrSb ) );
        SetBlockSize( m_dwBlkSize );
    }while( 0 );
    return ret;
}

gint32 CSuperBlock::Reload()
{
    gint32 ret = 0;
    do{
        char arrSb[ SUPER_BLOCK_SIZE ];
        ret = m_pAlloc->LoadSuperBlock(
            arrSb, sizeof( arrSb ) );
        if( ERROR( ret ) )
            break;

        if( arrSb[ 0 ] != 'r' ||
            arrSb[ 1 ] != 'e' ||
            arrSb[ 2 ] != 'g' ||
            arrSb[ 3 ] != 'i' )
        {
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
        ret = SetBlockSize( ntohl( p[ 2 ] ) );
        if( ERROR( ret ) )
            break;
        m_dwBlkSize = GetBlockSize();
        m_dwGroupNum = ntohl( p[ 3 ] );
        if( m_dwGroupNum > BLKGRP_NUMBER )
            ret = -ENOTSUP;
    }while( 0 );
    return ret;
}

gint32 CGroupBitmap::FreeGroup( guint32 dwGrpIdx )
{
    gint32 ret = 0;
    do{
        if( m_wFreeCount == BLKGRP_NUMBER )
            break;
        LONGWORD* pbmp = m_arrBytes;

        constexpr int iNumBits =
            sizeof( LONGWORD ) * BYTE_BITS;

        if( dwGrpIdx >= BLKGRP_NUMBER )
        {
            ret = -EINVAL;
            break;
        }

        guint32 shift = find_shift( iNumBits );
        guint32 dwWordIdx = ( dwGrpIdx >> shift );
        int offset = dwGrpIdx & ( iNumBits - 1 );

#if BUILD_64
        LONGWORD a = ntohll( pbmp[ dwWordIdx ] );
#else
        LONGWORD a = ntohl( pbmp[ dwWordIdx ] );
#endif
        if( a == 0 )
            break;

        LONGWORD bit = ( 1 << offset );
        a &= ( ~bit );
#if BUILD_64
        pbmp[ dwWordIdx ] = htonll( a );
#else
        pbmp[ dwWordIdx ] = htonl( a );
#endif
        m_wFreeCount++;

    }while( 0 );

    return ret;
}

gint32 CGroupBitmap::AllocGroup( guint32& dwGrpIdx )
{
    if( m_wFreeCount == 0 )
        return -ENOMEM;
    gint32 ret = 0;
    LONGWORD* pbmp = m_arrBytes;

    constexpr int count =
        sizeof( m_arrBytes )/sizeof( LONGWORD );

    bool bFound = false;
    for( int i = 0; i < count; i++ )
    {
#if BUILD_64
        LONGWORD a = ntohll( *pbmp );
#else
        LONGWORD a = ntohl( *pbmp );
#endif
        constexpr int iNumBits =
            sizeof( LONGWORD ) * BYTE_BITS;
        if( i < count - 1 )
        {
            if( a == ULONG_MAX )
            {
                pbmp++;
                continue;
            }
            LONGWORD bit = 1;
            for( int j = 0; j < iNumBits; j++ )
            {
                if( ( a & bit ) == 0 )
                {
                    dwGrpIdx = i * iNumBits + j;
                    bFound = true;
                    a |= bit;
#if BUILD_64
                    *pbmp = htonll( a );
#else
                    *pbmp = htonl( a );
#endif
                    break;
                }
                bit <<= 1;
            }
            break;
        }
        a &= ( ULONG_MAX >> 16 );
        if( a == ( ULONG_MAX >> 16 )
            break;
        guint64 bit = 1;
        for( int j = 0; j < iNumBits - 16; j++ )
        {
            if( ( a & bit ) == 0 )
            {
                dwGrpIdx = i * iNumBits + j;
                bFound = true;
                a |= bit;
#if BUILD_64
                *pbmp = htonll( a );
#else
                *pbmp = htonl( a );
#endif
                break;
            }
            bit <<= 1;
        }
    }
    if( bFound )
    {
        m_wFreeCount--;
        return STATUS_SUCCESS;
    }
    return -ENOMEM;
}

gint32 CGroupBitmap::InitBitmap()
{
    constexpr guint32 dwOffset =
        sizeof( m_arrBytes ) - sizeof( guint16 );
    memset( m_arrBytes, 0, dwOffset );
    guint16* pbmp = &m_arrBytes;
    m_wFreeCount = BLKGRP_NUMBER;
    pbmp[ dwOffset ] = htons( m_wFreeCount );
    return 0;
}

gint32 CGroupBitmap::IsBlockGroupFree(
    guint32 dwGrpIdx ) const
{
    gint32 ret = STATUS_SUCCESS;
    do{
        if( m_wFreeCount == BLKGRP_NUMBER )
            break;

        LONGWORD* pbmp = m_arrBytes;

        constexpr int iNumBits =
            sizeof( LONGWORD ) * BYTE_BITS;

        if( dwGrpIdx >= BLKGRP_NUMBER )
        {
            ret = -EINVAL;
            break;
        }

        guint32 shift = find_shift( iNumBits );
        guint32 dwWordIdx = ( dwGrpIdx >> shift );
        int offset = dwGrpIdx & ( iNumBits - 1 );

#if BUILD_64
        LONGWORD a = ntohll( pbmp[ dwWordIdx ] );
#else
        LONGWORD a = ntohl( pbmp[ dwWordIdx ] );
#endif
        if( a == 0 )
            break;

        LONGWORD bit = ( 1 << offset );
        if( a & bit )
            ret = ERROR_FALSE;

    }while( 0 );

    return ret;
}

gint32 CGroupBitmap::Flush()
{
    gint32 ret = 0;
    do{
        guint16* p = ( guint16*)
            ( m_arrBytes + sizeof( m_arrBytes );
        p[ -1 ] = htons( m_wFreeCount );
        ret = m_pAlloc->SaveGroupBitmap(
            m_arrBytes, sizeof( m_arrBytes ) );

    }while( 0 );
    return ret;
}
gint32 CGroupBitmap::Format()
{
    gint32 ret = 0;
    do{
        m_wFreeCount = BLKGRP_NUMBER;
        memset( m_arrBytes, 0, sizeof( m_arrBytes ) );
        guint16* p = ( guint16*)
            ( m_arrBytes + sizeof( m_arrBytes );
        p[ -1 ] = htons(  m_wFreeCount );
        ret = m_pAlloc->SaveGroupBitmap(
            m_arrBytes, sizeof( m_arrBytes ) );
    }while( 0 );
    return ret;
}

gint32 CGroupBitmap::Reload()
{
    gint32 ret = 0;
    do{
        ret = m_pAlloc->LoadGroupBitmap(
            m_arrBytes, sizeof( m_arrBytes ) );
        if( ERROR( ret ) )
            break;
        guint16* p = ( guint16*)
            ( m_arrBytes + sizeof( m_arrBytes );
        m_wFreeCount = htons( p[ -1 ] );
        if( m_wFreeCount > BLKGRP_NUMBER )
        {
            ret = -ERANGE;
            break;
        }
    }while( 0 );
    return ret;
}

gint32 CBlockGroup::Flush()
{
    gint32 ret = 0;
    do{
        guint32 dwBlkIdx =
            ( m_dwGroupIdx << GROUP_INDEX_SHIFT );
        guint32 arrBlks[ BLKBMP_BLKNUM ];
        for( int i = 0; i < BLKBMP_BLKNUM; i++ )
            arrBlks[ i ] = dwBlkIdx + i;
        ret = pAlloc->WriteBlocks(
            arrBlks, sizeof( arrBlks ),
            m_pBlockBitmap->m_arrBytes, true );
    }while( 0 );
    return ret;
}

gint32 CBlockGroup::Format()
{
    gint32 ret = 0;
    do{
        m_pBlockBitmap->InitBitmap();
        ret = Flush();
    }while( 0 );
    return ret;
}

gint32 CBlockGroup::Reload()
{
    gint32 ret = 0;
    do{
        guint32 dwBlkIdx =
            ( m_dwGroupIdx << GROUP_INDEX_SHIFT );
        guint32 arrBlks[ BLKBMP_BLKNUM ];
        for( int i = 0; i < BLKBMP_BLKNUM; i++ )
            arrBlks[ i ] = dwBlkIdx + i;
        ret = pAlloc->ReadBlocks(
            arrBlks, sizeof( arrBlks ),
            m_pBlockBitmap->m_arrBytes, true );
    }while( 0 );
    return ret;
}

gint32 CBlockGroup::IsBlockFree(
    guint32 dwBlkIdx )
{
    gint32 ret = 0;
    if( dwBlkIdx == 0 )
        return -EINVAL;
    do{
        if( m_wFreeCount ==
            BLOCKS_PER_GROUP - BLKBMP_BLKNUM )
            break;

        LONGWORD* pbmp = m_arrBytes;
        constexpr int iNumBits =
            sizeof( LONGWORD ) * BYTE_BITS;

        guint32 shift = find_shift( iNumBits );
        guint32 dwWordIdx =
            ( dwBlkIdx >> shift );

        int offset =
            dwBlkIdx & ( iNumBits - 1 );

#if BUILD_64
        LONGWORD a =
            ntohll( pbmp[ dwWordIdx ] );
#else
        LONGWORD a =
            ntohl( pbmp[ dwWordIdx ] );
#endif
        if( a == 0 )
            continue;

        LONGWORD bit = ( 1 << offset );
        if( a & bit )
            ret = ERROR_FALSE;

    }while( 0 );

    return ret;
}

gint32 CBlockBitmap::InitBitmap()
{
    constexpr guint32 dwOffset =
        sizeof( m_arrBytes ) - sizeof( guint16 );
    memset( m_arrBytes, 0, dwOffset );
    guint16* pbmp = &m_arrBytes;
    gint32 dwBytes = BLKBMP_BLKNUM / 8;
    for( int i = 0; i < dwBytes; i++ )
        pbmp[ i ] = 0xff;

    for( int i = 0; i < ( BLKBMP_BLKNUM & 0x07 ); i++ )
        pbmp[ dwBytes ] |= ( 1 << i );

    m_wFreeCount = BLOCKS_PER_GROUP - BLKBMP_BLKNUM;
    pbmp[ dwOffset ] = htons( m_wFreeCount );
    return 0;
}

gint32 CBlockBitmap::AllocBlocks(
    guint32* pvecBlocks,
    guint32& dwNumBlocks )
{
    if( m_wFreeCount == 0 )
        return -ENOMEM;

    if( pvecBlocks == nullptr ||
        dwNumBlocks == 0 )
        return -EINVAL;
    
    gint32 ret = 0;
    guint32 dwBlocks = 0;

    do{
        LONGWORD* pbmp = m_arrBytes;

        constexpr int count =
            sizeof( m_arrBytes )/sizeof( LONGWORD );

        bool bFull = false;
        guint32 dwBlkIdx = 0;
        for( int i = 0;
            i < count && dwBlocks < dwNumBlocks;
            i++ )
        {
#if BUILD_64
            LONGWORD a = ntohll( *pbmp );
#else
            LONGWORD a = ntohl( *pbmp );
#endif
            LONGWORD bit = 1;
            constexpr int iNumBits =
                sizeof( LONGWORD ) * BYTE_BITS;
            if( i < count - 1 )
            {
                if( a == ULONG_MAX )
                {
                    pbmp++;
                    continue;
                }
                for( int j = 0; j < iNumBits; j++ )
                {
                    if( ( a & bit ) == 0 )
                    {
                        dwBlkIdx = i * iNumBits + j;
                        pvecBlocks[ dwBlocks ] =
                            dwBlkIdx;
                        dwBlocks++;
                        a |= bit;
                        if( dwBlocks == dwNumBlocks )
                        {
                            bFull = true;
                            break;
                        }
                    }
                    bit <<= 1;
                }
#if BUILD_64
                *pbmp = htonll( a );
#else
                *pbmp = htonl( a );
#endif
                if( bFull )
                    break;
                pbmp++;
                continue;
            }

            a &= ( ULONG_MAX >> 16 );
            if( a == ( ULONG_MAX >> 16 )
                break;
            for( int j = 0; j < iNumBits - 16; j++ )
            {
                if( ( a & bit ) == 0 )
                {
                    dwBlkIdx = i * iNumBits + j;
                    pvecBlocks[ dwBlocks ] =
                        dwBlkIdx;
                    dwBlocks++;
                    a |= bit;
                    if( dwBlocks == dwNumBlocks )
                    {
                        bFull = true;
                        break;
                    }
                }
                bit <<= 1;
            }
#if BUILD_64
            *pbmp = htonll( a );
#else
            *pbmp = htonl( a );
#endif
        }

    }while( 0 )

    if( bFull )
    {
        m_wFreeCount -= dwNumBlocks;
        return STATUS_SUCCESS;
    }
    else if( dwBlocks < dwNumBlocks )
    {
        dwNumBlocks -= dwBlocks;
        m_wFreeCount -= dwNumBlocks;
        return STATUS_MORE_PROCESS_NEEDED;
    }
    return -ENOMEM;
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

        LONGWORD* pbmp = m_arrBytes;
        constexpr int iNumBits =
            sizeof( LONGWORD ) * BYTE_BITS;

        for( int i = 0; i < dwNumBlocks; i++ )
        {
            guint32 dwBlkIdx =
            ( pvecBlocks[ i ] & BLOCK_IDX_MASK );
            if( dwBlkIdx == 0 )
                continue;

            guint32 shift = find_shift( iNumBits );
            guint32 dwWordIdx =
                ( dwBlkIdx >> shift );
            int offset =
                dwBlkIdx & ( iNumBits - 1 );
#if BUILD_64
            LONGWORD a =
                ntohll( pbmp[ dwWordIdx ] );
#else
            LONGWORD a =
                ntohl( pbmp[ dwWordIdx ] );
#endif
            if( a == 0 )
                continue;

            LONGWORD bit = ( 1 << offset );
            a &= ( ~bit );
#if BUILD_64
            pbmp[ dwWordIdx ] = htonll( a );
#else
            pbmp[ dwWordIdx ] = htonl( a );
#endif
            m_wFreeCount++;
        }

    }while( 0 );

    return ret;
}

gint32 CBlockBitmap::Flush()
{
    gint32 ret = 0;
    do{
        guint32 dwBlockIdx =
            ( m_dwGroupIdx << GROUP_SHIFT );
        guint32 arrBlks[ BLKBMP_BLKNUM ];
        for( int i = 0; i < sizeof( arrBlks ); i++ )
            arrBlks[ i ] = dwBlockIdx + i;
        guint16* p = ( guint16* )
            m_arrBytes + sizeof( m_arrBytes );
        p[ -1 ] = htons( m_wFreeCount );
        ret = m_pAlloc->WriteBlocks(
            arrBlks, sizeof( arrBlks ),
            m_arrBytes, true );
        if( ERROR( ret ) )
            break;
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
            ( m_dwGroupIdx << GROUP_SHIFT );
        guint32 arrBlks[ BLKBMP_BLKNUM ];
        for( int i = 0; i < sizeof( arrBlks ); i++ )
            arrBlks[ i ] = dwBlockIdx + i;
        ret = m_pAlloc->ReadBlocks(
            arrBlks, sizeof( arrBlks ),
            m_arrBytes, true );
        if( ERROR( ret ) )
            break;
        guint16* p = ( guint16* )
            &m_arrBytes[sizeof( m_arrBytes )];
        m_wFreeCount = ntohs( p[ -1 ] );
    }while( 0 );

    return ret;
}

CBlockAllocator::CBlockAllocator(
    const IConfigDb* pCfg )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg( pCfg );
        ret = oCfg.GetStrProp(
            propConfigPath, strPath );
        if( ERROR( ret ) )
            break;
        ret = open( strPath.c_str(),
            O_CREAT | O_RDWR,
            00600 );
        if( ERROR( ret ) )
        {
            ret = -errno;
            break;
        }
        m_iFd = ret;
        m_pSuperBlock.reset(
            new CSuperBlock( this ) );
    }while( 0 );

    if( ERROR( ret ) )
    {
        string strMsg = DebugMsg( ret,
            "Error in CBlockAllocator ctor" );
        throw std::runtime_error( strMsg );
    }
    return ret;
}

gint32 CBlockAllocator::ReadWriteFile(
    char* pBuf, guint32 dwSize,
    guint32 dwOff, bool bRead )
{
    gint32 ret = 0;
    do{
        if( pBuf == nullptr )
        {
            ret = -EINVAL;
            break;
        }
        CStdRMutex oLock( this->GetLock() );
        ret = lseek( m_iFd, dwOff, SEEK_SET );
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
    return ReadWriteFile(
        const_cast< char* >( pSb ),
        SUPER_BLOCK_SIZE, 0, false );
}

gint32 CBlockAllocator::LoadSuperBlock(
    char* pSb, guint32 dwSize )
{
    gint32 ret = 0;
    if( dwSize < SUPER_BLOCK_SIZE )
        return -EINVAL;
    return ReadWriteFile(
        pSb, SUPER_BLOCK_SIZE, 0, true );
}

gint32 CBlockAllocator::SaveGroupBitmap(
    const* pbmp, guint32 dwSize );
{
    if( dwSize < GRPBMP_SIZE_FULL )
        return -EINVAL;

    gint32 ret = ReadWriteFile(
        pbmp, GRPBMP_SIZE_FULL,
        GRPBMP_START, false );
    return ret;
}

gint32 CBlockAllocator::LoadGroupBitmap(
    char* pbmp, guint32 dwSize );
{
    if( dwSize < GRPBMP_SIZE_FULL )
        return -EINVAL;

    gint32 ret = ReadWriteFile(
        pbmp, GRPBMP_SIZE_FULL,
        GRPBMP_START, true );
    return ret;
}

gint32 CBlockAllocator::FreeBlocks(
    const guint32* pvecBlocks,
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
        gutin32 dwRest = dwNumBlocks;
        for( guint32 i = 0; i < dwNumBlocks; i++ )
        {
            guint32 dwBlkIdx = pvecBlocks[ i ];
            guint32 dwGrpIdx = GROUP_INDEX( dwBlkIdx );
            CGroupBitmap* p = m_pGroupBitmap;
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
                guint32 arrBlks[ 1 ] = { dwBlkIdx };
                ret = pbg->FreeBlocks( arrBlks, 1 );
                if( ERROR( ret ) )
                    break;
                m_mapBlkGrps.insert(
                    dwGrpIdx, std::move( pbg ) );
            }
        }
    }while( 0 );
    return ret;
}

gint32 CBlockAllocator::AllocBlocks( 
    guint32* pvecBlocks,
    guint32& dwNumBlocks )
{
    if( pvecBlocks == nullptr ||
        dwNumBlocks == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CStdRMutex oLock( GetLock() );
        // search within the cache
        guint32 dwBlocks = 0;
        guint32 pCurBlk = pvecBlocks;
        gutin32 dwRest = dwNumBlocks;
        while( dwBlocks < dwNumBlocks )
        {
            for( auto elem : m_mapBlkGrps )
            {
                guint32 dwFree =
                    elem->GetFreeCount();
                if( dwFree == 0 )
                    continue;
                guint32 dwCount = 
                    std::min( dwFree, dwRest );
                ret = elem->AllocBlocks(
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
                    if( itr != m_mapBlkGrps.end )
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
                        i, std::move( pbg ) );
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
                ret = m_pGroupBitmap.AllocGroup(
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
                        dwGrpIdx, std::move( pbg ) );
                    break;
                }
                dwNeed -= pbg->GetFreeCount();
                m_mapBlkGrps.insert(
                    dwGrpIdx, std::move( pbg ) );
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
    if( dwBlockIdx == 0 || pBuf == nullptr )
        return nullptr;

    guint32 arrBlks[ 1 ] = { dwBlockIdx };
    return ReadWriteBlocks( true,
        arrBlks, 1, pBuf, false );
}

gint32 CBlockAllocator::WriteBlock(
    guint32 dwBlockIdx, const guint8* pBuf )
{
    if( dwBlockIdx == 0 || pBuf == nullptr )
        return nullptr;

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
    std::vector< CONTBLKS >& vecBlocks )
{
    if( pBlocks == nullptr || dwNumBlocks == 0 )
        return -EINVAL;
    guint32 dwPrev = pBlocks[ 0 ];
    guint32 dwBase = dwPrev;
    guint32 dwCount = 1;
    for( guint32 i = 1; i < dwNumBlocks - 1; i++ )
    {
        if( pBlocks[ i ] == dwPrev + 1 &&
            ( pBlocks[ i ] & BLOCK_IDX_MASK ) != 0 )
        {
            dwCount++;
            dwPrev += 1;
        }
        else if( pBlocks[ i ] == 0 && dwBase == 0 )
        {
            // a hole
            dwCount++;
            dwPrev = pBlocks[ i ];   
        }
        else
        {
            vecBlocks.push_back( { dwBase, dwCount } );
            dwBase = dwPrev = pBlocks[ i ];
            dwCount = 1;
        }
    }
    vecBlocks.push_back( { dwBase, dwCount } );
    return STATUS_SUCCESS;
}

gint32 CBlockAllocator::ReadWriteBlocks2(
    std::vector< CONTBLKS >& vecBlocks,
    guint8* pBuf,
    bool bRead )
{
    gint32 ret = 0;
    do{
        guint32 dwOffInBuf = 0;
        for( auto& elem : vecBlocks )
        {
            guint32 dwBlkIdx = elem.first;
            guint32 dwIdx =
                ( dwBlkIdx & BLOCK_IDX_MASK );
            guint64 off = ABS_ADDR(
                GROUP_START( dwBlkIdx ) +
                dwIdx * BLOCK_SIZE );
            if( dwBlkIdx == 0 && bRead )
            {
                memset( pBuf + dwOffInBuf, 0,
                    BLOCK_SIZE * elem.second );
                dwOffInBuf +=
                    BLOCK_SIZE * elem.second;
                continue;
            }
            else if( dwBlkIdx == 0 && bWrite )
            {
                ret = -ENOSPC;
                break;
            }
            ret = ReadWriteFile(
                pBuf + dwOffInBuf,
                BLOCK_SIZE * elem.second,
                off, bRead );

            if( ERROR( ret ) )
                break;

            dwOffInBuf +=
                BLOCK_SIZE * elem.second;
        }
    }while( 0 );
    return ret;
}
    
gint32 CBlockAllocator::ReadWriteBlocks(
    bool bRead, const guint32* pBlocks,
    guint32 dwNumBlocks, guint8* pBuf,
    bool bContigous )
{
    if( pBlocks == nullptr ||
        pBuf == nullptr ||
        dwNumBlocks = 0 )
        return -EINVAL;
    gint32 ret = 0;
    do{
        CStdRMutex oLock( this->GetLock() );
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
                GROUP_INDEX( pLast[ 0 ] )
            {
                guint32 dwBlkIdx = pBlocks[ 0 ];
                guint32 dwIdx =
                    ( dwBlkIdx & BLOCK_IDX_MASK );
                guint64 off = ABS_ADDR(
                    GROUP_START( dwBlkIdx ) +
                    dwIdx * BLOCK_SIZE );
                ret = ReadWriteFile( pBuf,
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
    guint32 dwBlkIdx ) const
{
    gint32 ret = 0;
    do{
        CStdRMutex oLock( this->GetLock() );
        guint32 dwGrpIdx =
            GROUP_INDEX( dwBlkIdx );
        gint32 iRet = this->IsBlockGroupFree();
        if( SUCCEEDED( iRet ) )
        {
            ret = -ERANGE;
            break;
        }
        auto itr = m_mapBlkGrps.find( dwGrpIdx );
        if( itr != m_mapBlkGrps.end() )
        {
            CBlockGroup* pGrp = itr->second.get();
            if( pGrp == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            ret = pGrp->IsBlockFree( dwBlkIdx );
        }
        else
        {
            BlkGrpUPtr pbg(
                new CBlockGroup( this, dwGrpIdx ) );
            ret = pbg->Reload();
            if( ERROR( ret ) )
                break;
            ret = pGrp->IsBlockFree( dwBlkIdx );
            m_mapBlkGrps.insert( dwGrpIdx,
                std::move( pbg ) );
        }

    }while( 0 );
    return ret;
}

gint32 CBlockAllocator::Format()
{
    gint32 ret = 0;
    do{
        CStdRMutex oLock( this->GetLock() );
        ret = truncate( m_iFd, 0 );
        if( ret < 0 )
        {
            ret = -errno;
            break;
        }
        this->m_pSuperBlock->Format();
        m_pGroupBitmap.reset( new CGroupBitmap() );
        ret = m_pGroupBitmap->Format();
        if( ERROR( ret ) )
            break;
        BlkGrpUPtr pbg(
            new CBlockGroup( this, 0 ) );
        ret = pbg->Format();
        if( ERROR( ret ) )
            break;
        m_mapBlkGrps.insert(
            0, std::move( pbg ) );
    }while( 0 );
    return ret;
}

gint32 CBlockAllocator::Flush()
{
    gint32 ret = 0;
    do{
        CStdRMutex oLock( this->GetLock() );
        ret = this->m_pSuperBlock->Flush();
        if( ERROR( ret ) )
            break;
        ret = m_pGroupBitmap->Flush();
        if( ERROR( ret ) )
            break;
        for( auto elem : m_mapBlkGrps )
        {
            ret = elem.second->Flush();
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
        m_pGroupBitmap.reset( new CGroupBitmap() );
        ret = m_pGroupBitmap->Reload();
        if( ERROR( ret ) )
            break;
        BlkGrpUPtr pbg(
            new CBlockGroup( this, 0 ) );
        ret = pbg->Reload();
        if( ERROR( ret ) )
            break;
        m_mapBlkGrps.insert(
            0, std::move( pbg ) );
    }while( 0 );
    return ret;
}

}
