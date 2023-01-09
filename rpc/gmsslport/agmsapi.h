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
#include <stdint.h>
namespace rpcf
{
    using PIOVE=std::unique_ptr< AGMS_IOVE > ;
    using IOV = std::deque< PIOVE >;

    struct BLKIO
    {
        IOV io_vec;
        int read ( PIOVE& iover );
        int write( PIOVE& iovew );
        int put_back( PIOVE& iover );
    };

    enum AGMS_STATE : uint32_t
    {
        STAT_INIT,
        STAT_START_HANDSHAKE,
        STAT_READY,
        STAT_HANDSHAKE_FAILED,
        STAT_SHUTDOWN
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
        void* ptr;
        uint32_t mem_size;
        uint32_t start;
        uint32_t end;
        ~AGMS_IOVE()
        {
            if( ptr ) free( ptr );
            ptr = nullptr;
        }

        int attach( void* ptr,
            uint32_t mem_size,
            uint32_t start = 0,
            uint32_t end = 0 );

        int detach( void** pptr,
            uint32_t& mem_size,
            uint32_t& start,
            uint32_t& end );

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
}
