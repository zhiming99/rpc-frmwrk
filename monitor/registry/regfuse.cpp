/*
 * =====================================================================================
 *
 *       Filename:  regfuse.cpp
 *
 *    Description:  a FUSE wrapper for registry file system 
 *
 *        Version:  1.0
 *        Created:  10/12/2024 09:18:34 AM
 *       Revision:  none
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
#define FUSE_USE_VERSION FUSE_VERSION
#if BUILD_64 == 0
#define _FILE_OFFSET_BITS 64
#endif

//#define _GNU_SOURCE

#ifdef linux
/* For pread()/pwrite()/utimensat() */
#define _XOPEN_SOURCE 700
#endif

#include <fuse.h>
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

#define XATTR_NAME  "user.regfs"
static RegFsPtr g_pRegfs;
static int fill_dir_plus = 0;
static std::string g_strMPoint;
static std::string g_strRegFsFile;
static bool g_bFormat = false;

ObjPtr g_pIoMgr;
std::set< guint32 > g_setMsgIds;
static bool g_bLogging = false;
static bool g_bLocal = false;


static void Usage( const char* szName );
static void *regfs_init(struct fuse_conn_info *conn,
		      struct fuse_config *cfg)
{
	(void) conn;
	cfg->use_ino = 1;

	/* Pick up changes from lower filesystem right away. This is
	   also necessary for better hardlink support. When the kernel
	   calls the unlink() handler, it does not know the inode of
	   the to-be-removed entry and can therefore not invalidate
	   the cache of the associated inode - resulting in an
	   incorrect st_nlink value being reported for any remaining
	   hardlinks to this inode. */
	cfg->entry_timeout = 0;
	cfg->attr_timeout = 0;
	cfg->negative_timeout = 0;

	return NULL;
}

static int regfs_getattr(const char *path, struct stat *stbuf,
		       struct fuse_file_info *fi)
{
    gint32 ret = -EACCES;
    do{
        CRegistryFs* pfs = g_pRegfs;
        ret = pfs->GetAttr( path, *stbuf );

    }while( 0 );

    return ret;
}

