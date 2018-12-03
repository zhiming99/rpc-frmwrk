/*
 * =====================================================================================
 *
 *       Filename:  evloop.cpp
 *
 *    Description:  Implementation of the CEvLoop and related classes
 *
 *        Version:  1.0
 *        Created:  09/25/2018 04:48:56 PM
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
#include "frmwrk.h"
#include "mainloop.h"

#ifdef _USE_LIBEV
#include "evloop.h"
#include "ifhelper.h"

gint32 CEvLoop::SOURCE_HEADER::StartStop(
    bool bStart )
{
    gint32 ret = 0;
    CStdRMutex oLock( m_pParent->GetLock() );

    switch( m_bySrcType )
    {
    case srcTimer:
    case srcIo:
        {
            if( m_bySrcType == srcIo )
            {
                if( bStart )
                    m_pParent->m_bIoAdd = true;
                else
                    m_pParent->m_bIoRemove = true;
            }
            else
            {
                if( bStart )
                    m_pParent->m_bTimeAdd = true;
                else
                    m_pParent->m_bTimeRemove = true;
            }
            // fall through
        }
    case srcAsync:
        {
            if( bStart )
                SetState( srcsReady );
            else
                SetState( srcsDone );
            break;
        }
    default:
        ret = -ENOTSUP;
        break;
    }
    oLock.Unlock();
    m_pParent->WakeupLoop();
    return ret;
}

gint32 CEvLoop::SOURCE_HEADER::Start()
{
    return StartStop( true );
}

gint32 CEvLoop::SOURCE_HEADER::Stop()
{
    return StartStop( false );
}

CEvLoop::TIMER_SOURCE::TIMER_SOURCE(
    CEvLoop* pLoop,
    guint32 dwIntervalMs,
    bool bRepeat,
    TaskletPtr& pCallback )
    : SOURCE_HEADER( pLoop, srcTimer, pCallback )
{
    m_bRepeat = bRepeat;
    m_dwIntervalMs = dwIntervalMs;
    m_pCallback = pCallback;
}

CEvLoop::TIMER_SOURCE::~TIMER_SOURCE()
{
}

/**
* @name TimerCallback : the callback for timer watch,
* which is called by CSimpleEvPoll::RunSource when the
* timer is triggered
* @{ */
/**
 *  Parameters:
 *
 *  iParams: fixed to be eventTimeout
 *
 *  return value:
 *      the return value can be 
 *      G_SOURCE_REMOVE, the watcher will be removed
 *      from the event loop. or
 *      G_SOURCE_CONTINUE, the watcher will be kept in
 *      the event loop for the next event.
 *
 *      It requires the callback task must also have
 *      its OnEvent to return the same type of value
 *      after call .
 * @} */

gint32 CEvLoop::TIMER_SOURCE::TimerCallback(
    int iParams )
{
    gint32 ret = G_SOURCE_CONTINUE;
    if( !m_pCallback.IsEmpty() )
    {
        ret = m_pCallback->OnEvent( eventTimeout,
            iParams, ( HANDLE )this, nullptr );
    }
    if( !m_bRepeat )
    {
        Stop();
        ret = G_SOURCE_REMOVE;
    }
    return ret;
}

gint32 CEvLoop::TIMER_SOURCE::StartStop(
    bool bStart )
{
    if( bStart )
    {
        m_qwAbsTimeUs = m_pParent->NowUs()
            + m_dwIntervalMs * 1000;
    }
    return super::StartStop( bStart );
}

CEvLoop::IO_SOURCE::IO_SOURCE(
    CEvLoop* pLoop,
    gint32 iFd,
    guint32 dwOpt,
    TaskletPtr& pCallback )
    : SOURCE_HEADER( pLoop, srcIo, pCallback )
{
    m_iFd = iFd;
    m_dwOpt = dwOpt;
}

CEvLoop::IO_SOURCE::~IO_SOURCE()
{
}

/**
* @name IoCallback : the callback for io watch
* @{ */
/**
 *  Parameters:
 *
 *  condition: the events which triggered this callback
 *  and it could be one of the following:
 *      POLLIN, POLLOUT, POLLHUP, POLLERR
 *
 *  return value:
 *      the return value can be 
 *      G_SOURCE_REMOVE, the watcher will be removed
 *      from the event loop. or
 *      G_SOURCE_CONTINUE, the watcher will be kept in
 *      the event loop for the next event.
 *
 *      It requires the callback task must also have
 *      its OnEvent to return the same type of value
 *      after call .
 * @} */

