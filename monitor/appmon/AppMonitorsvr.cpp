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
#include "AppManagersvr.h"
#include <fcntl.h>
#include <sys/file.h>
#include "monconst.h"

extern RegFsPtr g_pAppRegfs;
extern RegFsPtr g_pUserRegfs;

extern gint32 GetMonitorToNotify(
    CFastRpcServerBase* pIf,
    RegFsPtr pAppReg,
    const stdstr& strAppName,
    std::vector< HANDLE > vecStms );

extern gint32 GetCurStream(
    CFastRpcServerBase* pIf,
    IConfigDb* pReqCtx, HANDLE& hstm );

extern InterfPtr GetAppManager();

extern gint32 SplitPath(
    const stdstr& strPath,
    std::vector< stdstr >& vecComp );

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

CFileHandle::~CFileHandle()
{
    if( m_hFile != INVALID_HANDLE &&
        !m_pFs.IsEmpty() )
    {
        CRegistryFs* pfs = m_pFs;
        pfs->CloseFile( m_hFile );
        m_hFile = INVALID_HANDLE;
    }
    m_pFs.Clear();
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
        CAccessContext oac, *pac; \
        ret = GetAccessContext( ( _pContext ), oac ); \
        if( ret == -EACCES ) \
            break; \
        if( ret != -ENOENT && ERROR( ret ) ) \
            break; \
        else if( ret == -ENOENT ) pac = nullptr; \
        else pac = &oac;

/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::ListApps( 
    IConfigDb* pContext, 
    std::vector<std::string>& arrApps /*[ Out ]*/ )
{
    gint32 ret = 0;
    RFHANDLE hDir = INVALID_HANDLE;
    do{
        GETAC( pContext );
        stdstr strPath = "/" APPS_ROOT_DIR;
        std::vector< KEYPTR_SLOT > vecks;
        ret = m_pAppRegfs->OpenDir(
            strPath, O_RDONLY, hDir, pac );
        if( ERROR( ret ) )
            break;
        CFileHandle oHandle(
            m_pAppRegfs, hDir );
        ret = m_pAppRegfs->ReadDir(
            hDir, vecks );
        if( ERROR( ret ) )
            break;
        if( vecks.empty() )
        {
            ret = -ENOENT;
            break;
        }
        for( auto& elem : vecks )
        {
            ret = m_pAppRegfs->Access(
                hDir, elem.szKey,
                R_OK| X_OK, pac );
            if( ERROR( ret ) )
            {
                ret = 0;
                continue;
            }
            arrApps.push_back( elem.szKey );
        }
    }while( 0 );
    return ret;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::ListPoints( 
    IConfigDb* pContext, 
    const std::string& strAppPath /*[ In ]*/, 
    std::vector<std::string>& arrPoints /*[ Out ]*/ )
{
    gint32 ret = 0;
    RFHANDLE hDir = INVALID_HANDLE;
    do{
        if( strAppPath.empty() )
        {
            ret = -EINVAL;
            break;
        }
        GETAC( pContext );
        stdstr strPath = "/" APPS_ROOT_DIR "/";
        strPath+= strAppPath;
        std::vector< KEYPTR_SLOT > vecks;
        ret = m_pAppRegfs->OpenDir(
            strPath, O_RDONLY, hDir, pac );
        if( ERROR( ret ) )
            break;
        CFileHandle oHandle(
            m_pAppRegfs, hDir );
        ret = m_pAppRegfs->ReadDir(
            hDir, vecks );
        if( ERROR( ret ) )
            break;
        if( vecks.empty() )
        {
            ret = -ENOENT;
            break;
        }
        for( auto& elem : vecks )
        {
            ret = m_pAppRegfs->Access(
                hDir, elem.szKey,
                R_OK | X_OK, pac );
            if( ERROR( ret ) )
            {
                ret = 0;
                continue;
            }
            arrPoints.push_back( elem.szKey );
        }
    }while( 0 );
    return ret;
}

/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::ListAttributes( 
    IConfigDb* pContext, 
    const std::string& strPtPath /*[ In ]*/, 
    std::vector<std::string>& arrAttributes /*[ Out ]*/ )
{
    gint32 ret = 0;
    RFHANDLE hDir = INVALID_HANDLE;
    do{
        if( strPtPath.empty() )
        {
            ret = -EINVAL;
            break;
        }
        GETAC( pContext );
        stdstr strPath = "/" APPS_ROOT_DIR "/";
        strPath += strPtPath;
        std::vector< KEYPTR_SLOT > vecks;
        ret = m_pAppRegfs->OpenDir(
            strPath, O_RDONLY, hDir, pac );
        if( ERROR( ret ) )
            break;
        CFileHandle oHandle(
            m_pAppRegfs, hDir );
        ret = m_pAppRegfs->ReadDir(
            hDir, vecks );
        if( ERROR( ret ) )
            break;
        if( vecks.empty() )
        {
            ret = -ENOENT;
            break;
        }
        for( auto& elem : vecks )
            arrAttributes.push_back( elem.szKey );
    }while( 0 );
    return ret;
}

bool CAppMonitor_SvrImpl::IsAppRegistered(
    HANDLE hstm, const stdstr& strApp ) const
{
    bool ret = false;
    if( hstm == INVALID_HANDLE )
        return ret;
    do{
        CStdRMutex oLock( GetLock() );
        auto itr = m_mapListeners.find( hstm );
        if( itr == m_mapListeners.end() )
            break;
        StrSetPtr pStrSet = itr->second;
        auto itr2 = ( *pStrSet )().find( strApp );
        if( itr2 != ( *pStrSet )().end() )
            ret = true;
    }while( 0 );
    return ret;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::SetPointValue( 
    IConfigDb* pContext, 
    const std::string& strPtPath /*[ In ]*/,
    const Variant& value /*[ In ]*/ )
{
    gint32 ret = 0;
    do{
        GETAC( pContext );
        stdstr strPath = "/" APPS_ROOT_DIR "/";
        Variant oOrigin;

        std::vector< stdstr > vecComps;
        ret = SplitPath( strPtPath, vecComps );
        if( ERROR( ret ) )
        {
            ret = -EINVAL;
            break;
        }

        HANDLE hcurStm = INVALID_HANDLE;
        GetCurStream( this, pContext, hcurStm );
        const stdstr& strApp = vecComps[ 0 ];
        const stdstr& strPoint = vecComps[ 1 ];

        if( !IsAppRegistered( hcurStm, strApp ) )
            break;

        RFHANDLE hPtDir;
        ret = m_pAppRegfs->OpenDir( strPath +
            strApp + "/" POINTS_DIR "/" + strPoint,
            O_RDONLY, hPtDir, pac );
        if( ERROR( ret ) )
            break;

        CFileHandle oHandle( m_pAppRegfs, hPtDir );
        ret = m_pAppRegfs->Access( hPtDir, 
            OUTPUT_PULSE, F_OK, pac );
        if( ERROR( ret ) )
        {
            ret = m_pAppRegfs->GetValue( hPtDir,
                VALUE_FILE, oOrigin );
            if( SUCCEEDED( ret ) &&
                oOrigin == value )
                break;
            ret = 0;
        }
        ret = m_pAppRegfs->SetValue(
            hPtDir, VALUE_FILE, value, pac );
        if( ERROR( ret ) )
            break;

        CAppManager_SvrImpl* pm = GetAppManager();
        pm->NotifyValChange(
            strPtPath, value, hcurStm );

        this->OnPointChangedInternal(
            strPtPath, value, hcurStm );

    }while( 0 );
    return ret;
}

/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::GetPointValue( 
    IConfigDb* pContext, 
    const std::string& strPtPath /*[ In ]*/, 
    Variant& rvalue /*[ Out ]*/ )
{
    gint32 ret = 0;
    do{
        stdstr strAttrPath =
            strPtPath + "/" VALUE_FILE;
        ret = GetAttrValue(
            pContext, strAttrPath, rvalue );
    }while( 0 );
    return ret;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::SetAttrValue( 
    IConfigDb* pContext, 
    const std::string& strAttrPath /*[ In ]*/,
    const Variant& value /*[ In ]*/ )
{
    gint32 ret = 0;
    do{
        GETAC( pContext );
        stdstr strPath = "/" APPS_ROOT_DIR "/";

        std::vector< stdstr > vecComps;
        ret = SplitPath( strAttrPath, vecComps );
        if( ERROR( ret ) )
        {
            ret = -EINVAL;
            break;
        }
        const stdstr& strApp = vecComps[ 0 ];
        const stdstr& strPoint = vecComps[ 1 ];
        const stdstr& strAttr = vecComps[ 2 ];
        stdstr strAttrVal =  strPath + strApp +
            "/" POINTS_DIR "/" +
            strPoint + "/" + strAttr; 
        ret = m_pAppRegfs->SetValue(
            strAttrVal, value, pac );
    }while( 0 );
    return ret;
}

/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::GetAttrValue( 
    IConfigDb* pContext, 
    const std::string& strAttrPath /*[ In ]*/, 
    Variant& rvalue /*[ Out ]*/ )
{
    gint32 ret = 0;
    do{
        GETAC( pContext );
        stdstr strPath = "/" APPS_ROOT_DIR "/";

        std::vector< stdstr > vecComps;
        ret = SplitPath( strAttrPath, vecComps );
        if( ERROR( ret ) )
        {
            ret = -EINVAL;
            break;
        }
        const stdstr& strApp = vecComps[ 0 ];
        const stdstr& strPoint = vecComps[ 1 ];
        const stdstr& strAttr = vecComps[ 2 ];
        stdstr strAttrVal =  strPath + strApp +
            "/" POINTS_DIR "/" +
            strPoint + "/" + strAttr; 
        ret = m_pAppRegfs->SetValue(
            strAttrVal, rvalue, pac );
    }while( 0 );
    return ret;
}

/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::SetPointValues( 
    IConfigDb* pContext, 
    std::vector<KeyValue>& arrValues /*[ In ]*/ )
{
    gint32 ret = 0;
    do{
        guint32 dwCount = 0;
        for( auto& elem : arrValues )
        {
            ret = SetPointValue( pContext,
                elem.strKey, elem.oValue );
            if( ERROR( ret ) )
                break;
        }
    }while( 0 );
    return ret;
}

/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::GetPointValues( 
    IConfigDb* pContext, 
    std::vector<std::string>& arrPtPaths /*[ In ]*/, 
    std::vector<KeyValue>& arrKeyVals /*[ Out ]*/ )
{
    gint32 ret = 0;
    do{
        for( auto& elem : arrPtPaths )
        {
            Variant rval;
            ret = GetPointValue( pContext, elem, rval );
            if( SUCCEEDED( ret ) )
            {
                KeyValue okv;
                okv.strKey = elem;
                okv.oValue = rval;
                arrKeyVals.push_back( okv );
            }
        }
    }while( 0 );
    if( arrKeyVals.empty() )
        return -ENOENT;
    return ret;
}

/* RPC event sender */
gint32 CAppMonitor_SvrImpl::OnPointChangedInternal(
    const std::string& strPtPath /*[ In ]*/,
    const Variant& value /*[ In ]*/,
    HANDLE hcurStm )
{
    gint32 ret = 0;
    do{
        auto pos =
            strPtPath.find_first_of( '/' );
        if( pos == stdstr::npos )
        {
            ret = -EINVAL;
            break;
        }
        stdstr strApp =
            strPtPath.substr( 0, pos );

        // notify the monitors
        std::vector< HANDLE > vecStms;
        ret = GetMonitorToNotify( this,
            m_pAppRegfs, strApp, vecStms );
        if( ERROR( ret ) )
            break;

        for( auto& elem : vecStms )
        {
            if( hcurStm == elem )
                continue;
            InterfPtr ptrSkel;
            gint32 iRet =
                GetStmSkel( elem, ptrSkel );
            if( ERROR( iRet ) )
                break;
            CAppMonitor_SvrSkel* pSkel = ptrSkel;
            pSkel->IIAppStore_SImpl::OnPointChanged(
                strPtPath, value );
        }
    }while( 0 );
    return ret;
}

/* RPC event sender */
gint32 CAppMonitor_SvrImpl::OnPointsChangedInternal(
    std::vector<KeyValue>& arrKVs /*[ In ]*/,
    HANDLE hcurStm )
{
    std::vector< InterfPtr > vecSkels;
    gint32 ret = this->EnumStmSkels( vecSkels );
    if( ERROR( ret ) )
        return ret;
    for( auto& elem : vecSkels )
    {
        CAppMonitor_SvrSkel* pSkel = elem;
        ret = pSkel->IIAppStore_SImpl::OnPointsChanged(
            arrKVs );
    }
    return ret;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::RegisterListener( 
    IConfigDb* pContext, 
    std::vector<std::string>& arrApps /*[ In ]*/ )
{
    gint32 ret = 0;
    do{
        HANDLE hstm;
        ret = GetCurStream( this, pContext, hstm );
        if( ERROR( ret ) )
            break;

        GETAC( pContext );
        stdstr strPath = "/" APPS_ROOT_DIR;
        std::vector< KEYPTR_SLOT > vecks;
        RFHANDLE hDir = INVALID_HANDLE;
        ret = m_pAppRegfs->OpenDir(
            strPath, O_RDONLY, hDir, pac );
        if( ERROR( ret ) )
            break;
        CFileHandle oHandle(
            m_pAppRegfs, hDir );

        std::vector< stdstr > vecValidApps;
        auto itr1 = arrApps.begin();
        while( itr1 != arrApps.end() )
        {
            ret = m_pAppRegfs->Access(
                hDir, itr1->c_str(),
                R_OK | X_OK, pac );
            if( ERROR( ret ) )
            {
                ret = 0;
                continue;
            }
            vecValidApps.push_back( *itr1 );
        }

        stdstr strStmFile = std::to_string( hstm );
        for( auto& elem : vecValidApps )
        {
            stdstr strAppPath =
                strPath + "/" + elem;
            stdstr strStmPath = strAppPath + "/" 
                MONITOR_STREAM_DIR + "/" +
                strStmFile;
            guint32 dwUid = 0, dwGid = 0;
            ret = m_pAppRegfs->GetUid(
                strAppPath, dwUid );
            if( ERROR( ret ) )
                continue;
            ret = m_pAppRegfs->GetGid(
                strAppPath, dwGid );
            if( ERROR( ret ) )
                continue;
            HANDLE hFile = INVALID_HANDLE;
            ret = m_pAppRegfs->CreateFile(
                strStmFile, 0640, O_RDWR,
                hFile );
            if( ERROR( ret ) )
                continue;

            CFileHandle oHandle(
                m_pAppRegfs, hFile );
            m_pAppRegfs->SetUid( hFile, dwUid );
            m_pAppRegfs->SetGid( hFile, dwGid );
        }
        ret = 0;
        CStdRMutex oLock( GetLock() );
        auto itr = m_mapListeners.find( hstm );
        StrSetPtr pStrSet;
        if( itr == m_mapListeners.end() )
        {
            ret = pStrSet.NewObj();
            if( ERROR( ret ) )
                break;
            m_mapListeners.insert(
                { hstm, pStrSet } );
        }
        else
        {
            pStrSet = itr->second;
        }
        for( auto& elem : vecValidApps )
            ( *pStrSet )().insert( elem );

    }while( 0 );
    return ret;
}

gint32 CAppMonitor_SvrImpl::RemoveListenerInternal(
    HANDLE hstm, CAccessContext* pac )
{
    gint32 ret = 0;
    do{
        stdstr strPath = "/" APPS_ROOT_DIR;

        RFHANDLE hDir = INVALID_HANDLE;
        std::vector< KEYPTR_SLOT > vecks;
        ret = m_pAppRegfs->OpenDir(
            strPath, O_RDONLY, hDir, pac );
        if( ERROR( ret ) )
            break;
        CFileHandle oHandle(
            m_pAppRegfs, hDir );

        CStdRMutex oLock( GetLock() );
        auto itr = m_mapListeners.find( hstm );
        StrSetPtr pStrSet;
        if( itr == m_mapListeners.end() )
            break;
        pStrSet = itr->second;
        m_mapListeners.erase( itr );
        oLock.Unlock();

        stdstr strStmFile = std::to_string( hstm );
        for( auto& elem : ( *pStrSet )() )
        {
            stdstr strAppPath =
                strPath + "/" + elem;
            stdstr strStmPath = strAppPath + "/" 
                MONITOR_STREAM_DIR + "/" +
                strStmFile;
            ret = m_pAppRegfs->RemoveFile(
                strStmFile, pac );
            if( ERROR( ret ) )
                DebugPrint( ret, "Error, filed to "
                    "remove listener %s",
                    strStmFile );
        }
    }while( 0 );
    return ret;
}

/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::RemoveListener( 
    IConfigDb* pContext )
{
    gint32 ret = 0;
    do{
        HANDLE hstm;
        ret = GetCurStream( this, pContext, hstm );
        if( ERROR( ret ) )
            break;

        GETAC( pContext );
        ret = RemoveListenerInternal( hstm, pac );

    }while( 0 );
    return ret;
}

gint32 CAppMonitor_SvrImpl::RemoveStmSkel(
    HANDLE hstm )
{
    gint32 ret = 0;
    do{
        RemoveListenerInternal( hstm, nullptr );
        ret = super::RemoveStmSkel( hstm );
    }while( 0 );
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
        ret = pfs->OpenDir( "/" USER_ROOT_DIR, 0, hDir );
        if( ERROR( ret ) )
            break;
        CFileHandle oHandle( pfs, hDir );
        std::vector<KEYPTR_SLOT> vecDirEnt;
        ret = pfs->ReadDir( hDir, vecDirEnt );
        if( ERROR( ret ) )
            break;
        if( vecDirEnt.empty() )
            break;
        for( auto& ks : vecDirEnt )
        {
            stdstr strPath = "/" USER_ROOT_DIR "/";
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
            strPath = "/" USER_ROOT_DIR "/";
            strPath += ks.szKey;
            strPath += "/groups";

            RFHANDLE hGrps = INVALID_HANDLE;
            ret = pfs->OpenDir( strPath, 0, hGrps );
            if( ERROR( ret ) )
                break;
            CFileHandle oHandle( pfs, hGrps );
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

