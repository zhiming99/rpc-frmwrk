/*
 * =====================================================================================
 *
 *       Filename:  irpthread.cpp
 *
 *    Description:  implementation of CIrpCompThread
 *
 *        Version:  1.0
 *        Created:  06/02/2016 09:24:59 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 * =====================================================================================
 */

#include <map>
#include <string>
#include <algorithm>
#include <vector>
#include <deque>

#include "defines.h"
#include "tasklets.h"
#include "proxy.h"
#include "stlcont.h"

using namespace std;

CTaskQueue::CTaskQueue()
    : CObjBase()
{
    SetClassId( clsid( CTaskQueue ) );
}

void CTaskQueue::AddTask( TaskletPtr& pTask )
{
    if( pTask.IsEmpty() )
    {
        bool bAdd = true;
        CStdRMutex a( m_oLock );
        for( auto& pElem : m_queTasks )
        {
            if( pElem == pTask )
            {
                bAdd = false;
                break;
            }
        }
        if( bAdd )
            m_queTasks.push_back( pTask ); 
    }
}

gint32 CTaskQueue::RemoveTask( TaskletPtr& pTask )
{
    gint32 ret = -ENOENT;

    if( pTask.IsEmpty() )
    {
        CStdRMutex a( m_oLock );
        if( m_queTasks.empty() )
            return ret;

        deque< TaskletPtr >::iterator itr;
        itr = std::find( m_queTasks.begin(),
            m_queTasks.end(), pTask );

        if( itr == m_queTasks.end() )
            return ret;

        m_queTasks.erase( itr );
        ret = 0;
        
    }

    return ret;
}

gint32 CTaskQueue::PopHead( TaskletPtr& pTask )
{
    gint32 ret = -ENOENT;
    CStdRMutex a( m_oLock );
    if( !m_queTasks.empty() )
    {
        pTask = m_queTasks.front();
        m_queTasks.pop_front();
        ret = 0;
    }
    return ret;
}

gint32 CTaskQueue::GetSize()
{
    CStdRMutex a( m_oLock );
    return m_queTasks.size();
}

CTaskThread::CTaskThread() :
    m_pTaskQue( true )
{
    SetClassId( clsid( CTaskThread ) );
    m_pServiceThread = nullptr;
    m_bExit = false;
    sem_init( &m_semSync, 0, 0 );
}

CTaskThread::~CTaskThread()
{
    sem_destroy( &m_semSync );
}

// test if the thread is running
bool CTaskThread::IsRunning()
{
    return ( m_pServiceThread != nullptr
        && m_pServiceThread->joinable() );
}

gint32 CTaskThread::GetLoadCount() const
{
    return m_pTaskQue->GetSize();
}

gint32 CTaskThread::Start()
{
    m_bExit = false;
    // NOTE: don't change the value of the
    // last parameter. It is a interface
    // definition
    m_pServiceThread = new std::thread(
            &CTaskThread::ThreadProcWraper,
            std::ref(*this),
            ( void* )( eventTaskThrdCtx ) );
    return 0;
}

gint32 CTaskThread::Stop()
{
    if( m_pServiceThread != nullptr )
    {
        m_bExit = true;

        sem_post( &m_semSync );

        // waiting till it ends
        m_pServiceThread->join();

        delete m_pServiceThread;

        m_pServiceThread = nullptr;
    }
    return 0;
}

void CTaskThread::ThreadProc(
    void* dwContext )
{
    gint32 ret = 0;

#ifdef DEBUG
    gint32 iTaskRet = 0;
#endif

    while( !m_bExit )
    {
        do{
            TaskletPtr pTask;
            ret = PopHead( pTask );
            if( SUCCEEDED( ret ) && !pTask.IsEmpty() )
            {
#ifdef DEBUG
                iTaskRet =
                    ( *pTask )( ( guint32 )dwContext );
                if( ERROR( iTaskRet ) )
                {
                    string strTaskName =
                        CoGetClassName( pTask->GetClsid() );

                    string strMsg = DebugMsg(
                        iTaskRet, strTaskName + " failed" ); 

                    printf( strMsg.c_str() );
                }
#else
                ( *pTask )( ( guint32 )dwContext );
#endif
            }

            // to exhaust the queue before
            // sleep
            if( SUCCEEDED( ret ) )
                continue;

            break;
        }while( 1 );

        sem_wait( &m_semSync );
    }

    return;
}