gint32 CEvLoop::IO_SOURCE::IoCallback(
    int condition )
{
    gint32 ret = G_SOURCE_REMOVE;
    if( !m_pCallback.IsEmpty() )
    {
        ret = m_pCallback->OnEvent( eventIoWatch,
            condition, ( HANDLE )this, nullptr );
    }

    return ret;
}

CEvLoop::ASYNC_SOURCE::ASYNC_SOURCE(
    CEvLoop* pLoop,
    TaskletPtr& pCallback )
    :SOURCE_HEADER( pLoop, srcAsync, pCallback )
{
}

CEvLoop::ASYNC_SOURCE::~ASYNC_SOURCE()
{
}

/**
* @name AsyncCallback : the callback for async watch,
* which is called by CSimpleEvPoll::RunSource when the
* command pipe has some some command to exec
* @{ */
/**
 *  Parameters:
 *
 *  iEvents: fixed to be eventAsyncWatch
 *
 *  return value:
 *      the return value can be 
 *      G_SOURCE_REMOVE, the watcher will be removed
 *      from the event loop. or
 *      G_SOURCE_CONTINUE, the watcher will be kept in
 *      the event loop for the next event.
 *
 *      It requires the callback task must also have
 *      its OnEvent to return the same type of value
 *      after call .
 * @} */
gint32 CEvLoop::ASYNC_SOURCE::AsyncCallback(
    int iEvents )
{
    gint32 ret = G_SOURCE_CONTINUE;
    if( !m_pCallback.IsEmpty() )
    {
        ret = m_pCallback->OnEvent( eventAsyncWatch,
            0, ( HANDLE )this, nullptr );
    }
    return ret;
}

gint32 CEvLoop::ASYNC_SOURCE::Send()
{
    return m_pParent->WakeupLoop(
        aevtRunAsync, ( HANDLE )this );
}

gint32 CEvLoop::Start()
{
    // now we have a way to inject the command
    // to the main loop
    HANDLE hAsync = INVALID_HANDLE;
    TaskletPtr pTask;
    CParamList oParams;
    oParams.Push( ObjPtr( this ) );
    gint32 ret = pTask.NewObj(
        clsid( CEvLoopAsyncCallback ),
        oParams.GetCfg() );
    if( ERROR( ret ) )
        return ret;

    CParamList oTaskCfg(
        ( IConfigDb* )pTask->GetConfig() );
    oTaskCfg.ClearParams();
    // Start immediately
    oTaskCfg.Push( m_iPiper );
    // flags for poll
    guint32 dwCond = POLLIN;
    oTaskCfg.Push( dwCond );
    oTaskCfg.Push( true );
    ret = AddIoWatch( pTask, hAsync );
    if( ERROR( ret ) )
        return ret;
    ret = RunLoop();
    RemoveIoWatch( hAsync );
    return ret;
}

gint32 CEvLoop::Stop()
{
    gint32 ret = 0;
    do{
        CParamList oParams;
        oParams.Push( ObjPtr( this ) );

        ret = GetIoMgr()->ScheduleTaskMainLoop(
            clsid( CEvLoopStopCb ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        // wait setupdone change to false  to
        // enable resource removal from other
        // threads
        // while( m_pDBusHook->IsSetupDone() )

        ret = m_pDBusHook->Stop();
        if( ERROR( ret ) )
            break;

        // don't clear it here because the dbus
        // connections may not have removed the
        // callbacks yet
        //
        // ClearAll();

    }while( 0 );

    return ret;
}

void CEvLoop::ClearAll()
{
    m_pDBusHook->ClearAll();
    
    CStdRMutex oLock( GetLock() );
    for( auto elem : m_mapTimWatch )
        RemoveSource( elem.second );

    for( auto elem : m_mapIoWatch )
        RemoveSource( elem.second );

    for( auto elem : m_mapAsyncWatch )
        RemoveSource( elem.second );
}

CEvLoop::CEvLoop( const IConfigDb* pCfg )
    : CSimpleEvPoll( pCfg )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg( pCfg );
        gint32 ret = oCfg.GetPointer(
            propIoMgr, m_pIoMgr );

        if( ERROR( ret ) )
            break;

        CParamList oParams;
        oParams.Push( ObjPtr( this ) );
        ret = m_pDBusHook.NewObj(
            clsid( CDBusLoopHooks ),
            oParams.GetCfg() );

    }while( 0 );

    if( ERROR( ret ) )
    {
        throw std::runtime_error(
            DebugMsg( ret,
            "Error in CEvLoop's ctor" ) );
    }

}

