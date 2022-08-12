/*
 * =====================================================================================
 *
 *       Filename:  taskschd.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/15/2021 09:22:48 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:  
 *
 *      Copyright:  2020 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#include "configdb.h"
#include "defines.h"
#include "proxy.h"
#include "rpcroute.h"
#include "dbusport.h"
#include "taskschd.h"

namespace rpcf
{

void CRRTaskScheduler::SetLimitRunningGrps(
    CONNQUE_ELEM& ocq,
    guint32 dwNewLimit )
{
    std::list< TaskGrpPtr >&
        orq = *ocq.m_plstReady;

    std::list< TaskGrpPtr >&
        owq = ocq.m_lstWaitSlot;

    for( auto elem : orq )
    {
        CIfParallelTaskGrpRfc2* pGrpRfc = elem;
        if( pGrpRfc->GetMaxRunning() ==
            ( gint32 )dwNewLimit )
            continue;

        pGrpRfc->SetLimit( dwNewLimit,
            pGrpRfc->GetMaxPending(),
            true );
    }

    if( dwNewLimit == 0 && orq.size() > 0 )
    {
        owq.insert( owq.begin(),
            orq.begin(), orq.end() );
        orq.clear();
    }

    return;
}

void CRRTaskScheduler::SetLimitGrps(
    CONNQUE_ELEM& ocq,
    guint32 dwNewLimit )
{
    std::list< TaskGrpPtr >& orq =
        *ocq.m_plstReady;

    std::list< TaskGrpPtr >& owq =
        ocq.m_lstWaitSlot;

    SetLimitRunningGrps( ocq, dwNewLimit );
    if( dwNewLimit == 0 )
        return;

    for( auto elem : owq )
    {
        CIfParallelTaskGrpRfc2* pGrpRfc = elem;
        pGrpRfc->SetLimit( dwNewLimit,
            pGrpRfc->GetMaxPending(),
            true );
        orq.push_back( pGrpRfc );
    }

    if( dwNewLimit > 0 )
        owq.clear();
}

void CRRTaskScheduler::IncRunningGrps(
    CONNQUE_ELEM& ocq,
    guint32 dwCount )
{
    if( dwCount == 0 )
        return;

    std::list< TaskGrpPtr >&
        orq = *ocq.m_plstReady;

    for( auto elem : orq )
    {
        CIfParallelTaskGrpRfc2* pGrpRfc = elem;
        guint32 dwLimit = ( guint32 )
            pGrpRfc->GetMaxRunning() + 1;
        pGrpRfc->SetLimit( dwLimit,
            pGrpRfc->GetMaxPending(),
            true );
        dwCount--;
        if( dwCount == 0 )
            break;
    }

    return;
}

void CRRTaskScheduler::SelTasksToKill(
    CONNQUE_ELEM& ocq,
    std::vector< TaskletPtr >& vecTasks,
    bool bWaitQue )
{
    std::list< TaskGrpPtr >&
        orq = *ocq.m_plstReady;
    for( auto elem : orq )
    {
        CIfParallelTaskGrpRfc2* pGrpRfc = elem;
        pGrpRfc->SelTasksToKill( vecTasks );
    }

    if( !bWaitQue )
        return;

    std::list< TaskGrpPtr >& owq =
        ocq.m_lstWaitSlot;

    for( auto elem : owq )
    {
        CIfParallelTaskGrpRfc2* pGrpRfc = elem;
        pGrpRfc->SelTasksToKill( vecTasks );
    }

    return;
}

void CRRTaskScheduler::IncWaitingGrp(
    CONNQUE_ELEM& ocq,
    guint32 dwCount )
{
    if( dwCount == 0 )
        return;

    std::list< TaskGrpPtr >& owq =
        ocq.m_lstWaitSlot;

    std::list< TaskGrpPtr >& onq =
        *ocq.m_plstReady;

    std::list< TaskGrpPtr >::iterator
        itr = owq.begin();

    guint32 dwNum = std::min(
        dwCount, ( guint32 )owq.size() );
    std::list< TaskGrpPtr > lstReadyGrps;

    while( dwNum > 0 && itr != owq.end() )
    {
        CIfParallelTaskGrpRfc2*
            pGrpRfc = *itr;

        if( pGrpRfc->HasPendingTasks() > 0 )
        {
            lstReadyGrps.push_back( *itr );
            itr = owq.erase( itr );
            dwNum--;
            continue;
        }
        itr++;
    }

    for( auto elem : lstReadyGrps )
    {
        CIfParallelTaskGrpRfc2* pGrpRfc = elem;
        pGrpRfc->SetLimit( 1,
            pGrpRfc->GetMaxPending(),
            true );
        onq.push_back( elem );
    }
    dwCount -= lstReadyGrps.size();

    itr = owq.begin();
    while( itr != owq.end() && dwCount > 0 )
    {
        CIfParallelTaskGrpRfc2* pGrpRfc = *itr;
        pGrpRfc->SetLimit( 1,
            pGrpRfc->GetMaxPending(),
            true );
        onq.push_back( *itr );
        itr = owq.erase( itr );
        dwCount--;
    }
}

gint32 CRRTaskScheduler::IncSlotCount(
    CONNQUE_ELEM& ocq, guint32 dwCount )
{
    if( dwCount == 0 )
        return STATUS_SUCCESS;

    guint32 dwOldCount = ocq.m_dwMaxSlots;
    guint32 dwNumGrps = ocq.GetTaskGrpCount();
    if( dwNumGrps == 0 )
        return STATUS_SUCCESS;

    guint32 dwNewCount = dwOldCount + dwCount;
    guint32 dwAvg = dwNewCount / dwNumGrps;
    guint32 dwModulo =
        dwNewCount - dwAvg * dwNumGrps;

    if( ocq.m_lstWaitSlot.empty() )
    {
        SetLimitRunningGrps( ocq, dwAvg );
        IncRunningGrps( ocq, dwModulo );
    }
    else
    {
        if( dwAvg == 0 )
        {
            IncWaitingGrp( ocq, dwCount );
        }
        else
        {
            SetLimitGrps( ocq, dwAvg );
            IncRunningGrps( ocq, dwModulo );
        }
    }
    return STATUS_SUCCESS;
}

gint32 CRRTaskScheduler::ResetSlotCountAlloc(
    CONNQUE_ELEM& ocq,
    guint32 dwDelta,
    std::vector< TaskletPtr >& vecTasks )
{
    std::list< TaskGrpPtr >&
        orq = *ocq.m_plstReady;

    std::list< TaskGrpPtr >&
        owq = ocq.m_lstWaitSlot;

    guint32 dwOldCount = ocq.m_dwMaxSlots;
    guint32 dwNumGrps = ocq.GetTaskGrpCount();
    if( dwNumGrps == 0 )
        return STATUS_SUCCESS;

    guint32 dwNewCount = dwOldCount - dwDelta;
    guint32 dwAvg = dwNewCount / dwNumGrps;
    guint32 dwModulo =
        dwNewCount - dwAvg * dwNumGrps;

    do{
        if( dwAvg == 0 )
        {
            SetLimitGrps( ocq, 0 );
            IncWaitingGrp( ocq, dwModulo );
            SelTasksToKill( ocq, vecTasks, true );
        }
        else
        {
            SetLimitGrps( ocq, dwAvg );
            IncRunningGrps( ocq, dwModulo );
            SelTasksToKill( ocq, vecTasks, false );
        }
    }while( 0 );

    return STATUS_SUCCESS;
}

gint32 CRRTaskScheduler::ResetSlotCountRelease(
    CONNQUE_ELEM& ocq,
    guint32 dwDelta )
{
    guint32 dwOldCount = ocq.m_dwMaxSlots;
    guint32 dwNumGrps = ocq.GetTaskGrpCount();
    if( dwNumGrps == 0 )
        return STATUS_SUCCESS;

    guint32 dwNewCount = dwOldCount + dwDelta;
    guint32 dwAvg = dwNewCount / dwNumGrps;
    guint32 dwModulo =
        dwNewCount - dwAvg * dwNumGrps;

    do{
        std::list< TaskGrpPtr >&
            orq = *ocq.m_plstReady;

        std::list< TaskGrpPtr >&
            owq = ocq.m_lstWaitSlot;

        if( dwAvg == 0 )
        {
            if( dwModulo > orq.size() )
            {
                dwModulo -= orq.size();
                IncWaitingGrp( ocq, dwModulo );
                break;
            }
            else if( dwModulo == orq.size() )
            {
                break;
            }
        }
        else
        {
            SetLimitGrps( ocq, dwAvg );
            IncRunningGrps( ocq, dwModulo );
        }

    }while( 0 );

    return STATUS_SUCCESS;
}

gint32 CRRTaskScheduler::DecSlotCount(
    CONNQUE_ELEM& ocq,
    guint32 dwCount,
    std::vector< TaskletPtr >& vecTasks )
{
    if( dwCount == 0 )
        return STATUS_SUCCESS;

    return ResetSlotCountAlloc(
        ocq, dwCount, vecTasks );
}

CRRTaskScheduler::CRRTaskScheduler(
    const IConfigDb* pCfg )
{
    gint32 ret = 0;
    do{
        SetClassId( clsid( CRRTaskScheduler ) );
        CCfgOpener oCfg( pCfg );
        ret = oCfg.GetPointer(
            propIfPtr, m_pSchedMgr );
        if( ERROR( ret ) )
            break;

    }while( 0 );

    if( ERROR( ret ) )
    {
        stdstr strMsg = DebugMsg( ret,
            "Error occurs in CRRTaskScheduler ctor" );
        throw std::runtime_error( strMsg );
    }
}

CRRTaskScheduler::~CRRTaskScheduler()
{

}

CIoManager* CRRTaskScheduler::GetIoMgr()
{
    CRpcServices* pSvc = m_pSchedMgr;
    return pSvc->GetIoMgr();
}

gint32 CRRTaskScheduler::SetSlotCount(
    InterfPtr& pIf,
    guint32 dwNumSlot )
{
    if( pIf.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;
    std::vector< TaskletPtr > vecTasks;

    do{
        CStdRMutex oLock( GetLock() );
        std::map< InterfPtr, CONNQUE_ELEM >::iterator
            itr = m_mapConnQues.find( pIf );
        if( itr == m_mapConnQues.end() )
        {
            ret = -ENOENT;
            break;
        }

        CONNQUE_ELEM& ocq = itr->second;
        if( ocq.m_dwMaxSlots == dwNumSlot )
            break;

        if( ocq.m_dwMaxSlots > dwNumSlot )
        {
            guint32 dwDelta =
                ocq.m_dwMaxSlots - dwNumSlot;
            ret = IncSlotCount( ocq, dwDelta );
            if( SUCCEEDED( ret ) )
                ocq.m_dwMaxSlots = dwNumSlot;
        }
        else if( ocq.m_dwMaxSlots < dwNumSlot )
        {
            guint32 dwDelta =
                dwNumSlot - ocq.m_dwMaxSlots ;
            ret = DecSlotCount(
                ocq, dwDelta, vecTasks );
            if( SUCCEEDED( ret ) )
                ocq.m_dwMaxSlots = dwNumSlot;
        }
        else
        {
            ret = DecSlotCount( ocq, 0, vecTasks );
        }

        oLock.Unlock();

        if( !vecTasks.empty() )
            KillTasks( pIf, vecTasks );

    }while( 0 );

    return ret;
}

gint32 CRRTaskScheduler::RunNextTaskGrp(
    CTasklet* pCurGrp,
    guint32 dwHint )
{
    TaskGrpPtr pGrp;
    gint32 ret = GetNextTaskGrp( pGrp, dwHint );
    if( ERROR( ret ) )
        return 0;

    ( *pGrp )( eventZero );
    return ret;
}

gint32 CRRTaskScheduler::SchedNextTaskGrp(
    CTasklet* pCurGrp,
    guint32 dwHint )
{
    TaskGrpPtr pGrp;
    gint32 ret = GetNextTaskGrp( pGrp, dwHint );
    if( ERROR( ret ) )
        return 0;

    CIoManager* pMgr = GetIoMgr();
    TaskletPtr pTask = pGrp;
    return pMgr->RescheduleTask( pTask );
}

gint32 CRRTaskScheduler::GetNextTaskGrpAAR(
    TaskGrpPtr& pGrp )
{
    gint32 ret = 0;
    int i = 0;

    CStdRMutex oLock( GetLock() );
    for( ; i < m_lstConns.size(); i++ )
    {
        InterfPtr pIf = m_lstConns.front();
        if( m_lstConns.size() > 1 )
        {
            m_lstConns.pop_front();
            m_lstConns.push_back( pIf );
        }

        if( pIf->GetObjId() ==
            m_pSchedMgr->GetObjId() )
            continue;

        auto itr = m_mapConnQues.find( pIf );
        if( itr == m_mapConnQues.end() )
            continue;

        CONNQUE_ELEM& ocq = itr->second;
        std::list< TaskGrpPtr >&
            orq = *ocq.m_plstReady;

        std::list< TaskGrpPtr >&
            owq = ocq.m_lstWaitSlot;

        int j = 0;
        int iCount = orq.size();
        for( ; j < iCount; j++ )
        {
            TaskGrpPtr pHead = orq.front();
            orq.pop_front();

            CIfParallelTaskGrpRfc2* pGrpRfc = pHead;

            orq.push_back( pHead );
            if( !pGrpRfc->HasFreeSlot() )
                continue;

            if( !pGrpRfc->HasTaskToRun() )
                continue;

            pGrp = pHead;
            break;
        }
        if( j < orq.size() )
            break;
    }

    if( i == m_lstConns.size() )
        ret = -ENOENT;

    return ret;
}

gint32 CRRTaskScheduler::GetNextTaskGrpsRMG(
    InterfPtr pIf,
    std::deque< TaskGrpPtr >& queGrps )
{
    gint32 ret = 0;
    int i = 0;

    do{
        if( pIf->GetObjId() ==
            m_pSchedMgr->GetObjId() )
            break;

        auto itr = m_mapConnQues.find( pIf );
        if( itr == m_mapConnQues.end() )
            break;

        CONNQUE_ELEM& ocq = itr->second;
        std::list< TaskGrpPtr >&
            orq = *ocq.m_plstReady;

        std::list< TaskGrpPtr >&
            owq = ocq.m_lstWaitSlot;

        int j = 0;
        int iCount = orq.size();
        for( ; j < iCount; j++ )
        {
            TaskGrpPtr pHead = orq.front();
            orq.pop_front();

            CIfParallelTaskGrpRfc2* pGrpRfc = pHead;
            orq.push_back( pHead );
            bool bNoFree =
                ( !pGrpRfc->HasFreeSlot() );

            if( bNoFree )
                continue;

            if( pGrpRfc->HasTaskToRun() )
                queGrps.push_back( pHead );
        }
        if( queGrps.size() )
            break;

    }while( 0 );

    return ret;
}

/*
 * hint value:
 *   0: the caller is AddAndRun
 *   1: the caller is OnChildComplete
 *   2: the caller is RemoveTaskGrps
 */

