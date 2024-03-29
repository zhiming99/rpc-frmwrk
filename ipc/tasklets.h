/*
 * =====================================================================================
 *
 *       Filename:  tasklets.h
 *
 *    Description:  implementation of various tasklets
 *
 *        Version:  1.0
 *        Created:  10/06/2016 04:37:03 PM
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
#include <system_error>
#include <unistd.h>
#include "defines.h"
#include "autoptr.h"
#include "irp.h"

namespace rpcf
{

class CTasklet : public ICancellableTask
{
    std::atomic<guint32> m_dwTid;

    std::atomic<bool> m_bPending;
    // flag to avoid reentrancy
    std::atomic<bool> m_bInProcess;
    std::atomic<guint8> m_byPriority = { 0 };

    std::atomic<guint32> m_dwCallCount;

    protected:
    std::atomic<gint32> m_iRet;
    CfgPtr m_pCtx;
    sem_t m_semReentSync;

    public:

    typedef ICancellableTask super;

    CTasklet( const IConfigDb* pCfg = nullptr );

    virtual gint32 operator()( guint32 dwContext ) = 0;
    virtual bool IsMultiThreadSafe()
    { return false; }

    void SetTid()
    { m_dwTid = rpcf::GetTid(); }

    guint32 GetTid()
    { return m_dwTid; }

    inline std::atomic< guint8 >& GetPriority()
    { return m_byPriority; }

    inline void SetPriority( guint8 bPriority )
    { m_byPriority = bPriority; }

    operator IConfigDb*()
    { return ( IConfigDb* )m_pCtx; }

    gint32 GetProperty(
        gint32 iProp,
        Variant& oVar ) const override;

    gint32 SetProperty(
        gint32 iProp,
        const Variant& oVar ) override;

    gint32 EnumProperties(
        std::vector< gint32 >& vecProps ) const;

    gint32 GetPropertyType(
        gint32 iProp, gint32& iType ) const;

    gint32 RemoveProperty(
            gint32 iProp );
    // method to hook up the event interface
    gint32 OnEvent( EnumEventId iEvent,
            LONGWORD dwParam1,
            LONGWORD dwParam2,
            LONGWORD* pData );

    gint32 OnCancel( guint32 dwContext )
    { return 0;}

    const CfgPtr& GetConfig() const
    {  return m_pCtx; }

    CfgPtr& GetConfig()
    {  return m_pCtx; }

    virtual bool IsAsync() const
    { return true; }

    inline gint32 GetError() const
    { return m_iRet; }

    inline gint32 SetError( gint32 err )
    {
        m_iRet = err;
        return err;
    }

    LwVecPtr GetParamList(
        EnumPropId iProp = propParamList );

    virtual gint32 GetParamList(
        std::vector< LONGWORD >& vecParams,
        EnumPropId iProp = propParamList );

    virtual gint32 GetIrpFromParams(
        IrpPtr& pIrp );

    class CReentrancyGuard
    {
        CTasklet* m_pTask;

        public:
        CReentrancyGuard( CTasklet* pTask )
            : m_pTask( pTask )
        {
            if( pTask != nullptr )
                pTask->MarkInProcess();
        }
        ~CReentrancyGuard()
        {
            if( m_pTask != nullptr )
                m_pTask->ClearInProcess();
        }
    };

    void MarkInProcess()
    {
        if( IsMultiThreadSafe() )
        {
            ++m_dwCallCount;
            return;
        }

        bool bExpected = false;
        while( m_bInProcess.compare_exchange_strong(
            bExpected, true ) == false )
        {
            if( bExpected == false )
            {
                sched_yield();
                continue;
            }
            // already locked by someone
            Sem_Wait( &m_semReentSync );
        }
    }

    void ClearInProcess()
    {
        if( IsMultiThreadSafe() )
        {
            --m_dwCallCount;
            return;
        }
        bool bExpected = true;
        while( m_bInProcess.compare_exchange_strong(
            bExpected, false ) == false )
        {
            if( bExpected == true )
            {
                // retry
                sched_yield();
                continue;
            }
            // already cleared?
            usleep( 10000 );
        }
        Sem_Post( &m_semReentSync );
    }

    bool IsInProcess()
    {
        if( IsMultiThreadSafe() )
            return m_dwCallCount > 0;

        return m_bInProcess;
    }

    void MarkPending( bool bPending = true )
    { m_bPending = bPending; }

    bool IsPending()
    { return m_bPending; }
};

typedef CAutoPtr< Clsid_Invalid, CTasklet > TaskletPtr;

}

namespace std
{
    using namespace rpcf;
    template<>
    struct less<TaskletPtr>
    {
        bool operator()(const TaskletPtr& k1, const TaskletPtr& k2) const
        {
            if( unlikely( k2.IsEmpty() ) )
                return false;

            if( unlikely( k1.IsEmpty() ) )
                return true;

            return k1->GetObjId() < k2->GetObjId();
        }
    };
}

namespace rpcf
{

class CTaskletSync : public CTasklet
{
    protected:
    sem_t m_semSync;

    public:
    typedef CTasklet super;

    CTaskletSync( const IConfigDb* pCfg = nullptr )
        : super( pCfg )
    {
        Sem_Init( &m_semSync, 0, 0 );
    }

    ~CTaskletSync()
    {
        sem_destroy( &m_semSync );
    }

    // method to hook up the event interface
    gint32 OnEvent( EnumEventId iEvent,
            LONGWORD dwParam1,
            LONGWORD dwParam2,
            LONGWORD* pData )
    {

        gint32 ret = CTasklet::OnEvent(
            iEvent, dwParam1, dwParam2, pData );

        Sem_Post( &m_semSync );
        return ret;
    }

    inline bool IsAsync() const
    { return false; }

    inline gint32 WaitForComplete()
    {
        return Sem_Wait( &m_semSync );
    }
};

#define TASKLET_RETRY_DEF_INTERVALSEC 10
#define TASKLET_RETRY_TIMES 3
class CTaskletRetriable : public CTasklet
{

    gint32 m_iTimerId;


    public:

    typedef CTasklet  super;

    CTaskletRetriable( const IConfigDb* pCfg );

    virtual gint32 operator()( guint32 dwContext );
    virtual gint32 Process( guint32 dwContext ) = 0;
    void ResetRetries();
    gint32 DecRetries();
    bool CanRetry() const;
    gint32 ScheduleRetry();

    gint32 ResetTimer( gint32 iTimerId ) const;
    gint32 ResetTimer();
    gint32 RemoveTimer( gint32 iTimerId ) const;
    gint32 RemoveTimer();
    inline void ClearTimer()
    { m_iTimerId = 0; }

    gint32 AddTimer(
        guint32 dwIntervalSec,
        guint32 dwParam );

    virtual gint32 SetProperty(
            gint32 iProp, const Variant& oBuf );
    virtual gint32 OnCancel( guint32 dwContext );
};

class CIoMgrStopTask
    : public CTaskletSync
{
    public:
    typedef CTaskletSync super;
    CIoMgrStopTask( const IConfigDb* pCfg = nullptr )
        : CTaskletSync( pCfg )
    {
        SetClassId( clsid( CIoMgrStopTask ) );
    }
    gint32 operator()( guint32 dwContext );
};

class CIoMgrPostStartTask
    : public CTasklet
{
    public:
    typedef CTasklet super;
    CIoMgrPostStartTask( const IConfigDb* pCfg = nullptr )
        : CTasklet( pCfg )
    {
        SetClassId( clsid( CIoMgrPostStartTask ) );
    }

    gint32 operator()( guint32 dwContext );
};

class CPnpMgrQueryStopCompletion
    : public CTasklet
{
    public:
    typedef CTasklet super;
    CPnpMgrQueryStopCompletion( const IConfigDb* pCfg = nullptr )
        : CTasklet( pCfg )
    {
        SetClassId( clsid( CPnpMgrQueryStopCompletion ) );
    }

    gint32 operator()( guint32 dwContext );
};

class CPnpMgrDoStopNoMasterIrp
    : public CTasklet
{
    public:
    typedef CTasklet super;
    CPnpMgrDoStopNoMasterIrp( const IConfigDb* pCfg = nullptr )
        : CTasklet( pCfg )
    {
        SetClassId( clsid( CPnpMgrDoStopNoMasterIrp ) );
    }

    gint32 operator()( guint32 dwContext );
};

class CPortStartStopNotifTask
    : public CTasklet
{
    public:
    typedef CTasklet super;
    CPortStartStopNotifTask( const IConfigDb* pCfg = nullptr )
        : CTasklet( pCfg )
    {
        SetClassId( clsid( CPortStartStopNotifTask ) );
    }
    gint32 operator()( guint32 dwContext );
};
class CPortAttachedNotifTask 
    : public CTasklet
{
    public:
    typedef CTasklet super;
    CPortAttachedNotifTask( const IConfigDb* pCfg = nullptr )
        : CTasklet( pCfg )
    {
        SetClassId( clsid( CPortAttachedNotifTask ) );
    }
    gint32 operator()( guint32 dwContext );
};

class CDBusBusPortCreatePdoTask
    : public CTasklet
{
    public:
    typedef CTasklet super;
    CDBusBusPortCreatePdoTask( const IConfigDb* pCfg = nullptr )
        : CTasklet( pCfg )
    {
        SetClassId( clsid( CDBusBusPortCreatePdoTask ) );
    }
    gint32 operator()( guint32 dwContext );
};

class CIoMgrIrpCompleteTask
    : public CTasklet
{
    public:
    typedef CTasklet super;
    CIoMgrIrpCompleteTask( const IConfigDb* pCfg = nullptr )
        : CTasklet( pCfg )
    {
        SetClassId( clsid( CIoMgrIrpCompleteTask ) );
    }
    gint32 operator()( guint32 dwContext );
};

class CProxyPdoDisconnectTask
    : public CTasklet
{

    public:
    typedef CTasklet super;
    CProxyPdoDisconnectTask( const IConfigDb* pCfg = nullptr )
        : CTasklet( pCfg )
    {
        SetClassId( clsid(  CProxyPdoDisconnectTask ) );
    }
    gint32 operator()( guint32 dwContext );
};

/*class CDBusPdoListenDBEventTask
    : public CTasklet
{
    public:
    typedef CTasklet super;
    CDBusPdoListenDBEventTask( const IConfigDb* pCfg = nullptr )
        : CTasklet( pCfg )
    {
        SetClassId( clsid( CDBusPdoListenDBEventTask ) );
    }
    gint32 operator()( guint32 dwContext );
};*/

