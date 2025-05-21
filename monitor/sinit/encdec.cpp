/*
 * =====================================================================================
 *
 *       Filename:  encdec.cpp
 *
 *    Description:  Encryption and decryption functions using OpenSSL
 *
 *        Version:  1.0
 *        Created:  05/07/2025 11:25:59 PM
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

#include <iostream>
#include <string>
#include <rpc.h>

namespace rpcf
{
std::string GetPubKeyPath( bool bGmSSL )
{
    stdstr strPath = GetHomeDir();
    if( bGmSSL )
        strPath += "/.rpcf/gmssl/clientcert.pem" ;
    else
        strPath += "/.rpcf/openssl/clientcert.pem" ;
    return strPath;
}
std::string GetPrivKeyPath( bool bGmSSL )
{
    stdstr strPath = GetHomeDir();
    if( bGmSSL )
        strPath += "/.rpcf/gmssl/clientkey.pem" ;
    else
        strPath += "/.rpcf/openssl/clientkey.pem" ;
    return strPath;
}

#ifdef OPENSSL
extern "C"{
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/rand.h>

}

gint32 GenSha2Hash_OpenSSL(
    const stdstr& strData, stdstr& strHash )
{

    guint8 finalHash[ SHA256_DIGEST_LENGTH ];
    SHA256(reinterpret_cast<const guint8*>(strData.data()),
        strData.size(), finalHash);
    gint32 ret = BytesToString(finalHash,
        SHA256_DIGEST_LENGTH, strHash );
    return ret;
}

// Combine strPassword and SHA256(strSalt), then hash again
std::string GenPasswordSaltHash_OpenSSL(
    const std::string& strPassword,
    const std::string& strSalt )
{
    guint32 dwHash = 0;
    stdstr strSaltHash;
    std::string strRet;

    GenSha2Hash_OpenSSL( strSalt, strSaltHash );

    stdstr strPassHash;
    GenSha2Hash_OpenSSL( strPassword, strPassHash );

    std::string combined = strPassHash + strSaltHash;
    GenSha2Hash_OpenSSL( combined, strRet );
    return strRet;
}

//        stdstr strHash = GenPasswordSaltHash_OpenSSL(
//            strPassword, strSalt );

int EncryptWithPubKey_OpenSSL(
    BufPtr& pBlock,
    BufPtr& pEncrypted )
{
    int ret = 0;
    EVP_PKEY* pPubKey = nullptr;
    EVP_PKEY_CTX* pCtx = nullptr;
    FILE* pFp = nullptr;
    X509* pCert = nullptr;

    do {
        stdstr strPath = GetPubKeyPath( false );
        pFp = fopen( strPath.c_str(), "r" );
        if (!pFp) {
            std::cerr << "Cannot open public "
                "key file.\n";
            ret = -errno;
            break;
        }
        pCert = PEM_read_X509(
            pFp, nullptr, nullptr, nullptr);
        fclose(pFp);
        if (!pCert) {
            std::cerr << "Cannot read X509 "
                "certificate.\n";
            ret = -ENODATA;
            break;
        }
        pPubKey = X509_get_pubkey(pCert);
        X509_free(pCert);
        if( !pPubKey )
        {
            std::cerr << "Cannot extract public "
                "key from certificate.\n";
            ret = -ENODATA;
            break;
        }

        pCtx = EVP_PKEY_CTX_new( pPubKey, nullptr );
        if (!pCtx)
        {
            std::cerr << "Cannot create EVP_PKEY_CTX.\n";
            ret = -ENOMEM;
            break;
        }

        if( EVP_PKEY_encrypt_init( pCtx ) <= 0 )
        {
            std::cerr <<
                "EVP_PKEY_encrypt_init failed.\n";
            ret = -1;
            break;
        }
        if( EVP_PKEY_CTX_set_rsa_padding(
            pCtx, RSA_PKCS1_OAEP_PADDING ) <= 0 )
        {
            std::cerr <<
                "EVP_PKEY_CTX_set_rsa_padding failed.\n";
            ret = -1;
            break;
        }

        size_t outlen = 0;
        if( EVP_PKEY_encrypt( pCtx, nullptr, &outlen,
             ( guint8* )pBlock->ptr(),
             pBlock->size() ) <= 0 )
         {
            std::cerr << "EVP_PKEY_encrypt "
                "(size query) failed.\n";
            ret = -1;
            break;
        }

        if( pEncrypted.IsEmpty() )
            pEncrypted.NewObj();
        ret = pEncrypted->Resize( outlen );
        if (ERROR(ret))
            break;

        if (EVP_PKEY_encrypt(pCtx,
            ( guint8* )pEncrypted->ptr(),
            &outlen,
            ( guint8* )pBlock->ptr(),
            pBlock->size() ) <= 0 )
        {
            char* szError = ERR_error_string(
                ERR_get_error(), nullptr);
            std::cerr << "Encryption failed: "
                << szError << "\n";
            ret = ERROR_FAIL;
            break;
        }
        ret = pEncrypted->Resize( outlen );
    } while (0);

    if( pCtx )
        EVP_PKEY_CTX_free( pCtx );
    if( pPubKey )
        EVP_PKEY_free( pPubKey );

    return ret;
}

// Encrypts pToken using AES-256-GCM with key pKey
// Output: [12-byte IV][ciphertext][16-byte tag]
gint32 EncryptAesGcmBlock_OpenSSL(
    const BufPtr& pToken,
    const BufPtr& pKey, BufPtr& pEncrypted )
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

static gint32 DecryptWithPrivKey_OpenSSL(
    const BufPtr& pEncKey, BufPtr& pKey )
{
    gint32 ret = 0;
    do{
        if( pEncKey.IsEmpty() || pEncKey->empty() )
        {
            ret = -EINVAL;
            break;
        }

        BufPtr pDec( true );
        size_t dec_len = 0;
        
        do{
            stdstr strKeyPath = GetPrivKeyPath( false );
            FILE *pri_fp = fopen(
                strKeyPath.c_str(), "r");

            if( pri_fp == nullptr )
            {
                ret = -EFAULT;
                break;
            }

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
                break;
            }

            ret = EVP_PKEY_decrypt(dctx, NULL,
                &dec_len, ( guint8* )pEncKey->ptr(),
                pEncKey->size() );
            if( ret <= 0 )
            {
                ret = -EFAULT;
                break;
            }

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
            pDec->Resize( dec_len );
            EVP_PKEY_CTX_free(dctx);

        }while( 0 );

        if( ret > 0 )
        {
            pKey = pDec;
            ret = STATUS_SUCCESS;
        }
        else
            ret = -EACCES;
    }while( 0 );
    return ret;
}

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

#endif

#ifdef GMSSL
gint32 EncryptWithPubKey_GmSSL(
    BufPtr& pBlock, BufPtr& pEncrypted );
gint32 EncryptAesGcmBlock_GmSSL(
    const BufPtr& pToken,
    const BufPtr& pKey,
    BufPtr& pEncrypted );
gint32 DecryptWithPrivKey_GmSSL(
    const BufPtr& pEncKey, BufPtr& pKey );
gint32 DecryptAesGcmBlock_GmSSL(
    const BufPtr& pKey,
    const BufPtr& pToken,
    BufPtr& outPlaintext );
gint32 GenSha2Hash_GmSSL(
    const stdstr& strData, stdstr& strHash );
stdstr GenPasswordSaltHash_GmSSL(
    const std::string& strPassword,
    const std::string& strSalt );
#endif

gint32 EncryptWithPubKey(
    BufPtr& pBlock,
    BufPtr& pEncrypted,
    bool bGmSSL )
{
    gint32 ret = 0;
    do{
        if( bGmSSL )
        {
#ifdef GMSSL
            ret = EncryptWithPubKey_GmSSL(
                pBlock, pEncrypted );
#else
            ret = -ENOTSUP;
#endif
        }
        else
        {
#ifdef OPENSSL
            ret = EncryptWithPubKey_OpenSSL(
                pBlock, pEncrypted );
#else
            ret = -ENOTSUP;
#endif
        }
    }while( 0 );
    if( ret == -ENOTSUP )
        std::cerr << "No crypto library defined.\n";
    return ret;
}

gint32 EncryptAesGcmBlock(
    const BufPtr& pBlock,
    const BufPtr& pKey,
    BufPtr& pEncrypted,
    bool bGmSSL )
{
    gint32 ret = 0;
    if( bGmSSL )
#ifdef GMSSL
        ret = EncryptAesGcmBlock_GmSSL(
            pBlock, pKey, pEncrypted );
#else
        ret = -ENOTSUP;
#endif
    else
#ifdef OPENSSL
        ret = EncryptAesGcmBlock_OpenSSL(
            pBlock, pKey, pEncrypted );
#else
        ret = -ENOTSUP;
#endif
    return ret;
}

gint32 DecryptWithPrivKey(
    const BufPtr& pEncrypted,
    BufPtr& pBlock,
    bool bGmSSL )
{
    gint32 ret = 0;
    if( bGmSSL )
#ifdef GMSSL
        ret = DecryptWithPrivKey_GmSSL(
            pEncrypted, pBlock );
#else
        ret = -ENOTSUP;
#endif
    else
#ifdef OPENSSL
        ret = DecryptWithPrivKey_OpenSSL(
            pEncrypted, pBlock );
#else
        ret = -ENOTSUP;
#endif
    return ret;
}

gint32 DecryptAesGcmBlock(
    const BufPtr& pKey,
    const BufPtr& pToken,
    BufPtr& outPlaintext,
    bool bGmSSL )
{
    gint32 ret = 0;
    if( bGmSSL )
#ifdef GMSSL
        ret = DecryptAesGcmBlock_GmSSL(
            pKey, pToken, outPlaintext );
#else
        ret = -ENOTSUP;
#endif
    else
#ifdef OPENSSL
        ret = DecryptAesGcmBlock_OpenSSL(
            pKey, pToken, outPlaintext );
#else
        ret = -ENOTSUP;
#endif
    return ret;
}

gint32 GenSha2Hash(
    const stdstr& strData,
    stdstr& strHash,
    bool bGmSSL )
{
    gint32 ret = 0;
    if( bGmSSL )
#ifdef GMSSL
        ret = GenSha2Hash_GmSSL(
            strData, strHash );
#else
        ret = -ENOTSUP;
#endif
    else
#ifdef OPENSSL
        ret = GenSha2Hash_OpenSSL(
            strData, strHash );
#else
        ret = -ENOTSUP;
#endif
    return ret;
}

std::string GenPasswordSaltHash(
    const std::string& strPassword,
    const std::string& strSalt,
    bool bGmSSL )
{

    stdstr strEmpty;
    if( bGmSSL )
#ifdef GMSSL
        return GenPasswordSaltHash_GmSSL(
            strPassword, strSalt );
#else
        return strEmpty;
#endif
#ifdef OPENSSL
    return GenPasswordSaltHash_OpenSSL(
        strPassword, strSalt );
#else
    return strEmpty;
#endif
}

}
