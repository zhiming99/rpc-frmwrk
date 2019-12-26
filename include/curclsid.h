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
// rpc-frmwrk components
#define CLSIDS_ALLOCATED    ( clsid( ClassFactoryStart ) + 20 )
#define PROPIDS_ALLOCATED   ( propReservedEnd + 100 )

// iid allocated
#define IID_ALLOCATED    ( clsid( ReservedIidStart ) + 210 )

// for non-rpc-frmwrk components, please refer to
// test/helloworld/ifsvr.h as an example
#define USER_CLSIDS_ALLOCATED    ( clsid( UserClsidStart  ) + 1000 )
