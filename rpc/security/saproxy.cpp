/*
 * =====================================================================================
 *
 *       Filename:  saproxy.cpp
 *
 *    Description:  Implementation of SimpAuth proxy for non-js client login
 *
 *        Version:  1.0
 *        Created:  05/14/2025 10:29:55 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:  
 *
 *      Copyright:  2025 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <pwd.h>
#include "rpc.h"
using namespace rpcf;
#include "ifhelper.h"
#include "security.h"
#include "oa2proxy.h"
#include "saproxy.h"
#include "blkalloc.h"

#define SIMPAUTH_DIR "simpleauth"

namespace rpcf 
{

CSimpAuthLoginProxy::CSimpAuthLoginProxy(
    const IConfigDb* pCfg ) :
    super( pCfg )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg( pCfg );
        CRpcServices* pSvc = nullptr;

        ret = oCfg.GetPointer(
            propRouterPtr, pSvc );
        if( ERROR( ret ) )
        {
            ret = 0;
            break;
        }
        m_pRouter = ObjPtr( pSvc );

        CCfgOpenerObj oIfCfg( this );
        oIfCfg.RemoveProperty(
            propRouterPtr );

    }while( 0 );
    return;
}

gint32 CSimpAuthLoginProxy::RebuildMatches()
{
    // this interface does not I/O normally
    CCfgOpenerObj oIfCfg( this );
    if( m_vecMatches.empty() )
        return -ENOENT;

    gint32 ret = super::RebuildMatches();
    if( ERROR( ret ) )
        return ret;

    MatchPtr pMatch;
    for( auto& elem : m_vecMatches )
    {
        CCfgOpenerObj oMatch(
            ( CObjBase* )elem );

        std::string strIfName;

        ret = oMatch.GetStrProp(
            propIfName, strIfName );

        if( ERROR( ret ) )
            continue;

        if( DBUS_IF_NAME( "IAuthenticate" )
            == strIfName )
        {
            pMatch = elem;
            break;
        }
    }

    if( !pMatch.IsEmpty() )
    {
        // overwrite the destination info
        oIfCfg.CopyProp(
            propDestDBusName, pMatch );

        oIfCfg.CopyProp(
            propObjPath, pMatch );
    }

    return 0;
}

gint32 CSimpAuthLoginProxy::InitUserFuncs()
{
    BEGIN_IFPROXY_MAP( IAuthenticate, false );

    ADD_PROXY_METHOD_ASYNC( 1,
        CSimpAuthLoginProxy::Login,
        SYS_METHOD( AUTH_METHOD_LOGIN ) );

    ADD_PROXY_METHOD_ASYNC( 1,
        CSimpAuthLoginProxy::MechSpecReq,
        SYS_METHOD( AUTH_METHOD_MECHSPECREQ ) );

    END_IFPROXY_MAP;
    return 0;
}

gint32 CSimpAuthLoginProxy::Login(
    IEventSink* pCallback,
    IConfigDb* pInfo, /*[ in ]*/
    CfgPtr& pResp ) /*[ out ]*/
{
    if( pInfo == nullptr )
        return -EINVAL;

    CCfgOpener oOptions;
    oOptions.SetStrProp( propIfName,
        DBUS_IF_NAME( "IAuthenticate" ) );
    oOptions.SetBoolProp( propSysMethod, true );
    return this->AsyncCall( pCallback,
        oOptions.GetCfg(), pResp,
        AUTH_METHOD_LOGIN, pInfo );
}

gint32 CSimpAuthLoginProxy::DoLogin(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        TaskletPtr pDeferTask;

        CfgPtr pResp;
        ret = DEFER_IFCALLEX_NOSCHED2(
            0, pDeferTask, ObjPtr( this ),
            &CSimpAuthLoginProxy::StartLogin,
            nullptr );

        if( ERROR( ret ) )
            break;

        CIfRetryTask* pRetryTask = pDeferTask;
        if( pRetryTask == nullptr )
        {
            ( *pDeferTask )( eventCancelTask );
            ret = -EFAULT;
            break;
        }

        pRetryTask->SetClientNotify( pCallback );

        // run this task on a standalone thread
        CIoManager* pMgr = GetIoMgr();
        ret = pMgr->RescheduleTask(
            pDeferTask, true );

        if( ERROR( ret ) )
        {
            ( *pDeferTask )( eventCancelTask );
            break;
        }

        ret = STATUS_PENDING;

    }while( 0 );
    return ret;
}