void CTaskThread::AddTask(
    TaskletPtr& pTask )
{
    m_pTaskQue->AddTask( pTask );
    sem_post( &m_semSync );
}

gint32 CTaskThread::RemoveTask(
    TaskletPtr& pTask )
{
    return m_pTaskQue->RemoveTask( pTask );
}

gint32 CTaskThread::PopHead(
    TaskletPtr& pTask )
{
    return  m_pTaskQue->PopHead( pTask );
}

COneshotTaskThread::COneshotTaskThread()
    : CTaskThread()
{
    SetClassId( clsid( COneshotTaskThread ) );
    m_bTaskDone = false;
}

void COneshotTaskThread::ThreadProc(
    void* dwContext )
{
    gint32 ret = 0;

    while( !m_bTaskDone )
    {
        TaskletPtr pTask;
        ret = PopHead( pTask );
        if( SUCCEEDED( ret ) && !pTask.IsEmpty() )
        {
            ( *pTask )( ( guint32 )dwContext );
            m_bTaskDone = true;
        }

        if( !m_bTaskDone )
            sem_wait( &m_semSync );
    }

    return;
}

gint32 COneshotTaskThread::Start()
{
    m_bExit = false;
    // NOTE: don't change the value of the
    // last parameter. It is a interface
    // definition
    m_pServiceThread = new std::thread(
            &COneshotTaskThread::ThreadProc,
            std::ref(*this),
            ( void* )( eventOneShotTaskThrdCtx ) );
    return 0;
}

CIrpCompThread::CIrpCompThread()
{
    SetClassId( Clsid_CIrpCompThread );

    sem_init( &m_semIrps, 0, 0 );
    sem_init( &m_semSlots, 0, ICT_MAX_IRPS );
}

gint32 CIrpCompThread::AddIrp( IRP* pirp )
{
    if( pirp == NULL )
        return -EINVAL;

    if( m_bExit )
        return ERROR_STATE;

    do{

        sem_wait( &m_semSlots );
        CStdMutex a( m_oMutex );
        // we will held an irp reference count here
        m_quePendingIrps.push_back( IrpPtr( pirp ) );
        sem_post( &m_semIrps );

    }while( 0 );
    return 0;
}

gint32 CIrpCompThread::Start()
{
    m_bExit = false;

    // NOTE: don't change the value of the
    // last parameter. It is a interface
    // definition
    m_pServiceThread = new std::thread(
            &CIrpCompThread::ThreadProc,
            std::ref(*this),
            ( void* )( eventIrpCompThrdCtx ) );
    return 0;
}

void CIrpCompThread::ThreadProc( void* context )
{
    do
    {
        IrpPtr pIrp;
        bool bEmpty = true;
        do{
            sem_wait( &m_semIrps );
            CStdMutex a( m_oMutex );
            if( m_quePendingIrps.size() )
            {
                pIrp = m_quePendingIrps.front();
                m_quePendingIrps.pop_front();
            }
            bEmpty = m_quePendingIrps.empty();
            sem_post( &m_semSlots );

        }while( 0 );

        if( !pIrp.IsEmpty() )
        {
            CompleteIrp( pIrp );
        }
        // pIrp destroyed
        if( !bEmpty )
            continue;

    }while( !m_bExit );
    
}

void CIrpCompThread::CompleteIrp( PIRP pIrp )
{
    
    if( !pIrp->m_pCallback.IsEmpty() )
    {
        //FIXME: can I continue
        guint32 dwParam1 = ( guint32 )pIrp;
        pIrp->m_pCallback->OnEvent(
            eventIrpComp, dwParam1, 0, 0 );
    }
}

CIrpCompThread::~CIrpCompThread()
{
    sem_destroy( &m_semSlots );
    sem_destroy( &m_semIrps );
}


gint32 CIrpCompThread::AddIf( IGenericInterface* pif )
{
    gint32 ret = 0;
    CStdMutex a( m_oMutex );
    if( m_mapIfs.size() < ICT_MAX_IFS )
    {
        m_mapIfs[ pif ] = 1;
    }
    else
    {
        ret = -ENOMEM;
    }

    return ret;
}

gint32 CIrpCompThread::RemoveIf( IGenericInterface* pif )
{

    gint32 ret = 0;

    CStdMutex a( m_oMutex );

    if( m_mapIfs.erase( pif ) == 0 )
        ret = -ENOENT;

    return ret;
}

