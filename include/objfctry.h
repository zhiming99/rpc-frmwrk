/*
 * =====================================================================================
 *
 *       Filename:  objfctry.h
 *
 *    Description:  declaration of object factory related classes
 *
 *        Version:  1.0
 *        Created:  01/13/2018 12:56:12 PM
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
#pragma once
#include "defines.h"
#include "stlcont.h"

class CObjMakerBase
{
    public:
    virtual CObjBase* operator()(
        const IConfigDb* pCfg )
    { return nullptr; }
    virtual ~CObjMakerBase()
    { ; }
};

template< class T >
class CObjMakerCfg :
    public CObjMakerBase
{
    public:

    virtual CObjBase* operator()(
        const IConfigDb* pCfg = nullptr )
    {
        CObjBase* pObj = new T( pCfg );
        pObj->DecRef();
        return pObj;
    }

    ~CObjMakerCfg()
    {;}
};

template< class T >
class CObjMaker :
    public CObjMakerBase
{
    public:

    virtual CObjBase* operator()(
        const IConfigDb* pCfg = nullptr )
    {
        CObjBase* pObj = new T();
        pObj->DecRef();
        return pObj;
    }
    ~CObjMaker()
    {;}
};

class IClassFactory : public CObjBase
{
    public:

    typedef CObjBase super;

    virtual gint32 CreateInstance( 
        EnumClsid clsid,
        CObjBase*& pObj,
        const IConfigDb* pCfg ) = 0;
    
    virtual const char* GetClassName(
        EnumClsid iClsid ) = 0;

    virtual EnumClsid GetClassId(
        const char* szClassName ) = 0;

    virtual void EnumClassIds(
        std::vector< EnumClsid >& vecClsIds ) = 0;
};

struct cmp_str
{
    bool operator()(char const *a, char const *b)
    {
        return strcmp(a, b) < 0;
    }
};

typedef std::map< EnumClsid, const char* > ID2NAME_MAP;
typedef std::map< const char*, EnumClsid, cmp_str > NAME2ID_MAP;
typedef std::map< EnumClsid, CObjMakerBase* > OBJMAKER_MAP;

#define OBJMAKER_ENTRY( name ) \
do{ oMapObjMakers[ clsid( name) ] = ( new CObjMaker< name >()  ); }while( 0 )

#define OBJMAKERCFG_ENTRY( name ) \
do{ oMapObjMakers[ clsid( name) ] = ( new CObjMakerCfg< name >()  ); }while( 0 )

#define BEGIN_FACTORY_MAPS \
{ \
    ID2NAME_MAP oMapId2Name; \
    NAME2ID_MAP oMapName2Id; \
    OBJMAKER_MAP oMapObjMakers; \

#define INIT_MAP_ENTRY( className ) \
do{ \
    oMapId2Name[ clsid( className ) ] = #className; \
    oMapName2Id[ #className ] = clsid( className ); \
    OBJMAKER_ENTRY( className ); \
}while( 0 )

#define INIT_MAP_ENTRYCFG( className ) \
do{ \
    oMapId2Name[ clsid( className ) ] = #className; \
    oMapName2Id[ #className ] = clsid( className ); \
    OBJMAKERCFG_ENTRY( className ); \
}while( 0 )

#define END_FACTORY_MAPS \
    FactoryPtr pFactory( FactoryPtr( new CClassFactory( \
        oMapId2Name, oMapName2Id, oMapObjMakers ), false ) ); \
   return pFactory; \
}\

class CClassFactory : public IClassFactory
{
    ID2NAME_MAP m_oMapId2Name;
    NAME2ID_MAP m_oMapName2Id;
    OBJMAKER_MAP m_oMapObjMakers;

    public:

    typedef IClassFactory super;

    CClassFactory( ID2NAME_MAP& map1,
        NAME2ID_MAP& map2, OBJMAKER_MAP& map3 )
    {
        m_oMapId2Name = map1;
        m_oMapName2Id = map2;
        m_oMapObjMakers = map3;
        SetClassId( clsid( CClassFactory ) );
    }

    ~CClassFactory()
    {
        for( auto oEntry : m_oMapObjMakers )
        {
            CObjMakerBase* pMaker = oEntry.second;
            delete pMaker;
        }

        m_oMapObjMakers.clear();
    }

    virtual gint32 CreateInstance( 
        EnumClsid clsid,
        CObjBase*& pObj,
        const IConfigDb* pCfg );
    
    virtual const char* GetClassName(
        EnumClsid iClsid );

    virtual EnumClsid GetClassId(
        const char* szClassName );

    virtual void EnumClassIds(
        std::vector< EnumClsid >& vecClsIds );
};

typedef CAutoPtr< clsid( Invalid ), IClassFactory >    FactoryPtr;

typedef std::pair< void*, FactoryPtr > ELEM_CLASSFACTORIES;
struct CClassFactories: public CStlVector< ELEM_CLASSFACTORIES >
{
    public:
    typedef CStlVector< ELEM_CLASSFACTORIES > super;

    CClassFactories();
    ~CClassFactories();

    /**
    * @name CreateInstance similiar to
    * CoCreateInstance, except the name.
    * @{ */
    /**  @} */
    gint32 CreateInstance( 
        EnumClsid clsid,
        CObjBase*& pObj,
        const IConfigDb* pCfg );

    const char* GetClassName(
        EnumClsid iClsid );

    EnumClsid GetClassId(
        const char* pszClassName );

    gint32 AddFactory(
        const FactoryPtr& pFactory, void* hDll );

    gint32 RemoveFactory(
        FactoryPtr& pFactory );

    void EnumClassIds(
        std::vector< EnumClsid >& vecClsIds );

    void Clear();
};

typedef CAutoPtr< clsid( CClassFactories ), CClassFactories > FctryVecPtr;

extern const char* CoGetClassName( EnumClsid iClsid );
extern EnumClsid CoGetClassId( const char* szClassName );
extern gint32 CoLoadClassFactory( const char* pszPath );
extern gint32 CoLoadClassFactories( const char* dir );
extern gint32 CoAddClassFactory( const FactoryPtr& pFactory );

extern gint32 CoInitialize( guint32 dwContext = 0 );
extern gint32 CoUninitialize();

