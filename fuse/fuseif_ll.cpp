/*
 * =====================================================================================
 *
 *       Filename:  fuseif_ll.cpp
 *
 *    Description:  implementations of lower-level operations for read/write
 *
 *        Version:  1.0
 *        Created:  03/16/2022 10:17:06 AM
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
/*
  part of the code is from FUSE project.
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  Implementation of the high-level FUSE API on top of the low-level
  API.

  This program can be distributed under the terms of the GNU LGPLv2.
  See the file COPYING.LIB
*/
#include "fuseif.h"
#include "fuse_i.h"
#include <signal.h>
using namespace rpcf;

fuse_session* fuseif_get_session( MYFUSE* fuse )
{ return fuse->se; }

static size_t fuseif_buf_size(const fuse_bufvec *bufv)
{
    size_t i;
    size_t size = 0;

    for (i = bufv->idx; i < bufv->count; i++) {
        if (bufv->buf[i].size == SIZE_MAX)
        {
            size = SIZE_MAX;
            break;
        }
        else
        {
            size += bufv->buf[i].size;
            if( i == bufv->idx )
                size -= bufv->off;
        }
    }

    return size;
}

void fuseif_free_buf(struct fuse_bufvec *buf)
{ free(buf); }

inline void fuseif_reply_err( fuse_req_t req, int err )
{
    fuse_reply_err( req, -err );
}

void fuseif_complete_read(
    MYFUSE* pFuse,
    fuse_req_t req,
    gint32 ret,
    fuse_bufvec* buf )
{
    if( SUCCEEDED( ret ) )
    {
        if( buf != nullptr )
        {
            fuse_reply_data( req,
                buf, FUSE_BUF_SPLICE_MOVE );
        }
        else
            fuseif_reply_err( req, 0 );
    }
    else
    {
        fuseif_reply_err( req, ret );
    }
    if( buf != nullptr )
        fuseif_free_buf( buf );
}

static void fuseif_ll_read(fuse_req_t req,
    fuse_ino_t ino, size_t size,
    off_t off, fuse_file_info *fi);

static void fuseif_ll_write_buf(fuse_req_t req,
    fuse_ino_t ino, fuse_bufvec *buf,
    off_t off, fuse_file_info *fi);

#define fuseif_prepare_interrupt( f, req, d)
#define fuseif_finish_interrupt( f, req, d)

gint32 fuseif_get_fhctx(
    fuse_ino_t ino, FHCTX& fhctx )
{
    gint32 ret = 0;
    do{
        InterfPtr pIf = GetRootIf();
        CFuseRootServer* pSvr = pIf;
        CFuseRootProxy* pCli = pIf;
        guint64 fh;
        if( pSvr )
        {
            ret = pSvr->GetFhFromIno( ino, fh );
            if( SUCCEEDED( ret ) )
                pSvr->GetFhCtx( fh, fhctx );
        }
        else
        {
            ret = pCli->GetFhFromIno( ino, fh );
            if( SUCCEEDED( ret ) )
                pCli->GetFhCtx( fh, fhctx );
        }

    }while( 0 );
    return ret;
}
gint32 fuseif_get_path(
    fuse_ino_t ino, stdstr& strPath )
{
    gint32 ret = 0;
    do{
        ROOTLK_SHARED;
        FHCTX fhctx;
        ret = fuseif_get_fhctx( ino, fhctx );
        if( ERROR( ret ) )
            break;
        if( !fhctx.pFile )
        {
            ret = -EFAULT;
            break;
        }

        fhctx.pFile->GetFullPath( strPath );

    }while( 0 );
    return ret;
}

static void fuseif_ll_getattr(
    fuse_req_t req,
    fuse_ino_t ino, 
    fuse_file_info *fi) 
{
    gint32 ret = 0;
    struct stat stbuf;
    do{
        stdstr strPath;
        const char* szPath = nullptr;
        if( fi == nullptr )
        {
            ret = fuseif_get_path( ino, strPath );
            if( ERROR( ret ) )
                break;
            szPath = strPath.c_str();
        }
        ret = SafeCall(
            "getattr", false,
            &CFuseObjBase::fs_getattr,
            szPath, fi, &stbuf );

    }while( 0 );
    if( ERROR( ret ) )
    {
        fuseif_reply_err( req, ret );
        return;
    }
    fuse_reply_attr( req, &stbuf, 10000 );
    return;
}

