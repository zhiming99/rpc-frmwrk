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
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 * =====================================================================================
 */
#include "ifhelper.h"
#include "dbusport.h"
#include "sslfido.h"

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
    default:
        {
            ret = ERROR_FAIL;
            break;
        }
    }

    return ret;
}

gint32 CRpcOpenSSLFidoDrv::InitSSLContext()
{
    gint32 ret = 0;
    do{
        DebugPrint( 0, "initialising SSL\n");

        /*  SSL library initialisation */
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();
        ERR_load_BIO_strings();
        ERR_load_crypto_strings();

        /*  create the SSL server
        *  context */
        m_pSSLCtx = SSL_CTX_new( SSLv23_method() );
        if (!m_pSSLCtx)
        {
            ret = ERROR_FAIL;
            break;
        }


        /*  Load certificate and private key
         *  files, and check consistency */
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
            DebugPrint( ret,
                "SSL_CTX_check_private_key failed" );
            break;
        }

        DebugPrint( 0, "certificate and private"
            "key loaded and verified");

        /*  Recommended to avoid SSLv2 & SSLv3 */
        SSL_CTX_set_options( m_pSSLCtx, SSL_OP_ALL
            | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);

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
        m_pSSL = SSL_new( m_pSSLCtx );
        if( IsClient() )
        {
            SSL_set_connect_state( m_pSSL );
        }
        else
        {
            SSL_set_accept_state( m_pSSL );
        }

        SSL_set_bio( m_pSSL, m_prbio, m_pwbio );

    }while( 0 );

    return ret;
}


