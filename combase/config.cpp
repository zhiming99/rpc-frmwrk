/*
 * =====================================================================================
 *
 *       Filename:  config.cpp
 *
 *    Description:  implementation of CConfigDB
 *
 *        Version:  1.0
 *        Created:  05/06/2016 07:41:20 PM
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

#include <string>
#include <algorithm>
#include <vector>
#include <endian.h>

#include "configdb.h"

using namespace std;

namespace rpcf
{

CConfigDb::CConfigDb( const IConfigDb* pCfg )
{
    SetClassId( Clsid_CConfigDb );
    if( pCfg )
    {
        *this = *pCfg;
    }
}

CConfigDb::~CConfigDb()
{
    m_mapProps.clear();
}

void CConfigDb::BuildConfig(
    const std::string& strJson )
{

}

void CConfigDb::BuildConfig(
    const std::hashmap<gint32, std::string>& strPropValues )
{

}

gint32 CConfigDb::SetProperty(
    gint32 iProp, const BufPtr& pBuf )
{
    if( pBuf.IsEmpty() )
    {
        m_mapProps.erase( iProp );
        return 0;
    }
    m_mapProps[ iProp ] = pBuf;
    return 0;
}

gint32 CConfigDb::GetProperty(
    gint32 iProp, BufPtr& pBuf ) const
{
    gint32 ret = -ENOENT;
    
    hashmap<gint32, BufPtr>::const_iterator itr =
        m_mapProps.find( iProp );

    if( itr != m_mapProps.cend() )
    {
        pBuf = itr->second;
        ret = 0;
    }
    return ret;
}

gint32 CConfigDb::GetProperty(
    gint32 iProp, CBuffer& oBuf ) const
{
    gint32 ret = -ENOENT;
    
    hashmap<gint32, BufPtr>::const_iterator itr =
        m_mapProps.find( iProp );

    if( itr != m_mapProps.cend() )
    {
        oBuf = (*itr->second);
        if( oBuf.ptr() == nullptr )
        {
            DebugPrint( -EFAULT,
                "oBuf contains nothing, thread=%d",
                ::GetTid() );
        }
        ret = 0;
    }

    if( ret == -ENOENT )
    {
        ret = CObjBase::GetProperty(
            iProp, oBuf );
    }
    return ret;
}

gint32 CConfigDb::GetPropertyType(
    gint32 iProp, gint32& iType ) const
{
    gint32 ret = -ENOENT;
    
    hashmap<gint32, BufPtr>::const_iterator itr =
        m_mapProps.find( iProp );

    if( itr != m_mapProps.cend() )
    {
        iType = ( *itr->second ).GetExDataType();
        ret = 0;
    }

    if( ret == -ENOENT )
    {
        ret = CObjBase::GetPropertyType(
            iProp, iType );
    }
    return ret;
}

gint32 CConfigDb::SetProperty(
    gint32 iProp, const CBuffer& oProp )
{
    try{
        // NOTE: we don't forward the request to
        // cobjbase because properties on objbase
        // are all read-only            

        BufPtr oBufPtr( true );
        *oBufPtr = oProp;
        m_mapProps[ iProp ] = oBufPtr;

    }
    catch( std::bad_alloc& e )
    {
        return -ENOMEM;
    }
    return 0;
}

CBuffer& CConfigDb::operator[]( gint32 iProp )
{
    hashmap<gint32, BufPtr>::iterator itr
        = m_mapProps.find( iProp );

    if( itr == m_mapProps.end() )
    {
        BufPtr pBuf( true );
        m_mapProps[ iProp ] = pBuf;
        itr = m_mapProps.find( iProp );
    }

    return *itr->second;
}

const CBuffer& CConfigDb::operator[](
    gint32 iProp ) const
{
    hashmap<gint32, BufPtr>::const_iterator itr
        = m_mapProps.find( iProp );

    if( itr == m_mapProps.cend() )
    {
        // make an empty entry
        string strMsg = DebugMsg(
            -ENOENT, "no such element" );
        throw std::out_of_range( strMsg );
    }

    return *itr->second;
}

#define RESIZE_BUF( _Size, _Ret ) \
do{ \
    guint32 dwLocOff = pLoc - oBuf.ptr(); \
    oBuf.Resize( ( _Size ) ); \
    if( oBuf.ptr() == nullptr ) \
    { \
        _Ret = -ENOMEM; \
        break; \
    } \
    pLoc = ( dwLocOff + oBuf.ptr() ); \
    pEnd = ( oBuf.size() + oBuf.ptr() ); \
 \
}while( 0 ) 

gint32 CConfigDb::Serialize( CBuffer& oBuf ) const
{
    struct SERI_HEADER oHeader;
    gint32 ret = 0;

    hashmap<gint32, BufPtr>::const_iterator itr
        = m_mapProps.cbegin();

    oHeader.dwClsid = clsid( CConfigDb );
    oHeader.dwCount = m_mapProps.size();
    oHeader.bVersion = 1;

    for( auto oPair : m_mapProps )
    {
        // reserve room for key and size values
        oHeader.dwSize += sizeof( gint32 ) * 2;
        oHeader.dwSize += oPair.second->size();
    }

    // NOTE: oHeader.dwSize at this point is an
    // estimated size at this moment

    oBuf.Resize( oHeader.dwSize +
        sizeof( SERI_HEADER ) + PAGE_SIZE );

    char* pLoc = oBuf.ptr();
    oHeader.hton();
    memcpy( pLoc, &oHeader, sizeof( oHeader ) );
    pLoc += sizeof( oHeader );
    oHeader.ntoh();

    itr = m_mapProps.begin();

    char* pEnd = oBuf.ptr() + oBuf.size();

    while( itr != m_mapProps.cend() )
    {
        guint32 dwKey = htonl( itr->first );
        if( pEnd - pLoc > ( gint32 )( sizeof( guint32 ) * 2 ) )
        {
            memcpy( pLoc, &dwKey, sizeof( guint32 ) );
            pLoc += sizeof( guint32 ) * 2;
        }
        else
        {
            RESIZE_BUF( oBuf.size() + PAGE_SIZE, ret );
            if( ERROR( ret ) )
                break;

            memcpy( pLoc, &dwKey, sizeof( guint32 ) );
            pLoc += sizeof( guint32 ) * 2;
        }

        BufPtr pValBuf( true );
        ret = itr->second->Serialize( *pValBuf );
        if( ERROR( ret ) )
            break;

        if( pEnd - pLoc >= ( gint32 )pValBuf->size() )
        {
            memcpy( pLoc, pValBuf->ptr(), pValBuf->size() );
        }
        else
        {
            guint32 dwSizeInc = 
                AlignDword( pValBuf->size() ) + PAGE_SIZE ;

            RESIZE_BUF( oBuf.size() + dwSizeInc, ret );
            if( ERROR( ret ) )
                break;

            memcpy( pLoc, pValBuf->ptr(),
                pValBuf->size() );
        }

        // make sure the buffer element are on the
        // boundary of 4 bytes, so that the
        // SERI_HEADER is properly aligned
        *( guint32* )( pLoc - sizeof( guint32 ) ) =
            htonl( AlignDword( pValBuf->size() ) );

        pLoc += AlignDword( pValBuf->size() );

        if( pLoc - oBuf.ptr() > CFGDB_MAX_SIZE )
        {
            ret = -ENOMEM;
            break;
        }
        ++itr;
    }

    if( SUCCEEDED( ret ) )
    {
        SERI_HEADER* pHeader =
            ( SERI_HEADER* )oBuf.ptr();

        pHeader->dwSize = htonl(
            pLoc - oBuf.ptr() -
            sizeof( SERI_HEADER_BASE ) );

        RESIZE_BUF( pLoc - oBuf.ptr(), ret );
    }

    return ret;
}

gint32 CConfigDb::Deserialize(
    const char* pBuf, guint32 dwSize )
{
    if( pBuf == nullptr || dwSize == 0 )
        return -EINVAL;

    const SERI_HEADER* pHeader =
        reinterpret_cast< const SERI_HEADER* >( pBuf );

    SERI_HEADER oHeader( *pHeader );
    oHeader.ntoh();

    gint32 ret = 0;

    do{
        if( oHeader.dwClsid != clsid( CConfigDb ) ||
            oHeader.dwCount > CFGDB_MAX_ITEM ||
            oHeader.dwSize + sizeof( SERI_HEADER_BASE ) > dwSize ||
            oHeader.dwSize > CFGDB_MAX_SIZE )
        {
            ret = -EINVAL;
            break;
        }

        const char* pLoc = pBuf + sizeof( SERI_HEADER );

        for( guint32 i = 0; i < oHeader.dwCount; i++ )
        {
            gint32 iKey = ntohl( *( guint32* )pLoc );
            guint32 dwValSize = ntohl( ( ( guint32* )pLoc )[ 1 ] );
            pLoc += sizeof( guint32 ) * 2;

            BufPtr pValBuf( true );

            ret = pValBuf->Deserialize( pLoc, dwValSize );
            if( ERROR( ret ) )
                break;

            SetProperty( iKey, *pValBuf );

            pLoc += dwValSize; 

            if( ( guint32 )( pLoc - pBuf ) > dwSize )
            {
                // don't know how to handle
                ret = -E2BIG;
                break;
            }
        }

    }while( 0 );

    return ret;
}

gint32 CConfigDb::Deserialize(
    const CBuffer& oBuf )
{
    if( oBuf.empty() )
        return -EINVAL;

    return Deserialize( oBuf.ptr(), oBuf.size() );
}

gint32 CConfigDb::GetPropIds(
    std::vector<gint32>& vecIds ) const
{
    hashmap<gint32, BufPtr>::const_iterator itr =
        m_mapProps.cbegin();

    while( itr != m_mapProps.cend() )
    {
        vecIds.push_back( itr->first );
    }
    std::sort( vecIds.begin(), vecIds.end() );
    return vecIds.size();
}

const IConfigDb& CConfigDb::operator=(
    const IConfigDb& rhs )
{
    if( rhs.GetClsid() == clsid( CConfigDb ) )
    {
        // we are sure the rhs referencing a
        // CConfigDb object
        const CConfigDb* pSrc = static_cast
            < const CConfigDb* >( &rhs );

        if( pSrc != nullptr )
        {
            m_mapProps.clear();

            hashmap<gint32, BufPtr>::const_iterator
                itrSrc = pSrc->m_mapProps.begin();

            while( itrSrc != pSrc->m_mapProps.end() )
            {
                BufPtr pBuf( true );
                // NOTE: we will clone all the
                // properties of simple types. for the
                // objptr or dmsgptr, we only copy the
                // pointer
                *pBuf = *itrSrc->second;
                m_mapProps[ itrSrc->first ] = pBuf;
                ++itrSrc;
            }
        }
    }
    else
    {
        throw invalid_argument(
            "the rhs is not derived from CConfigDb" );
    }
    return *this;
}

gint32 CConfigDb::Clone( const IConfigDb& rhs )
{
    BufPtr bufPtr( true );
    int ret = 0;
    do{

        ret = rhs.Serialize( *bufPtr );
        if( ret < 0 )
            break;

        ret = this->Deserialize( *bufPtr );
        if( ret < 0 )
            break;

    }while( 0 );

    return ret;
}

gint32 CConfigDb::RemoveProperty( gint32 iProp )
{
    gint32 ret = m_mapProps.erase( iProp );

    if( ret == 0 )
        ret = -ENOENT;
    else
        ret = 0;

    return ret;
}

void CConfigDb::RemoveAll()
{
    m_mapProps.clear();
}

gint32 CConfigDb::EnumProperties(
    vector< gint32 >& vecProps ) const
{

    // we don't enum the underlying properties from the
    // objbase
    gint32 ret = 0;

    for( auto& oElem : m_mapProps )
    {
        vecProps.push_back( oElem.first );
    }

    std::sort( vecProps.begin(), vecProps.end() );
    return ret;
}


guint64 htonll(guint64 iVal )
{
    return htobe64( iVal );
}
guint64 ntohll(guint64 iVal )
{
    return be64toh( iVal );
}

template<>
gint32 CParamList::Push< BufPtr& > ( BufPtr& val  )
{
    gint32 ret = 0;
    do{
        gint32 iPos = GetCount();
        if( ERROR( iPos ) )
        {
            ret = iPos;
            break;
        }

        try{
            ret = GetCfg()->SetProperty( iPos, val );

            if( SUCCEEDED( ret ) )
                SetCount( iPos + 1 );
        }
        catch( std::invalid_argument& e )
        {
            ret = -EINVAL;
        }

    }while( 0 );

    return ret;
}

template<>
gint32 CParamList::Push< const BufPtr& > ( const BufPtr& val  )
{
    gint32 ret = 0;
    do{
        gint32 iPos = GetCount();
        if( ERROR( iPos ) )
        {
            ret = iPos;
            break;
        }

        BufPtr pBuf;
        try{
            pBuf = const_cast< BufPtr& >( val );
            ret = GetCfg()->SetProperty(
                iPos, pBuf );

            if( SUCCEEDED( ret ) )
                SetCount( iPos + 1 );
        }
        catch( std::invalid_argument& e )
        {
            ret = -EINVAL;
        }

    }while( 0 );

    return ret;
}

}
