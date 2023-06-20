%ModuleHeaderCode
#pragma once
#include "../fuse/fuseif.h"
extern ObjPtr g_pRouter;
extern gint32 AddFilesAndDirs( ObjPtr& pRt );
%End

%ModuleCode

gint32 AddSvcStatFilesInner(
    CRpcServices* pIf,
    const std::string& strName )
{
    gint32 ret = 0;
    do{
        CFuseRootServer* pSvr = GetRootIf();
        if( pSvr == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        CFuseObjBase* pList =
            new CFuseSvcStat( nullptr );
        pList->SetMode( S_IRUSR );
        pList->DecRef();
        stdstr strFile = strName + "_SvcStat";
        pList->SetName( strFile );
        ObjPtr pObj = pIf;
        pList->SetUserObj( pObj );
        auto pEnt = DIR_SPTR( pList );
        ROOTLK_EXCLUSIVE;
        pSvr->Add2UserDir( pEnt );

    }while( 0 );
    return ret;
}

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
%End

gint32 fuseif_mainloop(
    std::string& szAppName, std::string& szMPoint );
%MethodCode
    gint32 ret = 0;
    Py_BEGIN_ALLOW_THREADS;
    char* pMem = nullptr;
    do{
        if( a1 == nullptr )
        {
            ret = -EINVAL;
            break;
        }
        ret = access( ( *a1 ).c_str(), R_OK );
        if( ret < 0 )
        {
            ret = -errno;
            break;
        }

        std::vector< std::string > strArgv;
        strArgv.push_back( *a0 );
        strArgv.push_back( "-f" );
        strArgv.push_back( *a1 );
        
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
    sipRes = ret;
    Py_END_ALLOW_THREADS;
%End

gint32 AddSvcStatFiles(
    cpp::ObjPtr* pIf,
    const std::string& strName );
%MethodCode
    Py_BEGIN_ALLOW_THREADS;
    do{
        CRpcServices* pSvc = *a0;
        if( pSvc == nullptr ) 
        {
            sipRes = -EFAULT;
            break;
        }
        sipRes = AddSvcStatFilesInner(
            pSvc, strName );
    }while( 0 );
    Py_END_ALLOW_THREADS;
%End
