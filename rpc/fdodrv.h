/*
 * =====================================================================================
 *
 *       Filename:  fdodrv.h
 *
 *    Description:  declaration of the CProxyFdoDriver
 *
 *        Version:  1.0
 *        Created:  01/15/2017 10:10:30 PM
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

#pragma once

#include <dbus/dbus.h>
#include "defines.h"
#include "port.h"
#include "autoptr.h"
#include "dbusport.h"


class CProxyFdoDriver : public CPortDriver
{

    public:
    typedef CPortDriver super;

    CProxyFdoDriver( const IConfigDb* pCfg );

	gint32 Probe(
        IPort* pLowerPort,
        PortPtr& pNewPort,
        const IConfigDb* pConfig = NULL );

    // create the fdo bus port, one instance
    // for now
    gint32 CreatePort(
        PortPtr& pNewPort,
        const IConfigDb* pConfig = NULL);

    virtual gint32 OnEvent( EnumEventId iEvent,
        LONGWORD dwParam1 = 0,
        LONGWORD dwParam2 = 0,
        LONGWORD* pData = NULL  )
    { return -ENOTSUP; }
};

