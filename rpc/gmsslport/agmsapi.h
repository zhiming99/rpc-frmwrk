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
 *   Adapted from GmSSL's source code. The License and disclaimer as follows
 *
 *   Copyright 2014-2023 The GmSSL Project. All Rights Reserved.
 *
 *   Licensed under the Apache License, Version 2.0 (the License); you may
 *   not use this file except in compliance with the License.
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * =====================================================================================
 */

#pragma once
#include <stdint.h>
#include <list>

#define RET_OK 0
#define RET_PENDING 65537

#define IOVE_NOFREE  1

namespace gmssl
{

struct AGMS_IOVE
{
    uint32_t flags = 0;
    uint8_t* ptr = nullptr;
    size_t mem_size = 0;
    size_t start = 0;
    size_t content_end = 0;

    ~AGMS_IOVE()
    { clear(); }

    inline bool no_free()
    { return ( flags & IOVE_NOFREE ) > 0; }

    inline void set_no_free(
        bool no_free = true )
    {
        if( no_free )
            flags |= IOVE_NOFREE;
        else
            flags &= ~IOVE_NOFREE;
    }

    inline void clear()
    {
        if( ptr && !no_free() )
        { free( ptr ); }
        ptr = nullptr;
        mem_size = start = content_end = 0;
        flags = 0;
    }

    inline bool empty()
    {
        return ( ptr == nullptr ||
            ( content_end - start == 0 ) );
    }

    int alloc( size_t size = 0 );
    int realloc( size_t newsize  );
    inline void clear_content()
    { start = 0; content_end = mem_size;}

    int attach( uint8_t* ptr,
        size_t size,
        size_t start = 0,
        size_t end = 0 );

    int detach( uint8_t** pptr,
        size_t& size,
        size_t& start,
        size_t& end );

    void expose( uint8_t** pptr,
        size_t& size,
        size_t& start,
        size_t& end );

    inline size_t size() const
    { return content_end - start; }

    int split(
        AGMS_IOVE& bottom_half
        size_t offset );

    int copy( uint8_t* src,
        size_t src_size = 0 );

    int merge(
        AGMS_IOVE& bottom_half );

    inline uint8_t* begin()
    { return ptr + start; }

    inline uint8_t* end()
    { return ptr + content_end; }

    inline int trim_bytes_front( size_t bytes )
    {
        if( begin() + bytes > end() )
            return -ERANGE;
        start += bytes;
        return 0;
    }

    inline int trim_bytes_end( size_t bytes )
    {
        if( end() - bytes < begin() )
            return -ERANGE;
        content_end -= bytes;
        return 0;
    }

    // tls record related methods
    bool is_single_rec();
    bool is_partial_rec();
    uint8_t* get_partial_rec();
};

using PIOVE=std::shared_ptr< AGMS_IOVE > ;
using IOV = std::list< PIOVE >;

struct BLKIO_BASE
{
    IOV io_vec;
    virtual int read ( PIOVE& iover ) = 0;
    virtual int write( PIOVE& iovew ) = 0;
    virtual size_t size() const = 0;
    void put_back( PIOVE& iover );

    protected:
    bool is_partial_record();
    size_t record_size(); 
};

// incoming message queue
struct BLKIN : public BLKIO_BASE
{
    int write( PIOVE& iovew ) override;
    int read( PIOVE& iover ) override;
    size_t size() const override;
    bool has_partial_rec() const;
};

struct BLKOUT : public BLKIO_BASE
{
    int write( PIOVE& iovew ) override;
    int read( PIOVE& iover ) override;
    size_t size() const override;
};

typedef enum : uint32_t
{
    STAT_HANDSHAKE_FAILED = -2,
    STAT_CLOSED = -1,
    STAT_INIT,
    STAT_START_HANDSHAKE,
    STAT_READY,
    STAT_WAIT_SVR_HELLO,
    STAT_WAIT_SVR_ENCEXT,
    STAT_WAIT_SVR_CERT,
    STAT_WAIT_SVR_CERTVERIFY,
    STAT_WAIT_SVR_FIN,
    STAT_WAIT_CLI_HELLO,
    STAT_WAIT_CLI_CERT,
    STAT_WAIT_CLI_CERTVERIFY,
    STAT_WAIT_CLI_FIN,
    STAT_START_SHUTDOWN,
    STAT_WAIT_CLOSE_NOTIFY,
    STAT_CLOSE_NOTIFY_RECEIVED,

} AGMS_STATE;

struct AGMS_CTX : TLS_CTX
{
    AGMS_CTX();
    int check_private_key();
    unsigned long set_options( unsigned long op);
    int set_cipher_list( const char *str);
    void cleanup();
};

struct AGMS : public TLS_CONNECT
{
    std::atomic< AGMS_STATE > gms_state;
    std::unique_ptr< AGMS_CTX > gms_ctx;
    BLKIN read_bio;
    BLKOUT write_bio;

    std::string certfile;
    std::string keyfile;
    std::string cacertfile;
    std::string password;

    AGMS();
    virtual ~AGMS_CTX()
    { cleanup(); }

    virtual int init( bool is_client ) = 0; 
    virtual int handshake() = 0;

    virtual int recv( PIOVE& iove ) = 0;
    virtual int send( PIOVE& iove ) = 0;
    int shutdown();

    int send_alert( int alert );
    int get_error(int ret_code) const;

    void set_state( AGMS_STATE state )
    { gms_stat = state; }

    AGMS_STATE get_state() const
    { return gms_state; }

    void library_init();
    bool is_ready() const
    { return get_state() == STAT_READY; }

    int use_PrivateKey_file(const char *file, int type);
    int use_certificate_file(const char *file, int type);
    void add_all_algorithms();
    void load_error_strings();
    void set_connect_state();
    void set_accept_state();
    bool is_client() const;

    int pending_bytes();
    void cleanup();
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

	uint8_t verify_data[32];
	size_t verify_data_len;

	DIGEST_CTX dgst_ctx; // secret generation过程中需要ClientHello等数据输入的
	DIGEST_CTX null_dgst_ctx; // secret generation过程中不需要握手数据的
	const BLOCK_CIPHER *cipher = NULL;

    void clear();
}

struct TLS13_HSCTX_CLI :
    public TLS13_HSCTX_BASE
{
    typedef TLS13_HSCTX_BASE super;
    SM2_KEY client_ecdhe;
    SM2_KEY server_sign_key;
    bool cert_req_received = false;

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
    TLS13_HSCTX_CLI hctxc;
    TLS13_HSCTX_SVR hctxs;

    int init( bool is_client ) override;
    int handshake() override;
    int shutdown() override;

    // receiving data in cleartext
    int recv( PIOVE& iove ) override;

    // sending data in cleartext
    int send( PIOVE& iove ) override;

    protected:
    int handle_alert( uint8_t* record );

    int handshake_svr();
    int handshake_cli();

    int init_cli();
    int init_svr();

    int init_connect();
};

}
