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

ObjPtr ParsePeriAddr( const char* yytext )
{
    // 1. Convert the Flex C-string match to a C++ string object
    std::string raw_match(yytext);

    // 2. Define the matching C++ pattern using capture groups ().
    // Group 1: Area ([IQMiqm])
    // Group 2: Size ([XBWDLxbwdl]?) -> Optional capture group
    // Group 3: Digits ([0-9]+)
    std::regex extract_pattern("^%([IQMiqm])([XBWDLxbwdl]?)([0-9]+):P$");
    std::regex extract_pattern2("^%P([IQMiqm])([XBWDLxbwdl]?)([0-9]+)$");
    std::smatch matches;
    bool bRet = false;
    if( yytext[1] != 'P' )
        bRet = std::regex_match(raw_match, matches, extract_pattern);
    else
        bRet = std::regex_match(raw_match, matches, extract_pattern2);

    // 3. Execute regex_match to isolate the capture blocks
    if ( bRet )
    {
        IntVecPtr pvecInt;
        pvecInt.NewObj();
        // Extract using sub-match indexing (Index 0 is the full string)
        (*pvecInt)().push_back( matches[1].str()[0] );
        stdstr strSize = matches[2].str();

        // Auto-resolves to "" if missing
        (*pvecInt)().push_back( strSize.empty() ? 'X' : strSize[0] ); 
        (*pvecInt)().push_back( std::stoi(matches[3].str()) );

        // the last one as a flag of peripherial address
        (*pvecInt)().push_back( 1 );
        return ObjPtr( pvecInt );
    }
    return ObjPtr();
}


