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

static void fuseif_rwinterrupt(
    fuse_req_t req, void *d_)
{
    if( d_ == nullptr )
        return;

    fuseif_intr_data *d =
        ( fuseif_intr_data* )d_;
    fuse *f = rpcf::GetFuse();

    if (d->id == pthread_self())
        return;

    OutputMsg( 0, "Checkpoint 18(%s): "
        "about to cancel req",
        __func__ );
    CFuseObjBase* pObj = d->fe;
    while( pObj != nullptr )
    {
        CFuseStmFile* pstm = dynamic_cast
            < CFuseStmFile* >( pObj );
        if( pstm != nullptr )
        {
            CFuseMutex oLock( pstm->GetLock() );
            pstm->CancelFsRequest( req );
            break;
        }
        CFuseFileEntry* pfe = dynamic_cast
            < CFuseFileEntry* >( pObj );
        if( pfe != nullptr )
        {
            CFuseMutex oLock( pfe->GetLock() );
            pfe->CancelFsRequest( req );
            break;
        }
        break;
    }
    pthread_mutex_lock(&f->lock);
    while (!d->finished) {
        struct timeval now;
        struct timespec timeout;

        pthread_kill(d->id, f->conf.intr_signal );
        gettimeofday(&now, NULL);
        timeout.tv_sec = now.tv_sec + 1;
        timeout.tv_nsec = now.tv_usec * 1000;
        pthread_cond_timedwait(&d->cond, &f->lock, &timeout);
    }
    pthread_mutex_unlock(&f->lock);
}

static void fuseif_do_finish_interrupt(
    fuse *f, fuse_req_t req,
    fuseif_intr_data *d)
{
    pthread_mutex_lock(&f->lock);
    d->finished = 1;
    pthread_cond_broadcast(&d->cond);
    pthread_mutex_unlock(&f->lock);
    fuse_req_interrupt_func(req, NULL, NULL);
    pthread_cond_destroy(&d->cond);
}

static void fuseif_do_prepare_interrupt(
    fuse_req_t req,
    fuseif_intr_data *d)
{   
    d->id = pthread_self();
    pthread_cond_init(&d->cond, NULL);
    d->finished = 0;
    fuse_req_interrupt_func(req, fuseif_rwinterrupt, d);
}       
        
void fuseif_finish_interrupt(
    fuse *f, fuse_req_t req,
    fuseif_intr_data *d)
{   
    if (f->conf.intr)
        fuseif_do_finish_interrupt(f, req, d);
}       

void fuseif_prepare_interrupt(
    fuse *f, fuse_req_t req,
    fuseif_intr_data *d)
{              
    if (f->conf.intr)
        fuseif_do_prepare_interrupt(req, d);
}

void fuseif_free_buf(struct fuse_bufvec *buf)
{      
    free(buf);
}

static stdmutex s_oFuseLock;

void fuseif_reply_err( fuse_req_t req, int err )
{
    CStdMutex oLock( s_oFuseLock );
    fuse_reply_err( req, err );
}

void fuseif_complete_read(
    fuse* pFuse,
    fuse_req_t req,
    gint32 ret,
    fuse_bufvec* buf )
{
    CStdMutex oLock( s_oFuseLock );
    if( SUCCEEDED( ret ) )
    {
        if( buf != nullptr )
        {
            ret = fuse_reply_data( req,
                buf, FUSE_BUF_NO_SPLICE );
            if( ERROR( ret ) )
            {
                OutputMsg( ret, "Checkpoint 20: "
                    "failed to reply data" );
            }
        }
        else
            fuse_reply_err( req, 0 );
    }
    else
    {
        fuse_reply_err( req, -ret );
    }
    oLock.Unlock();
    if( buf != nullptr )
        fuseif_free_buf( buf );
}

void fuseif_ll_read(fuse_req_t req,
    fuse_ino_t ino, size_t size,
    off_t off, fuse_file_info *fi)
{
    gint32 ret = 0;
    do{
        fuse* pFuse = GetFuse();
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
        {
            OutputMsg( ret, "Checkpoint 17(%s): "
                "pending returned",
                __func__ );
            break;
        }

        fuseif_finish_interrupt( pFuse, req, d );
        if( d != nullptr )
            free( d );
        fuseif_complete_read(
            pFuse, req, ret, buf );

    }while( 0 );

    return;
}


void fuseif_ll_write_buf(fuse_req_t req,
    fuse_ino_t ino, fuse_bufvec *buf,
    off_t off, fuse_file_info *fi)
{
    gint32 ret = 0;

    auto d = ( fuseif_intr_data* )
        calloc( 1, sizeof( fuseif_intr_data ) );
    do{
        fuse* pFuse = GetFuse();
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
            fuse_reply_err( req, -ret );

    }while( 0 );
    free( d );

    return;
}

void fuseif_tweak_llops( fuse_session* se )
{
    if( se == nullptr )
        return;

    se->op.read = fuseif_ll_read;
    se->op.write_buf = fuseif_ll_write_buf;
    return;
}

