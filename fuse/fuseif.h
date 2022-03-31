/*
 * =====================================================================================
 *
 *       Filename:  fuseif.h
 *
 *    Description:  Declaration of rpc interface for fuse integration and
 *                  related classes
 *
 *        Version:  1.0
 *        Created:  03/01/2022 05:33:58 PM
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
#pragma once
#define FUSE_USE_VERSION 35
#include <fuse.h>
#include <fuse_lowlevel.h>  /* for fuse_cmdline_opts */

#include "configdb.h"
#include "defines.h"
#include "ifhelper.h"
#include <exception>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "dbusport.h"
#include "streamex.h"

#define CONN_PARAM_FILE "conn_params"
#define HOP_DIR  "_nexthop"
#define STREAM_DIR "_streams"
#define JSON_REQ_FILE "json_request"
#define JSON_RESP_FILE "json_response"
#define JSON_EVT_FILE "json_event"
#define JSON_STMEVT_FILE "json_stmevt"
#define CONN_DIR_PREFIX "connection_"

#define MAX_STREAMS_PER_SVC 1024
#define MAX_STM_QUE_SIZE   STM_MAX_PACKETS_REPORT
#define MAX_REQ_QUE_SIZE   32
#define MAX_EVT_QUE_SIZE   32

#define MAX_FUSE_MSG_SIZE    ( MAX_BYTES_PER_TRANSFER * 2 )


struct fuseif_intr_data;

namespace rpcf
{

InterfPtr& GetRootIf();

fuse* GetFuse();

class CFuseObjBase;
class CFuseDirectory;

gint32 GetFuseObj( const stdstr& strPath,
    CFuseObjBase*& pDir );

CFuseDirectory* GetRootDir();

gint32 InitRootIf(
    CIoManager* pMgr, bool bProxy );

gint32 CloseChannel(
    InterfPtr& pIf, HANDLE hStream );

gint32 AddSvcPoint(
    const stdstr& strObj,
    const stdstr& strDesc,
    EnumClsid iClsid,
    bool bProxy );

class CSharedLock
{
    stdmutex m_oLock;
    bool m_bWrite = false;
    gint32 m_iReadCount = 0;
    sem_t m_semReader;
    sem_t m_semWriter;
    // true : reader, false: writer
    std::deque< bool > m_queue;

    public:
    CSharedLock()
    {
        Sem_Init( &m_semReader, 0, 0 );
        Sem_Init( &m_semWriter, 0, 0 );
    }

    ~CSharedLock()
    {
        sem_destroy( &m_semReader );
        sem_destroy( &m_semWriter );
    }

    inline gint32 LockRead()
    {
        CStdMutex oLock( m_oLock );
        if( m_bWrite )
        {
            m_queue.push_back( true );
            oLock.Unlock();
            return Sem_Wait_Wakable( &m_semReader );
        }
        else
        {
            if( m_queue.empty() )
            {
                m_iReadCount++;
                oLock.Unlock();
                return STATUS_SUCCESS;
            }
            m_queue.push_back( true );
            oLock.Unlock();
            return Sem_Wait_Wakable( &m_semReader );
        }
    }

    inline gint32 TryLockRead()
    {
        CStdMutex oLock( m_oLock );
        if( !m_bWrite && m_queue.empty() )
        {
            m_iReadCount++;
            oLock.Unlock();
            return STATUS_SUCCESS;
        }
        return -EAGAIN;
    }

    inline gint32 LockWrite()
    {
        CStdMutex oLock( m_oLock );
        if( m_bWrite )
        {
            m_queue.push_back( false );
            oLock.Unlock();
            return Sem_Wait_Wakable( &m_semWriter );
        }
        else if( m_iReadCount > 0 )
        {
            m_queue.push_back( false );
            oLock.Unlock();
            return Sem_Wait_Wakable( &m_semWriter );
        }
        else if( m_iReadCount == 0 )
        {
            m_bWrite = true;
            return 0;
        }
        return -EFAULT;
    }

    inline gint32 TryLockWrite()
    {
        CStdMutex oLock( m_oLock );
        if( m_bWrite || m_iReadCount > 0 ) 
            return -EAGAIN;

        if( m_iReadCount == 0 )
        {
            m_bWrite = true;
            oLock.Unlock();
            return 0;
        }
        return -EFAULT;
    }

    inline gint32 ReleaseRead()
    {
        CStdMutex oLock( m_oLock );
        m_iReadCount--;
        if( m_iReadCount > 0 )
            return STATUS_SUCCESS;

        if( m_iReadCount == 0 )
        {
            if( m_queue.empty() )
                return STATUS_SUCCESS;

            if( m_queue.front() == true )
                return -EFAULT;

            m_bWrite = true;
            m_queue.pop_front();
            return Sem_Post( &m_semWriter );
        }
        return -EFAULT;
    }

    inline gint32 ReleaseWrite()
    {
        CStdMutex oLock( m_oLock );
        if( m_queue.empty() )
        {
            m_bWrite = false;
        }
        else if( m_queue.front() == false )
        {
            m_bWrite = true;
            m_queue.pop_front();
            Sem_Post( &m_semWriter );
        }
        else while( m_queue.front() == true )
        {
            m_iReadCount++;
            Sem_Post( &m_semReader );
            m_queue.pop_front();
        }
        return STATUS_SUCCESS;
    }
};

#define CFuseMutex  CWriteLock

template < bool bRead >
struct CLocalLock
{
    // only works on the same thread
    gint32 m_iStatus = STATUS_SUCCESS;
    CSharedLock* m_pLock = nullptr;
    bool m_bLocked = false;
    CLocalLock( CSharedLock& oLock )
    {
        m_pLock = &oLock;
        m_iStatus = ( bRead ?
            oLock.LockRead():
            oLock.LockWrite() );
        m_bLocked = true;
    }

    ~CLocalLock()
    { 
        if( m_bLocked && m_pLock != nullptr )
        {
            m_bLocked = false;
            bRead ? m_pLock->ReleaseRead() : 
                m_pLock->ReleaseWrite();
            m_pLock = nullptr;
        }
    }

    void Unlock()
    {
        if( !m_bLocked )
            return;
        m_bLocked = false;
        bRead ? m_pLock->ReleaseRead() :
            m_pLock->ReleaseWrite();
    }

    void Lock()
    {
        if( !m_pLock )
            return;
        m_iStatus = ( bRead ?
            m_pLock->LockRead() :
            m_pLock->LockWrite() );
        m_bLocked = true;
    }

    inline gint32 GetStatus() const
    { return m_iStatus; }
};

typedef CLocalLock< true > CReadLock;
typedef CLocalLock< false > CWriteLock;

class CFuseObjBase : public CDirEntry
{
    timespec m_tsUpdTime = { 0 };
    timespec m_tsModtime = { 0 };
    guint32 m_dwStat;
    guint32 m_dwMode = S_IRUSR;
    guint32 m_dwOpenCount = 0;
    bool m_bHidden = false;
    std::atomic< bool > m_bNonBlock;

    // poll related members
    fuse_pollhandle* m_pollHandle = nullptr;

    // events to return
    guint32 m_dwRevents = 0;
    mutable CSharedLock m_oLock;

    public:
    typedef CDirEntry super;
    CFuseObjBase() :
        super(),
        m_bNonBlock( false )
    {}

    ~CFuseObjBase()
    {
        if( m_pollHandle != nullptr )
        {
            fuse_pollhandle_destroy(
                m_pollHandle );
            m_pollHandle = nullptr;
        }
    }

    CSharedLock& GetLock() const
    { return m_oLock; }

    inline guint32 GetRevents() const
    { return m_dwRevents; }

    inline void SetRevents( guint32 revent )
    { m_dwRevents = revent; }

    inline void SetHidden( bool bHiddden = true )
    { m_bHidden = bHiddden; }

    inline bool IsHidden()
    { return m_bHidden; }

    inline gint32 IncOpCount()
    { return ++m_dwOpenCount; }

    inline gint32 DecOpCount()
    { return --m_dwOpenCount; }

    inline gint32 GetOpCount() const
    { return m_dwOpenCount; }

    inline void SetMode( guint32 dwMode )
    { m_dwMode = dwMode; }

    inline guint32 GetMode() const
    { return m_dwMode; }

    inline void SetNonBlock( bool bNonBlock )
    { m_bNonBlock = bNonBlock; }

