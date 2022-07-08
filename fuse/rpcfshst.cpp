/*
 * =====================================================================================
 *
 *       Filename:  rpcfshst.cpp
 *
 *    Description:  implementation of an rpcfs host program to load shared
 *                  libraries of the 'service point'
 *
 *        Version:  1.0
 *        Created:  07/06/2022 11:07:17 PM
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
#include "rpc.h"
using namespace rpcf;

#include "fuseif.h"
#include "fuse_i.h"
#include <sys/ioctl.h>
#include <unistd.h>

ObjPtr g_pIoMgr;
std::set< guint32 > g_setMsgIds;

gint32 InitContext()
{
    gint32 ret = CoInitialize( 0 );
    if( ERROR( ret ) )
        return ret;
    do{
        CParamList oParams;
#ifdef SERVER
        oParams.Push( "hostsvr" );
#endif
#ifdef CLIENT
        oParams.Push( "hostcli" );
#endif

        // adjust the thread number if necessary
        oParams[ propMaxIrpThrd ] = 2;
        oParams[ propMaxTaskThrd ] = 2;

        ret = g_pIoMgr.NewObj(
            clsid( CIoManager ),
            oParams.GetCfg() );
        if( ERROR( ret ) )
            break;

        IService* pSvc = g_pIoMgr;
        ret = pSvc->Start();

    }while( 0 );

    if( ERROR( ret ) )
    {
        g_pIoMgr.Clear();
        CoUninitialize();
    }
    return ret;
}

gint32 DestroyContext()
{
    IService* pSvc = g_pIoMgr;
    if( pSvc != nullptr )
    {
        pSvc->Stop();
        g_pIoMgr.Clear();
    }

    CoUninitialize();
    DebugPrintEx( logErr, 0,
        "#Leaked objects is %d",
        CObjBase::GetActCount() );
    return STATUS_SUCCESS;
}

#ifdef CLIENT
bool bProxy = true;
#endif

#ifdef SERVER
bool bProxy = false;
#endif

int main( int argc, char** argv)
{
    gint32 ret = 0;
    do{ 
        fuse_args args = FUSE_ARGS_INIT(argc, argv);
        fuse_cmdline_opts opts;
        ret = fuseif_daemonize( args, opts, argc, argv );
        if( ERROR( ret ) ) 
            return ret;

        ret = InitContext();
        if( ERROR( ret ) ) 
            return ret;

        ret = InitRootIf( g_pIoMgr, bProxy );
        if( ERROR( ret ) ) 
            break;

        CRpcServices* pRoot = GetRootIf();
        args = FUSE_ARGS_INIT(argc, argv);
        ret = fuseif_main( args, opts );
     
        // Stop the root object
        pRoot->Stop();
        ReleaseRootIf();
     
    }while( 0 );

    DestroyContext();
    return ret;
}