static gint32 ReadRegFile( RegFsPtr& pfs,
    const stdstr& strPath, BufPtr& pData )
{
    RFHANDLE hFile = INVALID_HANDLE;
    gint32 ret = 0;
    do{
        ret = pfs->OpenFile(
            strPath, O_RDONLY, hFile );
        if( ERROR( ret ) )
            break;
        struct stat st;
        ret = pfs->GetAttr( hFile, st );
        if( ERROR( ret ) )
            break;
        guint32 dwSize = st.st_size;
        if( dwSize == 0 )
            break;
        BufPtr pBuf( true );
        pBuf->Resize( dwSize );
        ret = pfs->ReadFile(
            hFile, pBuf->ptr(), dwSize, 0 );
        if( ERROR( ret ) )
            break;
        pData = pBuf;
    }while( 0 );
    if( hFile != INVALID_HANDLE )
        pfs->CloseFile( hFile );
    return ret;
}

static gint32 GetDefaultUser(
    RegFsPtr& pfs, stdstr& strDefault )
{
    RFHANDLE hFile = INVALID_HANDLE;
    gint32 ret = 0;

    const stdstr strDefCred = "/" SIMPAUTH_DIR
        "/credentials/.default";
    do{
        BufPtr pBuf;
        ret = ReadRegFile(
            pfs, strDefCred, pBuf );
        if( ERROR( ret ) )
            break;
        if( pBuf.IsEmpty() || pBuf->empty() )
        {
            ret = -ENODATA;
            break;
        }
        strDefault.append(
            pBuf->ptr(), pBuf->size() );
    }while( 0 );
    return ret;
}

static gint32 DecryptKey_OpenSSL( RegFsPtr& pfs,
    const BufPtr& pEncKey, BufPtr& pKey )
{
    gint32 ret = 0;
    do{
        if( pEncKey.IsEmpty() || pEncKey->empty() )
        {
            ret = -EINVAL;
            break;
        }

        stdstr strHome = GetHomeDir();
        strHome += "/.rpcf/openssl/";
        std::vector< stdstr > vecKeys = { "clientkey.pem" };

        BufPtr pDec( true );
        size_t dec_len = 0;
        for( auto& elem : vecKeys )
        {
            stdstr strKeyPath = strHome + elem;
            FILE *pri_fp = fopen(
                strKeyPath.c_str(), "r");

            if( pri_fp == nullptr )
                continue;

            EVP_PKEY *pri_pkey =
                PEM_read_PrivateKey(
                    pri_fp, NULL, NULL, NULL);

            fclose(pri_fp);

            EVP_PKEY_CTX *dctx =
                EVP_PKEY_CTX_new(pri_pkey, NULL);
            EVP_PKEY_decrypt_init(dctx);
            ret = EVP_PKEY_CTX_set_rsa_padding(
                dctx, RSA_PKCS1_OAEP_PADDING );
            if( ret <= 0 )
            {
                ret = -EFAULT;
                continue;
            }
            ret = EVP_PKEY_CTX_set_rsa_oaep_md(
                dctx, EVP_sha256() );
            if( ret <= 0 )
            {
                ret = -EFAULT;
                continue;
            }

            ret = EVP_PKEY_decrypt(dctx, NULL,
                &dec_len, ( guint8* )pEncKey->ptr(),
                pEncKey->size() );
            if( ret <= 0 )
                continue;

            std::vector<unsigned char>
                dec_data(dec_len);
            pDec->Resize( dec_len );

            ret = EVP_PKEY_decrypt(
                dctx, ( guint8* )pDec->ptr(),
                &dec_len, ( guint8* )pEncKey->ptr(),
                pEncKey->size());
            if( ret <= 0 )
            {
                unsigned long error = ERR_get_error(); 
                // Get a human-readable error string
                char error_string[256];
                ERR_error_string(error, error_string);
                OutputMsg(0, "Error message: %s\n", error_string);
            }
            EVP_PKEY_CTX_free(dctx);
            if( ret > 0 )
                break;
        }

        if( ret > 0 )
            pKey = pDec;

        else
            ret = -EACCES;
    }while( 0 );
    return ret;
}

static std::string GetClientRegPath()
{
    stdstr strCliReg = GetHomeDir();
    strCliReg += "/.rpcf/clientreg.dat";
    return strCliReg;
}