gint32 CRRTaskScheduler::GetNextTaskGrp(
    TaskGrpPtr& pGrp, guint32 dwHint )
{
    if( dwHint == 0 )
        return GetNextTaskGrpAAR( pGrp );
    if( dwHint == 1 )
        return GetNextTaskGrpOCC( pGrp );
    return -ENOTSUP; 
}

gint32 CRRTaskScheduler::GetNextTaskGrpOCC(
    TaskGrpPtr& pGrp )
{
    gint32 ret = 0;
    int i = 0;

    CStdRMutex oLock( GetLock() );

    if( m_lstConns.size() >= 32 &&
        ( m_dwNumSched & 0x1f ) == 0 )
    {
        // let the schdmgr's taskgrp every once per 32
        // scheduling.
        CONNQUE_ELEM& ocq = m_ocqSchdMgr;
        auto& orq = *ocq.m_plstReady;
        CIfParallelTaskGrp* pPalGrp = orq.front();
        if( pPalGrp->GetPendingCountLocked() > 0 )
        {
            TaskletPtr pTask = orq.front();
            GetIoMgr()->RescheduleTask( pTask );
        }
    }

    m_dwNumSched++;
    for( ; i < m_lstConns.size(); i++ )
    {
        InterfPtr pIf = m_lstConns.front();
        if( m_lstConns.size() > 1 )
        {
            m_lstConns.pop_front();
            m_lstConns.push_back( pIf );
        }

        if( pIf->GetObjId() ==
            m_pSchedMgr->GetObjId() )
            continue;

        auto itr = m_mapConnQues.find( pIf );
        if( itr == m_mapConnQues.end() )
            continue;

        CONNQUE_ELEM& ocq = itr->second;
        std::list< TaskGrpPtr >&
            orq = *ocq.m_plstReady;

        std::list< TaskGrpPtr >&
            owq = ocq.m_lstWaitSlot;

        int j = 0;
        int iCount = orq.size();
        for( ; j < iCount; j++ )
        {
            TaskGrpPtr pHead = orq.front();
            orq.pop_front();

            CIfParallelTaskGrpRfc2* pGrpRfc = pHead;
            if( owq.empty() )
            {
                orq.push_back( pHead );
                if( !pGrpRfc->HasFreeSlot() )
                    continue;
                if( !pGrpRfc->HasTaskToRun() )
                    continue;
            }
            else
            {
                bool bRunning =
                    ( !pGrpRfc->HasFreeSlot() );

                if( bRunning )
                {
                    orq.push_back( pHead );
                    continue;
                }

                TaskGrpPtr pwh;
                CIfParallelTaskGrpRfc2* pGrpRfc2 = nullptr;
                gint32 iWaitCount = owq.size(), k = 0;
                for( ; k < iWaitCount; k++ )
                {
                    pwh = owq.front();
                    owq.pop_front();
                    pGrpRfc2 = pwh;
                    if( pGrpRfc2->HasPendingTasks() )
                        break;
                    owq.push_back( pwh );
                }

                if( k < iWaitCount )
                {
                    pGrpRfc->SetLimit( 0,
                        pGrpRfc->GetMaxPending(),
                        true );

                    pGrpRfc2->SetLimit( 1,
                        pGrpRfc2->GetMaxPending(),
                        true );

                    orq.push_back( pwh );
                    owq.push_back( pHead );
                    pHead = pwh;
                }
                else
                {
                    orq.push_back( pHead );
                    if( !pGrpRfc->HasTaskToRun() )
                        continue;
                }
            }

            pGrp = pHead;
            break;
        }
        if( j < orq.size() )
            break;
    }

    if( i == m_lstConns.size() )
        ret = -ENOENT;

    return ret;
}

