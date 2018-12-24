/*
 * =====================================================================================
 *
 *       Filename:  drvmgr.cpp
 *
 *    Description:  implementation of CDriverManager
 *
 *        Version:  1.0
 *        Created:  08/19/2016 06:48:22 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <vector>
#include <string>
#include <system_error>
#include "defines.h"
#include "port.h"
#include "dbusport.h"
#include "frmwrk.h"
#include "jsondef.h"
#include "stlcont.h"
#include "objfctry.h"

using namespace std;

gint32 ReadJsonCfg(
    const std::string& strFile,
    Json::Value& valConfig )
{

    gint32 ret = 0;
    FILE* fp = NULL;
    size_t iLen = 0;
    do{
        fp = fopen( strFile.c_str(), "rb" );
        if( fp == NULL )
        {
            ret = -errno;
            break;
        }
        ret = fseek( fp, 0, SEEK_END );
        if( ERROR( ret ) )
        {
            ret = -errno;
            break;
        }
        iLen = ( size_t )ftell( fp );
        if( ERROR( iLen ) )
        {
            ret = -errno;
            break;
        }

        if( iLen < 100
            || iLen > 1024 * 1024 )
        {
            ret = -EBADMSG;
            break;
        }

        fseek( fp, 0, SEEK_SET );

        BufPtr buf( true );
        buf->Resize( iLen + 16 );
        size_t iSize = fread( buf->ptr(), 1, iLen, fp );
        if( iSize < iLen )
        {
            ret = -EBADF;
            break;
        }

        Json::CharReaderBuilder oBuilder;
        Json::CharReader* pReader = nullptr;
        pReader = oBuilder.newCharReader();
        if( pReader == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        if( !pReader->parse( buf->ptr(),
            buf->ptr() + buf->size(),
            &valConfig, nullptr ) )
        {
            ret = -EBADMSG;
        }

    }while( 0 );

    if( fp != nullptr )
    {
        fclose( fp );
    }

    return ret;
}

CDriverManager::CDriverManager(
    const IConfigDb* pCfg )
{
    if( pCfg == nullptr )
        return;

    SetClassId( clsid( CDriverManager ) );
    BufPtr bufPtr( true );

    CCfgOpener a( pCfg );
    // DrvMgr don't hold the reference of the io manager
    // because it totally contained in the lifecycle of
    // io manager
    gint32 ret = a.GetPointer(
        propIoMgr, m_pIoMgr );

    if( ERROR( ret ) )
    {
        // we cannot live without io manager
        throw std::invalid_argument(
            "cannot get IoMananger" );
    }

    // we also need the config file to find more
    // information for driver loading
    ret = a.GetStrProp( propConfigPath, m_strCfgPath );

    if( ERROR( ret ) )
    {
        // we cannot live without the config file
        throw std::invalid_argument(
            "Cannot get config path" );
    }

    ret = ReadConfig( m_oConfig );
    if( ERROR( ret ) )
    {
        throw std::invalid_argument(
            "error read config file" );
    }

    // done
    return;
}

CDriverManager::~CDriverManager()
{
    m_strCfgPath.clear();
}

gint32 CDriverManager::Start()
{

    int ret = 0;
    do{
        // make the registry entries    
        CRegistry& oReg = GetIoMgr()->GetRegistry();

        string strPath = GetBusDrvRegRoot();
        ret = oReg.MakeDir( strPath );
        if( ERROR( ret ) )
        {
            break;
        }

        strPath = GetPortDrvRegRoot();
        ret = oReg.MakeDir( strPath );
        if( ERROR( ret ) )
        {
            break;
        }

        // load all the class factories
        ret = LoadClassFactories();
        if( ret != -ENOPKG && ERROR( ret ) )
            break;

        // load all the bus drivers
        ret = LoadStaticDrivers();
        if( ERROR( ret ) )
            break;

        ret = 0;
        CreatePreloadable();

    }while( 0 );
    return ret;
}

gint32 CDriverManager::StopDrivers()
{
    gint32 ret = 0;
    string strPath = GetBusDrvRegRoot();
    do{
        // let's walk through the bus port list
        // to stop all the active bus ports
        CRegistry& oReg = GetIoMgr()->GetRegistry();
        vector< string > vecContents;

        do{
            CStdRMutex a( oReg.GetLock() );
            ret = oReg.ChangeDir( strPath );

            if( ERROR( ret ) )
                break;

            ret = oReg.ListDir( vecContents );
            if( ERROR( ret ) )
                break;

        }while( 0 );

        if( ERROR( ret ) )
        {
            // well, no child
        }
        for( gint32 i = 0; i < ( gint32 )vecContents.size(); i++ )
        {
            IPortDriver* pDrv = nullptr;
            ret = FindDriver( vecContents[ i ], pDrv );

            if( ERROR( ret ) )
                continue;

            ret = pDrv->Stop();
            if( ERROR( ret ) )
            {
                //FIXME: what should I do?
            }
        }

        ret = oReg.RemoveDir( strPath );

        if( strPath == GetPortDrvRegRoot() )
            break;

        strPath = GetPortDrvRegRoot();
        
    }while( 1 );
    return ret;
}

gint32 CDriverManager::Stop()
{
    gint32 ret = StopPreloadable();
    if( ERROR( ret ) )
    {
        if( ret != -ENOENT &&
            ret != -ENOPKG )
            return ret;
    }

    ret = StopDrivers();

    return ret;
}

gint32 CDriverManager::LoadClassFactories()
{

    gint32 ret = 0;

    do{
        Json::Value& oModuleArray =
            m_oConfig[ JSON_ATTR_MODULES ];

        if( oModuleArray == Json::Value::null )
        {
            ret = -ENOENT;
            break;
        }

        if( !oModuleArray.isArray() 
            || oModuleArray.size() == 0 )
        {
            ret = -EBADMSG;
            break;
        }

        Json::Value oClsToLoad;
        guint32 i;
        for( i = 0; i < oModuleArray.size(); i++ )
        {
            Json::Value& oModDesc =
                oModuleArray[ i ];

            Json::Value& oModName =
                oModDesc[ JSON_ATTR_MODNAME ];

            if( oModName.asString() ==
                GetIoMgr()->GetModName() )
            {
                oClsToLoad = oModDesc[ JSON_ATTR_CLSFACTORY ];
                break;
            }
        }
        if( i == oModuleArray.size() )
        {
            ret = -ENOPKG;
            break;
        }

        if( oClsToLoad == Json::Value::null )
        {
            ret = -ENOPKG;
            break;
        }

        if( !oClsToLoad.isArray()
            || oClsToLoad.size() == 0 )
        {
            ret = -ENOPKG;
            break;
        }

        for( guint32 i = 0; i < oClsToLoad.size(); i++ )
        {
            if( oClsToLoad[ i ].empty() ||
                !oClsToLoad[ i ].isString() )
                continue;

            ret = CoLoadClassFactory(
                oClsToLoad[ i ].asString().c_str() );

            if( ERROR( ret ) )
            {
                // FIXME: bad thing happens, what to do
                // simply fail?
                break;
            }
        }

    }while( 0 );

    return ret;
}

gint32 CDriverManager::LoadStaticDrivers()
{

    gint32 ret = 0;

    do{
        Json::Value& oModuleArray =
            m_oConfig[ JSON_ATTR_MODULES ];

        if( oModuleArray == Json::Value::null )
        {
            ret = -ENOENT;
            break;
        }

        if( !oModuleArray.isArray() 
            || oModuleArray.size() == 0 )
        {
            ret = -EBADMSG;
            break;
        }

        Json::Value oDrvToLoad;
        guint32 i;
        for( i = 0; i < oModuleArray.size(); i++ )
        {
            Json::Value& oModDesc =
                oModuleArray[ i ];

            Json::Value& oModName =
                oModDesc[ JSON_ATTR_MODNAME ];

            if( oModName.asString() ==
                GetIoMgr()->GetModName() )
            {
                oDrvToLoad =
                    oModDesc[ JSON_ATTR_DRVTOLOAD ];
                break;
            }
        }
        if( i == oModuleArray.size() )
        {
            ret = -ENOPKG;
            break;
        }

        if( oDrvToLoad == Json::Value::null )
        {
            ret = -ENOPKG;
            break;
        }

        if( !oDrvToLoad.isArray()
            || oDrvToLoad.size() == 0 )
        {
            ret = -ENOPKG;
            break;
        }

        for( guint32 i = 0; i < oDrvToLoad.size(); i++ )
        {
            if( oDrvToLoad[ i ].empty()
                || !oDrvToLoad[ i ].isString() )
                continue;

            ret = LoadDriver( oDrvToLoad[ i ].asString() );
            if( ERROR( ret ) )
            {
                // FIXME: bad thing happens, what to do
            }
        }
    }while( 0 );

    return ret;
}

gint32 CDriverManager::CreatePreloadable()
{
    gint32 ret = 0;

    do{
        Json::Value& oModuleArray =
            m_oConfig[ JSON_ATTR_MODULES ];

        if( oModuleArray == Json::Value::null )
        {
            ret = -ENOENT;
            break;
        }

        if( !oModuleArray.isArray() 
            || oModuleArray.size() == 0 )
        {
            ret = -EBADMSG;
            break;
        }

        Json::Value oObjToLoad;
        guint32 i;
        for( i = 0; i < oModuleArray.size(); i++ )
        {
            Json::Value& oModDesc =
                oModuleArray[ i ];

            Json::Value& oModName =
                oModDesc[ JSON_ATTR_MODNAME ];

            if( oModName.asString() ==
                GetIoMgr()->GetModName() )
            {
                oObjToLoad = oModDesc[ JSON_ATTR_OBJTOLOAD ];
                break;
            }
        }
        if( i == oModuleArray.size() )
        {
            ret = -ENOPKG;
            break;
        }

        if( oObjToLoad == Json::Value::null )
        {
            ret = -ENOPKG;
            break;
        }

        if( !oObjToLoad.isArray() ||
            oObjToLoad.size() == 0 )
        {
            ret = -ENOPKG;
            break;
        }
        else
        {
            CRegistry& oReg = GetIoMgr()->GetRegistry();

            string strPath = string( REG_PRELOADABLE_ROOT )
                + string( "/" )
                + GetIoMgr()->GetModName();

            ret = oReg.MakeDir( strPath );
            if( ERROR( ret ) )
                break;
        }

        for( guint32 i = 0; i < oObjToLoad.size(); i++ )
        {
            if( oObjToLoad[ i ].empty() ||
                !oObjToLoad[ i ].isObject() )
                continue;

            Json::Value& oObjElem = oObjToLoad[ i ];

            if( !oObjElem.isMember( JSON_ATTR_OBJNAME ) ||
                !oObjElem[ JSON_ATTR_OBJNAME ].isString() )
                continue;

            string strObjName =
                oObjElem[ JSON_ATTR_OBJNAME ].asString();


            if( !oObjElem.isMember( JSON_ATTR_OBJCLASS ) ||
                !oObjElem[ JSON_ATTR_OBJCLASS ].isString() )
                continue;

            string strClass =
                oObjElem[ JSON_ATTR_OBJCLASS ].asString();

            ret = CreateObject( strObjName, strClass );

            if( ERROR( ret ) )
            {
                // FIXME: bad thing happens, what to do
            }
        }

    }while( 0 );

    return ret;
}

// Create an object of the specified of this module
gint32 CDriverManager::CreateObject(
    const std::string& strObjName,
    const std::string& strClass )
{
    gint32 ret = 0;
    do{
        CParamList oParams;
        oParams.SetPointer( propIoMgr, GetIoMgr() );
        ObjPtr pObj;
        
        string strClassName =
            CPort::ExClassToInClass( strClass );

        EnumClsid iClsid =
            CoGetClassId( strClassName.c_str() );

        if( iClsid == clsid( Invalid ) )
        {
            ret = -ENOENT;
            break;
        }

        ret = pObj.NewObj(
            iClsid, oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        // NOTE: all the preloadable objects
        // should have inherited from the IService
        // interface
        IService* pService = pObj;
        if( pService == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        //NOTE: if the preloadable object failed to
        //start, or keep a reference to itself after
        //the Start, it will be removed when this
        //methods returns.

        ret = pService->Start();
        if( ERROR( ret ) )
            break;

        string strPath = string( REG_PRELOADABLE_ROOT )
            + string( "/" )
            + GetIoMgr()->GetModName();

        if( strPath.size() )
        {
            CRegistry& oReg = GetIoMgr()->GetRegistry();
            CStdRMutex a( oReg.GetLock() );
            ret = oReg.ChangeDir( strPath );
            if( ERROR( ret ) )
                break;

            ret = oReg.MakeDir( strObjName );
            if( ERROR( ret ) )
                break;

            ret = oReg.ChangeDir( strObjName );
            if( ERROR( ret ) )
                break;

            ret = oReg.SetObject(
                propObjPtr, pObj );
        }

    }while( 0 );

    return ret;
}
 
gint32 CDriverManager::StopPreloadable()
{
    gint32 ret = 0;
    do{
        // let's walk through the bus port list
        // to stop all the active bus ports
        CRegistry& oReg = GetIoMgr()->GetRegistry();

        string strPath = string( REG_PRELOADABLE_ROOT )
            + string( "/" )
            + GetIoMgr()->GetModName();

        vector< string > vecContents;

        if( !strPath.empty() )
        {
            CStdRMutex a( oReg.GetLock() );
            ret = oReg.ChangeDir( strPath );

            if( ERROR( ret ) )
                break;

            ret = oReg.ListDir( vecContents );
            if( ERROR( ret ) )
                break;
        }

        if( ERROR( ret ) )
        {
            // well, no child
            break;
        }

        for( gint32 i = 0; i < ( gint32 )vecContents.size(); i++ )
        {
            string strClassPath =
                strPath + "/" + vecContents[ i ];

            ObjPtr pObj;
            ret = GetIoMgr()->GetObject(
                strClassPath, pObj );

            if( ERROR( ret ) )
                continue;

            IService* pSvc = pObj;
            if( pSvc == nullptr )
                continue;

            pSvc->Stop();
        }
        ret = oReg.RemoveDir( strPath );

    }while( 0 );

    return ret;
}

// driver loading 
gint32 CDriverManager::LoadDriver(
    const string& strDriverName )
{
    gint32 ret = 0;
    do{
        Json::Value& oDriverArray =
            ( Json::Value& )m_oConfig[ JSON_ATTR_DRIVERS ];

        if( oDriverArray == Json::Value::null )
        {
            ret = -ENOENT;
            break;
        }

        if( !oDriverArray.isArray() 
            || oDriverArray.size() == 0 )
        {
            ret = -EBADMSG;
            break;
        }

        guint32 i;
        string strInClass;
        for( i = 0; i < oDriverArray.size(); i++ )
        {
            Json::Value& oDrvDesc =
                oDriverArray[ i ];

            Json::Value& oDrvName =
                oDrvDesc[ JSON_ATTR_DRVNAME ];

            if( oDrvName.asString() == strDriverName )
            {
                strInClass =
                    oDrvDesc[ JSON_ATTR_INCLASS ].asString();
                break;
            }
        }

        if( i == oDriverArray.size() )
        {
            ret = -ENOPKG;
            break;
        }

        EnumClsid iClsid = CoGetClassId(
            strInClass.c_str() );

        if( iClsid == Clsid_Invalid )
        {
            ret = -EINVAL;
            break;
        }

        CCfgOpener oExtCfg;

        ObjPtr objPtr( GetIoMgr() );

        // to pass a pointer to iomanager
        ret = oExtCfg.SetPointer( propIoMgr, GetIoMgr() );

        if( ERROR( ret ) )
            break;

        // to pass the driver name
        ret = oExtCfg.SetStrProp(
            propDrvName, strDriverName );

        if( ERROR( ret ) )
            break;

        // to pass the config name in case necessary
        ret = oExtCfg.SetStrProp(
            propConfigPath, m_strCfgPath );

        if( ERROR( ret ) )
            break;

        // create the object
        PortDrvPtr pDrv;
        ret = pDrv.NewObj(
            iClsid, oExtCfg.GetCfg() );

        if( ERROR( ret ) )
            break;

        // on success, the driver will register itself
        // to the proper registry
        ret = pDrv->Start();

    }while( 0 );

    return ret;
}

string CDriverManager::GetPreloadRegRoot(
    const string& strClass) const
{
    return string( REG_PRELOADABLE_ROOT )
        + string( "/" )
        + GetIoMgr()->GetModName()
        + string( "/" )
        + strClass;
}

string CDriverManager::GetBusDrvRegRoot() const
{
    return string( REG_DRV_ROOT )
        + string( "/" )
        + GetIoMgr()->GetModName()
        + string( "/bus_drivers" );
}

string CDriverManager::GetPortDrvRegRoot() const
{
    return string( REG_DRV_ROOT  )
        + string( "/" )
        + GetIoMgr()->GetModName()
        + string( "/port_drivers" );
}

gint32 CDriverManager::GetDrvRegPath(
    CPortDriver* pDrv,
    std::string& strPath,
    bool bBusDrv ) const
{
    gint32 ret = 0;

    if( pDrv == nullptr )
        return -EINVAL;

    string strRegRoot;
    if( bBusDrv )
        strRegRoot = GetBusDrvRegRoot();
    else
        strRegRoot = GetPortDrvRegRoot();

    string strDrvName;
    ret = pDrv->GetDrvName( strDrvName );
    if( ERROR( ret ) )
        return ret;

    strPath = strRegRoot
        + string( "/" )
        + strDrvName;

    return ret;
}

gint32 CDriverManager::RegDrvInternal(
    const std::string& strRegPath,
    CPortDriver* pDrv ) const
{

    gint32 ret = 0;
    do{
        CRegistry& oReg = GetIoMgr()->GetRegistry();
        if( true )
        {
            CStdRMutex oMutex( oReg.GetLock() );
            ret = oReg.MakeDir( strRegPath );
            if( ERROR( ret ) )
                break;

            ObjPtr objPtr( pDrv );
            ret = oReg.SetObject( propObjPtr, objPtr );
            if( ERROR( ret ) )
                break;

            // set the reg path for this driver object
            CCfgOpenerObj a( pDrv );
            ret = a.SetStrProp( propRegPath, strRegPath );
            if( ERROR( ret ) )
                break;

            // add a event map to the driver's registry
            ObjPtr pEvtMap;
            ret = pEvtMap.NewObj(
                clsid( CStlEventMap ) );

            if( ERROR( ret ) )
                break;
            
            ret = oReg.ChangeDir( strRegPath );
            if( ERROR( ret ) )
                break;

            ret = oReg.SetObject(
                propEventMapPtr, pEvtMap );

        }
    }while( 0 );

    return ret;
}


gint32 CDriverManager::RegBusDrv(
    IBusDriver* pDrv ) const
{
    gint32 ret = 0;
    do{
        string strDrvName;
        string strRegPath = GetBusDrvRegRoot();

        ret = pDrv->GetDrvName( strDrvName );
        if( ERROR( ret ) )
            break;

        strRegPath += string( "/" ) + strDrvName;

        ret = RegDrvInternal( strRegPath, pDrv );
        if( ERROR( ret ) )
            break;


    }while( 0 );
    return ret;
}

gint32 CDriverManager::RegPortDrv(
    CPortDriver* pDrv ) const
{
    gint32 ret = 0;
    do{
        if( pDrv == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        string strDrvName;
        CCfgOpenerObj a( pDrv );
        string strRegPath = GetPortDrvRegRoot();

        ret = a.GetStrProp( propDrvName, strDrvName );
        if( ERROR( ret ) )
            break;

        strRegPath += string( "/" ) + strDrvName;
        ret = RegDrvInternal( strRegPath, pDrv );

    }while( 0 );

    return ret;
}

gint32 CDriverManager::UnRegDrvInternal(
    const std::string& strRegPath,
    CPortDriver* pDrv ) const
{

    gint32 ret = 0;

    CRegistry& oReg = GetIoMgr()->GetRegistry();
    ret = oReg.RemoveDir( strRegPath );

    return ret;
}

/**
* @name UnRegBusDrv
* @{ */
/** to remove the registry associated to
 * the bus driver `pDrv' previously registered
 * by it @} */

