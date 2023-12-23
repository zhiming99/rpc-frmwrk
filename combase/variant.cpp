/*
 * =====================================================================================
 *
 *       Filename:  variant.cpp
 *
 *    Description:  Implementation of Variant class
 *
 *        Version:  1.0
 *        Created:  01/02/2022 09:00:00 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2022 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#include "variant.h"
#include "dmsgptr.h"
using namespace std;

namespace rpcf
{
Variant::Variant()
{
    m_qwVal = 0;
    m_iType = typeNone;
}

Variant::~Variant()
{ Clear(); }

Variant::Variant( bool bVal )
{
    m_bVal = bVal;
    m_iType = typeByte;
}
Variant::Variant( guint8 byVal )
{
    m_byVal = byVal;
    m_iType = typeByte;
}
Variant::Variant( guint16 wVal )
{
    m_wVal = wVal;
    m_iType = typeUInt16;
}
Variant::Variant( guint32 dwVal )
{
    m_dwVal = dwVal;
    m_iType = typeUInt32;
}
Variant::Variant( guint64 qwVal )
{
    m_qwVal = qwVal;
    m_iType = typeUInt64;
}
Variant::Variant( float fVal )
{
    m_fVal = fVal;
    m_iType = typeFloat;
}
Variant::Variant( double dblVal )
{
    m_dblVal = dblVal;
    m_iType = typeDouble;
}
Variant::Variant( const ObjPtr& pObj )
{
    new ( &m_pObj ) ObjPtr( pObj );
    m_iType = typeObj;
}
Variant::Variant( const BufPtr& pBuf )
{
    new ( &m_pBuf ) BufPtr( pBuf );
    m_iType = typeByteArr;
}
Variant::Variant( const DMsgPtr& msgVal )
{
    new ( &m_pMsg ) DMsgPtr( msgVal );
    m_iType = typeDMsg;
}
Variant::Variant( const string& strVal )
{
    new ( &m_strVal ) string( strVal );
    m_iType = typeString;
}
Variant::Variant( const char* szVal )
{
    new ( &m_strVal ) string( szVal );
    m_iType = typeString;
}
Variant::Variant(
    const char* szVal, guint32 dwSize )
{
    new ( &m_strVal ) string( szVal, dwSize );
    m_iType = typeString;
}

Variant::Variant( gint8 val )
{
    m_byVal = ( guint8 )val;
    m_iType = typeByte;
}
Variant::Variant( gint16 val )
{
    m_wVal = ( guint16 )val;
    m_iType = typeUInt16;
}
Variant::Variant( gint32 val )
{
    m_dwVal = ( guint32 )val;
    m_iType = typeUInt32;
}
Variant::Variant( gint64 val )
{
    m_qwVal = ( guint64 )val;
    m_iType = typeUInt64;
}
Variant::Variant( EnumEventId val )
{
    m_dwVal = ( guint32 )val;
    m_iType = typeUInt32;
}
Variant::Variant( EnumClsid val )
{
    m_dwVal = ( guint32 )val;
    m_iType = typeUInt32;
}
Variant::Variant( EnumPropId val )
{
    m_dwVal = ( guint32 )val;
    m_iType = typeUInt32;
}

Variant::Variant( const uintptr_t* pVal )
{
    m_iType = typeNone;
    *this = ( uintptr_t )pVal;
}

Variant::Variant( const Variant& rhs )
{
    switch( rhs.m_iType )
    {
    case typeUInt32:
        m_dwVal = rhs.m_dwVal;
        break;
    case typeUInt64:
        m_qwVal = rhs.m_qwVal;
        break;
    case typeObj:
        new( &m_pObj ) ObjPtr( rhs.m_pObj );
        break;
    case typeByteArr:
        new( &m_pBuf ) BufPtr( rhs.m_pBuf );
        break;
    case typeString:
        new( &m_strVal ) string( rhs.m_strVal );
        break;
    case typeFloat:
        m_fVal = rhs.m_fVal;
        break;
    case typeDouble:
        m_dblVal = rhs.m_dblVal;
        break;
    case typeDMsg:
        new( &m_pMsg ) DMsgPtr( rhs.m_pMsg );
        break;
    case typeByte: 
        m_byVal = rhs.m_byVal;
        break;
    case typeUInt16:
        m_wVal = rhs.m_wVal;
        break;
    default:
        {
            std::string strMsg = DebugMsg(
                -EINVAL, "Invalid variant type" );
            throw std::invalid_argument( strMsg );
        }

    }
    m_iType = rhs.m_iType;
}

Variant::Variant( Variant&& rhs )
{
    switch( rhs.m_iType )
    {
    case typeUInt32:
        m_dwVal = rhs.m_dwVal;
        break;
    case typeUInt64:
        m_qwVal = rhs.m_qwVal;
        break;
    case typeObj:
        {
            CObjBase* p = rhs.m_pObj;
            new ( &m_pObj )ObjPtr( p, false );
            rhs.m_pObj.Detach();
            break;
        }
    case typeByteArr:
        {
            CBuffer* p = rhs.m_pBuf;
            new ( &m_pBuf )BufPtr( p, false );
            rhs.m_pBuf.Detach();
            break;
        }
    case typeString:
        {
            new( &m_strVal ) string(
                std::move( rhs.m_strVal ) );
            rhs.Clear();
            break;
        }
    case typeFloat:
        m_fVal = rhs.m_fVal;
        break;
    case typeDouble:
        m_dblVal = rhs.m_dblVal;
        break;
    case typeDMsg:
        {
            m_qwVal = 0;
            m_pMsg = rhs.m_pMsg;
            rhs.m_pMsg.Clear();
            break;
        }
    case typeByte: 
        m_byVal = rhs.m_byVal;
        break;
    case typeUInt16:
        m_wVal = rhs.m_wVal;
        break;
    default:
        {
            std::string strMsg = DebugMsg(
                -EINVAL, "Invalid variant type" );
            throw std::invalid_argument( strMsg );
        }

    }
    m_iType = rhs.m_iType;
}

Variant::Variant( const CBuffer& oBuf )
{
    EnumTypeId iType = oBuf.GetExDataType();

    switch( iType )
    {
    case typeUInt32:
        m_dwVal = ( guint32& )oBuf;
        break;
    case typeUInt64:
        m_qwVal = ( guint64& )oBuf;
        break;
    case typeObj:
        new( &m_pObj ) ObjPtr( ( ObjPtr& )oBuf );
        break;
    case typeByteArr:
        new( &m_pBuf ) BufPtr( oBuf );
        break;
    case typeString:
        new( &m_strVal ) string( ( const char* )oBuf );
        break;
    case typeFloat:
        m_fVal = ( float& )oBuf;
        break;
    case typeDouble:
        m_dblVal = ( double& )oBuf;
        break;
    case typeByte: 
        m_byVal = ( guint8& )oBuf;
        break;
    case typeUInt16:
        m_wVal = ( guint16& )oBuf;
        break;
    case typeDMsg:
        new( &m_pMsg ) DMsgPtr( ( DMsgPtr& )oBuf );
        break;
    case typeNone:
        {
            // legacy support
            EnumDataType idt = oBuf.GetDataType();
            if( idt == DataTypeMem )
            {
                new( &m_pBuf ) BufPtr( oBuf );
                iType = typeByteArr;
                break;
            }
            else if( idt == DataTypeObjPtr )
            {
                new ( &m_pObj )ObjPtr( (ObjPtr&)oBuf );
                iType = typeObj;
                break;
            }
            else if( idt == DataTypeMsgPtr )
            {
                new ( &m_pMsg )DMsgPtr( (DMsgPtr&)oBuf );
                iType = typeDMsg;
                break;
            }
            // fall through
        }
    default:
        {
            std::string strMsg = DebugMsg(
                -EINVAL, "Invalid variant type" );
            throw std::invalid_argument( strMsg );
        }
    }
    m_iType = iType;
}

Variant& Variant::operator=(
    const Variant& rhs )
{
    Clear();
    switch( rhs.m_iType )
    {
    case typeUInt32:
        m_dwVal = rhs.m_dwVal;
        break;
    case typeUInt64:
        m_qwVal = rhs.m_qwVal;
        break;
    case typeObj:
        m_pObj = rhs.m_pObj;
        break;
    case typeByteArr:
        m_pBuf = rhs.m_pBuf;
        break;
    case typeString:
        new( &m_strVal ) string( rhs.m_strVal );
        break;
    case typeFloat:
        m_fVal = rhs.m_fVal;
        break;
    case typeDouble:
        m_dblVal = rhs.m_dblVal;
        break;
    case typeByte: 
        m_byVal = rhs.m_byVal;
        break;
    case typeUInt16:
        m_wVal = rhs.m_wVal;
        break;
    case typeDMsg:
        m_pMsg = rhs.m_pMsg;
        break;
    case typeNone:
	break;
    default:
	{
            std::string strMsg = DebugMsg(
                -EINVAL, "Invalid variant type" );
            throw std::invalid_argument( strMsg );
            break;
	}
    }
    m_iType = rhs.m_iType;
    return *this;
}

Variant& Variant::operator=( bool bVal )
{
    Clear();
    m_bVal = bVal;
    m_iType = typeByte;
    return *this;
}
Variant& Variant::operator=( guint8 byVal )
{
    Clear();
    m_byVal = byVal;
    m_iType = typeByte;
    return *this;
}
Variant& Variant::operator=( guint16 wVal )
{
    Clear();
    m_wVal = wVal;
    m_iType = typeUInt16;
    return *this;
}
Variant& Variant::operator=( guint32 dwVal )
{
    Clear();
    m_dwVal = dwVal;
    m_iType = typeUInt32;
    return *this;
}
Variant& Variant::operator=( guint64 qwVal )
{
    Clear();
    m_qwVal = qwVal;
    m_iType = typeUInt64;
    return *this;
}
Variant& Variant::operator=( float fVal )
{
    Clear();
    m_fVal = fVal;
    m_iType = typeFloat;
    return *this;
}
Variant& Variant::operator=( double dblVal )
{
    Clear();
    m_dblVal = dblVal;
    m_iType = typeDouble;
    return *this;
}
Variant& Variant::operator=( const ObjPtr& pObj )
{
    if( m_iType == typeObj )
    {
        m_pObj = pObj;
        return *this;
    }
    Clear();
    new( &m_pObj ) ObjPtr( pObj );
    m_iType = typeObj;
    return *this;
}
Variant& Variant::operator=( const BufPtr& pBuf )
{
    if( m_iType == typeByteArr )
    {
        m_pBuf = pBuf;
        return *this;
    }
    Clear();
    new( &m_pBuf ) BufPtr( pBuf );
    m_iType = typeByteArr;
    return *this;
}
Variant& Variant::operator=( const DMsgPtr& msgVal )
{
    if( m_iType == typeDMsg )
    {
        m_pMsg = msgVal;
        return *this;
    }
    Clear();
    m_pMsg = msgVal;
    m_iType = typeDMsg;
    return *this;
}

/*Variant& Variant::operator=( DBusMessage* msgVal )
{
    if( m_iType == typeDMsg )
    {
        m_pMsg = msgVal;
        return *this;
    }
    Clear();
    m_pMsg = msgVal;
    m_iType = typeDMsg;
    return *this;
}*/

