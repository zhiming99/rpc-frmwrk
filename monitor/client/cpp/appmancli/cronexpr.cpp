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
#include <set>
#include <sstream>
#include <ctime>
#include <algorithm>
#include <stdint.h>
#include "time.h"
#include <regex>
#include <memory>
#include "cronexpr.h"

PBFIELD CronSchedule::ignored =
    PBFIELD( new std::ordered_set< uint8_t >() );

PWFIELD CronSchedule::accepted =
    PWFIELD( new std::ordered_set< uint16_t >() );

bool CronSchedule::IsEmpty() const
{
    return !second || second->empty() ||
        !minute || minute->empty() ||
        !hour || hour->empty() ||
        !day || day->empty() ||
        !day || month->empty() ||
        !year || ( year != CronSchedule::accepted && year->empty() );
}

static int mystoi( const std::string& str )
{
    for( auto& elem : str )
    {
        if( elem >= '0' && elem <= '9' )
            continue;
        throw std::runtime_error(
            "Error not a valid value" );
    }
    return std::stoi( str );
}


// Function to trim leading whitespace and newlines
void TrimLeft(std::string &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

// Function to trim trailing whitespace and newlines
void TrimRight(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// Function to trim both leading and trailing whitespace and newlines
void TrimString(std::string& s)
{
    TrimLeft(s);
    TrimRight(s);
}

template< class T >
std::ordered_set<T> ParseOneField(
    const std::string& field, T minval, T maxval)
{
    std::ordered_set<T> result;
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

PBFIELD ParseField( const std::string& field,
    uint8_t minval, uint8_t maxval)
{
    std::ordered_set< uint8_t > result = 
        ParseOneField( field, minval, maxval );
    return std::make_shared< std::ordered_set< uint8_t > >( result );
}

PWFIELD ParseYear( const std::string& field,
    uint16_t minval, uint16_t maxval)
{
    if( field == "*" )
        return CronSchedule::accepted;
    std::ordered_set< uint16_t > result = 
        ParseOneField( field, minval, maxval );
    return std::make_shared< std::ordered_set< uint16_t > >( result );
}

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
    int32_t iValue = 0;
    int8_t iStep = 1; // For range step
    int8_t iNthWeek = 0;
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
        iNthWeek = r.iNthWeek;
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
    std::ordered_set< uint8_t >& result )
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

 PBFIELD ParseDayOfMonth(
    const std::string& field,
    uint8_t minval, uint8_t maxval,
    std::tm& now )
{
    PBFIELD result( new std::ordered_set< uint8_t>() );
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
            return CronSchedule::ignored;
        }
        else if( part == "*" )
        {
            if( field.size() > 1 )
            {
                throw std::runtime_error(
                    "Error ',' is unexpected after '*'");
            }
            for (int i = minval; i <= maxval; ++i)
                result->insert(i);
            return result;
        }
        std::unique_ptr<CronToken> pRoot =
            ParseMonthDayTokens( part, maxval, now );
        FillMonthDaySet( pRoot, *result );
    }
    return result;
}

std::unique_ptr<CronToken> ParseWeekDayTokens(
    const std::string& strCronExpr,
    int maxmdays )
{
    std::regex oLwd( "^([0-6])[Ll]" );
    std::regex oNwd( "^([0-6])#([1-5])" );
    std::regex oRange( "^[*]/([1-7])" );
    std::regex oDayOfWeek( "^([0-6])" );

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
                token.iType = type;
                if( type == tokLastDayOfWeek )
                {
                    if( cOp == '/' )
                    {
                        throw std::runtime_error(
                            "Error unexpected last-weekday value");
                    }

                    token.iValue = mystoi(s[1]);
                }
                else if( type == tokNthDayOfWeek )
                {
                    if( cOp == '/' )
                    {
                        throw std::runtime_error(
                            "Error unexpected nth-day-of-week value");
                    }
                    int wday = mystoi(s[1]);
                    int nth = mystoi(s[2]);

                    token.iNthWeek = nth;
                    token.iValue = wday;
                }
                else if( type == tokRangeStep )
                {
                    if( s.size() < 2 )
                    {
                        throw std::runtime_error(
                            "Error range-step format");
                    }
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
        else if( strText[ 0 ] == '/' && cOp == '-' )
        {
            iLen++;
            cOp = '/';
            strText = strText.substr(1);
            continue;
        }

        throw std::runtime_error(
            "Error unexpected content in "
            "cron expression" );

    }while( true );
    return pRoot;
}

