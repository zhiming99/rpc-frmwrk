/*
 * =====================================================================================
 *
 *       Filename:  tasklets.cpp
 *
 *    Description:  implementation of the various tasklets
 *
 *        Version:  1.0
 *        Created:  10/22/2016 02:39:30 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi ( woodhead99@gmail.com )
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

#include "tasklets.h"
#include "frmwrk.h"
#include "stlcont.h"
#include <stdlib.h>

namespace rpcf
{

using namespace std;

CTasklet::CTasklet( const IConfigDb* pCfg )
{
    m_pCtx.NewObj();
    if( pCfg != nullptr )
        *m_pCtx = *pCfg;
    m_dwTid = rpcf::GetTid();
    m_iRet = 0;
    m_bPending = false;
    m_bInProcess = false;
    m_dwCallCount = 0;
    Sem_Init( &m_semReentSync, 0, 0 );
}

gint32 CTasklet::GetProperty(
    gint32 iProp,
    Variant& oVar ) const
{
    if( m_pCtx.IsEmpty() )
        return -ENOENT;

    gint32 ret = 0;
    ret = m_pCtx->GetProperty( iProp, oVar );
    if( ret == -ENOENT )
        ret =super::GetProperty( iProp, oVar );

    return ret;
}

gint32 CTasklet::SetProperty(
    gint32 iProp,
    const Variant& oVar )
{
    if( m_pCtx.IsEmpty() )
        return -ENOENT;

    return m_pCtx->SetProperty( iProp, oVar );
}

gint32 CTasklet::RemoveProperty(
    gint32 iProp )
{
    return m_pCtx->RemoveProperty( iProp );
}

gint32 CTasklet::EnumProperties(
    std::vector< gint32 >& vecProps ) const
{
    gint32 ret = super::EnumProperties( vecProps );
    if( ERROR( ret ) )
        return ret;

    if( !m_pCtx.IsEmpty() )
        ret = m_pCtx->EnumProperties( vecProps );

    return ret;
}

gint32 CTasklet::GetPropertyType(
    gint32 iProp, gint32& iType ) const
{
    gint32 ret = 0;

    if( m_pCtx.IsEmpty() )
        return ret;

    ret = m_pCtx->GetPropertyType( iProp, iType );
    if( SUCCEEDED( ret ) )
        return ret;

     return super::GetPropertyType( iProp, iType );
}

gint32 CTasklet::OnEvent(
    EnumEventId iEvent,
    LONGWORD dwParam1,
    LONGWORD dwParam2,
    LONGWORD* pData )
{ 
    // an trigger from event interface
    LwVecPtr pVec( true );

    ( *pVec )().push_back( iEvent );
    ( *pVec )().push_back( dwParam1 );
    ( *pVec )().push_back( dwParam2 );
    ( *pVec )().push_back( ( LONGWORD )pData);

    CCfgOpenerObj oCfg( this );

    // NOTE: propParamList is used here to pass
    // the event parameters. so don't use
    // propParamList when passing parameters to
    // tasklet.
    oCfg.SetObjPtr( propParamList, ObjPtr( pVec ) );

    ( *this )( iEvent );

    // NOTE: here there is slim chance of risk the
    // removal can conflict with external synchronized
    // threads who is wakeup in the operatr(), and
    // begin to read property on this task.
    this->RemoveProperty( propParamList );
    return GetError();
}

LwVecPtr CTasklet::GetParamList(
    EnumPropId iProp )
{
    CCfgOpener oCfg(
        ( IConfigDb* )m_pCtx );
    LwVecPtr pVec;

    ObjPtr pParams;
    gint32 ret = oCfg.GetObjPtr(
        iProp, pParams );

    if( SUCCEEDED( ret ) )
        pVec = pParams;
    return pVec;
}

gint32 CTasklet::GetParamList(
    vector< LONGWORD >& vecParams,
    EnumPropId iProp )
{

    gint32 ret = 0;

    do{
        LwVecPtr pVec = GetParamList( iProp );
        if( pVec.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        auto& oIntVec = ( *pVec )();
        guint32 dwArgCount = 
            GetArgCount( &IEventSink::OnEvent );

        if( oIntVec.empty()
            || oIntVec.size() < dwArgCount )
        {
            ret = -ENOENT;
            break;
        }
        
        vecParams = oIntVec;

    }while( 0 );

    return ret;
}

gint32 CTasklet::GetIrpFromParams( IrpPtr& pIrp )
{
    gint32 ret = 0;

    do{
        LwVecPtr pVec = GetParamList();
        if( pVec.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }
        auto& oIntVec = ( *pVec )();
        if( oIntVec[ 1 ] == 0 )
        {
            ret = -EFAULT;
            break;
        }

        pIrp = reinterpret_cast< IRP* >
            ( oIntVec[ 1 ] );

    }while( 0 );

    return ret;
}

gint32 CTaskletRetriable::AddTimer(
    guint32 dwIntervalSec,
    guint32 dwParam )
{
    if( dwIntervalSec == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CIoManager* pMgr = nullptr;
        CCfgOpener oParams(
            ( const IConfigDb* )GetConfig() );

        ret = oParams.GetPointer(
            propIoMgr, pMgr );

        if( ERROR( ret ) )
            break;

        CTimerService& oTimerSvc =
            pMgr->GetUtils().GetTimerSvc();

        ret = oTimerSvc.AddTimer(
            dwIntervalSec, this, dwParam );

    }while( 0 );

    return ret;
}

gint32 CTaskletRetriable::ScheduleRetry()
{
    gint32 ret = 0;

    do{
        guint32 dwInterval = 0;

        CCfgOpener oParams(
            ( IConfigDb* )GetConfig() );

        ret = oParams.GetIntProp(
            propIntervalSec, dwInterval );

        if( ERROR( ret ) )
            break;

        gint32 iRetries = DecRetries();
        if( iRetries <  0 )
        {
            ret = iRetries;
            break;
        }

        m_iTimerId = AddTimer(
            dwInterval, eventRetry );

        if( m_iTimerId < 0 )
        {
            ret = m_iTimerId;
            m_iTimerId = 0;
        }

    }while( 0 );

    return ret;
}

/**
* @name CTaskletRetriable::operator()
* the main entry of the retriable tasklet
* @{ it will check whether the consecutive retry
* times exceeds the limit, if the limit is
* exceeded, it will stop. otherwise, the next
* retry will be schedule in propIntervalSec
* seconds.
* Note: please make sure at a time the task is
* referenced only by one potential caller, the
* task scheduler, the timer, or the irp
* completion. referenced by more than one will
* have the risk of race condition.
* */
/**  @} */