static int regfs_statfs(const char *path, struct statvfs *stbuf)
{
	int res;

	res = statvfs(path, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int regfs_access(const char *path, int mask)
{
    gint32 ret = -EACCES;
    do{
        CRegistryFs* pfs = g_pRegfs;
        ret = pfs->Access( path, mask );

    }while( 0 );

    return ret;
}

static int regfs_readlink(
    const char *path, char *buf, size_t size)
{
    stdstr strPath = path;
    CRegistryFs* pfs = g_pRegfs;
    guint32 dwSize = size;
    return pfs->ReadLink( strPath, buf, dwSize );
}


static int regfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi,
		       enum fuse_readdir_flags flags)
{
	DIR *dp;
	struct dirent *de;

	(void) fi;
	(void) flags;
    gint32 ret = 0;
    do{
        stdstr strPath = path;
        CRegistryFs* pfs = g_pRegfs;

        bool bClose = false;
        RFHANDLE hFile = INVALID_HANDLE;
        if( fi && fi->fh )
            hFile = fi->fh;
        else
        {
            ret = pfs->OpenDir(
                strPath, R_OK, hFile, nullptr );
            if( ERROR( ret ) )
                break; 
            bClose = true;
        }

        std::vector< KEYPTR_SLOT > vecDirEnt;
        ret = pfs->ReadDir( hFile, vecDirEnt );
        if( ERROR( ret ) )
        {
            if( bClose )
                pfs->CloseFile( hFile );
            break;
        }

        off_t i = offset;
        for( ;i < vecDirEnt.size(); i++ )
        {
            auto& elem =  vecDirEnt[ i ];
            struct stat st;
            memset(&st, 0, sizeof(st));
            st.st_ino = elem.oLeaf.dwInodeIdx;
            if( elem.oLeaf.byFileType == ftRegular )
                st.st_mode = S_IFREG;
            else if( elem.oLeaf.byFileType == ftDirectory )
                st.st_mode = S_IFDIR;
            else if( elem.oLeaf.byFileType == ftLink )
                st.st_mode = S_IFLNK;
            if( filler( buf, elem.szKey, &st, 0,
                ( fuse_fill_dir_flags )0 ) )
                break;
        }
        if( bClose )
            pfs->CloseFile( hFile );

    }while( 0 );
	return ret;
}

static int regfs_releasedir(
    const char *path, struct fuse_file_info* fi )
{
    gint32 ret = 0;
    do{
        CRegistryFs* pfs = g_pRegfs;
        bool bClose = false;
        RFHANDLE hFile = INVALID_HANDLE;
        if( fi && fi->fh )
            hFile = fi->fh;
        else
        {
            ret = -EINVAL;
            break;
        }
        ret = pfs->CloseFile( hFile );

    }while( 0 );
	return ret;
}

static int regfs_mkdir(const char *path, mode_t mode)
{
    stdstr strPath = path;
    CRegistryFs* pfs = g_pRegfs;
    return  pfs->MakeDir( strPath, mode );
}

static int regfs_unlink(const char *path)
{
    stdstr strPath = path;
    CRegistryFs* pfs = g_pRegfs;
    if( strcmp( path, "/" ) == 0 )
    {
        DebugPrint( -EACCES, "Error, cannot "
            "remove root directory." );
        return -EACCES;
    }
    return pfs->RemoveFile( strPath );
}

static int regfs_rmdir(const char *path)
{
    stdstr strPath = path;
    CRegistryFs* pfs = g_pRegfs;
    if( strcmp( path, "/" ) == 0 )
    {
        DebugPrint( -EACCES, "Error, cannot "
            "remove root directory." );
        return -EACCES;
    }
    return  pfs->RemoveDir( strPath );
}

static int regfs_symlink(const char *from, const char *to)
{
    stdstr strFrom = from;
    stdstr strTo = to;
    CRegistryFs* pfs = g_pRegfs;
    return  pfs->SymLink( strFrom, strTo );
}

static int regfs_rename( const char *from,
    const char *to, unsigned int flags)
{
    if( flags )
        return -EINVAL;
    stdstr strFrom = from;
    stdstr strTo = to;
    CRegistryFs* pfs = g_pRegfs;
    return  pfs->Rename( strFrom, strTo );
}

static int regfs_chmod(const char *path, mode_t mode,
		     struct fuse_file_info *fi)
{
    gint32 ret = -EACCES;
    do{
        CRegistryFs* pfs = g_pRegfs;
        ret = pfs->Chmod( path, mode, nullptr );

    }while( 0 );

    return ret;
}

static int regfs_chown(const char *path, uid_t uid, gid_t gid,
		     struct fuse_file_info *fi)
{
    gint32 ret = -EACCES;
    do{
        CRegistryFs* pfs = g_pRegfs;
        ret = pfs->Chown(
            path, uid, gid, nullptr );

    }while( 0 );

    return ret;
}

static int regfs_truncate(const char *path, off_t size,
			struct fuse_file_info *fi)
{
    gint32 ret = 0;
    do{
        stdstr strPath = path;
        CRegistryFs* pfs = g_pRegfs;
        RFHANDLE hFile;
        if( fi != nullptr )
            ret = pfs->Truncate( fi->fh, size );
        else
        {
            ret = pfs->OpenFile( strPath,
                O_WRONLY, hFile, nullptr );
            if( ERROR( ret ) )
                break;
            ret = pfs->Truncate( hFile, size );
            pfs->CloseFile( hFile );
            if( ERROR( ret ) )
                break;
        }
    }while( 0 );

	return ret;
}

static int regfs_create(const char *path, mode_t mode,
    struct fuse_file_info *fi)
{
    gint32 ret = 0;
    do{
        stdstr strPath = path;
        CRegistryFs* pfs = g_pRegfs;
        RFHANDLE hFile;
        ret = pfs->CreateFile( strPath,
            mode, fi->flags, hFile, nullptr );
        if( ERROR( ret ) )
            break;
        fi->fh = hFile;

    }while( 0 );

	return ret;
}

static int regfs_open(const char *path,
    struct fuse_file_info *fi)
{
    gint32 ret = 0;
    do{
        stdstr strPath = path;
        CRegistryFs* pfs = g_pRegfs;
        RFHANDLE hFile;
        ret = pfs->OpenFile( strPath,
            fi->flags, hFile, nullptr );
        if( ERROR( ret ) )
            break;
        fi->fh = hFile;

    }while( 0 );

	return ret;
}

static int regfs_read(const char *path,
    char *buf, size_t size, off_t offset,
    struct fuse_file_info *fi)
{
    gint32 ret = 0;
    do{
        RFHANDLE hFile = fi->fh;
        if( hFile == INVALID_HANDLE )
        {
            ret = -EBADF;
            break;
        }
        CRegistryFs* pfs = g_pRegfs;
        guint32 dwSize = size;
        ret = pfs->ReadFile(
            hFile, buf, dwSize, offset );
        if( ERROR( ret ) )
            break;
        ret = dwSize;

    }while( 0 );

	return ret;
}

static int regfs_write(const char *path,
    const char *buf, size_t size,
	off_t offset, struct fuse_file_info *fi)
{
    gint32 ret = 0;
    do{
        RFHANDLE hFile = fi->fh;
        if( hFile == INVALID_HANDLE )
        {
            ret = -EBADF;
            break;
        }
        CRegistryFs* pfs = g_pRegfs;
        guint32 dwSize = size;
        ret = pfs->WriteFile(
            hFile, buf, dwSize, offset );
        if( SUCCEEDED( ret ) )
            ret = dwSize;

    }while( 0 );

	return ret;
}

static int regfs_flush( const char *path,
    struct fuse_file_info *fi )
{ return 0; }

static int regfs_release(const char *path, struct fuse_file_info *fi)
{
    gint32 ret = 0;
    do{
        RFHANDLE hFile = fi->fh;
        if( hFile == INVALID_HANDLE )
        {
            ret = -EBADF;
            break;
        }
        CRegistryFs* pfs = g_pRegfs;
        ret = pfs->CloseFile( hFile );

    }while( 0 );

	return ret;
}

static int regfs_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */
    gint32 ret = 0;
    do{
        RFHANDLE hFile = fi->fh;
        if( hFile == INVALID_HANDLE )
        {
            ret = -EBADF;
            break;
        }
        guint32 dwFlags = FLAG_FLUSH_DATA;
        CRegistryFs* pfs = g_pRegfs;
        READ_LOCK( pfs );
        FileSPtr pOpen;
        ret = pfs->GetOpenFile( hFile, pOpen );
        if( ERROR( ret ) )
            break;
        ret = pOpen->Flush( dwFlags );

    }while( 0 );

	return ret;
}

