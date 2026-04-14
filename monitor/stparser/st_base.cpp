/*
 * =====================================================================================
 *
 *       Filename:  st_base.cpp
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

std::string process_st_string(const char* input)
{
    if (!input) return "";

    std::string result;
    size_t len = std::strlen(input);

    // Skip the opening and closing single quotes (assuming '...' format)
    for (size_t i = 1; i < len - 1; ++i) {
        // 1. Handle escaped single quotes ('')
        if (input[i] == '\'' && input[i + 1] == '\'') {
            result += '\'';
            i++; // Skip the second quote
        } 
        // 2. Handle dollar sign escape sequences ($)
        else if (input[i] == '$') {
            i++;
            if (i >= len - 1) break; // Safety check

            char next = std::tolower(input[i]);
            switch (next) {
                case 'l': result += '\n'; break; // Line feed
                case 'n': result += "\r\n"; break; // New line
                case 'p': result += '\f'; break; // Page break
                case 'r': result += '\r'; break; // Carriage return
                case 't': result += '\t'; break; // Tab
                case '$': result += '$';  break; // Literal dollar
                case '\'': result += '\''; break; // Literal single quote
                default:
                    // 3. Handle hex escapes ($hh)
                    if (std::isxdigit(input[i]) && i + 1 < len - 1 && std::isxdigit(input[i + 1])) {
                        char hex[3] = { input[i], input[i + 1], '\0' };
                        result += static_cast<char>(std::strtol(hex, nullptr, 16));
                        i++; 
                    } else {
                        // If invalid escape, keep the character
                        result += input[i];
                    }
                    break;
            }
        } 
        // 4. Handle regular characters
        else {
            result += input[i];
        }
    }
    return result;
}

// Helper to convert ST literals to Unix Epoch
uint64_t parse_st_literal_to_unix(const char* input) {
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

