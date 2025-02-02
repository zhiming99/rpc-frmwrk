/****BACKUP YOUR CODE BEFORE RUNNING RIDLC***/
// ../../ridl/.libs/ridlc -sO . ./appmon.ridl 
// Implement the following methods
// to get the RPC proxy/server work
#include "rpc.h"
using namespace rpcf;
#include "stmport.h"
#include "fastrpc.h"
#include "blkalloc.h"
#include "appmon.h"
#include "AppMonitorsvr.h"
#include <fcntl.h>
#include <sys/file.h>

extern RegFsPtr g_pAppRegfs;
extern RegFsPtr g_pUserRegfs;

CFlockHelper::CFlockHelper(
    gint32 iFd, bool bRead )
{
    m_iFd = iFd;
    if( m_iFd < 0 )
        return;
    if( bRead )
        flock( m_iFd, LOCK_SH );
    else
        flock( m_iFd, LOCK_EX );
}

CFlockHelper::~CFlockHelper()
{
    if( m_iFd == -1 )
        return;
    flock( m_iFd, LOCK_UN );
}

gint32 CAppMonitor_SvrImpl::GetLoginInfo(
    IConfigDb* pCtx, CfgPtr& pInfo ) const
{
    gint32 ret = 0;
    do{
        IEventSink* pTask;
        CCfgOpener oCfg( pCtx );
        ret = oCfg.GetPointer(
            propEventSink, pTask );
        if( ERROR( ret ) )
            break;
        Variant oVar;
        ret = pTask->GetProperty(
            propLoginInfo, oVar );
        if( ERROR( ret ) )
            break;
        pInfo = ( ObjPtr& )oVar;
        if( pInfo.IsEmpty() )
        {
            ret = -ENOENT;
            break;
        }
    }while( 0 );
    return ret;
}

gint32 CAppMonitor_SvrImpl::GetUid(
    IConfigDb* pInfo, guint32& dwUid ) const
{
    Variant oVar;
    gint32 ret = pInfo->GetProperty(
        propUid, oVar );
    if( ERROR( ret ) )
        return ret;
    dwUid = ( guint32& )oVar; 
    return STATUS_SUCCESS;
}

gint32 CAppMonitor_SvrImpl::GetAccessContext(
    IConfigDb* pReqCtx,
    CAccessContext& oac ) const
{
    gint32 ret = 0;
    do{
        CfgPtr pInfo;
        ret = GetLoginInfo( pReqCtx, pInfo );
        if( ERROR( ret ) )
            break;
        guint32 dwUid = 0;
        ret = GetUid( pInfo, dwUid );
        if( ERROR( ret ) )
            break;

        CStdRMutex oLock( this->GetLock() );
        auto itr = m_mapUid2Gids.find( dwUid );
        if( itr == m_mapUid2Gids.end() )
        {
            ret = -EACCES;
            break;
        }
        oac.dwUid = dwUid;
        oac.pGids = itr->second;

    }while( 0 );
    return ret;
}

#define GETAC( _pContext ) \
        CAccessContext oac; \
        ret = GetAccessContext( ( _pContext ), oac ); \
        if( ret == -EACCES ) \
            break; \
        if( ret != -ENOENT && ERROR( ret ) ) \
            break

