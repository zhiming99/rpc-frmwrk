/*
 * =====================================================================================
 *
 *       Filename:  fusedefs.h
 *
 *    Description:  public defines for fuse rpc development
 *
 *        Version:  1.0
 *        Created:  04/04/2022 07:42:52 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2022 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */

#pragma once

#define CONN_PARAM_FILE "conn_params"
#define HOP_DIR  "_nexthop"
#define STREAM_DIR "_streams"
#define JSON_REQ_FILE "json_request"
#define JSON_RESP_FILE "json_response"
#define JSON_EVT_FILE "json_event"
#define JSON_STMEVT_FILE "json_stmevt"
#define CONN_DIR_PREFIX "connection_"

#define MAX_STREAMS_PER_SVC 1024

// don't modify these values
#define MAX_STM_QUE_SIZE   32     // STM_MAX_PACKETS_REPORT
#define MAX_REQ_QUE_SIZE   32
#define MAX_EVT_QUE_SIZE   32

// max bytes per request/response
#define MAX_FUSE_MSG_SIZE  ( 1024 * 1024 ) // MAX_BYTES_PER_TRANSFER

// max bytes per stream read/write
#define MAX_FUSE_STM_PACKET ( MAX_FUSE_MSG_SIZE * 16 )

