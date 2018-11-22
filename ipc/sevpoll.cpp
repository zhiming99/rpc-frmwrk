/*
 * =====================================================================================
 *
 *       Filename:  sevpoll.cpp
 *
 *    Description:  implementation of the CSimpleEvPoll
 *
 *        Version:  1.0
 *        Created:  10/05/2018 09:33:46 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:  
 *
 * =====================================================================================
 */


#include <stdio.h>
#include <vector>
#include <string>
#include <unistd.h>
#include <functional>
#include  <sys/syscall.h>
#include "configdb.h"
#include "mainloop.h"

#ifdef _USE_LIBEV
#include "evloop.h"
#include "sevpoll.h"
#include <fcntl.h>

CSimpleEvPoll::CSimpleEvPoll(
    const IConfigDb* pCfg ) :
    m_bStop( true ), m_iPiper( -1 ),
    m_iPipew( -1 ), m_bIoAdd( false ),
    m_bIoRemove( false ), m_bTimeAdd( false ),
    m_bTimeRemove( false )
{
    gint32 ret = 0;
    do{
        m_pLoop = static_cast< CEvLoop* >( this );
        if( m_pLoop == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        int iPipeFds[ 2 ];
        gint32 ret = pipe( iPipeFds );
        if( ret == -1 )
        {
            ret = -errno;
            break;
        }
        m_iPiper = iPipeFds[ 0 ];
        m_iPipew = iPipeFds[ 1 ];

        int iFlags = fcntl( m_iPiper, F_GETFL );
        fcntl( m_iPiper, F_SETFL,
            iFlags | O_NONBLOCK );

    }while( 0 );

    if( ERROR( ret ) )
    {
        throw std::runtime_error(
            DebugMsg( ret,
            "Error in CSimpleEvPoll's ctor" ) );
    }
}

CSimpleEvPoll::~CSimpleEvPoll()
{
    if( m_iPiper >= 0 )
        close( m_iPiper );
    if( m_iPipew >= 0 )
        close( m_iPipew );

    m_iPiper = -1;
    m_iPipew = -1;
}

gint32 CSimpleEvPoll::StartStopSource(
    HANDLE hWatch,
    EnumSrcType iType,
    bool bStart )
{
    gint32 ret = 0;

    CEvLoop::SOURCE_HEADER* pSrc = nullptr;
    CStdRMutex oLock( GetLock() );
    ret = m_pLoop->GetSource(
        hWatch, iType, pSrc );

    if( ERROR( ret ) )
        return ret;

    return pSrc->StartStop( bStart );
}

gint32 CSimpleEvPoll::UpdateSource(
    HANDLE hWatch,
    EnumSrcType iType,
    IConfigDb* pCfg )
{
    gint32 ret = 0;
    if( hWatch == 0 || pCfg == nullptr )
        return -EINVAL;

    do{

        CStdRMutex oLock( GetLock() );

        CEvLoop::SOURCE_HEADER* pSrc = nullptr;
        gint32 ret = m_pLoop->GetSource(
            hWatch, iType, pSrc );

        if( ERROR( ret ) )
            break;

        EnumSrcState iState = pSrc->GetState();

        ret = StopSource( hWatch, iType );
        if( ERROR( ret ) )
            break;

        ret = pSrc->UpdateSource( pCfg );
        if( ERROR( ret ) )
            break;
        
        if( iState == srcsReady )
            ret = StartSource( hWatch, iType );

    }while( 0 );

    return ret;
}

gint32 CSimpleEvPoll::StopSource(
    HANDLE hWatch, EnumSrcType iType )
{
    return StartStopSource(
        hWatch, iType, false );
}

gint32 CSimpleEvPoll::StartSource(
    HANDLE hWatch, EnumSrcType iType )
{
    return StartStopSource(
        hWatch, iType, true );
}

gint32 CSimpleEvPoll::RunSource( HANDLE hWatch,
    EnumSrcType iType, guint32 dwContext )
{
    CEvLoop::SOURCE_HEADER* pSrc = nullptr;
    CStdRMutex oLock( GetLock() );
    gint32 ret = m_pLoop->GetSource(
        hWatch, iType, pSrc );
    if( ERROR( ret ) )
        return ret;
    if( pSrc->GetState() != srcsReady )
        return ERROR_STATE;
    TaskletPtr pCallback = pSrc->m_pCallback;
    if( pCallback.IsEmpty() )
        return -EFAULT;
    oLock.Unlock();
    // NOTE: the task should have the ability to
    // be aware of it could have enterred a state
    // that its state has changed from ready to
    // done and behave properly at such a
    // situation.
    if( iType == srcTimer )
    {
        CEvLoop::TIMER_SOURCE* pTimerSrc =
            ( CEvLoop::TIMER_SOURCE* )pSrc;
        ret = pTimerSrc->TimerCallback( dwContext );
    }
    else if( iType == srcIo )
    {
        CEvLoop::IO_SOURCE* pIoSrc =
            ( CEvLoop::IO_SOURCE* )pSrc;
        ret = pIoSrc->IoCallback( dwContext ); 
    }
    else if( iType == srcAsync )
    {
        CEvLoop::ASYNC_SOURCE* pAsyncSrc =
            ( CEvLoop::ASYNC_SOURCE* )pSrc;
        ret = pAsyncSrc->AsyncCallback( dwContext );
    }
    return ret;
}

gint32 CSimpleEvPoll::ReadAsyncEvent(
    EnumAsyncEvent& iEvent )
{
    return ReadAsyncData(
        ( guint8* )&iEvent, sizeof( iEvent ) );
}

gint32 CSimpleEvPoll::ReadAsyncData(
    guint8* pData, guint32 dwSize )
{
    if( pData == nullptr ||
        !( dwSize > 0 && dwSize < 16 ) )
        return -EINVAL;

    gint32 ret = 0;
    guint8 buf[ 32 ];
    bool bFull = false;
    do{
        if( !bFull &&
            m_queReadBuf.size() >= dwSize )
        {
            for( guint32 i = 0; i < dwSize; ++i )
            {
                pData[ i ] = m_queReadBuf.front();
                m_queUndoBuf.push_back(
                    m_queReadBuf.front() );
                m_queReadBuf.pop_front();
                if( m_queUndoBuf.size() > UNDO_DEPTH )
                    m_queUndoBuf.pop_front();
            }
            bFull = true;
        }
        ret = read( m_iPiper, &buf, sizeof( buf ) );
        if( ret > 0 )
        {
            for( int i = 0; i < ret; ++i )
                m_queReadBuf.push_back( buf[ i ] );
            if( m_queReadBuf.size() > 1024 )
                break;
            continue;
        }
        else if( ret == -1 )
        {
            if( bFull )
                ret = 0;
            else
                ret = -errno;
            break;
        }
        break;

    }while( 1 );

    return ret;
}

gint32 CSimpleEvPoll::AsyncDataUnwind(
        gint32 iSize )
{
    if( iSize > UNDO_DEPTH )
        return -EINVAL;
    for( int i = 0; i < iSize; i++ )
    {
        m_queReadBuf.push_front(
            m_queUndoBuf.back() );
        m_queUndoBuf.pop_back();
    }
    return 0;
}

gint32 CSimpleEvPoll::WriteAsyncData(
    guint8* pData, gint32 iSize )
{
    gint32 ret = 0;
    guint8* ptr = pData;
    do{
        ret = write( m_iPipew, ptr, iSize );
        if( ret == iSize )
            break;
        if( ret < iSize )
        {
            // the write is interrupted
            ptr += ret;
            iSize -= ret;
            continue;
        }
        if( ret == -1 )
        {
            ret = -errno;
            break;
        }
        break;

    }while( 1 );

    return ret;
}

gint32 CSimpleEvPoll::WakeupLoop(
    EnumAsyncEvent iEvent,
    guint32 dwContext )
{
    if( m_iPipew < 0 )
        return ERROR_STATE;

    gint32 ret = 0;
    do{
        CStdRMutex oLock( GetLock() );
        ret = WriteAsyncData(
            ( guint8* )&iEvent,
            sizeof( iEvent ) );
        if( ERROR( ret ) )
            break;

        if( iEvent == aevtStop ||
            iEvent == aevtRunSched ||
            iEvent == aevtWakeup )
            break;

        ret = WriteAsyncData(
            ( guint8* )&dwContext,
            sizeof( dwContext ) );
        if( ERROR( ret ) )
            break;
    }while( 0 );

    return ret;
}

#define IO_KEY( fd_, flag_ ) \
    ( ( gint32 )( ( fd_ << 16 ) | ( flag_ & 0xffff ) ) )

gint32 CSimpleEvPoll::UpdateIoMaps(
    std::map< gint32, HANDLE >& mapActFds )
{
    gint32 ret = 0;
    std::map< HANDLE, CEvLoop::SOURCE_HEADER* >*
        pMap = m_pLoop->GetMap( srcIo );

    if( m_bIoAdd )
    {
        for( auto elem : *pMap )
        {
            CEvLoop::IO_SOURCE* pSrc =
                ( CEvLoop::IO_SOURCE* )elem.second;
            if( pSrc->GetState() == srcsReady )
            {
                gint32 iIoKey = IO_KEY(
                    pSrc->m_iFd, pSrc->m_dwOpt );
                if( mapActFds.find( iIoKey ) ==
                    mapActFds.end() )
                {
                    mapActFds[ iIoKey ] =
                        ( HANDLE )pSrc;
                }
            }
        }
        m_bIoAdd = false;
    }
    if( m_bIoRemove )
    {
        std::map< gint32, HANDLE >::iterator
            itr = mapActFds.begin();
        while( itr != mapActFds.end() )
        {
            HANDLE hWatch = itr->second;
            if( pMap->find( hWatch ) ==
                pMap->end() )
            {
                itr = mapActFds.erase( itr );
                continue;
            }

            CEvLoop::SOURCE_HEADER* psh = nullptr;
            ret = m_pLoop->GetSource(
                itr->second, srcIo, psh );
            if( ERROR( ret ) )
            {
                itr = mapActFds.erase( itr );
                continue;
            }

           CEvLoop::IO_SOURCE* pSrc =
               ( CEvLoop::IO_SOURCE* )psh;
            if( pSrc->GetState() != srcsReady )
            {
                itr = mapActFds.erase( itr );
                continue;
            }
            itr++;
        }
        m_bIoRemove = false;
    }

    return 0;
}

gint32 CSimpleEvPoll::UpdateTimeMaps(
    std::multimap< guint64, HANDLE >& mapActTimers )
{
    gint32 ret = 0;
    std::map< HANDLE, CEvLoop::SOURCE_HEADER* >* pMap =
        m_pLoop->GetMap( srcTimer );

    if( m_bTimeAdd )
    {
        for( auto elem : *pMap )
        {
            CEvLoop::TIMER_SOURCE* pSrc =
                ( CEvLoop::TIMER_SOURCE* )elem.second;
            if( pSrc->GetState() == srcsReady )
            {
                guint64 qwAbsTime = pSrc->m_qwAbsTimeUs;
                if( mapActTimers.find( qwAbsTime ) ==
                    mapActTimers.end() )
                {
                    mapActTimers.insert( 
                    std::pair< guint64, HANDLE >
                    ( qwAbsTime, ( HANDLE )pSrc ) );
                }
            }
        }
        m_bTimeAdd = false;
    }

    if( m_bTimeRemove )
    {
        std::vector< std::multimap< guint64, HANDLE >::iterator > vecItrs;
        std::multimap< guint64, HANDLE >::iterator
        itr = mapActTimers.begin();
        while( itr != mapActTimers.end() )
        {
            HANDLE hWatch = itr->second;
            if( pMap->find( hWatch ) ==
                pMap->end() )
            {
                mapActTimers.erase( itr++ );
                continue;
            }

            CEvLoop::SOURCE_HEADER* psh = nullptr;
            ret = m_pLoop->GetSource(
                itr->second, srcTimer, psh );
            if( ERROR( ret ) )
            {
                mapActTimers.erase( itr++ );
                continue;
            }

            CEvLoop::TIMER_SOURCE* pSrc =
                ( CEvLoop::TIMER_SOURCE* )psh;
            if( pSrc->GetState() != srcsReady )
            {
                mapActTimers.erase( itr++ );
                continue;
            }
            itr++;
        }
        m_bTimeRemove = false;
    }

    return 0;
}

gint32 CSimpleEvPoll::CalcWaitTime(
    std::multimap< guint64, HANDLE >& mapActTimers,
    guint32& dwIntervalMs )
{
    if( mapActTimers.empty() )
        return -ENOENT;
    guint64 qwLeastTo =
        mapActTimers.begin()->first;
    guint64 qwNow = NowUsFast();
    if( qwLeastTo > qwNow )
    {
        dwIntervalMs = ( qwLeastTo - qwNow + 999 ) / 1000;
        // printf( "last time %lld, now time %lld, next time %lld \n",
        //     qwLeastTo, qwNow, qwNow + dwIntervalMs * 1000 );
        return 0;
    }
    return -ETIMEDOUT;
}

gint32 CSimpleEvPoll::UpdateTimeSource(
    HANDLE hWatch, guint64& qwNewTimeUs )
{
    if( hWatch == INVALID_HANDLE )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CEvLoop::SOURCE_HEADER *psh = nullptr;
        CStdRMutex oLock( GetLock() );
        ret = m_pLoop->GetSource(
            hWatch, srcTimer, psh );
        if( ERROR( ret ) )
            break;
        CEvLoop::TIMER_SOURCE* pTs =
            ( CEvLoop::TIMER_SOURCE* )psh;
        if( pTs->GetState() != srcsReady )
        {
            ret = ERROR_STATE;
            break;
        }
        if( !pTs->m_bRepeat )
        {
            pTs->Stop();
            break;
        }

        if( NowUsFast() < pTs->m_qwAbsTimeUs )
        {
            ret = -EAGAIN;
            break;
        }

        // to make sure the next due time is afer
        // the NowUsFast
        guint64 qwIntervalUs =
            NowUsFast() - pTs->m_qwAbsTimeUs;

        guint64 nFactor = qwIntervalUs /
            ( ( guint64 )pTs->m_dwIntervalMs * 1000 );

        pTs->m_qwAbsTimeUs +=
            pTs->m_dwIntervalMs * 1000 *
            ( nFactor + 1 );

        // printf( "nFactor is %lld\n", nFactor );
        qwNewTimeUs = pTs->m_qwAbsTimeUs;

    }while( 0 );

    return ret;
}

