/*
 * =====================================================================================
 *
 *       Filename:  clsids.cpp
 *
 *    Description:  a map of class id to class name
 *
 *        Version:  1.0
 *        Created:  08/07/2016 01:18:27 PM
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
#include <map>
#include <unordered_map>
#include <vector>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#include "defines.h"
#include "buffer.h"
#include "configdb.h"
#include "registry.h"
#include "stlcont.h"
#include <dlfcn.h>
#include "objfctry.h"
#include "variant.h"

using namespace std;

namespace rpcf
{

extern FctryVecPtr g_pFactories;

// c++11 required

static FactoryPtr InitClassFactory()
{

    BEGIN_FACTORY_MAPS;

    INIT_MAP_ENTRY( CBuffer );
    INIT_MAP_ENTRY( CRegistry );
    INIT_MAP_ENTRY( CStlIntVector );
    INIT_MAP_ENTRY( CStlIntQueue );
    INIT_MAP_ENTRY( CStlBufVector );
    INIT_MAP_ENTRY( CStlEventMap );
    INIT_MAP_ENTRY( CStlIntMap );
    INIT_MAP_ENTRY( CStlObjSet );
    INIT_MAP_ENTRY( CStlObjVector );
    INIT_MAP_ENTRY( CStlStringSet );
    INIT_MAP_ENTRY( CStlLongWordVector );
    INIT_MAP_ENTRY( CStlQwordVector );
    INIT_MAP_ENTRY( CStlObjMap );

    //INIT_MAP_ENTRYCFG( CConfigDb );
    INIT_MAP_ENTRYCFG( CConfigDb2 );

    END_FACTORY_MAPS;
};

const char* CoGetClassName( EnumClsid iClsid )
{
    map< EnumClsid, const char* >::iterator itr;
    if( iClsid == clsid( Invalid ) )
        return "Invalid";

    // the search is said to be optimized
    // Let's assume it as a binary search
    return g_pFactories->GetClassName( iClsid );
}

EnumClsid CoGetClassId( const char* szClassName )
{
    if( szClassName == nullptr )
        return Clsid_Invalid;

    return g_pFactories->GetClassId( szClassName );

}

typedef gint32 (*PLOADCLASSFACTORY)( FactoryPtr& pFactory ) ;

gint32 CoLoadClassFactory( const char* pszPath  )
{
    if( pszPath == nullptr )
        return -EINVAL;

    FactoryPtr pFactory;
    void* hDll = nullptr;
    gint32 ret = 0;

    do{
        ret = g_pFactories->IsDllLoaded( pszPath );
        if( SUCCEEDED( ret ) )
            break;

        hDll = dlopen( pszPath, RTLD_LAZY );
        if( hDll == nullptr )
        {
            ret = -EBADF;
            char* pszError = dlerror();
            if( pszError )
            {
                fprintf( stderr,
                    "Warning: Fail to load dll: %s\n", pszError );
            }

            break;
        }

        PLOADCLASSFACTORY pFunc =
            ( PLOADCLASSFACTORY )dlsym( hDll, "DllLoadFactory" );

        if( pFunc == nullptr )
        {
            ret = -ENOENT;
            break;
        }

        ret = ( *pFunc )( pFactory );

        if( ERROR( ret ) )
        {
            pFactory.Clear();
            break;
        }

        ret = g_pFactories->AddFactory(
            pFactory, hDll );
        if( ERROR( ret ) )
            break;

        g_pFactories->AddFactoryPath(
            pszPath, hDll );

    }while( 0 );

    if( ERROR( ret ) && hDll != nullptr )
        dlclose( hDll );

    return ret;
}

gint32 CoAddClassFactory(
    const FactoryPtr& pFactory )
{

    if( pFactory.IsEmpty() )
        return -EINVAL;

    return g_pFactories->AddFactory(
        pFactory, nullptr );
}

/**
* @name Load all the class factory from the
* directory pszDir, and put them to vector
* g_pFactories.
* @{ */
/**  @} */

gint32 CoLoadClassFactories( const char* dir )
{
    string strPrefix = dir;
    std::vector< stdstr > vecLibs =
        { "libipc.so", "librpc.so" };

    gint32 ret = 0;
    for( auto& elem : vecLibs )
    {
        string strPath =
            strPrefix + "/" + elem;

        ret = CoLoadClassFactory(
            strPath.c_str() );

        if( ERROR( ret ) )
        {
            strPath.clear();
            // in case the libipc.so is not in
            // the same directory as the one of
            // libcombase.so
            ret = GetLibPath(
                strPath, elem.c_str() );

            // error again, give up
            if( ERROR( ret ) )
                continue;

            strPath = strPath + "/" + elem;
            ret = CoLoadClassFactory(
                strPath.c_str() );
            if( ERROR( ret ) )
                continue;
        }
    }
    // sometimes it is not necessary to load these
    // libraries
    return 0;
}

#ifdef DEBUG

// using guint32 instead of EnumClsid for
// compability with GCC-4.9.3
std::unordered_map<guint32, std::string> g_mapId2Name;

static gint32 CoInitClsidNames()
{
    vector<EnumClsid> vecIds;
    g_pFactories->EnumClassIds( vecIds );
    for( auto iClsid : vecIds )
    {
        const char* pszName = CoGetClassName( iClsid );
        if( pszName != nullptr )
        {
            g_mapId2Name[ iClsid ] = string( pszName );
        }
    }
    return 0;
}

#endif
// call this method before anything else is
// called
gint32 CoInitialize( guint32 dwContext )
{
    if( !g_pFactories.IsEmpty() )
        return 0;

    g_pFactories = new CClassFactories(); 
    if( g_pFactories.IsEmpty() )
        return -EFAULT;

    g_pFactories->DecRef();

    // load the class factory of combase ahead of 
    // any of the rest libraires
    FactoryPtr pBaseFactory = InitClassFactory();
    if( pBaseFactory.IsEmpty() )
        return -EFAULT;

    gint32 ret = g_pFactories->AddFactory(
        pBaseFactory, nullptr );

    if( ERROR( ret ) )
        return ret;

    std::string strPath;
    ret = GetLibPath( strPath );

    if( ERROR( ret ) )
        return ret;

    if( ( dwContext && COINIT_NORPC ) == 0 )
        ret = CoLoadClassFactories(
            strPath.c_str() );

#ifdef DEBUG
    if( SUCCEEDED( ret ) )
        CoInitClsidNames();        
#endif
    return ret;
}

gint32 CoUninitialize()
{
    g_pFactories->Clear();    
    g_pFactories.Clear();
    return 0;
}

extern "C"
gint32 DllLoadFactory( FactoryPtr& pFactory )
{
    pFactory = InitClassFactory();
    if( pFactory.IsEmpty() )
        return -EFAULT;

    return 0;
}

}
