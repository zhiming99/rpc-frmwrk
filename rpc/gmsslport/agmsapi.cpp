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


}
