/*
 * =====================================================================================
 *
 *       Filename:  encdec.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/07/2025 11:25:59 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include <iostream>
#include <string>
#include <rpc.h>
using namespace rpcf;
extern std::string GetPubKeyPath( bool bGmSSL );

extern "C"{

#ifdef OPENSSL
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#endif

}

#ifdef OPENSSL
// Combine strPassword and SHA256(strSalt), then hash again
std::string GenPasswordSaltHash_OpenSSL(
    const std::string& strPassword,
    const std::string& strSalt )
{
    guint32 dwHash = 0;
    stdstr strSaltHash;
    std::string strRet;

    GenShaHash( strSalt.c_str(),
        strSalt.size(), strSaltHash );
    std::string combined = strPassword + strSaltHash;
    unsigned char finalHash[ SHA256_DIGEST_LENGTH ];
    SHA256(reinterpret_cast<const unsigned char*>(combined.data()),
        combined.size(), finalHash);

    BytesToString(finalHash,
        SHA256_DIGEST_LENGTH, strRet );
    return strRet;
}

int EncryptWithPubKey_OpenSSL(
    const std::string& strPassword,
    const std::string& strSalt,
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

        stdstr strHash = GenPasswordSaltHash_OpenSSL(
            strPassword, strSalt );

        size_t outlen = 0;
        if( EVP_PKEY_encrypt( pCtx, nullptr, &outlen,
             reinterpret_cast<const unsigned char*>(strPassword.data()),
             strPassword.size() ) <= 0 )
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
            reinterpret_cast<unsigned char*>(pEncrypted->ptr()),
            &outlen,
            reinterpret_cast<const unsigned char*>(strPassword.data()),
            strPassword.size() ) <= 0 )
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
#endif

#ifdef GMSSL
extern "C"
{
#include <gmssl/rand.h>
#include <gmssl/x509.h>
#include <gmssl/error.h>
#include <gmssl/sm2.h>
#include <gmssl/sm3.h>
#include <gmssl/sm4.h>
#include <gmssl/pem.h>
#include <gmssl/tls.h>
#include <gmssl/digest.h>
#include <gmssl/hmac.h>
#include <gmssl/hkdf.h>
#include <gmssl/mem.h>
}

std::string GenPasswordSaltHash_GmSSL(
    const std::string& strPassword,
    const std::string& strSalt )
{
    guint32 dwHash = 0;
    stdstr strSaltHash;
    std::string strRet;

    GenShaHash( strSalt.c_str(),
        strSalt.size(), strSaltHash );

    gint32 ret = 0;
    std::string combined = strPassword + strSaltHash;
    do{
        unsigned char dgst[ 32 ];
        SM3_DIGEST_CTX sm3_ctx;
        if( sm3_digest_init(&sm3_ctx, NULL, 0) != 1 )
        {
            ret = ERROR_FAIL;
            break;
        }
        if (sm3_digest_update(&sm3_ctx,
            (uint8_t *)combined.c_str(),
            combined.size()) != 1) 
        {
            ret = ERROR_FAIL;
            break;
        }

        if (sm3_digest_finish( &sm3_ctx, dgst ) != 1)
        {
            ret = ERROR_FAIL;
            break;
        }
        memset( &sm3_ctx, 0, sizeof( sm3_ctx ) );
        BytesToString(
            dgst, sizeof( dgst ), strRet );
    }while( 0 );
    return strRet;
}

int EncryptWithPubKey_GmSSL(
    const std::string& strPassword,
    const std::string& strSalt,
    BufPtr& pEncrypted )
{
    gint32 ret = 0;
    SM2_ENC_CTX ctx;
    stdstr strPassHash;
    do {
        strPassHash = GenPasswordSaltHash_GmSSL(
            strPassword, strSalt );
        if( strPassHash.empty() )
        {
            std::cerr << "Cannot failed to generate "
                "password hash.\n";
            ret = ERROR_FAIL;
            break;
        }
        uint8_t cert[18192];
        size_t certlen = 0;
        stdstr strPath = GetPubKeyPath( true );
        FILE* pFp = fopen( strPath.c_str(), "r" );
        if (!pFp) {
            std::cerr << "Error cannot open public "
                "key file.\n";
            ret = -errno;
            break;
        }
        ret = x509_cert_from_pem(
            cert, &certlen, sizeof(cert), pFp );
        fclose(pFp);
        if( ret != 1 )
        {
            if( ret < 0 )
            {
                std::cerr << "Error read certificate\n";
                break;
            }
            ret = 0;
            break;
        }

        SM2_KEY key;

        ret = x509_cert_get_subject_public_key(
            cert, certlen, &key);
        if( ret != 1 )
        {
            std::cerr << "Cannot extract public "
                "key from certificate.\n";
            ret = ERROR_FAIL;
            break;
        }

        ret = sm2_encrypt_init( &ctx );
        if( ret != 1 )
        {
            std::cerr << "Error sm2_encrypt_init failed.\n";
            ret = ERROR_FAIL;
            break;
        }

        if( sm2_encrypt_update( &ctx,
             reinterpret_cast<const unsigned char*>(strPassHash.data()),
             strPassHash.size() ) <= 0 )
         {
            std::cerr << "Error sm2_encrypt_update failed.\n";
            ret = -1;
            break;
        }

        if( pEncrypted.IsEmpty() )
            pEncrypted.NewObj();

        ret = pEncrypted->Resize(
            SM2_MAX_CIPHERTEXT_SIZE );
        if( ERROR( ret ) )
            break;
        size_t outlen = pEncrypted->size();
        ret = sm2_encrypt_finish( &ctx, &key,
            reinterpret_cast<unsigned char*>( pEncrypted->ptr() ),
            &outlen );
        if( ret != 1 )
        {
            std::cerr << "Error encryption failed\n";
            ret = ERROR_FAIL;
            break;
        }
        ret = pEncrypted->Resize( outlen );
    } while (0);
    gmssl_secure_clear( &ctx, sizeof( ctx ) );
    strPassHash.assign( strPassHash.size(), ' ' );
    return ret;
}
#endif

int EncryptWithPubKey(
    const std::string& strPassword,
    const std::string& strSalt,
    BufPtr& pEncrypted )
{
#ifdef OPENSSL
    return EncryptWithPubKey_OpenSSL(
        strPassword, strSalt, pEncrypted );
#elif defined( GMSSL )
    return EncryptWithPubKey_GmSSL(
        strPassword, strSalt, pEncrypted );
#else
    std::cerr << "No crypto library defined.\n";
    return -ENOTSUP;
#endif
}
