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
#include "SimpleAuthsvr.h"
#include "encdec.h"

extern RegFsPtr GetRegFs( bool bUser );

#define GETAC( _pContext ) \
    CAccessContext oac; \
    ret = m_pAppMon->GetAccessContext(\
        ( _pContext ), oac ); \
    if( ret == -EACCES ) \
        break; \
    if( SUCCEEDED( ret ) ) \
    { ret = -EACCES; break; } \
    if( ret != -ENOENT && ERROR( ret ) ) \
        break; \
    ret = 0

/* Async Req Handler*/
gint32 CSimpleAuth_SvrImpl::GetUidByOAuth2Name( 
    IConfigDb* pContext, 
    const std::string& strOA2Name /*[ In ]*/, 
    gint32& dwUid /*[ Out ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'GetUidByOAuth2NameComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CSimpleAuth_SvrImpl::GetUidByKrb5Name( 
    IConfigDb* pContext, 
    const std::string& strKrb5Name /*[ In ]*/, 
    gint32& dwUid /*[ Out ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'GetUidByKrb5NameComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CSimpleAuth_SvrImpl::GetUidByUserName( 
    IConfigDb* pContext, 
    const std::string& strUserName /*[ In ]*/, 
    gint32& dwUid /*[ Out ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'GetUidByUserNameComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CSimpleAuth_SvrImpl::GetPasswordSalt( 
    IConfigDb* pContext, 
    gint32 dwUid /*[ In ]*/, 
    std::string& strSalt /*[ Out ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'GetPasswordSaltComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}

/* Async Req Handler*/
gint32 CSimpleAuth_SvrImpl::CheckClientToken( 
    IConfigDb* pContext, 
    ObjPtr& pCliInfo /*[ In ]*/,
    ObjPtr& pSvrInfo /*[ In ]*/, 
    ObjPtr& pRetInfo /*[ Out ]*/ )
{
    gint32 ret = 0;
    do{
        CCfgOpener oci( ( IConfigDb* )pCliInfo );
        CCfgOpener osi( ( IConfigDb* )pSvrInfo );

        bool bGmSSL = false;
        ret = oci.GetBoolProp( propGmSSL, bGmSSL );
        if( ERROR( ret ) )
            break;

        BufPtr pToken;
        ret = oci.GetBufPtr( 0, pToken );
        if( ERROR( ret ) )
            break;

        stdstr strUser;
        ret = oci.GetStrProp(
            propUserName, strUser );
        if( ERROR( ret ) )
        {
            ret = -EACCES;
            break;
        }

        RegFsPtr pfs = GetRegFs( true );
        if( pfs.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }
        stdstr strPath = "/users/";
        strPath += strUser;

        Variant oVar;
        ret = pfs->GetValue(
            strPath + "/passwd", oVar );
        if( ERROR( ret ) )
        {
            ret = -EACCES;
            break;
        }
        stdstr& strKey = oVar;
        BufPtr pKey( true );
        if( strKey.size() != 64 )
        {
            ret = -EINVAL;
            break;
        }
        pKey->Resize( strKey.size() / 2 );
        ret = HexStringToBytes(
            strKey.c_str(), strKey.size(),
            ( guint8* )pKey->ptr() );
        if( ERROR( ret ) )
        {
            ret = -EACCES;
            break;
        }

        BufPtr pOutBuf;
        ret = DecryptAesGcmBlock(
            pKey, pToken, pOutBuf, bGmSSL );
        if( ERROR( ret ) )
            break;

        ret = pfs->GetValue(
            strPath + "/uid", oVar );
        if( ERROR( ret ) )
            break;

        guint32 dwUid = oVar;        
        std::vector< KEYPTR_SLOT > vecks;
        RFHANDLE hDir = INVALID_HANDLE;
        ret = pfs->OpenDir(
            strPath + "/groups", O_RDONLY, hDir );
        if( ERROR( ret ) )
            break;

        ret = pfs->ReadDir( hDir, vecks );
        pfs->CloseFile( hDir );
        if( ERROR( ret ) )
        {
            hDir = INVALID_HANDLE;
            break;
        }

        IntVecPtr pGids( true );
        for( auto& elem : vecks )
        {
            ( *pGids )().push_back(
                atoi( elem.szKey ) );
        }

        CCfgOpener oCliInfo;
        ret = oCliInfo.Deserialize( pOutBuf );
        if( ERROR( ret ) )
            break;

        stdstr strVal1;
        ret = oCliInfo.GetStrProp(
            propSessHash, strVal1 );
        if( ERROR( ret ) )
            break;

        stdstr strVal2;
        ret = osi.GetStrProp(
            propSessHash, strVal2 );
        if( strVal1.empty() || strVal1 != strVal2 )
        {
            ret = -EACCES;
            break;
        }
        ret = oCliInfo.GetStrProp(
            propUserName, strVal1 );
        if( ERROR( ret ) )
        {
            ret = -EACCES;
            break;
        }

        if( strVal1.empty() || strVal1 != strUser )
        {
            ret = -EACCES;
            break;
        }
        
        guint64 qwTs1, qwTs2;
        ret = oCliInfo.GetQwordProp(
            propTimestamp, qwTs1 );
        if( ERROR( ret ) )
        {
            ret = -EACCES;
            break;
        }

        ret = osi.GetQwordProp(
            propTimestamp, qwTs2 );
        if( ERROR( ret ) )
        {
            ret = -EACCES;
            break;
        }
        if( qwTs2 != qwTs1 )
        {
            ret = -EACCES;
            break;
        }
        qwTs1+=1;
        BufPtr pTokBuf( true ), pRetBuf;
        pTokBuf->Append(
            ( char* )&qwTs1, sizeof( qwTs1 ) );
        pTokBuf->Append(
            strUser.c_str(), strUser.size() );
        pTokBuf->Append(
            strVal2.c_str(), strVal2.size() );
        ret = EncryptAesGcmBlock(
            pTokBuf, pKey, pRetBuf, bGmSSL );
        if( ERROR( ret ) )
            break;

        CParamList oRetInfo;
        oRetInfo.Push( pRetBuf );
        oRetInfo.SetQwordProp(
            propTimestamp, qwTs2 );
        oRetInfo.SetStrProp(
            propUserName, strUser );
        oRetInfo.SetBoolProp(
            propContinue, false );
        oRetInfo.SetBoolProp(
            propGmSSL, bGmSSL );
        oRetInfo.SetIntProp(
            propUid, dwUid );
        oRetInfo.SetObjPtr(
            propGid, ObjPtr( pGids ) );
        pRetInfo = oRetInfo.GetCfg();

    }while( 0 );
    return ret;
}

gint32 CSimpleAuth_SvrImpl::CreateStmSkel(
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
            strDesc,"SimpleAuth_SvrSkel",
            true, oCfg.GetCfg() );
        if( ERROR( ret ) )
            break;
        oCfg.CopyProp( propSkelCtx, this );
        oCfg[ propPortId ] = dwPortId;
        ret = pIf.NewObj(
            clsid( CSimpleAuth_SvrSkel ),
            oCfg.GetCfg() );
    }while( 0 );
    return ret;
}
gint32 CSimpleAuth_SvrImpl::OnPreStart(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCtx;
        CCfgOpenerObj oIfCfg( this );
        oCtx[ propClsid ] = clsid( 
            CSimpleAuth_ChannelSvr );
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
        strInstName = "SimpleAuth_ChannelSvr";
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
