/****BACKUP YOUR CODE BEFORE RUNNING RIDLC***/
// ../../ridl/.libs/ridlc -sO . ./appmon.ridl 
// Implement the following methods
// to get the RPC proxy/server work
#include "rpc.h"
using namespace rpcf;
#include "stmport.h"
#include "fastrpc.h"
#include "appmon.h"
#include "AppMonitorsvr.h"
#include <fcntl.h>
#include <sys/file.h>
#include "blkalloc.h"

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

/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::CreateFile( 
    IConfigDb* pContext, 
    const std::string& strPath /*[ In ]*/,
    guint32 dwMode /*[ In ]*/,
    guint32 dwFlags /*[ In ]*/, 
    guint64& hFile /*[ Out ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'CreateFileComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::MakeDir( 
    IConfigDb* pContext, 
    const std::string& strPath /*[ In ]*/,
    guint32 dwMode /*[ In ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'MakeDirComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::OpenFile( 
    IConfigDb* pContext, 
    const std::string& strPath /*[ In ]*/,
    guint32 dwFlags /*[ In ]*/, 
    guint64& hFile /*[ Out ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'OpenFileComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::CloseFile( 
    IConfigDb* pContext, 
    guint64 hFile /*[ In ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'CloseFileComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::RemoveFile( 
    IConfigDb* pContext, 
    const std::string& strPath /*[ In ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'RemoveFileComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::ReadFile( 
    IConfigDb* pContext, 
    guint64 hFile /*[ In ]*/,
    guint32 dwSize /*[ In ]*/,
    guint32 dwOff /*[ In ]*/, 
    BufPtr& buffer /*[ Out ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'ReadFileComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::WriteFile( 
    IConfigDb* pContext, 
    guint64 hFile /*[ In ]*/,
    BufPtr& buffer /*[ In ]*/,
    guint32 dwOff /*[ In ]*/, 
    guint32& dwSizeWrite /*[ Out ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'WriteFileComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::Truncate( 
    IConfigDb* pContext, 
    guint64 hFile /*[ In ]*/,
    guint32 dwOff /*[ In ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'TruncateComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::CloseDir( 
    IConfigDb* pContext, 
    guint64 hFile /*[ In ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'CloseDirComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::RemoveDir( 
    IConfigDb* pContext, 
    const std::string& strPath /*[ In ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'RemoveDirComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::SetGid( 
    IConfigDb* pContext, 
    const std::string& strPath /*[ In ]*/,
    guint32 wGid /*[ In ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'SetGidComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::SetUid( 
    IConfigDb* pContext, 
    const std::string& strPath /*[ In ]*/,
    guint32 wUid /*[ In ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'SetUidComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::GetGid( 
    IConfigDb* pContext, 
    const std::string& strPath /*[ In ]*/, 
    guint32& gid /*[ Out ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'GetGidComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::GetUid( 
    IConfigDb* pContext, 
    const std::string& strPath /*[ In ]*/, 
    guint32& uid /*[ Out ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'GetUidComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::SymLink( 
    IConfigDb* pContext, 
    const std::string& strSrcPath /*[ In ]*/,
    const std::string& strDestPath /*[ In ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'SymLinkComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::GetValue( 
    IConfigDb* pContext, 
    const std::string& strPath /*[ In ]*/, 
    std::string& strJson /*[ Out ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'GetValueComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::SetValue( 
    IConfigDb* pContext, 
    const std::string& strPath /*[ In ]*/,
    const std::string& strJson /*[ In ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'SetValueComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::Chmod( 
    IConfigDb* pContext, 
    const std::string& strPath /*[ In ]*/,
    guint32 dwMode /*[ In ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'ChmodComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::Chown( 
    IConfigDb* pContext, 
    const std::string& strPath /*[ In ]*/,
    guint32 dwUid /*[ In ]*/,
    guint32 dwGid /*[ In ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'ChownComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::ReadLink( 
    IConfigDb* pContext, 
    const std::string& strPath /*[ In ]*/, 
    BufPtr& buf /*[ Out ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'ReadLinkComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::Rename( 
    IConfigDb* pContext, 
    const std::string& szFrom /*[ In ]*/,
    const std::string& szTo /*[ In ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'RenameComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::Flush( 
    IConfigDb* pContext, 
    guint32 dwFlags /*[ In ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'FlushComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::Access( 
    IConfigDb* pContext, 
    const std::string& strPath /*[ In ]*/,
    guint32 dwFlags /*[ In ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'AccessComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::GetAttr( 
    IConfigDb* pContext, 
    const std::string& strPath /*[ In ]*/, 
    FileStat& oStat /*[ Out ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'GetAttrComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::ReadDir( 
    IConfigDb* pContext, 
    guint64 hDir /*[ In ]*/, 
    std::vector<FileStat>& vecDirEnt /*[ Out ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'ReadDirComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::OpenDir( 
    IConfigDb* pContext, 
    const std::string& strPath /*[ In ]*/,
    guint32 dwFlags /*[ In ]*/, 
    guint64& hDir /*[ Out ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'OpenDirComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
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