gint32 CTaskletRetriable::operator()(
    guint32 dwContext )
{
    // dwContext = 0, the first run
    // dwContext = eventTimeout, this is a retry 
    gint32 ret = 0;

    CReentrancyGuard oTaskEntered( this );
    // test if we are from a retry
    if( unlikely( dwContext == eventTimeout ) )
    {
        LwVecPtr pVec = GetParamList();
        if( !pVec.IsEmpty() )
        {
            auto& vecParams = ( *pVec )();
            gint32 iEvent = vecParams[ 1 ];
            if( iEvent == eventRetry )
            {
                // reset the timer id
                m_iTimerId = 0;
            }
        }
    }

    ret = Process( dwContext );

    if( ret == STATUS_MORE_PROCESS_NEEDED )
    {
        gint32 iRet = ScheduleRetry();
        if( ERROR( iRet ) )
            ret = iRet;
    }

    return ret;
}

gint32 CTaskletRetriable::SetProperty(
        gint32 iProp, const Variant& oBuf )
{
    gint32 ret = 0;
    if( m_pCtx.IsEmpty() )
        return -ENOENT;

    ret = m_pCtx->SetProperty( iProp, oBuf );
    if( ERROR( ret ) )
        return ret;

    if( iProp == propRetries )
    {
        ret = m_pCtx->SetProperty(
            propRetriesBackup, oBuf );
    }

    return ret;
}

void CTaskletRetriable::ResetRetries()
{
    if( m_pCtx.IsEmpty() )
        return;

    CCfgOpener oCfg( ( IConfigDb* )m_pCtx );
    oCfg.CopyProp( propRetries,
        propRetriesBackup );
    RemoveTimer();

    return;
}

bool CTaskletRetriable::CanRetry() const
{
    if( m_pCtx.IsEmpty() )
        return -ENOENT;

    gint32 ret = 0;
    gint32 iValue = 0;
    CCfgOpener oCfg( ( const IConfigDb* )m_pCtx );

    ret = oCfg.GetIntProp(
        propRetries, ( guint32& ) iValue );

    if( ERROR( ret ) )
        return false;

    if( iValue > 0 )
        return true;

    return false;
}
/**
* @name DecRetries
* @{ Decrement the the retry count of this
* tasklet */
/** return the retry count decremented.
 * or a negative number as error code  @} */

gint32 CTaskletRetriable::DecRetries()
{
    if( m_pCtx.IsEmpty() )
        return -ENOENT;

    gint32 ret = 0;
    gint32 iValue = 0;
    CCfgOpener oCfg( ( IConfigDb* )m_pCtx );

    ret = oCfg.GetIntProp(
        propRetries, ( guint32& ) iValue );

    if( ERROR( ret ) )
        return ret;

    if( iValue > 0 )
    {
        --iValue;
    }
    else
    {
       return -ERANGE;
    }

    ret = oCfg.SetIntProp(
        propRetries, ( guint32& )iValue );

    if( SUCCEEDED( ret ) )
        ret = iValue;

    return ret;
}

