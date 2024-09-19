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
        // the block address for the parent directory
        m_oInodeStore.m_dwParentInode =
            ntohl( pInode->m_dwParentInode );
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
                m_oVal = *( ( float* )&dwVal );
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

        // the block address for the parent directory
        m_oInodeStore.m_dwParentInode = 0;

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
        // the block address for the parent directory
        pInode->m_dwParentInode =
            htonl( m_oInodeStore.m_dwParentInode );
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

    }while( 0 );
    return ret;
}

gint32 CFileImage::ReadFile(
    guint32 dwOff, guint32 size,
    guint8* pBuf )
{
}

gint32 CFileImage::WriteFile(
    guint32 dwOff, guint32 size,
    guint8* pBuf )
{
}


}
