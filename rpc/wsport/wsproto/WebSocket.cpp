// WebSocket, v1.00 2012-09-13
//
// Description: WebSocket RFC6544 codec, written in C++.
// Homepage: http://katzarsky.github.com/WebSocket
// Author: katzarsky@gmail.com
//
// NOTE: Modified for `rpc-frmwrk' by
// woodhead99@gmail.com

#include "rpc.h"
#include "WebSocket.h"

#include "base64.h"
#include "sha1.h"

#include <iostream>
#include <string>
#include <vector>

#define RFC6544_MAGIC_KEY "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"; 

using namespace std;

WebSocket::WebSocket() :
    protocol( "rpc-frmwrk" )
{

}

WebSocketFrameType WebSocket::parseHandshake(unsigned char* input_frame, int input_len, int& header_size )
{
	// 1. copy char*/len into string
	// 2. try to parse headers until \r\n occurs
	string headers((char*)input_frame, input_len); 
	size_t header_end = headers.find("\r\n\r\n");

	if(header_end == string::npos) { // end-of-headers not found - do not parse
		return INCOMPLETE_FRAME;
	}

    bool bValid = false;
	headers.resize(header_end); // trim off any data we don't need after the headers
	vector<string> headers_rows;
    explode(headers, string("\r\n"), headers_rows );
	for(size_t i=0; i<headers_rows.size(); i++) {
		string& header = headers_rows[i];
		if(header.find("GET") == 0) {
			vector<string> get_tokens;
            explode(header, string(" "), get_tokens);
			if(get_tokens.size() >= 2) {
				this->resource = get_tokens[1];
			}
		}
		else {
			size_t pos = header.find(":");
			if(pos != string::npos) {
				string header_key(header, 0, pos);
				string header_value(header, pos+1);
				trim(header_value);
				if(header_key == "Host") this->host = header_value;
				else if(header_key == "Origin") this->origin = header_value, bValid = true;
				else if(header_key == "Sec-WebSocket-Key") this->key = header_value, bValid = true;
				else if(header_key == "Sec-WebSocket-Protocol") this->protocol = header_value, bValid = true;
			}
		}
	}

    if( !bValid )
        return ERROR_FRAME;

    header_size = header_end + 4;
	return OPENING_FRAME;
}

void WebSocket::trim(string& str) 
{
	//printf("TRIM\n");
	const char* whitespace = " \t\r\n";
	string::size_type pos = str.find_last_not_of(whitespace);
	if(pos != string::npos) {
		str.erase(pos + 1);
		pos = str.find_first_not_of(whitespace);
		if(pos != string::npos) str.erase(0, pos);
	}
	return;
}

int WebSocket::explode(	
	const string&  theString,
    const string&  theDelimiter,
    vector< string >& vecStrings )
{
    //printf("EXPLODE\n");
    //UASSERT( theDelimiter.size(), >, 0 );

    if( theString.empty() || theDelimiter.empty() )
        return -EINVAL;

    size_t start = 0, end = 0, length = 0;
    size_t str_size = theString.size();
    do{
        end = theString.find( theDelimiter, start );
        if( end == string::npos )
        {
            length = str_size - start;
        }
        else
        {
            length = end - start;
        }

        if( length > 0 )
        {
            vecStrings.push_back(
                theString.substr( start, length ) );
        }

        if( end == string::npos )
            break;

        start = end + theDelimiter.size();

    }while( start < str_size );

    return 0;
}

gint32 WebSocket::makeHandshake(
    guint64 sec_key,
    const char* url,
    string& strCliReq )
{
    if( sec_key == 0 )
        return -EINVAL;

    key = base64_encode( ( guint8* )&sec_key, sizeof( sec_key ) );
    if( key.empty() )
        return -EINVAL;

    gint32 ret = 0;

    do{
        vector< string > vec_nodes;
        string strUrl = url;

        ret = CRegistry::Namei(
            strUrl, vec_nodes );

        if( ERROR( ret ) )
            break;

        if( vec_nodes.size() < 3 )
        {
            ret = -EINVAL;
            break;
        }

        if( strcasecmp( "http:",
            vec_nodes[ 0 ].c_str() ) != 0 )
        {
            if( strcasecmp( "https:",
                vec_nodes[ 0 ].c_str() ) != 0 )
            {
                ret = -EINVAL;
                break;
            }
        }

        string strDomain = vec_nodes[ 0 ] + "//";
        strDomain += vec_nodes[ 1 ];

        string strPath = vec_nodes[ 2 ];

        strCliReq = "GET ";
        strCliReq += strPath + " HTTP/1.1\r\n";
        strCliReq += "Host: server.example.com\r\n";
        strCliReq += "Upgrade: websocket\r\n";
        strCliReq += "Connection: Upgrade\r\n";
        strCliReq += "Sec-WebSocket-Key: " + key + "\r\n";
        strCliReq += "Sec-WebSocket-Protocol: " + this->protocol + "\r\n";
        strCliReq += "Sec-WebSocket-Version: 13\r\n";
        // origin is not necessary yet
        strCliReq += "Origin: ";
        strCliReq += strDomain + "\r\n";
        strCliReq += "\r\n";

    }while( 0 );

    return ret;
}

