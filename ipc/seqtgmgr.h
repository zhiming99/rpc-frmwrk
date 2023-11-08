/* * =====================================================================================
 *
 *       Filename:  seqtgmgr.h
 *
 *    Description:  declarations of classes and  functions for sequential tasks
 *
 *        Version:  1.0
 *        Created:  10/07/2023 08:46:09 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:  
 *
 *      Copyright:  2023 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#pragma once
#include "ifhelper.h"
namespace rpcf{

template< class T >
bool IsKeyInvalid( const T& key )
{
    static_assert( "Type not defined" );
    return false;
}

template<>
bool IsKeyInvalid( const HANDLE& key );

template<>
bool IsKeyInvalid( const stdstr& key );

#ifdef BUILD_64
template<>
bool IsKeyInvalid( const guint32& key );
#endif

struct SEQTG_ELEM
{
    TaskGrpPtr m_pSeqTasks;
    EnumTaskState m_iState = stateStarting;
    SEQTG_ELEM( const SEQTG_ELEM& elem )
    {
        m_pSeqTasks = elem.m_pSeqTasks;
        m_iState = elem.m_iState;
    }
    SEQTG_ELEM()
    {}
};

template< class Handle, class HostClass >
struct CSeqTaskGrpMgr : public CObjBase
{
    CRpcServices* m_pParent;
    std::hashmap< Handle, SEQTG_ELEM > m_mapSeqTgs;
    EnumIfState m_iMgrState = stateStarted;
    mutable stdrmutex m_oLock;

    typedef CObjBase super;
    CSeqTaskGrpMgr() : super()
    {}

    inline void SetParent( CRpcServices* pParent )
    { m_pParent = pParent; }

    inline CRpcServices* GetParent() const
    { return m_pParent; }

    CIoManager* GetIoMgr()
    {
        if( GetParent() == nullptr )
            return nullptr;
        return GetParent()->GetIoMgr();
    }

    inline stdrmutex& GetLock()
    { return m_oLock; }

    inline void SetState( EnumIfState iState )
    {
        CStdRMutex oLock( GetLock() );
        m_iMgrState = iState;
    }

    gint32 AllocSeqSlot( Handle htg )
    {
        if( IsKeyInvalid( htg ) )
            return -EINVAL;

        CStdRMutex oLock( GetLock() );
        if( m_iMgrState != stateStarted )
            return ERROR_STATE;

        auto itr = m_mapSeqTgs.find( htg );
        if( itr != m_mapSeqTgs.end() )
            return -EEXIST;

        SEQTG_ELEM otg;
        m_mapSeqTgs[ htg ] = otg;
        itr = m_mapSeqTgs.find( htg );
        return 0;
    }

    gint32 AddStartTask(
        Handle htg, TaskletPtr& pTask )
    {
        if( IsKeyInvalid( htg ) || pTask.IsEmpty() )
            return -EINVAL;

        gint32 ret = 0;
        CStdRMutex oLock( GetLock() );
        if( m_iMgrState != stateStarted )
            return ERROR_STATE;

        auto itr = m_mapSeqTgs.find( htg );
        if( itr == m_mapSeqTgs.end() )
        {
            SEQTG_ELEM otg;
            m_mapSeqTgs[ htg ] = otg;
            itr = m_mapSeqTgs.find( htg );
        }
        auto& elem = itr->second;
        elem.m_iState = stateStarted;
        oLock.Unlock();

        ret = AddSeqTaskIf(
            GetParent(),
            elem.m_pSeqTasks,
            pTask, false );

        oLock.Lock();
        if( ERROR( ret ) )
        {
            m_mapSeqTgs.erase( htg );
        }
        return ret;
    }

    gint32 AddSeqTask( Handle htg,
        TaskletPtr& pTask )
    {
        if( IsKeyInvalid( htg ) || pTask.IsEmpty() )
            return -EINVAL;

        gint32 ret = 0;
        do{
            CStdRMutex oLock( GetLock() );
            if( m_iMgrState != stateStarted )
            {
                ret = ERROR_STATE;
                OutputMsg( ret,
                    "AddSeqTask Failed state=%d",
                    m_iMgrState );
                break;
            }
            auto itr = m_mapSeqTgs.find( htg );
            if( itr == m_mapSeqTgs.end() )
            {
                ret = -ENOENT;
                break;
            }

            if( itr->second.m_iState != stateStarted )
            {
                ret = ERROR_STATE;
                OutputMsg( ret,
                    "AddSeqTask Failed state2=%d",
                    itr->second.m_iState );
                break;
            }

            auto& pSeqTasks =
                itr->second.m_pSeqTasks;

            oLock.Unlock();
            ret = AddSeqTaskIf(
                GetParent(),
                pSeqTasks,
                pTask, false );
            if( ret == ERROR_STATE )
            {
                OutputMsg( ret,
                    "AddSeqTaskIf Failed state3=%d",
                    GetParent()->GetState() );
            }
        }while( 0 );

        return ret;
    }

    gint32 AddStopTask( 
        IEventSink* pCallback, Handle htg,
        TaskletPtr& pTask )
    {
        if( IsKeyInvalid( htg ) )
            return -EINVAL;

        gint32 ret = 0;
        do{
            CStdRMutex oLock( GetLock() );
            if( m_iMgrState == stateStopped )
            {
                ret = ERROR_STATE;
                break;
            }
            auto itr = m_mapSeqTgs.find( htg );
            if( itr == m_mapSeqTgs.end() )
            {
                // caller should guarantee the stop
                // task come later than start task
                ret = ERROR_STATE;
                break;
            }

            auto iState = itr->second.m_iState;
            if( unlikely( iState == stateStarting ) )
            {
                oLock.Unlock();
                std::this_thread::yield();
                continue;
            }
            else if( iState != stateStarted )
            {
                ret = ERROR_STATE;
                break;
            }

            TaskGrpPtr& pSeqTasks =
                itr->second.m_pSeqTasks;

            itr->second.m_iState = stateStopped;

            oLock.Unlock();

            if( !pTask.IsEmpty() )
            {
                ret = AddSeqTaskIf( GetParent(),
                    pSeqTasks, pTask, false );

                if( ERROR( ret ) )
                    break;
            }

            gint32 (*func)( ObjPtr&,
                Handle, IEventSink*,
                CRpcServices* ) =

            ([]( ObjPtr& ptgm, Handle htg,
                IEventSink* pCb,
                CRpcServices* pSvc )->gint32
            {
                gint32 ret = 0;
                do{
                    if( ptgm.IsEmpty() ||
                        IsKeyInvalid( htg ) )
                    {
                        ret = -EINVAL;
                        if( pCb != nullptr )
                        {
                            pCb->OnEvent(
                                eventTaskComp,
                                -EINVAL, 0, 0 );
                        }
                        break;
                    }
                    CSeqTaskGrpMgr* pTgMgr = ptgm;

                    CIoManager* pMgr = pSvc->GetIoMgr();
                    TaskletPtr pRemove;
                    ret = DEFER_OBJCALL_NOSCHED(
                        pRemove, pTgMgr,
                        &CSeqTaskGrpMgr::RemoveTg,
                        htg );
                    if( ERROR( ret ) )
                        break;

                    if( pCb != nullptr )
                    {
                        CIfRetryTask* pRetry = pRemove;
                        pRetry->SetClientNotify( pCb );
                    }

                    pRemove->MarkPending();
                    TaskletPtr pManaged;
                    ret = DEFER_CALL_NOSCHED(
                        pManaged, pSvc,
                        &CRpcServices::RunManagedTask,
                        pRemove, false );

                    if( ERROR( ret ) )
                        ( *pRemove )( eventCancelTask );

                    ret = pMgr->RescheduleTaskByTid(
                        pManaged );
                }while( 0 );
                return ret;
            });

            TaskletPtr pCleanup;
            CIoManager* pMgr =
                GetParent()->GetIoMgr();

            gint32 iRet = NEW_FUNCCALL_TASK(
                pCleanup, pMgr, func,
                ObjPtr( this ), htg,
                pCallback, GetParent() );

            if( SUCCEEDED( iRet ) )
            {
                iRet = AddSeqTaskIf(
                    GetParent(),
                    pSeqTasks,
                    pCleanup, false );
            }
            break;

        }while( 1 );

        return ret;
    }

    gint32 RemoveTg( Handle htg )
    {
        if( IsKeyInvalid( htg ) )
            return -EINVAL;
        gint32 ret = 0;
        do{
            CStdRMutex oLock( GetLock() );
            auto itr = m_mapSeqTgs.find( htg );
            if( itr == m_mapSeqTgs.end() )
                break;
            if( itr->second.m_iState != stateStopped )
            {
                ret = ERROR_STATE;
                break;
            }
            SEQTG_ELEM otg = itr->second;
            m_mapSeqTgs.erase( itr );
            auto& pGrp = otg.m_pSeqTasks;
            EnumTaskState iState = pGrp->GetTaskState();
            oLock.Unlock();
            if( !pGrp.IsEmpty() &&
                !pGrp->IsStopped( iState ) )
                ( *pGrp )( eventCancelTask );

        }while( 0 );

        return ret;
    }

    typedef std::pair< Handle, TaskletPtr > HTPAIR;
    gint32 Stop( IEventSink* pCallback )
    {
        gint32 ret = 0;
        do{
            CParamList oParams;
            oParams.SetPointer(
                propIfPtr, GetParent() );
            TaskGrpPtr pGrp;
            ret = pGrp.NewObj(
                clsid( CIfParallelTaskGrp ),
                oParams.GetCfg() );
            if( ERROR( ret ) )
                break;

            CStdRMutex oLock( GetLock() );
            if( m_mapSeqTgs.empty() )
            {
                m_iMgrState = stateStopped;
                break;
            }
            if( m_iMgrState != stateStarted )
            {
                ret = ERROR_STATE;
                break;
            }
            m_iMgrState = stateStopping;
            std::vector< HTPAIR > vecTasks;
            for( auto& elem : m_mapSeqTgs )
            {
                if( elem.second.m_iState ==
                    stateStopped )
                    continue;
                vecTasks.push_back(
                    { elem.first, TaskletPtr() } );
            }
            oLock.Unlock();
            if( vecTasks.empty() )
            {
                ret = -ENOENT;
                break;
            }
            auto pMgr = GetParent()->GetIoMgr();

            gint32 (*func)( IEventSink* ) =
            ([]( IEventSink* pSvc )
            { return 0; } );
            
            for( auto& elem : vecTasks )
            {
                TaskletPtr pNotify;
                NEW_COMPLETE_FUNCALL( -1,
                    pNotify, pMgr,
                    func, GetParent() );
                pGrp->AppendTask( pNotify );
                elem.second = pNotify;
            }

            TaskletPtr pSync;
            CIfRetryTask* pRetry = pGrp;
            if( pCallback != nullptr )
            {
                gint32 (*func1)( IEventSink*, IEventSink*, ObjPtr& ) =
                ([]( IEventSink* pCb,
                    IEventSink* pUserCb,
                    ObjPtr& pseqmgr )->gint32
                {
                    CSeqTaskGrpMgr* pSeqMgr = pseqmgr;
                    if( pSeqMgr != nullptr )
                        pSeqMgr->SetState( stateStopped );

                    IConfigDb* pResp;
                    CCfgOpenerObj oCbCfg( pCb );
                    gint32 ret = oCbCfg.GetPointer(
                        propRespPtr, pResp );
                    if( SUCCEEDED( ret ) )
                    {
                        guint32 dwVal = 0;
                        CCfgOpener oResp( pResp );
                        gint32 iRet = oResp.GetIntProp(
                            propReturnValue,
                            ( guint32& )ret );

                        if( ERROR( iRet ) )
                            ret = iRet;
                    }
                    pUserCb->OnEvent(
                        eventTaskComp, ret, 0, nullptr );
                    return ret;
                });
                TaskletPtr pNotify;
                NEW_COMPLETE_FUNCALL( 0, pNotify,
                    pMgr, func1, nullptr, pCallback,
                    ObjPtr( this ) );
                pRetry->SetClientNotify( pNotify );
            }
            else
            {
                pSync.NewObj( clsid( CSyncCallback ) );
                pRetry->SetClientNotify( pSync );
            }

            ret = GetParent()->RunManagedTask( pGrp );
            if( ERROR( ret ) )
            {
                ( *pGrp )( eventCancelTask );
                break;
            }

            auto phc = dynamic_cast< HostClass* >( GetParent() );
            for( auto& elem : vecTasks )
            {
                ret = phc->OnClose(
                    elem.first, elem.second );
                if( ERROR( ret ) )
                    ( *elem.second )( eventCancelTask );
            }

            if( pGrp->GetTaskCount() == 0 )
            {
                ret = 0;
                break;
            }

            if( !pSync.IsEmpty() )
            {
                CSyncCallback* pscb = pSync;
                pscb->WaitForCompleteWakable();
                ret = pSync->GetError();
                this->SetState( stateStopped );
            }
            else
            {
                ret = STATUS_PENDING;
            }

        }while( 0 );
        return ret;
    }
};

struct CStmSeqTgMgr :
    public CSeqTaskGrpMgr< HANDLE, IStream >
{
    typedef CSeqTaskGrpMgr< HANDLE, IStream > super;
    CStmSeqTgMgr() : super()
    { SetClassId( clsid( CStmSeqTgMgr ) ); }
};

}