gint32 GetPassHash( stdstr& strUser,
    BufPtr& pPassHash, bool& bGmSSL )
{
    gint32 ret = 0;
    RegFsPtr pfs;
    do{
        stdstr strCliReg = GetHomeDir();
        strCliReg += "/.rpcf/clientreg.dat";

        const stdstr strCredDir =
            "/" SIMPAUTH_DIR "/credentials";

        CParamList oParams;
        oParams.SetStrProp( propConfigPath,
            GetClientRegPath() );
        ret = pfs.NewObj(
            clsid( CRegistryFs ),
            oParams.GetCfg() );
        if( ERROR( ret ) )
            break;

        ret = pfs->Start();
        if( ERROR( ret ) )
        {
            OutputMsg( ret, "Error start the registry "
            "and you may want to format the registry "
            "with command 'regfsmnt -i %s' first",
            strCliReg.c_str() );
            break;
        }

        if( strUser.empty() )
        {
            ret = GetDefaultUser( pfs, strUser );
            if( ERROR( ret ) )
            {
                OutputMsg( ret, "Error cannot find "
                    "default user to login with" );
                break;
            }
        }
        stdstr strPath =
            strCredDir + "/" + strUser;

        BufPtr pEncKey;
        ret = ReadRegFile( pfs,
            strPath + ".cred", pEncKey );

        if( ERROR( ret ) )
        {
            OutputMsg( ret,
                "Error read the key file %s",
                ( strPath + ".cred" ).c_str() );
            break;
        }

        BufPtr pEmpty;
        ret = ReadRegFile( pfs,
            strPath + ".gmssl", pEmpty );
        if( SUCCEEDED( ret ) )
            bGmSSL = true;
        else
            bGmSSL = false;

        if( bGmSSL )
            break;

        BufPtr pTextHash;
        ret = DecryptKey_OpenSSL(
            pfs, pEncKey, pTextHash );
        if( ERROR( ret ) )
        {
            OutputMsg( ret,
                "Error decrypt the key file %s",
                ( strPath + ".cred" ).c_str() );
            break;
        }
        BufPtr pBinHash( true );
        ret = pBinHash->Resize(
            pTextHash->size() / 2 );
        if( ERROR( ret ) )
            break;
        ret = HexStringToBytes(
            pTextHash->ptr(), pTextHash->size(),
            ( guint8* )pBinHash->ptr() );
        if( ERROR( ret ) )
            break;
        if( pBinHash->size() != 32 )
        {
            ret = -EINVAL;
            break;
        }
        pPassHash = pBinHash;

    }while( 0 );
    if( !pfs.IsEmpty() )
        pfs->Stop();
    return ret;
}
#include <openssl/rand.h>

// Encrypts pToken using AES-256-GCM with key pKey
// Output: [12-byte IV][ciphertext][16-byte tag]
gint32 EncryptToken(const BufPtr& pToken,
    const BufPtr& pKey, BufPtr& pEncrypted)
{
    if( pToken.IsEmpty() ||
        pKey.IsEmpty() ||
        pKey->size() != 32)
        return -EINVAL;

    const int nIvLen = 12;
    const int nTagLen = 16;
    unsigned char iv[nIvLen];
    if(RAND_bytes(iv, nIvLen) != 1)
        return -EFAULT;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
        return -ENOMEM;

    gint32 ret = 0;
    int len = 0;
    int ciphertext_len = 0;
    unsigned char tag[nTagLen];

    do {
        if( EVP_EncryptInit_ex( ctx,
            EVP_aes_256_gcm(),
            NULL, NULL, NULL) != 1)
        {
            ret = ERROR_FAIL;
            break;
        }
        if (EVP_EncryptInit_ex(ctx, NULL, NULL,
            ( guint8* )pKey->ptr(), iv ) != 1 )
        {
            ret = -EFAULT;
            break;
        }

        std::vector<unsigned char>
            ciphertext( pToken->size() + nTagLen );

        if( EVP_EncryptUpdate(ctx,
            ciphertext.data(), &len,
           ( unsigned char* )pToken->ptr(),
           pToken->size()) != 1)
        {
            ret = ERROR_FAIL;
            break;
        }
        ciphertext_len = len;

        if( EVP_EncryptFinal_ex(ctx,
            ciphertext.data() + len, &len) != 1)
        {
            ret = ERROR_FAIL;
            break;
        }
        ciphertext_len += len;

        if( EVP_CIPHER_CTX_ctrl( ctx,
            EVP_CTRL_GCM_GET_TAG,
            nTagLen, tag) != 1)
        {
            ret = ERROR_FAIL;
            break;
        }

        // Output: [IV][ciphertext][tag]
        pEncrypted.NewObj();

        pEncrypted->Resize(
            nIvLen + ciphertext_len + nTagLen );

        memcpy(pEncrypted->ptr(), iv, nIvLen );

        memcpy(pEncrypted->ptr() + nIvLen,
            ciphertext.data(), ciphertext_len );

        memcpy( pEncrypted->ptr() + nIvLen +
            ciphertext_len, tag, nTagLen);

        ret = nIvLen + ciphertext_len + nTagLen;

    } while (0);

    EVP_CIPHER_CTX_free(ctx);
    return ret;
}

