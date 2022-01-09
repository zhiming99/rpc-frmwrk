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

CCfgDbOpener2::CCfgDbOpener( const IConfigDb* pObj )
    : m_pCfg( nullptr ),
    m_pConstCfg( nullptr )
{
    if( pObj == nullptr )
        return;

    m_pConstCfg = pObj;
}

CCfgDbOpener2::CCfgDbOpener( IConfigDb* pObj ) :
    CCfgDbOpener( ( const IConfigDb* )pObj )
{
    if( pObj == nullptr )
        return;

    m_pCfg = pObj;
}

gint32 CCfgDbOpener2::GetProperty(
    gint32 iProp, BufPtr& pBuf ) const
{
    if( m_pConstCfg == nullptr )
        return -EFAULT;
    return m_pConstCfg->GetProperty( iProp, pBuf );
}

gint32 CCfgDbOpener2::SetProperty(
    gint32 iProp, const BufPtr& pBuf )
{
    if( m_pCfg == nullptr )
        return -EFAULT;

    return m_pCfg->SetProperty( iProp, pBuf );
}

gint32 CCfgDbOpener2::GetProperty(
    gint32 iProp, CBuffer& oBuf ) const
{
    if( m_pConstCfg == nullptr )
        return -EFAULT;
    return m_pConstCfg->GetProperty( iProp, oBuf );
}

gint32 CCfgDbOpener2::SetProperty(
    gint32 iProp, const CBuffer& oBuf )
{
    if( m_pCfg == nullptr )
        return -EFAULT;

    return m_pCfg->SetProperty( iProp, oBuf );
}

gint32 CCfgDbOpener2::GetProperty(
    gint32 iProp, Variant& oVar ) const
{
    auto pdb2 = CCFGDB2( m_pConstCfg );
    return pdb2->GetProperty( iProp, oVar );
}

gint32 CCfgDbOpener2::SetProperty(
    gint32 iProp, const Variant& oVar )
{
    auto pdb2 = CFGDB2( m_pCfg );
    return pdb2->SetProperty( iProp, oVar );
}

const Variant* CCfgDbOpener2::GetPropPtr( gint32 iProp ) const
{
    auto pdb2 = CCFGDB2( m_pConstCfg );
    return pdb2->GetPropertyPtr( iProp );
}

Variant* CCfgDbOpener2::GetPropPtr( gint32 iProp )
{
    auto pdb2 = CFGDB2( m_pCfg );
    return pdb2->GetPropertyPtr( iProp );
}

const Variant& CCfgDbOpener2::operator[]( gint32 iProp ) const
{
    auto pdb2 = CCFGDB2( m_pConstCfg );
    return pdb2->GetProperty( iProp );
}

Variant& CCfgDbOpener2::operator[]( gint32 iProp )
{
    auto pdb2 = CFGDB2( m_pCfg );
    return pdb2->GetProperty( iProp );
}

gint32 CCfgDbOpener2::GetObjPtr( gint32 iProp, ObjPtr& pObj ) const
{
    gint32 ret = 0;

    if( m_pConstCfg == nullptr )
        return -EFAULT;

    auto pdb2 = CCFGDB2( m_pConstCfg );
    do{
        const Variant* p;
        p = pdb2->GetPropertyPtr( iProp );
        if( p == nullptr )
        {
            ret = -ENOENT;
            break;
        }
        if( p->GetTypeId() != typeObj )
        {
            ret = -EFAULT;
            break;
        }
        pObj = ( const ObjPtr& )*p;
    
    }while( 0 );

    return ret;
}

