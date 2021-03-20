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
 *      Copyright:  2019 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
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

namespace rpcfrmwrk
{

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

extern void DumpTask( TaskletPtr& pTask );

class IMainLoop: public IService
{
public:
    // io watcher + timer
    virtual gint32 SetupDBusConn(
        DBusConnection* pConn ) = 0;

    // io watcher + timer
    virtual gint32 StopDBusConn() = 0;

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
        HANDLE& hWatch ) = 0;

    virtual gint32 RemoveAsyncWatch(
        HANDLE hWatch ) = 0;

    virtual gint32 OnPreLoop() = 0;
    virtual gint32 OnPostLoop() = 0;
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
    gint32 PopHead();
    gint32 GetHead( TaskletPtr& pTask );
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
    std::string                     m_strName = "MainLoop";
    bool                            m_bNewThread = true;
    std::deque< TaskletPtr >        m_queQuitTasks;

    public:

    typedef T super;
    CMainIoLoopT( const IConfigDb* pCfg = nullptr )
        : super( pCfg ),
        m_queTasks( true ),
        m_pThread( nullptr ),
        m_dwTid( 0 ),
        m_hTaskWatch( 0 ),
        m_bTaskDone( true )
    {
        this->SetClassId( clsid( CMainIoLoop ) );

        if( pCfg == nullptr )
            return;

        CCfgOpener oCfg( pCfg );
        gint32 ret = oCfg.GetStrProp( 0, m_strName );
        if( ERROR( ret ) )
            return;

        oCfg.GetBoolProp( 1, m_bNewThread );
        return;
    }

    ~CMainIoLoopT()
    {
        if( m_pThread != nullptr )
        {
            delete m_pThread;
            m_pThread = nullptr;
        }
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
        super::Stop();
        if( m_pThread != nullptr )
        {
            m_pThread->join();
            delete m_pThread;
            m_pThread = nullptr;
        }

        return 0;
    }

    void AddTask( TaskletPtr& pTask )
    {
        CStdRMutex oLock( this->GetLock() );
        m_queTasks->AddTask( pTask );
        InstallTaskSource();
    }

    void AddQuitTask( TaskletPtr& pTask )
    {
        CStdRMutex oLock( this->GetLock() );
        m_queQuitTasks.push_back( pTask );
    }

    virtual gint32 OnPreLoop() 
    {
        // there are already some tasks befor the
        // loop starts
        CStdRMutex oLock( this->GetLock() );
        if( GetTaskQue()->GetSize() > 0 )
            InstallTaskSource();
        return 0;
    }

    virtual gint32 OnPostLoop() 
    {
        gint32 ret = 0;
        std::vector< TaskletPtr > vecTasks;
        CStdRMutex oLock( this->GetLock() );
        while( GetTaskQue()->GetSize() > 0 )
        {
            TaskletPtr pElem;
            ret = GetTaskQue()->GetHead( pElem ); 
            if( SUCCEEDED( ret ) )
                vecTasks.push_back( pElem );
            GetTaskQue()->PopHead();
        }

        std::deque< TaskletPtr > queQuitTasks =
            m_queQuitTasks;
        m_queQuitTasks.clear();
        oLock.Unlock();
        for( auto elem : vecTasks )
            ( *elem )( eventCancelTask );

        // run the exit tasks
        for( auto elem : queQuitTasks )
            ( *elem )( eventZero );

        return 0;
    }

    static gboolean TaskCallback(
        gpointer pdata )
    {
        CMainIoLoopT* pLoop =
            reinterpret_cast< CMainIoLoopT* >( pdata );

        while( !pLoop->IsStopped() )
        {
            gint32 ret = 0;
            TaskletPtr pTask;

            CStdRMutex oLock( pLoop->GetLock() );

            TaskQuePtr& pQue =
                pLoop->GetTaskQue();

            ret = pQue->GetHead( pTask );
            if( ERROR( ret ) )
            {
                pLoop->SetTaskDone( true );
                break;
            }
            pQue->PopHead();
            oLock.Unlock();

            ( *pTask )( eventZero );
            DumpTask( pTask );
        }
        return G_SOURCE_REMOVE;
    }

    gint32 ThreadProc()
    {

        gint32 ret = 0;
        SetThreadName( m_strName );
        SetTid( rpcfrmwrk::GetTid() );
        super::Start();
        SetTid( 0 );
        // Place OnPostLoop here, because the tid
        // is cleared, which is used as a flag
        // whether the loop is running.
        OnPostLoop();
        this->RemoveAsyncWatch(
            m_hTaskWatch );

        m_pSchedCb.Clear();

        return ret;
    }

    gint32 Start()
    {
        gint32 ret = 0;

        // c++11 required for std::bind
        std::function<gint32()> oThreadProc =
            std::bind( &CMainIoLoopT::ThreadProc, this );

        if( m_bNewThread )
        {
            m_pThread = new std::thread( oThreadProc );
            if( m_pThread == nullptr )
                ret = -EFAULT;
        }
        else
        {
            ret = ThreadProc();
        }
        return ret;
    }

    gint32 InstallTaskSource();
};

}
#ifdef _USE_LIBEV
#include "evloop.h"
namespace rpcfrmwrk
{
typedef  CMainIoLoopT< CEvLoop > CMainIoLoop;
}
#else
#error _USE_LIBEV is not defined
#endif

