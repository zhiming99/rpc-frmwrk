/*
 * =====================================================================================
 *
 *       Filename:  dllexp.cpp
 *
 *    Description:  implementation of the class factory for gmssl support
 *
 *        Version:  1.0
 *        Created:  02/24/2023 05:00:05 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2023 Ming Zhi( woodhead99@gmail.com )
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
#include "ifhelper.h"
#include "dbusport.h"
#include <gmssl/rand.h>
#include <gmssl/x509.h>
#include <gmssl/error.h>
#include <gmssl/sm2.h>
#include <gmssl/sm3.h>
#include <gmssl/sm4.h>
#include <gmssl/pem.h>
#include <gmssl/tls.h>
#include <gmssl/digest.h>
#include <gmssl/gcm.h>
#include <gmssl/hmac.h>
#include <gmssl/hkdf.h>
#include <gmssl/mem.h>
#include "gmsfido.h"

namespace rpcf
{

// mandatory part, just copy/paste'd from clsids.cpp
static FactoryPtr InitClassFactory()
{
    BEGIN_FACTORY_MAPS;

    INIT_MAP_ENTRYCFG( CRpcGmSSLFido );
    INIT_MAP_ENTRYCFG( CRpcGmSSLFidoDrv );
    INIT_MAP_ENTRYCFG( CGmSSLHandshakeTask );
    INIT_MAP_ENTRYCFG( CGmSSLShutdownTask );

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