    inline bool IsNonBlock() const
    { return m_bNonBlock; }

    inline fuse_pollhandle* GetPollHandle() const
    { return m_pollHandle; }

    void SetPollHandle( fuse_pollhandle* pollHandle )
    {
        if( m_pollHandle != nullptr )
            fuse_pollhandle_destroy( m_pollHandle );
        m_pollHandle = pollHandle;
    }

    /*
    gint32 create();
    gint32 unlink();
    gint32 open();
    gint32 write();
    gint32 readdir();
    gint32 opendir();
    gint32 releasedir();
    gint32 getattr();
    gint32 setattr();
    gint32 flock();
    gint32 rename();
    void* init();
    void destroy();
    */
    virtual gint32 fs_read(
        fuse_req_t req,
        fuse_bufvec*& buf,
        off_t off,
        size_t size,
        fuse_file_info *fi,
        std::vector< BufPtr >& vecBackup,
        fuseif_intr_data* d )
    { return -ENOSYS; }

    virtual gint32 fs_write_buf(
        fuse_req_t req,
        fuse_bufvec *buf,
        fuse_file_info *fi,
        fuseif_intr_data* d )
    { return -ENOSYS; }

    virtual gint32 fs_poll(const char *path,
        fuse_file_info *fi,
        fuse_pollhandle *ph,
        unsigned *reventsp )
    { return -ENOSYS; }

    virtual gint32 fs_open(
        const char *path,
        struct fuse_file_info *fi )
    { return -ENOSYS; }

    virtual gint32 fs_readdir(const char *path,
        void *buf, fuse_fill_dir_t filler,
        off_t off, fuse_file_info *fi,
        fuse_readdir_flags flags)
    { return -ENOSYS; }

    virtual gint32 fs_getattr(
        const char *path,
        struct stat *stbuf,
        fuse_file_info* fi)
    { return -ENOSYS; }

    virtual gint32 fs_ioctl(
        const char *path, unsigned int cmd,
        void *arg, fuse_file_info *fi,
        unsigned int flags, void *data )
    { return -ENOSYS; }

    virtual gint32 fs_unlink(
        const char* path )
    { return -ENOSYS; }

    virtual gint32 fs_opendir(
        const char* path,
        fuse_file_info* fi )
    { return -ENOSYS; }

    virtual gint32 fs_releasedir(
        const char* path,
        fuse_file_info* fi )
    { return -ENOSYS; }
};

class CFuseDirectory : public CFuseObjBase
{
    protected:
    CRpcServices* m_pIf = nullptr;

    public:
    typedef CFuseObjBase super;
    CFuseDirectory(
        const stdstr& strName,
        CRpcServices* pIf ) :
        super(), m_pIf( pIf )
    {
        SetName( strName );
        SetClassId( clsid( CFuseDirectory ) );
    }

    inline CRpcServices* GetIf() const
    { return m_pIf; }

    inline void SetIf( CRpcServices* pIf )
    { m_pIf = pIf; }

    gint32 fs_getattr(
        const char *path, 
        struct stat *stbuf,
        fuse_file_info* fi) override;

    gint32 fs_opendir(
        const char* path,
        fuse_file_info * fi ) override;

    gint32 fs_releasedir(
        const char* path,
        fuse_file_info * fi ) override;

    gint32 fs_readdir(
        const char *path,
        void *buf, fuse_fill_dir_t filler,
        off_t off, fuse_file_info *fi,
        fuse_readdir_flags flags) override;
};

class CFuseRootDir : public CFuseDirectory
{
    public:
    typedef CFuseDirectory super;
    CFuseRootDir() : super( "/", nullptr )
    {
        SetClassId( clsid( CFuseRootDir ) );
        SetMode( S_IRUSR | S_IXUSR );
    }
    gint32 fs_getattr(
        const char *path,
        struct stat *stbuf,
        fuse_file_info* fi) override;
};

class CFuseSvcDir : public CFuseDirectory
{
    protected:
    mutable CSharedLock m_oSvcLock;

    public:
    typedef CFuseDirectory super;
    CFuseSvcDir(
        const stdstr& strSvcName,
        CRpcServices* pIf ) :
        super( strSvcName, pIf )
    { SetClassId( clsid( CFuseSvcDir ) ); }

    CSharedLock& GetSvcLock() const
    { return m_oSvcLock; }

    inline CIoManager* GetIoMgr()
    {
        if( m_pIf == nullptr )
            return nullptr;
        return m_pIf->GetIoMgr();
    }
};

class CFuseFileEntry : public CFuseObjBase
{
    protected:

    struct INBUF
    {
        BufPtr pBuf;
        guint32 dwOff;
    };

    std::deque< INBUF > m_queIncoming;

    using PINTR = 
        std::unique_ptr< fuseif_intr_data >;
    struct REQCTX
    {
        fuse_req_t req;
        size_t dwReqSize;
        PINTR pintr;
    };

    std::deque< REQCTX > m_queReqs;
    CRpcServices* m_pIf;

    gint32 FillBufVec(
        size_t& dwReqSize,
        std::deque< INBUF >& queBufs,
        std::vector< BufPtr >& vecBufs,
        fuse_bufvec*& bufvec )
    {
        gint32 ret = 0;
        do{
            guint32 dwAvail = 0;
            auto itr = queBufs.begin();
            while( itr != queBufs.end() )
            {
              
                guint32& dwOff = itr->dwOff;
                BufPtr& pBuf = itr->pBuf;
                dwAvail += pBuf->size() - dwOff;

                if( dwReqSize == 0 )
                    dwReqSize = dwAvail;

                if( dwAvail >= dwReqSize )
                    break;

                itr++;
            }
            if( dwAvail == 0 )
            {
                dwReqSize = 0;
                queBufs.clear();
                break;
            }

            if( dwReqSize == 0 )
                dwReqSize = dwAvail;

            if( dwAvail < dwReqSize )
            {
                ret = STATUS_PENDING;
                break;
            }

            gint32 iCount = itr - queBufs.begin();
            bufvec = ( fuse_bufvec* )malloc( 
                sizeof( fuse_bufvec ) +
                sizeof( fuse_buf ) * iCount );
            iCount += 1;
            bufvec->count = iCount;
            bufvec->idx = 0;
            bufvec->off = 0;
            guint32 dwAlloced = 0;
            for( int i = 0; i < iCount; i++ )
            {
                BufPtr& pBuf =
                    queBufs.front().pBuf;
                guint32& dwOff =
                    queBufs.front().dwOff;

                fuse_buf& slot = bufvec->buf[ i ];
                slot.flags = (enum fuse_buf_flags) 0;
                slot.fd = -1;
                slot.pos = 0;
                slot.mem = pBuf->ptr() + dwOff;

                if( i < iCount - 1 )
                {
                    slot.size = pBuf->size() - dwOff;
                    dwAlloced += slot.size;
                    vecBufs.push_back( pBuf );
                    queBufs.pop_front();
                    continue;
                }
                
                slot.size = dwReqSize - dwAlloced;
                dwOff += slot.size;
                if( dwOff == pBuf->size() )
                {
                    vecBufs.push_back( pBuf );
                    queBufs.pop_front();
                }
            }

        }while( 0 );

        return ret; 
    }

    gint32 CopyFromPipe(
        BufPtr& pBuf, fuse_buf* src );

    // don't clear vecBuf, which may have contents in
    // advance
    gint32 CopyFromBufVec(
         std::vector< BufPtr >& vecBuf,
         fuse_bufvec* bufvec );

    public:
    typedef CFuseObjBase super;
    CFuseFileEntry(
        const stdstr& strName,
        CRpcServices* pIf ) :
        super(),
        m_pIf( pIf )
    {
        SetName( strName );
    }

    inline CRpcServices* GetIf() const
    { return m_pIf; }

    inline void SetIf( CRpcServices* pIf )
    { m_pIf = pIf; }

    virtual gint32 CancelFsRequest(
        fuse_req_t req );

    virtual gint32 CancelFsRequests();

    guint32 GetBytesAvail() const
    { 
        guint32 ret = 0;
        for( auto& elem : m_queIncoming )
            ret += elem.pBuf->size() -
                elem.dwOff;
        return ret;
    }

    guint32 GetBytesRequired() const
    { 
        guint32 ret = 0;
        for( auto& elem : m_queReqs )
            ret += elem.dwReqSize;
        return ret;
    }

