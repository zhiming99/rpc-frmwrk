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
#include "rpc.h"
using namespace rpcf;

#include "fuseif.h"
#include "ridl/serijson.h"
#include "fuse_i.h"
#include <sys/ioctl.h>
#include <unistd.h>

void fuseif_finish_interrupt(
    fuse *f, fuse_req_t req,
    fuseif_intr_data *d);

void fuseif_prepare_interrupt(
    fuse *f, fuse_req_t req,
    fuseif_intr_data *d);

void fuseif_free_buf(struct fuse_bufvec *buf);

namespace rpcf
{
InterfPtr g_pRootIf;
static std::unique_ptr< Json::CharReader > g_pReader;

InterfPtr& GetRootIf()
{ return g_pRootIf; }

gint32 GetSvcDir( const char* path,
    CFuseObjBase*& pSvcDir,
    std::vector< stdstr >& v )
{
    auto pIf = GetRootIf();
    CFuseRootServer* pSvr = pIf;
    CFuseRootProxy* pCli = pIf;
    gint32 ret = 0;
    if( pSvr != nullptr )
    {
        ret = pSvr->GetSvcDir( path, pSvcDir, v );
    }
    else
    {
        ret = pCli->GetSvcDir( path, pSvcDir, v );
    }

    return ret;
}

CFuseSvcDir* GetSvcDir( CRpcServices* pIf )
{
    CFuseSvcServer* pSvr = ObjPtr( pIf );
    CFuseSvcProxy* pCli = ObjPtr( pIf );
    CFuseSvcDir* pSvcDir = nullptr;
    if( pSvr != nullptr )
    {
        pSvcDir = static_cast< CFuseSvcDir* >
            ( pSvr->GetSvcDir().get() );
    }
    else
    {
        pSvcDir = static_cast< CFuseSvcDir* >
            ( pCli->GetSvcDir().get() );
    }
    return pSvcDir;
}

bool IsUnmounted( CRpcServices* pIf )
{
    CFuseSvcServer* pSvr = ObjPtr( pIf );
    CFuseSvcProxy* pCli = ObjPtr( pIf );
    CFuseSvcDir* pSvcDir = nullptr;
    if( pSvr != nullptr )
        return pSvr->IsUnmounted();

    return pCli->IsUnmounted();
}

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

/*gint32 GetFuseObj( const stdstr& strPath,
    CFuseObjBase*& pDir )
{
    CRpcServices* pSvc = GetRootIf();
    if( pSvc->IsServer() )
    {
        CFuseRootServer* pSvr = GetRootIf();
        return pSvr->GetFuseObj( strPath, pDir );
    }
    CFuseRootProxy* pProxy = GetRootIf();
    return pProxy->GetFuseObj( strPath, pDir );
}*/

gint32 CloseChannel(
    ObjPtr& pIf, HANDLE hStream )
{
    if( hStream == INVALID_HANDLE ||
        pIf.IsEmpty() )
        return -EINVAL;

    CRpcServices* pSvc = pIf;
    if( pSvc->GetState() != stateConnected )
        return ERROR_STATE;

    gint32 ret = 0;
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

gint32 AddSvcPoint(
    const stdstr& strObj,
    const stdstr& strDesc,
    EnumClsid iClsid,
    bool bProxy )
{
    if( bProxy )
    {
        CFuseRootProxy* pRoot = GetRootIf();
        return pRoot->AddSvcPoint(
            strObj, strDesc, iClsid );
    }
    CFuseRootServer* pRoot = GetRootIf();
    return pRoot->AddSvcPoint(
        strObj, strDesc, iClsid );
}


gint32 CFuseObjBase::AddChild(
    const CHILD_TYPE& pEnt )
{
    gint32 ret = 0;
    do{
        ret = super::AddChild( pEnt );
        if( ERROR( ret ) )
            break;
        
        auto pIf = GetRootIf();
        CFuseRootServer* pSvr = pIf;
        CFuseRootProxy* pCli = pIf;
        FHCTX fhctx;
        CFuseObjBase* pObj = dynamic_cast
            < CFuseObjBase* >( pEnt.get() );
        fhctx.pFile = pEnt;
        fhctx.pSvc = pObj->GetIf();
        if( pSvr != nullptr )
        {
            ret = pSvr->Add2FhMap(
                ( HANDLE )pObj, fhctx );
        }
        else
        {
            ret = pCli->Add2FhMap(
                ( HANDLE )pObj, fhctx );
        }

    }while( 0 );
    return ret;
}

gint32 CFuseObjBase::RemoveChild(
    const std::string& strName )
{
    gint32 ret = 0;
    do{
        CHILD_TYPE pEnt;
        ret = GetChild( strName, pEnt );
        if( ERROR( ret ) )
            break;

        ret = super::RemoveChild( strName );
        if( ERROR( ret ) )
            break;

        auto pObj = dynamic_cast
            <CFuseObjBase*>( pEnt.get() );
        
        auto pIf = GetRootIf();
        CFuseRootServer* pSvr = pIf;
        CFuseRootProxy* pCli = pIf;
        if( pSvr )
        {
            pSvr->RemoveFromFhMap(
                ( HANDLE )pObj );
        }
        else
        {
            pSvr->RemoveFromFhMap(
                ( HANDLE )pObj );
        }

    }while( 0 );

    return ret;
}

gint32 CFuseObjBase::fs_access(
    const char * path,
    fuse_file_info* fi,
    int flags )
{
    guint32 dwMode = GetMode();
    gint32 ret = -EACCES;
    do{
        bool bReq =
           (  ( flags & W_OK ) != 0 );
        bool bCur =
           ( ( dwMode & S_IWUSR ) != 0 );
        if( bReq != bCur )
            break;

        bReq = (  ( flags & R_OK ) != 0 );
        bCur = ( ( dwMode & S_IRUSR ) != 0 );
        if( bReq != bCur )
            break;

        bReq = (  ( flags & X_OK ) != 0 );
        bCur = ( ( dwMode & S_IXUSR ) != 0 );
        if( bReq != bCur )
            break;

    }while( 0 );

    return ret;
}

gint32 CFuseTextFile::fs_open(
    const char* path,
    fuse_file_info *fi )
{
    if( path == nullptr || fi == nullptr )
        return -EINVAL;

    CFuseMutex oLock( GetLock() );
    gint32 ret = oLock.GetStatus();
    if( ERROR( ret ) )
        return ret;

    IncOpCount();

    fi->direct_io = 1;
    fi->keep_cache = 0;
    fi->nonseekable = 1;
    fi->fh = ( guint64 )( CFuseObjBase* )this;

    return ret;
}

gint32 CFuseDirectory::fs_opendir(
    const char* path,
    fuse_file_info *fi )
{
    if( path == nullptr || fi == nullptr )
        return -EINVAL;

    CFuseMutex oLock( GetLock() );
    gint32 ret = oLock.GetStatus();
    if( ERROR( ret ) )
        return ret;

    IncOpCount();

    fi->direct_io = 1;
    fi->keep_cache = 0;
    fi->nonseekable = 1;
    fi->fh = ( guint64 )( CFuseObjBase* )this;

    return ret;
}

gint32 CFuseDirectory::fs_releasedir(
    const char* path,
    fuse_file_info *fi )
{
    if( path == nullptr || fi == nullptr )
        return -EINVAL;

    CFuseMutex oLock( GetLock() );
    gint32 ret = oLock.GetStatus();
    if( ERROR( ret ) )
        return ret;

    DecOpCount();

    return ret;
}

gint32 CFuseFileEntry::fs_open(
    const char* path,
    fuse_file_info *fi )
{
    if( path == nullptr || fi == nullptr )
        return -EINVAL;

    CFuseMutex oLock( GetLock() );
    gint32 ret = oLock.GetStatus();
    if( ERROR( ret ) )
        return ret;

    if( GetOpCount() > 0 )
        return -EUSERS;

    IncOpCount();

    if( fi->flags & O_NONBLOCK )
        SetNonBlock();

    fi->direct_io = 1;
    fi->keep_cache = 0;
    fi->nonseekable = 1;
    fi->fh = ( guint64 )( CFuseObjBase* )this;

    return ret;
}

#define INIT_STATBUF( _stbuf ) \
do{ \
    memset( _stbuf, 0, sizeof(struct stat ) );\
    ( _stbuf )->st_uid = getuid(); \
    ( _stbuf )->st_gid = getgid(); \
    ( _stbuf )->st_atime = GetAccTime();\
    ( _stbuf )->st_mtime = GetModifyTime(); \
}while( 0 )

gint32 CFuseRootDir::fs_getattr(
    const char *path,
    fuse_file_info* fi,
    struct stat *stbuf)
{
    INIT_STATBUF( stbuf );
    stbuf->st_ino = 1;
    stbuf->st_mode = S_IFDIR | GetMode();
    stbuf->st_nlink = 1;
    return 0;
}

gint32 CFuseDirectory::fs_getattr(
    const char *path,
    fuse_file_info* fi,
    struct stat *stbuf)
{
    INIT_STATBUF( stbuf );
    stbuf->st_ino = GetObjId();
    stbuf->st_mode = S_IFDIR | GetMode();
    stbuf->st_nlink = 1;
    return 0;
}

gint32 CFuseFileEntry::fs_getattr(
    const char *path,
    fuse_file_info* fi,
    struct stat *stbuf)
{
    INIT_STATBUF( stbuf );
    stbuf->st_ino = GetObjId();
    stbuf->st_mode = S_IFREG | GetMode();
    stbuf->st_nlink = 1;
    this->fs_ioctl( path, fi,
        FIONREAD, nullptr, 0,
        &stbuf->st_size );
    return 0;
}

gint32 CFuseTextFile::fs_getattr(
    const char *path,
    fuse_file_info* fi,
    struct stat *stbuf)
{
    INIT_STATBUF( stbuf );
    stbuf->st_ino = GetObjId();
    stbuf->st_mode = S_IFREG | GetMode();
    stbuf->st_nlink = 1;
    stbuf->st_size = m_strContent.size();
    return 0;
}

gint32 CFuseDirectory::fs_readdir(
    const char *path,
    fuse_file_info *fi,
    void *buf, fuse_fill_dir_t filler,
    off_t off, fuse_readdir_flags flags )
{
    gint32 ret = 0;
    do{
        CFuseMutex oLock( GetLock() );
        std::vector< CHILD_TYPE > vecChildren;
        GetChildren( vecChildren );
        for( auto& elem : vecChildren )
        {
            auto pObj = dynamic_cast
                < CFuseObjBase* >( elem.get() );
            if( pObj == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            struct stat file_stat;
            pObj->fs_getattr( "", nullptr, &file_stat );
            filler( buf, pObj->GetName().c_str(),
                &file_stat, 0,
                ( fuse_fill_dir_flags )0 );
        }

    }while( 0 );

    return ret;
}

gint32 CFuseTextFile::fs_read(
    const char* path,
    fuse_file_info *fi,
    fuse_req_t req,
    fuse_bufvec*& bufvec,
    off_t off, size_t size,
    std::vector< BufPtr >& vecBackup,
    fuseif_intr_data* d )
{
    gint32 ret = 0;
    do{
        if( size == 0 )        
            break;

        if( off >= m_strContent.size() )
        {
            ret = 0;
            break;
        }

        size_t dwBytes = std::min(
            size, m_strContent.size() - off );

        BufPtr pBuf = NewBufNoAlloc(
            m_strContent.c_str() + off,
            dwBytes, true );

        bufvec = new fuse_bufvec;
        *bufvec = FUSE_BUFVEC_INIT( dwBytes );
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
    const char *path, 
    fuse_file_info *fi,
    unsigned int cmd,
    void *arg, 
    unsigned int flags,
    void *data )
{
    gint32 ret = 0;
    do{
        switch( cmd )
        {
        case FIONREAD:
            {
                CFuseMutex oLock( GetLock() );
                ret = oLock.GetStatus();
                if( ERROR( ret ) )
                    break;

                size_t* pSize = ( size_t* )data;

                if( m_queReqs.size() )
                    break;

                *pSize = GetBytesAvail();
                break;
            }
        case FIOC_SETNONBLOCK:
            {
                CFuseMutex oLock( GetLock() );
                ret = oLock.GetStatus();
                if( ERROR( ret ) )
                    break;
                SetNonBlock( true );
                break;
            }
        case FIOC_SETBLOCK:
            {
                CFuseMutex oLock( GetLock() );
                ret = oLock.GetStatus();
                if( ERROR( ret ) )
                    break;
                SetNonBlock( false );
                break;
            }
        default:
            ret = super::fs_ioctl( path, fi,
                cmd, arg, flags, data );
            break;
        }

    }while( 0 );

    return ret;
}

gint32 CFuseReqFileProxy::fs_ioctl(
    const char *path, 
    fuse_file_info *fi,
    unsigned int cmd,
    void *arg,
    unsigned int flags,
    void *data )
{
    gint32 ret = 0;
    do{
        switch( cmd )
        {
        case FIONREAD:
            {
                CFuseMutex oLock( GetLock() );
                ret = oLock.GetStatus();
                if( ERROR( ret ) )
                    break;

                size_t* pSize = ( size_t* )data;

                if( m_queReqs.size() )
                    break;

                *pSize = GetBytesAvail();
                for( auto& elem : m_queTaskIds )
                {
                    *pSize += elem.strResp.size()
                        + sizeof( guint32 );
                }
                break;
            }
        default:
            ret = super::fs_ioctl( path, fi,
                cmd, arg, flags, data );
            break;
        }

    }while( 0 );

    return ret;
}

gint32 CFuseStmFile::fs_unlink(
    const char* path,
    fuse_file_info *fi )
{
    gint32 ret = 0;
    do{
        CRpcServices* pSvc = GetIf();

        CFuseMutex oFileLock(
                GetLock() );
        ret = oFileLock.GetStatus();
        if( ERROR( ret ) )
            break;
        if( GetOpCount() > 0 )
        {
            SetHidden();
            break;
        }

        CancelFsRequests();
        SetRemoved();

        oFileLock.Unlock();

        CFuseStmDir* pParent = dynamic_cast
            < CFuseStmDir* >( GetParent() );
        if( pParent == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        // keep a copy of pObj
        CHILD_TYPE pEnt;
        stdstr strName = GetName();
        ret = pParent->GetChild( strName, pEnt );
        if( ERROR( ret ) )
            break;

        pParent->RemoveChild( strName );

        ObjPtr pIf = GetIf();
        HANDLE hStream = GetStream();

        CIoManager* pMgr = pSvc->GetIoMgr();
        TaskletPtr pTask;
        ret = DEFER_CALL_NOSCHED( pTask,
            pSvc, &rpcf::IStream::OnClose,
            hStream, ( IEventSink* )nullptr );
        if( ERROR( ret ) )
            break;
        ret = pMgr->RescheduleTask( pTask );

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
                INBUF a={.pBuf=pBuf,.dwOff=0 };
                m_queIncoming.push_back( a );
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
                GetFuse(), req, d );
            if( dwReqSize > 0 )
                fuse_reply_data(
                    req, bufvec, FUSE_BUF_SPLICE_MOVE );
            else
                fuse_reply_err( req, -EAGAIN );
            fuseif_free_buf( bufvec );
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
                hStream, dwPReq );
        }
        else
        {
            ret = pStmProxy->GetPendingReqs(
                hStream, dwPReq );
        }
        if( ERROR( ret ) )
            break;

        if( dwPReq > 0 )
            break;
        
        if( pStmSvr != nullptr )
        {
            ret = pStmSvr->ReadStreamAsync(
                hStream, pBuf,
                ( IConfigDb* )nullptr );
        }
        else
        {
            ret = pStmProxy->ReadStreamAsync(
                hStream, pBuf,
                ( IConfigDb* )nullptr );
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
    const char* path,
    fuse_file_info *fi,
    fuse_req_t req, fuse_bufvec*& bufvec,
    off_t off, size_t size,
    std::vector< BufPtr >& vecBackup,
    fuseif_intr_data* d )
{
    gint32 ret = 0;

    if( size == 0 )
        return 0;

    do{
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

        CRpcServices* pSvc = GetIf();
        CStreamServerSync* pStmSvr = ObjPtr( pSvc );
        CStreamProxySync* pStmProxy = ObjPtr( pSvc );
        HANDLE hstm = GetStream();

        if( m_queReqs.size() &&
            m_queReqs.front().req != req )
        {
            if( IsNonBlock() )
            {
                ret = -EAGAIN;
                break;
            }
            // we need to complete the earilier
            // requests before this request
            ret = STATUS_PENDING;

            m_queReqs.push_back(
                { req, size, PINTR( d ) } );

            size_t dwReqSize =
                    m_queReqs.front().dwReqSize;
            if( dwReqSize <= dwAvail )
            {
                BufPtr pBuf = m_queIncoming.back().pBuf;
                m_queIncoming.pop_back();
                gint32 (*func)( CRpcServices*, CFuseStmFile*, BufPtr& )=
                    ([]( CRpcServices* pIf,
                        CFuseStmFile* pFile,
                        BufPtr& pBuf_ )
                {
                    gint32 ret = 0;
                    do{
                        HANDLE hstm = pFile->GetStream();
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
                    return ret;
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

        size_t dwBytesRead = 0;

        if( dwAvail >= size )
        {
            dwBytesRead = size;
        }
        else if( IsNonBlock() )
        {
            dwBytesRead = dwAvail;
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
                { req, size, PINTR( d ) } );
        }

        if( pStmSvr != nullptr )
        {
            pStmSvr->GetPendingReqs( hstm, dwPReqs );
        }
        else
        {
            pStmProxy->GetPendingReqs( hstm, dwPReqs );
        }

        bool bRead =
            ( dwPReqs == 0 && m_queReqs.size() );

        if( !bRead )
            break;

        BufPtr pBuf;
        gint32 iRet = 0;
        if( pStmSvr != nullptr )
        {
            iRet = pStmSvr->ReadStreamAsync( hstm,
                pBuf, ( IConfigDb* )nullptr );
        }
        else
        {
            iRet = pStmProxy->ReadStreamAsync( hstm,
                pBuf, ( IConfigDb* )nullptr );
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
    auto len = src->size;

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
        size_t& idx = oreq.idx;
        fuseif_intr_data* pintr = oreq.pintr.get();

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
            // copying is better than multiple round
            // trip in performance.
            if( pBuf->IsNoFree() )
                pBuf.NewObj();
            else
            {
                vecBufs.erase( vecBufs.begin() );
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
    const char* path,
    fuse_file_info *fi,
    fuse_req_t req,
    fuse_bufvec *bufvec,
    fuseif_intr_data* d )
{
    gint32 ret = 0;
    do{
        CRpcServices* pIf = GetIf();

        // this is a synchronous call since the bufvec
        // is controlled by the caller

        if( GetFlowCtrl() )
        {
            ret = -EAGAIN;
            break;
        }

        OUTREQ oreq = { req, bufvec,
                bufvec->idx, PINTR( d ) };
        ret = SendBufVec( oreq );
        oreq.pintr.release();

    }while( 0 );
    return ret;
}

gint32 CFuseStmFile::OnWriteResumed()
{
    gint32 ret = 0;
    do{
        CFuseMutex oFileLock( GetLock() );
        ret = oFileLock.GetStatus();
        if( ERROR( ret ) )
            break;

        SetFlowCtrl( false );
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
        if( req == itr->req )
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
            elem.req, ECANCELED );
    }
    m_queReqs.clear();

    return ret;
}

gint32 CFuseEvtFile::fs_read(
    const char* path,
    fuse_file_info *fi,
    fuse_req_t req, fuse_bufvec*& bufvec,
    off_t off, size_t size,
    std::vector< BufPtr >& vecBackup,
    fuseif_intr_data* d )
{
    gint32 ret = 0;
    do{
        if( size == 0 )
            break;

        CFuseMutex oFileLock( GetLock() );
        ret = oFileLock.GetStatus();
        if( ERROR( ret ) )
            break;

        if( m_queReqs.size() )
        {
            if( IsNonBlock() )
            {
                ret = -EAGAIN;
                break;
            }
            m_queReqs.push_back(
                { req, size, PINTR( d ) } );
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
            size = dwAvail;
            FillBufVec( size,
                m_queIncoming,
                vecBackup, bufvec );
        }
        else
        {
            ret = STATUS_PENDING;
            m_queReqs.push_back(
                { req, size, PINTR( d ) } );
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
            *pBuf = strMsg.size();
            pBuf->Append(
                strMsg.c_str(), strMsg.size());
            m_queIncoming.push_back( { pBuf, 0 } );
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
                    GetFuse(), req, d );
            if( dwReqSize > 0 )
                fuse_reply_data(
                    req, bufvec, FUSE_BUF_SPLICE_MOVE );
            else
                fuse_reply_err( req, -EAGAIN );
            fuseif_free_buf( bufvec );
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
            BufPtr pBuf;
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
                char* pData = ( char* )fb.mem;
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
                vecBufs.erase( vecBufs.begin() );
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

    }while( 0 );

    return ret;
}

gint32 CFuseRespFileSvr::fs_write_buf(
    const char* path,
    fuse_file_info *fi,
    fuse_req_t req,
    fuse_bufvec *bufvec,
    fuseif_intr_data* d )
{
    gint32 ret = 0;

    CFuseSvcServer* pSvr = ObjPtr( GetIf() );
    if( pSvr == nullptr )
        return -EFAULT;

    ret = CopyFromBufVec(
            m_vecOutBufs, bufvec );
    if( ERROR( ret ) )
        return ret;

    do{
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
        size_t dwReqSize =
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
                m_vecOutBufs.erase(
                    m_vecOutBufs.begin() );
                if( dwToAdd > 0 )
                    continue;
                break;
            }

            BufPtr pBuf = NewBufNoAlloc(
                elem->ptr(), dwToAdd, true );
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
                vecBufs.erase( vecBufs.begin() );
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
        CFuseSvcServer* pSvr = ObjPtr( GetIf() );
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
    if( m_queTaskIds.empty() )
        return -ENOENT;
    auto itr = m_queTaskIds.begin();
    while( itr != m_queTaskIds.end() )
    {
        if( itr->qwReqId == qwReqId )
        {
            bFound = true;
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
    size_t dwReqSize, 
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
    const char* path,
    fuse_file_info *fi,
    fuse_req_t req,
    fuse_bufvec*& bufvec,
    off_t off, size_t size,
    std::vector< BufPtr >& vecBackup,
    fuseif_intr_data* d )
{
    gint32 ret = 0;
    do{
        if( size == 0 )
            break;

        CFuseMutex oFileLock( GetLock() );
        ret = oFileLock.GetStatus();
        if( ERROR( ret ) )
            break;

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
            ret = FillBufVec( size, m_queIncoming,
                vecBackup, bufvec );
        }
        else if( dwAvail + dwNewBytes >= size )
        {
            ret = ConvertAndFillBufVec( dwAvail,
                dwNewBytes, size, 
                vecBackup, bufvec );
        }
        else if( IsNonBlock() )
        {
            size = dwAvail + dwNewBytes;
            if( size == 0 )
                break;
            ret = ConvertAndFillBufVec( dwAvail,
                dwNewBytes, size, 
                vecBackup, bufvec );
        }
        else
        {
            ret = STATUS_PENDING;
            m_queReqs.push_back(
                { req, size, PINTR( d ) } );
        }

    }while( 0 );

    return ret;
}

gint32 CFuseReqFileProxy::fs_write_buf(
    const char* path,
    fuse_file_info *fi,
    fuse_req_t req,
    fuse_bufvec *bufvec,
    fuseif_intr_data* d )
{
    gint32 ret = 0;

    CFuseSvcProxy* pProxy = ObjPtr( GetIf() );
    if( pProxy == nullptr )
        return -EFAULT;

    ret = CopyFromBufVec(
            m_vecOutBufs, bufvec );
    if( ERROR( ret ) )
        return ret;

    do{
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

        size_t dwReqSize = ( guint32& )*m_pReqSize;
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
                m_vecOutBufs.erase(
                    m_vecOutBufs.begin() );
                if( dwToAdd > 0 )
                    continue;
                break;
            }

            BufPtr pBuf = NewBufNoAlloc(
                elem->ptr(), dwToAdd, true );
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
                vecBufs.erase( vecBufs.begin() );
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
            Json::writeString( oBuilder, valResp );

        if( ret == STATUS_PENDING )
        {
            CFuseMutex oLock( GetLock() );
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
            gint32 (*func)( CFuseSvcProxy*,
                const stdstr& )=([](
                    CFuseSvcProxy* pIf,
                    const stdstr& strResp )
            {
                gint32 ret = 0;
                do{
                    RLOCK_TESTMNT2( pIf );
                    auto pResp = _pSvcDir->GetChild(
                        JSON_RESP_FILE );
                    CFuseEvtFile* p = dynamic_cast
                        < CFuseEvtFile* >( pResp );
                    p->ReceiveEvtJson( strResp );

                }while( 0 );
                return ret;
            });

            CIoManager* pMgr = pProxy->GetIoMgr();
            TaskletPtr pTask;
            ret = NEW_FUNCCALL_TASK( pTask,
                pMgr, func, pProxy, strResp );
            if( ERROR( ret ) )
                break;

            pMgr->RescheduleTask( pTask );
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

gint32 InitRootIf(
    CIoManager* pMgr, bool bProxy )
{
    gint32 ret = 0;
    do{
        CParamList oParams;
        oParams.SetPointer( propIoMgr, pMgr );
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

void ReleaseRootIf()
{ g_pRootIf.Clear(); }

static FactoryPtr InitClassFactory()
{
    BEGIN_FACTORY_MAPS;

    INIT_MAP_ENTRYCFG( CFuseRootServer );
    INIT_MAP_ENTRYCFG( CFuseRootProxy );

    END_FACTORY_MAPS;
};


} // namespace

gint32 fuseop_access(
    const char* path, int flags )
{
    fuse_file_info* fi = nullptr;
    return SafeCall( 
        false, &CFuseObjBase::fs_access,
        path, fi, flags );
}

gint32 fuseop_open(
    const char* path, fuse_file_info * fi)
{
    return SafeCall( 
        false, &CFuseObjBase::fs_open,
        path, fi );
}

gint32 fuseop_release(
    const char* path, fuse_file_info * fi )
{
    CFuseObjBase* pObj = reinterpret_cast
        < CFuseObjBase* >( fi->fh );
    if( pObj == nullptr )
        return -EFAULT;

    gint32 ret = 0;
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


        ObjPtr pIf = pStm->GetIf();
        HANDLE hStream = pStm->GetStream();

        CIoManager* pMgr = pSvc->GetIoMgr();
        TaskletPtr pTask;
        ret = DEFER_CALL_NOSCHED( pTask,
            pSvc, &rpcf::IStream::OnClose,
            hStream, ( IEventSink* )nullptr );
        if( ERROR( ret ) )
            break;
        ret = pMgr->RescheduleTask( pTask );

    }while( 0 );

    return ret;
}

gint32 fuseop_opendir(
    const char* path, fuse_file_info * fi )
{
    return SafeCall( false,
        &CFuseObjBase::fs_opendir,
        path, fi );
}

gint32 fuseop_releasedir(
    const char* path,
    fuse_file_info * fi )
{
    return SafeCall( false,
        &CFuseObjBase::fs_releasedir,
        path, fi );
}

gint32 fuseop_unlink( const char* path )
{
    if( path == nullptr )
        return -EINVAL;

    fuse_file_info* fi = nullptr;
    return SafeCall( true,
        &CFuseObjBase::fs_unlink,
        path, fi ) ;
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

        std::vector< stdstr > vecSubdirs;
        CFuseObjBase* pObj = nullptr;
        gint32 ret = GetSvcDir(
            strPath.c_str(), pObj, vecSubdirs );
        if( ERROR( ret ) )
            break;

        if( ret == ENOENT )
        {
            ret = -ENOENT;
            break;
        }

        if( vecSubdirs.size() != 1 &&
            vecSubdirs[ 0 ] != STREAM_DIR )
        {
            ret = -EACCES;
            break;
        }

        CFuseStmDir* pDir = dynamic_cast
            < CFuseStmDir* >( pObj );
        if( unlikely( pDir == nullptr ) )
        {
            // create allowed only under STREAM_DIR
            ret = -ENOSYS;
            break;
        }

        CRpcServices* pSvc = pDir->GetIf();
        if( pSvc->IsServer() )
        {
            ret = -EACCES;
            break;
        }
        else
        {
            WLOCK_TESTMNT2( pSvc );
            if( pDir->GetChild( strName ) != nullptr )
            {
                ret = -EEXIST;
                break;
            }
            if( pDir->GetCount() > MAX_STREAMS_PER_SVC )
            {
                ret = -EMFILE;
                break;
            }
        }

        CStreamProxySync* pStm = ObjPtr( pSvc );
        if( pStm == nullptr ) 
        {
            ret = -EFAULT;
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
        ret = pStm->StartStream(
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

        WLOCK_TESTMNT2( pStm );

        CFuseStmFile* pStmFile = new CFuseStmFile(
            strName, hStream, pStm );

        auto pEnt = std::shared_ptr
                < CDirEntry >( pStmFile ); 
        fi->fh = ( guint64 )
            ( CFuseObjBase* )pStmFile;
        fi->direct_io = 1;
        fi->keep_cache = 0;
        pStmFile->IncOpCount();
        pDir->AddChild( pEnt );

    }while( 0 );

    return ret;
}

int fuseop_poll(const char *path,
    fuse_file_info *fi,
    fuse_pollhandle *ph,
    unsigned *reventsp )
{
    return SafeCall( false, 
        &CFuseObjBase::fs_poll,
        path, fi, ph, reventsp );
}

#if FUSE_USE_VERSION < 35
int fuseop_ioctl(
    const char *path, int cmd,
    void *arg, fuse_file_info *fi,
    unsigned int flags, void *data )
#else
int fuseop_ioctl(
    const char *path, unsigned int cmd,
    void *arg, fuse_file_info *fi,
    unsigned int flags, void *data )
#endif
{
    return SafeCall( false,
        &CFuseObjBase::fs_ioctl,
        path, fi, cmd, arg, flags, data );
}

int fuseop_getattr(
    const char *path,
    struct stat *stbuf,
    fuse_file_info* fi )
{
    return SafeCall(
        false,
        &CFuseObjBase::fs_getattr,
        path, fi, stbuf );
}

int fuseop_readdir(
    const char *path,
    void *buf, fuse_fill_dir_t filler,
    off_t off, struct fuse_file_info *fi,
    enum fuse_readdir_flags flags)
{
    return SafeCall( false,
        &CFuseObjBase::fs_readdir,
        path, fi, buf, filler, off, flags );
}

void* fuseop_init(
   fuse_conn_info *conn,
   fuse_config *cfg )
{
    UNREFERENCED( conn );

    cfg->entry_timeout = -1;
    cfg->attr_timeout = -1;
    cfg->negative_timeout = 0;
    cfg->direct_io = 1;
    cfg->kernel_cache = 0;

    // init some other stuffs
    Json::CharReaderBuilder oBuilder;
    g_pReader.reset( oBuilder.newCharReader() );
    return GetFuse();
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
    int argc, char **argv )
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

static fuse_operations fuseif_ops =
{
    .getattr = fuseop_getattr,
    .unlink = fuseop_unlink,
    .open = fuseop_open,
    .release = fuseop_release,
    .opendir = fuseop_opendir,
    .readdir = fuseop_readdir,
    .releasedir = fuseop_releasedir,
    .init = fuseop_init,
    .create = fuseop_create,
    .ioctl = fuseop_ioctl,
    .poll = fuseop_poll,
};

/* Command line parsing */
struct options {
    int reconnect;
    // int update_interval;
};
static struct options options = {
        .reconnect = 0,
    //     .update_interval = 1,
};

#define OPTION(t, p) { t, offsetof(struct options, p), 1 }
static const struct fuse_opt option_spec[] = {
        OPTION("--reconnect", reconnect),
   //      OPTION("--update-interval=%d", update_interval),
        FUSE_OPT_END
};

extern ObjPtr g_pIoMgr;
gint32 fuseif_main( fuse_args& args,
    fuse_cmdline_opts& opts )
{
    struct fuse *fuse;
    struct fuse_loop_config config;
    struct fuse_session *se = nullptr; 
    int res;

    if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1)
        return 1;

    if( options.reconnect )
    {
        CIoManager* pMgr = g_pIoMgr;
        pMgr->SetCmdLineOpt( propConnRecover, true );
    }

    if (fuse_parse_cmdline(&args, &opts) != 0)
        return 1;

    fuse = fuse_new(&args, &fuseif_ops, sizeof(fuseif_ops), NULL);
    if (fuse == NULL) {
        res = 1;
        goto out1;
    }
    SetFuse( fuse );

    if (fuse_mount(fuse,opts.mountpoint) != 0) {
        res = 1;
        goto out2;
    }

    se = fuse_get_session(fuse);
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

// common method for a class factory library
extern "C"
gint32 DllLoadFactory( FactoryPtr& pFactory )
{
    pFactory = InitClassFactory();
    if( pFactory.IsEmpty() )
        return -EFAULT;

    return 0;
}
