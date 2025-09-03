/*
 * =====================================================================================
 *
 *       Filename:  cronexpr.h
 *
 *    Description:  declarations of the API for a cron expression parser
 *
 *        Version:  1.0
 *        Created:  23/07/2025 10:00:30 PM
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
#pragma once
#include <string>
#include <set>
#include <memory>
#include <regex>
#include <ctime>

#define ordered_set set

typedef std::shared_ptr<std::ordered_set< uint8_t >> PBFIELD;
typedef std::shared_ptr<std::ordered_set< uint16_t >> PWFIELD;

struct CronSchedule
{
    static PBFIELD ignored;
    static PWFIELD accepted;

    PBFIELD second={nullptr};
    PBFIELD minute={nullptr};
    PBFIELD hour={nullptr};
    PBFIELD day={nullptr};
    PBFIELD month={nullptr};
    PBFIELD weekday={nullptr};
    PWFIELD year={nullptr};

    bool bInit = false;
    bool IsInitialized() const
    { return bInit; }

    void SetInitialized()
    { bInit = true; }

    bool IsEmpty() const;
    CronSchedule()
    {}
    uint8_t byCurMonth = 0;
    uint16_t wCurYear = 0;

    uint8_t GetCurMonth() const
    { return byCurMonth; }

    uint16_t GetCurYear() const
    { return wCurYear; }

    CronSchedule( const CronSchedule& rhs )
    {
        second = rhs.second;
        minute = rhs.minute;
        hour = rhs.hour;
        if( rhs.day == ignored )
            day = ignored;
        else
        {
            day = rhs.day;
        }
        *month = *rhs.month;
        if( rhs.weekday == ignored )
            weekday = ignored;
        else
        {
            weekday = rhs.weekday;
        }
        year = rhs.year;
    }

    void DeepCopy( const CronSchedule& rhs )
    {
        second = PBFIELD(
            new std::ordered_set<uint8_t>() );
        *second = *rhs.second;
        minute = PBFIELD(
            new std::ordered_set< uint8_t >() );
        *minute = *rhs.minute;
        hour = PBFIELD(
            new std::ordered_set< uint8_t >() );
        *hour = *rhs.hour;
        if( rhs.day == ignored )
            day = ignored;
        else
        {
            day = PBFIELD(
                new std::ordered_set< uint8_t >() );
            *day = *rhs.day;
        }
        month = PBFIELD(
            new std::ordered_set< uint8_t >() );
        *month = *rhs.month;
        if( rhs.weekday == ignored )
            weekday = ignored;
        else
        {
            weekday = PBFIELD(
                new std::ordered_set< uint8_t >() );
            *weekday = *rhs.weekday;
        }
        if( rhs.year == accepted )
            year = accepted;
        else
        {
            year = PWFIELD(
                new std::ordered_set< uint16_t >() );
            *year = *rhs.year;
        }
    }
};

struct CronSchedules
{
    std::string m_strExpr;
    CronSchedule m_oCurrent;
    CronSchedule m_oNext;
    time_t  m_dwLastRun = 0;
    time_t m_dwNextRun = 0;
    time_t m_dwNextUpdate = 0;

    CronSchedules( const std::string& strExpr ) :
        m_strExpr( strExpr )
    {}

    CronSchedules()
    {}

    inline void SetExpression(
        const std::string& strExpr )
    { m_strExpr = strExpr; }

    /*
    * Parses a cron expression and returns a CronSchedule object.
    * The cron expression should be in the format:
    * "second minute hour day month weekday [year]"
    * where each field can contain:
    * - A single value (e.g., "5") 
    * - A range (e.g., "1-5")
    * - A list of values (e.g., "1,2,3")
    * - A step value (e.g. "1-10/2")
    * - A wildcard ("*")
    * - A question mark ("?") for "ignored", as used in 'day' field and
    * - The valid value ranges are : 'day' 1-31, 'month' 1-12, 'hour' 0-23,
    * 'min' 0-59, 'second' 0-59, year 1900-2100.
    * 'weekday' field, to skip parsing the field. It is invalid if both are
    * ignored or neither is ignored.
    * - A last weekday (e.g., "5L" for last Friday of a month)
    * - An nth weekday (e.g., "3#2" for the second Wednesday within a month)
    * - A last day of the month (e.g., "L" for last day)
    * - A day n days before the last day of the month (e.g., "L-3" for 3 days before the last day)
    * - A nearest weekday of the day of the month (e.g., "12w" for the nearest weekday to the 12th)
    * The function will throw std::runtime_error if the expression is invalid. 
    * The targetDate parameter is used to determine the current month and year for parsing.
    * If the year is not specified, it defaults to the current year.
    * @param targetDate A std::tm structure representing the current date and time.
    * @return A CronSchedule object representing the parsed cron expression.
    * @throws std::runtime_error if the cron expression is invalid.
    * Example:
    * CronSchedule sched = ParseCron("0 0-59/5 8-18 ? * 1-5", now);
    * This will create a schedule that runs every 5 minutes between 8 AM and 6 PM, Monday to Friday.   
    */
    void ParseCron( std::tm targetDate );
   /*
    * Checks if the given time matches the cron schedule.
    * @param tm A std::tm structure representing the time to check.
    * @return true if the time matches the schedule, false otherwise.
    * Example:
    * bool isMatch = Matches(sched, now); 
    * This will return true if the current time matches the cron schedule.
    */
    bool Matches( const std::tm& tm);

    int Start( std::tm now );

    private:
    int FindNextRun(
        const std::tm& now, time_t& tmRet );

    int DoFindNextRun(
        const CronSchedule& sched,
        const CronSchedule* pSchedNext,
        const std::tm& now,
        time_t& tmRet );

    CronSchedule ParseCronInternal(
        std::tm& targetDate ) const;
};
