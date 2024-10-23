/*
 * =====================================================================================
 *
 *       Filename:  file.cpp
 *
 *    Description:  Implementation of CFileImage and basic read/write
 *                  operations on a file. 
 *        Version:  1.0
 *        Created:  10/09/2024 04:17:56 PM
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

CFileImage::CFileImage( const IConfigDb* pCfg )
{
    gint32 ret = 0;
    do{
        SetClassId( clsid( CFileImage ) );
        CCfgOpener oCfg( pCfg );
        ObjPtr pObj;
        oCfg.GetObjPtr( 0, pObj );
        m_pAlloc = pObj;
        if( m_pAlloc.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }
        ret = oCfg.GetIntProp( 1, m_dwInodeIdx );
        if( ERROR( ret ) )
            break;
        oCfg.GetIntProp( 2, m_dwParentInode );
        if( ERROR( ret ) )
            break;
    }while( 0 );
    if( ERROR( ret ) )
    {
        stdstr strMsg = DebugMsg( ret,
            "Error in CFileImage ctor" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CFileImage::Reload()
{
    gint32 ret = 0;
    do{
        DECL_ARRAY( arrBytes, BLOCK_SIZE );
        ret = m_pAlloc->ReadBlock(
            m_dwInodeIdx, ( guint8* )arrBytes );
        if( ERROR( ret ) )
            break;

        auto pInode = ( RegFSInode* )arrBytes; 

        // file size in bytes
        SetSize( ntohl( pInode->m_dwSize ) );

        // time of last modification.
        m_oInodeStore.m_mtime.tv_sec =
            ntohl( pInode->m_mtime.tv_sec );
        m_oInodeStore.m_mtime.tv_nsec =
            ntohl( pInode->m_mtime.tv_nsec );

        m_oInodeStore.m_ctime.tv_sec =
            ntohl( pInode->m_ctime.tv_sec );
        m_oInodeStore.m_ctime.tv_nsec =
            ntohl( pInode->m_ctime.tv_nsec );
        // time of last access.
        timespec oAccTime;
        clock_gettime( CLOCK_REALTIME, &oAccTime );
        m_oInodeStore.m_atime = oAccTime;
        // file type
        m_oInodeStore.m_dwMode =
            ntohl( pInode->m_dwMode );
        // uid
        m_oInodeStore.m_wuid =
            ntohs( pInode->m_wuid );
        // gid
        m_oInodeStore.m_wgid =
            ntohs( pInode->m_wgid );
        // type of file content
        m_oInodeStore.m_dwFlags =
            ntohl( pInode->m_dwFlags );

        m_oInodeStore.m_dwRootBNode =
            ntohl( pInode->m_dwRootBNode );
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
                m_oValue = *( ( float* )&dwVal );
                break;
            }
        case typeDouble:
            {
                guint64 qwVal;
                memcpy( &qwVal,
                    m_oInodeStore.m_arrBuf,
                    sizeof( qwVal ) );
                qwVal = ntohll( qwVal );
                m_oValue = *( ( double* )&qwVal );
                break;
            }
        case typeString:
            {
                char szBuf[ VALUE_SIZE ];

                guint32 len = strnlen(
                    ( char* )pInode->m_arrBuf,
                    VALUE_SIZE - 1 );

                memcpy( szBuf,
                    m_oInodeStore.m_arrBuf,
                    len );

                szBuf[ len ] = 0;
                m_oValue = ( const char* )szBuf;
                break;
            }
        case typeNone:
            break;
        default:
            ret = -ENOTSUP;
            break;
        }
        guint32 dwSize = GetSize();
        if( dwSize == 0 )
            break;

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
                m_oInodeStore.m_arrBlocks;
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
                dwBlkIdx, ( guint8* )p->ptr() );
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
                m_oInodeStore.m_arrBlocks;
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
                dwBlkIdx, ( guint8* )p->ptr() );
            if( ERROR( ret ) )
                break;
        }
        if( ERROR( ret ) )
            break;
        ret = 0;
    }while( 0 );
    return ret;
}

gint32 CFileImage::Format()
{
    gint32 ret = 0;
    do{
        // file size in bytes
        SetSize( 0 );

        // time of last modification.
        timespec oAccTime;
        clock_gettime( CLOCK_REALTIME, &oAccTime );

        m_oInodeStore.m_atime = oAccTime;
        m_oInodeStore.m_mtime = oAccTime;
        m_oInodeStore.m_ctime = oAccTime;

        // file type
        m_oInodeStore.m_dwMode =
            ( S_IFREG | 0660 );

        // uid
        m_oInodeStore.m_wuid = geteuid();

        // gid
        m_oInodeStore.m_wgid = getegid();

        // type of file content
        m_oInodeStore.m_dwFlags = 0;

        // blocks for data section.
        memset( m_oInodeStore.m_arrBlocks, 0,
            sizeof( m_oInodeStore.m_arrBlocks ) );

        m_oInodeStore.m_dwReserved = 0;
        memset( m_oInodeStore.m_arrMetaFork, 0,
        sizeof( m_oInodeStore.m_arrMetaFork ) );

        memset( m_oInodeStore.m_arrBuf, 0,
            sizeof( m_oInodeStore.m_arrBuf ) );

        m_oInodeStore.m_iValType = typeNone;
        m_oInodeStore.m_dwRootBNode =
            INVALID_BNODE_IDX;
        // for small data with length less than 96 bytes.
    }while( 0 );
    return ret;
}

gint32 CFileImage::Flush( guint32 dwFlags )
{
    gint32 ret = 0;
    do{
        DECL_ARRAY( arrBytes, BLOCK_SIZE );
        memset( arrBytes, 0, BLOCK_SIZE );
        bool bMeta = true;
        if( ( dwFlags & FLAG_FLUSH_DATAONLY ) )
            bMeta = false;

        if( bMeta )
        {
            auto pInode = ( RegFSInode* ) arrBytes; 
            // file size in bytes
            pInode->m_dwSize = htonl( GetSize() );

            // time of last modification.
            pInode->m_mtime.tv_sec =
                htonl( m_oInodeStore.m_mtime.tv_sec );
            pInode->m_mtime.tv_nsec =
                htonl( m_oInodeStore.m_mtime.tv_nsec );

            pInode->m_ctime.tv_sec =
                htonl( m_oInodeStore.m_ctime.tv_sec );
            pInode->m_ctime.tv_nsec =
                htonl( m_oInodeStore.m_ctime.tv_nsec );
            // time of last access.
            timespec oAccTime;
            clock_gettime(
                CLOCK_REALTIME, &oAccTime );
            m_oInodeStore.m_atime = oAccTime;
            // file type
            pInode->m_dwMode =
                htonl( m_oInodeStore.m_dwMode );
            // uid
            pInode->m_wuid =
                htons( m_oInodeStore.m_wuid );
            // gid
            pInode->m_wgid =
                htons( m_oInodeStore.m_wgid );
            // type of file content
            pInode->m_dwFlags =
                htonl( m_oInodeStore.m_dwFlags );
            pInode->m_dwRootBNode =
                htonl( m_oInodeStore.m_dwRootBNode );
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
                        ( guint16* )pInode->m_arrBuf;
                    *p = htons( m_oValue.m_wVal );
                    break;
                }
            case typeUInt32:
            case typeFloat:
                {
                    auto p =
                        ( guint32* )pInode->m_arrBuf;
                    *p = htons( m_oValue.m_dwVal );
                    break;
                }
            case typeUInt64:
            case typeDouble:
                {
                    auto p =
                        ( guint64* )pInode->m_arrBuf;
                    *p = htonll( m_oValue.m_qwVal );
                    break;
                }
            case typeString:
                {
                    guint32 dwMax = VALUE_SIZE - 1;
                    strcpy( ( char* )pInode->m_arrBuf, 
                        m_oValue.m_strVal.substr(
                            0, dwMax ).c_str() );
                    break;
                }
            case typeNone:
            default:
                break;
            }

            ret = m_pAlloc->WriteBlock(
                m_dwInodeIdx, ( guint8* )arrBytes );
            if( ERROR( ret ) )
                break;
        }

        if( GetSize() == 0 )
            break;

        guint32 dwBlkIdx = 0;
        guint32* arrBlks =
            m_oInodeStore.m_arrBlocks;
        dwBlkIdx = arrBlks[ BIT_IDX ];
        if( !m_pBitBlk.IsEmpty() && dwBlkIdx )
        {
            m_pAlloc->WriteBlock( dwBlkIdx,
                ( guint8* )m_pBitBlk->ptr() );
        }
        dwBlkIdx = arrBlks[ BITD_IDX ];
        if( !m_pBitdBlk.IsEmpty() && dwBlkIdx )
        {
            m_pAlloc->WriteBlock( dwBlkIdx,
                ( guint8* )m_pBitdBlk->ptr() );
        }
        if( !m_pBitdBlk.IsEmpty() )
        {
            auto pbitd =
                ( guint32* )m_pBitdBlk->ptr();

            for( auto& elem : m_mapSecBitBlks )
            {
                guint32 dwIdx2 = elem.first; 
                BufPtr& pBuf = elem.second;
                dwBlkIdx = ntohl( pbitd[ dwIdx2 ] );
                m_pAlloc->WriteBlock( dwBlkIdx,
                    ( guint8* )pBuf->ptr() );
            }
        }
        for( auto& elem: m_mapBlocks )
        {
            m_pAlloc->WriteBlock( elem.first,
                ( guint8* )elem.second->ptr() );
        }
    }while( 0 );
    return ret;
}

gint32 CFileImage::CollectBlocksForRead(
    guint32 dwOff, guint32 dwSize,
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

        for( int i = dwLead; i < dwTail; i++ )
        {
            guint32 dwCurPos = i * BLOCK_SIZE;
            bool bFirst =
                WITHIN_INDIRECT_BLOCK( dwCurPos );
            bool bSec = 
                WITHIN_SEC_INDIRECT_BLOCK( dwCurPos );

            if( WITHIN_DIRECT_BLOCK( dwCurPos ) )
            {
                // if the block is not allocated, push
                // a zero
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
                    p->Resize( BLOCK_SIZE );
                    ret = m_pAlloc->ReadBlock(
                        dwIndirectIdx,
                        ( guint8* )p->ptr() );
                    if( ERROR( ret ) )
                        break;
                }
                guint32* pbis =
                    ( guint32* )p->ptr();

                guint32 dwBlkIdx = ntohl(
                pbis[ INDIRECT_BLK_IDX( dwCurPos ) ] );

                vecBlocks.push_back( dwBlkIdx );
            }
            else if( bSec )
            {
                guint32 dwBitIdx =
                    SEC_BIT_IDX( dwCurPos );

                auto p = ( guint32* )
                    m_pBitdBlk->ptr();    

                guint32 dwBlkIdxIdx =
                    ntohl( p[ dwBitIdx ] );
                if( dwBlkIdxIdx == 0 )
                {
                    vecBlocks.push_back( 0 );
                    continue;
                }
                CStdRMutex oExclLock( GetExclLock() );
                guint32 dwIdx2 =
                    SEC_BLKIDX_IDX( dwCurPos );
                auto itr = m_mapSecBitBlks.find( 
                    dwBitIdx );
                if( itr != m_mapSecBitBlks.end() )
                {
                    auto pblk = ( guint32* )
                        itr->second->ptr();
                    vecBlocks.push_back( 
                        ntohl( pblk[ dwIdx2 ] ) );
                }
                else
                {
                    BufPtr p( true );
                    p->Resize( BLOCK_SIZE );
                    ret = m_pAlloc->ReadBlock(
                        dwBlkIdxIdx,
                        ( guint8* )p->ptr() );

                    if( ERROR( ret ) )
                        break;
                    auto pblk = ( guint32* )p->ptr();
                    vecBlocks.push_back(
                        ntohl( pblk[ dwIdx2 ] ) );
                    m_mapSecBitBlks.insert(
                        { dwBitIdx, p } );
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
    guint32 dwOff, guint32 dwSize,
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

        for( int i = dwLead; i < dwTail; i++ )
        {
            guint32 dwCurPos = i * BLOCK_SIZE;
            bool bFirst =
                WITHIN_INDIRECT_BLOCK( dwCurPos );
            bool bSec = 
                WITHIN_SEC_INDIRECT_BLOCK( dwCurPos );

            if( WITHIN_DIRECT_BLOCK( dwCurPos ) )
            {
                guint32 dwDirectIdx =
                    m_oInodeStore.m_arrBlocks[ i ];
                if( dwDirectIdx == 0 )
                {
                    ret = m_pAlloc->AllocBlocks( 
                        &dwDirectIdx, 1 );
                    if( ERROR( ret ) )
                        break;
                    BufPtr p( true ); 
                    p->Resize( BLOCK_SIZE );
                    memset( p->ptr(), 0, BLOCK_SIZE );
                    guint32* arrBlks =
                        m_oInodeStore.m_arrBlocks;
                    arrBlks[ i ] = dwDirectIdx;
                }
                vecBlocks.push_back( dwDirectIdx );
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
                    p->Resize( BLOCK_SIZE );
                    ret = m_pAlloc->ReadBlock(
                        dwIndirectIdx,
                        ( guint8* )p->ptr() );
                    if( ERROR( ret ) )
                        break;
                }
                guint32* pbis =
                    ( guint32* )p->ptr();

                guint32 dwBlkIdx = ntohl( pbis[
                    INDIRECT_BLK_IDX( dwCurPos ) ] );

                if( dwBlkIdx == 0 )
                {
                    ret = m_pAlloc->AllocBlocks(
                        &dwBlkIdx, 1 );
                    if( ERROR( ret ) )
                        break;
                    pbis[ INDIRECT_BLK_IDX( dwCurPos ) ] =
                        htonl( dwBlkIdx );
                }

                vecBlocks.push_back( dwBlkIdx );
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
                    ret = m_pBitdBlk->Resize(
                        BLOCK_SIZE );
                    if( ERROR( ret ) )
                        break;
                    memset( m_pBitdBlk->ptr(),
                        0, m_pBitdBlk->size() );
                }
                guint32 dwBitIdx =
                    SEC_BIT_IDX( dwCurPos );

                auto p = ( guint32* )m_pBitdBlk->ptr();    
                guint32 dwBitBlkIdx =
                    ntohl( p[ dwBitIdx ] );
                if( dwBitBlkIdx == 0 )
                {
                    // allocate a block index table
                    ret = m_pAlloc->AllocBlocks(
                        &dwBitBlkIdx, 1 );
                    if( ERROR( ret ) )
                        break;
                    DebugPrint( ret, "Allocated "
                        "secondary bit directory at %d",
                        dwBitBlkIdx );

                    BufPtr pBuf( true );
                    pBuf->Resize( BLOCK_SIZE );
                    memset( pBuf->ptr(), 0, pBuf->size() );

                    ret = m_pAlloc->WriteBlock(
                        dwBitBlkIdx,
                        ( guint8* )pBuf->ptr() );
                    if( ERROR( ret ) )
                        break;

                    p[ dwBitIdx ] = htonl( dwBitBlkIdx );
                    m_mapSecBitBlks.insert(
                        { dwBitIdx, pBuf } );
                }
                guint32 dwIdx2 =
                    SEC_BLKIDX_IDX( dwCurPos );

                auto itr = m_mapSecBitBlks.find( 
                    dwBitIdx );
                if( itr != m_mapSecBitBlks.end() )
                {
                    auto pblk = ( guint32* )
                        itr->second->ptr();

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
                    pblk[ dwIdx2 ] =
                        htonl( dwBlkIdx );
                    vecBlocks.push_back( dwBlkIdx );
                    continue;
                }
                else
                {
                    BufPtr pBitBlk( true );
                    pBitBlk->Resize( BLOCK_SIZE );
                    memset( pBitBlk->ptr(),
                        0, pBitBlk->size() );
                    ret = m_pAlloc->ReadBlock(
                        dwBitBlkIdx,
                        ( guint8* )pBitBlk->ptr() );
                    if( ERROR( ret ) )
                        break;

                    auto pblk = ( guint32* )
                        pBitBlk->ptr();

                    auto dwBlkIdx = ( guint32 )
                        ntohl( pblk[ dwIdx2 ] );
                    if( dwBlkIdx == 0 )
                    {
                        ret = m_pAlloc->AllocBlocks(
                            &dwBlkIdx, 1 );
                        if( ERROR( ret ) )
                            break;
                        pblk[ dwIdx2 ] =
                            htonl( dwBlkIdx );
                    }
                    vecBlocks.push_back( dwBlkIdx );
                    m_mapSecBitBlks.insert(
                       { dwBitIdx, pBitBlk } );
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
        READ_LOCK( this );
        if( dwOff >= GetSize() )
        {
            dwSize = 0;
            break;
        }
        if( dwOff + dwSize > GetSize() )
            dwSize = GetSize() - dwOff;

        std::vector< guint32 > vecBlks;
        ret = CollectBlocksForRead(
            dwOff, dwSize, vecBlks );
        if( ERROR( ret ) )
            break;
        DECL_ARRAY( arrBytes, BLOCK_SIZE );

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
                vecBlks.back(),
                ( guint8* )arrBytes );
            if( ERROR( ret ) )
                break;
            memcpy( pBuf +
                BLOCK_SIZE * ( dwBlocks - 1 ),
                arrBytes, dwTail );
        }
        else if( dwHead > 0 && dwTail == 0 )
        {
            ret = m_pAlloc->ReadBlock(
                vecBlks.front(), 
                ( guint8* )arrBytes );
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
                vecBlks.front(),
                ( guint8* )arrBytes );
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
                vecBlks.back(), 
                ( guint8* )arrBytes );
            if( ERROR( ret ) )
                break;
            memcpy( pCur, arrBytes, dwTail );
        }

    }while( 0 );
    return ret;
}

gint32 CFileImage::WriteFile(
    guint32 dwOff, guint32& dwSize,
    guint8* pBuf )
{
    gint32 ret = 0;
    do{
        WRITE_LOCK( this );
        ret = WriteFileNoLock(
            dwOff, dwSize, pBuf );
    }while( 0 );
    return ret;
}
gint32 CFileImage::WriteFileNoLock(
    guint32 dwOff, guint32& dwSize,
    guint8* pBuf )
{
    gint32 ret = 0;
    if( pBuf == nullptr )
        return -EINVAL;
    if( BEYOND_MAX_LIMIT( dwOff + dwSize ) )
        return -ERANGE;

    do{
        std::vector< guint32 > vecBlks;
        ret = CollectBlocksForWrite(
            dwOff, dwSize, vecBlks );
        if( ERROR( ret ) )
            break;

        DECL_ARRAY( arrBytes, BLOCK_SIZE );

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
                vecBlks.back(),
                ( guint8* )arrBytes );
            if( ERROR( ret ) )
                break;
            if( ret < BLOCK_SIZE )
            {
                DebugPrint( ret, "read block "
                    "returned less bytes" );
            }
            memcpy( arrBytes,
                pBuf + BLOCK_SIZE * ( dwBlocks - 1 ),
                dwTail );
            ret = m_pAlloc->WriteBlock(
                vecBlks.back(),
                ( guint8* )arrBytes );
        }
        else if( dwHead > 0 && dwTail == 0 )
        {
            ret = m_pAlloc->ReadBlock(
                vecBlks.front(), 
                ( guint8* )arrBytes );
            if( ERROR( ret ) )
                break;

            memcpy( arrBytes + dwHead, pBuf,
                ( BLOCK_SIZE - dwHead ) );

            ret = m_pAlloc->WriteBlock(
                vecBlks.front(),
                ( guint8* )arrBytes );

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
                vecBlks.front(),
                ( guint8* )arrBytes );
            if( ERROR( ret ) )
                break;
            if( dwBlocks == 1 )
            {
                memcpy( arrBytes + dwHead,
                    pBuf, dwSize );
                ret = m_pAlloc->WriteBlock(
                    vecBlks.front(),
                    ( guint8* )arrBytes );
                break;
            }
            else
            {
                memcpy( arrBytes + dwHead, pBuf,
                    ( BLOCK_SIZE - dwHead ) );
                ret = m_pAlloc->WriteBlock(
                    vecBlks.front(), 
                    ( guint8* )arrBytes );
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
                vecBlks.back(),
                ( guint8* )arrBytes );
            if( ERROR( ret ) )
                break;
            memcpy( arrBytes, pCur, dwTail );
            ret = m_pAlloc->WriteBlock(
                vecBlks.back(), 
                ( guint8* )arrBytes );
        }

        if( ERROR( ret ) )
            break;

        if( dwSize + dwOff > GetSize() )
            SetSize( dwSize + dwOff );

        timespec mtime;
        clock_gettime( CLOCK_REALTIME, &mtime );
        m_oInodeStore.m_mtime = mtime;
        m_oInodeStore.m_ctime = mtime;

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
        if( m_pBitBlk.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        guint32 dwIndStart =
            ( INDIRECT_BLOCK_START >> BLOCK_SHIFT);

        guint32 i = lablkidx - dwIndStart;
        guint32 dwCount = std::min(
            ( ( GetSize() + BLOCK_SIZE - 1 ) >> BLOCK_SHIFT ) - lablkidx,
            BLKIDX_PER_TABLE - i );

        if( i >= BLKIDX_PER_TABLE )
        {
            ret = -ERANGE;
            break;
        }

        auto pbit = ( guint32* )m_pBitBlk->ptr();

        ret = m_pAlloc->FreeBlocks(
            pbit + i, dwCount, true );

        if( ERROR( ret ) )
            break;

        guint32 j = i;
        for( ; j < i + dwCount; j++ )
            pbit[ j ] = 0;

        if( i == 0 )
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
        if( m_pBitdBlk.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        guint32 dwByteStart =
            ( lablkidx << BLOCK_SHIFT );
        guint32 dwSecStart =
            SEC_INDIRECT_BLOCK_START;

        guint32 dwByteEnd = GetSize();

        guint32 dwBitIdx =
            SEC_BIT_IDX( dwByteStart );

        guint32 dwBlkIdxIdx =
            SEC_BLKIDX_IDX( dwByteStart );

        guint32 dwBitEndIdx = SEC_BIT_IDX(
            dwByteEnd + BLOCK_SIZE - 1 );

        guint32 dwBlkEndIdxIdx = SEC_BLKIDX_IDX(
            dwByteEnd + BLOCK_SIZE - 1 );

        guint32 dwBitCount = std::min(
            dwBitEndIdx - dwBitIdx + 1,
            BLKIDX_PER_TABLE - dwBitIdx );

        guint32 dwLastBlkIdxCount =
            dwBitEndIdx > dwBitIdx ? dwBlkEndIdxIdx : 
            dwBlkEndIdxIdx - dwBlkIdxIdx; 

        auto pbitd = ( guint32* ) m_pBitdBlk->ptr();

        guint32 i = dwBitIdx + 1;
        for( ; i < dwBitCount; i++ )
        {
            guint32 dwBitBlkIdx = ntohl( pbitd[ i ] );
            guint32 dwBlksToFree;
            if( i < dwBitCount - 1 )
                dwBlksToFree = BLKIDX_PER_TABLE;
            else
                dwBlksToFree = dwLastBlkIdxCount;

            auto itr = m_mapSecBitBlks.find( i );
            if( itr != m_mapSecBitBlks.end() )
            {
                auto p = ( guint32* )
                    itr->second->ptr();
                m_pAlloc->FreeBlocks( p,
                    dwBlksToFree, true );
                m_mapSecBitBlks.erase( itr );
            }
            else if( dwBitBlkIdx != 0 )
            {
                guint32 arrIdx[
                    BLOCK_SIZE / sizeof( guint32 ) ];
                auto p = ( guint8* )arrIdx;
                ret = m_pAlloc->ReadBlock(
                    dwBitBlkIdx, p );
                if( ERROR( ret ) )
                    break;
                ret = m_pAlloc->FreeBlocks(
                    arrIdx, dwBlksToFree, true );
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
            // the first block-index-table
            guint32 dwBlksToFree;
            if( dwBitCount == 1 )
                dwBlksToFree = dwLastBlkIdxCount;
            else
            {
                dwBlksToFree =
                    BLKIDX_PER_TABLE - dwBlkIdxIdx;
            }
            guint32 dwBitBlkIdx =
                ntohl( pbitd[ dwBitIdx ] );

            auto itr = m_mapSecBitBlks.find( dwBitIdx );
            if( itr != m_mapSecBitBlks.end() )
            {
                auto p = ( guint32* )
                    itr->second->ptr();

                guint32* pblkidx = p + dwBlkIdxIdx;
                m_pAlloc->FreeBlocks(
                    pblkidx, dwBlksToFree, true );
                guint32 i = dwBlkIdxIdx;
                for( ; i < dwBlksToFree; i++ )
                    pblkidx[ i ] = 0;
            }
            else if( dwBitBlkIdx != 0 )
            {
                BufPtr p( true );
                p->Resize( BLOCK_SIZE );
                ret = m_pAlloc->ReadBlock(
                    dwBitBlkIdx,
                    ( guint8* )p->ptr() );
                if( ERROR( ret ) )
                    break;

                guint32* pbit = ( guint32* )p->ptr();

                ret = m_pAlloc->FreeBlocks(
                    pbit + dwBlkIdxIdx,
                    dwBlksToFree, true );
                if( ERROR( ret ) )
                    break;

                if( dwBlkIdxIdx > 0 )
                {
                    guint32 j = dwBlkIdxIdx;
                    for(; j < dwBlksToFree; j++ )
                        pbit[ j ] = 0;
                    m_mapSecBitBlks.insert(
                        { dwBitIdx, p } );
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
        if( dwByteStart ==
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
    do{
        WRITE_LOCK( this );
        ret = TruncateNoLock( dwOff );
    }while( 0 );
    return ret;
}
gint32 CFileImage::TruncateNoLock( guint32 dwOff )
{
    gint32 ret = 0;
    if( BEYOND_MAX_LIMIT( dwOff ) )
        return -ERANGE;

    do{
        if( dwOff > GetSize() )
        {
            ret = Extend( dwOff );
            break;
        }
        if( dwOff == GetSize() )
            break;

        guint32 dwDelta = GetSize() - dwOff;

        guint32 dwHead =
            ( dwOff & ( BLOCK_SIZE - 1 ) );

        guint32 dwTail = ( ( dwOff + dwDelta ) &
                ( BLOCK_SIZE - 1 ) );

        guint32 dwBlocks = 0;
        guint32 dwBytes = dwDelta;
        guint32 dwTruncOff = UINT_MAX;

        if( dwHead > 0 )
        {
            dwBytes -= std::min(
                ( BLOCK_SIZE - dwHead ),
                dwDelta );
        }

        if( dwBytes > 0 )
        {
            dwTruncOff =
                ( ( dwOff + BLOCK_SIZE - 1 ) &
                ( ~( BLOCK_SIZE - 1 ) ) );
        }

        if( dwTruncOff == UINT_MAX )
        {
            // no need to truncate
            SetSize( dwOff );
            break;
        }

        guint32 dwTruncIdx = 
            ( dwTruncOff >> BLOCK_SHIFT );

        guint32 dwSecStart =
        ( SEC_INDIRECT_BLOCK_START >> BLOCK_SHIFT );

        guint32 dwIndStart =
            ( INDIRECT_BLOCK_START >> BLOCK_SHIFT );

        if( WITHIN_SEC_INDIRECT_BLOCK( dwTruncOff ) )
        {
            ret = TruncBlkSecIndirect( dwTruncIdx );
        }
        else if( WITHIN_INDIRECT_BLOCK( dwTruncOff ) )
        {
            if( WITHIN_SEC_INDIRECT_BLOCK( GetSize() ) )
            {
                ret = TruncBlkSecIndirect( dwSecStart );
                if( ERROR( ret ) )
                    break;
            }
            ret = TruncBlkIndirect( dwTruncIdx );
        }
        else
        {
            // truncate from secondary blocks to direct blocks
            if( WITHIN_SEC_INDIRECT_BLOCK( GetSize() ) )
            {
                ret = TruncBlkSecIndirect(
                    dwSecStart );
                if( ERROR( ret ) )
                    break;
                ret = TruncBlkIndirect(
                    dwIndStart );
            }
            else if( WITHIN_INDIRECT_BLOCK( GetSize() ) )
            {
                ret = TruncBlkIndirect(
                    dwIndStart );
                if( ERROR( ret ) )
                    break;
            }
            ret = TruncBlkDirect( dwTruncIdx );
        }
        SetSize( dwOff );
        timespec mtime;
        clock_gettime( CLOCK_REALTIME, &mtime );
        m_oInodeStore.m_mtime = mtime;
        m_oInodeStore.m_ctime = mtime;

    }while( 0 );
    return ret;
}

gint32 CFileImage::Extend( guint32 dwOff )
{
    gint32 ret = 0;
    do{
        guint32 laddr =
            ( dwOff & ~( BLOCK_SIZE - 1 ) );
        DECL_ARRAY( arrBytes, BLOCK_SIZE );
        memset( arrBytes, 0, BLOCK_SIZE );
        guint32 dwBlkOff =
            ( dwOff & ( BLOCK_SIZE - 1 ) );
        ret = WriteFileNoLock( laddr, dwBlkOff,
            ( guint8* )arrBytes );
    }while( 0 );
    return ret;
}

gint32 CFileImage::ReadValue(
    Variant& oVar ) const
{
    gint32 ret = 0;
    do{
        READ_LOCK( this );
        oVar = m_oValue;
    }while( 0 );
    return ret;
}

gint32 CFileImage::WriteValue(
    const Variant& oVar )
{
    gint32 ret = 0;
    do{
        WRITE_LOCK( this );
        m_oValue = oVar;
    }while( 0 );
    return ret;
}

void CFileImage::SetGid( gid_t wGid )
{
    gint32 ret = 0;
    do{
        WRITE_LOCK( this );
        m_oInodeStore.m_wgid = ( guint16 )wGid;
    }while( 0 );
}

void CFileImage::SetUid( uid_t wUid )
{
    gint32 ret = 0;
    do{
        WRITE_LOCK( this );
        m_oInodeStore.m_wuid = ( guint16 )wUid;
    }while( 0 );
}

void CFileImage::SetMode( mode_t dwMode )
{
    gint32 ret = 0;
    do{
        WRITE_LOCK( this );
        m_oInodeStore.m_dwMode =
            ( guint32 )dwMode;
    }while( 0 );
    return;
}

void CFileImage::SetTimes(
    const struct timespec tv[ 2 ] )
{
    gint32 ret = 0;
    do{
        WRITE_LOCK( this );
        auto& s = m_oInodeStore;
        for( int i = 0; i < 2; i++ )
        {
            if( tv[ i ].tv_nsec == UTIME_OMIT )
                continue;
            if( tv[ i ].tv_nsec != UTIME_NOW )
            {
                if( i == 0 )
                    s.m_atime = tv[ i ];
                else
                    s.m_mtime = tv[ i ];
                continue;
            }
            timespec ot;
            clock_gettime( CLOCK_REALTIME, &ot );
            if( i == 0 )
                s.m_atime = ot;
            else
                s.m_mtime = ot;
        }
    }while( 0 );
    return;
}

gid_t CFileImage::GetGid() const
{
    gint32 ret = 0;
    do{
        READ_LOCK( this );
        return GetGidNoLock(); 
    }while( 0 );
    return ret;
}

uid_t CFileImage::GetUid() const
{
    gint32 ret = 0;
    do{
        READ_LOCK( this );
        return GetUidNoLock();
    }while( 0 );
    return ret;
}

mode_t CFileImage::GetMode() const
{
    gint32 ret = 0;
    do{
        READ_LOCK( this );
        return GetModeNoLock();
    }while( 0 );
    return ret;
}

gint32 CFileImage::GetAttr( struct stat& stBuf )
{
    gint32 ret = 0;
    do{
        READ_LOCK( this );
        stBuf.st_uid = this->GetUidNoLock();
        stBuf.st_gid = this->GetGidNoLock();
        stBuf.st_atim = m_oInodeStore.m_atime;
        stBuf.st_mtim = m_oInodeStore.m_mtime;
        stBuf.st_ctim = m_oInodeStore.m_ctime;
        stBuf.st_mode = this->GetModeNoLock();
        stBuf.st_ino = this->GetInodeIdx();

        guint32 fs = this->GetSize();
        guint32 bs = BLOCK_SIZE;
        stBuf.st_size = fs;
        stBuf.st_blksize = bs;
        stBuf.st_blocks = ( fs + bs - 1 ) / bs;

    }while( 0 );
    return ret;
}
gint32 CFileImage::CheckAccess(
    mode_t dwReq )
{
    gint32 ret = 0;
    do{
        mode_t dwMode = this->GetModeNoLock();
        uid_t dwUid = this->GetUidNoLock();
        gid_t dwGid = this->GetGidNoLock();

        uid_t dwCurUid = geteuid();
        gid_t dwCurGid = getegid();

        guint32 dwReadFlag;
        guint32 dwWriteFlag;
        guint32 dwExeFlag;
        if( dwCurUid == dwUid )
        {
            dwReadFlag = S_IRUSR;
            dwWriteFlag = S_IWUSR;
            dwExeFlag = S_IXUSR;
        }
        else if( dwCurGid == dwGid )
        {
            dwReadFlag = S_IRGRP;
            dwWriteFlag = S_IWGRP;
            dwExeFlag = S_IXGRP;
        }
        else
        {
            dwReadFlag = S_IROTH;
            dwWriteFlag = S_IWOTH;
            dwExeFlag = S_IXOTH;
        }

        ret = -EACCES;
        bool bReq =
           (  ( dwReq & W_OK ) != 0 );
        bool bCur =
           ( ( dwMode & dwWriteFlag ) != 0 );

        if( !bCur && bReq )
            break;

        bReq = (  ( dwReq & R_OK ) != 0 );
        bCur = ( ( dwMode & dwReadFlag ) != 0 );
        if( !bCur && bReq )
            break;

        bReq = ( ( dwReq & X_OK ) != 0 );
        bCur = ( ( dwMode & dwExeFlag ) != 0 );
        if( !bCur && bReq )
            break;
        ret = 0;
    }while( 0 );
    return ret;
}

gint32 CLinkImage::Reload()
{
    gint32 ret = 0;
    do{
        ret = super::Reload();
        BufPtr pBuf( true );
        guint32 dwSize = this->GetSize();
        ret = pBuf->Resize( dwSize + 1 );
        if( ERROR( ret ) )
            break;
        ret = ReadFile( 0, dwSize,
            ( guint8* )pBuf->ptr() );
        if( ERROR( ret ) )
            break;
        auto szLink = pBuf->ptr();
        szLink[ dwSize ] = 0;
        m_strLink = szLink;
    }while( 0 );
    return ret;
}

gint32 CLinkImage::Format()
{
    gint32 ret = super::Format();
    if( ERROR( ret ) )
        return ret;
    m_oInodeStore.m_dwMode =
        ( S_IFLNK | 0777 );
    return ret;
}

gint32 CLinkImage::ReadLink(
    guint8* buf, guint32& dwSize )
{
    guint32 dwBytes =
        std::min( m_strLink.size(),
            ( size_t )dwSize );
    strncpy( ( char* )buf,
        m_strLink.c_str(), dwBytes );
    dwSize = dwBytes;
    return 0;
}

gint32 COpenFileEntry::Flush( guint32 dwFlags )
{
    gint32 ret = 0;
    do{
        WRITE_LOCK( m_pFileImage );
        ret = m_pFileImage->Flush( dwFlags );
    }while( 0 );
    return ret;
}

gint32 COpenFileEntry::ReadFile(
    guint32& dwSize, guint8* pBuf,
    guint32 dwOff )
{
    gint32 ret = m_pFileImage->ReadFile(
        dwOff, dwSize, pBuf );
    if( ERROR( ret ) )
        return ret;

    return STATUS_SUCCESS;
}

gint32 COpenFileEntry::WriteFile(
    guint32& dwSize, guint8* pBuf,
    guint32 dwOff  )
{
    gint32 ret = m_pFileImage->WriteFile(
        dwOff, dwSize, pBuf );
    if( ERROR( ret ) )
        return ret;

    return STATUS_SUCCESS;
}

}
