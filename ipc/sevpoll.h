/*
 * =====================================================================================
 *
 *       Filename:  sevpoll.h
 *
 *    Description:  declaration of the simplified event loop
 *
 *        Version:  1.0
 *        Created:  10/04/2018 02:27:53 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:  
 *
 * =====================================================================================
 */
#pragma once

#ifdef _USE_LIBEV
#include <json/json.h>
#include <vector>
#include <unistd.h>
#include <sys/syscall.h>
#include "namespc.h"
#include "defines.h"
#include "tasklets.h"
#include <poll.h>
#include <deque>

enum EnumAsyncEvent : guint8
{
    aevtInvalid,
    aevtWakeup,
    aevtRunSched,
    aevtRunAsync,
    aevtStop
};

#define UNDO_DEPTH 4
class CEvLoop;
class CSimpleEvPoll
{
    bool                                m_bStop; 
    stdrmutex                           m_oLock;
    CEvLoop*                            m_pLoop;

    protected:
    // pipe fd for async tasks
    gint32                              m_iPiper, m_iPipew; 

    // current timestamp
    std::atomic< guint64 >              m_qwNowUs;

    // timer map
    std::multimap< guint64, HANDLE >    m_mapTimer2Handle;

    // flags to indicate if the map is changed
    bool                                m_bIoAdd;
    bool                                m_bIoRemove;
    bool                                m_bTimeAdd;
    bool                                m_bTimeRemove;

    std::deque< guint8 >                m_queReadBuf;
    std::deque< guint8 >                m_queUndoBuf;

    gint32 StartStop( HANDLE hWatch,
        EnumSrcType iType,  bool bStart );

    gint32 UpdateTimeMaps(
        std::multimap< guint64, HANDLE >& mapActTimers );

    gint32 UpdateIoMaps(
        std::map< gint32, HANDLE >& mapFd2Handle );

    gint32 GetWaitTime( guint32& qwIntervalMs );

    gint32 CalcWaitTime(
        std::multimap< guint64, HANDLE >& mapActTimers,
        guint32& dwIntervalMs );

    gint32 UpdateTimeSource(
        HANDLE hWatch,
        guint64& qwNewTimeUs );

    gint32 HandleTimeout(
        std::multimap< guint64, HANDLE >& mapActTimers );

    gint32 HandleIoEvents(
        std::map< gint32, HANDLE >& mapActFds,
        pollfd* pPollInfo, gint32 iCount );

    gint32 BuildPollFds(
        std::map< gint32, HANDLE >& mapActFds,
        pollfd* pPollInfo, gint32& iCount );

    public:

    CSimpleEvPoll( const IConfigDb* pCfg );
    ~CSimpleEvPoll();

    inline stdrmutex& GetLock()
    { return m_oLock; }

    inline CEvLoop* GetLoop()
    { return m_pLoop; }

    CIoManager* GetIoMgr();

    gint32 RunLoop();

    gint32 StartSource(
        HANDLE hWatch, EnumSrcType iType );

    gint32 StopSource(
        HANDLE hWatch, EnumSrcType iType );

    gint32 RunSource(
        HANDLE hWatch,
        EnumSrcType iType,
        guint32 dwContext );

    inline guint64 NowUs()
    { 
        timespec ts;
        clock_gettime (CLOCK_MONOTONIC, &ts);
        m_qwNowUs = ( ( guint64 )ts.tv_sec ) * ( 1000 * 1000 ) +
            ts.tv_nsec / 1000;
        return m_qwNowUs;
    }

    inline guint64 NowUsFast()
    {
        return m_qwNowUs;
    }

    gint32 WakeupLoop(
        EnumAsyncEvent iEvent,
        guint32 dwContext = 0 );

    gint32 WriteAsyncData(
        guint8* pData, gint32 iSize );

    gint32 ReadAsyncEvent(
        EnumAsyncEvent& iEvent );

    gint32 ReadAsyncData(
        guint8* pData, guint32 iSize );

    gint32 AsyncDataUnwind(
        gint32 iSize );

    inline void SetStop( bool bStop )
    { m_bStop = bStop; }

    inline bool IsStopped()
    { return m_bStop; }
};

#endif
