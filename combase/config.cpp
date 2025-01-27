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

guint64 htonll(guint64 iVal )
{
    return htobe64( iVal );
}
guint64 ntohll(guint64 iVal )
{
    return be64toh( iVal );
}

const IConfigDb& CConfigDb2::operator=(
    const IConfigDb& rhs )
{
    if( rhs.GetClsid() == clsid( CConfigDb2 ) )
    {
        m_mapProps.clear();
        const CConfigDb2* pSrc = static_cast
            < const CConfigDb2* >( &rhs );
        m_mapProps.insert(
            pSrc->m_mapProps.begin(),
            pSrc->m_mapProps.end() );
    }
    else
    {
        throw invalid_argument(
            "unsupported source object" );
    }
    return *this;
}

CConfigDb2::CConfigDb2( const IConfigDb* pCfg )
{
    SetClassId( Clsid_CConfigDb2 );
    if( pCfg == nullptr )
        return;
    *this = *pCfg;
}

CConfigDb2::~CConfigDb2()
{
    RemoveAll();
}

gint32 CConfigDb2::GetProperty(
    gint32 iProp, BufPtr& pBuf ) const
{
    gint32 ret = 0;
    Variant oVar;
    ret = GetProperty( iProp, oVar );
    if( ERROR( ret ) )
        return ret;
    pBuf = oVar.ToBuf();
    return 0;
}

gint32 CConfigDb2::SetProperty(
    gint32 iProp, const BufPtr& pBuf )
{
    Variant oVar( *pBuf );
    return SetProperty( iProp, oVar );
}

// get a reference to variant from the config db
gint32 CConfigDb2::GetProperty(
    gint32 iProp, Variant& oVar ) const
{
    auto itr = m_mapProps.find( iProp );
    if( itr == m_mapProps.cend() )
        return -ENOENT;
    oVar = itr->second;
    return 0;
}
// add a reference to variant to the config db
gint32 CConfigDb2::SetProperty(
    gint32 iProp, const Variant& oVar )
{
    m_mapProps[ iProp ] = oVar;
    return 0;
}

gint32 CConfigDb2::GetProperty(
    gint32 iProp, CBuffer& oBuf ) const
{
    Variant o;
    gint32 ret = GetProperty( iProp, o );
    if( ERROR( ret ) )
        return ret;

    EnumTypeId iType = o.GetTypeId();
    switch( iType )
    {
    case typeByte: 
        oBuf = o.m_byVal;
        break;
    case typeUInt16:
        oBuf = o.m_wVal;
        break;
    case typeUInt32:
        oBuf = o.m_dwVal;
        break;
    case typeUInt64:
        oBuf = o.m_qwVal;
        break;
    case typeFloat:
        oBuf = o.m_fVal;
        break;
    case typeDouble:
        oBuf = o.m_dblVal;
        break;
    case typeDMsg:
        oBuf = o.m_pMsg;
        break;
    case typeObj:
        oBuf = o.m_pObj;
        break;
    case typeByteArr:
        oBuf = *o.m_pBuf;
        break;
    case typeString:
        oBuf = o.m_strVal;
        break;
    case typeNone:
    default:
        ret = -EINVAL;
    }
    return ret;
}

gint32 CConfigDb2::SetProperty(
    gint32 iProp, const CBuffer& oBuf )
{
    Variant oVar( oBuf );
    return SetProperty( iProp, oVar );
}

gint32 CConfigDb2::GetPropertyType(
    gint32 iProp, gint32& iType ) const
{
    auto itr = m_mapProps.find( iProp );
    if( itr == m_mapProps.end() )
        return -ENOENT;
    iType = itr->second.GetTypeId();
    return STATUS_SUCCESS;
}

gint32 CConfigDb2::RemoveProperty(
    gint32 iProp )
{
    gint32 i = m_mapProps.erase( iProp );
    if( i == 0 )
        return -ENOENT;
    return STATUS_SUCCESS;
}

