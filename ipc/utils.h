/*
 * =====================================================================================
 *
 *       Filename:  utils.h
 *
 *    Description:  declaration of utilities
 *
 *        Version:  1.0
 *        Created:  01/19/2016 08:21:40 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi(woodhead99@gmail.com )
 *   Organization:  
 *
 * =====================================================================================
 */

#pragma once

#include <thread>
#include "configdb.h"
#include "tasklets.h"

#define MAX_THREAD_LOAD     5
#define MAX_THREADS         10

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

class CTaskThread : public IThread
{
    protected:
    std::thread                 *m_pServiceThread;
    bool                        m_bExit;
    TaskQuePtr                  m_pTaskQue;
    sem_t                       m_semSync;

    gint32 PopHead( TaskletPtr& pTask );

    void ThreadProcWraper( void* context )
    { this->ThreadProc( context ); }

    public:
    CTaskThread();
    ~CTaskThread();

    // the thread started
    gint32 Start();

    // the thread stoped
    gint32 Stop();

    // test if the thread is running
    bool IsRunning();

    virtual void ThreadProc( void* context );

    gint32 GetLoadCount() const;

    void AddTask( TaskletPtr& pTask );
    gint32 RemoveTask( TaskletPtr& pTask );

    // IEventSink method
    gint32 OnEvent( EnumEventId iEvent,
            guint32 dwParam1 = 0,
            guint32 dwParam2 = 0,
            guint32* pData = NULL  );
};

class COneshotTaskThread : public CTaskThread
{
    bool m_bTaskDone;
    public:
    COneshotTaskThread();
    virtual void ThreadProc( void* context );
    gint32 Start();
};

class CThreadPool : public IService
{

    // we are thread-safe
    stdrmutex               m_oLock;
    gint32                  m_iLoadLimit;
    gint32                  m_iMaxThreads;
    EnumClsid               m_iThreadClass;

    protected:
    std::vector< ThreadPtr > m_vecThreads;

    public:

    CThreadPool(
        const IConfigDb* pCfg = nullptr );

    ~CThreadPool();

    stdrmutex& GetLock()
    { return m_oLock; }

    gint32 GetThread(
        ThreadPtr& pThread,
        bool bStart = true );

    // don't call pThread->Release, call PutThread instead
    gint32 PutThread(
        IThread* pThread );

    gint32 Start();
    gint32 Stop();
    
    gint32 OnEvent( EnumEventId iEvent,
            guint32 dwParam1,
            guint32 dwParam2,
            guint32* pData )
    { return 0; }
};

typedef CAutoPtr< Clsid_Invalid, CThreadPool > ThrdPoolPtr;

class CTaskThreadPool : public CThreadPool
{
    static CfgPtr InitCfg( const IConfigDb* pCfg )
    {
        CCfgOpener a;
        if( pCfg == nullptr )
        {
            // load limit
            a.SetIntProp( 0, 2 );
            // max threads
            a.SetIntProp( 1, 2 );
        }
        else
        {
            a.CopyProp( 0, pCfg );
            a.CopyProp( 1, pCfg );
            a.SetIntProp( 2, clsid( CTaskThread ) );
        }
        return a.GetCfg();
    }
    public:

    CTaskThreadPool( const IConfigDb* pCfg ) :
        CThreadPool( ( IConfigDb* )InitCfg( pCfg ) )
    {
        SetClassId( clsid( CTaskThreadPool ) );
    }

    gint32 RemoveTask( TaskletPtr& pTask );
};

class IGenericInterface;

class CIrpCompThread : public IThread
{

    // the incoming request irp to complete
    std::deque<IrpPtr>          m_quePendingIrps;
    mutable std::mutex          m_oMutex;
    sem_t                       m_semIrps;
    sem_t                       m_semSlots;
    bool                        m_bExit;
    std::thread                 *m_pServiceThread;

    std::map< const IGenericInterface*, gint32 > m_mapIfs;
    

    public:
    CIrpCompThread();
    ~CIrpCompThread();
    gint32 AddIrp( IRP* pirp );
    void CompleteIrp( IRP* pirp );
    void ThreadProc( void* context );

    // the thread started
    gint32 Start();

    // the thread stoped
    gint32 Stop();

    // we will use AddRef and Release to
    // manange the reference count.
    gint32 AddIf( IGenericInterface* pif );
    gint32 RemoveIf( IGenericInterface* pif );
    gint32 GetLoadCount() const;
    gint32 OnEvent(EnumEventId, guint32, guint32, guint32*)
    { return 0;}
};

class CIrpThreadPool : public CThreadPool
{
    static CfgPtr InitCfg( const IConfigDb* pCfg )
    {
        CCfgOpener a;

        if( pCfg == nullptr )
        {
            // load limit
            a.SetIntProp( 0, 5 );
            // max threads
            a.SetIntProp( 1, 20 );
        }
        else
        {
            // load limit
            a.CopyProp( 0, pCfg );
            // max threads
            a.CopyProp( 1, pCfg );

            a.SetIntProp( 2, clsid( CIrpCompThread ) );
        }
        return a.GetCfg();
    }

    public:
    CIrpThreadPool( const IConfigDb* pCfg ) :
        CThreadPool( ( IConfigDb* )InitCfg( pCfg ) )
    {
        SetClassId( clsid( CIrpThreadPool ) );
    }
};

#define TIMER_TICK_MS   1000

class CUtilities;
class CTimerService : public IService
{
	struct TIMER_ENTRY
	{
        static std::atomic<gint32> m_atmTimerId;
        gint32 m_iTimerId;
		guint64 m_qwIntervalMs;
        guint64 m_qwTicks;
		guint32 m_dwParam;
        EventPtr m_pCallback;

