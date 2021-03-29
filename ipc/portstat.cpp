/*
 * =====================================================================================
 *
 *       Filename:  portstat.cpp
 *
 *    Description:  implementation of the CPortState
 *
 *        Version:  1.0
 *        Created:  12/03/2016 11:29:14 AM
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

#include <vector>
#include <string>
//NOTE: c++11 required
#include <stdexcept>
#include "defines.h"
#include "port.h"

namespace rpcf
{

using namespace std;
// refer to portstat.xlsx
const gint8  CPortState::m_arrStatMap[ PORTSTAT_MAP_LEN ][ PORTSTAT_MAP_LEN ] =
{
    // 1: on submit
    // 2: depending on context
    // 3: poped from stack
    // 4: switched directly
    //
    // -1: an error code other than ERROR_STATE
    // will be returned
    //
    // row: current state
    // col: target state
    //
    //    0  1  2   3  4  5  6  7  8  9  10
    // 0: READY
        { 1, 0, 0,  1, 0, 0, 1, 1, 0, 0, 0, },
    // 1: ATTACHED                        
        { 0, 2, 1,  0, 0, 0, 1, 0, 0, 0, 0, },
    // 2: STARTING                        
        { 4, 3, 2,  0, 2, 0, 0, 0, 0, 0, 0, },
    // 3: STOPPING                        
        { 0, 0, 0,  2, 4, 0, 0, 0, 0, 0, 0, },
    // 4: STOPPED                         
        { 0, 1, 0,  0, 2, 1, 1, 0, 0, 0, 0, },
    // 5: REMOVED                         
        { 0, 0, 0,  0, 0, 2, 0, 0, 0, 0, 0, },
    // 6: BUSY                            
        { 3, 3, 0,  0, 3, 0, 2, 0, 0, 0, 0, },
    // 7: BUSY SHARED                     
        { 3, 0, 0, -1, 0, 0, 0, 1, 1, 0, 0, },
    // 8: EXECLUDE_PENDING                
        { 0, 0, 0,  0, 0, 0, 0, 0, 0, 4, 4, },
    // 9: STOP_RESUME                     
        { 0, 0, 0,  1, 0, 0, 0, 0, 0, 0, 0, },
    // 10: BUSY_RESUME                     
        { 0, 0, 0,  0, 0, 0, 1, 0, 0, 4, 0, },
};
#define MAX_PORT_STATE 10

CPortState::CPortState(
    const IConfigDb* pCfg )
{
    SetClassId( clsid( CPortState ) );

    Sem_Init( &m_semWaitForReady, 0, 0 );

    m_vecStates.push_back(
        PORT_STATE_ATTACHED );

    CCfgOpener a( pCfg );
    ObjPtr pObj;

    gint32 ret = a.GetObjPtr(
        propPortPtr, pObj );

    if( ERROR( ret ) )
        throw invalid_argument(
            "pointer to the parent port does not exist" );

    m_pPort = pObj;
    if( m_pPort == nullptr )
        throw invalid_argument(
            "pointer to the parent port is null" );
}

CPortState::~CPortState()
{
    SEM_WAKEUP_ALL( m_semWaitForReady );
    sem_destroy( &m_semWaitForReady );
}

guint32 CPortState::GetPortState() const
{
    CStdRMutex a( GetLock() );

    if( m_vecStates.empty() )
        return PORT_STATE_INVALID;

    return m_vecStates.back();
}

bool CPortState::IsValidSubmit(
    guint32 dwNewState ) const
{
    if( dwNewState > MAX_PORT_STATE )
        return -EINVAL;

    guint32 dwCurState = m_vecStates.back();

    const gint8* pMapActions =
        m_arrStatMap[ dwCurState ];

    gint8 bAction =
        pMapActions[ dwNewState ];

    // prohibitted transition
    if( bAction == 1 )
        return true;

    if( bAction == -1 )
        return true;

    return false;
}

stdrmutex& CPortState::GetLock() const
{
    return ( stdrmutex& )m_oLock;
}

gint32 CPortState::CanContinue(
    IRP* pIrp,
    guint32 dwNewState,
    guint32* pdwOldState )
{
    gint32 ret = 0;
    if( dwNewState > PORTSTAT_MAP_LEN )
        return -EINVAL;

    do{
        CStdRMutex a( GetLock() );
        if( m_vecStates.empty() )
            return ERROR_STATE;

        guint32 dwCurState = m_vecStates.back();
        if( pdwOldState != nullptr )
            *pdwOldState = dwCurState;

        if( !IsValidSubmit( dwNewState ) )
            return ERROR_STATE;

        switch( dwCurState ) 
        {
        case PORT_STATE_READY:
            {
                ret = HandleReady(
                    pIrp, dwNewState, pdwOldState );
                break;
            }
        case PORT_STATE_ATTACHED:
            {
                ret = HandleAttached(
                    pIrp, dwNewState, pdwOldState );
                break;
            }
        case PORT_STATE_STARTING:
            {
                ret = HandleStarting(
                    pIrp, dwNewState, pdwOldState );
                break;
            }
        case PORT_STATE_STOPPING:
            {
                ret = HandleStopping(
                    pIrp, dwNewState, pdwOldState );
                break;
            }
        case PORT_STATE_STOPPED:
            {
                ret = HandleStopped(
                    pIrp, dwNewState, pdwOldState );
                break;
            }
        case PORT_STATE_REMOVED:
            {
                ret = HandleRemoved(
                    pIrp, dwNewState, pdwOldState );
                break;
            }
        case PORT_STATE_BUSY:
            {
                ret = HandleBusy(
                    pIrp, dwNewState, pdwOldState );
                break;
            }
        case PORT_STATE_BUSY_SHARED:
            {
                ret = HandleBusyShared(
                    pIrp, dwNewState, pdwOldState );
                break;
            }
        case PORT_STATE_EXCLUDE_PENDING:
            {
                ret = HandleExclPending(
                    pIrp, dwNewState, pdwOldState );
                break;
            }
        case PORT_STATE_STOP_RESUME:
            {
                ret = HandleStopResume(
                    pIrp, dwNewState, pdwOldState );
                break;
            }
        case PORT_STATE_BUSY_RESUME:
            {
                ret = HandleBusyResume(
                    pIrp, dwNewState, pdwOldState );
                break;
            }
        default:
            {
                ret = ERROR_STATE;        
                break;
            }
        }

        if( ret == ERROR_REPEAT )
            continue;

        if( SUCCEEDED( ret ) )
        {
            if( pdwOldState != nullptr )
                *pdwOldState = dwCurState;
        }

        break;
    }while( 1 );

    if( ERROR( ret ) )
    {
        DebugPrint( ret,
            "Failed to change port state" );
    }
    
    return ret;
}
/**
* @name SetResumePoint
* @{ set the resume function to call previously 
* suspended. The method is called when the irp is
* able to enter this port, when all the busy_shared
* requests have gone and the port is about to return
* to ready state*/
/** pFunc: the function to call.
 * To be a qualified resumable function, the function
 * must contains a CPort::Continue or its override to
 * recheck if it can pass. It should also not assume
 * that it is running on a specific thread.
 *
 * The usage is to call this method whenever the
 * CanContinue return's STATUS_PENDING
 *
 * @} */

