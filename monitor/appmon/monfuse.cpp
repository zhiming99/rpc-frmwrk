/*
 * =====================================================================================
 *
 *       Filename:  monfuse.cpp
 *
 *    Description:  the user management and application registry for the monitor 
 *
 *        Version:  1.0
 *        Created:  01/07/2025 22:31:30 PM
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

#include <fuse3/fuse.h>
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
#include <signal.h>
#include <stdlib.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif
#include "rpc.h"
using namespace rpcf;
#include "fastrpc.h"
#include "blkalloc.h"
#include "AppManagersvr.h"

#ifdef FUSE3

#define  ROOT_HANDLE  0x01000
#define  USER_INODE_OFFSET \
    ( BLKGRP_NUMBER_FULL * BLOCKS_PER_GROUP_FULL )

#define  USER_DIR   "usereg"
#define  APP_DIR    "appreg"

extern InterfPtr GetAppManager();
extern RegFsPtr g_pAppRegfs, g_pUserRegfs;

static void *monfs_init(struct fuse_conn_info *conn,
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

static int GetFs( const char* path,
    RegFsPtr& pFs, stdstr& strNewPath )
{
    constexpr guint32 u = sizeof( USER_DIR );
    constexpr guint32 a = sizeof( APP_DIR );
    if( strncmp( path, "/" APP_DIR, a ) == 0 )
    {
        if( path[ a ] != 0 )
            strNewPath = path + a;
        else
            strNewPath = "/";
        pFs = g_pAppRegfs;
        return 1;
    }
    else if( path[ 1 ] == 0 && path[0]=='/' )
        return 0;
    else if(
        strncmp( path, "/" USER_DIR, u ) == 0 )
    {
        if( path[ u ] != 0 )
            strNewPath = path + u;
        else
            strNewPath = "/";
        pFs = g_pUserRegfs;
        return 2;
    }
    return -1;
}

#define GETFS( path_ ) \
    stdstr strPath; \
    RegFsPtr pfs; \
    gint32 ifs = GetFs( path_, pfs, strPath ); \
    if( ifs < 0 ){ ret = -ENOENT; break; }

#define GETFS_HANDLE( handle_ ) \
    RegFsPtr pfs; \
    gint32 ifs; \
    ifs = GetFs( handle_, pfs ); \
    if( ifs < 0 ){ ret = -ENOENT; break; } \

static int GetFs( RFHANDLE hFile, RegFsPtr& pfs )
{
    if( hFile == INVALID_HANDLE )
        return -EINVAL;
    if( hFile == ROOT_HANDLE )
        return 0;
    if( g_pAppRegfs.IsEmpty() ||
        g_pUserRegfs.IsEmpty() )
        return ERROR_STATE;
    FileSPtr pFile;
    gint32 ret = g_pAppRegfs->GetFile(
        hFile, pFile );
    if( SUCCEEDED( ret ) )
    {
        pfs = g_pAppRegfs;
        return 1;
    }
    pfs = g_pUserRegfs;
    return 2;
}

static int monfs_getattr(const char *path,
    struct stat *stbuf,
	struct fuse_file_info *fi)
{
    gint32 ret = -EACCES;
    do{
        GETFS( path )
        if( ifs == 0 )
        {
            pfs = g_pAppRegfs;
            ret = pfs->GetAttr( "/", *stbuf );
            stbuf->st_uid = 0;
            stbuf->st_gid = GID_ADMIN;
            stbuf->st_mode = ( S_IFDIR | 0555 );
            stbuf->st_ino = 0;

            guint32 fs = BNODE_SIZE;
            guint32 bs = BLOCK_SIZE;
            stbuf->st_size = fs;
            stbuf->st_blksize = bs;
            stbuf->st_blocks = ( fs + bs - 1 ) / bs;
            break;
        }
        ret = pfs->GetAttr( strPath, *stbuf );
        if( strPath != "/" )
            break;

        if( ifs == 2 )
            stbuf->st_ino += MAX_FS_SIZE;
    }while( 0 );

    return ret;
}

static int monfs_access(const char *path, int mask)
{
    gint32 ret = 0;
    do{
        GETFS( path );
        if( ifs == 0 )
        {
            if( mask & W_OK )
            {
                ret = -EACCES;
                break;
            }
            if( mask == F_OK )
                break;
            break;
        }
        ret = pfs->Access( strPath, mask );

    }while( 0 );

    return ret;
}

static int monfs_readlink(
    const char *path, char *buf, size_t size)
{
    gint32 ret = 0;
    do{
        GETFS( path );
        if( ifs == 0 )
        {
            ret = -EINVAL;
            break;
        }
        guint32 dwSize = size;
        ret = pfs->ReadLink(
            strPath, buf, dwSize );
    }while( 0 );
    return ret;
}


static int monfs_readdir(const char *path,
    void *buf, fuse_fill_dir_t filler,
    off_t offset, struct fuse_file_info *fi,
	enum fuse_readdir_flags flags)
{
	(void) fi;
	(void) flags;
    gint32 ret = 0;
    do{
        bool bClose = false;
        RFHANDLE hFile = INVALID_HANDLE;
        RegFsPtr pfs_;
        gint32 ifs_;
        if( fi && fi->fh )
        {
            hFile = fi->fh;
            GETFS_HANDLE( hFile );
            ifs_ = ifs;
            if( ifs != 0 )
                pfs_ = pfs;
        }
        else
        {
            GETFS( path );
            ifs_ = ifs;
            if( ifs != 0 )
            {
                ret = pfs->OpenDir( strPath,
                    R_OK, hFile, nullptr );
                if( ERROR( ret ) )
                    break; 
                bClose = true;
                pfs_ = pfs;
            }
        }

        std::vector< KEYPTR_SLOT > vecDirEnt;
        if( ifs_ != 0 )
        {
            ret = pfs_->ReadDir( hFile, vecDirEnt );
            if( ERROR( ret ) )
            {
                if( bClose )
                    pfs_->CloseFile( hFile );
                break;
            }
        }
        else do
        {
            // prepare the content of root directory
            KEYPTR_SLOT ks;
            strcpy( ks.szKey, APP_DIR );
            struct stat stBuf;
            ret = g_pAppRegfs->GetAttr(
                "/", stBuf );
            if( ERROR( ret ) )
                break;
            ks.oLeaf.dwInodeIdx = stBuf.st_ino;
            ks.oLeaf.byFileType = ftDirectory;
            vecDirEnt.push_back( ks );

            ks.szKey[ 0 ] = 0;
            strcpy( ks.szKey, USER_DIR );
            ret = g_pUserRegfs->GetAttr(
                "/", stBuf );
            if( ERROR( ret ) )
                break;
            ks.oLeaf.dwInodeIdx = stBuf.st_ino +
                USER_INODE_OFFSET;
            ks.oLeaf.byFileType = ftDirectory;
            vecDirEnt.push_back( ks );

        }while( 0 );

        int i = offset;
        for( ;i < ( int )vecDirEnt.size(); i++ )
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
            pfs_->CloseFile( hFile );

    }while( 0 );
	return ret;
}

static int monfs_releasedir(
    const char *path, struct fuse_file_info* fi )
{
    gint32 ret = 0;
    do{
        RFHANDLE hFile = INVALID_HANDLE;
        if( fi && fi->fh )
        {
            hFile = fi->fh;
            GETFS_HANDLE( hFile );
            if( ifs == 0 )
                break;
            ret = pfs->CloseFile( hFile );
        }
        else
        {
            ret = -EINVAL;
            break;
        }

    }while( 0 );
	return ret;
}

static int monfs_mkdir(const char *path, mode_t mode)
{
    gint32 ret = 0;
    do{
        GETFS( path );
        if( ifs == 0 )
        {
            ret = -EACCES;
            break;
        }
        ret = pfs->MakeDir( strPath, mode );
    }while( 0 );
    return ret;
}

static int monfs_unlink(const char *path)
{
    gint32 ret = 0;
    do{
        if( strcmp( path, "/" ) == 0 ||
            strcmp( path, "/" USER_DIR ) == 0 ||
            strcmp( path, "/" APP_DIR ) == 0 )
        {
            DebugPrint( -EACCES, "Error, cannot "
                "remove root directory." );
            ret = -EACCES;
            break;
        }
        GETFS( path );
        ret = pfs->RemoveFile( strPath );
    }while( 0 );
    return ret;
}

static int monfs_rmdir(const char *path)
{
    gint32 ret = 0;
    do{
        if( strcmp( path, "/" ) == 0 ||
            strcmp( path, "/" USER_DIR ) == 0 ||
            strcmp( path, "/" APP_DIR ) == 0 )
        {
            DebugPrint( -EACCES, "Error, cannot "
                "remove root directory." );
            ret = -EACCES;
            break;
        }
        GETFS( path );
        ret =  pfs->RemoveDir( strPath );
    }while( 0 );
    return ret;
}

static int monfs_symlink(const char *from, const char *to)
{
    gint32 ret = 0;
    do{
        stdstr strFrom = from;
        stdstr strTo = to;
        GETFS( to );
        if( ifs == 0 )
        {
            ret = -EACCES;
            break;
        }
        ret = pfs->SymLink( strFrom, strPath );
    }while( 0 );
    return ret;
}

static int monfs_rename( const char *from,
    const char *to, unsigned int flags)
{
    gint32 ret = 0;
    if( flags )
        return -EINVAL;
    do{
        GETFS( from );
        if( ifs == 0 )
        {
            ret = -EACCES;
            break;
        }
        RegFsPtr pfs1 = pfs;
        stdstr strTo;
        if( true )
        {
            GETFS( to );
            strTo = strPath;
            if( pfs1 != pfs )
            {
                ret = -ENOTSUP;
                break;
            }
            if( ifs == 0 )
            {
                ret = -EACCES;
                break;
            }
        }
        ret = pfs1->Rename( strPath, strTo );
    }while( 0 );
    return ret;
}

static int monfs_chmod(const char *path, mode_t mode,
		     struct fuse_file_info *fi)
{
    gint32 ret = -EACCES;
    do{
        GETFS( path );
        if( ifs == 0 )
            break;
        ret = pfs->Chmod(
            strPath, mode, nullptr );
    }while( 0 );

    return ret;
}

static int monfs_chown(const char *path, uid_t uid, gid_t gid,
		     struct fuse_file_info *fi)
{
    gint32 ret = -EACCES;
    do{
        GETFS( path );
        if( ifs == 0 )
            break;
        ret = pfs->Chown( strPath,
            uid, gid, nullptr );
    }while( 0 );

    return ret;
}

static int monfs_truncate(const char *path, off_t size,
			struct fuse_file_info *fi)
{
    gint32 ret = 0;
    do{
        if( fi != nullptr )
        {
            GETFS_HANDLE( fi->fh );
            if( ifs == 0 )
            {
                ret = -EACCES;
                break;
            }
            ret = pfs->Truncate( fi->fh, size );
        }
        else
        {
            RFHANDLE hFile;
            GETFS( path );
            if( ifs == 0 )
            {
                ret = -EACCES;
                break;
            }
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

static int monfs_create(const char *path, mode_t mode,
    struct fuse_file_info *fi)
{
    gint32 ret = 0;
    do{
        GETFS( path );
        if( ifs == 0 )
        {
            ret = -EACCES;
            break;
        }
        RFHANDLE hFile;
        ret = pfs->CreateFile( strPath,
            mode, fi->flags, hFile, nullptr );
        if( ERROR( ret ) )
            break;
        fi->fh = hFile;

    }while( 0 );

	return ret;
}

static int monfs_open(const char *path,
    struct fuse_file_info *fi)
{
    gint32 ret = 0;
    do{
        GETFS( path );
        if( ifs == 0 )
        {
            ret = -EACCES;
            break;
        }
        RFHANDLE hFile;
        ret = pfs->OpenFile( strPath,
            fi->flags, hFile, nullptr );
        if( ERROR( ret ) )
            break;
        fi->fh = hFile;

    }while( 0 );

	return ret;
}

static int monfs_read(const char *path,
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
        GETFS_HANDLE( hFile );
        if( ifs == 0 )
        {
            ret = -EACCES;
            break;
        }
        guint32 dwSize = size;
        ret = pfs->ReadFile(
            hFile, buf, dwSize, offset );
        if( ERROR( ret ) )
            break;
        ret = dwSize;

    }while( 0 );

	return ret;
}

static int monfs_write(const char *path,
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
        GETFS_HANDLE( hFile );
        if( ifs == 0 )
        {
            ret = -EACCES;
            break;
        }
        guint32 dwSize = size;
        ret = pfs->WriteFile(
            hFile, buf, dwSize, offset );
        if( SUCCEEDED( ret ) )
            ret = dwSize;

    }while( 0 );

	return ret;
}

static int monfs_flush( const char *path,
    struct fuse_file_info *fi )
{ return 0; }

static int monfs_release(const char *path, struct fuse_file_info *fi)
{
    gint32 ret = 0;
    do{
        RFHANDLE hFile = fi->fh;
        if( hFile == INVALID_HANDLE )
        {
            ret = -EBADF;
            break;
        }
        GETFS_HANDLE( hFile );
        if( ifs == 0 )
            break;
        ret = pfs->CloseFile( hFile );

    }while( 0 );

	return ret;
}

static int monfs_fsync(const char *path, int isdatasync,
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

        GETFS_HANDLE( hFile );
        if( ifs == 0 )
        {
            ret = -EACCES;
            break;
        }
        guint32 dwFlags = FLAG_FLUSH_DATA;
        READ_LOCK( pfs );
        FileSPtr pOpen;
        ret = pfs->GetOpenFile( hFile, pOpen );
        if( ERROR( ret ) )
            break;
        ret = pOpen->Flush( dwFlags );

    }while( 0 );

	return ret;
}

static int monfs_utimens( const char * path,
    const struct timespec tv[2],
    struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */
    gint32 ret = 0;
    do{
        RegFsPtr pfs1;
        RFHANDLE hFile;
        bool bClose = false;
        if( fi == nullptr ||
            fi->fh == INVALID_HANDLE )
        {
            GETFS( path );
            if( ifs == 0 )
            {
                ret = -ENOTSUP;
                break;
            }
            ret = pfs->OpenFile( strPath,
                O_RDONLY, hFile, nullptr );
            if( ERROR( ret ) )
                break;
            bClose = true;
            pfs1 = pfs;
        }
        else
        {
            hFile = fi->fh;
            if( hFile == ROOT_HANDLE )
            {
                ret = -ENOTSUP;
                break;
            }
            GETFS_HANDLE( hFile );
            pfs1 = pfs; 
        }
        READ_LOCK( pfs1 );
        FileSPtr pOpen;
        ret = pfs1->GetOpenFile( hFile, pOpen );
        if( SUCCEEDED( ret ) )
            pOpen->SetTimes( tv );
        if( bClose )
            pfs1->CloseFile( hFile );

    }while( 0 );

	return ret;
}