static int regfs_utimens( const char * path,
    const struct timespec tv[2],
    struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */
    gint32 ret = 0;
    do{
        CRegistryFs* pfs = g_pRegfs;
        RFHANDLE hFile;
        bool bClose = false;
        if( fi == nullptr ||
            fi->fh == INVALID_HANDLE )
        {
            stdstr strPath = path;
            ret = pfs->OpenFile( strPath,
                O_RDONLY, hFile, nullptr );
            if( ERROR( ret ) )
                break;
            bClose = true;
        }
        else
        {
            hFile = fi->fh;
        }
        READ_LOCK( pfs );
        FileSPtr pOpen;
        ret = pfs->GetOpenFile( hFile, pOpen );
        if( SUCCEEDED( ret ) )
            pOpen->SetTimes( tv );
        if( bClose )
            pfs->CloseFile( hFile );

    }while( 0 );

	return ret;
}

/* xattr operations are optional and can safely be left unimplemented */
static int regfs_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{
    gint32 ret = 0;
    do{
        if( strcmp( name, XATTR_NAME ) )
        {
            ret = -ENOTSUP;
            break;
        }
        if( size > VALUE_SIZE - 1 )
        {
            ret = -EOVERFLOW;
            break;
        }
        stdstr strPath = path;
        Variant oVar;
        stdstr strAttr( value, size );
        ret = oVar.DeserializeFromJson(
            strAttr.c_str() );
        if( ERROR( ret ) )
            break;
        g_pRegfs->SetValue( strPath, oVar );

    }while( 0 );
	return ret;
}

static int regfs_getxattr(const char *path, const char *name, char *value,
			size_t size)
{
    gint32 ret = 0;
    do{
        if( strcmp( name, XATTR_NAME ) )
        {
            ret = -ENOTSUP;
            break;
        }
        stdstr strPath = path;
        Variant oVar;
        ret = g_pRegfs->GetValue( strPath, oVar );
        if( ERROR( ret ) )
            break;

        stdstr strAttr;
        ret = oVar.SerializeToJson( strAttr );
        if( ERROR( ret ) )
            break;
        if( size == 0 )
        {
            ret = strAttr.size();
            break;
        }
        if( strAttr.size() < size )
            strcpy( value, strAttr.c_str() );
        else
            strncpy( value,
                strAttr.c_str(), size );
        ret = std::min( strAttr.size(), size );
    }while( 0 );
	return ret;
}