void CRRTaskScheduler::ReportCounters(
    TaskGrpPtr pGrp )
{
    CIfParallelTaskGrpRfc* pGrpRfc = pGrp;
    if( pGrpRfc == nullptr )
        return;

    auto psc = dynamic_cast
        < CStatCountersServer* >( m_pSchedMgr );
    if( psc == nullptr )
        return;

    psc->IncCounter( propTaskRejected,
        pGrpRfc->GetTaskRejected() );

    psc->IncCounter( propTaskAdded,
        pGrpRfc->GetTaskAdded() );
    return;
}

void CRRTaskScheduler::ReportCounters(
    std::vector< TaskGrpPtr >& vecGrps )
{
    auto psc = dynamic_cast
        < CStatCountersServer* >( m_pSchedMgr );
    if( psc == nullptr )
        return;

    for( auto& pGrp : vecGrps )
    {
        CIfParallelTaskGrpRfc* pGrpRfc = pGrp;
        if( pGrpRfc == nullptr )
            continue;

        psc->IncCounter( propTaskRejected,
            pGrpRfc->GetTaskRejected() );

        psc->IncCounter( propTaskAdded,
            pGrpRfc->GetTaskAdded() );
    }
    return;
}

void CRRTaskScheduler::EnumTaskGrps(
    InterfPtr pIf,
    std::vector< TaskGrpPtr >& vecGrps )
{
    gint32 ret = 0;
    do{
        if( pIf.IsEmpty() )
            break;
        CStdRMutex oLock( GetLock() );
        auto itrm = m_mapConnQues.find( pIf );
        if( itrm == m_mapConnQues.end() )
            break;
        auto& ocq = itrm->second;
        ocq.EnumTaskGrps( vecGrps );

    }while( 0 );

    return;
}

