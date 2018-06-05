/*
 * =====================================================================================
 *
 *       Filename:  stlcont.h
 *
 *    Description:  declaration of wrappers for stl container objects
 *
 *        Version:  1.0
 *        Created:  07/27/2016 08:59:08 PM
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
#include <deque>
#include <atomic>
#include <algorithm>
#include <set>
#include <glib.h>
#include "clsids.h"
#include "propids.h"
#include "autoptr.h"
#include "defines.h"
#include "dlfcn.h"

using stdstr = std::string;
using cchar = const char;

template< typename T >
class CStlQueue : public CObjBase
{
    protected:

    std::deque< T > m_vecElems;
    std::recursive_mutex   m_oLock;

    public:

    typedef CObjBase super;
    typedef typename std::deque< T >::iterator MyItr;
    typedef typename std::deque< T > MyType;

    CStlQueue()
    {;}

    gint32 Serialize(
        CBuffer& oBuf ) const
    { return -ENOTSUP; }

    gint32 Deserialize(
        const CBuffer& obuf )
    { return -ENOTSUP; }
    
    const std::deque< T >& operator()() const
    { return m_vecElems; }

    std::deque< T >& operator()()
    { return m_vecElems; }

    std::recursive_mutex& GetLock() const
    {  return ( std::recursive_mutex& )m_oLock; }
};

class CStlIntQueue : public CStlQueue< guint32 >
{

    public:
    typedef CStlQueue< guint32 > super;
    CStlIntQueue()
        :CStlQueue<guint32>()
    {
        SetClassId( clsid( CStlIntQueue ) );
    }
};

typedef CAutoPtr< clsid( CStlIntQueue ), CStlIntQueue > IntQuePtr;

template< typename T >
class CStlVector : public CObjBase
{
    protected:

    std::vector< T > m_vecElems;
    std::recursive_mutex   m_oLock;

    public:

    typedef CObjBase super;
    typedef typename std::vector< T >::iterator MyItr;
    typedef typename std::vector< T > MyType;
    typedef T  ElemType;

    struct SERI_HEADER : public SERI_HEADER_BASE
    {
        typedef SERI_HEADER_BASE super;
        guint32 dwCount;
        ElemType arrElems[ 0 ];

        SERI_HEADER()
        {
            dwCount = 0; 
        }

        SERI_HEADER( const SERI_HEADER& oSrc )
            : super( oSrc )
        {
            dwCount = oSrc.dwCount;
        }

        SERI_HEADER& operator=(
            const SERI_HEADER& oSrc )
        {
            super::operator=( oSrc );
            dwCount = oSrc.dwCount;
            return *this;
        }

        void ntoh()
        {
            super::ntoh();
            dwCount = ntohl( dwCount );
        }

        void hton()
        {
            super::hton();
            dwCount = htonl( dwCount );
        }
    };

    CStlVector()
    {;}

    gint32 Serialize(
        CBuffer& oBuf ) const
    { return -ENOTSUP; }

    gint32 Deserialize(
        const CBuffer& obuf )
    { return -ENOTSUP; }
    
    const std::vector< T >& operator()() const
    { return m_vecElems; }

    std::vector< T >& operator()()
    { return m_vecElems; }

    std::recursive_mutex& GetLock() const
    {  return ( std::recursive_mutex& )m_oLock; }
};

class CStlIntVector : public CStlVector< guint32 >
{
    public:

    typedef CStlVector< guint32 > super;
    CStlIntVector()
        :CStlVector<guint32>()
    {
        SetClassId( clsid( CStlIntVector ) );
    }

    gint32 Serialize(
        CBuffer& oBuf ) const;

    gint32 Deserialize(
        const CBuffer& obuf );
};

typedef CAutoPtr< clsid( CStlIntVector ), CStlIntVector > IntVecPtr;

class CStlBufVector : public CStlVector< BufPtr >
{

    public:
    typedef CStlVector<BufPtr> super;
    CStlBufVector()
        :CStlVector<BufPtr>()
    {
        SetClassId( clsid( CStlBufVector  ) );
    }
};

typedef CAutoPtr< clsid( CStlBufVector ), CStlBufVector > BufVecPtr;

typedef std::pair< void*, FactoryPtr > ELEM_CLASSFACTORIES;

struct CClassFactories: public CStlVector< ELEM_CLASSFACTORIES >
{
    public:
    CClassFactories()
        :CStlVector<ELEM_CLASSFACTORIES>()
    {
        SetClassId( clsid( CClassFactories  ) );
    }

    ~CClassFactories()
    {
        Clear();
    }

    /**
    * @name CreateInstance similiar to
    * CoCreateInstance, except the name.
    * @{ */
    /**  @} */
    
    gint32 CreateInstance( 
        EnumClsid clsid,
        CObjBase*& pObj,
        const IConfigDb* pCfg )
    {
        gint32 ret = -ENOTSUP;

        CStdRMutex oLock( GetLock() );
        MyType& vecFactories = ( *this )();
        MyItr itr = vecFactories.begin();
        while( itr != vecFactories.end() )
        {
            ret = ( itr->second )->CreateInstance(
                    clsid, pObj, pCfg );

            if( SUCCEEDED( ret ) )
                break;

            ++itr;
        }
        return ret;

    }
    
    const char* GetClassName(
        EnumClsid iClsid )
    {
        CStdRMutex oLock( GetLock() );

        const char* pszName = nullptr;
        MyType& vecFactories = ( *this )();
        MyItr itr = vecFactories.begin();

        while( itr != vecFactories.end() )
        {
            pszName = ( itr->second )->GetClassName( iClsid );
            if( pszName != nullptr )
                return pszName;

            ++itr;
        }
        return nullptr;
    }

    EnumClsid GetClassId(
        const char* pszClassName )
    {
        if( pszClassName == nullptr )
            return clsid( Invalid );

        CStdRMutex oLock( GetLock() );
        EnumClsid iClsid = clsid( Invalid );
        MyType& vecFactories = ( *this )();
        MyItr itr = vecFactories.begin();

        while( itr != vecFactories.end() )
        {
            iClsid = ( itr->second )->GetClassId( pszClassName );
            if( iClsid != clsid( Invalid ) )
                return iClsid;

            ++itr;
        }
        return clsid( Invalid );
    }

    gint32 AddFactory(
        FactoryPtr& pFactory, void* hDll )
    {
        if( pFactory.IsEmpty() )
            return -EINVAL;

        if( hDll == nullptr )
        {
            // that's fine, indicating this
            // factory is from current module
        }

        CStdRMutex oLock( GetLock() );
        MyType& vecFactories = ( *this) ();

        vecFactories.push_back(
            ELEM_CLASSFACTORIES( hDll, pFactory ) );

        return 0;
    }

    gint32 RemoveFactory(
        FactoryPtr& pFactory )
    {

        CStdRMutex oLock( GetLock() );
        MyType& vecFactories = ( *this) ();
        MyItr itr = vecFactories.begin();

        while( itr != vecFactories.end() )
        {
            if( itr->second->GetObjId()
                == pFactory->GetObjId() )
            {
                if( itr->first != nullptr )
                    dlclose( itr->first );
                vecFactories.erase( itr );
                return 0;
            }

            ++itr;
        }
        return -ENOENT;

    }

    void Clear()
    {
        CStdRMutex oLock( GetLock() );
        MyType& vecFactories = ( *this) ();
        MyItr itr = vecFactories.begin();

        while( itr != vecFactories.end() )
        {
            if( itr->first != nullptr )
            {
                dlclose( itr->first );
                itr->first = nullptr;
            }
            ++itr;
        }
        vecFactories.clear();
    }

};
typedef CAutoPtr< clsid( CClassFactories ), CClassFactories > FctryVecPtr;

