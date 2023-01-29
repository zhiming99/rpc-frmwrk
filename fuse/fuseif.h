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
#if BUILD_64
#define FUSE_USE_VERSION 35
#else
#if ARM
#define FUSE_USE_VERSION 34
#else
#define FUSE_USE_VERSION 34
#endif
#define _FILE_OFFSET_BITS 64
#endif

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
#include "tractgrp.h"

struct fuseif_intr_data;
gint32 fuseif_remkdir( const stdstr&, guint32 );

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

const char* IfStateToString(
    EnumIfState dwState );

#define CFuseMutex  CStdRMutex

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
    mutable stdrmutex m_oLock;

    protected:
    CRpcServices* m_pIf = nullptr;

    // an object to hold user data
    ObjPtr m_pUserObj;

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

    stdrmutex& GetLock() const
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

    void SetPollHandle(
        fuse_pollhandle* pollHandle );

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

    inline ObjPtr GetUserObj() const
    { return m_pUserObj; }

    inline void SetUserObj( ObjPtr& pObj )
    { m_pUserObj = pObj; }

    virtual int fs_mkdir( const char *,
        fuse_file_info*,
        mode_t )
    { return -ENOSYS; }

    virtual int fs_rmdir(
        const char *,
        fuse_file_info*,
        bool bReload )
    { return -ENOSYS; }

    virtual gint32 fs_lseek(
        const char* path,
        fuse_file_info* fi,
        off_t off, int whence,
        off_t& offr )
    { return -ENOSYS; }

    protected:
    gint32 CopyFromPipe(
        BufPtr& pBuf, fuse_buf* src );

    // don't clear vecBuf, which may have contents in
    // advance
    gint32 CopyFromBufVec(
         std::vector< BufPtr >& vecBuf,
         fuse_bufvec* bufvec );
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

    gint32 fs_mkdir( const char *,
        fuse_file_info*,
        mode_t ) override;

    gint32 fs_rmdir(
        const char *,
        fuse_file_info*,
        bool bReload ) override;

    gint32 FindSvcDir(
        const stdstr&, DIR_SPTR& pObj );
};

class CFuseRootDir : public CFuseDirectory
{
    mutable CSharedLock m_oRootLock;
    public:
    typedef CFuseDirectory super;

    CFuseRootDir( CRpcServices* pIf );
    inline CSharedLock& GetRootLock() const
    { return m_oRootLock; }

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

    guint32 GetBytesOneMsg() const
    { 
        if( m_queIncoming.empty() )
            return 0;

        auto elem = m_queIncoming.front();
        guint32 dwSize = 0;
        if( elem.pBuf->size() - elem.dwOff < 4 )
            return 0;

        char* src = elem.pBuf->ptr() + elem.dwOff; 
        char* dst = ( char* )&dwSize;
        dst[ 0 ] = src[0];
        dst[ 1 ] = src[1];
        dst[ 2 ] = src[2];
        dst[ 3 ] = src[3];
        return ntohl( dwSize ) + sizeof( dwSize );
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
    protected:
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

    gint32 fs_unlink(
        const char* path,
        fuse_file_info *fi,
        bool bSched ) override
    { return -EACCES;}

};

class CFuseEvtFile : public CFuseFileEntry
{
    guint32 m_dwGrpId = 0;
    guint32 m_dwMsgRead = 0;
    guint32 m_dwLastOff = 0;
    guint32 m_dwBytesAvail = 0;

    gint32 do_remove( bool bSched );

    protected:
#ifdef DEBUG
    stdstr m_strLastMsg;
#endif
    guint32 m_dwMsgCount = 0;

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

    gint32 fs_open(
        const char* path,
        fuse_file_info *fi );
};

class CFuseRespFileSvr : public CFuseEvtFile
{
    protected:

    std::vector< BufPtr > m_vecOutBufs;
    BufPtr  m_pReqSize;

    std::unique_ptr< Json::CharReader > m_pReader;
    std::set< guint64 > m_setTaskIds;

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

    inline void AddTaskId( guint64 qwTaskId )
    {
        CFuseMutex oLock( GetLock() );
        m_setTaskIds.insert( qwTaskId );
    }

    inline gint32 RemoveTaskId( guint64 qwTaskId )
    {
        CFuseMutex oLock( GetLock() );
        size_t count =
            m_setTaskIds.erase( qwTaskId );
        if( count > 0 )
            return STATUS_SUCCESS;
        return -ENOENT;
    }

    gint32 CancelFsRequests(
        gint32 iRet ) override;

    gint32 fs_release(
        const char* path,
        fuse_file_info * fi ) override;

    gint32 OnUserCancelRequest( guint64 qwReqId )
    { return RemoveTaskId( qwReqId ); }
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
        guint64 qwReqId );
};

