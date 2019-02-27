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
#include "proxy.h"

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
    CTasklet*           m_pParentTask;

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

    gint32 OnRetry();
    virtual gint32 WaitForComplete();
    gint32 OnCancel( guint32 dwContext );

    bool Retriable( gint32 iRet ) const
    {
        bool ret = false;
        do{
            if( !CanRetry() )
                break;

            if( iRet == STATUS_MORE_PROCESS_NEEDED ||
                iRet == -EAGAIN )
            {
                ret = true;
                break;
            }
        }while( 0 );

        return ret;
    }

    gint32 OnEvent(
        EnumEventId iEvent,
        guint32 dwParam1,
        guint32 dwParam2,
        guint32* pData );

    gint32 GetProperty( gint32 iProp,
            CBuffer& oBuf ) const;

    gint32 SetProperty( gint32 iProp,
        const CBuffer& oBuf );

    gint32 RemoveProperty( gint32 iProp );

    gint32 EnumProperties(
        std::vector< gint32 >& vecProps ) const;

    gint32 GetPropertyType(
        gint32 iProp, gint32& iType ) const;

    inline void ClearClientNotify()
    {
        CCfgOpener oParams(
            ( IConfigDb* )GetConfig() );
        oParams.RemoveProperty( propNotifyClient );
        oParams.RemoveProperty( propEventSink );
    }

    inline gint32 SetClientNotify( IEventSink* pCallback )
    {
        if( pCallback == nullptr )
            return -EINVAL;
        CCfgOpener oParams(
            ( IConfigDb* )GetConfig() );
        oParams.SetBoolProp( propNotifyClient, true );
        oParams.SetObjPtr(
            propEventSink, ObjPtr( pCallback ) );
        return 0;
    }
};

class CIfParallelTask
    : public CIfRetryTask
{

    protected:
    typedef EnumIfState EnumTaskState;
    EnumTaskState m_iTaskState;

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

    gint32 GetProperty( gint32 iProp,
            CBuffer& oBuf ) const;

    gint32 SetProperty( gint32 iProp,
        const CBuffer& oBuf );

    gint32 RemoveProperty( gint32 iProp );

    gint32 EnumProperties(
        std::vector< gint32 >& vecProps ) const;

    gint32 GetPropertyType(
        gint32 iProp, gint32& iType ) const;

    virtual gint32 OnNotify( guint32 event,
        guint32 dwParam1,
        guint32 dwParam2,
        guint32* pData )
    { return STATUS_PENDING; }

    virtual bool IsMultiThreadSafe()
    { return true; }

    gint32 OnEvent(
        EnumEventId iEvent,
        guint32 dwParam1,
        guint32 dwParam2,
        guint32* pData );
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
    virtual gint32 OnCancel( guint32 dwContext )
    { return STATUS_PENDING; }
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
    public:
    typedef CIfParallelTask super;
    CIfCleanupTask( const IConfigDb* pCfg )
        : super( pCfg )
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

class CIfTaskGroup
    : public CIfRetryTask
{
    EnumLogicOp m_iTaskRel;

    protected:
    void PopTask();

    gint32 CancelRemainingTasks();

    gint32 WaitingToCancel(
        TaskletPtr& pTask );

    std::deque< TaskletPtr > m_queTasks;
    std::vector< gint32 > m_vecRetVals;

    public:
    typedef CIfRetryTask super;

    CIfTaskGroup( const IConfigDb* pCfg )
       : super( pCfg ) 
    {
        SetClassId( clsid( CIfTaskGroup ) );
        SetRelation( logicAND );
    }

    virtual bool IsMultiThreadSafe()
    { return true; }

    virtual gint32 RunTask();
    virtual gint32 OnChildComplete(
        gint32 ret, CTasklet* pChild );

    bool exist( TaskletPtr& pTask );

    bool IsRunning() const;
    void SetRunning();

    bool IsCanceling() const;
    void SetCanceling( bool bCancel );

    virtual gint32 AppendTask( TaskletPtr& pTask );
    gint32 AppendAndRun( TaskletPtr& pTask );

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

    gint32 FindTaskByClsid(
        EnumClsid iClsid,
        std::vector< TaskletPtr >& vecTasks );

    // the tasks have two types of relation,
    // AND and OR
    void SetRelation( EnumLogicOp iOp )
    { m_iTaskRel = iOp; }

    EnumLogicOp GetRelation() const
    { return m_iTaskRel; }

    virtual gint32 Process( guint32 dwContext );
    virtual gint32 OnCancel( guint32 dwContext );
    virtual gint32 RemoveTask( TaskletPtr& pTask );

    virtual gint32 OnRetry();
};

typedef CAutoPtr< Clsid_Invalid, CIfTaskGroup > TaskGrpPtr;

class CIfRootTaskGroup
    : public CIfTaskGroup
{
    public:
    typedef CIfTaskGroup super;
    CIfRootTaskGroup( const IConfigDb* pCfg )
       : super( pCfg ) 
    {
        SetClassId( clsid( CIfRootTaskGroup ) );
    }
    // the mutal exclusion can happen between 
    //  ( RunTask, OnCancel, OnChildComplete )
    // 
    virtual gint32 OnComplete( gint32 iRet );
    gint32 GetHeadTask( TaskletPtr& pHead );
    gint32 GetTailTask( TaskletPtr& pTail );
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
        guint32 dwParam1,
        guint32 dwParam2,
        guint32* pData );
};

