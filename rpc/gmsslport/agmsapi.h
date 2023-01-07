/*
 * =====================================================================================
 *
 *       Filename:  agmsapi.h
 *
 *    Description:  Declarations of async api for gmssl
 *
 *        Version:  1.0
 *        Created:  01/07/2023 09:10:44 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2023 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */

#pragma once
namespace gmsa
{
    struct BIO
    {
        int do_read ( BIO* b, void*&  data, int dlen);
        int do_write( BIO* b, void* data, int dlen);
    };

    struct AGMS_METHOD
    {
        int ( *do_init )( AGMS* conn );
        int ( *do_handshake_connect )( AGMS* conn);
        
        int ( *do_handshake_accept )( AGMS* conn);

        int ( *do_shutdown )( AGMS* conn );

        int ( *do_recv )( AGMS* conn,
            uint8_t *& out, size_t outlen,
            size_t *recvlen);

        int ( *do_send )( AGMS* conn,
            uint8_t *in, size_t inlen,
            size_t *sentlen);
    };

    enum AGMS_STATE : guint32
    {
        STAT_INIT,
        STAT_START_HANDSHAKE,
        STAT_READY,
        STAT_HANDSHAKE_FAILED,
        STAT_SHUTDOWN
    };

    struct AGMS_CTX : TLS_CTX
    {
    };

    struct AGMS : public TLS_CONNECT
    {
        AGMS_STATE gms_state;
        std::unique_ptr< AGMS_CTX > gms_ctx;
    };

    struct AGMS_IOV
    {
        void* ptr;
        guint32 mem_size;
        guint32 start;
        guint32 end;
        ~AGMS_IOV()
        {
            if( ptr ) free( ptr );
            ptr = nullptr;
        }

        gint32 attach( void* ptr,
            guint32 mem_size,
            guint32 start = 0,
            guint32 end = 0 );

        gint32 detach( void** pptr,
            guint32& mem_size,
            guint32& start,
            guint32& end );
    };

    int AGMS_get_error(const AGMS *s, int ret_code);
    void AGMS_library_init();
    int AGMS_is_init_finished(const AGMS *s);

    AGMS_CTX *AGMS_CTX_new(const AGMS_METHOD *meth);
    int AGMS_use_PrivateKey_file(AGMS *gms, const char *file, int type);
    int AGMS_use_certificate_file(AGMS *gms, const char *file, int type);

    int AGMS_CTX_check_private_key(const AGMS_CTX *ctx);

    unsigned long AGMS_CTX_set_options(AGMS_CTX *ctx, unsigned long op);
    int AGMS_CTX_set_cipher_list(AGMS_CTX *, const char *str);

    int AGMS_CTX_up_ref(AGMS_CTX *ctx);
    void AGMS_CTX_free(AGMS_CTX *);

    void AGMS_add_all_algorithms();
    void AGMS_load_error_strings();
    void ERR_load_BIO_strings();
    void ERR_load_crypto_strings();

    AGMS *AGMS_new(AGMS_CTX *ctx);

    void AGMS_set_connect_state(AGMS *s);
    void AGMS_set_accept_state(AGMS *s);

    void AGMS_set0_rbio(AGMS *s, BIO *rbio);
    void AGMS_set0_wbio(AGMS *s, BIO *wbio);

    int AGMS_write(AGMS *gms, void *buf, int num);
    int AGMS_read(AGMS *gms, void *& buf, int num);

    int AGMS_pending(const AGMS *s);
    int AGMS_do_handshake(AGMS *s);
    int AGMS_shutdown(AGMS *s);

    BIO *BIO_new(const BIO_METHOD *type);
    int BIO_free(BIO *a);

    int BIO_read(BIO *b, void *& data, int dlen);
    {
        return b->do_read(
            b, data, dlen );
    }

    int BIO_write(BIO *b, void *data, int dlen);
    {
        return b->do_write(
            b, data, dlen );
    }

    const BIO_METHOD *BIO_s_mem(void);

    const AGMS_METHOD *TLS_method(void);
    const AGMS_METHOD *TLS_server_method(void);
    const AGMS_METHOD *TLS_client_method(void);
}
