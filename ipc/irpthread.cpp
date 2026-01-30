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
 *      Copyright:  2019 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */

#include <map>
#include <string>
#include <algorithm>
#include <vector>
#include <deque>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>

#include "defines.h"
#include "tasklets.h"
#include "proxy.h"
#include "stlcont.h"
#include "objfctry.h"
#include "frmwrk.h"

namespace rpcf
{

using namespace std;

void DumpTask( TaskletPtr& pTask )
{
#ifdef DEBUG
    gint32 iTaskRet = pTask->GetError();
    if( ERROR( iTaskRet ) )
    {
        EnumClsid iClsid = pTask->GetClsid();
        const char* pName =
            CoGetClassName( iClsid );

        string strTaskName;
        if( pName == nullptr )
        {
            strTaskName =
                std::to_string( iClsid );
        }
        else
        {
            strTaskName = pName;
        }

        DebugPrintEx(
            logWarning, iTaskRet,
            strTaskName + " failed" ); 
    }
#endif
}

gint32 IThread::SetThreadName(
    const char* szName )
{
    gint32 ret = 0;
    string strName;

    if( szName != nullptr &&
        szName[ 0 ] != 0 )
    {
        strName = szName;
    }
    else
    {
        strName = CoGetClassName(
            GetClsid() );

        gint32 iTid = 0;
        Variant oVar;
        ret = GetProperty( propThreadId, oVar );
        if( ERROR( ret ) )
            iTid = this->GetTid();
        else
            iTid = oVar; 
        strName += "-";
        strName += std::to_string( iTid );
    }

    ret = rpcf::SetThreadName( strName );

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
    if( pTask.IsEmpty() )
        return;

    guint8 byPriority = pTask->GetPriority();
    CStdRMutex a( m_oLock );
    if( likely( byPriority == 0 ) ||
        m_queTasks.size() <= 1 )
    {
        m_queTasks.push_back( pTask ); 
        return;
    }

    auto itr = m_queTasks.begin();
    while( itr != m_queTasks.end() )
    {
        if( (*itr)->GetPriority() >= byPriority )
        {
            itr++;
            continue;
        }
        m_queTasks.insert( itr, pTask );
        break;
    }

    if( itr == m_queTasks.end() )
        m_queTasks.push_back( pTask );

    return;
}

gint32 CTaskQueue::RemoveTask( TaskletPtr& pTask )
{
    if( pTask.IsEmpty() )
        return -EINVAL;

    gint32 ret = -ENOENT;
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

    return ret;
}

gint32 CTaskQueue::PopHead()
{
    gint32 ret = -ENOENT;
    CStdRMutex a( m_oLock );
    if( !m_queTasks.empty() )
    {
        m_queTasks.pop_front();
        ret = 0;
    }
    return ret;
}
gint32 CTaskQueue::GetHead( TaskletPtr& pTask )
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
    Sem_Init( &m_semSync, 0, 0 );
    CCfgOpener oCfg( pCfg );
    gint32 ret = oCfg.GetStrProp(
        0, m_strName );
    if( ERROR( ret ) )
    {
        m_strName = CoGetClassName(
            GetClsid() );
    }

}

CTaskThread::~CTaskThread()
{
    sem_destroy( &m_semSync );
}

// test if the thread is running
bool CTaskThread::IsRunning() const
{
    return ( m_pServiceThread != nullptr &&
        m_pServiceThread->joinable() &&
        m_bRunning );
}

gint32 CTaskThread::GetLoadCount() const
{
    return m_dwTaskCount;
}

gint32 CTaskThread::Start()
{
    // NOTE: don't change the value of the
    // last parameter. It is a interface
    // definition
    m_pServiceThread = new std::thread(
            &CTaskThread::ThreadProcWraper,
            this,
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
        Join();
        delete m_pServiceThread;

        m_pServiceThread = nullptr;
    }
    return 0;
}

void CTaskThread::Join()
{
    if( m_pServiceThread != nullptr &&
        m_pServiceThread->joinable() )
        m_pServiceThread->join();
}

