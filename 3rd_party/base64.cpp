/* 
   base64.cpp and base64.h

   Copyright (C) 2004-2008 René Nyffenegger

   This source code is provided 'as-is', without any express or implied
   warranty. In no event will the author be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this source code must not be misrepresented; you must not
      claim that you wrote the original source code. If you use this source code
      in a product, an acknowledgment in the product documentation would be
      appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
      misrepresented as being the original source code.

   3. This notice may not be removed or altered from any source distribution.

   René Nyffenegger rene.nyffenegger@adp-gmbh.ch

*/

/* 
 *
 *  Tue 31 Dec 2019 11:48:43 AM Beijing
 *  NOTE: Modified for `rpc-frmwrk' by woodhead99@gmail.com
*/

#include "rpc.h"
#include <iostream>

using namespace rpcf;
#include "base64.h"

static const std::string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";


static inline bool is_base64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

guint32 base64_enc_len( guint32 buf_len ) {
    return ( ( buf_len + 2 ) / 3 ) * 4;
}

// make sure padding is pointing to a 4 bytes
// string
guint32 base64_dec_len( guint32 buf_len, const char* padding ) {
    if( padding == nullptr )
        return 0;

    gint32 padding_num = 0;
    if( padding[ 3 ] == '=' )
    {
        ++padding_num;
        if( padding[ 2 ] == '=' ) 
            ++padding_num;
    }
    return ( ( buf_len >> 2 ) * 3 - padding_num );
}

gint32 base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len, BufPtr& dest_buf ) {
  char* buf = nullptr;
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  if( bytes_to_encode == 0 )
      return 0;

  guint32 dest_bytes = base64_enc_len( in_len );

  if( dest_buf.IsEmpty() )
  {
      gint32 ret = dest_buf.NewObj();
      if( ERROR( ret ) )
          return ret;
  }

  guint32 dwOldSize = dest_buf->size();
  dest_buf->Resize( dwOldSize + dest_bytes );
  buf = dest_buf->ptr() + dwOldSize; 

  while (in_len--) {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for(i = 0; (i <4) ; i++)
        *buf++ = base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i)
  {
    for(j = i; j < 3; j++)
      char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++)
      *buf++ = base64_chars[char_array_4[j]];

    while((i++ < 3))
      *buf++ = '=';
  }

  if( buf != dest_buf->ptr() + dest_buf->size() )
      return -ERANGE;
  
  return dest_bytes;
}

std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len )
{
    BufPtr dest_buf( true );
    gint32 ret = base64_encode( bytes_to_encode, in_len, dest_buf );
    if( ERROR( ret ) )
        return std::string( "" );

    BufPtr str_buf( true );
    str_buf->Resize( dest_buf->size() + 1 );
    memcpy( str_buf->ptr(), dest_buf->ptr(), dest_buf->size() );
    str_buf->ptr()[ dest_buf->size() ] = 0;
    return std::string( str_buf->ptr() );
}

gint32 base64_decode( const char* src_buf, unsigned int in_len, BufPtr& dest_buf ) {

  if( src_buf == nullptr || in_len < 4 )
      return -EINVAL;

  if( ( in_len & 3 ) != 0 )
      return -EINVAL;

  int i = 0;
  int j = 0;
  int in_ = 0;
  unsigned char char_array_4[4], char_array_3[3];
  const char* padding = src_buf + in_len;
  padding -= 4;

  guint32 dest_bytes = base64_dec_len( in_len, padding );
  if( dest_bytes == 0 )
      return -EINVAL;

  gint32 ret = 0;
  if( dest_buf.IsEmpty() )
  {
      ret = dest_buf.NewObj();
      if( ERROR( ret ) )
          return ret;
  }
  guint32 dwOldSize = dest_buf->size();
  ret = dest_buf->Resize( dwOldSize + dest_bytes );
  if( ERROR( ret ) )
    return ret;

  char* buf = dest_buf->ptr() + dwOldSize;

  while (in_len-- && ( src_buf[in_] != '=') && is_base64(src_buf[in_])) {
    char_array_4[i++] = src_buf[in_]; in_++;
    if (i ==4) {
      for (i = 0; i <4; i++)
        char_array_4[i] = static_cast<unsigned char>(base64_chars.find(char_array_4[i]));

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++)
        *buf++ = char_array_3[i];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j <4; j++)
      char_array_4[j] = 0;

    for (j = 0; j <4; j++)
      char_array_4[j] = static_cast<unsigned char>(base64_chars.find(char_array_4[j]));

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; (j < i - 1); j++) *buf++ = char_array_3[j];
  }

  if( buf != dest_buf->ptr() + dest_buf->size() )
      return -ERANGE;

  return dest_bytes;
}

std::string base64_decode(std::string const& s)
{
    BufPtr dest_buf( true );
    gint32 ret = base64_decode( s.c_str(), s.size(), dest_buf );
    if( ERROR( ret ) )
        return std::string( "" );

    BufPtr str_buf( true );
    str_buf->Resize( dest_buf->size() + 1 );
    memcpy( str_buf->ptr(), dest_buf->ptr(), dest_buf->size() );
    str_buf->ptr()[ dest_buf->size() ] = 0;
    return std::string( str_buf->ptr() );
}