gint32 CConfigDb2::GetPropIds(
    std::vector<gint32>& vecIds ) const
{
    if( m_mapProps.empty() )
        return -ENOENT;
    for( auto& elem : m_mapProps )
        vecIds.push_back( elem.first );
    return STATUS_SUCCESS;
}

void CConfigDb2::RemoveAll()
{ m_mapProps.clear(); }

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

#define QPAGE ( ( guint32 )( PAGE_SIZE >> 1 ) )

gint32 CConfigDb2::Serialize(
    CBuffer& oBuf ) const
{
    struct SERI_HEADER oHeader;
    gint32 ret = 0;

    oHeader.dwClsid = clsid( CConfigDb2 );
    oHeader.dwCount = m_mapProps.size();
    oHeader.bVersion = 2;

    // NOTE: oHeader.dwSize at this point is an
    // estimated size at this moment

    guint32 dwOrigOff = oBuf.size();
    ret = oBuf.Resize( dwOrigOff +
        sizeof( SERI_HEADER ) + QPAGE );
    if( ERROR( ret ) )
        return ret;

    char* pLoc = oBuf.ptr() + dwOrigOff;

    oHeader.hton();
    memcpy( pLoc, &oHeader, sizeof( oHeader ) );
    pLoc += sizeof( oHeader );
    oHeader.ntoh();

    auto itr = m_mapProps.cbegin();

    char* pEnd = oBuf.ptr() + oBuf.size();

    while( itr != m_mapProps.cend() )
    {
        guint32 dwKey = htonl( itr->first );
        if( pEnd - pLoc < ( gint32 )( sizeof( guint32 ) ) )
        {
            RESIZE_BUF( oBuf.size() + QPAGE, ret );
            if( ERROR( ret ) )
                break;
        }
        memcpy( pLoc, &dwKey, sizeof( guint32 ) );
        pLoc += sizeof( guint32 );

        const Variant& oVar = itr->second;
        if( oVar.empty() )
        {
            ret = -EFAULT;
            break;
        }
        guint8 byType = oVar.GetTypeId();
        if( pEnd - pLoc < sizeof( byType ) )
        {
            RESIZE_BUF( oBuf.size() + QPAGE, ret );
            if( ERROR( ret ) )
                break;
        }
        *pLoc++ = byType;

        switch( ( EnumTypeId )byType )
        {
        case typeByte: 
            {
                if( pEnd - pLoc < sizeof( guint8 ) )
                {
                    RESIZE_BUF( oBuf.size() + QPAGE, ret );
                    if( ERROR( ret ) )
                        break;
                }
                *pLoc++ = ( guint8& )oVar;
                break;
            }
        case typeUInt16:
            {
                guint16 val =
                    htons( ( guint16& )oVar );
                if( pEnd - pLoc < sizeof( guint16 ) )
                {
                    RESIZE_BUF( oBuf.size() + QPAGE, ret );
                    if( ERROR( ret ) )
                        break;
                }
                memcpy( pLoc, &val, sizeof( guint16 ) );
                pLoc += sizeof( guint16 );
                break;
            }
        case typeUInt32:
        case typeFloat:
            {
                guint32 val =
                    htonl( ( guint32& )oVar );
                if( pEnd - pLoc < sizeof( guint32 ) )
                {
                    RESIZE_BUF( oBuf.size() + QPAGE, ret );
                    if( ERROR( ret ) )
                        break;
                }
                memcpy( pLoc, &val, sizeof( guint32 ) );
                pLoc += sizeof( guint32 );
                break;
            }
        case typeUInt64:
        case typeDouble:
            {
                guint64 val =
                    htonll( ( guint64& )oVar );

                if( pEnd - pLoc < sizeof( guint64 ) )
                {
                    RESIZE_BUF( oBuf.size() + QPAGE, ret );
                    if( ERROR( ret ) )
                        break;
                }
                memcpy( pLoc, &val, sizeof( guint64 ) );
                pLoc += sizeof( guint64 );
                break;
            }
        case typeString:
            {
                guint32 len = oVar.m_strVal.size(); 
                guint32 dwFree = pEnd - pLoc;
                if( dwFree < sizeof( guint32 ) + len )
                {
                    guint32 dwIncSize = 
                        len + sizeof( guint32 ) - dwFree + QPAGE;

                    RESIZE_BUF( oBuf.size() + dwIncSize, ret );
                    if( ERROR( ret ) )
                        break;
                }

                guint32 len1 = htonl( len );
                memcpy( pLoc, &len1, sizeof( guint32 ) );
                pLoc += sizeof( guint32 );
                if( len == 0 )
                {
                    // empty string
                    break;
                }

                memcpy( pLoc, oVar.m_strVal.c_str(), len );
                pLoc += len;
                break;
            }
        case typeDMsg:
            {
                BufPtr pBuf( true );
                DMsgPtr& pMsg = ( DMsgPtr& )oVar;
                ret = pMsg.Serialize( *pBuf );
                if( ERROR( ret ) )
                    break;

                guint32 dwFree = pEnd - pLoc;
                guint32 len = pBuf->size();
                if( dwFree < len + sizeof( guint32 ) )
                {
                    guint32 dwIncSize = 
                        len + sizeof( guint32 ) - dwFree + QPAGE;

                    RESIZE_BUF( oBuf.size() + dwIncSize, ret );
                    if( ERROR( ret ) )
                        break;
                }

                len = htonl( len );
                memcpy( pLoc, &len, sizeof( guint32 ) );
                len = ntohl( len );

                pLoc += sizeof( guint32 );
                memcpy( pLoc, pBuf->ptr(), len );
                pLoc += len;
                break;
            }
        case typeObj:
            {
                BufPtr pBuf( true );
                ObjPtr& pObj = ( ObjPtr& )oVar;
                if( pObj.IsEmpty() )
                {
                    SERI_HEADER_BASE oBase;
                    memcpy( pLoc,
                        &oBase, sizeof( oBase ) );
                    pLoc += sizeof( oBase );
                    break;
                }
                ret = pObj->Serialize( *pBuf );
                if( ERROR( ret ) )
                    break;
                guint32 len = pBuf->size();
                if( pEnd - pLoc < len )
                {
                    guint32 dwIncSize = std::max(
                        QPAGE, len + QPAGE );

                    RESIZE_BUF( oBuf.size() + dwIncSize, ret );
                    if( ERROR( ret ) )
                        break;
                }
                memcpy( pLoc, pBuf->ptr(), len );
                pLoc += len;
                break;
            }
        case typeByteArr:
            {
                BufPtr& pBufVal = ( BufPtr& )oVar;
                guint32 len = 0;
                if( !pBufVal.IsEmpty() && !pBufVal->empty() )
                    len = pBufVal->size();

                guint32 dwFree = pEnd - pLoc;
                if( dwFree < len + sizeof( guint32 ) )
                {
                    guint32 dwIncSize =
                        len + sizeof( guint32 ) - dwFree + QPAGE;

                    RESIZE_BUF( oBuf.size() + dwIncSize, ret );
                    if( ERROR( ret ) )
                        break;
                }
                len = htonl( len );
                memcpy( pLoc, &len, sizeof( guint32 ) );
                len = ntohl( len );

                if( len == 0 )
                    break;

                pLoc += sizeof( guint32 );
                memcpy( pLoc, pBufVal->ptr(), len );
                pLoc += len;
                break;
            }
        default:
            ret = -EINVAL;
            break;
        }

        if( ERROR( ret ) )
            break;

        ++itr;
    }

    if( SUCCEEDED( ret ) )
    {
        SERI_HEADER* pHeader =
            ( SERI_HEADER* )( oBuf.ptr() + dwOrigOff );

        guint32 dwSize = htonl(
            pLoc - oBuf.ptr() - dwOrigOff -
            sizeof( SERI_HEADER_BASE ) );

        memcpy( &pHeader->dwSize,
            &dwSize, sizeof( dwSize ) );

        RESIZE_BUF( pLoc - oBuf.ptr(), ret );
    }

    return ret;
}