bool IsSetPointValue(
    const char* szPath,
    RegFsPtr& pfs,
    stdstr& strPtPath )
{
    gint32 ret = 0;
    do{
        std::vector< stdstr > vecComps;
        ret = pfs->Namei( szPath, vecComps );
        if( ERROR( ret ) )
            break;
        
        if( vecComps.size() == 6 &&
            vecComps[ 0 ] == APP_DIR &&
            vecComps[ 1 ] == "apps" &&
            vecComps[ 3 ] == "points" &&
            vecComps[ 5 ] == "value" )
        {
            strPtPath = vecComps[ 1 ] + "/" +
                vecComps[ 4 ];
            break;
        }
        ret = ERROR_FALSE;
    }while( 0 );
    if( ERROR( ret ) )
        return false;
    return true;
}

/* xattr operations are optional and can safely be left unimplemented */
static int monfs_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{
    gint32 ret = 0;
    do{
        if( strcmp( name, "user.regfs" ) )
        {
            ret = -ENOTSUP;
            break;
        }
        if( size > VALUE_SIZE - 1 )
        {
            ret = -EOVERFLOW;
            break;
        }
        GETFS( path );
        if( ifs == 0 )
        {
            ret = -ENOTSUP;
            break;
        }
        Variant oVar;
        stdstr strAttr( value, size );
        ret = oVar.DeserializeFromJson(
            strAttr.c_str() );
        if( ERROR( ret ) )
            break;
        stdstr strPt;
        if( IsSetPointValue( path, pfs, strPt ) )
        {
            InterfPtr pam = GetAppManager();
            CAppManager_SvrImpl* pAM = pam;
            if( pAM != nullptr )
            {
                CCfgOpener oCfg;
                ret = pAM->SetPointValue(
                    oCfg.GetCfg(), strPt, oVar );
                break;
            }
        }
        ret = pfs->SetValue( strPath, oVar );

    }while( 0 );
	return ret;
}