Variant& Variant::operator=( const string& strVal )
{
    if( m_iType == typeString )
    {
        m_strVal = strVal;
        return *this;
    }
    Clear();
    new( &m_strVal ) string( strVal );
    m_iType = typeString;
    return *this;
}

Variant& Variant::operator=( const char* szVal )
{
    if( m_iType == typeString )
    {
        m_strVal = szVal;
        return *this;
    }
    Clear();
    new( &m_strVal ) string( szVal );
    m_iType = typeString;
    return *this;
}

Variant& Variant::operator=( const CBuffer& oBuf )
{
    Clear();
    new( this ) Variant( oBuf );
    return *this;
}

Variant& Variant::operator=( gint8 val )
{
    return operator=( ( guint8 )val );
}
Variant& Variant::operator=( gint16 val )
{
    return operator=( ( guint16 )val );
}
Variant& Variant::operator=( gint32 val )
{
    return operator=( ( guint32 )val );
}
Variant& Variant::operator=( gint64 val )
{
    return operator=( ( guint64 )val );
}
Variant& Variant::operator=( EnumEventId val )
{
    return operator=( ( guint32 )val );
}
Variant& Variant::operator=( EnumClsid val )
{
    return operator=( ( guint32 )val );
}
Variant& Variant::operator=( EnumPropId val )
{
    return operator=( ( guint32 )val );
}

