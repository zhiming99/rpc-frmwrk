%ModuleHeaderCode
#pragma once
#include "../fuse/fuseif.h"
extern ObjPtr g_pRouter;
extern gint32 AddFilesAndDirs( ObjPtr& pRt );
%End

%ModuleCode

static std::vector< std::pair< ObjPtr, stdstr > > s_vecIfs;
gint32 AddSvcStatFiles()
{
    gint32 ret = 0;
    do{
        CFuseRootServer* pSvr = GetRootIf();
        if( pSvr == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        for( auto elem : s_vecIfs )
        {
            CFuseObjBase* pList =
                new CFuseSvcStat( nullptr );
            pList->SetMode( S_IRUSR );
            pList->DecRef();
            stdstr strFile =
                elem.second + "_SvcStat";
            pList->SetName( strFile );
            pList->SetUserObj( elem.first );
            auto pEnt = DIR_SPTR( pList );
            pSvr->Add2UserDir( pEnt );
        }
        s_vecIfs.clear();

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

        AddSvcStatFiles();

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

gint32 AddSvcStatFile(
    cpp::ObjPtr* pIf,
    const std::string& strName );
%MethodCode
    sipRes = 0;
    do{
        if( a0 == nullptr || a1 == nullptr )
        {
            sipRes = -EINVAL;
            break;
        }
        if( a0->IsEmpty() || a1->empty() )
        {
            sipRes = -EINVAL;
            break;
        }
        bool bDup = false;
        for( auto& elem : s_vecIfs )
        {
            if( *a1 == elem.second ||
                ( *a0 )->GetObjId() ==
                    elem.first->GetObjId() )
            {
                bDup = true;
                break;
            }
        }
        if( bDup )
        {
            sipRes = -EEXIST;
            break;
        }
        s_vecIfs.push_back( { *a0, *a1 } );

    }while( 0 );

%End