gint32 CDriverManager::UnRegBusDrv(
    IBusDriver* pDrv ) const
{
    gint32 ret = 0;
    do{
        if( pDrv == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        string strRegPath = GetBusDrvRegRoot();
        CCfgOpenerObj a( pDrv );
        string strDrvName;
        ret = a.GetStrProp( propDrvName, strDrvName );
        if( ERROR( ret ) )
            break;

        strRegPath += string( "/" ) + strDrvName;

        ret = UnRegDrvInternal( strRegPath, pDrv );

    }while( 0 );
    return ret;
}

gint32 CDriverManager::UnRegPortDrv(
    CPortDriver* pDrv ) const
{

    gint32 ret = 0;
    do{
        if( pDrv == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        string strDrvName;
        string strRegPath = GetPortDrvRegRoot();
        CCfgOpenerObj a( pDrv );
        ret = a.GetStrProp( propDrvName, strDrvName );
        if( ERROR( ret ) )
            break;

        strRegPath += string( "/" ) + strDrvName;

        ret = UnRegDrvInternal( strRegPath, pDrv );

    }while( 0 );
    return ret;
}

gint32 CDriverManager::ReadConfig(
    Json::Value& valConfig ) const
{
    return ReadJsonCfg(
        m_strCfgPath, valConfig );
}

gint32 CDriverManager::BuildPortStack(
    IPort* pPort )
{
    gint32 ret = 0;

    if( pPort == nullptr )
    {
        return -EINVAL;
    }


    Json::Value& oMatchArray = m_oConfig[ JSON_ATTR_MATCH ];

    if( oMatchArray == Json::Value::null )
    {
        return -ENOENT;
    }

    if( !oMatchArray.isArray() 
        || oMatchArray.size() == 0 )
    {
        return -ENOENT;
    }

    CCfgOpener a;
    a.SetStrProp( propConfigPath, m_strCfgPath );

    do{

        CCfgOpenerObj oPortCfg( pPort );

        string strPortClass;
        string strDrvName;

        ret = oPortCfg.GetStrProp(
            propPortClass, strPortClass );

        if( ERROR( ret ) )
            break;

        guint32 i = 0;
        for( ; i < oMatchArray.size(); i++ )
        {
            Json::Value elem = oMatchArray[ i ];
            if( elem == Json::Value::null )
                continue;

            if( elem[ JSON_ATTR_PORTCLASS ].asString() != strPortClass )
                continue;

            if( elem[ JSON_ATTR_FDODRIVER ] != Json::Value::null )
            {
                strDrvName = elem[ JSON_ATTR_FDODRIVER ].asString();
                break;
            }

            if( elem[ JSON_ATTR_FDODRIVER ] != Json::Value::null )
            {
                strDrvName = elem[ JSON_ATTR_FDODRIVER ].asString();
                break;
            }
        }

        //
        // TODO: we need to add support for
        // filter drivers in both the config file
        // and the code if needed
        //
        if( i == oMatchArray.size() )
        {
            // driver not installed
            ret = -ENOPKG;
            break;
        }

        IPortDriver* pDrv = nullptr;
        ret = FindDriver( strDrvName, pDrv ); 
        if( ERROR( ret ) )
        {
            ret = LoadDriver( strDrvName );
            if( ERROR( ret ) )
                break;

            ret = FindDriver( strDrvName, pDrv );
            if( ERROR( ret ) )
                break;
        }

        PortPtr pNewPort;
        ret = pDrv->Probe(
            pPort, pNewPort, a.GetCfg() );

        if( ERROR( ret ) )
        {
            break;
        }

        // repeat to attach more port on top of pNewPort
        pPort = pNewPort;

    }while( 1 );
    ret = 0;
    return ret;
}

gint32 CDriverManager::FindDriver(
    const std::string& strDriverName,
    IPortDriver*& pRet ) const
{

    gint32 ret = 0;
    bool bBus = false;
    do{

        Json::Value& oDriverArray =
            ( Json::Value& )m_oConfig[ JSON_ATTR_DRIVERS ];

        if( oDriverArray == Json::Value::null )
        {
            ret = -ENOENT;
            break;
        }

        if( !oDriverArray.isArray() 
            || oDriverArray.size() == 0 )
        {
            ret = -EBADMSG;
            break;
        }
        string strDrvType;
        guint32 i;
        for( i = 0; i < oDriverArray.size(); i++ )
        {
            Json::Value& oDrvDesc = oDriverArray[ i ];
            Json::Value& oDrvName = oDrvDesc[ JSON_ATTR_DRVNAME ];
            if( oDrvName.asString() == strDriverName )
            {
                strDrvType = oDrvDesc[ JSON_ATTR_DRVTYPE ].asString();
                break;
            }
        }
        if( i == oDriverArray.size() )
        {
            ret = -ENOPKG;
            break;
        }
        if( strDrvType == JSON_VAL_BUSDRV )
            bBus = true;
        else if( strDrvType == JSON_VAL_PORTDRV )
            bBus = false;
        else
        {
            ret = -ENOTSUP;
            break;
        }


        CRegistry& oReg = GetIoMgr()->GetRegistry();
        string strPath;
        if( bBus )
        {
            strPath = GetBusDrvRegRoot();
        }
        else
        {
            strPath = GetPortDrvRegRoot();
        }
         
        strPath += string( "/" ) + strDriverName;
        if( true )
        {
            CStdRMutex a( oReg.GetLock() );

            ret = oReg.ChangeDir( strPath );
            if( ERROR( ret ) )
                break;

            ObjPtr pObj;
            CCfgOpenerObj regOpener( &oReg );
            ret = regOpener.GetObjPtr( propObjPtr, pObj );
            if( ERROR( ret ) )
                break;

            pRet = pObj;

            if( pRet == nullptr )
            {
                ret = -EFAULT;
            }
            break;
        }

    }while( 0 );

    return ret;
}