void CRRTaskScheduler::EnumIfs(
    std::vector< InterfPtr >& vecIfs )
{
    do{
        CStdRMutex oLock( GetLock() );
        for( auto& elem : m_lstConns )
        {
            if( elem->GetObjId() ==
                m_pSchedMgr->GetObjId() )
                continue;
            vecIfs.push_back( elem );
        }

    }while( 0 );

    return;
}

void CRRTaskScheduler::ReportCounters(
    guint32 dwPortId )
{
    do{
        InterfPtr pIf;
        CStdRMutex oLock( GetLock() );
        auto itr2 = m_mapId2If.find( dwPortId );
        if( itr2 == m_mapId2If.end() )
            break;
        pIf = itr2->second;
        std::vector< TaskGrpPtr > vecGrps;
        EnumTaskGrps( pIf, vecGrps );
        oLock.Unlock();

        ReportCounters( vecGrps );

    }while( 0 );

    return;
}

gint32 CRRTaskScheduler::RemoveTaskGrp(
    TaskGrpPtr& pGrp )
{
    gint32 ret = 0;
    bool bFreeSlot = false;
    do{
        guint32 dwPortId = 0;

        CCfgOpenerObj oGrpCfg(
            ( CObjBase* )pGrp );

        ret = oGrpCfg.GetIntProp(
            propPrxyPortId, dwPortId );
        if( ERROR( ret ) )
            break;

        InterfPtr pIf;
        CStdRMutex oLock( GetLock() );
        std::hashmap< guint32, InterfPtr >::iterator
            itr2 = m_mapId2If.find( dwPortId );

        if( itr2 == m_mapId2If.end() )
        {
            ret = -ENOENT;
            break;
        }

        pIf = itr2->second;
        auto& ocq = m_mapConnQues[ pIf ];
        auto itr = std::find(
                ocq.m_lstWaitSlot.begin(),
                ocq.m_lstWaitSlot.end(),
                pGrp );

        if( itr == ocq.m_lstWaitSlot.end() )
        {
            itr = std::find(
                ocq.m_plstReady->begin(),
                ocq.m_plstReady->end(),
                pGrp );

            if( itr == ocq.m_plstReady->end() )
            {
                ret = -ENOENT;
                break;
            }
            ocq.m_plstReady->erase( itr );
            bFreeSlot = true;
        }
        else
        {
            ocq.m_lstWaitSlot.erase( itr );
            break;
        }

        guint32 dwCount = ocq.m_dwMaxSlots;
        guint32 dwNumGrps = ocq.GetTaskGrpCount();
        if( dwNumGrps == 0 )
        {
            ret = 0;
            break;
        }
        guint32 dwAvg = dwCount / dwNumGrps;
        guint32 dwModulo =
            dwCount - dwAvg * dwNumGrps;

        if( dwAvg == 0 )
        {
            if( ocq.m_lstWaitSlot.empty() )
                break;
            IncWaitingGrp( ocq, 1 );
        }
        else
        {
            SetLimitRunningGrps( ocq, dwAvg );
            IncRunningGrps( ocq, dwModulo );
        }

        if( !bFreeSlot )
            break;

        std::deque< TaskGrpPtr > queGrps;
        ret = GetNextTaskGrpsRMG( pIf, queGrps );
        if( ERROR( ret ) || queGrps.empty() )
            break;

        CIoManager* pMgr = GetIoMgr();
        for( auto& elem : queGrps )
        {
            TaskletPtr pTask = elem;
            pMgr->RescheduleTask( pTask );
        }

    }while( 0 );

    ReportCounters( pGrp );
    return 0;
}

