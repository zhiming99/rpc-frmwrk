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
    end = size;
    ptr = malloc( size );
}

int AGMS_IOVE::realloc( uint32_t newsize )
{
    if( newsize == 0 )
        return -EINVAL;

    if( newsize == mem_size )
        return newsize;

    int ret = newsize;
    void* newbuf = realloc( ptr, newsize );
    if( newbuf != nullptr )
    {
        ptr = newbuf;
        mem_size = size;
        end = newsize;
    }
    else
    {
        // keep the old pointer
        ret = -ENOMEM;
    }
    return ret;
}

int AGMS_IOVE::copy( 
    void* src, uint32_t src_size );
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
    if( this->start - this->end <= offset ||
        return -EINVAL;

    uint32_t newsize1 = offset
    uint32_t newsize2 = this->size() - offset;

    this->end = this->start + offset;
    
    bottom_half.clear();
    bottom_half.alloc( newsize2 );
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

    memcpy( ptr + end,
        ( bottom_half->ptr + start ),
        ( bottom_half->size() );

    this->mem_size = newsize;
    this->end += bottom_half->size();
    return 0;
}

int AGMS_IOVE::attach( void* ptr,
    uint32_t mem_size,
    uint32_t start = 0,
    uint32_t end = 0 )
{
    if( ptr == nullptr || mem_size == 0 )
        return -EINVAL;
    clear();
    return 0;
}

int AGMS_IOVE::detach( void** pptr,
    uint32_t& nmem_size,
    uint32_t& nstart,
    uint32_t& nend );
{
    *pptr = ptr;
    nmem_size = memsize;
    nstart = start;
    nend = end;
    ptr = nullptr;
    clear();
    return 0;
}

int BLKIO::read( PIOVE& iover )
{
}

int BLKIO::write( PIOVE& iovew )
{
}

// tls record count
int BLKIO::size()
{
    for( auto& elem : io_vec )
    {
    }
}

uint8_t* get_partial_rec( PIOVE& piove )
{
    PIOVE& last = piove;
    if( last->size() < TLS_RECORD_HEADER_SIZE )
        return piove->ptr;

    uint32_t total_bytes = 0;
    uint8_t* cur_rec = last->ptr;
    uint8_t* tail_ptr = last->ptr + last->end;

    bool exp_hdr = true;

    while( cur_rec < tail_ptr )
    {
        if( tail_ptr - cur_rec < TLS_RECORD_HEADER_SIZE )
        {
            exp_hdr = false;
            break;
        }
        uint32_t rec_len = tls_record_length( cur_rec );
        if( cur_rec + rec_len > tail_ptr )
        {
            exp_hdr = false;
            break;
        }
        cur_rec += rec_len;
    }

    if( cur_rec > tail_ptr )
        exp_hdr = false;

    if( !exp_hdr )
        return cur_rec;

    return nullptr;
}

int BLKIN::write( PIOVE& piove )
{
    if( ! piove )
        return -EINVAL;

    int ret = 0;
    uint8_t* record = ( uint8_t* )piove->ptr();
    bool exp_hdr = true;
    uint8_t* last_rec = nullptr;
    while( !io_vec.empty() )
    {
        PIOVE& last = io_vec.back();
        if( last->size() < TLS_RECORD_HEADER_SIZE )
        {
            ret = last->merge( piove );
            if( ret < 0 )
                break;
            exp_hdr = false;
        }
        else 
        {
            uint32_t rec_len = tls_record_length( last->ptr() );
            if( rec_len > last->size() )
            {
                ret = last->merge( piove );
                if( ret < 0 )
                    break;
                exp_hdr = false;
                break;
            }
            partial_rec = get_partial_rec( last );
            if( partial_rec == nullptr )
                break;

            exp_hdr = false;
            uint32_t offset = partial_rec -
                ( last->ptr + start );

            PIOVE ppartial( new AGMS_IOVE );

            ppartial->alloc(
                last->size() - offset );

            ret = last->split( ppartial, offset );
            if( ret < 0 )
                break;
            ret = ppartial->merge( piove );
            if( ret < 0 )
                break;
            io_vec.push_back( ppartial );
        }
        break;
    }

    if( ret < 0 )
        return ret;

    while( exp_hdr )
    {
        if( piove->size() < TLS_RECORD_HEADER_SIZE )
        {
            io_vec.push_back( piove );
            ret = EWOULDBLOCK;
            break;
        }
        if (!tls_record_type_name(tls_record_type(record)))
        {
            ret = -EPROTO;
            break;
        }
        if (!tls_protocol_name(tls_record_protocol(record)))
        {
            error_print();
            ret = -EPROTO;
            break;
        }
        if( tls_record_length( piove->ptr ) > piove->size() )
        {
            io_vec.push_back( piove );
            ret = EWOULDBLOCK;
            break;
        }
        io_vec.push_back( piove );
        break;
    }

    if( ret != 0 )
        return ret;

    return ret;
}

int BLKIN::read( PIOVE& piove )
{

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

int TLS13::handshake_cli()
{
	int ret = -1;
	uint8_t *record = conn->record;
	uint8_t *enced_record = conn->enced_record;
	size_t recordlen;

	size_t enced_recordlen;


	int type;
	const uint8_t *data;
	size_t datalen;

	int protocol;
	uint8_t client_random[32];
	uint8_t server_random[32];
	int cipher_suite;
	const uint8_t *random;
	const uint8_t *session_id;
	size_t session_id_len;

	int protocols[] = { TLS_protocol_tls13 };
	int supported_groups[] = { TLS_curve_sm2p256v1 };
	int sign_algors[] = { TLS_sig_sm2sig_sm3 };

	uint8_t client_exts[TLS_MAX_EXTENSIONS_SIZE];
	size_t client_exts_len;
	const uint8_t *server_exts;
	size_t server_exts_len;

	uint8_t sig[TLS_MAX_SIGNATURE_SIZE];
	size_t siglen = sizeof(sig);
	uint8_t verify_data[32];
	size_t verify_data_len;

	int server_sign_algor;
	const uint8_t *server_sig;
	size_t server_siglen;
	const uint8_t *server_verify_data;
	size_t server_verify_data_len;

	SM2_KEY client_ecdhe;
	SM2_POINT server_ecdhe_public;
	SM2_KEY server_sign_key;

	const DIGEST *digest = DIGEST_sm3();
	DIGEST_CTX dgst_ctx; // secret generation过程中需要ClientHello等数据输入的
	DIGEST_CTX null_dgst_ctx; // secret generation过程中不需要握手数据的
	const BLOCK_CIPHER *cipher = NULL;
	size_t padding_len;

	uint8_t zeros[32] = {0};
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


	const uint8_t *request_context;
	size_t request_context_len;
	const uint8_t *cert_request_exts;
	size_t cert_request_extslen;
	const uint8_t *cert_list;
	size_t cert_list_len;
	const uint8_t *cert;
	size_t certlen;


	conn->is_client = 1;
	tls_record_set_protocol(enced_record, TLS_protocol_tls12);

	digest_init(&dgst_ctx, digest);
	null_dgst_ctx = dgst_ctx;


	// send ClientHello
	tls_trace("send ClientHello\n");
	tls_record_set_protocol(record, TLS_protocol_tls1);
	rand_bytes(client_random, 32); // TLS 1.3 Random 不再包含 UNIX Time
	sm2_key_generate(&client_ecdhe);
	tls13_client_hello_exts_set(client_exts, &client_exts_len, sizeof(client_exts), &(client_ecdhe.public_key));
	tls_record_set_handshake_client_hello(record, &recordlen,
		TLS_protocol_tls12, client_random, NULL, 0,
		tls13_ciphers, sizeof(tls13_ciphers)/sizeof(tls13_ciphers[0]),
		client_exts, client_exts_len);
	tls13_record_trace(stderr, record, recordlen, 0, 0);
	if (tls_record_send(record, recordlen, conn->sock) != 1) {
		error_print();
		return -1;
	}
	// 此时尚未确定digest算法，因此无法digest_update


	// recv ServerHello
	tls_trace("recv ServerHello\n");
	if (tls_record_recv(enced_record, &enced_recordlen, conn->sock) != 1) {
		error_print();
		tls_send_alert(conn, TLS_alert_unexpected_message);
		return -1;
	}
	tls13_record_trace(stderr, enced_record, enced_recordlen, 0, 0);
	if (tls_record_get_handshake_server_hello(enced_record,
		&protocol, &random, &session_id, &session_id_len,
		&cipher_suite, &server_exts, &server_exts_len) != 1) {
		error_print();
		tls_send_alert(conn, TLS_alert_unexpected_message);
		return -1;
	}
	if (protocol != TLS_protocol_tls12) {
		error_print();
		tls_send_alert(conn, TLS_alert_protocol_version);
		return -1;
	}
	memcpy(server_random, random, 32);
	memcpy(conn->session_id, session_id, session_id_len);
	conn->session_id_len = session_id_len;
	if (tls_cipher_suite_in_list(cipher_suite,
		tls13_ciphers, sizeof(tls13_ciphers)/sizeof(tls13_ciphers[0])) != 1) {
		error_print();
		tls_send_alert(conn, TLS_alert_handshake_failure);
		return -1;
	}
	conn->cipher_suite = cipher_suite;
	if (tls13_server_hello_extensions_get(server_exts, server_exts_len, &server_ecdhe_public) != 1) {
		error_print();
		tls_send_alert(conn, TLS_alert_handshake_failure);
		return -1;
	}
	conn->protocol = TLS_protocol_tls13;

	tls13_cipher_suite_get(conn->cipher_suite, &digest, &cipher);
	digest_update(&dgst_ctx, record + 5, recordlen - 5);
	digest_update(&dgst_ctx, enced_record + 5, enced_recordlen - 5);


	printf("generate handshake secrets\n");
	/*
	generate handshake keys
		uint8_t client_write_key[32]
		uint8_t server_write_key[32]
		uint8_t client_write_iv[12]
		uint8_t server_write_iv[12]
	*/
	sm2_ecdh(&client_ecdhe, &server_ecdhe_public, &server_ecdhe_public);
	/* [1]  */ tls13_hkdf_extract(digest, zeros, psk, early_secret);
	/* [5]  */ tls13_derive_secret(early_secret, "derived", &null_dgst_ctx, handshake_secret);
	/* [6]  */ tls13_hkdf_extract(digest, handshake_secret, (uint8_t *)&server_ecdhe_public, handshake_secret);
	/* [7]  */ tls13_derive_secret(handshake_secret, "c hs traffic", &dgst_ctx, client_handshake_traffic_secret);
	/* [8]  */ tls13_derive_secret(handshake_secret, "s hs traffic", &dgst_ctx, server_handshake_traffic_secret);
	/* [9]  */ tls13_derive_secret(handshake_secret, "derived", &null_dgst_ctx, master_secret);
	/* [10] */ tls13_hkdf_extract(digest, master_secret, zeros, master_secret);
	//[sender]_write_key = HKDF-Expand-Label(Secret, "key", "", key_length)
	//[sender]_write_iv  = HKDF-Expand-Label(Secret, "iv", "", iv_length)
	//[sender] in {server, client}
	tls13_hkdf_expand_label(digest, server_handshake_traffic_secret, "key", NULL, 0, 16, server_write_key);
	tls13_hkdf_expand_label(digest, server_handshake_traffic_secret, "iv", NULL, 0, 12, conn->server_write_iv);
	block_cipher_set_encrypt_key(&conn->server_write_key, cipher, server_write_key);
	memset(conn->server_seq_num, 0, 8);
	tls13_hkdf_expand_label(digest, client_handshake_traffic_secret, "key", NULL, 0, 16, client_write_key);
	tls13_hkdf_expand_label(digest, client_handshake_traffic_secret, "iv", NULL, 0, 12, conn->client_write_iv);
	block_cipher_set_encrypt_key(&conn->client_write_key, cipher, client_write_key);
	memset(conn->client_seq_num, 0, 8);
	/*
	format_bytes(stderr, 0, 4, "client_write_key", client_write_key, 16);
	format_bytes(stderr, 0, 4, "server_write_key", server_write_key, 16);
	format_bytes(stderr, 0, 4, "client_write_iv", conn->client_write_iv, 12);
	format_bytes(stderr, 0, 4, "server_write_iv", conn->server_write_iv, 12);
	format_print(stderr, 0, 0, "\n");
	*/

	// recv {EncryptedExtensions}
	printf("recv {EncryptedExtensions}\n");
	if (tls_record_recv(enced_record, &enced_recordlen, conn->sock) != 1) {
		error_print();
		tls_send_alert(conn, TLS_alert_handshake_failure);
		return -1;
	}
	if (tls13_record_decrypt(&conn->server_write_key, conn->server_write_iv,
		conn->server_seq_num, enced_record, enced_recordlen,
		record, &recordlen) != 1) {
		error_print();
		tls_send_alert(conn, TLS_alert_bad_record_mac);
		return -1;
	}
	tls13_record_trace(stderr, record, recordlen, 0, 0);
	if (tls13_record_get_handshake_encrypted_extensions(record) != 1) {
		tls_send_alert(conn, TLS_alert_handshake_failure);
		error_print();
		return -1;
	}
	digest_update(&dgst_ctx, record + 5, recordlen - 5);
	tls_seq_num_incr(conn->server_seq_num);


	// recv {CertififcateRequest*} or {Certificate}
	if (tls_record_recv(enced_record, &enced_recordlen, conn->sock) != 1) {
		error_print();
		tls_send_alert(conn, TLS_alert_handshake_failure);
		return -1;
	}
	if (tls13_record_decrypt(&conn->server_write_key, conn->server_write_iv,
		conn->server_seq_num, enced_record, enced_recordlen,
		record, &recordlen) != 1) {
		error_print();
		tls_send_alert(conn, TLS_alert_bad_record_mac);
		return -1;
	}
	if (tls_record_get_handshake(record, &type, &data, &datalen) != 1) {
		error_print();
		tls_send_alert(conn, TLS_alert_handshake_failure);
		return -1;
	}
	if (type == TLS_handshake_certificate_request) {
		tls_trace("recv {CertificateRequest*}\n");
		tls13_record_trace(stderr, record, recordlen, 0, 0);
		if (tls13_record_get_handshake_certificate_request(record,
			&request_context, &request_context_len,
			&cert_request_exts, &cert_request_extslen) != 1) {
			error_print();
			tls_send_alert(conn, TLS_alert_handshake_failure);
			return -1;
		}
		// 当前忽略 request_context 和 cert_request_exts
		// request_context 应该为空，当前实现中不支持Post-Handshake Auth
		digest_update(&dgst_ctx, record + 5, recordlen - 5);
		tls_seq_num_incr(conn->server_seq_num);


		// recv {Certificate}
		if (tls_record_recv(enced_record, &enced_recordlen, conn->sock) != 1) {
			error_print();
			tls_send_alert(conn, TLS_alert_handshake_failure);
			return -1;
		}
		if (tls13_record_decrypt(&conn->server_write_key, conn->server_write_iv,
			conn->server_seq_num, enced_record, enced_recordlen,
			record, &recordlen) != 1) {
			error_print();
			tls_send_alert(conn, TLS_alert_bad_record_mac);
			return -1;
		}
	} else {
		conn->client_certs_len = 0;
		// 清空客户端签名密钥
	}

	// recv {Certificate}
	tls_trace("recv {Certificate}\n");
	tls13_record_trace(stderr, record, recordlen, 0, 0);
	if (tls13_record_get_handshake_certificate(record,
		&request_context, &request_context_len,
		&cert_list, &cert_list_len) != 1) {
		error_print();
		tls_send_alert(conn, TLS_alert_unexpected_message);
		return -1;
	}
	if (tls13_process_certificate_list(cert_list, cert_list_len, conn->server_certs, &conn->server_certs_len) != 1) {
		error_print();
		tls_send_alert(conn, TLS_alert_unexpected_message);
		return -1;
	}
	if (x509_certs_get_cert_by_index(conn->server_certs, conn->server_certs_len, 0, &cert, &certlen) != 1
		|| x509_cert_get_subject_public_key(cert, certlen, &server_sign_key) != 1) {
		error_print();
		tls_send_alert(conn, TLS_alert_unexpected_message);
		return -1;
	}
	digest_update(&dgst_ctx, record + 5, recordlen - 5);
	tls_seq_num_incr(conn->server_seq_num);


	// recv {CertificateVerify}
	tls_trace("recv {CertificateVerify}\n");
	if (tls_record_recv(enced_record, &enced_recordlen, conn->sock) != 1) {
		error_print();
		tls_send_alert(conn, TLS_alert_unexpected_message);
		return -1;
	}
	if (tls13_record_decrypt(&conn->server_write_key, conn->server_write_iv,
		conn->server_seq_num, enced_record, enced_recordlen,
		record, &recordlen) != 1) {
		error_print();
		tls_send_alert(conn, TLS_alert_bad_record_mac);
		return -1;
	}
	tls13_record_trace(stderr, record, recordlen, 0, 0);
	if (tls13_record_get_handshake_certificate_verify(record,
		&server_sign_algor, &server_sig, &server_siglen) != 1) {
		error_print();
		tls_send_alert(conn, TLS_alert_unexpected_message);
		return -1;
	}
	if (server_sign_algor != TLS_sig_sm2sig_sm3) {
		error_print();
		tls_send_alert(conn, TLS_alert_unexpected_message);
		return -1;
	}
	if (tls13_verify_certificate_verify(TLS_server_mode, &server_sign_key, TLS13_SM2_ID, TLS13_SM2_ID_LENGTH, &dgst_ctx, server_sig, server_siglen) != 1) {
		error_print();
		return -1;
	}
	digest_update(&dgst_ctx, record + 5, recordlen - 5);
	tls_seq_num_incr(conn->server_seq_num);


	// use Transcript-Hash(Handshake Context, Certificate*, CertificateVerify*)
	tls13_compute_verify_data(server_handshake_traffic_secret,
		&dgst_ctx, verify_data, &verify_data_len);


	// recv {Finished}
	tls_trace("recv {Finished}\n");
	if (tls_record_recv(enced_record, &enced_recordlen, conn->sock) != 1) {
		error_print();
		tls_send_alert(conn, TLS_alert_unexpected_message);
		return -1;
	}
	if (tls13_record_decrypt(&conn->server_write_key, conn->server_write_iv,
		conn->server_seq_num, enced_record, enced_recordlen,
		record, &recordlen) != 1) {
		error_print();
		tls_send_alert(conn, TLS_alert_bad_record_mac);
		return -1;
	}
	tls13_record_trace(stderr, record, recordlen, 0, 0);
	if (tls13_record_get_handshake_finished(record,
		&server_verify_data, &server_verify_data_len) != 1) {
		error_print();
		tls_send_alert(conn, TLS_alert_unexpected_message);
		return -1;
	}
	if (server_verify_data_len != verify_data_len
		|| memcmp(server_verify_data, verify_data, verify_data_len) != 0) {
		error_print();
		tls_send_alert(conn, TLS_alert_unexpected_message);
		return -1;
	}
	digest_update(&dgst_ctx, record + 5, recordlen - 5);
	tls_seq_num_incr(conn->server_seq_num);


	// generate server_application_traffic_secret
	/* [12] */ tls13_derive_secret(master_secret, "s ap traffic", &dgst_ctx, server_application_traffic_secret);
	// generate client_application_traffic_secret
	/* [11] */ tls13_derive_secret(master_secret, "c ap traffic", &dgst_ctx, client_application_traffic_secret);


	if (conn->client_certs_len) {
		int client_sign_algor;
		uint8_t sig[TLS_MAX_SIGNATURE_SIZE];
		size_t siglen;

		// send client {Certificate*}
		tls_trace("send {Certificate*}\n");
		if (tls13_record_set_handshake_certificate(record, &recordlen,
			NULL, 0, // certificate_request_context
			conn->client_certs, conn->client_certs_len) != 1) {
			error_print();
			tls_send_alert(conn, TLS_alert_internal_error);
			goto end;
		}
		tls13_record_trace(stderr, record, recordlen, 0, 0);
		tls13_padding_len_rand(&padding_len);
		if (tls13_record_encrypt(&conn->client_write_key, conn->client_write_iv,
			conn->client_seq_num, record, recordlen, padding_len,
			enced_record, &enced_recordlen) != 1) {
			error_print();
			tls_send_alert(conn, TLS_alert_internal_error);
			return -1;
		}
		if (tls_record_send(enced_record, enced_recordlen, conn->sock) != 1) {
			error_print();
			tls_send_alert(conn, TLS_alert_internal_error);
			return -1;
		}
		digest_update(&dgst_ctx, record + 5, recordlen - 5);
		tls_seq_num_incr(conn->client_seq_num);


		// send {CertificateVerify*}
		tls_trace("send {CertificateVerify*}\n");
		client_sign_algor = TLS_sig_sm2sig_sm3; // FIXME: 应该放在conn里面
		tls13_sign_certificate_verify(TLS_client_mode, &conn->sign_key, TLS13_SM2_ID, TLS13_SM2_ID_LENGTH, &dgst_ctx, sig, &siglen);
		if (tls13_record_set_handshake_certificate_verify(record, &recordlen,
			client_sign_algor, sig, siglen) != 1) {
			error_print();
			tls_send_alert(conn, TLS_alert_internal_error);
			return -1;
		}
		tls13_record_trace(stderr, record, recordlen, 0, 0);
		tls13_padding_len_rand(&padding_len);
		if (tls13_record_encrypt(&conn->client_write_key, conn->client_write_iv,
			conn->client_seq_num, record, recordlen, padding_len,
			enced_record, &enced_recordlen) != 1) {
			error_print();
			tls_send_alert(conn, TLS_alert_internal_error);
			return -1;
		}
		if (tls_record_send(enced_record, enced_recordlen, conn->sock) != 1) {
			error_print();
			return -1;
		}
		digest_update(&dgst_ctx, record + 5, recordlen - 5);
		tls_seq_num_incr(conn->client_seq_num);
	}

	// send Client {Finished}
	tls_trace("send {Finished}\n");
	tls13_compute_verify_data(client_handshake_traffic_secret, &dgst_ctx, verify_data, &verify_data_len);
	if (tls_record_set_handshake_finished(record, &recordlen, verify_data, verify_data_len) != 1) {
		error_print();
		tls_send_alert(conn, TLS_alert_internal_error);
		goto end;
	}
	tls13_record_trace(stderr, record, recordlen, 0, 0);
	tls13_padding_len_rand(&padding_len);
	if (tls13_record_encrypt(&conn->client_write_key, conn->client_write_iv,
		conn->client_seq_num, record, recordlen, padding_len,
		enced_record, &enced_recordlen) != 1) {
		error_print();
		tls_send_alert(conn, TLS_alert_internal_error);
		goto end;
	}
	if (tls_record_send(enced_record, enced_recordlen, conn->sock) != 1) {
		error_print();
		goto end;
	}
	digest_update(&dgst_ctx, record + 5, recordlen - 5);
	tls_seq_num_incr(conn->client_seq_num);



	// update server_write_key, server_write_iv, reset server_seq_num
	tls13_hkdf_expand_label(digest, server_application_traffic_secret, "key", NULL, 0, 16, server_write_key);
	block_cipher_set_encrypt_key(&conn->server_write_key, cipher, server_write_key);
	tls13_hkdf_expand_label(digest, server_application_traffic_secret, "iv", NULL, 0, 12, conn->server_write_iv);
	memset(conn->server_seq_num, 0, 8);
	/*
	format_print(stderr, 0, 0, "update server secrets\n");
	format_bytes(stderr, 0, 4, "server_write_key", server_write_key, 16);
	format_bytes(stderr, 0, 4, "server_write_iv", conn->server_write_iv, 12);
	format_print(stderr, 0, 0, "\n");
	*/

	//update client_write_key, client_write_iv, reset client_seq_num
	tls13_hkdf_expand_label(digest, client_application_traffic_secret, "key", NULL, 0, 16, client_write_key);
	tls13_hkdf_expand_label(digest, client_application_traffic_secret, "iv", NULL, 0, 12, conn->client_write_iv);
	block_cipher_set_encrypt_key(&conn->client_write_key, cipher, client_write_key);
	memset(conn->client_seq_num, 0, 8);

	/*
	format_print(stderr, 0, 0, "update client secrets\n");
	format_bytes(stderr, 0, 4, "client_write_key", client_write_key, 16);
	format_bytes(stderr, 0, 4, "client_write_iv", conn->client_write_iv, 12);
	format_print(stderr, 0, 0, "\n");
	*/
	fprintf(stderr, "Connection established\n");
	ret = 1;

end:
	gmssl_secure_clear(&client_ecdhe, sizeof(client_ecdhe));
	gmssl_secure_clear(&server_sign_key, sizeof(server_sign_key));
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
	return ret;
}

}