gint32 CEvLoop::SetupDBusConn(
    DBusConnection* pConn )
{
    if( pConn == nullptr )
        return -EINVAL;

    // setup the dbus hooks
    return m_pDBusHook->Start( pConn );
}

CEvLoop::~CEvLoop()
{
    ClearAll();
    std::map< HANDLE, SOURCE_HEADER* >*
        pMap = GetMap( srcAsync );
    while( !pMap->empty() )
    {
        auto p = pMap->begin()->second;
        RemoveSource( p );
    }
}

guint32 CEvLoop::AddSource(
    SOURCE_HEADER* pSource )
{
    if( pSource == nullptr )
        return -EINVAL;

    CStdRMutex oLock( GetLock() );
    HANDLE hKey = ( HANDLE )pSource;
    EnumSrcType iType = pSource->m_bySrcType;

    std::map< HANDLE, SOURCE_HEADER* >*
        pMap = GetMap( iType );

    if( pMap == nullptr )
        return -EFAULT;

    if( pMap->find( hKey ) != pMap->end() )
        return -EEXIST;

    ( *pMap )[ hKey ] = pSource;

    if( pSource->GetState() == srcsReady )
    {
        if( iType == srcIo )
            m_bIoAdd = true;
        else if( iType == srcTimer )
            m_bTimeAdd = true;
    }

    return 0;
}

// remove from the mainloop's notify
guint32 CEvLoop::RemoveSource(
    SOURCE_HEADER* pSource )
{
    if( pSource == nullptr )
        return -EINVAL;

    CStdRMutex oLock( GetLock() );
    HANDLE hKey = ( HANDLE )pSource;
    EnumSrcType iType = pSource->m_bySrcType;

    std::map< HANDLE, SOURCE_HEADER* >*
        pMap = GetMap( iType );

    if( pMap == nullptr )
        return -EFAULT;

    // there is a chance the RemoveSource can be
    // reentered in the delete, and therefore, it
    // is necessary to remove the source from the
    // map before the source get deleted
    if( pMap->find( hKey ) == pMap->end() )
        return -ENOENT;

    SOURCE_HEADER* p = (*pMap)[ hKey ];
    EnumSrcState iState = p->GetState();
    ( *pMap ).erase( hKey );
    delete p;

    if( iState == srcsReady )
    {
        if( iType == srcIo )
            m_bIoRemove = true;
        else if( iType == srcTimer )
            m_bTimeRemove = true;
    }

    return 0;
}

// remove from the user side
guint32 CEvLoop::RemoveSource(
    HANDLE hKey, EnumSrcType iType )
{
    if( hKey == INVALID_HANDLE )
        return -EINVAL;

    CStdRMutex oLock( GetLock() );
    std::map< HANDLE, SOURCE_HEADER* >*
        pMap = GetMap( iType );

    if( pMap == nullptr )
        return -EFAULT;

    if( pMap->find( hKey ) == pMap->end() )
        return -ENOENT;

    SOURCE_HEADER* p = (*pMap)[ hKey ];
    EnumSrcState iState = p->GetState();
    ( *pMap ).erase( hKey );
    delete p;

    if( iState == srcsReady )
    {
        if( iType == srcIo )
            m_bIoRemove = true;
        else if( iType == srcTimer )
            m_bTimeRemove = true;
    }
    return 0;
}

// timer
gint32 CEvLoop::AddTimerWatch(
    TaskletPtr& pCallback,
    guint32& hTimer )
{
    gint32 ret = 0;
    do{
        if( pCallback.IsEmpty() )
        {
            ret = -EINVAL;
            break;
        }

        CParamList oParams( ( IConfigDb* )
            pCallback->GetConfig() );

        if( oParams.GetCount() < 2 )
        {
            ret = -EINVAL;
            break;
        }

        guint32 dwIntervalMs = oParams[ 0 ];
        bool bRepeat = ( bool& )oParams[ 1 ];
        bool bStart = true;
        if( oParams.exist( 2 ) )
            bStart = oParams[ 2 ];

        TIMER_SOURCE* pTimer = new TIMER_SOURCE(
            this, dwIntervalMs,
            bRepeat, pCallback );

        if( pTimer == nullptr )
        {
            ret = -ENOMEM;
            break;
        }

        ret = AddSource( pTimer );
        if( ERROR( ret ) )
            break;

        if( bStart )
        {
            ret = pTimer->Start();
            if( ERROR( ret ) )
                break;
        }

        hTimer = ( HANDLE )pTimer;

    }while( 0 );

    return ret;
}