gint32 CTaskThread::ProcessTask(
    guint32 dwContext )
{
    gint32 ret = 0;
    TaskletPtr pTask;

    ret = GetHead( pTask );
    if( ERROR( ret ) )
        return ret;

    if( pTask.IsEmpty() )
        return -EFAULT;

    ( *pTask )( dwContext );
    m_dwTaskCount--;

    DumpTask( pTask );

    // to exhaust the queue before
    // sleep
    return 0;
}

void CTaskThread::ThreadProc(
    void* pContext )
{
    gint32 ret = 0;
    LONGWORD dwContext = ( LONGWORD )pContext;

    this->SetThreadName( m_strName.c_str() );
    this->SetTid( rpcf::GetTid() );
    while( !m_bExit )
    {
        ProcessTask( ( guint32 )dwContext );
        if( m_bExit )
            break;

        ret = Sem_TimedwaitSec( &m_semSync,
            THREAD_WAKEUP_INTERVAL );

        if( ERROR( ret ) && ret != -EAGAIN )
            break;
    }

    while( m_bExit )
    {
        // process all the tasks in the queue before we
        // quit
        ret = ProcessTask( ( guint32 )dwContext );
        if( ERROR( ret ) )
            break;
    }

    m_bRunning = false;
    return;
}

void CTaskThread::AddTask(
    TaskletPtr& pTask )
{
    if( m_bExit )
        return;

    m_pTaskQue->AddTask( pTask );
    Sem_Post( &m_semSync );
    m_dwTaskCount++;
}

gint32 CTaskThread::RemoveTask(
    TaskletPtr& pTask )
{
    m_dwTaskCount--;
    return m_pTaskQue->RemoveTask( pTask );
}

gint32 CTaskThread::GetHead(
    TaskletPtr& pTask )
{
    return  m_pTaskQue->GetHead( pTask );
}

gint32 CTaskThread::GetProperty(
    gint32 iProp, Variant& oVar ) const
{
    gint32 ret = 0;
    switch( iProp )
    {
    case propThreadId:
        oVar = GetMyTid();
        break;

    default:
        ret = super::GetProperty(
            iProp, oVar );
        break;
    }
    return ret;
}

gint32 CTaskThread::SetProperty(
    gint32 iProp, const Variant& oVar )
{
    gint32 ret = 0;
    switch( iProp )
    {
    case propThreadId:
        SetMyTid( oVar );
        break;

    default:
        ret = super::SetProperty(
            iProp, oVar );
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
    m_iTaskClsid = clsid( Invalid );
}

COneshotTaskThread::~COneshotTaskThread()
{
    if( m_pServiceThread != nullptr )
    {
        delete m_pServiceThread;
        m_pServiceThread = nullptr;
    }
}

void COneshotTaskThread::ThreadProc(
    void* pContext )
{
    gint32 ret = 0;
    LONGWORD lContext = ( LONGWORD )pContext;

    this->SetThreadName( m_strName.c_str() );
    this->SetTid( rpcf::GetTid() );

    bool bTaskDone = false;
    while( !m_bExit )
    {
        ret = ProcessTask( lContext );
        if( SUCCEEDED( ret ) )
        {
            bTaskDone = true;
            break;
        }

        ret = Sem_TimedwaitSec( &m_semSync,
            THREAD_WAKEUP_INTERVAL );

        if( ERROR( ret ) )
        {
            if( ret != -EAGAIN )
                return;
        }
        // either timeout or signaled, let's
        // check the task again
    }

    // in case the task is in the queue
    if( m_bExit && !bTaskDone )
        ProcessTask( lContext );
 
    m_bRunning = false;
    return;
}

gint32 COneshotTaskThread::Start()
{
    // NOTE: don't change the value of the
    // last parameter. It is a interface
    // definition
    m_pServiceThread = new std::thread(
            &COneshotTaskThread::ThreadProc,
            this,
            ( void* )( eventOneShotTaskThrdCtx ) );
    return 0;
}

gint32 COneshotTaskThread::GetProperty(
    gint32 iProp, Variant& oBuf ) const
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
    gint32 iProp, const Variant& oBuf )
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

        CStdMutex a( m_oMutex );
        // hold an irp reference count
        m_quePendingIrps.push_back( IrpPtr( pirp ) );
        Sem_Post( &m_semIrps );
        m_dwIrpCount++;

    }while( 0 );
    return 0;
}

