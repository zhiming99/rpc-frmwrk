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
#include <rpc.h>

using namespace rpcfrmwrk;
#include "streamex.h"
#include "seribase.h"

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

gint32 HSTREAM::Serialize( BufPtr& pBuf ) const
{
    if( pBuf.IsEmpty() )
        return -EINVAL;

    if( m_pIf == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    guint64 qwHash = 0;

    if( m_pIf->IsServer() )
    {
        IStream* pStm =
            dynamic_cast< IStream* >( m_pIf );
        if( pStm == nullptr )
            return -EFAULT;

        InterfPtr pIf;
        ret = pStm->GetUxStream( m_hStream, pIf );
        if( ERROR( ret ) )
            return ret;

        guint64 qwId = pIf->GetObjId();
        guint64 qwHash = 0;
        ret = GetObjIdHash( qwId, qwHash );
        if( ERROR( ret ) )
            return ret;
    }
    else
    {
        CStreamProxySync* pStm =
            dynamic_cast< CStreamProxySync* >
                ( m_pIf );
        if( pStm == nullptr )
            return -EFAULT;

        ret = pStm->GetPeerIdHash(
            m_hStream, qwHash );
        if( ERROR( ret ) )
            return ret;
    }
    CSerialBase oSerial;
    return oSerial.Serialize( pBuf, qwHash );
}

gint32 HSTREAM::Deserialize( BufPtr& pBuf )
{
    if( pBuf.IsEmpty() ) 
        return -EINVAL;

    if( m_pIf == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    guint64 qwHash = 0;
    CSerialBase oSerial;
    ret = oSerial.Deserialize( pBuf, qwHash );
    if( ERROR( ret ) )
        return ret;

    if( m_pIf->IsServer() )
    {
        CStreamServerSync* pStm =
            dynamic_cast< CStreamServerSync* >
                ( m_pIf );
        if( pStm == nullptr )
            return -EFAULT;

        m_hStream = pStm->
            GetChanByIdHash( qwHash );
        if( m_hStream == INVALID_HANDLE )
            return -ENOENT;
    }
    else
    {
        CStreamProxySync* pStm =
            dynamic_cast< CStreamProxySync* >
                ( m_pIf );
        if( pStm == nullptr )
            return -EFAULT;

        m_hStream = pStm->
            GetChanByIdHash( qwHash );
        if( m_hStream == INVALID_HANDLE )
            return -ENOENT;
    }
    return STATUS_SUCCESS;
}
