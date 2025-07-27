/*
 * =====================================================================================
 *
 *       Filename:  cronexpr.cpp
 *
 *    Description:  implementation of a cron expression parser
 *
 *        Version:  1.0
 *        Created:  23/07/2025 01:38:30 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2025 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#include <iostream>
#include <vector>
#include <unordered_set>
#include <sstream>
#include <ctime>
#include <algorithm>
#include <stdint.h>
#include "time.h"
#include <regex>
#include <memory>
#include "cronexpr.h"

static int mystoi( const std::string& str )
{
    for( auto& elem : str )
        if( elem >= '0' && elem <= '9' )
            continue;
        throw std::runtime_error(
            "Error not a valid value" );
    return std::stoi( str );
}

template< class T >
std::unordered_set<T> ParseOneField(
    const std::string& field, T minval, T maxval)
{
    std::unordered_set<T> result;
    std::stringstream ss(field);
    std::string part;
    while (std::getline(ss, part, ',')) {
        if (part == "*") {
            for (int i = minval; i <= maxval; ++i)
                result.insert(i);
        } else if (part.find('/') != std::string::npos) {
            size_t slash = part.find('/');
            std::string base = part.substr(0, slash);
            int step = mystoi(part.substr(slash + 1));
            int start = minval, end = maxval;
            if (base != "*") {
                size_t dash = base.find('-');
                if (dash != std::string::npos) {
                    start = mystoi(base.substr(0, dash));
                    end = mystoi(base.substr(dash + 1));
                } else {
                    start = end = mystoi(base);
                }
            }
            for( int i = start; i <= end; i+=step )
                result.insert(i);
        } else if (part.find('-') != std::string::npos) {
            size_t dash = part.find('-');
            int start = mystoi(part.substr(0, dash));
            int end = mystoi(part.substr(dash + 1));
            if( start < minval || end > maxval ||
                start > end )
            {
                throw std::runtime_error(
                    "Error invald range value" );
            }
            for (int i = start; i <= end; ++i)
                result.insert(i);
        } else {
            result.insert(mystoi(part));
        }
    }
    return result;
}

#define ParseField ParseOneField<uint8_t>
#define ParseYear ParseOneField<uint16_t>

// Function to check if a year is a leap year
bool IsLeapYear(int year)
{
    return( year % 4 == 0 && year % 100 != 0 ) ||
        ( year % 400 == 0 );
}

// Function to get the number of days in a given month and year
int GetDaysInMonth(
    uint8_t& days, int month, int year )
{
    static int daysInMonth[] = {
        31, 28, 31, 30, 31, 30, 31,
        31, 30, 31, 30, 31
        };

    if( month < 1 || month > 12 )
        return -1;

    if( month == 2 && IsLeapYear(year) )
    {
        days = 29;
        return 0;
    }
    days = daysInMonth[month - 1];
    return 0;
}

enum EnumToken
{
    tokMonthDay,
    tokNearWeekday,
    tokLastMonthDay,
    tokRangeStep,
    tokDayOfWeek,
    tokNthDayOfWeek,
    tokLastDayOfWeek,
};

struct CronToken
{
    EnumToken iType = tokMonthDay;
    int iValue = 0;
    int iStep = 1; // For range step
    std::unique_ptr<CronToken> pStart;
    std::unique_ptr<CronToken> pEnd;
    CronToken()
    {}

    CronToken( const CronToken& t )
    {
        auto& r = const_cast<CronToken&>( t );
        iType = r.iType;
        iValue = r.iValue;
        iStep = r.iStep;
        pStart = std::move( r.pStart );
        pEnd = std::move( r.pEnd );
    }
};

std::unique_ptr<CronToken> ParseMonthDayTokens(
    const std::string& strCronExpr,
    int maxval, std::tm& now )
{
    std::regex oNwd( "([0-9][0-9]?)[Ww]" );
    std::regex oLmd( "[Ll](-[0-9][0-9]?)?" );
    std::regex oRange( "[*]/([0-9][0-9]?)" );
    std::regex oMonthDay( "([0-9][0-9]?)" );

    std::vector<std::pair<std::regex, EnumToken>> regexes = {
        {oNwd, tokNearWeekday},
        {oLmd, tokLastMonthDay},
        {oRange, tokRangeStep},
        {oMonthDay, tokMonthDay}
    };

    std::unique_ptr<CronToken> pRoot;
    char cOp = 0;
    int iLen = 0;
    std::string strText = strCronExpr;
    do{
        for( const auto& pair : regexes )
        {
            std::smatch s;
            const std::regex& regex = pair.first;
            EnumToken type = pair.second;
            CronToken token;
            if( std::regex_search(strText, s, regex))
            {
                std::string matched = s[0];
                token.iType = tokMonthDay;
                if( type == tokNearWeekday )
                {
                    if( cOp == '/' )
                    {
                        throw std::runtime_error(
                            "Error unexpected near-weekday value");
                    }
                    std::tm oTime = {0};
                    oTime.tm_year = now.tm_year;
                    oTime.tm_mon = now.tm_mon;
                    oTime.tm_mday = mystoi(s[1]);
                    if( mktime( &oTime ) == -1 )
                    {
                        throw std::runtime_error(
                            "Error near-weekday date out of range");
                    }
                    if( oTime.tm_wday == 0 ||
                        oTime.tm_wday == 6 )
                    {
                        // If it's Saturday or Sunday,
                        // adjust to the nearest
                        // weekday
                        if( oTime.tm_wday == 0 ) // Sunday
                            oTime.tm_mday += 1; // Move to next Monday
                        else // Saturday
                            oTime.tm_mday -= 1; // Move to Friday
                    }

                    token.iValue = oTime.tm_mday;
                    if( token.iValue < 1 ||
                        token.iValue > maxval )
                    {
                        throw std::runtime_error(
                            "Error near-weekday value out of range");
                    }
                }
                else if( type == tokLastMonthDay )
                {
                    if( cOp == '/' )
                    {
                        throw std::runtime_error(
                            "Error unexpected last-month-day value");
                    }
                    token.iValue = maxval; // Last day of month
                    if( s.size() > 1 && !s[1].str().empty() )
                    {
                        int offset = mystoi(s[1].str().substr(1));
                        if( offset >= maxval )
                        {
                            throw std::runtime_error(
                                "Error last-month-day offset out of range");
                        }
                        token.iValue -= offset; // Adjust to the last day minus offset
                    }
                }
                else if( type == tokRangeStep )
                {
                    if( s.size() < 2 )
                    {
                        throw std::runtime_error(
                            "Error range-step format");
                    }
                    token.iType = tokRangeStep;
                    token.iStep = mystoi(s[1]);
                    if( token.iStep < 1 ||
                        token.iStep > maxval )
                    {
                        throw std::runtime_error(
                            "Error range-step value out of range");
                    }
                    CronToken oStart, oEnd;
                    oEnd.iType = oStart.iType = tokMonthDay;
                    oStart.iValue = 1;
                    oEnd.iValue = maxval;
                    token.pStart.reset( new CronToken( oStart ) );
                    token.pEnd.reset( new CronToken(oEnd) );
                    if( matched.size() != strText.size() )
                    {
                        throw std::runtime_error(
                            "Error syntax error in cron expression");
                    }
                }
                else if( type == tokMonthDay )
                {
                    if( s.size() < 2 )
                    {
                        throw std::runtime_error(
                            "Error month-day format");
                    }
                    token.iValue = mystoi(s[1]);
                    if( token.iValue < 1 ||
                        token.iValue > maxval )
                    {
                        throw std::runtime_error(
                            "Error month-day value out of range");
                    }
                }
                else
                {
                    throw std::runtime_error(
                        "Error unexpected token in cron expression" );
                }

                iLen += matched.size();
                if( cOp == 0 )
                {
                    pRoot.reset( new CronToken(token) );
                }
                else if( cOp == '-' )
                {
                    CronToken oRange;
                    oRange.iType = tokRangeStep;
                    oRange.pStart = std::move( pRoot );
                    oRange.pEnd.reset( new CronToken(token) );
                    pRoot.reset( new CronToken(oRange));
                    if( token.iValue < pRoot->pStart->iValue )
                    {
                        throw std::runtime_error(
                            "Error range-value out of bounds");
                    }
                }
                else if( cOp == '/' )
                {
                    pRoot->iStep = token.iValue;
                }
                break;
            }
        }

        if( iLen == strCronExpr.size() )
            return pRoot;

        strText = strCronExpr.substr( iLen );
        if( strText[ 0 ] == '-' && cOp == 0 )
        {
            iLen++;
            cOp = '-';
            strText = strText.substr(1);
            continue;
        }
        if( strText[ 0 ] == '/' && cOp == '-' )
        {
            iLen++;
            cOp = '/';
            strText = strText.substr(1);
            continue;
        }

        throw std::runtime_error(
            "Error unexpected content in cron expression" );

    }while( true );

    return pRoot;
}

void FillMonthDaySet(
    std::unique_ptr<CronToken>& pRoot,
    std::unordered_set< uint8_t > result )
{
    if( !pRoot )
        return;

    if( pRoot->iType == tokMonthDay )
    {
        result.insert( pRoot->iValue );
    }
    else if( pRoot->iType == tokRangeStep )
    {
        for( int i = pRoot->pStart->iValue;
            i <= pRoot->pEnd->iValue;
            i += pRoot->iStep )
            result.insert(i);
    }
    return;
}

std::unordered_set<uint8_t> ParseDayOfMonth(
    const std::string& field,
    uint8_t minval, uint8_t maxval,
    std::tm& now )
{
    std::unordered_set<uint8_t> result;
    std::stringstream ss(field);
    std::string part;
    while (std::getline(ss, part, ','))
    {
        if( part == "?" )
        {
            if( field.size() > 1 )
            {
                throw std::runtime_error(
                    "Error '?' can only be used alone");
            }
            for( int i = minval; i <= maxval; ++i )
                result.insert(i);
            continue;
        }
        else if( part == "*" )
        {
            if( field.size() > 1 )
            {
                throw std::runtime_error(
                    "Error ',' is unexpected after '*'");
            }
            for (int i = minval; i <= maxval; ++i)
                result.insert(i);
            continue;
        }
        std::unique_ptr<CronToken> pRoot =
            ParseMonthDayTokens( part, maxval, now );
        FillMonthDaySet( pRoot, result );
    }
    return result;
}

std::unique_ptr<CronToken> ParseWeekDayTokens(
    const std::string& strCronExpr,
    int maxmdays, std::tm& now )
{
    std::regex oLwd( "([0-6])[Ll]" );
    std::regex oNwd( "([0-6])#([1-5])" );
    std::regex oRange( "[*]/([1-7])" );
    std::regex oDayOfWeek( "([0-6])" );

    std::vector<std::pair<std::regex, EnumToken>> regexes =
    {
        {oLwd, tokLastDayOfWeek},
        {oNwd, tokNthDayOfWeek},
        {oRange, tokRangeStep},
        {oDayOfWeek, tokDayOfWeek}
    };

    char cOp = 0;
    int iLen = 0;
    std::unique_ptr<CronToken> pRoot;
    std::string strText = strCronExpr;
    do{
        for( const auto& pair : regexes )
        {
            std::smatch s;
            const std::regex& regex = pair.first;
            EnumToken type = pair.second;
            CronToken token;
            if( std::regex_search(strText, s, regex))
            {
                std::string matched = s[0];
                token.iType = tokDayOfWeek;
                if( type == tokLastDayOfWeek )
                {
                    if( cOp == '/' )
                    {
                        throw std::runtime_error(
                            "Error unexpected near-weekday value");
                    }

                    int wday = mystoi(s[1]);
                    if( maxmdays - 7 >= now.tm_mday )
                        token.iValue = -1;
                    else
                        token.iValue = wday;
                }
                else if( type == tokNthDayOfWeek )
                {
                    if( cOp == '/' )
                    {
                        throw std::runtime_error(
                            "Error unexpected last-month-day value");
                    }
                    int wday = mystoi(s[1]);
                    int nth = mystoi(s[2]);

                    std::tm oTime = {0};
                    oTime.tm_year = now.tm_year;
                    oTime.tm_mon = now.tm_mon;
                    oTime.tm_mday = 1; // Start from the first day of the month
                    if( mktime( &oTime ) == -1 )
                    {
                        throw std::runtime_error(
                            "Error nth-day-of-week date out of range");
                    }

                    int startWday = oTime.tm_wday; // Get the weekday of the first day of the month

                    int iLow = 1 + (nth - 1) * 7;
                    int iHigh = iLow + 6;
                    if( iLow > maxmdays )
                    {
                        throw std::runtime_error(
                            "Error nth-day-of-week value out of range");
                    }
                    if( iHigh > maxmdays )
                        iHigh = maxmdays;
                    if( now.tm_mday < iLow || now.tm_mday > iHigh )
                        token.iValue = -1;
                    else if( nth < 5 )
                        token.iValue = wday;
                    else
                    {
                        // nth == 5
                        int diff = iHigh - iLow;
                        if( wday == startWday )
                        {
                            token.iValue = wday;
                        }
                        else if( wday > startWday && wday - startWday <= diff )
                        {
                            token.iValue = wday;
                        }
                        else if( wday < startWday && wday + 7 - startWday <= diff )
                        {
                            token.iValue = wday;
                        }
                        else
                        {
                            token.iValue = -1;
                        }
                    }
                    token.iType = tokNthDayOfWeek;
                    token.iStep = nth | ( startWday << 8 ) | ( maxmdays << 16 );
                }
                else if( type == tokRangeStep )
                {
                    if( s.size() < 2 )
                    {
                        throw std::runtime_error(
                            "Error range-step format");
                    }
                    token.iType = tokRangeStep;
                    token.iStep = mystoi(s[1]);
                    if( token.iStep < 1 || token.iStep > 6 )
                    {
                        throw std::runtime_error(
                            "Error range-step value out of range");
                    }
                    CronToken oStart, oEnd;
                    oEnd.iType = oStart.iType = tokDayOfWeek;
                    oStart.iValue = 0;
                    oEnd.iValue = 6;
                    token.pStart.reset( new CronToken( oStart ) );
                    token.pEnd.reset( new CronToken(oEnd) );
                    if( matched.size() != strText.size() )
                    {
                        throw std::runtime_error(
                            "Error syntax error in cron expression");
                    }
                }
                else if( type == tokDayOfWeek )
                {
                    if( s.size() < 2 )
                    {
                        throw std::runtime_error(
                            "Error month-day format");
                    }
                    token.iValue = mystoi(s[1]);
                    if( token.iValue < 0 || token.iValue > 6 )
                    {
                        throw std::runtime_error(
                            "Error month-day value out of range");
                    }
                }
                else
                {
                    throw std::runtime_error(
                         "Error unexpected token in cron expression" );
                }

                iLen += matched.size();
                if( cOp == 0 )
                {
                    pRoot.reset( new CronToken(token) );
                }
                else if( cOp == '-' )
                {
                    CronToken oRange;
                    oRange.iType = tokRangeStep;
                    oRange.pStart = std::move( pRoot );
                    oRange.pEnd.reset( new CronToken(token) );
                    oRange.iStep = 1; // Default step for range
                    if( oRange.pStart->iType == tokDayOfWeek ||
                        oRange.pEnd->iType == tokDayOfWeek )
                    {
                        if( token.iValue < oRange.pStart->iValue )
                        {
                            throw std::runtime_error(
                                "Error range-value out of bounds");
                        }
                    }
                    pRoot.reset( new CronToken(oRange) );
                }
                else if( cOp == '/' )
                {
                    pRoot->iStep = token.iValue;
                }
                break;
            }
        }

        if( iLen == strCronExpr.size() )
            return pRoot;

        strText = strCronExpr.substr( iLen );
        if( strText[ 0 ] == '-' && cOp == 0 )
        {
            iLen++;
            cOp = '-';
            strText = strText.substr(1);
            continue;
        }
        if( strText[ 0 ] == '/' && cOp == '-' )
        {
            iLen++;
            cOp = '/';
            strText = strText.substr(1);
            continue;
        }

        throw std::runtime_error(
            "Error unexpected content in cron expression" );

    }while( true );
    return pRoot;
}

void FillWeekDaySet(
    std::unique_ptr<CronToken>& pRoot,
    std::unordered_set<uint8_t>& result,
    int startWday )
{
    do{
        if( !pRoot )
            return;

        if( pRoot->iType == tokDayOfWeek )
        {
            if( pRoot->iValue >= 0 && pRoot->iValue <= 6 )
                result.insert( pRoot->iValue );
        }
        else if( pRoot->iType == tokRangeStep )
        {
            std::unique_ptr<CronToken>& pStart = pRoot->pStart;
            std::unique_ptr<CronToken>& pEnd = pRoot->pEnd;
            int iStep = pRoot->iStep;
            if( pStart->iType == tokDayOfWeek &&
                pEnd->iType == tokDayOfWeek )
            {
                if( pStart->iValue > pEnd->iValue )
                {
                    throw std::runtime_error(
                         "Error day-of-week range is invalid");
                }
                for( int i = pStart->iValue;
                    i <= pEnd->iValue; i += iStep )
                    result.insert(i);
            }
            else if( pStart->iType != pEnd->iType ) 
            {
                throw std::runtime_error(
                    "Error day-of-week range is invalid");
            }
            else if( pStart->iType == tokNthDayOfWeek )
            {
                // Handle nth day of week
                int nth = ( pStart->iStep & 0xff );
                int nth2 = ( pEnd->iStep & 0xff );
                if( nth2 - nth > 1 )
                {
                    throw std::runtime_error(
                         "Error range of two nth-day-of-week "
                         "cannot exceed one week");
                }
                if( nth == nth2 )
                {
                    if( pStart->iValue < 0 || pEnd->iValue < 0 )
                        break;
                    if( pStart->iValue >= pEnd->iValue )
                    {
                        throw std::runtime_error(
                             "Error invalid range value");
                    }
                    for( int i = pStart->iValue; i <= pEnd->iValue; i += iStep )
                        result.insert(i);
                }
                else if( pStart->iValue < 0 && pEnd->iValue < 0 )
                {
                    break;
                }
                else if( pStart->iValue < 0 )
                {
                    int startWday = ( ( pEnd->iStep >> 8 ) & 0xff );
                    int upperBound = pEnd->iValue;
                    if( startWday > upperBound )
                        upperBound += 7;
                    for( int i = startWday; i < upperBound; i += iStep )
                        result.insert( i % 7 );
                }
                else if( pEnd->iValue < 0 )
                {
                    int upperBound = ( ( pStart->iStep >> 8 ) & 0xff );
                    if( upperBound < pStart->iValue )
                        upperBound += 7;
                    for( int i = pStart->iValue; i < upperBound; i += iStep )
                        result.insert( i % 7 );
                }
            }
            else
            {
                throw std::runtime_error(
                    "Error unexpected token in cron expression");
            }
        }
        else if( pRoot->iType == tokNthDayOfWeek)
        {
            if( pRoot->iValue >= 0 )
                result.insert( pRoot->iValue );
        }
    }while( 0 );
    return;
}
std::unordered_set<uint8_t> ParseDayOfWeek(
    const std::string& field,
    uint8_t minval, uint8_t maxval,
    std::tm& now )
{
    std::unordered_set<uint8_t> result;
    std::stringstream ss(field);
    std::string part;
    while (std::getline(ss, part, ','))
    {
        if( part == "?" )
        {
            if( field.size() > 1 )
                throw std::runtime_error("Error '?' can only be used alone");
            for( int i = minval; i <= maxval; ++i )
                result.insert(i);
            continue;
        }
        else if( part == "*" )
        {
            if( field.size() > 1 )
                throw std::runtime_error("Error ',' is unexpected after '*'");
            for (int i = minval; i <= maxval; ++i)
                result.insert(i);
            continue;
        }
        uint8_t maxmdays = 0;
        int ret = GetDaysInMonth( maxmdays,
            now.tm_mon + 1, now.tm_year + 1900 );
        if( ret < 0 )
            throw std::runtime_error("Error get days of current month" );
        std::unique_ptr<CronToken> pRoot =
            ParseWeekDayTokens( part, maxmdays, now );
        FillWeekDaySet( pRoot, result, maxmdays );
    }
    return result;
}
CronSchedule ParseCron(const std::string& expr, std::tm& targetDate )
{
    std::stringstream ss(expr);
    std::string field;
    std::vector<std::string> fields;
    while (ss >> field) fields.push_back(field);
    if (fields.size() < 6)
        throw std::runtime_error("Cron expression must have at least 6 fields");
    CronSchedule sched;
    sched.second = ParseField(fields[0], 0, 59);
    sched.minute = ParseField(fields[1], 0, 59);
    sched.hour = ParseField(fields[2], 0, 23);
    sched.month = ParseField(fields[4], 1, 12);
    
    uint8_t maxdays = 0;
    int ret = GetDaysInMonth( maxdays,
        targetDate.tm_mon + 1,
        targetDate.tm_year + 1900 );
    if( ret < 0 )
        throw std::runtime_error("Error get days of current month" );
    sched.day = ParseDayOfMonth(
        fields[3], 1, maxdays, targetDate );
    sched.weekday = ParseDayOfWeek(
        fields[5], 0, 6, targetDate); // 0=Sunday
    if( fields.size() == 6 )
        fields.push_back( "*" );
    sched.year = ParseYear(fields[6], 1970, 2100);
    sched.SetInitialized();
    return sched;
}

bool Matches(
    const CronSchedule& sched,
    const std::tm& tm )
{
    if( !sched.IsIntitialzed() )
        return false;
    return sched.second.count(tm.tm_sec) &&
           sched.minute.count(tm.tm_min) &&
           sched.hour.count(tm.tm_hour) &&
           sched.day.count(tm.tm_mday) &&
           sched.month.count(tm.tm_mon + 1) &&
           sched.weekday.count(tm.tm_wday) &&
           sched.year.count(tm.tm_year + 1900 );
}
