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

