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

#include "rpc.h"
#include "agmsapi.h"

#define RECV_RECORD( prec, rec_ptr, rec_len ) \
PIOVE prec; \
{ \
    ret = this->read_bio.read( prec ); \
    if( ret == -ENOENT ) \
    { \
        ret = RET_PENDING; \
        break; \
    } \
    else if( ERROR( ret ) ) \
        break; \
    rec_ptr = prec->begin(); \
    rec_len = \
        tls_record_length( rec_ptr ); \
}

#define SEND_RECORD( rec_ptr, rec_len ) \
{ \
    PIOVE prec_send( new AGMS_IOVE ); \
    ret = prec_send->alloc( rec_len );\
    if( ret < 0 ) \
        break; \
    prec_send->copy( rec_ptr, rec_len );\
    ret = this->write_bio.write( prec_send ); \
    if( ret < 0 ) \
        break; \
} \

#define SWITCH_STATE( new_state, rec_len ) \
{ \
    set_state( new_state );\
    if( prec->size() == enced_recordlen )\
    {\
        if( this->read_bio.size() == 0 )\
        {\
            ret = RET_PENDING;\
            break;\
        }\
    }\
    else\
    {\
        prec->trim_bytes_front( enced_recordlen );\
        this->read_bio.put_back( prec );\
    }\
}

#define SEND_ALERT( _alert ) \
{ \
    error_print(); \
    this->send_alert(_alert); \
    ret = -1000 - _alert; \
}

extern "C"
{
int tls13_client_hello_exts_set(uint8_t *exts, size_t *extslen, size_t maxlen,
    const SM2_POINT *client_ecdhe_public);

int tls13_record_decrypt(const BLOCK_CIPHER_KEY *key, const uint8_t iv[12],
    const uint8_t seq_num[8], const uint8_t *enced_record, size_t enced_recordlen,
    uint8_t *record, size_t *recordlen);

int tls13_server_hello_extensions_get(const uint8_t *exts, size_t extslen,
    SM2_POINT *sm2_point);

int tls13_cipher_suite_get(int cipher_suite, const DIGEST **digest, const
    BLOCK_CIPHER **cipher);

int tls13_record_get_handshake_encrypted_extensions(const uint8_t *record);

int tls13_record_get_handshake_certificate_request(const uint8_t *record,
    const uint8_t **requst_context, size_t *request_context_len,
    const uint8_t **exts, size_t *exts_len);

int tls13_record_get_handshake_certificate(const uint8_t *record,
    const uint8_t **cert_request_context, size_t *cert_request_context_len,
    const uint8_t **cert_list, size_t *cert_list_len);

int tls13_process_certificate_list(const uint8_t *cert_list, size_t cert_list_len,
    uint8_t *certs, size_t *certs_len);

int tls13_record_get_handshake_certificate_verify(const uint8_t *record,
    int *sign_algor, const uint8_t **sig, size_t *siglen);

int tls13_verify_certificate_verify(int tls_mode,
    const SM2_KEY *public_key, const char *signer_id, size_t signer_id_len,
    const DIGEST_CTX *tbs_dgst_ctx, const uint8_t *sig, size_t siglen);

int tls13_compute_verify_data(const uint8_t *handshake_traffic_secret,
    const DIGEST_CTX *dgst_ctx, uint8_t *verify_data, size_t *verify_data_len);

int tls13_record_get_handshake_finished(const uint8_t *record,
    const uint8_t **verify_data, size_t *verify_data_len);

int tls13_record_set_handshake_certificate_verify(uint8_t *record, size_t *recordlen,
    int sign_algor, const uint8_t *sig, size_t siglen);

int tls13_padding_len_rand(size_t *padding_len);

int tls13_record_encrypt(const BLOCK_CIPHER_KEY *key, const uint8_t iv[12],
    const uint8_t seq_num[8], const uint8_t *record, size_t recordlen, size_t padding_len,
    uint8_t *enced_record, size_t *enced_recordlen);

int tls13_record_set_handshake_certificate_verify(uint8_t *record, size_t *recordlen,
    int sign_algor, const uint8_t *sig, size_t siglen);

int tls13_sign_certificate_verify(int tls_mode,
    const SM2_KEY *key, const char *signer_id, size_t signer_id_len,
    const DIGEST_CTX *tbs_dgst_ctx,
    uint8_t *sig, size_t *siglen);

int tls13_record_set_handshake_certificate_verify(uint8_t *record, size_t *recordlen,
    int sign_algor, const uint8_t *sig, size_t siglen);

int tls13_record_set_handshake_certificate(uint8_t *record, size_t *recordlen,
    const uint8_t *request_context, size_t request_context_len,
    const uint8_t *certs, size_t certslen);

int tls13_process_client_hello_exts(const uint8_t *exts, size_t extslen,
    const SM2_KEY *server_ecdhe_key, SM2_POINT *client_ecdhe_public,
    uint8_t *server_exts, size_t *server_exts_len, size_t server_exts_maxlen);


int tls13_record_set_handshake_encrypted_extensions(
    uint8_t *record, size_t *recordlen);

int tls13_record_set_handshake_certificate_request_default(
    uint8_t *record, size_t *recordlen);

int tls13_record_set_handshake_finished(uint8_t *record, size_t *recordlen,
    const uint8_t *verify_data, size_t verify_data_len);
}

