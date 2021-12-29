#pragma once
#define BUF2STR( pBuf_, szBuf_ ) \
do{ \
    szBuf_[ sizeof( szBuf_ ) - 1 ] = 0; \
    gint32 iSize = std::min( \
        sizeof( szBuf_ ) - 1,  \
        ( size_t )pBuf_->size() ); \
    memcpy( szBuf_, pBuf_->ptr(), iSize ); \
    szBuf_[ iSize ] = 0; \
}while( 0 );

