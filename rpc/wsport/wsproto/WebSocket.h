// WebSocket, v1.00 2012-09-13
//
// Description: WebSocket RFC6544 codec, written in C++.
// Homepage: http://katzarsky.github.com/WebSocket
// Author: katzarsky@gmail.com

#ifndef WEBSOCKET_H
#define	WEBSOCKET_H

#include <assert.h>
#include <stdint.h> /* uint8_t */
#include <stdio.h> /* sscanf */
#include <ctype.h> /* isdigit */
#include <stddef.h> /* int */

// std c++
#include <vector> 
#include <string> 

using namespace std;

enum WebSocketFrameType {
	ERROR_FRAME=0xFF00,
	INCOMPLETE_FRAME=0xFE00,

	OPENING_FRAME=0x3300,
	CLOSING_FRAME=0x3400,

	INCOMPLETE_TEXT_FRAME=0x01,
	INCOMPLETE_BINARY_FRAME=0x02,

	TEXT_FRAME=0x81,
	BINARY_FRAME=0x82,

	PING_FRAME=0x19,
	PONG_FRAME=0x1A,
    CLOSE_FRAME=0x1B,
};

class WebSocket
{
	public:

	string resource;
	string host;
	string origin;
	string protocol;
	string key;

	WebSocket();

	/**
	 * @param input_frame .in. pointer to input frame
	 * @param input_len .in. length of input frame
	 * @return [WS_INCOMPLETE_FRAME, WS_ERROR_FRAME, WS_OPENING_FRAME]
	 */
	WebSocketFrameType parseHandshake(unsigned char* input_frame, int input_len, int& header_size);
    WebSocketFrameType checkHandshakeResp(unsigned char* input_frame, int input_len, int& header_size);
	string answerHandshake();
	int makeHandshake( guint64 sec_key, const char* url, string& strRet );

	int makeFrame(WebSocketFrameType frame_type, unsigned char* msg, int msg_len, BufPtr& dest_buf);
	WebSocketFrameType getFrame(BufPtr& src_buf, BufPtr& dest_buf );

	void trim(string& str);
	int explode(
        const string& theString,
        const string& theDelimiter,
        vector< string >& vecStrings );
};

#endif	/* WEBSOCKET_H */