gint32 CIrpCompThread::Start()
{
    // NOTE: don't change the value of the
    // last parameter. It is a interface
    // definition
    m_pServiceThread = new std::thread(
        &CIrpCompThread::ThreadProc, this,
        ( void* )( eventIrpCompThrdCtx ) );
    return 0;
}

bool CIrpCompThread::IsRunning() const
{
    return ( m_pServiceThread != nullptr &&
        m_pServiceThread->joinable() &&
        m_bRunning );
}

gint32 CIrpCompThread::ProcessIrp()
{
    do{
        std::deque<IrpPtr> quePendingIrps;

        IrpPtr pIrp;
        CStdMutex a( m_oMutex );
        if( m_quePendingIrps.empty() )
            break;
        pIrp = m_quePendingIrps.front();
        m_quePendingIrps.pop_front();
        a.Unlock();
        this->CompleteIrp( pIrp );
        m_dwIrpCount--;

    }while( 0 );
    return 0;
}

gint32 CIrpCompThread::ProcessIrps()
{
    gint32 ret = 0;
    do{
        std::deque<IrpPtr> quePendingIrps;

        CStdMutex a( m_oMutex );
        guint32 dwCount = m_quePendingIrps.size();
        if( dwCount > 0 )
        {
            quePendingIrps = m_quePendingIrps;
            m_quePendingIrps.clear();
        }
        a.Unlock();

        for( auto elem : quePendingIrps )
            this->CompleteIrp( elem );
        if( dwCount > 0 )
            m_dwIrpCount -= dwCount;

    }while( 0 );

    return ret;
}

void CIrpCompThread::ThreadProc( void* context )
{
    IrpPtr pIrp;
    gint32 ret = 0;

    this->SetThreadName();
    this->SetTid( rpcf::GetTid() );

    while( !m_bExit )
    {
        ret = Sem_Wait( &m_semIrps );
        if( ERROR( ret ) )
            break;
        ProcessIrp();
    }

    ProcessIrps();
    m_bRunning = false;
    return;
}

void CIrpCompThread::CompleteIrp( PIRP pIrp )
{
    
    if( !pIrp->m_pCallback.IsEmpty() )
    {
        //FIXME: can I continue
        LONGWORD dwParam1 = ( LONGWORD )pIrp;
        TaskletPtr pCb = pIrp->m_pCallback;
        pCb->OnEvent(
            eventIrpComp, dwParam1, 0, 0 );
    }
}

CIrpCompThread::~CIrpCompThread()
{
    sem_destroy( &m_semSlots );
    sem_destroy( &m_semIrps );
}


gint32 CIrpCompThread::GetLoadCount() const
{ return m_dwIrpCount; }

gint32 CIrpCompThread::Stop()
{
    if( m_pServiceThread != nullptr )
    {
        m_bExit = true;

        // wake up the thread
        Sem_Post( &m_semIrps );

        // waiting till it ends
        Join();

        delete m_pServiceThread;

        m_pServiceThread = nullptr;
    }
    return 0;
}

void CIrpCompThread::Join()
{
    if( m_pServiceThread != nullptr &&
        m_pServiceThread->joinable() )
        m_pServiceThread->join();
}

gint32 CIrpCompThread::GetProperty(
    gint32 iProp, Variant& oVar ) const
{
    gint32 ret = 0;
    switch( iProp )
    {
    case propThreadId:
        oVar = GetMyTid();
        break;

    default:
        ret = super::GetProperty(
            iProp, oVar );
        break;
    }
    return ret;
}

gint32 CIrpCompThread::SetProperty(
    gint32 iProp, const Variant& oVar )
{
    gint32 ret = 0;
    switch( iProp )
    {
    case propThreadId:
        SetMyTid( oVar );
        break;

    default:
        ret = super::SetProperty(
            iProp, oVar );
        break;
    }
    return ret;
}

