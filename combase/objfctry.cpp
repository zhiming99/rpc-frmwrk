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
    const IConfigDb* pCfg )
{
    gint32 ret = -ENOENT;

    if( m_oMapObjMakers.find( iClsid )
        != m_oMapObjMakers.end() )
    {
        ret = 0;
        pObj = ( *m_oMapObjMakers.at( iClsid ) )( pCfg );
        if( pObj != nullptr )
        {
            pObj->AddRef();
        }
        else
        {
            ret = -EFAULT;
        }
    }

    return ret;
}

const char* CClassFactory::GetClassName(
    EnumClsid iClsid )
{
    ID2NAME_MAP::iterator itr = 
        m_oMapId2Name.find( iClsid );

    if( itr == m_oMapId2Name.end() )
        return nullptr;

    return itr->second;
}

EnumClsid CClassFactory::GetClassId(
    const char* szClassName )
{
    if( szClassName == nullptr )
        return clsid( Invalid );

    NAME2ID_MAP::iterator itr = 
        m_oMapName2Id.find( szClassName );

    if( itr == m_oMapName2Id.end() )
        return clsid( Invalid );

    return itr->second;
}

void CClassFactory::EnumClassIds(
    std::vector< EnumClsid >& vecClsIds )
{
    for( auto itr : m_oMapId2Name )
    {
        vecClsIds.push_back( itr.first );
    }
    return;
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
    const IConfigDb* pCfg )
{
    gint32 ret = -ENOTSUP;

    // CStdRMutex oLock( GetLock() );
    // we are exposed to concurrent condition
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

const char* CClassFactories::GetClassName(
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

EnumClsid CClassFactories::GetClassId(
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
}

void CClassFactories::EnumClassIds(
    std::vector< EnumClsid >& vecClsIds )
{
    CStdRMutex oLock( GetLock() );
    MyType& vecFactories = ( *this) ();
    MyItr itr = vecFactories.begin();

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