gint32 CConfigDb2::Deserialize(
    const char* pBuf, guint32 dwSize )
{
    if( pBuf == nullptr || dwSize == 0 )
        return -EINVAL;

    const SERI_HEADER* pHeader =
        reinterpret_cast< const SERI_HEADER* >( pBuf );

    SERI_HEADER oHeader;
    memcpy( &oHeader, pHeader, sizeof( oHeader ) );
    oHeader.ntoh();

    gint32 ret = 0;

    do{
        if( oHeader.dwClsid != clsid( CConfigDb2 ) ||
            oHeader.dwCount > CFGDB_MAX_ITEM ||
            oHeader.dwSize + sizeof( SERI_HEADER_BASE ) > dwSize ||
            oHeader.dwSize > CFGDB_MAX_SIZE )
        {
            ret = -EINVAL;
            break;
        }
        if( oHeader.bVersion != 2 )
        {
            ret = -EINVAL;
            break;
        }

        const char* pLoc = pBuf + sizeof( SERI_HEADER );
        const char* pEnd = pBuf + dwSize;

        for( guint32 i = 0; i < oHeader.dwCount; i++ )
        {
            gint32 iKey = ntohl( *( guint32* )pLoc );
            guint32 dwValSize = ntohl( ( ( guint32* )pLoc )[ 1 ] );
            pLoc += sizeof( guint32 );
            guint8 byType = pLoc[ 0 ];
            Variant oVar;
            pLoc += 1;

            switch( ( EnumTypeId )byType )
            {
            case typeByte: 
                {
                    guint8 val = pLoc[ 0 ];
                    oVar = val;
                    pLoc++;
                    break;
                }
            case typeUInt16:
                {
                    guint16 val = 0;
                    memcpy( &val, pLoc, sizeof( guint16 ) );
                    oVar = ntohs( val );
                    pLoc += sizeof( guint16 );
                    break;
                }
            case typeUInt32:
            case typeFloat:
                {
                    guint32 val = 0;
                    memcpy( &val, pLoc, sizeof( guint32 ) );
                    val = ntohl( val );
                    if( byType == typeUInt32 )
                        oVar = val;
                    else
                        oVar = *( float* )&val;
                    pLoc += sizeof( guint32 );
                    break;
                }
            case typeUInt64:
            case typeDouble:
                {
                    guint64 val = 0;
                    memcpy( &val, pLoc, sizeof( guint64 ) );
                    val = ntohll( val );
                    if( byType == typeUInt64 )
                        oVar = val;
                    else
                        oVar = *( double* )&val;
                    pLoc += sizeof( guint64 );
                    break;
                }
            case typeString:
                {
                    guint32 len = 0;
                    if( pLoc + sizeof( guint32 ) > pEnd )
                    {
                        ret = -E2BIG;
                        break;
                    }
                    memcpy( &len, pLoc, sizeof( guint32 ) );
                    len = ntohl( len );
                    pLoc += sizeof( guint32 );
                    if( len == 0 )
                    {
                        new ( &oVar ) Variant("");
                        break;
                    }
                    if( pLoc + len > pEnd )
                    {
                        ret = -E2BIG;
                        break;
                    }
                    new ( &oVar ) Variant( ( const char* )pLoc, len );
                    pLoc += len;
                    break;
                }
            case typeDMsg:
                {
                    guint32 len = 0;
                    if( pLoc + sizeof( guint32 ) > pEnd )
                    {
                        ret = -E2BIG;
                        break;
                    }
                    memcpy( &len, pLoc, sizeof( guint32 ) );
                    len = ntohl( len );
                    pLoc += sizeof( guint32 );
                    if( pLoc + len > pEnd )
                    {
                        ret = -E2BIG;
                        break;
                    }
                    DMsgPtr pMsg;
                    ret = pMsg.Deserialize( pLoc, len );
                    if( ERROR( ret ) )
                        break;

                    oVar = ( DBusMessage* )pMsg; 
                    pLoc += len;
                    break;
                }
            case typeObj:
                {
                    SERI_HEADER_BASE* pHdr =
                        ( SERI_HEADER_BASE* )pLoc;
                    SERI_HEADER_BASE oHdr;
                    memcpy( &oHdr, pHdr, sizeof( oHdr ) );
                    oHdr.ntoh();
                    guint32 len = oHdr.dwSize +
                        sizeof( SERI_HEADER_BASE );
                    
                    if( pLoc + len > pEnd )
                    {
                        ret = -E2BIG;
                        break;
                    }

                    ObjPtr pObj;
                    if( len == 0 ||
                        oHdr.dwClsid == clsid( Invalid ) )
                    {
                        oVar = pObj;
                        pLoc += len;
                        break;
                    }

                    ret = pObj.NewObj(
                        ( EnumClsid )oHdr.dwClsid );
                    if( ERROR( ret ) )
                        break;

                    ret = pObj->Deserialize( pLoc, len );
                    if( ERROR( ret ) )
                        break;

                    oVar = pObj;
                    pLoc += len;
                    break;
                }
            case typeByteArr:
                {
                    guint32 len = 0;
                    if( pLoc + sizeof( guint32 ) > pEnd )
                    {
                        ret = -E2BIG;
                        break;
                    }
                    memcpy( &len, pLoc, sizeof( guint32 ) );
                    len = ntohl( len );
                    pLoc += sizeof( guint32 );
                    if( pLoc + len > pEnd )
                    {
                        ret = -E2BIG;
                        break;
                    }
                    BufPtr pBufVal( true );
                    ret = pBufVal->Resize( len );
                    if( ERROR( ret ) )
                        break;
                    memcpy( pBufVal->ptr(), pLoc, len );
                    oVar = pBufVal;
                    pLoc += len;
                    break;
                }
            default:
                ret = -EINVAL;
                break;
            }

            if( ERROR( ret ) )
                break;

            SetProperty( iKey, oVar );
            if( pLoc > pEnd )
            {
                // don't know how to handle
                ret = -E2BIG;
                break;
            }
        }

    }while( 0 );

    return ret;
}