gint32 CIrpCompThread::EnumProperties(
    std::vector< gint32 >& vecProps ) const
{
    super::EnumProperties( vecProps );
    vecProps.push_back( propThreadId );
    return 0;
}

gint32 CThreadPool::GetThread(
    ThreadPtr& pThread, bool bStart )
{
    gint32 ret = 0;
    CStdRMutex oLock( m_oLock ); 

    do{
        ThreadPtr thptr;

        if( m_iMaxThreads == 0 )
            return -ENOENT;

        gint32 iLeastLoad = 10000;
        ThreadPtr thLeast;

        if( m_bAscend )
        {
            auto itr = m_vecThreads.begin();
            while( itr != m_vecThreads.end() ) 
            {
                gint32 iLoad =
                    ( *itr )->GetLoadCount();

                if( iLoad == 0 )
                {
                    iLeastLoad = 0;
                    thLeast = *itr;
                    break;
                }

                if( iLoad < iLeastLoad )
                {
                    iLeastLoad = iLoad;
                    thLeast = *itr;
                }
                ++itr;
            }
        }
        else
        {
            auto itr = m_vecThreads.rbegin();
            while( itr != m_vecThreads.rend() ) 
            {
                gint32 iLoad =
                    ( *itr )->GetLoadCount();

                if( iLoad == 0 )
                {
                    iLeastLoad = 0;
                    thLeast = *itr;
                    break;
                }

                if( iLoad < iLeastLoad )
                {
                    iLeastLoad = iLoad;
                    thLeast = *itr;
                }
                ++itr;
            }
        }

        if( iLeastLoad == 0 )
        {
            pThread = thLeast;
            break;
        }

        if( unlikely( m_vecThreads.size() <
            ( guint32 )m_iMaxThreads ) )
        {
            CCfgOpener oCfg;
            if( m_strPrefix.size() )
            {
                stdstr strName = m_strPrefix +
                    std::to_string( m_iThreadCount );
                oCfg.SetStrProp( 0, strName );
            }

            ret = thptr.NewObj( m_iThreadClass,
                ( IConfigDb* )oCfg.GetCfg() );

            if( ERROR( ret ) )
                break;

            CCfgOpenerObj oThrdCfg;
            oThrdCfg.SetIntProp( propThreadId,
                m_iThreadCount++ );

            if( bStart )
                thptr->Start();
            m_vecThreads.push_back( thptr );
        }
        else
        {
            thptr = thLeast;
        }
        pThread = thptr;

    }while( 0 );
    m_bAscend = !m_bAscend;

    return ret;
}

gint32 CThreadPool::GetThreadByTid(
    ThreadPtr& pThread,
    guint32 dwTid )
{
    gint32 ret = 0;
    if( dwTid == 0 )
        dwTid = rpcf::GetTid();
    do{
        bool bFound = false;
        ThreadPtr thptr = pThread;
        // find the specified thread
        CStdRMutex oLock( m_oLock ); 
        auto itr = m_vecThreads.begin();
        while( itr != m_vecThreads.end() )
        {
            if( ( *itr )->GetTid() != dwTid )
            {
                ++itr;
                continue;
            }
            pThread = *itr;
            bFound = true;
            break;
        }
    }while( 0 );
    return ret;
}

gint32 CThreadPool::GetLoadCount()
{
    gint32 ret = 0;
    do{
        // find the specified thread
        CStdRMutex oLock( m_oLock ); 
        auto itr = m_vecThreads.begin();
        while( itr != m_vecThreads.end() )
        {
            ret += ( *itr )->GetLoadCount();
            break;
        }
    }while( 0 );
    return ret;
}

