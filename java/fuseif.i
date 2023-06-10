/*
 * =====================================================================================
 *
 *       Filename:  fuseif.i
 *
 *    Description:  the swig file with helper functions to interface with rpcfs.
 *  
 *        Version:  1.0
 *        Created:  06/09/2023 12:09:30
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2021 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */ 

%{
#include "fuseif.h"
std::set< guint32 > g_setMsgIds;
gint32 AddFilesAndDirs( ObjPtr& pRt )
{
    gint32 ret = 0;
    do{
        stdstr strPath;
        ret = GetLibPath(
            strPath, "libipc.so" );
        if( ERROR( ret ) )
            break;

        // we do this because we don't want to staticly
        // link the so with librpc.so
        strPath += "/rpcf/librtfiles.so";
        void* handle = dlopen( strPath.c_str(),
            RTLD_NOW | RTLD_GLOBAL );
        if( handle == nullptr )
        {
            ret = -ENOENT;
            DebugPrintEx( logErr, ret,
                "Error librpc.so not found" );
            break;
        }
        auto func = ( gint32 (*)(bool, cpp::CRpcServices*) )
            dlsym( handle, "AddFilesAndDirs" );

        if( func == nullptr )
        {
            ret = -ENOENT;
            break;
        }
        ret = func( false, pRt );

    }while( 0 );
    return ret; 
}

gint32 fuseif_mainloop(
    const std::string& strAppName,
    const std::string& strMPoint )
{
    gint32 ret = 0;
    char* pMem = nullptr;
    do{
        if( strMPoint.empty() )
        {
            ret = -EINVAL;
            break;
        }
        ret = access( strMPoint.c_str(), R_OK );
        if( ret < 0 )
        {
            ret = -errno;
            break;
        }

        std::vector< std::string > strArgv;
        strArgv.push_back( strAppName );
        strArgv.push_back( "-f" );
        strArgv.push_back( strMPoint );
        
        size_t argcf = 3;
        char* argvf[ argcf ];
        size_t dwSize = 0;
        for( size_t i = 0; i < argcf; i++ )
        {    
            argvf[ i ] = ( char* )dwSize;
            dwSize += strArgv[ i ].size() + 1;
        }    
        if( dwSize > 1024 )
        {
            ret = -EINVAL;
            break;
        }    
        pMem = ( char* )malloc( dwSize );
        if( pMem == nullptr )
        {    
            ret = -ENOMEM;
            break;
        }
        for( size_t i = 0; i < argcf; i++ )
        {
            argvf[ i ] += ( intptr_t )pMem;
            strcpy( argvf[ i ],
                strArgv[ i ].c_str() );
        }

        int argcfi = argcf;
        fuse_args args = FUSE_ARGS_INIT(argcfi, argvf);
        fuse_cmdline_opts opts;
        ret = fuseif_daemonize( args, opts, argcf, argvf );
        if( ERROR( ret ) )
            break;

        ret = InitRootIf( g_pIoMgr, false );
        if( ERROR( ret ) )
            break;
        ret = AddFilesAndDirs( g_pRouter );
        if( ERROR( ret ) )
            break;

        args = FUSE_ARGS_INIT(argcfi, argvf);
        ret = fuseif_main( args, opts );

    }while( 0 );
    if( pMem != nullptr )
    {
        free( pMem );
        pMem = nullptr;
    }
    // Stop the root object
    InterfPtr pRoot = GetRootIf();
    if( !pRoot.IsEmpty() )
        pRoot->Stop();
    ReleaseRootIf();
    return ret;
}

%}

gint32 fuseif_mainloop(
    const std::string& strAppName,
    const std::string& strMPoint );
