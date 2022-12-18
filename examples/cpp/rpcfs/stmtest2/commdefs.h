#pragma once
#define BUF2STR( pBuf_ ) \
({ \
    stdstr strBuf_( pBuf_->ptr(), pBuf_->size() );\
    strBuf_;\
})