inline int CalculateMday(
    int iWday, int startWday, int iWeek )
{
    int mday;
    if( iWday < startWday )
        mday = iWday + 7 - startWday;
    else
        mday = iWday - startWday;
    mday += 7 * iWeek;
    return mday + 1;
}

void FillWeekDaySet(
    std::unique_ptr<CronToken>& pRoot,
    std::ordered_set<uint8_t>& result,
    int maxmdays,
    const std::tm& now )
{
    if( !pRoot )
        return;

    int iWeeks = maxmdays / 7;
    std::tm firstDay = now;
    firstDay.tm_mday = 1;
    mktime( &firstDay );
    int startWday = firstDay.tm_wday;
    for( int i = 0; i < iWeeks; i++ )
    {
        if( pRoot->iType == tokDayOfWeek )
        {
            int mday = CalculateMday(
                pRoot->iValue, startWday, i );
            if( mday > maxmdays )
                continue;
            result.insert( mday );
        }
        else if( pRoot->iType == tokNthDayOfWeek )
        {
            if( pRoot->iNthWeek != i + 1 )
                continue;
            if( pRoot->iValue < 0 )
                continue;
            int mday = CalculateMday(
                pRoot->iValue, startWday, i );
            if( mday <= maxmdays )
                result.insert( mday );
        }
        else if( pRoot->iType == tokLastDayOfWeek)
        {
            if( i != 4 )
                continue;
            int mday = CalculateMday(
                pRoot->iValue, startWday, i );
            if( maxmdays < mday )
            {
                mday = CalculateMday(
                    pRoot->iValue, startWday, i - 1 );
                result.insert( mday );
            }
        }
        else if( pRoot->iType == tokRangeStep )
        {
            std::unique_ptr<CronToken>& pStart = pRoot->pStart;
            std::unique_ptr<CronToken>& pEnd = pRoot->pEnd;
            int iStep = pRoot->iStep;
            if( pStart->iType == tokDayOfWeek &&
                pEnd->iType == tokDayOfWeek )
            {
                if( pStart->iValue >= pEnd->iValue )
                {
                    throw std::runtime_error(
                         "Error day-of-week range is invalid");
                }
                for( int iValue = pStart->iValue;
                    iValue <= pEnd->iValue;
                    iValue += iStep )
                {
                    int mday = CalculateMday(
                        iValue, startWday, i );
                    if( mday <= maxmdays )
                        result.insert( mday );
                }
                continue;
            }
            else
            {
                // non-weekly range
                int startMday = 0;
                int endMday = 0;

                if( pStart->iType == tokDayOfWeek )
                {
                    throw std::runtime_error(
                        "Error invalid week-day range");
                }

                if( pStart->iType == tokNthDayOfWeek )
                {
                    int mday = CalculateMday(
                        pStart->iValue, startWday,
                        pStart->iNthWeek - 1 );

                    if( mday > maxmdays )
                        throw std::runtime_error(
                            "Error invalid week-day range");
                    startMday = mday;
                }
                else if( pStart->iType == tokLastDayOfWeek)
                {
                    int mday = CalculateMday(
                        pStart->iValue, startWday, 4 );
                    if( mday > maxmdays )
                        mday = CalculateMday(
                            pStart->iValue, startWday, 3 );
                    startMday = mday;
                }

                if( pEnd->iType == tokDayOfWeek )
                {
                    throw std::runtime_error(
                        "Error invalid week-day range");
                }
                if( pEnd->iType == tokNthDayOfWeek )
                {
                    int mday = CalculateMday(
                        pEnd->iValue, startWday,
                        pEnd->iNthWeek - 1 );
                    if( mday > maxmdays )
                        mday = maxmdays;
                    endMday = mday;
                }
                else if( pEnd->iType == tokLastDayOfWeek)
                {
                    int mday = CalculateMday(
                        pEnd->iValue, startWday, 4 );
                    if( mday > maxmdays )
                        mday = CalculateMday(
                            pEnd->iValue, startWday, 3 );
                    endMday = mday;
                }
                if( startMday > endMday )
                    throw std::runtime_error(
                        "Error invalid week-day range");

                for( i = startMday; i <= endMday; i += iStep ) 
                    result.insert( i );
                break;
            }
        }
    }
    return;
}