gint32 fuseif_post_mkdir(
    IEventSink* pCallback,
    fuse_file_info* fi )
{
    gint32 ret = 0;
    do{
        ObjPtr pSp;
        CCfgOpenerObj oTaskCfg( pCallback );
        ret = oTaskCfg.GetObjPtr(
            propSvcPoint, pSp );
        if( ERROR( ret ) )
            break;

        CFuseSvcProxy* pCli = pSp;
        CFuseSvcServer* pSvr = pSp;
        CFuseSvcDir* pSvcDir = nullptr;
        if( pCli != nullptr )
        {
            // proxy side has a complex process of
            // directory creation 
            ret = -ENOENT;
            break;
        }

        pSvcDir = static_cast< CFuseSvcDir* >
            ( pSvr->GetSvcDir().get() );

        if( fi == nullptr ||
            fi->fh == INVALID_HANDLE )
            break;

        CBuffer* pBuf = ( CBuffer* )fi->fh;
        pBuf->Resize(
            sizeof( fuse_entry_param ) );

        struct stat stbuf;
        pSvcDir->fs_getattr(
            nullptr, nullptr, &stbuf );

        auto e =
            ( fuse_entry_param* )pBuf->ptr();
        e->ino = pSvcDir->GetObjId();
        e->generation = 0;
        e->entry_timeout = 100000;
        e->attr_timeout = 100000;
        e->attr = stbuf;

    }while( 0 );

    return ret;
}

static void fuseif_ll_mkdir(
    fuse_req_t req, fuse_ino_t parent,
    const char *name, mode_t mode )
{
    gint32 ret = 0;    
    do{
        stdstr strPath;
        ret = fuseif_get_path( parent, strPath );
        if( ERROR( ret ) )
            break;

        strPath.push_back( '/' );
        strPath += name;

        BufPtr pBuf( true );

        fuse_file_info fi;
        memset( &fi, sizeof( fi ), 0 );
        fi.fh = ( HANDLE )( ( CBuffer* )pBuf );
        ret = SafeCall( "mkdir", false,
            &CFuseObjBase::fs_mkdir,
            strPath.c_str(), &fi, mode );

        if( ERROR( ret ) )
            break;

        if( pBuf->empty() )
        {
            ret = -ENOENT;
            break;
        }

        auto e = ( fuse_entry_param* )pBuf->ptr();
        fuse_reply_entry( req, e );

    }while( 0 );

    if( ERROR( ret ) )
        fuseif_reply_err( req, ret );
}

static void fuseif_ll_readdir(
    fuse_req_t req,
    fuse_ino_t ino,
    size_t size, off_t off,
    fuse_file_info *fi)
{
    dirbuf dbuf;
    dbuf.req = req;
    if( size == 0 ||
        size > MAX_BYTES_PER_BUFFER )
    {
        fuseif_reply_err( req, -EINVAL );
        return;
    }
    dbuf.p = ( char* )malloc( size );
    do{
        gint32 ret = SafeCall(
            "readdir", false,
            &CFuseObjBase::fs_readdir_ll,
            nullptr, fi, dbuf, off, size,
            ( fuse_readdir_flags )0 );
        if( ret == -ENOENT )
        {
            fuse_reply_buf( req, nullptr, 0 );
            break;
        }
        if( ERROR( ret ) )
        {
            fuseif_reply_err( req, ret );
            break;
        }

        fuse_reply_buf( req, dbuf.p, dbuf.size );

    }while( 0 );

    free( dbuf.p );
    dbuf.p = nullptr;
    return;
}

extern gint32 fuseop_opendir(
    const char* path, fuse_file_info * fi );

extern gint32 fuseop_releasedir(
    const char* path, fuse_file_info * fi );

