#include <iostream>
#include <vector>
#include <unordered_set>
#include <sstream>
#include <ctime>
#include <algorithm>
#include <stdint.h>
#include "time.h"
#include <regex>

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
            int step = std::stoi(part.substr(slash + 1));
            int start = minval, end = maxval;
            if (base != "*") {
                size_t dash = base.find('-');
                if (dash != std::string::npos) {
                    start = std::stoi(base.substr(0, dash));
                    end = std::stoi(base.substr(dash + 1));
                } else {
                    start = end = std::stoi(base);
                }
            }
            for (int i = start; i <= end; ++i)
                if ((i - minval) % step == 0)
                    result.insert(i);
        } else if (part.find('-') != std::string::npos) {
            size_t dash = part.find('-');
            int start = std::stoi(part.substr(0, dash));
            int end = std::stoi(part.substr(dash + 1));
            for (int i = start; i <= end; ++i)
                result.insert(i);
        } else {
            result.insert(std::stoi(part));
        }
    }
    return result;
}

#define ParseField ParseOneField<uint8_t>
#define ParseYear ParseOneField<uint16_t>


// Function to check if a year is a leap year
bool IsLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

// Function to get the number of days in a given month and year
int GetDaysInMonth( uint8_t& days, int month, int year )
{
    static int daysInMonth[] =
        {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

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

int GetCurTime( std::tm& oTime )
{
    time_t rt;
    time( &rt );
    if( localtime_r(&rt, &oTime ) == nullptr )
        return -errno;
    return 0; 
}

enum EnumDOM
{
    tokMonthDay,
    tokNearWeekday,
    tokLastMonthDay,
    tokRangeStep
};

struct DOMToken
{
    EnumDOM iType = tokMonthDay;
    int iValue = 0;
    int iStep = 1; // For range step
    std::unique_ptr<DOMToken> pStart;
    std::unique_ptr<DOMToken> pEnd;
    DOMToken()
    {}

    DOMToken( const DOMToken& t )
    {
        auto r = const_cast<DOMToken&>( t );
        iType = r.iType;
        iValue = r.iValue;
        iStep = r.iStep;
        pStart.reset( r.pStart.release() );
        pEnd.reset( r.pEnd.release() );
    }
};

std::unique_ptr<DOMToken> ParseMonthDayTokens(
    const std::string& strCronExpr,
    int maxval, std::tm& now )
{
    std::regex oNwd( "([0-9][0-9]?)W" );
    std::regex oLmd( "L(-[0-9][0-9]?)?" );
    std::regex oRange( "[*]/([0-9][0-9]?)" );
    std::regex oMonthDay( "([0-9][0-9]?)" );

    std::vector<std::pair<std::regex, EnumDOM>> regexes = {
        {oNwd, tokNearWeekday},
        {oLmd, tokLastMonthDay},
        {oRange, tokRangeStep},
        {oMonthDay, tokMonthDay}
    };

    std::unique_ptr<DOMToken> pRoot;
    char cOp = 0;
    int iLen = 0;
    std::string strText = strCronExpr;
    do{
        for( const auto& pair : regexes )
        {
            std::smatch s;
            const std::regex& regex = pair.first;
            EnumDOM type = pair.second;
            DOMToken token;
            if( std::regex_match(strText, s, regex))
            {
                std::string matched = s[0];
                token.iType = tokMonthDay;
                if( type == tokNearWeekday )
                {
                    if( cOp == '/' )
                        throw std::runtime_error("Error unexpected near-weekday value");
                    std::tm oTime = {0};
                    oTime.tm_year = now.tm_year;
                    oTime.tm_mon = now.tm_mon;
                    oTime.tm_mday = std::stoi(s[1]);
                    if( mktime( &oTime ) == -1 )
                        throw std::runtime_error("Error near-weekday date out of range");
                    if( oTime.tm_wday == 0 || oTime.tm_wday == 6 )
                    {
                        // If it's Saturday or Sunday, adjust to the nearest weekday
                        if( oTime.tm_wday == 0 ) // Sunday
                            oTime.tm_mday -= 2; // Move to Friday
                        else // Saturday
                            oTime.tm_mday -= 1; // Move to Friday
                    }

                    token.iValue = oTime.tm_mday;
                    if( token.iValue < 1 || token.iValue > maxval )
                        throw std::runtime_error("Error near-weekday value out of range");
                }
                else if( type == tokLastMonthDay )
                {
                    if( cOp == '/' )
                        throw std::runtime_error("Error unexpected last-month-day value");
                    token.iValue = maxval; // Last day of month
                    if( s.size() > 1 && !s[1].str().empty() )
                    {
                        int offset = std::stoi(s[1].str().substr(1));
                        if( offset >= maxval )
                            throw std::runtime_error("Error last-month-day offset out of range");
                        token.iValue -= offset; // Adjust to the last day minus offset
                    }
                }
                else if( type == tokRangeStep )
                {
                    if( s.size() < 2 )
                        throw std::runtime_error("Error range-step format");
                    token.iType = tokRangeStep;
                    token.iStep = std::stoi(s[1]);
                    if( token.iStep < 1 || token.iStep > maxval )
                        throw std::runtime_error("Error range-step value out of range");
                    DOMToken oStart, oEnd;
                    oEnd.iType = oStart.iType = tokMonthDay;
                    oStart.iValue = 1;
                    oEnd.iValue = maxval;
                    token.pStart = std::make_unique<DOMToken>( oStart );
                    token.pEnd = std::make_unique<DOMToken>(oEnd);
                    if( matched.size() != strText.size() )
                        throw std::runtime_error("Error syntax error in cron expression");
                }
                else if( type == tokMonthDay )
                {
                    if( s.size() < 2 )
                        throw std::runtime_error("Error month-day format");
                    token.iValue = std::stoi(s[1]);
                    if( token.iValue < 1 || token.iValue > maxval )
                        throw std::runtime_error("Error month-day value out of range");
                }
                else
                    throw std::runtime_error( "Error unexpected token in cron expression" );

                iLen += matched.size();
                if( cOp == 0 )
                {
                    pRoot = std::make_unique<DOMToken>(token);
                }
                else if( cOp == '-' )
                {
                    DOMToken oRange;
                    oRange.iType = tokRangeStep;
                    oRange.pStart = std::move( pRoot );
                    oRange.pEnd = std::make_unique<DOMToken>(token);
                    pRoot = std::make_unique<DOMToken>(oRange);
                    if( token.iValue < pRoot->pStart->iValue )
                        throw std::runtime_error("Error range-value out of bounds");
                }
                else if( cOp == '/' )
                {
                    pRoot->iStep = token.iValue;
                }
                break;
            }
        }

        if( iLen == strText.size() )
            return pRoot;

        strText = strText.substr( iLen );
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
            strText = strText.substr(1);
            continue;
        }

        throw std::runtime_error( "Error unexpected content in cron expression" );

    }while( true );
    return pRoot;
}

void FillMonthDaySet(
    std::unique_ptr<DOMToken>& pRoot,
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
        std::unique_ptr<DOMToken> pRoot =
            ParseMonthDayTokens( part, maxval, now );
        FillMonthDaySet( pRoot, result );
    }
    return result;
}