static int regfs_listxattr(const char *path, char *list, size_t size)
{
    size_t dwSize = strlen( XATTR_NAME );
    if( size == 0 )
        return dwSize + 1;
    if( dwSize < size )
        strcpy( list, XATTR_NAME );
    else
        strncpy( list, XATTR_NAME, size );
	return dwSize + 1;
}

static int regfs_removexattr(const char *path, const char *name)
{ return -ENOTSUP; }

static int regfs_opendir(
    const char* path, fuse_file_info* fi )
{
    gint32 ret = 0;
    do{
        stdstr strPath = path;
        CRegistryFs* pfs = g_pRegfs;
        RFHANDLE hFile;
        ret = pfs->OpenDir( strPath,
            fi->flags, hFile, nullptr );
        if( ERROR( ret ) )
            break;
        fi->fh = hFile;
    }while( 0 );
    return STATUS_SUCCESS;
}

static const struct fuse_operations regfs_oper = {
	.getattr	= regfs_getattr,
	.readlink	= regfs_readlink,
	.mkdir		= regfs_mkdir,
	.unlink		= regfs_unlink,
	.rmdir		= regfs_rmdir,
	.symlink	= regfs_symlink,
	.rename		= regfs_rename,
//	.link		= regfs_link,
	.chmod		= regfs_chmod,
	.chown		= regfs_chown,
	.truncate	= regfs_truncate,
	.open		= regfs_open,
	.read		= regfs_read,
	.write		= regfs_write,
//	.statfs		= regfs_statfs,
    .flush      = regfs_flush,
	.release	= regfs_release,
	.fsync		= regfs_fsync,
	.setxattr	= regfs_setxattr,
	.getxattr	= regfs_getxattr,
	.listxattr	= regfs_listxattr,
//    .removexattr = regfs_removexattr,
    .opendir    = regfs_opendir,
	.readdir	= regfs_readdir,
    .releasedir = regfs_releasedir,
	.init       = regfs_init,
	.access		= regfs_access,
	.create 	= regfs_create,
	.utimens 	= regfs_utimens,
};

