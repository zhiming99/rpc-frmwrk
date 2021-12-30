/*
 * =====================================================================================
 *
 *       Filename:  curclsid.h
 *
 *    Description:  To record the current max number of clsids allocated 
 *
 *        Version:  1.0
 *        Created:  12/20/2019 07:49:59 PM
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
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */

#include "clsids.h"
#include "propids.h"

// please increase the numbers of the following
// macro when you have added new clsid's or
// propids, which serve as a start point for
// future addition.
//

namespace rpcf
{

// rpc-frmwrk components, last allocation was from
// ridl/astnode.h
#define CLSIDS_ALLOCATED    ( clsid( ClassFactoryStart ) + 80 )

// last allocation was from proxy.i
#define PROPIDS_ALLOCATED   ( propReservedEnd + 130 )

// iid allocated by rpc/security/secclsid
#define IID_ALLOCATED    ( clsid( ReservedIidStart ) + 220 )

// for non-rpc-frmwrk components, please refer to
// test/helloworld/ifsvr.h as an example
// last allocation of 10 slots from examples/cpp/sftest/transctx.h
#define USER_CLSIDS_ALLOCATED    ( clsid( UserClsidStart  ) + 1010 )

}