gint32 CSimpAuthLoginProxy::BuildLoginInfo(
    const stdstr& strUser, CfgPtr& pInfo )
{
    gint32 ret = 0;
    do{
        BufPtr pKey;
        bool bGmSSL = false;
        stdstr strUser2 = strUser;
        ret = GetPassHash(
            strUser2, pKey, bGmSSL );
        if( ERROR( ret ) )
            break;

        Variant oVar;
        ret = this->GetProperty(
            propConnHandle, oVar );
        if( ERROR( ret ) )
            break;
        guint32 dwPortId = oVar;
        InterfPtr pIf;
        ret = m_pRouter->GetBridgeProxy(
            dwPortId, pIf );
        if( ERROR( ret ) )
            break;
        ret = pIf->GetProperty(
            propSessHash, oVar );
        if( ERROR( ret ) )
            break;

        CRpcTcpBridgeProxy* pbp = pIf;
        guint64 qwTs = pbp->GetPeerStartSec();

        CCfgOpener oLogin;
        oLogin.SetProperty( propSessHash, oVar );

        oLogin.SetStrProp(
            propUserName, strUser2 );

        oLogin.SetQwordProp(
            propTimestamp, qwTs );

        BufPtr pToken( true );
        ret = oLogin.Serialize( pToken );
        if( ERROR( ret ) )
            break;

        BufPtr pEncrypted;
        ret = EncryptToken(
            pToken, pKey, pEncrypted );

        if( ERROR( ret ) )
        {
            OutputMsg( ret,
                "Error encrypting the token" );
            break;
        }
        ret = pEncrypted->Resize( ret );
        if( ERROR( ret ) )
            break;
        CParamList oInfo( ( IConfigDb* )pInfo );
        oInfo.SetStrProp( propUserName, strUser2 );
        oInfo.Push( pEncrypted );
        oInfo.SetBoolProp( propGmSSL, bGmSSL );

    }while( 0 );
    return ret;
}

gint32 CSimpAuthLoginProxy::SimpAuthLogin(
    IEventSink* pCallback,
    IConfigDb* pInfo )
{
    gint32 ret = 0;
    do{
        CCfgOpener oResp;
        gint32 (*func)(CTasklet*,
            IEventSink*, CSimpAuthLoginProxy* ) =
        ([]( CTasklet* pCb,
            IEventSink* pLoginCb,
            CSimpAuthLoginProxy* pIf )->gint32
        {
            if( pCb == nullptr )
                return -EINVAL;

            gint32 ret = 0;
            do{
                CCfgOpener oCfg( ( IConfigDb* )
                    pCb->GetConfig() );
                IConfigDb* pResp = nullptr;
                ret = oCfg.GetPointer(
                    propRespPtr, pResp );
                if( ERROR( ret ) )
                    break;

                CCfgOpener oResp( pResp );
                gint32 iRet = 0;
                ret = oResp.GetIntProp(
                    propReturnValue, ( guint32& )iRet );
                if( ERROR( ret ) )
                    break;

                if( ERROR( iRet ) )
                {
                    ret = iRet;
                    break;
                }
                stdstr strSess;
                ret = oResp.GetStrProp(
                    propSessHash, strSess );
                if( ERROR( ret ) )
                    break;
                pIf->SetSess( strSess );
                DebugPrint( 0, "Sess hash is %s",
                    strSess.c_str() );
                Variant oVar( false );
                pIf->SetProperty( propNoEnc, oVar );
            }while( 0 );
            pLoginCb->OnEvent( eventTaskComp,
                ret, 0, nullptr );
            return 0;
        });

        TaskletPtr pLoginCb;
        ret = NEW_COMPLETE_FUNCALL( 0,
            pLoginCb, this->GetIoMgr(),
            func, nullptr, pCallback, this );
        if( ERROR( ret ) )
            break;

        ret = this->Login( pLoginCb,
            pInfo, oResp.GetCfg() );

        if( ret == STATUS_PENDING )
            break;

        if( ERROR( ret ) )
        {
            ( *pLoginCb )( eventCancelTask );
            break;
        }

        oResp.GetIntProp(
            propReturnValue, ( guint32& )ret );
        if( ERROR( ret ) )
            break;

        std::string strSess;
        ret = oResp.GetStrProp(
            propSessHash, strSess );
        DebugPrint( 0, "Sess hash is %s",
            strSess.c_str() );
        SetSess( strSess );

    }while( 0 );
    return ret;
}

gint32 CSimpAuthLoginProxy::StartLogin(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    bool bHooked = false;
    do{
        std::string strMech = GET_MECH( this );
        if( strMech != "SimpAuth" )
        {
            ret = -ENOTSUP;
            break;
        }

        CCfgOpener oAuth(
            ( IConfigDb* )GET_AUTH( this ) );

        stdstr strUser;
        oAuth.GetStrProp(
            propUserName, strUser );

        CParamList oParams;
        ret = BuildLoginInfo(
            strUser, oParams.GetCfg() );
        if( ERROR( ret ) )
            break;

        ret = this->SimpAuthLogin(
            pCallback, oParams.GetCfg() );

    }while( 0 );
    return ret;
}
}
