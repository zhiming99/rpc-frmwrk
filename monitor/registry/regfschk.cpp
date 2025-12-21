/*
 * =====================================================================================
 *
 *       Filename:  regfschk.cpp
 *
 *    Description:  a regfs tool the check and repair the errors and
 *                  remove the corrupted content from the regfs.
 *
 *        Version:  1.0
 *        Created:  12/16/2025 01:39:30 PM
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi(woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2024 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#ifdef __FreeBSD__
#include <sys/socket.h>
#include <sys/un.h>
#endif
#include <sys/time.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif
#include "rpc.h"
using namespace rpcf;
#include "blkalloc.h"

extern EnumLogLvl rpcf::g_dwLogLevel;
static guint64 g_qwSize = 0;
static RegFsPtr g_pRegfs;
static stdstr g_strRegFsFile;
static bool g_bVerbose = false;

gint32 InitContext()
{
    gint32 ret = CoInitialize( 0 );
    if( ERROR( ret ) )
        return ret;
    do{
        stdstr strPath;
        ret = GetLibPath( strPath );
        if( ERROR( ret ) )
            break;

        strPath += "/libregfs.so";
        ret = CoLoadClassFactory(
            strPath.c_str() );

    }while( 0 );

    if( ERROR( ret ) )
    {
        CoUninitialize();
    }
    return ret;
}

gint32 DestroyContext()
{
    CoUninitialize();
    DebugPrintEx( logErr, 0,
        "#Leaked objects is %d",
        CObjBase::GetActCount() );
    return STATUS_SUCCESS;
}

gint32 ScanBlocks( RegFsPtr& pFs )
{
    return 0;
}

gint32 GetPathFromImg( FImgSPtr& pFile,
    stdstr& strPath )
{
    if( pFile.IsEmpty() )
        return -EINVAL;
    gint32 ret = 0;
    CFileImage* pNode = pFile;
    do{
        stdstr strName = pNode->GetName();
        if( strName == "/" )
        {
            strPath = strName + strPath;
            break;
        }
        else if( strPath.size() )
            strPath = strName + "/" + strPath;
        else
            strPath = strName;
        pNode = pNode->GetParentDir();
        if( pNode == nullptr )
            break;
    }while( 1 );
    return ret;
}

typedef enum {
    errOpen = 1,
    errSize,
    errDirEnt,
    errLoad,
    errInode,
    errChild,
} EnumErrType;

static stdstr GetErrorString( EnumErrType iErr )
{
    switch( iErr )
    {
    case errOpen:
        return stdstr( "error open" );
    case errSize:
        return stdstr( "error size" );
    case errDirEnt:
        return stdstr( "error dirent" );
    case errLoad:
        return stdstr( "error load object" );
    case errInode:
        return stdstr( "error inode" );
    case errChild:
        return stdstr( "error child" );
    }
    return stdstr( "none" );
}

gint32 CheckInode( RegFsPtr& pFs,
    FImgSPtr pFile )
{
    return 0;
}

using ELEM_BADFILE=std::tuple< stdstr, guint32, gint32 >;
using ELEM_GOODFILE=std::pair< stdstr, guint32 >;

gint32 FindBadFilesDir( RegFsPtr& pFs,
    FImgSPtr ptrDir, RFHANDLE hDir,
    std::vector< ELEM_BADFILE >& vecBadFiles,
    std::vector< ELEM_GOODFILE  >& vecGoodFiles )
{
    gint32 ret = 0;
    if( ptrDir.IsEmpty() )
        return -EINVAL;
    stdstr strPath;
    GetPathFromImg( ptrDir, strPath );
    CDirImage* pDir = ptrDir;
    bool bCorrupted = false;
    std::vector< stdstr > vecNames;
    pFs->Namei( strPath, vecNames );

    do{
        std::vector< KEYPTR_SLOT > vecDirEnt;
        ret = pDir->ListDir( vecDirEnt );

        if( ERROR( ret ) )
            bCorrupted = true;
        if( bCorrupted && vecDirEnt.empty() )
        {
            if( strPath.size() )
                OutputMsg( ret,
                    "Error list directory %s",
                    strPath.c_str() );
            else
                OutputMsg( ret,
                    "Error list directory" );
            vecBadFiles.push_back(
                { strPath, errDirEnt, ftDirectory} );
            break;
        }

        stdstr strIndent;
        strIndent.insert( 0, vecNames.size() * 4, ' ' );
        for( auto& ks : vecDirEnt )
        {
            if( ks.oLeaf.byFileType != ftDirectory )
                continue;
            if( g_bVerbose )
                OutputMsg( 0, "%s%s",
                    strIndent.c_str(), ks.szKey );
            if( strnlen( ks.szKey, REGFS_NAME_LENGTH ) == 0 )
            {
                stdstr strName = strPath + "/unkdir_";
                if( strPath == "/" )
                    strName.erase( 0, 1 );

                strName += std::to_string(
                    ks.oLeaf.dwInodeIdx );

                EnumErrType iErr = errOpen;
                if( ks.oLeaf.dwInodeIdx == 0 )
                    iErr = errDirEnt;

                vecBadFiles.push_back( { strName,
                    iErr, ks.oLeaf.byFileType} );
                continue;
            }

            stdstr strChild = strPath + "/" + ks.szKey;
            if( strPath == "/" )
                strChild.erase( 0, 1 );

            FImgSPtr pChildDir;
            ret = pDir->Search( ks.szKey, pChildDir );
            if( ERROR( ret ) )
            {
                // name is damaged
                vecBadFiles.push_back( { strChild,
                    errDirEnt, ks.oLeaf.byFileType} );
                continue;
            }

            RFHANDLE hChildDir = INVALID_HANDLE;
            ret = pFs->OpenDir( strChild,
                O_RDONLY, hChildDir, nullptr );
            if( ERROR( ret ) )
            {
                vecBadFiles.push_back( { strChild,
                    errOpen, ks.oLeaf.byFileType} );
                bCorrupted = true;
                continue;
            }
            CFileHandle ofh( pFs, hChildDir );
            ret = pFs->GetDirImage(
                hChildDir, pChildDir );
            if( ERROR( ret ) )
            {
                vecBadFiles.push_back( {strChild,
                    errInode, ks.oLeaf.byFileType} );
                bCorrupted = true;
                continue;
            }
            FindBadFilesDir( pFs,
                pChildDir, hChildDir, vecBadFiles,
                vecGoodFiles );
        }

        for( auto& ks : vecDirEnt )
        {
            if( ks.oLeaf.byFileType == ftDirectory )
                continue;
            if( strnlen( ks.szKey, REGFS_NAME_LENGTH ) == 0 )
            {
                stdstr strName = strPath + "/unkfile_";
                if( strPath == "/" )
                    strName.erase( 0, 1 );

                strName += std::to_string(
                    ks.oLeaf.dwInodeIdx );

                EnumErrType iErr = errOpen;
                if( ks.oLeaf.dwInodeIdx == 0 )
                    iErr = errDirEnt;

                vecBadFiles.push_back( { strName,
                    iErr, ks.oLeaf.byFileType} );
                bCorrupted = true;
                continue;
            }

            stdstr strFile = strPath + "/" + ks.szKey;
            if( strPath == "/" )
                strFile.erase( 0, 1 );

            FImgSPtr pFile;
            ret = pDir->Search( ks.szKey, pFile );
            if( ERROR( ret ) )
            {
                // name is damaged
                vecBadFiles.push_back( { strFile,
                    errDirEnt, ks.oLeaf.byFileType} );
                continue;
            }
            
            ret = pFs->GetFileImage( hDir, ks.szKey,
                pFile, 0, nullptr );
            if( ERROR( ret ) )
            {
                vecBadFiles.push_back( {strFile,
                    errInode, ks.oLeaf.byFileType} );
                bCorrupted = true;
                continue;
            }
            struct stat stBuf;
            pFile->GetAttr( stBuf );
            if( stBuf.st_size >= MAX_FILE_SIZE )
            {
                vecBadFiles.push_back( { strFile,
                    errSize, ks.oLeaf.byFileType} );
            }
            if( g_bVerbose )
                OutputMsg( stBuf.st_size, "%s%s",
                    strIndent.c_str(), ks.szKey );
            vecGoodFiles.push_back( { strFile,
                ks.oLeaf.byFileType } );
        }
        if( bCorrupted )
        {
            vecBadFiles.push_back(
                { strPath, errChild, ftDirectory } );
        }
        else
        {
            vecGoodFiles.push_back(
                { strPath, ftDirectory } );
        }
                
    }while( 0 );

    if( bCorrupted )
    {
        OutputMsg( 0, "Corrupted dir %s", strPath.c_str() );
        pDir->PrintBNode();
    }
    return ret;
}

gint32 FindBadFiles( RegFsPtr& pFs,
    std::vector< ELEM_BADFILE >& vecBadFiles,
    std::vector< ELEM_GOODFILE  >& vecGoodFiles )
{
    gint32 ret = 0;
    if( pFs.IsEmpty() ) 
        return -EINVAL;
    do{
        RFHANDLE hRoot = INVALID_HANDLE;
        ret = pFs->OpenDir(
            "/", O_RDONLY, hRoot, nullptr );
        if( ERROR( ret ) )
        {
            OutputMsg( ret,
                "Error unable to open "
                "root directory" );
            break;
        }
        CFileHandle orh( pFs, hRoot );
        FImgSPtr pRootDir;
        ret = pFs->GetDirImage(
            hRoot, pRootDir );
        if( ERROR( ret ) )
            break;
        ret = FindBadFilesDir( pFs,
            pRootDir, hRoot,
            vecBadFiles, vecGoodFiles );
        if( vecBadFiles.empty() )
        {
            OutputMsg( ret,
                "Nothing bad found" );
            break;
        }

        OutputMsg( vecBadFiles.size(),
            "Found bad files:" );

        for( auto& elem : vecBadFiles )
        {
            stdstr strError = GetErrorString(
                ( EnumErrType )std::get<1>(elem) );
            auto iType = std::get<2>(elem);
            stdstr strType;
            if( iType == ftDirectory )
                strType = "dir";
            else if( iType == ftLink )
                strType = "link";
            else if( iType == ftRegular )
                strType = "file";
            else
                strType = stdstr( "unk" ) +
                    std::to_string( iType );
            stdstr strPath =
                std::get<0>(elem).c_str();
            OutputMsg( 0, "%s: (%s)%s",
                strPath.c_str(),
                strType.c_str(), strError.c_str() );
        }

        OutputMsg( 0,
            "Good files: %d", vecGoodFiles.size() );
        if( g_bVerbose )
        {
            for( auto& elem : vecGoodFiles )
            {
                stdstr strType;
                auto iType = elem.second;
                if( iType == ftDirectory )
                    strType = "dir";
                else if( iType == ftLink )
                    strType = "link";
                else if( iType == ftRegular )
                    strType = "file";
                else
                    strType = stdstr( "unk" ) +
                        std::to_string( iType );

                auto& strPath = elem.first;
                OutputMsg( 0, "%s: %s",
                    strPath.c_str(), strType.c_str() );
            }
        }

    }while( 0 );
    return ret;
}

gint32 BuildBadTree( RegFsPtr& pFs,
    const std::vector< ELEM_BADFILE >& vecBadFiles,
    std::map< stdstr, EnumErrType >& map )
{
    return 0;
}

gint32 RemoveBadDir( RegFsPtr& pFs,
    const std::vector< ELEM_BADFILE >& vecBadFiles )
{
    return 0;    
}

gint32 ClaimOrphanBlocks( RegFsPtr& pFs )
{
    return 0;
}

static void Usage( const char* szName )
{
    fprintf( stderr,
        "Usage: %s [OPTIONS] <regfs path> [<mount point>]\n"
        "\t [ <regfs path> the path to the file containing regfs  ]\n"
        "\t [ -h this help ]\n", szName );
}

int _main()
{
    gint32 ret = 0;
    do{ 
        rpcf::g_dwLogLevel = logEmerg;
        CParamList oParams;
        if( g_strRegFsFile.empty() )
        {
            ret = -EINVAL;
            OutputMsg( ret, "Error no "
                "registry path specified" );
            Usage( "regfsmnt" );
            break;
        }
        oParams.Push( false );
        oParams.SetStrProp(
            propConfigPath, g_strRegFsFile );
        ret = g_pRegfs.NewObj(
            clsid( CRegistryFs ),
            oParams.GetCfg() );
        if( ERROR( ret ) )
            break;

        CRegistryFs* pFs = g_pRegfs;
        ret = pFs->Start();
        if( ERROR( ret ) )
            break;
        std::vector< ELEM_BADFILE > vecBadFiles;
        std::vector< ELEM_GOODFILE > vecGoodFiles;
        FindBadFiles( g_pRegfs, vecBadFiles, vecGoodFiles );
        pFs->Stop();
        g_pRegfs.Clear();

    }while( 0 );
    if( ERROR( ret ) )
    {
        OutputMsg( ret, "Warning, regfs "
            "quitting with error" );
    }

    return ret;
}

int main( int argc, char** argv)
{
    bool bDaemon = false;
    bool bDebug = false;
    bool bKernelCheck = false;
    int opt = 0;
    int ret = 0;
    do{
        while( ( opt = getopt( argc, argv, "hv" ) ) != -1 )
        {
            switch( opt )
            {
                case 'v':
                    g_bVerbose = true;
                    break;
                case 'h':
                default:
                    { Usage( argv[ 0 ] ); exit( 0 ); }
            }
        }
        if( ERROR( ret ) )
            break;

        if( optind >= argc )
        {
            ret = -EINVAL;
            OutputMsg( ret, "Error missing "
                "'regfs path' and 'mount point'" );
            break;
        }

        g_strRegFsFile = argv[ optind ];
        if( g_strRegFsFile.size() > REG_MAX_PATH - 1 )
        {
            OutputMsg( ret,
                "Error file name too long" );
            ret = -ENAMETOOLONG;
            break;
        }

        ret = access( g_strRegFsFile.c_str(),
            R_OK | W_OK );
        if( ret == -1 )
        {
            ret = -errno;
            OutputMsg( ret,
                "Error invalid registry file" );
            break;
        }
        struct stat st;
        ret = stat( g_strRegFsFile.c_str(), &st );
        if( ERROR( ret ) )
        {
            ret = -errno;
            break;
        }
        g_qwSize = st.st_size;
        if( g_qwSize > MAX_FS_SIZE ) 
        {
            OutputMsg( -ERANGE,
                "Warning, found invalid filesystem size" );
        }

        ret = InitContext();
        if( ERROR( ret ) ) 
        {
            DestroyContext();
            OutputMsg( ret,
                "Error failed to start regfschk, "
                "abort..." );
            break;
        }
        ret = _main();
        DestroyContext();
    }while( 0 );
    return ret;
}
