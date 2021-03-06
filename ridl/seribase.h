/*
 * =====================================================================================
 *
 *       Filename:  seribase.h
 *
 *    Description:  declarations of utilities for ridl serializations
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
#pragma once
#include "rpc.h"
#include "proxy.h"
#include <vector>
#include <map>

#define DATATYPE_COMP_MASK 0x300UL
#define MAX_ELEM_COUNT  1000000

enum EnumContType : guint32
{
    typeArray = 0x100UL,
    typeMap = 0x200UL,

};

template < typename T >
struct is_vector{
    static const bool value = false;
};

template < typename T, typename Alloc >
struct is_vector< std::vector< T, Alloc > > {
    static const bool value = true;
};

template < typename T >
struct is_map{
    static const bool value = false;
};

template < typename key, typename T, typename Compare, typename Alloc >
struct is_map< std::map< key, T, Compare, Alloc > >{
    static const bool value = true;
};

struct CContCtx
{
    EnumContType m_iContType = typeArray;
    EnumTypeId  m_iType = typeNone;
    guint32*    m_pdwBytes = nullptr;
    guint32*    m_pdwCount = nullptr;
    guint32     m_dwCurSize = 0;
    bool        m_bSerialize = true;

    CContCtx()
    {}

    CContCtx( const CContCtx& rhs )
    {
        m_iContType = rhs.m_iContType;
        m_iType = rhs.m_iType;
        m_pdwBytes = rhs.m_pdwBytes;
        m_pdwCount = rhs.m_pdwCount;
    }
};

struct HSTREAM
{
    HANDLE m_hStream = INVALID_HANDLE;
    guint64 m_qwStmHash = 0;
    CRpcServices* m_pIf = nullptr;
    gint32 Serialize( BufPtr& pBuf ) const;
    gint32 Deserialize( BufPtr& );
};

class CSerialBase
{
    public:
    CSerialBase()
    {}

    template< typename T >
    inline gint32 Serialize(
        BufPtr& pBuf, const T& val ) const
    { return -ENOTSUP; }

    template< typename T >
    gint32 Deserialize(
        BufPtr& pBuf, T& val )
    { return -ENOTSUP; }

    template< typename T,
        typename T2=typename std::enable_if<
            std::is_same<T, guint32 >::value
            || std::is_same<T, gint32 >::value
            || std::is_same<T, guint16 >::value
            || std::is_same<T, gint16 >::value
            || std::is_same<T, guint64>::value
            || std::is_same<T, gint64>::value
            || std::is_same<T, double >::value
            || std::is_same<T, float >::value
            || std::is_same<T, bool >::value
            || std::is_same<T, guint8 >::value
            || std::is_same<T, gint8 >::value
            || std::is_same<T, ObjPtr>::value
            || std::is_same<T, BufPtr>::value
            || std::is_same<T, std::string>::value,
             T >::type >
    gint32 SerialElem(
        BufPtr& pBuf, const T& val,
        const char* szSignature )
    { return Serialize( pBuf, val ); }

    template< typename T,
        typename T2=typename std::enable_if<
            is_vector<T>::value, T >::type,
        typename T3 = T >
    gint32 SerialElem(
        BufPtr& pBuf, const T& val,
        const char* szSignature )
    { return SerializeArray( pBuf, val, szSignature ); }

    template< typename T,
        typename T2=typename std::enable_if<
            is_map<T>::value, T >::type,
        typename T3 = T,
        typename T4 = T >
    gint32 SerialElem(
        BufPtr& pBuf, const T& val,
        const char* szSignature )
    { return SerializeMap( pBuf, val, szSignature ); }

    template< typename T,
        typename T2=typename std::enable_if<
            std::is_base_of<CSerialBase, T>::value, T >::type,
        typename T3 = T,
        typename T4 = T,
        typename T5 = T >
    gint32 SerialElem(
        BufPtr& pBuf, const T& val,
        const char* szSignature )
    { return SerialStruct( pBuf, val ); }

    template< typename T,
        typename T2=typename std::enable_if<
            std::is_same<HSTREAM, T>::value, T >::type,
        typename T3 = T,
        typename T4 = T,
        typename T5 = T,
        typename T6 = T >
    gint32 SerialElem(
        BufPtr& pBuf, const T& val,
        const char* szSignature )
    { return val.Serialize( pBuf ); }

    template< typename T >
    gint32 SerializeArray(
        BufPtr& pBuf, T& val,
        const char* szSignature )
    {
        if( pBuf.IsEmpty() ||
            pBuf->empty() ||
            szSignature == nullptr ||
            *szSignature != '(' )
            return -EINVAL;

        guint32 dwHdrSize = sizeof( guint32 ) * 2;
        if( pBuf->size() < dwHdrSize ) 
            return -EINVAL;

        CContCtx occ;
        std::string strElemSig = szSignature + 1;
        if( strElemSig.empty() ||
            strElemSig.back() != ')' )
            return -EINVAL;

        std::string::iterator itr =
            strElemSig.begin() +
            ( strElemSig.size() - 1 );

        strElemSig.erase( itr );
        occ.m_pdwBytes = ( guint32* )pBuf->ptr();
        occ.m_pdwCount = ( guint32* )
            pBuf->ptr() + sizeof( guint32 );

        pBuf->IncOffset( dwHdrSize );
        if( val.size() == 0 )
        {
            *occ.m_pdwBytes = 0;
            *occ.m_pdwCount = 0;
            return STATUS_SUCCESS;
        }

        gint32 ret = 0;
        *occ.m_pdwCount = htonl( val.size() );
        for( auto& elem : val )
        {
            ret = SerialElem(
                pBuf, elem, strElemSig.c_str() );
            if( ERROR( ret ) )
                break;
        }

        if( ERROR( ret ) )
            return ret;

        guint32 dwBytes =
            pBuf->ptr() - ( char* )occ.m_pdwBytes;
        *occ.m_pdwBytes = htonl( dwBytes );

        return STATUS_SUCCESS;
    }

    template< typename key, typename val >
    gint32 SerializeMap(
        BufPtr& pBuf,
        const std::map< key, val >& mapVals,
        const char* szSignature )
    {
        if( pBuf.IsEmpty() ||
            pBuf->empty() ||
            szSignature == nullptr ||
            *szSignature != '[' )
            return -EINVAL;

        guint32 dwHdrSize = sizeof( guint32 ) * 2;
        if( pBuf->size() < dwHdrSize ) 
            return -EINVAL;

        CContCtx occ;
        std::string strElemSig = szSignature + 1;
        if( strElemSig.empty() ||
            strElemSig.back() != ']' )
            return -EINVAL;

        std::string::iterator itr =
            strElemSig.begin() +
            ( strElemSig.size() - 1 );

        strElemSig.erase( itr );
        occ.m_pdwBytes = ( guint32* )pBuf->ptr();
        occ.m_pdwCount = ( guint32* )
            pBuf->ptr() + sizeof( guint32 );

        std::string strKeySig =
            strElemSig.substr( 0, 1 );
        strElemSig.erase( strElemSig.begin() );
        pBuf->IncOffset( dwHdrSize );
        if( mapVals.size() == 0 )
        {
            *occ.m_pdwBytes = 0;
            *occ.m_pdwCount = 0;
            return STATUS_SUCCESS;
        }
        gint32 ret = 0;
        *occ.m_pdwCount = htonl( mapVals.size() );
        for( auto& elem : mapVals )
        {
            ret = SerialElem(
                pBuf, elem.first, strKeySig.c_str() );
            if( ERROR( ret ) )
                break;
            ret = SerialElem(
                pBuf, elem.second, strElemSig.c_str() );
            if( ERROR( ret ) )
                break;
        }

        if( ERROR( ret ) )
            return ret;

        guint32 dwBytes =
            pBuf->ptr() - ( char* )occ.m_pdwBytes;
        *occ.m_pdwBytes = htonl( dwBytes );

        return ret;
    }

    template< typename T >
    gint32 SerialStruct(
        BufPtr& pBuf, const T& val )
    {
        return val.Serialize( pBuf );
    }

    template< typename T,
        typename T2=typename std::enable_if<
            std::is_same<T, guint32 >::value
            || std::is_same<T, gint32 >::value
            || std::is_same<T, guint16 >::value
            || std::is_same<T, gint16 >::value
            || std::is_same<T, guint64>::value
            || std::is_same<T, gint64>::value
            || std::is_same<T, double >::value
            || std::is_same<T, float >::value
            || std::is_same<T, bool >::value
            || std::is_same<T, guint8 >::value
            || std::is_same<T, gint8 >::value
            || std::is_same<T, ObjPtr>::value
            || std::is_same<T, BufPtr>::value
            || std::is_same<T, std::string>::value,
             T >::type >
    gint32 DeserialElem(
        BufPtr& pBuf, T& val,
        const char* szSignature )
    { return Deserialize( pBuf, val ); }

    template< typename T,
        typename T2=typename std::enable_if<
            is_vector<T>::value, T >::type,
        typename T3 = T >
    gint32 DeserialElem(
        BufPtr& pBuf, T& val,
        const char* szSignature )
    { return DeserialArray( pBuf, val, szSignature ); }

    template< typename T,
        typename T2=typename std::enable_if<
            is_map<T>::value, T >::type,
        typename T3 = T,
        typename T4 = T >
    gint32 DeserialElem(
        BufPtr& pBuf, T& val,
        const char* szSignature )
    { return DeserialMap( pBuf, val, szSignature ); }

    template< typename T,
        typename T2=typename std::enable_if<
            std::is_base_of<CObjBase, T>::value, T >::type,
        typename T3 = T,
        typename T4 = T,
        typename T5 = T >
    gint32 DeserialElem(
        BufPtr& pBuf, T& val,
        const char* szSignature )
    { return DeserialStruct( pBuf, val ); }

    template< typename T,
        typename T2=typename std::enable_if<
            std::is_same<HSTREAM, T>::value, T >::type,
        typename T3 = T,
        typename T4 = T,
        typename T5 = T,
        typename T6 = T >
    gint32 DeserialElem(
        BufPtr& pBuf, T& val,
        const char* szSignature )
    { return val.Deserialize( pBuf ); }

    template< typename T >
    gint32 DeserialArray(
        BufPtr& pBuf, T& val,
        const char* szSignature )
    {
        if( pBuf.IsEmpty() ||
            pBuf->empty() ||
            szSignature == nullptr ||
            *szSignature != '(' )
            return -EINVAL;

        guint32 dwHdrSize = sizeof( guint32 ) * 2;
        if( pBuf->size() < dwHdrSize ) 
            return -EINVAL;

        CContCtx occ;
        std::string strElemSig = szSignature + 1;
        if( strElemSig.empty() ||
            strElemSig.back() != ')' )
            return -EINVAL;

        std::string::iterator itr =
            strElemSig.begin() +
            ( strElemSig.size() - 1 );

        strElemSig.erase( itr );
        occ.m_pdwBytes = ( guint32* )pBuf->ptr();
        occ.m_pdwCount = ( guint32* )
            pBuf->ptr() + sizeof( guint32 );

        pBuf->IncOffset( dwHdrSize );
        if( *occ.m_pdwCount == 0 )
            return STATUS_SUCCESS;

        gint32 ret = 0;
        guint32 dwCount = ntohl( *occ.m_pdwCount );
        if( dwCount > MAX_ELEM_COUNT )
            return -ERANGE;

        val.resize( dwCount );
        for( auto& elem : val )
        {
            ret = DeserialElem(
                pBuf, elem, strElemSig.c_str() );
            if( ERROR( ret ) )
                break;
        }

        if( ERROR( ret ) )
            return ret;

        guint32 dwBytes =
            pBuf->ptr() - ( char* )occ.m_pdwBytes;

        if( ntohl( *occ.m_pdwBytes ) != dwBytes )
            return -EBADMSG;

        return STATUS_SUCCESS;
    }

    template< typename keytype, typename valtype >
    gint32 DeserialMap( BufPtr& pBuf,
        std::map< keytype, valtype >& mapVals,
        const char* szSignature )
    {
        if( pBuf.IsEmpty() ||
            pBuf->empty() ||
            szSignature == nullptr ||
            *szSignature != '[' )
            return -EINVAL;

        guint32 dwHdrSize = sizeof( guint32 ) * 2;
        if( pBuf->size() < dwHdrSize ) 
            return -EINVAL;

        CContCtx occ;
        std::string strElemSig = szSignature + 1;
        if( strElemSig.empty() ||
            strElemSig.back() != ']' )
            return -EINVAL;

        std::string::iterator itr =
            strElemSig.begin() +
            ( strElemSig.size() - 1 );

        strElemSig.erase( itr );
        occ.m_pdwBytes = ( guint32* )pBuf->ptr();
        occ.m_pdwCount = ( guint32* )
            pBuf->ptr() + sizeof( guint32 );

        std::string strKeySig =
            strElemSig.substr( 0, 1 );
        strElemSig.erase( strElemSig.begin() );
        pBuf->IncOffset( dwHdrSize );
        guint32 dwCount = ntohl(
            *occ.m_pdwCount );

        if( dwCount == 0 )
            return STATUS_SUCCESS;

        if( dwCount > MAX_ELEM_COUNT )
            return -ERANGE;

        gint32 ret = 0;
        for( guint32 i = 0; i < dwCount; ++i )
        {
            std::pair< keytype, valtype > elem;
            ret = DeserialElem(
                pBuf, elem.first,
                strKeySig.c_str() );

            if( ERROR( ret ) )
                break;

            ret = DeserialElem(
                pBuf, elem.second,
                strElemSig.c_str() );

            if( ERROR( ret ) )
                break;

            mapVals.insert( elem );
        }

        if( ERROR( ret ) )
            return ret;

        guint32 dwBytes =
            pBuf->ptr() - ( char* )occ.m_pdwBytes;

        if( ntohl( *occ.m_pdwBytes ) != dwBytes )
            return -EBADMSG;

        return ret;
    }

    template< typename T >
    gint32 DeserialStruct(
        BufPtr& pBuf, T& val )
    {
        return val.Deserialize( pBuf );
    }
};

class CStructBase : public CSerialBase
{
    public:
    typedef CSerialBase super;
    CStructBase() : super()
    {}
    
    virtual gint32 Serialize(
        BufPtr& pBuf ) const
    { return -ENOTSUP; }

    virtual gint32 Deserialize(
        BufPtr& pBuf, guint32 dwSize )
    { return -ENOTSUP; }
};


#define APPEND( pBuf_, ptr_, size_ ) \
do{ \
    guint32 dwOffset = ( pBuf_ )->offset(); \
    ( pBuf_ )->Append( ( guint8* )( ptr_ ), size_ ); \
    ( pBuf_ )->SetOffset( dwOffset + size_ ); \
}while( 0 )

template<>
inline gint32 CSerialBase::Serialize< gint16 >(
    BufPtr& pBuf, const gint16& val ) const
{
    if( pBuf.IsEmpty() )
        return -EINVAL;
    guint16 iVal = htons( ( guint16 )val );
    APPEND( pBuf, &iVal, sizeof( val ) );
    return STATUS_SUCCESS;
}

template<>
inline gint32 CSerialBase::Serialize< guint16 >(
    BufPtr& pBuf, const guint16& val ) const
{
    if( pBuf.IsEmpty() )
        return -EINVAL;
    guint16 iVal = htons( val );
    APPEND( pBuf, &iVal, sizeof( val ) );
    return STATUS_SUCCESS;
}

template<>
inline gint32 CSerialBase::Serialize< gint32 >(
    BufPtr& pBuf, const gint32& val ) const
{
    if( pBuf.IsEmpty() )
        return -EINVAL;
    guint32 iVal = htonl( ( guint32 )val );
    APPEND( pBuf, &iVal, sizeof( val ) );
    return STATUS_SUCCESS;
}

template<>
inline gint32 CSerialBase::Serialize< guint32 >(
    BufPtr& pBuf, const guint32& val ) const
{
    if( pBuf.IsEmpty() )
        return -EINVAL;
    guint32 iVal = htonl( val );
    APPEND( pBuf, &iVal, sizeof( val ) );
    return STATUS_SUCCESS;
}

template<>
inline gint32 CSerialBase::Serialize< gint64 >(
    BufPtr& pBuf, const gint64& val ) const
{
    if( pBuf.IsEmpty() )
        return -EINVAL;
    guint64 iVal = htonll( ( guint64& )val );
    APPEND( pBuf, &iVal, sizeof( val ) );
    return STATUS_SUCCESS;
}

template<>
inline gint32 CSerialBase::Serialize< guint64 >(
    BufPtr& pBuf, const guint64& val ) const
{
    if( pBuf.IsEmpty() )
        return -EINVAL;
    guint64 iVal = htonll( val );
    APPEND( pBuf, &iVal, sizeof( val ) );
    return STATUS_SUCCESS;
}

template<>
inline gint32 CSerialBase::Serialize< float >(
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
inline gint32 CSerialBase::Serialize< double >(
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
inline gint32 CSerialBase::Serialize< bool >(
    BufPtr& pBuf, const bool& val ) const
{
    if( pBuf.IsEmpty() )
        return -EINVAL;
    APPEND( pBuf, &val, sizeof( val ) );
    return STATUS_SUCCESS;
}

template<>
inline gint32 CSerialBase::Serialize< guint8 >(
    BufPtr& pBuf, const guint8& val ) const
{
    if( pBuf.IsEmpty() )
        return -EINVAL;
    APPEND( pBuf, &val, sizeof( val ) );
    return STATUS_SUCCESS;
}

template<>
inline gint32 CSerialBase::Serialize< char >(
    BufPtr& pBuf, const char& val ) const
{
    if( pBuf.IsEmpty() )
        return -EINVAL;
    APPEND( pBuf, &val, sizeof( val ) );
    return STATUS_SUCCESS;
}

template<>
inline gint32 CSerialBase::Serialize< std::string >(
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
inline gint32 CSerialBase::Serialize< ObjPtr >(
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
inline gint32 CSerialBase::Serialize< BufPtr >(
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
    BufPtr& pBuf, bool& val );

template<>
gint32 CSerialBase::Deserialize< char >(
    BufPtr& pBuf, char& val );

template<>
gint32 CSerialBase::Deserialize< guint8 >(
    BufPtr& pBuf, guint8& val );

template<>
gint32 CSerialBase::Deserialize< gint16 >(
    BufPtr& pBuf, gint16& val );
template<>
gint32 CSerialBase::Deserialize< guint16 >(
    BufPtr& pBuf, guint16& val );

template<>
gint32 CSerialBase::Deserialize< gint16 >(
    BufPtr& pBuf, gint16& val );

template<>
gint32 CSerialBase::Deserialize< gint32 >(
    BufPtr& pBuf, gint32& val );

template<>
gint32 CSerialBase::Deserialize< guint64 >(
    BufPtr& pBuf, guint64& val );

template<>
gint32 CSerialBase::Deserialize< gint64 >(
    BufPtr& pBuf, gint64& val );

template<>
gint32 CSerialBase::Deserialize< float >(
    BufPtr& pBuf, float& val );

template<>
gint32 CSerialBase::Deserialize< double >(
    BufPtr& pBuf, double& val );

template<>
gint32 CSerialBase::Deserialize< char* >(
    BufPtr& pBuf, char*& val );

template<>
gint32 CSerialBase::Deserialize< std::string >(
    BufPtr& pBuf, std::string& val );

template<>
gint32 CSerialBase::Deserialize< ObjPtr >(
    BufPtr& pBuf, ObjPtr& val );

// bytearray
template<>
gint32 CSerialBase::Deserialize< BufPtr >(
    BufPtr& pBuf, BufPtr& val );
