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

namespace rpcf
{

gint32 CFileImage::Reload()
{
    gint32 ret = 0;
    do{
        __attribute__((aligned (8))) 
        guint8 arrBytes[ BLOCK_SIZE ];
        ret = m_pAlloc->ReadBlock(
            m_dwInodeIdx, arrBytes );
        if( ERROR( ret ) )
            break;

        RegFSInode* pInode = arrBytes; 

        // file size in bytes
        m_oInodeStore.m_dwSize =
            ntohl( pInode->m_dwSize );

        // time of last modification.
        m_oInodeStore.m_dwmtime =
            ntohl( pInode->m_dwmtime );
        // time of last access.
        timespec oAccTime;
        clock_gettime( CLOCK_REALTIME, &oAccTime );
        m_oInodeStore.m_dwatime =
            oAccTime.tv_sec;
        // file type
        m_oInodeStore.m_wMode =
            ntohs( pInode->m_wMode );
        // uid
        m_oInodeStore.m_wuid =
            ntohs( pInode->m_wuid );
        // gid
        m_oInodeStore.m_wgid =
            ntohs( pInode->m_wgid );
        // others
        m_oInodeStore.m_woid =
            ntohs( pInode->m_woid );
        // type of file content
        m_oInodeStore.m_dwFlags =
            ntohl( pInode->m_dwFlags );
        // blocks for data section.
        int count =
            sizeof( pInode->m_arrBlocks ) >> 2;
        for( int i = 0; i < count; i++ )
            m_oInodeStore.m_arrBlocks[ i ] = 
                ntohl( pInode->m_arrBlocks[ i ] );

        // blocks for extended inode
        count =
            sizeof( pInode->m_arrMetaFork ) >> 2;
        for( int i = 0; i < count; i++ )
            m_oInodeStore.m_arrMetaFork[ i ] =
              ntohl( pInode->m_arrMetaFork[ i ] );

        switch( pInode->m_iValType )
        {
        case typeByte:
            {
                m_oValue =
                    m_oInodeStore.m_arrBuf[ 0 ];
                break;
            }
        case typeUInt16:
            {
                m_oValue = ntohs( *( ( guint16* )
                    m_oInodeStore.m_arrBuf ) );
                break;
            }
        case typeUInt32:
            {
                m_oValue = ntohl( *( ( guint32* )
                    m_oInodeStore.m_arrBuf ) );
                break;
            }
        case typeUInt64:
            {
                guint64 qwVal;
                memcpy( &qwVal,
                    m_oInodeStore.m_arrBuf,
                    sizeof( qwVal ) );
                m_oValue = ntohll( qwVal );
                break;
            }
        case typeFloat:
            {
                guint32 dwVal = ntohl( *( ( guint32* )
                    m_oInodeStore.m_arrBuf ) );
                oVal = *( ( float* )&dwVal );
                break;
            }
        case typeDouble:
            {
                guint64 qwVal;
                memcpy( &qwVal,
                    m_oInodeStore.m_arrBuf,
                    sizeof( qwVal ) );
                qwVal = ntohll( qwVal );
                oVal = *( ( double* )&qwVal );
                break;
            }
        case typeString:
            {
                char szBuf[ sizeof(
                    RegFSInode::m_arrBuf ) ];

                guint32 len = strnlen(
                    pInode->m_arrBuf,
                    sizeof( szBuf ) - 1 );

                memcpy( szBuf,
                    m_oInodeStore.m_arrBuf,
                    len );

                szBuf[ len + 1 ] = 0;
                m_oValue = ( const char* )szBuf;
                break;
            }
        default:
            ret = -ENOTSUP;
            break;
        }
        guint32& dwSize = 
            m_oInodeStore.m_dwSize;
        bool bFirst = 
            WITHIN_INDIRECT_BLOCK( dwSize );

        bool bSec = 
            WITHIN_SEC_INDIRECT_BLOCK( dwSize );

        if( bSec )
        {
            // load the block index table directory
            m_pBitdBlk.NewObj();
            BufPtr& p = m_pBitdBlk;
            p->Resize( BLOCK_SIZE );
            guint32* arrBlks =
                m_oInodeStore.m_arrBuf;
            guint32 dwBlkIdx =
                arrBlks[ BITD_IDX ];
            if( dwBlkIdx == 0 )
            {
                ret = m_pAlloc->AllocBlocks(
                    &dwBlkIdx, 1 );
                if( ERROR( ret ) )
                    break;
                arrBlks[ BITD_IDX ] = dwBlkIdx;
                memset( p->ptr(), 0, p->size() );
                break;
            }
            ret = m_pAlloc->ReadBlock(
                dwBlkIdx, p->ptr() );
            if( ERROR( ret ) )
                break;
        }
        if( bFirst || bSec )
        {
            // load the block index table
            m_pBitBlk.NewObj();
            BufPtr& p = m_pBitBlk;
            p->Resize( BLOCK_SIZE );
            guint32* arrBlks =
                m_oInodeStore.m_arrBuf;
            guint32 dwBlkIdx =
                arrBlks[ BIT_IDX ];
            if( dwBlkIdx == 0 )
            {
                ret = m_pAlloc->AllocBlocks(
                    &dwBlkIdx, 1 );
                if( ERROR( ret ) )
                    break;
                arrBlks[ BIT_IDX ] = dwBlkIdx;
                memset( p->ptr(), 0, p->size() );
                break;
            }
            ret = m_pAlloc->ReadBlock(
                dwBlkIdx, p->ptr() );
            if( ERROR( ret ) )
                break;
        }
        // for small data with length less than 96 bytes.
    }while( 0 );
    return ret;
}

gint32 CFileImage::Format()
{
    gint32 ret = 0;
    do{
        __attribute__((aligned (8))) 
        guint8 arrBytes[ BLOCK_SIZE ] = {0};

        RegFSInode* pInode = arrBytes; 

        // file size in bytes
        m_oInodeStore.m_dwSize = 0;

        // time of last modification.
        clock_gettime( CLOCK_REALTIME, &oAccTime );

        m_oInodeStore.m_dwatime =
            oAccTime.tv_sec;
        m_oInodeStore.m_dwmtime =
            oAccTime.tv_sec;
        m_oInodeStore.m_dwctime =
            oAccTime.tv_sec;

        // file type
        m_oInodeStore.m_wMode = S_IFREG;

        // uid
        m_oInodeStore.m_wuid = 0;

        // gid
        m_oInodeStore.m_wgid = 0;

        // type of file content
        m_oInodeStore.m_dwFlags = 0;

        // blocks for data section.
        m_oInodeStore.m_iValType = typeNone;

        // for small data with length less than 96 bytes.
    }while( 0 );
    return ret;
}

gint32 CFileImage::Flush()
{
    gint32 ret = 0;
    do{
        __attribute__((aligned (8))) 
        guint8 arrBytes[ BLOCK_SIZE ] = {0};

        auto pInode = ( RegFSInode* ) arrBytes; 
        // file size in bytes
        pInode->m_dwSize =
            htonl( m_oInodeStore.m_dwSize );

        // time of last modification.
        pInode->m_dwmtime =
            htonl( m_oInodeStore.m_dwmtime );
        // time of last access.
        m_oInodeStore.m_dwatime =
            oAccTime.tv_sec;
        // file type
        pInode->m_wMode =
            ntohs( m_oInodeStore.m_wMode );
        // uid
        pInode->m_wuid =
            ntohs( m_oInodeStore.m_wuid );
        // gid
        pInode->m_wgid =
            ntohs( m_oInodeStore.m_wgid );
        // others
        pInode->m_woid =
            ntohs( m_oInodeStore.m_woid );
        // type of file content
        pInode->m_dwFlags =
            htonl( m_oInodeStore.m_dwFlags );
        // blocks for data section.
        int count =
            sizeof( pInode->m_arrBlocks ) >> 2;
        for( int i = 0; i < count; i++ )
            pInode->m_arrBlocks[ i ] = 
                htonl( m_oInodeStore.m_arrBlocks[ i ] );

        // blocks for extended inode
        count =
            sizeof( pInode->m_arrMetaFork ) >> 2;
        for( int i = 0; i < count; i++ )
            pInode->m_arrMetaFork[ i ] =
              htonl( m_oInodeStore.m_arrMetaFork[ i ] );
 
        pInode->m_iValType = m_oValue.GetTypeId();
        switch( m_oValue.GetTypeId() )
        {
        case typeByte:
            {
                pInode->m_arrBuf[ 0 ] =
                    m_oValue.m_byVal;
                break;
            }
        case typeUInt16:
            {
                auto p =
                    ( guint16* )pIonde->m_arrBuf;
                *p = htons( m_oValue.m_wVal );
                break;
            }
        case typeUInt32:
        case typeFloat:
            {
                auto p =
                    ( guint32* )pIonde->m_arrBuf;
                *p = htons( m_oValue.m_dwVal );
                break;
            }
        case typeUInt64:
        case typeDouble:
            {
                auto p =
                    ( guint64* )pIonde->m_arrBuf;
                *p = htonll( m_oValue.m_qwVal );
                break;
            }
        case typeString:
            {
                guint32 dwMax =
                    sizeof( pInode->m_arrBuf ) - 1;
                strcpy( pInode->m_arrBuf, 
                    m_oValue.m_strVal.substr(
                        0, dwMax ).c_str() );
                break;
            }
        default:
            break;
        }

        ret = m_pAlloc->WriteBlock(
            m_dwInodeIdx, arrBytes );
        if( ERROR( ret ) )
            break;

        if( m_oInodeStore.m_dwSize == 0 )
            break;

        guint32 dwSize =
            m_oInodeStore.m_dwSize / 512;

        guint32 dwBlkIdx = 0;
        guint32* arrBlks =
            m_oInodeStore.m_arrBlocks;
        dwBlkIdx = arrBlks[ BIT_IDX ];
        if( !m_pBitBlk.IsEmpty() && dwBlkIdx )
        {
            m_pAlloc->WriteBlock(
                dwBlkIdx, m_pBitBlk->ptr() );
        }
        dwBlkIdx = arrBlks[ BITD_IDX ];
        if( !m_pBitdBlk.IsEmpty() && dwBlkIdx )
        {
            m_pAlloc->WriteBlock(
                dwBlkIdx, m_pBitdBlk->ptr() );
        }
        guint32 *pbitd = m_pBitdBlk->ptr();
        std::vector< std::pair< guint32, BufPtr > > vecBlks;
        for( auto& elem : m_mapSecBitBlks )
        {
            guint32 dwIdx2 = elem.first; 
            BufPtr& pBuf = elem.second;
            dwBlkIdx = ntohl( pbitd[ dwIdx2 ] );
            m_pAlloc->WriteBlock(
                dwBlkIdx, pBuf->ptr() );
        }
        for( auto& elem: m_mapBlocks )
        {
            m_pAlloc->WriteBlock(
                elem.first, elem.second->ptr() );
        }
    }while( 0 );
    return ret;
}

gint32 CFileImage::CollectBlocksForRead(
    guint32 dwOff, guint32& dwSize,
    std::vector< guint32 >& vecBlocks )
{
    if( BEYOND_MAX_LIMIT( dwOff ) ||
        dwSize == 0 )
        return -EINVAL;

    if( BEYOND_MAX_LIMIT( dwSize + dwOff ) )
        dwSize = MAX_FILE_SIZE - dwOff;

    gint32 ret = 0;
    do{
        guint32 dwLead =
            ( dwOff >> BLOCK_SHIFT );
        guint32 dwTail =
            ( ( dwOff + dwSize + BLOCK_SIZE - 1 ) >>
                BLOCK_SHIFT );

        guint32 dwIndirectIdx =
            m_oInodeStore.m_arrBlocks[ BIT_IDX ];

        guint32 dw2IndIdx =
            m_oInodeStore.m_arrBlocks[ BITD_IDX ];

        for( int i = dwLead; i < dwTail + 1; i++ )
        {
            bool bFirst =
                WITHIN_INDIRECT_BLOCK( i );
            bool bSec = 
                WITHIN_SEC_INDIRECT_BLOCK( i );

            if( WITHIN_DIRECT_BLOCK( i ) )
            {
                vecBlocks.push_back(
                    m_oInodeStore.m_arrBlocks[ i ] );
            }
            else if( bFirst )
            {
                if( dwIndirectIdx == 0 )   
                {
                    vecBlocks.push_back( 0 );
                    continue;
                }
                CStdRMutex oExclLock( GetExclLock() );
                BufPtr& p = m_pBitBlk;
                if( p.IsEmpty() )
                {
                    p.NewObj();
                    p.Resize( BLOCK_SIZE );
                    ret = m_pAlloc->ReadBlock(
                        dwIndirectIdx, p->ptr() );
                    if( ERROR( ret ) )
                        break;
                }
                guint32* pbis =
                    ( guint32* )p->ptr();

                dwBlkIdx = ntohl( pbis[
                    INDIRECT_BLK_IDX( dwOff ) ] );

                vecBlocks.push_back(
                    ntohl( dwBlkIdx ) );
            }
            else if( bSec )
            {
                guint32 dwBitIdx =
                    SEC_BLKIDX_TABLE_IDX( i );
                BufPtr& p = m_pBitdBlk;    
                guint32 dwBlkIdxIdx =
                    ntohl( p[ dwBitIdx ] );
                if( dwBlkIdxIdx == 0 )
                {
                    vecBlocks.push_back( 0 );
                    continue;
                }
                CStdRMutex oExclLock( GetExclLock() );
                guint32 dwIdx2 = SEC_BLKIDX_IDX( i );
                auto itr = m_mapSecBitBlks.find( 
                    dwBitIdx );
                if( itr != m_mapSecBitBlks.end() )
                {
                    guint32* pblk = itr->second->ptr();
                    dwBlkIdx = ntohl( pblk[ dwIdx2 ] );
                    vecBlocks.push_back( dwBlkIdx );
                }
                else
                {
                    BufPtr p( true );
                    p->Resize( BLOCK_SIZE );
                    ret = m_pAlloc->ReadBlock(
                        dwBlkIdxIdx, p->ptr() );
                    if( ERROR( ret ) )
                        break;

                    guint32* pblk = p->ptr();
                    dwBlkIdx = ntohl( pblk[ dwIdx2 ] );
                    vecBlocks.push_back( dwBlkIdx );
                    m_mapSecBitBlks.insert(
                        dwBitIdx, p );
                }
            }
            else
            {
                ret = -ERANGE;
                break;
            }
        }
    }while( 0 );
    return ret;
}

gint32 CFileImage::CollectBlocksForWrite(
    guint32 dwOff, guint32& dwSize,
    std::vector< guint32 >& vecBlocks )
{
    if( BEYOND_MAX_LIMIT( dwOff ) ||
        dwSize == 0 )
        return -EINVAL;

    if( BEYOND_MAX_LIMIT( dwSize + dwOff ) )
        dwSize = MAX_FILE_SIZE - dwOff;

    gint32 ret = 0;
    do{
        guint32 dwLead =
            ( dwOff >> BLOCK_SHIFT );
        guint32 dwTail =
            ( ( dwOff + dwSize + BLOCK-SIZE - 1 ) >>
            BLOCK_SHIFT );

        guint32 dwIndirectIdx =
            m_oInodeStore.m_arrBlocks[ BIT_IDX ];

        guint32 dw2IndIdx =
            m_oInodeStore.m_arrBlocks[ BITD_IDX ];

        for( int i = dwLead; i < dwTail + 1; i++ )
        {
            bool bFirst =
                WITHIN_INDIRECT_BLOCK( i );
            bool bSec = 
                WITHIN_SEC_INDIRECT_BLOCK( i );

            if( WITHIN_DIRECT_BLOCK( i ) )
            {
                vecBlocks.push_back(
                    m_oInodeStore.m_arrBlocks[ i ] );
            }
            else if( bFirst )
            {
                if( dwIndirectIdx == 0 )   
                {
                    ret = m_pAlloc->AllocBlocks( 
                        &dwIndirectIdx, 1 );
                    if( ERROR( ret ) )
                        break;
                    BufPtr& p = m_pBitBlk;
                    p.NewObj();
                    p->Resize( BLOCK_SIZE );
                    memset( p->ptr(), 0, BLOCK_SIZE );
                    guint32* arrBlks =
                        m_oInodeStore.m_arrBlocks;
                    arrBlks[ BIT_IDX ] =
                        dwIndirectIdx;
                }
                BufPtr& p = m_pBitBlk;
                if( p.IsEmpty() )
                {
                    p.NewObj();
                    p.Resize( BLOCK_SIZE );
                    ret = m_pAlloc->ReadBlock(
                        dwIndirectIdx, p->ptr() );
                    if( ERROR( ret ) )
                        break;
                }
                guint32* pbis =
                    ( guint32* )p->ptr();

                guint32 dwBlkIdx = ntohl(
                    pbis[ INDIRECT_BLK_IDX( dwOff ) ] );

                if( dwBlkIdx == 0 )
                {
                    ret = m_pAlloc->AllocBlocks(
                        &dwBlkIdx, 1 );
                    if( ERROR( ret ) )
                        break;
                    pbis[ INDIRECT_BLK_IDX( dwOff ) ] =
                        htonl( dwBlkIdx );
                }

                vecBlocks.push_back(
                    ntohl( dwBlkIdx ) );
            }
            else if( bSec )
            {
                guint32* arrBlks =
                    m_oInodeStore.m_arrBlocks;
                if( dw2IndIdx == 0 )
                {
                    ret = m_pAlloc->AllocBlocks(
                        &dw2IndIdx, 1 );
                    if( ERROR( ret ) )
                        break;
                    arrBlks[ BITD_IDX ] =
                        dw2IndIdx;

                    m_pBitdBlk.NewObj();
                    ret = m_pBitdBlk.Resize(
                        BLOCK_SIZE );
                    if( ERROR( ret ) )
                        break;
                    memset( m_pBitdBlk->ptr(),
                        0, m_pBitdBlk->size() );
                }
                guint32 dwBitIdx =
                    SEC_BLKIDX_TABLE_IDX( i );
                BufPtr& p = m_pBitdBlk;    
                guint32 dwBitBlkIdx =
                    ntohl( p[ dwBitIdx ] );
                if( dwBitBlkIdx == 0 )
                {
                    // allocate a block index table
                    ret = m_pAlloc->AllocBlocks(
                        &dwBitBlkIdx, 1 );
                    if( ERROR( ret ) )
                        break;
                    BufPtr pBuf( true );
                    pBuf->Resize( BLOCK_SIZE );
                    memset( pBuf->ptr(), 0, pBuf->size() );
                    m_pAlloc->WriteBlock( pBuf->ptr(), 1 );
                    p[ dwBitIdx ] = htonl( dwBitBlkIdx );
                    m_mapSecBitBlks.insert(
                        dwBitIdx, pBuf );
                }
                guint32 dwIdx2 = SEC_BLKIDX_IDX( i );
                auto itr = m_mapSecBitBlks.find( 
                    dwBitIdx );
                if( itr != m_mapSecBitBlks.end() )
                {
                    guint32* pblk = itr->second->ptr();

                    guint32 dwBlkIdx =
                        ntohl( pblk[ dwIdx2 ] );

                    if( dwBlkIdx != 0 )
                    {
                        vecBlocks.push_back( dwBlkIdx );
                        continue;
                    }
                    ret = m_pAlloc->AllocBlocks(
                        &dwBlkIdx, 1 );
                    if( ERROR( ret ) )
                        break;
                    pblk[ INDIRECT_BLK_IDX( dwOff ) ] =
                        htonl( dwBlkIdx );
                    vecBlocks.push_back( dwBlkIdx );
                    continue;
                }
                else
                {
                    BufPtr pBitBlk( true );
                    pBitBlk->Resize( BLOCK_SIZE );
                    memset( pBitBlk->ptr,
                        0, pBitBlk->size() );
                    ret = m_pAlloc->ReadBlock(
                        dwBitBlkIdx, pBitBlk );
                    if( ERROR( ret ) )
                        break;
                    guint32* pblk = pBitBlk->ptr();
                    dwBlkIdx = ntohl( pblk[ dwIdx2 ] );
                    if( dwBlkIdx == 0 )
                    {
                        ret = m_pAlloc->AllocBlock(
                            &dwBlkIdx, 1 );
                        if( ERRO( ret ) )
                            break;
                    }
                    vecBlocks.push_back( dwBlkIdx );
                    m_mapSecBitBlks.insert(
                        dwBitIdx, pBitBlk );
                }
            }
            else
            {
                ret = -ERANGE;
                break;
            }
        }

    }while( 0 );

    return ret;
}

gint32 CFileImage::ReadFile(
    guint32 dwOff, guint32& dwSize,
    guint8* pBuf )
{
    gint32 ret = 0;
    if( pBuf == nullptr )
        return -EINVAL;
    if( BEYOND_MAX_LIMIT( dwOff ) )
        return -ERANGE;

    do{
        CReadLock oLock( this->GetLock() );
        if( dwOff >= m_oInodeStore.m_dwSize )
        {
            dwSize = 0;
            return 0;
        }
        if( dwOff + dwSize >
            m_oInodeStore.m_dwSize )
        {
            dwSize =
                m_oInodeStore.m_dwSize - dwOff;
        }
        std::vector< guint32 > vecBlks;
        ret = CollectBlocksForRead(
            dwOff, dwSize, vecBlks );
        if( ERROR( ret ) )
            break;
        guint8 arrBytes[ BLOCK_SIZE ];

        guint32 dwHead =
            ( dwOff & ( BLOCK_SIZE - 1 ) );

        guint32 dwTail =
            ( ( dwOff + dwSize ) &
                ( BLOCK_SIZE - 1 ) );

        guint32 dwBlocks = vecBlks.size();
        if( dwHead == 0 && dwTail == 0 )
        {
            ret = m_pAlloc->ReadBlocks(
                vecBlks.data(),
                dwBlocks, pBuf );
            break;
        }
        else if( dwHead == 0 && dwTail > 0 )
        {
            if( dwBlocks > 1 )
            {
                ret = m_pAlloc->ReadBlocks(
                    vecBlks.data(),
                    dwBlocks - 1, pBuf );
                if( ERROR( ret ) )
                    break;
            }
            ret = m_pAlloc->ReadBlock(
                vecBlks.back(), arrBytes );
            if( ERROR( ret ) )
                break;
            memcpy( pBuf +
                BLOCK_SIZE * ( dwBlocks - 1 ),
                arrBytes, dwTail );
        }
        else if( dwHead > 0 && dwTail == 0 )
        {
            ret = m_pAlloc->ReadBlock(
                vecBlks.front(), arrBytes );
            if( ERROR( ret ) )
                break;
            memcpy( pBuf, arrBytes + dwHead,
                ( BLOCK_SIZE - dwHead ) );
            if( dwBlocks == 1 )
                break;
            ret = m_pAlloc->ReadBlocks(
                vecBlks.data() + 1,
                vecBlks.size() - 1,
                pBuf + ( BLOCK_SIZE - dwHead ) );
        }
        else //( dwHead > 0 && dwTail > 0 )
        {
            guint8* pCur = pBuf;
            ret = m_pAlloc->ReadBlock(
                vecBlks.front(), arrBytes );
            if( ERROR( ret ) )
                break;
            if( dwBlocks == 1 )
            {
                memcpy( pBuf,
                    arrBytes + dwHead, dwSize );
                break;
            }
            else
            {
                memcpy( pBuf, arrBytes + dwHead,
                    ( BLOCK_SIZE - dwHead ) );
                pCur = pBuf + ( BLOCK_SIZE - dwHead );
            }
            if( dwBlocks > 2 )
            {
                ret = m_pAlloc->ReadBlocks(
                    vecBlks.data() + 1,
                    dwBlocks - 2, pCur );
                if( ERROR( ret ) )
                    break;
                pCur += ( dwBlocks - 2 ) * BLOCK_SIZE;
            }
            ret = m_pAlloc->ReadBlock(
                vecBlks.back(), arrBytes );
            if( ERROR( ret ) )
                break;
            memcpy( pCur, arrBytes, dwTail );
        }

    }while( 0 );
    return ret;
}

gint32 CFileImage::WriteFile(
    guint32 dwOff, guint32 dwSize,
    guint8* pBuf )
{
    gint32 ret = 0;
    if( pBuf == nullptr )
        return -EINVAL;
    if( BEYOND_MAX_LIMIT( dwOff + dwSize ) )
        return -ERANGE;

    do{
        CWriteLock oLock( this->GetLock() );
        std::vector< guint32 > vecBlks;
        ret = CollectBlocksForWrite(
            dwOff, dwSize, vecBlks );
        if( ERROR( ret ) )
            break;

        guint8 arrBytes[ BLOCK_SIZE ];

        guint32 dwHead =
            ( dwOff & ( BLOCK_SIZE - 1 ) );

        guint32 dwTail =
            ( ( dwOff + dwSize ) &
                ( BLOCK_SIZE - 1 ) );

        guint32 dwBlocks = vecBlks.size();
        if( dwHead == 0 && dwTail == 0 )
        {
            ret = m_pAlloc->WriteBlocks(
                vecBlks.data(),
                dwBlocks, pBuf );
            break;
        }
        else if( dwHead == 0 && dwTail > 0 )
        {
            if( dwBlocks > 1 )
            {
                ret = m_pAlloc->WriteBlocks(
                    vecBlks.data(),
                    dwBlocks - 1, pBuf );
                if( ERROR( ret ) )
                    break;
            }
            ret = m_pAlloc->ReadBlock(
                vecBlks.back(), arrBytes );
            if( ERROR( ret ) )
                break;
            memcpy( arrBytes,
                pBuf + BLOCK_SIZE * ( dwBlocks - 1 ),
                dwTail );
            ret = m_pAlloc->WriteBlock(
                vecBlks.back(), arrBytes );
        }
        else if( dwHead > 0 && dwTail == 0 )
        {
            ret = m_pAlloc->ReadBlock(
                vecBlks.front(), arrBytes );
            if( ERROR( ret ) )
                break;

            memcpy( arrBytes + dwHead, pBuf,
                ( BLOCK_SIZE - dwHead ) );

            ret = m_pAlloc->WriteBlock(
                vecBlks.front(), arrBytes );

            if( dwBlocks == 1 )
                break;

            ret = m_pAlloc->WriteBlocks(
                vecBlks.data() + 1,
                vecBlks.size() - 1,
                pBuf + ( BLOCK_SIZE - dwHead ) );
        }
        else //( dwHead > 0 && dwTail > 0 )
        {
            guint8* pCur = pBuf;
            ret = m_pAlloc->ReadBlock(
                vecBlks.front(), arrBytes );
            if( ERROR( ret ) )
                break;
            if( dwBlocks == 1 )
            {
                memcpy( arrBytes + dwHead,
                    pBuf, dwSize );
                ret = m_pAlloc->WriteBlock(
                    vecBlks.front(), arrBytes );
                break;
            }
            else
            {
                memcpy( arrBytes + dwHead, pBuf,
                    ( BLOCK_SIZE - dwHead ) );
                ret = m_pAlloc->WriteBlock(
                    vecBlks.front(), arrBytes );
                pCur = pBuf + ( BLOCK_SIZE - dwHead );
            }
            if( dwBlocks > 2 )
            {
                ret = m_pAlloc->WriteBlocks(
                    vecBlks.data() + 1,
                    dwBlocks - 2, pCur );
                if( ERROR( ret ) )
                    break;
                pCur += ( dwBlocks - 2 ) * BLOCK_SIZE;
            }
            ret = m_pAlloc->ReadBlock(
                vecBlks.back(), arrBytes );
            if( ERROR( ret ) )
                break;
            memcpy( arrBytes, pCur, dwTail );
            ret = m_pAlloc->WriteBlock(
                vecBlks.back(), arrBytes );
        }

        if( dwSize + dwOff >
            m_oInodeStore.m_dwSize )
            m_oInodeStore.m_dwSize = dwSize + dwOff;

        timespec mtime;
        clock_gettime( CLOCK_REALTIME, &mtime );
        m_oInodeStore.m_dwmtime = mtime.tv_sec;

    }while( 0 );
    return ret;
}

gint32 CFileImage::TruncBlkDirect( guint32 lablkidx )
{
    gint32 ret = 0;
    do{
        if( !WITHIN_DIRECT_BLOCK(
            ( lablkidx << BLOCK_SHIFT ) ) )
        {
            ret = -EINVAL;
            break;
        }
        guint32* arrBlks =
            m_oInodeStore.m_arrBlocks;
        ret = m_pAlloc->FreeBlocks(
            arrBlks + lablkidx,
            BIT_IDX - lablkidx );
        if( ERROR( ret ) )
            break;
        for( int i = lablkidx; i < BIT_IDX; i++ )
            arrBlks[ i ] = 0;

    }while( 0 );

    return ret;
}

gint32 CFileImage::TruncBlkIndirect(
    guint32 lablkidx )
{
    gint32 ret = 0;
    do{
        if( lablkidx >= BLKIDX_PER_TABLE )
        {
            ret = -EINVAL;
            break;
        }
        if( !WITHIN_INDIRECT_BLOCK(
            ( lablkidx << BLOCK_SHIFT ) ) ) 
        {
            ret = -EINVAL;
            break;
        }
        if( m_pBitBlk.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }
        guint32 dwCount = 
            BLKIDX_PER_TABLE - lablkidx;

        guint32 i = lablkidx;
        guint32* pbit = m_pBitBlk->ptr();

        ret = m_pAlloc->FreeBlocks(
            pbit + i, dwCount, true );

        if( ERROR( ret ) )
            break;

        for( ; i < lablkidx + dwCount; i++ )
            pbit[ i ] = 0;

        if( ( lablkidx << BLOCK_SHIFT ) ==
            INDIRECT_BLOCK_START )
        {
            guint32 dwBitdBlkIdx =
                m_oInodeStore.m_arrBlocks[ BIT_IDX ];
            ret = m_pAlloc->FreeBlocks(
                &dwBitdBlkIdx, 1 );
            m_pBitBlk.Clear();
        }
    }while( 0 );
    return ret;
}

gint32 CFileImage::TruncBlkSecIndirect(
    guint32 lablkidx )
{
    gint32 ret = 0;
    do{
        if( !WITHIN_SEC_INDIRECT_BLOCK(
            ( lablkidx << BLOCK_SHIFT ) ) ) 
        {
            ret = -EINVAL;
            break;
        }
        if( m_pBitdBlk.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        guint32 dwBitIdx = SEC_BLKIDX_TABLE_IDX(
            ( lablkidx << BLOCK_SHIFT ) );

        guint32 dwBlkIdxIdx = SEC_BLKIDX_IDX(
            ( lablkidx << BLOCK_SHIFT ) );

        guint32 i = dwBitIdx + 1;
        guint32* pbitd = m_pBitdBlk->ptr();
        for( ; i < BLKIDX_PER_TABLE; i++ )
        {
            guint32 dwBitBlkIdx = pbitd[ i ];
            auto itr = m_mapSecBitBlks.find( i );
            if( itr != m_mapSecBitBlks.end() )
            {
                BufPtr& p = itr->second;
                m_pAlloc->FreeBlocks( p->ptr(),
                    BLKIDX_PER_TABLE, true );
                m_mapSecBitBlks.erase( itr );
            }
            else if( dwBitBlkIdx != 0 )
            {
                guint32 arrIdx[
                    BLOCK_SIZE / sizeof( guint32 ) ];
                guint8* p = ( guint8* )arrIdx;
                ret = m_pAlloc->ReadBlock(
                    dwBitBlkIdx, p );
                if( ERROR( ret ) )
                    break;
                ret = m_pAlloc->FreeBlocks(
                    arrIdx, BLKIDX_PER_TABLE,
                    true );
                if( ERROR( ret ) )
                    break;
            }
            if( dwBitBlkIdx != 0 )
            {
                m_pAlloc->FreeBlocks(
                    &dwBitBlkIdx, 1 );
                pbitd[ i ] = 0;
            }
        }
        if( true )
        {
            // the last block index table
            guint32 dwBitBlkIdx = pbitd[ dwBitIdx ];
            auto itr = m_mapSecBitBlks.find( dwBitIdx );
            if( itr != m_mapSecBitBlks.end() )
            {
                BufPtr& p = itr->second;
                guint32* pblkidx =
                    p->ptr() + dwBlkIdxIdx;
                m_pAlloc->FreeBlocks( pblkidx,
                    BLKIDX_PER_TABLE - dwBlkIdxIdx,
                    true );
                for( int i = 0;
                    i < BLKIDX_PER_TABLE - dwBlkIdxIdx;
                    i++ )
                    pblkidx[ i ] = 0;
            }
            else if( dwBitBlkIdx != 0 )
            {
                BufPtr p( true );
                p->Resize( BLOCK_SIZE );
                ret = m_pAlloc->ReadBlock(
                    dwBitBlkIdx, p );
                if( ERROR( ret ) )
                    break;

                guint32* pbit = ( guint32* )p->ptr();

                ret = m_pAlloc->FreeBlocks(
                    pbit + dwBlkIdxIdx,
                    BLKIDX_PER_TABLE - dwBlkIdxIdx,
                    true );
                if( ERROR( ret ) )
                    break;

                if( dwBlkIdxIdx > 0 )
                {
                    for( int j = dwBlkIdxIdx;
                        j < BLKIDX_PER_TABLE; j++ )
                        pbit[ j ] = 0;
                    m_mapSecBitBlks.insert(
                        dwBitIdx, p );
                }
            }
            if( dwBitBlkIdx != 0 &&
                dwBlkIdxIdx == 0 )
            {
                m_pAlloc->FreeBlocks(
                    &dwBitBlkIdx, 1 );
                pbitd[ dwBitIdx ] = 0;
            }
        }
        if( ( lablkidx << BLOCK_SHIFT ) ==
            SEC_INDIRECT_BLOCK_START )
        {
            guint32 dwBitdBlkIdx =
            m_oInodeStore.m_arrBlocks[ BITD_IDX ];

            ret = m_pAlloc->FreeBlocks(
                &dwBitdBlkIdx, 1 );

            m_pBitdBlk.Clear();
        }
    }while( 0 );
    return ret;
}

gint32 CFileImage::Truncate( guint32 dwOff )
{
    gint32 ret = 0;
    if( BEYOND_MAX_LIMIT( dwOff ) )
        return -ERANGE;

    do{
        CWriteLock oLock( this->GetLock() );
        guint32& dwSize = m_oInodeStore.m_dwSize;
        if( dwOff > dwSize )
        {
            ret = Extend( dwOff );
            break;
        }
        if( dwOff == dwSize )
            break;

        guint32 dwDelta = dwSize - dwOff;

        guint32 dwHead =
            ( dwOff & ( BLOCK_SIZE - 1 ) );

        guint32 dwTail = ( ( dwOff + dwDelta ) &
                ( BLOCK_SIZE - 1 ) );

        guint32 dwBlocks = 0;
        guint32 dwBytes = dwDelta;
        guint32 dwTruncOff = 0xffffffff;

        if( dwHead > 0 )
        {
            dwBytes -= std::min(
                ( dwOff & ( BLOCK_SIZE - 1 ) ),
                dwDelta );
        }

        if( dwBytes > 0 )
        {
            dwTruncOff =
                ( ( dwOff + BLOCK_SIZE - 1 ) &
                ( ~( BLOCK_SIZE - 1 ) ) );
        }

        if( dwBytes > BLOCK_SIZE )
        {
            guint32 dwBlks = 
                ( dwBytes >> BLOCK_SHIFT );
            dwBlocks += dwBlks;
            dwBytes &= ( BLOCK_SIZE - 1 );
        }

        if( dwTail > 0 && dwBytes > 0 )
            dwBlocks++;

        if( dwTruncOff == 0xffffffff )
        {
            // no need to truncate
            dwSize = dwOff;
            break;
        }

        guint32 dwSecStart =
        ( SEC_INDIRECT_BLOCK_START >> BLOCK_SHIFT );

        guint32 dwIndStart =
            ( INDIRECT_BLOCK_START >> BLOCK_SHIFT );

        if( WITHIN_SEC_INDIRECT_BLOCK( dwTruncOff ) )
        {
            ret = TruncBlkSecIndirect(
                dwTruncOff >> BLOCK_SHIFT );
        }
        else if( WITHIN_INDIRECT_BLOCK( dwTruncOff ) )
        {
            if( WITHIN_SEC_INDIRECT_BLOCK( dwSize ) )
            {
                ret = TruncBlkSecIndirect( dwSecStart );
                if( ERROR( ret ) )
                    break;
            }
            ret = TruncBlkIndirect(
                dwTruncOff >> BLOCK_SHIFT );
        }
        else
        {
            // truncate from direct block
            if( WITHIN_SEC_INDIRECT_BLOCK( dwSize ) )
            {
                ret = TruncBlkSecIndirect(
                    dwSecStart );
                if( ERROR( ret ) )
                    break;
                ret = TruncBlkIndirect(
                    dwIndStart );
            }
            else if( WITHIN_INDIRECT_BLOCK( dwSize ) )
            {
                ret = TruncBlkIndirect(
                    dwIndStart );
                if( ERROR( ret ) )
                    break;
            }
            ret = TruncBlkDirect(
                dwTruncOff >> BLOCK_SHIFT );
        }
        dwSize = dwOff;
        timespec mtime;
        clock_gettime( CLOCK_REALTIME, &mtime );
        m_oInodeStore.m_dwmtime = mtime.tv_sec;

    }while( 0 );
    return ret;
}

gint32 CFileImage::Extend( guint32 dwOff )
{
    gint32 ret = 0;
    do{
        guint32 laddr =
            ( dwOff & ~( BLOCK_SIZE - 1 ) );
        guint8 arrBytes[ BLOCK_SIZE ] = {0};
        ret = WriteFile( laddr,
            ( dwOff & ( BLOCK_SIZE - 1 ) ),
            arrBytes );
    }while( 0 );
    return ret;
}

gint32 CFileImage::ReadValue(
    Variant& oVar )
{
    gint32 ret = 0;
    CReadLock oLock( this->GetLock() );
    oVar = m_oValue;
    return ret;
}

gint32 CFileImage::WriteValue(
    const Variant& oVar )
{
    gint32 ret = 0;
    CReadLock oLock( this->GetLock() );
    m_oValue = oVar;
    return ret;
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
        m_pRootImg.reset( new CDirImage(
            this, ROOT_INODE_BLKIDX, 0 ) );
        ret = m_pRootImg->Format();
        if( ERROR( ret ) )
            break;
        ret = m_pRootImg->Flush();
        if( ERROR( ret ) )
            break;
        m_pRootDir.reset(
            new CDirFileEntry(
            m_pAlloc, m_pRootImg ) );

    }while( 0 );
    return ret;
}

gint32 CRegistryFs::OpenRootDir()
{
    gint32 ret = 0;
    do{
        m_pRootImg.reset( new CDirImage(
            this, ROOT_INODE_BLKIDX, 0 ) );
        ret = m_pRootImg->Reload();
        if( ERROR( ret ) )
            break;
        m_pRootDir.reset(
            new CDirFileEntry(
            m_pAlloc, m_pRootImg ) );
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

}