        TIMER_ENTRY()
        {
            m_iTimerId = m_atmTimerId.fetch_add( 1 );
            if( m_iTimerId == 0x7fffffff )
            {
                // avoid turning negative
                m_atmTimerId.store( 1 );
            }
            m_qwIntervalMs = 0;
            m_qwTicks = 0;
            m_dwParam = 0;
        }
        TIMER_ENTRY( const TIMER_ENTRY& te )
        {
            m_iTimerId = te.m_iTimerId;
            m_qwIntervalMs = te.m_qwIntervalMs;
            m_qwTicks = te.m_qwTicks;
            m_dwParam = te.m_dwParam;
            m_pCallback = te.m_pCallback;
        }

        TIMER_ENTRY& operator=( const TIMER_ENTRY& te )
        {
            new( this ) TIMER_ENTRY( te );
            return *this;
        }
	};

    // timers ticking
	std::vector< TIMER_ENTRY > m_vecTimers;

    // timers timeout
	std::vector< TIMER_ENTRY > m_vecPendingTimeouts;

    CIoManager* m_pIoMgr;
    CUtilities* m_pUtils;
    std::mutex  m_oMapLock;
    GSource*    m_pTimeSrc;
    guint       m_dwTimerId;
    gint64      m_qwTimeStamp;

	public:

    CTimerService(
        CUtilities* pUtils,
        CIoManager* pMgr );

    CTimerService(
        const IConfigDb* pCfg );

    ~CTimerService();

	gint32 AddTimer(
        guint32 dwIntervalMs,
        IEventSink* pEvent,
        guint32 dwParam);

    gint32 RemoveTimer(
        gint32 iTimerId );

    gint32 ResetTimer(
        gint32 iTimerId );

    gint32 AdjustTimer(
        gint32 iTimerId,
        gint32 iOffset );

	static gboolean TimerCallback(
        gpointer dwParams );

    gint32 ProcessTimers();
    gint32 TickTimers();
    gint32 Start();
    gint32 Stop();

    gint32 OnEvent( EnumEventId iEvent,
            guint32 dwParam1,
            guint32 dwParam2,
            guint32* pData );

    inline CIoManager* GetIoMgr() const
    { return m_pIoMgr; }
};

typedef CAutoPtr< clsid( CTimerService ), CTimerService > TimerSvcPtr;

class CWorkitemManager : public CObjBase
{

    struct WORK_ITEM
    {
        static std::atomic<gint32> m_atmWorkitemId;

        gint32 m_iWorkitemId;
        guint32 m_dwFlags;
        EventPtr m_pEventSink;
        guint32	m_dwContext;
        WORK_ITEM()
        {
            m_iWorkitemId = m_atmWorkitemId.fetch_add( 1 );
            if( m_iWorkitemId == 0x7fffffff )
            {
                // avoid turning negative
                m_atmWorkitemId.store( 1 );
            }
            m_dwFlags = 0;
            m_dwContext = 0;
        }

        WORK_ITEM( const WORK_ITEM& wi )
        {
            // we won't increase workitem if it 
            // is a copy constructor
            m_iWorkitemId = wi.m_iWorkitemId;
            m_dwFlags = wi.m_dwFlags;
            m_pEventSink = wi.m_pEventSink;
            m_dwContext = wi.m_dwContext;
        }

        WORK_ITEM& operator=( const WORK_ITEM& rhs )
        {
            new ( this ) WORK_ITEM( rhs );
            return *this;
        }
    };

	std::deque<WORK_ITEM> m_queWorkitems;
	std::deque<WORK_ITEM> m_queWorkitemsLongWait;

    CIoManager* m_pIoMgr;
    CUtilities* m_pUtils;

    std::mutex  m_oQueLock;

	public:

    CWorkitemManager(
        CUtilities* pUtils,
        CIoManager* pMgr );

    CWorkitemManager(
        const IConfigDb* pCfg );

	gint32 ScheduleWorkitem(
        guint32 dwFlags,
        IEventSink* pWorkitem,
        guint32 dwContext,
        bool bLongWait );

	gint32 CancelWorkitem(
        gint32 iWorkitemId );

    gint32 ProcessWorkItems(
        gint32 iThreadIdx );
};

typedef CAutoPtr< clsid( CWorkitemManager ), CWorkitemManager > WiMgrPtr;

class CUtilities : public IService
{
    TimerSvcPtr   m_pTimerSvc;
    WiMgrPtr      m_pWiMgr;

    // helper thread for timer and workitem
    std::thread   *m_pWorkerThread;
    sem_t         m_semWaitingLock;
 
    // another workitem thread if the wait time is longer than 1s
    std::thread   *m_pWorkerThreadLongWait;
    sem_t         m_semWaitingLockLongWait;

    // flag to exit the worker thread
    bool          m_bExit;
    CIoManager*   m_pIoMgr;

    public:
    CUtilities( const IConfigDb* pCfg );
    ~CUtilities();

    
    gint32 Start();
    gint32 Stop();

    // wake up the thread if there is an event
    void Wakeup( gint32 iThreadIdx );

    CTimerService& GetTimerSvc()
    { return *m_pTimerSvc; }

    CWorkitemManager& GetWorkitemManager()
    { return *m_pWiMgr; }

    gint32 ThreadProc( gint32 iThreadIdx );
    gint32 OnEvent( EnumEventId iEvent,
            guint32 dwParam1,
            guint32 dwParam2,
            guint32* pData )
    { return 0;}

    inline CIoManager* GetIoMgr() const
    { return m_pIoMgr; }
};

typedef CAutoPtr< clsid( CUtilities ), CUtilities > UtilsPtr;

