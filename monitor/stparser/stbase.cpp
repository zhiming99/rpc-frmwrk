/*
 * =====================================================================================
 *
 *       Filename:  stbase.cpp
 *
 *    Description:  support functions for ST parser 
 *
 *        Version:  1.0
 *        Created:  04/12/2026 10:26:54 PM
 *       Revision:  none
 *       Compiler:  Bison
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
#include "rpc.h"
#include "stlexer.h"
#include "stparser.h"
#include "stbase.h"

#include <climits>
#include <cerrno>

using namespace rpcf;

// All the long time literal forms ('LT#', 'LTIME#', 'LDATE#',
// 'LDT#', 'LTOD#', ...) start with an 'L' in their prefix.
static bool StLiteralIsLong( const char* text )
{
    const char* pHash = strchr( text, '#' );
    if( pHash == nullptr || pHash == text )
        return false;
    return std::toupper( ( unsigned char )text[ 0 ] ) == 'L';
}

// Pack a time value into the configuration object shared by all
// the time literal conversions.
static gint32 PackStTimeObject(
    const void* pData, size_t dwSize,
    bool bLongTime, ObjPtr& pObj )
{
    BufPtr pBuf;
    gint32 ret = pBuf.NewObj();
    if( ERROR( ret ) || pBuf.IsEmpty() )
        return ret;

    ret = pBuf->Resize( dwSize );
    if( ERROR( ret ) )
        return ret;

    memcpy( pBuf->ptr(), pData, dwSize );

    CfgPtr pCfg;
    ret = pCfg.NewObj();
    if( ERROR( ret ) || pCfg.IsEmpty() )
        return ret;

    CCfgOpener oCfg( ( IConfigDb* )pCfg );
    ret = oCfg.SetBufPtr( propStTimeSpec, pBuf );
    if( ERROR( ret ) )
        return ret;

    ret = oCfg.SetBoolProp( propStTimeIsLong, bLongTime );
    if( ERROR( ret ) )
        return ret;

    pObj = pCfg;
    return 0;
}

timespec st_time_to_timespec(const char* text)
{
    struct timespec ts = {0, 0};
    if (!text) return ts;

    // Skip the prefix, which can be T#, TIME#, LT# or LTIME#.
    // The prefix only selects the literal's width; the lexical
    // form after '#' is the same for all of them.
    const char* ptr = strchr(text, '#');
    if (!ptr) return ts;
    ptr++; 

    bool bNegative = false;
    if( *ptr == '+' )
        ptr++;
    else if( *ptr == '-' )
    {
        bNegative = true;
        ptr++;
    }

    // Accumulate in nanoseconds instead of 'double seconds'.
    // LTIME covers the full signed 64-bit range, and a double
    // only has 53 bits of mantissa, so large LTIME values used
    // to lose their sub-second part (e.g. the standard maximum
    // LTIME#106751d23h47m16s854ms775us807ns).  __int128 is used
    // as an overflow guard before the final clamp.
    __int128 iTotalNs = 0;

    while (*ptr != '\0') {
        // Skip underscores (ST allows T#1_000ms)
        if( *ptr == '_' )
        {
            ptr++;
            continue;
        }
        if( !std::isdigit( ( unsigned char )*ptr ) )
            break;

        // Integer part; saturate early so the later unit
        // multiplication cannot overflow __int128.
        __int128 iIntPart = 0;
        while( std::isdigit( ( unsigned char )*ptr ) )
        {
            if( iIntPart < 1000000000000000000LL )
                iIntPart = iIntPart * 10 + ( *ptr - '0' );
            ptr++;
        }

        // Optional fractional part, applied with the unit below
        __int128 iFracNum = 0;
        __int128 iFracDen = 1;
        if( *ptr == '.' )
        {
            ptr++;
            while( std::isdigit( ( unsigned char )*ptr ) )
            {
                if( iFracDen <= 1000000000000000000LL )
                {
                    iFracNum = iFracNum * 10 + ( *ptr - '0' );
                    iFracDen *= 10;
                }
                ptr++;
            }
        }

        // Longest units first so that 'ms' is not read as 'm'
        __int128 iUnitNs = 0;
        if (strncasecmp(ptr, "ms", 2) == 0) { 
            iUnitNs = 1000000LL;
            ptr += 2; 
        } else if (strncasecmp(ptr, "us", 2) == 0) { 
            iUnitNs = 1000LL;
            ptr += 2; 
        } else if (strncasecmp(ptr, "ns", 2) == 0) { 
            iUnitNs = 1LL;
            ptr += 2; 
        } else {
            char unit = std::tolower( ( unsigned char )*ptr );
            if (unit == 'd')      iUnitNs = 86400LL * 1000000000LL;
            else if (unit == 'h') iUnitNs = 3600LL * 1000000000LL;
            else if (unit == 'm') iUnitNs = 60LL * 1000000000LL;
            else if (unit == 's') iUnitNs = 1000000000LL;
            else break;   // unknown unit: keep what we have
            ptr++;
        }

        iTotalNs += iIntPart * iUnitNs;
        if( iFracNum != 0 )
            iTotalNs += iFracNum * iUnitNs / iFracDen;
    }

    if( bNegative )
        iTotalNs = -iTotalNs;

    // Clamp to the signed 64-bit nanosecond range that
    // time_t/long can carry on this platform.
    if( iTotalNs > ( __int128 )LLONG_MAX )
        iTotalNs = ( __int128 )LLONG_MAX;
    else if( iTotalNs < ( __int128 )LLONG_MIN )
        iTotalNs = ( __int128 )LLONG_MIN;

    gint64 qwNs = ( gint64 )iTotalNs;
    ts.tv_sec = ( time_t )( qwNs / 1000000000LL );
    ts.tv_nsec = ( long )( qwNs % 1000000000LL );

    // Safety: ensure nsec is within [0, 999999999]
    if (ts.tv_nsec < 0) {
        ts.tv_sec -= 1;
        ts.tv_nsec += 1000000000L;
    }
    
    return ts;
}

std::pair< gint32, ObjPtr > ParseStDuration( const char* text )
{
    std::pair< gint32, ObjPtr > result(
        stparser::TOK_DURATION, ObjPtr() );

    if( text == nullptr )
        return result;

    timespec ts = st_time_to_timespec( text );

    ObjPtr pObj;
    gint32 ret = PackStTimeObject( &ts, sizeof( ts ),
        StLiteralIsLong( text ), pObj );
    if( ERROR( ret ) )
        return result;

    result.second = pObj;
    return result;
}

// Parse the characters following '#' into a 'struct tm'.  Only
// the fields carried by the literal are filled in.
static gint32 ParseStTmText(
    const char* pText, gint32 iTokId,
    struct tm& tmVal, double* pdSec = nullptr )
{
    memset( &tmVal, 0, sizeof( tmVal ) );
    tmVal.tm_isdst = -1;
    // A TIME_OF_DAY literal has no date, keep the fields at a
    // valid epoch day so the struct is always well formed.
    tmVal.tm_mday = 1;

    int iYear = 0, iMon = 0, iDay = 0;
    int iHour = 0, iMin = 0;
    double dSec = 0;
    int iCount = 0;

    switch( iTokId )
    {
    case stparser::TOK_TIME_OF_DAY:
        iCount = sscanf( pText, "%d:%d:%lf",
            &iHour, &iMin, &dSec );
        if( iCount != 3 )
            return -EINVAL;
        break;
    case stparser::TOK_DATE:
        iCount = sscanf( pText, "%d-%d-%d",
            &iYear, &iMon, &iDay );
        if( iCount != 3 )
            return -EINVAL;
        break;
    case stparser::TOK_DATE_TIME:
        iCount = sscanf( pText, "%d-%d-%d-%d:%d:%lf",
            &iYear, &iMon, &iDay, &iHour, &iMin, &dSec );
        if( iCount != 6 )
            return -EINVAL;
        break;
    default:
        return -EINVAL;
    }

    if( iTokId != stparser::TOK_TIME_OF_DAY &&
        ( iMon < 1 || iMon > 12 || iDay < 1 || iDay > 31 ) )
        return -EINVAL;

    if( iTokId != stparser::TOK_DATE &&
        ( iHour < 0 || iHour > 23 ||
          iMin < 0 || iMin > 59 ||
          dSec < 0 || dSec > 60 ) )
        return -EINVAL;

    if( iTokId != stparser::TOK_TIME_OF_DAY )
    {
        tmVal.tm_year = iYear - 1900;
        tmVal.tm_mon = iMon - 1;
        tmVal.tm_mday = iDay;
    }

    if( iTokId != stparser::TOK_DATE )
    {
        tmVal.tm_hour = iHour;
        tmVal.tm_min = iMin;
        // 'struct tm' only has second resolution, the
        // fractional part is dropped
        tmVal.tm_sec = ( int )dSec;
    }

    if( pdSec != nullptr )
        *pdSec = dSec;

    return 0;
}

std::pair< gint32, ObjPtr > ParseStTimeLiteral(
    const char* text, gint32 iTokId )
{
    std::pair< gint32, ObjPtr > result( iTokId, ObjPtr() );

    if( text == nullptr )
        return result;

    const char* pStart = strchr( text, '#' );
    if( pStart == nullptr )
        return result;
    pStart++;

    struct tm tmVal;
    gint32 ret = ParseStTmText( pStart, iTokId, tmVal );
    if( ERROR( ret ) )
        return result;

    ObjPtr pObj;
    ret = PackStTimeObject( &tmVal, sizeof( tmVal ),
        StLiteralIsLong( text ), pObj );
    if( ERROR( ret ) )
        return result;

    result.second = pObj;
    return result;
}

// Parse an integer literal into a 64-bit bit pattern.
// The radix comes from the '2#'/'16#' prefix, or is 10
// without a prefix.  Underscores are ignored, and a
// leading '-' negates the value.
static gint32 ParseStIntText(
    const char* pText, guint64& qwValRet, gint32& iBase )
{
    const char* pHash = strchr( pText, '#' );
    const char* p = pText;
    iBase = 10;

    if( pHash != nullptr )
    {
        // The prefix before '#' is the radix in decimal
        char* pEnd = nullptr;
        unsigned long ulBase = strtoul( pText, &pEnd, 10 );
        if( pEnd != pHash ||
            ( ulBase != 2 && ulBase != 16 ) )
            return -EINVAL;
        iBase = ( gint32 )ulBase;
        p = pHash + 1;
    }

    bool bNegative = false;
    if( *p == '+' )
        p++;
    else if( *p == '-' )
    {
        bNegative = true;
        p++;
    }

    guint64 qwVal = 0;
    bool bAnyDigit = false;

    for( ; *p != '\0'; p++ )
    {
        if( *p == '_' )
            continue;

        int iDigit = 0;
        if( *p >= '0' && *p <= '9' )
            iDigit = *p - '0';
        else if( *p >= 'A' && *p <= 'F' )
            iDigit = *p - 'A' + 10;
        else if( *p >= 'a' && *p <= 'f' )
            iDigit = *p - 'a' + 10;
        else
            return -EINVAL;

        if( iDigit >= iBase )
            return -EINVAL;

        if( qwVal > ( UINT64_MAX - iDigit ) / iBase )
            return -ERANGE;

        qwVal = qwVal * iBase + iDigit;
        bAnyDigit = true;
    }

    if( false == bAnyDigit )
        return -EINVAL;

    if( bNegative )
        qwVal = 0 - qwVal; // keep the two's complement bit pattern

    qwValRet = qwVal;
    return 0;
}

std::pair< gint32, ObjPtr > ParseStNumericLiteral(
    const char* text, gint32 iTokId )
{
    std::pair< gint32, ObjPtr > result( iTokId, ObjPtr() );

    if( text == nullptr )
        return result;

    CfgPtr pCfg;
    gint32 ret = pCfg.NewObj();
    if( ERROR( ret ) || pCfg.IsEmpty() )
        return result;

    CCfgOpener oCfg( ( IConfigDb* )pCfg );

    if( iTokId == stparser::TOK_INT )
    {
        guint64 qwVal = 0;
        gint32 iBase = 10;
        ret = ParseStIntText( text, qwVal, iBase );
        if( ERROR( ret ) )
            return result;

        ret = oCfg.SetQwordProp( propStNumValue, qwVal );
        if( ERROR( ret ) )
            return result;

        ret = oCfg.SetIntProp( propStNumFlag, iBase );
        if( ERROR( ret ) )
            return result;
    }
    else if( iTokId == stparser::TOK_REAL )
    {
        const char* pHash = strchr( text, '#' );
        const char* p = ( pHash != nullptr ) ? pHash + 1 : text;

        // strtod does not accept the ST '_' separators
        std::string strVal;
        for( ; *p != '\0'; p++ )
        {
            if( *p != '_' )
                strVal += *p;
        }

        if( strVal.empty() )
            return result;

        errno = 0;
        char* pEnd = nullptr;
        double dVal = strtod( strVal.c_str(), &pEnd );
        if( pEnd == strVal.c_str() || *pEnd != '\0' ||
            errno == ERANGE )
            return result;

        ret = oCfg.SetDoubleProp( propStNumValue, dVal );
        if( ERROR( ret ) )
            return result;

        // 'LREAL#' is double precision; 'REAL#' and the
        // unprefixed form are single precision
        bool bDouble = ( pHash != nullptr &&
            std::toupper( ( unsigned char )text[ 0 ] ) == 'L' );

        ret = oCfg.SetIntProp( propStNumFlag,
            bDouble ? stRealDouble : stRealFloat );
        if( ERROR( ret ) )
            return result;
    }
    else
        return result;

    result.second = pCfg;
    return result;
}

std::pair< gint32, ObjPtr > ParseStBoolLiteral( const char* text )
{
    std::pair< gint32, ObjPtr > result(
        stparser::TOK_BOOL, ObjPtr() );

    if( text == nullptr )
        return result;

    const char* p = text;
    const char* pHash = strchr( text, '#' );
    if( pHash != nullptr )
    {
        // the only accepted prefix is 'BOOL#'
        if( pHash - text != 4 ||
            strncasecmp( text, "BOOL", 4 ) != 0 )
            return result;
        p = pHash + 1;
    }

    bool bVal = false;
    if( strcasecmp( p, "TRUE" ) == 0 )
        bVal = true;
    else if( strcasecmp( p, "FALSE" ) == 0 )
        bVal = false;
    else if( p[ 0 ] == '0' && p[ 1 ] == '\0' )
        bVal = false;
    else if( p[ 0 ] == '1' && p[ 1 ] == '\0' )
        bVal = true;
    else
        // rejects 'BOOL#23', 'BOOL#01', 'BOOL#-1' and the like
        return result;

    CfgPtr pCfg;
    gint32 ret = pCfg.NewObj();
    if( ERROR( ret ) || pCfg.IsEmpty() )
        return result;

    CCfgOpener oCfg( ( IConfigDb* )pCfg );
    ret = oCfg.SetBoolProp( propStBoolValue, bVal );
    if( ERROR( ret ) )
        return result;

    result.second = pCfg;
    return result;
}

// Helper to convert ST date/time literals to Unix
// Epoch (UTC).  For a TIME_OF_DAY literal the returned
// value is milliseconds since midnight instead, as
// PLCs commonly store it that way.  Returns 0 on
// malformed input.
uint64_t ParseStTimeToUnix(const char* input)
{
    if( input == nullptr )
        return 0;

    const char* pStart = strchr( input, '#' );
    if( pStart == nullptr )
        return 0;
    pStart++;

    gint32 iTokId = 0;
    bool bHasDate = strchr( pStart, '-' ) != nullptr;
    bool bHasTime = strchr( pStart, ':' ) != nullptr;
    if( bHasDate && bHasTime )
        iTokId = stparser::TOK_DATE_TIME;
    else if( bHasDate )
        iTokId = stparser::TOK_DATE;
    else if( bHasTime )
        iTokId = stparser::TOK_TIME_OF_DAY;
    else
        return 0;

    struct tm tmVal;
    double dSec = 0;
    gint32 ret = ParseStTmText( pStart, iTokId, tmVal, &dSec );
    if( ERROR( ret ) )
        return 0;

    if( iTokId == stparser::TOK_TIME_OF_DAY )
    {
        // Milliseconds since midnight
        return ( uint64_t )( tmVal.tm_hour * 3600000LL +
            tmVal.tm_min * 60000LL +
            ( gint64 )( dSec * 1000.0 + 0.5 ) );
    }

    // Use UTC rather than the local timezone so the result
    // does not depend on the environment
    time_t t = timegm( &tmVal );
    if( t == ( time_t )-1 )
        return 0;

    return ( uint64_t )t;
}


std::string TranslateSTString(const std::string& input) {
    // 1. Remove the surrounding single quotes
    if (input.size() < 2) return "";
    std::string st = input.substr(1, input.size() - 2);

    std::string result;
    result.reserve(st.size()); // Optimization

    for (size_t i = 0; i < st.size(); ++i) {
        if (st[i] == '$') {
            i++; // Move to character after $
            if( i >= st.size() )
            {
                result += '$'; // Safely preserve trailing $
                break;
            }

            char escape = st[i];
            switch (escape) {
                case 'L': case 'l': result += '\n'; break; // Line feed
                case 'N': case 'n': result += '\n'; break; // New line
                case 'P': case 'p': result += '\f'; break; // Page (form feed)
                case 'R': case 'r': result += '\r'; break; // Carriage return
                case 'T': case 't': result += '\t'; break; // Tab
                case '$': result += '$';  break;           // Literal $
                case '\'': result += '\''; break;          // Literal '
                default:
                    // Handle Hex: $0A
                    if (isxdigit(escape) && i + 1 < st.size() && isxdigit(st[i+1])) {
                        std::string hexStr = st.substr(i, 2);
                        char c = (char)std::stoi(hexStr, nullptr, 16);
                        result += c;
                        i++; // Skip second hex digit
                    } else {
                        // Fallback for unknown escape
                        result += '$';
                        result += escape;
                    }
                    break;
            }
        } else if (st[i] == '\'' && i + 1 < st.size() && st[i+1] == '\'') {
            // Handle ST's '' (double single quote)
            result += '\'';
            i++;
        } else {
            result += st[i];
        }
    }
    return result;
}

#include <cctype>

std::wstring TranslateSTWString(const std::string& input) 
{
    // 1. Remove surrounding double quotes
    if (input.size() < 2) return L"";
    std::string st = input.substr(1, input.size() - 2);
                                                                                                                                                                                                                   
    std::wstring result;
    result.reserve(st.size());

    for (size_t i = 0; i < st.size(); ++i) {
        if (st[i] == '$') {
            i++;
            if (i >= st.size()) {
                result += L'$'; // Hanging dollar sign safety handler
                break;
            }

            char escape = st[i];
            switch (escape) {
            case 'L': case 'l': result += L'\n'; break;
            case 'N': case 'n': result += L'\n'; break; // Treat as \n or change to \r\n based on destination OS target
            case 'P': case 'p': result += L'\f'; break;
            case 'R': case 'r': result += L'\r'; break;
            case 'T': case 't': result += L'\t'; break;
            case '$': result += L'$';  break;
            case '\"': result += L'\"'; break; // Double quote escape ($")
            default:
                // Check for 4-digit hex: $000A
                if (i + 3 < st.size() &&
                    std::isxdigit(st[i]) && std::isxdigit(st[i+1]) &&
                    std::isxdigit(st[i+2]) && std::isxdigit(st[i+3])) {
                                                                                                                                                                                                               
                    std::string hexStr = st.substr(i, 4);
                    wchar_t wc = static_cast<wchar_t>(std::stoi(hexStr, nullptr, 16));
                    result += wc;
                    i += 3; // Skip the remaining 3 hex digits (4th skipped by loop update)
                }
                // Fallback for 2-digit hex: $0A
                else if (i + 1 < st.size() && std::isxdigit(st[i]) && std::isxdigit(st[i+1])) {
                    std::string hexStr = st.substr(i, 2);
                    wchar_t wc = static_cast<wchar_t>(std::stoi(hexStr, nullptr, 16));
                    result += wc;
                    i += 1; // FIXED: Skip the remaining 1 hex digit (2nd skipped by loop update)
                }
                else {
                    result += L'$'; // Re-insert escape delimiter
                    result += static_cast<wchar_t>(escape);
                }
                break;
            }
        } else {
            result += static_cast<wchar_t>(st[i]);
        }
    }
    return result;
}

#include <sstream>
#include <iomanip>
#include <iostream>

std::string TranslateSTUString(const std::string& yytext)
{
    std::string result;
    if (yytext.empty()) return result;

    // 1. Identify and skip the prefix (USTRING#' or U#')
    size_t start_idx = 0;
    if (yytext.rfind("USTRING#'", 0) == 0) {
        start_idx = 9; // length of "USTRING#'"
    } else if (yytext.rfind("U#'", 0) == 0) {
        start_idx = 3; // length of "U#'"
    } else {
        // Fallback if the token doesn't match expected prefixes
        return yytext;
    }

    // Ensure we don't read past the terminating single quote
    size_t end_idx = yytext.length() - 1;
    // C++11 Safe: check back element via index instead of using string::back()
    if (yytext[yytext.length() - 1] != '\'') {
        end_idx = yytext.length(); // Malformed string safety check
    }

    // 2. Parse the body of the string literal
    for (size_t i = start_idx; i < end_idx; ++i) {
        if (yytext[i] == '$') {
            // Check if there is a character following the escape symbol
            if (i + 1 >= end_idx) {
                result += '$'; // Malformed hanging '$', treat literally
                continue;
            }

            char next_char = yytext[i + 1];

            // Handle standard IEC control code escapes
            if (next_char == 'n' || next_char == 'N') { result += '\n'; i += 1; }
            else if (next_char == 't' || next_char == 'T') { result += '\t'; i += 1; }
            else if (next_char == 'r' || next_char == 'R') { result += '\r'; i += 1; }
            else if (next_char == 'l' || next_char == 'L') { result += '\n'; i += 1; } // line feed
            else if (next_char == 'p' || next_char == 'P') { result += '\f'; i += 1; } // page (form feed)
            else if (next_char == '$') { result += '$';  i += 1; }
            else if (next_char == '\'') { result += '\''; i += 1; }

            // Handle standard 2-digit Hex Escapes ($FF)
            else if (next_char != '{' && i + 2 < end_idx && std::isxdigit(yytext[i+1]) && std::isxdigit(yytext[i+2])) {
                std::string hex_str = yytext.substr(i + 1, 2);
                char decoded_byte = static_cast<char>(std::stoul(hex_str, nullptr, 16));
                result += decoded_byte;
                i += 2;
            }

            // Handle Edition 4 Unicode Bracket Escapes (${1F579})
            else if (next_char == '{') {
                size_t close_brace = yytext.find('}', i + 2);
                if (close_brace != std::string::npos && close_brace < end_idx) {
                    size_t hex_len = close_brace - (i + 2);
                    std::string hex_str = yytext.substr(i + 2, hex_len);

                    try {
                        unsigned long cp = std::stoul(hex_str, nullptr, 16);

                        // Convert the Unicode Code Point to UTF-8 Multi-byte Code Units
                        if (cp <= 0x7F) {
                            result += static_cast<char>(cp);
                        } else if (cp <= 0x7FF) {
                            result += static_cast<char>(0xC0 | ((cp >> 6) & 0x1F));
                            result += static_cast<char>(0x80 | (cp & 0x3F));
                        } else if (cp <= 0xFFFF) {
                            result += static_cast<char>(0xE0 | ((cp >> 12) & 0x0F));
                            result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                            result += static_cast<char>(0x80 | (cp & 0x3F));
                        } else if (cp <= 0x10FFFF) {
                            result += static_cast<char>(0xF0 | ((cp >> 18) & 0x07));
                            result += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
                            result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                            result += static_cast<char>(0x80 | (cp & 0x3F));
                        }
                    } catch (...) {
                        // If parsing hex fails, fallback to printing it raw
                        result += yytext.substr(i, close_brace - i + 1);
                    }
                    i = close_brace; // Move loop index to the closing brace
                } else {
                    // No matching closing brace found
                    result += "$}";
                    i += 1;
                }
            }
            else {
                // Unknown character escape, treat '$' literally
                result += '$';
            }
        } else {
            // Pass standard and multi-byte raw characters straight through
            result += yytext[i];
        }
    }

    return result;
}

// The legacy address scanners below depend on the bison
// scanner (yyscan_t, yyget_*, ParserPrint) and on the old
// CSTParserContext, which are not part of the ANTLR build.
// Keep the code for the future port, but do not compile it.
#ifdef ST_LEGACY_ADDR_SCANNERS

ObjPtr ParsePeriAddr( const char* yytext, CSTParserContext* pCtx )
{
    gint32 ret = 0;
    IntVecPtr pvecInt;
    pvecInt.NewObj();
    yyscan_t scanner = pCtx->GetScanner();

    do{
        // "%"[Pp]?{ADDR_AREA}{ADDR_SIZE}?{DIGIT}+("."{DIGIT}+|"*")?(":P"|":p")? 
        // 1. Dynamically find where the ADDR_AREA begins
        // Skip '%' (index 0). If index 1 is 'P' or 'p', skip that too.
        int area_idx = (yytext[1] == 'P' || yytext[1] == 'p') ? 2 : 1;
        int size_idx = area_idx + 1;
        int is_periphrial = ( area_idx == 2 );

        int addr_size, addr_idx = 0, addr_bidx = -1;

        // 2. Extract Area ('I', 'Q', 'M')
        int text_len = yyget_leng( scanner );
        if( !is_periphrial &&
            (  yytext[ text_len - 1 ] == 'P' ||
            yytext[ text_len - 1 ] == 'p' ) )
            is_periphrial = true;

        ( *pvecInt )().push_back( is_periphrial );
        ( *pvecInt )().push_back( yytext[ area_idx ] );

        // 3. Determine if Size is explicit, or if we hit a number immediately
        int digit_start_idx = size_idx;
        bool size_is_explicit = !(yytext[size_idx] >= '0' && yytext[size_idx] <= '9');

        if (size_is_explicit) {
            addr_size = std::toupper(yytext[size_idx]); // Normalize to uppercase
            digit_start_idx = size_idx + 1;
        } else {
            addr_size = 'X'; // Default fallback under IEC 61131-3
        }
        ( *pvecInt )().push_back( addr_size );

        gint32 iLineNo = yyget_lineno( scanner );
        auto current_ptr = yytext + digit_start_idx;
        if( *current_ptr == '*' )
        {
            if( is_periphrial )
            {
                ParserPrint( 
                    pCtx->GetCurFileName().c_str(), 
                    iLineNo,
                    "Error, 'incomplete address' cannot specify 'P' flag", true );
                ret = -EINVAL;
                break;
            }
            // address will be set in the VAR_CONFIG section.
            ( *pvecInt )().push_back( -2 );
            ( *pvecInt )().push_back( -2 );
            break;
        }

        // 4. Extract the primary index (Byte/Word offset)
        addr_idx = std::atoi(current_ptr);

        // 5. Look for the dot '.' separating the sub-bit index
        auto dot_ptr = strchr(current_ptr, '.');
        gint32 ret = 0;
        int max_bit_limit = 7;
        if (dot_ptr != nullptr)
        {
            addr_bidx = std::atoi(dot_ptr + 1);

            // Compute maximum allowable bit boundary based on the token context
            switch (addr_size) {
                case 'W': max_bit_limit = 15; break;
                case 'D': max_bit_limit = 31; break;
                case 'L': max_bit_limit = 63; break;
                default:  max_bit_limit = 7;  break;
            }

            // Strict Range Validation
            if (addr_bidx < 0 || addr_bidx > max_bit_limit) {
                ret = -EINVAL;
            }
        }
        else if( addr_size == 'X' )
        {
            addr_bidx = addr_bidx;
            addr_idx = 0;
            if( addr_bidx > 7 )
                ret = -EINVAL;
        }
        else
        {
            addr_bidx = -1;
        }

        if( ERROR( ret ) )
        {
            char szBuf[ 512 ];
            sprintf( szBuf, 
                "Error, Bit index %d exceeds the limit of %d bits "
                "for size modifier '%c'",
                addr_bidx, max_bit_limit, addr_size );
            ParserPrint( 
                pCtx->GetCurFileName().c_str(), 
                iLineNo,
                szBuf, true );
            break;
        }

        // direct address type: 0 for peripheral access
        ( *pvecInt )().push_back( addr_idx );
        ( *pvecInt )().push_back( addr_bidx );

    }while( 0 );

    if( ERROR( ret ) )
        return ObjPtr();
    return ObjPtr( pvecInt );
}

ObjPtr ParseRpcfAddr( const char* yytext, CSTParserContext* pCtx )
{
    StrVecPtr pvecStr;
    pvecStr.NewObj();
    gint32 ret = 0;

    //"@"{ATTR_DATA_TYPE}{IDENTIFIER}(\.{IDENTIFIER}){0,2}(:{IDENTIFIER})? 
    // 1. Dynamically find where the ADDR_AREA begins
    // Skip '%' (index 0). If index 1 is 'P' or 'p', skip that too.
    do{
        int ptype_idx = 1;
        int size_idx = ptype_idx + 1;
        int attr_idx = -1;

        int addr_size, addr_idx = 0, addr_bidx = -1;
        yyscan_t scanner = pCtx->GetScanner();


        ( *pvecStr )().push_back(
            stdstr( yytext + ptype_idx, 1 ) );

        ( *pvecStr )().push_back(
            stdstr( yytext + size_idx, 1 ) );

        const char* pend = nullptr;
        const char* colon_ptr = strchr(
            yytext + size_idx + 1, ':');

        stdstr strAttr = "value";
        if( colon_ptr != nullptr )
        {
            pend = colon_ptr;
            strAttr = colon_ptr + 1;
        }
        else
        {
            int text_len = yyget_leng( scanner );
            pend = yytext + text_len;
        }

        ( *pvecStr )().push_back( strAttr );

        stdstr strPath( yytext + 3, pend - yytext - 3 );
        std::replace( strPath.begin(), strPath.end(), '.', '/' );
        std::vector< stdstr > comps;
        ret = CRegistry::Namei( strPath, comps );
        if( ERROR( ret ) )
            break;

        if( comps.empty() )
        {
            gint32 iLineNo = yyget_lineno( scanner );
            ParserPrint( 
                pCtx->GetCurFileName().c_str(), 
                iLineNo,
                "Error, Invalid rpcf address", true );
            ret = -EINVAL;
            break;
        }
        // reversely push the component
        auto iter = comps.rbegin();
        while( iter != comps.rend() )
            ( *pvecStr )().push_back( *iter );
        // the layout is
        // <point type> <data size> <attr name > <point name> [<app name> [<node name>]]

    }while( 0 );
    if( ERROR( ret ) )
        return ObjPtr();

    return ObjPtr( pvecStr );
}

#endif // ST_LEGACY_ADDR_SCANNERS
