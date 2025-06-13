/****BACKUP YOUR CODE BEFORE RUNNING RIDLC***/
// ../../ridl/.libs/ridlc --server -sO . ./appmon.ridl 
// Implement the following methods
// to get the RPC proxy/server work
#include "rpc.h"
using namespace rpcf;
#include "stmport.h"
#include "fastrpc.h"
#include "appmon.h"
#include "AppManagersvr.h"
#include "AppMonitorsvr.h"
#include <stdlib.h>
#include "monconst.h"
#include "blkalloc.h"

extern std::vector< InterfPtr > g_vecIfs;

InterfPtr GetAppMonitor()
{ return g_vecIfs[ 1 ]; }

InterfPtr GetAppManager()
{ return g_vecIfs[ 0 ]; }

InterfPtr GetSimpAuth()
{ return g_vecIfs[ 2 ]; }

gint32 GetPointType(
    RegFsPtr pAppReg,
    const stdstr& strPtPath,
    EnumPointType& dwType )
{
    gint32 ret = 0;
    do{
        stdstr strPath="/" APPS_ROOT_DIR "/";
        strPath += strPtPath;

        Variant oVar;
        ret = pAppReg->GetValue(
            strPath + "/" POINT_TYPE_FILE, oVar  );
        if( ERROR( ret ) )
            break;

        dwType = (EnumPointType)( guint32& )oVar;

    }while( 0 );
    return ret;
}

gint32 SplitPath( const stdstr& strPath,
    std::vector< stdstr >& vecComp )
{
    return CRegistryFs::Namei( strPath, vecComp );
}

gint32 GetMonitorToNotify(
    CFastRpcServerBase* pIf,
    RegFsPtr pAppReg,
    const stdstr& strAppName,
    std::vector< HANDLE >& vecStms )
{
    gint32 ret = 0;
    do{
        stdstr strPath = "/" APPS_ROOT_DIR "/";
        strPath += strAppName;
        strPath += "/" MONITOR_STREAM_DIR;
        std::vector< KEYPTR_SLOT > vecks;
        RFHANDLE hDir = INVALID_HANDLE;
        ret = pAppReg->OpenDir(
            strPath, O_RDONLY, hDir );
        if( ERROR( ret ) )
            break;
        CFileHandle oHandle( pAppReg, hDir );
        ret = pAppReg->ReadDir( hDir, vecks );
        if( ERROR( ret ) )
            break;
        if( vecks.empty() )
        {
            ret = -ENOENT;
            break;
        }
        for( auto& elem : vecks )
        {
            HANDLE hstm = ( HANDLE )
                strtol( elem.szKey, nullptr, 10 );
            InterfPtr ptrIf;
            gint32 iRet = pIf->GetStmSkel(
                hstm, ptrIf );
            if( SUCCEEDED( iRet ) )
            {
                vecStms.push_back( hstm );
                continue;
            }
            pAppReg->RemoveFile(
                hDir, elem.szKey );
        }
    }while( 0 );
    return ret;
}

gint32 GetOwnerStream(
    CFastRpcServerBase* pIf,
    RegFsPtr pAppReg,
    const stdstr& strAppName,
    HANDLE& hstm )
{
    gint32 ret = 0;
    do{
        HANDLE hStm = INVALID_HANDLE;
        stdstr strPath = "/" APPS_ROOT_DIR "/";
        strPath += strAppName;
        strPath += "/" OWNER_STREAM_FILE;
        Variant oVar;
        ret = pAppReg->GetValue(
            strPath, oVar  );
        if( ERROR( ret ) )
            break;
        hStm = ( HANDLE )( guint64& )oVar;
        if( hStm == INVALID_HANDLE )
        {
            ret = -ENOENT;
            break;
        }
        InterfPtr pSkel;
        ret = pIf->GetStmSkel( hStm, pSkel );
        if( ERROR( ret ) )
            break;
        hstm = hStm;
    }while( 0 );
    return ret;
}

