/*
 * =====================================================================================
 *
 *       Filename:  genrpcfs.cpp
 *
 *    Description:  implentations of classes and functions for rpcfs programming 
 *
 *        Version:  1.0
 *        Created:  10/08/2022 09:49:25 PM
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "rpc.h"

using namespace rpcf;
#include "astnode.h" 
#include "ridlc.h"

#include "gencpp.h"
#include "rpcfs/genpy2.h"

extern stdstr g_strLang;
gint32 GenPyProj2(
    const std::string& strOutPath,
    const std::string& strAppName,
    ObjPtr pRoot );

gint32 GenRpcFSkelton(
    const std::string& strOutPath,
    const std::string& strAppName,
    ObjPtr pRoot )
{
    gint32 ret = -ENOTSUP;
    if( g_strLang == "py" )
    {
        ret = GenPyProj2( strOutPath,
            strAppName, pRoot );
    }
    return ret;
}