class CFuseRespFileProxy : public CFuseEvtFile
{

    public:
    typedef CFuseEvtFile super;

    CFuseRespFileProxy(
        const stdstr& strName,
        CRpcServices* pIf ) :
        super( strName, pIf )
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
    protected:
    std::vector< BufPtr > m_vecOutBufs;
    BufPtr  m_pReqSize;

    public:

    std::unique_ptr< Json::CharReader > m_pReader;

    typedef CFuseRespFileProxy super;

    CFuseReqFileProxy(
        const stdstr& strName,
        CRpcServices* pIf ) :
            super( strName, pIf ),
            m_pReqSize( true )
    {
        SetClassId( clsid( CFuseReqFileProxy ) );
        Json::CharReaderBuilder oBuilder;
        m_pReader.reset( oBuilder.newCharReader() );
    }

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

    gint32 fs_release(
        const char* path,
        fuse_file_info * fi ) override;
};

class CFuseStmDir : public CFuseDirectory
{
    public:
    typedef CFuseDirectory super;
    CFuseStmDir( CRpcServices* pIf ) :
        super( STREAM_DIR, pIf )
    { SetClassId( clsid( CFuseStmDir ) ); }
};

class CFuseSvcStat : public CFuseTextFile
{
    public:
    typedef CFuseTextFile super;
    CFuseSvcStat( CRpcServices* pIf ) :
        super( SVCSTAT_FILE )
    { SetIf( pIf ); }

    virtual gint32 UpdateContent();
    gint32 fs_read(
        const char* path,
        fuse_file_info *fi,
        fuse_req_t req,
        fuse_bufvec*& bufvec,
        off_t off, size_t size,
        std::vector< BufPtr >& vecBackup,
        fuseif_intr_data* d );
};

class CFuseCmdFile : public CFuseObjBase
{
    protected:

    public:
    typedef CFuseObjBase super;
    CFuseCmdFile() : super()
    { SetName( COMMAND_FILE ); }

    gint32 fs_getattr(
        const char *path,
        fuse_file_info* fi,
        struct stat *stbuf) override;

    gint32 fs_open(
        const char *path,
        fuse_file_info *fi ) override;

    gint32 fs_release(
        const char* path,
        fuse_file_info * fi ) override;

    gint32 fs_write_buf(
        const char* path,
        fuse_file_info *fi,
        fuse_req_t req,
        fuse_bufvec *buf,
        fuseif_intr_data* d ) override;

    gint32 fs_read(
        const char* path,
        fuse_file_info *fi,
        fuse_req_t req,
        fuse_bufvec*& bufvec,
        off_t off, size_t size,
        std::vector< BufPtr >& vecBackup,
        fuseif_intr_data* d ) override;
};

class CFuseClassList : public CFuseSvcStat
{
    public:
    typedef CFuseSvcStat super;
    CFuseClassList( CRpcServices* pIf )
        : super( pIf )
    { SetName( CLSLIST_FILE ); }

    gint32 UpdateContent() override;
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

    gint32 SendBufVec( fuse_bufvec* bufvec );

    gint32 FillIncomingQue(
        std::vector<INBUF>& vecIncoming );
    sem_t m_semFlowCtrl;

    gint32 fs_read_blocking(
        const char* path,
        fuse_file_info *fi,
        fuse_req_t req, fuse_bufvec*& bufvec,
        off_t off, size_t size,
        std::vector< BufPtr >& vecBackup,
        fuseif_intr_data* d );

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

        // using STM_MAX_PACKETS_REPORT - 1 to avoid
        // sending out the flow-control-lifted
        // notification
        Sem_Init( &m_semFlowCtrl,
            0, STM_MAX_PACKETS_REPORT - 1 );
    }

    inline bool GetFlowCtrl() const
    { return m_bFlowCtrl; }

    inline void SetFlowCtrl( bool bFlowCtl )
    { m_bFlowCtrl = bFlowCtl; }

    inline HANDLE GetStream() const
    { return m_hStream; }

    void SetStream( HANDLE hStream )
    { m_hStream = hStream; }

    gint32 fs_open(
        const char *path,
        fuse_file_info *fi ) override;

    gint32 OnReadStreamComplete(
        HANDLE hStream,
        gint32 iRet,
        BufPtr& pBuf,
        IConfigDb* pCtx );

    gint32 StartNextRead( BufPtr& pRetBuf );
    gint32 OnCompleteReadReq();

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

    gint32 fs_lseek(
        const char* path,
        fuse_file_info* fi,
        off_t off, int whence,
        off_t& offr ) override;
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
    CFuseConnDir( const stdstr& strName );
    stdstr GetRouterPath( CDirEntry* pDir ) const;

    gint32 AddSvcDir( const DIR_SPTR& pEnt );
    void SetConnParams( const CfgPtr& pCfg );
    bool IsEqualConn( const IConfigDb* pConn ) const;
    stdstr DumpConnParams();
};

