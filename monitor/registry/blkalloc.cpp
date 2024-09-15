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
#include "fcntl.h"
#include "unistd.h"

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
