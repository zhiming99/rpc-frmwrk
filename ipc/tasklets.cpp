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
 * =====================================================================================
 */

#include "tasklets.h"
#include "frmwrk.h"
#include "stlcont.h"
#include <stdlib.h>

using namespace std;

atomic< guint32 > CTasklet::m_atmTid(
    ( guint32 )random() );

CTasklet::CTasklet( const IConfigDb* pCfg )
{
    m_pCtx.NewObj( Clsid_CConfigDb, pCfg );
    m_dwTid = m_atmTid++;
    m_iRet = 0;
    m_bPending = false;
    m_bInProcess = false;
    m_dwCallCount = 0;
    Sem_Init( &m_semReentSync, 0, 0 );
}

gint32 CTasklet::GetProperty(
        gint32 iProp,
        CBuffer& oBuf ) const
{
    if( m_pCtx.IsEmpty() )
        return -ENOENT;

    gint32 ret = 0;

    ret = m_pCtx->GetProperty( iProp, oBuf );
    if( ret == -ENOENT )
        ret =super::GetProperty( iProp, oBuf );

    return ret;
}

gint32 CTasklet::SetProperty(
    gint32 iProp,
    const CBuffer& oBuf )
{
    if( m_pCtx.IsEmpty() )
        return -ENOENT;

    return m_pCtx->SetProperty( iProp, oBuf );
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
    guint32 dwParam1,
    guint32 dwParam2,
    guint32* pData )
{ 
    // an trigger from event interface
    IntVecPtr pVec( true );

    ( *pVec )().push_back( ( guint32 )iEvent );
    ( *pVec )().push_back( ( guint32 )dwParam1 );
    ( *pVec )().push_back( ( guint32 )dwParam2 );
    ( *pVec )().push_back( ( guint32 )pData);

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

gint32 CTasklet::GetParamList(
    vector< guint32 >& vecParams, EnumPropId iProp )
{

    gint32 ret = 0;
    ObjPtr pParams;

    do{
        CCfgOpener oCfg( ( IConfigDb* )m_pCtx );
        IntVecPtr pVec;

        ret = oCfg.GetObjPtr( iProp, pParams );
        if( ERROR( ret ) )
            break;

        pVec = pParams;
        if( pVec.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        CStlIntVector::MyType& oIntVec = ( *pVec )();

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
    ObjPtr pParams;

    do{
        CStlIntVector::MyType oIntVec;
        ret = GetParamList( oIntVec );
        if( ERROR( ret ) )
            break;

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

gint32 CTaskletRetriable::ScheduleRetry()
{
    gint32 ret = 0;

    do{
        guint32 dwInterval = 0;

        CIoManager* pMgr = nullptr;
        CCfgOpener oParams(
            ( IConfigDb* )GetConfig() );

        ret = oParams.GetPointer(
            propIoMgr, pMgr );

        if( ERROR( ret ) )
            break;

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
        CTimerService& oTimerSvc =
            pMgr->GetUtils().GetTimerSvc();

        m_iTimerId = oTimerSvc.AddTimer(
            dwInterval, this, eventRetry );

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
    bool bExcept = false;

    do{
        try{
            // no allow of reentrency from concurrency or
            // the cyclic call
            CReentrancyGuard oTaskEntered( this );
            // test if we are from a retry
            if( dwContext == eventTimeout )
            {
                vector< guint32 > vecParams;
                ret = GetParamList( vecParams );
                if( SUCCEEDED( ret ) )
                {
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
        }
        catch( std::system_error& oErr )
        {
            ret = oErr.code().value();
            bExcept = true;
        }

        if( bExcept )
        {
            // reschedule
            sem_wait( &m_semReentSync );
            bExcept = false;
            continue;
        }
        break;

    }while( 1 );

    return ret;
}

gint32 CTaskletRetriable::SetProperty(
        gint32 iProp, const CBuffer& oBuf )
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

    BufPtr pBuf( true );

    gint32 ret = m_pCtx->GetProperty(
        propRetriesBackup, *pBuf );

    if( SUCCEEDED( ret ) )
    {
        m_pCtx->SetProperty(
            propRetries, *pBuf );
    }

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

gint32 CTaskletRetriable::ResetTimer()
{
    if( m_iTimerId <= 0 )
        return -EINVAL;

    CIoManager* pMgr = nullptr;
    CCfgOpener oParams( ( IConfigDb* )m_pCtx );

    gint32 ret = oParams.GetPointer(
        propIoMgr, pMgr );

    if( ERROR( ret ) )
        return ret;

    CTimerService& oTimerSvc =
        pMgr->GetUtils().GetTimerSvc();

    ret = oTimerSvc.ResetTimer( m_iTimerId );
    return ret;
}

gint32 CTaskletRetriable::RemoveTimer(
    gint32 iTimerId )
{
    if( iTimerId <= 0 )
        return -EINVAL;

    CIoManager* pMgr = nullptr;
    CCfgOpener oParams( ( IConfigDb* )m_pCtx );

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
    guint32 dwParam1,
    guint32 dwParam2,
    guint32* pData )
{ 
    // an trigger from event interface
    IntVecPtr pVec( true );
    ( *pVec )().push_back( ( guint32 )iEvent );
    ( *pVec )().push_back( ( guint32 )dwParam1 );
    ( *pVec )().push_back( ( guint32 )dwParam2 );
    ( *pVec )().push_back( ( guint32 )pData);


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

gint32 CThreadSafeTask::GetProperty( gint32 iProp,
        CBuffer& oBuf ) const
{
    CStdRTMutex oTaskLock( GetLock() );
    return super::GetProperty( iProp, oBuf );
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
            std::vector< guint32 > vecParams;
            ret = GetParamList( vecParams );
            if( ERROR( ret ) )
            {
                ret = 0;
                break;
            }
            if( vecParams.size() < 
                GetArgCount( &IEventSink::OnEvent ) )
                break;

            ret = ( gint32 )vecParams[ 1 ];
            break;
        }
    default:
        {
            ret = 0;
            break;
        }
    }
    Sem_Post( &m_semWait );
    return SetError( ret );
}

gint32 CSyncCallback::WaitForComplete()
{
    return Sem_Wait( &m_semWait );
}

