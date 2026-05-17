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
#include <cstring>
#include <strings.h>
#include <ctime>
#include <cctype>
#include <cstdlib>
#include <string>
#include <stdint.h>
#include <sstream>
#include <iomanip>
#include "rpc.h"

using namespace rpcf

struct timespec st_time_to_timespec(const char* text) {
    struct timespec ts = {0, 0};
    if (!text) return ts;

    // Skip the "T#" or "TIME#" prefix
    const char* ptr = std::strchr(text, '#');
    if (!ptr) return ts;
    ptr++; 

    double total_seconds = 0.0;
    char* endptr;

    while (*ptr != '\0') {
        // strtod handles numeric values (including decimals like 1.5)
        double val = std::strtod(ptr, &endptr);
        if (ptr == endptr) break;
        ptr = endptr;

        // Skip underscores (ST allows T#1_000ms)
        while (*ptr == '_') ptr++;

        // Unit detection using POSIX strncasecmp
        if (strncasecmp(ptr, "ms", 2) == 0) { 
            total_seconds += val * 0.001; 
            ptr += 2; 
        } else if (strncasecmp(ptr, "us", 2) == 0) { 
            total_seconds += val * 1e-6; 
            ptr += 2; 
        } else if (strncasecmp(ptr, "ns", 2) == 0) { 
            total_seconds += val * 1e-9; 
            ptr += 2; 
        } else {
            char unit = std::tolower(*ptr);
            if (unit == 'd')      total_seconds += val * 86400.0;
            else if (unit == 'h') total_seconds += val * 3600.0;
            else if (unit == 'm') total_seconds += val * 60.0;
            else if (unit == 's') total_seconds += val * 1.0;
            ptr++;
        }
    }

    // Convert to timespec
    ts.tv_sec = static_cast<time_t>(total_seconds);
    ts.tv_nsec = static_cast<long>((total_seconds - static_cast<double>(ts.tv_sec)) * 1e9);
    
    // Safety: ensure nsec is within [0, 999999999]
    if (ts.tv_nsec < 0) {
        ts.tv_sec -= 1;
        ts.tv_nsec += 1000000000L;
    }
    
    return ts;
}

// Helper to convert ST literals to Unix Epoch
uint64_t ParseStTimeToUnix(const char* input) {
    struct tm tm_info = {0};
    std::string s(input);

    // 1. DATE_AND_TIME / DT (yyyy-mm-dd-hh:mm:ss)
    if (s.find("DT#") == 0 || s.find("DATE_AND_TIME#") == 0) {
        const char* start = strchr(input, '#') + 1;
        sscanf(start, "%d-%d-%d-%d:%d:%d",
               &tm_info.tm_year, &tm_info.tm_mon, &tm_info.tm_mday,
               &tm_info.tm_hour, &tm_info.tm_min, &tm_info.tm_sec);
        tm_info.tm_year -= 1900; // Years since 1900
        tm_info.tm_mon -= 1;     // Months 0-11
        return (uint64_t)mktime(&tm_info);
    }

    // 2. DATE (yyyy-mm-dd)
    if (s.find("D#") == 0 || s.find("DATE#") == 0) {
        const char* start = strchr(input, '#') + 1;
        sscanf(start, "%d-%d-%d", &tm_info.tm_year, &tm_info.tm_mon, &tm_info.tm_mday);
        tm_info.tm_year -= 1900;
        tm_info.tm_mon -= 1;
        return (uint64_t)mktime(&tm_info); // Returns start of that day
    }

    // 3. TIME_OF_DAY (hh:mm:ss)
    if (s.find("TOD#") == 0 || s.find("TIME_OF_DAY#") == 0) {
        const char* start = strchr(input, '#') + 1;
        int ms = 0;
        sscanf(start, "%d:%d:%d.%d", &tm_info.tm_hour, &tm_info.tm_min, &tm_info.tm_sec, &ms);
        // TOD is often stored as milliseconds since midnight in PLCs
        return (tm_info.tm_hour * 3600000) + (tm_info.tm_min * 60000) + (tm_info.tm_sec * 1000) + ms;
    }

    return 0;
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
            if (i >= st.size()) break;

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

#include <cwctype>

std::wstring TranslateSTWString(const std::string& input) {
    // 1. Remove surrounding double quotes
    if (input.size() < 2) return L"";
    std::string st = input.substr(1, input.size() - 2);

    std::wstring result;
    result.reserve(st.size());

    for (size_t i = 0; i < st.size(); ++i) {
        if (st[i] == '$') {
            i++;
            if (i >= st.size()) break;

            char escape = st[i];
            switch (escape) {
                case 'L': case 'l': result += L'\n'; break;
                case 'N': case 'n': result += L'\n'; break;
                case 'P': case 'p': result += L'\f'; break;
                case 'R': case 'r': result += L'\r'; break;
                case 'T': case 't': result += L'\t'; break;
                case '$': result += L'$';  break;
                case '\"': result += L'\"'; break; // Double quote escape
                default:
                    // Check for 4-digit hex: $000A
                    if (i + 3 < st.size() &&
                        isxdigit(st[i]) && isxdigit(st[i+1]) &&
                        isxdigit(st[i+2]) && isxdigit(st[i+3])) {

                        std::string hexStr = st.substr(i, 4);
                        wchar_t wc = (wchar_t)std::stoi(hexStr, nullptr, 16);
                        result += wc;
                        i += 3; // Skip the rest of the hex digits
                    }
                    // Fallback for 2-digit hex: $0A
                    else if (i + 1 < st.size() && isxdigit(st[i]) && isxdigit(st[i+1])) {
                        std::string hexStr = st.substr(i, 2);
                        wchar_t wc = (wchar_t)std::stoi(hexStr, nullptr, 16);
                        result += wc;
                        i += 1;
                    }
                    else {
                        result += (wchar_t)escape;
                    }
                    break;
            }
        } else if (st[i] == '\"' && i + 1 < st.size() && st[i+1] == '\"') {
            // Handle ST's "" (double double-quote)
            result += L'\"';
            i++;
        } else {
            result += (wchar_t)st[i];
        }
    }
    return result;
}

ObjPtr ParsePeriAddr( const char* yytext, yyscan_t scanner )
{
    IntVecPtr pvecInt;
    pvecInt.NewObj();

    // "%"[Pp]?{ADDR_AREA}{ADDR_SIZE}?{DIGIT}+("."{DIGIT}+)?(":P"|":p")? 
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

    // 4. Extract the primary index (Byte/Word offset)
    char* current_ptr = yytext + digit_start_idx;
    addr_idx = std::atoi(current_ptr);

    // 5. Look for the dot '.' separating the sub-bit index
    char* dot_ptr = std::strchr(current_ptr, '.');
    gint32 ret = 0;
    if (dot_ptr != nullptr)
    {
        addr_bidx = std::atoi(dot_ptr + 1);

        // Compute maximum allowable bit boundary based on the token context
        int max_bit_limit = 7;
        switch (addr.size) {
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
        std::cerr << "Lexical Error at line " << yylineno
                  << ": Bit index '" << addr_bidx
                  << "' exceeds maximum capacity of " << max_bit_limit
                  << " bits for Size modifier '" << addr.size << "'\n";

        yyerror("Bit index out of bounds");
        return ObjPtr();
    }

    // direct address type: 0 for peripheral access
    ( *pvecInt )().push_back( addr_idx );
    ( *pvecInt )().push_back( addr_bidx );

    return ObjPtr( pvecInt );
}