class CPnpMgrStartPortCompletionTask
    : public CTasklet
{
    public:
    typedef CTasklet super;
    CPnpMgrStartPortCompletionTask( const IConfigDb* pCfg = nullptr )
        : CTasklet( pCfg )
    {
        SetClassId( clsid( CPnpMgrStartPortCompletionTask ) );
    }
    gint32 operator()( guint32 dwContext );
};

class CPortStateResumeSubmitTask
    : public CTasklet
{
    public:
    typedef CTasklet super;
    CPortStateResumeSubmitTask( const IConfigDb* pCfg = nullptr )
        : CTasklet( pCfg )
    {
        SetClassId( clsid( CPortStateResumeSubmitTask ) );
    }
    gint32 operator()( guint32 dwContext );
};


class CDBusBusPortDBusOfflineTask
    : public CTasklet
{
    public:
    typedef CTasklet super;
    CDBusBusPortDBusOfflineTask( const IConfigDb* pCfg = nullptr )
        : CTasklet( pCfg )
    {
        SetClassId( clsid( CDBusBusPortDBusOfflineTask ) );
    }
    gint32 operator()( guint32 dwContext );
};

class CRpcBasePortModOnOfflineTask
    : public CTasklet
{
    public:
    typedef CTasklet super;
    CRpcBasePortModOnOfflineTask( const IConfigDb* pCfg = nullptr )
        : CTasklet( pCfg )
    {
        SetClassId( clsid( CRpcBasePortModOnOfflineTask ) );
    }
    gint32 operator()( guint32 dwContext );
};

