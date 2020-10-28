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
 *      Copyright:  2019 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include "defines.h"
#include "frmwrk.h"
#include "utils.h"
#include <functional>

namespace rpcfrmwrk
{

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

    // m_pWorkerThread = new thread( oThreadProc, 0 );
    // m_pWorkerThreadLongWait = new thread( oThreadProc, 1 );

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

        if( m_pWorkerThreadLongWait != nullptr )
        {
            Wakeup( 1 );
            m_pWorkerThreadLongWait->join();
            delete m_pWorkerThreadLongWait;
            m_pWorkerThreadLongWait = NULL;
        }
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
    LONGWORD dwContext,
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
    : m_dwTimerId( 0 )
{
    SetClassId( Clsid_CTimerService );
    CCfgOpener a( pCfg );
    a.GetPointer( propIoMgr, m_pIoMgr );
    ObjPtr pObj;
    a.GetObjPtr( propParentPtr, pObj );
    m_pUtils = pObj;
}

gint32 CTimerService::AddTimer(
    guint32 dwIntervalSec,
    IEventSink* pEvent,
    LONGWORD dwParam )
{
    gint32 ret = -EINVAL;
    if( pEvent == nullptr )
        return ret;

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
    pthis->ProcessTimers();

    // don't quit the timer
    return G_SOURCE_CONTINUE;
}

gint32 CTimerService::TickTimers()
{
    do{
        MloopPtr pLoop =
            GetIoMgr()->GetMainIoLoop();

        CStdMutex oMutex( m_oMapLock );
        vector<TIMER_ENTRY>::iterator itr =
                m_vecTimers.begin();

        // performance call
        guint64 qwNow = pLoop->NowUs();
        guint32 dwInterval = ( guint32 )
            ( ( qwNow - m_qwTimeStamp ) / 1000 );
        m_qwTimeStamp = qwNow;
        // printf( "Interval is %d ms\n", dwInterval );
        while( itr != m_vecTimers.end() )
        {
            itr->m_qwTicks += dwInterval;
            if( itr->m_qwTicks
                    >= itr->m_qwIntervalMs )
            {
                // due time
                m_vecPendingTimeouts.push_back( *itr );
                itr = m_vecTimers.erase( itr );
                continue;
            }
            ++itr;
        }
        if( m_vecPendingTimeouts.size() )
            m_pUtils->Wakeup( 0 );

    }while( 0 );

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
                    ( LONGWORD )GetIoMgr() );
            }
            tempPending.pop_back();
        }
        tempPending.clear();

    }while( 1 );

    return ret;
}

gint32 CTimerWatchCallback::operator()(
    guint32 dwContext )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );
        CIoManager* pMgr = nullptr;

        ret = oCfg.GetPointer(
            propIoMgr, pMgr );

        if( ERROR( ret ) )
            break;

        CTimerService& oTimerSvc =
            pMgr->GetUtils().GetTimerSvc();

        ret = CTimerService::TimerCallback(
            ( gpointer )&oTimerSvc );

    }while( 0 );

    return SetError( ret );
}

gint32 CTimerService::Start()
{
    gint32 ret = 0;
    do{
        // setup the connection to the glib source
        CParamList oParams;
        oParams.SetPointer(
            propIoMgr, GetIoMgr() );

        oParams.Push( 500 ); // interval in ms
        oParams.Push( true ); // repeatable timer
        oParams.Push( true ); // start immediately

        TaskletPtr pCallback;
        gint32 ret = pCallback.NewObj(
            clsid( CTimerWatchCallback ),
            oParams.GetCfg() );

        MloopPtr pLoop =
            GetIoMgr()->GetMainIoLoop();

        ret = pLoop->AddTimerWatch(
            pCallback, m_dwTimerId );

        if( ERROR( ret ) )
            break;

        m_qwTimeStamp = pLoop->NowUs();

    }while( 0 );

    return ret;
}

gint32 CTimerService::Stop()
{
    if( m_dwTimerId )
    {
        MloopPtr pLoop =
            GetIoMgr()->GetMainIoLoop();

        pLoop->RemoveTimerWatch(
            m_dwTimerId );

        m_dwTimerId = 0;
    }
    return 0;
}

}