class CIfServiceNextMsgTask
    : public CIfParallelTask
{
    public:
    typedef CIfParallelTask super;
    CIfServiceNextMsgTask( const IConfigDb* pCfg )
       : super( pCfg ) 
    {
        SetClassId( clsid( CIfServiceNextMsgTask ) );
    }
    virtual gint32 RunTask();
};

class CIfParallelTaskGrp
    : public CIfTaskGroup
{
    // a task group to synchronize the concurrent
    // tasks with the sequencial tasks
    std::set< TaskletPtr > m_setTasks;
    std::set< TaskletPtr > m_setPendingTasks;

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

    virtual gint32 RunTask();
    virtual gint32 OnChildComplete(
        gint32 ret, CTasklet* pChild );

    gint32 RunTaskDirect( TaskletPtr& pTask );
    gint32 AddAndRun( TaskletPtr& pTask );
    gint32 AppendTask( TaskletPtr& pTask );

    gint32 FindTask( guint64 iTaskId,
        TaskletPtr& pTask );

    gint32 FindTaskByRmtId(
        guint64 iTaskId, TaskletPtr& pRet );

    gint32 FindTaskByClsid(
        EnumClsid iClsid,
        std::vector< TaskletPtr >& vecTasks );

    virtual guint32 GetTaskCount() 
    {
        CStdRTMutex oLock( GetLock() );
        return m_setTasks.size() +
            m_setPendingTasks.size();
    }

    virtual guint32 Clear() 
    {
        CStdRTMutex oLock( GetLock() );
        m_setTasks.clear();
        m_setPendingTasks.clear();
        return 0;
    }

    guint32 GetPendingCount()
    {
        return m_setPendingTasks.size();
    }

    virtual gint32 RemoveTask( TaskletPtr& pTask );
    gint32 OnCancel( guint32 dwContext );
};


class CIfStartRecvMsgTask
    : public CIfParallelTask
{
    public:
    typedef CIfParallelTask super;
    CIfStartRecvMsgTask( const IConfigDb* pCfg );
    virtual gint32 OnIrpComplete( PIRP pIrp );
    virtual gint32 RunTask();
    virtual gint32 OnCancel( guint32 dwContext );
    gint32 StartNewRecv( ObjPtr& pCfg );

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

    gint32 SubmitIoRequest();

    virtual gint32 RunTask();
    // virtual gint32 WaitForComplete();

    virtual gint32 OnKeepAlive(
        guint32 dwContext );

    virtual gint32 OnNotify( guint32 event,
        guint32 dwParam1,
        guint32 dwParam2,
        guint32* pData );

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
    gint32 GetInterceptTask( EventPtr& ptr ) const
    {
        CCfgOpener oCfg(
            ( const IConfigDb* )m_pCtx );

        ObjPtr pObj;

        gint32 ret = oCfg.GetObjPtr(
            propEventSink, pObj );

        if( ERROR( ret ) )
            return ret;

        ptr = pObj;
        if( ptr.IsEmpty() )
            return -EFAULT;

        return 0;
    }

    gint32 GetCallerTask( TaskletPtr& pTask )
    {
        // get the task ptr, from whom this task
        // gets called via IEventSink::OnEvent
        gint32 ret = 0;
        do{
            std::vector< guint32 > vecParams;
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
};

class CIfInterceptTaskProxy :
    public CIfInterceptTask
{
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
        GetInterceptTask( pEvt );
        SetInterceptTask( pEvt );
    };

    // set the task to intercept
    gint32 SetInterceptTask( IEventSink* pEvent )
    {
        CCfgOpener oCfg( ( IConfigDb* )m_pCtx );
        if( pEvent != nullptr )
        {
            oCfg.SetObjPtr( propEventSink,
                ObjPtr( pEvent ) );
        }

        oCfg.SetBoolProp(
            propNotifyClient, true );
        
        return 0;
    }
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