PBFIELD ParseDayOfWeek(
    const std::string& field,
    uint8_t minval, uint8_t maxval,
    std::tm& now )
{
    PBFIELD result( new std::ordered_set<uint8_t>() );
    std::stringstream ss(field);
    std::string part;
    while (std::getline(ss, part, ','))
    {
        if( part == "?" )
        {
            if( field.size() > 1 )
                throw std::runtime_error(
                    "Error '?' can only be used alone");
            return CronSchedule::ignored;
        }
        else if( part == "*" )
        {
            if( field.size() > 1 )
                throw std::runtime_error(
                    "Error ',' is unexpected after '*'");
            for (int i = minval; i <= maxval; ++i)
                result->insert(i);
            return result;
        }
        uint8_t maxmdays = 0;
        int ret = GetDaysInMonth( maxmdays,
            now.tm_mon + 1, now.tm_year + 1900 );
        if( ret < 0 )
            throw std::runtime_error(
                "Error get days of current month" );

        std::unique_ptr<CronToken> pRoot =
            ParseWeekDayTokens( part, maxmdays );

        FillWeekDaySet( pRoot, *result, maxmdays, now );
    }
    return result;
}

CronSchedule CronSchedules::ParseCronInternal(
    std::tm& targetDate ) const
{
    std::string expr = m_strExpr;
    TrimString( expr );
    if( expr.empty() )
        throw std::runtime_error(
            "Error empty cron expression");
    std::stringstream ss(expr);
    std::string field;
    std::vector<std::string> fields;
    while (ss >> field) fields.push_back(field);
    if (fields.size() < 6)
        throw std::runtime_error(
            "Cron expression must have at least 6 fields");

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
        throw std::runtime_error(
            "Error get days of current month" );
    PBFIELD retSet = ParseDayOfMonth(
        fields[3], 1, maxdays, targetDate );
    sched.day = retSet;
    if( sched.day == CronSchedule::ignored )
    {
        retSet = ParseDayOfWeek(
            fields[5], 0, 6, targetDate); // 0=Sunday
        sched.day = retSet;
    }
    if( sched.day == CronSchedule::ignored )
        throw std::runtime_error(
            "Error both 'day' and 'week day' are ignored." );

    if( fields.size() == 6 )
        fields.push_back( "*" );

    sched.year = ParseYear(fields[6], 1970, 2100);
    sched.SetInitialized();
    sched.byCurMonth = targetDate.tm_mon;
    sched.wCurYear = targetDate.tm_year;
    return sched;
}

void CronSchedules::ParseCron( std::tm now )
{
    if( !m_oNext.IsEmpty() )
        m_oCurrent = m_oNext;
    else
        m_oCurrent = ParseCronInternal( now );
    std::tm tmNext = {0};
    tmNext.tm_mon = now.tm_mon + 1;
    tmNext.tm_year = now.tm_year;
    tmNext.tm_mday = 1;
    tmNext.tm_hour = tmNext.tm_sec = tmNext.tm_min = 0;
    m_oNext = ParseCronInternal( tmNext );
    m_dwNextUpdate = mktime( &tmNext );
}

int CronSchedules::Start( std::tm now )
{
    time_t tmRet = 0;
    m_dwLastRun = 0;
    m_dwNextRun = 0;
    ParseCron( now );
    FindNextRun( now, m_dwNextRun );
    return 0;
}

