/*
 * =====================================================================================
 *
 *       Filename:  iftasks.h
 *
 *    Description:  declaration of interface related tasks
 *
 *        Version:  1.0
 *        Created:  03/15/2017 09:49:27 PM
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

#include <sys/types.h>
#include <vector>
#include <string>
#include <deque>
#include "defines.h"
#include "autoptr.h"
#include "registry.h"
#include "tasklets.h"
#include "ifstat.h"

namespace rpcf
{

#define GET_IOMGR( _oCfg, _pMgr ) \
({ \
    gint32 ret_ = 0;\
    do{\
        CRpcInterfaceBase* pIf = nullptr;\
        ret_ = _oCfg.GetPointer( propIfPtr, pIf );\
        if( ERROR( ret_ ) )\
        {\
            ret_ = _oCfg.GetPointer(\
                propIoMgr, _pMgr );\
            if( ERROR( ret_ ) )\
                break;\
        }\
        else\
        {\
            _pMgr = pIf->GetIoMgr();\
        }\
    }while( 0 );\
    ret_;\
})

#define RFC_MAX_REQS 16U
#define RFC_MAX_PENDINGS 32U

class CIfRetryTask
    : public CThreadSafeTask
{
    // properties:
    //
    // propIoMgr: pointer to the CIoManager, which
    //      is required by the CTaskletRetriable
    //
    // propIfPtr: pointer to the interface,
    //      mandatory
    //
    // propEventPtr: pointer to the client
    //      callback, optional
    //
    // propContext: context for the propEventPtr,
    //      optional
    //
    // propNotifyClient: bool value to indicate
    //      whether to notify the client, if it is
    //      true, the propEventPtr will be
    //      retrieved and make the call, or if not
    //      existing, post a semaphore
    //
    // propParentTask: the pointer to the parent
    //      task, which is usually used to notify the
    //      parent task to move on to next task.
    //      it is optional.
    //
    protected:
    sem_t               m_semWait;
    TaskletPtr          m_pParentTask;
    TaskletPtr          m_pFwdrTask;
    bool                m_bSyncCancel = false;

    gint32 SetFwrdTask( IEventSink* pCallback );
    virtual gint32 CancelTaskChain(
        guint32 dwContest,
        gint32 iError = -ECANCELED );

    gint32 DoCancelTaskChain(
        guint32 dwContext, gint32 iError );

    public:
    typedef CThreadSafeTask super;

    CIfRetryTask( const IConfigDb* pCfg );
    ~CIfRetryTask()
    {  sem_destroy( &m_semWait ); }

    virtual gint32 Process( guint32 dwContext );
    virtual gint32 OnIrpComplete( PIRP pIrp );
    virtual gint32 RunTask() = 0;
    virtual gint32 OnComplete( gint32 iRetVal );
    virtual gint32 OnTaskComplete( gint32 iRetVal )
    { return -ENOTSUP; }

    virtual gint32 OnRetry();
    virtual gint32 WaitForComplete();
    gint32 OnCancel( guint32 dwContext );

    bool Retriable( gint32 iRet ) const
    {
        bool ret = false;
        do{
            if( iRet != STATUS_MORE_PROCESS_NEEDED &&
                iRet != -EAGAIN )
                break;

            if( !CanRetry() )
                break;

            ret = true;

        }while( 0 );

        return ret;
    }

    gint32 OnEvent(
        EnumEventId iEvent,
        LONGWORD dwParam1,
        LONGWORD dwParam2,
        LONGWORD* pData );

    gint32 GetProperty(
        gint32 iProp,
        Variant& oVar ) const override;

    gint32 SetProperty(
        gint32 iProp,
        const Variant& oVar ) override;

    gint32 RemoveProperty( gint32 iProp );

    gint32 EnumProperties(
        std::vector< gint32 >& vecProps ) const;

    gint32 GetPropertyType(
        gint32 iProp, gint32& iType ) const;

    void ClearClientNotify();
    gint32 SetClientNotify( IEventSink* pCallback );
    gint32 GetClientNotify( EventPtr& pEvt ) const;

    TaskletPtr GetEndFwrdTask( bool bClear = false );
    TaskletPtr GetFwrdTask( bool bClear = false );
    gint32 ClearFwrdTask();

    template< class ClassName >
    gint32 DelayRun(
        guint32 dwSecsDelay,
        EnumEventId iEvent = eventZero,
        LONGWORD dwParam1 = 0,
        LONGWORD dwParam2 = 0,
        LONGWORD* pdata = nullptr );

    inline void SetSyncCancel()
    { m_bSyncCancel = true; }

    inline bool IsSyncCancel() const
    { return m_bSyncCancel; }
};

typedef EnumIfState EnumTaskState;
class CIfParallelTask
    : public CIfRetryTask
{

    protected:
    std::atomic< EnumTaskState > m_iTaskState;
    TaskletPtr m_pCancelTask;

    gint32 CancelTaskChain(
        guint32 dwContest,
        gint32 iError = -ECANCELED ) override;

    gint32 DoCancelTaskChainSync(
        TaskletPtr pTask,
        guint32 dwContest,
        gint32 iError = -ECANCELED );

    public:
    typedef CIfRetryTask super;
    CIfParallelTask( const IConfigDb* pCfg );

    // NOTE:To get the state machine run
    // correctly, the first call to the operator()
    // should have the  dwContext to be eventZero,
    // eventTaskThrdCtx, eventOneShotTaskThrdCtx,
    // eventIrpComp, or eventTimeout.
    virtual gint32 operator()( guint32 dwContext );
    virtual gint32 OnComplete( gint32 iRet );
    gint32 CancelIrp();
    virtual gint32 OnCancel( guint32 dwContext );

    inline EnumTaskState GetTaskState() const
    { return m_iTaskState; }

    inline void SetTaskState( EnumTaskState iState )
    { m_iTaskState = iState; }

    // helper methods
    gint32 SetReqCall( IConfigDb* pReq );
    gint32 SetRespData( IConfigDb* pResp );

    gint32 GetReqCall( CfgPtr& pReq );
    gint32 GetRespData( CfgPtr& pResp );
    virtual gint32 Process( guint32 dwContext );
    virtual gint32 OnKeepAlive(
        guint32 dwContext )
    { return -ENOTSUP; }

    gint32 GetProperty(
        gint32 iProp,
        Variant& oVar ) const override;

    gint32 SetProperty(
        gint32 iProp,
        const Variant& oVar ) override;

    gint32 RemoveProperty( gint32 iProp );

    gint32 EnumProperties(
        std::vector< gint32 >& vecProps ) const;

    gint32 GetPropertyType(
        gint32 iProp, gint32& iType ) const;

    virtual bool IsMultiThreadSafe()
    { return true; }

    gint32 OnEvent(
        EnumEventId iEvent,
        LONGWORD dwParam1,
        LONGWORD dwParam2,
        LONGWORD* pData );

    bool IsStopped( EnumTaskState iState ) const;
};

class CIfEnableEventTask
    : public CIfParallelTask
{
    public:
    typedef CIfParallelTask super;
    CIfEnableEventTask( const IConfigDb* pCfg );
    virtual gint32 OnIrpComplete( PIRP pIrp );
    virtual gint32 RunTask();

};

class CIfOpenPortTask
    : public CIfParallelTask
{
    public:
    typedef CIfParallelTask super;
    CIfOpenPortTask( const IConfigDb* pCfg );
    virtual gint32 OnIrpComplete( PIRP pIrp );
    virtual gint32 RunTask();
    // canceling is not supported yet
    virtual gint32 OnCancel( guint32 dwContext );
};

class CIfStopTask
    : public CIfParallelTask
{
    public:
    typedef CIfParallelTask super;
    CIfStopTask( const IConfigDb* pCfg )
        : super( pCfg )
    { SetClassId( clsid( CIfStopTask ) ); }

    virtual gint32 RunTask();
    virtual gint32 OnIrpComplete( PIRP pIrp );
};

class CIfPauseResumeTask
    : public CIfParallelTask
{

    public:
    typedef CIfParallelTask super;
    CIfPauseResumeTask( const IConfigDb* pCfg )
        : super( pCfg )
    { SetClassId( clsid( CIfPauseResumeTask ) ); }

    virtual gint32 RunTask();
};

class CIfStopRecvMsgTask
    : public CIfParallelTask
{
    public:
    typedef CIfParallelTask super;
    CIfStopRecvMsgTask( const IConfigDb* pCfg )
        : super( pCfg )
    { SetClassId( clsid( CIfStopRecvMsgTask ) ); }

    virtual gint32 RunTask();
};

class CIfCpEventTask
    : public CIfParallelTask
{
    // properties:
    // propId 0: iEvent
    // propId 1: depends on the event method's
    //      declaration
    public:
    typedef CIfParallelTask super;
    CIfCpEventTask( const IConfigDb* pCfg );
    virtual gint32 OnIrpComplete( PIRP pIrp );
    virtual gint32 RunTask();
};

class CIfCleanupTask
    : public CIfParallelTask
{
    // properties:
    // propId 0: iEvent
    // propId 1: depends on the event method's
    //      declaration
    std::atomic< bool >  m_bCalled;
    public:
    typedef CIfParallelTask super;
    CIfCleanupTask( const IConfigDb* pCfg )
        : super( pCfg ), m_bCalled( false )
    {
        SetClassId( clsid( CIfCleanupTask ) );
    }
    virtual gint32 OnIrpComplete( PIRP pIrp );
    virtual gint32 OnTaskComplete( gint32 iRetVal );
    virtual gint32 RunTask();
};


typedef enum 
{
    logicAND,
    logicOR,
    logicNONE

} EnumLogicOp;

typedef enum 
{
    waitStart,
    waitComplete,
    waitRunning,

} EnumHeadState;

class CIfTaskGroup
    : public CIfRetryTask
{
    EnumLogicOp m_iTaskRel;

    std::atomic< EnumTaskState > m_iTaskState;

    bool m_bRunning = false;
    bool m_bCanceling = false;
    bool m_bNoSched = false;
    EnumHeadState m_byHeadState = waitStart;

    protected:

    void PopTask();

    std::deque< TaskletPtr > m_queTasks;
    std::vector< gint32 > m_vecRetVals;
#ifdef DEBUG
    std::vector< gint32 > m_vecClsids;
#endif

    virtual gint32 RunTaskInternal(
        guint32 dwContext );

    public:
    typedef CIfRetryTask super;

    CIfTaskGroup( const IConfigDb* pCfg )
       : super( pCfg ) 
    {
        SetClassId( clsid( CIfTaskGroup ) );
        SetRelation( logicAND );
        m_iTaskState = stateStarting;
    }

    virtual bool IsMultiThreadSafe()
    { return true; }

    virtual gint32 RunTask();
    virtual gint32 OnChildComplete(
        gint32 ret, CTasklet* pChild );

    bool exist( TaskletPtr& pTask );

    inline const std::vector< gint32 >&
        GetRetVals() const
    { return m_vecRetVals; }

    inline bool IsRunning() const
    { return m_bRunning; }

    inline void SetRunning( bool bRun = true )
    { m_bRunning = bRun; }

    inline bool IsCanceling() const
    { return m_bCanceling; }

    void SetCanceling( bool bCancel );

    inline bool IsNoSched() const
    { return m_bNoSched; }

    inline void SetNoSched( bool bNoSched = true )
    { m_bNoSched = bNoSched; }

    inline EnumTaskState GetTaskState() const
    {
        return m_iTaskState;
    }

    inline void SetTaskState(
        EnumTaskState iState )
    {
        m_iTaskState = iState;
    }

    virtual gint32 AppendTask(
        TaskletPtr& pTask );

    virtual guint32 GetTaskCount() 
    {
        CStdRTMutex oLock( GetLock() );
        return m_queTasks.size();
    }

    virtual gint32 InsertTask( TaskletPtr& pTask );
    virtual guint32 Clear() 
    {
        CStdRTMutex oLock( GetLock() );
        m_queTasks.clear();
        return 0;
    }
    virtual gint32 FindTask( guint64 iTaskId,
        TaskletPtr& pTask );

    virtual gint32 FindTaskByRmtId(
        guint64 iTaskId, TaskletPtr& pRet );

    virtual gint32 FindTaskByClsid(
        EnumClsid iClsid,
        std::vector< TaskletPtr >& vecTasks );

    // the tasks have two types of relation,
    // AND and OR
    void SetRelation( EnumLogicOp iOp )
    { m_iTaskRel = iOp; }

    EnumLogicOp GetRelation() const
    { return m_iTaskRel; }

    gint32 Process( guint32 dwContext ) override;
    gint32 OnCancel( guint32 dwContext ) override;
    virtual gint32 RemoveTask( TaskletPtr& pTask );

    gint32 OnRetry() override;
    gint32 OnComplete( gint32 iRet ) override;

    inline void GetRetVals(
        std::vector< gint32 >& vecRet ) const
    { vecRet = m_vecRetVals; }

    gint32 GetHeadTask( TaskletPtr& pHead );
    gint32 GetTailTask( TaskletPtr& pTail );
    gint32 OnTaskComplete( gint32 iRetVal ) override;
    bool IsStopped( EnumTaskState iState ) const;

    inline void SetHeadState( EnumHeadState byState )
    { m_byHeadState = byState; }

    inline EnumHeadState GetHeadState() const
    { return m_byHeadState; }

    template< class T >
    gint32 FindTaskByType(
        TaskletPtr& pTask, T* pHint,
        bool bReverse = false );

    template< class T >
    gint32 FindTasksByType(
        std::vector< TaskletPtr > vecTasks,
        T* pHint );
};

typedef CAutoPtr< Clsid_Invalid, CIfTaskGroup > TaskGrpPtr;

template< class T >
gint32 CIfTaskGroup::FindTaskByType(
    TaskletPtr& pTask, T* pHint,
    bool bReverse )
{
    pHint = nullptr;
    CStdRTMutex oLock( GetLock() );
    if( m_queTasks.empty() )
        return -ENOENT;
    if( !bReverse )
    {
        auto itr = m_queTasks.begin();
        while( itr != m_queTasks.end() )
        {
            pHint = *itr;
            if( pHint == nullptr )
            {
                itr++;
                continue;
            }
            pTask = *itr;
            break;
        }
    }
    else
    {
        auto itr = m_queTasks.rbegin();
        while( itr != m_queTasks.rend() )
        {
            pHint = *itr;
            if( pHint == nullptr )
            {
                itr++;
                continue;
            }
            pTask = *itr;
            break;
        }
    }
    if( pHint != nullptr )
        return STATUS_SUCCESS;
    return -ENOENT;
}

class CIfRootTaskGroup
    : public CIfTaskGroup
{
    protected:
    gint32 RunTaskInternal(
        guint32 dwContext ) override;
    public:
    typedef CIfTaskGroup super;

    CIfRootTaskGroup( const IConfigDb* pCfg );

    gint32 OnComplete( gint32 iRet ) override;

    gint32 OnChildComplete(
        gint32 iRet, CTasklet* pChild ) override;

    gint32 AppendAndRun(
        TaskletPtr& pTask );

    gint32 OnCancel(
        guint32 dwContext ) override;
};


class CIfDummyTask
    : public CTasklet
{
    public:
    typedef CTasklet super;

    CIfDummyTask( const IConfigDb* pCfg );
    virtual gint32 RunTask();
    gint32 operator()( guint32 dwContext );
    virtual gint32 OnEvent(
        EnumEventId iEvent,
        LONGWORD dwParam1,
        LONGWORD dwParam2,
        LONGWORD* pData );
};

class CIfParallelTaskGrp
    : public CIfTaskGroup
{
    // a task group to synchronize the concurrent
    // tasks with the sequencial tasks
    protected:
    std::set< TaskletPtr > m_setTasks;
    std::deque< TaskletPtr > m_quePendingTasks;

    guint32 GetPendingCount() const
    { return m_quePendingTasks.size(); }

    public:

    virtual bool IsMultiThreadSafe()
    { return true; }

    typedef CIfTaskGroup super;
    CIfParallelTaskGrp( const IConfigDb* pCfg )
       : super( pCfg ) 
    {
        SetClassId( clsid( CIfParallelTaskGrp ) );
        SetRelation( logicNONE );
    }

    virtual gint32 RunTaskInternal(
        guint32 dwContext );

    virtual gint32 OnChildComplete(
        gint32 ret, CTasklet* pChild );

    gint32 AppendTask(
        TaskletPtr& pTask ) override;

    gint32 FindTask( guint64 iTaskId,
        TaskletPtr& pTask );

    gint32 FindTaskByRmtId(
        guint64 iTaskId, TaskletPtr& pRet );

    gint32 FindTaskByClsid(
        EnumClsid iClsid,
        std::vector< TaskletPtr >& vecTasks );

    guint32 GetTaskCount() override
    {
        CStdRTMutex oLock( GetLock() );
        return m_setTasks.size() +
            m_quePendingTasks.size();
    }

    virtual guint32 Clear() 
    {
        CStdRTMutex oLock( GetLock() );
        m_setTasks.clear();
        m_quePendingTasks.clear();
        return 0;
    }

    guint32 GetPendingCountLocked()
    {
        CStdRTMutex oLock( GetLock() );
        return GetPendingCount();
    }

    virtual gint32 RemoveTask( TaskletPtr& pTask );
    gint32 OnCancel( guint32 dwContext );

    template< class T >
    gint32 FindTaskByType(
        TaskletPtr& pTask, T* pHint )
    {
        pHint = nullptr;
        CStdRTMutex oLock( GetLock() );
        for( auto elem : m_setTasks )
        {
            pHint = elem;
            if( pHint == nullptr )
                continue;
            pTask = elem;
            break;
        }
        if( pHint != nullptr )
            return STATUS_SUCCESS;
        for( auto elem : m_quePendingTasks )
        {
            pHint = elem;
            if( pHint == nullptr )
                continue;
            pTask = elem;
            break;
        }
        if( pHint != nullptr )
            return STATUS_SUCCESS;
        return -ENOENT;
    }

    template< class T >
    gint32 FindTasksByType(
        std::vector< TaskletPtr > vecTasks, T* pHint )
    {
        pHint = nullptr;
        std::vector< TaskGrpPtr > vecGrps;
        CStdRTMutex oLock( GetLock() );
        for( auto elem : m_setTasks )
        {
            pHint = elem;
            CIfTaskGroup* pGrp = elem;
            if( pHint == nullptr && pGrp == nullptr )
                continue;
            if( pHint )
            {
                vecTasks.push_back( elem );
                continue;
            }
            vecGrps.push_back( 
                TaskGrpPtr( pGrp ) );
        }
        for( auto elem : m_quePendingTasks )
        {
            pHint = elem;
            CIfTaskGroup* pGrp = elem;
            if( pHint == nullptr && pGrp == nullptr )
                continue;
            if( pHint )
            {
                vecTasks.push_back( elem );
                continue;
            }
            vecGrps.push_back( 
                TaskGrpPtr( pGrp ) );
        }
        oLock.Unlock();
        for( auto elem : vecGrps )
        {
            CIfParallelTaskGrp* ppGrp = elem;
            if( ppGrp )
                ppGrp->FindTasksByType(
                    vecTasks, pHint );
            else
                elem->FindTasksByType(
                    vecTasks, pHint );
        }
        if( vecTasks.size() )
            return STATUS_SUCCESS;
        return -ENOENT;
    }
};

template< class T >
gint32 CIfTaskGroup::FindTasksByType(
    std::vector< TaskletPtr > vecTasks, T* pHint )
{
    pHint = nullptr;
    std::vector< TaskGrpPtr > vecGrps;
    CStdRTMutex oLock( GetLock() );
    for( auto elem : m_queTasks )
    {
        pHint = elem;
        CIfTaskGroup* pGrp = elem;
        if( pHint == nullptr && pGrp == nullptr )
            continue;
        if( pHint )
        {
            vecTasks.push_back( elem );
            continue;
        }
        vecGrps.push_back( 
            TaskGrpPtr( pGrp ) );
    }
    oLock.Unlock();
    for( auto elem : vecGrps )
    {
        CIfParallelTaskGrp* ppGrp = elem;
        if( ppGrp )
            ppGrp->FindTasksByType(
                vecTasks, pHint );
        else
            elem->FindTasksByType(
                vecTasks, pHint );
    }
    if( vecTasks.size() )
        return STATUS_SUCCESS;
    return -ENOENT;
}

class CIfStartRecvMsgTask
    : public CIfParallelTask
{
    public:
    typedef CIfParallelTask super;
    CIfStartRecvMsgTask( const IConfigDb* pCfg );
    virtual gint32 OnIrpComplete( PIRP pIrp );
    virtual gint32 RunTask();
    virtual gint32 OnCancel( guint32 dwContext );
    virtual gint32 StartNewRecv( ObjPtr& pCfg );

    template< typename T1, 
        typename T=typename std::enable_if<
            std::is_same<T1, CfgPtr>::value ||
            std::is_same<T1, DMsgPtr>::value, T1 >::type >
    gint32 HandleIncomingMsg( ObjPtr& ifPtr,  T1& pMsg );
};


class CIfIoReqTask
    : public CIfParallelTask
{
    public:
    typedef CIfParallelTask super;
    CIfIoReqTask( const IConfigDb* pCfg )
        : super( pCfg )
    { SetClassId( clsid( CIfIoReqTask ) ); }

    virtual gint32 OnIrpComplete( IRP* pIrp );
    virtual gint32 OnCancel( guint32 dwContext );
    virtual gint32 OnComplete( gint32 iRet );

    gint32 OnSendKANotify();

    gint32 SubmitIoRequest();

    virtual gint32 RunTask();
    // virtual gint32 WaitForComplete();

    virtual gint32 OnKeepAlive(
        guint32 dwContext );

    gint32 FilterMessageSend( 
        IConfigDb* pReqMsg );

    gint32 Process(
        guint32 dwContext );

    gint32 OnFilterComp();

    // reset the irp expire timer
    gint32 ResetTimer();
};

class CIfInvokeMethodTask
    : public CIfParallelTask
{

    // the timer id for keep alive
    gint32 m_iKeepAlive;

    // the timer id for service timeout
    gint32 m_iTimeoutId;

    protected:

    gint32 OnKeepAliveOrig();
    gint32 OnKeepAliveRelay();

    public:
    typedef CIfParallelTask super;


    CIfInvokeMethodTask( const IConfigDb* pCfg );

    // don't call these methods directly
    virtual gint32 RunTask();
    virtual gint32 OnCancel( guint32 dwContext );
    virtual gint32 OnTaskComplete( gint32 iRetVal );
    // gint32 operator()( guint32 dwContext );
    virtual gint32 OnComplete( gint32 iRetVal );

    // helper methods for request options
    gint32 GetCallOptions( CfgPtr& pCfg ) const;
    gint32 GetCallFlags( guint32& dwCallFlags ) const;
    // interval to send out the heartbeat
    gint32 GetKeepAliveSec( guint32& dwTimeoutSec ) const; 
    gint32 DisableKeepAlive();

    // interval before cancel the request if async
    gint32 GetTimeoutSec( guint32& dwTimeoutSec ) const; 

    bool IsKeepAlive() const;
    gint32 SetAsyncCall( bool bAsync = true );
    bool HasReply() const;
    virtual gint32 OnKeepAlive(
        guint32 dwContext );

    // filter related methods
    gint32 FilterMessage( 
        DBusMessage* pReqMsg,
        IConfigDb* pReqMsg2 );

    gint32 OnFilterComp();

    virtual gint32 Process( guint32 dwContext );

    // get interface id of the request method
    gint32 GetIid( EnumClsid& iid );

    // reset the task expire timer
    gint32 ResetTimer();

    gint32 GetTimeLeft();
    gint32 GetAgeSec( guint32& dwAge );
};

class CIfInterceptTask :
    public CIfParallelTask
{
    public:
    typedef CIfParallelTask super;

    // this task serves as a joint task of two IO
    // tasks from one interface to another
    // interface. it is used in the asynchronous
    // server side serivice call.
    //
    // propEventSink: a pointer to the task to
    // intercept, this is the mandatory property
    //
    // NOTE: to use this class, you must first
    // call RunTask via OnEvent or operator() to
    // initialize the task state and the
    // sync/async flag. If RunTask returns other
    // than STATUS_PENDING, the task is done, and
    // it will not do any further processing.
    // Otherwise the rest work will be done in
    // OnTaskComplete, or OnIrpComplete, depending
    // which callback this task is attached to
    //
    CIfInterceptTask( const IConfigDb* pCfg )
        : super( pCfg )
    {};

    // get the task to intercept
    inline gint32 GetInterceptTask( EventPtr& ptr ) const
    { return GetClientNotify( ptr ); }

    gint32 GetCallerTask( TaskletPtr& pTask )
    {
        // get the task ptr, from whom this task
        // gets called via IEventSink::OnEvent
        gint32 ret = 0;
        do{
            std::vector< LONGWORD > vecParams;
            ret = GetParamList( vecParams );
            if( ERROR( ret ) )
            {
                // not called from the OnEvent method
                break;
            }

            if( vecParams[ 0 ] != eventTaskComp )
            {
                ret = -EINVAL;
                break;
            }

            // 64bit incompatible
            CObjBase* pObjBase =
                ( CObjBase* )vecParams.back();

            if( pObjBase == nullptr )
            {
                // no response data
                break;
            }

            CTasklet* pCallerTask =
                dynamic_cast< CTasklet* >( pObjBase );

            if( pCallerTask == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            pTask = pCallerTask;

        }while( 0 );

        return ret;
    }

    virtual gint32 OnKeepAlive(
        guint32 dwContext );
};

class CIfInterceptTaskProxy :
    public CIfInterceptTask
{
    gint32 m_iTimeoutId = 0;

    public:
    typedef CIfInterceptTask super;

    // this task serves as a joint task of two IO
    // tasks from one interface to another
    // interface. it is used in proxy side
    // asynchronous service call.
    //
    // the difference from the CIfInterceptTask is
    // that it will intercept a callback from the
    // CIfIoReqTask, which is a proxy side call,
    // and it will utilize the notification
    // mechanism in CIfRetryTask::OnComplete to
    // notify the intercepted callback
    //
    // propEventSink( optional ): a pointer to the
    // task to intercept, this is the mandatory
    // property
    CIfInterceptTaskProxy( const IConfigDb* pCfg )
        : super( pCfg )
    {
        EventPtr pEvt;
        gint32 ret = GetInterceptTask( pEvt );
        if( ERROR( ret ) )
            return;
        SetInterceptTask( pEvt );
    };

    ~CIfInterceptTaskProxy()
    { DisableTimer(); }

    gint32 EnableTimer(
        guint32 dwTimeoutSec,
        EnumEventId timerEvt =
            eventTimeoutCancel );

    gint32 DisableTimer();

    // set the task to intercept
    gint32 SetInterceptTask( IEventSink* pEvent )
    {
        if( pEvent != nullptr )
        {
            SetClientNotify( pEvent );
        }
        else
        {
            ClearClientNotify();
        }

        return 0;
    }

    gint32 OnCancel( guint32 dwContext );
    gint32 OnComplete( gint32 iRetVal );
};

class CIfFetchDataTask : public CIfRetryTask
{
    std::atomic< bool > m_bExit;
    std::atomic< bool > m_bEnded;

    public:

    typedef CIfRetryTask super;

    CIfFetchDataTask( const IConfigDb* pCfg )
        : super( pCfg ),
        m_bExit( false ),
        m_bEnded( true )
    {
        SetClassId( clsid( CIfFetchDataTask ) );
    }
    gint32 RunTask();
    gint32 OnCancel( guint32 dwContext );
    gint32 operator()( guint32 dwContext )
    {
        gint32 ret = super::operator()( dwContext );
        if( ret != STATUS_PENDING &&
            ret != STATUS_MORE_PROCESS_NEEDED )
        {
            m_bEnded.store( true,
                std::memory_order_release );
        }
        return ret;
    }
};

class CIfStartExCompletion :
    public CIfInterceptTaskProxy
{
    public:
    typedef CIfInterceptTaskProxy super;
    CIfStartExCompletion( const IConfigDb* pCfg )
        : CIfInterceptTaskProxy( pCfg )
    {
        SetClassId( clsid( CIfStartExCompletion ) );
    }
    virtual gint32 RunTask()
    { return STATUS_PENDING; }

    virtual gint32 OnTaskComplete( gint32 iRetVal );

    gint32 OnCancel( gint32 dwContext )
    {  return OnTaskComplete( -ECANCELED ); }
};

class CBusPortStopSingleChildTask
    : public CIfRetryTask
{
    public:
    typedef CIfRetryTask super;
    CBusPortStopSingleChildTask( const IConfigDb* pCfg = nullptr )
        : super( pCfg )
    {
        SetClassId( clsid( CBusPortStopSingleChildTask ) );
    }
    virtual gint32 RunTask();
    virtual gint32 OnIrpComplete( PIRP pIrp );
};

class CPnpMgrStopPortAndDestroyTask
    : public CIfParallelTask
{
    public:
    typedef CIfParallelTask super;
    CPnpMgrStopPortAndDestroyTask ( const IConfigDb* pCfg = nullptr )
        : super( pCfg )
    {
        SetClassId( clsid( CPnpMgrStopPortAndDestroyTask ) );
    }
    gint32 RunTask();
    gint32 OnIrpComplete( PIRP pIrp );
    gint32 OnScheduledTask( guint32 dwContext );
};

class CGenBusPortStopChildTask
    : public CIfParallelTask
{
    public:
    typedef CIfParallelTask super;
    CGenBusPortStopChildTask( const IConfigDb* pCfg = nullptr )
        : super( pCfg )
    {
        SetClassId( clsid( CGenBusPortStopChildTask ) );
    }
    gint32 RunTask();
    gint32 OnTaskComplete( gint32 iRet );
};


class CIfCallbackInterceptor :
    public CIfInterceptTaskProxy
{
    TaskletPtr m_pCallAhead;
    TaskletPtr m_pCallAfter;
    public: 
    typedef CIfInterceptTaskProxy super;

    CIfCallbackInterceptor( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CIfCallbackInterceptor ) ); }

    // for those tasks such as DEFER_CALL's output, who
    // cannot call propEventSink on complete
    //
    // Run pTask ahead of propEventSink
    void InsertCall( TaskletPtr& pTask )
    { m_pCallAhead = pTask; }

    // Run pTask after propEventSink 
    void InsertCallAfter( TaskletPtr& pTask )
    { m_pCallAfter = pTask; }

    gint32 RunTask();
    gint32 OnTaskComplete( gint32 iRet );
    gint32 OnComplete( gint32 iRet );
};

#define CHECK_GRP_STATE \
    CStdRTMutex oTaskLock( GetLock() ); \
    EnumTaskState iState = GetTaskState(); \
    if( IsStopped( iState ) ) \
    { \
        ret = ERROR_STATE; \
        break; \
    } \
    if( IsCanceling() ) \
    { \
        ret = ERROR_STATE; \
        break; \
    } \
    if( !IsRunning() && \
        iState == stateStarted ) \
    { \
        ret = ERROR_STATE; \
        break; \
    }

class CIfParallelTaskGrpRfc :
    public CIfParallelTaskGrp
{
    guint32 m_dwMaxRunning = RFC_MAX_REQS;
    guint32 m_dwMaxPending = RFC_MAX_PENDINGS;
    std::atomic< guint32 > m_dwTaskAdded;
    std::atomic< guint32 > m_dwTaskRejected;

    public:
    typedef CIfParallelTaskGrp super;
    CIfParallelTaskGrpRfc( const IConfigDb* pCfg );

    gint32 SetLimit(
        guint32 dwMaxRunning,
        guint32 dwMaxPending,
        bool bNoResched = false );

    gint32 GetLimit(
        guint32& dwMaxRunning,
        guint32& dwMaxPending ) const;

    inline gint32 GetMaxRunning() const
    { return m_dwMaxRunning; }

    inline guint32 GetMaxPending() const
    { return m_dwMaxPending; }

    inline gint32 GetRunningCount() const
    { return m_setTasks.size() - 1; }

    virtual gint32 RunTaskInternal(
        guint32 dwContext ) override;

    virtual gint32 OnChildComplete(
        gint32 ret, CTasklet* pChild ) override;

    gint32 AddAndRun( TaskletPtr& pTask );

    gint32 AppendTask(
        TaskletPtr& pTask ) override ;

    virtual gint32 InsertTask(
        TaskletPtr& pTask ) override ;

    inline guint32 GetTaskAdded() const
    { return m_dwTaskAdded; }

    inline guint32 GetTaskRejected() const
    { return m_dwTaskRejected; }

    gint32 IsQueueFull( bool& bFull ) const;
};

class CTimerWatchCallback2 :
    public CTasklet
{
    TaskletPtr m_pCallback;
    guint32 m_dwEvent = 0;
    guint32 m_dwContext = 0;
    bool  m_bRepeat = true;
    stdrmutex m_oLock;
    bool m_bQuit = false;
    public:
    typedef CTasklet super;
    CTimerWatchCallback2(
        const IConfigDb* pCfg );
    inline void SetQuit()
    {
        CStdRMutex oLock( m_oLock );
        m_bQuit = true;
    }
    gint32 operator()(
        guint32 dwContext ) override;
};

struct CTokenBucketTask :
    public CIfParallelTask
{
    protected:
    guint32     m_dwMaxTokens = ( guint32 )-1;
    guint32     m_dwTokens = 0;
    EventPtr    m_pNotify;
    TaskletPtr  m_pWatchTask;
    HANDLE      m_hWatch;
    MloopPtr    m_pLoop;
    guint32     m_dwIntervalMs = 0;

    public:
    typedef CIfParallelTask super;

    CTokenBucketTask( const IConfigDb* pCfg );
    gint32 RunTask() override;

    inline bool IsDisabled()
    { 
        guint32 dwToken = 0;;
        gint32 ret = GetMaxTokens( dwToken );
        if( ERROR( ret ) )
            return true;
        return ( ( guint32 )-1 ) == dwToken;
    }

    inline void Disable()
    { SetMaxTokens( ( guint32 )-1 ); }

    gint32 AllocToken( guint32& dwNumReq );
    gint32 FreeToken( guint32 dwNumReq );
    gint32 OnRetry() override;
    gint32 OnComplete( gint32 iRet ) override;

    gint32 GetMaxTokens(
        guint32& dwMaxTokens ) const;
    gint32 EnableTimerWatch();
    gint32 DisableTimerWatch();
    gint32 SetMaxTokens(
        guint32 dwMaxTokens );
};

}
