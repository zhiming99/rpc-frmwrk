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
 * =====================================================================================
 */

#include <glib/glib.h>
#include <map>
#include <vector>
#include "defines.h"
#include "stlcont.h"
#include "objfctry.h"


gint32 CClassFactory::CreateInstance( 
    EnumClsid clsid,
    CObjBase*& pObj,
    const IConfigDb* pCfg )
{
    gint32 ret = -ENOENT;

    if( m_oMapIdToObjMaker.find( iClsid )
        != m_oMapIdToObjMaker.end() )
    {
        ret = 0;
        pObj = ( *m_oMapIdToObjMaker.at( iClsid ) )( pCfg );
        if( !pObj.IsEmpty() )
        {
            pObj->AddRef();
        }
        else
        {
            ret = -EFAUTL;
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
        m_oMapName2Id.find( iClsid );

    if( itr == m_oMapName2Id.end() )
        return nullptr;

    return itr->second;
}

void CClassFactory::EnumClassIds(
    std::vector< EnumClsid >& vecClsIds )
{
    for( auto itr : m_oMapId2Name )
    {
        vecClsIds.push_back( itr->first );
    }
    return;
}
