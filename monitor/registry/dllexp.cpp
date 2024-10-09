/*
 * =====================================================================================
 *
 *       Filename:  dllexp.cpp
 *
 *    Description:  implementation of the DllLoadFactory for libregfs.so
 *
 *        Version:  1.0
 *        Created:  10/09/2024 09:52:00 AM
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
 *
 * =====================================================================================
 */

#include "blkalloc.h"

namespace rpcf
{

// mandatory part, just copy/paste'd from clsids.cpp
static FactoryPtr InitClassFactory()
{
    BEGIN_FACTORY_MAPS;

    INIT_MAP_ENTRYCFG( CRegistryFs );
    INIT_MAP_ENTRYCFG( COpenFileEntry );
    INIT_MAP_ENTRYCFG( CDirFileEntry );
    INIT_MAP_ENTRYCFG( CLinkFileEntry );
    INIT_MAP_ENTRYCFG( CFileImage );
    INIT_MAP_ENTRYCFG( CDirImage );
    INIT_MAP_ENTRYCFG( CLinkImage );

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