gint32 CConfigDb2::Deserialize(
    const CBuffer& oBuf )
{
    if( oBuf.empty() )
        return -EINVAL;

    return Deserialize(
        oBuf.ptr(), oBuf.size() );
}

gint32 CConfigDb2::Clone(
    const IConfigDb& rhs )
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

gint32 CConfigDb2::EnumProperties(
    std::vector< gint32 >& vecProps ) const 
{
    return GetPropIds( vecProps );
}

// get a reference to variant from the config db
const Variant& CConfigDb2::GetProperty( gint32 iProp ) const
{
    auto itr = m_mapProps.find( iProp );
    if( itr == m_mapProps.cend() )
    {
        stdstr strMsg = DebugMsg(
            -ENOENT, "no such element" );
        throw std::out_of_range( strMsg );
    }
    return itr->second;
}

Variant& CConfigDb2::GetProperty( gint32 iProp )
{ return m_mapProps[ iProp ]; }

const Variant* CConfigDb2::GetPropertyPtr( gint32 iProp ) const
{
    auto itr = m_mapProps.find( iProp );
    if( itr == m_mapProps.cend() )
        return nullptr;
    return &itr->second;
}

Variant* CConfigDb2::GetPropertyPtr( gint32 iProp )
{
    auto itr = m_mapProps.find( iProp );
    if( itr == m_mapProps.end() )
        return nullptr;
    return &itr->second;
}

gint32 CConfigDb2::size() const
{ return m_mapProps.size(); }

bool CConfigDb2::exist( gint32 iProp ) const
{ 
    return m_mapProps.cend() != m_mapProps.find( iProp ) ;
}

}
