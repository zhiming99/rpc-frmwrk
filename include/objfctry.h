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
 * =====================================================================================
 */
#pragma once
#include "defines.h"

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
        return new T( pCfg );
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
        return new T();
    }
    ~CObjMaker()
    {;}
};

extern const char* CoGetClassName( EnumClsid iClsid );
extern EnumClsid CoGetClassId( const char* szClassName );
extern gint32 CoLoadClassFactory( const char* pszPath );
extern gint32 CoLoadClassFactories( const char* dir );

extern gint32 CoCreateInstance( EnumClsid clsid,
    CObjBase*& pObj, const IConfigDb* pCfg = nullptr ); 

extern gint32 CoInitialize( guint32 dwContext = 0 );
extern gint32 CoUninitialize();

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

typedef std::map< EnumClsid, const char* > ID2NAME_MAP;
typedef std::map< const char*, EnumClsid > NAME2ID_MAP;
typedef std::map< EnumClsid, CObjMakerBase* > OBJMAKER_MAP;

#define OBJMAKER_ENTRY( name ) \
do{ oMapObjMakers[ clsid( name) ] = ( new CObjMaker< name >()  ); }while( 0 )

#define OBJMAKERCFG_ENTRY( name ) \
do{ oMapObjMakers[ clsid( name) ] = ( new CObjMakerCfg< name >()  ); }while( 0 )

#define BEGIN_FACTORY_MAPS \
do{ \
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
    FactoryPtr pFactory = new CClassFactory( \
        oMapId2Name, oMapName2Id, oMapObjMakers ); \
    return pFactory; \
}while( 0 );

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

// c++11 required
template <typename R, typename C, typename ... Types>
inline constexpr size_t GetArgCount( R(C::*f)(Types ...) )
{
   return sizeof...(Types);
};

