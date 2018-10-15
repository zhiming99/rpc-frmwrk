/*
 * =====================================================================================
 *
 *       Filename:  mainloop.h
 *
 *    Description:  Declarations of mainloop related classes
 *
 *        Version:  1.0
 *        Created:  09/21/2018 09:17:54 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:  
 *
 * =====================================================================================
 */

#pragma once

#include <json/json.h>
#include <vector>
#include <unistd.h>
#include <functional>
#include <sys/syscall.h>
#include "namespc.h"
#include "defines.h"
#include "tasklets.h"

enum EnumSrcType : guint32
{
    srcTimer,
    srcIo,
    srcAsync,
    srcIdle
};

enum EnumSrcState : guint8
{
    srcsStarting,
    srcsReady,
    srcsDone,
};

class IMainLoop: public IService
{
public:
    // io watcher + timer
    virtual gint32 SetupDBusConn(
        DBusConnection* pConn ) = 0;

    // timer
    virtual gint32 AddTimerWatch(
        TaskletPtr& pCallback,
        HANDLE& hTimer ) = 0;

    virtual gint32 RemoveTimerWatch(
        HANDLE hTimer ) = 0;

    virtual guint64 NowUs() = 0;

    // NOTE: all the Callback tasks must return
    // G_SOURCE_REMOVE and G_SOURCE_CONTINUE on
    // return to indicate whether to remove or
    // keep watching
    //
    // io watcher
    virtual gint32 AddIoWatch(
        TaskletPtr& pCallback,
        HANDLE& hWatch ) = 0;

    virtual gint32 RemoveIoWatch(
        HANDLE hWatch ) = 0;

    // idle watcher
    // it runs once and is removed after done.
    virtual gint32 AddIdleWatch(
       TaskletPtr& pCallback,
       HANDLE& hWatch ) = 0;

    virtual gint32 IsWatchDone(
        HANDLE hWatch, EnumSrcType itype ) = 0;

    virtual gint32 RemoveIdleWatch(
       HANDLE hWatch ) = 0;

    // async watcher
    virtual gint32 AddAsyncWatch(
        TaskletPtr& pCallback,
        guint32& hWatch ) = 0;

    virtual gint32 RemoveAsyncWatch(
        guint32 hWatch ) = 0;
};

typedef CAutoPtr< clsid( Invalid ), IMainLoop > MloopPtr;

class CSchedTaskCallback : public CTasklet
{
    public:
    typedef CTasklet super;
    CSchedTaskCallback( const IConfigDb* pCfg )
        : super( pCfg )
    {
        SetClassId( clsid( CSchedTaskCallback ) );
    }

    gint32 operator()( guint32 dwContext );
};

class CTaskQueue : public CObjBase
{
    protected:

    std::deque< TaskletPtr >    m_queTasks;
    std::recursive_mutex        m_oLock;

    public:

    CTaskQueue();
    void AddTask( TaskletPtr& pTask );
    gint32 RemoveTask( TaskletPtr& pTask );
    gint32 PopHead( TaskletPtr& pTask );
    gint32 GetSize();
};

typedef CAutoPtr< Clsid_CTaskQueue, CTaskQueue > TaskQuePtr;

template< class T, typename T2 = typename std::enable_if<
    std::is_base_of< IMainLoop, T >::value, T >::type >
class CMainIoLoopT : public T
{
    TaskQuePtr                      m_queTasks;
    std::thread                     *m_pThread;
    guint32                         m_dwTid;
    HANDLE                          m_hTaskWatch;
    TaskletPtr                      m_pSchedCb;
    std::atomic<bool>               m_bTaskDone;

    public:

    typedef T super;
    CMainIoLoopT( const IConfigDb* pCfg = nullptr )
        : super( pCfg ),
        m_queTasks( true ),
        m_dwTid( 0 ),
        m_hTaskWatch( 0 ),
        m_bTaskDone( true )
    {
        this->SetClassId( clsid( CMainIoLoop ) );
    }

    TaskQuePtr& GetTaskQue() const
    { return ( TaskQuePtr& )m_queTasks; }

    guint32 GetThreadId() const
    { return m_dwTid; }

    void SetTaskDone( bool bDone )
    { m_bTaskDone = bDone; }

    bool IsTaskDone() const
    { return m_bTaskDone; }

    void SetTid( guint32 dwTid )
    { m_dwTid = dwTid; }

    gint32 Stop()
    {
#ifndef _USE_LIBEV
        this->RemoveIdleWatch(
            m_hTaskWatch );
#endif

        super::Stop();
        if( m_pThread  )
        {
            m_pThread->join();
            delete m_pThread;
            m_pThread = nullptr;
        }
#ifdef _USE_LIBEV
        this->RemoveAsyncWatch(
            m_hTaskWatch );
#endif
        m_dwTid = 0;
        m_pSchedCb.Clear();
        return 0;
    }

    void AddTask( TaskletPtr& pTask )
    {
        CStdRMutex oLock( this->GetLock() );
        m_queTasks->AddTask( pTask );
        InstallTaskSource();
    }

    static gboolean TaskCallback(
        gpointer pdata )
    {
        CMainIoLoopT* pLoop =
            reinterpret_cast< CMainIoLoopT* >( pdata );

        while( 1 )
        {
            gint32 ret = 0;
            TaskletPtr pTask;

            CStdRMutex oLock( pLoop->GetLock() );

            TaskQuePtr& pQue =
                pLoop->GetTaskQue();

            ret = pQue->PopHead( pTask );
            if( ERROR( ret ) )
            {
                pLoop->SetTaskDone( true );
                break;
            }
            oLock.Unlock();

            ( *pTask )( ( guint32 )pdata );
        }
        return G_SOURCE_REMOVE;
    }

    gint32 ThreadProc()
    {

        gint32 ret = 0;
        SetThreadName( "MainLoop" );
        m_dwTid = ::GetTid();
        super::Start();

        return ret;
    }

    gint32 Start()
    {
        gint32 ret = 0;

        // c++11 required for std::bind
        std::function<gint32()> oThreadProc =
            std::bind( &CMainIoLoopT::ThreadProc, this );

        m_pThread = new std::thread( oThreadProc );
        if( m_pThread == nullptr )
            ret = -EFAULT;

        return ret;
    }

    gint32 InstallTaskSource();
};

#ifdef _USE_LIBEV
#include "evloop.h"
typedef  CMainIoLoopT< CEvLoop > CMainIoLoop;
#else
#include "gmloop.h"
typedef  CMainIoLoopT< CGMainLoop > CMainIoLoop;
#endif