gint32 InitContext()
{
    gint32 ret = CoInitialize( 0 );
    if( ERROR( ret ) )
        return ret;
    do{
        CParamList oParams;
        oParams.Push( "regfsmnt" );

        oParams[ propEnableLogging ] = g_bLogging;
        oParams[ propSearchLocal ] = g_bLocal;
        oParams[ propConfigPath ] =
            "invalidpath/driver.json";

        // adjust the thread number if necessary
        oParams[ propMaxIrpThrd ] = 0;
        oParams[ propMaxTaskThrd ] = 1;

        ret = g_pIoMgr.NewObj(
            clsid( CIoManager ),
            oParams.GetCfg() );
        if( ERROR( ret ) )
            break;

        IService* pSvc = g_pIoMgr;
        ret = pSvc->Start();
        if( ERROR( ret ) )
        {
            OutputMsg( ret,
                "Error start io manager" );
        }

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

int _main( int argc, char** argv)
{
    gint32 ret = 0;
    do{ 
        CParamList oParams;
        if( g_strRegFsFile.empty() )
        {
            ret = -EINVAL;
            OutputMsg( ret, "Error no "
                "registry path specified" );
            Usage( "regfsmnt" );
            break;
        }
        oParams.Push( g_bFormat );
        oParams.SetStrProp(
            propConfigPath, g_strRegFsFile );
        ret = g_pRegfs.NewObj(
            clsid( CRegistryFs ),
            oParams.GetCfg() );
        if( ERROR( ret ) )
            break;

        if( g_bFormat )
        {
            ret = g_pRegfs->Format();
            if( SUCCEEDED( ret ) )
            {
                ret = g_pRegfs->Flush();
                OutputMsg( ret, "Formatting "
                    "completed successfully" );
            }
        }
        else
        {
            ret = g_pRegfs->Start();
            if( SUCCEEDED( ret ) )
            {
                ret = fuse_main(
                    argc, argv, &regfs_oper,
                    ( CRegistryFs* )g_pRegfs );
                g_pRegfs->Stop();
            }
        }
        if( ret > 0 )
            ret = -ret;

        g_pRegfs.Clear();

    }while( 0 );
    if( ERROR( ret ) )
    {
        OutputMsg( ret, "Warning, regfs quitting "
            "with error" );
    }

    return ret;
}

static void Usage( const char* szName )
{
    fprintf( stderr,
        "Usage: %s [OPTIONS] <regfs path> [<mount point>]\n"
        "\t [ <regfs path> the path to the file containing regfs  ]\n"
        "\t [ <mount point> the directory to mount regfs ]\n"
        "\t [ -c to delegate the kernel for access check ]\n"
        "\t [ -d to run as a daemon ]\n"
        "\t [ -g send logs to log server ]\n"
        "\t [ -i FORMAT the regfs ]\n"
        "\t [ -u to enable fuse to dump debug information ]\n"
        "\t [ -h this help ]\n", szName );
}

int main( int argc, char** argv)
{
    bool bDaemon = false;
    bool bDebug = false;
    bool bKernelCheck = false;
    int opt = 0;
    int ret = 0;
    do{
        while( ( opt = getopt( argc, argv, "chgdiul" ) ) != -1 )
        {
            switch( opt )
            {
                case 'c':
                    { bKernelCheck = true; break; }
                case 'd':
                    { bDaemon = true; break; }
                case 'g':
                    { g_bLogging = true; break; }
                case 'i':
                    { g_bFormat = true; break; }
                case 'u':
                    { bDebug = true; break; }
                case 'l':
                    { g_bLocal = true; break; }
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
        if( ret == -1 && !g_bFormat )
        {
            ret = -errno;
            OutputMsg( ret,
                "Error invalid registry file" );
            break;
        }

        if( !g_bFormat )
        {
            optind++;
            if( optind >= argc )
            {
                ret = -EINVAL;
                OutputMsg( ret,
                    "Error missing 'mount point'" );
                Usage( argv[ 0 ] );
                break;
            }

            g_strMPoint = argv[ optind ];
            if( g_strMPoint.size() > REG_MAX_PATH - 1 )
            {
                ret = -ENAMETOOLONG;
                OutputMsg( ret,
                    "Error invalid mount point" );
                break;
            }

            struct stat sb;
            if( lstat( g_strMPoint.c_str(), &sb ) == -1 )
            if( ret == -1 )
            {
                ret = -EINVAL;
                OutputMsg( ret,
                    "Error mount path unaccessable" );
                Usage( argv[ 0 ] );
                ret = -errno;
                break;
            }
            mode_t iFlags =
                ( sb.st_mode & S_IFMT );

            if( iFlags != S_IFDIR )
            {
                ret = -EINVAL;
                OutputMsg( ret,
                    "Error mount point is not a "
                    "directory" );
                Usage( argv[ 0 ] );
                ret = -errno;
                break;
            }
        }
        // using fuse
        std::vector< std::string > strArgv;
        strArgv.push_back( argv[ 0 ] );
        strArgv.push_back( "-f" );
        if( bDebug )
            strArgv.push_back( "-d" );
        if( bKernelCheck )
        {
            strArgv.push_back( "-o" );
            strArgv.push_back( "default_permissions" );
        }
        strArgv.push_back( g_strMPoint );
        int argcf = strArgv.size();
        char* argvf[ 100 ];
        size_t dwCount = std::min(
            strArgv.size(),
            sizeof( argvf ) / sizeof( argvf[ 0 ] ) );
        BufPtr pArgBuf( true );
        size_t dwOff = 0;
        for( size_t i = 0; i < dwCount; i++ )
        {
            ret = pArgBuf->Append(
                strArgv[ i ].c_str(),
                strArgv[ i ].size() + 1 );
            if( ERROR( ret ) )
                break;
            argvf[ i ] = ( char* )dwOff;
            dwOff += strArgv[ i ].size() + 1;
        }
        if( ERROR( ret ) )
            break;
        for( size_t i = 0; i < dwCount; i++ )
            argvf[ i ] += ( intptr_t )pArgBuf->ptr();

        if( bDaemon )
        {
            ret = daemon( 1, 0 );
            if( ret < 0 )
            { ret = -errno; break; }
        }
        ret = InitContext();
        if( ERROR( ret ) ) 
        {
            DestroyContext();
            OutputMsg( ret, "Error start regfsmnt" );
            break;
        }
        ret = _main( argcf, argvf );
        pArgBuf.Clear();
        DestroyContext();
    }while( 0 );
    return ret;
}