gint32 CRRTaskScheduler::RemoveTaskGrps(
    guint32 dwPortId )
{
    gint32 ret = 0;
    do{
        ReportCounters( dwPortId );

        CStdRMutex oLock( GetLock() );
        InterfPtr pIf;
        std::hashmap< guint32, InterfPtr >::iterator
            itr2 = m_mapId2If.find( dwPortId );
        if( itr2 == m_mapId2If.end() )
        {
            ret = -ENOENT;
            break;
        }
        pIf = itr2->second;
        RemoveConn( pIf );

    }while( 0 );

    return ret;
}

gint32 CRRTaskScheduler::RemoveTaskGrps(
    std::vector< TaskGrpPtr >& vecGrps )
{
    gint32 ret = 0;
    do{
        CStdRMutex oLock( GetLock() );

        std::set< InterfPtr > setIfs;
        for( auto pGrp : vecGrps )
        {
            CCfgOpenerObj oGrpCfg( ( CObjBase* )pGrp );
            guint32 dwPortId = 0;
            ret = oGrpCfg.GetIntProp(
                propPrxyPortId, dwPortId );
            if( ERROR( ret ) )
                continue;

            InterfPtr pIf;
            std::hashmap< guint32, InterfPtr >::iterator
                itr2 = m_mapId2If.find( dwPortId );
            if( itr2 == m_mapId2If.end() )
                continue;

            pIf = itr2->second;
            CONNQUE_ELEM& ocq = m_mapConnQues[ pIf ];

            std::list< TaskGrpPtr >& orq =
                *ocq.m_plstReady;

            std::list< TaskGrpPtr >& owq =
                ocq.m_lstWaitSlot;

            std::list< TaskGrpPtr >::iterator
                itr = std::find(
                    owq.begin(), owq.end(), pGrp );

            if( itr == owq.end() )
            {
                itr = std::find(
                    orq.begin(), orq.end(), pGrp );

                if( itr == orq.end() )
                    continue;
                orq.erase( itr );
                setIfs.insert( pIf );
            }
            else
            {
                owq.erase( itr );
            }
        }

        CIoManager* pMgr = GetIoMgr();
        for( auto& elem : setIfs )
        {
            auto itr = m_mapConnQues.find( elem );
            if( itr == m_mapConnQues.end() )
                continue;

            CONNQUE_ELEM& ocq = itr->second;
            if( ocq.m_dwMaxSlots == 0 )
                continue;

            ResetSlotCountRelease( ocq, 0 );
            std::deque< TaskGrpPtr > queGrps;
            ret = GetNextTaskGrpsRMG( elem, queGrps );
            if( ERROR( ret ) || queGrps.empty() )
                continue;

            for( auto& elem : queGrps )
            {
                TaskletPtr pTask = elem;
                pMgr->RescheduleTask( pTask );
            }
        }

    }while( 0 );

    ReportCounters( vecGrps );
    return 0;
}