gint32 CEvLoop::RemoveTimerWatch( HANDLE hTimer )
{
    return RemoveSource( hTimer, srcTimer );
}

guint64 CEvLoop::NowUs()
{
    return CSimpleEvPoll::NowUs();
}

// io watcher
gint32 CEvLoop::AddIoWatch(
    TaskletPtr& pCallback,
    guint32& hWatch )
{
    gint32 ret = 0;
    do{
        if( pCallback.IsEmpty() )
        {
            ret = -EINVAL;
            break;
        }

        CParamList oParams( ( IConfigDb* )
            pCallback->GetConfig() );
        if( oParams.GetCount() <= 2 )
        {
            ret = -EINVAL;
            break;
        }

        guint32 dwFd = oParams[ 0 ];
        EnumClsid dwOpt = oParams[ 1 ];
        bool bStart = ( bool& )oParams[ 2 ];

        IO_SOURCE* pIow = new IO_SOURCE(
            this, ( gint32 )dwFd,
            dwOpt, pCallback );

        if( pIow == nullptr )
        {
            ret = -ENOMEM;
            break;
        }

        ret = AddSource( pIow );
        if( ERROR( ret ) )
            break;

        if( bStart )
        {
            ret = pIow->Start();
            if( ERROR( ret ) )
                break;
        }

        hWatch = ( HANDLE )pIow;        

    }while( 0 );

    return ret;
}

gint32 CEvLoop::RemoveIoWatch(
    guint32 hWatch )
{
    return RemoveSource( hWatch, srcIo );
}

gint32 CEvLoop::IsWatchDone(
    HANDLE hWatch, EnumSrcType iType )
{
    CStdRMutex oLock( GetLock() );
    std::map< HANDLE, SOURCE_HEADER* >*
        pMap = GetMap( iType );

    if( pMap == nullptr )
        return EFAULT;

    if( pMap->find( hWatch ) == pMap->end() )
        return 0;

    SOURCE_HEADER* pSrc = ( *pMap )[ hWatch ];
    if( pSrc->GetState() == srcsDone )
        return 0;

    return ERROR_FALSE;
}

// tasks on the mainloop queue
gint32 CEvLoop::AddAsyncWatch(
   TaskletPtr& pCallback,
   HANDLE& hWatch )
{
    gint32 ret = 0;
    do{
        if( pCallback.IsEmpty() )
        {
            ret = -EINVAL;
            break;
        }

        CParamList oParams( ( IConfigDb* )
            pCallback->GetConfig() );

        ASYNC_SOURCE* pAsync =
            new ASYNC_SOURCE( this, pCallback );

        if( pAsync == nullptr )
        {
            ret = -ENOMEM;
            break;
        }

        bool bStart = ( bool& )oParams[ 0 ];
        ret = AddSource( pAsync );
        if( ERROR( ret ) )
            break;

        if( bStart )
        {
            ret = pAsync->Start();
            if( ERROR( ret ) )
                break;
        }

        hWatch = ( HANDLE )pAsync;

    }while( 0 );

    return ret;
}

gint32 CEvLoop::RemoveAsyncWatch(
   HANDLE hWatch )
{
    return RemoveSource( hWatch, srcAsync );
}

gint32 CEvLoop::GetSource(
    HANDLE hWatch,
    EnumSrcType iType,
    SOURCE_HEADER*& pSrc )
{
    std::map< HANDLE, SOURCE_HEADER* >*
        pMap = GetMap( iType );

    if( pMap == nullptr )
        return -EFAULT;

    if( pMap->find( hWatch ) == pMap->end() )
        return -ENOENT;

    pSrc = ( *pMap )[ hWatch ];
    return 0;
}

void CEvLoop::WakeupLoop()
{ WakeupLoop( aevtWakeup ); }

