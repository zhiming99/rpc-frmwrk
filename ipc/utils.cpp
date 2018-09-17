/*
 * =====================================================================================
 *
 *       Filename:  utils.cpp
 *
 *    Description:  implementation of timer and workitem
 *
 *        Version:  1.0
 *        Created:  04/24/2016 04:14:49 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi(woodhead99@gmail.com )
 *   Organization:
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include "defines.h"
#include "frmwrk.h"
#include "utils.h"
#include <functional>

#define SecToMs( dwSec ) ( ( guint64 )( dwSec * 1000 ) )

using namespace std;
std::atomic<gint32> CWorkitemManager::WORK_ITEM::m_atmWorkitemId( 1 );
std::atomic<gint32> CTimerService::TIMER_ENTRY::m_atmTimerId( 1 );

CUtilities::CUtilities( const IConfigDb* pCfg ) :
    m_pWorkerThread( nullptr ),
    m_pWorkerThreadLongWait( nullptr ),
    m_bExit( false )
{
    gint32 ret = 0;
    SetClassId( Clsid_CUtilities );

    ret = Sem_Init( &m_semWaitingLock, 0, 0 );
    if( ERROR( ret ) )
    {
        std::string strMsg = DebugMsg( errno,
            "sem init failed" );
        throw std::runtime_error( strMsg );
    }

    ret = Sem_Init( &m_semWaitingLockLongWait, 0, 0 );
    if( ERROR( ret ) )
    {
        std::string strMsg = DebugMsg( errno,
            "sem init failed" );
        throw std::runtime_error( strMsg );
    }

    do{
        CCfgOpener a( pCfg );
        ret = a.GetPointer( propIoMgr, m_pIoMgr );
        if( ERROR( ret ) )
            throw std::invalid_argument(
                "the pointer to IoManager does not exist" );

        CCfgOpener b;
        b.SetPointer( propIoMgr, GetIoMgr() );
        b.SetObjPtr( propParentPtr, this );

        m_pTimerSvc.NewObj(
            clsid( CTimerService ), b.GetCfg() );

        m_pWiMgr.NewObj(
            clsid( CWorkitemManager ), b.GetCfg() );

    }while( 0 );
}

CUtilities::~CUtilities()
{
    sem_destroy( &m_semWaitingLock );
    sem_destroy( &m_semWaitingLockLongWait );
}

gint32 CUtilities::ThreadProc( gint32 iThreadIdx )
{

    gint32 ret = 0;

    sem_t* pSem = nullptr;

    if( iThreadIdx > 1 )
        return -EINVAL;

    if( iThreadIdx == 0 )
        pSem = &m_semWaitingLock;

    else if( iThreadIdx == 1 )
        pSem = &m_semWaitingLockLongWait;

    string strName = "Utilities-";
    strName += std::to_string( iThreadIdx );
    SetThreadName( strName );

    while( !m_bExit )
    {
        ret = Sem_TimedwaitSec( pSem,
            THREAD_WAKEUP_INTERVAL );

        if( ret == -EAGAIN )
            continue;

        if( ERROR( ret ) )
            break;

        if( iThreadIdx == 0 )
        {
            // timers only processed on thread 0
            m_pTimerSvc->ProcessTimers();
        }

        m_pWiMgr->ProcessWorkItems( iThreadIdx );
    }
    return ret;
}

using namespace std::placeholders;

gint32 CUtilities::Start()
{
    // start the thread
    if( m_pWorkerThread )
        return -EEXIST;

    // c++11 required for std::bind
    function<gint32(gint32)> oThreadProc =
        std::bind( &CUtilities::ThreadProc, this, _1 );

    m_pWorkerThread = new thread( oThreadProc, 0 );
    m_pWorkerThreadLongWait = new thread( oThreadProc, 1 );

    m_pTimerSvc->Start();

    return 0;
}

gint32 CUtilities::Stop()
{
    m_pTimerSvc->Stop();
    if( m_pWorkerThread )
    {
        m_bExit = true;
        Wakeup( 0 );
        m_pWorkerThread->join();
        delete m_pWorkerThread;
        m_pWorkerThread = NULL;

        Wakeup( 1 );
        m_pWorkerThreadLongWait->join();
        delete m_pWorkerThreadLongWait;
        m_pWorkerThreadLongWait = NULL;
    }
    return 0;
}

void CUtilities::Wakeup( gint32 iThreadIdx )
{
    if( iThreadIdx == 0 )
        Sem_Post( &m_semWaitingLock );
    else
    {
        Sem_Post( &m_semWaitingLockLongWait );
    }

}

CWorkitemManager::CWorkitemManager(
    const IConfigDb* pCfg )
{
    SetClassId( Clsid_CWorkitemManager );
    CCfgOpener a( pCfg );
    a.GetPointer( propIoMgr, m_pIoMgr );

    ObjPtr pObj;
    a.GetObjPtr( propParentPtr, pObj );
    m_pUtils = pObj;
}

gint32 CWorkitemManager::ScheduleWorkitem(
    guint32 dwFlags,
    IEventSink* pCallback,
    guint32 dwContext,
    bool bLongWait )
{
    if( pCallback == nullptr )
        return -EINVAL;

    CStdMutex oMutex( m_oQueLock );
    WORK_ITEM wi;

    wi.m_dwFlags = dwFlags;
    wi.m_pEventSink = EventPtr( pCallback );
    wi.m_dwContext = dwContext;
    if( !bLongWait )
    {
        m_queWorkitems.push_back( wi );
        m_pUtils->Wakeup( 0 );
    }
    else
    {
        m_queWorkitemsLongWait.push_back( wi );
        m_pUtils->Wakeup( 1 );
    }

    return wi.m_iWorkitemId;
}

gint32 CWorkitemManager::CancelWorkitem(
    gint32 iWorkitemId )
{
    WORK_ITEM wi;
    bool bFound = false;
    gint32 ret = -ENOENT;
    do{
        CStdMutex oMutex( m_oQueLock );

        deque< WORK_ITEM >::iterator itr;

        if( !m_queWorkitems.empty() )
        {
            itr = m_queWorkitems.end();
            do{
                --itr;
                if( itr->m_iWorkitemId == iWorkitemId )
                {
                    // only one match at a time
                    wi = *itr;
                    bFound = true;
                    break;
                }

            } while( itr != m_queWorkitems.begin() );

            if( itr == m_queWorkitems.begin() 
                && itr->m_iWorkitemId == iWorkitemId )
            {
                wi = *itr;
                bFound = true;
            }
            if( bFound )
            {
                m_queWorkitems.erase( itr );
            }
        }

        if( !bFound && !m_queWorkitemsLongWait.empty() )
        {
            // do it again on long wait queue
            itr = m_queWorkitemsLongWait.end();
            do{
                --itr;
                if( itr->m_iWorkitemId == iWorkitemId )
                {
                    // only one match at a time
                    wi = *itr;
                    bFound = true;
                    break;
                }
            } while( itr != m_queWorkitemsLongWait.begin() );

            if( bFound )
            {
                wi = *itr;
                m_queWorkitemsLongWait.erase( itr );
            }
        }

    }while( 0 );

    if( bFound && !wi.m_pEventSink.IsEmpty() )
    {
        wi.m_pEventSink->OnEvent(
            eventWorkitemCancel,
            wi.m_dwFlags,
            wi.m_dwContext );

        ret = 0;
    }
    return ret;
}

gint32 CWorkitemManager::ProcessWorkItems(
    gint32 iThreadIdx )
{

	std::deque<WORK_ITEM>* pQueue = nullptr ;
    deque<WORK_ITEM> tempWorkitems;
    gint32 ret = -ENOENT;
    do{
        if( iThreadIdx == 0 )
            pQueue = &m_queWorkitems;
        else
            pQueue = &m_queWorkitemsLongWait;

        if( 1 )
        {
            CStdMutex oMutex( m_oQueLock );
            if( pQueue->empty() )
                break;

            tempWorkitems = *pQueue;
            pQueue->clear();
        }

        ret = 0;
        while( tempWorkitems.size() )
        {
            WORK_ITEM& wi = tempWorkitems.front();
            if( !wi.m_pEventSink.IsEmpty() )
            {
                wi.m_pEventSink->OnEvent(
                    eventWorkitem, wi.m_dwFlags, wi.m_dwContext );
            }
            tempWorkitems.pop_front();
        }

    }while( 1 );

    return ret;
}

CTimerService::CTimerService(
    const IConfigDb* pCfg )
{
    SetClassId( Clsid_CTimerService );
    CCfgOpener a( pCfg );
    a.GetPointer( propIoMgr, m_pIoMgr );
    ObjPtr pObj;
    a.GetObjPtr( propParentPtr, pObj );
    m_pUtils = pObj;
}

CTimerService::~CTimerService()
{
    g_source_unref( m_pTimeSrc );
}

gint32 CTimerService::AddTimer(
    guint32 dwIntervalSec,
    IEventSink* pEvent,
    guint32 dwParam )
{
    gint32 ret = 0;
    if( pEvent == NULL )
        ret = -EINVAL;

    do{
        TIMER_ENTRY te;
        te.m_qwIntervalMs = dwIntervalSec * 1000;
        te.m_pCallback = EventPtr( pEvent );
        te.m_qwTicks = 0;
        te.m_dwParam = dwParam;

        CStdMutex oMutex( m_oMapLock );
        m_vecTimers.push_back( te );
        ret = te.m_iTimerId;

    }while( 0 );

    return ret;
}

gint32 CTimerService::ResetTimer(
    gint32 iTimerId )
{
    gint32 ret = -ENOENT;
    CStdMutex oMutex( m_oMapLock );
    vector<TIMER_ENTRY>::iterator itr =
            m_vecTimers.begin();

    while( itr != m_vecTimers.end() )
    {
        TIMER_ENTRY& te = *itr;
        ++itr;

        if( te.m_iTimerId == iTimerId )
        {
            te.m_qwTicks = 0;
            ret = 0;
            break;
        }
    }

    if( m_vecPendingTimeouts.size() )
    {
        // CTimerService lives on only on
        // thread 0
        m_pUtils->Wakeup( 0 );
    }

    return ret;
}

gint32 CTimerService::AdjustTimer(
    gint32 iTimerId,
    gint32 iOffset )
{
    gint32 ret = -ENOENT;

    CStdMutex oMutex( m_oMapLock );

    vector<TIMER_ENTRY>::iterator itr =
            m_vecTimers.begin();

    while( itr != m_vecTimers.end() )
    {
        TIMER_ENTRY& te = *itr;

        guint32 dwOffset = abs( iOffset );
        if( te.m_iTimerId == iTimerId )
        {
            if( iOffset > 0 )
            {
                te.m_qwIntervalMs += SecToMs( dwOffset );
            }
            else
            {
                te.m_qwTicks += SecToMs( dwOffset );
            }

            if( te.m_qwTicks > te.m_qwIntervalMs )
            {
                m_vecPendingTimeouts.push_back( te );        
                m_vecTimers.erase( itr );
            }
            ret = 0;
            break;
        }
        ++itr;
    }

    if( m_vecPendingTimeouts.size() )
    {
        // CTimerService lives on only on
        // thread 0
        m_pUtils->Wakeup( 0 );
    }

    return ret;
}

gint32 CTimerService::RemoveTimer(
    gint32 iTimerId )
{
    gint32 ret = 0;
    bool bFound = false;

    CStdMutex oMutex( m_oMapLock );
    if( m_vecTimers.empty() )
    {
        ret = -ENOENT;
    }
    else
    {
        vector<TIMER_ENTRY>::iterator itr =
                m_vecTimers.begin();

        while( itr != m_vecTimers.end() )
        {
            if( itr->m_iTimerId == iTimerId )
            {
                m_vecTimers.erase( itr );
                bFound = true;
                break;
            }
            itr++;
        }

        if( !bFound )
            ret = -ENOENT;
    }

    if( m_vecPendingTimeouts.size() )
    {
        // CTimerService lives on only on
        // thread 0
        m_pUtils->Wakeup( 0 );
    }

    return ret;
}

gboolean CTimerService::TimerCallback(
    gpointer dwParams )
{
    CTimerService* pthis =
        reinterpret_cast< CTimerService* >(dwParams);

    pthis->TickTimers();

    // don't quit the timer
    return G_SOURCE_CONTINUE;
}

gint32 CTimerService::TickTimers()
{
    do{
        CStdMutex oMutex( m_oMapLock );
        vector<TIMER_ENTRY>::iterator itr =
                m_vecTimers.begin();

        // performance call
        guint64 qwNow = g_get_monotonic_time();

        guint32 dwInterval = ( guint32 )
            ( ( qwNow - m_qwTimeStamp ) / 1000 );

        m_qwTimeStamp = qwNow;
        vector< gint32 > vecRemove;

        while( itr != m_vecTimers.end() )
        {
            itr->m_qwTicks += dwInterval;

            if( itr->m_qwTicks
                    >= itr->m_qwIntervalMs )
            {
                m_vecPendingTimeouts.push_back( *itr );
                vecRemove.push_back(
                    itr - m_vecTimers.begin() );
            }
            ++itr;
        }
        
        for( gint32 i = vecRemove.size() - 1; i >= 0; --i )
        {
            m_vecTimers.erase(
                m_vecTimers.begin() + vecRemove[ i ] );
        }

        if( m_vecPendingTimeouts.size() )
            m_pUtils->Wakeup( 0 );

        break;

    }while( 1 );

    return 0;
}

gint32 CTimerService::ProcessTimers()
{
    gint32 ret = 0;
    vector<TIMER_ENTRY> tempPending;

    do{
        if( 1 )
        {
            CStdMutex oMutex( m_oMapLock );
            if( m_vecPendingTimeouts.empty() )
            {
                break;
            }
            tempPending = m_vecPendingTimeouts;
            m_vecPendingTimeouts.clear();
        }

        while( !tempPending.empty() )
        {
            TIMER_ENTRY* pte = &tempPending.back();
            if( !pte->m_pCallback.IsEmpty() )
            {
                pte->m_pCallback->OnEvent(
                    eventTimeout,
                    pte->m_dwParam,
                    ( guint32 )GetIoMgr() );
            }
            tempPending.pop_back();
        }
        tempPending.clear();

    }while( 1 );

    return ret;
}

gint32 CTimerService::Start()
{
    // setup the connection to the glib source
    m_pTimeSrc = g_timeout_source_new( 500 );
    if( m_pTimeSrc == nullptr )
        return -EFAULT;

    g_source_set_callback(
        m_pTimeSrc, TimerCallback, ( gpointer )this, nullptr );

    m_dwTimerId = g_source_attach(
        m_pTimeSrc, GetIoMgr()->GetMainIoLoop().GetMainCtx() );

    m_qwTimeStamp = g_get_monotonic_time();
    return 0;
}

gint32 CTimerService::Stop()
{
    if( m_pTimeSrc )
    {
        g_source_destroy( m_pTimeSrc );
        g_source_unref( m_pTimeSrc );
        m_dwTimerId = 0;
    }
    return 0;
}
