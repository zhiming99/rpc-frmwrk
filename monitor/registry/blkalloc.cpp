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
        ret = m_pParent->SaveSuperBlock(
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
        ret = m_pParent->SaveSuperBlock(
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
        ret = m_pParent->LoadSuperBlock(
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

gint32 CBlockBitmap::AllocBlocks(
    guint32* pvecBlocks, guint32& dwNumBlocks )
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
        for( int i = 0; i < count || dwBlocks == dwNumBlocks; i++ )
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

gint32 CBlockBitmap::FreeBlocks(
    const guint32* pvecBlocks,
    guint32 dwNumBlocks )
{
    gint32 ret = 0;
    if( pvecBlocks == nullptr || dwNumBlocks == 0 )
        return -EINVAL;
    do{
        if( m_wFreeCount == BLOCKS_PER_GROUP )
            break;

        LONGWORD* pbmp = m_arrBytes;
        constexpr int iNumBits =
            sizeof( LONGWORD ) * BYTE_BITS;

        for( int i = 0; i < dwNumBlocks; i++ )
        {
            guint32 dwBlkIdx =
                ( pvecBlocks[ i ] & BLOCK_IDX_MASK );

            guint32 shift = find_shift( iNumBits );
            guint32 dwWordIdx = ( dwBlkIdx >> shift );
            int offset = dwBlkIdx & ( iNumBits - 1 );

#if BUILD_64
            LONGWORD a = ntohll( pbmp[ dwWordIdx ] );
#else
            LONGWORD a = ntohl( pbmp[ dwWordIdx ] );
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
    }while( 0 );

    if( ERROR( ret ) )
    {
        string strMsg = DebugMsg( ret,
            "Error in CBlockAllocator ctor" );
        throw std::runtime_error( strMsg );
    }
    return ret;
}

gint32 CBlockAllocator::SaveSuperBlock(
    char* pSb, guint32 dwSize )
{
    gint32 ret = 0;
    do{
        CWriteLock oLock( this->GetLock() );
        ret = lseek( m_iFd, 0, SEEK_SET );
        if( ERROR( ret ) )
        {
            ret = -errno;
            break;
        }
        ret = write( m_iFd, pSb,
            std::min( dwSize, SUPER_BLOCK_SIZE ) );
        if( ERROR( ret ) )
        {
            ret = -errno;
            break;
        }

    }while( 0 );
    return ret;
}

gint32 CBlockAllocator::LoadSuperBlock(
    char* pSb, guint32 dwSize )
{
    gint32 ret = 0;
    do{
        CWriteLock oLock( this->GetLock() );
        ret = lseek( m_iFd, 0, SEEK_SET );
        if( ERROR( ret ) )
        {
            ret = -errno;
            break;
        }
        ret = read( m_iFd, pSb,
            std::min( dwSize, SUPER_BLOCK_SIZE ) );
        if( ERROR( ret ) )
        {
            ret = -errno;
            break;
        }

    }while( 0 );
    return ret;
}

gint32 CBlockAllocator::SaveGroupBitmap(
    const CGroupBitmap& gbmp )
{
    gint32 ret = 0;
    do{
        CWriteLock oLock( this->GetLock() );
        ret = lseek( m_iFd, GRPBMP_START , SEEK_SET );
        if( ERROR( ret ) )
        {
            ret = -errno;
            break;
        }

        guint16* pCount = gbmp.m_arrBytes +
            sizeof( gbmp.m_arrBytes ) - sizeof( guint16 )

        *pCount = htons( gbmp.m_wFreeCount );

        ret = write( m_iFd, gbmp.m_arrBytes,
            sizeof( gbmp.m_arrBytes ) );

        if( ERROR( ret ) )
        {
            ret = -errno;
            break;
        }
    }while( 0 );
    return ret;
}

gint32 CBlockAllocator::LoadGroupBitmap(
    CGroupBitmap& gbmp )
{
    gint32 ret = 0;
    do{
        CWriteLock oLock( this->GetLock() );
        ret = lseek( m_iFd, GRPBMP_START , SEEK_SET );
        if( ERROR( ret ) )
        {
            ret = -errno;
            break;
        }
        ret = read( m_iFd, gbmp.m_arrBytes,
            sizeof( gbmp.m_arrBytes ) );

        if( ERROR( ret ) )
        {
            ret = -errno;
            break;
        }
        guint16* pCount = gbmp.m_arrBytes +
            sizeof( gbmp.m_arrBytes ) - sizeof( guint16 )
        gbmp.m_wFreeCount = htons( *pCount );

    }while( 0 );

    return ret;
}
