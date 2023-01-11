/*
 * =====================================================================================
 *
 *       Filename:  agmsapi.h
 *
 *    Description:  Declarations of classes for GmSSL Async API
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
#include <stdint.h>
namespace gmssl
{

using PIOVE=std::unique_ptr< AGMS_IOVE > ;
using IOV = std::deque< PIOVE >;

struct BLKIO_BASE
{
    IOV io_vec;
    virtual int read ( PIOVE& iover ) = 0;
    virtual int write( PIOVE& iovew ) = 0;
    size_t size();

    protected:
    bool is_partial_record();
    size_t record_size(); 
};

// incoming message queue
struct BLKIN : public BLKIO_BASE
{
    int write( PIOVE& iovew ) override;
    int read( PIOVE& iover ) override;
};

struct BLKOUT : public BLKIO_BASE
{
    int write( PIOVE& iovew ) override;
    int read( PIOVE& iover ) override;
};

enum AGMS_STATE : uint32_t
{
    STAT_INIT,
    STAT_START_HANDSHAKE,
    STAT_READY,
    STAT_HANDSHAKE_FAILED,
    STAT_SHUTDOWN,
    STAT_WAIT_SVR_HELLO,
    STAT_WAIT_SVR_ENCEXT,
    STAT_WAIT_SVR_CERT,
    STAT_WAIT_SVR_CERTVERIFY,
    STAT_WAIT_SVR_FIN,
    STAT_WAIT_CLI_HELLO,
    STAT_WAIT_CLI_CERT,
    STAT_WAIT_CLI_CERTVERIFY,
    STAT_WAIT_CLI_FIN,
};

struct AGMS_CTX : TLS_CTX
{
    AGMS_CTX();
    int check_private_key();
    unsigned long set_options( unsigned long op);
    int set_cipher_list( const char *str);
    int up_ref();
    void down_ref();
};

struct AGMS_IOVE
{
    void* ptr = nullptr;
    uint32_t mem_size = 0;
    uint32_t start = 0;
    uint32_t end = 0;

    ~AGMS_IOVE()
    { clear(); }

    void clear()
    {
        if( ptr )
        {
            free( ptr );
            ptr = nullptr;
        }
        mem_size = start = end = 0;
    }

    bool empty()
    {
        return ( ptr == nullptr ||
            end - start == 0 );
    }

    int alloc( uint32_t size = 0 );
    int realloc( uint32_t newsize  );

    int attach( void* ptr,
        uint32_t size,
        uint32_t start = 0,
        uint32_t end = 0 );

    int detach( void** pptr,
        uint32_t& size,
        uint32_t& start,
        uint32_t& end );

    size_t size() const
    { return end - start; }

    int split(
        AGMS_IOVE& bottom_half
        uint32_t offset );

    int copy( void* src,
        uint32_t src_size = 0 );

    int merge(
        AGMS_IOVE& bottom_half );
};

struct AGMS : public TLS_CONNECT
{
    std::atomic< AGMS_STATE > gms_state;
    std::unique_ptr< AGMS_CTX > gms_ctx;

    AGMS( AGMS_CTX *ctx );

    virtual int init() = 0;
    virtual int handshake() = 0;
    virtual int shutdown() = 0;

    virtual int recv( PIOVE& iove ) = 0;
    virtual int send( PIOVE& iove ) = 0;

    int get_error(int ret_code) const;

    void set_state( AGMS_STATE state );
    AGMS_STATE get_state() const
    { return gms_state; }

    void library_init();
    bool is_init_finished() const
    { return get_state() == STAT_READY; }

    int use_PrivateKey_file(const char *file, int type);
    int use_certificate_file(const char *file, int type);
    void add_all_algorithms();
    void load_error_strings();
    void set_connect_state();
    void set_accept_state();
    bool is_client() const;

    void set0_rbio( BLKIO *rbio);
    void set0_wbio( BLKIO *wbio);

    int pending_bytes();
};

struct TLS13_HSCTX_BASE
{
    uint8_t psk[32] = {0}; 
    uint8_t early_secret[32];
    uint8_t handshake_secret[32];
    uint8_t master_secret[32];
    uint8_t client_handshake_traffic_secret[32];
    uint8_t server_handshake_traffic_secret[32];
    uint8_t client_application_traffic_secret[32];
    uint8_t server_application_traffic_secret[32];
    uint8_t client_write_key[16];
    uint8_t server_write_key[16];
    void clear();
}

struct TLS13_HSCTX_CLI :
    public TLS13_HSCTX_BASE
{
    typedef TLS13_HSCTX_BASE super;
    SM2_KEY client_ecdhe;
    SM2_KEY server_sign_key;
    void clear();
};

struct TLS13_HSCTX_SVR :
    public TLS13_HSCTX_BASE
{
    typedef TLS13_HSCTX_BASE super;
    SM2_KEY server_ecdhe;
    SM2_KEY client_sign_key;
    void clear();
};

struct TLS13 : public AGMS
{
    TLS13_HSCTX_CLI hsctx_cli;
    TLS13_HSCTX_SVR hsctx_svr;
    int init() override;
    int handshake() override;
    int shutdown() override;
    int recv( PIOVE& iove ) override;
    int send( PIOVE& iove ) override;

    protected:
    int handshake_svr();
    int handshake_cli();
};

}
