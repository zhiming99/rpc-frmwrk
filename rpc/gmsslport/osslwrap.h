#pragma once
namespace gmssl
{

    struct BIO_METHOD
    {
        int ( *do_read )(BIO* b, void*&  data, int dlen);
        int ( *do_write )(BIO* b, void* data, int dlen);
    };

    struct BIO
    {
        BIO_METHOD bio_methods;
    };

    struct SSL_METHOD
    {
        int ( *do_init )( SSL* conn );
        int ( *do_handshake_connect )( SSL* conn);
        
        int ( *do_handshake_accept )( SSL* conn);

        int ( *do_shutdown )( SSL* conn );

        int ( *do_recv )( SSL* conn,
            uint8_t *& out, size_t outlen,
            size_t *recvlen);

        int ( *do_send )( SSL* conn,
            uint8_t *in, size_t inlen,
            size_t *sentlen);
    };

    enum SSL_STATE : guint32
    {
        STAT_INIT,
        STAT_START_HANDSHAKE,
        STAT_READY,
        STAT_HANDSHAKE_FAILED,
        STAT_SHUTDOWN
    };

    struct SSL_CTX : TLS_CTX
    {
    };

    struct SSL : public TLS_CONNECT
    {
        SSL_STATE ssl_state;
        std::unique_ptr< SSL_CTX > ssl_ctx;
    };

    int SSL_get_error(const SSL *s, int ret_code);
    void SSL_library_init();
    int SSL_is_init_finished(const SSL *s);

    SSL_CTX *SSL_CTX_new(const SSL_METHOD *meth);
    int SSL_use_PrivateKey_file(SSL *ssl, const char *file, int type);
    int SSL_use_certificate_file(SSL *ssl, const char *file, int type);

    int SSL_CTX_check_private_key(const SSL_CTX *ctx);

    unsigned long SSL_CTX_set_options(SSL_CTX *ctx, unsigned long op);
    int SSL_CTX_set_cipher_list(SSL_CTX *, const char *str);

    int SSL_CTX_up_ref(SSL_CTX *ctx);
    void SSL_CTX_free(SSL_CTX *);

    void OpenSSL_add_all_algorithms();
    void SSL_load_error_strings();
    void ERR_load_BIO_strings();
    void ERR_load_crypto_strings();

    SSL *SSL_new(SSL_CTX *ctx);

    void SSL_set_connect_state(SSL *s);
    void SSL_set_accept_state(SSL *s);

    void SSL_set0_rbio(SSL *s, BIO *rbio);
    void SSL_set0_wbio(SSL *s, BIO *wbio);

    int SSL_write(SSL *ssl, void *buf, int num);
    int SSL_read(SSL *ssl, void *& buf, int num);

    int SSL_pending(const SSL *s);
    int SSL_do_handshake(SSL *s);
    int SSL_shutdown(SSL *s);

    BIO *BIO_new(const BIO_METHOD *type);
    int BIO_free(BIO *a);

    int BIO_read(BIO *b, void *& data, int dlen);
    {
        return b->bio_methods.do_read(
            b, data, dlen );
    }

    int BIO_write(BIO *b, void *data, int dlen);
    {
        return b->bio_methods.do_write(
            b, data, dlen );
    }

    const BIO_METHOD *BIO_s_mem(void);

    const SSL_METHOD *TLS_method(void);
    const SSL_METHOD *TLS_server_method(void);
    const SSL_METHOD *TLS_client_method(void);
}
