/*
 * =====================================================================================
 *
 *       Filename:  monconst.h
 *
 *    Description:  declarations of constants of the monitor module  
 *
 *        Version:  1.0
 *        Created:  02/07/2025 02:17:38 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi(woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2025 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#pragma once
#define APPS_ROOT_DIR       "apps"
#define MONITOR_STREAM_DIR  "notify_streams"
#define OWNER_STREAM_FILE   "owner_stream"
#define POINT_TYPE_FILE     "ptype"
#define OUTPUT_LINKS_DIR    "ptrs"
#define OUTPUT_PULSE        "pulse"
#define OUTPUT_LATCH        "latch"
#define POINTS_DIR          "points"
#define LOGPTR_DIR          "logptrs"
#define LOGS_DIR            "logs"
#define VALUE_FILE          "value"
#define USER_ROOT_DIR       "users"
#define LOAD_ON_START       "load_on_start"
#define DATA_TYPE           "datatype"
#define PID_FILE            "pid"
#define KRB5_ASSOC_DIR       "krb5users"
#define OA2_ASSOC_DIR       "oa2users"
#define POINT_FLAGS         "point_flags"

#define OFFLINE_NOTIFY      "offline_notify"
#define ONLINE_NOTIFY       "online_notify"

#define POINTFLAG_NOSTORE      0x01

#define GETPTDESC_VALUE     0
#define GETPTDESC_PTYPE     1
#define GETPTDESC_PTFLAG    2
#define GETPTDESC_AVERAGE   3
#define GETPTDESC_UNIT      4
#define GETPTDESC_DATATYPE  5
#define GETPTDESC_SIZE      6
#define GETPTDESC_HASLOG    7
#define GETPTDESC_AVGALGO   8