bool IsUnmounted( CRpcServices* pIf );

#define ROOTLK_SHARED \
    CFuseRootDir* _pRootDir = static_cast \
        < CFuseRootDir* >( GetRootDir() ); \
    if( _pRootDir == nullptr ) \
    { ret = -EFAULT; break; } \
    CReadLock _ortlk( _pRootDir->GetRootLock() ); \
    ret = _ortlk.GetStatus(); \
    if( ERROR( ret ) ) \
        break;

#define ROOTLK_EXCLUSIVE \
    CFuseRootDir* _pRootDir = static_cast \
        < CFuseRootDir* >( GetRootDir() ); \
    if( _pRootDir == nullptr ) \
    { ret = -EFAULT; break; } \
    CWriteLock _ortlk( _pRootDir->GetRootLock() ); \
    ret = _ortlk.GetStatus(); \
    if( ERROR( ret ) ) \
        break;

#define GET_STMDESC_LOCKED( _hStream, _pCtx ) \
    auto _pSvc = const_cast< \
        ValType( decltype( this ) )* >( this );\
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
    CCfgOpener oCfg( ( IConfigDb* )_pCtx ); \

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

#define WLOCK_TESTMNT \
    ROOTLK_SHARED; \
    auto _pDir = GetSvcDir(); \
    WLOCK_TESTMNT0( _pDir.get() )

#define RLOCK_TESTMNT \
    ROOTLK_SHARED; \
    auto _pDir = GetSvcDir(); \
    RLOCK_TESTMNT0( _pDir.get() )

#define RLOCK_TESTMNT2( _pIf ) \
    ROOTLK_SHARED; \
    auto _pDir = rpcf::GetSvcDir( _pIf ); \
    RLOCK_TESTMNT0( _pDir );

#define WLOCK_TESTMNT2( _pIf ) \
    ROOTLK_SHARED; \
    auto _pDir = rpcf::GetSvcDir( _pIf ); \
    WLOCK_TESTMNT0( _pDir );

class CFuseSvcServer;
class CFuseSvcProxy;