WebSocketFrameType WebSocket::checkHandshakeResp(unsigned char* input_frame, int input_len, int& header_size)
{
	// 1. copy char*/len into string
	// 2. try to parse headers until \r\n occurs
	string headers((char*)input_frame, input_len);
	int header_end = headers.find("\r\n\r\n");
    WebSocketFrameType ret = ERROR_FRAME;

	if(header_end == ( int )string::npos) { // end-of-headers not found - do not parse
		return INCOMPLETE_FRAME;
	}

	headers.resize(header_end); // trim off any data we don't need after the headers
	vector<string> headers_rows;
    explode(headers, string("\r\n"), headers_rows);
	for(int i=0; i<( int )headers_rows.size(); i++) {
		string& header = headers_rows[i];
        int pos = ( int )header.find(":");
        if(pos != ( int )string::npos) {
            string header_key(header, 0, pos);
            if(header_key != "Sec-WebSocket-Accept")
                continue;
            string header_value(header, pos+1);
            trim(header_value);
            string resp_key = header_value;
            string local_key = key + RFC6544_MAGIC_KEY;

            //160bit = 20 bytes/chars
            unsigned char digest[ 20 ];
            SHA1 sha;
            sha.Input(local_key.c_str(), local_key.size());
            sha.Result((unsigned*)digest);

            for(int i=0; i<( int )sizeof( digest ); i+=4) {
                unsigned char c;

                c = digest[i];
                digest[i] = digest[i+3];
                digest[i+3] = c;

                c = digest[i+1];
                digest[i+1] = digest[i+2];
                digest[i+2] = c;
            }

            local_key = base64_encode(digest, sizeof(digest)); 
            if( local_key != resp_key )
                ret = ERROR_FRAME;
            else
            {
                ret = OPENING_FRAME;
                header_size = header_end + 4;
            }
            break;
        }
	}
	return ret;
}

string WebSocket::answerHandshake()
{
    unsigned char digest[20]; // 160 bit sha1 digest

	string answer;
	answer += "HTTP/1.1 101 Switching Protocols\r\n";
	answer += "Upgrade: WebSocket\r\n";
	answer += "Connection: Upgrade\r\n";
	if(this->key.length() > 0) {
		string accept_key;
		accept_key += this->key;
		accept_key += RFC6544_MAGIC_KEY;

		//printf("INTERMEDIATE_KEY:(%s)\n", accept_key.data());

		SHA1 sha;
		sha.Input(accept_key.data(), accept_key.size());
		sha.Result((unsigned*)digest);
		
		//printf("DIGEST:"); for(int i=0; i<20; i++) printf("%02x ",digest[i]); printf("\n");

		//little endian to big endian
		for(int i=0; i<20; i+=4) {
			unsigned char c;

			c = digest[i];
			digest[i] = digest[i+3];
			digest[i+3] = c;

			c = digest[i+1];
			digest[i+1] = digest[i+2];
			digest[i+2] = c;
		}

		//printf("DIGEST:"); for(int i=0; i<20; i++) printf("%02x ",digest[i]); printf("\n");

		accept_key = base64_encode((const unsigned char *)digest, 20); //160bit = 20 bytes/chars

		answer += "Sec-WebSocket-Accept: "+(accept_key)+"\r\n";
	}
	if(this->protocol.length() > 0) {
		answer += "Sec-WebSocket-Protocol: "+(this->protocol)+"\r\n";
	}
	answer += "\r\n";
	return answer;

	//return WS_OPENING_FRAME;
}