static int monfs_getxattr(
    const char *path, const char *name,
    char *value, size_t size)
{
    gint32 ret = 0;
    do{
        if( strcmp( name, "user.regfs" ) )
        {
            ret = -ENOTSUP;
            break;
        }
        GETFS( path );
        if( ifs == 0 )
        {
            ret = -ENOTSUP;
            break;
        }
        Variant oVar;
        ret = pfs->GetValue( strPath, oVar );
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
        ret = std::min( strAttr.size(), size );
        strncpy( value, strAttr.c_str(), ret );

    }while( 0 );
	return ret;
}

static int monfs_listxattr(const char *path, char *list, size_t size)
{
    constexpr auto dwSize = sizeof( "user.regfs" );
    if( size == 0 )
        return dwSize;
    size = std::min( size, dwSize );
    strncpy( list, "user.regfs", size );
	return size;
}

static int monfs_opendir(
    const char* path, fuse_file_info* fi )
{
    gint32 ret = 0;
    do{
        GETFS( path );
        RFHANDLE hFile;
        if( ifs == 0 )
        {
            fi->fh = ROOT_HANDLE;
            break;
        }
        ret = pfs->OpenDir( strPath,
            fi->flags, hFile, nullptr );
        if( ERROR( ret ) )
            break;
        fi->fh = hFile;
    }while( 0 );
    return STATUS_SUCCESS;
}

static const struct fuse_operations monfs_oper = {
	.getattr	= monfs_getattr,
	.readlink	= monfs_readlink,
	.mkdir		= monfs_mkdir,
	.unlink		= monfs_unlink,
	.rmdir		= monfs_rmdir,
	.symlink	= monfs_symlink,
	.rename		= monfs_rename,
	.chmod		= monfs_chmod,
	.chown		= monfs_chown,
	.truncate	= monfs_truncate,
	.open		= monfs_open,
	.read		= monfs_read,
	.write		= monfs_write,
    .flush      = monfs_flush,
	.release	= monfs_release,
	.fsync		= monfs_fsync,
	.setxattr	= monfs_setxattr,
	.getxattr	= monfs_getxattr,
	.listxattr	= monfs_listxattr,
    .opendir    = monfs_opendir,
	.readdir	= monfs_readdir,
    .releasedir = monfs_releasedir,
	.init       = monfs_init,
	.access		= monfs_access,
	.create 	= monfs_create,
	.utimens 	= monfs_utimens,
};

gint32 FuseMain( int argc, char** argv )
{
    return fuse_main( argc, argv,
        &monfs_oper, nullptr );
}
#endif
