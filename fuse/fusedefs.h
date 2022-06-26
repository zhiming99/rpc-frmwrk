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

// Json request/response must have a string attribute
// 'interface' and a string attributes 'method' to
// specify which interface and which method the
// parameters are for.
#define JSON_ATTR_IFNAME        "Interface"
#define JSON_ATTR_METHOD        "Method"

// A json message must have a string attribute MsgType
// and the value wiill be one of 'req', 'resp' or
// 'evt'
#define JSON_ATTR_MSGTYPE       "MessageType"

// A json message can have an object attribute 'params'
#define JSON_ATTR_PARAMS        "Parameters"

// Json representation of a struct must have a UInt
// attribute 'structId', as mapped to the internal
// msgid
#define JSON_ATTR_STRUCTID      "StructId"

// Each json request/response/event message has an
// UInt64 attribute reqctxid, as a unique identifier of
// the request. Particularly, the response's identifer
// must be the same as the one from the associated
// request.
#define JSON_ATTR_REQCTXID      "RequestId"

// Each json response msg must have an attribute
// ReturnCode with a negative int value defined in
// 'errno.h'. If it is < 0, attribute PARAMS is
// ignored. if its value is STATUS_PENDING on proxy
// side, a uint64 attribute 'taskid' is also returned
// as for canceling purpose. And the true response
// message will be returned in the future.
#define JSON_ATTR_RETCODE       "ReturnCode"
#define CONN_PARAM_FILE "conn_params"
#define HOP_DIR  "nexthop"
#define STREAM_DIR "streams"
#define COMMAND_FILE "commands"
#define SVCSTAT_FILE "svcstat"
#define JSON_REQ_FILE "jreq_0"
#define JSON_RESP_FILE "jrsp_0"
#define JSON_EVT_FILE "jevt_0"
#define JSON_STMEVT_FILE "jevt_streams"
#define CONN_DIR_PREFIX "connection_"
#define USER_DIR "user"

#define MAX_STREAMS_PER_SVC 1024
#define MAX_STREAMS_PER_SESS 256

// don't modify these values
#define MAX_STM_QUE_SIZE   32     // STM_MAX_PACKETS_REPORT
#define MAX_REQ_QUE_SIZE   1024
#define MAX_EVT_QUE_SIZE   1024

// max bytes per request/response
#define MAX_FUSE_MSG_SIZE  ( 1024 * 1024 ) // MAX_BYTES_PER_TRANSFER
#define MAX_EVT_QUE_BYTES  ( 16 * MAX_FUSE_MSG_SIZE )

// max bytes per stream read/write
#define MAX_FUSE_STM_PACKET ( MAX_FUSE_MSG_SIZE * 16 )

#include <sys/ioctl.h>
enum {
    // set the fd to operate in blocking mode
    FIOC_SETBLOCK = _IO('J', 0),    // 0x4a00
    // set the fd to operate in non-blocking mode
    FIOC_SETNONBLOCK = _IO('J', 1),  // 0x4a01
    FIOC_GETSIZE = _IOR('J', 2, int)  // 0x80044a02
};