gint32 CRRTaskScheduler::AddTaskGrp(
    InterfPtr& pIf,
    TaskGrpPtr& pGrp )
{
    gint32 ret = 0;
    do{
        if( pIf->GetObjId() ==
            m_pSchedMgr->GetObjId() )
        {
            return 0;
        }
        std::vector< TaskletPtr > vecTasks;
        CStdRMutex oLock( GetLock() );
        std::map< InterfPtr, CONNQUE_ELEM >::iterator
            itr = m_mapConnQues.find( pIf );
        if( itr == m_mapConnQues.end() )
        {
            oLock.Unlock();
            ret = AddConn( pIf );
            if( ERROR( ret ) )
                break;
            continue;
        }

        CONNQUE_ELEM& ocq = itr->second;
        if( ocq.m_lstWaitSlot.size() > 0 )
        {
            ocq.m_lstWaitSlot.push_back( pGrp );
            break;
        }
        guint32 dwCount = ocq.m_dwMaxSlots;
        guint32 dwNumGrps = ocq.GetTaskGrpCount() + 1;
        guint32 dwAvg = dwCount / dwNumGrps;
        guint32 dwModulo =
            dwCount - dwAvg * dwNumGrps;

        if( dwAvg == 0 )
        {
            ocq.m_lstWaitSlot.push_back( pGrp );
            break;
        }
        else
        {
            ocq.m_plstReady->push_back( pGrp );
            SetLimitRunningGrps( ocq, dwAvg );
            IncRunningGrps( ocq, dwModulo );
            SelTasksToKill( ocq, vecTasks, false );
        }
        oLock.Unlock();

        if( !vecTasks.empty() )
            KillTasks(pIf, vecTasks );

        break;

    }while( 1 );

    return ret;
}

