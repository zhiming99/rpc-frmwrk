/*
 * =====================================================================================
 *
 *       Filename:  fuseif.cpp
 *
 *    Description:  Implementation of rpc interface for fuse integration and
 *                  related classes
 *
 *        Version:  1.0
 *        Created:  03/09/2022 09:29:45 PM
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
#include "fuseif.h"
namespace rpcf
{
InterfPtr g_pRootIf;
static std::unique_ptr< Json::CharReader > g_pReader;

InterfPtr& GetRootIf()
{ return g_pRootIf; }

fuse* GetFuse()
{
    CRpcServices* pSvc = GetRootIf();
    if( pSvc->IsServer() )
    {
        CFuseRootServer* pSvr = GetRootIf();
        return pSvr->GetFuse();
    }
    CFuseRootProxy* pProxy = GetRootIf();
    return pProxy->GetFuse();
}

void SetFuse( fuse* pFuse )
{
    CRpcServices* pSvc = GetRootIf();
    if( pSvc->IsServer() )
    {
        CFuseRootServer* pSvr = GetRootIf();
        return pSvr->SetFuse( pFuse );
    }
    CFuseRootProxy* pProxy = GetRootIf();
    return pProxy->SetFuse( pFuse );
}

CFuseDirectory* GetRootDir()
{
    CRpcServices* pSvc = GetRootIf();
    if( pSvc->IsServer() )
    {
        CFuseRootServer* pSvr = GetRootIf();
        return pSvr->GetRootDir();
    }
    CFuseRootProxy* pProxy = GetRootIf();
    return pProxy->GetRootDir();
};

gint32 GetFuseObj(
    const stdstr& strPath,
    CFuseObjBase*& pDir )
{
    CRpcServices* pSvc = GetRootIf();
    if( pSvc->IsServer() )
    {
        CFuseRootServer* pSvr = GetRootIf();
        return pSvr->GetFuseObj(
            strPath, pDir );
    }
    CFuseRootProxy* pProxy = GetRootIf();
    return pProxy->GetFuseObj(
        strPath, pDir );
}

gint32 CloseChannel(
    InterfPtr& pIf, HANDLE hStream )
{
    if( hStream == INVALID_HANDLE ||
        pIf.IsEmpty() )
        return -EINVAL;

    CRpcServices* pSvc = pIf;
    if( pSvc->GetState() != stateConnected )
        return ERROR_STATE;

    do{
        TaskletPtr pCallback;
        ret = pCallback.NewObj(
            clsid( CSyncCallback ) );
        if( ERROR( ret ) )
            break;

        CSyncCallback* pSync = pCallback;

        if( pSvc->IsServer() )
        {
            CStreamServerSync* pSvr = pIf;
            ret = pSvr->OnClose(
                hStream, pCallback);
        }
        else
        {
            CStreamProxySync* pProxy = pIf;
            ret = pProxy->OnClose(
                hStream, pCallback);
        }
        if( ret == STATUS_PENDING )
            ret = pSync->WaitForCompleteWakable();

    }while( 0 );

    return ret;
}

gint32 CFuseRootDir::fs_getattr(
    const char *path,
    stat *stbuf, fuse_file_info* fi)
{
    stbuf->st_ino = 1;
    stbuf->st_mode = S_IFDIR | GetMode();
    stbuf->st_nlink = 1;
    return 0;
}

gint32 CFuseDirectory::fs_getattr(
    const char *path,
    stat *stbuf, fuse_file_info* fi)
{
    stbuf->st_ino = GetObjId();
    stbuf->st_mode = S_IFDIR | GetMode();
    stbuf->st_nlink = 1;
    return 0;
}

gint32 CFuseFileEntry::fs_getattr(
    const char *path,
    stat *stbuf, fuse_file_info* fi)
{
    stbuf->st_ino = GetObjId();
    stbuf->st_mode = S_IFREG | GetMode();
    stbuf->st_nlink = 1;
    stbuf->st_size = MAX_FUSE_MSG_SIZE;
    return 0;
}

gint32 CFuseTextFile::fs_getattr(
    const char *path,
    stat *stbuf, fuse_file_info* fi)
{
    stbuf->st_ino = GetObjId();
    stbuf->st_mode = S_IFREG | 0400;
    stbuf->st_nlink = 1;
    stbuf->st_size = m_strContent.size();
    return 0;
}

gint32 CFuseDirectory::fs_readdir(
    const char *path,
    void *buf, fuse_fill_dir_t filler,
    off_t off, struct fuse_file_info *fi,
    enum fuse_readdir_flags flags)
{
    gint32 ret = 0;
    do{
        if( size == 0 )        
            break;
        stdstr strPath = path;
        RLOCK_TESTMNT3( path );

        CFuseMutex oLock( GetLock() );
        std::vector< CHILD_TYPE > vecChildren;
        GetChildren( vecChildren );
        for( auto& elem : vecChildren )
        {
            CFuseObjBase* pObj = dynamic_cast
                < CFuseObjBase* >( elem.get() );
            if( pObj == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            stat file_stat;
            pObj->fs_getattr(
                "", &file_stat, nullptr );
            filler( buf, pObj->GetName().c_str(),
                &file_stat, 0, 0 );
        }

    }while( 0 );
    return ret;
}

gint32 CFuseTextFile::fs_read(
    fuse_req_t req,
    fuse_bufvec*& bufvec,
    off_t off, size_t size,
    fuse_file_info *fi,
    std::vector< BufPtr >& vecBackup
    fuseif_intr_data* d ) override
{
    gint32 ret = 0;
    do{
        if( size == 0 )        
            break;

        if( off >= m_strContent.size() )
        {
            ret = -ERANGE;
            break;
        }

        size_t dwBytes = std::min(
            size, m_strContent.size() - off );

        BufPtr pBuf = NewBufNoAlloc(
            m_strContent.c_str() + off, dwBytes );

        bufvec = new fuse_bufvec;
        bufvec = FUSE_BUFVEC_INIT( dwBytes );
        bufvec->buf[ 0 ].mem = pBuf->ptr();
        bufvec->buf[ 0 ].size = pBuf->size();
        vecBackup.push_back( pBuf );

    }while( 0 );

    return ret;
}

gint32 CFuseEvtFile::fs_poll(
    const char *path,
    fuse_file_info *fi,
    fuse_pollhandle *ph,
    unsigned *reventsp )
{
    gint32 ret = 0;
    do{
        RLOCK_TESTMNT3( path )

        CFuseMutex oLock( GetLock() );
        ret = oLock.GetStatus();
        if( ERROR( ret ) )
            break;

        auto oldph = GetPollHandle();
        if( ph != nullptr )
        {
            fuse_pollhandle_destroy( oldph );
            SetPollHandle( ph );
        }

        if( GetMode() & S_IRUSR )
        {
            if( m_queIncoming.size() > 0 &&
                m_queReqs.empty() )
                *reventsp |= POLLIN;
        }

        *reventsp = (
            fi->poll_events & GetRevents() );

        if( GetRevents() & POLLHUP )
            *reventsp |= POLLHUP;
        if( GetRevents() & POLLERR )
            *reventsp |= POLLERR;
        if( *reventsp != 0 )
        {
            SetRevents( ( ~*reventsp ) &
                GetRevents() );
        }

    }while( 0 );

    return ret;
}

gint32 CFuseFileEntry::fs_ioctl(
    const char *path, unsigned int cmd,
    void *arg, fuse_file_info *fi,
    unsigned int flags,
    void *data )
{
    gint32 ret = 0;
    do{
        if( cmd != FIONREAD )
        {
            ret = -ENOSYS;
            break;
        }

        CRpcServices* pSvc = GetIf();
        RLOCK_TESTMNT2( pSvc );

        CFuseMutex oLock( GetLock() );
        ret = oLock.GetStatus();
        if( ERROR( ret ) )
            break;

        *pSize = 0;   
        size_t* pSize = ( size_t* )pData;
        if( m_queReqs.size() )
            break;

        *pSize = GetBytesAvail();

    }while( 0 );

    return ret;
}

gint32 CFuseReqFileProxy::fs_ioctl(
    const char *path, unsigned int cmd,
    void *arg, fuse_file_info *fi,
    unsigned int flags,
    void *data )
{
    gint32 ret = 0;
    do{
        if( cmd != FIONREAD )
        {
            ret = -ENOSYS;
            break;
        }

        CRpcServices* pSvc = GetIf();
        RLOCK_TESTMNT2( pSvc );

        CFuseMutex oLock( GetLock() );
        ret = oLock.GetStatus();
        if( ERROR( ret ) )
            break;

        *pSize = 0;   
        size_t* pSize = ( size_t* )pData;

        if( m_queReqs.size() )
            break;

        *pSize = GetBytesAvail();
        for( auto& elem : m_queTaskIds )
        {
            *pSize += elem.strResp.size() +
                sizeof( guint32 );
        }

    }while( 0 );

    return ret;
}

gint32 CFuseStmFile::OnReadStreamComplete(
    HANDLE hStream,
    gint32 iRet,
    BufPtr& pBuf,
    IConfigDb* pCtx )
{
    UNREFERENCED( pCtx );
    gint32 ret = 0;
    do{
        CFuseMutex oLock( GetLock() );
        ret = oLock.GetStatus();
        if( ERROR( ret ) )
            break;
        if( m_queReqs.empty() )
        {
            guint32 dwFlags = POLLIN;
            if( ERROR( iRet ) )
            {
                if( m_queIncoming.size() )
                    m_queIncoming.clear();
                ret = iRet;
                dwFlags |= POLLERR;
            }
            else
            {
                m_queIncoming.push_back( { pBuf, 0 } );
                if( m_queIncoming.size() >
                    MAX_STM_QUE_SIZE )
                    m_queIncoming.pop_front();
            }
            SetRevents( GetRevents() | dwFlags );
            fuse_pollhandle* ph = GetPollHandle();
            if( ph != nullptr )
            {
                fuse_notify_poll( ph );
                SetPollHandle( nullptr );
            }
            break;
        }

        if( ERROR( iRet ) )
        {
            if( !m_queIncoming.size() )
                m_queIncoming.clear();
            while( m_queReqs.size() )
            {
                auto& elem = m_queReqs.front();
                auto req = elem.req;
                fuseif_finish_interrupt( GetFuse(),
                    req, elem.pintr.get() );
                fuse_reply_err( req, iRet );
                m_queReqs.pop_front();
            }
            break;
        }

        m_queIncoming.push_back( { pBuf, 0 } );
        while( m_queReqs.size() > 0 &&
            m_queIncoming.size() > 0 )
        {
            auto& elem = m_queReqs.front();
            fuse_req_t req = elem.req;
            size_t& dwReqSize = elem.dwReqSize;
            auto d = elem.pintr.get();
            fuse_bufvec* bufvec = nullptr;

            // the vector to keep the buffer valid
            std::vector< BufPtr > vecRemoved;
            ret = FillBufVec( dwReqSize,
                m_queIncoming, vecRemoved, bufvec );
            if( ret == STATUS_PENDING )
            {
                // incoming data is not enough.
                ret = 0;
                break;
            }

            fuseif_finish_interrupt(
                    GetFuse, req, d );
            if( dwReqSize > 0 )
                fuse_reply_data(
                    req, bufvec, FUSE_BUF_SPLICE_MOVE );
            else
                fuse_reply_err( req, -EAGAIN );
            fuse_free_buf( bufvec );
            m_queReqs.pop_front();
        }

        if( m_queReqs.empty() )
            break;

        oLock.Unlock();
        InterfPtr pIf = GetIf();
        CRpcServices* pSvc = pIf;
        CStreamServerSync* pStmSvr = pIf;
        CStreamProxySync* pStmProxy = pIf;
        guint32 dwPReq = 0;
        if( pStmSvr != nullptr )
        {
            ret = pStmSvr->GetPendingReqs(
                dwPReq );
        }
        else
        {
            ret = pStmProxy->GetPendingReqs(
                dwPReq );
        }
        if( ERROR( ret ) )
            break;

        if( dwPReq > 0 )
            break;
        
        if( pStmSvr != nullptr )
        {
            ret = pStmSvr->ReadStreamAsync(
                hStream, pBuf, nullptr );
        }
        else
        {
            ret = pStmProxy->ReadStreamAsync(
                hStream, pBuf, nullptr );
        }
        if( ret == STATUS_PENDING )
        {
            ret = 0;
            break;
        }
        if( ERROR( ret ) )
            iRet = ret;

    }while( 1 );

    return ret;
}

gint32 CFuseStmFile::FillIncomingQue()
{
    gint32 ret = 0;
    do{

        InterfPtr pIf = GetIf();
        CStreamServerSync* pStmSvr = pIf;
        CStreamProxySync* pStmProxy = pIf;
        HANDLE hstm = GetStream();

        guint32 dwSize = 0;
        if( pStmSvr != nullptr )
        {
            ret = pStmSvr->GetPendingBytes(
                hstm, dwSize );
        }
        else
        {
            ret = pStmProxy->GetPendingBytes(
                hstm, dwSize );
        }
        if( ERROR( ret ) )
            break;
        
        while( dwSize > 0 )
        {
            BufPtr pBuf;
            if( pStmSvr != nullptr )
            {
                ret = pStmSvr->ReadStreamNoWait(
                    hstm, pBuf );
            }
            else
            {
                ret = pStmProxy->ReadStreamNoWait(
                    hstm, pBuf );
            }
            if( ERROR( ret ) )
                break;

            dwSize -= pBuf->size();
            m_queIncoming.push_back( { pBuf, 0 } );
        }

    }while( 0 );

    return ret;
}
 
gint32 CFuseStmFile::fs_read(
    fuse_req_t req, fuse_bufvec*& bufvec,
    off_t off, size_t size,
    fuse_file_info *fi
    std::vector< BufPtr >& vecBackup,
    fuseif_intr_data* d )
{
    gint32 ret = 0;

    if( size == 0 )
        return 0;

    PINTR pdata( d );
    do{
        CRpcServices* pIf = GetIf();
        RLOCK_TESTMNT2( pIf );

        CFuseMutex oFileLock( GetLock() );
        ret = oFileLock.GetStatus();
        if( ERROR( ret ) )
            break;

        if( m_queReqs.size() > MAX_STM_QUE_SIZE )
        {
            ret = -EAGAIN;
            break;
        }

        ret = FillIncomingQue();
        if( ERROR( ret ) )
            break;

        guint32 dwAvail = GetBytesAvail();
        guint32 dwPReqs = 0;

        InterfPtr pIf = GetIf();
        CRpcServices* pSvc = pIf;
        CStreamServerSync* pStmSvr = pIf;
        CStreamProxySync* pStmProxy = pIf;
        HANDLE hstm = GetStream();

        if( m_queReqs.size() &&
            m_queReqs.front().req->unique != req->uniuqe )
        {
            // we need to complete the earilier
            // requests before this request
            ret = STATUS_PENDING;
            m_queReqs.push_back(
                { req, size, pdata } );
            guint32 dwReqSize =
                    m_queReqs.front().size;
            if( dwReqSize <= dwAvail )
            {
                BufPtr pBuf = m_queIncoming.back().pBuf;
                m_queIncoming.pop_back();
                void (*func)=([]( CRpcServices* pIf,
                    CFuseStmFile* pFile, BufPtr& pBuf_ )
                {
                    gint32 ret = 0;
                    do{
                        if( pIf->IsServer() )
                        {
                            CFuseSvcServer* pSvr = ObjPtr( pIf );
                            RLOCK_TESTMNT2( pSvr );
                            pFile->OnReadStreamComplete(
                                hstm, 0, pBuf_, nullptr ); 
                        }
                        else
                        {
                            CFuseSvcProxy* pProxy = ObjPtr( pIf );
                            RLOCK_TESTMNT2( pProxy );
                            pFile->OnReadStreamComplete(
                                hstm, 0, pBuf_, nullptr ); 
                        }

                    }while( 0 );
                    return;
                });

                CIoManager* pMgr = pSvc->GetIoMgr();
                TaskletPtr pTask;
                ret = NEW_FUNCCALL_TASK( pTask,
                    pMgr, func, pSvc, this, pBuf );
                if( ERROR( ret ) )
                    break;
                ret = pMgr->RescheduleTask( pTask );
            }
            break;
        }

        guint32 dwBytesRead = 0;

        if( dwAvail >= size )
        {
            dwBytesRead = size;
        }
        else if( IsNonBlock() )
        {
            ret = -EAGAIN;
        }
        else
        {
            ret = STATUS_PENDING;
        }

        if( dwBytesRead > 0 )
        {
            ret = FillBufVec( dwBytesRead, m_queIncoming,
                vecBackup, bufvec );
            if( ret == STATUS_PENDING )
                ret = ERROR_STATE;
        }
        else if( ret == STATUS_PENDING )
        {
            m_queReqs.push_back(
                { req, size, pdata } );
        }

        if( pStmSvr != nullptr )
        {
            pStmSvr->GetPendingReqs( hstm, dwPReqs );
        }
        else
        {
            pStmProxy->GetPendingReqs( hstm, dwPReqs );
        }

        bool bRead = ( dwPReqs == 0 && m_queReqs.size() )
        if( !bRead )
            break;

        BufPtr pBuf;
        gint32 iRet = 0;
        if( pStmSvr != nullptr )
        {
            iRet = pStmSvr->ReadStreamAsync(
                hstm, pBuf, nullptr );
        }
        else
        {
            iRet = pStmProxy->ReadStreamAsync(
                hstm, pBuf, nullptr );
        }
        if( ERROR( iRet ) )
            break;

        if( iRet == STATUS_PENDING )
            break;

        m_queIncoming.push_back( { pBuf, 0 } );

    }while( 1 );

    return ret;
}


gint32 CFuseFileEntry::CopyFromPipe(
    BufPtr& pBuf, fuse_buf* src )
{
    gint32 ret = 0;
    auto len = fb.size();
    if( pBuf.IsEmpty() )
        pBuf.NewObj();
    ret = pBuf->Resize( len );
    if( ERROR( ret ) )
        return ret;

    gint32 res = 0;
    gint32 dst_off = 0;
    gint32 src_off = 0;
    gint32 copied = 0;
    while (len)
    {
        if (src->flags & FUSE_BUF_FD_SEEK)
        {
            res = pread(src->fd,
                pBuf->ptr() + dst_off, len,
                src->pos + src_off);
        }
        else
        {
            res = read(src->fd,
                pBuf->ptr() + dst_off, len );
        }
        if (res == -1)
        {
            if (!copied )
                ret = -errno;
            break;
        }

        if (res == 0)
            break;

        copied += res;
        if (!(src->flags & FUSE_BUF_FD_RETRY))
            break;

        dst_off += res;
        src_off += res;
        len -= res;
    }
    if( copied < len )
        pBuf->Resize( copied );

    return ret;
}

gint32 CFuseStmFile::SendBufVec( OUTREQ& oreq )
{
    gint32 ret = 0;
    do{
        fuse_req_t& req = oreq.req;
        fuse_bufvec* bufvec= oreq.bufvec; 
        gint32& idx = oreq.idx;
        fuseif_intr_data* pintr = oreq.pintr;

        InterfPtr pIf = GetIf();
        CRpcServices* pSvc = pIf;
        CStreamServerSync* pStmSvr = pIf;
        CStreamProxySync* pStmProxy = pIf;
        HANDLE hstm = GetStream();

        int i = bufvec->idx;
        std::vector< BufPtr > vecBufs;
        ret = CopyFromBufVec( vecBufs, bufvec );
        if( ERROR( ret ) )
            break;

        TaskletPtr pCb;
        ret = pCb.NewObj(
            clsid( CSyncCallback ) );
        if( ERROR( ret ) )
            break;

        BufPtr pBuf;
        pBuf = vecBufs.front();
        if( vecBufs.size() > 1 || IsNonBlock() )
        {
            // local copy is better than multiple round
            // trip.
            if( pBuf->IsNoFree() )
                pBuf.NewObj();
            else
            {
                vecBufs.pop_front();
            }
            for( auto& elem :vecBufs )
            {
                pBuf->Append(
                    elem->ptr(), elem->size() );
            }
        }

        // write can complete immediately, if the
        // system load is not very high.
        if( pStmSvr != nullptr )
        {
            if( IsNonBlock() )
                ret = pStmSvr->WriteStreamNoWait(
                    hstm, pBuf );
            else
                ret = pStmSvr->WriteStreamAsync(
                    hstm, pBuf, ( IEventSink* )pCb );
        }
        else
        {
            if( IsNonBlock() )
                ret = pStmProxy->WriteStreamNoWait(
                    hstm, pBuf );
            else
                ret = pStmProxy->WriteStreamAsync(
                    hstm, pBuf, pCb );
        }

        if( ret == STATUS_PENDING )
        {
            CSyncCallback* pSync = pCb;
            pSync->WaitForCompleteWakable();
            ret = pSync->GetError();
        }

        if( ret == ERROR_QUEUE_FULL )
        {
            SetFlowCtrl( true );
            ret = -EAGAIN;
            break;
        }

        if( ERROR( ret ) )
            break;

        bufvec->idx = bufvec->count;

    }while( 0 );

    return ret;
}

gint32 CFuseStmFile::fs_write_buf(
    fuse_req_t req,
    fuse_bufvec *bufvec,
    fuse_file_info *fi
    fuseif_intr_data* d )
{
    gint32 ret = 0;
    do{
        CRpcServices* pIf = GetIf();
        RLOCK_TESTMNT2( pIf );

        // this is a synchronous call since the bufvec
        // is controlled by the caller

        if( GetFlowCtrl() )
        {
            ret = -EAGAIN;
            break;
        }

        PINTR pintr( d );
        OUTREQ oreq =
            { req, bufvec, bufvec->idx, pintr };

        ret = SendBufVec( oreq );

    }while( 0 );

    return ret;
}

gint32 CFuseStmFile::OnWriteResumed()
{
    gint32 ret = 0;
    do{
        CFuseMutex oFileLock( GetLock() );
        pStmFile->SetFlowCtrl( false );
        fuse_pollhandle* ph =
                GetPollHandle();
        if( ph != nullptr )
        {
            fuse_notify_poll( ph );
            SetPollHandle( nullptr );
        }
        break;

    }while( 0 );

    return ret;
}

gint32 CFuseStmFile::OnWriteStreamComplete(
    HANDLE hStream,
    gint32 iRet,
    BufPtr& pBuf,
    IConfigDb* pCtx )
{
    UNREFERENCED( pCtx );
    gint32 ret = 0;
    do{
        CFuseMutex oFileLock( GetLock() );
        SetFlowCtrl( false );
        fuse_pollhandle* ph = GetPollHandle();
        if( ph != nullptr )
        {
            fuse_notify_poll( ph );
            SetPollHandle( nullptr );
        }

    }while( 0 );

    return ret;
}

gint32 CFuseStmFile::fs_poll(
    const char *path,
    fuse_file_info *fi,
    fuse_pollhandle *ph,
    unsigned *reventsp )
{
    gint32 ret = 0;
    do{
        RLOCK_TESTMNT3( path )

        CFuseMutex oLock( GetLock() );
        ret = oLock.GetStatus();
        if( ERROR( ret ) )
            break;
        auto oldph = GetPollHandle();
        if( ph != nullptr )
        {
            fuse_pollhandle_destroy( oldph );
            SetPollHandle( ph );
        }

        if( m_queIncoming.size() > 0 &&
            m_queReqs.empty() )
            *reventsp |= POLLIN;

        if( !GetFlowCtrl() )
            *reventsp |= POLLOUT;

        *reventsp = (
            fi->poll_events & GetRevents() );

        if( GetRevents() & POLLHUP )
            *reventsp |= POLLHUP;
        if( GetRevents() & POLLERR )
            *reventsp |= POLLERR;
        if( *reventsp != 0 )
        {
            SetRevents( ( ~*reventsp ) &
                GetRevents() );
        }

    }while( 0 );

    return ret;
}

gint32 CFuseFileEntry::CancelFsRequest(
    fuse_req_t req )
{
    //NOTE: must have file lock acquired
    gint32 ret = 0;
    bool bFound = false;

    auto itr = m_queReqs.begin();
    while( itr != m_queReqs.end() )
    {
        if( req->unique == itr->req->unique)
        {
            bFound = true;
            break;
        }
        itr++;
    }
    if( bFound )
    {
        auto d = itr->pintr.get();
        auto req = itr->req;
        fuseif_finish_interrupt(
                GetFuse(), req, d );
        fuse_reply_err( req, ECANCELED );
        m_queReqs.erase( itr );
        return STATUS_SUCCESS;
    }

    return -ENOENT;
}

gint32 CFuseFileEntry::CancelFsRequests()
{
    //NOTE: must have file lock acquired
    gint32 ret = 0;
    for( auto& elem : m_queReqs ) 
    {
        fuseif_finish_interrupt( GetFuse(),
            elem.req, 
            elem.pintr.get() );
        fuse_reply_err(
            std::get<0>( elem ), ECANCELED );
    }
    m_queReqs.clear();

    return ret;
}

gint32 CFuseEvtFile::fs_read(
    fuse_req_t req, fuse_bufvec*& bufvec,
    off_t off, size_t size,
    fuse_file_info *fi
    std::vector< BufPtr >& vecBackup,
    fuseif_intr_data* d )
{
    gint32 ret = 0;
    do{
        if( size == 0 )
            break;

        CRpcServices* pSvc = GetIf();
        RLOCK_TESTMNT2( pSvc );

        CFuseMutex oFileLock( GetLock() );
        ret = oFileLock.GetStatus();
        if( ERROR( ret ) )
            break;

        PINTR pdata( d );

        if( m_queReqs.size() )
        {
            m_queReqs.push_back(
                { req, size, pdata } );
            ret = STATUS_PENDING;
            break;
        }

        guint32 dwAvail = GetBytesAvail();
        if( dwAvail >= size )
        {
            FillBufVec( size,
                m_queIncoming,
                vecBackup, bufvec );
        }
        else if( IsNonBlock() )
        {
            ret = -EAGAIN;
        }
        else
        {
            ret = STATUS_PENDING;
            m_queReqs.push_back(
                { req, size, pdata } );
        }

    }while( 0 );

    return ret;
}

gint32 CFuseEvtFile::ReceiveEvtJson(
        const stdstr& strMsg )
{
    gint32 ret = 0;
    do{
        CFuseMutex oFileLock( GetLock() );
        ret = oFileLock.GetStatus();
        if( ERROR( ret ) )
            break;

        BufPtr pBuf;
        if( m_queReqs.size() )
        {
            BufPtr pSizeBuf( true );
            pBuf = NewBufNoAlloc(
                strMsg.c_str(), strMsg.size(), true );
            *pSizeBuf = pBuf->size();
            m_queIncoming.push_back( { pSizeBuf, 0 } );
            m_queIncoming.push_back( { pBuf, 0 } );
        }
        else
        {
            pBuf.NewObj();
            pBuf->Append(
                strMsg.c_str(), strMsg.size());
            m_queIncoming.push_back( { pSizeBuf, 0 } );
            m_queIncoming.push_back( {pBuf, 0} );
            fuse_pollhandle* ph = GetPollHandle();
            if( ph != nullptr )
            {
                fuse_notify_poll( ph );
                SetPollHandle( nullptr );
            }
        }

        while( m_queReqs.size() > 0 &&
            m_queIncoming.size() > 0 )
        {
            auto& elem = m_queReqs.front();
            fuse_req_t req = elem.req;
            size_t& dwReqSize = elem.dwReqSize;
            auto d = elem.pintr.get();
            fuse_bufvec* bufvec = nullptr;

            // the vector to keep the buffer valid
            std::vector< BufPtr > vecRemoved;
            ret = FillBufVec( dwReqSize,
                m_queIncoming, vecRemoved, bufvec );
            if( ret == STATUS_PENDING )
            {
                // more incoming data is needed
                ret = 0;
                break;
            }

            fuseif_finish_interrupt(
                    GetFuse, req, d );
            if( dwReqSize > 0 )
                fuse_reply_data(
                    req, bufvec, FUSE_BUF_SPLICE_MOVE );
            else
                fuse_reply_err( req, -EAGAIN );
            fuse_free_buf( bufvec );
            m_queReqs.pop_front();
        }

    }while( 0 );

    return ret;
}

gint32 CFuseFileEntry::CopyFromBufVec(
     std::vector< BufPtr >& vecBuf,
     fuse_bufvec* bufvec )
{
    gint32 ret = 0;
    do{
        int i = bufvec->idx;
        for( ;i < bufvec->count; i++ )
        {
            fuse_buf& fb = bufvec->buf[ i ];
            if( fb.flags & FUSE_BUF_IS_FD )
            {
                ret = CopyFromPipe( pBuf, &fb );
                if( ERROR( ret ) )
                    break;
                // end of file
                if( pBuf.IsEmpty() || pBuf->empty() )
                    continue;
            }
            else
            {
                char* pData = fb.mem;
                guint32 dwSize = fb.size;
                if( i == bufvec->idx )
                {
                    pData += bufvec->off;
                    dwSize -= bufvec->off;
                }
                pBuf = NewBufNoAlloc(
                    pData, dwSize, true );
            }
            vecBuf.push_back( pBuf );
        }

    }while( 0 );

    return ret;
}

static std::unique_ptr< Json::CharReader > g_pReader;

gint32 CFuseEvtFile::GetReqSize(
    BufPtr& pReqSize,
    BUFARR& vecBufs )
{
    gint32 ret = 0;
    do{
        guint32 dwSize = sizeof( guint32 );
        if( pReqSize->size() == dwSize )
            break;

        if( pReqSize->size() > dwSize )
        {
            ret = -EOVERFLOW;
            break;
        }

        guint32 dwCopy = sizeof( guint32 ) -
            pReqSize->size();
        
        while( vecBufs.size() )
        {
            auto& elem = vecBufs.front();
            if( elem->size() <= dwCopy )
            {
                pReqSize->Append(
                    elem->ptr(), elem->size() );
                dwCopy -= elem->size();
                vecBufs.pop_front();
                if( dwCopy == 0 )
                    break;
                continue;
            }
            pReqSize->Append(
                elem->ptr(), dwCopy );
            elem->SetOffset(
                elem->offset() + dwCopy );
            dwCopy = 0;
            break;
        }

        if( dwCopy > 0 )
        {
            // wait for more input
            ret = STATUS_PENDING;
            break;
        }

    }while( 0 )

    return ret;
}

gint32 CFuseRespFileSvr::fs_write_buf(
    fuse_req_t req,
    fuse_bufvec *bufvec,
    fuse_file_info *fi
    fuseif_intr_data* d )
{
    gint32 ret = 0;

    PINTR pintr( d );

    CFuseSvcServer* pSvr = GetIf();
    if( pSvr == nullptr )
        return -EFAULT;

    ret = CopyFromBufVec(
            m_vecOutBufs, bufvec );
    if( ERROR( ret ) )
        return ret;

    do{
        RLOCK_TESTMNT2( pSvr );

        // receive the 4-byte size in local endian
        if( m_pReqSize->size() < sizeof( guint32 ) )
        {
            ret = GetReqSize(
                m_pReqSize, m_vecOutBufs );
            if( ERROR( ret ) )
                break;
            if( ret == STATUS_PENDING )
            {
                // wait for more input
                bufvec->idx = bufvec->count;
                ret = 0;
                break;
            }
        }
        guint32 dwReqSize =
            ( guint32& )*m_pReqSize;
        if( dwReqSize >
            MAX_BYTES_PER_TRANSFER * 2 )
        {
            ret = -EFBIG;
            break;
        }

        // receive the bytearray of dwReqSize bytes
        guint32 dwAvail = 0;
        for( auto& elem : m_vecOutBufs )
            dwAvail += elem->size();

        std::vector< BufPtr > vecBufs;
        if( dwAvail < dwReqSize )
        {
            // make a copy and waiting for more input
            for( auto& elem : m_vecOutBufs )
            {
                BufPtr pCopy;
                if( elem->IsNoFree() )
                {
                    pCopy.NewObj();
                    ret = pCopy->Resize(
                        elem->size() );
                    if( ERROR( ret ) )
                        break;

                    memcpy( pCopy->ptr(),
                        elem->ptr(),
                        elem->size() );
                    elem = pCopy;
                }
            }
            bufvec->idx = bufvec->count;
            ret = 0;
            break;
        }

        // has received complete requests
        guint32 dwToAdd = dwReqSize;
        while( m_vecOutBufs.size() )
        {
            auto& elem = m_vecOutBufs.front();
            if( dwToAdd >= elem->size() )
            {
                dwToAdd -= elem->size();
                vecBufs.push_back( elem );
                m_vecOutBufs.pop_front();
                if( dwToAdd > 0 )
                    continue;
                break;
            }

            BufPtr pBuf = NewBufNoAlloc(
                elem->ptr(), dwToAdd );
            vecBufs.push_back( pBuf );
            elem->SetOffset(
                elem->offset() + dwToAdd );
            dwToAdd = 0;
            break;
        }

        // merge to a single buffer
        BufPtr pBuf = vecBufs.front();
        if( vecBufs.size() > 1 )
        {
            if( pBuf->IsNoFree() )
                pBuf.NewObj();
            else
                vecBufs.pop_front();
            for( auto& elem : vecBufs )
            {
                pBuf->Append(
                    elem->ptr(), elem->size() );
            }
        }
        vecBufs.clear();

        Json::Value valResp;
        if( !g_pReader->parse( pBuf->ptr(),
            pBuf->ptr() + pBuf->size(),
            &valResp, nullptr ) )
        {
            ret = -EINVAL;
            break;
        }

        // send the response
        CFuseSvcServer* pSvr = GetIf();
        {
            ret = pSvr->DispatchMsg( valResp );
        }
        if( ret == STATUS_PENDING )
            ret = 0;

        if( m_vecOutBufs.empty() )
        {
            bufvec->idx = bufvec->count;
            ret = 0;
            break;
        }

        m_pReqSize->Resize( 0 );

    }while( 1 );

    return ret;
}

gint32 CFuseRespFileProxy::ReceiveMsgJson(
    const stdstr& strMsg,
    guint64 qwReqId )
{
    gint32 ret = 0;
    do{
        InterfPtr pIf = GetIf();
        CFuseSvcProxy* pProxy = pIf;
        CHILD_TYPE pEnt = pProxy->GetSvcDir();
        CFuseSvcDir* pDir = static_cast
            < CFuseSvcDir* >( pEnt.get() );
        auto p = pDir->GetChild( JSON_REQ_FILE );
        CFuseReqFileProxy* pReqFile = static_cast
            < CFuseReqFileProxy* >( p );
        pReqFile->RemoveResp( qwReqId );

        ret = super::ReceiveMsgJson(
            strMsg, qwReqId );

    }while( 0 );

    return ret;
}

gint32 CFuseReqFileProxy::RemoveResp(
    guint64 qwReqId )
{
    CFuseMutex oFileLock( GetLock() );
    gint32 ret = oFileLock.GetStatus();
    if( ERROR( ret ) )
        return ret;

    bool bFound = false;
    auto itr = m_queTaskIds.begin();
    while( itr != m_queTaskIds.end() )
    {
        if( itr->qwReqId == qwReqId )
        {
            bFount = true;
            break;
        }
        itr++;
    }

    ret = -ENOENT;
    if( bFound )
    {
        m_queTaskIds.erase( itr );
        ret = 0;
    }

    return ret;
}

gint32 CFuseReqFileProxy::ConvertAndFillBufVec(
    guint32 dwAvail,
    guint32 dwNewBytes,
    guint32 dwReqSize, 
    std::vector< BufPtr >& vecBackup,
    fuse_bufvec*& bufvec )
{
    gint32 ret = 0;
    if( dwReqSize == 0 )
        return -EINVAL;

    if( dwAvail + dwNewBytes < dwReqSize )
        return -EINVAL;

    do{
        guint32 dwBytesCvt = dwReqSize - dwAvail;
        while( m_queTaskIds.size() )
        {
            auto& elem = m_queTaskIds.front();
            BufPtr pBuf( true );
            *pBuf = ( guint32 )elem.strResp.size();

            pBuf->Append( elem.strResp.c_str(),
                elem.strResp.size() );

            m_queIncoming.push_back( { pBuf, 0 } );
            m_queTaskIds.pop_front();

            if( dwBytesCvt <= elem.strResp.size() +
                sizeof( guint32 ) )
                break;
            dwBytesCvt -= elem.strResp.size();
        }

        ret = FillBufVec(
            dwReqSize, m_queIncoming,
            vecBackup, bufvec );

    }while( 0 );

    return ret;
}

gint32 CFuseReqFileProxy::fs_read(
    fuse_req_t req,
    fuse_bufvec*& buf,
    off_t off, size_t size,
    fuse_file_info *fi
    std::vector< BufPtr >& vecBackup,
    fuseif_intr_data* d )
{
    gint32 ret = 0;
    do{
        if( size == 0 )
            break;

        CRpcServices* pSvc = GetIf();
        RLOCK_TESTMNT2( pSvc );

        CFuseMutex oFileLock( GetLock() );
        ret = oFileLock.GetStatus();
        if( ERROR( ret ) )
            break;

        PINTR pdata( d );
        guint32 dwAvail = GetBytesAvail();
        guint32 dwNewBytes = 0;
        for( auto& elem : m_queTaskIds )
        {
            dwNewBytes += elem.strResp.size() +
                sizeof( guint32 );
            if( dwAvail >= size )
                break;
        }

        if( dwAvail >= size )
        {
            FillBufVec( size,
                m_queIncoming,
                vecBackup, bufvec );
        }
        else if( dwAvail + dwNewBytes >= size )
        {
            ConvertAndFillBufVec( dwAvail,
                dwNewBytes, size, 
                vecBackup, bufvec );
        }
        else
        {
            if( IsNonBlock() )
                ret = -EAGAIN;
            else
            {
                ret = STATUS_PENDING;
                m_queReqs.push_back(
                    { req, size, pdata } );
            }
        }

    }while( 0 );

    return ret;
}

gint32 CFuseReqFileProxy::fs_write_buf(
    fuse_req_t req,
    fuse_bufvec *bufvec,
    fuse_file_info *fi
    fuseif_intr_data* d )
{
    gint32 ret = 0;
    PINTR pintr( d );

    CFuseSvcProxy* pProxy = GetIf();
    if( pProxy == nullptr )
        return -EFAULT;

    ret = CopyFromBufVec(
            m_vecOutBufs, bufvec );
    if( ERROR( ret ) )
        return ret;

    do{
        RLOCK_TESTMNT2( pProxy );

        // receive the size value of 4-bytes in local
        // endian
        if( m_pReqSize->size() < sizeof( guint32 ) )
        {
            ret = GetReqSize(
                m_pReqSize, m_vecOutBufs );

            if( ERROR( ret ) )
                break;
            if( ret == STATUS_PENDING )
            {
                // wait for more input
                bufvec->idx = bufvec->count;
                ret = 0;
                break;
            }
        }

        guint32 dwReqSize =
            ( guint32& )*m_pReqSize;
        if( dwReqSize >
            MAX_BYTES_PER_TRANSFER * 2 )
        {
            ret = -EFBIG;
            break;
        }

        // receive the bytearray of dwReqSize bytes
        guint32 dwAvail = 0;
        for( auto& elem : m_vecOutBufs )
            dwAvail += elem->size();

        std::vector< BufPtr > vecBufs;
        if( dwAvail < dwReqSize )
        {
            // need more input
            for( auto& elem : m_vecOutBufs )
            {
                BufPtr pCopy;
                if( elem->IsNoFree() )
                {
                    pCopy.NewObj();
                    ret = pCopy->Resize(
                        elem->size() );
                    if( ERROR( ret ) )
                        break;

                    memcpy( pCopy->ptr(),
                        elem->ptr(),
                        elem->size() );
                    elem = pCopy;
                }
            }
            bufvec->idx = bufvec->count;
            ret = 0;
            break;
        }
        // received one or more complete requests
        guint32 dwToAdd = dwReqSize;
        while( m_vecOutBufs.size() )
        {
            auto& elem = m_vecOutBufs.front();
            if( dwToAdd >= elem->size() )
            {
                dwToAdd -= elem->size();
                vecBufs.push_back( elem );
                m_vecOutBufs.pop_front();
                if( dwToAdd > 0 )
                    continue;
                break;
            }

            BufPtr pBuf = NewBufNoAlloc(
                elem->ptr(), dwToAdd );
            vecBufs.push_back( pBuf );
            elem->SetOffset(
                elem->offset() + dwToAdd );
            dwToAdd = 0;
            break;
        }

        BufPtr pBuf = vecBufs.front();
        if( vecBufs.size() > 1 )
        {
            if( pBuf->IsNoFree() )
                pBuf.NewObj();
            else
                vecBufs.pop_front();
            for( auto& elem : vecBufs )
            {
                pBuf->Append(
                    elem->ptr(), elem->size() );
            }
        }
        vecBufs.clear();

        Json::Value valReq, valResp;
        if( !g_pReader->parse( pBuf->ptr(),
            pBuf->ptr() + pBuf->size(),
            &valReq, nullptr ) )
        {
            ret = -EINVAL;
            break;
        }
        
        {
            if( pProxy->GetState() != stateConnected )
            {
                ret = -EBADF;
                break;
            }
            ret = pProxy->DispatchReq(
                nullptr, valReq, valResp );
        }

        Json::Value& valrid =
            valResp[ JSON_ATTR_REQCTXID ];

        guint64 qwReqId = valrid.asUInt64();

        Json::StreamWriterBuilder oBuilder;
        oBuilder["commentStyle"] = "None";
        stdstr strResp =
            Json::writeString( oBuilder, oResp );

        if( ret == STATUS_PENDING )
        {
            // append the response to the
            // m_queTaskIds of request file
            m_queTaskIds.push_back(
                { strResp, qwReqId } );
            ret = 0;
        }
        else if( SUCCEEDED( ret ) )
        {
            // append the response to the file
            // JSON_RESP_FILE for an immediate return.
            void (*func)( CFuseSvcProxy*,
                const stdstr& )=([](
                    CFuseSvcProxy* pIf,
                    const stdstr& strResp )
            {
                gint32 ret = 0;
                do{
                    RLOCK_TESTMNT2( pIf );
                    auto pResp = _pDir->GetChild(
                        JSON_RESP_FILE );
                    CFuseEvtFile* p = dynamic_cast
                        < CFuseEvtFile* >( pResp );
                    p->ReceiveEvtJson( strMsg );

                }while( 0 );

                return;
            });

            TaskletPtr pTask;
            ret = NEW_FUNCCALL_TASK( pTask,
                this->GetIoMgr(), func,
                pProxy, strResp );
            if( ERROR( ret ) )
                break;

            GetIoMgr()->RescheduleTask( pTask );
        }

        m_pReqSize->Resize( 0 );
        if( m_vecOutBufs.empty() )
        {
            bufvec->idx = bufvec->count;
            ret = 0;
            break;
        }

    }while( 1 );

    if( ERROR( ret ) )
    {
        m_pReqSize.Clear();
        m_vecOutBufs.clear();    
    }

    return ret;
}

} // namespace

gint32 fuseop_open(
    const char* path, fuse_file_info * fi)
{
    if( path == nullptr || fi == nullptr )
        return -EINVAL;

    CFuseObjBase* pObj;
    gint32 ret = GetFuseObj( path, pObj );
    if( ERROR( ret ) )
        return ret;

    CFuseMutex oLock( pObj->GetLock() );
    ret = oLock.GetStatus();
    if( ERROR( ret ) )
        return ret;
    pObj->IncOpCount();

    if( fi->flags & O_NONBLOCK )
        SetNonBlock();

    fi->direct_io = 1;
    fi->keep_cache = 0;
    fi->nonseekable = 1;
    fi->fh = ( guint64 )pObj;

    return ret;
}

gint32 fuseop_release(
    const char* path, fuse_file_info * fi )
{
    CFuseObjBase* pObj = reinterpret_cast
        < CFuseObjBase* >( fi->fh );
    if( pObj == nullptr )
        return -EFAULT;
    do{
        CFuseStmFile* pStm = dynamic_cast
            < CFuseStmFile* >( pObj );
        if( pStm == nullptr )
        {
            CFuseMutex oFileLock(
                pObj->GetLock() );
            ret = oFileLock.GetStatus();
            if( ERROR( ret ) )
                break;
            pObj->DecOpCount();
            break;
        }

        CRpcServices* pSvc = pStm->GetIf();
        WLOCK_TESTMNT2( pSvc );

        if( pStm->DecOpCount() > 0 )
            break;

        if( !pStm->IsHidden() )
            break;

        CFuseStmDir* pParent = dynamic_cast
            < CFuseStmDir* >( pStm->GetParent() );
        if( pParent == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        pStm->CancelFsRequests();

        // keep a copy of pStm
        CHILD_TYPE pEnt;
        stdstr strName = pStm->GetName();
        ret = pParent->GetChild( strName, pEnt );
        if( ERROR( ret ) )
            break;

        pParent->RemoveChild( strName );
        fuse_ino_t inoParent =
                pParent->GetObjId();

        oSvcLock.Unlock();

        fuse_ino_t inoChild = pStm->GetObjId();
        fuse* pFuse = GetFuse();
        if( pFuse == nullptr )
            break;

        fuse_session* se =
            fuse_get_session( pFuse );

        fuse_lowlevel_notify_delete( se,
            inoParent, inoChild,
            strName.c_str(),
            strName.size() );


        InterfPtr pIf = pStm->GetIf();
        HANDLE hStream = pStm->GetStream();
        CloseChannel( pIf, hStream );

    }while( 0 );

    return STATUS_SUCCESS;
}

gint32 fuseop_opendir(
    const char* path, fuse_file_info * fi )
{ return fuseop_open( path, fi ); }

gint32 fuseop_releasedir(
    const char* path, fuse_file_info * fi )
{ return fuseop_release( path, fi ); }

gint32 fuseop_unlink( const char* path )
{
    if( path == nullptr )
        return -EINVAL;
    do{
        CFuseObjBase* pObj;
        gint32 ret = GetFuseObj( path, pObj );
        if( ERROR( ret ) )
            return ret;
        if( pObj->GetClsid() !=
            clsid( CFuseStmFile ) )
        {
            ret = -ENOSYS;
            break;
        }

        CFuseStmFile* pStmFile =
            static_cast< CFuseStmFile* >( pObj );

        CRpcServices* pSvc = pStmFile->GetIf();
        WLOCK_TESTMNT2( pSvc );

        if( !pStmFile->IsHidden() )
            break;

        CFuseMutex oFileLock(
                pStmFile->GetLock() );
        ret = oFileLock.GetStatus();
        if( ERROR( ret ) )
            break;
        if( pStmFile->GetOpCount() > 0 )
        {
            pStmFile->SetHidden();
            break;
        }

        pStmFile->CancelFsRequests();

        CFuseStmDir* pParent = dynamic_cast
            < CFuseStmDir* >( pObj->GetParent() );
        if( pParent == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        // keep a copy of pObj
        CHILD_TYPE pEnt;
        stdstr strName = pObj->GetName();
        ret = pParent->GetChild( strName, pEnt );
        if( ERROR( ret ) )
            break;

        pParent->RemoveChild( strName );

        oFileLock.Unlock();
        oSvcLock.Unlock();

        InterfPtr pIf = pStmFile->GetIf();
        HANDLE hStream = pStmFile->GetStream();
        CloseChannel( pIf, hStream );

    }while( 0 );

    return ret;
}

gint32 fuseop_create( const char* path,
    mode_t mode, fuse_file_info* fi )
{
    if( path == nullptr || fi == nullptr )
        return -EINVAL;
    gint32 ret = 0;
    do{
        stdstr strPath( path );
        size_t pos = strPath.rfind( '/' );
        if( pos + 1 == strPath.size() )
        {
            ret = -EINVAL;
            break;
        }
        stdstr strName =
            strPath.substr( pos + 1 );
        strPath.erase( pos );

        CFuseObjBase* pObj;
        gint32 ret = GetFuseObj( strPath, pObj );
        if( ERROR( ret ) )
            break;

        CFuseStmDir* pDir = dynamic_cast
            < CFuseStmDir* >( pObj );
        if( unlikely( pDir == nullptr ) )
        {
            // create allowed only under STREAM_DIR
            ret = -ENOSYS;
            break;
        }

        CParamList oParams;
        oParams.SetPointer(
            propIoMgr, pSvc->GetIoMgr() );

        TaskletPtr pCallback;
        ret = pCallback.NewObj(
            clsid( CIoReqSyncCallback ),
            oParams.GetCfg() );
        if( ERROR( ret ) )
            break;

        CIoReqSyncCallback* pSync = pCallback;

        CParamList oDesc;
        oDesc[ propNodeName ] = strName;
        HANDLE hStream = INVALID_HANDLE;
        ret = pSvc->StartStream(
            hStream, oDesc.GetCfg(), pSync );
        if( ERROR( ret ) )
            break;
        if( ret == STATUS_PENDING )
        {
            // if ctrl-c, it is possible the stream be
            // created invisible, but no harm
            ret = pSync->WaitForCompleteWakable();
            if( ERROR( ret ) )
                break;
            ret = pSync->GetError();
            if( ERROR( ret ) )
                break;
        }

        CStreamProxySync* pSvc =
            pDir->GetIf();

        RLOCK_TESTMNT2( pSvc );

        CFuseObjBase* pObj = new CFuseStmFile(
            strName, hStream, pSvc );
        auto pEnt = std::shared_ptr
                < CDirEntry >( pObj ); 
        fi->fh = ( guint64 )pObj;
        fi->direct_io = 1;
        fi->keep_cache = 0;
        CFuseStmFile* pStm = static_cast
            < CFuseStmFile* >( pObj );
        pStm->IncOpCount();
        pDir->AddChild( pEnt );

    }while( 0 );

    return ret;
}

int fuseop_poll(const char *path,
    fuse_file_info *fi,
    fuse_pollhandle *ph,
    unsigned *reventsp )
{
    gint32 ret = 0;
    do{
        CFuseObjBase* pObj = fi->fh;
        if( pObj == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        ret = pObj->fs_poll(
            path, fi, ph, reventsp );

    }while( 0 );

    return ret;
}

void fuseop_init(
   fuse_conn_info *conn,
   fuse_config *cfg )
{
    UNREFERENCED( conn );
    cfg->entry_timeout = NO_TIMEOUT;
    cfg->attr_timeout = NO_TIMEOUT;
    cfg->negative_timeout = 0;
    cfg->direct_io = 1;
    cfg->kernel_cache = 0;

    // init some other stuffs
    Json::CharReaderBuilder oBuilder;
    g_pReader.reset( oBuilder.newCharReader() );
}

gint32 InitRootIf(
    CIoManager* pMgr, bool bProxy )
{
    gint32 ret = 0;
    do{
        CParamList oParams;
        oParams.SetPointer( propIoMgr, this );
        oParams.SetBoolProp( propNoPort, true );

        EnumClsid iClsid;
        if( bProxy )
            iClsid = clsid( CFuseRootProxy );
        else
            iClsid = clsid( CFuseRootServer );

        ret = GetRootIf().NewObj(
            iClsid, oParams.GetCfg() );
        if( ERROR( ret ) )
            break;

        CRpcServices* pIf = GetRootIf();
        ret = pIf->Start();

    }while( 0 );
    return ret;
}


void fuseif_tweak_llops( fuse_session* se );

static gint32 fuseif_unmount()
{
    InterfPtr& pIf = GetRootIf();
    CRpcServices* pSvc = GetRootIf();
    if( pSvc->IsServer() )
    {
        CFuseRootServer* pSvr = GetRootIf();
        return pSvr->DoUnmount();
    }
    CFuseRootProxy* pProxy = GetRootIf();
    return pProxy->DoUnmount();
}

gint32 fuseif_daemonize( fuse_args& args,
    fuse_cmdline_opts& opts,
    int argc, char **argv, )
{
    gint32 res = STATUS_SUCCESS;

    if (fuse_parse_cmdline(&args, &opts) != 0)
        return 0;

    if (opts.show_version) {
        printf("FUSE library version %s\n", fuse_pkgversion());
        fuse_lowlevel_version();
        res = 0;
        goto out1;
    } else if (opts.show_help) {
        // fuseif_show_help(argv[0]);
        fuse_cmdline_help();
        fuse_lib_help(&args);
        res = 0;
        goto out1;
    } else if (!opts.mountpoint) {
        fprintf(stderr, "error: no mountpoint specified\n");
        res = 1;
        goto out1;
    }

    /*if (fuse_daemonize(opts.foreground) != 0)
        ret = -ERROR_FAIL;
    */