gint32 SetOwnerStream(
    RegFsPtr pAppReg,
    const stdstr& strAppName,
    HANDLE hstm )
{
    gint32 ret = 0;
    RFHANDLE hDir = INVALID_HANDLE;
    do{
        stdstr strPath = "/" APPS_ROOT_DIR "/";
        strPath += strAppName;
        strPath += "/" OWNER_STREAM_FILE;
        Variant oVar( ( HANDLE )hstm );
        ret = pAppReg->SetValue(
            strPath, oVar );
    }while( 0 );
    return ret;
}

gint32 GetCurStream(
    CFastRpcServerBase* pIf,
    IConfigDb* pReqCtx, HANDLE& hstm )
{
    gint32 ret = 0;
    CCfgOpener oCfg( pReqCtx );
    do{
        IEventSink* pCallback;
        ret = oCfg.GetPointer(
            propEventSink, pCallback );
        if( ERROR( ret ) )
            break;
        ret = pIf->GetStream( pCallback, hstm );
    }while( 0 );
    return ret;
}

using STM_POINT=std::pair< HANDLE, stdstr >;
gint32 GetInputToNotify(
    CFastRpcServerBase* pIf,
    RegFsPtr pAppReg,
    const stdstr& strPtPath,
    std::vector< STM_POINT  >& vecStms )
{
    gint32 ret = 0;
    do{
        RFHANDLE hDir = INVALID_HANDLE;
        stdstr strPath = "/" APPS_ROOT_DIR "/";
        strPath += strPtPath;
        strPath += "/" OUTPUT_LINKS_DIR;
        std::vector< KEYPTR_SLOT > vecks;
        ret = pAppReg->OpenDir(
            strPath, O_RDONLY, hDir );
        if( ERROR( ret ) )
            break;
        CFileHandle oHandle( pAppReg, hDir );
        ret = pAppReg->ReadDir( hDir, vecks );
        if( ERROR( ret ) )
            break;
        if( vecks.empty() )
        {
            ret = -ENOENT;
            break;
        }
        for( auto& elem : vecks )
        {
            Variant oVar;
            ret = pAppReg->GetValue( hDir,
                elem.szKey, oVar );
            if( ERROR( ret ) )
                continue;

            std::vector< stdstr > vecComps;
            ret = SplitPath(
                ( stdstr& )oVar, vecComps );
            if( ERROR( ret ) )
            {
                ret = -EINVAL;
                break;
            }

            HANDLE hstm;
            ret = GetOwnerStream( pIf,
                pAppReg, vecComps[ 0 ], hstm );
            if( ERROR( ret ) )
                break;
            vecStms.push_back(
                { hstm, ( stdstr& )oVar  } );
        }
        if( vecStms.size() )
            ret = 0;
    }while( 0 );
    return ret;
}


bool CAppManager_SvrImpl::IsAppOnline(
    const stdstr& strAppName )
{
    HANDLE hstm = INVALID_HANDLE;
    gint32 ret = GetOwnerStream( this,
        m_pAppRegfs, strAppName, hstm );
    if( SUCCEEDED( ret ) )
        return true;
    return false;
}
/* Async Req Handler*/
gint32 CAppManager_SvrImpl::ListApps( 
    IConfigDb* pContext, 
    std::vector<std::string>& arrApps /*[ Out ]*/ )
{
    gint32 ret = 0;
    do{
        stdstr strPath = "/" APPS_ROOT_DIR;
        std::vector< KEYPTR_SLOT > vecks;
        RFHANDLE hDir = INVALID_HANDLE;
        ret = m_pAppRegfs->OpenDir(
            strPath, O_RDONLY, hDir );
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
            arrApps.push_back( elem.szKey );
    }while( 0 );
    return ret;
}

/* Async Req Handler*/
gint32 CAppManager_SvrImpl::ListPoints( 
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
        stdstr strPath = "/" APPS_ROOT_DIR "/";
        strPath+= strAppPath;
        std::vector< KEYPTR_SLOT > vecks;
        ret = m_pAppRegfs->OpenDir(
            strPath, O_RDONLY, hDir );
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
            arrPoints.push_back( elem.szKey );
    }while( 0 );
    return ret;
}