gint32 CRRTaskScheduler::AddConn(
    InterfPtr& pIf )
{
    guint32 dwPortId = 0;
    CCfgOpenerObj oIfCfg( ( CObjBase* )pIf );
    gint32 ret = oIfCfg.GetIntProp(
        propPortId, dwPortId );
    if( ERROR( ret ) )
        return ret;

    do{
        TaskGrpPtr pGrp;
        CRpcTcpBridgeProxy* pProxy = pIf;
        ret = pProxy->GetGrpRfc( pGrp );
        if( ERROR( ret ) )
            break;

        CIfParallelTaskGrpRfc* pGrpRfc = pGrp;
        guint32 dwMaxRunning = 0;
        guint32 dwMaxPending = 0;
        ret = pGrpRfc->GetLimit(
            dwMaxRunning, dwMaxPending );
        if( ERROR( ret ) )
            break;

        CStdRMutex oLock( GetLock() );
        if( m_mapId2If.find( dwPortId ) !=
            m_mapId2If.end() )
        {
            ret = -EEXIST;
            break;
        }

        m_lstConns.push_back( pIf );
        CONNQUE_ELEM& ocq = m_mapConnQues[ pIf ];
        m_mapId2If[ dwPortId ] = ObjPtr( pIf );

        ret = SetSlotCount( pIf,
            dwMaxRunning + dwMaxPending );

    }while( 0 );

    return ret;
}

