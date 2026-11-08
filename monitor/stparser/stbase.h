/*
 * =====================================================================================
 *
 *       Filename:  stbase.h
 *
 *    Description:  declarations of utilities functions for lexer.
 *
 *        Version:  1.0
 *        Created:  05/26/2026 07:07:26 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2026 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */

#pragma once
#include <cstring>
#include <strings.h>
#include <ctime>
#include <cctype>
#include <cstdlib>
#include <string>
#include <stdint.h>
#include <sstream>
#include <iomanip>
#include <utility>
#include "rpc.h"

// legacy parser context used by the address scanners
struct CSTParserContext;

timespec st_time_to_timespec(const char* text);

// Property indices of the configuration object returned by
// ParseStDuration and ParseStTimeLiteral in the pair's second
// element.
enum {
    // BufPtr holding the value: a 'struct timespec' for
    // TOK_DURATION, or a 'struct tm' for TOK_TIME_OF_DAY,
    // TOK_DATE and TOK_DATE_TIME.
    propStTimeSpec = rpcf::propReservedEnd + 1024 + 1,
    // bool: true for the long ('L') forms, i.e. 'LT#'/'LTIME#',
    // 'LDATE#', 'LDT#' and 'LTOD#'
    propStTimeIsLong,
    // Variant holding the value of a numeric literal: guint64 for
    // TOK_INT (two's complement bit pattern) or double for
    // TOK_REAL.
    propStNumValue = rpcf::propReservedEnd + 1024 + 3,
    // gint32 flag describing the literal:
    //   TOK_INT:  radix, 2, 10 or 16
    //   TOK_REAL: stRealFloat for REAL, stRealDouble for LREAL
    propStNumFlag,
    // bool: the value of a bool literal, i.e. 'TRUE', 'FALSE',
    // 'BOOL#0', 'BOOL#1', 'BOOL#TRUE' or 'BOOL#FALSE'
    propStBoolValue = rpcf::propReservedEnd + 1024 + 5,
};

// Values of propStNumFlag for a TOK_REAL literal
enum {
    stRealFloat = 0,  // REAL,  single precision
    stRealDouble = 1, // LREAL, double precision
};

/**
 * Convert a duration literal to a configuration object.
 *
 * Accepts the IEC 61131-3 duration forms T#, TIME#, LT# and
 * LTIME#, with an optional sign and any of the d/h/m/s/ms/us/ns
 * components.
 *
 * @return a pair whose first element is stparser::TOK_DURATION and
 *         whose second element is an ObjPtr holding a CConfigDb2
 *         with propStTimeSpec set to a BufPtr containing a
 *         'struct timespec', and propStTimeIsLong set to whether
 *         the literal is an 'LT' form. The ObjPtr is empty if the
 *         conversion failed.
 */
std::pair< gint32, rpcf::ObjPtr >
    ParseStDuration( const char* text );

/**
 * Convert a time-of-day, date or date-and-time literal to a
 * configuration object.
 *
 * @param text   the literal text, including its prefix.
 * @param iTokId one of stparser::TOK_TIME_OF_DAY,
 *               stparser::TOK_DATE or stparser::TOK_DATE_TIME.
 * @return a pair whose first element is iTokId and whose second
 *         element is an ObjPtr holding a CConfigDb2 with
 *         propStTimeSpec set to a BufPtr containing a
 *         'struct tm', and propStTimeIsLong set to whether the
 *         literal is a long ('L') form. The ObjPtr is empty if
 *         the conversion failed.
 */
std::pair< gint32, rpcf::ObjPtr >
    ParseStTimeLiteral( const char* text, gint32 iTokId );

/**
 * Convert a numeric literal to a configuration object.
 *
 * @param text   the literal text, including its prefix.
 * @param iTokId stparser::TOK_INT or stparser::TOK_REAL.
 * @return a pair whose first element is iTokId and whose second
 *         element is an ObjPtr holding a CConfigDb2 with
 *         propStNumValue set to a guint64 (TOK_INT) or a double
 *         (TOK_REAL), and propStNumFlag set to the radix 2, 10
 *         or 16 for TOK_INT, or to stRealFloat/stRealDouble for
 *         TOK_REAL. The ObjPtr is empty if the conversion failed.
 */
std::pair< gint32, rpcf::ObjPtr >
    ParseStNumericLiteral( const char* text, gint32 iTokId );

/**
 * Convert a bool literal to a configuration object.
 *
 * Accepts 'TRUE' and 'FALSE', case-insensitively, with or without
 * the 'BOOL#' type prefix. In the typed form the integer values
 * 0 and 1 are also accepted; any other integer value, such as
 * 'BOOL#23', is rejected.
 *
 * @param text the literal text, including its prefix.
 * @return a pair whose first element is stparser::TOK_BOOL and
 *         whose second element is an ObjPtr holding a CConfigDb2
 *         with propStBoolValue set to the value. The ObjPtr is
 *         empty if the conversion failed.
 */
std::pair< gint32, rpcf::ObjPtr >
    ParseStBoolLiteral( const char* text );

uint64_t ParseStTimeToUnix(const char* input);
std::string TranslateSTString(const std::string& input);
std::wstring TranslateSTWString(const std::string& input);
std::string TranslateSTUString(const std::string& input);
rpcf::ObjPtr ParsePeriAddr(
    const char* szText, CSTParserContext* pCtx );
rpcf::ObjPtr ParseRpcfAddr(
    const char* szText, CSTParserContext* pCtx );
