/****BACKUP YOUR CODE BEFORE RUNNING RIDLC***/
// ../../ridl/.libs/ridlc --server -sO . ./appmon.ridl 
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
    std::vector< HANDLE >& vecStms );

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
    gint32 ret = 0;
    Variant oVar;
    do{
        ret = pInfo->GetProperty(
            propAuthMech, oVar );
        if( ERROR( ret ) )
            break;
        stdstr strAuthMech = oVar;
        ret = pInfo->GetProperty(
            propUserName, oVar );
        if( ERROR( ret ) )
            break;
        stdstr strUser = oVar;
        CStdRMutex oLock( this->GetLock() );
        if( strAuthMech == "SimpAuth" )
        {
            auto itr =
                m_mapName2Uid.find( strUser );
            if( itr == m_mapName2Uid.end() )
            {
                ret = -EACCES;
                break;
            }
            dwUid = itr->second;
        }
        else if( strAuthMech == "krb5" )
        {
            auto itr =
                m_mapKrb5Name2Uid.find( strUser );
            if( itr == m_mapKrb5Name2Uid.end() )
            {
                ret = -EACCES;
                break;
            }
            dwUid = itr->second;
        }
        else if( strAuthMech == "OAuth2" )
        {
            auto itr =
                m_mapOA2Name2Uid.find( strUser );
            if( itr == m_mapOA2Name2Uid.end() )
            {
                ret = -EACCES;
                break;
            }
            dwUid = itr->second;
        }
    }while( 0 );
    return ret;
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