static void fuseif_ll_opendir( fuse_req_t req,
    fuse_ino_t ino, fuse_file_info *fi)
{
    gint32 ret = 0;
    do{
        stdstr strPath;
        ret = fuseif_get_path( ino, strPath );
        if( ERROR( ret ) )
            break;

        ret = fuseop_opendir(
            strPath.c_str(), fi );
        if( ERROR( ret ) )
            break;

        ret = fuse_reply_open( req, fi );
        if( ret == -ENOENT )
        {
            // the syscall was interrupted, must
            // rollback what have done.
            fuseop_releasedir(
                strPath.c_str(), fi );
            ret = 0;
        }

    }while( 0 );

    if( ERROR( ret ) )
        fuseif_reply_err( req, ret );
    return;
}

static void fuseif_ll_releasedir(fuse_req_t req,
    fuse_ino_t ino, fuse_file_info *fi)
{
    gint32 ret = fuseop_releasedir( nullptr, fi );
    fuseif_reply_err( req, ret );
}

static void fuseif_ll_lookup(fuse_req_t req,
    fuse_ino_t parent, const char *name)
{
    gint32 ret = -ENOENT;
    fuse_entry_param* e = nullptr;
    do{
        ROOTLK_SHARED;
        FHCTX fhctx;
        ret = fuseif_get_fhctx( parent, fhctx );
        if( ERROR( ret ) )
            break;
        if( !fhctx.pFile )
        {
            ret = -EFAULT;
            break;
        }
        auto pDir = dynamic_cast< CFuseDirectory* >
            ( fhctx.pFile.get() );
        if( pDir == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        DIR_SPTR pChild;
        ret = pDir->GetChild( name, pChild );
        if( ERROR( ret ) )
        {
            ret = -ENOENT;
            break;
        };
        auto pfd = dynamic_cast< CFuseObjBase* >
            ( pChild.get() );

        BufPtr pBuf( true );
        pBuf->Resize(
            sizeof( fuse_entry_param ) );

        struct stat stbuf;
        pfd->fs_getattr(
            nullptr, nullptr, &stbuf );

        auto e = ( fuse_entry_param* )pBuf->ptr();
        e->ino = pfd->GetObjId();
        e->generation = 0;
        e->entry_timeout = 100000;
        e->attr_timeout = 100000;
        e->attr = stbuf;
        fuse_reply_entry( req, e );
        ret = 0;

    }while( 0 );
    if( ERROR( ret ) )
        fuseif_reply_err( req, ret );
}

extern gint32 fuseop_unlink( const char* path );
static void fuseif_ll_unlink(fuse_req_t req,
    fuse_ino_t parent, const char *name)
{
    gint32 ret = 0;
    do{
        stdstr strPath;
        ret = fuseif_get_path( parent, strPath );
        if( ERROR( ret ) )
            break;
        strPath.push_back( '/' );
        strPath += name;
        ret = fuseop_unlink( strPath.c_str() );

    }while( 0 );
    fuseif_reply_err( req, ret );
}

extern int fuseif_rmdir_internal(
    const char *path, IEventSink* pCb )
{
    gint32 ret = 0;
    do{
        ROOTLK_SHARED;
        std::vector<stdstr> vecSubdirs;
        CFuseObjBase* pSvcDir = nullptr;
        ret = rpcf::GetSvcDir(
            path, pSvcDir, vecSubdirs );
        if( SUCCEEDED( ret ) && vecSubdirs.empty() )
        {
            fuse_file_info fi1 = {0};
            fi1.fh = ( HANDLE )( IEventSink* )pCb;
            stdstr strName = pSvcDir->GetName();
            auto pParent = dynamic_cast
            < CFuseDirectory* >( pSvcDir->GetParent() );
            ret = pParent->fs_rmdir(
                strName.c_str(), &fi1, false );
            break;
        }
        else
        {
            ret = -EACCES;
            break;
        }
    }while( 0 );
    return ret;
}
static void fuseif_ll_rmdir( fuse_req_t req,
    fuse_ino_t parent, const char *name)
{
    gint32 ret = 0;
    gint32 (*func)( HANDLE, guint64,
        const char*, IEventSink* ) =
    ([]( HANDLE req, guint64 parent,
        const char* name, IEventSink* pCb )->gint32
    {
        gint32 ret = 0;
        do{
            stdstr strPath;
            ret = fuseif_get_path( parent, strPath );
            if( ERROR( ret ) )
                break;
            strPath.push_back( '/' );
            strPath += name;
            ret = fuseif_rmdir_internal(
                strPath.c_str(), pCb );

        }while( 0 );
        return ret;
    });
    
    TaskletPtr pRmdir;
    do{
        InterfPtr pIf = GetRootIf();
        if( pIf.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }
        CRpcServices* pSvc = pIf;

        TaskletPtr pSyncTask;
        ret = pSyncTask.NewObj(
            clsid( CSyncCallback ) );
        if( ERROR( ret ) )
            break;
        CSyncCallback* pSync = pSyncTask;

        ret = NEW_FUNCCALL_TASK2(
            3, pRmdir, pSvc->GetIoMgr(),
            func, ( HANDLE )req,
            ( guint64)parent, name, nullptr );
        if( ERROR( ret ) )
            break;

        CIfRetryTask* pTemp = pRmdir;
        pTemp->SetClientNotify( pSync );

        CFuseRootProxy* pProxy = pIf;
        CFuseRootServer* pSvr = pIf;
        if( pSvr )
            ret = pSvr->AddRmdirTask( pRmdir );
        else
            ret = pProxy->AddRmdirTask( pRmdir );
        if( ERROR( ret ) )
        {
            ( *pRmdir )( eventCancelTask );
            break;
        }

        pSync->WaitForCompleteWakable();
        ret = pSync->GetError();

    }while( 0 );
    fuseif_reply_err( req, ret );
}

extern gint32 fuseop_open(
    const char* path, fuse_file_info * fi);

static void fuseif_ll_open(fuse_req_t req,
    fuse_ino_t ino, fuse_file_info *fi)
{
    gint32 ret = 0;
    do{
        stdstr strPath;
        ret = fuseif_get_path( ino, strPath );
        if( ERROR( ret ) )
            break;
        ret = fuseop_open( strPath.c_str(), fi );

    }while( 0 );
    if( ERROR( ret ) )
        fuseif_reply_err( req, ret );
    else
        fuse_reply_open( req, fi );
}

extern void do_nothing(int sig);
extern int set_one_signal_handler(
    int sig, void (*handler)(int), int remove);

static void fuseif_ll_init(void *userdata,
    struct fuse_conn_info *conn)
{
    MYFUSE* pFuse = ( MYFUSE* )userdata;
    conn->max_readahead = 0;
    set_one_signal_handler(
        SIGUSR1, do_nothing, 0 );
}

extern gint32 fuseop_create_internal(
    const char* path, mode_t mode,
    fuse_file_info* fi,
    CReadLock& ortlock );

static void fuseif_ll_create(fuse_req_t req,
    fuse_ino_t parent, const char *name,
    mode_t mode, fuse_file_info *fi)
{
    gint32 ret = 0;

    do{
        stdstr strPath;
        ret = fuseif_get_path( parent, strPath );
        if( ERROR( ret ) )
            break;

        ROOTLK_SHARED;

        strPath.push_back( '/' );
        strPath += name;
        ret = fuseop_create_internal(
            strPath.c_str(), mode,
            fi, _ortlk );
        if( ERROR( ret ) )
            break;

        auto pObj = reinterpret_cast
            < CFuseObjBase* >( fi->fh );
        struct stat stbuf;
        pObj->fs_getattr(
            nullptr, nullptr, &stbuf );

        fuse_entry_param e;
        memset( &e, 0, sizeof( e ) );
        e.ino = pObj->GetObjId();
        e.generation = 0;
        e.entry_timeout = 100000;
        e.attr_timeout = 100000;
        e.attr = stbuf;

        ret = fuse_reply_create( req, &e, fi );
        if( ret == -ENOENT )
        {
            _ortlk.Unlock();
            fuseop_unlink( strPath.c_str() );
        }

    }while( 0 );

    if( ERROR( ret ) )
        fuseif_reply_err( req, ret );

    return;
}

#if FUSE_USE_VERSION < 35
void fuseif_ll_ioctl(fuse_req_t req,
    fuse_ino_t ino, int cmd,
    void *arg, struct fuse_file_info *fi,
    unsigned flags, const void *in_buf,
    size_t in_bufsz, size_t out_bufsz)
#else
void fuseif_ll_ioctl(fuse_req_t req,
    fuse_ino_t ino, unsigned int cmd,
    void *arg, struct fuse_file_info *fi,
    unsigned flags, const void *in_buf,
    size_t in_bufsz, size_t out_bufsz)
#endif
{
    gint32 ret = 0;
    do{
        BufPtr pOutBuf( true );
        if( out_bufsz )
        {
            size_t act_size = std::max(
                in_bufsz, out_bufsz );
            ret = pOutBuf->Resize( act_size );
            if( ERROR( ret ) )
                break;
        }

        if( out_bufsz && in_bufsz )
            memcpy( pOutBuf->ptr(),
                in_buf, in_bufsz );

        ret = SafeCall( "ioctl", false,
            &CFuseObjBase::fs_ioctl,
            nullptr, fi, cmd, arg, flags,
            out_bufsz ? pOutBuf->ptr() :
                ( void* )in_buf );

        if( ERROR( ret ) )
            break;

        fuse_reply_ioctl( req, ret,
            pOutBuf->ptr(), out_bufsz );

    }while( 0 );

    if( ERROR( ret ) )
        fuseif_reply_err( req, ret );
}

static void fuseif_ll_poll(fuse_req_t req,
    fuse_ino_t ino, fuse_file_info *fi,
    fuse_pollhandle *ph)
{
    gint32 ret = 0;
    do{
        unsigned revents = 0;
        ret = SafeCall( "poll", false, 
            &CFuseObjBase::fs_poll,
            nullptr, fi, ph, &revents );
        if( ERROR( ret ) )
            break;
        fuse_reply_poll( req, revents );

   }while( 0 );

   if( ERROR( ret ) )
       fuseif_reply_err( req, ret );
}

extern gint32 fuseop_access(
    const char* path, int flags );
static void fuseif_ll_access(fuse_req_t req,
    fuse_ino_t ino, int mask)
{
    gint32 ret = 0;
    do{
        stdstr strPath;
        ret = fuseif_get_path( ino, strPath );
        if( ERROR( ret ) )
            break;
        ret = fuseop_access(
            strPath.c_str(), mask );

    }while( 0 );
    fuseif_reply_err( req, ret );
}

extern gint32 fuseop_release(
    const char* path, fuse_file_info * fi );

static void fuseif_ll_release(fuse_req_t req,
    fuse_ino_t ino, fuse_file_info *fi)
{
    gint32 ret = 0;
    ret = fuseop_release( nullptr, fi );
    fuseif_reply_err( req, ret );
}

static const struct fuse_lowlevel_ops fuseif_ll_oper = {
    .init       = fuseif_ll_init,
    .lookup     = fuseif_ll_lookup,
    .getattr    = fuseif_ll_getattr,
    .mkdir      = fuseif_ll_mkdir,
    .unlink     = fuseif_ll_unlink,
    .rmdir      = fuseif_ll_rmdir,
    .open       = fuseif_ll_open,
    .read       = fuseif_ll_read,
    .release    = fuseif_ll_release,
    .opendir    = fuseif_ll_opendir,
    .readdir    = fuseif_ll_readdir,
    .releasedir = fuseif_ll_releasedir,
    .access     = fuseif_ll_access,
    .create     = fuseif_ll_create,
    .ioctl      = fuseif_ll_ioctl,
    .poll       = fuseif_ll_poll,
    .write_buf  = fuseif_ll_write_buf,
};

/* Command line parsing */
struct options {
    int no_reconnect;
    // int update_interval;
};
static struct options options = {
        .no_reconnect = 0,
    //     .update_interval = 1,
};

#define OPTION(t, p) { t, offsetof(struct options, p), 1 }
static const struct fuse_opt option_spec[] = {
        OPTION("--no-reconnect", no_reconnect),
   //      OPTION("--update-interval=%d", update_interval),
        FUSE_OPT_END
};

extern ObjPtr g_pIoMgr;
namespace rpcf{
extern void SetFuse( MYFUSE* );
}

gint32 fuseif_main_ll( fuse_args& args,
    fuse_cmdline_opts& opts )
{
    struct MYFUSE fuse;
    struct fuse_loop_config config;
    struct fuse_session *se = nullptr; 
    int res;

    if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1)
        return 1;

    if( options.no_reconnect )
    {
        CIoManager* pMgr = g_pIoMgr;
        pMgr->SetCmdLineOpt( propConnRecover, false );
    }

    if (fuse_parse_cmdline(&args, &opts) != 0)
        return 1;

    memset( &fuse, 0, sizeof( fuse ) );
    SetFuse( &fuse );
    if( opts.mountpoint == nullptr )
    {
        res = 1;
        goto out1;
    }

    se = fuse_session_new(&args,
        &fuseif_ll_oper , sizeof(fuse_lowlevel_ops ), &fuse);
    if (se == NULL)
        goto out1;

    fuse.se = se;

    if (fuse_set_signal_handlers(se) != 0)
        goto out2;

    if (fuse_session_mount(se, opts.mountpoint) != 0)
        goto out3;

    /* Block until ctrl+c or fusermount -u */
    if (opts.singlethread)
        res = fuse_session_loop(se);
    else {
        config.clone_fd = opts.clone_fd;
        config.max_idle_threads = opts.max_idle_threads;
        res = fuse_session_loop_mt(se, &config);
    }

    fuse_session_unmount(se);
out3:
    fuse_remove_signal_handlers(se);
out2:
    fuse_session_destroy(fuse.se);
out1:
    free(opts.mountpoint);
    fuse_opt_free_args(&args);
    if( res != 0 )
        return ERROR_FAIL;
    return res;
}

