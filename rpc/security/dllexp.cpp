/*
 * =====================================================================================
 *
 *       Filename:  dllexp.cpp
 *
 *    Description:  class factories for security related objects 
 *
 *        Version:  1.0
 *        Created:  07/21/2020 08:46:39 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:  
 *
 *      Copyright:  2020 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */


#include "clsids.h"
#include "defines.h"
#include "autoptr.h"
#include "objfctry.h"
#include "security.h"
#include "secfido.h"
#include "k5server.h"
#include "k5proxy.h"
#include "kdcfdo.h"

namespace rpcf
{
// mandatory part, just copy/paste'd from clsids.cpp
static FactoryPtr InitClassFactory()
{
    BEGIN_FACTORY_MAPS;

    INIT_MAP_ENTRYCFG( CRpcReqForwarderProxyAuthImpl );
    INIT_MAP_ENTRYCFG( CRpcTcpBridgeProxyAuthImpl );
    INIT_MAP_ENTRYCFG( CRpcReqForwarderAuthImpl );
    INIT_MAP_ENTRYCFG( CRpcTcpBridgeAuthImpl );
    INIT_MAP_ENTRYCFG( CRpcRouterReqFwdrAuthImpl );
    INIT_MAP_ENTRYCFG( CRpcRouterBridgeAuthImpl );
    INIT_MAP_ENTRYCFG( CAuthentProxyK5Impl );
    INIT_MAP_ENTRYCFG( CKdcChannelProxy );
    INIT_MAP_ENTRYCFG( CK5AuthServer );
    INIT_MAP_ENTRYCFG( CKdcRelayProxy );
    INIT_MAP_ENTRYCFG( CRpcSecFidoDrv );
    INIT_MAP_ENTRYCFG( CRpcSecFido );
    INIT_MAP_ENTRYCFG( CKdcRelayFdoDrv );
    INIT_MAP_ENTRYCFG( CKdcRelayFdo );
    INIT_MAP_ENTRYCFG( CKdcRelayPdo );
    INIT_MAP_ENTRYCFG( CKrb5InitHook );
    INIT_MAP_ENTRYCFG( CRemoteProxyStateAuth );
    INIT_MAP_ENTRYCFG( CKdcRelayProxyStat );

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