void CPortState::SetResumePoint(
    TaskletPtr& pFunc )
{
    CStdRMutex a( GetLock() );
    m_pResume = pFunc;
}

gint32 CPortState::SetPortState(
    guint32 dwNewState )
{
    if( dwNewState > MAX_PORT_STATE )
        return -EINVAL;

    CStdRMutex a( GetLock() );

    if( m_vecStates.empty() )
    {
        m_vecStates.push_back( dwNewState );
        return 0;
    }

    const gint8* pMapActions =
        m_arrStatMap[ m_vecStates.back() ];

    gint8 bAction =
        pMapActions[ dwNewState ];

    // NA transition are not allowed
    if( bAction == 0 )
        return ERROR_STATE;

    if( dwNewState != PORT_STATE_STOP_RESUME )
    {
        m_vecStates.back() = ( guint8 )dwNewState;
    }
    else
    {
        guint32 dwCurState = m_vecStates.back();
        if( dwCurState != PORT_STATE_BUSY_RESUME )
            return ERROR_STATE;

        m_vecStates.pop_back();
        if( m_vecStates.back() != PORT_STATE_READY )
            return ERROR_STATE;
        m_vecStates.back() = dwNewState;
    }

    return 0;
}

// return the poped state
gint32 CPortState::PopState( guint32& dwState )
{
    gint32 ret = 0;

    do{
        CStdRMutex oStatLock( GetLock() );
        if( m_vecStates.empty() )
            return ERROR_STATE;

        dwState = m_vecStates.back();
        m_vecStates.pop_back();

        if( PORT_STATE_BUSY == dwState )
        {
            SEM_WAKEUP_SINGLE( m_semWaitForReady );
            break;
        }

        if( PORT_STATE_EXCLUDE_PENDING != dwState )
            break;

        do{
            if( m_vecStates.size() <= 1 )
            {
                // there should be at least two
                // states in the stack
                ret = ERROR_STATE;
                break;
            }

            // check the second top to see if it
            // is the bottom of the stack
            dwState = m_vecStates.back();
            if( dwState != PORT_STATE_BUSY_SHARED )
            {
                // the state machine gone wild
                ret = ERROR_STATE;
                break;
            }
            m_vecStates.pop_back();

            if( m_vecStates.back() == 
                PORT_STATE_BUSY_SHARED )
            {
                // continue to wait
                m_vecStates.push_back(
                    PORT_STATE_EXCLUDE_PENDING );
                ret = 0;
                break;
            }

            // we are at the bottom of the stack
            // all the on-going busy_shared requests
            // are gone. let's enter the resume
            // state, and schedule the resume task
            if( m_pExclIrp.IsEmpty() ||
                m_pExclIrp->GetStackSize() == 0 ||
                m_pResume.IsEmpty()  )
            {
                // there should be one irp pending
                // otherwise, something wrong

                throw std::runtime_error( 
                    "Cannot find irp or the resume addr" );

                break;
            }

            TaskletPtr pTask = m_pResume;
            m_pResume.Clear();

            IrpPtr pIrp = m_pExclIrp;

            // whether or not STOP_RESUME or
            // BUSY_RESUME, we will determine it
            // later. here set to BUSY RESUME to
            // block any other further requests
            m_vecStates.push_back( 
                PORT_STATE_EXCLUDE_PENDING );
            ret = SetPortState(
                PORT_STATE_BUSY_RESUME );
            if( ERROR( ret ) )
                break;

            oStatLock.Unlock();

            ret = ScheduleResumeTask( pTask, pIrp );

        }while( 0 );

        if( ERROR( ret ) )
        {
            // this port won't respond to any
            // request any more.
        }

    }while( 0 );

    return ret;
}

