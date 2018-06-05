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
 * =====================================================================================
 */
#include <map>
#include <string>
#include <algorithm>
#include <vector>

#include "defines.h"
#include "configdb.h"
#include "stlcont.h"

gint32 CStlIntVector::Serialize(
    CBuffer& oBuf ) const
{
    SERI_HEADER oHeader;

    guint32 dwSize = 
        sizeof( ElemType ) * m_vecElems.size();

    oHeader.dwClsid = clsid( CStlIntVector );
    oHeader.dwSize = dwSize ;

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
    const SERI_HEADER* pHeader =
        ( SERI_HEADER* )oBuf.ptr();

    if( pHeader == nullptr )
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

        if( oHeader.dwSize / sizeof( ElemType ) !=
            oHeader.dwCount )
        {
            ret = -EINVAL;
            break;
        }

        for( guint32 i = 0; i < oHeader.dwCount; ++i )
        {
            m_vecElems.push_back(
                ntohl( oHeader.arrElems[ i ] ) );
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
    oHeader.dwSize = dwSize;

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
    const CBuffer& oBuf )
{
    const SERI_HEADER* pHeader =
        ( SERI_HEADER* )oBuf.ptr();

    if( pHeader == nullptr )
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
