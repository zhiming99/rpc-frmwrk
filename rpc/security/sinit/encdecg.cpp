/*
 * =====================================================================================
 *
 *       Filename:  encdecg.cpp
 *
 *    Description:  Encryption and decryption functions using GmSSL
 *
 *        Version:  1.0
 *        Created:  05/09/2025 10:00:56 PM
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
using namespace rpcf;
extern std::string GetPubKeyPath( bool bGmSSL );

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
#include <gmssl/digest.h>
#include <gmssl/hmac.h>
#include <gmssl/hkdf.h>
#include <gmssl/mem.h>
#include <gmssl/sha2.h>
}

gint32 GenSha2Hash_GmSSL(
    const stdstr& strData, stdstr& strHash )
{
    gint32 ret = 0;
    do{
        guint8 dgst[ SHA256_DIGEST_SIZE ];
        SHA256_CTX ctx;
        sha256_init( &ctx );
        sha256_update( &ctx,
            ( guint8 *)strData.c_str(),
            strData.size() );
        sha256_finish( &ctx, dgst );
        ret = BytesToString(
            dgst, sizeof( dgst ), strHash );
    }while( 0 );
    return ret;
}

std::string GenPasswordSaltHash_GmSSL(
    const std::string& strPassword,
    const std::string& strSalt )
{
    guint32 dwHash = 0;
    stdstr strSaltHash;
    std::string strRet;

    GenSha2Hash_GmSSL( strSalt, strSaltHash );
    
    stdstr strPassHash;
    GenSha2Hash_GmSSL( strPassword, strPassHash );

    std::string combined = strPassHash + strSaltHash;
    GenSha2Hash_GmSSL( combined, strRet );
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
