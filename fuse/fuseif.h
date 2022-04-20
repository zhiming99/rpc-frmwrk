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
#include "fusedefs.h"

struct fuseif_intr_data;

namespace rpcf
{

InterfPtr& GetRootIf();

fuse* GetFuse();

class CFuseObjBase;
class CFuseDirectory;

CFuseDirectory* GetRootDir();

gint32 InitRootIf(
    CIoManager* pMgr, bool bProxy );

void ReleaseRootIf();

gint32 CloseChannel(
    ObjPtr& pIf, HANDLE hStream );

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

    inline gint32 ReleaseRead()
    {
        CStdMutex oLock( m_oLock );
        --m_iReadCount;
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
        else
        {
            m_bWrite = false;
            while( m_queue.front() == true )
            {
                m_iReadCount++;
                m_queue.pop_front();
                Sem_Post( &m_semReader );
                if( m_queue.empty() )
                    break;
            }
        }
        return STATUS_SUCCESS;
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
        Lock();
    }

    ~CLocalLock()
    { 
        Unlock();
        m_pLock = nullptr;
    }

    void Unlock()
    {
        if( !m_bLocked || m_pLock == nullptr )
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

using DIR_SPTR=CHILD_TYPE;

class CFuseObjBase : public CDirEntry
{
    time_t m_tsAccTime = { 0 };
    time_t m_tsModTime = { 0 };
    guint32 m_dwStat;
    guint32 m_dwMode = S_IRUSR;
    guint32 m_dwOpenCount = 0;
    bool m_bHidden = false;
    bool m_bRemoved = false;
    bool m_bNonBlock = true;

    // poll related members
    fuse_pollhandle* m_pollHandle = nullptr;

    // events to return
    guint32 m_dwRevents = 0;
    mutable CSharedLock m_oLock;

    protected:
    CRpcServices* m_pIf = nullptr;

    public:
    typedef CDirEntry super;
    CFuseObjBase() :
        super()
    {
        UpdateTime();
        UpdateTime( true );
    }

    ~CFuseObjBase()
    {
        SetPollHandle( nullptr );
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

    inline void SetRemoved()
    { m_bRemoved = true; }

    inline bool IsRemoved() const
    {return m_bRemoved; }

    inline void SetNonBlock(
        bool bNonBlock = true )
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

    inline const time_t& GetAccTime() const
    { return m_tsAccTime; }

    inline void UpdateTime( bool bMod = false )
    {
        bMod ? m_tsModTime = time( nullptr ) :
           m_tsAccTime = time( nullptr ); 
    }
    inline void SetTime(
        const timespec& ts, bool bMod )
    {
        bMod ? m_tsModTime = ts.tv_sec :
           m_tsAccTime = ts.tv_sec; 
    }

    inline const time_t& GetModifyTime() const
    { return m_tsModTime; }

    gint32 AddChild(
        const DIR_SPTR& pEnt ) override;

    gint32 RemoveChild(
        const std::string& strName ) override;

    gint32 fs_access(const char *,
        fuse_file_info*, int);
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
        const char* path,
        fuse_file_info *fi,
        fuse_req_t req,
        fuse_bufvec*& buf,
        off_t off,
        size_t size,
        std::vector< BufPtr >& vecBackup,
        fuseif_intr_data* d )
    { return 0; }

    virtual gint32 fs_write_buf(
        const char* path,
        fuse_file_info *fi,
        fuse_req_t req,
        fuse_bufvec *buf,
        fuseif_intr_data* d )
    { return 0; }

    virtual gint32 fs_poll(
        const char *path,
        fuse_file_info *fi,
        fuse_pollhandle *ph,
        unsigned *reventsp )
    { return -ENOSYS; }

    virtual gint32 fs_open(
        const char *path,
        struct fuse_file_info *fi )
    { return -ENOSYS; }

    virtual gint32 fs_readdir(
        const char *path,
        fuse_file_info *fi,
        void *buf, fuse_fill_dir_t filler,
        off_t off, 
        fuse_readdir_flags flags)
    { return -ENOSYS; }

    virtual gint32 fs_getattr(
        const char *path, 
        fuse_file_info *fi,
        struct stat *stbuf )
    { return -ENOSYS; }

    virtual gint32 fs_ioctl(
        const char *path,
        fuse_file_info *fi,
        unsigned int cmd,
        void *arg, 
        unsigned int flags, void *data )
    { return -ENOSYS; }

    virtual gint32 fs_unlink(
        const char* path,
        fuse_file_info *fi,
        bool bSched )
    { return -EACCES; }

    virtual gint32 fs_opendir(
        const char* path,
        fuse_file_info* fi )
    { return -ENOSYS; }

    virtual gint32 fs_releasedir(
        const char* path,
        fuse_file_info* fi )
    { return -ENOSYS; }

    virtual gint32 fs_release(
        const char* path,
        fuse_file_info * fi );

    gint32 fs_utimens(
        const char *path,
        fuse_file_info *fi,
        const timespec tv[2] )
    { return 0; }

    inline CRpcServices* GetIf() const
    { return m_pIf; }

    inline void SetIf( CRpcServices* pIf )
    { m_pIf = pIf; }

};

class CFuseDirectory : public CFuseObjBase
{
    protected:

    public:
    typedef CFuseObjBase super;
    CFuseDirectory(
        const stdstr& strName,
        CRpcServices* pIf ) :
        super()
    {
        SetName( strName );
        SetIf( pIf );
        SetClassId( clsid( CFuseDirectory ) );
    }

    gint32 fs_getattr(
        const char *path, 
        fuse_file_info* fi,
        struct stat *stbuf) override;

    gint32 fs_opendir(
        const char* path,
        fuse_file_info * fi ) override;

    gint32 fs_releasedir(
        const char* path,
        fuse_file_info * fi ) override;

    gint32 fs_readdir(
        const char *path,
        fuse_file_info *fi,
        void *buf, fuse_fill_dir_t filler,
        off_t off,
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
        fuse_file_info* fi,
        struct stat *stbuf) override;
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

    gint32 FillBufVec(
        size_t& dwReqSize,
        std::deque< INBUF >& queBufs,
        std::vector< BufPtr >& vecBufs,
        fuse_bufvec*& bufvec );

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
        super()
    {
        SetName( strName );
        SetIf( pIf );
    }

    virtual gint32 CancelFsRequest(
        fuse_req_t req,
        gint32 iRet = -ECANCELED );

    virtual gint32 CancelFsRequests(
        gint32 iRet = -ECANCELED );

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
        fuse_file_info* fi,
        struct stat *stbuf) override;

    gint32 fs_ioctl(
        const char *path,
        fuse_file_info *fi,
        unsigned int cmd,
        void *arg,
        unsigned int flags,
        void *data ) override;

    gint32 fs_open(
        const char *path,
        fuse_file_info *fi ) override;

    gint32 fs_release(
        const char* path,
        fuse_file_info * fi ) override
    {
        DecOpCount();
        return 0;
    }

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

    gint32 fs_read(
        const char* path,
        fuse_file_info *fi,
        fuse_req_t req,
        fuse_bufvec*& buf,
        off_t off,
        size_t size,
        std::vector< BufPtr >& vecBackup,
        fuseif_intr_data* d ) override;

    gint32 fs_getattr(
        const char *path,
        fuse_file_info* fi,
        struct stat *stbuf) override;

    gint32 fs_open(
        const char *path,
        fuse_file_info *fi ) override;
};

class CFuseEvtFile : public CFuseFileEntry
{
    guint32 m_dwGrpId = 0;