class CProxyFdoModOnOfflineTask
    : public CTasklet
{
    public:
    typedef CTasklet super;
    CProxyFdoModOnOfflineTask( const IConfigDb* pCfg = nullptr )
        : CTasklet( pCfg )
    {
        SetClassId( clsid( CProxyFdoModOnOfflineTask ) );
    }
    gint32 operator()( guint32 dwContext );
};

class CProxyFdoListenTask
    : public CTaskletRetriable
{
    public:
    typedef CTaskletRetriable super;
    CProxyFdoListenTask( const IConfigDb* pCfg = nullptr )
        : super( pCfg )
    {
        SetClassId( clsid( CProxyFdoListenTask ) );
    }
    virtual gint32 Process( guint32 dwContext );
};

class CProxyFdoRmtDBusOfflineTask
    : public CTasklet
{
    public:
    typedef CTasklet super;
    CProxyFdoRmtDBusOfflineTask( const IConfigDb* pCfg = nullptr )
        : CTasklet( pCfg )
    {
        SetClassId( clsid( CProxyFdoRmtDBusOfflineTask ) );
    }
    gint32 operator()( guint32 dwContext );
};

class CDBusTransLpbkMsgTask
    : public CTasklet
{
    public:
    typedef CTasklet super;
    CDBusTransLpbkMsgTask( const IConfigDb* pCfg = nullptr )
        : CTasklet( pCfg )
    {
        SetClassId( clsid( CDBusTransLpbkMsgTask ) );
    }
    gint32 operator()( guint32 dwContext );
};

