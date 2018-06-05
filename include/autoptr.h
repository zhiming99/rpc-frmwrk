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
 * =====================================================================================
 */

#pragma once
#include "objfctry.h"
#include <dbus/dbus.h>

// note: class T must be a class inherited from CObjBase
// c++11 required
template< EnumClsid iClsid, class T >
//    typename T = typename std::enable_if< std::is_base_of<CObjBase, B>::value, B >::type >

class CAutoPtr
{
    private:

    T* m_pObj;

    public:

    CAutoPtr( bool bNew, IConfigDb* pCfg = nullptr )
    {
        m_pObj = nullptr;

        static_assert( iClsid != Clsid_Invalid );

        if( bNew )
        {
            CObjBase* pObj;
            gint32 ret = CoCreateInstance(
                iClsid, pObj, pCfg );

            if( ERROR( ret ) )
            {
                std::string strMsg = DebugMsg(
                    ret, std::string( "failed to create the object" ) );

                throw std::invalid_argument( strMsg );
            }
            m_pObj = static_cast< T* >( pObj );
        }
    }

    // NOTE: cannot convert between different object
    // classes
    CAutoPtr( T* pObj = nullptr, bool bAddRef = true )
    {
        static_assert( std::is_base_of< CObjBase, T >::value );
        m_pObj = nullptr;
        if( pObj != nullptr )
        {
            m_pObj = pObj;
            if( m_pObj && bAddRef )
            {
                m_pObj->AddRef();
            }
        }
    }

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
        static_assert( std::is_base_of< CObjBase, U >::value );
        return dynamic_cast< U* >( m_pObj );
    }

    template< class T1, class U = typename std::enable_if<
        std::is_base_of< T, T1 >::value ||
        std::is_base_of< T1, T >::value, T1 >::type >
    operator U&() const
    {
        // T1 must be either the base class of T or
        // super class of T
        static_assert( std::is_base_of< CObjBase, U >::value );
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

        static_assert( std::is_base_of<CObjBase, U>::value );

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

    inline bool IsEmpty() const
    {
        return ( m_pObj == nullptr );
    }

    inline void Clear()
    {
        if( m_pObj != nullptr )
        {
            m_pObj->Release();
            m_pObj = nullptr;
        }
    }

    gint32 NewObj(
        EnumClsid iNewClsid = Clsid_Invalid,
        const IConfigDb* pCfg = nullptr )
    {
        Clear();

        // with clsid invalid, we don't 
        // support object creation dynamically.

        EnumClsid iEffective = Clsid_Invalid;
        if( iClsid != Clsid_Invalid )
        {
            iEffective = iClsid;
        }
        else
        {
            if( iNewClsid != Clsid_Invalid )
                iEffective = iNewClsid;
        }

        if( iEffective == Clsid_Invalid )
            return 0;

        CObjBase* pObj = nullptr;

        gint32 ret = CoCreateInstance( iEffective, pObj, pCfg );

        if( ret == 0 )
        {
            m_pObj = dynamic_cast< T* >( pObj );
            if( m_pObj == nullptr )
            {
                pObj->Release();
                std::string strMsg =
                    DebugMsg( -EBADE, "bad cast happens" );

                printf( strMsg.c_str() );
                ret = -ENOTSUP;
            }
        }
        return ret;
    }
};

typedef CAutoPtr< clsid( Invalid ), CObjBase >         ObjPtr;
typedef CAutoPtr< clsid( CBuffer ), CBuffer >          BufPtr;
typedef CAutoPtr< clsid( Invalid ), IService >         ServicePtr;
typedef CAutoPtr< clsid( CConfigDb ), IConfigDb >      CfgPtr;
typedef CAutoPtr< clsid( Invalid ), IEventSink >       EventPtr;
typedef CAutoPtr< clsid( Invalid ), IThread >          ThreadPtr;
typedef CAutoPtr< clsid( Invalid ), DBusMessage >      DMsgPtr;
typedef CAutoPtr< clsid( Invalid ), IClassFactory >    FactoryPtr;


#include "dmsgptr.h"

namespace std {

    template<>
    struct less<ObjPtr>
    {
        bool operator()(const ObjPtr& k1, const ObjPtr& k2) const
            {
                return ( reinterpret_cast< unsigned long >( ( CObjBase* )k1  )
                        < reinterpret_cast< unsigned long >( ( CObjBase* )k2 ) );
            }
    };

    template<>
    struct less<EventPtr>
    {
        bool operator()(const EventPtr& k1, const EventPtr& k2) const
            {
              return ( reinterpret_cast< unsigned long >( ( IEventSink* )k1  )
                        < reinterpret_cast< unsigned long >( ( IEventSink* )k2 ) );
            }
    };

};
