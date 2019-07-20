/*
 * =====================================================================================
 *
 *       Filename:  gmloop.h
 *
 *    Description:  declaration of CGMainLoop class
 *
 *        Version:  1.0
 *        Created:  09/23/2018 02:25:06 PM
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
#ifndef _USE_LIBEV

#pragma once
#include <json/json.h>
#include <vector>
#include <unistd.h>
#include <sys/syscall.h>
#include "namespc.h"
#include "defines.h"
#include "tasklets.h"
#include "mainloop.h"
#include <glib.h>


class CIoManager;
class CGMainLoop : public IMainLoop
{
    protected:

    GMainLoop                       *m_pMainLoop;
    GMainContext                    *m_pMainCtx;

    CIoManager*                     m_pIoMgr;
    mutable stdrmutex               m_oLock;

	public:
    struct SOURCE_HEADER
    {
        EnumSrcType m_bySrcType;
        std::atomic< guint8 > m_byState;
        TaskletPtr m_pCallback;
        CGMainLoop* m_pParent;
        SOURCE_HEADER(
            CGMainLoop* pMainLoop,
            EnumSrcType iType,
            TaskletPtr& pCallback )
            : m_bySrcType( iType ),
            m_byState( srcsReady ),
            m_pCallback( pCallback ),
            m_pParent( pMainLoop )
        {;}

        void SetState( guint8 byState )
        { m_byState = byState; }

        guint8 GetState() const
        { return m_byState; }

        virtual ~SOURCE_HEADER()
        { m_pCallback.Clear(); }
    };

    struct TIMER_SOURCE :
        public SOURCE_HEADER
    {
        GSource* m_pTimerSrc;
        bool m_bRepeat;
        guint32 m_dwIntervalMs;

        TIMER_SOURCE(
            CGMainLoop* pLoop,
            bool bRepeat,
            guint32 dwIntervalMs,
            TaskletPtr& pCallback );
        ~TIMER_SOURCE();

        gint32 InitSource( guint32 dwInterval );
        gint32 Start();

        static void TimerRemove(
            gpointer dwParams );

        static gboolean TimerCallback(
            gpointer dwParams );
    };

    struct IDLE_SOURCE :
        public SOURCE_HEADER
    {
        GSource* m_pIdleSource;

        IDLE_SOURCE(
            CGMainLoop* pLoop,
            TaskletPtr& pCallback );

        ~IDLE_SOURCE();
        gint32 InitSource();
        gint32 Start();
        static void IdleRemove(
            gpointer dwParams );
        static gboolean IdleCallback(
            gpointer pdata );
    };

    struct IO_SOURCE:
        public SOURCE_HEADER
    {
        GSource* m_pIoSource;
        GIOChannel* m_pIoChannel;
        gint32 m_iFd;
        guint32 m_dwOpt;

        IO_SOURCE(
            CGMainLoop* pLoop,
            gint32 iFd,
            guint32 dwOpt,
            TaskletPtr& pCallback );

        ~IO_SOURCE();
        gint32 InitSource( gint32 iFd, guint32 dwOpt );
        gint32 Start();
        static gboolean IoCallback(
            GIOChannel* source,
            GIOCondition condition,
            gpointer data);
        static void IoRemove( gpointer pdata );
    };

    inline std::map< HANDLE, SOURCE_HEADER* >*
    GetMap( EnumSrcType iType )
    {
        switch( iType )
        {
        case srcTimer:
            return &m_mapTimWatch;
        case srcIo:
            return &m_mapIoWatch;
        case srcIdle:
            return &m_mapIdleWatch;
        default:
            break;
        }
        return nullptr;
    }

    guint32 AddSource( SOURCE_HEADER* pSource );

    // remove from the mainloop's notify
    guint32 RemoveSource( SOURCE_HEADER* pSource );

    // remove from the user side
    guint32 RemoveSource( HANDLE hKey,
        EnumSrcType iType );

    typedef IMainLoop super;

    std::map< HANDLE, SOURCE_HEADER* > m_mapTimWatch;
    std::map< HANDLE, SOURCE_HEADER* > m_mapIoWatch;
    std::map< HANDLE, SOURCE_HEADER* > m_mapIdleWatch;

    CGMainLoop( const IConfigDb* pCfg = nullptr );
    ~CGMainLoop();

    GMainContext* GetMainCtx() const
    { return m_pMainCtx; }

    GMainLoop* GetMainLoop() const
    { return m_pMainLoop; }

    inline stdrmutex& GetLock() const
    { return m_oLock; }

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
        guint32& hTimer );

    virtual gint32 RemoveTimerWatch(
        HANDLE hTimer );

    virtual guint64 NowUs();
    

    // io watcher
    virtual gint32 AddIoWatch(
        TaskletPtr& pCallback,
        guint32& hWatch );

    virtual gint32 RemoveIoWatch(
        guint32 hWatch );

    // idle watcher
    // Start a one-shot idle watcher to execute the
    // tasks on the mainloop queue
    virtual gint32 AddIdleWatch(
       TaskletPtr& pCallback,
       HANDLE& hWatch );

    virtual gint32 IsWatchDone(
        HANDLE hWatch, EnumSrcType iType );

    virtual gint32 RemoveIdleWatch(
       HANDLE hWatch );

    gint32 GetSource( HANDLE hWatch,
        EnumSrcType iType,
        SOURCE_HEADER*& pSrc );
    //
    // async watcher
    virtual gint32 AddAsyncWatch(
        TaskletPtr& pCallback,
        guint32& hWatch )
    { return -ENOTSUP; }

    virtual gint32 RemoveAsyncWatch(
        guint32 hWatch )
    { return -ENOTSUP; }

    // IService
    gint32 Start();
    gint32 Stop();
};

#endif
