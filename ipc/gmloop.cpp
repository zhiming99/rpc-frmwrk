/*
 * =====================================================================================
 *
 *       Filename:  gmloop.cpp
 *
 *    Description:  implementation the io loop with glib's mainloop
 *
 *        Version:  1.0
 *        Created:  09/23/2018 09:34:06 AM
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
#include <dbus/dbus.h>
#include "configdb.h"
#include "frmwrk.h"

#ifndef _USE_LIBEV
#include "gmloop.h"

using namespace std;

CGMainLoop::TIMER_SOURCE::TIMER_SOURCE(
    CGMainLoop* pLoop,
    bool bRepeat,
    guint32 dwInterval,
    TaskletPtr& pCallback )
    : SOURCE_HEADER( pLoop, srcTimer, pCallback )
{
    m_bRepeat = bRepeat;
    m_dwIntervalMs = dwInterval;
    m_pCallback = pCallback;
    InitSource( dwInterval );
}

CGMainLoop::TIMER_SOURCE::~TIMER_SOURCE()
{
    if( m_pTimerSrc != nullptr )
    {
        g_source_destroy( m_pTimerSrc );
        g_source_unref( m_pTimerSrc );
        m_pTimerSrc = nullptr;
    }
}

gint32 CGMainLoop::TIMER_SOURCE::InitSource(
    guint32 dwInterval )
{
    m_pTimerSrc = g_timeout_source_new(
        m_dwIntervalMs );

    if( m_pTimerSrc == nullptr )
        return -EFAULT;

    g_source_set_callback( m_pTimerSrc,
        TimerCallback,
        ( gpointer )this,
        TimerRemove );

    return 0;
}

gint32 CGMainLoop::TIMER_SOURCE::Start()
{
    if( m_pTimerSrc == nullptr )
        return ERROR_STATE;

    g_source_attach( m_pTimerSrc,
        m_pParent->GetMainCtx() );

    return 0;
}

void CGMainLoop::TIMER_SOURCE::TimerRemove(
    gpointer dwParams )
{
    TIMER_SOURCE* pThis =
        ( TIMER_SOURCE* )dwParams;
    CGMainLoop* pParent = pThis->m_pParent;
    CStdRMutex oLock( pParent->GetLock() );
    pParent->RemoveSource( pThis );    
}

gboolean CGMainLoop::TIMER_SOURCE::TimerCallback(
    gpointer dwParams )
{
    TIMER_SOURCE* pSrc =
        ( TIMER_SOURCE* )dwParams;

    if( !pSrc->m_pCallback.IsEmpty() )
    {
        pSrc->m_pCallback( eventTimeout,
            0, 0, nullptr );
    }

    if( pSrc->m_bRepeat )
        return G_SOURCE_CONTINUE;

    pSrc->SetState( srcsDone );

    return G_SOURCE_REMOVE;
}

CGMainLoop::IDLE_SOURCE::IDLE_SOURCE(
    CGMainLoop* pLoop, TaskletPtr& pCallback )
    :SOURCE_HEADER( pLoop, srcIdle, pCallback ),
    m_pIdleSource( nullptr )
{
    InitSource();
}

CGMainLoop::IDLE_SOURCE::~IDLE_SOURCE()
{
    g_source_unref( m_pIdleSource );
}

gint32 CGMainLoop::IDLE_SOURCE::InitSource()
{
    m_pIdleSource =
        g_idle_source_new();

    g_source_set_callback(
        m_pIdleSource, IdleCallback,
        ( gpointer )this, IdleRemove );

    return 0;
}

gint32 CGMainLoop::IDLE_SOURCE::Start()
{
    if( m_pIdleSource == nullptr )
        return ERROR_STATE;

    g_source_attach( m_pIdleSource,
        m_pParent->GetMainCtx() );

    return 0;
}

void CGMainLoop::IDLE_SOURCE::IdleRemove(
    gpointer dwParams )
{
    IDLE_SOURCE* pThis =
        ( IDLE_SOURCE* )dwParams;

    CGMainLoop* pParent = pThis->m_pParent;
    CStdRMutex oLock( pParent->GetLock() );
    pParent->RemoveSource( pThis );    
}

gboolean CGMainLoop::IDLE_SOURCE::IdleCallback(
    gpointer pdata )
{
    TIMER_SOURCE* pSrc =
        ( TIMER_SOURCE* )pdata;

    if( !pSrc->m_pCallback.IsEmpty() )
    {
        pSrc->m_pCallback->OnEvent(
            eventAsyncWatch, 0, 0, nullptr );
    }
    pSrc->SetState( srcsDone );

    return G_SOURCE_REMOVE;
}

CGMainLoop::IO_SOURCE::IO_SOURCE(
    CGMainLoop* pLoop,
    gint32 iFd,
    guint32 dwOpt,
    TaskletPtr& pCallback )
    : SOURCE_HEADER( pLoop, srcIo, pCallback )
{
    m_iFd = iFd;
    m_dwOpt = dwOpt;
    InitSource( iFd, dwOpt );
}

CGMainLoop::IO_SOURCE::~IO_SOURCE()
{
    if( m_pIoSource != nullptr )
    {
        g_source_destroy( m_pIoSource );
        g_source_unref( m_pIoSource );
        m_pIoSource = nullptr;
    }

    if( m_pIoChannel != nullptr )
    {
        g_io_channel_unref( m_pIoChannel );
        m_pIoChannel = nullptr;
    }
}

gint32 CGMainLoop::IO_SOURCE::InitSource(
    gint32 iFd, guint32 dwOpt )
{
    if( m_pIoSource == nullptr || iFd < 0 )
        return -EINVAL;

    m_pIoChannel =
        g_io_channel_unix_new( iFd );

    if( m_pIoChannel == nullptr )
        return -ENOMEM;

    m_pIoSource = g_io_create_watch(
        m_pIoChannel,
        ( GIOCondition )dwOpt );

    if( m_pIoSource == nullptr )
        return -ENOMEM;

    g_source_set_callback( m_pIoSource,
        (GSourceFunc)IoCallback,
        this, IoRemove );

    return 0;
}

gint32 CGMainLoop::IO_SOURCE::Start()
{
    if( m_pIoSource == nullptr )
        return ERROR_STATE;

    g_source_attach( m_pIoSource,
        m_pParent->GetMainCtx() );

    return 0;
}

gboolean CGMainLoop::IO_SOURCE::IoCallback(
    GIOChannel* source,
    GIOCondition condition,
    gpointer data)
{
    gint32 ret = 0;
    IO_SOURCE* pSrc =
        ( IO_SOURCE* )data;
    if( !pSrc->m_pCallback.IsEmpty() )
    {
        ret = pSrc->m_pCallback->OnEvent(
            eventIoWatch,
            ( guint32 )condition,
            0, nullptr );
    }
    else
    {
        pSrc->SetState( srcsDone );
        ret = G_SOURCE_REMOVE;
    }
    return ret;
}

void CGMainLoop::IO_SOURCE::IoRemove(
    gpointer pdata )
{
    IO_SOURCE* pThis = ( IO_SOURCE* )pdata;
    CGMainLoop* pParent = pThis->m_pParent;
    CStdRMutex oLock( pParent->GetLock() );
    pParent->RemoveSource( pThis );    
    return;
}

gint32 CGMainLoop::Start()
{
    // now we have a way to inject the command
    // to the main loop

    // for the tasks added before start
    g_main_context_push_thread_default( m_pMainCtx );
    g_main_loop_run( m_pMainLoop );
    return 0;
}

gint32 CGMainLoop::Stop()
{
    g_main_loop_quit( m_pMainLoop );
    return 0;
}

CGMainLoop::CGMainLoop( const IConfigDb* pCfg )
{
    ObjPtr pObj;
    CCfgOpener a( pCfg );
    a.GetObjPtr( propIoMgr, pObj );
    m_pIoMgr = ( CIoManager* )pObj;
    m_pMainCtx = g_main_context_new();

    m_pMainLoop = g_main_loop_new(
        m_pMainCtx, false );
}

gint32 CGMainLoop::SetupDBusConn(
    DBusConnection* pConn )
{
    if( pConn == nullptr )
        return -EINVAL;

    if( m_pMainCtx == nullptr )
        return ERROR_STATE;

    dbus_connection_setup_with_g_main(
        pConn, m_pMainCtx );

    return 0;
}

CGMainLoop::~CGMainLoop()
{
    g_main_context_unref( m_pMainCtx );
    g_main_loop_unref( m_pMainLoop );
}

guint32 CGMainLoop::AddSource(
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

    return 0;
}

// remove from the mainloop's notify
guint32 CGMainLoop::RemoveSource(
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

    SOURCE_HEADER* p = (*pMap)[ hKey ];
    ( *pMap ).erase( hKey );
    delete p;

    return 0;
}

// remove from the user side
guint32 CGMainLoop::RemoveSource(
    HANDLE hKey, EnumSrcType iType )
{
    if( hKey == INVALID_HANDLE )
        return -EINVAL;

    CStdRMutex oLock( GetLock() );
    std::map< HANDLE, SOURCE_HEADER* >*
        pMap = GetMap( iType );

    if( pMap == nullptr )
        return -EFAULT;

    SOURCE_HEADER* p = (*pMap)[ hKey ];
    ( *pMap ).erase( hKey );
    delete p;

    return 0;
}

// timer
gint32 CGMainLoop::AddTimerWatch(
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
        bool bStart = ( bool& )oParams[ 2 ];

        TIMER_SOURCE* pTimer = new TIMER_SOURCE(
            this, bRepeat,
            dwIntervalMs, pCallback );

        if( pTimer == nullptr )
        {
            ret = -ENOMEM;
            break;
        }

        ret = AddSource( pTimer );
        if( ERROR( ret ) )
            break;

        oParams.Push( ( guint32 )pTimer );
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

gint32 CGMainLoop::RemoveTimerWatch( HANDLE hTimer )
{
    return RemoveSource( hTimer, srcTimer );
}

guint64 CGMainLoop::NowUs()
{
    return g_get_monotonic_time();
}

// io watcher
gint32 CGMainLoop::AddIoWatch(
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
        guint32 dwOpt = oParams[ 1 ];
        bool bStart = ( bool& )oParams[ 2 ];

        IO_SOURCE* pIow = new IO_SOURCE(
            this, ( gint32 )dwFd,
            dwOpt, pCallback );

        if( pIow == nullptr )
        {
            ret = -ENOMEM;
            break;
        }

        oParams.Push( ( guint32 )pIow );
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

gint32 CGMainLoop::RemoveIoWatch(
    guint32 hWatch )
{
    return RemoveSource( hWatch, srcIo );
}

gint32 CGMainLoop::IsWatchDone(
    HANDLE hWatch, EnumSrcType iType )
{
    // not supported yet
    return -ENOTSUP;
}

// idle watcher
// Start a one-shot idle watcher to execute the
// tasks on the mainloop queue
gint32 CGMainLoop::AddIdleWatch(
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

        IDLE_SOURCE* pIdle =
            new IDLE_SOURCE( this, pCallback );

        if( pIdle == nullptr )
        {
            ret = -ENOMEM;
            break;
        }

        bool bStart = ( bool& )oParams[ 0 ];

        ret = AddSource( pIdle );
        if( ERROR( ret ) )
            break;

        oParams.Push( ( guint32 )pIdle );
        if( bStart )
        {
            ret = pIdle->Start();
            if( ERROR( ret ) )
                break;
        }
        hWatch = ( HANDLE )pIdle;

    }while( 0 );

    return ret;
}

gint32 CGMainLoop::RemoveIdleWatch(
   HANDLE hWatch )
{
    return RemoveSource( hWatch, srcIdle );
}

gint32 CGMainLoop::GetSource(
    HANDLE hWatch,
    EnumSrcType iType,
    SOURCE_HEADER*& pSrc )
{
    if( pSrc == nullptr )
        return -EINVAL;

    CStdRMutex oLock( GetLock() );
    std::map< HANDLE, SOURCE_HEADER* >*
        pMap = GetMap( iType );

    if( pMap == nullptr )
        return -EFAULT;

    if( pMap->find( hWatch ) == pMap->end() )
        return -ENOENT;

    pSrc = ( *pMap )[ hWatch ];
    return 0;
}
#endif
