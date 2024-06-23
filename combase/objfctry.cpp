/*
 * =====================================================================================
 *
 *       Filename:  objfctry.cpp
 *
 *    Description:  implementation of CClassFactory class
 *
 *        Version:  1.0
 *        Created:  01/13/2018 01:51:09 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi(woodhead99@gmail.com )
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

#include <map>
#include <vector>
#include "defines.h"
#include "stlcont.h"
#include "objfctry.h"

namespace rpcf
{

typedef gint32 (*PUNLOADLIBRARY )();


gint32 UnloadLibrary( void* hDll )
{
    if( hDll == nullptr )
        return -EINVAL;

    PUNLOADLIBRARY pFunc =
        ( PUNLOADLIBRARY  )dlsym( hDll, "DllUnload" );

    if( pFunc == nullptr )
        return -ENOENT;

    gint32 ret = ( *pFunc )();
    dlclose( hDll );

    return ret;
}

gint32 CClassFactory::CreateInstance( 
    EnumClsid iClsid,
    CObjBase*& pObj,
    const IConfigDb* pCfg ) const
{
    gint32 ret = 0;

    auto itr = m_oMapObjMakers.find( iClsid );
    if( itr != m_oMapObjMakers.end() )
    {
        ret = 0;
        pObj = ( *itr->second )( pCfg );
        if( pObj != nullptr )
        {
            pObj->AddRef();
        }
        else
        {
            ret = -EFAULT;
        }
    }
    else
    {
        ret = -ENOENT;
    }

    return ret;
}

const char* CClassFactory::GetClassName(
    EnumClsid iClsid ) const
{
    ID2NAME_MAP::const_iterator itr = 
        m_oMapId2Name.find( iClsid );

    if( itr == m_oMapId2Name.end() )
        return nullptr;

    return itr->second;
}

EnumClsid CClassFactory::GetClassId(
    const char* szClassName ) const
{
    if( szClassName == nullptr )
        return clsid( Invalid );

    NAME2ID_MAP::const_iterator itr = 
        m_oMapName2Id.find( szClassName );

    if( itr == m_oMapName2Id.end() )
        return clsid( Invalid );

    return itr->second;
}

void CClassFactory::EnumClassIds(
    std::vector< EnumClsid >& vecClsIds ) const
{
    for( auto elem : m_oMapId2Name )
    {
        vecClsIds.push_back( elem.first );
    }
    return;
}

gint32 CClassFactory::AppendFactory(
    CClassFactory& rhs )
{
    if( !rhs.m_oMapId2Name.empty() )
        m_oMapId2Name.insert(
            rhs.m_oMapId2Name.begin(),
            rhs.m_oMapId2Name.end() );
    if( !rhs.m_oMapName2Id.empty() )
        m_oMapName2Id.insert(
            rhs.m_oMapName2Id.begin(),
            rhs.m_oMapName2Id.end() );
    if( !rhs.m_oMapObjMakers.empty() )
    {
        m_oMapObjMakers.insert(
            rhs.m_oMapObjMakers.begin(),
            rhs.m_oMapObjMakers.end() );
        rhs.m_oMapObjMakers.clear();
    }
    return 0;
}

CClassFactories::CClassFactories()
    :CStlVector<ELEM_CLASSFACTORIES>()
{
    SetClassId( clsid( CClassFactories  ) );
}

CClassFactories::~CClassFactories()
{
    Clear();
}

/**
* @name CreateInstance similiar to
* CoCreateInstance, except the name.
* @{ */
/**  @} */

gint32 CClassFactories::CreateInstance( 
    EnumClsid clsid,
    CObjBase*& pObj,
    const IConfigDb* pCfg ) const
{
    gint32 ret = -ENOTSUP;

    // CStdRMutex oLock( GetLock() );
    // we are exposed to concurrent condition
    auto& vecFactories = ( *this )();
    auto itr = vecFactories.begin();
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

const char* CClassFactories::GetClassName(
    EnumClsid iClsid ) const
{
    CStdRMutex oLock( GetLock() );

    const char* pszName = nullptr;
    auto& vecFactories = ( *this )();
    auto itr = vecFactories.begin();

    while( itr != vecFactories.end() )
    {
        pszName = ( itr->second )->GetClassName( iClsid );
        if( pszName != nullptr )
            return pszName;

        ++itr;
    }
    return nullptr;
}

EnumClsid CClassFactories::GetClassId(
    const char* pszClassName ) const
{
    if( pszClassName == nullptr )
        return clsid( Invalid );

    CStdRMutex oLock( GetLock() );
    EnumClsid iClsid = clsid( Invalid );
    auto& vecFactories = ( *this )();
    auto itr = vecFactories.begin();

    while( itr != vecFactories.end() )
    {
        iClsid = ( itr->second )->GetClassId( pszClassName );
        if( iClsid != clsid( Invalid ) )
            return iClsid;

        ++itr;
    }
    return clsid( Invalid );
}

gint32 CClassFactories::AddFactory(
    const FactoryPtr& pFactory, void* hDll )
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

gint32 CClassFactories::RemoveFactory(
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
                UnloadLibrary( itr->first );

            vecFactories.erase( itr );
            return 0;
        }

        ++itr;
    }
    return -ENOENT;

}

void CClassFactories::Clear()
{
    CStdRMutex oLock( GetLock() );
    MyType& vecFactories = ( *this) ();
    MyItr itr = vecFactories.begin();

    while( itr != vecFactories.end() )
    {
        if( !itr->second.IsEmpty() )
        {
            itr->second.Clear();
        }
        if( itr->first != nullptr )
        {
            UnloadLibrary( itr->first );
            itr->first = nullptr;
        }
        ++itr;
    }
    vecFactories.clear();
    m_mapPathToHandle.clear();
}

void CClassFactories::EnumClassIds(
    std::vector< EnumClsid >& vecClsIds ) const
{
    CStdRMutex oLock( GetLock() );
    auto& vecFactories = ( *this) ();
    auto itr = vecFactories.begin();

    while( itr != vecFactories.end() )
    {
        std::vector< EnumClsid > vecIds;
        if( !itr->second.IsEmpty() )
        {
            itr->second->EnumClassIds( vecIds );
            vecClsIds.insert( vecClsIds.end(),
                vecIds.begin(), vecIds.end() );
        }
        ++itr;
    }
}

gint32 CClassFactories::IsDllLoaded(
    const char* pszPath )
{
    if( pszPath == nullptr )
        return -EINVAL;

    CStdRMutex oLock( GetLock() );
    if( m_mapPathToHandle.find( pszPath ) ==
        m_mapPathToHandle.end() )
        return ERROR_FALSE;

    return STATUS_SUCCESS;
}

gint32 CClassFactories::AddFactoryPath(
    const char* pszPath, void* hDll )
{
    if( pszPath == nullptr )
        return -EINVAL;

    CStdRMutex oLock( GetLock() );
    if( m_mapPathToHandle.find( pszPath ) !=
        m_mapPathToHandle.end() )
        return -EEXIST;

    m_mapPathToHandle[ pszPath ] = hDll;
    return STATUS_SUCCESS;
}

gint32 CClassFactories::RemoveFactoryPath(
    void* hDll )
{
    CStdRMutex oLock( GetLock() );
    std::map< std::string, void* >::iterator
       itr = m_mapPathToHandle.begin(); 
    while( itr != m_mapPathToHandle.end() )
    {
        if( itr->second == hDll )
        {
            m_mapPathToHandle.erase( itr );
            return 0;
        }
    }

    return -ENOENT;
}

}