template< typename Key_, typename Val_ >
class CStlMap : public CObjBase
{
    protected:

    std::map< Key_, Val_ > m_mapElems;
    std::recursive_mutex   m_oLock;

    public:

    typedef CObjBase super;

    typedef typename std::map< Key_, Val_ >::iterator MyItr;
    typedef typename std::pair< Key_, Val_ > MyPair;
    typedef typename std::map< Key_, Val_ > MyMap;
    typedef MyPair ElemType;
    typedef Key_ KeyType;
    typedef Val_ ValueType;

    CStlMap()
    {;}

    // a functor to return the container
    std::map< Key_, Val_ >& operator()() const
    {
        return ( std::map< Key_, Val_ >& )m_mapElems;
    }

    struct SERI_HEADER : public SERI_HEADER_BASE
    {
        typedef SERI_HEADER_BASE super;
        guint32 dwCount;
        ElemType arrElems[ 0 ];

        SERI_HEADER()
        {
            dwCount = 0; 
        }

        SERI_HEADER( const SERI_HEADER& oSrc )
            : super( oSrc )
        {
            dwCount = oSrc.dwCount;
        }

        SERI_HEADER& operator=(
            const SERI_HEADER& oSrc )
        {
            super::operator=( oSrc );
            dwCount = oSrc.dwCount;
            return *this;
        }