gint32 CAppMonitor_SvrImpl::GetAccessContext(
    HANDLE hstm, CAccessContext& oac ) const
{
    gint32 ret = 0;
    do{
        CfgPtr pInfo;
        InterfPtr pIf;
        ret = GetStmChanSvr( pIf );
        if( ERROR( ret ) )
            break;
        CRpcStmChanSvr* pSvr = pIf;
        SESS_INFO si;
        ret = pSvr->GetSessInfo( hstm, si );
        if( ERROR( ret ) )
            break;

        guint32 dwUid = 0;
        ret = GetUid( si.m_pLoginInfo, dwUid );
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

bool CAppMonitor_SvrImpl::IsAppSubscribed(
    HANDLE hstm, const stdstr& strApp ) const
{
    return true;
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

        if( !IsAppSubscribed( hcurStm, strApp ) )
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
        EnumTypeId iType = value.GetTypeId();
        if( iType < typeDMsg )
        {
            ret = m_pAppRegfs->SetValue(
                hPtDir, VALUE_FILE, value, pac );
            if( ERROR( ret ) )
                break;
        }
        else if( iType == typeByteArr )
        {
            RFHANDLE hFile;
            ret = m_pAppRegfs->OpenFile(
                VALUE_FILE, O_WRONLY | O_TRUNC,
                hFile, pac );
            if( ERROR( ret ) )
                break;
            CFileHandle oh2( m_pAppRegfs, hFile );
            BufPtr pBuf = ( BufPtr& )value;
            if( pBuf.IsEmpty() || pBuf->empty() )
                break;
            guint32 dwSize = pBuf->size();
            if( dwSize > MAX_FILE_SIZE )
            {
                ret = -EINVAL;
                break;
            }
            ret = m_pAppRegfs->WriteFile( hFile,
                pBuf->ptr(), dwSize, 0 );
        }
        else
        {
            ret = -ENOTSUP;
            break;
        }

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
gint32 CAppMonitor_SvrImpl::SetLargePointValue( 
    IConfigDb* pContext, 
    const std::string& strPtPath /*[ In ]*/,
    BufPtr& value /*[ In ]*/ )
{
    gint32 ret = 0;
    if( value.IsEmpty() || value->empty() )
        return -EINVAL;
    do{
        GETAC( pContext );
        stdstr strPath = "/" APPS_ROOT_DIR "/";
        std::vector< stdstr > vecComps;
        ret = SplitPath( strPtPath, vecComps );
        if( ERROR( ret ) )
        {
            ret = -EINVAL;
            break;
        }
        const stdstr& strApp = vecComps[ 0 ];
        const stdstr& strPoint = vecComps[ 1 ];

        HANDLE hcurStm = INVALID_HANDLE;
        GetCurStream( this, pContext, hcurStm );
        if( !IsAppSubscribed( hcurStm, strApp ) )
        {
            ret = -EACCES;
            break;
        }
        stdstr strAttrVal =  strPath + strApp +
            "/" POINTS_DIR "/" +
            strPoint + "/" VALUE_FILE; 
        RFHANDLE hFile;
        ret = m_pAppRegfs->OpenFile(
            strAttrVal, O_WRONLY | O_TRUNC,
            hFile, pac );
        if( ERROR( ret ) )
            break;
        CFileHandle oHandle( m_pAppRegfs, hFile );
        guint32 dwSize = value->size();
        if( dwSize == 0 )
            break;
        if( dwSize > MAX_FILE_SIZE )
        {
            ret = -EINVAL;
            break;
        }
        ret = m_pAppRegfs->WriteFile( hFile,
            value->ptr(), dwSize, 0 );
    }while( 0 );
    return ret;
}

/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::GetLargePointValue( 
    IConfigDb* pContext, 
    const std::string& strPtPath /*[ In ]*/, 
    BufPtr& value /*[ Out ]*/ )
{
    gint32 ret = 0;
    if( value.IsEmpty() )
        value.NewObj();
    do{
        GETAC( pContext );
        stdstr strPath = "/" APPS_ROOT_DIR "/";
        std::vector< stdstr > vecComps;
        ret = SplitPath( strPtPath, vecComps );
        if( ERROR( ret ) )
        {
            ret = -EINVAL;
            break;
        }
        if( vecComps.size() > 2 )
        {
            ret = -EINVAL;
            break;
        }

        HANDLE hcurStm = INVALID_HANDLE;
        GetCurStream( this, pContext, hcurStm );
        const stdstr& strApp = vecComps[ 0 ];
        const stdstr& strPoint = vecComps[ 1 ];

        if( !IsAppSubscribed( hcurStm, strApp ) )
        {
            ret = -EACCES;
            break;
        }

        stdstr strAttrVal =  strPath + strApp +
            "/" POINTS_DIR "/" +
            strPoint + "/" VALUE_FILE; 
        RFHANDLE hFile;
        ret = m_pAppRegfs->OpenFile(
            strAttrVal, O_RDONLY, hFile, pac );
        if( ERROR( ret ) )
            break;
        CFileHandle oHandle( m_pAppRegfs, hFile );
        struct stat st;
        ret = m_pAppRegfs->GetAttr( hFile, st );
        if( ERROR( ret ) )
            break;
        guint32 dwSize = st.st_size;
        if( dwSize == 0 )
        {
            ret = -ENODATA;
            break;
        }
        ret = value->Resize( dwSize );
        if( ERROR( ret ) )
            break;
        ret = m_pAppRegfs->ReadFile( hFile,
            value->ptr(), dwSize, 0 );
        if( dwSize < st.st_size )
            ret = value->Resize( dwSize );
    }while( 0 );
    return ret;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::SubscribeStreamPoint( 
    IConfigDb* pContext, 
    const std::string& strPtPath /*[ In ]*/,
    HANDLE hstm_h /*[ In ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'SubscribeStreamPointComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
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

        HANDLE hcurStm = INVALID_HANDLE;
        GetCurStream( this, pContext, hcurStm );
        if( !IsAppSubscribed( hcurStm, strApp ) )
        {
            ret = -EACCES;
            break;
        }
        stdstr strAttrVal =  strPath + strApp +
            "/" POINTS_DIR "/" +
            strPoint + "/" + strAttr; 
        ret = m_pAppRegfs->GetValue(
            strAttrVal, rvalue, pac );
    }while( 0 );
    return ret;
}

/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::SetPointValues( 
    IConfigDb* pContext, 
    const std::string& strAppName /*[ In ]*/,
    std::vector<KeyValue>& arrValues /*[ In ]*/ )
{
    gint32 ret = 0;
    do{
        GETAC( pContext );
        auto& pfs = m_pAppRegfs;
        HANDLE hcurStm = INVALID_HANDLE;
        GetCurStream( this, pContext, hcurStm );
        if( !IsAppSubscribed( hcurStm, strAppName ) )
        {
            ret = -EACCES;
            break;
        }

        RFHANDLE hPtDir = INVALID_HANDLE;
        stdstr strDir = "/" APPS_ROOT_DIR "/";
        strDir += strAppName + "/" POINTS_DIR "/";

        ret = pfs->OpenDir( strDir,
            O_RDONLY, hPtDir, pac );
        if( ERROR( ret ) )
            break;

        CFileHandle oHandle( pfs, hPtDir );

        for( auto& elem : arrValues )
        {
            auto iType = elem.oValue.GetTypeId();
            if( iType < typeDMsg )
            {
                stdstr strPtName =
                    strAppName + "/" + elem.strKey;
                ret = SetPointValue( pContext,
                    strPtName, elem.oValue );
            }
            else if( iType == typeByteArr )
            do{
                BufPtr& pBuf = elem.oValue;
                guint32 dwSize = pBuf->size();
                if( dwSize > MAX_FILE_SIZE )
                {
                    ret = -ERANGE;
                    break;
                }

                stdstr strPath =
                    elem.strKey + "/" VALUE_FILE;

                RFHANDLE hFile;
                ret = pfs->OpenFile( hPtDir,
                    strPath, O_WRONLY | O_TRUNC,
                    hFile, pac );
                if( ERROR( ret ) )
                    break;
                CFileHandle ofh( pfs, hFile );
                if( iType != typeByteArr )
                {
                    ret = -ENOTSUP;
                    break;
                }
                ret = pfs->WriteFile( hFile,
                    pBuf->ptr(), dwSize, 0 );
            }while( 0 );
            else
            {
                ret = -ENOTSUP;
            }
            if( ERROR( ret ) )
            {
                OutputMsg( ret,
                    "Error SetPointValue %s/%s",
                    strAppName.c_str(),
                    elem.strKey.c_str() );
            }
        }
    }while( 0 );
    return ret;
}

/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::GetPointValues( 
    IConfigDb* pContext, 
    const std::string& strAppName /*[ In ]*/,
    std::vector<std::string>& arrPtPaths /*[ In ]*/, 
    std::vector<KeyValue>& arrKeyVals /*[ Out ]*/ )
{
    gint32 ret = 0;
    auto& pfs = m_pAppRegfs;
    do{
        if( strAppName.empty() )
        {
            ret = -EINVAL;
            break;
        }
        if( strAppName != "none" &&
            strAppName != "None" )
        {
            GetPointValuesInternal(
                pContext, strAppName,
                arrPtPaths, arrKeyVals, true );
            break;
        }
        std::hashmap< stdstr, std::vector< stdstr > > mapAppPts;
        for( auto& elem : arrPtPaths )
        {
            std::vector< stdstr > vecComps;
            ret = SplitPath( elem, vecComps );
            if( ERROR( ret ) )
            {
                ret = -EINVAL;
                break;
            }

            const stdstr& strApp = vecComps[ 0 ];
            const stdstr& strPoint = vecComps[ 1 ];

            auto itr = mapAppPts.find( strApp );
            if( itr == mapAppPts.end() )
            {
                std::vector< stdstr > vecPts;
                vecPts.push_back( strPoint );
                mapAppPts[ strApp ] = vecPts;
            }
            else
            {
                itr->second.push_back( strPoint );
            }
        }

        for( auto& elem : mapAppPts )
        {
            GetPointValuesInternal(
                pContext, elem.first,
                elem.second, arrKeyVals, false );
        }

    }while( 0 );
    return ret;
}
    
gint32 CAppMonitor_SvrImpl::GetPointValuesInternal ( 
    IConfigDb* pContext, 
    const std::string& strAppName /*[ In ]*/,
    std::vector<std::string>& arrPtPaths /*[ In ]*/, 
    std::vector<KeyValue>& arrKeyVals /*[ Out ]*/,
    bool bShortKey )
{
    gint32 ret = 0;
    do{
        GETAC( pContext );
        auto& pfs = m_pAppRegfs;
        HANDLE hcurStm = INVALID_HANDLE;
        GetCurStream( this, pContext, hcurStm );
        if( !IsAppSubscribed( hcurStm, strAppName ) )
        {
            ret = -EACCES;
            break;
        }
        RFHANDLE hPtDir = INVALID_HANDLE;
        stdstr strDir = "/" APPS_ROOT_DIR "/";
        strDir += strAppName + "/" POINTS_DIR "/";

        ret = pfs->OpenDir(
            strDir, O_RDONLY, hPtDir, pac );
        if( ERROR( ret ) )
            break;
        CFileHandle oHandle( pfs, hPtDir );
        for( auto& elem : arrPtPaths )
        {
            stdstr strFile =
                elem + "/" VALUE_FILE;

            KeyValue kv;
            if( bShortKey )
                kv.strKey = elem;
            else
                kv.strKey =
                    strAppName + "/" + elem;

            stdstr strType =
                elem + "/" DATA_TYPE;
            Variant dt;
            ret = pfs->GetValue(
                hPtDir, strType, dt );
            if( ERROR( ret ) )
            {
                OutputMsg( ret,
                    "Error get data type %s/%s",
                    strAppName.c_str(),
                    elem.c_str() );
                continue;
            }
            auto iType = ( guint32& )dt;
            if( iType < typeDMsg )
            {
                ret = pfs->GetValue( hPtDir,
                    strFile, kv.oValue );
            }
            else if( iType == typeByteArr )
            do{
                RFHANDLE hFile;
                ret = pfs->OpenFile( hPtDir,
                    strFile, O_RDONLY,
                    hFile, pac );
                if( ERROR( ret ) )
                    break;
                CFileHandle ofh( pfs, hFile );
                BufPtr pBuf( true );

                struct stat st;
                ret = pfs->GetAttr( hFile, st );
                if( ERROR( ret ) )
                    break;
                guint32 dwSize = st.st_size;
                if( dwSize == 0 )
                {
                    ret = -ENODATA;
                    break;
                }
                ret = pBuf->Resize( dwSize );
                if( ERROR( ret ) )
                    break;
                ret = pfs->ReadFile( hFile,
                    pBuf->ptr(), dwSize, 0 );
                if( ERROR( ret ) )
                    break;
                kv.oValue = pBuf;
            }while( 0 );
            else
            {
                ret = -ENOTSUP;
            }
            if( ERROR( ret ) )
            {
                OutputMsg( ret,
                    "Error GetPointValue %s/%s",
                    strAppName.c_str(),
                    elem.c_str() );
                continue;
            }
            arrKeyVals.push_back( kv );
        }
        ret = 0;
    }while( 0 );
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
        std::vector< stdstr > vecComps;
        ret = SplitPath( strPtPath, vecComps );
        if( ERROR( ret ) )
        {
            ret = -EINVAL;
            break;
        }
        if( vecComps.size() < 2 )
        {
            ret = -EINVAL;
            break;
        }

        const stdstr& strApp = vecComps[ 0 ];
        const stdstr& strPt = vecComps[ 1 ];

        // notify the other monitors
        std::vector< HANDLE > vecStms;
        ret = GetMonitorToNotify( this,
            m_pAppRegfs, strApp, vecStms );
        if( ERROR( ret ) )
            break;

        for( auto& elem : vecStms )
        {
            if( hcurStm == elem )
                continue;

            CAccessContext oac;
            ret = GetAccessContext( elem, oac );
            if( ERROR( ret ) )
                continue;
            stdstr strValPath = "/" APPS_ROOT_DIR "/";
            strValPath += strApp +
                "/" POINTS_DIR "/" + strPt +
                "/" VALUE_FILE;

            ret = m_pAppRegfs->Access(
                strValPath, R_OK, &oac );
            if( ERROR( ret ) )
                continue;

            InterfPtr ptrSkel;
            gint32 iRet =
                GetStmSkel( elem, ptrSkel );
            if( ERROR( iRet ) )
                continue;
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
gint32 CAppMonitor_SvrImpl::IsAppOnline( 
    IConfigDb* pContext, 
    std::vector<std::string>& strApps /*[ In ]*/, 
    std::vector<std::string>& strOnlineApps /*[ Out ]*/ )
{
    gint32 ret = 0;
    do{
        InterfPtr pIf = GetAppManager();
        CAppManager_SvrImpl* pm = pIf;
        if( pm == nullptr )
        {
            ret = ERROR_STATE;
            break;
        }
        if( strApps.size() > 20 )
        {
            ret = -E2BIG;
            break;
        }
        for( auto& elem : strApps )
        {
            if( pm->IsAppOnline( elem ) )
                strOnlineApps.push_back( elem );
        }
    }while( 0 );
    return ret;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::GetPointDesc( 
    IConfigDb* pContext, 
    std::vector<std::string>& arrPtPaths /*[ In ]*/, 
    std::map<std::string,ObjPtr>& mapPtDescs /*[ Out ]*/ )
{
    gint32 ret = 0;
    do{
        std::hashmap< stdstr, std::vector< stdstr > > mapAppPts;
        for( auto& elem : arrPtPaths )
        {
            std::vector< stdstr > vecComps;
            ret = SplitPath( elem, vecComps );
            if( ERROR( ret ) )
            {
                ret = -EINVAL;
                break;
            }
            if( vecComps.size() < 2 )
                continue;

            const stdstr& strApp = vecComps[ 0 ];
            const stdstr& strPoint = vecComps[ 1 ];

            auto itr = mapAppPts.find( strApp );
            if( itr == mapAppPts.end() )
            {
                std::vector< stdstr > vecPts;
                vecPts.push_back( strPoint );
                mapAppPts[ strApp ] = vecPts;
            }
            else
            {
                itr->second.push_back( strPoint );
            }
        }
        if( mapAppPts.empty() )
            break;

        GETAC( pContext );
        auto& pfs = m_pAppRegfs;
        HANDLE hcurStm = INVALID_HANDLE;
        GetCurStream( this, pContext, hcurStm );

        for( auto& elem : mapAppPts )
        {
            const stdstr& strAppName = elem.first;
            if( !IsAppSubscribed( hcurStm, strAppName ) )
            {
                ret = -EACCES;
                break;
            }
            RFHANDLE hPtsDir = INVALID_HANDLE;
            stdstr strDir = "/" APPS_ROOT_DIR "/";
            strDir += elem.first + "/" POINTS_DIR "/";
            const std::vector< stdstr >& arrPoints =
                elem.second;

            ret = pfs->OpenDir(
                strDir, O_RDONLY, hPtsDir, pac );
            if( ERROR( ret ) )
                break;
            CFileHandle oHandle( pfs, hPtsDir );
            for( auto& elem : arrPoints )
            {
                CCfgOpener oParams;
                Variant oVar;
                stdstr strFile =
                    elem + "/" VALUE_FILE;

                stdstr strPtPath =
                    strAppName + "/" + elem;

                oParams.SetIntProp(
                    propReturnValue, 0 );

                stdstr strType =
                    elem + "/" DATA_TYPE;
                Variant dt;
                ret = pfs->GetValue(
                    hPtsDir, strType, dt );
                if( ERROR( ret ) )
                {
                    oParams.SetIntProp(
                        propReturnValue, ret );
                    mapPtDescs[ strPtPath ] =
                        ObjPtr( oParams.GetCfg() );
                    ret = 0;
                    continue;
                }
                auto iType = ( guint32& )dt;
                if( iType < typeDMsg )
                {
                    ret = pfs->GetValue(
                        hPtsDir, strFile, oVar );
                }
                else if( iType == typeByteArr )
                do{
                    RFHANDLE hFile;
                    ret = pfs->OpenFile( hPtsDir,
                        strFile, O_RDONLY,
                        hFile, pac );
                    if( ERROR( ret ) )
                        break;
                    CFileHandle ofh( pfs, hFile );
                    BufPtr pBuf( true );

                    struct stat st;
                    ret = pfs->GetAttr( hFile, st );
                    if( ERROR( ret ) )
                        break;
                    guint32 dwSize = st.st_size;
                    if( dwSize == 0 ||
                        dwSize > MAX_BYTES_PER_TRANSFER )
                    {
                        oParams.SetIntProp(
                            GETPTDESC_SIZE, dwSize );
                        // return an empty buffer with size
                        oVar = pBuf;
                        break;
                    }
                    ret = pBuf->Resize( dwSize );
                    if( ERROR( ret ) )
                        break;
                    ret = pfs->ReadFile( hFile,
                        pBuf->ptr(), dwSize, 0 );
                    if( ERROR( ret ) )
                        break;
                    oVar = pBuf;
                }while( 0 );
                else
                {
                    ret = -ENOTSUP;
                    OutputMsg( ret,
                        "Error GetPointValue %s/%s",
                        strAppName.c_str(),
                        elem.c_str() );
                    oParams.SetIntProp(
                        propReturnValue, ret );
                    mapPtDescs[ strPtPath ] =
                        ObjPtr( oParams.GetCfg() );
                    ret = 0;
                    continue;
                }

                // fine if failed to get value
                if( SUCCEEDED( ret ) )
                    oParams.SetProperty(
                        GETPTDESC_VALUE, oVar );

                guint32 dwFlags = 0;
                do{
                    RFHANDLE hPtDir = INVALID_HANDLE;
                    ret = pfs->OpenDir( strDir + elem,
                        O_RDONLY, hPtDir, pac );
                    if( ERROR( ret ) )
                        break;
                    CFileHandle oHandle( pfs, hPtDir );
                    Variant oVar;
                    ret = pfs->GetValue(
                        hPtDir, "ptype", oVar, pac );
                    if( ERROR( ret ) )
                        break;
                    oParams.SetProperty(
                        GETPTDESC_PTYPE, oVar );

                    // average
                    ret = pfs->GetValue( hPtDir,
                        "average", oVar, pac );
                    if( SUCCEEDED( ret ) )
                        oParams.SetProperty(
                            GETPTDESC_AVERAGE, oVar );
                    ret = 0;

                    // unit
                    oVar = ( guint32 )0;
                    ret = pfs->GetValue( hPtDir,
                        "unit", oVar, pac );
                    if( SUCCEEDED( ret ) )
                        oParams.SetProperty(
                            GETPTDESC_UNIT, oVar );
                    ret = 0;

                    // datatype
                    oVar = ( guint32 )0;
                    ret = pfs->GetValue( hPtDir,
                        "datatype", oVar, pac );
                    if( ERROR( ret ) )
                        break;
                    oParams.SetProperty(
                        GETPTDESC_DATATYPE, oVar );

                    // has log
                    ret = pfs->Access( hPtDir,
                        "logs/ptr0-0", R_OK, pac );
                    if( SUCCEEDED( ret ) )
                    {
                        oVar = true;
                        oParams.SetProperty(
                            GETPTDESC_HASLOG, oVar );
                    }
                    else
                    {
                        ret = 0;
                    }
                }while( 0 );
                if( ERROR( ret ) )
                {
                    oParams.SetIntProp(
                        propReturnValue, ret );
                    mapPtDescs[ strPtPath ] =
                        ObjPtr( oParams.GetCfg() );
                    ret = 0;
                    continue;
                }

                if( oParams.GetCfg()->size() == 0 )
                {
                    oParams.SetIntProp(
                        propReturnValue, -ENOENT );
                }
                mapPtDescs[ strPtPath ] =
                    ObjPtr( oParams.GetCfg() );
            }
        }
    }while( 0 );
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
            itr1++;
        }

        stdstr strStmFile = std::to_string( hstm );
        for( auto& elem : vecValidApps )
        {
            stdstr strAppPath =
                strPath + "/" + elem;
            stdstr strStmPath = strAppPath + "/" 
                MONITOR_STREAM_DIR "/" +
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
            RFHANDLE hFile = INVALID_HANDLE;
            ret = m_pAppRegfs->CreateFile(
                strStmPath, 0640, O_RDWR,
                hFile, pac );
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
                strStmPath, pac );
            if( ERROR( ret ) )
                DebugPrint( ret, "Error, filed to "
                    "remove listener %s",
                    strStmFile.c_str() );
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
        gint32 ret = LoadUserGrpsMap();
        if( ERROR( ret ) )
            break;

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

gint32 CAppMonitor_SvrImpl::LoadAssocMaps(
    RegFsPtr& pfs, int iAuthMech )
{
    gint32 ret = 0;
    do{
        CRegistryFs* pfs = g_pUserRegfs;
        gint32 iFd = g_pUserRegfs->GetFd();
        RFHANDLE hDir = INVALID_HANDLE;
        stdstr strAssocDir;
        if( iAuthMech == 0 )
            strAssocDir = "/" KRB5_ASSOC_DIR;
        else if( iAuthMech == 1 )
            strAssocDir = "/" OA2_ASSOC_DIR;
        else
        {
            ret = -EINVAL;
            break;
        }
        ret = pfs->OpenDir( strAssocDir, 0, hDir );
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
            stdstr strPath = strAssocDir + "/";
            Variant oVar;
            ret = pfs->GetValue(
                hDir, ks.szKey, oVar );
            if( ERROR( ret ) )
                continue;

            CStdRMutex oLock( this->GetLock() );
            auto itr = m_mapName2Uid.find(
                ( stdstr& )oVar );
            if( itr == m_mapName2Uid.end() )
                continue;
            if( iAuthMech == 0 )
                m_mapKrb5Name2Uid.insert(
                    { ks.szKey, itr->second } );
            else if( iAuthMech == 1 )
                m_mapOA2Name2Uid.insert(
                    { ks.szKey, itr->second } );
        }
    }while( 0 );
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
                continue;

            for( auto& ks2 : vecGidEnt )
            {
                guint32 dwGid = std::strtol(
                    ks2.szKey, nullptr, 10 );
                ( *psetGids )().insert( dwGid );
            }
            CStdRMutex oLock( this->GetLock() );
            m_mapUid2Gids.insert(
                { dwUid, psetGids });
            m_mapName2Uid.insert(
                { ks.szKey, dwUid } );
        }

        ret = LoadAssocMaps( g_pUserRegfs, 0 );
        if( ERROR( ret ) )
            break;

        ret = LoadAssocMaps( g_pUserRegfs, 1 );
        if( ERROR( ret ) )
            break;

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
            else if( iMech == 2 )
            {
                oVar = strName;
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

