/*
 * =====================================================================================
 *
 *       Filename:  variant.h
 *
 *    Description:  Declaration of Variant and part of the member definitions
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
#pragma once
#include "autoptr.h"
#include "buffer.h"
namespace rpcf
{
struct Variant
{
    union
    {
        bool m_bVal;
        guint8 m_byVal;
        guint16 m_wVal;
        guint32 m_dwVal;
        guint64 m_qwVal;
        float   m_fVal;
        double  m_dblVal;
        ObjPtr  m_pObj;
        BufPtr  m_pBuf;
        DMsgPtr m_pMsg;
        std::string m_strVal;
        EnumEventId m_evtId;
        EnumClsid m_clsId;
        EnumPropId m_propId;
    };

    EnumTypeId m_iType = typeNone;
    guint32 m_dwReserved = 0;
    Variant();
    Variant( bool bVal );
    Variant( guint8 byVal );
    Variant( guint16 wVal );
    Variant( guint32 dwVal );
    Variant( guint64 qwVal );
    Variant( float fVal );
    Variant( double dblVal );
    Variant( const ObjPtr& pObj );
    Variant( const BufPtr& pBuf );
    Variant( const DMsgPtr& msgVal );
    Variant( const std::string& strVal );
    Variant( const char* szVal );
    Variant( const char* szVal, guint32 dwSize );
    Variant( const Variant& );
    Variant( Variant&& );
    Variant( const CBuffer& );
    Variant( gint8 );
    Variant( gint16 );
    Variant( gint32 );
    Variant( gint64 );
    Variant( EnumEventId );
    Variant( EnumClsid );
    Variant( EnumPropId );
    Variant( const uintptr_t* szVal );

    Variant& operator=( bool bVal );
    Variant& operator=( guint8 byVal );
    Variant& operator=( guint16 wVal );
    Variant& operator=( guint32 dwVal );
    Variant& operator=( guint64 qwVal );
    Variant& operator=( float fVal );
    Variant& operator=( double dblVal );
    Variant& operator=( const ObjPtr& pObj );
    Variant& operator=( const BufPtr& pBuf );
    Variant& operator=( const DMsgPtr& msgVal );
    Variant& operator=( const std::string& strVal );
    Variant& operator=( const char* pszVal );
    Variant& operator=( gint8 );
    Variant& operator=( gint16 );
    Variant& operator=( gint32 );
    Variant& operator=( gint64 );
    Variant& operator=( EnumEventId );
    Variant& operator=( EnumClsid );
    Variant& operator=( EnumPropId );
    Variant& operator=( const CBuffer& );
    Variant& operator=( const Variant& );
    void Clear();
    ~Variant();

    BufPtr ToBuf() const;

    inline bool empty() const
    { return m_iType == typeNone; }
    inline EnumTypeId GetTypeId() const
    { return m_iType; }

    operator const bool&() const
    { return m_bVal; }
    operator bool&()
    { return m_bVal; }

    operator const guint8& () const
    { return m_byVal; }
    operator guint8& ()
    { return m_byVal; }

    operator const guint16& () const
    { return m_wVal; }
    operator guint16& ()
    { return m_wVal; }

    operator const guint32& () const
    { return m_dwVal; }
    operator guint32& ()
    { return m_dwVal; }

    operator const guint64& () const
    { return m_qwVal; }
    operator guint64& ()
    { return m_qwVal; }

    operator const float&() const
    { return m_fVal; }
    operator float&()
    { return m_fVal; }

    operator const double& () const
    { return m_dblVal; }
    operator double& ()
    { return m_dblVal; }

    operator const ObjPtr&() const
    { return m_pObj; }
    operator ObjPtr&()
    { return m_pObj; }

    operator const DMsgPtr&() const
    { return m_pMsg; }
    operator DMsgPtr&()
    { return m_pMsg; }

    operator const BufPtr&() const
    { return m_pBuf; }
    operator BufPtr&()
    { return m_pBuf; }

    operator const stdstr&() const
    { return m_strVal; }
    operator stdstr&()
    { return m_strVal; }

    operator const char*() const
    { return m_strVal.c_str(); }

    operator const DMsgPtr*() const
    { return &m_pMsg; }
    operator DMsgPtr*()
    { return &m_pMsg; }

    operator const DBusMessage*() const
    { return ( DBusMessage* )m_pMsg; }
    operator DBusMessage*()
    { return ( DBusMessage* )m_pMsg; }

    operator const ObjPtr*() const
    { return &m_pObj; }
    operator ObjPtr*()
    { return &m_pObj; }

    operator const gint8&() const
    { return ( gint8& )m_byVal; }
    operator gint8&()
    { return ( gint8& )m_byVal; }

    operator const gint16&() const
    { return ( gint16& )m_wVal; }
    operator gint16&()
    { return ( gint16& )m_wVal; }

    operator const gint32&() const
    { return ( gint32& )m_dwVal; }
    operator gint32&()
    { return ( gint32& )m_dwVal; }

    operator const gint64&() const
    { return ( gint64& )m_qwVal; }
    operator gint64&()
    { return ( gint64& )m_qwVal; }

    operator const EnumEventId&() const
    { return m_evtId; }
    operator EnumEventId&()
    { return m_evtId; }

    operator const EnumClsid&() const
    { return m_clsId; }
    operator EnumClsid&()
    { return m_clsId; }

    operator const EnumPropId&() const
    { return m_propId; }
    operator EnumPropId&()
    { return m_propId; }

    bool operator==( const Variant& rhs ) const;
    bool operator!=( const Variant& rhs ) const
    { return !( *this == rhs ); }

    gint32 Serialize( BufPtr& pBuf, void* psb = nullptr ) const;
    gint32 Deserialize( BufPtr& pBuf, void* psb = nullptr );
} __attribute__((aligned (8)));

/*template< typename T,
    typename allowed = typename std::enable_if<
    !std::is_base_of< CObjBase, T >::value &&
    !std::is_base_of< IAutoPtr, T >::value, 
    T >::type >
inline Variant GetDefVar( T* )
{
    std::string strMsg = DebugMsg(
        -EFAULT, "we should not be here" );
    throw std::runtime_error( strMsg );
}*/