    gint32 do_remove( bool bSched );

    public:
    typedef CFuseFileEntry super;
    CFuseEvtFile(
        const stdstr& strName,
        CRpcServices* pIf ) :
            super( strName, pIf )
    { SetClassId( clsid( CFuseEvtFile ) ); }

    inline void SetGroupId( guint32 dwGrpId )
    { m_dwGrpId = dwGrpId; }

    inline guint32 GetGroupId() const
    { return m_dwGrpId; }

    gint32 fs_read(
        const char* path,
        fuse_file_info *fi,
        fuse_req_t req,
        fuse_bufvec*& buf,
        off_t off,
        size_t size,
        std::vector< BufPtr >& vecBackup,
        fuseif_intr_data* d ) override;

    gint32 ReceiveEvtJson(
        const stdstr& strMsg );

    virtual gint32 ReceiveMsgJson(
        const stdstr& strMsg,
        guint64 qwReqId )
    { return -EACCES; };

    using BUFARR=std::vector< BufPtr >;
    gint32 GetReqSize(
        BufPtr& pReqSize,
        BUFARR& vecBufs );

    gint32 fs_poll(
        const char *path,
        fuse_file_info *fi,
        fuse_pollhandle *ph,
        unsigned *reventsp ) override;

    gint32 fs_release(
        const char* path,
        fuse_file_info * fi ) override;

    gint32 fs_unlink(
        const char* path,
        fuse_file_info *fi,
        bool bSched ) override;
};

class CFuseRespFileSvr : public CFuseEvtFile
{
    protected:

    std::vector< BufPtr > m_vecOutBufs;
    BufPtr  m_pReqSize;

    std::unique_ptr< Json::CharReader > m_pReader;

    public:

    typedef CFuseEvtFile super;
    CFuseRespFileSvr(
        const stdstr& strName,
        CRpcServices* pIf ) :
            super( strName, pIf ),
            m_pReqSize( true )
    {
        SetClassId( clsid( CFuseRespFileSvr ) );
        Json::CharReaderBuilder oBuilder;
        m_pReader.reset( oBuilder.newCharReader() );
    }

