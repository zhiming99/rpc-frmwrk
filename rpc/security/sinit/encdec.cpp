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
extern std::string GetPubKeyPath();

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>

// Combine strPassword and SHA256(strSalt), then hash again
std::string GenPasswordSaltHash_OpenSSL(
    const std::string& strPassword,
    const std::string& strSalt )
{
    guint32 dwHash = 0;
    stdstr strSaltHash;
    std::string strRet;

    GenStrHash( strSalt, dwHash );
    BytesToString( ( guint8* )&dwHash,
        sizeof( dwHash ), strSaltHash );

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
        stdstr strPath = GetPubKeyPath();
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

int EncryptWithPubKey_GmSSL(
    const std::string& strPassword,
    const std::string& strSalt,
    BufPtr& pEncrypted )
{
    return 0;
}

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
