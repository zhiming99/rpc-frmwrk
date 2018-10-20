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
#include <sys/prctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>

#include "defines.h"
#include "tasklets.h"
#include "proxy.h"
#include "stlcont.h"
#include "objfctry.h"
#include "frmwrk.h"

using namespace std;

// set the name of the current thread
string GetThreadName()
{
    gint32 ret = 0;
    char szBuf[ 32 ] = { 0 };

    ret = prctl( PR_GET_NAME, szBuf, 0, 0, 0 );

    if( ret == -1 )
        return string( "Unknown" );

    return string( szBuf );
}

// set the name of the current thread
gint32 SetThreadName(
    const string& strName )
{
    gint32 ret = 0;
    if( strName.size() == 0 )
        return -EINVAL;

    ret = prctl( PR_SET_NAME,
        strName.c_str(), 0, 0, 0 );

    if( ret == -1 )
        ret = -errno;

    return ret;
}

gint32 IThread::SetThreadName(
    const char* szName )
{
    gint32 ret = 0;
    string strName;

    if( szName != nullptr )
    {
        strName = szName;
    }
    else
    {
        strName = CoGetClassName(
            GetClsid() );

        gint32 iTid = GetTid();
        strName += "-";
        strName += std::to_string( iTid );
    }

    ret = ::SetThreadName( strName );

    return ret;
}

CTaskQueue::CTaskQueue()
    : CObjBase()
{
    SetClassId( clsid( CTaskQueue ) );
}

void CTaskQueue::AddTask( TaskletPtr& pTask )
{
    bool bAdd = true;

    CStdRMutex a( m_oLock );
    if( !pTask.IsEmpty() )
    {
        for( auto& pElem : m_queTasks )
        {
            if( pElem == pTask )
            {
                bAdd = false;
                break;
            }
        }
    }
    if( bAdd )
        m_queTasks.push_back( pTask ); 
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

CTaskThread::CTaskThread(
    const IConfigDb* pCfg ) :
    m_pTaskQue( true )
{
    SetClassId( clsid( CTaskThread ) );
    m_pServiceThread = nullptr;
    m_bExit = false;
    Sem_Init( &m_semSync, 0, 0 );
}

CTaskThread::~CTaskThread()
{
    sem_destroy( &m_semSync );
}

// test if the thread is running
bool CTaskThread::IsRunning() const
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

        Sem_Post( &m_semSync );

        // waiting till it ends
        m_pServiceThread->join();

        delete m_pServiceThread;

        m_pServiceThread = nullptr;
    }
    return 0;
}

gint32 CTaskThread::SetThreadName(
    const char* szName )
{
    string strName;

    if( szName != nullptr )
    {
        strName = szName;
    }
    else
    {
        strName = CoGetClassName(
            GetClsid() );

        gint32 iTid = this->GetTid(); 
        strName += "-";
        strName += std::to_string( iTid );
    }

    return super::SetThreadName(
        strName.c_str() );
}

gint32 CTaskThread::ProcessTask(
    guint32 dwContext )
{
    gint32 ret = 0;
    TaskletPtr pTask;
    ret = PopHead( pTask );

    if( ERROR( ret ) )
        return ret;

    if( pTask.IsEmpty() )
        return -EFAULT;

#ifdef DEBUG
    ( *pTask )( ( guint32 )dwContext );
    gint32 iTaskRet = pTask->GetError();

    if( ERROR( iTaskRet ) )
    {
        string strTaskName =
            CoGetClassName( pTask->GetClsid() );

        DebugPrint( iTaskRet,
            strTaskName + " failed" ); 
    }
#else
    ( *pTask )( ( guint32 )dwContext );
#endif

    // to exhaust the queue before
    // sleep
    return 0;
}

void CTaskThread::ThreadProc(
    void* dwContext )
{
    gint32 ret = 0;

    this->SetThreadName();
    while( !m_bExit )
    {
        do{
            ret = ProcessTask( ( guint32 )dwContext );

            // to exhaust the queue before
            // sleep
            if( SUCCEEDED( ret ) )
                continue;

            break;

        }while( 1 );

        while( !m_bExit )
        {
            ret = Sem_TimedwaitSec( &m_semSync,
                THREAD_WAKEUP_INTERVAL );

            if( ERROR( ret ) )
            {
                if( ret == -EAGAIN )
                    continue;
            }
            break;
        }
    }

    while( m_bExit )
    {
        // process all the tasks in the queue before we
        // quit
        ret = ProcessTask( ( guint32 )dwContext );
        if( ERROR( ret ) )
            break;
    }

    return;
}

