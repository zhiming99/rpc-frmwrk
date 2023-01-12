/*
 * =====================================================================================
 *
 *       Filename:  agmsapi.cpp
 *
 *    Description:  implementations of classes for GmSSL Async API
 *
 *        Version:  1.0
 *        Created:  01/11/2023 01:45:54 PM
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

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gmssl/rand.h>
#include <gmssl/x509.h>
#include <gmssl/error.h>
#include <gmssl/sm2.h>
#include <gmssl/sm3.h>
#include <gmssl/sm4.h>
#include <gmssl/pem.h>
#include <gmssl/tls.h>
#include <gmssl/digest.h>
#include <gmssl/gcm.h>
#include <gmssl/hmac.h>
#include <gmssl/hkdf.h>
#include <gmssl/mem.h>

#include "agmsapi.h"

namespace gmssl
{
        
int AGMS_IOVE::alloc( uint32_t size )
{
    if( size == 0 )
        size = 256;

    start = 0;
    mem_size = size;
    content_end = size;
    ptr = malloc( size );
    if( ptr == nullptr )
        return -ENOMEM;
    return 0;
}

int AGMS_IOVE::realloc( uint32_t newsize )
{
    if( newsize == 0 )
        return -EINVAL;

    if( newsize == mem_size )
        return newsize;

    int ret = newsize;
    uint8_t* newbuf = ( uint8_t* )
        ::realloc( ptr, newsize );

    if( newbuf != nullptr )
    {
        ptr = ( uint8_t* )newbuf;
        mem_size = size;
        content_end = newsize;
    }
    else
    {
        // keep the old pointer
        ret = -ENOMEM;
    }
    return ret;
}

int AGMS_IOVE::copy( 
    uint8_t* src, uint32_t src_size );
{
    if( src_size == 0 )
        src_size = mem_size;

    uint32_t newsize =
        std::min( mem_size, src_size );

    if( new_size == 0 )
        return -EINVAL;

    memcpy( ptr, src, newsize );
    return newsize;
}

int AGMS_IOVE::split(
    AGMS_IOVE& bottom_half,
    uint32_t offset )
{
    if( this->content_end - this->start <= offset ||
        return -EINVAL;

    uint32_t newsize1 = offset
    uint32_t newsize2 = this->size() - offset;

    this->content_end = this->start + offset;
    
    bottom_half.clear();
    int ret = bottom_half.alloc( newsize2 );
    if( ret < 0 )
        return ret;
    bottom_half.copy( ptr + offset, newsize2 );
    return 0;
}

int AGMS_IOVE::merge(
    AGMS_IOVE& bottom_half )
{
    if( empty() || bottom_half.empty() )
        return -EINVAL;

    uint32_t newsize =
        this->mem_size + bottom_half->size();
    int ret = this->realloc( newsize );
    if( ret < 0 )
        return ret;

    memcpy( ptr + content_end,
        ( bottom_half->begin() ),
        ( bottom_half->size() );

    this->mem_size = newsize;
    this->content_end += bottom_half->size();
    return 0;
}

int AGMS_IOVE::attach( uint8_t* ptr,
    uint32_t mem_size,
    uint32_t start = 0,
    uint32_t end = 0 )
{
    if( ptr == nullptr || mem_size == 0 )
        return -EINVAL;
    clear();
    return 0;
}

int AGMS_IOVE::detach( uint8_t** pptr,
    uint32_t& nmem_size,
    uint32_t& nstart,
    uint32_t& nend );
{
    *pptr = ptr;
    nmem_size = memsize;
    nstart = start;
    nend = content_end;
    ptr = nullptr;
    clear();
    return 0;
}

// tls record count
int BLKIO::size()
{
    for( auto& elem : io_vec )
    {
    }
}

bool is_valid_hdr( uint8_t* record )
{
    if (!tls_record_type_name(
        tls_record_type(record)))
        return false; 

    if (!tls_protocol_name(
        tls_record_protocol(record)))
        return false;

    if( tls_record_length( record ) >
        TLS_MAX_RECORD_SIZE )
        return false;

    return true;
}

#define INVALID_REC ( ( uint8_t* )-1 )
uint8_t* get_partial_rec( PIOVE& piove )
{
    PIOVE& p = piove;
    if( p->size() < TLS_RECORD_HEADER_SIZE )
        return piove->begin();

    uint32_t total_bytes = 0;
    uint8_t* cur_rec = p->begin();
    uint8_t* tail_ptr = p->end();

    bool partial_rec = false;

    while( cur_rec < tail_ptr )
    {
        if( tail_ptr - cur_rec <
            TLS_RECORD_HEADER_SIZE )
        {
            partial_rec = true;
            break;
        }

        if( !is_valid_hdr( cur_rec ) )
        {
            partial_rec = true;
            cur_rec = INVALID_REC;
            break;
        }

        uint32_t rec_len =
            tls_record_length( cur_rec );

        if( cur_rec + rec_len > tail_ptr )
        {
            partial_rec = true;
            break;
        }

        cur_rec += rec_len;
    }

    if( cur_rec > tail_ptr )
        partial_rec = true;

    if( partial_rec )
        return cur_rec;

    return nullptr;
}

static bool is_partial_rec( PIOVE& piove )
{
    if( TLS_RECORD_HEADER_SIZE >
        piove->size() )
        return true;

    uint32_t rec_len =
        tls_record_length( piove->begin() );

    if( rec_len > piove->size() )
        return true;

    return false;
}

static bool is_single_rec( PIOVE& piove )
{
    if( TLS_RECORD_HEADER_SIZE >
        piove->size() )
        return false;

    uint32_t rec_len =
        tls_record_length( piove->begin() );

    if( rec_len == piove->size() )
        return true;

    return false;
}

int BLKIN::write( PIOVE& piove )
{
    if( ! piove )
        return -EINVAL;

    int ret = 0;
    do{
        bool check_hdr = true;
        uint8_t* last_rec = nullptr;

        if( !io_vec.empty() )
        {
            PIOVE& last = io_vec.back();
            if( is_partial_rec( last ) )
            {
                ret = last->merge( piove );
                if( ret < 0 )
                    break;

                if( is_partial_rec( last ) ||
                    is_single_rec( last ) )
                    break;

                check_hdr = false;
            }
        }

        if( check_hdr )
        {
            uint8_t* record =
                ( uint8_t* )piove->begin();

            if( TLS_RECORD_HEADER_SIZE >
                piove->size() )
            {
                io_vec.push_back( piove );
                break;
            }
            if( !is_valid_hdr( record ) )
            {
                ret = -EPROTO;
                break;
            }

            if( tls_record_length( record ) >=
                piove->size() )
            {
                io_vec.push_back( piove );
                break;
            }

            io_vec.push_back( piove );
        }

        // tail has more than one record
        PIOVE& last = io_vec.back();
        uint8_t* partial_rec =
            get_partial_rec( last );

        // all the records are complete
        if( partial_rec == nullptr )
            break;

        if( partial_rec == INVALID_REC )
        {
            ret = -EPROTO;
            break;
        }

        PIOVE ppartial( new AGMS_IOVE );
        uint32_t offset =
            partial_rec - last->begin();

        ret = ppartial->alloc(
            last->content_end - offset );
        if( ret < 0 )
            break;

        ret = last->split( ppartial, offset );
        if( ret < 0 )
            break;

        io_vec.push_back( ppartial );

    }while( 0 );

    return ret;
}

int BLKIN::read( PIOVE& piove )
{
    if( io_vec.size() == 1 )
    {
        PIOVE& last = io_vec.back();
        if( is_partial_rec( last ) )
            return -ENOENT;
    }
    piove = io_vec.front();
    io_vec.pop_front();
    return 0;
}

void BLKIN::put_back( PIOVE& iover )
{
    io_vec.push_front( iover );
}

int BLKOUT::write( PIOVE& iovew )
{
    io_vec.push_back( iovew );
    return 0;
}

int BLKOUT::read( PIOVE& iover )
{
    iover = io_vec.front();
    return 0;
}

void TLS13_HSCTX_BASE::clear()
{
    gmssl_secure_clear(psk, sizeof(psk));
    gmssl_secure_clear(early_secret, sizeof(early_secret));
    gmssl_secure_clear(handshake_secret, sizeof(handshake_secret));
    gmssl_secure_clear(master_secret, sizeof(master_secret));
    gmssl_secure_clear(client_handshake_traffic_secret, sizeof(client_handshake_traffic_secret));
    gmssl_secure_clear(server_handshake_traffic_secret, sizeof(server_handshake_traffic_secret));
    gmssl_secure_clear(client_application_traffic_secret, sizeof(client_application_traffic_secret));
    gmssl_secure_clear(server_application_traffic_secret, sizeof(server_application_traffic_secret));
    gmssl_secure_clear(client_write_key, sizeof(client_write_key));
    gmssl_secure_clear(server_write_key, sizeof(server_write_key));
}

void TLS13_HSCTX_CLI::clear()
{
    super::clear();
    gmssl_secure_clear(&client_ecdhe, sizeof(client_ecdhe));
    gmssl_secure_clear(&server_sign_key, sizeof(server_sign_key));
}

void TLS13_HSCTX_SVR::clear()
{
    super::clear();
    gmssl_secure_clear(&server_ecdhe, sizeof(server_ecdhe));
    gmssl_secure_clear(&client_sign_key, sizeof(client_sign_key));
}


int TLS13::send( PIOVE& piove )
{
    const BLOCK_CIPHER_KEY *key;
    const uint8_t *iv;
    uint8_t *seq_num;
    uint8_t *cur_blk = piove->begin();
    size_t recordlen;
    size_t padding_len = 0; //FIXME: 在this中设置是否加随机填充，及设置该值
    int ret = 0;

    if( !is_init_finished() )
        return -EBUSY;

    tls_trace("send {ApplicationData}\n");
    if( !piove ) 
        return -EINVAL;

    if( piove->size() > 2048 * 1024 )
        return -E2BIG;
    
    if (this->is_client) {
        key = &this->client_write_key;
        iv = this->client_write_iv;
        seq_num = this->client_seq_num;
    } else {
        key = &this->server_write_key;
        iv = this->server_write_iv;
        seq_num = this->server_seq_num;
    }

    uint8_t* tail_ptr = piove->end();
    size_t datalen = 0;
    while( cur_blk < tail_ptr )
    {
        datalen = std::min(
            ( size_t )tail_ptr - cur_blk,
            ( size_t )TLS_MAX_CIPHERTEXT_SIZE );
            
        PIOVE prec( new AGMS_IOVE );
        size_t mem_size = datalen + 1 +
            padding_len + GHASH_SIZE  +
            TLS_RECORD_HEADER_SIZE;

        ret = prec->alloc( mem_size );
        if( ret < 0 )
            break;

        cur_rec = prec->begin();
        if (tls13_gcm_encrypt(key, iv,
            seq_num, TLS_record_application_data,
            cur_blk, datalen, padding_len,
            cur_rec + TLS_RECORD_HEADER_SIZE,
            &recordlen) != 1)
        {
            error_print();
            ret = -1;
            break;
        }

        cur_rec[0] = TLS_record_application_data;
        cur_rec[1] = 0x03;
        cur_rec[2] = 0x03;
        cur_rec[3] = (uint8_t)(recordlen >> 8);
        cur_rec[4] = (uint8_t)(recordlen);
        recordlen += TLS_RECORD_HEADER_SIZE;

        memcpy( prec->begin(), cur_rec, recordlen );
        this->write_bio.write( prec );

        tls_record_trace(stderr, cur_rec,
            tls_record_length(cur_rec), 0, 0);

        tls_seq_num_incr(seq_num);
        cur_blk += datalen;

    }while( true );

    return ret;
}

int TLS13::handshake()
{
    if( this->is_client )
        return handshake_cli();
    return handshake_svr();
}

int TLS13::handshake_cli()
{
	int ret = 0;
	uint8_t *record = this->record;
	size_t recordlen;

	uint8_t *enced_record = this->enced_record;
	size_t enced_recordlen;
	const DIGEST *digest = DIGEST_sm3();
	size_t padding_len;


	// int protocols[] = { TLS_protocol_tls13 };
	// int supported_groups[] = { TLS_curve_sm2p256v1 };
	// int sign_algors[] = { TLS_sig_sm2sig_sm3 };

	// uint8_t psk[32] = {0};
	// uint8_t early_secret[32];
	// uint8_t handshake_secret[32];
	// uint8_t master_secret[32];
	// uint8_t client_handshake_traffic_secret[32];
	// uint8_t server_handshake_traffic_secret[32];
	// uint8_t client_application_traffic_secret[32];
	// uint8_t server_application_traffic_secret[32];
	// uint8_t client_write_key[16];
	// uint8_t server_write_key[16];

    switch( get_state() )
    {
    case STAT_START_HANDSHAKE:
        {
            this->is_client = 1;
            tls_record_set_protocol(enced_record, TLS_protocol_tls12);

            digest_init(&hctxc.dgst_ctx, digest);
            hctxc.null_dgst_ctx = hctxc.dgst_ctx;


            // send ClientHello

            uint8_t client_exts[TLS_MAX_EXTENSIONS_SIZE];
            size_t client_exts_len;
            uint8_t client_random[32];

            tls_trace("send ClientHello\n");
            tls_record_set_protocol(record, TLS_protocol_tls1);
            rand_bytes(client_random, 32); // TLS 1.3 Random 不再包含 UNIX Time
            sm2_key_generate(&hctxc.client_ecdhe);

            tls13_client_hello_exts_set(client_exts,
                &client_exts_len, sizeof(client_exts),
                &(hctxc.client_ecdhe.public_key));

            tls_record_set_handshake_client_hello(record, &recordlen,
                TLS_protocol_tls12, client_random, NULL, 0,
                tls13_ciphers, sizeof(tls13_ciphers)/sizeof(tls13_ciphers[0]),
                client_exts, client_exts_len);

            tls13_record_trace(stderr, record, recordlen, 0, 0);

            PIOVE prec( new AGMS_IOVE );
            prec->alloc( recordlen );
            ret = prec->copy(
                ( uint8_t* )record, recordlen );
            if( ret < 0 )
                break;

            this->write_bio.write( prec );
            set_state( STAT_WAIT_SVR_HELLO );
            break;
            // 此时尚未确定digest算法，因此无法digest_update
        }
    case STAT_WAIT_SVR_HELLO:
        {
            // recv ServerHello
            tls_trace("recv ServerHello\n");
            PIOVE prec;
            ret = this->read_bio.read( prec );
            if( ret < 0 )
            {
                ret = 0;
                break;
            }

            enced_record = prec->begin();
            enced_recordlen =
                tls_record_length( enced_record );

            const uint8_t *server_exts;
            size_t server_exts_len;
            int protocol;
            int cipher_suite;
            const uint8_t *random;
            const uint8_t *session_id;
            size_t session_id_len;
            uint8_t server_random[32];

            tls13_record_trace(stderr, enced_record, enced_recordlen, 0, 0);
            if (tls_record_get_handshake_server_hello(enced_record,
                &protocol, &random, &session_id, &session_id_len,
                &cipher_suite, &server_exts, &server_exts_len) != 1) {
                error_print();
                // tls_send_alert(this, TLS_alert_unexpected_message);
                ret = -1;
                break;
            }
            if (protocol != TLS_protocol_tls12)
            {
                error_print();
                // tls_send_alert(this, TLS_alert_protocol_version);
                ret = -1;
                break;
            }

            memcpy(server_random, random, 32);
            memcpy(this->session_id, session_id, session_id_len);
            this->session_id_len = session_id_len;
            if (tls_cipher_suite_in_list(cipher_suite, tls13_ciphers,
                sizeof(tls13_ciphers)/sizeof(tls13_ciphers[0])) != 1)
            {
                error_print();
                // tls_send_alert(this, TLS_alert_handshake_failure);
                ret = -1;
                break;
            }

            this->cipher_suite = cipher_suite;
            SM2_POINT server_ecdhe_public;
            if (tls13_server_hello_extensions_get(
                server_exts, server_exts_len,
                &server_ecdhe_public) != 1) {
                error_print();
                // tls_send_alert(this, TLS_alert_handshake_failure);
                ret = -1;
                break;
            }
            this->protocol = TLS_protocol_tls13;

            tls13_cipher_suite_get(this->cipher_suite, &digest, &hctxc.cipher);
            digest_update(&hctxc.dgst_ctx, record + 5, recordlen - 5);
            digest_update(&hctxc.dgst_ctx, enced_record + 5, enced_recordlen - 5);


            printf("generate handshake secrets\n");
            /*
            generate handshake keys
                uint8_t client_write_key[32]
                uint8_t server_write_key[32]
                uint8_t client_write_iv[12]
                uint8_t server_write_iv[12]
            */

            uint8_t zeros[32] = {0};
            sm2_ecdh(&hctxc.client_ecdhe, &server_ecdhe_public, &server_ecdhe_public);
            /* [1]  */ tls13_hkdf_extract(digest, zeros, hctxc.psk, hctxc.early_secret);
            /* [5]  */ tls13_derive_secret(hctxc.early_secret, "derived", &hctxc.null_dgst_ctx, hctxc.handshake_secret);
            /* [6]  */ tls13_hkdf_extract(digest, hctxc.handshake_secret, (uint8_t *)&server_ecdhe_public, hctxc.handshake_secret);
            /* [7]  */ tls13_derive_secret(hctxc.handshake_secret, "c hs traffic", &hctxc.dgst_ctx, hctxc.client_handshake_traffic_secret);
            /* [8]  */ tls13_derive_secret(hctxc.handshake_secret, "s hs traffic", &hctxc.dgst_ctx, hctxc.server_handshake_traffic_secret);
            /* [9]  */ tls13_derive_secret(hctxc.handshake_secret, "derived", &hctxc.null_dgst_ctx, hctxc.master_secret);
            /* [10] */ tls13_hkdf_extract(digest, hctxc.master_secret, zeros, hctxc.master_secret);
            //[sender]_write_key = HKDF-Expand-Label(Secret, "key", "", key_length)
            //[sender]_write_iv  = HKDF-Expand-Label(Secret, "iv", "", iv_length)
            //[sender] in {server, client}
            tls13_hkdf_expand_label(digest,
            hctxc.server_handshake_traffic_secret, "key", NULL, 0, 16, hctxc.server_write_key);
            tls13_hkdf_expand_label(digest,
            hctxc.server_handshake_traffic_secret, "iv", NULL, 0, 12,
                this->server_write_iv);
            block_cipher_set_encrypt_key(&this->server_write_key, hctxc.cipher, hctxc.server_write_key);
            memset(this->server_seq_num, 0, 8);
            tls13_hkdf_expand_label(digest, hctxc.client_handshake_traffic_secret, "key", NULL, 0, 16, hctxc.client_write_key);
            tls13_hkdf_expand_label(digest,
            hctxc.client_handshake_traffic_secret, "iv", NULL, 0, 12,
            this->client_write_iv);
            block_cipher_set_encrypt_key(&this->client_write_key, hctxc.cipher, hctxc.client_write_key);
            memset(this->client_seq_num, 0, 8);
            /*
            format_bytes(stderr, 0, 4, "client_write_key", client_write_key, 16);
            format_bytes(stderr, 0, 4, "server_write_key", server_write_key, 16);
            format_bytes(stderr, 0, 4, "client_write_iv", this->client_write_iv, 12);
            format_bytes(stderr, 0, 4, "server_write_iv", this->server_write_iv, 12);
            format_print(stderr, 0, 0, "\n");
            */
            set_state( STAT_WAIT_SVR_ENCEXT );
            if( prec->size() == enced_recordlen )
            {
                if( this->read_bio.size() == 0 )
                    break;
            }
            else
            {
                prec->trim_bytes_front( enced_recordlen );
                this->read_bio.put_back( prec );
            }

            // fall through
        }
        case STAT_WAIT_SVR_ENCEXT:
        {
            // recv {EncryptedExtensions}
            printf("recv {EncryptedExtensions}\n");
            PIOVE prec;
            ret = this->read_bio.read( prec );
            if( ret < 0 )
            {
                ret = 0;
                break;
            }

            enced_record = prec->begin();
            enced_recordlen =
                tls_record_length( enced_record );

            if (tls13_record_decrypt(&this->server_write_key,
                this->server_write_iv,
                this->server_seq_num, enced_record, enced_recordlen,
                record, &recordlen) != 1) {
                error_print();
                // tls_send_alert(this, TLS_alert_bad_record_mac);
                ret = -1;
                break;
            }
            tls13_record_trace(stderr, record, recordlen, 0, 0);
            if (tls13_record_get_handshake_encrypted_extensions(record) != 1) {
                // tls_send_alert(this, TLS_alert_handshake_failure);
                error_print();
                ret = -1;
                break;
            }
            digest_update(&hctxc.dgst_ctx, record + 5, recordlen - 5);
            tls_seq_num_incr(this->server_seq_num);

            set_state( STAT_WAIT_SVR_CERT );
            if( prec->size() == enced_recordlen )
            {
                if( this->read_bio.size() == 0 )
                    break;
            }
            else
            {
                prec->trim_bytes_front( enced_recordlen );
                this->read_bio.put_back( prec );
            }
            // fall through
        }
        case STAT_WAIT_SVR_CERT:
        {

        // recv {CertififcateRequest*} or {Certificate}
        PIOVE prec;
        ret = this->read_bio.read( prec );
        if( ret < 0 )
        {
            ret = 0;
            break;
        }

        enced_record = prec->begin();
        enced_recordlen =
            tls_record_length( enced_record );

        if (tls13_record_decrypt(&this->server_write_key,
            this->server_write_iv,
            this->server_seq_num, enced_record, enced_recordlen,
            record, &recordlen) != 1) {
            error_print();
            // tls_send_alert(this, TLS_alert_bad_record_mac);
            ret = -1;
            break;
        }

        int type;
        const uint8_t *data;
        size_t datalen;

        if (tls_record_get_handshake(record, &type, &data, &datalen) != 1) {
            error_print();
            // tls_send_alert(this, TLS_alert_handshake_failure);
            ret = -1;
            break;
        }

        if (type == TLS_handshake_certificate_request) {
            const uint8_t *request_context;
            size_t request_context_len;
            const uint8_t *cert_request_exts;
            size_t cert_request_extslen;
            tls_trace("recv {CertificateRequest*}\n");
            tls13_record_trace(stderr, record, recordlen, 0, 0);
            if (tls13_record_get_handshake_certificate_request(record,
                &request_context, &request_context_len,
                &cert_request_exts, &cert_request_extslen) != 1) {
                error_print();
                tls_send_alert(this, TLS_alert_handshake_failure);
                return -1;
            }
            // 当前忽略 request_context 和 cert_request_exts
            // request_context 应该为空，当前实现中不支持Post-Handshake Auth
            digest_update(&hctxc.dgst_ctx, record + 5, recordlen - 5);
            tls_seq_num_incr(this->server_seq_num);


            // recv {Certificate}
            if (tls_record_recv(enced_record,
                &enced_recordlen, this->sock) != 1) {
                error_print();
                // tls_send_alert(this, TLS_alert_handshake_failure);
                ret = -1;
                break;
            }
            if (tls13_record_decrypt(&this->server_write_key,
                    this->server_write_iv,
                    this->server_seq_num,
                    enced_record, enced_recordlen,
                    record, &recordlen) != 1)
            {
                error_print();
                // tls_send_alert(this, TLS_alert_bad_record_mac);
                ret = -1;
                break;
            }
        }
        else
        {
            this->client_certs_len = 0;
            // 清空客户端签名密钥
        }

        // recv {Certificate}
        const uint8_t *request_context;
        size_t request_context_len;

        const uint8_t *cert_request_exts;
        size_t cert_request_extslen;

        const uint8_t *cert_list;
        size_t cert_list_len;
        const uint8_t *cert;
        size_t certlen;

        tls_trace("recv {Certificate}\n");
        tls13_record_trace(stderr, record, recordlen, 0, 0);
        if (tls13_record_get_handshake_certificate(record,
            &request_context, &request_context_len,
            &cert_list, &cert_list_len) != 1) {
            error_print();
            tls_send_alert(this, TLS_alert_unexpected_message);
            return -1;
        }
        if (tls13_process_certificate_list(cert_list,
            cert_list_len, this->server_certs, &this->server_certs_len) != 1) {
            error_print();
            tls_send_alert(this, TLS_alert_unexpected_message);
            return -1;
        }
        if
            (x509_certs_get_cert_by_index(this->server_certs,
            this->server_certs_len, 0, &cert, &certlen) != 1
            || x509_cert_get_subject_public_key(cert, certlen, &hctxc.server_sign_key) != 1) {
            error_print();
            tls_send_alert(this, TLS_alert_unexpected_message);
            return -1;
        }
        digest_update(&hctxc.dgst_ctx, record + 5, recordlen - 5);
        tls_seq_num_incr(this->server_seq_num);


        // recv {CertificateVerify}
        tls_trace("recv {CertificateVerify}\n");
        if (tls_record_recv(enced_record, &enced_recordlen,
            this->sock) != 1) {
            error_print();
            tls_send_alert(this, TLS_alert_unexpected_message);
            return -1;
        }
        if (tls13_record_decrypt(&this->server_write_key,
            this->server_write_iv,
            this->server_seq_num, enced_record, enced_recordlen,
            record, &recordlen) != 1) {
            error_print();
            tls_send_alert(this, TLS_alert_bad_record_mac);
            return -1;
        }
        tls13_record_trace(stderr, record, recordlen, 0, 0);
        const uint8_t *server_sig;
        int server_sign_algor;
        size_t server_siglen;
        if (tls13_record_get_handshake_certificate_verify(record,
            &server_sign_algor, &server_sig, &server_siglen) != 1) {
            error_print();
            tls_send_alert(this, TLS_alert_unexpected_message);
            return -1;
        }
        if (server_sign_algor != TLS_sig_sm2sig_sm3) {
            error_print();
            tls_send_alert(this, TLS_alert_unexpected_message);
            return -1;
        }
        if (tls13_verify_certificate_verify(TLS_server_mode, &hctxc.server_sign_key, TLS13_SM2_ID, TLS13_SM2_ID_LENGTH, &hctxc.dgst_ctx, server_sig, server_siglen) != 1) {
            error_print();
            return -1;
        }
        digest_update(&hctxc.dgst_ctx, record + 5, recordlen - 5);
        tls_seq_num_incr(this->server_seq_num);


        // use Transcript-Hash(Handshake Context, Certificate*, CertificateVerify*)
        tls13_compute_verify_data(hctxc.server_handshake_traffic_secret,
            &hctxc.dgst_ctx, hctxc.verify_data, &hctxc.verify_data_len);


        // recv {Finished}
        tls_trace("recv {Finished}\n");
        if (tls_record_recv(enced_record, &enced_recordlen,
            this->sock) != 1) {
            error_print();
            tls_send_alert(this, TLS_alert_unexpected_message);
            return -1;
        }
        if (tls13_record_decrypt(&this->server_write_key,
            this->server_write_iv,
            this->server_seq_num, enced_record, enced_recordlen,
            record, &recordlen) != 1) {
            error_print();
            tls_send_alert(this, TLS_alert_bad_record_mac);
            return -1;
        }

        const uint8_t *server_verify_data;
        size_t server_verify_data_len;

        tls13_record_trace(stderr, record, recordlen, 0, 0);
        if (tls13_record_get_handshake_finished(record,
            &server_verify_data, &server_verify_data_len) != 1) {
            error_print();
            tls_send_alert(this, TLS_alert_unexpected_message);
            return -1;
        }
        if (server_verify_data_len != hctxc.verify_data_len
            || memcmp(server_verify_data, hctxc.verify_data, hctxc.verify_data_len) != 0) {
            error_print();
            tls_send_alert(this, TLS_alert_unexpected_message);
            return -1;
        }
        digest_update(&hctxc.dgst_ctx, record + 5, recordlen - 5);
        tls_seq_num_incr(this->server_seq_num);


        // generate server_application_traffic_secret
        /* [12] */ tls13_derive_secret(hctxc.master_secret, "s ap traffic", &hctxc.dgst_ctx, hctxc.server_application_traffic_secret);
        // generate client_application_traffic_secret
        /* [11] */ tls13_derive_secret(hctxc.master_secret, "c ap traffic", &hctxc.dgst_ctx, hctxc.client_application_traffic_secret);


        if (this->client_certs_len) {
            int client_sign_algor;
            uint8_t sig[TLS_MAX_SIGNATURE_SIZE];
            size_t siglen;

            // send client {Certificate*}
            tls_trace("send {Certificate*}\n");
            if (tls13_record_set_handshake_certificate(record, &recordlen,
                NULL, 0, // certificate_request_context
                this->client_certs, this->client_certs_len) != 1) {
                error_print();
                tls_send_alert(this, TLS_alert_internal_error);
                goto end;
            }
            tls13_record_trace(stderr, record, recordlen, 0, 0);
            tls13_padding_len_rand(&padding_len);
            if (tls13_record_encrypt(&this->client_write_key,
                this->client_write_iv,
                this->client_seq_num, record, recordlen, padding_len,
                enced_record, &enced_recordlen) != 1) {
                error_print();
                tls_send_alert(this, TLS_alert_internal_error);
                return -1;
            }
            if (tls_record_send(enced_record,
                enced_recordlen, this->sock) != 1) {
                error_print();
                tls_send_alert(this, TLS_alert_internal_error);
                return -1;
            }
            digest_update(&hctxc.dgst_ctx, record + 5, recordlen - 5);
            tls_seq_num_incr(this->client_seq_num);


            // send {CertificateVerify*}
            tls_trace("send {CertificateVerify*}\n");
            client_sign_algor = TLS_sig_sm2sig_sm3; // FIXME: 应该放在this里面
            tls13_sign_certificate_verify(TLS_client_mode, &this->sign_key, TLS13_SM2_ID, TLS13_SM2_ID_LENGTH, &hctxc.dgst_ctx, sig, &siglen);
            if (tls13_record_set_handshake_certificate_verify(record, &recordlen,
                client_sign_algor, sig, siglen) != 1) {
                error_print();
                tls_send_alert(this, TLS_alert_internal_error);
                return -1;
            }
            tls13_record_trace(stderr, record, recordlen, 0, 0);
            tls13_padding_len_rand(&padding_len);
            if (tls13_record_encrypt(&this->client_write_key,
                this->client_write_iv,
                this->client_seq_num, record, recordlen, padding_len,
                enced_record, &enced_recordlen) != 1) {
                error_print();
                tls_send_alert(this, TLS_alert_internal_error);
                return -1;
            }
            if (tls_record_send(enced_record,
                enced_recordlen, this->sock) != 1) {
                error_print();
                return -1;
            }
            digest_update(&hctxc.dgst_ctx, record + 5, recordlen - 5);
            tls_seq_num_incr(this->client_seq_num);
        }

        // send Client {Finished}
        tls_trace("send {Finished}\n");
        tls13_compute_verify_data(hctxc.client_handshake_traffic_secret, &hctxc.dgst_ctx, hctxc.verify_data, &hctxc.verify_data_len);
        if (tls_record_set_handshake_finished(record, &recordlen, hctxc.verify_data, hctxc.verify_data_len) != 1) {
            error_print();
            tls_send_alert(this, TLS_alert_internal_error);
            goto end;
        }
        tls13_record_trace(stderr, record, recordlen, 0, 0);
        tls13_padding_len_rand(&padding_len);
        if (tls13_record_encrypt(&this->client_write_key,
            this->client_write_iv,
            this->client_seq_num, record, recordlen, padding_len,
            enced_record, &enced_recordlen) != 1) {
            error_print();
            tls_send_alert(this, TLS_alert_internal_error);
            goto end;
        }
        if (tls_record_send(enced_record, enced_recordlen,
            this->sock) != 1) {
            error_print();
            goto end;
        }
        digest_update(&hctxc.dgst_ctx, record + 5, recordlen - 5);
        tls_seq_num_incr(this->client_seq_num);



        // update server_write_key, server_write_iv, reset server_seq_num
        tls13_hkdf_expand_label(digest, hctxc.server_application_traffic_secret, "key", NULL, 0, 16, hctxc.server_write_key);
        block_cipher_set_encrypt_key(&this->server_write_key, hctxc.cipher, hctxc.server_write_key);
        tls13_hkdf_expand_label(digest,
        hctxc.server_application_traffic_secret, "iv", NULL, 0,
        12, this->server_write_iv);
        memset(this->server_seq_num, 0, 8);
        /*
        format_print(stderr, 0, 0, "update server secrets\n");
        format_bytes(stderr, 0, 4, "server_write_key", server_write_key, 16);
        format_bytes(stderr, 0, 4, "server_write_iv", this->server_write_iv, 12);
        format_print(stderr, 0, 0, "\n");
        */

        //update client_write_key, client_write_iv, reset client_seq_num
        tls13_hkdf_expand_label(digest, hctxc.client_application_traffic_secret, "key", NULL, 0, 16, hctxc.client_write_key);
        tls13_hkdf_expand_label(digest,
        hctxc.client_application_traffic_secret, "iv", NULL, 0,
        12, this->client_write_iv);
        block_cipher_set_encrypt_key(&this->client_write_key, hctxc.cipher, hctxc.client_write_key);
        memset(this->client_seq_num, 0, 8);

        /*
        format_print(stderr, 0, 0, "update client secrets\n");
        format_bytes(stderr, 0, 4, "client_write_key", client_write_key, 16);
        format_bytes(stderr, 0, 4, "client_write_iv", this->client_write_iv, 12);
        format_print(stderr, 0, 0, "\n");
        */
        fprintf(stderr, "Connection established\n");
        ret = 1;
    }

    if( ret < 0 ||
        get_state() == STAT_READY )
        hctxc.clear();

	return ret;
}



}
