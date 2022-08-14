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

namespace rpcf
{

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

    public:

    typedef CRpcPdoPort super;

    CDBusStreamPdo(
        const IConfigDb* pCfg );

    inline ObjPtr GetStreamIf() const
    { return m_pIf; }

    bool IsServer() const
    { return m_bServer; }

    gint32 SendDBusMsg( DBusMessage* pMsg,
        guint32* pdwSerial ) override;

    DBusHandlerResult PreDispatchMsg(
        gint32 iType, DBusMessage* pMsg ) override;

    bool Unloadable() override
    { return true; }

    gint32 PreStop( IRP* pIrp ) override;

    gint32 PostStart( IRP* pIrp ) override;
};

}