namespace gmssl
{
        
static const int tls13_ciphers[] = { TLS_cipher_sm4_gcm_sm3 };

int AGMS_IOVE::alloc( size_t new_size )
{
    if( new_size == 0 )
        new_size = 256;

    start = 0;
    mem_size = new_size;
    content_end = new_size;
    pdata = ( uint8_t* )malloc( new_size );
    if( pdata == nullptr )
        return -ENOMEM;
    return 0;
}

int AGMS_IOVE::realloc( size_t newsize )
{
    if( newsize == 0 )
        return -EINVAL;

    if( newsize == mem_size )
        return newsize;

    int ret = newsize;
    if( pdata == nullptr )
        return alloc( newsize );

    uint8_t* newbuf = ( uint8_t* )
        ::realloc( pdata, newsize );

    if( newbuf != nullptr )
    {
        pdata = ( uint8_t* )newbuf;
        mem_size = newsize;
        if( newsize < content_end )
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
    uint8_t* src, size_t src_size )
{
    if( src_size == 0 )
        src_size = mem_size;

    size_t newsize =
        std::min( mem_size, src_size );

    if( newsize == 0 )
        return -EINVAL;

    memcpy( pdata, src, newsize );
    return newsize;
}

int AGMS_IOVE::split(
    AGMS_IOVE& bottom_half,
    size_t offset )
{
    if( this->content_end - this->start <= offset )
        return -EINVAL;

    size_t newsize = this->size() - offset;

    this->content_end = this->start + offset;
    
    bottom_half.clear();
    int ret = bottom_half.alloc( newsize );
    if( ret < 0 )
        return ret;
    bottom_half.copy( pdata + offset, newsize );
    return 0;
}

int AGMS_IOVE::merge(
    AGMS_IOVE& bottom_half )
{
    if( empty() || bottom_half.empty() )
        return -EINVAL;

    size_t newsize =
        this->mem_size + bottom_half.size();
    int ret = this->realloc( newsize );
    if( ret < 0 )
        return ret;

    memcpy( pdata + content_end,
        ( bottom_half.begin() ),
        ( bottom_half.size() ) );

    this->mem_size = newsize;
    this->content_end += bottom_half.size();
    return 0;
}

int AGMS_IOVE::attach( uint8_t* nptr,
    size_t nsize,
    size_t nstart,
    size_t nend )
{
    if( nptr == nullptr || nsize == 0 )
        return -EINVAL;
    clear();

    pdata = nptr;
    mem_size = nsize;

    if( nend == 0 )
        content_end = nsize;
    else
        content_end = nend;

    start = nstart;

    return 0;
}

void AGMS_IOVE::expose( uint8_t** pptr,
    size_t& nmem_size,
    size_t& nstart,
    size_t& nend )
{
    *pptr = pdata;
    nmem_size = mem_size;
    nstart = start;
    nend = content_end;
}

int AGMS_IOVE::detach( uint8_t** pptr,
    size_t& nmem_size,
    size_t& nstart,
    size_t& nend )
{
    expose( pptr,
        nmem_size, nstart, nend );
    set_no_free();
    clear();
    return 0;
}

static bool is_valid_hdr( uint8_t* record )
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

uint8_t* AGMS_IOVE::get_partial_rec()
{
    if( this->size() < TLS_RECORD_HEADER_SIZE )
        return this->begin();

    uint8_t* cur_rec = this->begin();
    uint8_t* tail_ptr = this->end();

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

        size_t rec_len =
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

bool AGMS_IOVE::is_partial_rec()
{
    if( TLS_RECORD_HEADER_SIZE >
        this->size() )
        return true;

    size_t rec_len =
        tls_record_length( this->begin() );

    if( rec_len > this->size() )
        return true;

    return false;
}

bool AGMS_IOVE::is_single_rec()
{
    if( TLS_RECORD_HEADER_SIZE >
        this->size() )
        return false;

    size_t rec_len =
        tls_record_length( this->begin() );

    if( rec_len == this->size() )
        return true;

    return false;
}

void BLKIO_BASE::put_back( PIOVE& piover )
{
    io_vec.push_front( piover );
}

int BLKIN::write( PIOVE& piove )
{
    if( ! piove )
        return -EINVAL;

    int ret = 0;
    do{
        bool check_hdr = true;

        if( !io_vec.empty() )
        {
            PIOVE& last = io_vec.back();
            if( last->is_partial_rec() )
            {
                ret = last->merge( *piove );
                if( ret < 0 )
                    break;

                if( last->is_partial_rec() ||
                    last->is_single_rec() )
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
                ( int )piove->size() )
            {
                io_vec.push_back( piove );
                break;
            }

            io_vec.push_back( piove );
        }

        // tail has more than one record
        PIOVE& last = io_vec.back();
        uint8_t* partial_rec =
            last->get_partial_rec();

        // all the records are complete
        if( partial_rec == nullptr )
            break;

        if( partial_rec == INVALID_REC )
        {
            ret = -EPROTO;
            break;
        }

        PIOVE ppartial( new AGMS_IOVE );
        size_t offset =
            partial_rec - last->begin();

        ret = ppartial->alloc(
            last->content_end - offset );
        if( ret < 0 )
            break;

        ret = last->split( *ppartial, offset );
        if( ret < 0 )
            break;

        io_vec.push_back( ppartial );

    }while( 0 );

    return ret;
}

int BLKIN::read( PIOVE& piove )
{
    if( io_vec.empty() )
        return -ENOENT;
    if( io_vec.size() == 1 )
    {
        PIOVE& last = io_vec.back();
        if( last->is_partial_rec() )
            return -ENOENT;
    }
    piove = io_vec.front();
    io_vec.pop_front();
    return 0;
}

size_t BLKIN::size() const
{
    size_t count = io_vec.size();
    if( count > 0 )
    {
        const PIOVE& last = io_vec.back();
        if( last->is_partial_rec() )
            return count - 1;
        return count;
    }
    return 0;
}

bool BLKIN::has_partial_rec()
{
    if( io_vec.empty() )
        return false;
    PIOVE& last = io_vec.back();
    if( last->is_partial_rec() )
        return true;
    return false;
}

int BLKOUT::write( PIOVE& iovew )
{
    io_vec.push_back( iovew );
    return 0;
}

int BLKOUT::read( PIOVE& iover )
{
    if( io_vec.empty() )
        return -ENOENT;
    iover = io_vec.front();
    io_vec.pop_front();
    return 0;
}

size_t BLKOUT::size() const
{ return io_vec.size(); }

void AGMS_CTX::cleanup()  
{
    TLS_CTX* ctx = this;
    gmssl_secure_clear(&ctx->signkey, sizeof(SM2_KEY));
    gmssl_secure_clear(&ctx->kenckey, sizeof(SM2_KEY));
    if (ctx->certs) free(ctx->certs);
    if (ctx->cacerts) free(ctx->cacerts);
    memset(ctx, 0, sizeof(TLS_CTX));
}

void TLS13_HSCTX_BASE::clear()
{
    gmssl_secure_clear(psk, sizeof(psk));

    gmssl_secure_clear(early_secret,
        sizeof(early_secret));

    gmssl_secure_clear(handshake_secret,
        sizeof(handshake_secret));

    gmssl_secure_clear(
        master_secret, sizeof(master_secret));

    gmssl_secure_clear(
        client_handshake_traffic_secret,
        sizeof(client_handshake_traffic_secret));

    gmssl_secure_clear(
        server_handshake_traffic_secret,
        sizeof(server_handshake_traffic_secret));

    gmssl_secure_clear(
        client_application_traffic_secret,
        sizeof(client_application_traffic_secret));

    gmssl_secure_clear(
        server_application_traffic_secret,
        sizeof(server_application_traffic_secret));

    gmssl_secure_clear(
        client_write_key,
        sizeof(client_write_key));

    gmssl_secure_clear(
        server_write_key,
        sizeof(server_write_key));

    gmssl_secure_clear(
        verify_data, sizeof(verify_data));
}

void TLS13_HSCTX_CLI::clear()
{
    super::clear();
    gmssl_secure_clear(&client_ecdhe,
        sizeof(client_ecdhe));

    gmssl_secure_clear(&server_sign_key,
        sizeof(server_sign_key));

    gmssl_secure_clear(&dgst_ctx,
        sizeof(dgst_ctx));

    cipher = nullptr;
}

void TLS13_HSCTX_SVR::clear()
{
    super::clear();
    gmssl_secure_clear(&server_ecdhe,
        sizeof(server_ecdhe));

    gmssl_secure_clear(&client_sign_key,
        sizeof(client_sign_key));
}
 
AGMS::AGMS()
{}

int AGMS::shutdown()
{
    int ret = 0;

    switch( get_state() )
    {
    case STAT_START_SHUTDOWN: 
        {
            tls_trace("send Alert close_notify\n");
            SEND_ALERT( TLS_alert_close_notify); 
            set_state( STAT_WAIT_CLOSE_NOTIFY );
            break;
        }
    case STAT_WAIT_CLOSE_NOTIFY:
        {
            int level, alert;
            ret = RET_PENDING;
            do{
                size_t recordlen = 0;
                uint8_t* record = nullptr;
                RECV_RECORD( prec, record, recordlen );
                UNREFERENCED( recordlen );
                uint8_t* tail_ptr =
                    prec->begin() + prec->size();
                while( record < tail_ptr )
                {
                    if (tls_record_get_alert(
                        record, &level, &alert) == 1 &&
                        alert == TLS_alert_close_notify )
                    {
                        set_state( STAT_CLOSED );
                        ret = 0;
                        break;
                    }
                    record +=
                        tls_record_length( record );
                }
                if( ret == 0 )
                    break;

            }while( 0 );

            break;
        }
    default:
        break;
    }

    return ret;
}

void AGMS::cleanup()
{
    TLS_CONNECT* conn = this;
    gmssl_secure_clear( conn,
        sizeof( TLS_CONNECT ) );

}

int AGMS::send_alert( int alert )
{
    int ret = 0;

    do{
        uint8_t record[5 + 2];
        size_t recordlen = 0;

        int proto = this->protocol;
        if( proto == TLS_protocol_tls13 )
            proto = TLS_protocol_tls12;

        tls_record_set_protocol(record, proto );

        tls_record_set_alert(
            record, &recordlen,
            TLS_alert_level_fatal, alert);

        SEND_RECORD( record, recordlen ); 

        tls_record_trace(stderr,
            record, sizeof(record), 0, 0);

    }while( 0 );

    return ret;
}

int TLS13::handle_alert( uint8_t* record )
{
    int ret = 0;
    if( record == nullptr )
        return -EINVAL;
    do{
        int level;
        int alert;

        size_t recordlen =
            tls_record_length( record );

        UNREFERENCED( recordlen );

        if (tls_record_get_alert(
            record, &level, &alert) != 1)
        { 
            error_print();
            ret = -1;
        }    
        tls_record_trace(stderr, record, *recordlen, 0, 0);
        if (level == TLS_alert_level_warning)
        {
            error_puts("Warning record received!\n");
        }
        else if (alert == TLS_alert_close_notify)
        {
            // close_notify是唯一需要提供反馈的Fatal
            // Alert，其他直接中止连接
            send_alert( TLS_alert_close_notify );
            this->set_state( STAT_CLOSED );
            ret = -ENOTCONN;
        }

    }while( 0 );

    return ret;
}

int TLS13::recv( IOV& iov )
{
	int ret = 0;
	const BLOCK_CIPHER_KEY *key;
	const uint8_t *iv;
	uint8_t *seq_num;
	uint8_t *enced_record; 
	size_t enced_recordlen;

	size_t recordlen;

	int record_type;

    TLS_CONNECT* conn = this;
	if (conn->is_client)
    {
		key = &conn->server_write_key;
		iv = conn->server_write_iv;
		seq_num = conn->server_seq_num;
	}
    else
    {
		key = &conn->client_write_key;
		iv = conn->client_write_iv;
		seq_num = conn->client_seq_num;
	}

    do{
        RECV_RECORD( prec, enced_record,
            enced_recordlen );

        uint8_t* tail_ptr = prec->end();

        while( enced_record < tail_ptr )
        {
            int rec_type =
                tls_record_type(enced_record);
            bool bNext = false;
            switch( rec_type )
            {
            case TLS_record_application_data:
                break;
            case TLS_record_alert:
                {
                    ret = this->handle_alert( enced_record );
                    bNext = true;
                    break;
                }
            case TLS_record_change_cipher_spec:
            case TLS_record_handshake:
            case TLS_record_heartbeat: 
            case TLS_record_tls12_cid:
            default:
                {
                    bNext = true;
                    break;
                }
            }

            if( ERROR( ret ) )
                break;

            if( bNext )
            {
                enced_record += enced_recordlen;
                continue;
            }

            PIOVE pdec_rec( new AGMS_IOVE );
            pdec_rec->alloc( enced_recordlen );

            if (tls13_gcm_decrypt(key, iv, seq_num,
                enced_record + TLS_RECORD_HEADER_SIZE,
                enced_recordlen - TLS_RECORD_HEADER_SIZE,
                &record_type,
                pdec_rec->begin(),
                &recordlen) != 1)
            {
                error_print();
                ret = -1;
                break;
            }

            if( recordlen > enced_recordlen )
            {
                ret = -ERANGE;
                break;
            }

            size_t overhead =
                ( enced_recordlen - recordlen );

            pdec_rec->trim_bytes_end( overhead );
            tls_seq_num_incr(seq_num);

            tls_trace("decrypt ApplicationData\n");

            tls_record_trace( stderr,
                pdec_rec->begin(), recordlen, 0, 0);

            iov.push_back( pdec_rec );

            enced_record += enced_recordlen;
            if( enced_record > tail_ptr )
            {
                ret = -EBADMSG;
                break;
            }

            if( enced_record == tail_ptr )
                break;

            enced_recordlen =
                tls_record_length( enced_record );
        }

    }while( 1 );

	return ret;
}

int TLS13::send( PIOVE& piove )
{
    const BLOCK_CIPHER_KEY *key;
    const uint8_t *iv;
    uint8_t *seq_num;
    uint8_t *cur_blk = piove->begin();
    size_t recordlen;
    size_t padding_len = 0; //FIXME: 在conn中设置是否加随机填充，及设置该值
    int ret = 0;

    if( !is_ready() )
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
            ( size_t )( tail_ptr - cur_blk ),
            ( size_t )TLS_MAX_CIPHERTEXT_SIZE );
         
        PIOVE prec( new AGMS_IOVE );
        size_t mem_size = datalen + 1 +
            padding_len + GHASH_SIZE  +
            TLS_RECORD_HEADER_SIZE;

        ret = prec->alloc( mem_size );
        if( ret < 0 )
            break;

        uint8_t* cur_rec = prec->begin();
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
	size_t recordlen = 0;

	uint8_t *enced_record = this->enced_record;
	size_t enced_recordlen = 0;

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
            digest_init(&hctxc.dgst_ctx, hctxc.digest);
            hctxc.null_dgst_ctx = hctxc.dgst_ctx;
            // send ClientHello

            uint8_t client_exts[TLS_MAX_EXTENSIONS_SIZE];
            size_t client_exts_len;
            uint8_t client_random[32];

            tls_trace("send ClientHello\n");
            tls_record_set_protocol(record, TLS_protocol_tls1);
            rand_bytes(client_random, 32);
            sm2_key_generate(&hctxc.client_ecdhe);

            tls13_client_hello_exts_set(client_exts,
                &client_exts_len, sizeof(client_exts),
                &(hctxc.client_ecdhe.public_key));

            size_t cipher_count =            
                sizeof(tls13_ciphers)/sizeof(tls13_ciphers[0]);

            tls_record_set_handshake_client_hello(
                record, &recordlen,
                TLS_protocol_tls12,
                client_random, NULL, 0,
                tls13_ciphers, cipher_count,
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
            ret = RET_PENDING;
            break;
            // 此时尚未确定digest算法，因此无法digest_update
        }
    case STAT_WAIT_SVR_HELLO:
        {
            // recv ServerHello
            tls_trace("recv ServerHello\n");
            RECV_RECORD( prec,
                enced_record, enced_recordlen );

            const uint8_t *server_exts;
            size_t server_exts_len;
            int protocol;
            int cipher_suite;
            const uint8_t *random;
            const uint8_t *session_id;
            size_t session_id_len;
            uint8_t server_random[32];

            tls13_record_trace(stderr,
                enced_record, enced_recordlen, 0, 0);
            if (tls_record_get_handshake_server_hello(
                enced_record, &protocol, &random,
                &session_id, &session_id_len,
                &cipher_suite, &server_exts,
                &server_exts_len) != 1)
            {
                SEND_ALERT(TLS_alert_unexpected_message);
                break;
            }
            if (protocol != TLS_protocol_tls12)
            {
                SEND_ALERT(TLS_alert_protocol_version);
                break;
            }

            memcpy(server_random, random, 32);
            memcpy(this->session_id, session_id, session_id_len);
            this->session_id_len = session_id_len;
            size_t cipher_count = 
                sizeof(tls13_ciphers)/sizeof(tls13_ciphers[0]);

            if (tls_cipher_suite_in_list( cipher_suite,
                tls13_ciphers, cipher_count ) != 1)
            {
                SEND_ALERT(TLS_alert_handshake_failure);
                break;
            }

            this->cipher_suite = cipher_suite;
            SM2_POINT server_ecdhe_public;
            if (tls13_server_hello_extensions_get(
                server_exts, server_exts_len,
                &server_ecdhe_public) != 1)
            {
                SEND_ALERT(TLS_alert_handshake_failure);
                break;
            }
            this->protocol = TLS_protocol_tls13;

            tls13_cipher_suite_get(
                this->cipher_suite, &hctxc.digest, &hctxc.cipher);

            digest_update(&hctxc.dgst_ctx,
                record + 5, recordlen - 5);

            digest_update(&hctxc.dgst_ctx,
                enced_record + 5, enced_recordlen - 5);


            printf("generate handshake secrets\n");
            /*
            generate handshake keys
                uint8_t client_write_key[32]
                uint8_t server_write_key[32]
                uint8_t client_write_iv[12]
                uint8_t server_write_iv[12]
            */

            uint8_t zeros[32] = {0};
            sm2_do_ecdh(&hctxc.client_ecdhe,
                &server_ecdhe_public,
                &server_ecdhe_public);

            /* [1]  */ tls13_hkdf_extract(hctxc.digest,
                zeros, hctxc.psk,
                hctxc.early_secret);

            /* [5]  */ tls13_derive_secret(
                hctxc.early_secret, "derived",
                &hctxc.null_dgst_ctx,
                hctxc.handshake_secret);

            /* [6]  */ tls13_hkdf_extract(
                hctxc.digest, hctxc.handshake_secret,
                (uint8_t *)&server_ecdhe_public,
                hctxc.handshake_secret);

            /* [7]  */ tls13_derive_secret(
                hctxc.handshake_secret,
                "c hs traffic", &hctxc.dgst_ctx,
                hctxc.client_handshake_traffic_secret);

            /* [8]  */ tls13_derive_secret(
                hctxc.handshake_secret,
                "s hs traffic", &hctxc.dgst_ctx,
                hctxc.server_handshake_traffic_secret);

            /* [9]  */ tls13_derive_secret(
                hctxc.handshake_secret,
                "derived", &hctxc.null_dgst_ctx,
                hctxc.master_secret);

            /* [10] */ tls13_hkdf_extract(
                hctxc.digest, hctxc.master_secret,
                zeros, hctxc.master_secret);

            //[sender]_write_key = HKDF-Expand-Label(Secret, "key", "", key_length)
            //[sender]_write_iv  = HKDF-Expand-Label(Secret, "iv", "", iv_length)
            //[sender] in {server, client}

            tls13_hkdf_expand_label(hctxc.digest,
                hctxc.server_handshake_traffic_secret,
                "key", NULL, 0, 16, hctxc.server_write_key);

            tls13_hkdf_expand_label(hctxc.digest,
                hctxc.server_handshake_traffic_secret,
                "iv", NULL, 0, 12, this->server_write_iv);
            block_cipher_set_encrypt_key(
                &this->server_write_key,
                hctxc.cipher, hctxc.server_write_key);

            memset(this->server_seq_num, 0, 8);
            tls13_hkdf_expand_label(hctxc.digest,
                hctxc.client_handshake_traffic_secret,
                "key", NULL, 0, 16, hctxc.client_write_key);

            tls13_hkdf_expand_label(hctxc.digest,
                hctxc.client_handshake_traffic_secret,
                "iv", NULL, 0, 12, this->client_write_iv);

            block_cipher_set_encrypt_key(
                &this->client_write_key,
                hctxc.cipher, hctxc.client_write_key);

            memset(this->client_seq_num, 0, 8);

            SWITCH_STATE(
                STAT_WAIT_SVR_ENCEXT, enced_recordlen );
            // fall through
        }
    case STAT_WAIT_SVR_ENCEXT:
        {
            // recv {EncryptedExtensions}
            tls_trace("recv {EncryptedExtensions}\n");
            RECV_RECORD( prec,
                enced_record, enced_recordlen );

            if (tls13_record_decrypt(
                &this->server_write_key,
                this->server_write_iv,
                this->server_seq_num,
                enced_record, enced_recordlen,
                record, &recordlen) != 1)
            {
                SEND_ALERT(TLS_alert_bad_record_mac);
                break;
            }

            tls13_record_trace(
                stderr, record, recordlen, 0, 0);

            if (tls13_record_get_handshake_encrypted_extensions(record) != 1)
            {
                SEND_ALERT(TLS_alert_handshake_failure);
                break;
            }

            digest_update(&hctxc.dgst_ctx,
                record + 5, recordlen - 5);

            tls_seq_num_incr(this->server_seq_num);

            SWITCH_STATE(
                STAT_WAIT_SVR_CERT, enced_recordlen );
            // fall through
        }
    case STAT_WAIT_SVR_CERT:
        {
            // recv {CertififcateRequest*} or {Certificate}
            PIOVE prec;

            int type;
            const uint8_t *data;
            size_t datalen;
            bool wait_cert = false;

            do{
                const uint8_t *request_context;
                size_t request_context_len;
                const uint8_t *cert_request_exts;
                size_t cert_request_extslen;

                ret = this->read_bio.read( prec );
                if( ret < 0 )
                {
                    ret = 0;
                    break;
                }

                enced_record = prec->begin();
                enced_recordlen =
                    tls_record_length( enced_record );

                if (tls13_record_decrypt(
                    &this->server_write_key,
                    this->server_write_iv,
                    this->server_seq_num,
                    enced_record, enced_recordlen,
                    record, &recordlen) != 1)
                {
                    SEND_ALERT(TLS_alert_bad_record_mac);
                    break;
                }

                if (tls_record_get_handshake(
                    record, &type, &data, &datalen) != 1)
                {
                    SEND_ALERT(TLS_alert_handshake_failure);
                    break;
                }

                if (!hctxc.cert_req_received &&
                    type == TLS_handshake_certificate_request)
                {
                    tls_trace("recv {CertificateRequest*}\n");
                    tls13_record_trace(
                        stderr, record, recordlen, 0, 0);

                    if (tls13_record_get_handshake_certificate_request(
                        record, &request_context,
                        &request_context_len,
                        &cert_request_exts,
                        &cert_request_extslen) != 1)
                    {
                        SEND_ALERT(TLS_alert_handshake_failure);
                        break;
                    }

                    // 当前忽略 request_context 和 cert_request_exts
                    // request_context 应该为空，当前实现中不支持Post-Handshake Auth

                    digest_update(&hctxc.dgst_ctx,
                        record + 5, recordlen - 5);

                    tls_seq_num_incr(
                        this->server_seq_num);

                    hctxc.cert_req_received = true;
                    if( prec->size() == enced_recordlen )
                    {
                        if( this->read_bio.size() == 0 )
                        {
                            // waiting for cert
                            wait_cert = true;
                            break;
                        }
                    }
                    else
                    {
                        prec->trim_bytes_front( enced_recordlen );
                        this->read_bio.put_back( prec );
                    }
                    // move on to next record
                    continue;
                }
                else if(hctxc.cert_req_received &&
                    type == TLS_handshake_certificate_request)
                {
                    ret = -EPROTO;
                    break;
                }
                else
                {
                    if( !hctxc.cert_req_received )
                        this->client_certs_len = 0;
                }

                break;

            }while( 1 );

            if( ret < 0 || wait_cert )
                break;

            // received {Certificate}
            const uint8_t *cert_list;
            size_t cert_list_len;
            const uint8_t *cert;
            size_t certlen;
            const uint8_t *request_context;
            size_t request_context_len;

            tls_trace("recv {Certificate}\n");
            tls13_record_trace(stderr, record, recordlen, 0, 0);
            if (tls13_record_get_handshake_certificate(
                record, &request_context,
                &request_context_len,
                &cert_list, &cert_list_len) != 1)
            {
                SEND_ALERT(TLS_alert_unexpected_message);
                break;
            }
            if (tls13_process_certificate_list(
                cert_list,cert_list_len,
                this->server_certs,
                &this->server_certs_len) != 1)
            {
                SEND_ALERT(TLS_alert_unexpected_message);
                break;
            }

            if (x509_certs_get_cert_by_index(
                this->server_certs,
                this->server_certs_len,
                0, &cert, &certlen) != 1 ||
                x509_cert_get_subject_public_key(
                cert, certlen, &hctxc.server_sign_key) != 1)
            {
                SEND_ALERT(TLS_alert_unexpected_message);
                break;
            }

            digest_update(&hctxc.dgst_ctx, record + 5, recordlen - 5);
            tls_seq_num_incr(this->server_seq_num);

            // verify server certificate
            int verify_result = 0;
            if (x509_certs_verify(
                this->server_certs,
                this->server_certs_len,
                X509_cert_chain_server,
                this->ca_certs,
                this->ca_certs_len,
                X509_MAX_VERIFY_DEPTH,
                &verify_result) != 1)
            {
                SEND_ALERT(TLS_alert_bad_certificate);
                break;
            }

            SWITCH_STATE( STAT_WAIT_SVR_CERTVERIFY,
                enced_recordlen );
            // fall through
        }
    case STAT_WAIT_SVR_CERTVERIFY:
        {
            // recv {CertificateVerify}

            RECV_RECORD( prec,
                enced_record, enced_recordlen );

            tls_trace("recv {CertificateVerify}\n");
            if (tls13_record_decrypt(
                &this->server_write_key,
                this->server_write_iv,
                this->server_seq_num,
                enced_record, enced_recordlen,
                record, &recordlen) != 1)
            {
                SEND_ALERT(TLS_alert_bad_record_mac);
                break;
            }

            tls13_record_trace(
                stderr, record, recordlen, 0, 0);

            const uint8_t *server_sig;
            int server_sign_algor;
            size_t server_siglen;

            if (tls13_record_get_handshake_certificate_verify(
                record, &server_sign_algor,
                &server_sig, &server_siglen) != 1)
            {
                SEND_ALERT(TLS_alert_unexpected_message);
                break;
            }
            if (server_sign_algor != TLS_sig_sm2sig_sm3)
            {
                SEND_ALERT(TLS_alert_unexpected_message);
                break;
            }
            if (tls13_verify_certificate_verify(
                TLS_server_mode,
                &hctxc.server_sign_key,
                TLS13_SM2_ID,
                TLS13_SM2_ID_LENGTH,
                &hctxc.dgst_ctx,
                server_sig,
                server_siglen) != 1)
            {
                error_print();
                ret = -1;
                break;
            }

            digest_update(&hctxc.dgst_ctx,
                record + 5, recordlen - 5);

            tls_seq_num_incr(
                this->server_seq_num);

            // use Transcript-Hash(Handshake Context, Certificate*, CertificateVerify*)
            tls13_compute_verify_data(
                hctxc.server_handshake_traffic_secret,
                &hctxc.dgst_ctx,
                hctxc.verify_data,
                &hctxc.verify_data_len);

            SWITCH_STATE( STAT_WAIT_SVR_FIN, enced_recordlen );
            // fall through
        }
    case STAT_WAIT_SVR_FIN:
        {
            RECV_RECORD( prec,
                enced_record, enced_recordlen );
            // recv {Finished}
            tls_trace("recv {Finished}\n");

            if (tls13_record_decrypt(
                &this->server_write_key,
                this->server_write_iv,
                this->server_seq_num,
                enced_record,
                enced_recordlen,
                record, &recordlen) != 1)
            {
                SEND_ALERT(TLS_alert_bad_record_mac);
                break;
            }

            const uint8_t *server_verify_data;
            size_t server_verify_data_len;
            size_t padding_len;

            tls13_record_trace(
                stderr, record, recordlen, 0, 0);

            if (tls13_record_get_handshake_finished(
                record, &server_verify_data,
                &server_verify_data_len) != 1)
            {
                SEND_ALERT(TLS_alert_unexpected_message);
                break;
            }
            if (server_verify_data_len != hctxc.verify_data_len ||
                memcmp(server_verify_data, hctxc.verify_data,
                hctxc.verify_data_len) != 0)
            {
                SEND_ALERT(TLS_alert_unexpected_message);
                break;
            }

            digest_update(&hctxc.dgst_ctx,
                record + 5, recordlen - 5);

            tls_seq_num_incr(this->server_seq_num);

            // generate server_application_traffic_secret
            /* [12] */ tls13_derive_secret(hctxc.master_secret,
                "s ap traffic", &hctxc.dgst_ctx,
                hctxc.server_application_traffic_secret);

            // generate client_application_traffic_secret
            /* [11] */ tls13_derive_secret(hctxc.master_secret,
                "c ap traffic", &hctxc.dgst_ctx,
                hctxc.client_application_traffic_secret);

            if (this->client_certs_len)
            {
                int client_sign_algor;
                uint8_t sig[TLS_MAX_SIGNATURE_SIZE];
                size_t siglen;

                // send client {Certificate*}
                tls_trace("send {Certificate*}\n");
                if (tls13_record_set_handshake_certificate(
                    record, &recordlen, NULL, 0,
                    this->client_certs,
                    this->client_certs_len) != 1)
                {
                    SEND_ALERT(TLS_alert_internal_error);
                    break;
                }

                tls13_record_trace(
                    stderr, record, recordlen, 0, 0);

                tls13_padding_len_rand(&padding_len);
                if (tls13_record_encrypt(
                    &this->client_write_key,
                    this->client_write_iv,
                    this->client_seq_num,
                    record, recordlen, padding_len,
                    enced_record, &enced_recordlen) != 1) {
                    error_print();
                    SEND_ALERT(TLS_alert_internal_error);
                    return -1;
                }

                SEND_RECORD( enced_record, enced_recordlen );

                digest_update(&hctxc.dgst_ctx,
                    record + 5, recordlen - 5);

                tls_seq_num_incr(this->client_seq_num);

                // send {CertificateVerify*}
                tls_trace("send {CertificateVerify*}\n");
                client_sign_algor = TLS_sig_sm2sig_sm3; // FIXME: 应该放在this里面

                tls13_sign_certificate_verify(
                    TLS_client_mode,
                    &this->sign_key,
                    TLS13_SM2_ID,
                    TLS13_SM2_ID_LENGTH,
                    &hctxc.dgst_ctx,
                    sig, &siglen);

                if (tls13_record_set_handshake_certificate_verify(
                    record, &recordlen,
                    client_sign_algor,
                    sig, siglen) != 1)
                {
                    SEND_ALERT(TLS_alert_internal_error);
                    break;
                }

                tls13_record_trace(
                    stderr, record, recordlen, 0, 0);
                tls13_padding_len_rand(&padding_len);

                if (tls13_record_encrypt(
                    &this->client_write_key,
                    this->client_write_iv,
                    this->client_seq_num,
                    record, recordlen, padding_len,
                    enced_record, &enced_recordlen) != 1)
                {
                    SEND_ALERT(TLS_alert_internal_error);
                    break;
                }

                SEND_RECORD( enced_record, enced_recordlen );

                digest_update(&hctxc.dgst_ctx,
                    record + 5, recordlen - 5);
                tls_seq_num_incr(this->client_seq_num);
            }

            // send Client {Finished}
            tls_trace("send {Finished}\n");
            tls13_compute_verify_data(
                hctxc.client_handshake_traffic_secret,
                &hctxc.dgst_ctx, hctxc.verify_data,
                &hctxc.verify_data_len);

            if (tls_record_set_handshake_finished(
                record, &recordlen, hctxc.verify_data,
                hctxc.verify_data_len) != 1)
            {
                SEND_ALERT(TLS_alert_internal_error);
                break;
            }

            tls13_record_trace(
                stderr, record, recordlen, 0, 0);
            tls13_padding_len_rand(&padding_len);
            if (tls13_record_encrypt(
                &this->client_write_key,
                this->client_write_iv,
                this->client_seq_num,
                record, recordlen, padding_len,
                enced_record, &enced_recordlen) != 1)
            {
                SEND_ALERT(TLS_alert_internal_error);
                break;
            }

            SEND_RECORD( enced_record, enced_recordlen );

            digest_update(&hctxc.dgst_ctx,
                record + 5, recordlen - 5);

            tls_seq_num_incr(this->client_seq_num);

            // update server_write_key, server_write_iv, reset server_seq_num
            tls13_hkdf_expand_label(hctxc.digest,
                hctxc.server_application_traffic_secret,
                "key", NULL, 0, 16,
                hctxc.server_write_key);

            block_cipher_set_encrypt_key(
                &this->server_write_key,
                hctxc.cipher,
                hctxc.server_write_key);

            tls13_hkdf_expand_label(hctxc.digest,
                hctxc.server_application_traffic_secret,
                "iv", NULL, 0, 12,
                this->server_write_iv);

            memset(this->server_seq_num, 0, 8);

            //update client_write_key, client_write_iv, reset client_seq_num
            tls13_hkdf_expand_label(hctxc.digest,
                hctxc.client_application_traffic_secret,
                "key", NULL, 0, 16,
                hctxc.client_write_key);

            tls13_hkdf_expand_label(hctxc.digest,
                hctxc.client_application_traffic_secret,
                "iv", NULL, 0, 12,
                this->client_write_iv);

            block_cipher_set_encrypt_key(
                &this->client_write_key,
                hctxc.cipher,
                hctxc.client_write_key);

            memset(this->client_seq_num, 0, 8);
            set_state( STAT_READY );
            ret = 0;
            break;
        }
    default:
        {
            ret = -EFAULT;
            break;
        }
    }

    if( ret < 0 )
        set_state( STAT_HANDSHAKE_FAILED );

    if( ret < 0 || get_state() == STAT_READY )
        hctxc.clear();

    return ret;
}

int TLS13::handshake_svr()
{
	int ret = 0;
	uint8_t *record = this->record;
	size_t recordlen = 0;
	uint8_t* enced_record = this->enced_record;
	size_t enced_recordlen = 0;

    const int* server_ciphers = tls13_ciphers;

	// SM2_KEY server_ecdhe;
	// SM2_KEY client_sign_key;

	// const BLOCK_CIPHER *cipher;
	// const DIGEST *digest;
	// DIGEST_CTX dgst_ctx;
	// DIGEST_CTX null_dgst_ctx;


	// uint8_t verify_data[32];
	// size_t verify_data_len;

	// uint8_t client_write_key[16];
	// uint8_t server_write_key[16];

	// uint8_t psk[32] = {0};
	// uint8_t early_secret[32];
	// uint8_t handshake_secret[32];
	// uint8_t client_handshake_traffic_secret[32];
	// uint8_t server_handshake_traffic_secret[32];
	// uint8_t client_application_traffic_secret[32];
	// uint8_t server_application_traffic_secret[32];
	// uint8_t master_secret[32];

	int client_verify = 0;
	if (this->ca_certs_len)
		client_verify = 1;

    switch( get_state() )
    {
    case STAT_START_HANDSHAKE:
        {
            // 1. Recv ClientHello
            tls_trace("recv ClientHello\n");
            RECV_RECORD( prec, record, recordlen );

            int protocol;
            const uint8_t *random;
            size_t padding_len;

            const uint8_t *session_id;
            size_t session_id_len;

            const uint8_t *client_exts;
            size_t client_exts_len;

            uint8_t client_random[32];
            uint8_t server_random[32];

            const uint8_t *client_ciphers;
            size_t client_ciphers_len;

            uint8_t server_exts[TLS_MAX_EXTENSIONS_SIZE];
            size_t server_exts_len;

            SM2_POINT client_ecdhe_public;

            tls13_record_trace(stderr, record, recordlen, 0, 0);
            if (tls_record_get_handshake_client_hello(record,
                &protocol, &random,
                &session_id, &session_id_len, // 不支持SessionID，不做任何处理
                &client_ciphers, &client_ciphers_len,
                &client_exts, &client_exts_len) != 1)
            {
                SEND_ALERT(TLS_alert_unexpected_message);
                break;
            }
            if (protocol != TLS_protocol_tls12)
            {
                SEND_ALERT(TLS_alert_protocol_version);
                break;
            }
            memcpy(client_random, random, 32);

            int server_cipher_count =
                sizeof(server_ciphers)/sizeof(int);

            if (tls_cipher_suites_select(
                client_ciphers, client_ciphers_len,
                server_ciphers, server_cipher_count, 
                &this->cipher_suite) != 1)
            {
                SEND_ALERT(TLS_alert_insufficient_security);
                break;
            }
            if (!client_exts)
            {
                error_print();
                ret = -1;
                break;
            }

            tls13_cipher_suite_get(this->cipher_suite,
                &hctxs.digest, &hctxs.cipher); // 这个函数是否应该放到tls_里面？

            digest_init(&hctxs.dgst_ctx, hctxs.digest);

            hctxs.null_dgst_ctx = hctxs.dgst_ctx; // 在密钥导出函数中可能输入的消息为空，因此需要一个空的dgst_ctx，这里不对了，应该在tls13_derive_secret里面直接支持NULL！
            digest_update(&hctxs.dgst_ctx, record + 5, recordlen - 5);


            // 2. Send ServerHello
            tls_trace("send ServerHello\n");
            rand_bytes(server_random, 32);
            sm2_key_generate(&hctxs.server_ecdhe);
            if (tls13_process_client_hello_exts(
                client_exts, client_exts_len,
                &hctxs.server_ecdhe, &client_ecdhe_public,
                server_exts, &server_exts_len,
                sizeof(server_exts)) != 1)
            {
                SEND_ALERT(TLS_alert_unexpected_message);
                break;
            }

            tls_record_set_protocol(
                record, TLS_protocol_tls12);

            if (tls_record_set_handshake_server_hello(
                record, &recordlen,
                TLS_protocol_tls12, server_random,
                NULL, 0, this->cipher_suite,
                server_exts, server_exts_len) != 1)
            {
                SEND_ALERT(TLS_alert_unexpected_message);
                break;
            }

            tls13_record_trace(stderr, record, recordlen, 0, 0);

            SEND_RECORD( record, recordlen );

            digest_update(&hctxs.dgst_ctx,
                record + 5, recordlen - 5);


            uint8_t zeros[32] = {0};
            sm2_do_ecdh(&hctxs.server_ecdhe,
                &client_ecdhe_public, &client_ecdhe_public);

            /* 1  */ tls13_hkdf_extract(hctxs.digest,
                zeros, hctxs.psk, hctxs.early_secret);

            /* 5  */ tls13_derive_secret(
                hctxs.early_secret,
                "derived", &hctxs.null_dgst_ctx,
                hctxs.handshake_secret);

            /* 6  */ tls13_hkdf_extract(
                hctxs.digest,
                hctxs.handshake_secret,
                (uint8_t *)&client_ecdhe_public,
                hctxs.handshake_secret);

            /* 7  */ tls13_derive_secret(
                hctxs.handshake_secret,
                "c hs traffic", &hctxs.dgst_ctx,
                hctxs.client_handshake_traffic_secret);

            /* 8  */ tls13_derive_secret(
                hctxs.handshake_secret,
                "s hs traffic", &hctxs.dgst_ctx,
                hctxs.server_handshake_traffic_secret);

            /* 9  */ tls13_derive_secret(
                hctxs.handshake_secret,
                "derived", &hctxs.null_dgst_ctx,
                hctxs.master_secret);

            /* 10 */ tls13_hkdf_extract(
                hctxs.digest, hctxs.master_secret,
                zeros, hctxs.master_secret);

            // generate server_write_key, server_write_iv, reset server_seq_num
            tls13_hkdf_expand_label(hctxs.digest,
                hctxs.server_handshake_traffic_secret,
                "key", NULL, 0, 16, hctxs.server_write_key);

            block_cipher_set_encrypt_key(
                &this->server_write_key,
                hctxs.cipher, hctxs.server_write_key);

            tls13_hkdf_expand_label(hctxs.digest,
                hctxs.server_handshake_traffic_secret,
                "iv", NULL, 0, 12, this->server_write_iv);

            memset(this->server_seq_num, 0, 8);
            // generate client_write_key, client_write_iv, reset client_seq_num
            tls13_hkdf_expand_label(hctxs.digest,
                hctxs.client_handshake_traffic_secret,
                "key", NULL, 0, 16, hctxs.client_write_key);

            block_cipher_set_encrypt_key(
                &this->client_write_key,
                hctxs.cipher, hctxs.client_write_key);

            tls13_hkdf_expand_label(
                hctxs.digest,
                hctxs.client_handshake_traffic_secret,
                "iv", NULL, 0, 12, this->client_write_iv);

            memset(this->client_seq_num, 0, 8);

            // 3. Send {EncryptedExtensions}
            tls_trace("send {EncryptedExtensions}\n");
            tls_record_set_protocol(record, TLS_protocol_tls12);

            tls13_record_set_handshake_encrypted_extensions(
                record, &recordlen);

            tls13_record_trace(stderr, record, recordlen, 0, 0);
            tls13_padding_len_rand(&padding_len);

            if (tls13_record_encrypt(
                &this->server_write_key,
                this->server_write_iv,
                this->server_seq_num,
                record, recordlen, padding_len,
                enced_record, &enced_recordlen) != 1)
            {
                SEND_ALERT(TLS_alert_internal_error);
                break;
            }
            // FIXME: tls13_record_encrypt需要支持握手消息
            // tls_record_data(enced_record)[0] = TLS_handshake_encrypted_extensions;
            SEND_RECORD( enced_record, enced_recordlen );

            digest_update(&hctxs.dgst_ctx,
                record + 5, recordlen - 5);

            tls_seq_num_incr(this->server_seq_num);


            // send {CertificateRequest*}
            if (client_verify)
            {
                tls_trace("send {CertificateRequest*}\n");

                // TODO: 设置certificate_request中的extensions!
                if (tls13_record_set_handshake_certificate_request_default(
                    record, &recordlen) != 1)
                {
                    SEND_ALERT(TLS_alert_internal_error);
                    break;
                }

                tls13_record_trace(
                    stderr, record, recordlen, 0, 0);

                tls13_padding_len_rand(&padding_len);

                if (tls13_record_encrypt(
                    &this->server_write_key,
                    this->server_write_iv,
                    this->server_seq_num,
                    record, recordlen, padding_len,
                    enced_record, &enced_recordlen) != 1)
                {
                    SEND_ALERT(TLS_alert_internal_error);
                    break;
                }
                SEND_RECORD( enced_record, enced_recordlen );

                digest_update(&hctxs.dgst_ctx,
                    record + 5, recordlen - 5);

                tls_seq_num_incr(this->server_seq_num);
            }

            // send Server {Certificate}
            tls_trace("send {Certificate}\n");
            if (tls13_record_set_handshake_certificate(
                record, &recordlen,
                NULL, 0, this->server_certs,
                this->server_certs_len) != 1)
            {
                SEND_ALERT(TLS_alert_internal_error);
                break;
            }

            tls13_record_trace(
                stderr, record, recordlen, 0, 0);

            tls13_padding_len_rand(&padding_len);

            if (tls13_record_encrypt(
                &this->server_write_key,
                this->server_write_iv,
                this->server_seq_num,
                record, recordlen, padding_len,
                enced_record, &enced_recordlen) != 1)
            {
                SEND_ALERT(TLS_alert_internal_error);
                break;
            }

            SEND_RECORD( enced_record, enced_recordlen );

            digest_update(&hctxs.dgst_ctx,
                record + 5, recordlen - 5);

            tls_seq_num_incr(this->server_seq_num);


            uint8_t sig[TLS_MAX_SIGNATURE_SIZE];
            size_t siglen = sizeof(sig);

            // send Server {CertificateVerify}
            tls_trace("send {CertificateVerify}\n");
            tls13_sign_certificate_verify(
                TLS_server_mode, &this->sign_key,
                TLS13_SM2_ID, TLS13_SM2_ID_LENGTH,
                &hctxs.dgst_ctx, sig, &siglen);

            if (tls13_record_set_handshake_certificate_verify(
                record, &recordlen, TLS_sig_sm2sig_sm3,
                sig, siglen) != 1)
            {
                SEND_ALERT(TLS_alert_internal_error);
                break;
            }
            tls13_record_trace(
                stderr, record, recordlen, 0, 0);

            tls13_padding_len_rand(&padding_len);
            if (tls13_record_encrypt(
                &this->server_write_key,
                this->server_write_iv,
                this->server_seq_num,
                record, recordlen, padding_len,
                enced_record, &enced_recordlen) != 1)
            {
                SEND_ALERT(TLS_alert_internal_error);
                break;
            }

            SEND_RECORD( enced_record, enced_recordlen );

            digest_update(&hctxs.dgst_ctx,
                record + 5, recordlen - 5);

            tls_seq_num_incr(this->server_seq_num);


            // Send Server {Finished}
            tls_trace("send {Finished}\n");

            // compute server verify_data before digest_update()
            tls13_compute_verify_data(
                hctxs.server_handshake_traffic_secret,
                &hctxs.dgst_ctx, hctxs.verify_data,
                &hctxs.verify_data_len);

            if (tls13_record_set_handshake_finished(
                record, &recordlen, hctxs.verify_data,
                hctxs.verify_data_len) != 1)
            {
                SEND_ALERT(TLS_alert_internal_error);
                break;
            }
            tls13_record_trace(
                stderr, record, recordlen, 0, 0);

            tls13_padding_len_rand(&padding_len);

            if (tls13_record_encrypt(
                &this->server_write_key,
                this->server_write_iv,
                this->server_seq_num,
                record, recordlen, padding_len,
                enced_record, &enced_recordlen) != 1)
            {
                SEND_ALERT(TLS_alert_internal_error);
                break;
            }

            SEND_RECORD( enced_record, enced_recordlen );
            digest_update(&hctxs.dgst_ctx,
                record + 5, recordlen - 5);
            tls_seq_num_incr(this->server_seq_num);

            // generate server_application_traffic_secret
            /* 12 */ tls13_derive_secret(
                hctxs.master_secret,
                "s ap traffic", &hctxs.dgst_ctx,
                hctxs.server_application_traffic_secret);

            // Generate client_application_traffic_secret
            /* 11 */ tls13_derive_secret(
                hctxs.master_secret,
                "c ap traffic", &hctxs.dgst_ctx,
                hctxs.client_application_traffic_secret);

            // 因为后面还要解密握手消息，因此client application key, iv 等到握手结束之后再更新

            if( client_verify )
            {
                SWITCH_STATE( STAT_WAIT_CLI_CERT, recordlen );
            }
            else
            {
                SWITCH_STATE( STAT_WAIT_CLI_FIN, recordlen );
            }
            break;
        }
    case STAT_WAIT_CLI_CERT:
        {
            // Recv Client {Certificate*}
            tls_trace("recv {Certificate*}\n");
            RECV_RECORD( prec, enced_record, enced_recordlen );

            const uint8_t *request_context;
            size_t request_context_len;

            const uint8_t *cert_list;
            size_t cert_list_len;

            const uint8_t *cert;
            size_t certlen;

            if (tls13_record_decrypt(
                &this->client_write_key,
                this->client_write_iv,
                this->client_seq_num,
                enced_record, enced_recordlen,
                record, &recordlen) != 1)
            {
                SEND_ALERT(TLS_alert_bad_record_mac);
                break;
            }
            tls13_record_trace(stderr, record, recordlen, 0, 0);

            if (tls13_record_get_handshake_certificate(
                record, &request_context,
                &request_context_len,
                &cert_list, &cert_list_len) != 1)
            {
                SEND_ALERT(TLS_alert_unexpected_message);
                break;
            }

            if (tls13_process_certificate_list(
                cert_list, cert_list_len,
                this->client_certs,
                &this->client_certs_len) != 1)
            {
                SEND_ALERT(TLS_alert_unexpected_message);
                break;
            }
            if (x509_certs_get_cert_by_index(
                this->client_certs,
                this->client_certs_len,
                0, &cert, &certlen) != 1)
            {
                SEND_ALERT(TLS_alert_unexpected_message);
                break;
            }
            if (x509_cert_get_subject_public_key(
                cert, certlen, &hctxs.client_sign_key) != 1)
            {
                SEND_ALERT(TLS_alert_unexpected_message);
                break;
            }

            digest_update(&hctxs.dgst_ctx,
                record + 5, recordlen - 5);

            tls_seq_num_incr(this->client_seq_num);

            // verify client certificate
            int verify_result;
            if (x509_certs_verify(
                this->client_certs,
                this->client_certs_len,
                X509_cert_chain_client,
                this->ca_certs, this->ca_certs_len,
                X509_MAX_VERIFY_DEPTH,
                &verify_result) != 1)
            {
                SEND_ALERT(TLS_alert_bad_certificate);
                break;
            }

            SWITCH_STATE(
                STAT_WAIT_CLI_CERTVERIFY, enced_recordlen );
            // fall through
        }
    case STAT_WAIT_CLI_CERTVERIFY:
        {
            // Recv client {CertificateVerify*}
            int client_sign_algor;
            const uint8_t *client_sig;
            size_t client_siglen;

            tls_trace("recv Client {CertificateVerify*}\n");
            RECV_RECORD( prec, enced_record, enced_recordlen );
            if (tls13_record_decrypt(
                &this->client_write_key,
                this->client_write_iv,
                this->client_seq_num,
                enced_record, enced_recordlen,
                record, &recordlen) != 1)
            {
                SEND_ALERT(TLS_alert_bad_record_mac);
                break;
            }

            tls13_record_trace(stderr, record, recordlen, 0, 0);

            if (tls13_record_get_handshake_certificate_verify(
                record, &client_sign_algor,
                &client_sig, &client_siglen) != 1)
            {
                SEND_ALERT(TLS_alert_unexpected_message);
                break;
            }
            if (tls13_verify_certificate_verify(
                TLS_client_mode,
                &hctxs.client_sign_key,
                TLS13_SM2_ID, TLS13_SM2_ID_LENGTH,
                &hctxs.dgst_ctx, client_sig,
                client_siglen) != 1)
            {
                SEND_ALERT(TLS_alert_decrypt_error);
                break;
            }

            digest_update(&hctxs.dgst_ctx,
                record + 5, recordlen - 5);

            tls_seq_num_incr(this->client_seq_num);
            SWITCH_STATE( STAT_WAIT_CLI_FIN, enced_recordlen );
            // fall through
        }
    case STAT_WAIT_CLI_FIN:
        {
            // 12. Recv Client {Finished}
            tls_trace("recv {Finished}\n");
            RECV_RECORD( prec, enced_record, enced_recordlen );

            const uint8_t *client_verify_data;
            size_t client_verify_data_len;

            if (tls13_record_decrypt(
                &this->client_write_key,
                this->client_write_iv,
                this->client_seq_num,
                enced_record, enced_recordlen,
                record, &recordlen) != 1)
            {
                SEND_ALERT(TLS_alert_bad_record_mac);
                break;
            }
            tls13_record_trace(
                stderr, record, recordlen, 0, 0);

            if (tls13_record_get_handshake_finished(
                record, &client_verify_data,
                &client_verify_data_len) != 1)
            {
                SEND_ALERT(TLS_alert_unexpected_message);
                break;
            }

            if (tls13_compute_verify_data(
                hctxs.client_handshake_traffic_secret,
                &hctxs.dgst_ctx, hctxs.verify_data,
                &hctxs.verify_data_len) != 1)
            {
                SEND_ALERT(TLS_alert_internal_error);
                break;
            }

            if (client_verify_data_len != hctxs.verify_data_len ||
                memcmp(client_verify_data,
                    hctxs.verify_data,
                    hctxs.verify_data_len) != 0)
            {
                SEND_ALERT(TLS_alert_bad_record_mac);
                break;
            }

            digest_update(&hctxs.dgst_ctx,
                record + 5, recordlen - 5);

            tls_seq_num_incr(this->client_seq_num);


            // 注意：OpenSSL兼容模式在此处会收发ChangeCipherSpec报文

            // update server_write_key, server_write_iv, reset server_seq_num
            tls13_hkdf_expand_label(hctxs.digest,
                hctxs.server_application_traffic_secret,
                "key", NULL, 0, 16,
                hctxs.server_write_key);

            tls13_hkdf_expand_label(hctxs.digest,
                hctxs.server_application_traffic_secret,
                "iv", NULL, 0, 12, this->server_write_iv);

            block_cipher_set_encrypt_key(
                &this->server_write_key,
                hctxs.cipher, hctxs.server_write_key);

            memset(this->server_seq_num, 0, 8);

            // update client_write_key, client_write_iv
            // reset client_seq_num
            tls13_hkdf_expand_label(hctxs.digest,
                hctxs.client_application_traffic_secret,
                "key", NULL, 0, 16,
                hctxs.client_write_key);

            tls13_hkdf_expand_label(hctxs.digest,
                hctxs.client_application_traffic_secret,
                "iv", NULL, 0, 12,
                this->client_write_iv);

            block_cipher_set_encrypt_key(
                &this->client_write_key,
                hctxs.cipher, hctxs.client_write_key);

            memset(this->client_seq_num, 0, 8);
            set_state( STAT_READY );
            break;
        }
    default:
        ret = -1;
        break;
    }

    if( ret < 0 )
        set_state( STAT_HANDSHAKE_FAILED );

    if( ret < 0 ||
        get_state() == STAT_READY )
        hctxs.clear();

    return ret;
}

int TLS13::init( bool is_client )
{
    this->is_client = ( int )is_client;
    TLS_CONNECT* conn = this;
    memset(conn, 0, sizeof(*conn));
    return init_connect();
}

int init_ctx( AGMS_CTX* ctx, bool is_client )
{
    if( ctx == nullptr )
        return -EINVAL;

    int ret = 0;
    int ciphers[] = { TLS_cipher_sm4_gcm_sm3 };
    do{
        int mode = TLS_server_mode;
        if( is_client )
            mode = TLS_client_mode;

        ret = tls_ctx_init( ctx,
            TLS_protocol_tls13, mode );

        if( ret < 0 )
            break;

        size_t cipher_count = sizeof(ciphers);
        cipher_count /= sizeof(ciphers[0]);

        ret = tls_ctx_set_cipher_suites(
            ctx, ciphers,
            cipher_count );
        if( ret != 1 )
            break;

        if( ctx->certfile.size() &&
            tls_ctx_set_certificate_and_key(
            ctx, ctx->certfile.c_str(),
            ctx->keyfile.c_str(),
            ctx->password.c_str()) != 1)
        {
            ret = -1;
            break;
        }

        if (ctx->cacertfile.size())
        {
            if (tls_ctx_set_ca_certificates(
                ctx, ctx->cacertfile.c_str(),
                TLS_DEFAULT_VERIFY_DEPTH) != 1)
            {
                ret = -1;
                break;
            }
        }

    }while( 0 );

    ctx->password.replace(
        0, std::string::npos,
        ctx->password.size(), '0' );

    return ret;
}

int TLS13::init_connect()
{
    int ret = 0;
    do{
        TLS_CTX* ctx = gms_ctx;
        this->protocol = ctx->protocol;
        this->is_client = ctx->is_client;

        size_t i = 0;
        for (; i < ctx->cipher_suites_cnt; i++)
        {
            this->cipher_suites[i] =
                ctx->cipher_suites[i];
        }

        this->cipher_suites_cnt =
            ctx->cipher_suites_cnt;

        if (ctx->certslen >
            TLS_MAX_CERTIFICATES_SIZE)
        {
            error_print();
            ret = -1;
            break;
        }

        if (this->is_client)
        {
            memcpy(this->client_certs,
                ctx->certs, ctx->certslen);

            this->client_certs_len =
                ctx->certslen;
        }
        else
        {
            memcpy(this->server_certs,
                ctx->certs, ctx->certslen);

            this->server_certs_len =
                ctx->certslen;
        }

        if (ctx->cacertslen >
            TLS_MAX_CERTIFICATES_SIZE)
        {
            error_print();
            ret = -1;
            break;
        }
        memcpy(this->ca_certs,
            ctx->cacerts, ctx->cacertslen);

        this->ca_certs_len = ctx->cacertslen;
        this->sign_key = ctx->signkey;

    }while( 0 );

    return ret;
}

}
