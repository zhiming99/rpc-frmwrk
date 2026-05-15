/*
 * =====================================================================================
 *
 *       Filename:  stclsids.h
 *
 *    Description:  declarations of classids for ST parser classes 
 *
 *        Version:  1.0
 *        Created:  05/15/2026 11:11:38 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2026 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#pragma once
#include "rpc.h"
#include "objfctry.h"
namespace rpcf
{ 

enum EnumSTParserClsid
{
    DECL_CLSID( CStructDecl ) = clsid( ClassFactoryStart ) + 100,
    DECL_CLSID( CFuncDecl ),
    DECL_CLSID( CFuncBlockDecl ),
}

}