bool CPortState::SetPortStateWake(
    guint32 dwCurState,
    guint32 dwNewState,
    bool bWakeAll )
{
    CStdRMutex a( GetLock() );
    if( m_vecStates.empty() )
        return false;

    if( dwCurState != m_vecStates.back() )
        return false;

    gint32 ret = SetPortState( dwNewState );
    if( ERROR( ret ) )
        return false;

    if( bWakeAll ) 
    {
        SEM_WAKEUP_ALL( m_semWaitForReady );
    }
    else
    {
        SEM_WAKEUP_SINGLE( m_semWaitForReady );
    }
    return true;
}

bool CPortState::IsPortReady()
{
    CStdRMutex a( GetLock() );

    if( m_vecStates.empty() )
        return false;

    return ( m_vecStates.back()
        == PORT_STATE_READY );
}

guint32 CPortState::PushState(
    guint32 dwState, bool bCheckLimit )
{
    if( dwState > PORTSTAT_MAP_LEN )
        return -EINVAL;

    CPort* pPort =
        static_cast< CPort* >( m_pPort );

    CIoManager* pMgr = pPort->GetIoMgr();
    if( pMgr == nullptr )
        return -EFAULT;

    CStdRMutex a( GetLock() );

    guint32 dwSize = m_vecStates.size();
    guint32 dwNumCores = pMgr->GetNumCores();

    if( bCheckLimit &&
        dwSize > dwNumCores  + 1 )
    {
        // reach the limit
        return -EAGAIN;
    }

    m_vecStates.push_back( ( guint8 ) dwState );

    return 0;
}