        void ntoh()
        {
            super::ntoh();
            dwCount = ntohl( dwCount );
        }

        void hton()
        {
            super::hton();
            dwCount = htonl( dwCount );
        }
    };

    gint32 Serialize(
        CBuffer& oBuf ) const
    { return -ENOTSUP; }

    gint32 Deserialize(
        const CBuffer& obuf )
    { return -ENOTSUP; }

    std::recursive_mutex& GetLock()
    {  return m_oLock; }
};

class CStlIntMap
    : public CStlMap< guint32, guint32 > 
{
 
    public:
    typedef CStlMap< guint32, guint32 > super;

    CStlIntMap()
        : CStlMap< guint32, guint32>()
    {
        SetClassId( clsid( CStlIntMap ) );
    }
    gint32 Serialize(
        CBuffer& oBuf ) const;

    gint32 Deserialize(
        const CBuffer& obuf );

};

typedef CAutoPtr< clsid( CStlIntMap ), CStlIntMap > IntMapPtr;

class CStlEventMap:
    public CStlMap< EventPtr, gint32 > 
{
    public:

    typedef CStlMap< EventPtr, gint32 > super;

    CStlEventMap()
        : CStlMap< EventPtr, gint32 >()
    {
        SetClassId( clsid( CStlEventMap ) );
    }

    gint32 SubscribeEvent(
        EventPtr& pEventSink ) 
    {

        CStdRMutex oLock( GetLock() );
        MyMap& oMap = m_mapElems;
        if( oMap.find( pEventSink ) == oMap.end() )
        {
            oMap[ pEventSink ] = 1;
        }
        else
        {
            ++oMap[ pEventSink ];
        }
        return 0;
    }

    gint32 UnsubscribeEvent(
        EventPtr& pEventSink )
    {

        gint32 ret = -ENOENT;

        CStdRMutex oLock( GetLock() );

        MyMap& oMap = m_mapElems;
        if( oMap.find( pEventSink ) != oMap.end() )
        {
            ret = --oMap[ pEventSink ];

            if( ret <= 0 )
                oMap.erase( pEventSink );

            ret = 0;
        }

        return ret;
    }

    gint32 BroadcastEvent(
        EnumEventId iEvent,
        guint32 dwParam1,
        guint32 dwParam2,
        guint32* pData )
    {
        MyMap& oMap = m_mapElems;
        std::vector<EventPtr> vecEvents;

        if( iEvent != eventInvalid )
        {
            CStdRMutex oLock( GetLock() );
            MyItr itr = oMap.begin();
            while( itr != oMap.end() )
            {
                vecEvents.push_back( itr->first );
                ++itr;
            }
        }

        for( guint32 i = 0; i < vecEvents.size(); i++ )
        {
            vecEvents[ i ]->OnEvent(
                iEvent, dwParam1, dwParam2, pData );
        }

        return 0;
    }
};

typedef CAutoPtr< clsid( CStlEventMap ), CStlEventMap > EvtMapPtr;

template< typename Val_ >
class CStlSet : public CObjBase
{
    protected:

    std::set< Val_ > m_setElems;
    stdrmutex m_oLock;

    public:
    typedef CObjBase super;
    typedef typename std::set< Val_ >::iterator MyItr;
    typedef typename std::set< Val_ > MySet;


    CStlSet()
    {;}

    ~CStlSet()
    { m_setElems.clear(); }

    // a functor to return the container
    std::set< Val_ >& operator()() const
    { return ( std::set< Val_ >& )m_setElems; }

    gint32 Serialize(
        CBuffer& oBuf ) const
    { return -ENOTSUP; }

    gint32 Deserialize(
        const CBuffer& obuf )
    { return -ENOTSUP; }

    std::recursive_mutex& GetLock()
    {  return m_oLock; }
};

class CStlObjSet : public CStlSet< ObjPtr >
{
public:
    typedef ObjPtr ElemType;

    CStlObjSet() :CStlSet< ObjPtr >()
    { SetClassId( clsid( CStlObjSet ) ); }

    ~CStlObjSet()
    {;}
};

typedef CAutoPtr< clsid( CStlObjSet ), CStlObjSet > ObjSetPtr;