gint32 CSimpleEvPoll::HandleTimeout(
    std::multimap< guint64, HANDLE >& mapActTimers )
{
    gint32 ret = 0;
    do{
        if( mapActTimers.empty() )
            break;
        guint64 qwDueTime = NowUsFast() + 1;
        std::multimap< guint64, HANDLE >::iterator
            itrGreater = mapActTimers.lower_bound(
                qwDueTime );

        // no timeout
        if( itrGreater == mapActTimers.begin() )
            break;

        std::multimap< guint64, HANDLE >::iterator
            itr = mapActTimers.begin();
        std::vector< std::pair< guint64, HANDLE > >
            vecReAdd;
        while( itr != itrGreater )
        {
            if( IsStopped() )
            {
                ret = ERROR_STATE;
                break;
            }
            ret = RunSource( itr->second,
                srcTimer, eventTimeout );
            if( ret == G_SOURCE_REMOVE )
            {
                m_pLoop->RemoveSource(
                    itr->second, srcTimer );
            }
            else if( ret == G_SOURCE_CONTINUE )
            {
                // update to the next m_qwAbsTimeUs 
                guint64 qwNewTimeUs = 0;
                ret = UpdateTimeSource(
                    itr->second, qwNewTimeUs );
                if( ret == -EAGAIN )
                {
                    ++itr;
                    continue;
                }
                if( ERROR( ret ) )
                {
                    StopSource( itr->second,
                        srcTimer );
                }
                else
                {
                    std::pair< guint64, HANDLE >
                        oVal( qwNewTimeUs, itr->second );
                    vecReAdd.push_back( oVal );
                }
            }
            else
            {
                // internal error, stop the source
                StopSource( itr->second, srcTimer );
            }
            itr = mapActTimers.erase( itr );
        }
        for( auto elem : vecReAdd )
            mapActTimers.insert( elem );
    }while( 0 );

    if( IsStopped() )
        return ret;

    CStdRMutex oLock( GetLock() );
    UpdateTimeMaps( mapActTimers );
    return ret;
}

