/****BACKUP YOUR CODE BEFORE RUNNING RIDLC***/
// ../../ridl/.libs/ridlc -sO . ./appmon.ridl 
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
    std::vector< HANDLE > vecStms )
{
    gint32 ret = 0;
    RFHANDLE hDir = INVALID_HANDLE;
    do{
        stdstr strPath = "/" APPS_ROOT_DIR "/";
        strPath += strAppName;
        strPath += "/" MONITOR_STREAM_DIR;
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
        HANDLE hStm = INVALID_HANDLE;
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
    std::vector< STM_POINT  > vecStms )
{
    gint32 ret = 0;
    RFHANDLE hDir = INVALID_HANDLE;
    do{
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

            stdstr strInPath(
                "/" APPS_ROOT_DIR "/" );
            strInPath += ( stdstr& )oVar;

            HANDLE hstm;
            ret = GetOwnerStream(
                pIf, pAppReg, strInPath, hstm );
            if( ERROR( ret ) )
                break;
            vecStms.push_back( { hstm, strInPath } );
        }
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
    RFHANDLE hDir = INVALID_HANDLE;
    do{
        stdstr strPath = "/" APPS_ROOT_DIR;
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
    RFHANDLE hDir = INVALID_HANDLE;
    do{
        if( strPtPath.empty() )
        {
            ret = -EINVAL;
            break;
        }
        stdstr strPath = "/" APPS_ROOT_DIR "/";
        strPath += strPtPath;
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
        EnumPointType dwType;
        ret = GetPointType( m_pAppRegfs,
            strPtPath, dwType );
        if( ERROR( ret ) )
            break;

        std::vector< stdstr > vecComps;
        ret = SplitPath( strPtPath, vecComps );
        if( ERROR( ret ) )
        {
            ret = -EINVAL;
            break;
        }

        const stdstr& strApp = vecComps[ 0 ];

        gint32 iRet = 0;
        if( dwType == ptOutput )
        do{
            // notify the input app
            std::vector< STM_POINT > vecStms;
            iRet = GetInputToNotify( this,
                m_pAppRegfs, strPtPath, vecStms );
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
        else do{
            // notify the owner app
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
        RFHANDLE hPtDir;

        ret = m_pAppRegfs->OpenDir( strPath +
            strApp + POINTS_DIR "/" + strPoint,
            O_RDONLY, hPtDir );
        if( ERROR( ret ) )
            break;

        CFileHandle oHandle(
            m_pAppRegfs, hPtDir );

        ret = m_pAppRegfs->Access(
            hPtDir, OUTPUT_PULSE, F_OK );
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
    // TODO: Emit an async operation here.
    // And make sure to call 'SetLargePointValueComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppManager_SvrImpl::GetLargePointValue( 
    IConfigDb* pContext, 
    const std::string& strPtPath /*[ In ]*/, 
    BufPtr& value /*[ Out ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'GetLargePointValueComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
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
        stdstr strAttrVal =  strPath + strApp +
            "/" POINTS_DIR "/" +
            strPoint + "/" + strAttr; 
        ret = m_pAppRegfs->SetValue(
            strAttrVal, rvalue );
    }while( 0 );
    return ret;
}
/* Async Req Handler*/
gint32 CAppManager_SvrImpl::SetPointValues( 
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
gint32 CAppManager_SvrImpl::GetPointValues( 
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

/* Async Req Handler*/
gint32 CAppManager_SvrImpl::ClaimAppInsts( 
    IConfigDb* pContext, 
    std::vector<std::string>& arrApps /*[ In ]*/, 
    std::vector<KeyValue>& arrInitKVs /*[ Out ]*/ )
{
    gint32 ret = 0;
    do{
        HANDLE hstm;
        ret = GetCurStream( this, pContext, hstm );
        if( ERROR( ret ) )
            break;
        for( auto& elem : arrApps )
        {
            ret = GetOwnerStream( this,
                m_pAppRegfs, elem, hstm );
            if( ERROR( ret ) )
            {
                ret = 0;
            }
            else
            {
                InterfPtr ptrIf;
                gint32 iRet = GetStmSkel(
                    hstm, ptrIf );
                if( SUCCEEDED( iRet ) )
                {
                    ret = -EEXIST;
                    break;
                }
            }
        }
        if( ERROR( ret ) )
            break;
        for( auto& elem : arrApps )
        {
            ret = SetOwnerStream(
                m_pAppRegfs, elem, hstm );
            if( ERROR( ret ) )
                break;
            CStdRMutex oLock( GetLock() );
            auto itr = m_mapAppOwners.find( hstm );
            StrSetPtr pStrSet;
            if( itr == m_mapAppOwners.end() )
            {
                pStrSet.NewObj();
                ( *pStrSet )().insert( elem );
                m_mapAppOwners.insert(
                    { hstm, pStrSet } );
            }
            else
            {
                ( *itr->second )().insert( elem );
            }
            oLock.Unlock();
            RFHANDLE hPtDir = INVALID_HANDLE;
            stdstr strDir = "/" APPS_ROOT_DIR "/";
            strDir += elem + "/" POINTS_DIR "/";
            ret = m_pAppRegfs->OpenDir(
                strDir , O_RDONLY, hPtDir );
            if( ERROR( ret ) )
                continue;
            CFileHandle oHandle(
                m_pAppRegfs, hPtDir );

            std::vector< KEYPTR_SLOT > vecks;
            ret = m_pAppRegfs->ReadDir(
                hPtDir, vecks );

            if( ERROR( ret ) )
                continue;

            for( auto& elem2 : vecks )
            {
                stdstr strLoad = elem2.szKey;
                strLoad += "/" LOAD_ON_START;
                ret = m_pAppRegfs->Access(
                    hPtDir, strLoad, F_OK );
                if( ERROR( ret ) )
                    continue;
                Variant oVar;
                ret = m_pAppRegfs->GetValue(
                    hPtDir, VALUE_FILE, oVar );
                if( ERROR( ret ) )
                    continue;
                KeyValue kv;
                kv.strKey =
                    elem + "/" + elem2.szKey;
                kv.oValue = oVar;
                arrInitKVs.push_back( kv );
            }
        }
    }while( 0 );
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
        }
        if( oSet.empty() )
            m_mapAppOwners.erase( itr );
        oLock.Unlock();

        for( auto& elem : vecValidApps )
        {
            SetOwnerStream( m_pAppRegfs,
                elem, INVALID_HANDLE );
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
        FreeAppInstsInternal( hstm, vecApps );
        ret = super::RemoveStmSkel( hstm );
    }while( 0 );
    return ret;
}