/* Async Req Handler*/
gint32 CAppManager_SvrImpl::ListAttributes( 
    IConfigDb* pContext, 
    const std::string& strPtPath /*[ In ]*/, 
    std::vector<std::string>& arrAttributes /*[ Out ]*/ )
{
    gint32 ret = 0;
    do{
        if( strPtPath.empty() )
        {
            ret = -EINVAL;
            break;
        }
        stdstr strPath = "/" APPS_ROOT_DIR "/";
        strPath += strPtPath;
        std::vector< KEYPTR_SLOT > vecks;
        RFHANDLE hDir = INVALID_HANDLE;
        ret = m_pAppRegfs->OpenDir(
            strPath, O_RDONLY, hDir );
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

gint32 CAppManager_SvrImpl::NotifyValChange(
    const stdstr strPtPath,
    const Variant& value,
    RFHANDLE hcurStm )
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

        const stdstr& strApp = vecComps[ 0 ];
        const stdstr& strPoint = vecComps[ 1 ];

        EnumPointType dwType;
        stdstr strPath = strApp + 
            "/" POINTS_DIR "/" + strPoint;

        ret = GetPointType( m_pAppRegfs, 
            strPath, dwType );
        if( ERROR( ret ) )
            break;

        gint32 iRet = 0;
        if( dwType == ptOutput )
        do{
            // notify the app of the input point
            std::vector< STM_POINT > vecStms;
            iRet = GetInputToNotify( this,
                m_pAppRegfs, strPath, vecStms );
            if( ERROR( iRet ) )
                break;
            for( auto& elem : vecStms )
            {
                InterfPtr ptrSkel;
                iRet = this->GetStmSkel(
                    elem.first, ptrSkel );
                if( ERROR( iRet ) )
                    continue;
                CAppManager_SvrSkel* pSkel = ptrSkel;
                pSkel->IIAppStore_SImpl::OnPointChanged(
                    elem.second, value );
            }
        }while( 0 );
        do{
            // notify the owner app of the point
            HANDLE hstm = INVALID_HANDLE;
            iRet = GetOwnerStream(
                this, m_pAppRegfs, strApp, hstm );
            if( ERROR( iRet ) )
                break;

            if( hcurStm == hstm )
                break;

            InterfPtr ptrSkel;
            iRet = GetStmSkel( hstm, ptrSkel );
            if( ERROR( iRet ) )
                break;
            CAppManager_SvrSkel* pSkel = ptrSkel;
            pSkel->IIAppStore_SImpl::OnPointChanged(
                strPtPath, value );
        }while( 0 );
    }while( 0 );
    return ret;
}

/* Async Req Handler*/
gint32 CAppManager_SvrImpl::SetPointValue( 
    IConfigDb* pContext, 
    const std::string& strPtPath /*[ In ]*/,
    const Variant& value /*[ In ]*/ )
{
    gint32 ret = 0;
    auto& pfs = m_pAppRegfs;
    do{
        stdstr strPath = "/" APPS_ROOT_DIR "/";
        Variant oOrigin;

        std::vector< stdstr > vecComps;
        ret = SplitPath( strPtPath, vecComps );
        if( ERROR( ret ) )
        {
            ret = -EINVAL;
            break;
        }

        const stdstr& strApp = vecComps[ 0 ];
        const stdstr& strPoint = vecComps[ 1 ];

        if( !IsAppOnline( strApp ) )
        {
            ret = -EACCES;
            break;
        }
        RFHANDLE hPtDir = INVALID_HANDLE;
        ret = pfs->OpenDir( strPath +
            strApp + "/" POINTS_DIR "/" + strPoint,
            O_RDONLY, hPtDir );
        if( ERROR( ret ) )
            break;

        CFileHandle oHandle( pfs, hPtDir );

        ret = pfs->Access(
            hPtDir, OUTPUT_PULSE, F_OK );
        if( ERROR( ret ) )
        {
            ret = pfs->GetValue( hPtDir,
                VALUE_FILE, oOrigin );
            if( SUCCEEDED( ret ) &&
                oOrigin == value )
                break;
            ret = 0;
        }
        ret = pfs->SetValue(
            hPtDir, VALUE_FILE, value );
        if( ERROR( ret ) )
            break;

        HANDLE hcurStm = INVALID_HANDLE;
        GetCurStream( this, pContext, hcurStm );

        NotifyValChange(
            strPtPath, value, hcurStm );
        // notify the monitors
        InterfPtr pIf = GetAppMonitor();
        CAppMonitor_SvrImpl* pAppMon = pIf;
        pAppMon->OnPointChangedInternal(
            strPtPath, value, hcurStm );

    }while( 0 );
    return ret;
}