    gint32 fs_getattr(
        const char *path,
        struct stat *stbuf,
        fuse_file_info* fi) override;

    gint32 fs_ioctl(
        const char *path, unsigned int cmd,
        void *arg, fuse_file_info *fi,
        unsigned int flags,
        void *data ) override;

    gint32 fs_open(
        const char *path,
        fuse_file_info *fi ) override;
};

class CFuseTextFile : public CFuseObjBase
{
    stdstr m_strContent;

    public:
    typedef CFuseObjBase super;
    CFuseTextFile( const stdstr& strName )
        : super()
    {
        SetName( strName );
        SetClassId( clsid( CFuseTextFile ) );
    }

    void SetContent( const stdstr& strConent )
    { m_strContent = strConent; }

    const stdstr& GetContent() const
    { return m_strContent; }

    gint32 fs_write_buf(
        fuse_req_t req,
        fuse_bufvec *buf,
        fuse_file_info *fi,
        fuseif_intr_data* d ) override
    { return -ENOSYS; }

    gint32 fs_read( fuse_req_t req,
        fuse_bufvec*& buf,
        off_t off,
        size_t size,
        fuse_file_info *fi,
        std::vector< BufPtr >& vecBackup,
        fuseif_intr_data* d ) override;

    gint32 fs_getattr(
        const char *path,
        struct stat *stbuf,
        fuse_file_info* fi) override;

    gint32 fs_open(
        const char *path,
        fuse_file_info *fi ) override;
};

class CFuseEvtFile : public CFuseFileEntry
{
    public:
    typedef CFuseFileEntry super;
    CFuseEvtFile(
        const stdstr& strName,
        CRpcServices* pIf ) :
            super( strName, pIf )
    { SetClassId( clsid( CFuseEvtFile ) ); }

    gint32 fs_read( fuse_req_t req,
        fuse_bufvec*& buf,
        off_t off,
        size_t size,
        fuse_file_info *fi,
        std::vector< BufPtr >& vecBackup,
        fuseif_intr_data* d ) override;

    gint32 ReceiveEvtJson(
        const stdstr& strMsg );

    virtual gint32 ReceiveMsgJson(
        const stdstr& strMsg,
        guint64 qwReqId )
    { return -EACCES; };

    gint32 fs_write_buf(
        fuse_req_t req,
        fuse_bufvec *buf,
        fuse_file_info *fi,
        fuseif_intr_data* d ) override
    { return -ENOSYS; }

    using BUFARR=std::vector< BufPtr >;
    gint32 GetReqSize(
        BufPtr& pReqSize,
        BUFARR& vecBufs );

    gint32 fs_poll(const char *path,
        fuse_file_info *fi,
        fuse_pollhandle *ph,
        unsigned *reventsp ) override;

};

class CFuseRespFileSvr : public CFuseEvtFile
{
    protected:

    std::vector< BufPtr > m_vecOutBufs;
    BufPtr  m_pReqSize;

    public:

    typedef CFuseEvtFile super;
    CFuseRespFileSvr(
        const stdstr& strName,
        CRpcServices* pIf ) :
            super( strName, pIf ),
            m_pReqSize( true )
    { SetClassId( clsid( CFuseRespFileSvr ) ); }

    gint32 fs_read( fuse_req_t req,
        fuse_bufvec*& buf,
        off_t off,
        size_t size,
        fuse_file_info *fi,
        std::vector< BufPtr >& vecBackup,
        fuseif_intr_data* d ) override
    { return -ENOSYS; }

    gint32 fs_write_buf(
        fuse_req_t req,
        fuse_bufvec *buf,
        fuse_file_info *fi,
        fuseif_intr_data* d ) override;
};

class CFuseReqFileSvr : public CFuseRespFileSvr
{
    public:
    typedef CFuseRespFileSvr super;

    CFuseReqFileSvr(
        const stdstr& strName,
        CRpcServices* pIf ) :
        super( strName, pIf )
    { SetClassId( clsid( CFuseReqFileSvr ) ); }

    gint32 fs_write_buf(
        fuse_req_t req,
        fuse_bufvec *buf,
        fuse_file_info *fi,
        fuseif_intr_data* d ) override
    { return -ENOSYS;}

    gint32 fs_read( fuse_req_t req,
        fuse_bufvec*& buf,
        off_t off,
        size_t size,
        fuse_file_info *fi,
        std::vector< BufPtr >& vecBackup,
        fuseif_intr_data* d ) override
    {
        return CFuseEvtFile::fs_read(
            req, buf, off, size, fi, vecBackup, d ); 
    }

    gint32 ReceiveMsgJson(
        const stdstr& strMsg,
        guint64 qwReqId )
    { return ReceiveEvtJson( strMsg ); }
};

class CFuseRespFileProxy : public CFuseEvtFile
{
    protected:

    std::vector< BufPtr > m_vecOutBufs;
    BufPtr  m_pReqSize;

    public:
    typedef CFuseEvtFile super;

    CFuseRespFileProxy(
        const stdstr& strName,
        CRpcServices* pIf ) :
        super( strName, pIf ),
        m_pReqSize( true )
    { SetClassId( clsid( CFuseRespFileProxy ) ); }

    gint32 fs_write_buf(
        fuse_req_t req,
        fuse_bufvec *buf,
        fuse_file_info *fi,
        fuseif_intr_data* d ) override
    { return -ENOSYS;}

    // receiving the request's response
    gint32 ReceiveMsgJson(
        const stdstr& strMsg,
        guint64 qwReqId );
};

class CFuseReqFileProxy : public CFuseRespFileProxy
{
    protected:

    struct REQID_RESP
    {
        stdstr strResp;
        guint64 qwReqId;
    };
    std::deque< REQID_RESP > m_queTaskIds;

    gint32 ConvertAndFillBufVec(
        guint32 dwAvail,
        guint32 dwNewBytes,
        size_t dwReqSize, 
        std::vector< BufPtr >& vecBackup,
        fuse_bufvec*& bufvec );

    public:

    typedef CFuseRespFileProxy super;

    CFuseReqFileProxy(
        const stdstr& strName,
        CRpcServices* pIf ) :
            super( strName, pIf )
    { SetClassId( clsid( CFuseReqFileProxy ) ); }

    gint32 fs_read( fuse_req_t req,
        fuse_bufvec*& buf,
        off_t off,
        size_t size,
        fuse_file_info *fi,
        std::vector< BufPtr >& vecBackup,
        fuseif_intr_data* d ) override;

    gint32 fs_write_buf(
        fuse_req_t req,
        fuse_bufvec *buf,
        fuse_file_info *fi,
        fuseif_intr_data* d ) override;

    gint32 RemoveResp( guint64 qwReqId );

    gint32 fs_ioctl(
        const char *path, unsigned int cmd,
        void *arg, fuse_file_info *fi,
        unsigned int flags,
        void *data ) override;
};

class CFuseStmDir : public CFuseDirectory
{
    public:
    typedef CFuseDirectory super;
    CFuseStmDir( CRpcServices* pIf ) :
        super( STREAM_DIR, pIf )
    { SetClassId( clsid( CFuseStmDir ) ); }
};

class CFuseStmFile : public CFuseFileEntry
{
    protected:

    struct OUTREQ
    {
        fuse_req_t req;
        fuse_bufvec* bufvec;
        size_t idx; 
        PINTR pintr;
    };

    HANDLE m_hStream = INVALID_HANDLE;
    std::atomic< bool >  m_bFlowCtrl;
    gint32 SendBufVec( OUTREQ& oreq );
    gint32 FillIncomingQue();

    public:

    typedef CFuseFileEntry super;
    CFuseStmFile( const stdstr& strName,
        HANDLE hStm, CRpcServices* pIf ) :
        super( strName, pIf ),
        m_hStream( hStm ),
        m_bFlowCtrl( false )
    {
        SetClassId( clsid( CFuseStmFile ) );
    }

    inline bool GetFlowCtrl() const
    { return m_bFlowCtrl; }

    inline void SetFlowCtrl( bool bFlowCtl )
    { m_bFlowCtrl = bFlowCtl; }

    HANDLE GetStream() const
    { return m_hStream; }

    void SetStream( HANDLE hStream )
    { m_hStream = hStream; }

    gint32 OnReadStreamComplete(
        HANDLE hStream,
        gint32 iRet,
        BufPtr& pBuf,
        IConfigDb* pCtx );

