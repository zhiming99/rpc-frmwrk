/*
 * =====================================================================================
 *
 *       Filename:  sslutils.cpp
 *
 *    Description:  OpenSSL helpers
 *
 *        Version:  1.0
 *        Created:  11/27/2019 12:53:30 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com ), 
 *
 *      
 *       this file is adapted from Darren Smith's project
 *       @https://github.com/darrenjs/openssl_examples
 *
 *       Copyright (c) 2017 Darren Smith ssl_examples is free software; you can
 *       redistribute it and/or modify it under the terms of the MIT license.
 *       See LICENSE for details.
 *      
 *
 * =====================================================================================
 */
#include "ifhelper.h"
#include "dbusport.h"
#include "sslfido.h"

using namespace rpcf;
extern char g_szKeyPass[SSL_PASS_MAX + 1];

namespace rpcf
{

gint32 GetSSLError( SSL* pssl, int n )
{
    gint32 ret = 0;
    if( pssl == nullptr )
        return -EINVAL;

    switch( SSL_get_error( pssl, n ) )
    {
    case SSL_ERROR_NONE:
        {
            ret = STATUS_SUCCESS;
            break;
        }
    case SSL_ERROR_WANT_WRITE:
        {
            ret = SSL_ERROR_WANT_WRITE;
            break;
        }
    case SSL_ERROR_WANT_READ:
        {
            ret = SSL_ERROR_WANT_READ;
            break;
        }
    case SSL_ERROR_SYSCALL:
        {
            ret = ERROR_FAIL;
            if( errno != 0 )
                ret = -errno;
            break;
        }
    case SSL_ERROR_ZERO_RETURN:
        {
            // the SSL connection is down
            ret = -ENOTCONN;
            break;
        }
    case SSL_ERROR_SSL:
        {
            ret = -EPROTO;
            break;
        }
    default:
        {
            ret = ERROR_FAIL;
            break;
        }
    }

    return ret;
}

gint32 CRpcOpenSSLFidoDrv::HandleKeyPasswd()
{
    gint32 ret = 0;
    char* szPass = m_szPasswd;
    do{
        if( m_strSecretPath == "1234" )
            break;

        if( m_strSecretPath == "console" )
        {
            char* pPass = g_szKeyPass;
            if( pPass[ 0 ] == 0 )
            {
                ret = -EACCES;
                break;
            }
            memcpy( szPass,
                pPass, SSL_PASS_MAX );
        }
        else
        {
            FILE* fp = fopen(
                m_strSecretPath.c_str(), "r" );
            if( fp == nullptr )
            {
                ret = -errno;
                break;
            }
            char* pszPass = nullptr;
            size_t len = 0;
            ret = getline( &pszPass, &len , fp );
            fclose( fp );
            fp = nullptr;
            if( ret == -1 )
            {
                if( pszPass )
                    free( pszPass );
                ret = -errno;
                break;
            }

            if( len == 0 )
            {
                // assuming no passwd
                break;
            }

            size_t actlen = 
                std::min( len, ( size_t )SSL_PASS_MAX );
            memcpy( szPass, pszPass, actlen );

            for( size_t i = 0; i < len; i++ )
                pszPass[ i ] = ' ';
            free( pszPass );
            pszPass = nullptr;

            char* ptail = szPass + actlen - 1;
            while( ptail >= szPass )
            {
                if( *ptail == '\n' || *ptail == '\r' )
                {
                    *ptail = 0;
                    --ptail;
                    continue;
                }
                break;
            }
            if( szPass[ 0 ] == 0 )
            {
                ret = -EACCES;
                break;
            }
        }

    }while( 0 );

    memset( g_szKeyPass, 0, SSL_PASS_MAX + 1 );
    return ret;
}

gint32 CRpcOpenSSLFidoDrv::InitSSLContext(
    bool bServer )
{
    gint32 ret = 0;
    do{
        DebugPrint( 0, "initialising SSL...\n");

        /*  SSL library initialisation */
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();
        ERR_load_BIO_strings();
        ERR_load_crypto_strings();

        /* create the SSL server context */
        if( bServer )
        {
            m_pSSLCtx = SSL_CTX_new(
                TLS_method() );
        }
        else
        {
            m_pSSLCtx = SSL_CTX_new(
                TLS_client_method() );
        }

        if( m_pSSLCtx == nullptr )
        {
            ret = ERROR_FAIL;
            break;
        }


        /*  Load certificate and private key
         *  files, and check consistency */
        if( 1 )
        {
            if( m_strCAFile.size() )
            {
                ret = SSL_CTX_load_verify_locations(
                    m_pSSLCtx, m_strCAFile.c_str(),NULL);
                DebugPrint( ret,
                    "SSL_CTX_load_verify_locations failed");
            }

            if( m_bVerifyPeer )
            {
                SSL_CTX_set_verify( m_pSSLCtx,
                    SSL_VERIFY_PEER |
                    SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
                    NULL);
            }
            ret = 0;
            HandleKeyPasswd();
            if( m_szPasswd[ 0 ] != 0 )
            {
                int ( *passwd_cb )( char*, int, int, void* ) =
                ([](char *buf, int size,
                    int rwflag, void *userdata)->int
                {
                    if( rwflag == 1 )
                        return 0;
                    int len = std::min( size,
                        ( int )strlen( ( char* )userdata ) );
                    memcpy( buf, userdata, len );
                    return len;
                });

                SSL_CTX_set_default_passwd_cb(
                    m_pSSLCtx, passwd_cb );
                SSL_CTX_set_default_passwd_cb_userdata(
                    m_pSSLCtx, m_szPasswd );
            }

            if( SSL_CTX_use_certificate_file(
                m_pSSLCtx, m_strCertPath.c_str(),
                SSL_FILETYPE_PEM) != 1 )
            {
                ret = -ENOTSUP;
                DebugPrint( ret,
                    "SSL_CTX_use_certificate_file failed");
                break;
            }

            if( SSL_CTX_use_PrivateKey_file(
                m_pSSLCtx, m_strKeyPath.c_str(),
                SSL_FILETYPE_PEM ) != 1 )
            {
                ret = -ENOENT;
                DebugPrint( ret,
                    "SSL_CTX_use_PrivateKey_file failed" );
                break;
            }

            //  Make sure the key and certificate
            //  file match.
            if( SSL_CTX_check_private_key(
                m_pSSLCtx ) != 1 )
            {
                ret = ERROR_FAIL;
                OutputMsg( ret,
                    "SSL_CTX_check_private_key failed" );
                break;
            }

            DebugPrint( 0, "certificate and private"
                "key loaded and verified");
        }

        /*  Recommended to avoid SSLv2 & SSLv3 */
        SSL_CTX_set_options( m_pSSLCtx, SSL_OP_ALL
             | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
        //   | SSL_OP_NO_TLSv1_2 | SSL_OP_NO_TLSv1_3);

        // uncomment this for wireshark
        // decryption, that is, wireshark support
        // decode of rsa+aes ciphers. Many other
        // ciphers, especially Diffie–Hellman
        // ciphers cannot be decoded by wireshark.
        // SSL_CTX_set_cipher_list( m_pSSLCtx,
        //     "AES128-SHA:AES256-SHA" );
        memset( m_szPasswd, 0, sizeof( m_szPasswd ) );

   }while( 0 );

   return ret;
}

gint32 CRpcOpenSSLFido::InitSSL()
{
    gint32 ret = 0;
    do{
        m_pwbio = BIO_new( BIO_s_mem() );
        if( unlikely( m_pwbio == nullptr ) )
        {
            ret = -ENOMEM;
            break;
        }
        m_prbio = BIO_new( BIO_s_mem() );
        if( unlikely( m_prbio == nullptr ) )
        {
            ret = -ENOMEM;
            break;
        }

        CPortDriver* pPortDrv =
        static_cast< CPortDriver* >( m_pDriver );

        CStdRMutex oLock( pPortDrv->GetLock() );
        m_pSSL = SSL_new( m_pSSLCtx );

        if( IsClient() )
        {
            SSL_set_connect_state( m_pSSL );
        }
        else
        {
            SSL_set_accept_state( m_pSSL );
        }

        SSL_set0_rbio( m_pSSL, m_prbio );
        SSL_set0_wbio( m_pSSL, m_pwbio );

    }while( 0 );

    return ret;
}

}