gint32 CEvLoop::AsyncSend( HANDLE hWatch )
{
    gint32 ret = 0;
    do{
        if( hWatch == INVALID_HANDLE )
        {
            ret = -EINVAL;
            break;
        }

        CStdRMutex oLock( GetLock() );
        SOURCE_HEADER* pHeader = nullptr;
        ret = GetSource( hWatch,
            srcAsync, pHeader );

        if( ERROR( ret ) )
            break;

        ASYNC_SOURCE* pAs = static_cast
            < ASYNC_SOURCE* >( pHeader );
        // trigger the task to process the task
        // queue.
        //
        // this command is thread-safe
        pAs->Send();

    }while( 0 );

    return ret;
}

CEvLoopStopCb::CEvLoopStopCb(
    const IConfigDb* pCfg )
{
    SetClassId( clsid( CEvLoopStopCb ) );
    gint32 ret = 0;
    do{
        ObjPtr pObj;
        CCfgOpener oCfg( pCfg );
        ret = oCfg.GetObjPtr( 0, pObj );
        if( ERROR( ret ) )
            break;
        m_pLoop = pObj;
        if( m_pLoop == nullptr )
        {
            ret = -EINVAL;
            break;
        }

    }while( 0 );

    if( ERROR( ret ) )
    {
        throw std::runtime_error(
            DebugMsg( ret,
            "Error in CEvLoopStopCb's ctor" ) );
    }
}
gint32 CEvLoopStopCb::operator()(
    guint32 dwContext )
{
    m_pLoop->m_pDBusHook->SetSetupDone( false );
    ( ( CMainIoLoop* )m_pLoop )->SetTid( 0 );
    Sem_Post( &m_semSync );
    m_pLoop->SetStop( true );
    return 0;
}

CEvLoopAsyncCallback::CEvLoopAsyncCallback(
    const IConfigDb* pCfg )
{
    SetClassId( clsid( CEvLoopAsyncCallback ) );
    gint32 ret = 0;
    do{
        ObjPtr pObj;
        CCfgOpener oCfg( pCfg );
        ret = oCfg.GetObjPtr( 0, pObj );
        if( ERROR( ret ) )
            break;
        m_pLoop = pObj;
        if( m_pLoop == nullptr )
        {
            ret = -EINVAL;
            break;
        }

    }while( 0 );

    if( ERROR( ret ) )
    {
        throw std::runtime_error(
            DebugMsg( ret,
            "Error in CEvLoopAsyncCallback's ctor" ) );
    }
}


gint32 CEvLoopAsyncCallback::operator()(
    guint32 dwContext )
{
    gint32 ret = 0;
    do{
        ret = HandleCommand();
        if( ret == -EAGAIN )
        {
            ret = G_SOURCE_CONTINUE;
            break;
        }
        if( ERROR( ret ) )
        {
            ret = G_SOURCE_REMOVE;
        }
    }while( 1 );

    return SetError( ret );
}
gint32 CEvLoopAsyncCallback::HandleCommand()
{
    EnumAsyncEvent iEvent = aevtInvalid;
    gint32 ret = 0;

    ret = m_pLoop->ReadAsyncEvent( iEvent );
    if( ERROR( ret ) )
        return ret;

    switch( iEvent )
    {
    case aevtRunSched:
        {
            CMainIoLoop::TaskCallback(
                ( CMainIoLoop* )m_pLoop );
            ret = G_SOURCE_CONTINUE;
            break;
        }
    case aevtRunAsync:
        {
            guint32 hWatch = INVALID_HANDLE;
            ret = m_pLoop->ReadAsyncData(
                ( guint8* )&hWatch,
                sizeof( hWatch ) );
            if( ret == -EAGAIN )
            {
                // data not arrive completely
                // FOR EAGAIN, the caller has special
                // handling
                m_pLoop->AsyncDataUnwind( 1 );
                break;
            }
            if( ERROR( ret ) )
            {
                // fatal error
                ret = G_SOURCE_REMOVE;
                break;
            }

            ret = m_pLoop->RunSource(
                hWatch, srcAsync,
                eventAsyncWatch );

            break;
        }
    case aevtWakeup:
    default:
        {
            ret = G_SOURCE_CONTINUE;
            break;
        }
    }
    return ret;
}

std::map< HANDLE, CEvLoop::SOURCE_HEADER* >*
CEvLoop::GetMap( EnumSrcType iType )
{
    switch( iType )
    {
    case srcTimer:
        return &m_mapTimWatch;
    case srcIo:
        return &m_mapIoWatch;
    case srcAsync:
        return &m_mapAsyncWatch;
    default:
        break;
    }
    return nullptr;
}

#endif
