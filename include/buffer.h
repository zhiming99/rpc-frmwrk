/*
 * =====================================================================================
 *
 *       Filename:  buffer.h
 *
 *    Description:  declaration of CBuffer
 *
 *        Version:  1.0
 *        Created:  07/09/2016 05:10:47 PM
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
#pragma once

#include <map>
#include <string>
#include <vector>
#include <atomic>
#include <algorithm>
#include <type_traits>
#include <string.h>
#include "defines.h"
#include "autoptr.h"
#include <dbus/dbus.h>
#include "dmsgptr.h"

using cchar = const char;

#define DATATYPE_MASK       0x0FUL
#define DATATYPE_MASK_EX    0xF0UL
#define BUF_MAX_SIZE        ( 512 * 1024 * 1024 )

namespace rpcfrmwrk
{

typedef enum {

     DataTypeMem = 0,
     DataTypeObjPtr = 0x01,
     DataTypeMsgPtr = 0x02,

} EnumDataType;

enum EnumTypeId
{
    typeNone = 0,
    typeByte, 
    typeUInt16,
    typeUInt32,
    typeUInt64,
    typeFloat,
    typeDouble,
    typeString,
    typeDMsg,
    typeObj,
    typeByteArr,
};


template< class T >
inline gint32 GetTypeId( T* pT )
{ return typeNone; }

template<>
inline gint32 GetTypeId( EnumEventId* pT )
{ return typeUInt32; }

template<>
inline gint32 GetTypeId( EnumClsid* pT )
{ return typeUInt32; }

template<>
inline gint32 GetTypeId( EnumPropId* pT )
{ return typeUInt32; }

template<>
inline gint32 GetTypeId( bool* pT )
{ return typeByte; }

template<>
inline gint32 GetTypeId( guint8* pT )
{ return typeByte; }

template<>
inline gint32 GetTypeId( gint8* pT )
{ return typeByte; }


template<>
inline gint32 GetTypeId( guint16* pT )
{ return typeUInt16; }

template<>
inline gint32 GetTypeId( gint16* pT )
{ return typeUInt16; }


template<>
inline gint32 GetTypeId( guint32* pT )
{ return typeUInt32; }

template<>
inline gint32 GetTypeId( int* pT )
{ 
    return ( sizeof( int ) == 4 ?
        typeUInt32 : typeUInt64 );
}

#if BUILD_64 == 0
template<>
inline gint32 GetTypeId( long* pT )
{ 
    return ( sizeof( long ) == 4 ?
        typeUInt32 : typeUInt64 );
}
#endif

template<>
inline gint32 GetTypeId( guint64* pT )
{ return typeUInt64; }

template<>
inline gint32 GetTypeId( gint64* pT )
{ return typeUInt64; }

template<>
inline gint32 GetTypeId( float* pT )
{ return typeFloat; }

template<>
inline gint32 GetTypeId( double* pT )
{ return typeDouble; }

template<>
inline gint32 GetTypeId( std::string* pT )
{ return typeString; }

template<>
inline gint32 GetTypeId( DMsgPtr* pT )
{ return typeDMsg; }

template<>
inline gint32 GetTypeId( ObjPtr* pT )
{ return typeObj; }

class CBuffer : public CObjBase
{
	char* m_pData;
	guint32 m_dwSize;
    guint32 m_dwType;
    guint32 m_dwOffset = 0;
    guint32 m_dwTailOff = 0;
    char m_arrBuf[ 48 ] __attribute__( ( aligned ( 8 ) ) );

    protected:

    gint32 ResizeWithOffset( guint32 dwSize );

	public:

    struct SERI_HEADER : public SERI_HEADER_BASE
    {
        typedef SERI_HEADER_BASE super;

        guint32 dwType;
        // clsid for the contained object
        guint32 dwObjClsid;

        SERI_HEADER() : SERI_HEADER_BASE()
        {
            dwClsid = clsid( CBuffer );
            dwType = 0;
            dwObjClsid = 0;
        }

        SERI_HEADER( const SERI_HEADER& rhs )
            :SERI_HEADER_BASE( rhs )
        {
            dwType = rhs.dwType;
            dwObjClsid = rhs.dwObjClsid;
        }

        SERI_HEADER& operator=( const SERI_HEADER& rhs )
        {
            super::operator=( rhs );
            dwType = rhs.dwType;
            dwObjClsid = rhs.dwObjClsid;
            return *this;
        }

        void ntoh()
        {
            super::ntoh();
            dwType = ntohl( dwType );
            dwObjClsid = ntohl( dwObjClsid );
        }

        void hton()
        {
            super::hton();
            dwType = htonl( dwType );
            dwObjClsid = ntohl( dwObjClsid );
        }
    };

	CBuffer( guint32 dwSize = 0 );
	CBuffer( const char* pData, guint32 dwSize );
    CBuffer( const ObjPtr& rhs );
    CBuffer( const DMsgPtr& rhs );

	~CBuffer();

    CBuffer( const CBuffer& oInit );

    inline void SetBuffer(
        char* pData,
        guint32 dwSize,
        guint32 dwOffset = 0 )
    {
        m_pData = pData;
        m_dwTailOff = m_dwSize = dwSize;
        m_dwOffset = dwOffset;
    }

	char* ptr() const;
	guint32 size() const;
    guint32 type() const;
    void SetDataType( EnumDataType dt );
    EnumDataType GetDataType() const;
	gint32 Resize( guint32 dwSize );

    inline guint32 GetActSize() const
    { return m_dwSize; }

    inline guint32 GetShadowSize() const
    { return GetActSize() - GetTailOff(); }

    inline guint32 GetTailOff() const
    { return m_dwTailOff; }

    inline gint32 SetOffset( guint32 dwOffset )
    {
        if( GetDataType() != DataTypeMem )
            return ERROR_STATE;

        if( m_pData == nullptr )
            return ERROR_STATE;

        if( dwOffset > GetTailOff() )
            return -EINVAL;

        m_dwOffset = dwOffset;
        return STATUS_SUCCESS;
    }

    inline void SetTailOff( guint32 dwSize )
    {
        if( GetDataType() != DataTypeMem )
            return;

        if( dwSize > GetActSize() )
            dwSize = GetActSize();

        if( dwSize < offset() )
            dwSize = offset();

        m_dwTailOff = dwSize;
    }

    inline void SetWinSize( guint32 dwSize )
    {
        SetTailOff( offset() + dwSize );
    }

    inline gint32 SlideWindow( guint32 dwNewSize )
    {
        if( GetDataType() != DataTypeMem )
            return ERROR_STATE;

        guint32 dwOldWin = size();
        SetWinSize( dwOldWin + dwNewSize );
        return IncOffset( dwOldWin );
    }

    inline guint32 offset() const
    { return m_dwOffset; }

    inline gint32 IncOffset( guint32 dwOffset )
    { return SetOffset( offset() + dwOffset ); }

    void SetExDataType( EnumTypeId dt );
    EnumTypeId GetExDataType() const;

    bool operator==( const CBuffer& rhs ) const;
    gint32 Serialize( CBuffer& oBuf ) const;
    gint32 SerializePrimeType( guint8* pDest ) const;
    gint32 Deserialize( const CBuffer& oBuf );
    gint32 DeserializePrimeType( guint8* pSrc );
    gint32 Deserialize( const char* pBuf );
    gint32 Append( const guint8* pBlock, guint32 dwSize );
    gint32 Append( const char* pBlock, guint32 dwSize );

    inline bool empty() const
    {
        return ( size() == 0 || ptr() == nullptr );
    }

    operator stdstr() const;
    operator DMsgPtr*() const;
    operator DMsgPtr&() const;
    operator ObjPtr&() const;
    operator ObjPtr*() const;

    template<typename T,
        typename T2= typename std::decay< T >::type,
        typename T3=typename std::enable_if<
            std::is_same<T2, guint32 >::value
            || std::is_same<T2, guint16 >::value
            || std::is_same<T2, gint16 >::value
            || std::is_same<T2, guint64>::value
            || std::is_same<T2, gint64>::value
            || std::is_same<T2, EnumEventId >::value
            || std::is_same<T2, EnumClsid >::value
            || std::is_same<T2, EnumPropId >::value
            || std::is_same<T2, double >::value
            || std::is_same<T2, float >::value
            || std::is_same<T2, int >::value
            || std::is_same<T2, unsigned int >::value
            || std::is_same<T2, bool >::value
            || std::is_same<T2, guint8 >::value
            || std::is_same<T2, gint8 >::value
            || std::is_same<T2, intptr_t >::value
            || std::is_same<T2, uintptr_t >::value,
             T2 >::type,
        typename forbidden=typename std::enable_if< 
            !std::is_same<T2, const char* >::value
            && !std::is_same<T2, std::initializer_list<char> >::value
            && !std::is_same<T2, std::allocator<char> >::value
            && !std::is_base_of<IAutoPtr, T2 >::value,
            T2 >::type >
    operator T&() const 
    {
        if( empty() )
        {
            std::string strMsg = DebugMsg(
                -EFAULT, "The object is empty" );
            throw std::invalid_argument( strMsg );
        }

        if( size() < sizeof( T2 ) )
            throw std::invalid_argument(
                "The type size does not match" );

        if( GetDataType() == DataTypeMem )
        {
            return *( reinterpret_cast< T* >( ptr() ) );
        }
        throw std::invalid_argument(
            "The type is not `mem' type" );
    }

    template<typename T, 
        typename T2 = typename std::enable_if<
            std::is_base_of< IAutoPtr, T >::value, T >::type,
        typename exclude_dmsg = class std::enable_if<
            !std::is_same< DMsgPtr, T >::value &&
            !std::is_same< ObjPtr, T >::value >::type,
        typename T4=T >
    operator T()
    {
        if( empty() )
        {
            std::string strMsg = DebugMsg(
                -EFAULT, "The object is empty" );
            throw std::invalid_argument( strMsg );
        }

        ObjPtr* ppObj = ( ObjPtr* )ptr();
        T oPtr( *ppObj );
        return oPtr;
    }

    template<typename T, 
        typename T1 = typename std::enable_if<
            !std::is_same<DBusMessage, T>::value, T >::type,
        typename T3 = class std::enable_if<
            std::is_base_of<CObjBase, T>::value, T >::type >
    operator T*()
    {

        // this is a version for CObjBase inherited objects
        if( empty() )
        {
            std::string strMsg = DebugMsg(
                -EFAULT, "The object is empty" );
            throw std::invalid_argument( strMsg );
        }
        switch( GetDataType() )
        {
        case DataTypeObjPtr:
            {
                // c++11 required
                ObjPtr& objPtr = *reinterpret_cast
                    < ObjPtr* >( ptr() );
            
                return dynamic_cast< T* >( ( CObjBase* )objPtr );
            }
        default:
            {
                throw std::invalid_argument(
                    "The data type is not supported" );
            }
        }
        return nullptr;
    }

    inline operator CBuffer*()
    {
        return this;
    }

    inline operator DBusMessage*() const
    {
        DMsgPtr& pmsg = *( DMsgPtr* )ptr();
        return pmsg;
    }

    template<typename T, 
        typename T1 = typename std::enable_if<
            !std::is_same<DBusMessage, T>::value, T >::type,
        typename T2 = typename std::enable_if<
            !std::is_base_of<CObjBase, T>::value, T >::type,
        typename T3 = typename std::decay< T2 >::type,
        typename T4=typename std::enable_if< 
            !std::is_same<T3, const char* >::value
            && !std::is_same<T3, char* >::value
            && !std::is_same<T3, char >::value
            && !std::is_same<T3, std::initializer_list<char> >::value
            && !std::is_same<T3, std::allocator<char> >::value,
            T2 >::type >
    operator T*() const
    {

        if( empty() )
        {
            std::string strMsg = DebugMsg(
                -EFAULT, "The object is empty" );
            throw std::invalid_argument( strMsg );
        }
        switch( GetDataType() )
        {
        case DataTypeMem:
            {
                // c++11 required
                static_assert(
                    !std::is_base_of< CObjBase, T >::value,
                    "objects inherited from CObjBase are not allowed" );

                return  reinterpret_cast< T* >( ptr() );
            }
        default:
            {
                throw std::invalid_argument(
                    "The data type is not supported" );
            }
        }
        return nullptr;
    }

    explicit operator char*() const;
    explicit operator const char*() const;

    template<typename T,
        typename T2 = typename std::enable_if<
            !std::is_base_of< IAutoPtr, T >::value &&
            !std::is_base_of< CObjBase, T >::value, T >::type >
    CBuffer& operator=( const T& rhs )
    {
        // for plain object only. If you want a
        // type-specific destructor to be called,
        // wrap it following stlcont.h
        //
        // it is not recommended to add new types
        // here
        //
        // c++11 required
        // use static assert here to allow the
        // specialization of the following types
        static_assert(
        !std::is_base_of< CObjBase, T >::value &&
        !std::is_same<T, DMsgPtr >::value &&
        !std::is_same<T, ObjPtr>::value,
        "The rhs must not inherit from CObjBase."  );

        SetDataType( DataTypeMem );
        Resize( sizeof( T ) );
        if( ptr() )
        {
            memcpy( ptr(), &rhs, sizeof( T ) );
            SetExDataType( ( EnumTypeId )GetTypeId(
                ( typename std::remove_reference<T>::type* )nullptr ) );
        }

        // well, you cannot return *this directly
        // because an implicit coversion will
        // happen and a temporary object is
        // created, which is not what we want
        // return ( T& )( *( T* )ptr() );
        return *this;
    }

    template<typename T, 
        typename T2 = typename std::enable_if<
            std::is_base_of< IAutoPtr, T >::value, T >::type,
        typename exclude_dmsg = class std::enable_if<
            !std::is_same< DMsgPtr, T >::value, T >::type,
        typename T4=T >
    CBuffer& operator=( const T& rhs )
    {
        ObjPtr pObj = rhs;
        *this = pObj;
        return *this;
    }

    template< typename T,
        typename only_dmsg = typename std::enable_if<
            std::is_same< T, DMsgPtr >::value, T >::type,
        typename pad1 = T,
        typename pad2 = T,
        typename pad3 = T >
    CBuffer& operator=( const T& rhs )
    {
        *this = rhs;
        return *this;
    }

    template<typename T, 
        typename T2 = typename std::enable_if<
            std::is_base_of< CObjBase, T >::value, T >::type,
        typename T3=T2 >
    CBuffer& operator=( const T& rhs )
    {
        ObjPtr pObj( ( const CObjBase* )&rhs );
        *this = pObj;
        return *this;
    }

    template<typename T> const T* operator=( const T* rhs )
    {
        return nullptr;
    }

    template<typename T,
        typename T2 = class std::enable_if<
            !std::is_same< DBusMessage, T >::value, T >::type,
        typename T3 = typename std::enable_if<
            std::is_base_of<CObjBase, T>::value, T >::type >
    T* operator=( T* rhs )
    {
        *this = ObjPtr( rhs );
        return rhs;
    }

    template<typename T,
        typename T2 = class std::enable_if<
            std::is_same< DBusMessage, T >::value, T >::type >
    T* operator=( T* rhs )
    {
        *this = DMsgPtr( rhs );
        return rhs;
    }

    // default copy assignment, we cannot replace it
    // with template version copy assignment, let's
    // override it.
    CBuffer& operator=( const CBuffer& rhs );

    template< typename T,
        typename FundamentalObj = typename std::enable_if<
            std::is_fundamental< T >::value, T >::type,
        typename T2 = typename std::enable_if<
            !std::is_base_of< CObjBase, T >::value, T >::type >
    gint32 Append( const T2& rhs )
    {
        if( GetDataType() != DataTypeMem )
            return -EINVAL;

        // plain object only
        // NOTE: c++11 required
        gint32 ret = 0;
        guint32 dwOldSize = size();
        Resize( size() + sizeof( rhs ) );
        memcpy( ptr() + dwOldSize, &rhs, sizeof( rhs ) );
        return ret;
    }

    template< typename T,
        typename ObjBaseObj = typename std::enable_if<
            std::is_base_of< CObjBase, T >::value, T >::type >
    gint32 Append( const ObjBaseObj& rhs )
    {
        if( GetDataType() != DataTypeObjPtr )
            return -EINVAL;

        // plain object only
        // NOTE: c++11 required
        gint32 ret = 0;
        // BufPtr bufPtr( true );

        CBuffer* pBuf = new CBuffer();
        ret = rhs.Serialize( pBuf );
        if( SUCCEEDED( ret ) )
        {
            guint32 dwOldSize = size();
            Resize( size() + pBuf->size() );

            memcpy( ptr() + dwOldSize,
                pBuf->ptr(), pBuf->size() );
        }

        if( pBuf )
            delete pBuf;

        return ret;
    }

    template< typename T,
        typename CopyObj = typename std::enable_if<
            std::is_copy_constructible< T >::value, T >::type,
        typename T2 = typename std::enable_if<
            !std::is_base_of< CObjBase, T >::value, T >::type >
    gint32 Append( const CopyObj& rhs )
    {
        if( GetDataType() != DataTypeMem )
            return -EINVAL;

        // plain object only
        // NOTE: c++11 required
        gint32 ret = 0;

        guint32 dwOldSize = size();
        Resize( size() + sizeof( rhs ) );
        CopyObj* pObj = reinterpret_cast< CopyObj* >( ptr() + dwOldSize );
        // construct the object
        new ( pObj ) CopyObj( rhs );

        return ret;
    }

    gint32 Compress( guint8* pDest, guint32& dwSize ) const;
    gint32 Decompress( guint8* pDest, guint32 dwOrigSize ) const;
};

template<> CBuffer& CBuffer::operator=<stdstr>( const stdstr& rhs );
template<> CBuffer& CBuffer::operator=<CObjBase>( const CObjBase& rhs );
// template<> CBuffer& CBuffer::operator=<CBuffer>( const CBuffer& rhs );
template<> CBuffer& CBuffer::operator=<ObjPtr>( const ObjPtr& rhs );
template<> CBuffer& CBuffer::operator=<DMsgPtr>( const DMsgPtr& rhs );
template<> cchar* CBuffer::operator=<char>( cchar* rhs );

template< class T,
    class allowed = typename std::enable_if<
    std::is_scalar< T >::value ||
    std::is_same< stdstr, T >::value ||
    std::is_base_of< IAutoPtr, T >::value,
    T >::type >
inline BufPtr GetDefault( T* pT )
{
    BufPtr pBuf( true );
    pBuf->Resize( sizeof( T ) );
    memset( pBuf->ptr(), 0, sizeof( T ) );
    return pBuf;
}

template<>
inline BufPtr GetDefault<EnumEventId>( EnumEventId* pT )
{
    BufPtr pBuf( true );
    *pBuf = eventInvalid;
    return pBuf;
}

template<>
inline BufPtr GetDefault<EnumClsid>( EnumClsid* pT )
{
    BufPtr pBuf( true );
    *pBuf = clsid( Invalid );
    return pBuf;
}

template<>
inline BufPtr GetDefault<EnumPropId>( EnumPropId* pT )
{
    BufPtr pBuf( true );
    *pBuf = propInvalid;
    return pBuf;
}

template<>
inline BufPtr GetDefault<bool>( bool* pT )
{
    BufPtr pBuf( true );
    *pBuf = false;
    return pBuf;
}

template<>
inline BufPtr GetDefault<char>( char* pT )
{
    BufPtr pBuf( true );
    *pBuf = ( char )0;
    return pBuf;
}

template<>
inline BufPtr GetDefault<guint8>( guint8* pT )
{
    BufPtr pBuf( true );
    *pBuf = ( guint8 )0;
    return pBuf;
}

template<>
inline BufPtr GetDefault<guint16>( guint16* pT )
{
    BufPtr pBuf( true );
    *pBuf = ( guint16 )0;
    return pBuf;
}

template<>
inline BufPtr GetDefault<guint32>( guint32* pT )
{
    BufPtr pBuf( true );
    *pBuf = ( guint32 )0;
    return pBuf;
}

template<>
inline BufPtr GetDefault<guint64>( guint64* pT )
{
    BufPtr pBuf( true );
    *pBuf = ( guint64 )0;
    return pBuf;
}

template<>
inline BufPtr GetDefault<float>( float* pT )
{
    BufPtr pBuf( true );
    *pBuf = ( float )0;
    return pBuf;
}

template<>
inline BufPtr GetDefault<double>( double* pT )
{
    BufPtr pBuf( true );
    *pBuf = ( double )0;
    return pBuf;
}

template<>
inline BufPtr GetDefault<stdstr>( stdstr* pT )
{
    BufPtr pBuf( true );
    *pBuf = std::string( "none" );
    return pBuf;
}

template<>
inline BufPtr GetDefault<DMsgPtr>( DMsgPtr* pT )
{
    BufPtr pBuf( true );
    pBuf->Resize( sizeof( DMsgPtr ) );
    memset( pBuf->ptr(), 0, sizeof( DMsgPtr ) );
    pBuf->SetDataType( DataTypeMsgPtr );
    pBuf->SetExDataType( typeDMsg );
    return pBuf;
}

template<>
inline BufPtr GetDefault<ObjPtr>( ObjPtr* pT )
{
    BufPtr pBuf( true );
    pBuf->Resize( sizeof( ObjPtr ) );
    memset( pBuf->ptr(), 0, sizeof( ObjPtr ) );
    pBuf->SetDataType( DataTypeObjPtr );
    pBuf->SetExDataType( typeObj );
    return pBuf;
}

template< class T,
    class allowed = typename std::enable_if<
    std::is_base_of< CObjBase*, T >::value,
    T >::type,
    class T2 = T >
inline BufPtr GetDefault( T* pT )
{
    BufPtr pBuf( true );
    pBuf->Resize( sizeof( ObjPtr ) );
    memset( pBuf->ptr(), 0, sizeof( ObjPtr ) );
    pBuf->SetDataType( DataTypeObjPtr );
    pBuf->SetExDataType( typeObj );
    return pBuf;
}

template<>
inline BufPtr GetDefault< IConfigDb* >( IConfigDb** pT )
{
    return GetDefault<ObjPtr>( ( ObjPtr* )nullptr );
}

}