gint32 CCfgDbOpener2::SetObjPtr( gint32 iProp, const ObjPtr& pObj )
{
    gint32 ret = 0;

    do{
        if( m_pCfg == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        auto pdb2 = CFGDB2( m_pCfg );
        Variant& o = pdb2->GetProperty( iProp );
        o = pObj;

    }while( 0 );

    return ret;
}

gint32 CCfgDbOpener2::GetBufPtr( gint32 iProp, BufPtr& pBuf ) const
{
    gint32 ret = 0;

    if( m_pConstCfg == nullptr )
        return -EFAULT;

    auto pdb2 = CCFGDB2( m_pConstCfg );
    do{
        const Variant* p;
        p = pdb2->GetPropertyPtr( iProp );
        if( p == nullptr )
        {
            ret = -ENOENT;
            break;
        }

        if( p->GetTypeId() != typeByteArr )
        {
            ret = -EFAULT;
            break;
        }
        pBuf = ( const BufPtr& )*p;
    
    }while( 0 );

    return ret;
}

gint32 CCfgDbOpener2::SetBufPtr( gint32 iProp, const BufPtr& pBuf )
{
    gint32 ret = 0;

    do{
        if( m_pCfg == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        auto pdb2 = CFGDB2( m_pCfg );
        Variant& o = pdb2->GetProperty( iProp );
        o = pBuf;

    }while( 0 );

    return ret;
}

gint32 CCfgDbOpener2::GetStrProp( gint32 iProp, std::string& strVal ) const
{
    gint32 ret = 0;

    if( m_pConstCfg == nullptr )
        return -EFAULT;

    do{
        auto pdb2 = CCFGDB2( m_pConstCfg );
        const Variant* p =
            pdb2->GetPropertyPtr( iProp );
        if( p == nullptr )
        {
            ret = -ENOENT;
            break;
        }
        if( p->GetTypeId() != typeString )
        {
            ret = -EFAULT;
            break;
        }
        strVal = ( const stdstr& )*p;

    }while( 0 );

    return ret;
}

gint32 CCfgDbOpener2::SetStrProp( gint32 iProp, const std::string& strVal )
{

    gint32 ret = 0;
    do{
        if( m_pCfg == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        auto pdb2 = CFGDB2( m_pCfg );
        Variant& o = pdb2->GetProperty( iProp );
        o = strVal;

    }while( 0 );

    return ret;
}

gint32 CCfgDbOpener2::GetStringProp( gint32 iProp, const char*& pStr ) const
{
    gint32 ret = 0;

    if( m_pConstCfg == nullptr )
        return -EFAULT;

    do{
        auto pdb2 = CCFGDB2( m_pConstCfg );
        const Variant* p =
            pdb2->GetPropertyPtr( iProp );
        if( p == nullptr )
        {
            ret = -ENOENT;
            break;
        }
        if( p->GetTypeId() != typeString )
            break;
        pStr = p->m_strVal.c_str();

    }while( 0 );

    return ret;
}

gint32 CCfgDbOpener2::GetMsgPtr( gint32 iProp, DMsgPtr& pMsg ) const
{
    gint32 ret = 0;

    if( m_pConstCfg == nullptr )
        return -EFAULT;

    do{
        auto pdb2 = CCFGDB2( m_pConstCfg );
        const Variant* p =
            pdb2->GetPropertyPtr( iProp );

        if( p == nullptr )
        {
            ret = -ENOENT;
            break;
        }

        try{
            if( p->GetTypeId() != typeDMsg )
            {
                ret = -EFAULT;
                break;
            }
            pMsg = ( const DMsgPtr& )*p;
        }
        catch( std::invalid_argument& e )
        {
            ret = -EINVAL;
        }
    
    }while( 0 );

    return ret;
}

gint32 CCfgDbOpener2::SetMsgPtr( gint32 iProp, const DMsgPtr& pMsg )
{
    gint32 ret = 0;

    do{
        if( m_pCfg == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        auto pdb2 = CFGDB2( m_pCfg );
        Variant& o = pdb2->GetProperty( iProp );
        o = pMsg;

    }while( 0 );

    return ret;
}

gint32 CCfgDbOpener2::CopyProp(
    gint32 iProp, gint32 iSrcProp, const CObjBase* pSrcCfg )
{
    if( pSrcCfg == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    if( pSrcCfg->GetClsid() !=
        clsid( CConfigDb2 ) )
    {
        CCfgDbOpener< CObjBase > a( pSrcCfg );

        BufPtr pBuf( true );
        ret = a.GetProperty( iSrcProp, *pBuf);

        if( ERROR( ret ) )
            return ret;

        return SetProperty( iProp, pBuf );
    }
    do{
        auto psrc = CCFGDB2( pSrcCfg );
        const Variant* p =
            psrc->GetPropertyPtr( iSrcProp );
        if( p == nullptr )
        {
            ret = -ENOENT;
            break;
        }

        auto pdst = CFGDB2( m_pCfg );
        Variant& odst = pdst->GetProperty( iProp );
        odst = *p;

    }while( 0 );
    return ret;
}

gint32 CCfgDbOpener2::CopyProp( gint32 iProp, const CObjBase* pSrcCfg )
{
    return CopyProp( iProp, iProp, pSrcCfg );
}

gint32 CCfgDbOpener2::MoveProp( gint32 iProp, CObjBase* pSrcCfg )
{
    gint32 ret = CopyProp( iProp, pSrcCfg );
    if( ERROR( ret ) )
        return ret;

    ret = pSrcCfg->RemoveProperty( iProp );
    return ret;
}

gint32 CCfgDbOpener2::SwapProp( gint32 iProp1, gint32 iProp2 )
{
    auto pdb2 = CFGDB2( m_pCfg );
    Variant* p1 =
        pdb2->GetPropertyPtr( iProp1 );
    if( p1 == nullptr )
        return -ENOENT;;

    Variant* p2 =
        pdb2->GetPropertyPtr( iProp2 );
    if( p2 == nullptr )
        return -ENOENT;

    Variant o( *p1 );
    pdb2->SetProperty( iProp1, *p2 );
    pdb2->SetProperty( iProp2, o );
    return STATUS_SUCCESS;
}


gint32 CCfgDbOpener2::GetIntPtr( gint32 iProp, guint32*& val ) const
{
    intptr_t val1;
    auto pdb2 = CCFGDB2( m_pConstCfg );
    const Variant* p =
        pdb2->GetPropertyPtr( iProp );

    if( p == nullptr )
        return -ENOENT;

    val1 = *p;
    val = ( guint32* )val1;
    return 0;
}

gint32 CCfgDbOpener2::SetIntPtr( gint32 iProp, guint32* val )
{
    auto pdb2 = CFGDB2( m_pCfg );
    Variant& o = pdb2->GetProperty( iProp );
    o = ( intptr_t )val;
    return STATUS_SUCCESS;
}

gint32 CCfgDbOpener2::GetShortProp( gint32 iProp, guint16& val ) const
{
    return GetPrimProp( iProp, val );
}

gint32 CCfgDbOpener2::SetShortProp( gint32 iProp, guint16 val )
{
    return SetPrimProp( iProp, val );
}

gint32 CCfgDbOpener2::GetIntProp( gint32 iProp, guint32& val ) const
{
    return GetPrimProp( iProp, val );
}

gint32 CCfgDbOpener2::SetIntProp( gint32 iProp, guint32 val )
{
    return SetPrimProp( iProp, val );
}
gint32 CCfgDbOpener2::GetBoolProp( gint32 iProp, bool& val ) const
{
    return GetPrimProp( iProp, val );
}
gint32 CCfgDbOpener2::SetBoolProp( gint32 iProp, bool val )
{
    return SetPrimProp( iProp, val );
}

gint32 CCfgDbOpener2::GetByteProp( gint32 iProp, guint8& val ) const
{
    return GetPrimProp( iProp, val );
}
gint32 CCfgDbOpener2::SetByteProp( gint32 iProp, guint8 val )
{
    return SetPrimProp( iProp, val );
}

gint32 CCfgDbOpener2::GetFloatProp( gint32 iProp, float& val ) const
{
    return GetPrimProp( iProp, val );
}
gint32 CCfgDbOpener2::SetFloatProp( gint32 iProp, float val )
{
    return SetPrimProp( iProp, val );
}

gint32 CCfgDbOpener2::GetQwordProp( gint32 iProp, guint64& val ) const
{
    return GetPrimProp( iProp, val );
}

gint32 CCfgDbOpener2::SetQwordProp( gint32 iProp, guint64 val )
{
    return SetPrimProp( iProp, val );
}

gint32 CCfgDbOpener2::GetDoubleProp( gint32 iProp, double& val ) const
{
    return GetPrimProp( iProp, val );
}
gint32 CCfgDbOpener2::SetDoubleProp( gint32 iProp, double val )
{
    return SetPrimProp( iProp, val );
}

gint32 CCfgDbOpener2::IsEqualProp( gint32 iProp, const CObjBase* pObj )
{
    gint32 ret = 0;

    do{
        if( pObj == nullptr )
        {
            ret = -EINVAL;
            break;
        }
        if( m_pConstCfg == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        BufPtr pBuf( true );
        ret = pObj->GetProperty( iProp, *pBuf );
        if( ERROR( ret ) )
            break;

        BufPtr pBuf2;
        ret = this->GetProperty( iProp, pBuf2 );
        if( ERROR( ret ) )
            break;

        if( *pBuf == *pBuf2 )
            break;

        ret = ERROR_FALSE;

    }while( 0 );
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

/*
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
            auto pdb2 = CFGDB2(
                ( IConfigDb* )GetCfg() );
            Variant& o =
                pdb2->GetProperty( iPos + 1 );
            o = val;
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

        auto pdb2 = CFGDB2(
            ( IConfigDb* )GetCfg() );
        Variant& o = pdb2->GetProperty( iPos );
        o = val;
        SetCount( iPos + 1 );

    }while( 0 );

    return ret;
}

template<>
gint32 CParamList::Push< DMsgPtr > ( DMsgPtr&& val  )
{
    gint32 ret = 0;
    do{
        gint32 iPos = GetCount();
        if( ERROR( iPos ) )
        {
            ret = iPos;
            break;
        }

        auto pdb2 = CFGDB2(
            ( IConfigDb* )GetCfg() );
        Variant& o =
            pdb2->GetProperty( iPos );
        o = val.ptr();
        SetCount( iPos + 1 );

    }while( 0 );

    return ret;
}

template<>
gint32 CParamList::Pop< BufPtr > ( BufPtr& val  )
{
    gint32 ret = 0;
    do{
        gint32 iPos = GetCount();
        if( ERROR( iPos ) )
        {
            ret = iPos;
            break;
        }

        if( iPos <= 0 )
        {
            ret = -ENOENT;
            break;
        }

        auto pdb2 = CFGDB2(
            ( IConfigDb* )GetCfg() );
        Variant& o =
            pdb2->GetProperty( iPos - 1 );
        val = o;
        RemoveProperty( iPos - 1 );
        SetCount( iPos - 1 );

    }while( 0 );

    return ret;
}*/

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
    auto itr = m_mapProps.find( iProp );
    if( itr == m_mapProps.end() )
        return -ENOENT;
    pBuf = itr->second.ToBuf();
    return 0;
}

gint32 CConfigDb2::SetProperty(
    gint32 iProp, const BufPtr& pBuf )
{
    auto itr = m_mapProps.find( iProp );
    if( itr != m_mapProps.end() )
        itr->second = *pBuf;
    else
    {
        m_mapProps[ iProp ] = *pBuf;
    }
    return 0;
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
    auto itr = m_mapProps.find( iProp );
    if( itr == m_mapProps.end() )
        return -ENOENT;
    const Variant& o = itr->second;
    EnumTypeId iType = o.GetTypeId();
    gint32 ret = 0;
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
    m_mapProps[ iProp ] = oBuf;
    return 0;
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

const CBuffer& CConfigDb2::operator[](
    gint32 iProp ) const
{
    throw std::runtime_error( 
        DebugMsg( -ENOTSUP,
            "[] cannot return CBuffer" ) );
}
CBuffer& CConfigDb2::operator[](
    gint32 iProp )
{
    throw std::runtime_error( 
        DebugMsg( -ENOTSUP,
            "[] cannot return CBuffer" ) );
}

#define QPAGE ( ( guint32 )( PAGE_SIZE >> 1 ) )
gint32 CConfigDb2::Serialize(
    CBuffer& oBuf ) const
{
    struct SERI_HEADER oHeader;
    gint32 ret = 0;

    auto itr = m_mapProps.cbegin();
    oHeader.dwClsid = clsid( CConfigDb2 );
    oHeader.dwCount = m_mapProps.size();
    oHeader.bVersion = 2;

    // NOTE: oHeader.dwSize at this point is an
    // estimated size at this moment

    oBuf.Resize( sizeof( SERI_HEADER ) + QPAGE );

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
                guint32 len = 1 + oVar.m_strVal.size(); 
                guint32 dwFree = pEnd - pLoc;
                if( dwFree < sizeof( guint32 ) + len )
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
                memcpy( pLoc, oVar.m_strVal.c_str(), len - 1 );
                pLoc += len;
                pLoc[ -1 ] = 0;
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
                    if( pLoc + len > pEnd )
                    {
                        ret = -E2BIG;
                        break;
                    }
                    new ( &oVar ) Variant( ( const char* )pLoc, len - 1 );
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
                    pLoc += len + sizeof( guint32 );
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

}
