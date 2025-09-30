/*
 * =====================================================================================
 *
 *       Filename:  serial.cpp
 *
 *    Description:  implementations of serialization methods
 *
 *        Version:  1.0
 *        Created:  11/23/2017 09:05:29 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2019 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#include <map>
#include <string>
#include <algorithm>
#include <vector>

#include "defines.h"
#include "configdb.h"
#include "stlcont.h"
#define  HEADER_NBSIZE ( sizeof( SERI_HEADER ) - sizeof( SERI_HEADER_BASE ) )
namespace rpcf
{

gint32 CStlIntVector::Serialize(
    CBuffer& oBuf ) const
{
    SERI_HEADER oHeader;

    guint32 dwSize = 
        sizeof( ElemType ) * m_vecElems.size();

    oHeader.dwClsid = clsid( CStlIntVector );
    oHeader.dwSize = dwSize + HEADER_NBSIZE;

    oBuf.Resize(
        sizeof( oHeader ) + dwSize );

    oHeader.dwCount = m_vecElems.size();

    oHeader.hton();
    memcpy( oBuf.ptr(), &oHeader, sizeof( oHeader ) );

    guint32 *pElem = ( guint32* )
        ( oBuf.ptr() + sizeof( oHeader ) );

    for( auto i : m_vecElems )
        *pElem++ = htonl( i );

    return 0;
}

gint32 CStlIntVector::Deserialize(
    const CBuffer& oBuf )
{
    if( oBuf.empty() )
        return -EINVAL;

    return Deserialize(
        oBuf.ptr(), oBuf.size() );
}

gint32 CStlIntVector::Deserialize(
    const char* pBuf, guint32 dwBufSize )
{
    const SERI_HEADER* pHeader =
        ( SERI_HEADER* )pBuf;

    if( pHeader == nullptr )
        return -EINVAL;

    if( dwBufSize < sizeof( SERI_HEADER ) )
        return -EINVAL;

    SERI_HEADER oHeader( *pHeader );
    // memcpy( &oHeader, pHeader, sizeof( oHeader ) );
    oHeader.ntoh();

    m_vecElems.clear();
    gint32 ret = 0;

    do{
        if( oHeader.dwClsid != clsid( CStlIntVector ) )
        {
            ret = -ENOTSUP;
            break;
        }

        if( oHeader.bVersion != 1 )
        {
            ret = -ENOTSUP;
        }

        if( oHeader.dwSize > BUF_MAX_SIZE ) 
        {
            ret = -EINVAL;
            break;
        }

        oHeader.dwSize -= HEADER_NBSIZE;
        if( oHeader.dwSize / sizeof( ElemType ) !=
            oHeader.dwCount )
        {
            ret = -EINVAL;
            break;
        }

        guint32* pElems = ( guint32* )( pHeader + 1 );
        for( guint32 i = 0; i < oHeader.dwCount; ++i )
        {
            m_vecElems.push_back(
                ntohl( pElems[ i ] ) );
        }

    }while( 0 );

    return ret;
}

gint32 CStlIntMap::Serialize(
    CBuffer& oBuf ) const
{
    SERI_HEADER oHeader;

    guint32 dwSize = 2 *
        sizeof( guint32 ) * m_mapElems.size();

    oHeader.dwClsid = clsid( CStlIntMap );
    oHeader.dwSize = dwSize + HEADER_NBSIZE;

    oBuf.Resize(
        sizeof( oHeader ) + dwSize );

    oHeader.dwCount = m_mapElems.size();

    oHeader.hton();
    memcpy( oBuf.ptr(), &oHeader, sizeof( oHeader ) );

    guint32 *pElem = ( guint32* )
        ( oBuf.ptr() + sizeof( oHeader ) );

    for( auto oPair : m_mapElems )
    {
        *pElem++ = htonl( oPair.first );
        *pElem++ = htonl( oPair.second );
    }

    return 0;
}

gint32 CStlIntMap::Deserialize(
    const char* pBuf, guint32 dwBufSize )
{
    const SERI_HEADER* pHeader =
        ( SERI_HEADER* )pBuf;

    if( pHeader == nullptr )
        return -EINVAL;

    if( dwBufSize < sizeof( SERI_HEADER ) )
        return -EINVAL;

    SERI_HEADER oHeader( *pHeader );
    // memcpy( &oHeader, pHeader, sizeof( oHeader ) );

    oHeader.ntoh();
    m_mapElems.clear();

    gint32 ret = 0;
    do{
        if( oHeader.dwClsid != clsid( CStlIntVector ) )
        {
            ret = -ENOTSUP;
            break;
        }

        if( oHeader.bVersion != 1 )
        {
            ret = -ENOTSUP;
        }

        if( oHeader.dwSize > BUF_MAX_SIZE ) 
        {
            ret = -EINVAL;
            break;
        }

        oHeader.dwSize -= HEADER_NBSIZE;
        if( oHeader.dwSize / sizeof( ElemType ) !=
            oHeader.dwCount )
        {
            ret = -EINVAL;
            break;
        }
        guint32* pElems = &oHeader.arrElems[ 0 ].first;
        for( guint32 i = 0; i < oHeader.dwCount; ++i )
        {
            guint32 dwKey = *pElems++;
            guint32 dwVal = *pElems++;
            m_mapElems[ dwKey ] = dwVal;
        }

    }while( 0 );

    return ret;
}

gint32 CStlIntMap::Deserialize(
    const CBuffer& oBuf )
{
    if( oBuf.empty() )
        return -EINVAL;

    return Deserialize(
        oBuf.ptr(), oBuf.size() );
}

#define ALIGN_MASK ( sizeof( guint32 ) - 1 )

gint32 CStlObjVector::Serialize(
    CBuffer& oBuf ) const
{
    gint32 ret = 0;
    do{
        guint32 dwOffsetCount = m_vecElems.size() + 1;
        guint32 dwPos = 0;

        oBuf.Resize( sizeof( SERI_HEADER ) +
            dwOffsetCount * ( ALIGN_MASK + 1 ) );

        if( ERROR( ret ) )
            break;

        SERI_HEADER* pHeader =
            ( SERI_HEADER* )oBuf.ptr();

        new ( pHeader )SERI_HEADER();

        BufPtr pBufPadding( true );
        pBufPadding->Resize( 4 );
        *( ( guint32*) pBufPadding->ptr() ) = 0;

        for( guint32 i = 0; i < m_vecElems.size(); ++i )
        {
            pHeader->arrOffsets[ i ] = dwPos;
            ObjPtr pObj = m_vecElems[ i ];
            BufPtr pBuf( true );

            if( unlikely( pObj.IsEmpty() ) )
            {
                ret = -EFAULT;
                break;
            }

            ret = pObj->Serialize( *pBuf );
            if( ERROR( ret ) )
                break;

            ret = oBuf.Append( ( const guint8* )
                pBuf->ptr(), pBuf->size() );

            if( ERROR( ret ) )
                break;

            dwPos += pBuf->size();
            if( dwPos & ALIGN_MASK )
            {
                // put the offset to the align boundary
                guint32 dwPaddingSize =
                    ( ALIGN_MASK + 1 ) -
                    ( dwPos & ALIGN_MASK );

                pBufPadding->Resize( dwPaddingSize );

                oBuf.Append( ( const guint8*)
                    pBufPadding->ptr(),
                    pBufPadding->size() );

                dwPos += dwPaddingSize;
            }

            pHeader = ( SERI_HEADER* )oBuf.ptr();
        }

        // set the last offset to the payload size,
        // excluding the header section
        pHeader->arrOffsets[ dwOffsetCount - 1 ] = dwPos;
        pHeader->dwSize = oBuf.size() -
            sizeof( SERI_HEADER_BASE );
        pHeader->dwCount = dwOffsetCount;
        pHeader->hton();

    }while( 0 );

    return ret;
}

gint32 CStlObjVector::Deserialize(
    const CBuffer& oBuf )
{
    if( oBuf.empty() )
        return -EINVAL;

    return Deserialize(
        oBuf.ptr(), oBuf.size() );
}

gint32 CStlObjVector::Deserialize(
    const char* pBuf, guint32 dwBufSize )
{
    gint32 ret = 0;
    do{
        if( dwBufSize < sizeof( SERI_HEADER ) )
        {
            ret = -EINVAL;
            break;
        }

        const SERI_HEADER* pHeader =
            ( const SERI_HEADER* )pBuf;

        SERI_HEADER oHdr1;
        memcpy( &oHdr1, pBuf,
            sizeof( SERI_HEADER ) );

        oHdr1.ntoh();
        if( oHdr1.dwClsid !=
            clsid( CStlObjVector ) )
        {
            ret = -EINVAL;
            break;
        }

        if( dwBufSize <
            oHdr1.dwSize + HEADER_NBSIZE )
        {
            ret = -ERANGE;
            break;
        }

        if( oHdr1.dwCount > MAX_ELEM_CONTAINER )
        {
            ret = -ERANGE;
            break;
        }

        BufPtr pHdrBuf( true );
        pHdrBuf->Resize( sizeof( SERI_HEADER ) +
            sizeof( guint32 ) * oHdr1.dwCount );

        SERI_HEADER* pFullHdr =
            ( SERI_HEADER* )pHdrBuf->ptr();
        memcpy( pFullHdr, pBuf, pHdrBuf->size() );
        SERI_HEADER& oFullHdr = *pFullHdr;

        oFullHdr.ntoh( false );
        guint32 dwElemCount = oFullHdr.dwCount - 1;

        const char* pStart =
            pBuf + sizeof( SERI_HEADER ) +
            ( ALIGN_MASK + 1 ) * ( oFullHdr.dwCount );

        if( pStart - pBuf > ( gint32 )dwBufSize )
        {
            ret = -EINVAL;
            break;
        }

        m_vecElems.clear();

        for( guint32 i = 0; i < dwElemCount; ++i )
        {
            guint32 dwPos =
                oFullHdr.arrOffsets[ i ];

            guint32 dwObjSize =
                oFullHdr.arrOffsets[ i + 1 ] - dwPos;

            const SERI_HEADER_BASE* pObjBase =
                ( SERI_HEADER_BASE* )( pStart + dwPos );

            if( ( ( const char* )pObjBase ) - pBuf >=
                ( gint32 )dwBufSize )
            {
                ret = -ENOMEM;
                break;
            }
            EnumClsid iClsid = ( EnumClsid )
                ntohl( pObjBase->dwClsid );

            ObjPtr pObj;
            ret = pObj.NewObj( iClsid );
            if( ERROR( ret ) )
                break;

            ret = pObj->Deserialize(
                ( const char* )pObjBase, dwObjSize );

            if( ERROR( ret ) )
                break;

            m_vecElems.push_back( pObj );
        }

    }while( 0 );

    return ret;
}

gint32 CStlLongWordVector::Serialize(
    CBuffer& oBuf ) const
{
    SERI_HEADER oHeader;

    guint32 dwSize = 
        sizeof( ElemType ) * m_vecElems.size();

    oHeader.dwClsid = clsid( CStlLongWordVector );
    oHeader.dwSize = dwSize + HEADER_NBSIZE ;

    oBuf.Resize(
        sizeof( oHeader ) + dwSize );

    oHeader.dwCount = m_vecElems.size();

    oHeader.hton();
    memcpy( oBuf.ptr(), &oHeader, sizeof( oHeader ) );

    LONGWORD *pElem = ( LONGWORD* )
        ( oBuf.ptr() + sizeof( oHeader ) );

    for( auto i : m_vecElems )
    {
#if ( BUILD_64 == 1 )
            *pElem++ = htonll( i );
#else
            *pElem++ = htonl( i );
#endif
    }

    return 0;
}

gint32 CStlLongWordVector::Deserialize(
    const char* pBuf, guint32 dwBufSize )
{
    const SERI_HEADER* pHeader =
        ( SERI_HEADER* )pBuf;

    if( pHeader == nullptr )
        return -EINVAL;

    if( dwBufSize < sizeof( SERI_HEADER ) )
        return -EINVAL;

    SERI_HEADER oHeader( *pHeader );
    // memcpy( &oHeader, pHeader, sizeof( oHeader ) );
    oHeader.ntoh();

    m_vecElems.clear();
    gint32 ret = 0;

    do{
        if( oHeader.dwClsid != clsid( CStlLongWordVector ) )
        {
            ret = -ENOTSUP;
            break;
        }

        if( oHeader.bVersion != 1 )
        {
            ret = -ENOTSUP;
        }

        if( oHeader.dwSize > BUF_MAX_SIZE ) 
        {
            ret = -EINVAL;
            break;
        }

        oHeader.dwSize -= HEADER_NBSIZE;
        if( oHeader.dwSize / sizeof( ElemType ) !=
            oHeader.dwCount )
        {
            ret = -EINVAL;
            break;
        }

        LONGWORD* pElems =
            ( LONGWORD* )( pHeader + 1 );
        for( guint32 i = 0; i < oHeader.dwCount; ++i )
        {
#if( BUILD_64 == 1 )
                m_vecElems.push_back(
                    ntohll( pElems[ i ] ) );
#else
                m_vecElems.push_back(
                    ntohl( pElems[ i ] ) );
#endif
        }

    }while( 0 );

    return ret;
}

gint32 CStlLongWordVector::Deserialize(
    const CBuffer& oBuf )
{
    if( oBuf.empty() )
        return -EINVAL;

    return Deserialize(
        oBuf.ptr(), oBuf.size() );
}

gint32 CStlQwordVector::Serialize(
    CBuffer& oBuf ) const
{
    SERI_HEADER oHeader;

    guint32 dwSize = 
        sizeof( ElemType ) * m_vecElems.size();

    oHeader.dwClsid = clsid( CStlQwordVector );
    oHeader.dwSize = dwSize + HEADER_NBSIZE ;

    oBuf.Resize(
        sizeof( oHeader ) + dwSize );

    oHeader.dwCount = m_vecElems.size();

    oHeader.hton();
    memcpy( oBuf.ptr(), &oHeader, sizeof( oHeader ) );

    guint64 *pElem = ( guint64* )
        ( oBuf.ptr() + sizeof( oHeader ) );

    for( auto i : m_vecElems )
    {
        *pElem++ = htonll( i );
    }

    return 0;
}

gint32 CStlQwordVector::Deserialize(
    const char* pBuf, guint32 dwBufSize )
{
    const SERI_HEADER* pHeader =
        ( SERI_HEADER* )pBuf;

    if( pHeader == nullptr )
        return -EINVAL;

    if( dwBufSize < sizeof( SERI_HEADER ) )
        return -EINVAL;

    SERI_HEADER oHeader( *pHeader );
    oHeader.ntoh();

    m_vecElems.clear();
    gint32 ret = 0;

    do{
        if( oHeader.dwClsid != clsid( CStlQwordVector ) )
        {
            ret = -ENOTSUP;
            break;
        }

        if( oHeader.bVersion != 1 )
        {
            ret = -ENOTSUP;
        }

        if( oHeader.dwSize > BUF_MAX_SIZE ) 
        {
            ret = -EINVAL;
            break;
        }

        oHeader.dwSize -= HEADER_NBSIZE;
        if( oHeader.dwSize / sizeof( ElemType ) !=
            oHeader.dwCount )
        {
            ret = -EINVAL;
            break;
        }

        guint64* pElems =
            ( guint64* )( pHeader + 1 );
        for( guint32 i = 0; i < oHeader.dwCount; ++i )
        {
            m_vecElems.push_back(
                ntohll( pElems[ i ] ) );
        }

    }while( 0 );

    return ret;
}

gint32 CStlQwordVector::Deserialize(
    const CBuffer& oBuf )
{
    if( oBuf.empty() )
        return -EINVAL;

    return Deserialize(
        oBuf.ptr(), oBuf.size() );
}

gint32 CStlStrVector::Serialize(
    CBuffer& oBuf ) const
{
    SERI_HEADER oHeader;

    guint32 dwSize = 0;
    for( auto& elem : m_vecElems )
        dwSize += elem.size() + sizeof( guint32 );

    oHeader.dwClsid = clsid( CStlStrVector );
    oHeader.dwSize = dwSize + HEADER_NBSIZE ;

    oBuf.Resize( sizeof( SERI_HEADER_BASE ) +
        oHeader.dwSize );

    oHeader.dwCount = m_vecElems.size();

    oHeader.hton();
    memcpy( oBuf.ptr(),
        &oHeader, sizeof( oHeader ) );

    char *pElem = 
        ( oBuf.ptr() + sizeof( oHeader ) );

    for( auto& i : m_vecElems )
    {
        guint32 dwBytes = htonl( i.size() );
        memcpy( pElem,
            &dwBytes, sizeof( dwBytes ) );
        pElem += sizeof( dwBytes );
        memcpy( pElem, i.c_str(), i.size() );
        pElem += i.size(); 
    }
    return 0;
}

gint32 CStlStrVector::Deserialize(
    const CBuffer& oBuf )
{
    if( oBuf.empty() )
        return -EINVAL;

    return Deserialize(
        oBuf.ptr(), oBuf.size() );
}

gint32 CStlStrVector::Deserialize(
    const char* pBuf, guint32 dwBufSize )
{
    const SERI_HEADER* pHeader =
        ( SERI_HEADER* )pBuf;

    if( pHeader == nullptr )
        return -EINVAL;

    if( dwBufSize < sizeof( SERI_HEADER ) )
        return -EINVAL;

    SERI_HEADER oHeader( *pHeader );
    oHeader.ntoh();

    m_vecElems.clear();
    gint32 ret = 0;

    do{
        if( oHeader.dwClsid != clsid( CStlStrVector ) )
        {
            ret = -ENOTSUP;
            break;
        }

        if( oHeader.bVersion != 1 )
        {
            ret = -ENOTSUP;
        }

        if( oHeader.dwSize > BUF_MAX_SIZE ) 
        {
            ret = -EINVAL;
            break;
        }

        oHeader.dwSize -= HEADER_NBSIZE;
        char* pElems = ( char* )( pHeader + 1 );
        const char* pEnd = pBuf + dwBufSize;
        for( guint32 i = 0;
            i < oHeader.dwCount && pElems < pEnd;
            ++i )
        {
            guint32 dwBytes;
            memcpy( &dwBytes,
                pElems, sizeof( dwBytes ) );
            dwBytes = ntohl( dwBytes );
            pElems += sizeof( dwBytes );
            if( pElems + dwBytes > pEnd )
            {
                ret = -ERANGE;
                break;
            }
            m_vecElems.push_back(
                stdstr( pElems, dwBytes ) );
            pElems += dwBytes;
        }

    }while( 0 );

    return ret;
}

}