/* Async Req Handler*/
gint32 CAppManager_SvrImpl::GetPointValue( 
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
gint32 CAppManager_SvrImpl::SetLargePointValue( 
    IConfigDb* pContext, 
    const std::string& strPtPath /*[ In ]*/,
    BufPtr& value /*[ In ]*/ )
{
    gint32 ret = 0;
    if( value.IsEmpty() || value->empty() )
        return -EINVAL;
    do{
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
        if( !IsAppOnline( strApp ) )
        {
            ret = -EACCES;
            break;
        }
        stdstr strAttrVal =  strPath + strApp +
            "/" POINTS_DIR "/" +
            strPoint + "/" VALUE_FILE; 
        RFHANDLE hFile;
        ret = m_pAppRegfs->OpenFile(
            strAttrVal, O_WRONLY | O_TRUNC, hFile );
        if( ERROR( ret ) )
            break;
        CFileHandle oHandle( m_pAppRegfs, hFile );
        guint32 dwSize = value->size();
        ret = m_pAppRegfs->WriteFile( hFile,
            value->ptr(), dwSize, 0 );
    }while( 0 );
    return ret;
}
/* Async Req Handler*/
gint32 CAppManager_SvrImpl::GetLargePointValue( 
    IConfigDb* pContext, 
    const std::string& strPtPath /*[ In ]*/, 
    BufPtr& value /*[ Out ]*/ )
{
    gint32 ret = 0;
    if( value.IsEmpty() )
        value.NewObj();
    do{
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

        if( !IsAppOnline( strApp ) )
        {
            ret = -EACCES;
            break;
        }
        stdstr strAttrVal =  strPath + strApp +
            "/" POINTS_DIR "/" +
            strPoint + "/" VALUE_FILE; 
        RFHANDLE hFile;
        ret = m_pAppRegfs->OpenFile(
            strAttrVal, O_RDONLY, hFile );
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
gint32 CAppManager_SvrImpl::SubscribeStreamPoint( 
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
gint32 CAppManager_SvrImpl::SetAttrValue( 
    IConfigDb* pContext, 
    const std::string& strAttrPath /*[ In ]*/,
    const Variant& value /*[ In ]*/ )
{
    gint32 ret = 0;
    do{
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
        if( !IsAppOnline( strApp ) )
        {
            ret = -EACCES;
            break;
        }
        stdstr strAttrVal =  strPath + strApp +
            "/" POINTS_DIR "/" +
            strPoint + "/" + strAttr; 
        ret = m_pAppRegfs->SetValue(
            strAttrVal, value );
    }while( 0 );
    return ret;
}
/* Async Req Handler*/
gint32 CAppManager_SvrImpl::GetAttrValue( 
    IConfigDb* pContext, 
    const std::string& strAttrPath /*[ In ]*/, 
    Variant& rvalue /*[ Out ]*/ )
{
    gint32 ret = 0;
    do{
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
        if( !IsAppOnline( strApp ) )
        {
            ret = -EACCES;
            break;
        }
        stdstr strAttrVal =  strPath + strApp +
            "/" POINTS_DIR "/" +
            strPoint + "/" + strAttr; 
        ret = m_pAppRegfs->GetValue(
            strAttrVal, rvalue );
    }while( 0 );
    return ret;
}

/* Async Req Handler*/
gint32 CAppManager_SvrImpl::SetPointValues( 
    IConfigDb* pContext, 
    const std::string& strAppName /*[ In ]*/,
    std::vector<KeyValue>& arrValues /*[ In ]*/ )
{
    gint32 ret = 0;
    auto& pfs = m_pAppRegfs;
    do{
        if( !IsAppOnline( strAppName ) )
        {
            ret = -EACCES;
            break;
        }
        stdstr strDir = "/" APPS_ROOT_DIR "/";
        strDir += strAppName + "/" POINTS_DIR "/";

        RFHANDLE hPtDir = INVALID_HANDLE;
        ret = pfs->OpenDir(
            strDir, O_RDONLY, hPtDir );
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
                    hFile );
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
gint32 CAppManager_SvrImpl::GetPointValues( 
    IConfigDb* pContext, 
    const std::string& strAppName /*[ In ]*/,
    std::vector<std::string>& arrPtPaths /*[ In ]*/, 
    std::vector<KeyValue>& arrKeyVals /*[ Out ]*/ )
{
    gint32 ret = 0;
    auto& pfs = m_pAppRegfs;
    do{
        if( !IsAppOnline( strAppName ) )
        {
            ret = -EACCES;
            break;
        }
        stdstr strDir = "/" APPS_ROOT_DIR "/";
        strDir += strAppName + "/" POINTS_DIR "/";

        RFHANDLE hPtDir = INVALID_HANDLE;
        ret = pfs->OpenDir(
            strDir, O_RDONLY, hPtDir );
        if( ERROR( ret ) )
            break;
        CFileHandle oHandle( pfs, hPtDir );
        for( auto& elem : arrPtPaths )
        {
            stdstr strFile =
                elem + "/" VALUE_FILE;

            KeyValue kv;
            kv.strKey = elem;

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
                    strFile, O_RDONLY, hFile );
                if( ERROR( ret ) )
                    break;
                CFileHandle ofh( pfs, hFile );
                {
                    ret = -ENOTSUP;
                    break;
                }
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
                    "Error SetPointValue %s/%s",
                    strAppName.c_str(),
                    elem.c_str() );
            }
            arrKeyVals.push_back( kv );
        }
        ret = 0;
    }while( 0 );
    return ret;
}

/* Async Req Handler*/
gint32 CAppManager_SvrImpl::ClaimAppInst( 
    IConfigDb* pContext, 
    const std::string& strAppName /*[ In ]*/,
    std::vector<KeyValue>& arrPtToSet /*[ In ]*/, 
    std::vector<KeyValue>& arrPtToGet /*[ Out ]*/ )
{
    gint32 ret = 0;
    bool bOwnerSet = false;
    auto& pfs = m_pAppRegfs;
    do{
        HANDLE hstm, hOwner;
        ret = GetCurStream( this, pContext, hstm );
        if( ERROR( ret ) )
            break;
        ret = GetOwnerStream( this,
            pfs, strAppName, hOwner );
        if( ERROR( ret ) )
        {
            ret = 0;
        }
        else
        {
            InterfPtr ptrIf;
            gint32 iRet = GetStmSkel(
                hOwner, ptrIf );
            if( SUCCEEDED( iRet ) )
            {
                // already occupied
                ret = -EEXIST;
                break;
            }
        }

        ret = SetOwnerStream(
            pfs, strAppName, hstm );
        if( ERROR( ret ) )
            break;

        bOwnerSet = true;
        CStdRMutex oLock( GetLock() );
        auto itr = m_mapAppOwners.find( hstm );
        StrSetPtr pAppSet;
        if( itr == m_mapAppOwners.end() )
        {
            pAppSet.NewObj();
            ( *pAppSet )().insert( strAppName );
            m_mapAppOwners.insert(
                { hstm, pAppSet } );
        }
        else
        {
            auto& setApps = ( *itr->second )();
            setApps.insert( strAppName );
        }
        oLock.Unlock();

        ret = this->SetPointValues(
            pContext, strAppName, arrPtToSet );
        if( ERROR( ret ) )
            break;

        stdstr strDir = "/" APPS_ROOT_DIR "/";
        strDir += strAppName + "/" POINTS_DIR "/";

        std::vector< stdstr > vecPoints;
        RFHANDLE hPtsDir = INVALID_HANDLE;
        ret = pfs->OpenDir(
            strDir, O_RDONLY, hPtsDir );
        if( ERROR( ret ) )
            break;

        CFileHandle oHandle( pfs, hPtsDir );
        std::vector< KEYPTR_SLOT > vecks;
        ret = pfs->ReadDir( hPtsDir, vecks );
        if( ERROR( ret ) )
            break;
        for( auto& elem : vecks )
        {
            stdstr strLoad = elem.szKey;
            strLoad += "/" LOAD_ON_START;
            gint32 iRet = pfs->Access(
                hPtsDir, strLoad, F_OK );
            if( ERROR( iRet ) )
                continue;
            vecPoints.push_back(
                stdstr( elem.szKey ) );
        }
        if( vecPoints.empty() )
            break;

        ret = this->GetPointValues(
            pContext, strAppName,
            vecPoints, arrPtToGet );

    }while( 0 );
    if( ERROR( ret ) && bOwnerSet )
    {
        ret = SetOwnerStream( pfs,
            strAppName, INVALID_HANDLE );
    }
    if( SUCCEEDED( ret ) )
    {
        DebugPrint( 0, "App %s is online",
            strAppName.c_str() );
        LOGINFO( this->GetIoMgr(), 0,
            "App %s is online", strAppName.c_str() );
    }
    return ret;
}

gint32 CAppManager_SvrImpl::FreeAppInstsInternal(
    HANDLE hstm, std::vector<stdstr>& arrApps )
{
    gint32 ret = 0;
    do{
        CStdRMutex oLock( GetLock() );
        auto itr = m_mapAppOwners.find( hstm );
        if( itr == m_mapAppOwners.end() )
        {
            ret = -ENOENT;
            break;
        }
        std::set< stdstr >& oSet =
            ( *itr->second )();
        std::vector< stdstr > vecValidApps;
        if( arrApps.size() )
        {
            for( auto& elem : arrApps )
            {
                auto itr = oSet.find( elem );
                if( itr == oSet.end() )
                    continue;
                oSet.erase( itr );
                vecValidApps.push_back( elem );
            }
        }
        else
        {
            for( auto& elem : oSet )
                vecValidApps.push_back( elem );
            oSet.clear();
            if( vecValidApps.size() )
                arrApps = vecValidApps;
        }
        if( oSet.empty() )
            m_mapAppOwners.erase( itr );
        oLock.Unlock();

        for( auto& elem : vecValidApps )
        {
            SetOwnerStream( m_pAppRegfs,
                elem, INVALID_HANDLE );
            DebugPrint( 0, "App %s is offline",
                elem.c_str() );
            LOGINFO( this->GetIoMgr(), 0,
                "App %s is offline", elem.c_str() );
        }

    }while( 0 );
    return ret;
}

/* Async Req Handler*/
gint32 CAppManager_SvrImpl::FreeAppInsts( 
    IConfigDb* pContext, 
    std::vector<std::string>& arrApps /*[ In ]*/ )
{
    gint32 ret = 0;
    do{
        HANDLE hstm;
        ret = GetCurStream(
            this, pContext, hstm );
        if( ERROR( ret ) )
            break;
        ret = FreeAppInstsInternal(
            hstm, arrApps );
    }while( 0 );
    return ret;
}

gint32 CAppManager_SvrImpl::CreateStmSkel(
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
            strDesc,"AppManager_SvrSkel",
            true, oCfg.GetCfg() );
        if( ERROR( ret ) )
            break;
        oCfg.CopyProp( propSkelCtx, this );
        oCfg[ propPortId ] = dwPortId;
        ret = pIf.NewObj(
            clsid( CAppManager_SvrSkel ),
            oCfg.GetCfg() );
    }while( 0 );
    return ret;
}
gint32 CAppManager_SvrImpl::OnPreStart(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCtx;
        CCfgOpenerObj oIfCfg( this );
        oCtx[ propClsid ] = clsid( 
            CAppManager_ChannelSvr );
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
        strInstName = "AppManager_ChannelSvr";
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

gint32 CAppManager_SvrImpl::RemoveStmSkel(
    HANDLE hstm )
{
    gint32 ret = 0;
    do{
        std::vector< stdstr > vecApps;
        // an empty array will put all the owned
        // apps offline
        ret = FreeAppInstsInternal(
            hstm, vecApps );
        if( SUCCEEDED( ret ) )
        {
            // unexpected disconnection, make a log
            for( auto elem : vecApps )
            {
                LOGERR( GetIoMgr(), -ECONNABORTED, 
                    "%s@%llx goes offline unexpected",
                    elem.c_str(), hstm );
                DebugPrint( -ECONNABORTED,
                    "%s@%llx goes offline unexpected",
                    elem.c_str(), hstm );
            }
        }
        ret = super::RemoveStmSkel( hstm );
    }while( 0 );
    return ret;
}
