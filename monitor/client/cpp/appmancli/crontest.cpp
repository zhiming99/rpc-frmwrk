/*
 * =====================================================================================
 *
 *       Filename:  crontest.cpp
 *
 *    Description:  implementation of a tester of cron expression
 *
 *        Version:  1.0
 *        Created:  08/08/2025 08:38:30 PM
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
#include "cronexpr.h"
#include <atomic>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

std::atomic<bool> g_bExit = { false };

void SignalHandler( int signum )
{
    if( signum == SIGINT )
        g_bExit = true;
}

int main()
{
    auto iOldSig = signal( SIGINT, SignalHandler );
    // Every 5 minutes, 8-18h, Mon-Fri
    std::vector< std::string > vecExpr = {
        // "0 */5 8-23 * * 1-5",
        //"* * 17-23 * * 3#4,5l",
        // "0 * 17-23 ? * 5#1,5l",
        // weekday's non-weekly range, from first Saturday 
        // of a month to the last Tuesday of a month.
        // "0 * * ? * 6#4-2l",
        // month day cloaked weekday if not ignored
        // "0 * * * * 6#3-2l",
        "0 * * 1w 1 ? 2026",
        };

    std::string expr;

    try{
        CronSchedules arrScheds[ vecExpr.size() ];
        std::time_t t = std::time(nullptr);
        std::tm now = *std::localtime(&t);
        for( int i = 0; i < vecExpr.size(); i++ )
        {
            CronSchedules& sched = arrScheds[ i ];
            expr = vecExpr[ i ];
            sched.SetExpression( expr );
            sched.Start( now );
        }

        while( !g_bExit )
        {
            for( int i = 0; i < vecExpr.size(); i++ )
            {
                expr = vecExpr[ i ];
                CronSchedules& sched = arrScheds[ i ];
                std::time_t t = std::time(nullptr);
                std::tm now = *std::localtime(&t);
                std::cout << "Now: " << std::asctime(&now);
                std::cout << "Matches cron '" << expr << "': "
                  << (sched.Matches( now) ? "yes" : "no") << std::endl;
                if( sched.m_dwNextRun != 0xFFFFFFFF )
                {
                    std::tm tm;
                    localtime_r( &sched.m_dwNextRun, &tm );
                    std::cout << "Next run: "
                        << asctime( &tm ) << std::endl;
                }
                else
                {
                    std::cout << "Next run: unknown"
                        << std::endl;
                }
            }
            sleep( 3 );
        }
    }
    catch( std::runtime_error& e )
    {
        std::cerr << e.what();
        std::cerr << " in cron expression: " << expr << std::endl;
    }
    signal( SIGINT, iOldSig );
    return 0;
}