void Variant::Clear()
{
    switch( m_iType )
    {
    case typeObj:
        {
            m_pObj.Clear();
            break;
        }
    case typeDMsg:
        {
            m_pMsg.Clear();
            break;
        }
    case typeByteArr:
        {
            m_pBuf.Clear();
            break;
        }
    case typeString:
        {
            m_strVal.~string(); 
            m_qwVal = 0;
            break;
        }
    case typeNone:
    default:
        m_qwVal = 0;
        break;
    }
    m_iType = typeNone;
}

BufPtr Variant::ToBuf() const
{
    EnumTypeId iType = m_iType;
    BufPtr pBuf;

    if( iType == typeByteArr )
    { return m_pBuf; }

    pBuf.NewObj();
    switch( iType )
    {
    case typeUInt32:
        *pBuf = m_dwVal;
        break;
    case typeUInt64:
        *pBuf = m_qwVal;
        break;
    case typeObj:
        *pBuf = m_pObj;
        break;
    case typeString:
        *pBuf = m_strVal;
        break;
    case typeFloat:
        *pBuf = m_fVal;
        break;
    case typeDouble:
        *pBuf = m_dblVal;
        break;
    case typeByte: 
        *pBuf = m_byVal;
        break;
    case typeUInt16:
        *pBuf = m_wVal;
        break;
    case typeDMsg:
        *pBuf = m_pMsg;
        break;
    default:
        {
            std::string strMsg = DebugMsg(
                -EINVAL, "Invalid variant type" );
            throw std::invalid_argument( strMsg );
        }
    }
    return pBuf;
}
bool Variant::operator==(
    const Variant& r ) const
{
    EnumTypeId iType = GetTypeId();
    if( iType != r.GetTypeId() )
        return false;

    bool ret = true;
    switch( iType )
    {
    case typeByte:
        ret =( m_byVal == r.m_byVal );
        break;
    case typeUInt16:
        ret =( m_wVal == r.m_wVal );
        break;
    case typeUInt32:
        ret =( m_dwVal == r.m_dwVal );
        break;
    case typeUInt64:
        ret =( m_qwVal == r.m_qwVal );
        break;
    case typeFloat:
        ret =( abs( m_fVal - r.m_fVal ) <
            0.000001 );
        break;
    case typeDouble:
        ret =( abs( m_dblVal - r.m_dblVal ) <
            0.0000000001 );
        break;
    case typeObj:
        ret =( m_pObj == r.m_pObj );
        break;
    case typeByteArr:
        ret =( *m_pBuf == *r.m_pBuf );
        break;
    case typeDMsg:
        ret = ( m_pMsg.ptr() ==
            r.m_pMsg.ptr() );
        break;
    case typeString:
        ret = ( m_strVal == r.m_strVal );
        break;
    case typeNone:
        ret = true;
        break;
    }
    return ret;
}

}