CFuseSvcDir* GetSvcDir( const char* _path );
CFuseSvcDir* GetSvcDir( CRpcServices* pIf );
gint32 GetSvcDir( const char* path,
    CFuseObjBase*& pSvcDir,
    std::vector< stdstr >& v );

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
    timespec m_tsStartTime;

    protected:
    DIR_SPTR m_pSvcDir;
    std::atomic< guint32 > m_dwGrpIdx;
    std::hashmap< guint32, GRP_ELEM > m_mapGroups;

    public:
    typedef typename IFBASE( bProxy ) _MyVirtBase;
    typedef typename IFBASE( bProxy ) super;
    CFuseServicePoint( const IConfigDb* pCfg ) :
        super( pCfg ), m_dwGrpIdx( 1 )
    {
        clock_gettime(
            CLOCK_REALTIME, &m_tsStartTime );
    }

    using super::GetIid;
    const EnumClsid GetIid() const override
    { return iid( CFuseServicePoint ); }

    using super::OnPostStart;
    gint32 OnPostStart( IEventSink* pCallback ) override
    { return STATUS_SUCCESS; }

    inline timespec GetStartTime() const
    { return m_tsStartTime; }

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

    gint32 GetGroupCount() const
    {
        CStdRMutex oLock( this->GetLock() );
        return m_mapGroups.size();
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

            // add an RW directory for this svc
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
            pStmDir->DecRef();
            pStmDir->SetMode( S_IRWXU );
            m_pSvcDir->AddChild( pDir );

            auto pStat=
                new CFuseSvcStat( this );
            auto psf = DIR_SPTR( pStat );
            pStat->DecRef();
            pStat->SetMode( S_IRUSR );
            m_pSvcDir->AddChild( psf );

        }while( 0 );

        return ret;
    }

    gint32 ClearSvcDir()
    {
        gint32 ret = 0;
        if( m_pSvcDir.get() == nullptr )
            return ret;

        do{
            std::vector< DIR_SPTR > vecChildren;
            m_pSvcDir->GetChildren( vecChildren );
            if( vecChildren.empty() )
                break;

            for( auto& elem : vecChildren )
            {
                stdstr strName = elem->GetName();
                m_pSvcDir->RemoveChild( strName );
            }

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

    gint32 StreamToName( HANDLE hStream,
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
            auto pDir = static_cast< CFuseStmDir* >
                ( pSp->GetChild( STREAM_DIR ) );

            CfgPtr pCfg;
            auto pStmIf = dynamic_cast
                < IStream* >( this );
            if( unlikely( pStmIf == nullptr ) )
            {
                ret = -EFAULT;
                break;
            }

            pStmIf->GetDataDesc( hStream, pCfg );
            if( pCfg.IsEmpty() )
            {
                ret = -EFAULT;
                break;
            }

            stdstr strName;
            CCfgOpener oDesc( ( IConfigDb* )pCfg );
            ret = oDesc.GetStrProp(
                propNodeName, strName );

            if( strName.size() > REG_MAX_NAME )
                strName.erase( REG_MAX_NAME );

            if( SUCCEEDED( ret ) &&
                pDir->GetChild( strName ) != nullptr )
                ret = -EEXIST;

            if( ERROR( ret ) )
                break;

            auto pStmFile = new CFuseStmFile(
                strName, hStream, this );
            pStmFile->SetMode( S_IRUSR | S_IWUSR );
            pStmFile->DecRef();

            ret = pDir->AddChild(
                DIR_SPTR( pStmFile ) );

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
        std::vector< HANDLE > vecStreams;
        ObjPtr pIf = this;
        do{
            WLOCK_TESTMNT;
            std::vector< DIR_SPTR > vecChildren;

            m_bUnmounted = true;

            CFuseStmDir* pDir =
                static_cast< CFuseStmDir* >
                ( _pSvcDir->GetChild( STREAM_DIR ) );

            ret = pDir->GetChildren( vecChildren );
            if( ERROR( ret ) )
                break;

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

            vecChildren.clear();
            ret = _pSvcDir->GetChildren(
                vecChildren );

            oSvcLock.Unlock();

            for( auto& elem : vecChildren )
            {
                auto pFile =
                dynamic_cast< CFuseFileEntry* >
                    ( elem.get() );
                if( pFile != nullptr )
                    pFile->CancelFsRequests( -ECANCELED );
                _pSvcDir->RemoveChild(
                        elem->GetName() );
            }

        }while( 0 );
        if( SUCCEEDED( ret ) )
        {
            for( auto& elem : vecStreams )
                CloseChannel( pIf, elem );
        }

        return ret;
    }

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

    gint32 OnDBusEvent(
        EnumEventId iEvent ) override
    {
        gint32 ret = 0;

        switch( iEvent )
        {
        case eventDBusOffline:
            {
                TaskletPtr pTask;
                CIoManager* pMgr = this->GetIoMgr();
                bool bRestart = true;
                pMgr->GetCmdLineOpt(
                    propConnRecover, bRestart );

                if( bRestart )
                {
                    RestartSvcPoint( nullptr );
                    break;
                }

                CStdRMutex oIfLock( this->GetLock() );

                gint32 ret =
                    this->SetStateOnEvent( iEvent );

                if( ERROR( ret ) )
                    break;

                if( this->GetState() == stateStopped )
                    break;

                oIfLock.Unlock();

                TaskletPtr pStopTask;
                ret = DEFER_IFCALLEX_NOSCHED2(
                    0, pStopTask, this,
                    &CRpcServices::StopEx, 
                    ( IEventSink* )nullptr );

                if( ERROR( ret ) )
                    break;

                InterfPtr pRootIf = GetRootIf();
                CRpcServices* pRootSvc = pRootIf;
                ret = pRootSvc->AddSeqTask(
                    pStopTask );

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
            {
                ret = super::StartEx( pCallback );
                break;
            }

            // restart
            WLOCK_TESTMNT;
            CCfgOpenerObj oTaskCfg( pCallback );
            ObjPtr pMatches;
            ret = oTaskCfg.GetObjPtr(
                propMatchMap, pMatches );
            if( ERROR( ret ) )
                break;
            ObjVecPtr pvecMatches = pMatches;
            auto& vecMatches = this->m_vecMatches;
            vecMatches.clear();
            for( auto& elem : (*pvecMatches)() )
            {
                MatchPtr pMatch = elem;
                if( pMatch == this->m_pIfMatch )
                    continue;
                vecMatches.push_back( pMatch );
            }
            ret = super::StartEx( pCallback );

        }while( 0 );

        return ret;
    }

    gint32 StopEx( IEventSink* pCallback )
    {
        if( m_pSvcDir.get() == nullptr )
            return super::StopEx( pCallback );

        gint32 ret = 0;
        bool bUnmounted = true;
        do{
            WLOCK_TESTMNT;
            bUnmounted = false;
            ret = super::StopEx( pCallback );
        }while( 0 );

        if( bUnmounted )
            ret = super::StopEx( pCallback );

        return ret;
    }

    gint32 OnStmRecvFuse(
        HANDLE hChannel, BufPtr& pBuf )
    {
        gint32 ret = 0;
        do{
            if( pBuf->GetDataType()!=DataTypeMem )
                break;
            RLOCK_TESTMNT;
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

    gint32 RestartSvcPoint( IEventSink* pCallback )
    {
        gint32 ret = 0;
        TaskGrpPtr pGrp;

        do{
            TaskletPtr pStopTask;

            CParamList oParams;
            oParams[ propIfPtr ] = ObjPtr( this );
            ret = pGrp.NewObj(
                clsid( CIfTaskGroup ),
                oParams.GetCfg() );

            pGrp->SetRelation( logicNONE );
            if( pCallback != nullptr )
                pGrp->SetClientNotify( pCallback );

            CStdRMutex oIfLock( this->GetLock() );
            EnumIfState iState = this->GetState();

            if( iState == stateStarting ||
                iState == stateStopping )
            {
                ret = ERROR_STATE;
                DebugPrintEx( logErr, ret,
                "the svc point is already"
                " undergoing starting "
                "or stopping. Please standby." );
                break;
            }

            ObjVecPtr pvecMatches( true );
            if( iState != stateStopped )
            {
                if( iState == stateStartFailed )
                    ret = this->SetStateOnEvent( cmdCleanup );
                else
                    ret = this->SetStateOnEvent( cmdShutdown );

                if( ERROR( ret ) )
                    break;

                ret = DEFER_IFCALLEX_NOSCHED2(
                    0, pStopTask, this,
                    &CRpcServices::StopEx, 
                    ( IEventSink* )nullptr );

                if( ERROR( ret ) )
                    break;

                pGrp->AppendTask( pStopTask );

                for( auto& elem : this->m_vecMatches )
                {
                    ObjPtr pObj( elem );
                    ( *pvecMatches )().push_back( pObj );
                }
                oIfLock.Unlock();
            }
            else
            {
                // stopped already
                oIfLock.Unlock();

                // the m_vecMatches is cleared at this
                // point. rebuild the interface matches
                // from the config file.
                stdstr strPath, strName; 
                CCfgOpenerObj oIfCfg( this );
                ret = oIfCfg.GetStrProp(
                    propObjDescPath, strPath );
                if( ERROR( ret ) )
                    break;

                ret = oIfCfg.GetStrProp(
                    propObjName, strName );
                if( ERROR( ret ) )
                    break;

                CfgPtr pCfg;
                ret = CRpcServices::LoadObjDesc(
                    strPath, strName,
                    this->IsServer(), pCfg );
                if( ERROR( ret ) )
                    break;

                ObjPtr pObj;
                CCfgOpener oCfg( ( IConfigDb* )pCfg );
                ret = oCfg.GetObjPtr( propObjList, pObj );
                if( ERROR( ret ) )
                    break;

                pvecMatches = pObj;
            }

            ObjPtr pMatches = pvecMatches;
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

            TaskletPtr pTask = pGrp;
            if( pGrp->GetTaskCount() )
                pGrp->AppendTask( pStartTask );
            else
                pTask = pStartTask;

            if( pCallback != nullptr )
            {
                CIfRetryTask* pRetry = pTask;
                pRetry->SetClientNotify( pCallback );
            }

            InterfPtr pRootIf = GetRootIf();
            CRpcServices* pRootSvc = pRootIf;
            ret = pRootSvc->AddSeqTask( pTask );

            if( SUCCEEDED( ret ) )
                ret = pTask->GetError();

            break;

        }while( 0 );

        if( ERROR( ret ) && pGrp->GetTaskCount() )
        {
            ( *pGrp )( eventCancelTask );
        }

        return ret;
    }

    gint32 ReloadSvcPoint( IEventSink* pCallback )
    {
        gint32 ret = 0;
        TaskGrpPtr pGrp;
        do{
            stdstr strPath;
            ret = GetSvcPath( strPath );
            if( ERROR( ret ) )
                break;

            CFuseSvcDir* pSvcDir = static_cast
                < CFuseSvcDir* >( GetSvcDir().get() );
            CIoManager* pMgr = this->GetIoMgr();
            guint32 dwMode = pSvcDir->GetMode();
            TaskletPtr pMkDir;
            ret = NEW_FUNCCALL_TASK( pMkDir,
                pMgr, fuseif_remkdir, strPath, dwMode );
            if( ERROR( ret ) )
                break;

            if( pCallback != nullptr )
            {
                CIfRetryTask* pRetry = pMkDir;
                pRetry->SetClientNotify( pCallback );
            }

            // start a dedicated thread for this
            ret = pMgr->RescheduleTask(
                pMkDir, !this->IsServer() );

            if( SUCCEEDED( ret ) )
                ret = pMkDir->GetError();

        }while( 0 );

        if( ERROR( ret ) && !pGrp.IsEmpty() )
            ( *pGrp )( eventCancelTask );

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

    inline gint32 OnStreamReadyFuse( HANDLE hStream )
    { return 0; }

    void AddReqFiles(
        const stdstr& strSuffix ) override;

    gint32 DoRmtModEventFuse(
        EnumEventId iEvent,
        const std::string& strModule,
        IConfigDb* pEvtCtx );

    gint32 OnStmClosingFuse( HANDLE hStream );
};

class CFuseSvcServer :
    public CFuseServicePoint< false >
{
    gint32 m_iLastGrp = 0;
    std::vector< guint32 > m_vecGrpIds;
    std::hashmap< stdstr, int > m_mapSessCount;
    std::atomic< guint32 > m_dwStmFileId;

    public:
    typedef CFuseServicePoint< false > super;
    CFuseSvcServer( const IConfigDb* pCfg ):
    _MyVirtBase( pCfg ),
    super( pCfg ), m_dwStmFileId( 0 )
    {}

    gint32 IncStmCount( const stdstr& strSess );
    gint32 DecStmCount( const stdstr& strSess );

    guint32 NewStmFileId()
    { return m_dwStmFileId++; }

    //Response&Event Dispatcher
    virtual gint32 DispatchMsg(
        const Json::Value& valMsg ) = 0;

    // to receive a request on server side or a
    // response or event on the proxy side
    gint32 ReceiveMsgJson(
            const stdstr& strMsg,
            guint64 qwReqId );

    gint32 OnUserCancelRequest( guint64 qwReqId );

    gint32 AcceptNewStreamFuse(
        IEventSink* pCallback,
        IConfigDb* pDataDesc );

    gint32 OnStreamReadyFuse( HANDLE hStream );
    gint32 OnStmClosingFuse( HANDLE hStream );

    gint32 AddGroup( guint32 dwGrpId,
        const GRP_ELEM& oElem ) override;

    virtual gint32 RemoveGroup(
        guint32 dwGrpId ) override;

    void AddReqFiles(
        const stdstr& strSuffix ) override;
};

struct FHCTX
{
    CRpcServices* pSvc;
    DIR_SPTR pFile;
};

struct SVC_INFO
{
    stdstr m_strSvcName;
    stdstr m_strDescPath;
    EnumClsid m_iClsid;
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

    using FHMAP = std::hashmap< HANDLE, FHCTX  >;
    FHMAP   m_mapHandles;

    std::vector< SVC_INFO > m_vecServices;
    std::hashmap< stdstr, guint32 > m_mapSvcInsts;
    DIR_SPTR m_pUserDir;

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

    gint32 RemoveSvcPoint( CRpcServices* pIf )
    {
        if( pIf == nullptr )
            return -EINVAL;

        stdstr strInstName;
        if( this->IsServer() )
        {
            CFuseSvcServer* pSvc = ObjPtr( pIf );
            strInstName = pSvc->
                GetSvcDir()->GetName();
        }
        else
        {
            CFuseSvcProxy* pSvc = ObjPtr( pIf );
            strInstName = pSvc->
                GetSvcDir()->GetName();
        }
        m_mapSvcInsts.erase( strInstName );

        bool bErased = false;
        auto itr = m_vecIfs.begin();
        while( itr != m_vecIfs.end() )
        {
            if( ( *itr )->GetObjId() ==
                pIf->GetObjId() )
            {
                m_vecIfs.erase( itr );
                bErased = true;
                break;
            }
            itr++;
        }
        if( bErased )
            return STATUS_SUCCESS;

        return -ENOENT;
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
            pDir = dynamic_cast< CFuseObjBase* >
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
        m_pRootDir = DIR_SPTR( new CFuseRootDir( this ) );

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

    gint32 DoAddSvcPoint(
        ObjPtr& pSvc,
        const stdstr& strInstName,
        HANDLE lwsi )
    {
        gint32 ret = 0;
        SVC_TYPE* pSp = pSvc;
        if( pSp == nullptr )
            return -EINVAL;

        ret = pSp->BuildDirectories();
        if( ERROR( ret ) )
            return ret;

        do{
            ROOTLK_EXCLUSIVE;
            if( this->GetState() != stateConnected )
            {
                ret = ERROR_STATE;
                break;
            }

            SVC_INFO* psi = ( SVC_INFO* )lwsi;
            DIR_SPTR pSvcDir = pSp->GetSvcDir();
            CFuseObjBase* pRoot = _pRootDir;
            CFuseSvcDir* psd = static_cast
                < CFuseSvcDir* >( pSvcDir.get() );
            psd->SetName( strInstName );

            if( !this->IsServer() )
            {
                std::vector< DIR_SPTR > vecConns;
                pRoot->GetChildren( vecConns );
                ret = -ENOENT;
                for( auto& elem : vecConns )
                {
                    CFuseConnDir* pConn = dynamic_cast
                        < CFuseConnDir* >( elem.get() );
                    if( pConn == nullptr )
                        continue;
                    ret = pConn->AddSvcDir( pSvcDir );
                    if( SUCCEEDED( ret ) )
                        break;
                }
                if( ERROR( ret ) )
                {
                    stdstr strName =
                        stdstr( CONN_DIR_PREFIX );
                    guint32 dwSize = strName.size(); 
                    gint32 idx = 0;
                    
                    do{
                         if( strName.size() > dwSize )
                             strName.erase( dwSize );
                         strName +=
                             std::to_string( idx++ );
                    }while( pRoot->GetChild( strName ) );

                    DIR_SPTR pNewDir(
                        new CFuseConnDir( strName ) );

                    CFuseConnDir* pConn = static_cast
                        < CFuseConnDir* >( pNewDir.get() );
                    pConn->DecRef();
                    ret = pConn->AddSvcDir( pSvcDir );
                    if( ERROR( ret ) )
                        break;
                    ret = pRoot->AddChild( pNewDir );
                }
            }
            else
            {
                ret = pRoot->AddChild( pSvcDir );
            }

            if( ERROR( ret ) )
                break;

            InterfPtr pIf = ObjPtr( pSp );
            m_vecIfs.push_back( pIf );
            m_mapSvcInsts[ strInstName ] =
                psi->m_strSvcName.size();

        }while( 0 );

        if( ERROR( ret ) )
            pSp->ClearSvcDir();

        return ret;
    };

    void GetClassesAvail(
        std::vector< stdstr >& vecInfo ) const
    {
        CStdRMutex oLock( this->GetLock() );
        for( auto& elem : m_vecServices )
            vecInfo.push_back( elem.m_strSvcName );
    }

    gint32 AddSvcPoint(
        const stdstr& strInstName,
        IEventSink* pCallback = nullptr,
        guint32 dwTimeout = 90 )
    {
        gint32 ret = 0;
        bool bStarted = false;
        InterfPtr pIf;
        CIoManager* pMgr = this->GetIoMgr();
        TaskletPtr pTaskGrp;
        do{
            SVC_INFO* psi = nullptr;
            if( unlikely( !IsValidName( strInstName ) ) )
            {
                ret = -EINVAL;
                break;
            }

            // m_mapSvcInst is protected by ROOTLK
            if( m_mapSvcInsts.find( strInstName ) !=
                m_mapSvcInsts.end() )
            {
                ret = -EEXIST;
                break;
            }

            CStdRMutex oLock( this->GetLock() );
            for( auto& elem : m_vecServices )
            {
                if( strInstName.substr( 0,
                    elem.m_strSvcName.size() ) ==
                    elem.m_strSvcName )
                {
                    psi = &elem;
                    break;
                }
            }
            oLock.Unlock();

            if( psi == nullptr )
            {
                ret = -ENOENT;
                break;
            }

            gint32 pos = strInstName.rfind( '_' );
            stdstr strObjInst;
            if( pos == stdstr::npos )
            {
                strObjInst = strInstName;
            }
            else
            {
                strObjInst =
                    strInstName.substr( 0, pos );
            }

            CfgPtr pCfg( true );
            if( strObjInst != psi->m_strSvcName )
            {
                CCfgOpener oCfg(
                    ( IConfigDb* )pCfg );
                // to override the original object
                // instance name
                oCfg[ propObjInstName ] =
                    strObjInst;
            }

            ret = CRpcServices::LoadObjDesc(
                psi->m_strDescPath,
                psi->m_strSvcName, !bProxy, pCfg );
            if( ERROR( ret ) )
                break;

            CCfgOpener oCfg( ( IConfigDb* )pCfg );
            oCfg.SetPointer( propIoMgr, pMgr );
            ret = pIf.NewObj( psi->m_iClsid, pCfg );
            if( ERROR( ret ) )
                break;

            SVC_TYPE* pSp = pIf;
            if( pSp == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            CParamList oParams;
            oParams[ propIfPtr ] = ObjPtr( this );
            ret = pTaskGrp.NewObj(
                clsid( CIfTransactGroup ),
                oParams.GetCfg() );
            if( ERROR( ret ) )
                break;

            CIfTransactGroup* pTransGrp = pTaskGrp;

            TaskletPtr pStartTask;
            ret = DEFER_IFCALLEX_NOSCHED2(
                0, pStartTask, ObjPtr( pIf ),
                &CRpcServices::StartEx, nullptr );

            if( ERROR( ret ) )
                break;

            pTransGrp->AppendTask( pStartTask );

            TaskletPtr pUpdTask;
            ret = DEFER_OBJCALL_NOSCHED(
                pUpdTask, this,
                &CFuseRootBase::DoAddSvcPoint,
                ObjPtr( pSp ), strInstName,
                ( HANDLE )psi );
            if( ERROR( ret ) )
                break;

            pTransGrp->AppendTask( pUpdTask );

            TaskletPtr pStopTask;
            ret = DEFER_IFCALLEX_NOSCHED2(
                0, pStopTask, ObjPtr( pIf ),
                &CRpcServices::StopEx, nullptr );

            if( ERROR( ret ) )
                break;

            if( pCallback != nullptr )
                pTransGrp->SetClientNotify( pCallback );

            pTransGrp->AddRollback( pStopTask );
            ret = this->AddSeqTask( pTaskGrp );
            if( SUCCEEDED( ret ) )
                ret = pTaskGrp->GetError();

        }while( 0 );

        if( ERROR( ret ) )
        {
            if( !pTaskGrp.IsEmpty() )
                ( *pTaskGrp )( eventCancelTask );
        }

        return ret;
    }

    gint32 CanRemove( const stdstr& strName ) const
    {
        auto itr = m_mapSvcInsts.find( strName );
        if( itr == m_mapSvcInsts.cend() )
            return -ENOENT;
        return STATUS_SUCCESS;
    }

    // add a service point
    gint32 AddSvcPoint(
        const stdstr& strName,
        const stdstr& strDesc,
        EnumClsid iClsid,
        IEventSink* pCallback = nullptr )
    {
        if( strName.empty() ||
            strDesc.empty() ||
            iClsid == clsid( Invalid ) )
            return -EINVAL;
        gint32 ret = 0;
        bool bAdded = false;
        do{
            CStdRMutex oLock( this->GetLock() );
            for( auto& elem : m_vecServices )
            {
                stdstr strPrefix =
                    elem.m_strSvcName.substr(
                        0, strName.size() );
                if( strPrefix == strName )
                {
                    ret = -EEXIST;
                    break;
                }
                if( elem.m_iClsid == iClsid )
                {
                    ret = -EEXIST;
                    break;
                }
            }

            if( SUCCEEDED( ret ) )
                m_vecServices.push_back(
                    { strName, strDesc, iClsid } );
            else
                ret = 0;

            oLock.Unlock();

            if( pCallback != nullptr )
            {
                ret = AddSvcPoint(
                    strName, pCallback );
                break;
            }

            TaskletPtr pCallback;
            ret = pCallback.NewObj(
                clsid( CSyncCallback ) );
            if( ERROR( ret ) )
                break;

            CSyncCallback* pSync = pCallback;

            ret = AddSvcPoint( strName, pSync );
            if( ret == STATUS_PENDING )
            {
                pSync->WaitForCompleteWakable();
                ret = pSync->GetError();

            }

        }while( 0 );

        if( ERROR( ret ) )
        {
            OutputMsg( ret, "Checkpoint2: "
                "AddSvcPoint failed" );
        }

        return ret;
    }

    gint32 OnPostStart(
        IEventSink* pCallback )
    {
        gint32 ret = 0;
        do{
            auto pRoot = GetRootDir();
            // add a WO file as admin-command file
            auto pFile = DIR_SPTR(
                new CFuseCmdFile() ); 
            auto pObj = dynamic_cast
                < CFuseObjBase* >( pFile.get() );
            pObj->SetMode( S_IWUSR );
            pObj->DecRef();
            ret = pRoot->AddChild( pFile );
            if( ERROR( ret ) )
                break;

            auto pList = DIR_SPTR(
                new CFuseClassList( nullptr ) ); 
            pObj = dynamic_cast
                < CFuseObjBase* >( pList.get() );
            pObj->SetMode( S_IWUSR );
            pObj->DecRef();
            ret = pRoot->AddChild( pList );
            if( ERROR( ret ) )
                break;

            auto pUserDir = new CFuseDirectory(
                USER_DIR, nullptr );
            pUserDir->SetMode( S_IRUSR | S_IXUSR );
            pUserDir->DecRef();
            m_pUserDir = DIR_SPTR( pUserDir );
            ret = pRoot->AddChild( m_pUserDir );
            if( ERROR( ret ) )
                break;

        }while( 0 );

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

    gint32 Add2UserDir( DIR_SPTR& pEnt )
    {
        // don't call it after the initialization stage so
        // far
        return m_pUserDir->AddChild( pEnt );
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

    gint32 OnPostStart(
        IEventSink* pCallback ) override
    { return super::OnPostStart( pCallback ); }
};

class CFuseRootServer :
    public CFuseRootBase< false >
{
    public:
    typedef CFuseRootBase< false > super;
    CFuseRootServer( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CFuseRootServer ) ); }

    gint32 OnPostStart(
        IEventSink* pCallback ) override
    { return super::OnPostStart( pCallback ); }
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
    rpcf::IEventSink* pCb;
};

gint32 fuseif_main( fuse_args& args,
    fuse_cmdline_opts& opts );

gint32 fuseif_daemonize( fuse_args& args,
    fuse_cmdline_opts& opts,
    int argc, char **argv );