template< class T,
    class allowed = typename std::enable_if<
    std::is_scalar< T >::value ||
    std::is_same< stdstr, T >::value,
    T >::type >
inline Variant GetDefVar( T* )
{
    Variant o( ( T )0 );
    return o;
}

template<>
inline Variant GetDefVar<EnumEventId>( EnumEventId* )
{
    Variant o( eventInvalid );
    return o;
}

template<>
inline Variant GetDefVar<EnumClsid>( EnumClsid* )
{
    Variant o( ( guint32 ) clsid( Invalid ) );
    return o;
}

template<>
inline Variant GetDefVar<EnumPropId>( EnumPropId* )
{
    Variant o( ( guint32 )propInvalid );
    return o;
}

template<>
inline Variant GetDefVar<bool>( bool* )
{
    Variant o( false );
    return o;
}

template<>
inline Variant GetDefVar<char*>( char** )
{
    Variant o( stdstr("") );
    return o;
}

template<>
inline Variant GetDefVar<guint8>( guint8* )
{
    Variant o( ( guint8 )0  );
    return o;
}

template<>
inline Variant GetDefVar<guint16>( guint16* )
{
    Variant o( ( guint16 )0  );
    return o;
}

template<>
inline Variant GetDefVar<guint32>( guint32* )
{
    Variant o( ( guint32 )0  );
    return o;
}

template<>
inline Variant GetDefVar<guint64>( guint64* )
{
    Variant o( ( guint64 )0  );
    return o;
}

template<>
inline Variant GetDefVar<float>( float* )
{
    Variant o( ( float ).0  );
    return o;
}

template<>
inline Variant GetDefVar<double>( double* )
{
    Variant o( ( double ).0  );
    return o;
}

template< class T,
    class allowed = typename std::enable_if<
    std::is_base_of< CObjBase, T >::value ||
    std::is_base_of< IAutoPtr, T >::value,
    T >::type,
    class T2 = T >
inline Variant GetDefVar( T* )
{
    ObjPtr pObj;
    Variant o( pObj );
    return o;
}

template< class T,
    typename T1=typename std::enable_if<
        std::is_pointer<T>::value,
        std::remove_pointer<T> >::type,
    typename allowed = typename std::enable_if<
        std::is_base_of< CObjBase, T1 >::value ||
        std::is_base_of< IAutoPtr, T1 >::value,
        T >::type,
    class T2 = T >
inline Variant GetDefVar( T* )
{
    ObjPtr pObj;
    Variant o( pObj );
    return o;
}

template<>
inline Variant GetDefVar<stdstr>( stdstr* )
{
    Variant o( stdstr( "" )  );
    return o;
}

template<>
inline Variant GetDefVar<DMsgPtr>( DMsgPtr* )
{
    DMsgPtr pMsg;
    Variant o( pMsg );
    return o;
}

template<>
inline Variant GetDefVar<BufPtr>( BufPtr* )
{
    BufPtr pBuf;
    return Variant ( pBuf );
}

template<>
inline Variant GetDefVar<uintptr_t*>( uintptr_t** )
{ return Variant( ( uintptr_t )0 ); }

}
