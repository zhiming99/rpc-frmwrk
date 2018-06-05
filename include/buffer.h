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
 * =====================================================================================
 */
#pragma once

#include <map>
#include <string>
#include <vector>
#include <atomic>
#include <algorithm>
#include <glib.h>
#include <type_traits>
#include <string.h>
#include "defines.h"
#include "autoptr.h"
#include <dbus/dbus.h>

using cchar = const char;

#define DATATYPE_MASK       0x0FUL
#define DATATYPE_MASK_EX    0xF0UL
#define BUF_MAX_SIZE        ( 512 * 1024 * 1024 )

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
{ return 0; }

template<>
inline gint32 GetTypeId( bool* pT )
{ return typeByte; }

template<>
inline gint32 GetTypeId( guint8* pT )
{ return typeByte; }

template<>
inline gint32 GetTypeId( guint16* pT )
{ return typeUInt16; }

template<>
inline gint32 GetTypeId( guint32* pT )
{ return typeUInt32; }

template<>
inline gint32 GetTypeId( guint64* pT )
{ return typeUInt32; }

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
	protected:
	char* m_pData;
	guint32 m_dwSize;
    guint32 m_dwType;

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

	char*& ptr() const;
	guint32& size() const;
    guint32 type() const;
    void SetDataType( EnumDataType dt );
    EnumDataType GetDataType() const;
	void Resize( guint32 dwSize );

    void SetExDataType( EnumTypeId dt );
    EnumTypeId GetExDataType() const;

    bool operator==( const CBuffer& rhs ) const;
    gint32 Serialize( CBuffer& oBuf ) const;
    gint32 SerializePrimeType( guint8* pDest ) const;
    gint32 Deserialize( const CBuffer& oBuf );
    gint32 DeserializePrimeType( guint8* pSrc );
    gint32 Deserialize( const char* pBuf );
    gint32 Append( const guint8* pBlock, guint32 dwSize );

    inline bool empty() const
    {
        return ( size() == 0 || ptr() == nullptr );
    }

    template<typename T,
        typename T2= typename std::decay< T >::type,
        typename T3=typename std::enable_if<
            std::is_same<T2, guint32 >::value
            || std::is_same<T2, guint16 >::value
            || std::is_same<T2, gint16 >::value
            || std::is_same<T2, guint64>::value
            || std::is_same<T2, gint64>::value
            || std::is_same<T2, EnumEventId >::value
            || std::is_same<T2, double >::value
            || std::is_same<T2, float >::value
            || std::is_same<T2, int >::value
            || std::is_same<T2, unsigned int >::value
            || std::is_same<T, guint8 >::value
            || std::is_same<T, gint8 >::value,
             T2 >::type,
        typename forbidden=typename std::enable_if< 
            !std::is_same<T2, const char* >::value
            && !std::is_same<T2, std::initializer_list<char> >::value
            && !std::is_same<T2, std::allocator<char> >::value,
            T2 >::type >
    operator T&() const 
    {
        if( empty() )
        {
            throw std::invalid_argument(
                "the object is empty" );
        }

        if( GetDataType() == DataTypeMem )
        {
            return *( reinterpret_cast< T* >( ptr() ) );
        }
        throw std::invalid_argument(
            "The type is not `mem' type" );
    }


    // c++11 required
    /*template<typename T,
        typename T2=std::remove_cv<T>,
        typename T3=typename std::enable_if<
            !std::is_same<T2, stdstr>::value
            && !std::is_same<T2, CObjBase>::value
            && !std::is_same<T2, ObjPtr>::value
            && !std::is_same<T2, DMsgPtr >::value, T2 >::type >
    operator T&() const
    {
        if( empty() )
        {
            throw std::invalid_argument(
                "the object is empty" );
        }

        if( GetDataType() == DataTypeMem )
        {
            return *( reinterpret_cast< T* >( ptr() ) );
        }
        throw std::invalid_argument(
            "The type is not `mem' type" );
    }*/

    template<typename T, 
        typename T3 = class std::enable_if<
            std::is_base_of<CObjBase, T>::value, T >::type >
    operator T*()
    {

        // this is a version for CObjBase inherited objects
        if( empty() )
        {
            throw std::invalid_argument(
                "The object is empty" );
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

    template<typename T, 
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
            throw std::invalid_argument(
                "The object is empty" );
        }
        switch( GetDataType() )
        {
        case DataTypeMem:
            {
                // c++11 required
                // objects inherited from CObjBase are not allowed
                static_assert(
                    !std::is_base_of< CObjBase, T >::value );

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

    operator stdstr() const;
    operator DMsgPtr*() const;
    operator DMsgPtr&() const;
    operator ObjPtr&() const;
    operator ObjPtr*() const;


    template<typename T >
    const CBuffer& operator=( const T& rhs )
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
        !std::is_same<T, ObjPtr>::value );

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

    template<typename T, EnumClsid N,
        typename T2 = CAutoPtr< N, T >,
        typename exclude_dmsg = class std::enable_if<
            !std::is_same< DBusMessage, T >::value, T >::type >
    CBuffer& operator=( T2& rhs )
    {
        ObjPtr pObj = rhs;
        *this = pObj;
        return *this;
    }

    template< typename T,
        typename only_dmsg = typename std::enable_if<
            std::is_same< T, DMsgPtr >::value, T >::type >
    CBuffer& operator=( only_dmsg& rhs )
    {
        *this = rhs;
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
        BufPtr bufPtr( true );
        ret = rhs.Serialize( *bufPtr );
        if( SUCCEEDED( ret ) )
        {
            guint32 dwOldSize = size();
            Resize( size() + bufPtr->size() );

            memcpy( ptr() + dwOldSize,
                bufPtr->ptr(), bufPtr->size() );
        }

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
};

template<> const CBuffer& CBuffer::operator=<stdstr>( const stdstr& rhs );
template<> const CBuffer& CBuffer::operator=<CObjBase>( const CObjBase& rhs );
template<> const CBuffer& CBuffer::operator=<CBuffer>( const CBuffer& rhs );
template<> const CBuffer& CBuffer::operator=<ObjPtr>( const ObjPtr& rhs );
template<> const CBuffer& CBuffer::operator=<DMsgPtr>( const DMsgPtr& rhs );
template<> cchar* CBuffer::operator=<cchar>( cchar* rhs );

