/*
 * =====================================================================================
 *
 *       Filename:  stmpdo.h
 *
 *    Description:  declaration of stream pdo class 
 *
 *        Version:  1.0
 *        Created:  08/14/2022 09:31:38 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2022 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */

#pragma once

#include <dbus/dbus.h>
#include "defines.h"
#include "port.h"
#include "autoptr.h"
#include "msgmatch.h"
#include "dbusport.h"
#define SESS_HASH_SIZE 48

namespace rpcf
{

struct IRPCTX_EXTDSP {
    IRPCTX_EXTDSP()
    {
        m_hStream = 0;
        m_szHash[ 0 ] = 0;
        m_bWaitWrite = false;
    }
    HANDLE m_hStream;
    char m_szHash[ SESS_HASH_SIZE ];
    bool m_bWaitWrite;
};

class CDBusStreamPdo :
    public CRpcPdoPort
{
    ObjPtr m_pIf;
    bool m_bServer = false;

    protected:

    virtual gint32 SetupDBusSetting(
        IMessageMatch* pMatch ) override;
    { return 0; }

    virtual gint32 ClearDBusSetting(
        IMessageMatch* pMatch ) override
    { return 0; }

    gint32 StopStreamChan( IRP* pIrp );

    public:

    typedef CRpcPdoPort super;

    CDBusStreamPdo(
        const IConfigDb* pCfg );

    inline ObjPtr GetStreamIf() const
    { return m_pIf; }

    inline SetStreamIf( ObjPtr& pIf )
    { m_pIf = pIf; }

    bool IsServer() const
    { return m_bServer; }

    gint32 HandleSendReq(
        IRP* pIrp ) override;

    gint32 HandleSendEvent(
        IRP* pIrp ) override;

    DBusHandlerResult PreDispatchMsg(
        gint32 iType, DBusMessage* pMsg ) override;

    bool Unloadable() override
    { return true; }

    gint32 SendDBusMsg(
        PIRP pIrp, DMsgPtr& pMsg );

    gint32 BroadcastDBusMsg(
        PIRP pIrp, DMsgPtr& pMsg );

    gint32 PreStop( IRP* pIrp ) override;

    gint32 PostStart( IRP* pIrp ) override;
};

class CDBusStreamBusPort : 
    public CGenericBusPortEx
{
    gint32 CreateDBusStreamPdo(
        IConfigDb* pCfg,
        PortPtr& pNewPort );

    public:
    typedef CGenericBusPortEx super;
    CDBusStreamBusPort ( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CDBusStreamBusPort ) ); }

    virtual gint32 BuildPdoPortName(
        IConfigDb* pCfg,
        std::string& strPortName );

    virtual gint32 CreatePdoPort(
        IConfigDb* pConfig,
        PortPtr& pNewPort );
};

class CDBusStreamBusDrv :
    public CGenBusDriverEx
{
    public:
    typedef CGenBusDriverEx super;
    CDBusStreamBusDrv( const IConfigDb* pCfg );
	gint32 Probe( IPort* pLowerPort,
        PortPtr& pNewPort,
        const IConfigDb* pConfig = NULL );
};

}
