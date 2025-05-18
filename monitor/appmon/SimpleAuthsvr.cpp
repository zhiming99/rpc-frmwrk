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

#include <openssl/evp.h>
#include <cstring>

// Decrypts AES-GCM block in pToken using pKey.
// Returns 0 on success, negative error code on failure.
// The decrypted plaintext will be written to outPlaintext.
gint32 DecryptAesGcmBlock_OpenSSL(
    const BufPtr& pKey,
    const BufPtr& pToken,
    BufPtr& outPlaintext )
{
    if( pKey.IsEmpty() || pToken.IsEmpty() )
        return -EINVAL;
    if( pKey->size() != 32 )
        return -EINVAL; // AES-256-GCM

    const size_t iv_len = 12;
    const size_t tag_len = 16;
    if( pToken->size() <= iv_len + tag_len )
        return -EINVAL;

    const uint8_t* key =
        (const uint8_t*)pKey->ptr();

    const uint8_t* token =
        (const uint8_t*)pToken->ptr();

    const uint8_t* iv = token;
    const uint8_t* ciphertext = token + iv_len;

    size_t ciphertext_len =
        pToken->size() - iv_len - tag_len;

    const uint8_t* tag =
        token + pToken->size() - tag_len;

    if( outPlaintext.IsEmpty() )
        outPlaintext.NewObj();

    outPlaintext->Resize(ciphertext_len);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return -ENOMEM;

    int ret = 0;
    int len = 0;
    int plaintext_len = 0;

    do {
        if( 1 != EVP_DecryptInit_ex( ctx,
            EVP_aes_256_gcm(), NULL, NULL, NULL) )
        {
            ret = -EFAULT;
            break;
        }
        if( 1 != EVP_CIPHER_CTX_ctrl( ctx,
            EVP_CTRL_GCM_SET_IVLEN, iv_len, NULL))
        {
            ret = -EFAULT;
            break;
        }
        if( 1 != EVP_DecryptInit_ex( ctx,
            NULL, NULL, key, iv))
        {
            ret = -EFAULT;
            break;
        }

        if( 1 != EVP_DecryptUpdate( ctx,
            ( guint8* )outPlaintext->ptr(), &len,
            ciphertext, ciphertext_len ) )
        {
            ret = -EFAULT;
            break;
        }

        plaintext_len = len;
        if (1 != EVP_CIPHER_CTX_ctrl( ctx,
            EVP_CTRL_GCM_SET_TAG,
            tag_len, (void*)tag ) )
        {
            ret = -EFAULT;
            break;
        }
        int rv = EVP_DecryptFinal_ex( ctx,
            ( guint8* )outPlaintext->ptr() + len,
            &len);
        if( rv <= 0 )
        {
            ret = -EACCES; // Authentication failed
            break;
        }
        plaintext_len += len;
        outPlaintext->Resize( plaintext_len );
    } while (0);

    EVP_CIPHER_CTX_free(ctx);
    return ret;
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
        strPath += strUser + "/passwd";

        Variant oVar;
        ret = pfs->GetValue( strPath, oVar );
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
        if( bGmSSL )
            break;
        ret = DecryptAesGcmBlock_OpenSSL(
            pKey, pToken, pOutBuf );
        if( ERROR( ret ) )
            break;

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
        CCfgOpener oRetInfo;
        oRetInfo.SetQwordProp(
            propTimestamp, qwTs2 );
        oRetInfo.SetStrProp(
            propUserName, strUser );

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