int CronSchedules::DoFindNextRun(
    const CronSchedule& sched,
    const CronSchedule* pSchedNext,
    const std::tm& now,
    time_t& tmRet )
{
    int ret = 0;
    std::tm tmNext = now;
    do{
        const CronSchedule* pSched = &sched;
        bool bCarry = false;
        auto itrSec = pSched->second->
            upper_bound( now.tm_sec );
        if( itrSec != pSched->second->end() )
        {
            tmNext.tm_sec = *itrSec;
        }
        else
        {
            bCarry = true;
            itrSec = pSched->second->begin();
            tmNext.tm_sec = *itrSec;
        }

        if( !bCarry && !pSched->minute->count( now.tm_min ) )
            bCarry = true;

        if( bCarry )
        {
            bCarry = false;
            auto itrMin = pSched->minute->
                upper_bound( now.tm_min );
            if( itrMin != pSched->minute->end() )
            {
                tmNext.tm_min = *itrMin;
            }
            else
            {
                bCarry = true;
                itrMin = pSched->minute->begin();
                tmNext.tm_min = *itrMin;
            }
        }

        if( !bCarry && !pSched->hour->count( now.tm_hour ) )
            bCarry = true;

        if( bCarry )
        {
            bCarry = false;
            auto itrHour = pSched->hour->
                upper_bound( now.tm_hour );
            if( itrHour != pSched->hour->end() )
            {
                tmNext.tm_hour = *itrHour;
            }
            else
            {
                bCarry = true;
                itrHour = pSched->hour->begin();
                tmNext.tm_hour = *itrHour;
            }
        }

        if( !bCarry && !pSched->day->count( now.tm_mday ) )
            bCarry = true;

        if( bCarry )
        {
            bCarry = false;
            auto itrDay = pSched->day->
                upper_bound( now.tm_mday );
            if( itrDay != pSched->day->end() )
            {
                tmNext.tm_mday = *itrDay;
            }
            else
            {
                bCarry = true;
            }
        }

        if( !bCarry )
        {
            if( pSched->GetCurMonth() + 1 < now.tm_mon )
            {
                ret = -ENOENT;
                break;
            }
            if( !pSched->month->count( now.tm_mon ) )
                bCarry = true;
        }

        if( bCarry )
        {
            bCarry = false;
            // current schedule is exhausted
            if( pSchedNext == nullptr ||
                pSchedNext->IsEmpty() )
            {
                ret = -ENOENT;
                break;
            }
            pSched = pSchedNext;

            auto itrDay = pSched->day->begin();
            if( itrDay != pSched->day->end() )
            {
                tmNext.tm_mday = *itrDay;
            }
            else
            {
                ret = -ENOENT;
                break;
            }
        }

        // assuming calling Matches occur everyday.
        uint8_t byMon =pSched->GetCurMonth();
        uint16_t wYear = pSched->GetCurYear();

        if( !pSched->month->count( byMon ) ) 
        {
            ret = -ENOENT;
            break;
        }
        tmNext.tm_mon = byMon;

        if( pSched->year == CronSchedule::accepted ||
            pSched->year->count( wYear + 1900 ) )
        {
            tmNext.tm_year = wYear;
            break;
        }
        ret = -ENOENT;

    }while( 0 );
    if( ret == 0 )
        tmRet = mktime( &tmNext );
    return ret;
}

int CronSchedules::FindNextRun(
    const std::tm& now,
    time_t& tmRet )
{
    int ret = 0;
    do{
        std::tm tm = now;
        ret = DoFindNextRun( m_oCurrent,
            &m_oNext, tm, tmRet );
        if( ret == -ENOENT )
            m_dwNextRun = 0xFFFFFFFF;

    }while( 0 );
    return ret;
}

bool CronSchedules::Matches(
    const std::tm& tmNow )
{
    const CronSchedule& sched = m_oCurrent;

    if( !sched.IsInitialized() )
        return false;
    
    std::tm tm = tmNow;
    time_t now = mktime( &tm );
    if( m_dwNextRun != 0xFFFFFFFF
        && m_dwNextRun > now  )
        return false;

    int ret = 0;
    if( m_dwNextRun == 0 )
    {
        bool bRet = true;
        do{
            bRet = sched.second->count(tm.tm_sec) &&
                   sched.minute->count(tm.tm_min) &&
                   sched.hour->count(tm.tm_hour) &&
                   sched.month->count(tm.tm_mon + 1);
            if( !bRet )
                break;

            bRet = sched.day->count(tm.tm_mday);
            if( !bRet )
                break;

            if( sched.year != sched.accepted )
            {
                bRet = sched.year->count(
                    tm.tm_year + 1900 );
                break;
            }
            bRet = true;
        }while( 0 );
        if( bRet )
            m_dwLastRun = mktime( &tm );

        FindNextRun( tm, m_dwNextRun );
        return bRet;
    }
    if( m_dwNextRun == 0xFFFFFFFF )
    {
        if( now >= m_dwNextUpdate )
        {
            std::tm tmNext;
            localtime_r( &now, &tmNext );
            ParseCron( tmNext );
            FindNextRun( tmNext, m_dwNextRun );
        }
        return false;
    }
    else if( now >= m_dwNextRun )
    {
        m_dwLastRun = now;
        if( now >= m_dwNextUpdate )
            ParseCron( tm );

        FindNextRun( tm, m_dwNextRun );
        return true;
    }
    // now < m_dwNextRun
    if( now >= m_dwNextUpdate )
        ParseCron( tm );
    return false;
}
