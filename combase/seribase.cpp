/*
 * =====================================================================================
 *
 *       Filename:  seribase.cpp
 *
 *    Description:  implementation of utilities for ridl serializations
 *
 *        Version:  1.0
 *        Created:  02/23/2021 09:32:32 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2021 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#include "rpc.h"
#include "seribase.h"
namespace rpcf
{

template<>
gint32 CSerialBase::Serialize< gint16 >(
    BufPtr& pBuf, const gint16& val ) const
{
    if( pBuf.IsEmpty() )
        return -EINVAL;
    guint16 iVal = htons( ( guint16 )val );
    APPEND( pBuf, &iVal, sizeof( val ) );
    return STATUS_SUCCESS;
}

template<>
gint32 CSerialBase::Serialize< guint16 >(
    BufPtr& pBuf, const guint16& val ) const
{
    if( pBuf.IsEmpty() )
        return -EINVAL;
    guint16 iVal = htons( val );
    APPEND( pBuf, &iVal, sizeof( val ) );
    return STATUS_SUCCESS;
}

template<>
gint32 CSerialBase::Serialize< gint32 >(
    BufPtr& pBuf, const gint32& val ) const
{
    if( pBuf.IsEmpty() )
        return -EINVAL;
    guint32 iVal = htonl( ( guint32 )val );
    APPEND( pBuf, &iVal, sizeof( val ) );
    return STATUS_SUCCESS;
}

template<>
gint32 CSerialBase::Serialize< guint32 >(
    BufPtr& pBuf, const guint32& val ) const
{
    if( pBuf.IsEmpty() )
        return -EINVAL;
    guint32 iVal = htonl( val );
    APPEND( pBuf, &iVal, sizeof( val ) );
    return STATUS_SUCCESS;
}

template<>
gint32 CSerialBase::Serialize< gint64 >(
    BufPtr& pBuf, const gint64& val ) const
{
    if( pBuf.IsEmpty() )
        return -EINVAL;
    guint64 iVal = htonll( ( guint64& )val );
    APPEND( pBuf, &iVal, sizeof( val ) );
    return STATUS_SUCCESS;
}

template<>
gint32 CSerialBase::Serialize< guint64 >(
    BufPtr& pBuf, const guint64& val ) const
{
    if( pBuf.IsEmpty() )
        return -EINVAL;
    guint64 iVal = htonll( val );
    APPEND( pBuf, &iVal, sizeof( val ) );
    return STATUS_SUCCESS;
}

template<>
gint32 CSerialBase::Serialize< float >(
    BufPtr& pBuf, const float& val ) const
{
    if( pBuf.IsEmpty() )
        return -EINVAL;
    guint32* pval = ( guint32* )&val;
    guint32 iVal = htonll( *pval );
    APPEND( pBuf, &iVal, sizeof( val ) );
    return STATUS_SUCCESS;
}

template<>
gint32 CSerialBase::Serialize< double >(
    BufPtr& pBuf, const double& val ) const
{
    if( pBuf.IsEmpty() )
        return -EINVAL;
    guint64* pval = ( guint64* )&val;
    guint64 iVal = htonll( *pval );
    APPEND( pBuf, &iVal, sizeof( val ) );
    return STATUS_SUCCESS;
}

template<>
gint32 CSerialBase::Serialize< bool >(
    BufPtr& pBuf, const bool& val ) const
{
    if( pBuf.IsEmpty() )
        return -EINVAL;
    APPEND( pBuf, &val, sizeof( val ) );
    return STATUS_SUCCESS;
}

template<>
gint32 CSerialBase::Serialize< guint8 >(
    BufPtr& pBuf, const guint8& val ) const
{
    if( pBuf.IsEmpty() )
        return -EINVAL;
    APPEND( pBuf, &val, sizeof( val ) );
    return STATUS_SUCCESS;
}

template<>
gint32 CSerialBase::Serialize< char >(
    BufPtr& pBuf, const char& val ) const
{
    if( pBuf.IsEmpty() )
        return -EINVAL;
    APPEND( pBuf, &val, sizeof( val ) );
    return STATUS_SUCCESS;
}

template<>
gint32 CSerialBase::Serialize< std::string >(
    BufPtr& pBuf, const std::string& val ) const
{
    if( pBuf.IsEmpty() )
        return -EINVAL;
    guint32 i = val.size();
    if( i >= MAX_BYTES_PER_BUFFER )
        return -ERANGE;
    Serialize( pBuf, i );
    if( i > 0 )
        APPEND( pBuf, val.c_str(), i );
    return STATUS_SUCCESS;
}

template<>
gint32 CSerialBase::Serialize< ObjPtr >(
    BufPtr& pBuf, const ObjPtr& val ) const
{
    if( pBuf.IsEmpty() )
        return -EINVAL;

    if( val.IsEmpty() )
    {
        // protoSeriNone
        // clsid
        Serialize( pBuf,
            ( guint32 )clsid( Invalid ) );
        // size
        Serialize( pBuf, 0 );
        // version
        Serialize( pBuf, 1 );
        return STATUS_SUCCESS;
    }

    BufPtr pNewBuf( true );
    gint32 ret = val->Serialize( *pNewBuf );
    if( ERROR( ret ) )
        return ret;

    guint32 i = pNewBuf->size();
    if( i >= MAX_BYTES_PER_BUFFER )
        return -ERANGE;

    APPEND( pBuf, pNewBuf->ptr(),
        pNewBuf->size() );

    return STATUS_SUCCESS;
}

// bytearray
template<>
gint32 CSerialBase::Serialize< BufPtr >(
    BufPtr& pBuf, const BufPtr& val ) const
{
    if( pBuf.IsEmpty() )
        return -EINVAL;

    guint32 i = 0;
    if( val.IsEmpty() )
        i = 0;
    else
        i = val->size();

    if( i >= MAX_BYTES_PER_BUFFER )
        return -ERANGE;

    // append the size
    this->Serialize( pBuf, i );
    if( i == 0 )
        return STATUS_SUCCESS;

    APPEND( pBuf, val->ptr(), val->size() );
    return STATUS_SUCCESS;
}

template<>
gint32 CSerialBase::Deserialize< bool >(
    BufPtr& pBuf, bool& val )
{
    if( pBuf.IsEmpty() || pBuf->empty() )
        return -EINVAL;
    if( pBuf->size() < sizeof( val ) )
        return -ERANGE;
    memcpy( &val, pBuf->ptr(), sizeof( val ) );
    pBuf->IncOffset( sizeof( val ) );
    return STATUS_SUCCESS;
}

template<>
gint32 CSerialBase::Deserialize< char >(
    BufPtr& pBuf, char& val )
{
    if( pBuf.IsEmpty() || pBuf->empty() )
        return -EINVAL;
    if( pBuf->size() < sizeof( val ) )
        return -ERANGE;
    memcpy( &val, pBuf->ptr(), sizeof( val ) );
    pBuf->IncOffset( sizeof( val ) );
    return STATUS_SUCCESS;
}

template<>
gint32 CSerialBase::Deserialize< guint8 >(
    BufPtr& pBuf, guint8& val )
{
    if( pBuf.IsEmpty() || pBuf->empty() )
        return -EINVAL;
    if( pBuf->size() < sizeof( val ) )
        return -ERANGE;
    memcpy( &val, pBuf->ptr(), sizeof( val ) );
    pBuf->IncOffset( sizeof( val ) );
    return STATUS_SUCCESS;
}

template<>
gint32 CSerialBase::Deserialize< gint16 >(
    BufPtr& pBuf, gint16& val )
{
    if( pBuf.IsEmpty() || pBuf->empty() )
        return -EINVAL;
    if( pBuf->size() < sizeof( val ) )
        return -ERANGE;
    memcpy( &val, pBuf->ptr(), sizeof( val ) );
    val = ntohs( val );
    pBuf->IncOffset( sizeof( val ) );
    return STATUS_SUCCESS;
}
template<>
gint32 CSerialBase::Deserialize< guint16 >(
    BufPtr& pBuf, guint16& val )
{
    if( pBuf.IsEmpty() || pBuf->empty() )
        return -EINVAL;
    if( pBuf->size() < sizeof( val ) )
        return -ERANGE;
    memcpy( &val, pBuf->ptr(), sizeof( val ) );
    val = ntohs( val );
    pBuf->IncOffset( sizeof( val ) );
    return STATUS_SUCCESS;
}

template<>
gint32 CSerialBase::Deserialize< guint32 >(
    BufPtr& pBuf, guint32& val )
{
    if( pBuf.IsEmpty() || pBuf->empty() )
        return -EINVAL;
    if( pBuf->size() < sizeof( val ) )
        return -ERANGE;
    memcpy( &val, pBuf->ptr(), sizeof( val ) );
    val = ntohl( val );
    pBuf->IncOffset( sizeof( val ) );
    return STATUS_SUCCESS;
}

template<>
gint32 CSerialBase::Deserialize< gint32 >(
    BufPtr& pBuf, gint32& val )
{
    if( pBuf.IsEmpty() || pBuf->empty() )
        return -EINVAL;
    if( pBuf->size() < sizeof( val ) )
        return -ERANGE;
    memcpy( &val, pBuf->ptr(), sizeof( val ) );
    val = ntohl( ( guint32 )val );
    pBuf->IncOffset( sizeof( val ) );
    return STATUS_SUCCESS;
}

template<>
gint32 CSerialBase::Deserialize< guint64 >(
    BufPtr& pBuf, guint64& val )
{
    if( pBuf.IsEmpty() || pBuf->empty() )
        return -EINVAL;
    if( pBuf->size() < sizeof( val ) )
        return -ERANGE;
    memcpy( &val, pBuf->ptr(), sizeof( val ) );
    val = ntohll( val );
    pBuf->IncOffset( sizeof( val ) );
    return STATUS_SUCCESS;
}

template<>
gint32 CSerialBase::Deserialize< gint64 >(
    BufPtr& pBuf, gint64& val )
{
    if( pBuf.IsEmpty() || pBuf->empty() )
        return -EINVAL;
    if( pBuf->size() < sizeof( val ) )
        return -ERANGE;
    memcpy( &val, pBuf->ptr(), sizeof( val ) );
    val = ntohll( ( guint64 )val );
    pBuf->IncOffset( sizeof( val ) );
    return STATUS_SUCCESS;
}

template<>
gint32 CSerialBase::Deserialize< float >(
    BufPtr& pBuf, float& val )
{
    if( pBuf.IsEmpty() || pBuf->empty() )
        return -EINVAL;
    if( pBuf->size() < sizeof( val ) )
        return -ERANGE;
    guint32 dwVal = 0;
    memcpy( &dwVal, pBuf->ptr(), sizeof( val ) );
    dwVal = ntohl( dwVal );
    val = *( float* )&dwVal;
    pBuf->IncOffset( sizeof( val ) );
    return STATUS_SUCCESS;
}

template<>
gint32 CSerialBase::Deserialize< double >(
    BufPtr& pBuf, double& val )
{
    if( pBuf.IsEmpty() || pBuf->empty() )
        return -EINVAL;
    if( pBuf->size() < sizeof( val ) )
        return -ERANGE;
    guint64 qwVal = 0;
    memcpy( &qwVal, pBuf->ptr(), sizeof( val ) );
    qwVal = ntohll( qwVal );
    val = *( double* )&qwVal;
    pBuf->IncOffset( sizeof( val ) );
    return STATUS_SUCCESS;
}

template<>
gint32 CSerialBase::Deserialize< std::string >(
    BufPtr& pBuf, std::string& val )
{
    if( pBuf.IsEmpty() || pBuf->empty() )
        return -EINVAL;

    gint32 ret = STATUS_SUCCESS;
    guint32 dwCount = 0;
    ret = this->Deserialize( pBuf, dwCount );
    if( ERROR( ret ) )
        return ret;
    if( dwCount == 0 )
    {
        val = "";
        return ret;
    }
    guint32 dwBytes = 
        std::min( dwCount, pBuf->size() );
    val.append( pBuf->ptr(), dwBytes );
    pBuf->IncOffset( dwBytes );
    return STATUS_SUCCESS;
}

template<>
gint32 CSerialBase::Deserialize< ObjPtr >(
    BufPtr& pBuf, ObjPtr& val )
{
    if( pBuf.IsEmpty() )
        return -EINVAL;

    guint8* pHead = ( guint8* )pBuf->ptr();

    guint32 dwSize = 0;
    guint32 dwClsid = 0;

    guint32 dwHdrSize =
        sizeof( SERI_HEADER_BASE );

    if( pBuf->size() < dwHdrSize )
        return -ERANGE;

    memcpy( &dwSize,
        pHead + sizeof( guint32 ),
        sizeof( dwSize ) );

    memcpy( &dwClsid,
        pHead, sizeof( dwClsid ) );

    dwClsid = ntohl( dwClsid );
    dwSize = ntohl( dwSize );
    if( dwSize > pBuf->size() - dwHdrSize )
        return -ERANGE;

    if( dwSize == 0 )
    {
        // a null object
        val.Clear();
        return 0;
    }

    gint32 ret = val.NewObj(
        ( EnumClsid )dwClsid );
    if( ERROR( ret ) )
        return ret;

    ret = val->Deserialize( *pBuf );
    if( ERROR( ret ) )
        return ret;

    pBuf->IncOffset( dwSize + dwHdrSize );
    return STATUS_SUCCESS;
}

template<>
gint32 CSerialBase::Deserialize< BufPtr >(
    BufPtr& pBuf, BufPtr& val )
{
    if( pBuf.IsEmpty() || val.IsEmpty() )
        return -EINVAL;

    guint32 i = 0;
    gint32 ret = Deserialize( pBuf, i );
    if( ERROR( ret ) )
        return ret;

    if( i == 0 )
    {
        // an empty buffer
        val.Clear();
        return STATUS_SUCCESS;
    }

    ret = val->Append( pBuf->ptr(), i );
    if( ERROR( ret ) )
        return ret;

    pBuf->IncOffset( i );
    return STATUS_SUCCESS;
}

}