void CTaskThread::AddTask(
    TaskletPtr& pTask )
{
    m_pTaskQue->AddTask( pTask );
    Sem_Post( &m_semSync );
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

gint32 CTaskThread::GetProperty(
    gint32 iProp, CBuffer& oBuf ) const
{
    gint32 ret = 0;
    switch( iProp )
    {
    case propThreadId:
        oBuf = GetTid();
        break;

    default:
        ret = super::GetProperty(
            iProp, oBuf );
        break;
    }
    return ret;
}

gint32 CTaskThread::SetProperty(
    gint32 iProp, const CBuffer& oBuf )
{
    gint32 ret = 0;
    switch( iProp )
    {
    case propThreadId:
        SetTid( oBuf );
        break;

    default:
        ret = super::SetProperty(
            iProp, oBuf );
        break;
    }
    return ret;
}

gint32 CTaskThread::EnumProperties(
    std::vector< gint32 >& vecProps ) const
{
    super::EnumProperties( vecProps );
    vecProps.push_back( propThreadId );
    return 0;
}

COneshotTaskThread::COneshotTaskThread()
    : CTaskThread()
{
    SetClassId( clsid( COneshotTaskThread ) );
    m_bTaskDone = false;
    m_iTaskClsid = clsid( Invalid );
}

void COneshotTaskThread::ThreadProc(
    void* dwContext )
{
    gint32 ret = 0;

    this->SetThreadName();

    while( !m_bTaskDone )
    {
        TaskletPtr pTask;
        ret = PopHead( pTask );
        if( SUCCEEDED( ret ) && !pTask.IsEmpty() )
        {
            ( *pTask )( ( guint32 )dwContext );
            m_bTaskDone = true;
        }

        while( !m_bTaskDone )
        {
            ret = Sem_TimedwaitSec( &m_semSync,
                THREAD_WAKEUP_INTERVAL );

            if( ret == -EAGAIN )
                continue;

            if( ERROR( ret ) )
                return;

            // there comes the task to handle
            break;
        }
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

gint32 COneshotTaskThread::Stop()
{
    m_bTaskDone = true;
    m_pServiceThread->join();
    delete m_pServiceThread;
    m_pServiceThread = nullptr;

    return 0;
}

gint32 COneshotTaskThread::GetProperty(
    gint32 iProp, CBuffer& oBuf ) const
{
    gint32 ret = 0;
    switch( iProp )
    {
    case propClsid:
        oBuf = ( guint32 )m_iTaskClsid;
        break;

    default:
        ret = super::GetProperty(
            iProp, oBuf );
        break;
    }
    return ret;
}

gint32 COneshotTaskThread::SetProperty(
    gint32 iProp, const CBuffer& oBuf )
{
    gint32 ret = 0;
    switch( iProp )
    {
    case propClsid:
        {
            guint32 dwClsid = oBuf;
            m_iTaskClsid = ( EnumClsid )dwClsid;
            break;
        }
    default:
        ret = super::SetProperty(
            iProp, oBuf );
        break;
    }
    return ret;
}

gint32 COneshotTaskThread::EnumProperties(
    std::vector< gint32 >& vecProps ) const
{
    super::EnumProperties( vecProps );
    vecProps.push_back( propClsid );
    return 0;
}

CIrpCompThread::CIrpCompThread()
{
    SetClassId( Clsid_CIrpCompThread );

    Sem_Init( &m_semIrps, 0, 0 );
    Sem_Init( &m_semSlots, 0, ICT_MAX_IRPS );
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
        Sem_Post( &m_semIrps );

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

bool CIrpCompThread::IsRunning() const
{
    return ( m_pServiceThread != nullptr
        && m_pServiceThread->joinable() );
}

void CIrpCompThread::ThreadProc( void* context )
{
    IrpPtr pIrp;
    bool bEmpty = true;
    gint32 ret = 0;

    this->SetThreadName();

    while( !m_bExit )
    {
        bEmpty = true;
        ret = Sem_TimedwaitSec( &m_semIrps,
            THREAD_WAKEUP_INTERVAL );

        if( ret == -EAGAIN )
            continue;

        if( ERROR( ret ) )
            break;

        if( true )
        {
            CStdMutex a( m_oMutex );
            if( m_quePendingIrps.size() )
                pIrp = m_quePendingIrps.front();
        }

        if( !pIrp.IsEmpty() )
        {
            CompleteIrp( pIrp );

            CStdMutex a( m_oMutex );
            m_quePendingIrps.pop_front();
            bEmpty = m_quePendingIrps.empty();
            Sem_Post( &m_semSlots );
        }

        pIrp.Clear();
        // pIrp destroyed
        if( !bEmpty )
            continue;
    }
    
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
    return m_quePendingIrps.size();

}

gint32 CIrpCompThread::Stop()
{
    if( m_pServiceThread != nullptr )
    {
        m_bExit = true;

        // wake up the thread
        Sem_Post( &m_semIrps );

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

        gint32 iLeastLoad = 10000;
        ThreadPtr thLeast;

        while( itr != m_vecThreads.end() ) 
        {
            // a simple thread choosing strategy
            gint32 iLoad =
                ( *itr )->GetLoadCount();

            if( iLoad < iLeastLoad )
            {
                iLeastLoad = iLoad;
                thLeast = *itr;
            }
            ++itr;
        }

        if( iLeastLoad <= 0 )
        {
            pThread = thLeast;
            break;
        }

        if( m_vecThreads.size() <
            ( guint32 )m_iMaxThreads )
        {
            ret = thptr.NewObj( m_iThreadClass );
            if( ERROR( ret ) )
                break;

            CCfgOpenerObj oCfg( ( CObjBase* )thptr );
            oCfg.SetIntProp( propThreadId,
                m_iThreadCount++ );

            if( bStart )
                thptr->Start();
            m_vecThreads.push_back( thptr );
        }
        else if( m_vecThreads.size() >=
            ( guint32 )m_iMaxThreads )
        {
            thptr = thLeast;
        }
        else
        {
           ret = ERROR_FAIL;
           break;
        }
        pThread = thptr;

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

    ret = a.GetPointer( propIoMgr, m_pMgr );
    if( ERROR( ret ) )
        throw std::invalid_argument(
            "iomgr is not given" );

    m_iThreadClass = ( EnumClsid )dwValue;
    m_iThreadCount = 0;
}

CThreadPool::~CThreadPool()
{

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

namespace std {

    template<>
    struct less<ThreadPtr>
    {
        bool operator()(const ThreadPtr& k1, const ThreadPtr& k2) const
        {
            if( k2.IsEmpty() )
                return false;

            if( k1.IsEmpty() )
                return true;

            return k1->GetLoadCount() < k2->GetLoadCount();
        }
    };
}
