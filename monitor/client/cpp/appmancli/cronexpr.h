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
    PWFIELD year;
    bool bInit = false;
    bool IsIntitialzed() const
    { return bInit; }
    void SetInitialized()
    { bInit = true; }
    time_t tmLastRun = 0;
    time_t tmNextRun = 0;
};

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
    * - A question mark ("?") for "no specific value"
    * - A last weekday (e.g., "5L" for last Friday of a month)
    * - An nth weekday (e.g., "3#2" for the second Wednesday within a month)
    * - A last day of the month (e.g., "L" for last day)
    * - A day n days before the last day of the month (e.g., "L-3" for 3 days before the last day)
    * - A nearest weekday of the day of the month (e.g., "12w" for the nearest weekday to the 12th)
    * The function will throw std::runtime_error if the expression is invalid. 
    * The targetDate parameter is used to determine the current month and year for parsing.
    * If the year is not specified, it defaults to the current year.
    * @param expr The cron expression to parse.
    * @param targetDate A std::tm structure representing the current date and time.
    * @return A CronSchedule object representing the parsed cron expression.
    * @throws std::runtime_error if the cron expression is invalid.
    * Example:
    * CronSchedule sched = ParseCron("0 0-59/5 8-18 ? * 1-5", now);
    * This will create a schedule that runs every 5 minutes between 8 AM and 6 PM, Monday to Friday.   
    */
CronSchedule ParseCron(
    const std::string& expr,
    std::tm& targetDate );

   /*
    * Checks if the given time matches the cron schedule.
    * @param sched The CronSchedule object to check against.
    * @param tm A std::tm structure representing the time to check.
    * @return true if the time matches the schedule, false otherwise.
    * Example:
    * bool isMatch = Matches(sched, now); 
    * This will return true if the current time matches the cron schedule.
    */
bool Matches(
    const CronSchedule& sched,
    const std::tm& tm);