    gint32 fs_read(
        const char* path,
        fuse_file_info *fi,
        fuse_req_t req,
        fuse_bufvec*& buf,
        off_t off,
        size_t size,
        std::vector< BufPtr >& vecBackup,
        fuseif_intr_data* d ) override
    { return 0; }

    gint32 fs_write_buf(
        const char* path,
        fuse_file_info *fi,
        fuse_req_t req,
        fuse_bufvec *buf,
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
        const char* path,
        fuse_file_info *fi,
        fuse_req_t req,
        fuse_bufvec *buf,
        fuseif_intr_data* d ) override
    { return -ENOSYS;}

    gint32 fs_read(
        const char* path,
        fuse_file_info *fi,
        fuse_req_t req,
        fuse_bufvec*& buf,
        off_t off,
        size_t size,
        std::vector< BufPtr >& vecBackup,
        fuseif_intr_data* d ) override
    {
        return CFuseEvtFile::fs_read(
            nullptr, fi, req, buf,
            off, size, vecBackup, d ); 
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

    // receiving the request's response
    gint32 ReceiveMsgJson(
        const stdstr& strMsg,
        guint64 qwReqId );

    gint32 CancelFsRequests(
        gint32 iRet = -ECANCELED ) override;
};

class CFuseReqFileProxy :
    public CFuseRespFileProxy
{
    public:

    std::unique_ptr< Json::CharReader > m_pReader;

    typedef CFuseRespFileProxy super;

    CFuseReqFileProxy(
        const stdstr& strName,
        CRpcServices* pIf ) :
            super( strName, pIf )
    {
        SetClassId( clsid( CFuseReqFileProxy ) );
        Json::CharReaderBuilder oBuilder;
        m_pReader.reset( oBuilder.newCharReader() );
    }

    gint32 fs_read(
        const char* path,
        fuse_file_info *fi,
        fuse_req_t req,
        fuse_bufvec*& buf,
        off_t off,
        size_t size,
        std::vector< BufPtr >& vecBackup,
        fuseif_intr_data* d ) override;

    gint32 fs_write_buf(
        const char* path,
        fuse_file_info *fi,
        fuse_req_t req,
        fuse_bufvec *buf,
        fuseif_intr_data* d ) override;

    gint32 fs_ioctl(
        const char *path,
        fuse_file_info *fi,
        unsigned int cmd,
        void *arg,
        unsigned int flags,
        void *data ) override;

    gint32 fs_poll(const char *path,
        fuse_file_info *fi,
        fuse_pollhandle *ph,
        unsigned *reventsp ) override;
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
        SetMode( S_IRUSR | S_IWUSR );
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

    gint32 fs_read(
        const char* path,
        fuse_file_info *fi,
        fuse_req_t req,
        fuse_bufvec*& buf,
        off_t off,
        size_t size,
        std::vector< BufPtr >& vecBackup,
        fuseif_intr_data* d ) override;

    gint32 fs_write_buf(
        const char* path,
        fuse_file_info *fi,
        fuse_req_t req,
        fuse_bufvec *buf,
        fuseif_intr_data* d ) override;

    gint32 fs_unlink(
        const char* path,
        fuse_file_info *fi,
        bool bSched ) override;

    gint32 fs_ioctl(
        const char *path,
        fuse_file_info *fi,
        unsigned int cmd,
        void *arg, 
        unsigned int flags,
        void *data ) override;
    void NotifyPoll();

    gint32 fs_release(
        const char* path,
        fuse_file_info * fi ) override;

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
        auto pDir = DIR_SPTR(
            new CFuseDirectory( strHopDir, nullptr ) ); 
        CFuseObjBase* pObj = dynamic_cast
            < CFuseObjBase* >( pDir.get() );
        pObj->SetMode( S_IRUSR | S_IXUSR );
        pObj->DecRef();
        AddChild( pDir );

        // add an RO file for connection parameters
        stdstr strParams = CONN_PARAM_FILE;
        auto pFile = DIR_SPTR(
            new CFuseTextFile( strParams ) ); 
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
        const DIR_SPTR& pEnt )
    {
        auto pDir = static_cast
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

                    DIR_SPTR pEnt1( pNewDir );
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

                    DIR_SPTR pEnt1( pNewDir );
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
#define GET_STMDESC_LOCKED( _hStream, _pCtx ) \
    CFuseServicePoint* _pSp = const_cast \
        < CFuseServicePoint* >( this );\
    CRpcServices* _pSvc = _pSp;\
    if( this->IsServer() ) \
    {\
        CStreamServerSync *pStm =\
            dynamic_cast< CStreamServerSync*>( _pSvc );\
        ret = pStm->GetDataDesc( _hStream, _pCtx );\
        if( ERROR( ret ) )\
            break;\
    }\
    else\
    {\
        CStreamProxySync *pStm = \
            dynamic_cast< CStreamProxySync*>( _pSvc );\
        ret = pStm->GetDataDesc( _hStream, _pCtx ); \
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
    DIR_SPTR _pDir = GetSvcDir(); \
    RLOCK_TESTMNT0( _pDir.get() )

class CFuseSvcServer;
class CFuseSvcProxy;

CFuseSvcDir* GetSvcDir( const char* _path );
CFuseSvcDir* GetSvcDir( CRpcServices* pIf );
gint32 GetSvcDir( const char* path,
    CFuseObjBase*& pSvcDir,
    std::vector< stdstr >& v );

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

#define REQFILE( _bProxy ) std::conditional< \
    _bProxy, CFuseReqFileProxy, CFuseReqFileSvr>::type

#define RESPFILE( _bProxy ) std::conditional< \
    _bProxy, CFuseRespFileProxy, CFuseRespFileSvr>::type

template< bool bProxy >
class CFuseServicePoint :
    public virtual IFBASE( bProxy ) 
{
    public:
    template< bool bProxy_ >
    struct GRP_ELEM_T
    {
        typename REQFILE( bProxy_ )* m_pReqFile;
        typename RESPFILE( bProxy_ )* m_pRespFile;
        CFuseEvtFile* m_pEvtFile;
    };

    using GRP_ELEM=GRP_ELEM_T< bProxy >;

    private:
    stdstr m_strSvcPath;
    bool    m_bUnmounted = false;
    std::hashmap< stdstr, int > m_mapSessCount;

    protected:
    DIR_SPTR m_pSvcDir;
    std::atomic< guint32 > m_dwGrpIdx;
    std::hashmap< guint32, GRP_ELEM > m_mapGroups;

    public:
    typedef typename IFBASE( bProxy ) _MyVirtBase;
    typedef typename IFBASE( bProxy ) super;
    CFuseServicePoint( const IConfigDb* pCfg ) :
        super( pCfg ), m_dwGrpIdx( 1 )
    {}

    using super::GetIid;
    const EnumClsid GetIid() const override
    { return iid( CFuseServicePoint ); }

    using super::OnPostStart;
    gint32 OnPostStart( IEventSink* pCallback ) override
    {
        gint32 ret = BuildDirectories();
        return ret; 
    }

    inline DIR_SPTR GetSvcDir() const
    { return m_pSvcDir; }

    inline guint32 GetGroupId()
    { return m_dwGrpIdx++; }

    bool IsUnmounted() const
    { return m_bUnmounted; }

    inline gint32 GetGroup( guint32 dwGrpId,
        GRP_ELEM& oElem ) const
    {
        CStdRMutex oLock( this->GetLock() );
        auto itr = m_mapGroups.find( dwGrpId );
        if( itr == m_mapGroups.end() )
            return -ENOENT;
        oElem = itr->second;
        return STATUS_SUCCESS;
    }

    virtual gint32 AddGroup( guint32 dwGrpId,
        const GRP_ELEM& oElem )
    {
        CStdRMutex oLock( this->GetLock() );
        auto itr = m_mapGroups.find( dwGrpId );
        if( itr != m_mapGroups.end() )
            return -EEXIST;
        m_mapGroups[ dwGrpId ] = oElem;
        return STATUS_SUCCESS;
    }

    virtual gint32 RemoveGroup( guint32 dwGrpId )
    {
        CStdRMutex oLock( this->GetLock() );
        auto itr = m_mapGroups.find( dwGrpId );
        if( itr == m_mapGroups.end() )
            return -ENOENT;
        m_mapGroups.erase( itr );
        return STATUS_SUCCESS;
    }

    virtual void AddReqFiles(
        const stdstr& strSuffix ) = 0;

    // build the directory hierarchy for this service
    gint32 BuildDirectories()
    {
        gint32 ret = 0;
        do{
            stdstr strObjPath;
            CCfgOpenerObj oIfCfg( this );

            if( m_pSvcDir.get() != nullptr )
            {
                // restart in process
                break;
            }

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

            CFuseSvcDir* pSvcDir = 
                new CFuseSvcDir( strName, this );
            m_pSvcDir = DIR_SPTR( pSvcDir ); 
            m_pSvcDir->DecRef();
            pSvcDir->SetMode( S_IRWXU );

            AddReqFiles( "0" );

            // add an RW directory for streams
            auto pStmDir =
                new CFuseStmDir( this );
            auto pDir = DIR_SPTR( pStmDir ); 
            m_pSvcDir->AddChild( pDir );
            pStmDir->DecRef();
            pStmDir->SetMode( S_IRWXU );

            // add an RO stmevt file for stream events
            auto pStmEvt = 
                new CFuseStmEvtFile( this );
            auto pFile = DIR_SPTR( pStmEvt ); 
            pDir->AddChild( pFile );
            pStmEvt->DecRef();
            pStmEvt->SetMode( S_IRUSR );

        }while( 0 );

        return ret;
    }

    CFuseObjBase* GetEntryLocked(
        const std::vector< stdstr > vecSubdirs )
    {
        // must get locked before calling this method
        CFuseObjBase* pRet = nullptr;
        CDirEntry* pCur = GetSvcDir().get();
        if( vecSubdirs.empty() ) 
        {
            pRet = dynamic_cast
                < CFuseObjBase* >( pCur );
            return pRet;
        }

        for( auto& elem : vecSubdirs )
        {
            CDirEntry* pEnt =
                pCur->GetChild( elem );
            pRet = dynamic_cast
                < CFuseObjBase* >( pEnt );
            if( pRet == nullptr )
                break;
            pCur = pRet;
        }
        return pRet;
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
            GET_STMDESC_LOCKED( hStream, pCtx );
            ret = oCfg.GetStrProp(
                propNodeName, strName );
            
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

    gint32 CreateStmFile( HANDLE hStream )
    {
        gint32 ret = 0;        
        do{
            stdstr strSvcPath;
            ret = GetSvcPath( strSvcPath );
            if( ERROR( ret ) )
                break;
            DIR_SPTR pSp = GetSvcDir();
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
                DIR_SPTR( new CFuseStmFile( 
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
            DIR_SPTR pSp = GetSvcDir();
            CFuseStmDir* pDir =
                static_cast< CFuseStmDir* >
                ( pSp->GetChild( STREAM_DIR ) );

            DIR_SPTR pChild;
            ret = pDir->GetChild(
                    strName, pChild );
            if( ERROR( ret ) )
                break;
            CFuseStmFile* pFile =
                static_cast< CFuseStmFile* >
                ( pChild.get() );

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
            pFile->SetRemoved();

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
            std::vector< DIR_SPTR > vecChildren;

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

            ObjPtr pIf = this;
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

    gint32 IncStmCount( const stdstr& strSess )
    {
        auto itr = m_mapSessCount.find( strSess );

        if( itr == m_mapSessCount.end() )
        {
            m_mapSessCount[ strSess ] = 1;
        }
        else
        {
            if( itr->second >= MAX_STREAMS_PER_SESS )
            {
                return -EMFILE;
            }
            ++itr->second;
            DebugPrint( 0, "Session %s opened %d streams",
                strSess.c_str(), itr->second );
        }
        return STATUS_SUCCESS;
    }

    gint32 DecStmCount( const stdstr& strSess )
    {
        auto itr = m_mapSessCount.find( strSess );
        if( itr == m_mapSessCount.end() )
        {
            return -ENOENT;
        }
        else
        {
            if( itr->second <= 0 )
            {
                return -ERANGE;
            }
            --itr->second;
            DebugPrint( 0,
                "Session %s closed a stream",
                strSess.c_str() );
        }
        return STATUS_SUCCESS;
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

        while( ERROR( ret ) )
        {
            gint32 iRet = 0;
            CfgPtr pDesc;
            IConfigDb* ptctx = nullptr;
            CStdRMutex oLock( this->GetLock() );
            GET_STMDESC_LOCKED( hStream, pDesc );
            iRet = oCfg.GetPointer(
                propTransCtx, ptctx );
            if( ERROR( iRet ) )
                break;

            stdstr strSess;
            CCfgOpener oCtx( ptctx ); 
            iRet = oCtx.GetStrProp(
                propSessHash, strSess );
            if( ERROR( iRet ) )
                break;
            DecStmCount( strSess );
            break;
        }

        return ret;
    }

    inline gint32 OnStmClosingFuse(
        HANDLE hStream )
    {
        gint32 ret = 0;
        do{
            CfgPtr pDesc;
            IConfigDb* ptctx = nullptr;
            CStdRMutex oLock( this->GetLock() );
            GET_STMDESC_LOCKED( hStream, pDesc );
            ret = oCfg.GetPointer(
                propTransCtx, ptctx );
            if( SUCCEEDED( ret ) )
            {
                stdstr strSess;
                CCfgOpener oCtx( ptctx ); 
                ret = oCtx.GetStrProp(
                    propSessHash, strSess );
                if( SUCCEEDED( ret ) )
                    DecStmCount( strSess );
            }

            WLOCK_TESTMNT;
            stdstr strName;
            ret = StreamToName( hStream, strName);
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
                ret = -EMFILE;
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

            IConfigDb* ptctx = nullptr;
            CStdRMutex oLock( this->GetLock() );
            ret = oDesc.GetPointer(
                propTransCtx, ptctx );
            if( ERROR( ret ) )
                break;

            stdstr strSess;
            CCfgOpener oCtx( ptctx ); 
            ret = oCtx.GetStrProp(
                propSessHash, strSess );
            if( ERROR( ret ) )
                break;
            ret = IncStmCount( strSess );

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

                TaskletPtr pTask;
                CIoManager* pMgr = this->GetIoMgr();
                bool bRestart = true;
                pMgr->GetCmdLineOpt(
                    propConnRecover, bRestart );

                TaskletPtr pStopTask;
                ret = DEFER_IFCALLEX_NOSCHED2(
                    0, pStopTask, this,
                    &CRpcServices::StopEx, 
                    ( IEventSink* )nullptr );
                if( ERROR( ret ) )
                    break;

                if( bRestart )
                {
                    ObjVecPtr pvecMatches( true );
                    for( auto& elem : this->m_vecMatches )
                    {
                        ObjPtr pObj( elem );
                        ( *pvecMatches )().push_back( pObj );
                    }
                    ObjPtr pMatches = pvecMatches;

                    TaskGrpPtr pGrp;
                    CParamList oParams;
                    oParams[ propIfPtr ] = ObjPtr( this );
                    ret = pGrp.NewObj(
                        clsid( CIfTaskGroup ),
                        oParams.GetCfg() );

                    TaskletPtr pStartTask;
                    ret = DEFER_IFCALLEX_NOSCHED2(
                        0, pStartTask, this,
                        &CRpcServices::StartEx, 
                        ( IEventSink* )nullptr );
                    if( ERROR( ret ) )
                        break;

                    CCfgOpener oTaskCfg( ( IConfigDb* )
                        pStartTask->GetConfig() );

                    oTaskCfg.SetObjPtr(
                        propMatchMap, pMatches );

                    pGrp->SetRelation( logicAND );
                    pGrp->AppendTask( pStopTask );
                    pGrp->AppendTask( pStartTask );
                    pTask = pGrp;
                }
                else
                {
                    pTask = pStopTask;
                }

                ret = pMgr->RescheduleTask( pTask );
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

    gint32 StartEx( IEventSink* pCallback )
    {
        gint32 ret = 0;
        do{
            if( m_pSvcDir.get() == nullptr )
                ret = super::StartEx( pCallback );
            else
            {
                WLOCK_TESTMNT;
                CCfgOpenerObj oTaskCfg( pCallback );
                ObjPtr pMatches;
                ret = oTaskCfg.GetObjPtr(
                    propMatchMap, pMatches );
                if( SUCCEEDED( ret ) )
                {
                    ObjVecPtr pvecMatches = pMatches;
                    this->m_vecMatches.clear();
                    for( auto& elem : (*pvecMatches)() )
                    {
                        MatchPtr pMatch = elem;
                        this->m_vecMatches.push_back( pMatch );
                    }
                }
                ret = super::StartEx( pCallback );
            }
        }while( 0 );
        return ret;
    }

    gint32 StopEx( IEventSink* pCallback )
    {
        gint32 ret = 0;
        do{
            if( m_pSvcDir.get() == nullptr )
                ret = super::StopEx( pCallback );
            else 
            {
                bool bUnmounted = true;
                do{
                    WLOCK_TESTMNT;
                    bUnmounted = false;
                    ret = super::StopEx( pCallback );
                }while( 0 );
                if( bUnmounted )
                    ret = super::StopEx( pCallback );
            }
        }while( 0 );
        return ret;
    }

    gint32 OnStmRecvFuse(
        HANDLE hChannel, BufPtr& pBuf )
    {
        gint32 ret = 0;
        do{
            if( pBuf->GetDataType()!=DataTypeMem )
                break;
            RLOCK_TESTMNT2( this );
            auto pDir = static_cast< CFuseStmDir* >
            ( _pSvcDir->GetChild( STREAM_DIR ) );

            stdstr strName;
            ret = StreamToName( hChannel, strName );
            if( ERROR( ret ) )
                break;

            auto pObj = pDir->GetChild( strName );
            if( pObj == nullptr )
                break;

            auto pStmFile = static_cast
                < CFuseStmFile* >( pObj );
            pStmFile->NotifyPoll();

        }while( 0 );

        return ret;
    }

    gint32 DoRmtModEventFuse(
        EnumEventId iEvent,
        const std::string& strModule,
        IConfigDb* pEvtCtx )
    {
        gint32 ret = 0;
        do{
            if( iEvent != eventRmtModOffline )
                break;
            
            RLOCK_TESTMNT2( this );
            auto pChild = _pSvcDir->GetChild(
                JSON_REQ_FILE );

            auto pReq = dynamic_cast
                < CFuseFileEntry* >( pChild );
            pReq->CancelFsRequests( -EIO );

            pChild = _pSvcDir->GetChild(
                JSON_RESP_FILE );
            auto pResp = dynamic_cast
                < CFuseFileEntry* >( pChild );
            pResp->CancelFsRequests( -EIO );

            pChild = _pSvcDir->GetChild(
                JSON_EVT_FILE );
            auto pEvt = dynamic_cast
                < CFuseFileEntry* >( pChild );
            pEvt->CancelFsRequests( -EIO );

        }while( 0 );

        return ret;
    }
};

class CFuseSvcProxy :
    public CFuseServicePoint< true >
{
    std::hashmap< guint64, guint32 > m_mapReq2Grp;
    std::multimap< guint32, guint64 > m_mapGrp2Req;

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
        const stdstr& strMsg );

    // to receive a request on server side or a
    // response or event on the proxy side
    gint32 ReceiveMsgJson(
            const stdstr& strMsg,
            guint64 qwReqId );

    inline gint32 GetGrpByReq( guint64 qwReqId,
        guint32& dwGrpId ) const
    {
        CStdRMutex oLock( GetLock() );
        auto itr = m_mapReq2Grp.find( qwReqId );
        if( itr == m_mapReq2Grp.end() )
            return -ENOENT;
        dwGrpId = itr->second;
        return STATUS_SUCCESS;
    }

    inline gint32 AddReqGrp( guint64 qwReqId,
        guint32 dwGrpId )
    {
        CStdRMutex oLock( GetLock() );
        auto itr = m_mapReq2Grp.find( qwReqId );
        if( itr != m_mapReq2Grp.end() )
            return -EEXIST;
        m_mapReq2Grp[ qwReqId ]= dwGrpId;
        m_mapGrp2Req.insert( { dwGrpId, qwReqId } );
        return STATUS_SUCCESS;
    }

    inline gint32 RemoveReqGrp( guint64 qwReqId )
    {
        CStdRMutex oLock( GetLock() );
        auto itr = m_mapReq2Grp.find( qwReqId );
        if( itr == m_mapReq2Grp.end() )
            return -ENOENT;
        guint32 dwGrpId = itr->second;
        m_mapReq2Grp.erase( itr );

        auto itr1 =
            m_mapGrp2Req.lower_bound( dwGrpId );

        auto upb =
            m_mapGrp2Req.upper_bound( dwGrpId );
        for( ; itr1 != upb; ++itr1 )
        {
            if( itr1->second != qwReqId )
                continue;
            m_mapGrp2Req.erase( itr1 );
            break;
        }
        return STATUS_SUCCESS;
    }

    inline gint32 RemoveGrpReqs( guint32 dwGrp,
        std::vector< guint64 >& vecReqIds )
    {
        CStdRMutex oLock( GetLock() );
        auto range = m_mapGrp2Req.equal_range( dwGrp );
        if( std::distance(
            range.first, range.second ) == 0 )
            return -ENOENT;
        while( range.first != range.second )
        {
            vecReqIds.push_back(
                range.first->second );
            range.first++;
        }
        m_mapGrp2Req.erase( dwGrp );
        return STATUS_SUCCESS;
    }

    inline gint32 RemoveGrpReqs( guint32 dwGrp )
    {
        CStdRMutex oLock( GetLock() );
        auto itr = m_mapGrp2Req.find( dwGrp );
        if( itr == m_mapGrp2Req.end() )
            return -ENOENT;
        m_mapGrp2Req.erase( dwGrp );
        return STATUS_SUCCESS;
    }

    void AddReqFiles(
        const stdstr& strSuffix ) override;
};

class CFuseSvcServer :
    public CFuseServicePoint< false >
{
    std::vector< guint32 > m_vecGrpIds;
    gint32 m_iLastGrp = 0;

    public:
    typedef CFuseServicePoint< false > super;
    CFuseSvcServer( const IConfigDb* pCfg ):
    _MyVirtBase( pCfg ),
    super( pCfg )
    {}

    //Response&Event Dispatcher
    virtual gint32 DispatchMsg(
        const Json::Value& valMsg ) = 0;

    // to receive a request on server side or a
    // response or event on the proxy side
    gint32 ReceiveMsgJson(
            const stdstr& strMsg,
            guint64 qwReqId );

    gint32 AddGroup( guint32 dwGrpId,
        const GRP_ELEM& oElem ) override
    {
        CStdRMutex oLock( GetLock() );
        gint32 ret = super::AddGroup(
            dwGrpId, oElem );
        if( ERROR( ret ) )
            return ret;

        m_vecGrpIds.push_back( dwGrpId );
        return STATUS_SUCCESS;
    }

    virtual gint32 RemoveGroup(
        guint32 dwGrpId ) override
    {
        CStdRMutex oLock( GetLock() );
        gint32 ret = super::RemoveGroup(
            dwGrpId );
        if( ERROR( ret ) )
            return ret;

        bool bFound = false;
        auto itr = m_vecGrpIds.begin();
        while( itr != m_vecGrpIds.end() )
        {
            if( *itr == dwGrpId )
            {
                bFound = true;
                m_vecGrpIds.erase( itr );
                break;
            }
            ++itr;
        }
        if( bFound )
            return STATUS_SUCCESS;

        return -ENOENT;
    }

    void AddReqFiles(
        const stdstr& strSuffix ) override;

};

struct FHCTX
{
    CRpcServices* pSvc;
    DIR_SPTR pFile;
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
    DIR_SPTR m_pRootDir;
    std::vector< ObjPtr > m_vecIfs;
    CIoManager* m_pMgr = nullptr;

    using FHMAP =std::hashmap< HANDLE, FHCTX  >;
    FHMAP   m_mapHandles;

    public:
    typedef T super;

    using SVC_TYPE=typename std::conditional<
        std::is_base_of< CInterfaceProxy, T >::value,
        CFuseSvcProxy, CFuseSvcServer > ::type;

    inline CFuseRootDir* GetRootDir() const
    {
        return static_cast< CFuseRootDir* >
            ( m_pRootDir.get() );
    }

    gint32 Add2FhMap( HANDLE fh, FHCTX ctx )
    {
        CStdRMutex oIfLock( this->GetLock() );
        if( m_mapHandles.find( fh ) !=
            m_mapHandles.end() )
            return -EEXIST;
        m_mapHandles[ fh ] = ctx;
        return STATUS_SUCCESS;
    }

    gint32 RemoveFromFhMap( HANDLE fh )
    {
        CStdRMutex oIfLock( this->GetLock() );
        auto itr = m_mapHandles.find( fh );
        if( itr == m_mapHandles.end() )
            return -ENOENT;
        m_mapHandles.erase( itr );
        return STATUS_SUCCESS;
    }

    gint32 GetFhCtx( HANDLE fh, FHCTX& fhctx )
    {
        CStdRMutex oIfLock( this->GetLock() );
        auto itr = m_mapHandles.find( fh );
        if( itr == m_mapHandles.end() )
            return -ENOENT;
        fhctx = itr->second;
        return STATUS_SUCCESS;
    }

    gint32 GetSvcDir(
        const stdstr& strDir,
        CFuseObjBase*& pDir,
        std::vector< stdstr >& vecSubdirs )
    {
        gint32 ret = 0;
        std::vector<stdstr> vecComp;
        if( strDir.size() > REG_MAX_PATH )
            return -EINVAL;

        ret = CRegistry::Namei( strDir, vecComp );
        if( ERROR( ret ) )
            return ret;

        auto itr = vecComp.begin();
        CDirEntry* pCurDir = this->GetRootDir();
        ret = ENOENT;
        while( itr != vecComp.end() )
        {
            if( *itr == "." )
            {
                itr++;
                continue;
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
        {
            pDir = static_cast< CFuseSvcDir* >
                ( pCurDir );
            if( itr + 1 != vecComp.end() )
                vecSubdirs.insert( vecSubdirs.begin(),
                    ( itr + 1 ), vecComp.end() );
        }
        else if( ret == ENOENT &&
            pCurDir != nullptr )
        {
            pDir = static_cast< CFuseSvcDir* >
                ( pCurDir );
        }
        else if( ret == ENOENT )
        {
            ret = -ENOENT;
        }
        return ret;
    }

    gint32 GetSvcDir(
        const stdstr& strDir,
        CFuseSvcDir*& pDir )
    {
        std::vector< stdstr > vecSubdirs;
        CFuseObjBase* pDir1;
        gint32 ret = GetSvcDir(
            strDir, pDir1, vecSubdirs );
        if( SUCCEEDED( ret ) )
            pDir = static_cast
                < CFuseSvcDir* >( pDir1 );
        if( ret = ENOENT )
            ret = -ENOENT;
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
        m_pRootDir = DIR_SPTR( new CFuseRootDir() );

        CFuseObjBase* pObj = GetRootDir();
        FHCTX fc = { nullptr, m_pRootDir };
        Add2FhMap( ( HANDLE )pObj, fc );
    }

    fuse* GetFuse() const
    { return m_pFuse; }

    inline void SetFuse( fuse* pFuse )
    { m_pFuse = pFuse; }

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
            DIR_SPTR pSvcDir = pSp->GetSvcDir();
            CFuseObjBase* pRoot = GetRootDir();

            if( !this->IsServer() )
            {
                std::vector< DIR_SPTR > vecConns;
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
                DIR_SPTR pNewDir( new CFuseConnDir(
                    stdstr( CONN_DIR_PREFIX ) +
                    std::to_string( idx ) ) );
                CFuseConnDir* pConn = static_cast
                    < CFuseConnDir* >( pNewDir.get() );
                pConn->DecRef();
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
                    &CRpcServices::StopEx, 
                    ( IEventSink* )nullptr );

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

class CStreamServerFuse :
    public CStreamServerAsync
{
    public:
    typedef CStreamServerAsync super;

    CStreamServerFuse( const IConfigDb* pCfg ) :
        _MyVirtBase( pCfg ), super( pCfg )
    {}

    gint32 OnStmRecv( HANDLE hChannel,
        BufPtr& pBuf ) override;
};

class CStreamProxyFuse :
    public CStreamProxyAsync
{
    public:
    typedef CStreamProxyAsync super;

    CStreamProxyFuse( const IConfigDb* pCfg ) :
        _MyVirtBase( pCfg ), super( pCfg )
    {}

    gint32 OnStmRecv( HANDLE hChannel,
        BufPtr& pBuf ) override;
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
