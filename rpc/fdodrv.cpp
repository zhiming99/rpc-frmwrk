/*
 * =====================================================================================
 *
 *       Filename:  fdodrv.cpp
 *
 *    Description:  implementation of the
 *    CProxyFdoDriver
 *
 *        Version:  1.0
 *        Created:  01/15/2017 10:14:09 PM
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

#include <vector>
#include <string>
#include "defines.h"
#include "port.h"
#include "dbusport.h"
#include "emaphelp.h"
#include "fdodrv.h"

using namespace std;

CProxyFdoDriver::CProxyFdoDriver(
    const IConfigDb* pCfg )
    : super( pCfg )
{
    gint32 ret = 0;
    SetClassId( clsid( CProxyFdoDriver ) );

    do{
        if( pCfg == nullptr )
            break;

        string strDrvName;
        CCfgOpener a( pCfg );
        ret = a.GetPointer( propIoMgr, m_pIoMgr );

    }while( 0 );

    if( ERROR( ret ) )
    {
        string strMsg = DebugMsg(
            ret, "error happens in CProxyFdoDriver ctor" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CProxyFdoDriver::CreatePort(
    PortPtr& pNewPort,
    const IConfigDb* pConfig )
{
    if( pConfig == nullptr )
        return -EINVAL;

    PortPtr pPort;
    gint32 ret = 0;

    do{
        string strInClass;
        string strExClass;

        CCfgOpener oCfg( pConfig );

        ret = oCfg.GetStrProp(
            propPortClass, strExClass );

        if( ERROR( ret ) )
            break;

        strInClass = CPort::ExClassToInClass(
            strExClass );

        EnumClsid iClsid = CoGetClassId(
            strInClass.c_str() );

        if( iClsid == clsid( Invalid ) )
        {
            ret = -ENOTSUP;
            break;
        }

        ret = pPort.NewObj( iClsid, pConfig );
        if( ERROR( ret ) )
            break;

        pNewPort = pPort;

        ret = AddPort( pNewPort );
        if( ERROR( ret ) )
        {
            pNewPort.Clear();
        }

    }while( 0 );

    return ret;
}

/**
* @name CProxyFdoDriver::Probe
* @{ to probe the pLowerPort to decide if it is
* necessary for the driver to create a fdo driver
* uppon the pLowerPort. This happens during the
* pnp manager building the port stack. if it
* returns success, the port stack building will
* continue, otherwise, the building process will
* stop.
*  */
/** 
 * Parameters:
 * pLowerPort: the port to detect.
 * pNewPort: the new Fdo port to attach to the
 *           plowerPort
 * pConfig:  the config parameters for port
 *           creation.
 *@} */

gint32 CProxyFdoDriver::Probe(
    IPort* pLowerPort,
    PortPtr& pNewPort,
    const IConfigDb* pConfig )
{
    gint32 ret = 0;

    do{
        if( pLowerPort == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        string strPdoClass;
        string strClass( PORT_CLASS_DBUS_PROXY_PDO );

        CCfgOpenerObj oCfg( pLowerPort );
        ret = oCfg.GetStrProp(
            propPdoClass, strPdoClass );

        if( ERROR( ret ) )
            break;

        if( strClass != strPdoClass )
        {
            // this is not a port we support
            ret = -ENOTSUP;
            break;
        }
        
        CfgPtr pNewCfg( true );

        if( pConfig != nullptr )
        {
            *pNewCfg = *pConfig;
        }

        CCfgOpener oNewCfg(
            ( IConfigDb* )pNewCfg );

        ret = oNewCfg.SetPointer( propIoMgr, GetIoMgr() );

        if( ERROR( ret ) )
            break;

        guint32 dwPortId = 0;

        ret = oCfg.GetIntProp(
            propPortId, dwPortId );

        if( ERROR( ret ) )
            break;

        if( m_mapIdToPort.find( dwPortId )
            != m_mapIdToPort.end() )
        {
            ret = -EEXIST;
            break;
        }
        
        ret = oNewCfg.SetIntProp(
            propPortId, dwPortId );

        if( ERROR( ret ) )
            break;

        ret = oNewCfg.SetStrProp(
            propPortClass,
            PORT_CLASS_DBUS_PROXY_FDO );

        if( ERROR( ret ) )
            break;

        ObjPtr pObj( this );
        ret = oNewCfg.SetObjPtr(
            propDrvPtr, pObj );

        if( ERROR( ret ) )
            break;
        
        string strPortId;
        ret = BytesToString( ( guint8*)&dwPortId,
            sizeof( guint32 ),
            strPortId );

        string strPortName =
            string( PORT_CLASS_DBUS_PROXY_FDO ) +
            string("_" ) +
            strPortId;

        ret = oNewCfg.SetStrProp(
            propPortName, strPortName );

        if( ERROR( ret ) )
            break;

        string strRegPath;
        CCfgOpenerObj oDrvCfg( this );

        ret = oDrvCfg.GetStrProp(
            propRegPath, strRegPath );

        if( ERROR( ret ) )
            break;

        strRegPath += "/";
        strRegPath += strPortName;

        ret = oNewCfg.SetStrProp(
            propRegPath, strRegPath );

        if( ERROR( ret ) )
            break;

        ret = CreatePort( pNewPort, pNewCfg );

        if( ERROR( ret ) )
            break;

        ret = pNewPort->AttachToPort(
            pLowerPort );

        if( ERROR( ret ) )
            break;

    }while( 0 );

    if( ERROR( ret ) )
    {
        pNewPort.Clear();
    }

    return ret;
}