static void fuseif_ll_read(fuse_req_t req,
    fuse_ino_t ino, size_t size,
    off_t off, fuse_file_info *fi)
{
    gint32 ret = 0;
    do{
        MYFUSE* pFuse = GetFuse();
        auto d = ( fuseif_intr_data* )
            calloc( 1, sizeof( fuseif_intr_data ) );
        d->fe = ( CFuseObjBase* )fi->fh;
        fuseif_prepare_interrupt( pFuse, req, d );
        fuse_bufvec* buf = nullptr;
        std::vector< BufPtr > vecBackup;
        ret = SafeCall( "read", false,
            &CFuseObjBase::fs_read,
            ( const char* )nullptr, fi,
            req, buf, off, size,
            vecBackup, d );
        if( ret == STATUS_PENDING )
            break;
        fuseif_finish_interrupt( pFuse, req, d );
        if( d != nullptr )
            free( d );
        fuseif_complete_read(
            pFuse, req, ret, buf );

    }while( 0 );

    return;
}


static void fuseif_ll_write_buf(fuse_req_t req,
    fuse_ino_t ino, fuse_bufvec *buf,
    off_t off, fuse_file_info *fi)
{
    gint32 ret = 0;

    auto d = ( fuseif_intr_data* )
        calloc( 1, sizeof( fuseif_intr_data ) );
    do{
        MYFUSE* pFuse = GetFuse();
        d->fe = ( CFuseObjBase* )fi->fh;
        if( unlikely( d->fe == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }
        TaskletPtr pCallback;
        auto pCmd = dynamic_cast
            < CFuseCmdFile* >( d->fe );
        if( unlikely( pCmd != nullptr ) )
        {
            // extra parameter for cmdfile
            CParamList oParams;
            CRpcServices* pSvc = GetRootIf();
            if( unlikely( pSvc == nullptr ) )
            {
                ret = -EFAULT;
                break;
            }
            oParams.SetPointer(
                propIoMgr, pSvc->GetIoMgr() );
            ret = pCallback.NewObj(
                clsid( CSyncCallback ),
                oParams.GetCfg() );
            if( ERROR( ret ) )
                break;
            d->pCb = pCallback;
        }

        fuseif_prepare_interrupt( pFuse, req, d );
        ret = SafeCall( "write_buf", false,
            &CFuseObjBase::fs_write_buf,
            nullptr, fi, req, buf, d );
        if( ret == STATUS_PENDING &&
            !pCallback.IsEmpty() )
        {
            CSyncCallback* pSync = pCallback;
            pSync->WaitForCompleteWakable();
            ret = pSync->GetError();
        }
        fuseif_finish_interrupt( pFuse, req, d );
        if( SUCCEEDED( ret ) )
            fuse_reply_write(
                req, fuseif_buf_size( buf ) );
        else
            fuseif_reply_err( req, ret );

    }while( 0 );
    free( d );

    return;
}
