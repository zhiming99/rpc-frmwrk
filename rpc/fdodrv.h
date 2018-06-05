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

};

