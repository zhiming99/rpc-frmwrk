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
#include "fuse_i.h"
#include <sys/ioctl.h>
#include <unistd.h>
#include <counters.h>
#include <regex>

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
    if( pSvc == nullptr )
        return nullptr;
    if( pSvc->IsServer() )
    {
        CFuseRootServer* pSvr = GetRootIf();
        return pSvr->GetRootDir();
    }
    CFuseRootProxy* pProxy = GetRootIf();
    return pProxy->GetRootDir();
};

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
    const DIR_SPTR& pEnt )
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

gint32 CFuseObjBase::fs_release(
    const char* path,
    fuse_file_info * fi )
{
    CFuseMutex oFileLock( GetLock() );
    DecOpCount();
    return 0;
}

gint32 CFuseObjBase::RemoveChild(
    const std::string& strName )
{
    gint32 ret = 0;
    do{
        DIR_SPTR pEnt;
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
            pCli->RemoveFromFhMap(
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
    IncOpCount();

    fi->direct_io = 1;
    fi->keep_cache = 0;
    fi->nonseekable = 1;
    fi->fh = ( guint64 )( CFuseObjBase* )this;

    return STATUS_SUCCESS;
}

gint32 CFuseDirectory::fs_opendir(
    const char* path,
    fuse_file_info *fi )
{
    if( path == nullptr || fi == nullptr )
        return -EINVAL;

    CFuseMutex oLock( GetLock() );
    IncOpCount();

    fi->direct_io = 1;
    fi->keep_cache = 0;
    fi->nonseekable = 1;
    fi->fh = ( guint64 )( CFuseObjBase* )this;

    return STATUS_SUCCESS;
}

gint32 CFuseDirectory::fs_releasedir(
    const char* path,
    fuse_file_info *fi )
{
    if( path == nullptr || fi == nullptr )
        return -EINVAL;

    CFuseMutex oLock( GetLock() );
    DecOpCount();

    return STATUS_SUCCESS;
}

gint32 CFuseFileEntry::fs_open(
    const char* path,
    fuse_file_info *fi )
{
    if( path == nullptr || fi == nullptr )
        return -EINVAL;

    CFuseMutex oLock( GetLock() );

    if( GetOpCount() > 0 )
        return -EUSERS;

    IncOpCount();

    if( fi->flags & O_NONBLOCK )
        SetNonBlock();

    fi->direct_io = 1;
    fi->keep_cache = 0;
    fi->nonseekable = 1;
    fi->fh = ( guint64 )( CFuseObjBase* )this;

    return STATUS_SUCCESS;
}

#define INIT_STATBUF( _stbuf ) \
do{ \
    memset( _stbuf, 0, sizeof(struct stat ) );\
    ( _stbuf )->st_uid = getuid(); \
    ( _stbuf )->st_gid = getgid(); \
    ( _stbuf )->st_atime = GetAccTime();\
    ( _stbuf )->st_mtime = GetModifyTime(); \
}while( 0 )

CFuseRootDir::CFuseRootDir( CRpcServices* pIf ) :
    super( "/", pIf )
{
    SetClassId( clsid( CFuseRootDir ) );
    SetMode( S_IRUSR | S_IXUSR );
}

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

gint32 CFuseDirectory::fs_rmdir(
    const char *path,
    fuse_file_info* fi,
    bool bReload )
{
    gint32 ret = 0;
    TaskGrpPtr pStopTasks;
    do{
        IEventSink* pCallback =
            ( IEventSink* )fi->fh;
        CRpcServices* pIf = GetRootIf();
        EnumClsid iClsid = GetClsid();

        if( ( pIf->IsServer() &&
            iClsid != clsid( CFuseRootDir ) ) ||
            ( !pIf->IsServer() &&
            iClsid != clsid( CFuseConnDir ) ) )
        {
            ret = -EACCES;
            break;
        }

        if( pIf->IsServer() )
        {
            auto pSvc = static_cast
                < CFuseRootServer* >( pIf );
            ret = pSvc->CanRemove( path );
            if( ret == ERROR_FALSE && !bReload )
            {
                ret = -EACCES;
                break;
            }
            else if( ret == -ENOENT )
            {
                ret = 0;
                break;
            }
        }
        else
        {
            auto pSvc = static_cast
                < CFuseRootProxy* >( pIf );
            ret = pSvc->CanRemove( path );
            if( ret == ERROR_FALSE && !bReload )
            {
                ret = -EACCES;
                break;
            }
            else if( ret == -ENOENT )
            {
                ret = 0;
                break;
            }
        }

        DIR_SPTR pSvcDir;
        ret = this->GetChild( path, pSvcDir );
        if( ERROR( ret ) )
            break;

        auto psd = static_cast
            < CFuseSvcDir* >( pSvcDir.get() ); 

        std::vector< DIR_SPTR > vecChildren;
        psd->GetChildren( vecChildren );

        auto* pStmDir =
            psd->GetChild( STREAM_DIR );
        pStmDir->GetChildren( vecChildren );
        for( auto& elem : vecChildren )
        {
            auto pFile = dynamic_cast
                < CFuseObjBase* >( elem.get() );
            if( pFile == nullptr )
                continue;
            if( pFile->GetOpCount() > 0 )
            {
                ret = -EBUSY;
                break;
            }
        }

        vecChildren.clear();
        if( ERROR( ret ) )
            break;

        gint32 (*func)( CRpcServices*,
            const stdstr&, IEventSink* ) =
        ([]( CRpcServices* pSvcIf,
            const stdstr& strName,
            IEventSink* pCb )->gint32
        {
            gint32 ret = 0;
            do{
                fuse_ino_t inoParent = 0;
                fuse_ino_t inoChild = 0;

                if( pSvcIf->IsServer() )
                {
                    ROOTLK_EXCLUSIVE;
                    auto pSvc = dynamic_cast
                        < CFuseSvcServer* >( pSvcIf );

                    auto psd = static_cast< CFuseSvcDir* >
                        ( pSvc->GetSvcDir().get() );

                    auto pParent = static_cast
                    < CFuseRootDir* >( psd->GetParent() );

                    CFuseRootServer* pRootIf =
                        GetRootIf();

                    pParent->RemoveChild( strName );
                    pRootIf->RemoveSvcPoint( pSvc );

                    inoParent = pParent->GetObjId();
                    inoChild = psd->GetObjId();
                }
                else
                {
                    ROOTLK_EXCLUSIVE;
                    auto pSvc = dynamic_cast
                        < CFuseSvcProxy* >( pSvcIf );

                    auto psd = static_cast< CFuseSvcDir* >
                        ( pSvc->GetSvcDir().get() );

                    auto pParent = static_cast
                    < CFuseConnDir* >( psd->GetParent() );

                    CFuseRootProxy* pRootIf =
                        GetRootIf();

                    pParent->RemoveChild( strName );
                    pRootIf->RemoveSvcPoint( pSvc );

                    inoParent = pParent->GetObjId();
                    inoChild = psd->GetObjId();
                }

                fuse* pFuse = GetFuse();
                if( pFuse == nullptr )
                    break;

                fuse_session* se =
                    fuse_get_session( pFuse );

                fuse_lowlevel_notify_delete( se,
                    inoParent, inoChild,
                    strName.c_str(),
                    strName.size() );

                pCb->OnEvent( eventTaskComp,
                    STATUS_SUCCESS, 0, nullptr );

            }while( 0 );

            return ret;
        });

        CIoManager* pMgr = pIf->GetIoMgr();
        ObjPtr pSvc = psd->GetIf();

        TaskletPtr pRmDir;
        ret = NEW_FUNCCALL_TASK( pRmDir,
            pMgr, func, pSvc,
            stdstr( path ), pCallback );
        if( ERROR( ret ) )
            break;

        CParamList oParams;
        oParams.SetPointer( propIoMgr, pMgr );
        ret = pStopTasks.NewObj(
            clsid( CIfTaskGroup ),
            oParams.GetCfg() );
        if( ERROR( ret ) )
            break;
        pStopTasks->SetRelation( logicNONE );

        TaskletPtr pStopTask;
        ret = DEFER_IFCALLEX_NOSCHED2(
            0, pStopTask, pSvc,
            &CRpcServices::StopEx, 
            ( IEventSink* )nullptr );
        if( ERROR( ret ) )
            break;

        TaskletPtr pClearTask;
        if( pIf->IsServer() )
        {
            ret = DEFER_IFCALL_NOSCHED(
                pClearTask, pSvc,
                &CFuseSvcServer::Unmount );
        }
        else
        {
            ret = DEFER_IFCALL_NOSCHED(
                pClearTask, pSvc,
                &CFuseSvcProxy::Unmount );
        }
        if( ERROR( ret ) )
            break;

        pStopTasks->AppendTask( pStopTask );
        pStopTasks->AppendTask( pClearTask );
        pStopTasks->AppendTask( pRmDir );

        TaskletPtr pGrp = pStopTasks;
        ret = pMgr->RescheduleTask( pGrp );
        if( SUCCEEDED( ret ) )
            ret = STATUS_PENDING;

    }while( 0 );

    if( ERROR( ret ) && !pStopTasks.IsEmpty() )
        ( *pStopTasks )( eventCancelTask );

    return ret;
}

gint32 CFuseDirectory::fs_mkdir(
    const char *path,
    fuse_file_info* fi,
    mode_t mode )
{
    gint32 ret = 0;
    do{
        if( unlikely( fi == nullptr ||
            fi->fh == 0 ) )
        {
            ret = -EINVAL;
            break;
        }
        CRpcServices* pIf = GetRootIf();
        EnumClsid iClsid = GetClsid();

        if( ( pIf->IsServer() &&
            iClsid != clsid( CFuseRootDir ) ) ||
            ( !pIf->IsServer() &&
            iClsid != clsid( CFuseConnDir ) ) )
        {
            ret = -EACCES;
            break;
        }

        if( pIf->IsServer() )
        {
            auto pSvc = static_cast
                < CFuseRootServer* >( pIf );
            if( pSvc == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            IEventSink* pCb = ( IEventSink* )fi->fh;
            ret = pSvc->AddSvcPoint( path, pCb );
            if( ERROR( ret ) )
                break;
            fuse_invalidate_path(
                GetFuse(), "/" );
        }
        else
        {
            auto pSvc = static_cast
                < CFuseRootProxy* >( pIf );
            if( pSvc == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            IEventSink* pCb = ( IEventSink* )fi->fh;
            ret = pSvc->AddSvcPoint( path, pCb );
            if( ERROR( ret ) )
                break;
            stdstr strPath = "/" + GetName();
            fuse_invalidate_path(
                GetFuse(), strPath.c_str() );
        }

        // SetMode( mode );

    }while( 0 );

    return ret;
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
    guint32 size = 0;
    this->fs_ioctl( path, fi,
        FIOC_GETSIZE, nullptr, 0,
        &size );
    stbuf->st_size = size;
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
        std::vector< DIR_SPTR > vecChildren;
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

gint32 CFuseSvcStat::UpdateContent()
{
    gint32 ret = 0;
    do{
        CRpcServices* pIf = GetIf();
        if( unlikely( pIf == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }
        bool bServer = pIf->IsServer();

        timespec tsNow, tsStart;
        ret = clock_gettime(
            CLOCK_REALTIME, &tsNow );

        m_strContent += "CurTime=";
        char szBuf[ 64 ];
        szBuf[ sizeof( szBuf ) - 1 ] = 0;
        snprintf( szBuf,
            sizeof( szBuf ) - 1,
            "%ld.%09ld\n",
            tsNow.tv_sec,
            tsNow.tv_nsec );
        m_strContent += szBuf;
        
        stdstr strState;
        EnumIfState iState = pIf->GetState();
        switch( iState )
        {
        case stateStopped:
            strState = "Stopped";
            break;
        case stateStarting:
            strState = "Starting";
            break;
        case stateStarted:
            strState = "Started";
            break;
        case stateConnected:
            strState = "Connected";
            break;
        case stateRecovery:
            strState = "Recovery";
            break;
        case statePaused:
            strState = "Paused";
            break;
        case stateUnknown:
            strState = "Unknown";
            break;
        case stateStopping:
            strState = "Stopping";
            break;
        case statePausing:
            strState = "Pausing";
            break;
        case stateResuming:
            strState = "Resuming";
            break;
        case stateIoDone:
            strState = "IoDone";
            break;
        case stateStartFailed:
            strState = "StartFailed";
            break;
        case stateInvalid:
        default:
            strState = "Invalid";
            break;
        }
        m_strContent += "IfStat=";
        m_strContent += strState + "\n";
        guint32 dwCount = this->GetActCount();
        m_strContent += "Objects=";

        m_strContent +=
            std::to_string( dwCount ) + "\n";

        auto pSvr = dynamic_cast
            < CFuseSvcServer* >( pIf );

        auto pProxy = dynamic_cast
            < CFuseSvcProxy* >( pIf );

        time_t uptime;
        if( SUCCEEDED( ret ) )
        {
            if( bServer )
            {
                tsStart = pSvr->GetStartTime();
            }
            else
            {
                tsStart = pProxy->GetStartTime();
            }
            uptime =
                tsNow.tv_sec - tsStart.tv_sec;

            m_strContent += "UpTime=";
            m_strContent += std::to_string(
                uptime ) + "\n";
        }

        stdstr strVal;
        CCfgOpenerObj oIfCfg( pIf );
        ret = oIfCfg.GetStrProp(
            propObjDescPath, strVal );
        if( SUCCEEDED( ret ) )
        {
            m_strContent += "DescFile=";
            m_strContent += strVal + "\n";
        }
       ret = oIfCfg.GetStrProp(
        propObjName, strVal );
       if( SUCCEEDED( ret ) )
       {
           m_strContent += "ObjName=";
           m_strContent += strVal + "\n";
       }

        ret = oIfCfg.GetStrProp(
            propObjPath, strVal );
        if( SUCCEEDED( ret ) )
        {
            m_strContent += "ObjectPath=";
            m_strContent += strVal + "\n";
        }

        CFuseSvcDir* pSvcDir = GetSvcDir( pIf );
        strVal = pSvcDir->GetName();
        m_strContent += "SvcName=";
        m_strContent += strVal + "\n";

        TaskGrpPtr pParaGrp;
        ret = pIf->GetParallelGrp( pParaGrp );
        std::vector< TaskletPtr > vecTasks;
        if( SUCCEEDED( ret ) && bServer )
        {
            pParaGrp->FindTaskByClsid(
                clsid( CIfInvokeMethodTask ),
                vecTasks );
            m_strContent += "IncomingReqs=";
            m_strContent += std::to_string(
                vecTasks.size() ) + "\n";
        }
        else if( SUCCEEDED( ret ) )
        {
            pParaGrp->FindTaskByClsid(
                clsid( CIfIoReqTask ),
                vecTasks );
            m_strContent += "OutgoingReqs=";
            m_strContent += std::to_string(
                vecTasks.size() ) + "\n";
        }
        m_strContent += "IsServer=";
        strVal = bServer ? "true\n" : "false\n";
        m_strContent += strVal;

        m_strContent += "StmCount=";
        auto pStm = dynamic_cast
            < IStream* >( pIf );
        strVal = std::to_string( 
            pStm->GetStreamCount() );
        m_strContent += strVal + "\n";

        m_strContent += "ReqFiles=";
        if( bServer )
        {
            strVal = std::to_string(
                pSvr->GetGroupCount() );
        }
        else
        {
            strVal = std::to_string(
                pProxy->GetGroupCount() );
        }
        m_strContent += strVal + "\n";

        while( pIf->IsServer() )
        {
            auto psc = dynamic_cast
                < CStatCountersServer* >( pIf );
            guint32 dwVal = 0;
            if( psc == nullptr )
                break;
            ret = psc->GetCounter2(
                propMsgCount, dwVal );
            if( ERROR( ret ) )
                break;
            m_strContent += "IncomingReqs=";
            m_strContent +=
                std::to_string( dwVal ) + "\n";
            break;
        }

    }while( 0 );

    return 0;
}

gint32 CFuseSvcStat::fs_read(
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

        CFuseMutex oLock( GetLock() );
        m_strContent.clear();
        UpdateContent();
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

gint32 CFuseCmdFile::fs_getattr(
    const char *path,
    fuse_file_info* fi,
    struct stat *stbuf)
{
    INIT_STATBUF( stbuf );
    stbuf->st_ino = GetObjId();
    stbuf->st_mode = S_IFREG | S_IWUSR;
    stbuf->st_nlink = 1;
    stbuf->st_size = 0;
    return 0;
}

gint32 CFuseCmdFile::fs_open(
    const char *path,
    fuse_file_info *fi )
{
    if( path == nullptr || fi == nullptr )
        return -EINVAL;

    CFuseMutex oLock( GetLock() );
    IncOpCount();

    fi->direct_io = 1;
    fi->keep_cache = 0;
    fi->nonseekable = 1;
    fi->fh = ( guint64 )( CFuseObjBase* )this;
    return STATUS_SUCCESS;
}

gint32 CFuseCmdFile::fs_release(
    const char* path,
    fuse_file_info * fi )
{
    CFuseMutex oLock( GetLock() );
    DecOpCount();
    return 0;
}

std::set< stdstr > g_setValidCmd = {
    "restart", "reload", "loadl", "addsp"
};

gint32 CFuseCmdFile::fs_write_buf(
    const char* path,
    fuse_file_info *fi,
    fuse_req_t req,
    fuse_bufvec *buf,
    fuseif_intr_data* d )
{
    gint32 ret = 0;

    std::vector< BufPtr > vecOutBufs;
    ret = CopyFromBufVec( vecOutBufs, buf );
    if( ERROR( ret ) )
        return ret;

    do{
        if( vecOutBufs.size() != 1 )
        {
            ret = -EINVAL;
            break;
        }
        BufPtr& pCmd = vecOutBufs.front();
        if( pCmd->size() <= 2 ||
            pCmd->size() > REG_MAX_PATH ) 
        {
            ret = -EINVAL;
            break;
        }
        char* p = pCmd->ptr();
        char* pend = pCmd->ptr() + pCmd->size();
        if( pend[ -1 ] != '\n' )
        {
            ret = -EINVAL;
            break;
        }
        --pend;
        while( p != pend )
        {
            if( *p == ' ' )
                break;
            ++p;
        }
        if( p - pCmd->ptr() == 0 )
        {
            ret = -EINVAL;
            break;
        }

        stdstr strCmd( pCmd->ptr(),
            p - pCmd->ptr() );

        if( g_setValidCmd.find( strCmd ) ==
            g_setValidCmd.end() ) 
        {
            ret = -EINVAL;
            break;
        }

        if( strCmd == "reload" ||
            strCmd == "restart" )
        {
            stdstr strSvc( p + 1, pend - p - 1 );
            if( !IsValidName( strSvc ) )
            {
                ret = -EINVAL;
                break;
            }

            auto pParent = dynamic_cast
                < CFuseDirectory* >( GetParent() );
            auto pSd = pParent->GetChild( strSvc );
            if( pSd == nullptr && strCmd == "reload" )
            {
                TaskletPtr pCallback;
                ret = pCallback.NewObj(
                    clsid( CIfDummyTask ) );
                if( ERROR( ret ) )
                    break;

                IEventSink* pDummy = pCallback;
                fuse_file_info fi1 = {0};
                fi1.fh = ( intptr_t )pDummy;

                ret = pParent->fs_mkdir(
                    strSvc.c_str(), &fi1, S_IRWXU );

                if( ret == STATUS_PENDING )
                    ret = 0;

                break;
            }
            auto pSvcDir = dynamic_cast
                < CFuseSvcDir* >( pSd );
            if( pSvcDir == nullptr )
            {
                // not a svcpoint directory
                ret = -EACCES;
                break;
            }

            CRpcServices* pIf = pSvcDir->GetIf();
            if( unlikely( pIf == nullptr ) )
            {
                ret = -EFAULT;
                break;
            }

            TaskletPtr pTask;
            CIoManager* pMgr = pIf->GetIoMgr();
            if( strCmd == "restart" )
            {
                if( pIf->IsServer() )
                {
                    auto pSvr = dynamic_cast
                        < CFuseSvcServer* >( pIf );
                    ret = pSvr->RestartSvcPoint(
                        nullptr );
                }
                else
                {
                    auto pProxy = dynamic_cast
                        < CFuseSvcProxy* >( pIf );
                    ret = pProxy->RestartSvcPoint(
                        nullptr );
                }
            }
            else
            {
                if( pIf->IsServer() )
                {
                    auto pSvr = dynamic_cast
                        < CFuseSvcServer* >( pIf );
                    ret = pSvr->ReloadSvcPoint(
                        nullptr );
                }
                else
                {
                    auto pProxy = dynamic_cast
                        < CFuseSvcProxy* >( pIf );
                    ret = pProxy->ReloadSvcPoint(
                        nullptr );
                }
            }

            if( ret == STATUS_PENDING )
                ret = 0;
        }
        else if( strCmd == "loadl" )
        {
            stdstr strPath0( p + 1, pend - p - 1 );
            std::smatch m;
            std::regex e(
            "^\\s*((?:/[^/\\s]+)+|(?:[^/\\s]+(?:/[^/\\s]+)*))\\s*$" );
            if( !std::regex_match( strPath0, m, e ) )
            {
                ret = -EINVAL;
                break;
            }

            if( m.size() < 2 ||
                !m[ 1 ].matched )
            {
                ret = -EINVAL;
                break;
            }
            stdstr strPath = m[ 1 ].str();
            ret = access( strPath.c_str(), R_OK );
            if( ret < 0 )
            {
                ret = -errno;
                break;
            }
            ret = CoLoadClassFactory(
                strPath.c_str() );
        }
        else if( strCmd == "addsp" )
        {
            stdstr strArgs( p + 1, pend - p - 1 );
            std::regex e(
            "^\\s*([a-zA-Z_]\\w*)\\s+((?:/[^/\\s]+)+|(?:[^/\\s]+(?:/[^/\\s]+)*))\\s*$" );
            std::smatch m;
            if( !std::regex_match( strArgs, m, e ) )
            {
                ret = -EINVAL;
                break;
            }

            if( m.size() < 3 ||
                !m[ 1 ].matched ||
                !m[ 2 ].matched  )
            {
                ret = -EINVAL;
                break;
            }
            stdstr strSvc = m[ 1 ].str();
            stdstr strDesc = m[ 2 ].str();

            CRpcServices* pIf = GetRootIf();
            stdstr strClass = "C";
            bool bServer = true;
            if( pIf->IsServer() )
            {
                strClass += strSvc + "_SvrImpl";
            }
            else
            {
                bServer = false;
                strClass += strSvc + "_CliImpl";
            }

            EnumClsid iClsid = CoGetClassId(
                strClass.c_str() );
            if( iClsid == clsid( Invalid ) )
            {
                ret = -ENOENT;
                break;
            }

            ret = access( strDesc.c_str(), R_OK );
            if( ret < 0 )
            {
                ret = -errno;
                break;
            }

            CfgPtr pCfg;
            ret = CRpcServices::LoadObjDesc(
                strDesc, strSvc, bServer, pCfg );
            if( ERROR( ret ) )
            {
                ret = -EINVAL;
                break;
            }

            gint32 ( *func )( const stdstr&,
               EnumClsid, const stdstr& ) =
               ([]( const stdstr& strDesc,
                   EnumClsid iClsid,
                   const stdstr& strObj )->gint32
            {
                gint32 ret = 0;
                do{
                    auto pIf = GetRootIf();
                    ROOTLK_SHARED;
                    CFuseRootServer* ps = pIf;
                    CFuseRootProxy* pp = pIf;
                    if( pIf->IsServer() )
                    {
                        ret = ps->AddSvcPoint(
                            strObj, strDesc,
                            iClsid, false );
                            break;
                    }
                    else
                    {
                        ret = pp->AddSvcPoint(
                            strObj, strDesc,
                            iClsid, false );
                    }
                }while( 0 );

                return ret;
            });

            CIoManager* pMgr = pIf->GetIoMgr();
            TaskletPtr pTask;
            ret = NEW_FUNCCALL_TASK( pTask,
                pMgr, func, strDesc, iClsid,
                strSvc );

            if( ERROR( ret ) )
                break;

            ret = pMgr->RescheduleTask(
                 pTask, true );
            break;
        }

    }while( 0 );

    return ret;
}

gint32 CFuseCmdFile::fs_read(
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

        stdstr strContent;
        strContent = "Commands:\n";
        strContent += "reload <serivce point> :\n"
            "\treload a 'service point' under the "
            "same directory of this command file. The 'service point' will be "
            "removed and recreated\n";

        strContent += "restart <serivce point> :\n"
            "\trestart a service point under the "
            "same directory of this command file. The 'service point' will be "
            "stopped and started again.\n";

        strContent += "loadl <library> :\n"
            "\tload the class factory exported from a shared library.\n";

        strContent += "addsp <service point> <desc file> :\n"
            "\tadd a new 'service point' from the 'desc file'. "
            "<service point> must be an object as can be find in <desc file>.\n"
            "\t<desc file> is a path to an object description file\n";

        if( off >= strContent.size() )
            break;

        size_t dwBytes = std::min(
            size, strContent.size() - off );

        BufPtr pBuf( true );
        pBuf->Append(
            strContent.c_str() + off, dwBytes );

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
        SetPollHandle( ph );
        if( m_queIncoming.size() > 0 &&
            m_queReqs.empty() )
            *reventsp |= POLLIN;

        if( GetMode() & S_IWUSR )
            *reventsp |= POLLOUT;

        *reventsp = ( fi->poll_events &
            ( GetRevents() | *reventsp ) );

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
        case FIOC_GETSIZE:
            {
                CFuseMutex oLock( GetLock() );
                guint32* pSize = ( guint32* )data;

                if( m_queReqs.size() )
                    break;

                *pSize = GetBytesAvail();
                break;
            }
        case FIOC_SETNONBLOCK:
            {
                CFuseMutex oLock( GetLock() );
                SetNonBlock( true );
                break;
            }
        case FIOC_SETBLOCK:
            {
                CFuseMutex oLock( GetLock() );
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
        case FIOC_GETSIZE:
            {
                CFuseMutex oLock( GetLock() );
                guint32* pSize = ( guint32* )data;

                if( m_queReqs.size() )
                    break;

                *pSize = GetBytesAvail();
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

gint32 CFuseStmFile::fs_ioctl(
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
        case FIOC_GETSIZE:
            {
                CFuseMutex oLock( GetLock() );
                guint32* pSize = ( guint32* )data;

                if( m_queReqs.size() )
                    break;

                *pSize = GetBytesAvail();
                guint32 dwSize = 0;
                {
                    InterfPtr pIf = GetIf();
                    CStreamServerFuse* pSvr = pIf;
                    CStreamProxyFuse* pProxy = pIf;
                    HANDLE hstm = GetStream();

                    if( pSvr != nullptr )
                    {
                        ret = pSvr->GetPendingBytes(
                            hstm, dwSize );
                    }
                    else
                    {
                        ret = pProxy->GetPendingBytes(
                            hstm, dwSize );
                    }
                    if( ERROR( ret ) )
                        break;
                }
                *pSize += dwSize;

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
    fuse_file_info *fi,
    bool bSched )
{
    UNREFERENCED( bSched );
    gint32 ret = 0;
    do{
        CRpcServices* pSvc = GetIf();

        if( GetOpCount() > 0 )
        {
            SetHidden();
            break;
        }

        CancelFsRequests();
        SetRemoved();

        CFuseStmDir* pParent = static_cast
            < CFuseStmDir* >( GetParent() );
        if( pParent == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        // keep a copy of pObj
        DIR_SPTR pEnt;
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
                fuse_reply_err( req, -iRet );
                m_queReqs.pop_front();
            }
            break;
        }

        m_queIncoming.push_back( { pBuf, 0 } );
        size_t dwAvail = GetBytesAvail();
        while( m_queReqs.size() > 0 && dwAvail > 0 )
        {
            auto& elem = m_queReqs.front();
            fuse_req_t req = elem.req;
            size_t dwReqSize = elem.dwReqSize;
            auto d = elem.pintr.get();
            fuse_bufvec* bufvec = nullptr;

            // the vector to keep the buffer valid
            size_t dwToRead =
                std::min( dwAvail, dwToRead );
            std::vector< BufPtr > vecRemoved;
            ret = FillBufVec(
                dwToRead, m_queIncoming,
                vecRemoved, bufvec );
            fuseif_finish_interrupt(
                GetFuse(), req, d );
            if( dwToRead > 0 )
                fuse_reply_data(
                    req, bufvec, FUSE_BUF_SPLICE_MOVE );
            else
                fuse_reply_err( req, 0 );
            fuseif_free_buf( bufvec );
            m_queReqs.pop_front();
            dwAvail -= dwToRead;
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

    do{
        if( size == 0 )
            break;

        CFuseMutex oFileLock( GetLock() );
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

        if( pSvc->GetState() != stateConnected )
        {
            ret = -ENOTCONN;
            break;
        }

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
                gint32 (*func)( CRpcServices*,
                    CFuseStmFile*, BufPtr& ) =
                    ([]( CRpcServices* pIf,
                        CFuseStmFile* pFile,
                        BufPtr& pBuf_ )->gint32
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
            ret = FillBufVec(
                dwBytesRead, m_queIncoming,
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

gint32 CFuseFileEntry::FillBufVec(
    size_t& dwReqSize,
    std::deque< INBUF >& queBufs,
    std::vector< BufPtr >& vecBufs,
    fuse_bufvec*& bufvec )
{
    gint32 ret = 0;
    do{
        guint32 dwAvail = 0;
        if( dwReqSize == 0 )
            break;

        auto itr = queBufs.begin();
        while( itr != queBufs.end() )
        {
            guint32& dwOff = itr->dwOff;
            BufPtr& pBuf = itr->pBuf;
            dwAvail += pBuf->size() - dwOff;

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

gint32 CFuseObjBase::CopyFromPipe(
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
        if( vecBufs.size() > 1 )
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
            {
                ret = pStmSvr->WriteStreamNoWait(
                    hstm, pBuf );
                if( ret == STATUS_PENDING )
                    ret = 0;
            }
            else
                ret = pStmSvr->WriteStreamAsync(
                    hstm, pBuf, ( IEventSink* )pCb );
        }
        else
        {
            if( IsNonBlock() )
            {
                ret = pStmProxy->WriteStreamNoWait(
                    hstm, pBuf );
                if( ret == STATUS_PENDING )
                    ret = 0;
            }
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

        if( pIf->GetState() != stateConnected )
        {
            ret = -ENOTCONN;
            break;
        }
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

void CFuseStmFile::NotifyPoll()
{
    do{
        CFuseMutex oFileLock( GetLock() );
        fuse_pollhandle* ph = GetPollHandle();
        if( ph != nullptr )
        {
            fuse_notify_poll( ph );
            SetPollHandle( nullptr );
        }

    }while( 0 );
    return;
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
        SetPollHandle( ph );
        if( m_queIncoming.size() > 0 &&
            m_queReqs.empty() )
            *reventsp |= POLLIN;
        gint32 ret = 0;
        guint32 dwSize = 0;
        bool bCanSend = false;
        {
            HANDLE hstm = GetStream();
            ObjPtr pObj = GetIf();
            CStreamProxyFuse* pProxy = pObj;
            CStreamServerFuse* pSvr = pObj;
            if( pSvr != nullptr )
            {
                ret = pSvr->GetPendingBytes(
                    hstm, dwSize );
                bCanSend = pSvr->CanSend( hstm );
            }
            else
            {
                ret = pProxy->GetPendingBytes(
                    hstm, dwSize );
                bCanSend = pProxy->CanSend( hstm );
            }
        }
        if( SUCCEEDED( ret ) && dwSize > 0 )
            *reventsp |= POLLIN;

        if( !GetFlowCtrl() && bCanSend )
            *reventsp |= POLLOUT;

        *reventsp = ( fi->poll_events &
            ( GetRevents() | *reventsp ) );

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

gint32 CFuseStmFile::fs_release(
    const char* path,
    fuse_file_info * fi )
{
    gint32 ret = 0;
    do{
        if( DecOpCount() > 0 )
            break;

        if( !IsHidden() )
            break;

        CFuseStmDir* pParent = static_cast
            < CFuseStmDir* >( GetParent() );
        if( pParent == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        CancelFsRequests();

        // keep a copy of pStm
        DIR_SPTR pEnt;
        stdstr strName = GetName();
        ret = pParent->GetChild( strName, pEnt );
        if( ERROR( ret ) )
            break;

        pParent->RemoveChild( strName );
        fuse_ino_t inoParent =
                pParent->GetObjId();

        fuse_ino_t inoChild = GetObjId();
        fuse* pFuse = GetFuse();
        if( pFuse == nullptr )
            break;

        fuse_session* se =
            fuse_get_session( pFuse );

        fuse_lowlevel_notify_delete( se,
            inoParent, inoChild,
            strName.c_str(),
            strName.size() );

        CRpcServices* pSvc = GetIf();
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

gint32 CFuseFileEntry::CancelFsRequest(
    fuse_req_t req, gint32 iRet )
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
        fuse_reply_err( req, -iRet );
        m_queReqs.erase( itr );
        return STATUS_SUCCESS;
    }

    return -ENOENT;
}

gint32 CFuseFileEntry::CancelFsRequests(
    gint32 iRet )
{
    gint32 ret = 0;
    CFuseMutex oLock( GetLock() );
    for( auto& elem : m_queReqs ) 
    {
        fuseif_finish_interrupt( GetFuse(),
            elem.req, 
            elem.pintr.get() );
        fuse_reply_err( elem.req, -iRet );
    }
    m_queReqs.clear();

    return ret;
}

gint32 CFuseEvtFile::fs_open(
    const char* path,
    fuse_file_info *fi )
{
    gint32 ret = super::fs_open( path, fi );
    if( ERROR( ret ) )
        return ret;
    m_dwLastOff = 0;
    return ret;
}
gint32 CFuseEvtFile::fs_read(
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
        if( m_queReqs.size() )
        {
            if( IsNonBlock() )
                break;

            m_queReqs.push_back(
                { req, size, PINTR( d ) } );
            ret = STATUS_PENDING;
            break;
        }

        guint32 dwAvail = GetBytesAvail();
        if( dwAvail == 0 )
        {
            size = 0;
            break;
        }
        else if( dwAvail >= size )
        {
            FillBufVec( size,
                m_queIncoming,
                vecBackup, bufvec );
            m_dwBytesAvail -= size;
        }
        else if( IsNonBlock() )
        {
            m_dwMsgRead += m_queIncoming.size();
            size = dwAvail;
            if( m_dwLastOff > off )
            {
                DebugPrint( off-m_dwLastOff,
                    "alert, trying to read old data" );
            }
            else
            {
                m_dwLastOff = off + size;
            }
            FillBufVec( size,
                m_queIncoming,
                vecBackup, bufvec );
            m_dwBytesAvail -= size;
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

        BufPtr pBuf( true );
        *pBuf = htonl( strMsg.size() );
        pBuf->Append(
            strMsg.c_str(), strMsg.size());
        m_queIncoming.push_back( { pBuf, 0 } );
        m_dwBytesAvail += pBuf->size();

        while( m_dwBytesAvail > MAX_EVT_QUE_BYTES ||
            m_queIncoming.size() > MAX_EVT_QUE_SIZE )
        {
            // overflow
            m_dwBytesAvail -=
                m_queIncoming.front().pBuf->size();
            m_queIncoming.pop_front();
        }

        fuse_pollhandle* ph = GetPollHandle();
        if( ph != nullptr )
        {
            fuse_notify_poll( ph );
            SetPollHandle( nullptr );
        }

        ++m_dwMsgCount;
        size_t dwAvail = GetBytesAvail();
        while( m_queReqs.size() > 0 && dwAvail > 0 )
        {
            auto& elem = m_queReqs.front();
            fuse_req_t req = elem.req;
            size_t dwReqSize = elem.dwReqSize;
            auto d = elem.pintr.get();
            fuse_bufvec* bufvec = nullptr;

            // the vector to keep the buffer valid
            size_t dwToRead =
                std::min( dwAvail, dwReqSize );

            DebugPrint( dwToRead,
                "completing queued read request" );
            std::vector< BufPtr > vecRemoved;
            FillBufVec(
                dwToRead, m_queIncoming,
                vecRemoved, bufvec );

            fuseif_finish_interrupt(
                    GetFuse(), req, d );
            if( dwToRead > 0 )
            {
                fuse_reply_data( req, bufvec,
                    FUSE_BUF_SPLICE_MOVE );
            }
            else
                fuse_reply_err( req, 0 );
            fuseif_free_buf( bufvec );
            m_queReqs.pop_front();
            dwAvail -= dwToRead;
        }

    }while( 0 );

    return ret;
}

using INO_INFO=std::pair< guint64, stdstr>;
static gint32 fuseif_remove_req_svr(
    CRpcServices* pIf,
    stdstr& strSuffix,
    guint32 dwGrpId )
{
    gint32 ret = 0;
    do{
        auto pSvr = dynamic_cast
            < CFuseSvcServer* >( pIf );

        std::vector< INO_INFO > vecInoid;
        WLOCK_TESTMNT2( pIf );

        stdstr strPath;
        ret = pSvr->GetSvcPath( strPath );
        if( ERROR( ret ) )
            break;

        stdstr strReq = "jreq_";
        strReq += strSuffix;

        auto pReq = static_cast
            < CFuseReqFileSvr* >
            ( _pSvcDir->GetChild( strReq ) );

        if( pReq != nullptr )
        {
            fuse_file_info fi = {0};
            fi.fh = ( guint64 )
                ( CFuseObjBase* )pReq;
            stdstr strReqPath =
                strPath + "/" + strReq;
            pReq->fs_unlink(
                strReqPath.c_str(), &fi, false );
            vecInoid.push_back( {
               pReq->GetObjId(), strReq } );
        }

        stdstr strResp = "jrsp_";
        strResp += strSuffix;

        auto pResp = static_cast
            < CFuseRespFileSvr* >
            ( _pSvcDir->GetChild( strResp ) );

        if( pResp != nullptr )
        {
            fuse_file_info fi = {0};
            fi.fh = ( guint64 )
                ( CFuseObjBase* )pResp;
            stdstr strRespPath =
                strPath + "/" + strResp;
            pResp->fs_unlink(
                strRespPath.c_str(), &fi, false );

            vecInoid.push_back( {
               pResp->GetObjId(), strResp } );
        }
        pSvr->RemoveGroup( dwGrpId );

        fuse_ino_t inoParent =
            _pSvcDir->GetObjId();

        fuse* pFuse = GetFuse();
        if( pFuse == nullptr )
            break;

        fuse_session* se =
            fuse_get_session( pFuse );

        for( auto& elem : vecInoid )
        {
            fuse_lowlevel_notify_delete( se,
                inoParent, elem.first,
                elem.second.c_str(),
                elem.second.size() );
        }

    }while( 0 );

    return ret;
}

static gint32 fuseif_remove_req_proxy(
    CRpcServices* pIf,
    stdstr& strSuffix,
    guint32 dwGrpId )
{
    gint32 ret = 0;
    do{

        auto pProxy = dynamic_cast
            < CFuseSvcProxy* >( pIf );

        std::vector< INO_INFO > vecInoid;
        WLOCK_TESTMNT2( pIf );

        stdstr strPath;
        ret = pProxy->GetSvcPath( strPath );
        if( ERROR( ret ) )
            break;

        stdstr strReq = "jreq_";
        strReq += strSuffix;

        auto pReq = static_cast
            < CFuseReqFileProxy* >
            ( _pSvcDir->GetChild( strReq ) );

        if( pReq != nullptr )
        {
            fuse_file_info fi = {0};
            fi.fh = ( guint64 )
                ( CFuseObjBase* )pReq;
            stdstr strReqPath =
                strPath + "/" + strReq;
            pReq->fs_unlink(
                strReqPath.c_str(), &fi, false );

            vecInoid.push_back( {
               pReq->GetObjId(), strReq } );
        }

        stdstr strResp = "jrsp_";
        strResp += strSuffix;

        auto pResp = static_cast
            < CFuseRespFileProxy* >
            ( _pSvcDir->GetChild( strResp ) );

        if( pResp != nullptr )
        {
            fuse_file_info fi = {0};
            fi.fh = ( guint64 )pResp;
            fi.fh = ( guint64 )
                ( CFuseObjBase* )pResp;
            stdstr strRespPath =
                strPath + "/" + strResp;
            pResp->fs_unlink(
                strRespPath.c_str(), &fi, false );

            vecInoid.push_back( {
               pResp->GetObjId(), strResp } );
        }

        stdstr strEvt = "jevt_";
        strEvt += strSuffix;

        auto pEvt = static_cast< CFuseEvtFile* >
            ( _pSvcDir->GetChild( strEvt ) );

        if( pEvt != nullptr )
        {
            fuse_file_info fi = {0};
            fi.fh = ( guint64 )
                ( CFuseObjBase* )pEvt;

            stdstr strEvtPath =
                strPath + "/" + strEvt;

            pEvt->fs_unlink(
                strEvtPath.c_str(), &fi, false );

            vecInoid.push_back( {
               pEvt->GetObjId(), strEvt } );
        }

        pProxy->RemoveGroup( dwGrpId );

        fuse_ino_t inoParent =
            _pSvcDir->GetObjId();

        fuse* pFuse = GetFuse();
        if( pFuse == nullptr )
            break;

        fuse_session* se =
            fuse_get_session( pFuse );

        for( auto& elem : vecInoid )
        {
            fuse_lowlevel_notify_delete( se,
                inoParent, elem.first,
                elem.second.c_str(),
                elem.second.size() );
            DebugPrint( 0, "%s is removed",
                elem.second.c_str() );
        }

    }while( 0 );

    return ret;
}

gint32 CFuseEvtFile::do_remove( bool bSched )
{
    gint32 ret = 0;
    do{
        CFuseSvcDir* pParent = static_cast
            < CFuseSvcDir* >( GetParent() );
        if( pParent == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        // keep a copy of the file to delete
        DIR_SPTR pEnt;
        stdstr strName = GetName();
        ret = pParent->GetChild( strName, pEnt );
        if( ERROR( ret ) )
            break;

        pParent->RemoveChild( strName );

        if( !bSched )
            break;

        guint32 dwGrpId = GetGroupId();

        gint32 ( *func )( CRpcServices*,
            const stdstr&, guint32 ) =
            ([]( CRpcServices* pIf,
                const stdstr& strName,
                guint32 dwGrpId )->gint32
        {
            gint32 ret = 0;
            do{
                if( strName.size() <= 5 )
                {
                    ret = -EINVAL;
                    break;
                }

                stdstr strSuffix =
                    strName.substr( 5 );

                if( pIf->IsServer() )
                {
                    ret = fuseif_remove_req_svr(
                        pIf, strSuffix, dwGrpId );
                }
                else
                {
                    ret = fuseif_remove_req_proxy(
                        pIf, strSuffix, dwGrpId );
                }

            }while( 0 );

            return ret;
        });

        TaskletPtr pTask;
        CRpcServices* pIf = GetIf();
        CIoManager* pMgr = pIf->GetIoMgr();
        ret = NEW_FUNCCALL_TASK( pTask,
            pMgr, func, pIf, strName,
            GetGroupId() );

        if( ERROR( ret ) )
            break;

        pMgr->RescheduleTask( pTask );

    }while( 0 );

    return ret;
}

gint32 CFuseEvtFile::fs_release(
    const char* path,
    fuse_file_info * fi )
{
    gint32 ret = 0;
    do{
        if( DecOpCount() > 0 )
            break;

        CancelFsRequests( -ECANCELED );
        m_queIncoming.clear();

        if( !IsHidden() )
            break;

        if( GetGroupId() == 0 )
        {
            ret = -EACCES;
            break;
        }

        ret = do_remove( true );

    }while( 0 );

    return ret;
}

gint32 CFuseEvtFile::fs_unlink(
    const char* path,
    fuse_file_info * fi,
    bool bSched )
{
    gint32 ret = 0;
    do{
        if( GetGroupId() == 0 )
        {
            ret = -EACCES;
            break;
        }

        CancelFsRequests( -ECANCELED );

        if( GetOpCount() > 0 )
        {
            SetHidden();
            break;
        }

        SetRemoved();
        ret = do_remove( bSched );

    }while( 0 );

    return ret;
}

gint32 CFuseObjBase::CopyFromBufVec(
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

gint32 CFuseRespFileSvr::CancelFsRequests(
    gint32 iRet )
{
    gint32 ret = 0;
    do{
        // super::CancelFsRequests( iRet );
        auto pSvr = dynamic_cast
            < CFuseSvcServer* >( GetIf() );

        if( pSvr == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        QwVecPtr pqwVec( true );
        std::vector< guint64 >& vecTaskIds =
            ( *pqwVec )();
        CFuseMutex oFileLock( GetLock() );
        for( auto& elem : m_setTaskIds )
            vecTaskIds.push_back( elem );
        m_setTaskIds.clear();

        oFileLock.Unlock();

        // cannot cancel the task directly within an
        // exclusive lock, let's schedule a task.
        gint32 (*func)( CFuseSvcServer*, ObjPtr, gint32 ) =
        ([]( CFuseSvcServer* pSvr,
            ObjPtr pvecIds, gint32 iRet )->gint32
        {
            gint32 ret = 0;
            TaskGrpPtr pGrp;
            ret = pSvr->GetParallelGrp( pGrp );
            if( ERROR( ret ) )
                return ret;
            QwVecPtr pqwVec = pvecIds;
            std::vector< guint64 >& vecTaskIds =
                ( *pqwVec )();
            for( auto& elem : vecTaskIds )
            {
                TaskletPtr pTask;
                ret = pGrp->FindTask( elem, pTask );
                if( ERROR( ret ) )
                    continue;

                CCfgOpener oCfg;
                oCfg[ propReturnValue ] = iRet;
                oCfg[ propSeriProto ] = seriRidl;
                pSvr->OnServiceComplete(
                    oCfg.GetCfg(), pTask );
            }
            return 0;
        });

        TaskletPtr pTask;
        CIoManager* pMgr = pSvr->GetIoMgr();
        ret = NEW_FUNCCALL_TASK( pTask, pMgr,
            func, pSvr, pqwVec, iRet );
        if( ERROR( ret ) )
            break;

        ret = pMgr->RescheduleTask( pTask );

    }while( 0 );

    return iRet;
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

    CFuseMutex oFileLock( GetLock() );

    ret = CopyFromBufVec(
            m_vecOutBufs, bufvec );
    if( ERROR( ret ) )
        return ret;

    do{
        // receive the 4-byte size in big endian
        if( m_pReqSize->size() < sizeof( guint32 ) )
        {
            ret = GetReqSize(
                m_pReqSize, m_vecOutBufs );
            if( ERROR( ret ) )
                break;
            if( ret == STATUS_PENDING )
            {
                // wait for more input
                // bufvec->idx = bufvec->count;
                ret = 0;
                break;
            }
        }
        size_t dwReqSize =
            ntohl( ( guint32& )*m_pReqSize );
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
        if( !m_pReader->parse( pBuf->ptr(),
            pBuf->ptr() + pBuf->size(),
            &valResp, nullptr ) )
        {
            ret = -EINVAL;
            break;
        }

        if( !valResp.isMember( JSON_ATTR_REQCTXID ) ||
            !valResp[ JSON_ATTR_REQCTXID ].isUInt64() )
        {
            ret = -EINVAL;
            break;
        }

        guint64 dwTaskId =
            valResp[ JSON_ATTR_REQCTXID ].asUInt64();
        ret = RemoveTaskId( dwTaskId );
        if( ERROR( ret ) )
        {
            // the request is gone
            ret = 0;
            m_pReqSize->Resize( 0 );
            if( m_vecOutBufs.empty() )
                break;
            continue;
        }

        // send the response
        CFuseSvcServer* pSvr = ObjPtr( GetIf() );
        ret = pSvr->DispatchMsg( valResp );
        if( ret == STATUS_PENDING )
            ret = 0;

        m_pReqSize->Resize( 0 );
        if( m_vecOutBufs.empty() )
        {
            ret = 0;
            break;
        }

    }while( 1 );

    if( ERROR( ret ) )
    {
        m_pReqSize->Resize( 0 );
        m_vecOutBufs.clear();    
    }

    return ret;
}

gint32 CFuseRespFileSvr::fs_release(
    const char* path, fuse_file_info * fi )
{
    gint32 ret = 0;
    m_pReqSize->Resize(0);
    m_vecOutBufs.clear();
    return super::fs_release( path, fi );
}

gint32 CFuseReqFileSvr::ReceiveMsgJson(
    const stdstr& strMsg,
    guint64 qwReqId )
{
    gint32 ret = 0;
    do{
        guint32 dwGrpId = GetGroupId();
        auto pSvr = dynamic_cast
            < CFuseSvcServer* >( GetIf() );
        if( unlikely( pSvr == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }
        CFuseSvcServer::GRP_ELEM oElem;
        ret = pSvr->GetGroup( dwGrpId, oElem );
        if( ERROR( ret ) )
            break;

        oElem.m_pRespFile->AddTaskId( qwReqId );
        ret = super::ReceiveEvtJson( strMsg );
        if( SUCCEEDED( ret ) )
            break;
        oElem.m_pRespFile->RemoveTaskId( qwReqId );

    }while( 0 );

    return ret;
}

gint32 CFuseRespFileProxy::ReceiveMsgJson(
    const stdstr& strMsg,
    guint64 qwReqId )
{
    UNREFERENCED( qwReqId );
    return super::ReceiveEvtJson( strMsg );
}

gint32 CFuseRespFileProxy::CancelFsRequests(
    gint32 iRet )
{
    gint32 ret = 0;
    do{
        super::CancelFsRequests( iRet );

        auto pSvc = GetIf();
        auto pProxy = dynamic_cast
            < CFuseSvcProxy* >( pSvc );
        std::vector< guint64 > vecReqs;
        ret = pProxy->RemoveGrpReqs(
            GetGroupId(), vecReqs );
        if( ERROR( ret ) )
            break;

        if( vecReqs.empty() )
            break;

        CFuseMutex oFileLock( GetLock() );
        if( GetOpCount() > 0 || !IsHidden() )
        {
            Json::StreamWriterBuilder oBuilder;
            oBuilder["commentStyle"] = "None";

            for( auto& elem : vecReqs )
            {
                Json::Value oVal;
                oVal[ JSON_ATTR_REQCTXID ] =
                    (  Json::UInt64& )elem;
                oVal[ JSON_ATTR_MSGTYPE ] = "resp";
                oVal[ JSON_ATTR_RETCODE ] = iRet;
                oVal[ "information" ] =
                    "The request is canceled unexpectedly";
                stdstr strResp = Json::writeString(
                    oBuilder, oVal );

                BufPtr pBuf( true );
                *pBuf = htonl( strResp.size() );
                pBuf->Append(
                    strResp.c_str(), strResp.size());
                m_queIncoming.push_back( { pBuf, 0 } );
            }
        }

        fuse_pollhandle* ph = GetPollHandle();
        if( ph != nullptr )
        {
            fuse_notify_poll( ph );
            SetPollHandle( nullptr );
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

    CFuseMutex oFileLock( GetLock() );
    ret = CopyFromBufVec(
            m_vecOutBufs, bufvec );
    if( ERROR( ret ) )
        return ret;

    do{
        // receive the size value of 4-bytes in big
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
                // bufvec->idx = bufvec->count;
                ret = 0;
                break;
            }
        }

        size_t dwReqSize =
            ntohl( ( guint32& )*m_pReqSize );
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
            // bufvec->idx = vec->count;
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
        if( !m_pReader->parse( pBuf->ptr(),
            pBuf->ptr() + pBuf->size(),
            &valReq, nullptr ) )
        {
            ret = -EINVAL;
            break;
        }
        
        ret = pProxy->DispatchReq(
            nullptr, valReq, valResp );

        Json::Value& valrid =
            valResp[ JSON_ATTR_REQCTXID ];

        guint64 qwReqId = valrid.asUInt64();

        Json::StreamWriterBuilder oBuilder;
        oBuilder["commentStyle"] = "None";
        stdstr strResp =
            Json::writeString( oBuilder, valResp );

        if( ret == STATUS_PENDING ||
            ret == -STATUS_PENDING )
        {
            pProxy->AddReqGrp(
                qwReqId, GetGroupId() );
            ret = 0;
        }
        else
        {
            // append the response to the resp file
            gint32 (*func)( CFuseSvcProxy*,
                guint64, guint32,
                const stdstr& )=([](
                    CFuseSvcProxy* pIf,
                    guint64 qwReqId,
                    guint32 dwGrpId,
                    const stdstr& strResp )->gint32
            {
                gint32 ret = 0;
                do{
                    RLOCK_TESTMNT2( pIf );
                    CFuseSvcProxy::GRP_ELEM oElem;
                    ret = pIf->GetGroup( dwGrpId, oElem );
                    if( ERROR( ret ) )
                        break;
                    auto pResp = oElem.m_pRespFile;
                    auto p = static_cast
                        < CFuseRespFileProxy* >( pResp );
                    p->ReceiveMsgJson(
                        strResp, qwReqId );

                }while( 0 );
                return ret;
            });

            TaskletPtr pTask;
            CIoManager* pMgr = pProxy->GetIoMgr();
            ret = NEW_FUNCCALL_TASK( pTask,
                pMgr, func, pProxy, qwReqId,
                GetGroupId(), strResp );
            if( ERROR( ret ) )
                break;

            pMgr->RescheduleTask( pTask );
        }

        m_pReqSize->Resize( 0 );
        if( m_vecOutBufs.empty() )
        {
            // bufvec->idx = bufvec->count;
            ret = 0;
            break;
        }

    }while( 1 );

    if( ERROR( ret ) )
    {
        m_pReqSize->Resize( 0 );
        m_vecOutBufs.clear();    
    }

    return ret;
}

gint32 CFuseReqFileProxy::fs_poll(
    const char *path,
    fuse_file_info *fi,
    fuse_pollhandle *ph,
    unsigned *reventsp )
{
    gint32 ret = 0;
    do{
        CFuseMutex oLock( GetLock() );

        SetPollHandle( ph );
        if( m_queIncoming.size() &&
            m_queReqs.empty() )
            *reventsp |= POLLIN;

        *reventsp |= POLLOUT;

        *reventsp = ( fi->poll_events &
            ( GetRevents() | *reventsp ) );

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

gint32 CFuseReqFileProxy::fs_release(
    const char* path, fuse_file_info * fi )
{
    gint32 ret = 0;
    m_pReqSize->Resize(0);
    m_vecOutBufs.clear();
    return super::fs_release( path, fi );
}

CFuseConnDir::CFuseConnDir( const stdstr& strName )
    : super( strName, nullptr )
{
    SetClassId( clsid( CFuseConnDir ) );
    // SetMode( S_IRUSR | S_IXUSR );
    SetMode( S_IRWXU );

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

    // add a WO file as admin-command file
    auto pCmd = DIR_SPTR(
        new CFuseCmdFile() ); 
    pObj = dynamic_cast
        < CFuseObjBase* >( pCmd.get() );
    pObj->SetMode( S_IWUSR );
    pObj->DecRef();
    AddChild( pCmd );
}

stdstr CFuseConnDir::GetRouterPath(
    CDirEntry* pDir ) const
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

gint32 CFuseConnDir::AddSvcDir(
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

        if( super::GetCount() <= 3 )
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

void CFuseConnDir::SetConnParams(
    const CfgPtr& pCfg )
{
    m_pConnParams = pCfg;
    stdstr strDump = DumpConnParams();
    CDirEntry* pEnt =
        GetChild( CONN_PARAM_FILE );
    CFuseTextFile* pText = static_cast
        < CFuseTextFile* >( pEnt );
    pText->SetContent( strDump );
}

bool CFuseConnDir::IsEqualConn(
    const IConfigDb* pConn ) const
{
    CConnParamsProxy oConn(
        ( IConfigDb* )m_pConnParams );
    CConnParamsProxy oConn1( pConn );
    return ( oConn == oConn1 );
}

stdstr CFuseConnDir::DumpConnParams()
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
}

gint32 CStreamServerFuse::OnStmRecv(
    HANDLE hChannel, BufPtr& pBuf )
{
    gint32 ret = 0;
    do{
        ret = super::OnStmRecv( hChannel, pBuf );
        if( ERROR( ret ) )
            break;
        CFuseSvcServer* pSvc = ObjPtr( this );
        pSvc->OnStmRecvFuse( hChannel, pBuf );

    }while( 0 );

    return ret;
}

gint32 CStreamProxyFuse::OnStmRecv(
    HANDLE hChannel, BufPtr& pBuf )
{
    gint32 ret = 0;
    do{
        ret = super::OnStmRecv( hChannel, pBuf );
        if( ERROR( ret ) )
            break;

        CFuseSvcProxy* pSvc = ObjPtr( this );
        pSvc->OnStmRecvFuse( hChannel, pBuf );

    }while( 0 );

    return ret;
}

gint32 CFuseSvcProxy::ReceiveEvtJson(
    const stdstr& strMsg )
{
    gint32 ret = 0;
    do{
        RLOCK_TESTMNT; 

        CStdRMutex oLock( GetLock() );
        std::vector< CFuseRespFileProxy* > vecResps;
        // broadcast events
        for( auto& elem : m_mapGroups )
            vecResps.push_back(
                elem.second.m_pRespFile );

        oLock.Unlock();

        for( auto& elem : vecResps )
            elem->ReceiveEvtJson( strMsg );

    }while( 0 );

    return ret;
}

gint32 CFuseSvcProxy::ReceiveMsgJson(
        const stdstr& strMsg,
        guint64 qwReqId )
{
    gint32 ret = 0;
    do{
        RLOCK_TESTMNT;

        guint32 dwGrpId = 0;
        CStdRMutex oLock( GetLock() );
        ret = GetGrpByReq( qwReqId, dwGrpId );
        if( ERROR( ret ) )
            break;

        GRP_ELEM ge;
        ret = GetGroup( dwGrpId, ge );
        if( ERROR( ret ) )
            break;

        oLock.Unlock();
        auto pRespFile = ge.m_pRespFile;
        ret = pRespFile->ReceiveMsgJson(
            strMsg, qwReqId );

        RemoveReqGrp( qwReqId );

    }while( 0 );

    return ret;
}

void CFuseSvcProxy::AddReqFiles(
    const stdstr& strSuffix )
{
    // add an RW request file
    CFuseObjBase* pObj = nullptr;
    guint32 dwGrpId = 0;
    if( !( strSuffix.size() == 1 &&
        strSuffix.front() == '0' ) )
        dwGrpId = GetGroupId();
    stdstr strName = "jreq_";
    strName += strSuffix;

    auto pFile = DIR_SPTR(
        new CFuseReqFileProxy( strName, this ) ); 
    pObj = dynamic_cast< CFuseObjBase* >
        ( pFile.get() );
    pObj->SetMode( S_IRUSR | S_IWUSR );
    pObj->DecRef();
    auto pReqFile = static_cast
        < CFuseReqFileProxy* >( pObj );
    pReqFile->SetGroupId( dwGrpId );
    m_pSvcDir->AddChild( pFile );

    // add an RO RESP file 
    strName = "jrsp_";
    strName += strSuffix;
    pFile = DIR_SPTR(
        new CFuseRespFileProxy( strName, this ) );
    pObj = dynamic_cast
        < CFuseObjBase* >( pFile.get() );
    pObj->SetMode( S_IRUSR );
    pObj->DecRef();
    auto pRespFile = static_cast
        < CFuseRespFileProxy* >( pObj );
    pRespFile->SetGroupId( dwGrpId );
    m_pSvcDir->AddChild( pFile );

    // add an RO event file 
    strName = "jevt_";
    strName += strSuffix;
    pFile = DIR_SPTR(
        new CFuseEvtFile( strName, this ) ); 
    pObj = dynamic_cast
        < CFuseObjBase* >( pFile.get() );
    pObj->SetMode( S_IRUSR );
    pObj->DecRef();
    auto pEvtFile = static_cast
        < CFuseEvtFile* >( pObj );
    pEvtFile->SetGroupId( dwGrpId );
    AddGroup( dwGrpId,
        { pReqFile, pRespFile, pEvtFile } );
    m_pSvcDir->AddChild( pFile );
}

gint32 CFuseSvcProxy::DoRmtModEventFuse(
    EnumEventId iEvent,
    const std::string& strModule,
    IConfigDb* pEvtCtx )
{
    gint32 ret = 0;
    do{
        if( iEvent != eventRmtModOffline )
            break;
        
        RLOCK_TESTMNT;
        std::vector< DIR_SPTR > vecReqFiles;
        _pSvcDir->GetChildren( vecReqFiles );
        for( auto& elem : vecReqFiles )
        {
            auto pReq = dynamic_cast
            < CFuseFileEntry* >( elem.get() );
            if( pReq != nullptr )
                pReq->CancelFsRequests( -EIO );
        }

    }while( 0 );

    return ret;
}

gint32 CFuseSvcProxy::OnStmClosingFuse(
    HANDLE hStream )
{
    gint32 ret = 0;
    do{
        stdstr strName;
        WLOCK_TESTMNT;
        ret = StreamToName( hStream, strName);
        if( ERROR( ret ) )
            break;
        ret = DeleteStmFile( strName );

    }while( 0 );

    return ret;
}

gint32 CFuseSvcServer::OnUserCancelRequest(
    guint64 qwReqId )
{
    gint32 ret = 0;
    do{
        std::vector< CFuseRespFileSvr* >
            vecRespFile;

        RLOCK_TESTMNT;

        CStdRMutex oLock( GetLock() );
        for( auto& elem : m_mapGroups )
        {
            auto pRespFile =
                elem.second.m_pRespFile;
            vecRespFile.push_back( pRespFile );
        }
        oLock.Unlock();
        for( auto& elem : vecRespFile )
        {
            ret = elem->OnUserCancelRequest(
                qwReqId );
            if( SUCCEEDED( ret ) )
            {
                DebugPrint( 0,
                    "Fuse Request %lld canceled", 
                    qwReqId );
                break;
            }
        }

    }while( 0 );

    return ret;
}

gint32 CFuseSvcServer::ReceiveMsgJson(
        const stdstr& strMsg,
        guint64 qwReqId )
{
    gint32 ret = 0;
    do{
        RLOCK_TESTMNT;

        CStdRMutex oLock( GetLock() );
        GRP_ELEM oElem;
        gint32 iGrpId = m_vecGrpIds[ m_iLastGrp ];

        if( m_vecGrpIds.size() > 1 )
        {
            m_iLastGrp++;
            m_iLastGrp %= m_vecGrpIds.size();
        }

        ret = GetGroup( iGrpId, oElem );
        if( ERROR( ret ) )
            break;
        oLock.Unlock();

        auto pReqFile = oElem.m_pReqFile;

        ret = pReqFile->ReceiveMsgJson(
            strMsg, qwReqId );

    }while( 0 );

    return ret;
}

void CFuseSvcServer::AddReqFiles(
    const stdstr& strSuffix )
{
    // add an RO request file
    CFuseObjBase* pObj = nullptr;
    guint32 dwGrpId = 0;
    if( !( strSuffix.size() == 1 &&
        strSuffix.front() == '0' ) )
        dwGrpId = GetGroupId();

    stdstr strName = "jreq_";
    strName += strSuffix;
    auto pFile = DIR_SPTR(
        new CFuseReqFileSvr( strName, this ) ); 
    pObj = dynamic_cast< CFuseObjBase* >
        ( pFile.get() );
    pObj->SetMode( S_IRUSR );
    pObj->DecRef();
    auto pReqFile = static_cast
        < CFuseReqFileSvr* >( pObj );
    pReqFile->SetGroupId( dwGrpId );
    m_pSvcDir->AddChild( pFile );

    // add an WO RESP file for both
    // response and event
    strName = "jrsp_";
    strName += strSuffix;

    pFile = DIR_SPTR(
        new CFuseRespFileSvr( strName, this ) ); 
    pObj = dynamic_cast
        < CFuseObjBase* >( pFile.get() );
    pObj->SetMode( S_IWUSR );
    pObj->DecRef();
    auto pRespFile = static_cast
        < CFuseRespFileSvr* >( pObj );
    pRespFile->SetGroupId( dwGrpId );
    AddGroup( dwGrpId, 
        { pReqFile, pRespFile } );
    m_pSvcDir->AddChild( pFile );
}

gint32 CFuseSvcServer::IncStmCount(
    const stdstr& strSess )
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
    }
    return STATUS_SUCCESS;
}

gint32 CFuseSvcServer::DecStmCount(
    const stdstr& strSess )
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
        if( itr->second == 0 )
            m_mapSessCount.erase( itr );
    }
    return STATUS_SUCCESS;
}

gint32 CFuseSvcServer::AcceptNewStreamFuse(
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

gint32 CFuseSvcServer::OnStreamReadyFuse(
    HANDLE hStream )
{
    gint32 ret = 0;
    do{
        if( this->GetState() != stateConnected )
            break;

        WLOCK_TESTMNT;
        ret = CreateStmFile( hStream );
        if( SUCCEEDED( ret ) )
            break;

        gint32 iRet = 0;
        CfgPtr pDesc;
        IConfigDb* ptctx = nullptr;
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

    }while( 0 );

    return ret;
}


gint32 CFuseSvcServer::OnStmClosingFuse(
    HANDLE hStream )
{
    gint32 ret = 0;
    do{
        WLOCK_TESTMNT;
        CfgPtr pDesc;
        IConfigDb* ptctx = nullptr;
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
        stdstr strName;
        ret = StreamToName( hStream, strName);
        if( ERROR( ret ) )
            break;
        ret = DeleteStmFile( strName );

    }while( 0 );

    return ret;
}

gint32 CFuseSvcServer::AddGroup(
    guint32 dwGrpId,
    const GRP_ELEM& oElem )
{
    CStdRMutex oLock( GetLock() );
    gint32 ret = super::AddGroup(
        dwGrpId, oElem );
    if( ERROR( ret ) )
        return ret;

    m_vecGrpIds.push_back( dwGrpId );
    return STATUS_SUCCESS;
}

gint32 CFuseSvcServer::RemoveGroup(
    guint32 dwGrpId )
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

gint32 CFuseRootServer::OnPostStart(
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

gint32 CFuseRootServer::Add2UserDir(
    DIR_SPTR& pEnt )
{
    // don't call it after the initialization stage so
    // far
    return m_pUserDir->AddChild( pEnt );
}

} // namespace

gint32 fuseop_access(
    const char* path, int flags )
{
    fuse_file_info* fi = nullptr;
    return SafeCall( "access",
        false, &CFuseObjBase::fs_access,
        path, fi, flags );
}

gint32 fuseop_open(
    const char* path, fuse_file_info * fi)
{
    return SafeCall( "open",
        false, &CFuseObjBase::fs_open,
        path, fi );
}

gint32 fuseop_release(
    const char* path, fuse_file_info * fi )
{
    return SafeCall( "release",
        true, &CFuseObjBase::fs_release,
        path, fi );
}

gint32 fuseop_opendir(
    const char* path, fuse_file_info * fi )
{
    return SafeCall( "opendir", false,
        &CFuseObjBase::fs_opendir,
        path, fi );
}

gint32 fuseop_releasedir(
    const char* path,
    fuse_file_info * fi )
{
    return SafeCall( "releasedir",
        false, &CFuseObjBase::fs_releasedir,
        path, fi );
}

gint32 fuseop_unlink( const char* path )
{
    if( path == nullptr )
        return -EINVAL;

    fuse_file_info* fi = nullptr;
    return SafeCall( "unlink", true,
        &CFuseObjBase::fs_unlink,
        path, fi, true ) ;
}

static gint32 fuseif_create_req(
    const stdstr& strSuffix,
    CFuseSvcDir* pDir, 
    fuse_file_info* fi )
{
    gint32 ret = 0;
    do{
        if( strSuffix.empty() )
        {
            ret = -EINVAL;
            break;
        }

        CRpcServices* pSvc = pDir->GetIf();
        CFuseSvcProxy* pProxy = ObjPtr( pSvc );
        CFuseSvcServer* pSvr = ObjPtr( pSvc );
        if( pSvc->GetState() != stateConnected )
        {
            ret = -ENOTCONN;
            break;
        }
        DIR_SPTR pSvcEnt = nullptr;
        if( pProxy != nullptr )
            pSvcEnt = pProxy->GetSvcDir();
        else
            pSvcEnt = pSvr->GetSvcDir();

        auto pSvcDir = static_cast
            < CFuseSvcDir* >( pSvcEnt.get() );

        stdstr strName = "jreq_";
        strName += strSuffix;

        WLOCK_TESTMNT0( pSvcDir );
        if( pDir->GetChild( strName ) != nullptr )
        {
            ret = -EEXIST;
            break;
        }

        if( pProxy != nullptr )
            pProxy->AddReqFiles( strSuffix );
        else
            pSvr->AddReqFiles( strSuffix );

        auto pEnt = _pSvcDir->GetChild( strName );
        if( pEnt == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        auto pReqFile = dynamic_cast
            < CFuseObjBase* >( pEnt );

        fi->fh = ( guint64 )pReqFile;
        fi->direct_io = 1;
        fi->keep_cache = 0;

    }while( 0 );

    return ret;
}
         
static gint32 fuseif_create_stream(
    const stdstr& strName,
    CFuseStmDir* pDir, 
    fuse_file_info* fi )
{
    gint32 ret = 0;
    do{
        CRpcServices* pSvc = pDir->GetIf();
        CFuseSvcProxy* pIf = ObjPtr( pSvc );
        if( pIf == nullptr )
        {
            ret = -EACCES;
            break;
        }

        if( pIf->GetState() != stateConnected )
        {
            ret = -ENOTCONN;
            break;
        }

        auto pEnt = pIf->GetSvcDir();
        auto pSvcDir = static_cast
            < CFuseSvcDir* >( pEnt.get() );

        RLOCK_TESTMNT0( pSvcDir );
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

        oSvcLock.Unlock();

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
            clsid( CSyncCallback ),
            oParams.GetCfg() );
        if( ERROR( ret ) )
            break;

        CSyncCallback* pSync = pCallback;

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

            CCfgOpenerObj oTaskCfg( pSync );
            IConfigDb* pResp = nullptr;
            ret = oTaskCfg.GetPointer(
                propRespPtr, pResp );
            if( ERROR( ret ) )
                break;

            CCfgOpener oResp( pResp );
            gint32 iRet = 0;
            ret = oResp.GetIntProp(
                propReturnValue,
                ( guint32& )iRet );
            if( ERROR( ret ) )
                break;
            ret = iRet;
            if( ERROR( ret ) )
                break;

            ret = oResp.GetIntPtr(
                1, ( guint32*& )hStream );

        }

        if( SUCCEEDED( ret ) )
        {
            // elevate to exclusive lock
            WLOCK_TESTMNT0( pSvcDir );
            auto pStmFile = new CFuseStmFile(
                strName, hStream, pStm );
            auto pEnt = DIR_SPTR( pStmFile ); 
            fi->fh = ( guint64 )
                ( CFuseObjBase* )pStmFile;
            fi->direct_io = 1;
            fi->keep_cache = 0;
            pStmFile->IncOpCount();
            pDir->AddChild( pEnt );
        }

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
        ROOTLK_SHARED;
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
        ret = GetSvcDir( strPath.c_str(),
            pObj, vecSubdirs );
        if( ERROR( ret ) )
            break;

        if( ret == ENOENT )
        {
            ret = -ENOENT;
            break;
        }

        if( vecSubdirs.empty() )
        {
            if( strName.size() <= 5 ||
                strName.substr( 0, 5 ) != "jreq_" )
            {
                ret = -EINVAL;
                break;
            }
            auto pDir = static_cast
                < CFuseSvcDir* >( pObj );

            ret = fuseif_create_req(
                strName.substr( 5 ), pDir, fi );
            break;
        }

        if( vecSubdirs.size() != 1 || 
            vecSubdirs[ 0 ] != STREAM_DIR )
        {
            ret = -EACCES;
            break;
        }

        auto pDir = static_cast< CFuseStmDir* >
            ( pObj->GetChild( STREAM_DIR ) );

        if( unlikely( pDir == nullptr ) )
        {
            // create allowed only under STREAM_DIR
            ret = -ENOSYS;
            break;
        }

        ret = fuseif_create_stream(
            strName, pDir, fi );

    }while( 0 );

    return ret;
}

int fuseop_poll(const char *path,
    fuse_file_info *fi,
    fuse_pollhandle *ph,
    unsigned *reventsp )
{
    return SafeCall( "poll", false, 
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
    return SafeCall( "ioctl", false,
        &CFuseObjBase::fs_ioctl,
        path, fi, cmd, arg, flags, data );
}

int fuseop_getattr(
    const char *path,
    struct stat *stbuf,
    fuse_file_info* fi )
{
    return SafeCall(
        "getattr", false,
        &CFuseObjBase::fs_getattr,
        path, fi, stbuf );
}

int fuseop_readdir(
    const char *path,
    void *buf, fuse_fill_dir_t filler,
    off_t off, struct fuse_file_info *fi,
    enum fuse_readdir_flags flags)
{
    return SafeCall( "readdir", false,
        &CFuseObjBase::fs_readdir,
        path, fi, buf, filler, off, flags );
}

#include <signal.h>
static int set_one_signal_handler(
    int sig, void (*handler)(int), int remove)
{
    struct sigaction sa;
    struct sigaction old_sa;

    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = remove ? SIG_DFL : handler;
    sigemptyset(&(sa.sa_mask));
    sa.sa_flags = 0;

    if (sigaction(sig, NULL, &old_sa) == -1) {
        perror("fuse: cannot get old signal handler");
        return -1;
    }

    if (old_sa.sa_handler == (remove ? handler : SIG_DFL) &&
        sigaction(sig, &sa, NULL) == -1) {
        perror("fuse: cannot set signal handler");
        return -1;
    }
    return 0;
}

static void do_nothing(int sig)
{
    OutputMsg( sig, "doing nothing with signal" );
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
    cfg->intr = 1;

    set_one_signal_handler(
        cfg->intr_signal, do_nothing, 0 );
    // init some other stuffs
    return GetFuse();
}

int fuseop_utimens(
    const char *path,
    const timespec tv[2],
    fuse_file_info *fi)
{
    return SafeCall( "utimens", false,
        &CFuseObjBase::fs_utimens,
        path, fi, tv );
}

int fuseop_mkdir(
    const char *path, mode_t mode ) 
{
    return SafeCall( "mkdir", false,
        &CFuseObjBase::fs_mkdir,
        path, nullptr, mode );
}

int fuseop_rmdir(
    const char *path ) 
{
    return SafeCall( "rmdir", false,
        &CFuseObjBase::fs_rmdir,
        path, nullptr, false );
}

gint32 fuseif_remkdir(
    const stdstr& strPath,
    guint32 dwMode )
{
    gint32 ret = 0;
    ret = SafeCall( "rmdir", false,
        &CFuseObjBase::fs_rmdir,
        strPath.c_str(),
        nullptr, true );

    if( ERROR( ret ) )
        return ret;

    return fuseop_mkdir(
        strPath.c_str(), dwMode );
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

    if( opts.foreground == 0 )
        daemon( 1, 0 );
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
    .mkdir = fuseop_mkdir,
    .unlink = fuseop_unlink,
    .rmdir = fuseop_rmdir,
    .open = fuseop_open,
    .release = fuseop_release,
    .opendir = fuseop_opendir,
    .readdir = fuseop_readdir,
    .releasedir = fuseop_releasedir,
    .init = fuseop_init,
    .create = fuseop_create,
    .utimens = fuseop_utimens,
    .ioctl = fuseop_ioctl,
    .poll = fuseop_poll,
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
gint32 fuseif_main( fuse_args& args,
    fuse_cmdline_opts& opts )
{
    struct fuse *fuse;
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

    fuse = fuse_new(&args, &fuseif_ops, sizeof(fuseif_ops), NULL);
    if (fuse == NULL) {
        res = 1;
        goto out1;
    }
    SetFuse( fuse );
    if( opts.mountpoint == nullptr )
    {
        res = 1;
        goto out1;
    }

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