    gint32 OnWriteStreamComplete(
        HANDLE hStream,
        gint32 iRet,
        BufPtr& pBuf,
        IConfigDb* pCtx );

    gint32 OnWriteResumed();

    gint32 fs_poll(const char *path,
        fuse_file_info *fi,
        fuse_pollhandle *ph,
        unsigned *reventsp ) override;

    gint32 fs_read( fuse_req_t req,
        fuse_bufvec*& buf,
        off_t off,
        size_t size,
        fuse_file_info *fi,
        std::vector< BufPtr >& vecBackup,
        fuseif_intr_data* d ) override;

    gint32 fs_write_buf(
        fuse_req_t req,
        fuse_bufvec *buf,
        fuse_file_info *fi,
        fuseif_intr_data* d ) override;

    gint32 fs_unlink(
        const char* path ) override;
};

class CFuseStmEvtFile : public CFuseEvtFile
{
    public:
    typedef CFuseEvtFile super;
    CFuseStmEvtFile( CRpcServices* pIf ) :
        super( JSON_STMEVT_FILE, pIf )
    { SetClassId( clsid( CFuseStmEvtFile ) ); }
};

// top-level dirs under the mount point
// proxy side only
class CFuseConnDir : public CFuseDirectory
{
    CfgPtr m_pConnParams;

    public:
    typedef CFuseDirectory super;
    CFuseConnDir( const stdstr& strName )
        : super( strName, nullptr )
    {
        SetClassId( clsid( CFuseConnDir ) );
        SetMode( S_IRUSR | S_IXUSR );

        // add an RO _nexthop directory this dir is
        // for docking nodes of sub router-path
        stdstr strHopDir = HOP_DIR;
        auto pDir = std::shared_ptr<CDirEntry>
            ( new CFuseDirectory( strHopDir, nullptr ) ); 
        CFuseObjBase* pObj = dynamic_cast
            < CFuseObjBase* >( pDir.get() );
        pObj->SetMode( S_IRUSR | S_IXUSR );
        pObj->DecRef();
        AddChild( pDir );

        // add an RO file for connection parameters
        stdstr strParams = CONN_PARAM_FILE;
        auto pFile = std::shared_ptr<CDirEntry>
            ( new CFuseTextFile( strParams ) ); 
        pObj = dynamic_cast
            < CFuseObjBase* >( pFile.get() );
        pObj->SetMode( S_IRUSR );
        pObj->DecRef();
        AddChild( pFile );
    }

    stdstr GetRouterPath( CDirEntry* pDir ) const
    {
        if( pDir->IsRoot() )
            return stdstr( "" );

        stdstr strPath;
        strPath = "/";
        CFuseConnDir* pConnDir = dynamic_cast
            < CFuseConnDir* >( pDir );
        if( pConnDir != nullptr )
            return strPath;

        stdstr strName = pDir->GetName();
        strPath += strName;

        CDirEntry* pParent = pDir->GetParent();
        while( pParent != nullptr &&
            pConnDir == nullptr )
        {
            strName = "/";
            strName += pParent->GetName();
            pParent = pParent->GetParent();
            pConnDir = dynamic_cast
                < CFuseConnDir* >( pParent );
            if( strName == HOP_DIR )
                continue;
            strPath.insert( 0, strName );
        }
        return strPath;
    }