class CCancelIrpsTask 
    : public CTasklet
{

    public:
    typedef CTasklet super;
    CCancelIrpsTask( const IConfigDb* pCfg = nullptr )
        : CTasklet( pCfg )
    {
        SetClassId( clsid( CCancelIrpsTask ) );
    }
    gint32 operator()( guint32 dwContext );
};

class CStmSockCompleteIrpsTask 
    : public CTasklet
{

    public:
    typedef CTasklet super;
    CStmSockCompleteIrpsTask( const IConfigDb* pCfg = nullptr )
        : CTasklet( pCfg )
    {
        SetClassId( clsid( CStmSockCompleteIrpsTask ) );
    }
    gint32 operator()( guint32 dwContext );
};


class CStmSockInvalStmNotifyTask
    : public CTaskletRetriable
{
    public:
    typedef CTaskletRetriable super;
    CStmSockInvalStmNotifyTask( const IConfigDb* pCfg )
        : CTaskletRetriable( pCfg )
    {
        SetClassId( clsid( CStmSockInvalStmNotifyTask ) );
    }
    gint32 Process( guint32 dwContext );
};

class CStmPdoSvrOfflineNotifyTask
    : public CTaskletRetriable
{
    gint32 OnIrpComplete( gint32 iRet );
    gint32 SendNotify();

    public:
    typedef CTaskletRetriable super;
    CStmPdoSvrOfflineNotifyTask( const IConfigDb* pCfg )
        : CTaskletRetriable( pCfg )
    {
        SetClassId( clsid( CStmPdoSvrOfflineNotifyTask ) );
    }
    gint32 Process( guint32 dwContext );
};

class CFidoRecvDataTask 
    : public CTaskletRetriable
{

    public:
    typedef CTaskletRetriable super;
    CFidoRecvDataTask( const IConfigDb* pCfg = nullptr )
        : CTaskletRetriable( pCfg )
    {
        SetClassId( clsid( CFidoRecvDataTask ) );
    }
    gint32 Process( guint32 dwContext );
};

class CThreadSafeTask : public CTaskletRetriable
{

    protected:
    mutable stdrtmutex  m_oLock;

    public:
    typedef CTaskletRetriable super;

    CThreadSafeTask( const IConfigDb* pCfg )
        : super( pCfg )
    {;}

    inline stdrtmutex& GetLock() const
    { return m_oLock; }

    gint32 GetErrorLocked() const
    {
        CStdRTMutex oTaskLock( GetLock() );
        return m_iRet;
    }

    gint32 GetProperty(
        gint32 iProp,
        Variant& oVar ) const override;

    gint32 SetProperty(
        gint32 iProp,
        const Variant& oVar ) override;

    gint32 RemoveProperty( gint32 iProp )
    {
        CStdRTMutex oTaskLock( GetLock() );
        return super::RemoveProperty( iProp );
    }

    gint32 EnumProperties(
        std::vector< gint32 >& vecProps ) const
    {
        CStdRTMutex oTaskLock( GetLock() );
        return super::EnumProperties( vecProps );
    }

    gint32 GetPropertyType(
        gint32 iProp, gint32& iType ) const
    {
        CStdRTMutex oTaskLock( GetLock() );
        return super::GetPropertyType( iProp, iType );
    }

    using CTasklet::GetParamList;
    virtual gint32 GetParamList(
        std::vector< LONGWORD >& vecParams,
        EnumPropId iProp = propParamList )
    {
        CStdRTMutex oTaskLock( GetLock() );
        return super::GetParamList( vecParams, iProp );
    }

    virtual gint32 GetIrpFromParams(
        IrpPtr& pIrp )
    {
        CStdRTMutex oTaskLock( GetLock() );
        return super::GetIrpFromParams( pIrp );
    }

    gint32 OnEvent( EnumEventId iEvent,
            LONGWORD dwParam1,
            LONGWORD dwParam2,
            LONGWORD* pData );
};

class CSyncCallback :
    public CThreadSafeTask
{
    protected:
    sem_t m_semWait;

    public:
    typedef  CThreadSafeTask super;

    CSyncCallback( const IConfigDb* pCfg = nullptr)
        : CThreadSafeTask( pCfg ) 
    {
        SetClassId( clsid( CSyncCallback ) );
        Sem_Init( &m_semWait, 0, 0 );
        SetError( STATUS_PENDING );
    }

    ~CSyncCallback()
    {
        sem_destroy( &m_semWait );
    }

    gint32 operator()( guint32 dwContext = 0 );
    gint32 WaitForComplete();
    gint32 WaitForCompleteWakable();
    gint32 Process( guint32 dwContext )
    { return -ENOTSUP; }
};

}