out1:
    free(opts.mountpoint);
    fuse_opt_free_args(&args);
    if( res != 0 )
        return -EINVAL;
    return res;
}

gint32 fuseif_main( fuse_args& args,
    fuse_cmdline_opts& opts )
{
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    struct fuse_cmdline_opts opts;
    struct fuse *fuse;
    struct fuse_loop_config config;
    int res;

    // if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1)
    //     return 1;

    if (fuse_parse_cmdline(&args, &opts) != 0)
        return 1;

    fuse = fuse_new(&args, &xmp_oper, sizeof(xmp_oper), NULL);
    if (fuse == NULL) {
        res = 1;
        goto out1;
    }
    SetFuse( fuse );

    if (fuse_mount(fuse,opts.mountpoint) != 0) {
        res = 1;
        goto out2;
    }

    struct fuse_session *se = fuse_get_session(fuse);
    if (fuse_set_signal_handlers(se) != 0) {
        res = 1;
        goto out3;
    }
    fuseif_tweak_llops( se );

    if (opts.singlethread)
        res = fuse_loop(fuse);
    else {
        config.clone_fd = opts.clone_fd;
        config.max_idle_threads = opts.max_idle_threads;
        res = fuse_loop_mt(fuse, &config);
    }
    if (res)
        res = 1;

    fuse_remove_signal_handlers(se);
out3:
    fuseif_unmount();
    fuse_unmount(fuse);
out2:
    fuse_destroy(fuse);
out1:
    free(opts.mountpoint);
    fuse_opt_free_args(&args);
    if( res != 0 )
        return ERROR_FAIL;
    return res;
}

