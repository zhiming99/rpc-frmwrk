/*
 * =====================================================================================
 *
 *       Filename:  autoptr.h
 *
 *    Description:  declaration of autoptr classes
 *
 *        Version:  1.0
 *        Created:  07/04/2016 08:40:49 PM
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
#include <dbus/dbus.h>
#include "defines.h"


namespace rpcf
{

extern gint32 CoCreateInstance( EnumClsid clsid,
    CObjBase*& pObj, const IConfigDb* pCfg = nullptr ); 

extern gint32 CreateObjFast( EnumClsid iClsid,
    CObjBase*& pObj, const IConfigDb* pCfg = nullptr );

struct IAutoPtr{};
// typedef DBusMessage* DMSGPTR;

// note: class T must be a class inherited from CObjBase
// c++11 required
template< EnumClsid iClsid, class T >
    // typename TFilter = typename std::enable_if<
    //     std::is_base_of<CObjBase, T>::value ||
    //     std::is_same<DMSGPTR, T>::value,
    //       T>::type >
class CAutoPtr : public IAutoPtr
{
    private:

    T* m_pObj;

    public:

    CAutoPtr( bool bNew, IConfigDb* pCfg = nullptr )
    {
        m_pObj = nullptr;

        static_assert( iClsid != Clsid_Invalid,
            "class id cannot be invalid" );

        if( bNew )
        {
            gint32 ret = NewObj( iClsid, pCfg );
            if( ERROR( ret ) )
            {
                std::string strMsg = DebugMsg(
                    ret, std::string( "failed to create the object" ) );

                throw std::invalid_argument( strMsg );
            }
        }
    }

    // NOTE: cannot convert between different object
    // classes
    CAutoPtr( T* pObj = nullptr, bool bAddRef = true )
    {
        static_assert( std::is_base_of< CObjBase, T >::value,
            "CObjBase must be the super class"  );

        if( pObj != nullptr )
        {
            m_pObj = pObj;
            if( m_pObj && bAddRef )
            {
                m_pObj->AddRef();
            }
        }
        else
        {
            m_pObj = nullptr;
        }
    }

    // copy constructor
    template< EnumClsid iClsid2, class U,
        class T1 = typename std::enable_if<
        std::is_base_of< T, U >::value ||
        std::is_base_of< U, T >::value, U >::type >
    CAutoPtr( const CAutoPtr< iClsid2, U >& oInitPtr )
    {
        m_pObj = dynamic_cast< T* >( ( U* )oInitPtr );
        if( m_pObj )
        {
            m_pObj->AddRef();
        }
        else
        {
            // we allow empty initialization
        }
    }

    // defult copy constructor 
    CAutoPtr( const CAutoPtr& oInitPtr ) 
    {
        m_pObj = oInitPtr.m_pObj;
        if( m_pObj )
        {
            m_pObj->AddRef();
        }
    }

    CAutoPtr( CAutoPtr&& oInitPtr ) 
    {
        m_pObj = oInitPtr.m_pObj;
        if( m_pObj )
            oInitPtr.Detach();
    }

    // move constructor
    template< EnumClsid iClsid2, class U,
        class T1 = typename std::enable_if<
        std::is_base_of< T, U >::value ||
        std::is_base_of< U, T >::value, U >::type >
    CAutoPtr( CAutoPtr< iClsid2, U >&& oInitPtr )
    {
        m_pObj = dynamic_cast< T* >( ( U* )oInitPtr );
        if( m_pObj )
            oInitPtr.Detach();
    }

    ~CAutoPtr()
    {
        Clear();
    }

    T& operator*() const noexcept
    {
        return *m_pObj;
    }

    T* operator->() const noexcept
    {
        return m_pObj;
    }

    template< class U, class T1 = typename std::enable_if<
        std::is_base_of< T, U >::value ||
        std::is_base_of< U, T >::value, U >::type >
    operator U*() const
    {
        // U must be either the base class of T or
        // super class of T
        static_assert( std::is_base_of< CObjBase, U >::value,
            "CObjBase must be the base class of the type to cast" );
        return dynamic_cast< U* >( m_pObj );
    }

    template< class T1, class U = typename std::enable_if<
        std::is_base_of< T, T1 >::value ||
        std::is_base_of< T1, T >::value, T1 >::type >
    operator U&() const
    {
        // T1 must be either the base class of T or
        // super class of T
        static_assert( std::is_base_of< CObjBase, U >::value,
            "CObjBase must be the base class of the type to cast" );
        U* ptr = dynamic_cast< U* >( m_pObj );
        if( ptr == nullptr )
        {
            std::string strMsg = DebugMsg(
                -EINVAL, std::string( "failed to cast to new type" ) );
            throw std::invalid_argument( strMsg );
        }
        return *ptr;
    }

    T* operator=( T* rhs ) 
    {
        if( m_pObj == rhs )
            return m_pObj;

        Clear();
        if( rhs != nullptr )
        {
            m_pObj = rhs;
            m_pObj->AddRef();
        }
        return m_pObj;
    }

    template< EnumClsid iClsid2, typename U,
        class T2 = typename std::enable_if<
            std::is_base_of< T, U >::value ||
            std::is_base_of< U, T >::value, U >::type >
    CAutoPtr< iClsid, T >& operator=( const CAutoPtr< iClsid2, U >& rhs )
    {

        static_assert( std::is_base_of<CObjBase, U>::value,
            "CObjBase must be the base class of the type to cast" );

        if( rhs.IsEmpty() )
        {
            Clear();
            return *this;
        }

        if( m_pObj != nullptr
            && m_pObj->GetObjId() == rhs->GetObjId() )
            return *this;

        Clear();

        const T* pObj = dynamic_cast< const T* >( ( U* )rhs );
        if( pObj != nullptr )
        {
            // well, the data content is not changed,
            // and the change in ref count is not that
            // much concerned
            m_pObj = const_cast< T* >( pObj );
            m_pObj->AddRef();
            return *this;
        }

        std::string strMsg = DebugMsg(
            -EFAULT, "bad cast in operator=" );

        throw std::runtime_error( strMsg );
    }

    CAutoPtr& operator=( const CAutoPtr& rhs )
    {
        if( rhs.IsEmpty() )
        {
            Clear();
            return *this;
        }
        // self assignment
        if( m_pObj != nullptr
            && m_pObj->GetObjId() == rhs->GetObjId() )
        {
            return *this;
        }

        Clear();
        const T* pObj = rhs.m_pObj;
        if( pObj != nullptr )
        {
            // well, the data content is not changed,
            // and the change in ref count is not that
            // much concerned
            m_pObj = const_cast< T* >( pObj );
            m_pObj->AddRef();
            return *this;
        }
        std::string strMsg = DebugMsg(
            -EFAULT, "bad cast in operator=" );
        throw std::runtime_error( strMsg );
    }

    template< EnumClsid iClsid2, typename U,
        class T2 = typename std::enable_if<
            std::is_base_of< T, U >::value ||
            std::is_base_of< U, T >::value, U >::type >
    CAutoPtr< iClsid, T >& operator=( CAutoPtr< iClsid2, U >&& rhs )
    {
        static_assert( std::is_base_of<CObjBase, U>::value,
            "CObjBase must be the base class of the type to cast" );

        if( rhs.IsEmpty() )
        {
            Clear();
            return *this;
        }

        // self assignment
        if( m_pObj != nullptr
            && m_pObj->GetObjId() == rhs->GetObjId() )
        {
            rhs.Clear();
            return *this;
        }

        Clear();
        const T* pObj = dynamic_cast< const T* >( ( U* )rhs );
        if( pObj != nullptr )
        {
            // well, the data content is not changed,
            // and the change in ref count is not that
            // much concerned
            m_pObj = const_cast< T* >( pObj );
            rhs.Detach();
            return *this;
        }
        std::string strMsg = DebugMsg(
            -EFAULT, "bad cast in operator=" );
        throw std::runtime_error( strMsg );
    }


    bool operator==( const CAutoPtr& rhs ) const noexcept
    {
        if(rhs.IsEmpty() )
        {
            if( IsEmpty() )
                return true;

            return false;
        }
        else if( IsEmpty() )
        {
            return false;
        }
        guint64 qwId = rhs.m_pObj->GetObjId();
        return ( m_pObj->GetObjId() == qwId );
    }

    bool operator!=( const CAutoPtr& rhs ) const noexcept
    { return !operator==( rhs ); }

    bool operator==( const T* rhs ) const noexcept
    {
        if(rhs == nullptr )
        {
            if( IsEmpty() )
                return true;

            return false;
        }
        else if( IsEmpty() )
        {
            return false;
        }

        guint64 qwId = rhs.m_pObj->GetObjId();
        return ( m_pObj->GetObjId() == qwId );
    }

    bool operator!=( const T* rhs ) const noexcept
    { return !operator==( rhs ); }

    inline bool IsEmpty() const
    {
        return ( m_pObj == nullptr );
    }

    inline void Detach()
    { m_pObj = nullptr; }

    inline void Clear()
    {
        if( m_pObj != nullptr )
        {
            m_pObj->Release();
            Detach();
        }
    }

    gint32 NewObj(
        EnumClsid iNewClsid = Clsid_Invalid,
        const IConfigDb* pCfg = nullptr )
    {
        Clear();

        // if clsid is Clsid_Invalid, the object cannot
        // be created dynamically.

        EnumClsid iEffective = clsid( Invalid );
        if( iNewClsid != clsid( Invalid ) )
        {
            iEffective = iNewClsid;
        }
        else if( iClsid != clsid( Invalid ) )
        {
            iEffective = iClsid;
        }

        if( iEffective == clsid( Invalid ) )
            return 0;

        CObjBase* pObj = nullptr;

        gint32 ret = CreateObjFast(
            iEffective, pObj, pCfg );

        if( SUCCEEDED( ret ) )
        {
            m_pObj = dynamic_cast< T* >( pObj );
            if( m_pObj == nullptr )
            {
                pObj->Release();
                ret = -EFAULT;
            }
            return ret;
        }
 
        ret = CoCreateInstance( iEffective, pObj, pCfg );

        if( SUCCEEDED( ret ) )
        {
            m_pObj = dynamic_cast< T* >( pObj );
            if( m_pObj == nullptr )
            {
                pObj->Release();
                std::string strMsg =
                    DebugMsg( -EBADE, "bad cast happens" );

                printf( "%s", strMsg.c_str() );
                ret = -ENOTSUP;
            }
        }
        return ret;
    }

    gint32 Deserialize( const CBuffer& oBuf )
    {
        const SERI_HEADER_BASE* pHeader = oBuf;

        SERI_HEADER_BASE oHeader( *pHeader );
        oHeader.ntoh();
        if( oHeader.dwClsid != iClsid )
            return -ENOTSUP;
        
        Clear();
        gint32 ret = NewObj( iClsid );
        if( ERROR( ret ) )
            return ret;

        return m_pObj->Deserialize( oBuf );
    }
};

typedef CAutoPtr< clsid( Invalid ), CObjBase >         ObjPtr;
typedef CAutoPtr< clsid( CBuffer ), CBuffer >          BufPtr;
typedef CAutoPtr< clsid( Invalid ), IService >         ServicePtr;
typedef CAutoPtr< clsid( CConfigDb2 ), IConfigDb >     CfgPtr;
typedef CAutoPtr< clsid( Invalid ), IEventSink >       EventPtr;
typedef CAutoPtr< clsid( Invalid ), IThread >          ThreadPtr;
// typedef CAutoPtr< clsid( Invalid ), DBusMessage >      DMsgPtr;

}

#include "dmsgptr.h"

namespace std
{
    using namespace rpcf;
    template<>
    struct less<ObjPtr>
    {
        bool operator()(const ObjPtr& k1, const ObjPtr& k2) const
        {
            if( k2.IsEmpty() )
                return false;

            if( k1.IsEmpty() )
                return true;

            return k1->GetObjId() < k2->GetObjId();
        }
    };

    template<>
    struct less<EventPtr>
    {
        bool operator()(const EventPtr& k1, const EventPtr& k2) const
        {
            if( k2.IsEmpty() )
                return false;

            if( k1.IsEmpty() )
                return true;

            return k1->GetObjId() < k2->GetObjId();
        }
    };

};

namespace rpcf
{
// c++11 required
template <typename R, typename C, typename ... Types>
inline constexpr size_t GetArgCount( R(C::*f)(Types ...) )
{
   return sizeof...(Types);
};

extern gint32 DeserializeObj(
    const CBuffer& oBuf, ObjPtr& pObj );

}