gint32 CPortState::HandleReady(
    IRP* pIrp,
    guint32 dwNewState,
    guint32* pdwOldState )
{
    if( pIrp == nullptr 
        || pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = ERROR_STATE;

    guint32 dwMajorCmd =
        pIrp->GetTopStack()->GetMajorCmd();

    guint32 dwMinorCmd =
        pIrp->GetTopStack()->GetMinorCmd();

    switch( dwMajorCmd )
    {
    case IRP_MJ_PNP:
        {

            if( dwMinorCmd == IRP_MN_PNP_QUERY_STOP
                && dwNewState == PORT_STATE_BUSY_SHARED ) 
            {
                ret = PushState( dwNewState, false );
            }
            else if( dwMinorCmd == IRP_MN_PNP_STACK_READY
                && dwNewState == PORT_STATE_BUSY ) 
            {
                ret = PushState( dwNewState, false );
            }

            /*else if( dwMinorCmd == IRP_MN_PNP_STACK_DESTROY
                && dwNewState == PORT_STATE_BUSY ) 
            {
                ret = PushState( dwNewState, false );
            }*/

            else if( dwMinorCmd == IRP_MN_PNP_STOP_CHILD
                && dwNewState == PORT_STATE_BUSY_SHARED ) 
            {
                ret = PushState( dwNewState, false );
            }

            else if( dwMinorCmd == IRP_MN_PNP_STOP 
                && dwNewState == PORT_STATE_STOPPING )
            {
                ret = SetPortState( dwNewState );
            }

            break;
           
        }
    case IRP_MJ_FUNC:
        {
            if( dwNewState == PORT_STATE_BUSY_SHARED
                || dwNewState == PORT_STATE_BUSY )
            {
                ret = PushState( dwNewState );
            }
            break;
        }
    default:
        {
            break;
        }
    }

    return ret;
}

gint32 CPortState::HandleAttached(
    IRP* pIrp,
    guint32 dwNewState,
    guint32* pdwOldState )
{
    gint32 ret = 0;
    if( pIrp == nullptr 
        || pIrp->GetStackSize() == 0 )
        return -EINVAL;

    guint32 dwMajorCmd =
        pIrp->GetTopStack()->GetMajorCmd();

    guint32 dwMinorCmd =
        pIrp->GetTopStack()->GetMinorCmd();


    switch( dwMajorCmd )
    {
    case IRP_MJ_PNP:
        {
            if( dwMinorCmd == IRP_MN_PNP_START
                && dwNewState == PORT_STATE_STARTING )
            {
                ret = SetPortState( dwNewState );
                break;
            }

            else if( dwMinorCmd == IRP_MN_PNP_STACK_BUILT
                && dwNewState == PORT_STATE_BUSY )
            {
                ret = PushState( dwNewState );
                break;
            }

            else if( dwMinorCmd == IRP_MN_PNP_STACK_DESTROY
                && dwNewState == PORT_STATE_BUSY ) 
            {
                ret = PushState( dwNewState, false );
                break;
            }
        }
    default:
        {
            ret = ERROR_STATE;
            break;
        }
    }

    return ret;
}

gint32 CPortState::HandleStarting(
    IRP* pIrp,
    guint32 dwNewState,
    guint32* pdwOldState )
{
    if( pIrp == nullptr 
        || pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = ERROR_STATE;

    guint32 dwMajorCmd =
        pIrp->GetTopStack()->GetMajorCmd();

    guint32 dwMinorCmd =
        pIrp->GetTopStack()->GetMinorCmd();


    switch( dwMajorCmd )
    {
    case IRP_MJ_PNP:
        {
            // for compatibility with old CanContinue
            if( dwMinorCmd == IRP_MN_PNP_STOP
                && dwNewState == PORT_STATE_STOPPING )
            {
                ret = -EAGAIN;
            }
            break;
        }
    default:
        {
            ret = ERROR_STATE;
            break;
        }
    }

    return ret;
}

gint32 CPortState::HandleStopping(
    IRP* pIrp,
    guint32 dwNewState,
    guint32* pdwOldState )
{
    if( pIrp == nullptr 
        || pIrp->GetStackSize() == 0 )
        return -EINVAL;

    return ERROR_STATE;
}

gint32 CPortState::HandleStopped(
    IRP* pIrp,
    guint32 dwNewState,
    guint32* pdwOldState )
{
    gint32 ret = 0;
    if( pIrp == nullptr 
        || pIrp->GetStackSize() == 0 )
        return -EINVAL;

    guint32 dwMajorCmd =
        pIrp->GetTopStack()->GetMajorCmd();

    guint32 dwMinorCmd =
        pIrp->GetTopStack()->GetMinorCmd();

    switch( dwMajorCmd )
    {
    case IRP_MJ_PNP:
        {
            if( dwMinorCmd == IRP_MN_PNP_REATTACH
                && dwNewState == PORT_STATE_ATTACHED )
            {
                ret = SetPortState( dwNewState );
                break;
            }

            if( dwMinorCmd == IRP_MN_PNP_STACK_DESTROY
                && dwNewState == PORT_STATE_BUSY )
            {
                ret = PushState( dwNewState );
                break;
            }
            // fall through
        }
    default:
        {
            ret = ERROR_STATE;
            break;
        }
    }

    return ret;
}

gint32 CPortState::HandleRemoved(
    IRP* pIrp,
    guint32 dwNewState,
    guint32* pdwOldState )
{
    if( pIrp == nullptr 
        || pIrp->GetStackSize() == 0 )
        return -EINVAL;

    return ERROR_STATE;
}

gint32 CPortState::HandleBusy(
    IRP* pIrp,
    guint32 dwNewState,
    guint32* pdwOldState )
{
    gint32 ret = 0;
    if( pIrp == nullptr 
        || pIrp->GetStackSize() == 0 )
        return -EINVAL;

    ret = sem_wait( &m_semWaitForReady );

    if( ret == 0 )
    {
        // indicate the CanContinue to
        // test again
        ret = ERROR_REPEAT;
    }
    else
    {
        ret = -errno;
    }

    return ret;
}

gint32 CPortState::HandleBusyShared(
    IRP* pIrp,
    guint32 dwNewState,
    guint32* pdwOldState )
{
    gint32 ret = ERROR_STATE;

    if( pIrp == nullptr 
        || pIrp->GetStackSize() == 0 )
        return -EINVAL;

    guint32 dwMajorCmd =
        pIrp->GetTopStack()->GetMajorCmd();

    guint32 dwMinorCmd =
        pIrp->GetTopStack()->GetMinorCmd();


    switch( dwMajorCmd )
    {
    case IRP_MJ_PNP:
        {
            if( dwMinorCmd == IRP_MN_PNP_QUERY_STOP
                && dwNewState == PORT_STATE_BUSY_SHARED )
            {
                ret = PushState( dwNewState, false );

                if( SUCCEEDED( ret ) )
                    break;
            }
            else if( dwMinorCmd == IRP_MN_PNP_STOP_CHILD
                && dwNewState == PORT_STATE_BUSY_SHARED )
            {
                ret = PushState( dwNewState, false );

                if( SUCCEEDED( ret ) )
                    break;
            }

            if( dwMinorCmd == IRP_MN_PNP_STOP )
            {
                // NOTE: the dwNewState is ignored
                // which will be set later
                ret = PushState(
                    PORT_STATE_EXCLUDE_PENDING );

                if( SUCCEEDED( ret ) )
                {
                    m_pExclIrp = pIrp;
                    ret = STATUS_PENDING;
                }
                break;
            }
            break;
        }

    case IRP_MJ_FUNC:
        {
            if( dwNewState == PORT_STATE_BUSY )
            {
                ret = PushState(
                    PORT_STATE_EXCLUDE_PENDING );

                if( SUCCEEDED( ret ) )
                {
                    m_pExclIrp = pIrp;
                    ret = STATUS_PENDING;
                }
                break;
            }

            if( dwNewState == PORT_STATE_BUSY_SHARED )
            {
                ret = PushState(
                    PORT_STATE_BUSY_SHARED );
                break;
            }

            break;
        }
    default:
        {
            ret = -EAGAIN;
            break;
        }
    }

    return ret;
}

gint32 CPortState::HandleExclPending(
    IRP* pIrp,
    guint32 dwNewState,
    guint32* pdwOldState )
{
    // no more IRPs till all the irps 
    // in process are done
    if( pIrp == nullptr 
        || pIrp->GetStackSize() == 0 )
        return -EINVAL;

    return ERROR_STATE;
}

gint32 CPortState::HandleStopResume(
    IRP* pIrp,
    guint32 dwNewState,
    guint32* pdwOldState )
{
    if( dwNewState != PORT_STATE_STOPPING )
        return ERROR_STATE;

    return SetPortState( dwNewState );
}

gint32 CPortState::HandleBusyResume(
    IRP* pIrp,
    guint32 dwNewState,
    guint32* pdwOldState )
{
    if( dwNewState != PORT_STATE_BUSY )
        return ERROR_STATE;

    return SetPortState( dwNewState );
}

gint32 CPortStateResumeSubmitTask::operator()(
    guint32 dwContext )
{
    gint32 ret = 0;
    do{
        CCfgOpener oParams( ( IConfigDb* )m_pCtx );

        CPort* pPort = nullptr;
        ret = oParams.GetPointer( propPortPtr, pPort );
        if( ERROR( ret ) )
            break;

        CTasklet* pTask = nullptr;
        ret = oParams.GetPointer( propFuncAddr, pTask );
        if( ERROR( ret ) )
            break;

        PIRP pIrp = nullptr;
        ret = oParams.GetPointer( propIrpPtr, pIrp );
        if( ERROR( ret ) )
            break;

        if( true )
        {
            CStdRMutex oIrpLock( pIrp->GetLock() );
            ret = pIrp->CanContinue( IRP_STATE_READY );
            if( ERROR( ret ) )
                break;

            if( pIrp->GetStackSize() == 0 )
            {
                ret = -EFAULT;
                break;
            }
            CStdRMutex oPortLock( pPort->GetLock() );
            IrpCtxPtr pIrpCtx = pIrp->GetTopStack();
            guint32 dwMajorCmd = pIrpCtx->GetMajorCmd();
            guint32 dwMinorCmd = pIrpCtx->GetMinorCmd();

            if( dwMajorCmd == IRP_MJ_PNP
                && dwMinorCmd == IRP_MN_PNP_STOP )
            {
                // change the port state from
                // BUSY_RESUME to STOP_RESUME
                ret = pPort->SetPortState(
                    PORT_STATE_STOP_RESUME );

                if( ERROR( ret ) )
                    break;
            }
        }
        ret = ( *pTask )( eventZero );

    }while( 0 );

    return ret;
}

gint32 CPortState::ScheduleResumeTask(
    TaskletPtr& pTask, IrpPtr& pIrp )
{
    gint32 ret = 0;
    CParamList a;

    CPort* pPort =
        static_cast< CPort* >( m_pPort );

    do{
        // it is safe to use m_pExeclIrp here
        ret = a.SetObjPtr( propIrpPtr, pIrp );
        if( ERROR( ret ) )
            break;

        ObjPtr pObj( m_pPort );
        ret = a.SetObjPtr( propPortPtr, pObj );
        if( ERROR( ret ) )
            break;

        ret = a.SetObjPtr(
            propFuncAddr, ObjPtr( pTask ) );
    
        if( ERROR( ret ) )
            break;

        ret = pPort->GetIoMgr()->ScheduleTask(
            clsid( CPortStateResumeSubmitTask ),
            a.GetCfg() );

    }while( 0 );

    return ret;
}

}