gint32 CThreadPool::PutThread( IThread* pThread )
{
    gint32 ret = 0;
    if( pThread != nullptr )
    {
        
        ThreadPtr thptr = pThread;
        CStdRMutex oLock( m_oLock ); 
        if( m_vecThreads.size() > ( guint32 )m_iMaxThreads )
        {
            // if the threads limit is exceeded,
            // don't keep it in the pool

            vector< ThreadPtr >::iterator itr =
                    m_vecThreads.begin();

            while( itr != m_vecThreads.end() )
            {
                if( ( *itr )->GetObjId() !=
                    pThread->GetObjId() )
                {
                    ++itr;
                    continue;
                }
                m_vecThreads.erase( itr );
                break;
            }
            oLock.Unlock();
            thptr->Stop();
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

    a.GetStrProp( 3, m_strPrefix );

    ret = a.GetPointer( propIoMgr, m_pMgr );
    if( ERROR( ret ) )
        throw std::invalid_argument(
            "iomgr is not given" );

    m_iThreadClass = ( EnumClsid )dwValue;
    m_iThreadCount = 0;
    m_bAscend = true;
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

        vector< ThreadPtr > vecThreads =
            m_vecThreads;

        m_vecThreads.clear();
        m_iMaxThreads = 0;
        oLock.Unlock();

        for( auto pThread : vecThreads ) 
            pThread->Stop();

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

CThreadPools::CThreadPools( const IConfigDb* pCfg )
{
    gint32 ret = 0;
    do{
        SetClassId( clsid( CThreadPools ) );
        if( pCfg == nullptr )
        {
            ret = -EINVAL;
            break;
        }
        CCfgOpener oParams( pCfg );
        ret = oParams.GetPointer(
            propIoMgr, m_pMgr );

    }while( 0 );

    if( ERROR( ret ) )
    {
        throw std::runtime_error(
            DebugMsg( ret, "Error occurs "
                "in CLoopPool ctor" ) );
    }
}

gint32 CThreadPools::CreatePool(
    guint32 dwTag,
    guint32 dwLoadLimit,
    guint32 dwMaxThreads,
    const stdstr& strPrefix )
{
    CStdRMutex oLock( GetLock() );

    if( IsPool( dwTag ) )
        return 0;

    CIoManager* pMgr = GetIoMgr();
    if( dwMaxThreads == 0 )
        dwMaxThreads = pMgr->GetNumCores(); 

    ThrdPoolPtr pPool;
    CParamList oParams;
    oParams.Push( dwLoadLimit );
    oParams.Push( dwMaxThreads );
    oParams.Push( clsid( CTaskThread ) );
    oParams.Push( strPrefix );
    oParams.SetPointer( propIoMgr, pMgr );

    gint32 ret = pPool.NewObj(
        clsid( CTaskThreadPool ),
        oParams.GetCfg() );
    if( ERROR( ret ) )
        return ret;

    m_mapPools[ dwTag ] = pPool;
    return ret;
}

gint32 CThreadPools::GetLoadCount()
{
    gint32 ret = 0;
    CStdRMutex oLock( GetLock() );
    for( auto elem : m_mapPools )
        ret += elem.second->GetLoadCount();
    return ret;
}
gint32 CThreadPools::DestroyPool( guint32 dwTag )
{
    CStdRMutex oLock( GetLock() );

    if( !IsPool( dwTag ) )
        return -ENOENT;

    ThrdPoolPtr pPool = m_mapPools[ dwTag ];
    m_mapPools.erase( dwTag );
    oLock.Unlock();
    pPool->Stop();
    return STATUS_SUCCESS;
}

gint32 CThreadPools::GetThread(
    guint32 dwTag, 
    ThreadPtr& pThread )
{
    CStdRMutex oLock( GetLock() );
    if( !IsPool( dwTag ) )
        return -ENOENT;
    ThrdPoolPtr pPool = m_mapPools[ dwTag ];
    return pPool->GetThread( pThread );
}

gint32 CThreadPools::PutThread(
    guint32 dwTag,
    ThreadPtr& pThread )
{
    CStdRMutex oLock( GetLock() );
    if( !IsPool( dwTag ) )
        return -ENOENT;
    ThrdPoolPtr& pPool = m_mapPools[ dwTag ];
    return pPool->PutThread( pThread );
}


gint32 CThreadPools::Stop()
{
    std::vector< guint32 > vecPools;
    CStdRMutex oLock( GetLock() );
    for( auto elem : m_mapPools )
        vecPools.push_back( elem.first );

    m_mapPools.clear();
    oLock.Unlock();
    for( auto& elem : vecPools )
        DestroyPool( elem );

    return 0;
}

}

namespace std
{
    using namespace rpcf;
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