gint32 CRRTaskScheduler::RemoveConn(
    InterfPtr& pIf )
{
    if( pIf.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;
    do{
        gint32 ret = 0;
        guint32 dwPortId;

        CCfgOpenerObj oIfCfg( ( CObjBase* )pIf );
        ret = oIfCfg.GetIntProp(
            propPortId, dwPortId );
        if( ERROR( ret ) )
            dwPortId = 0xffffffff;

        CStdRMutex oLock( GetLock() );
        std::list< InterfPtr >::iterator itr =
            std::find( m_lstConns.begin(),
                m_lstConns.end(), pIf );
        if( itr == m_lstConns.end() )
        {
            ret = -ENOENT;
            break;
        }
        m_lstConns.erase( itr );
        std::map< InterfPtr, CONNQUE_ELEM>::iterator
            itrm = m_mapConnQues.find( pIf );
        if( itrm == m_mapConnQues.end() )
        {
            ret = -ENOENT;
            break;
        }
        m_mapConnQues.erase( itrm );
        m_mapId2If.erase( dwPortId );
        oLock.Unlock();

    }while( 0 );

    return ret;
}

void CRRTaskScheduler::KillTasks(
    InterfPtr& pIf,
    std::vector< TaskletPtr >& vecTasks )
{
    if( vecTasks.empty() )
        return;

    gint32 ret = 0;
    QwVecPtr pvecTaskIds( true );

    std::vector< guint64 >&
        vecTaskIds = ( *pvecTaskIds )();

    auto pReqFwdr = dynamic_cast
        < CRpcReqForwarder* >( m_pSchedMgr );

    if( pReqFwdr != nullptr )
    {
        for( auto elem : vecTasks )
        {
            guint64 qwTaskId = 0;
            ret = pReqFwdr->RetrieveTaskId(
                elem, qwTaskId );
            if( ERROR( ret ) )
                continue;
            vecTaskIds.push_back( qwTaskId );
        }
    }

    for( auto elem : vecTasks )
    {
        CIfRetryTask* pTask = elem;
        if( pTask == nullptr )
            continue;

        TaskletPtr pIoTask =
            pTask->GetEndFwrdTask();

        pIoTask->OnEvent( eventTaskComp,
            ERROR_KILLED_BYSCHED,
            0, nullptr );
    }

    if( vecTaskIds.size() > 0 )
    {
        TaskletPtr pClear;
        ObjPtr pEmptyObj;
        ObjPtr pTaskIds = pvecTaskIds;
        ret = DEFER_IFCALLEX_NOSCHED2(
            2, pClear, pIf,
            &CRpcTcpBridgeProxy::ClearRemoteEvents,
            pEmptyObj, pTaskIds, nullptr );
        if( SUCCEEDED( ret ) )
        {
            CIoManager* pMgr = pReqFwdr->GetIoMgr();
            pMgr->RescheduleTask( pClear );
        }
    }

    return;
}

gint32 CRRTaskScheduler::Start()
{
    CRpcServices* pSvc = m_pSchedMgr;
    if( pSvc == nullptr )
        return -EFAULT;
    TaskGrpPtr pGrp;
    gint32 ret = pSvc->GetParallelGrp( pGrp );
    if( ERROR( ret ) )
        return ret;

    InterfPtr pIf = m_pSchedMgr;
    CONNQUE_ELEM& ocq = m_ocqSchdMgr;
    ocq.m_dwMaxSlots = 1000;
    ocq.m_plstReady->push_back( pGrp );
    m_lstConns.push_back( pIf );
    return 0;
}

gint32 CRRTaskScheduler::Stop()
{
    InterfPtr pIf = m_pSchedMgr;
    return RemoveConn( pIf );
}

}