int WebSocket::makeFrame(WebSocketFrameType frame_type, unsigned char* msg, int msg_length, BufPtr& dest_buf )
{
    unsigned char* buffer = nullptr;
    int buffer_len = 0;
	int pos = 0;
	int size = msg_length; 

    if( size < 126 )
        buffer_len = 1 + 1 + size;
    else if( size < 65536 )
        buffer_len = 1 + 1 + 2 + size;
    else
        buffer_len = 1 + 1 + 8 + size;

    size_t iOffset = dest_buf->size();
    dest_buf->Resize( iOffset + buffer_len );
    if( dest_buf->size() < ( guint32 )iOffset + buffer_len )
        return -ENOMEM;

    buffer = iOffset + ( guint8* )dest_buf->ptr();
	buffer[pos++] = (unsigned char)frame_type; // text frame

	if(size <= 125) {
		buffer[pos++] = size;
	}
	else if(size <= 65535) {
		buffer[pos++] = 126; //16 bit length follows
		
		buffer[pos++] = (size >> 8) & 0xFF; // leftmost first
		buffer[pos++] = size & 0xFF;
	}
	else { // >2^16-1 (65535)
		buffer[pos++] = 127; //64 bit length follows
		
		// write 8 bytes length (significant first)
		
		// since msg_length is int it can be no longer than 4 bytes = 2^32-1
		// padd zeroes for the first 4 bytes
		for(int i=3; i>=0; i--) {
			buffer[pos++] = 0;
		}
		// write the actual 32bit msg_length in the next 4 bytes
		for(int i=3; i>=0; i--) {
			buffer[pos++] = ((size >> 8*i) & 0xFF);
		}
	}
	memcpy((void*)(buffer+pos), msg, size);
	return (size+pos);
}

WebSocketFrameType WebSocket::getFrame(
    BufPtr& src_buf, BufPtr& dest_buf )
{
    if( src_buf.IsEmpty() || dest_buf.IsEmpty() )
        return ERROR_FRAME;

    size_t frame_size = 0;
    guint8* in_buffer = ( guint8* )src_buf->ptr();
    size_t in_length = src_buf->size();

    if(in_length < 3) return INCOMPLETE_FRAME;

	unsigned char msg_opcode = in_buffer[0] & 0x0F;
	unsigned char msg_fin = (in_buffer[0] >> 7) & 0x01;
	unsigned char msg_masked = (in_buffer[1] >> 7) & 0x01;

	// *** message decoding 

	size_t payload_length = 0;
	size_t pos = 2;
	int length_field = in_buffer[1] & (~0x80);
	unsigned int mask = 0;

	//printf("IN:"); for(int i=0; i<20; i++) printf("%02x ",buffer[i]); printf("\n");

	if(length_field <= 125) {
		payload_length = length_field;
	}
	else if(length_field == 126) { //msglen is 16bit!
		//payload_length = in_buffer[2] + (in_buffer[3]<<8);
		payload_length = (
			(in_buffer[2] << 8) | 
			(in_buffer[3])
		);
		pos += 2;
	}
	else if(length_field == 127) { //msglen is 64bit!
		payload_length = (
			// (in_buffer[2] << 56) | 
			// (in_buffer[3] << 48) | 
			// (in_buffer[4] << 40) | 
			// (in_buffer[5] << 32) | 
			(in_buffer[6] << 24) | 
			(in_buffer[7] << 16) | 
			(in_buffer[8] << 8) | 
			(in_buffer[9])
		); 
		pos += 8;
	}
		
	//printf("PAYLOAD_LEN: %08x\n", payload_length);
	if(in_length < payload_length+pos) {
		return INCOMPLETE_FRAME;
	}

	if(msg_masked) {
		mask = *((unsigned int*)(in_buffer+pos));
		//printf("MASK: %08x\n", mask);
		pos += 4;

		// unmask data:
		unsigned char* c = in_buffer+pos;
		for(size_t i=0; i<payload_length; i++) {
			c[i] = c[i] ^ ((unsigned char*)(&mask))[i%4];
		}
	}
	
	if(payload_length > 1024 * 1024 ){
        return ERROR_FRAME;
	}

    gint32 ret = 0;
    frame_size = pos + payload_length;
    if( in_length == frame_size &&
        dest_buf->empty() )
    {
        // transfer the buffer from src_buf to
        // dest_buf without copying
        ret = src_buf->IncOffset( pos );
        if( ERROR( ret ) )
            return ERROR_FRAME;

        dest_buf = src_buf;
        src_buf.Clear();
        // make sure src_buf is not empty
        src_buf.NewObj();
    }
    else
    {
        ret = dest_buf->Append(
            in_buffer + pos, payload_length );

        if( ERROR( ret ) )
            return ERROR_FRAME;

        ret = src_buf->IncOffset( frame_size );
        if( ERROR( ret ) )
            return ERROR_FRAME;
    }
	
	if(msg_opcode == 0x0) return (msg_fin)?TEXT_FRAME:INCOMPLETE_TEXT_FRAME; // continuation frame ?
	if(msg_opcode == 0x1) return (msg_fin)?TEXT_FRAME:INCOMPLETE_TEXT_FRAME;
	if(msg_opcode == 0x2) return (msg_fin)?BINARY_FRAME:INCOMPLETE_BINARY_FRAME;
	if(msg_opcode == 0x9) return PING_FRAME;
	if(msg_opcode == 0xA) return PONG_FRAME;
	if(msg_opcode == 0x8) return CLOSE_FRAME;

	return ERROR_FRAME;
}
