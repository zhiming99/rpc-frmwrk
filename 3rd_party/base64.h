#pragma once

#include <string>

std::string base64_encode(unsigned char const* , unsigned int len);
std::string base64_decode(std::string const& s);

gint32 base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len, BufPtr& dest_buf );
gint32 base64_decode( const char* src_buf, unsigned int in_len, BufPtr& dest_buf );
