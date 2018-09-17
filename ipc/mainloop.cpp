/*
 * =====================================================================================
 *
 *       Filename:  mainloop.cpp
 *
 *    Description:  implementation of CMainIoLoop
 *
 *        Version:  1.0
 *        Created:  10/03/2016 11:50:47 AM
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
#include "irp.h"
#include "port.h"
#include "dbusport.h"
#include "frmwrk.h"

using namespace std;

CMainIoLoop::CMainIoLoop( const IConfigDb* pCfg ) :
    m_queTasks( true )
{
    SetClassId( clsid( CMainIoLoop ) );
    CCfgOpener a( pCfg );

    ObjPtr pObj;
    a.GetObjPtr( propIoMgr, pObj );
    m_pIoMgr = pObj;

    m_dwTid = 0;
    m_pTaskSrc = nullptr;

    m_pMainCtx = g_main_context_new();
    m_pMainLoop = g_main_loop_new( m_pMainCtx, false );

}

CMainIoLoop::~CMainIoLoop()
{

    if( m_pTaskSrc )
    {
        // g_source_destroy( m_pTaskSrc );
        g_source_unref( m_pTaskSrc );
    }

    g_main_context_unref( m_pMainCtx );
    g_main_loop_unref( m_pMainLoop );
}

gint32 CMainIoLoop::Stop()
{
    g_main_loop_quit( m_pMainLoop );
    if( m_pThread  )
    {
        m_pThread->join();
        delete m_pThread;
        m_pThread = nullptr;
    }
    m_dwTid = 0;
    return 0;
}

void CMainIoLoop::AddTask( TaskletPtr& pTask )
{
    m_queTasks->AddTask( pTask );
    InstallTaskSource();
}

gboolean CMainIoLoop::TaskCallback( gpointer pdata )
{
    CMainIoLoop* pLoop =
        reinterpret_cast< CMainIoLoop* >( pdata );

    while( 1 )
    {
        gint32 ret = 0;
        TaskletPtr pTask;

        ret = pLoop->GetTaskQue()->PopHead( pTask );

        if( ERROR( ret ) )
            break;

        ( *pTask )( ( guint32 )pdata );
    }

    return G_SOURCE_REMOVE;
}

gint32 CMainIoLoop::ThreadProc()
{

    gint32 ret = 0;

    SetThreadName( "MainLoop" );

    m_dwTid = GetTid();

    // now we have a way to inject the command
    // to the main loop

    // for the tasks added before start
    InstallTaskSource();

    g_main_context_push_thread_default( m_pMainCtx );
    g_main_loop_run( m_pMainLoop );

    // wait for those thread to quit
    GetIoMgr()->WaitThreadsQuit();
    return ret;
}

gint32 CMainIoLoop::Start()
{
    gint32 ret = 0;

    // c++11 required for std::bind
    function<gint32()> oThreadProc =
        std::bind( &CMainIoLoop::ThreadProc, this );

    m_pThread = new std::thread( oThreadProc );
    if( m_pThread == nullptr )
    {
        ret = -EFAULT;
    }
    return ret;
}

gint32 CMainIoLoop::InstallTaskSource()
{
    gint32 ret = 0;

    if( m_pMainLoop == nullptr ||
        m_pMainCtx == nullptr )
        return ERROR_STATE;

    CStdMutex oLock( m_oLock );
    bool bDestroied = false;
    if( m_pTaskSrc != nullptr )
    {
        bDestroied =
            g_source_is_destroyed( m_pTaskSrc );
    }

    gint32 iCount = GetTaskQue()->GetSize();

    if( m_pTaskSrc == nullptr )
    {
        m_pTaskSrc = g_idle_source_new();
    }
    else if( ( bDestroied && iCount > 0 ) ||
        ( !bDestroied && iCount == 0 ) )
    {
        // FIXME: we have the chance to install one
        // new idle handler while the old one has not
        // quit yet
        g_source_unref( m_pTaskSrc );
        m_pTaskSrc = g_idle_source_new();
    }
    else
    {
        return 0;
    }

    if( m_pTaskSrc == nullptr )
        return -ENOMEM;

    g_source_set_callback(
        m_pTaskSrc, TaskCallback,
        ( gpointer )this, nullptr );

    g_source_attach(
        m_pTaskSrc, m_pMainCtx );

    return ret;
}
