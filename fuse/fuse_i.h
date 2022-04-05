/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU LGPLv2.
  See the file COPYING.LIB
*/

#pragma once

#include "fuse.h"
#include "fuse_lowlevel.h"

#ifdef __cplusplus
extern "C" {
#endif
struct node_table {
    struct node **array;
    size_t use;
    size_t size;
    size_t split;
};

struct list_head {
    struct list_head *next;
    struct list_head *prev;
};

struct fuse {
    struct fuse_session *se;
    struct node_table name_table;
    struct node_table id_table;
    struct list_head lru_table;
    fuse_ino_t ctr;
    unsigned int generation;
    unsigned int hidectr;
    pthread_mutex_t lock;
    struct fuse_config conf;
    int intr_installed;
    struct fuse_fs *fs;
    struct lock_queue_element *lockq;
    int pagesize;
    struct list_head partial_slabs;
    struct list_head full_slabs;
    pthread_t prune_thread;
};

struct fuse_req {
	struct fuse_session *se;
	uint64_t unique;
	int ctr;
	pthread_mutex_t lock;
	struct fuse_ctx ctx;
	struct fuse_chan *ch;
	int interrupted;
	unsigned int ioctl_64bit : 1;
	union {
		struct {
			uint64_t unique;
		} i;
		struct {
			fuse_interrupt_func_t func;
			void *data;
		} ni;
	} u;
	struct fuse_req *next;
	struct fuse_req *prev;
};

struct fuse_notify_req {
	uint64_t unique;
	void (*reply)(struct fuse_notify_req *, fuse_req_t, fuse_ino_t,
		      const void *, const struct fuse_buf *);
	struct fuse_notify_req *next;
	struct fuse_notify_req *prev;
};

struct fuse_session {
	char *mountpoint;
	volatile int exited;
	int fd;
	struct mount_opts *mo;
	int debug;
	int deny_others;
	struct fuse_lowlevel_ops op;
	int got_init;
	struct cuse_data *cuse_data;
	void *userdata;
	uid_t owner;
	struct fuse_conn_info conn;
	struct fuse_req list;
	struct fuse_req interrupts;
	pthread_mutex_t lock;
	int got_destroy;
	pthread_key_t pipe_key;
	int broken_splice_nonblock;
	uint64_t notify_ctr;
	struct fuse_notify_req notify_list;
	size_t bufsize;
	int error;
};
#ifdef __cplusplus
}
#endif
namespace rpcf
{
template< typename ...Args1, typename ...Args2 >
static gint32 SafeCall(
    bool bExclusive,
    gint32 (CFuseObjBase::*func )(
        const char*, fuse_file_info*, Args1... ),
    const char* path,
    fuse_file_info* fi,
    Args2&&... args )
{
    gint32 ret = 0;
    do{
        CFuseObjBase* pObj = nullptr;
        if( fi != nullptr && fi->fh != 0 )
        {
            auto pIf = GetRootIf();
            CFuseRootServer* pSvr = pIf;
            CFuseRootProxy* pCli = pIf;
            FHCTX fhctx;
            if( pSvr )
            {
                ret = pSvr->GetFhCtx( fi->fh, fhctx );
            }
            else
            {
                ret = pCli->GetFhCtx( fi->fh, fhctx );
            }

            // removed
            if( ERROR( ret ) )
                break;

            pObj = reinterpret_cast
                < CFuseObjBase* >( fi->fh );
            if( fhctx.pSvc == nullptr )
            {
                ret = ( pObj->*func )( path, fi,
                    std::forward< Args2 >( args )... );
                break;
            }

            auto pSvc = fhctx.pSvc;
            if( bExclusive )
            {
                WLOCK_TESTMNT2( pSvc );
                if( pSvc->GetState() !=
                    stateConnected )
                {
                    ret = -EIO;
                    break;
                }
                else if( pObj->IsRemoved() )
                {
                    ret = -ENOENT;
                    break;
                }
                ret = ( pObj->*func )( path, fi, 
                    std::forward< Args2 >( args )... );
            }
            else
            {
                RLOCK_TESTMNT2( pSvc );
                if( pSvc->GetState() !=
                    stateConnected )
                {
                    ret = -EIO;
                    break;
                }
                else if( pObj->IsRemoved() )
                {
                    ret = -ENOENT;
                    break;
                }
                ret = ( pObj->*func )( path, fi,
                    std::forward< Args2 >( args )... );
            }
        }
        else if( path != nullptr )
        {
            std::vector<stdstr> vecSubdirs;
            CFuseObjBase* pSvcDir = nullptr;
            ret = rpcf::GetSvcDir(
                path, pSvcDir, vecSubdirs );
            if( ERROR( ret ) )
                break;
            if( ret == ENOENT )
            {
                ret = ( pSvcDir->*func )( path, fi,
                    std::forward< Args2 >( args )... );
                break;
            }

            CRpcServices* pSvc = pSvcDir->GetIf();

            if( bExclusive )
            {
                WLOCK_TESTMNT0( pSvcDir );

                if( pSvc->GetState() !=
                    stateConnected )
                {
                    ret = -EIO;
                    break;
                }

                CFuseSvcProxy* pProxy = ObjPtr( pSvc );
                CFuseSvcServer* pSvr = ObjPtr( pSvc );

                if( pSvr )
                    pObj = pSvr->GetEntryLocked(
                        vecSubdirs );
                else
                    pObj = pProxy->GetEntryLocked(
                        vecSubdirs );
                if( pObj == nullptr )
                {
                    ret = -ENOENT;
                    break;
                }
                ret = ( pObj->*func )( path, fi,
                    std::forward< Args2 >( args )... );
            }
            else
            {
                RLOCK_TESTMNT0( pSvcDir );
                if( pSvc->GetState() !=
                    stateConnected )
                {
                    ret = -EIO;
                    break;
                }

                CFuseSvcProxy* pProxy = ObjPtr( pSvc );
                CFuseSvcServer* pSvr = ObjPtr( pSvc );

                if( pSvr )
                    pObj = pSvr->GetEntryLocked(
                        vecSubdirs );
                else
                    pObj = pProxy->GetEntryLocked(
                        vecSubdirs );
                if( pObj == nullptr )
                {
                    ret = -ENOENT;
                    break;
                }
                ret = ( pObj->*func )( path, fi,
                    std::forward< Args2 >( args )... );
                        
            }
        }
        else
        {
            ret = -EINVAL;
            break;
        }

    }while( 0 );

    return ret;
}
}
