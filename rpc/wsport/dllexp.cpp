/*
 * =====================================================================================
 *
 *       Filename:  dllexp.cpp
 *
 *    Description:  implementation of the DllLoadFactory for libsslpt.so
 *
 *        Version:  1.0
 *        Created:  11/26/2019 08:19:10 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2019 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation.
 * =====================================================================================
 */

#include "clsids.h"
#include "defines.h"
#include "autoptr.h"
#include "objfctry.h"
#include "ifhelper.h"
#include "dbusport.h"
#include "wsfido.h"

namespace rpcf
{

// mandatory part, just copy/paste'd from clsids.cpp
static FactoryPtr InitClassFactory()
{
    BEGIN_FACTORY_MAPS;

    INIT_MAP_ENTRYCFG( CRpcWebSockFido );
    INIT_MAP_ENTRYCFG( CRpcWebSockFidoDrv );
    INIT_MAP_ENTRYCFG( CWsHandshakeTask );
    INIT_MAP_ENTRYCFG( CWsCloseTask );
    INIT_MAP_ENTRYCFG( CWsPingPongTask );

    END_FACTORY_MAPS;
};

}

using namespace rpcf;
// common method for a class factory library
extern "C"
gint32 DllLoadFactory( FactoryPtr& pFactory )
{
    pFactory = InitClassFactory();
    if( pFactory.IsEmpty() )
        return -EFAULT;

    return 0;
}