enum EnumDOW
{
    tokWeekDay,
    tokNthWeekday,
    tokLastWeekday,
    //tokRangeStep
};

struct CronSchedule {
    std::unordered_set<uint8_t> second, minute, hour, day, month, weekday;
    std::unordered_set<uint16_t> year;
};

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
    int ret = GetDaysInMonth(
        maxdays, targetDate.tm_mon, targetDate.tm_year );
    if( ret < 0 )
        throw std::runtime_error("Error get days of current month" );
    sched.day = ParseDayOfMonth(fields[3], 1, maxdays, targetDate );
    sched.weekday = ParseField(fields[5], 0, 6); // 0=Sunday
    if( fields.size() == 6 )
        fields.push_back( "*" );
    sched.year = ParseYear(fields[6], 1970, 2100);
    return sched;
}

bool matches(const CronSchedule& sched, const std::tm& tm) {
    return sched.second.count(tm.tm_sec) &&
           sched.minute.count(tm.tm_min) &&
           sched.hour.count(tm.tm_hour) &&
           sched.day.count(tm.tm_mday) &&
           sched.month.count(tm.tm_mon + 1) &&
           sched.weekday.count(tm.tm_wday) &&
           sched.year.count(tm.tm_year );
}

int main() {
    std::string expr = "0 */5 8-18 * * 1-5"; // Every 5 minutes, 8-18h, Mon-Fri

    std::time_t t = std::time(nullptr);
    std::tm now = *std::localtime(&t);
    CronSchedule sched = ParseCron(expr, now);

    std::cout << "Now: " << std::asctime(&now);
    std::cout << "Matches cron '" << expr << "': "
              << (matches(sched, now) ? "yes" : "no") << std::endl;
    return 0;
}
