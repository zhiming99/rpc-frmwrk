/*
 * =====================================================================================
 *
 *       Filename:  evloop.h
 *
 *    Description:  Declaration of the CEvLoop and related classes
 *
 *        Version:  1.0
 *        Created:  09/25/2018 11:53:11 AM
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
#include <sys/syscall.h>
#include "namespc.h"
#include "defines.h"
#include "tasklets.h"
#include "mainloop.h"

#ifdef _USE_LIBEV

class CIoManager;
class CEvLoop;

class CDBusLoopHooks : public CObjBase
{
    CEvLoop         *m_pLoop;
    DBusConnection  *m_pConn;
    TaskletPtr      m_pDispatchCb;
    TaskletPtr      m_pWakeupCb;

    mutable stdrmutex       m_oLock;
    std::set< TaskletPtr > m_setIoCbs;
    std::set< TaskletPtr > m_setTimerCbs;
    std::atomic<bool>       m_bSetupDone;

    public:
    typedef CObjBase super;
    CDBusLoopHooks( const IConfigDb* pCfg );
    ~CDBusLoopHooks(){;}

    gint32 Start( DBusConnection* pConn );
    gint32 Stop();

    stdrmutex& GetLock() const
    { return m_oLock; }

    inline DBusConnection* GetDBusConn() const
    { return m_pConn; }

    inline CEvLoop* GetLoop() const
    { return m_pLoop; }

    gint32 AddWatch( TaskletPtr& pCallback,
        EnumSrcType iType );

    gint32 RemoveWatch( TaskletPtr& pCallback,
        EnumSrcType iType );

    inline bool IsSetupDone()
    { return m_bSetupDone; }

    inline void SetSetupDone( bool bDone )
    { m_bSetupDone = bDone; }

    void ClearAll();

    void WakeupDispatch(
        DBusDispatchStatus iStat );

    inline bool IsDBusWatchRemoved()
    {
        CStdRMutex oLock( GetLock() );
        return m_setIoCbs.empty();
    }
};

typedef CAutoPtr< clsid( CDBusLoopHooks ), CDBusLoopHooks > DHookPtr;

#include "sevpoll.h"
class CEvLoop :
    public IMainLoop,
    public CSimpleEvPoll
{
    protected:
    CIoManager*                     m_pIoMgr;

	public:
    DHookPtr                        m_pDBusHook;
    struct SOURCE_HEADER
    {
        EnumSrcType m_bySrcType;
        std::atomic< EnumSrcState > m_byState;
        TaskletPtr m_pCallback;
        CEvLoop* m_pParent;
        SOURCE_HEADER(
            CEvLoop* pMainLoop,
            EnumSrcType iType,
            TaskletPtr& pCallback )
            : m_bySrcType( iType ),
            m_byState( srcsStarting ),
            m_pCallback( pCallback ),
            m_pParent( pMainLoop )
        {;}

        void SetState( EnumSrcState byState )
        { m_byState = byState; }

        EnumSrcState GetState() const
        { return m_byState; }

        virtual ~SOURCE_HEADER()
        { m_pCallback.Clear(); }

        SOURCE_HEADER& operator=(
            SOURCE_HEADER& rhs )
        {
            m_pParent = rhs.m_pParent;
            m_byState = ( EnumSrcState )rhs.m_byState;
            m_pCallback = rhs.m_pCallback;
            m_bySrcType = rhs.m_bySrcType;
            return *this;
        }
        virtual gint32 StartStop( bool bStart );
        virtual gint32 UpdateSource( IConfigDb* ) = 0;
        gint32 Start();
        gint32 Stop();
    };

    friend class SOURCE_HEADER;
    friend class CSimpleEvPoll;

    struct TIMER_SOURCE :
        public SOURCE_HEADER
    {
        typedef SOURCE_HEADER super;
        bool m_bRepeat;
        guint32 m_dwIntervalMs;
        guint64 m_qwAbsTimeUs;
        TIMER_SOURCE(
            CEvLoop* pLoop,
            guint32 dwIntervalMs,
            bool bRepeat,
            TaskletPtr& pCallback );

        ~TIMER_SOURCE();

        gint32 StartStop( bool bStart );
        gint32 TimerCallback(
            int iEvents );

        TIMER_SOURCE& operator=(
            TIMER_SOURCE& rhs )
        {
            m_bRepeat = rhs.m_bRepeat;
            m_dwIntervalMs = rhs.m_dwIntervalMs;
            return *this;
        }

        gint32 UpdateSource(
            IConfigDb* pCfg )
        {
            // please make sure the source is already
            // stopped.
            bool bRepeat = false;
            guint32 dwIntervalMs = 0;
            CParamList oParams( pCfg );

            gint32 ret = oParams.GetIntProp(
                0, dwIntervalMs );
            if( ERROR( ret ) )
                return ret;

            ret = oParams.GetBoolProp(
                1, bRepeat );
            if( ERROR( ret ) )
                return ret;

            m_bRepeat = bRepeat;
            m_dwIntervalMs = dwIntervalMs;
            return 0;
        }
    };

    struct IO_SOURCE:
        public SOURCE_HEADER
    {
        typedef SOURCE_HEADER super;
        gint32 m_iFd;
        guint32 m_dwOpt;
        IO_SOURCE(
            CEvLoop* pLoop,
            gint32 iFd,
            guint32 dwOpt,
            TaskletPtr& pCallback );

        ~IO_SOURCE();

        gint32 IoCallback(
            int iEvents );

        IO_SOURCE& operator=(
            IO_SOURCE& rhs )
        {
            m_iFd = rhs.m_iFd;
            m_dwOpt = rhs.m_dwOpt;
            return *this;
        }

        gint32 UpdateSource(
            IConfigDb* pCfg )
        {
            // please make sure the source is already
            // stopped.
            gint32 fd = -1;
            guint32 dwOpt = 0;
            CParamList oParams( pCfg );
            gint32 ret = oParams.GetIntProp(
                0, ( guint32& )fd );
            if( ERROR( ret ) )
                return ret;

            ret = oParams.GetIntProp(
                1, dwOpt );
            if( ERROR( ret ) )
                return ret;

            m_iFd = fd;
            m_dwOpt = dwOpt;
            return 0;
        }
    };

    struct ASYNC_SOURCE:
        public SOURCE_HEADER
    {
        typedef SOURCE_HEADER super;
        ASYNC_SOURCE(
            CEvLoop* pLoop,
            TaskletPtr& pCallback );

        ~ASYNC_SOURCE();

        gint32 AsyncCallback(
            int iEvents );

        gint32 Send();

        gint32 UpdateSource(
            IConfigDb* pCfg )
        { return 0; }
    };

    guint32 AddSource( SOURCE_HEADER* pSource );

    // remove from the mainloop's notify
    guint32 RemoveSource( SOURCE_HEADER* pSource );

    // remove from the user side
    guint32 RemoveSource( HANDLE hKey,
        EnumSrcType iType );

    typedef IMainLoop super;

    CEvLoop( const IConfigDb* pCfg = nullptr );
    ~CEvLoop();

    inline CIoManager* GetIoMgr() const
    { return m_pIoMgr; }

    gint32 OnEvent( EnumEventId iEvent,
            guint32 dwParam1 = 0,
            guint32 dwParam2 = 0,
            guint32* pData = NULL  )
    { return 0; }

    // io watcher + timer
    virtual gint32 SetupDBusConn(
        DBusConnection* pConn );

    // timer
    virtual gint32 AddTimerWatch(
        TaskletPtr& pCallback,
        HANDLE& hTimer );

    virtual gint32 RemoveTimerWatch(
        HANDLE hTimer );

    virtual guint64 NowUs();

    // io watcher
    virtual gint32 AddIoWatch(
        TaskletPtr& pCallback,
        HANDLE& hWatch );

    virtual gint32 RemoveIoWatch(
        HANDLE hWatch );

    virtual gint32 IsWatchDone(
        HANDLE hWatch, EnumSrcType iType );

    virtual gint32 AddAsyncWatch(
        TaskletPtr& pCallback,
        HANDLE& hWatch );

    virtual gint32 RemoveAsyncWatch(
        HANDLE hWatch );

    virtual gint32 AddIdleWatch(
       TaskletPtr& pCallback,
       HANDLE& hWatch )
    { return -ENOTSUP; }

    virtual gint32 RemoveIdleWatch(
       HANDLE hWatch )
    { return -ENOTSUP; }

    gint32 GetSource(
        HANDLE hWatch,
        EnumSrcType iType,
        SOURCE_HEADER*& pSrc );

    void ClearAll();

    // IService
    gint32 Start();
    gint32 Stop();

    gint32 StopDBusConn();

    gint32 AsyncSend( HANDLE hWatch );
    void WakeupLoop();
    using CSimpleEvPoll::WakeupLoop;

    private:
    // the maps for sources
    std::map< HANDLE, SOURCE_HEADER* >  m_mapTimWatch;
    std::map< HANDLE, SOURCE_HEADER* >  m_mapIoWatch;
    std::map< HANDLE, SOURCE_HEADER* >  m_mapAsyncWatch;

    std::map< HANDLE, SOURCE_HEADER* >*
        GetMap( EnumSrcType iType );

};

class CDBusLoopHookCb :
    public CThreadSafeTask
{
    protected:
    CDBusLoopHooks* m_pParent;
    HANDLE          m_hWatch;
    EnumSrcType     m_iType;

    public:
    typedef CThreadSafeTask super;
    inline EnumSrcType GetType() const
    { return m_iType; }

    CDBusLoopHooks* GetParent()
    { return m_pParent; }

    const CDBusLoopHooks* GetParent() const
    { return m_pParent; }

    gint32 Stop();
    gint32 Process( guint32 dwContext )
    { return -ENOTSUP; }

    CDBusLoopHookCb( const IConfigDb* pCfg );
    ~CDBusLoopHookCb();
};

class CDBusTimerCallback:
    public CDBusLoopHookCb
{
    DBusTimeout *m_pDT;
    public:
    typedef CDBusLoopHookCb super;
    CDBusTimerCallback( const IConfigDb* pCfg )
        : super( pCfg ),
        m_pDT( nullptr )
    { SetClassId( clsid( CDBusTimerCallback ) ); }

    // callbacks from libev
    gint32 operator()( guint32 dwContext );

    // callbacks from dbus
    static dbus_bool_t AddWatch( DBusTimeout* otw, void* data );
    static void RemoveWatch( DBusTimeout* otw, void* data );
    static void ToggleWatch( DBusTimeout* otw, void* data );

    gint32 DoRemoveWatch();
    gint32 DoAddWatch( bool bEnable );
    gint32 DoToggleWatch( bool bEnable );
};

class CDBusIoCallback:
    public CDBusLoopHookCb
{
    DBusWatch *m_pDT;
    public:
    typedef CDBusLoopHookCb super;
    CDBusIoCallback( const IConfigDb* pCfg )
        : super( pCfg ),
        m_pDT( nullptr )
    { SetClassId( clsid( CDBusIoCallback ) ); }

    // callbacks from libev
    gint32 operator()( guint32 dwContext );

    // callbacks from dbus
    static dbus_bool_t AddWatch( DBusWatch* odw, void* data );
    static void RemoveWatch( DBusWatch* odw, void* data );
    static void ToggleWatch( DBusWatch* odw, void* data );

    gint32 DoRemoveWatch();
    gint32 DoAddWatch( bool bEnable );
    gint32 DoToggleWatch( bool bEnable );
};

class CDBusDispatchCallback:
    public CDBusLoopHookCb
{
    public:
    typedef CDBusLoopHookCb super;
    CDBusDispatchCallback( const IConfigDb* pCfg );

    gint32 Initialize();
    // callbacks from libev
    gint32 operator()( guint32 dwContext );

    // callbacks from dbus
    static void DispatchChange(
        DBusConnection *con,
        DBusDispatchStatus s,
        void *data);

    gint32 DoDispatch();
};

class CDBusWakeupCallback:
    public CDBusLoopHookCb
{
    public:
    typedef CDBusLoopHookCb super;
    CDBusWakeupCallback( const IConfigDb* pCfg );
    gint32 Initialize();
    // callbacks from libev
    gint32 operator()( guint32 dwContext );

    // callbacks from dbus
    static void WakeupLoop( void *data);
    static void RemoveWatch( void *data );
    gint32 DoRemoveWatch();
};

class CEvLoopAsyncCallback : public CTasklet
{
    // we need this to wakeup the idle
    CEvLoop* m_pLoop;
    public:
    typedef CTasklet super;
    CEvLoopAsyncCallback( const IConfigDb* pCfg );
    gint32 operator()( guint32 dwContext );
    gint32 HandleCommand();
};

class CEvLoopStopCb : public CTaskletSync
{
    // we need this to wakeup the idle
    public:
    typedef CTaskletSync super;
    CEvLoop* m_pLoop;
    CEvLoopStopCb( const IConfigDb* pCfg );
    gint32 operator()( guint32 dwContext );
};

#endif
