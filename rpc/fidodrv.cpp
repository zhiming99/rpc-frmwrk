/*
 * =====================================================================================
 *
 *       Filename:  fidodrv.cpp
 *
 *    Description:  implementation of the CRpcTcpFidoDrv class
 *
 *        Version:  1.0
 *        Created:  01/11/2019 01:08:09 PM
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
#include "defines.h"
#include "stlcont.h"
#include "msgmatch.h"
#include "emaphelp.h"
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "dbusport.h"
#include "tcpport.h"

using namespace std;

CRpcTcpFidoDrv::CRpcTcpFidoDrv(
    const IConfigDb* pCfg )
    : super( pCfg )
{
    SetClassId( clsid( CRpcTcpFidoDrv ) );
}

gint32 CRpcTcpFidoDrv::Probe(
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

        CCfgOpenerObj oCfg( pLowerPort );
        ret = oCfg.GetStrProp(
            propPdoClass, strPdoClass );

        if( ERROR( ret ) )
            break;

        if( PORT_CLASS_TCP_STREAM_PDO != strPdoClass &&
            PORT_CLASS_TCP_STREAM_PDO2 != strPdoClass )
        {
            // this is not a port we support
            ret = -ENOTSUP;
            break;
        }

        CParamList oNewCfg;
        oNewCfg[ propPortClass ] =
            PORT_CLASS_RPC_TCP_FIDO;

        oNewCfg[ propPortId ] = NewPortId();

        oNewCfg.CopyProp(
            propConnParams, pLowerPort );

        oNewCfg.SetPointer(
            propIoMgr, GetIoMgr() );

        oNewCfg.SetPointer(
            propDrvPtr, this );

        ret = CreatePort( pNewPort,
            oNewCfg.GetCfg() );

        if( ERROR( ret ) )
            break;

        ret = pNewPort->AttachToPort(
            pLowerPort );

        if( ERROR( ret ) )
            break;

    }while( 0 );

    return ret;
}

gint32 CRpcTcpFidoDrv::CreatePort(
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