gint32 CTaskletRetriable::ResetTimer(
    gint32 iTimerId ) const
{
    if( iTimerId <= 0 )
        return -EINVAL;

    CIoManager* pMgr = nullptr;
    CCfgOpener oParams(
        ( const IConfigDb* )m_pCtx );

    gint32 ret = oParams.GetPointer(
        propIoMgr, pMgr );

    if( ERROR( ret ) )
        return ret;

    CTimerService& oTimerSvc =
        pMgr->GetUtils().GetTimerSvc();

    ret = oTimerSvc.ResetTimer( iTimerId );
    return ret;
}

gint32 CTaskletRetriable::ResetTimer()
{
    return ResetTimer( m_iTimerId );
}

gint32 CTaskletRetriable::RemoveTimer(
    gint32 iTimerId ) const
{
    if( iTimerId <= 0 )
        return -EINVAL;

    CIoManager* pMgr = nullptr;
    CCfgOpener oParams(
        ( const IConfigDb* )m_pCtx );

    gint32 ret = oParams.GetPointer(
        propIoMgr, pMgr );

    if( ERROR( ret ) )
        return ret;

    CTimerService& oTimerSvc =
        pMgr->GetUtils().GetTimerSvc();

    ret = oTimerSvc.RemoveTimer( iTimerId );

    return ret;
}

gint32 CTaskletRetriable::RemoveTimer()
{
    if( m_iTimerId == 0 )
        return 0;

    gint32 ret = RemoveTimer( m_iTimerId );
    m_iTimerId = 0;

    return ret;
}

gint32 CTaskletRetriable::OnCancel(
    guint32 dwContext )
{
    ResetRetries();
    return 0;
}

CTaskletRetriable::CTaskletRetriable(
    const IConfigDb* pCfg )
    :CTasklet( pCfg ),
     m_iTimerId( 0 )
{
    CCfgOpener oCfg( ( IConfigDb* )m_pCtx );
    if( !oCfg.exist( propRetries ) )
        oCfg.SetIntProp( propRetries,
            TASKLET_RETRY_TIMES );

    if( !oCfg.exist( propIntervalSec ) )
        oCfg.SetIntProp( propIntervalSec,
            TASKLET_RETRY_DEF_INTERVALSEC );
}

gint32 CThreadSafeTask::OnEvent(
    EnumEventId iEvent,
    LONGWORD dwParam1,
    LONGWORD dwParam2,
    LONGWORD* pData )
{ 
    // an trigger from event interface
    LwVecPtr pVec( true );
    ( *pVec )().push_back( iEvent );
    ( *pVec )().push_back( dwParam1 );
    ( *pVec )().push_back( dwParam2 );
    ( *pVec )().push_back( ( LONGWORD )pData);


    // NOTE: propParamList is used here to pass
    // the event parameters. so don't use
    // propParamList when passing parameters to
    // tasklet.
    CStdRTMutex oTaskLock( GetLock() );
    CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
    oCfg.SetObjPtr( propParamList, ObjPtr( pVec ) );
    oTaskLock.Unlock();

    ( *this )( iEvent );

    // NOTE: here there is slim chance of risk the
    // removal can conflict with external synchronized
    // threads who is wakeup in the operatr(), and
    // begin to read property on this task.
    oTaskLock.Lock();
    oCfg.RemoveProperty( propParamList );
    return GetError();
}

gint32 CThreadSafeTask::GetProperty(
    gint32 iProp,
    Variant& oVar ) const
{
    CStdRTMutex oTaskLock( GetLock() );
    return super::GetProperty( iProp, oVar );
}

gint32 CThreadSafeTask::SetProperty(
    gint32 iProp,
    const Variant& oVar )
{
    CStdRTMutex oTaskLock( GetLock() );
    return super::SetProperty( iProp, oVar );
}

gint32 CSyncCallback::operator()(
    guint32 dwContext )
{
    if( dwContext != eventTaskComp &&
        dwContext != eventIrpComp )
        return SetError( -ENOTSUP );

    gint32 ret = 0;
    switch( dwContext )
    {
    case eventTaskComp:
        {
            LwVecPtr pVec = GetParamList();
            if( pVec.IsEmpty() )
            {
                ret = 0;
                break;
            }
            auto& vecParams = ( *pVec )();
            if( vecParams.size() < 
                GetArgCount( &IEventSink::OnEvent ) )
                break;
            ret = vecParams[ 1 ];
            break;
        }
    case eventIrpComp:
        {
            IrpPtr pIrp;
            ret = GetIrpFromParams( pIrp );
            if( ERROR( ret ) )
                break;

            ret = pIrp->GetStatus();
            break;
        }
    default:
        {
            ret = 0;
            break;
        }
    }

    // NOTE: don't SetError after post the signal.
    // it could happen that the signal receiver
    // get stale error code before SetError was
    // done
    SetError( ret );
    Sem_Post( &m_semWait );

    return ret;
}

gint32 CSyncCallback::WaitForComplete()
{
    return Sem_Wait( &m_semWait );
}

gint32 CSyncCallback::WaitForCompleteWakable()
{
    gint32 ret = Sem_Wait_Wakable( &m_semWait );
    if( ERROR( ret ) )
        SetError( ret );
    return ret;
}

}