/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::CreateFile( 
    IConfigDb* pContext, 
    const std::string& strPath /*[ In ]*/,
    guint32 dwMode /*[ In ]*/,
    guint32 dwFlags /*[ In ]*/, 
    guint64& hFile /*[ Out ]*/ )
{
    gint32 ret = 0;
    do{
        GETAC( pContext );
        ret = m_pAppRegfs->CreateFile( strPath,
            dwMode, dwFlags, hFile, &oac );
    }while( 0 );
    return ret;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::MakeDir( 
    IConfigDb* pContext, 
    const std::string& strPath /*[ In ]*/,
    guint32 dwMode /*[ In ]*/ )
{
    gint32 ret = 0;
    do{
        GETAC( pContext );
        ret = m_pAppRegfs->MakeDir(
            strPath, dwMode, &oac );
    }while( 0 );
    return ret;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::OpenFile( 
    IConfigDb* pContext, 
    const std::string& strPath /*[ In ]*/,
    guint32 dwFlags /*[ In ]*/, 
    guint64& hFile /*[ Out ]*/ )
{
    gint32 ret = 0;
    do{
        GETAC( pContext );
        ret = m_pAppRegfs->OpenFile(
            strPath, dwFlags, hFile, &oac );
    }while( 0 );
    return ret;
}

/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::CloseFile( 
    IConfigDb* pContext, 
    guint64 hFile /*[ In ]*/ )
{
    return m_pAppRegfs->CloseFile( hFile );
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::RemoveFile( 
    IConfigDb* pContext, 
    const std::string& strPath /*[ In ]*/ )
{
    gint32 ret = 0;
    do{
        GETAC( pContext );
        ret = m_pAppRegfs->RemoveFile(
            strPath, &oac );
    }while( 0 );
    return ret;
}

/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::ReadFile( 
    IConfigDb* pContext, 
    guint64 hFile /*[ In ]*/,
    guint32 dwSize /*[ In ]*/,
    guint32 dwOff /*[ In ]*/, 
    BufPtr& buffer /*[ Out ]*/ )
{
    if( dwSize == 0 ||
        dwSize > MAX_BYTES_PER_TRANSFER )
        return -EINVAL;

    if( buffer.IsEmpty() )
        buffer.NewObj();

    gint32 ret = buffer->Resize( dwSize );
    if( ERROR( ret ) )
        return ret;

    return m_pAppRegfs->ReadFile(
        hFile, buffer->ptr(),
        dwSize, dwOff );
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::WriteFile( 
    IConfigDb* pContext, 
    guint64 hFile /*[ In ]*/,
    BufPtr& buffer /*[ In ]*/,
    guint32 dwOff /*[ In ]*/, 
    guint32& dwSizeWrite /*[ Out ]*/ )
{
    gint32 ret = 0;
    dwSizeWrite = buffer->size();
    return m_pAppRegfs->WriteFile(
        hFile, buffer->ptr(),
        dwSizeWrite, dwOff );
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::Truncate( 
    IConfigDb* pContext, 
    guint64 hFile /*[ In ]*/,
    guint32 dwOff /*[ In ]*/ )
{
    return m_pAppRegfs->Truncate(
        hFile, dwOff );
}

/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::CloseDir( 
    IConfigDb* pContext, 
    guint64 hFile /*[ In ]*/ )
{
    return m_pAppRegfs->CloseFile( hFile );
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::RemoveDir( 
    IConfigDb* pContext, 
    const std::string& strPath /*[ In ]*/ )
{
    gint32 ret = 0;
    do{
        GETAC( pContext );
        ret = m_pAppRegfs->RemoveDir(
            strPath, &oac );
    }while( 0 );
    return ret;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::SetGid( 
    IConfigDb* pContext, 
    const std::string& strPath /*[ In ]*/,
    guint32 wGid /*[ In ]*/ )
{
    gint32 ret = 0;
    do{
        GETAC( pContext );
        ret = m_pAppRegfs->SetGid(
            strPath, wGid, &oac );
    }while( 0 );
    return ret;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::SetUid( 
    IConfigDb* pContext, 
    const std::string& strPath /*[ In ]*/,
    guint32 wUid /*[ In ]*/ )
{
    gint32 ret = 0;
    do{
        GETAC( pContext );
        ret = m_pAppRegfs->SetGid(
            strPath, wUid, &oac );
    }while( 0 );
    return ret;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::GetGid( 
    IConfigDb* pContext, 
    const std::string& strPath /*[ In ]*/, 
    guint32& gid /*[ Out ]*/ )
{
    gint32 ret = 0;
    do{
        GETAC( pContext );
        ret = m_pAppRegfs->GetGid(
            strPath, gid, &oac );
    }while( 0 );
    return ret;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::GetUid( 
    IConfigDb* pContext, 
    const std::string& strPath /*[ In ]*/, 
    guint32& uid /*[ Out ]*/ )
{
    gint32 ret = 0;
    do{
        GETAC( pContext );
        ret = m_pAppRegfs->GetGid(
            strPath, uid, &oac );
    }while( 0 );
    return ret;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::SymLink( 
    IConfigDb* pContext, 
    const std::string& strSrcPath /*[ In ]*/,
    const std::string& strDestPath /*[ In ]*/ )
{
    gint32 ret = 0;
    do{
        GETAC( pContext );
        ret = m_pAppRegfs->SymLink(
            strSrcPath, strDestPath, &oac );
    }while( 0 );
    return ret;
}

/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::GetValue2( 
    IConfigDb* pContext, 
    const std::string& strPath /*[ In ]*/, 
    std::string& strJson /*[ Out ]*/ )
{
    gint32 ret = 0;
    do{
        Variant oVar;
        ret = GetValue(
            pContext, strPath, oVar );
        if( ERROR( ret ) )
            break;
        ret = oVar.SerializeToJson( strJson );
        if( ERROR( ret ) )
            break;
    }while( 0 );
    return ret;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::SetValue2( 
    IConfigDb* pContext, 
    const std::string& strPath /*[ In ]*/,
    const std::string& strJson /*[ In ]*/ )
{
    gint32 ret = 0;
    do{
        Variant oVar;
        ret = oVar.DeserializeFromJson( strJson );
        if( ERROR( ret ) )
            break;
        ret = this->SetValue(
            pContext, strPath, oVar );
    }while( 0 );
    return ret;
}

/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::GetValue( 
    IConfigDb* pContext, 
    const std::string& strPath /*[ In ]*/, 
    Variant& rvar /*[ Out ]*/ )
{
    gint32 ret = 0;
    do{
        GETAC( pContext );
        ret = m_pAppRegfs->GetValue(
            strPath, rvar, &oac );
    }while( 0 );
    return ret;
}

/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::SetValue( 
    IConfigDb* pContext, 
    const std::string& strPath /*[ In ]*/,
    const Variant& var /*[ In ]*/ )
{
    gint32 ret = 0;
    do{
        GETAC( pContext );
        ret = m_pAppRegfs->SetValue(
            strPath, var, &oac );
    }while( 0 );
    return ret;
}

/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::Chmod( 
    IConfigDb* pContext, 
    const std::string& strPath /*[ In ]*/,
    guint32 dwMode /*[ In ]*/ )
{
    gint32 ret = 0;
    do{
        GETAC( pContext );
        ret = m_pAppRegfs->Chmod(
            strPath, dwMode, &oac );
    }while( 0 );
    return ret;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::Chown( 
    IConfigDb* pContext, 
    const std::string& strPath /*[ In ]*/,
    guint32 dwUid /*[ In ]*/,
    guint32 dwGid /*[ In ]*/ )
{
    gint32 ret = 0;
    do{
        GETAC( pContext );
        ret = m_pAppRegfs->Chown(
            strPath, dwUid, dwGid, &oac );
    }while( 0 );
    return ret;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::ReadLink( 
    IConfigDb* pContext, 
    const std::string& strPath /*[ In ]*/, 
    BufPtr& buf /*[ Out ]*/ )
{
    gint32 ret = 0;
    do{
        GETAC( pContext );
        if( buf.IsEmpty() )
            buf.NewObj();
        struct stat stbuf;
        ret = m_pAppRegfs->GetAttr(
            strPath, stbuf );
        if( ERROR( ret ) )
            break;
        if( stbuf.st_size == 0 ||
            stbuf.st_size >= MAX_FILE_SIZE )
        {
            ret = -EINVAL;
            break;
        }
        guint32 dwSize = stbuf.st_size;
        ret = buf->Resize( dwSize );
        if( ERROR( ret ) )
            break;
        ret = m_pAppRegfs->ReadLink(
            strPath, buf->ptr(), dwSize, &oac );
    }while( 0 );
    return ret;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::Rename( 
    IConfigDb* pContext, 
    const std::string& szFrom /*[ In ]*/,
    const std::string& szTo /*[ In ]*/ )
{
    gint32 ret = 0;
    do{
        GETAC( pContext );
        ret = m_pAppRegfs->Rename(
            szFrom, szTo, &oac );
    }while( 0 );
    return ret;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::Flush( 
    IConfigDb* pContext, 
    guint32 dwFlags /*[ In ]*/ )
{
    //return m_pAppRegfs->Flush( dwFlags );
    return ERROR_NOT_IMPL;
}

/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::Access( 
    IConfigDb* pContext, 
    const std::string& strPath /*[ In ]*/,
    guint32 dwFlags /*[ In ]*/ )
{
    gint32 ret = 0;
    do{
        GETAC( pContext );
        ret = m_pAppRegfs->Access(
            strPath, dwFlags, &oac );
    }while( 0 );
    return ret;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::GetAttr( 
    IConfigDb* pContext, 
    const std::string& strPath /*[ In ]*/, 
    FileStat& oStat /*[ Out ]*/ )
{
    gint32 ret = 0;
    do{
        GETAC( pContext );
        struct stat stbuf;
        ret = m_pAppRegfs->GetAttr(
            strPath, stbuf , &oac );
        if( ERROR( ret ) )
            break;

        oStat.st_dev = stbuf.st_dev;
        oStat.st_ino = stbuf.st_ino;
        oStat.st_mode = stbuf.st_mode;
        oStat.st_nlink = stbuf.st_nlink;
        oStat.st_uid = stbuf.st_uid;
        oStat.st_gid = stbuf.st_gid;
        oStat.st_rdev = stbuf.st_rdev;
        oStat.st_size = stbuf.st_size;
        oStat.st_blksize = stbuf.st_blksize;
        oStat.st_blocks = stbuf.st_blocks;

        oStat.st_atim.tv_sec =
            stbuf.st_atim.tv_sec;
        oStat.st_atim.tv_nsec =
            stbuf.st_atim.tv_nsec;

        oStat.st_mtim.tv_sec =
            stbuf.st_mtim.tv_sec;
        oStat.st_mtim.tv_nsec =
            stbuf.st_mtim.tv_nsec;

        oStat.st_ctim.tv_sec =
            stbuf.st_ctim.tv_sec;
        oStat.st_ctim.tv_nsec =
            stbuf.st_ctim.tv_nsec;

        oStat.st_name =
            basename( strPath.c_str() );
    }while( 0 );
    return ret;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::ReadDir( 
    IConfigDb* pContext, 
    guint64 hDir /*[ In ]*/, 
    std::vector<FileStat>& vecDirEnt /*[ Out ]*/ )
{
    gint32 ret = 0;
    do{
        std::vector< KEYPTR_SLOT > vecks;
        ret = m_pAppRegfs->ReadDir(
            hDir, vecks );
        if( ERROR( ret ) )
            break;
        for( auto& ks : vecks )
        {
            FileStat oStat;
            oStat.st_name = ks.szKey;
            oStat.st_ino = ks.oLeaf.dwInodeIdx;
            guint32 dwMode = 0;
            if( ks.oLeaf.byFileType == ftDirectory )
                dwMode |= S_IFDIR;
            else if( ks.oLeaf.byFileType == ftRegular )
                dwMode |= S_IFREG;
            else if( ks.oLeaf.byFileType == ftLink )
                dwMode |= S_IFLNK;
            vecDirEnt.push_back( oStat );
        }
    }while( 0 );
    return ret;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::OpenDir( 
    IConfigDb* pContext, 
    const std::string& strPath /*[ In ]*/,
    guint32 dwFlags /*[ In ]*/, 
    guint64& hDir /*[ Out ]*/ )
{
    gint32 ret = 0;
    do{
        GETAC( pContext );
        ret = m_pAppRegfs->OpenDir(
            strPath, dwFlags, hDir, &oac );
    }while( 0 );
    return ret;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::ExecBat( 
    IConfigDb* pContext, 
    const std::string& strJson /*[ In ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'ExecBatComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}

/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::RegisterListener( 
    IConfigDb* pContext, 
    HANDLE hStream_h /*[ In ]*/,
    std::vector<std::string>& arrApps /*[ In ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'RegisterListenerComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::RemoveListener( 
    IConfigDb* pContext, 
    HANDLE hStream_h /*[ In ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'RemoveListenerComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::ListApps( 
    IConfigDb* pContext, 
    std::vector<std::string>& arrApps /*[ Out ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'ListAppsComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::ListPoints( 
    IConfigDb* pContext, 
    const std::string& strAppPath /*[ In ]*/, 
    std::vector<std::string>& arrPoints /*[ Out ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'ListPointsComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::ListAttributes( 
    IConfigDb* pContext, 
    const std::string& strPtPath /*[ In ]*/, 
    std::vector<std::string>& arrAttributes /*[ Out ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'ListAttributesComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::SetPointValue( 
    IConfigDb* pContext, 
    const std::string& strPtPath /*[ In ]*/,
    const Variant& value /*[ In ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'SetPointValueComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::GetPointValue( 
    IConfigDb* pContext, 
    const std::string& strPtPath /*[ In ]*/, 
    Variant& rvalue /*[ Out ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'GetPointValueComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::SetAttrValue( 
    IConfigDb* pContext, 
    const std::string& strAttrPath /*[ In ]*/,
    const Variant& value /*[ In ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'SetAttrValueComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::GetAttrValue( 
    IConfigDb* pContext, 
    const std::string& strAttrPath /*[ In ]*/, 
    Variant& rvalue /*[ Out ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'GetAttrValueComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::SetPointValues( 
    IConfigDb* pContext, 
    std::vector<KeyValue>& arrValues /*[ In ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'SetPointValuesComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::GetPointValues( 
    IConfigDb* pContext, 
    std::vector<std::string>& arrPtPaths /*[ In ]*/, 
    std::vector<KeyValue>& arrKeyVals /*[ Out ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'GetPointValuesComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* RPC event sender */
gint32 CAppMonitor_SvrImpl::OnPointChanged(
    const std::string& strPtPath /*[ In ]*/,
    const Variant& value /*[ In ]*/ )
{
    std::vector< InterfPtr > vecSkels;
    gint32 ret = this->EnumStmSkels( vecSkels );
    if( ERROR( ret ) )
        return ret;
    for( auto& elem : vecSkels )
    {
        CAppMonitor_SvrSkel* pSkel = elem;
        ret = pSkel->IIAppMonitor_SImpl::OnPointChanged(
            strPtPath,
            value );
    }
    return ret;
}
/* RPC event sender */
gint32 CAppMonitor_SvrImpl::OnPointsChanged(
    std::vector<KeyValue>& arrKVs /*[ In ]*/ )
{
    std::vector< InterfPtr > vecSkels;
    gint32 ret = this->EnumStmSkels( vecSkels );
    if( ERROR( ret ) )
        return ret;
    for( auto& elem : vecSkels )
    {
        CAppMonitor_SvrSkel* pSkel = elem;
        ret = pSkel->IIAppMonitor_SImpl::OnPointsChanged(
            arrKVs );
    }
    return ret;
}
gint32 CAppMonitor_SvrImpl::CreateStmSkel(
    HANDLE hStream, guint32 dwPortId, InterfPtr& pIf )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg;
        auto pMgr = this->GetIoMgr();
        oCfg[ propIoMgr ] = ObjPtr( pMgr );
        oCfg[ propIsServer ] = true;
        oCfg.SetIntPtr( propStmHandle,
            ( guint32*& )hStream );
        oCfg.SetPointer( propParentPtr, this );
        std::string strDesc;
        CCfgOpenerObj oIfCfg( this );
        ret = oIfCfg.GetStrProp(
            propObjDescPath, strDesc );
        if( ERROR( ret ) )
            break;
        ret = CRpcServices::LoadObjDesc(
            strDesc,"AppMonitor_SvrSkel",
            true, oCfg.GetCfg() );
        if( ERROR( ret ) )
            break;
        oCfg.CopyProp( propSkelCtx, this );
        oCfg[ propPortId ] = dwPortId;
        ret = pIf.NewObj(
            clsid( CAppMonitor_SvrSkel ),
            oCfg.GetCfg() );
    }while( 0 );
    return ret;
}
gint32 CAppMonitor_SvrImpl::OnPreStart(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCtx;
        CCfgOpenerObj oIfCfg( this );
        oCtx[ propClsid ] = clsid( 
            CAppMonitor_ChannelSvr );
        oCtx.CopyProp( propObjDescPath, this );
        oCtx.CopyProp( propSvrInstName, this );
        stdstr strInstName;
        ret = oIfCfg.GetStrProp(
            propObjName, strInstName );
        if( ERROR( ret ) )
            break;
        oCtx[ 1 ] = strInstName;
        guint32 dwHash = 0;
        ret = GenStrHash( strInstName, dwHash );
        if( ERROR( ret ) )
            break;
        char szBuf[ 16 ];
        sprintf( szBuf, "_%08X", dwHash );
        strInstName = "AppMonitor_ChannelSvr";
        oCtx[ 0 ] = strInstName;
        strInstName += szBuf;
        oCtx[ propObjInstName ] = strInstName;
        oCtx[ propIsServer ] = true;
        oIfCfg.SetPointer( propSkelCtx,
            ( IConfigDb* )oCtx.GetCfg() );
        ret = super::OnPreStart( pCallback );
    }while( 0 );
    return ret;
}

bool CAppMonitor_SvrImpl::IsUserValid(
    guint32 dwUid ) const
{
    bool ret = false;
    CStdRMutex oLock( this->GetLock() );
    auto itr = m_mapUid2Gids.find( dwUid );
    if( itr != m_mapUid2Gids.cend() )
        ret = true;
    return ret;
}

gint32 CAppMonitor_SvrImpl::LoadUserGrpsMap()
{
    gint32 ret = 0;
    do{
        CRegistryFs* pfs = g_pUserRegfs;
        gint32 iFd = g_pUserRegfs->GetFd();
        CFlockHelper oFlock( iFd );
        RFHANDLE hDir = INVALID_HANDLE;
        ret = pfs->OpenDir( "/users", 0, hDir );
        if( ERROR( ret ) )
            break;
        std::vector<KEYPTR_SLOT> vecDirEnt;
        ret = pfs->ReadDir( hDir, vecDirEnt );
        if( ERROR( ret ) )
            break;
        if( vecDirEnt.empty() )
            break;
        pfs->CloseFile( hDir );
        for( auto& ks : vecDirEnt )
        {
            stdstr strPath = "/users/";
            strPath += ks.szKey;
            Variant oVar;
            ret = pfs->GetValue(
                strPath + "/uid", oVar );
            if( ERROR( ret ) )
                continue;

            // check if user disabled
            struct stat stbuf;
            ret = pfs->GetAttr(
                strPath + "/disabled", stbuf );
            if( SUCCEEDED( ret ) )
                continue;

            guint32 dwUid( oVar );
            std::vector< KEYPTR_SLOT > vecGrpEnt;
            IntSetPtr psetGids;
            psetGids.NewObj( clsid( CStlIntSet ) );
            strPath = "/users/";
            strPath += ks.szKey;
            strPath += "/groups";

            RFHANDLE hGrps = INVALID_HANDLE;
            ret = pfs->OpenDir( strPath, 0, hGrps );
            if( ERROR( ret ) )
                break;
            std::vector<KEYPTR_SLOT> vecGidEnt;
            ret = pfs->ReadDir( hGrps, vecGidEnt );
            if( ERROR( ret ) )
            {
                pfs->CloseFile( hGrps );
                continue;
            }
            for( auto& ks2 : vecGidEnt )
            {
                guint32 dwGid = std::strtol(
                    ks2.szKey, nullptr, 10 );
                ( *psetGids )().insert( dwGid );
            }
            ret = pfs->CloseFile( hGrps );
            CStdRMutex oLock( this->GetLock() );
            m_mapUid2Gids.insert(
                { dwUid, psetGids });
        }
    }while( 0 );
    return ret;
}

gint32 CAppMonitor_SvrImpl::OnPostStart(
    IEventSink* pCallback )
{
    TaskletPtr pTask = GetUpdateTokenTask();
    StartQpsTask( pTask );
    if( !pTask.IsEmpty() )
        AllocReqToken();

    gint32 ret = LoadUserGrpsMap();
    if( ERROR( ret ) )
        return ret;

    return super::OnPostStart( pCallback );
}

gint32 CAppMonitor_ChannelSvr::OnStreamReady(
    HANDLE hstm )
{
    gint32 ret = super::OnStreamReady( hstm );
    if( ERROR( ret ) )
        return ret;
    do{

        // add a uid to the login info
        gint32 iMech = 0;
        stdstr strName;
        if( true )
        {
            SESS_INFO osi;
            CReadLock oLock( this->GetSharedLock() );
            ret = GetSessInfo( hstm, osi );
            if( ERROR( ret ) )
                break;
            stdstr strAuth =
                osi.m_strSessHash.substr( 0, 4 );
            if( strAuth.substr( 0, 2 ) != "AU" )
                break;
            if( strAuth == "AUoa" )
                iMech = 1;
            else if( strAuth == "AUsa" )
                iMech = 2;

            CCfgOpener oCfg(
                ( IConfigDb* )osi.m_pLoginInfo );
            ret = oCfg.GetStrProp(
                propUserName, strName );
            if( ERROR( ret ) )
            {
                ret = -EACCES;
                break;
            }
        }
        CRegistryFs* pReg = g_pUserRegfs;
        gint32 iFd = pReg->GetFd();
        if( iFd < 0 )
        {
            ret = ERROR_STATE;
            break;
        }

        Variant oVar;
        stdstr strPath;
        if( true )
        {
            CFlockHelper oFlock( iFd );
            if( iMech == 0 )
            {
                strPath = "/krb5users/";
                strPath += strName;
                Variant oVar;
                ret = pReg->GetValue( strPath, oVar );
                if( ERROR( ret ) )
                {
                    ret = -EACCES;
                    break;
                }
            }
            else if( iMech == 1 )
            {
                strPath = "/oa2users/";
                strPath += strName;
                ret = pReg->GetValue( strPath, oVar );
                if( ERROR( ret ) )
                {
                    ret = -EACCES;
                    break;
                }
            }
            stdstr strUname( ( stdstr& )oVar );
            strPath = "/users/";
            strPath += strUname + "/uid";
            ret = pReg->GetValue( strPath, oVar );
            if( ERROR( ret ) )
                break;
        }
        guint32 dwUid( ( guint32& )oVar );
        if( true )
        {
            SESS_INFO osi;
            CWriteLock oLock( GetSharedLock() );
            ret = GetSessInfo( hstm, osi );
            if( ERROR( ret ) )
                break;
            CCfgOpener oCfg(
                ( IConfigDb* )osi.m_pLoginInfo );
            ret = oCfg.SetIntProp( propUid, dwUid );
            if( ERROR( ret ) )
            {
                ret = -EACCES;
                break;
            }
        }

    }while( 0 );
    return ret;
}