gint32 CIrpCompThread::GetLoadCount() const
{
    CStdMutex a( m_oMutex );
    return m_mapIfs.size();

}

gint32 CIrpCompThread::Stop()
{
    if( m_pServiceThread != nullptr )
    {
        m_bExit = true;

        // wake up the thread
        sem_post( &m_semIrps );

        // waiting till it ends
        m_pServiceThread->join();

        delete m_pServiceThread;

        m_pServiceThread = nullptr;
    }
    return 0;
}

gint32 CThreadPool::GetThread(
    ThreadPtr& pThread, bool bStart )
{
    gint32 ret = 0;

    do{

        ThreadPtr thptr;
        CStdRMutex oLock( m_oLock ); 
        vector<ThreadPtr>::iterator itr =
            m_vecThreads.begin();

        while( itr != m_vecThreads.end() ) 
        {
            // a simple load balance
            if( ( *itr )->GetLoadCount() < m_iLoadLimit )
            {
                thptr = *itr;
                break;
            }
            ++itr;
        }

        bool bEmpty = thptr.IsEmpty();
        if( bEmpty || !thptr->IsRunning() )
        {
            ret = thptr.NewObj( m_iThreadClass );
            if( ERROR( ret ) )
            {
                break;
            }

            if( bStart )
                thptr->Start();

            if( bEmpty )
            {
                m_vecThreads.push_back( thptr );
            }
            pThread = thptr;
        }
    }while( 0 );

    return ret;
}

gint32 CThreadPool::PutThread( IThread* pThread )
{
    gint32 ret = 0;
    if( pThread != nullptr )
    {
        CStdRMutex oLock( m_oLock ); 

        gint32 iRefCount = pThread->AddRef();
        pThread->Release();

        if( iRefCount == 2
            && m_vecThreads.size() > ( guint32 )m_iMaxThreads )
        {
            // if the threads limit is exceeded,
            // don't keep it in the pool

            vector< ThreadPtr >::iterator itr =
                    m_vecThreads.begin();

            while( itr != m_vecThreads.end() )
            {
                if( ( *itr )->GetObjId() != pThread->GetObjId() )
                {
                    ++itr;
                    continue;
                }
                m_vecThreads.erase( itr );
                break;
            }
        }
    }
    else
    {
        ret = -EINVAL;
    }
    return ret;
}

CThreadPool::CThreadPool( const IConfigDb* pCfg )
{
    if( pCfg == nullptr )
        return;

    gint32 ret = 0;
    CCfgOpener a( pCfg );
    ret = a.GetIntProp( 0, ( guint32& )m_iLoadLimit );
    if( ERROR( ret ) )
        throw std::invalid_argument(
            "Load limit is not given" );

    ret = a.GetIntProp( 1, ( guint32& )m_iMaxThreads );
    if( ERROR( ret ) )
        throw std::invalid_argument(
            "Max number of threads is not given" );

    guint32 dwValue = 0;

    ret = a.GetIntProp( 2, dwValue );
    if( ERROR( ret ) )
        throw std::invalid_argument(
            "thread class is not given" );

    m_iThreadClass = ( EnumClsid )dwValue;
}

gint32 CThreadPool::Start()
{
    return 0;
}

gint32 CThreadPool::Stop()
{

    gint32 ret = 0;
    do{
        CStdRMutex oLock( m_oLock ); 
        ThreadPtr thptr;

        vector<ThreadPtr>::iterator itr =
            m_vecThreads.begin();

        while( itr != m_vecThreads.end() ) 
        {
            ( *itr )->Stop();
            ++itr;
        }
        m_vecThreads.clear();

    }while( 0 );

    return ret;
}

gint32 CTaskThreadPool::RemoveTask(
    TaskletPtr& pTask )
{
    gint32 ret = 0;

    do{

        ThreadPtr thptr;
        CStdRMutex oLock( GetLock() ); 
        vector<ThreadPtr>::iterator itr =
            m_vecThreads.begin();

        while( itr != m_vecThreads.end() )
        {
            ThreadPtr& pThrd = *itr;
            CTaskThread* pTaskThrd = pThrd;
            if( pTaskThrd == nullptr )
            {
                ++itr;
                continue;
            }
            ret = pTaskThrd->RemoveTask( pTask );
            if( SUCCEEDED( ret ) )
                break;

            ++itr;
        }
            
    }while( 0 );

    return ret;
}