gint32 CSimpleEvPoll::BuildPollFds(
    std::map< gint32, HANDLE >& mapActFds,
    pollfd* pPollInfo, gint32& iCount )
{
    if( mapActFds.empty() )
        return 0;

    if( pPollInfo == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    iCount = 0;
    CStdRMutex oLock( GetLock() );
    do{
        for( auto elem : mapActFds )
        {
            HANDLE hWatch = elem.second;
            CEvLoop::SOURCE_HEADER *psh = nullptr;
            ret = m_pLoop->GetSource( hWatch, srcIo, psh );
            if( ERROR( ret ) )
                continue;
            CEvLoop::IO_SOURCE* pIs =
                ( CEvLoop::IO_SOURCE* )psh;
            if( pIs->GetState() != srcsReady )
                continue;
            pPollInfo[ iCount ].fd = pIs->m_iFd;
            pPollInfo[ iCount ].events = pIs->m_dwOpt;
            pPollInfo[ iCount ].revents = 0;
            ++iCount;
        }
    }while( 0 );

    return 0;
}

gint32 CSimpleEvPoll::HandleIoEvents(
    std::map< gint32, HANDLE >& mapActFds,
    pollfd* pPollInfo, gint32 iCount )
{
    if( pPollInfo == nullptr ||
        iCount < 0 )
        return -EINVAL;

    gint32 ret = 0;
    gint32 iFound = 0;
    do{
        for( guint32 i = 0;
            i < mapActFds.size() && iFound < iCount;
            ++i )
        {
            if( pPollInfo[ i ].revents == 0 )
                continue;
            if( IsStopped() )
            {
                ret = ERROR_STATE;
                break;
            }
            ++iFound;

            gint32 iIoKey = IO_KEY( 
                pPollInfo[ i ].fd,
                pPollInfo[ i ].events );

            HANDLE hWatch =
                mapActFds[ iIoKey ];
            ret = RunSource( hWatch, srcIo,
                pPollInfo[ i ].revents );
            if( ret == G_SOURCE_REMOVE )
            {
                ret = m_pLoop->RemoveSource(
                    hWatch, srcIo );
                // BUGBUG: erased the entry may
                // cause the UpdateIoMaps to
                // iterate the map with nothing
                // found
                mapActFds.erase( iIoKey );
            }
            else if( ERROR( ret ) )
            {
                StopSource( hWatch, srcIo );
            }
        }

        if( IsStopped() )
            return ret;

        CStdRMutex oLock( GetLock() );
        UpdateIoMaps( mapActFds );
    }while( 0 );

    return ret;
}

gint32 CSimpleEvPoll::RunLoop()
{
    gint32 ret = 0;
    guint32 dwIntervalMs = 0;
    gint32 iFdCount = 4;
    std::map< gint32, HANDLE > mapActFds;
    std::multimap< guint64, HANDLE > mapActTimers;

    CStdRMutex oLock( GetLock() );
    SetStop( false );
    UpdateIoMaps( mapActFds );
    UpdateTimeMaps( mapActTimers );
    oLock.Unlock();

    BufPtr pBuf( true );
    pBuf->Resize( sizeof( pollfd ) * iFdCount );
    if( ERROR( ret ) )
        return ret;

    pollfd* pPollInfo = ( pollfd* )pBuf->ptr();
    while( !IsStopped() )
    {
        gint32 iCount = 0;
        if( iFdCount < ( gint32 )mapActFds.size() )
        {
            iFdCount = mapActFds.size() + 4; 
            pBuf->Resize(
                sizeof( pollfd ) * iFdCount );
            pPollInfo = ( pollfd* )pBuf->ptr();
            memset( pPollInfo, 0,
                sizeof( pollfd ) * iFdCount );
        }
        ret = BuildPollFds(
            mapActFds, pPollInfo, iCount );
        if( ERROR( ret ) )
            break;

        NowUs();
        ret = CalcWaitTime( mapActTimers,
            dwIntervalMs );
        if( ret == -ENOENT )
            dwIntervalMs = 1000;
        else if( ret == -ETIMEDOUT )
        {
            HandleTimeout( mapActTimers );
            continue;
        }

        gint32 iReadyCount = 0;
        do{
            ret = poll( pPollInfo,
                iCount, dwIntervalMs );
            if( ret > 0 )
            {
                // printf( "interval is %d\n", dwIntervalMs );
            }
            if( ret >= 0 )
            {
                iReadyCount = ret;
                break;
            }
            if( errno == EAGAIN ||
                errno == EINTR )
            {
                ret = 0;
                break;
            }
            ret = -errno;
            break;

        }while( 1 );

        if( ERROR( ret ) )
        {
            //NOTE: we cannot quit here,
            //otherwise, the system cannot receive
            //external commands
            //
            DebugPrint( ret,
                "Fatal error in mainloop" );
        }

        NowUs();
        HandleTimeout( mapActTimers );
        HandleIoEvents( mapActFds,
            pPollInfo, iReadyCount );
    }

    return 0;
}

CIoManager* CSimpleEvPoll::GetIoMgr()
{
    return m_pLoop->GetIoMgr();
}

#endif