    gint32 AddSvcDir(
        const CHILD_TYPE& pEnt )
    {
        const CFuseSvcDir* pDir = static_cast
            < CFuseSvcDir* >( pEnt.get() );
        gint32 ret = 0;
        do{
            if( pDir == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            ObjPtr pIf = pDir->GetIf();
            if( pIf.IsEmpty() )
            {
                ret = -EFAULT;
                break;
            }

            CCfgOpenerObj oIfCfg(
                ( CObjBase* )pIf ); 

            IConfigDb* pConnParams = nullptr;
            ret = oIfCfg.GetPointer(
                propConnParams, pConnParams );
            if( ERROR( ret ) )
                break;

            if( super::GetCount() <= 2 )
            {
                SetConnParams( pConnParams );
            }
            else if( !IsEqualConn( pConnParams ) )
            {
                ret = -EACCES;
                break;
            }

            stdstr strPath; 
            ret = oIfCfg.GetStrProp(
                propRouterPath, strPath );
            if( ERROR( ret ) )
                break;

            stdstr strPath2 = "/";
            if( strPath == strPath2 )
            {
                ret = AddChild( pEnt );
                break;
            }

            ret =IsMidwayPath( strPath2, strPath );
            if( ERROR( ret ) )
            {
                ret = -EINVAL;
                break;
            }

            stdstr strRest =
                strPath.substr( strPath2.size() );

            std::vector< stdstr > vecComps;
            ret = CRegistry::Namei(
                strRest, vecComps );
            if( ERROR( ret ) )
                break;

            if( vecComps.empty() )
            {
                ret = -EINVAL;
                break;
            }
            if( vecComps[ 0 ] == "/" )
                vecComps.erase( vecComps.begin() );

            CDirEntry* pDir = nullptr;
            CDirEntry* pCur = this;
            CDirEntry* pHop = nullptr;
            for( auto& elem : vecComps )
            {
                pDir = pCur->GetChild( HOP_DIR );
                if( pDir == nullptr )
                {
                    CFuseDirectory* pNewDir =
                    new CFuseDirectory( HOP_DIR, pIf );
                    pNewDir->DecRef();

                    std::shared_ptr< CDirEntry >
                        pEnt1( pNewDir );
                    pCur->AddChild( pEnt1 );
                    pDir = pNewDir;
                    pNewDir->SetMode( S_IRUSR | S_IXUSR );
                    pHop = nullptr;
                }
                else
                {
                    pHop = pDir->GetChild( elem );
                }

                if( pHop == nullptr )
                {
                    CFuseDirectory* pNewDir =
                        new CFuseDirectory( elem, pIf );
                    pNewDir->DecRef();

                    std::shared_ptr< CDirEntry >
                        pEnt1( pNewDir );
                    pDir->AddChild( pEnt1 );
                    pNewDir->SetMode( S_IRUSR | S_IXUSR );
                    pHop = pEnt1.get();
                }
                pCur = pHop;
            }
            pCur->AddChild( pEnt );

        }while( 0 );

        return ret;
    };

    void SetConnParams( const CfgPtr& pCfg )
    {
        m_pConnParams = pCfg;
        stdstr strDump = DumpConnParams();
        CDirEntry* pEnt =
            GetChild( CONN_PARAM_FILE );
        CFuseTextFile* pText = static_cast
            < CFuseTextFile* >( pEnt );
        pText->SetContent( strDump );
    }

    bool IsEqualConn( const IConfigDb* pConn ) const
    {
        CConnParamsProxy oConn(
            ( IConfigDb* )m_pConnParams );
        CConnParamsProxy oConn1( pConn );
        return ( oConn == oConn1 );
    }

    stdstr DumpConnParams()
    {
        gint32 ret = 0;
        if( m_pConnParams.IsEmpty() )
            return "";

        stdstr strRet;
        do{
            CConnParamsProxy oConn(
                ( IConfigDb* )m_pConnParams );
            
            bool bVal = oConn.IsServer();
            strRet += "IsServer=";
            strRet += ( bVal ? "true\n" : "false\n" );

            stdstr strVal = oConn.GetSrcIpAddr();
            if( strVal.size() )
            {
                strRet += "SrcIpAddr=";
                strRet += strVal + "\n";
            }

            guint32 dwVal = oConn.GetSrcPortNum();
            if( dwVal != 0 )
            {
                strRet += "SrcPortNum=";
                strRet += std::to_string( dwVal );
                strRet += "\n";
            }

            strVal = oConn.GetDestIpAddr();
            if( strVal.size() )
            {
                strRet += "DestIpAddr";
                strRet += strVal + "\n";
            }

            dwVal = oConn.GetDestPortNum();

            if( dwVal != 0 )
            {
                strRet += "DestPortNum=";
                strRet += std::to_string( dwVal );
                strRet += "\n";
            }

            bVal = oConn.IsWebSocket();
            strRet += "WebSocket=";
            strRet += ( bVal ?\
                "enabled\n" : "disabled\n" );

            if( bVal )
            {
                strRet += "URL=";
                strRet += oConn.GetUrl() + "\n";
            }

            bVal = oConn.IsSSL();
            strRet += "SSL=";
            strRet += ( bVal ?
                "enabled\n" : "disabled\n" );

            bVal = oConn.HasAuth(); 
            strRet += "Authentication=";
            strRet += ( bVal ?
                "enabled\n" : "disabled\n" );

            if( bVal )
            {
                CCfgOpener oCfg(
                    ( IConfigDb*)m_pConnParams );
                IConfigDb* pAuthInfo;
                ret = oCfg.GetPointer(
                    propAuthInfo, pAuthInfo );
                if( ERROR( ret ) )
                    break;

                CCfgOpener oAuth( pAuthInfo );
                ret = oAuth.GetStrProp(
                    propAuthMech, strVal );
                if( ERROR( ret ) )
                    break;

                strRet += "mechanism=";
                strRet += strVal + "\n";

                if( strVal != "krb5" )
                    break;

                ret = oAuth.GetStrProp(
                    propUserName, strVal );
                if( SUCCEEDED( ret ) )
                {
                    strRet += "\tuser=\"";
                    strRet += strVal + "\"\n";
                }

                ret = oAuth.GetStrProp(
                    propRealm, strVal );
                if( SUCCEEDED( ret ) ) 
                {
                    strRet += "\trealm=\"";
                    strRet += strVal + "\"\n";
                }

                ret = oAuth.GetBoolProp(
                    propSignMsg, bVal );
                if( SUCCEEDED( ret ) )
                {
                    strRet += "\tsignmsg=";
                    strRet += ( bVal ?
                        "true\n" : "false\n" );
                }
            }

        }while( 0 );

        return strRet;
    }
};

bool IsUnmounted( CRpcServices* pIf );
#define GET_STMCTX_LOCKED( _hStream, _pCtx ) \
    CFuseServicePoint* _pSp = const_cast \
        < CFuseServicePoint* >( this );\
    CRpcServices* _pSvc = _pSp;\
    if( this->IsServer() ) \
    {\
        CStreamServerSync *pStm =\
            dynamic_cast< CStreamServerSync*>( _pSvc );\
        ret = pStm->GetContext( _hStream, _pCtx );\
        if( ERROR( ret ) )\
            break;\
    }\
    else\
    {\
        CStreamProxySync *pStm = \
            dynamic_cast< CStreamProxySync*>( _pSvc );\
        ret = pStm->GetContext( _hStream, _pCtx ); \
        if( ERROR( ret ) ) \
            break; \
    } \
    CCfgOpener oCfg( \
        ( IConfigDb* )_pCtx ); \

#define WLOCK_TESTMNT0( _pDir_ )\
    CFuseSvcDir* _pSvcDir = static_cast \
        < CFuseSvcDir* >( _pDir_ ); \
    CWriteLock oSvcLock( _pSvcDir->GetSvcLock() ); \
    ret = oSvcLock.GetStatus();\
    if( ERROR( ret ) )\
        break;\
    if( rpcf::IsUnmounted( _pSvcDir->GetIf() ) ) \
    { \
        ret = ERROR_STATE; \
        break; \
    }
#define WLOCK_TESTMNT \
    auto _pDir = GetSvcDir(); \
    WLOCK_TESTMNT0( _pDir.get() )

#define RLOCK_TESTMNT0( _pDir_ ) \
    CFuseSvcDir* _pSvcDir = static_cast \
        < CFuseSvcDir* >( _pDir_ ); \
    CReadLock oSvcLock( _pSvcDir->GetSvcLock() ); \
    ret = oSvcLock.GetStatus();\
    if( ERROR( ret ) )\
        break;\
    if( rpcf::IsUnmounted( _pSvcDir->GetIf() ) ) \
    { \
        ret = ERROR_STATE; \
        break; \
    }

#define RLOCK_TESTMNT \
    CHILD_TYPE _pDir = GetSvcDir(); \
    RLOCK_TESTMNT0( _pDir.get() )

class CFuseSvcServer;
class CFuseSvcProxy;

CFuseSvcDir* GetSvcDir( const char* _path );
CFuseSvcDir* GetSvcDir( CRpcServices* pIf );

#define LOCK_TESTMNT( _pIf, _lock_ ) \
    CFuseSvcDir* _pSvcDir = rpcf::GetSvcDir( _pIf ); \
    _lock_ oSvcLock( _pSvcDir->GetSvcLock() ); \
    ret = oSvcLock.GetStatus();\
    if( ERROR( ret ) )\
        break;\
    if( rpcf::IsUnmounted( _pIf ) ) \
    { ret = ERROR_STATE; break; }

#define RLOCK_TESTMNT2( _pIf ) \
    LOCK_TESTMNT( _pIf, CReadLock )

#define WLOCK_TESTMNT2( _pIf ) \
    LOCK_TESTMNT( _pIf, CWriteLock )

#define RLOCK_TESTMNT3( _path ) \
    CFuseSvcDir* _pDir = rpcf::GetSvcDir( _path );\
    if( _pDir == nullptr ) \
    { ret = -ENOENT; break; } \
    RLOCK_TESTMNT0( _pDir );

#define WLOCK_TESTMNT3( _path ) \
    CFuseSvcDir* _pDir = rpcf::GetSvcDir( _path );\
    if( _pDir == nullptr ) \
    { ret = -ENOENT; break; } \
    WLOCK_TESTMNT0( _pDir );

#define IFBASE( _bProxy ) std::conditional< \
    _bProxy, CAggInterfaceProxy, CAggInterfaceServer>::type


template< bool bProxy >
class CFuseServicePoint :
    public virtual IFBASE( bProxy ) 
{
    std::shared_ptr< CDirEntry > m_pSvcDir;
    stdstr m_strSvcPath;
    bool    m_bUnmounted = false;

    public:
    typedef typename IFBASE( bProxy ) _MyVirtBase;
    typedef typename IFBASE( bProxy ) super;
    CFuseServicePoint( const IConfigDb* pCfg ) :
        super( pCfg )
    {}

    using super::GetIid;
    const EnumClsid GetIid() const override
    { return iid( CFuseServicePoint ); }

    using super::OnPostStart;
    gint32 OnPostStart(
        IEventSink* pCallback ) override
    { return BuildDirectories(); }

    inline CHILD_TYPE GetSvcDir() const
    { return m_pSvcDir; }

    bool IsUnmounted() const
    { return m_bUnmounted; }

    // build the directory hierarchy for this service
    gint32 BuildDirectories()
    {
        gint32 ret = 0;
        do{
            stdstr strObjPath;
            CCfgOpenerObj oIfCfg( this );

            ret = oIfCfg.GetStrProp(
                propObjPath, strObjPath );
            if( ERROR( ret ) )
                break;
            size_t pos = strObjPath.rfind( '/' );
            if( pos + 1 == strObjPath.size() )
            {
                ret = -EINVAL;
                break;
            }

            // add an RO directory for this svc
            stdstr strName =
                strObjPath.substr( pos + 1 );
            m_pSvcDir = std::shared_ptr<CDirEntry>
                ( new CFuseSvcDir( strName, this ) ); 
            m_pSvcDir->DecRef();

            strName = JSON_REQ_FILE;
            CFuseObjBase* pObj = nullptr;
            if( bProxy )
            {
                // add an RW request file
                auto pFile = std::shared_ptr<CDirEntry>
                    ( new CFuseReqFileProxy( strName, this ) ); 
                m_pSvcDir->AddChild( pFile );
                pObj = dynamic_cast< CFuseObjBase* >
                    ( pFile.get() );
                pObj->SetMode( S_IRUSR | S_IWUSR );
                pObj->DecRef();

                // add an RO RESP file 
                strName = JSON_RESP_FILE;
                pFile.reset( 
                    new CFuseRespFileProxy( strName, this ) );
                m_pSvcDir->AddChild( pFile );
                pObj = dynamic_cast
                    < CFuseObjBase* >( pFile.get() );
                pObj->SetMode( S_IRUSR );
                pObj->DecRef();

                // add an RO event file 
                strName = JSON_EVT_FILE;
                pFile = std::shared_ptr<CDirEntry>
                    ( new CFuseEvtFile( strName, this ) ); 
                m_pSvcDir->AddChild( pFile );
                pObj = dynamic_cast
                    < CFuseObjBase* >( pFile.get() );
                pObj->SetMode( S_IRUSR );
                pObj->DecRef();
            }
            else
            {
                // add an RO request file
                auto pFile = std::shared_ptr<CDirEntry>
                    ( new CFuseReqFileSvr( strName, this ) ); 
                m_pSvcDir->AddChild( pFile );
                pObj = dynamic_cast< CFuseObjBase* >
                    ( pFile.get() );
                if( bProxy )
                    pObj->SetMode( S_IRUSR | S_IWUSR );
                else
                    pObj->SetMode( S_IRUSR );
                pObj->DecRef();

                // add an WO RESP file for both
                // response and event
                strName = JSON_RESP_FILE;
                pFile = std::shared_ptr<CDirEntry>
                    ( new CFuseRespFileSvr( strName, this ) ); 
                m_pSvcDir->AddChild( pFile );
                pObj = dynamic_cast
                    < CFuseObjBase* >( pFile.get() );
                pObj->SetMode( S_IWUSR );
                pObj->DecRef();
            }

            // add an RW directory for streams
            auto pDir = std::shared_ptr<CDirEntry>
                ( new CFuseStmDir( this ) ); 
            m_pSvcDir->AddChild( pDir );
            pObj = dynamic_cast
                < CFuseObjBase* >( pDir.get() );
            pObj->SetMode( S_IRWXU );
            pObj->DecRef();

            // add an RO stmevt file for stream events
            auto pFile = std::shared_ptr<CDirEntry>
                ( new CFuseStmEvtFile( this ) ); 
            pDir->AddChild( pFile );
            pObj = dynamic_cast
                < CFuseObjBase* >( pFile.get() );
            pObj->SetMode( S_IRUSR );
            pObj->DecRef();

        }while( 0 );

        return ret;
    }

    gint32 GetSvcPath( stdstr& strPath )
    {
        if( m_strSvcPath.empty() )
        {
            CDirEntry* pDir = m_pSvcDir.get();
            if( unlikely( m_pSvcDir == nullptr ) )
                return -EFAULT;
            else while( pDir != nullptr )
            {
                stdstr strName = "/";
                strName.append( pDir->GetName() );
                strPath.insert( 0, strName );
                pDir = pDir->GetParent();
            }
            m_strSvcPath = strPath;
        }
        else
        {
            strPath = m_strSvcPath;
        }
        return STATUS_SUCCESS;
    }

    gint32 StreamToName(
        HANDLE hStream,
        stdstr& strName ) const
    {
        gint32 ret = 0;
        if( hStream == INVALID_HANDLE )
            return -EINVAL;
        do{
            CfgPtr pCtx;
            CStdRMutex oLock( this->GetLock() );
            GET_STMCTX_LOCKED( hStream, pCtx );
            ret = oCfg.GetStrProp(
                propPath1, strName );
            
        }while( 0 );

        return ret;
    }

    gint32 NameToStream(
        const stdstr& strName,
        HANDLE& hStream ) const
    {
        gint32 ret = 0;
        do{
            CDirEntry* pDir =
                m_pSvcDir->GetChild( STREAM_DIR );
            if( pDir == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            CDirEntry* pStmDir =
                pDir->GetChild( strName );
            if( pStmDir == nullptr )
            {
                ret = -ENOENT;
                break;
            }

            CFuseStmFile* pStmFile =
                static_cast< CFuseStmFile* >
                    ( pStmDir );
            hStream = pStmFile->GetStream();

        }while( 0 );

        return ret;
    }

    gint32 AddStreamName(
        HANDLE hStream,
        stdstr& strName )
    {
        gint32 ret = 0;
        do{
            CfgPtr pCtx;
            CStdRMutex oLock( this->GetLock() );
            GET_STMCTX_LOCKED( hStream, pCtx );
            ret = oCfg.SetStrProp(
                propPath1, strName );
        }while( 0 );

        return ret;
    }

    gint32 CreateStmFile( HANDLE hStream )
    {
        gint32 ret = 0;        
        do{
            stdstr strSvcPath;
            ret = GetSvcPath( strSvcPath );
            if( ERROR( ret ) )
                break;
            CHILD_TYPE pSp = GetSvcDir();
            CFuseStmDir* pDir =
                static_cast< CFuseStmDir* >
                ( pSp->GetChild( STREAM_DIR ) );

            CfgPtr pCfg;
            stdstr strName;
            IStream* pStmIf = dynamic_cast
                < IStream* >( this );
            if( unlikely( pStmIf == nullptr ) )
            {
                ret = -EFAULT;
                break;
            }
            ret = pStmIf->GetDataDesc(
                hStream, pCfg );
            if( SUCCEEDED( ret ) )
            {
                CCfgOpener oDesc(
                    ( IConfigDb* )pCfg );
                ret = oDesc.GetStrProp(
                    propNodeName, strName );

                if( strName.size() > REG_MAX_NAME )
                    strName.erase( REG_MAX_NAME );

                if( SUCCEEDED( ret ) &&
                    pDir->GetChild( strName ) != nullptr )
                    ret = -EEXIST;
            }

            if( ERROR( ret ) )
            {
                strName = "_stm";
                strName += std::to_string(
                    pDir->GetCount() );
            }

            ret = pDir->AddChild(
                std::shared_ptr< CDirEntry >
                ( new CFuseStmFile( 
                    strName, hStream, this ) ) );

            strSvcPath.push_back( '/' );
            strSvcPath.append( STREAM_DIR );
            if( GetFuse() == nullptr )
                break;
            fuse_invalidate_path(
                GetFuse(), strSvcPath.c_str() );
            
        }while( 0 );

        return ret;
    }

    gint32 DeleteStmFile(
        const stdstr& strName )
    {
        gint32 ret = 0;        
        do{
            stdstr strSvcPath;
            ret = GetSvcPath( strSvcPath );
            if( ERROR( ret ) )
                break;
            CHILD_TYPE pSp = GetSvcDir();
            CFuseStmDir* pDir =
                static_cast< CFuseStmDir* >
                ( pSp->GetChild( STREAM_DIR ) );

            CHILD_TYPE pChild;
            ret = pDir->GetChild(
                    strName, pChild );
            if( ERROR( ret ) )
                break;
            CFuseStmFile* pFile =
                static_cast< CFuseStmFile* >
                ( pChild.get() );

            CFuseMutex oFileLock( pFile->GetLock() );
            if( pFile->GetOpCount() > 0 )
            {
                pFile->SetHidden();
                if( pFile->GetPollHandle() != nullptr )
                {
                    pFile->SetRevents(
                        pFile->GetRevents() | POLLHUP );
                    fuse_notify_poll(
                        pFile->GetPollHandle() );
                    pFile->SetPollHandle( nullptr );
                }
                break;
            }
            oFileLock.Unlock();

            fuse_ino_t inoParent = pDir->GetObjId();
            fuse_ino_t inoChild = pFile->GetObjId();
            pDir->RemoveChild( strName );
            fuse* pFuse = GetFuse();
            if( pFuse == nullptr )
                break;

            fuse_session* se =
                fuse_get_session( pFuse );

            fuse_lowlevel_notify_delete( se,
                inoParent, inoChild,
                strName.c_str(),
                strName.size() );

        }while( 0 );

        return ret;
    }

    gint32 Unmount()
    {
        gint32 ret = 0;        
        do{
            WLOCK_TESTMNT;
            stdstr strSvcPath;
            ret = GetSvcPath( strSvcPath );
            if( ERROR( ret ) )
                break;
            std::vector< CHILD_TYPE > vecChildren;

            m_bUnmounted = true;

            CFuseStmDir* pDir =
                static_cast< CFuseStmDir* >
                ( _pSvcDir->GetChild( STREAM_DIR ) );

            ret = pDir->GetChildren( vecChildren );
            if( ERROR( ret ) )
                break;

            std::vector< HANDLE > vecStreams;
            for( auto& elem : vecChildren )
            {
                CFuseStmFile* pFile =
                dynamic_cast< CFuseStmFile* >
                    ( elem.get() );

                if( pFile != nullptr &&
                    pFile->GetStream() )
                {
                    vecStreams.push_back( 
                        pFile->GetStream() );
                }

                pDir->RemoveChild(
                        elem->GetName() );
            }
            oSvcLock.Unlock();

            InterfPtr pIf = this;
            for( auto& elem : vecStreams )
                CloseChannel( pIf, elem );

        }while( 0 );

        return ret;
    }

    gint32 SendStmEvent(
        const stdstr& strName, gint32 iRet )
    { return 0; }

    gint32 OnWriteResumedFuse(
        HANDLE hStream )
    {
        gint32 ret = 0;
        do{
            RLOCK_TESTMNT;
            stdstr strName;
            ret = StreamToName(
                hStream, strName );
            if( ERROR( ret ) )
                break;

            stdstr strSvcPath;
            ret = GetSvcPath( strSvcPath );
            if( ERROR( ret ) )
                break;
            CFuseStmDir* pDir =
                static_cast< CFuseStmDir* >
                ( _pSvcDir->GetChild( STREAM_DIR ) );

            auto pEnt = pDir->GetChild( strName );
            if( pEnt == nullptr )
            {
                ret = -ENOENT;
                break;
            }
            CFuseStmFile* pStmFile = static_cast
                < CFuseStmFile* >( pEnt );

            pStmFile->OnWriteResumed();
        }while( 0 );

        return ret;
    }

    inline gint32 OnReadStreamCompleteFuse(
        HANDLE hStream,
        gint32 iRet,
        BufPtr& pBuf,
        IConfigDb* pCtx )
    {
        gint32 ret = 0;
        do{
            RLOCK_TESTMNT;
            stdstr strName;
            ret = StreamToName(
                hStream, strName );
            if( ERROR( ret ) )
                break;

            stdstr strSvcPath;
            ret = GetSvcPath( strSvcPath );
            if( ERROR( ret ) )
                break;
            CFuseStmDir* pDir =
                static_cast< CFuseStmDir* >
                ( _pSvcDir->GetChild( STREAM_DIR ) );

            auto pEnt = pDir->GetChild( strName );
            if( pEnt == nullptr )
            {
                ret = -ENOENT;
                break;
            }
            CFuseStmFile* pStmFile = static_cast
                < CFuseStmFile* >( pEnt );

            pStmFile->OnReadStreamComplete(
                hStream, iRet, pBuf, pCtx );

            if( SUCCEEDED( iRet ) )
                break;

        }while( 0 );

        return ret;
    }

    inline gint32 OnWriteStreamCompleteFuse(
        HANDLE hStream,
        gint32 iRet,
        BufPtr& pBuf,
        IConfigDb* pCtx )
    {
        gint32 ret = 0;
        do{
            RLOCK_TESTMNT;
            stdstr strName;
            ret = StreamToName(
                hStream, strName );
            if( ERROR( ret ) )
                break;

            stdstr strSvcPath;
            ret = GetSvcPath( strSvcPath );
            if( ERROR( ret ) )
                break;
            CFuseStmDir* pDir =
                static_cast< CFuseStmDir* >
                ( _pSvcDir->GetChild( STREAM_DIR ) );

            auto pEnt = pDir->GetChild( strName );
            if( pEnt == nullptr )
            {
                ret = -ENOENT;
                break;
            }
            CFuseStmFile* pStmFile = static_cast
                < CFuseStmFile* >( pEnt );

            pStmFile->OnWriteStreamComplete(
                hStream, iRet, pBuf, pCtx );

        }while( 0 );

        return ret;
    }

    inline gint32 OnStreamReadyFuse(
        HANDLE hStream )
    {
        gint32 ret = 0;
        do{
            RLOCK_TESTMNT;
            if( bProxy )
                break;
            if( this->GetState() != stateConnected )
                break;
            ret = CreateStmFile( hStream );

        }while( 0 );

        return ret;
    }

    inline gint32 OnStmClosingFuse(
        HANDLE hStream )
    {
        gint32 ret = 0;
        do{
            WLOCK_TESTMNT;
            stdstr strName;
            ret = StreamToName(
                hStream, strName );
            if( ERROR( ret ) )
                break;
            ret = DeleteStmFile( strName );

        }while( 0 );

        return ret;
    }

    gint32 AcceptNewStreamFuse(
        IEventSink* pCallback,
        IConfigDb* pDataDesc )
    {
        gint32 ret = 0;
        do{
            RLOCK_TESTMNT;

            auto pEnt = GetSvcDir();
            auto pStmDir =
                pEnt->GetChild( STREAM_DIR );
            if( pStmDir->GetCount() >
                MAX_STREAMS_PER_SVC )
            {
                ret = -EOVERFLOW;
                break;
            }

            if( pDataDesc == nullptr )
            {
                ret = -EINVAL;
                break;
            }

            stdstr strName;
            CCfgOpener oDesc( pDataDesc );
            ret = oDesc.GetStrProp(
                propNodeName, strName );
            if( ERROR( ret ) )
            {
                ret = -ENOENT;
                break;
            }
            if( pStmDir->GetChild( strName ) )
            {
                ret = -EEXIST;
                break;
            }

        }while( 0 );

        return ret;
    }

    // to receive a request on server side or a
    // response or event on the proxy side
    gint32 ReceiveMsgJson(
            const stdstr& strMsg,
            guint64 qwReqId )
    {
        gint32 ret = 0;
        do{
            RLOCK_TESTMNT;

            stdstr strFile = bProxy ?
                JSON_RESP_FILE : JSON_REQ_FILE;

            auto pReqEnt =
                _pDir->GetChild( strFile );

            auto pEvtFile = dynamic_cast
                < CFuseEvtFile* >( pReqEnt );

            ret = pEvtFile->ReceiveMsgJson(
                strMsg, qwReqId );

        }while( 0 );

        return ret;
    }

    gint32 OnDBusEvent(
        EnumEventId iEvent ) override
    {
        gint32 ret = 0;

        switch( iEvent )
        {
        case eventDBusOffline:
            {
                ret = this->SetStateOnEvent( iEvent );
                if( ERROR( ret ) )
                    break;

                gint32 (*func)( CFuseServicePoint* pIf ) =
                    ([]( CFuseServicePoint* pIf )
                {
                    gint32 ret = 0;
                    do{
                        WLOCK_TESTMNT2( pIf );
                        TaskletPtr pDummy;
                        pDummy.NewObj( clsid(
                            CIfDummyTask ) );
                        pIf->StopEx( pDummy );

                    }while( 0 );
                    return 0;
                });

                CIoManager* pMgr = this->GetIoMgr();
                TaskletPtr pTask;
                ret = NEW_FUNCCALL_TASK(
                    pTask, pMgr, func, this );
                if( ERROR( ret ) )
                    break;
                pMgr->RescheduleTask( pTask );
                break;
            }
        default:
            {
                ret = super::OnDBusEvent( iEvent );
                break;
            }
        }
        return ret;
    }
};

class CFuseSvcProxy :
    public CFuseServicePoint< true >
{
    public:
    typedef CFuseServicePoint< true > super;
    CFuseSvcProxy( const IConfigDb* pCfg ):
    _MyVirtBase( pCfg ), super( pCfg )
    {}

    virtual gint32 DispatchReq(
        IConfigDb* pContext,
        const Json::Value& valReq,
        Json::Value& valResp ) = 0;

    // to receive a request on server side or a
    // response or event on the proxy side
    gint32 ReceiveEvtJson(
            const stdstr& strMsg )
    {
        gint32 ret = 0;
        do{
            RLOCK_TESTMNT; 
            stdstr strFile = JSON_EVT_FILE;

            auto pReqEnt =
                _pDir->GetChild( strFile );

            auto pEvtFile = dynamic_cast
                < CFuseEvtFile* >( pReqEnt );

            ret = pEvtFile->ReceiveEvtJson( strMsg );

        }while( 0 );

        return ret;
    }
};

class CFuseSvcServer :
    public CFuseServicePoint< false >
{
    public:
    typedef CFuseServicePoint< false > super;
    CFuseSvcServer( const IConfigDb* pCfg ):
    _MyVirtBase( pCfg ), super( pCfg )
    {}

    //Response&Event Dispatcher
    virtual gint32 DispatchMsg(
        const Json::Value& valMsg ) = 0;
};

template< bool bProxy,
    class T= typename std::conditional<
        bProxy,
        CInterfaceProxy,
        CInterfaceServer >::type >
class CFuseRootBase:
    public T
{
    protected:
    fuse* m_pFuse = nullptr;
    CRegistry m_oReg;
    std::vector< ObjPtr > m_vecIfs;
    CIoManager* m_pMgr = nullptr;

    public:
    typedef T super;

    using SVC_TYPE=typename std::conditional<
        std::is_base_of< CInterfaceProxy, T >::value,
        CFuseSvcProxy, CFuseSvcServer > ::type;

    inline CFuseRootDir* GetRootDir() const
    {
        CDirEntry* pRoot =
            m_oReg.GetRootDir();
        CFuseRootDir* pRet =
            static_cast< CFuseRootDir* >
                ( pRoot );
        return pRet;
    }

    gint32 GetSvcDir( const stdstr& strDir,
        CFuseSvcDir*& pDir )
    {
        gint32 ret = 0;
        std::vector<stdstr> vecComp;
        if( strDir.size() > REG_MAX_PATH )
            return -EINVAL;

        ret = CRegistry::Namei( strDir, vecComp );
        if( ERROR( ret ) )
            return ret;

        auto itr = vecComp.begin();
        CDirEntry* pCurDir = m_oReg.GetRootDir();
        ret = -ENOENT;
        while( itr != vecComp.end() )
        {
            if( *itr == "." )
            {
            }
            else if( *itr == ".." )
            {
                if( pCurDir == GetRootDir() )
                {
                    ret = -EINVAL;
                    break;
                }

                pCurDir = pCurDir->GetParent();
            }
            else if( *itr == "/" )
            {
                pCurDir = GetRootDir();
            }
            else
            {
                pCurDir = pCurDir->GetChild( *itr );
                if( pCurDir == NULL )
                    break;
            }
            if( pCurDir->GetClsid() == 
                clsid( CFuseSvcDir ) )
            {
                ret = STATUS_SUCCESS;
                break;
            }
            ++itr;
        }
        if( SUCCEEDED( ret ) )
            pDir = static_cast< CFuseSvcDir* >
                ( pCurDir );

        return ret;
    }

    CFuseRootBase( const IConfigDb* pCfg ):
        super( pCfg )
    {
        EnumClsid iClsid;
        if( this->IsServer() )
            iClsid = clsid( CFuseRootServer );
        else
            iClsid = clsid( CFuseRootProxy );
        this->SetClassId( iClsid );
        CCfgOpener oCfg( pCfg );
        gint32 ret = oCfg.GetPointer(
            propIoMgr, m_pMgr );
        if( ERROR( ret ) )
        {
            stdstr strMsg = DebugMsg( ret,
                "cannot find iomgr in "
                "CFuseRootBase's ctor" );
            throw std::runtime_error( strMsg );
        }
        CHILD_TYPE pRoot( new CFuseRootDir() );
        m_oReg.SetRootDir( pRoot );
    }

    fuse* GetFuse() const
    { return m_pFuse; }

    inline void SetFuse( fuse* pFuse )
    { m_pFuse = pFuse; }

    gint32 GetFuseObj(
        const stdstr& strPath,
        CFuseObjBase*& pDir )
    {
        if( strPath.empty() ||
            strPath[ 0 ] != '/' )
            return -EINVAL;

        if( strPath == "/" )
        {
            pDir = GetRootDir();
            return STATUS_SUCCESS;
        }

        CDirEntry* pTemp = nullptr;
        gint32 ret = m_oReg.GetEntry(
            strPath, pTemp );
        if( ERROR( ret ) )
            return ret;

        pDir = dynamic_cast
            < CFuseObjBase* >( pTemp );
        return STATUS_SUCCESS;
    }

    // mount the RPC file system at the mount point
    // strMntPt, with options pOptions
    gint32 DoMount(
        const stdstr& strMntPt,
        IConfigDb* pOptions );

    gint32 DoUnmount()
    {
        for( auto& elem : m_vecIfs )
        {
            if( bProxy )
            {
                CFuseSvcProxy* pIf = elem;
                pIf->Unmount();
            }
            else
            {
                CFuseSvcServer* pIf = elem;
                pIf->Unmount();
            }
        }
        SetFuse( nullptr );
        return STATUS_SUCCESS;
    }

    // add a service point
    gint32 AddSvcPoint(
        const stdstr& strName,
        const stdstr& strDesc,
        EnumClsid iClsid )
    {
        if( strName.empty() || strDesc.empty() )
            return -EINVAL;
        gint32 ret = 0;
        bool bAdded = false;
        do{
            CfgPtr pCfg;
            ret = CRpcServices::LoadObjDesc(
                strDesc, strName, !bProxy, pCfg );
            if( ERROR( ret ) )
                break;

            InterfPtr pIf;
            CCfgOpener oCfg( ( IConfigDb* )pCfg );
            oCfg.SetPointer(
                propIoMgr, this->GetIoMgr() );
            ret = pIf.NewObj( iClsid, pCfg );
            if( ERROR( ret ) )
                break;

            SVC_TYPE* pSp = pIf;
            if( pSp == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            ret = pIf->Start();
            if( ERROR( ret ) )
                break;

            if( bProxy )
            {
                while( pSp->GetState()== stateRecovery )
                    sleep( 1 );
                if( pSp->GetState() != stateConnected )
                {
                    ret = ERROR_STATE;
                    break;
                }
            }

            bAdded = true;
            m_vecIfs.push_back( pIf );
            CHILD_TYPE pSvcDir = pSp->GetSvcDir();

            CFuseObjBase* pRoot = nullptr;
            ret = GetFuseObj( "/", pRoot );
            if( ERROR( ret ) )
                break;

            if( !this->IsServer() )
            {
                std::vector< CHILD_TYPE > vecConns;
                pRoot->GetChildren( vecConns );
                ret = -ENOENT;
                for( auto& elem : vecConns )
                {
                    CFuseConnDir* pConn = static_cast
                        < CFuseConnDir* >( elem.get() );
                    ret = pConn->AddSvcDir( pSvcDir );
                    if( SUCCEEDED( ret ) )
                        break;
                }
                if( SUCCEEDED( ret ) )
                    break;

                gint32 idx = vecConns.size();
                CHILD_TYPE pNewDir( new CFuseConnDir(
                    stdstr( CONN_DIR_PREFIX ) +
                    std::to_string( idx ) ) );
                CFuseConnDir* pConn = static_cast
                    < CFuseConnDir* >( pNewDir.get() );
                ret = pConn->AddSvcDir( pSvcDir );
                if( ERROR( ret ) )
                    break;

                ret = pRoot->AddChild( pNewDir );
            }
            else
            {
                ret = pRoot->AddChild( pSvcDir );
            }

        }while( 0 );

        if( ERROR( ret ) )
        {
            if( bAdded )
            {
                InterfPtr pIf = m_vecIfs.back();
                m_vecIfs.pop_back();
                pIf->Stop();
            }
        }

        return ret;
    }

    gint32 OnPreStop( IEventSink* pCallback )
    {
        gint32 ret = 0;
        do{
            CParamList oParams;
            oParams.SetPointer( propIfPtr, this );

            TaskGrpPtr pStopTasks;
            ret = pStopTasks.NewObj(
                clsid( CIfTaskGroup ),
                oParams.GetCfg() );
            if( ERROR( ret ) )
                break;

            pStopTasks->SetRelation( logicNONE );

            for( auto& elem : m_vecIfs )
            {
                TaskletPtr pTask;
                ret = DEFER_IFCALLEX_NOSCHED2(
                    0, pTask, ObjPtr( elem ),
                    &CRpcServices::Stop );

                if( ERROR( ret ) )
                    break;

                pStopTasks->AppendTask( pTask );
            }
            m_vecIfs.clear();
            CIfRetryTask* pTask = pStopTasks;
            pTask->SetClientNotify( pCallback );
            CIoManager* pMgr = this->GetIoMgr();
            TaskletPtr pTempTask( pTask );
            ret = pMgr->RescheduleTask( pTempTask );
            if( ERROR( ret ) )
            {
                ( *pStopTasks )( eventCancelTask );
            }
            else
            {
                ret = STATUS_PENDING;
            }

        }while( 0 );
        return ret;
    }
};

class CFuseRootProxy :
    public CFuseRootBase< true > 
{
    public:
    typedef CFuseRootBase< true > super;
    CFuseRootProxy( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CFuseRootProxy ) ); }
};

class CFuseRootServer :
    public CFuseRootBase< false >
{
    public:
    typedef CFuseRootBase< false > super;
    CFuseRootServer( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CFuseRootServer ) ); }
};

}

struct fuseif_intr_data {
    pthread_t id;
    pthread_cond_t cond;
    int finished;
    rpcf::CFuseObjBase* fe;
};

gint32 fuseif_main( fuse_args& args,
    fuse_cmdline_opts& opts );

gint32 fuseif_daemonize( fuse_args& args,
    fuse_cmdline_opts& opts,
    int argc, char **argv );

