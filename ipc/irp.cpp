/*
 * =====================================================================================
 *
 *       Filename:  irp.cpp
 *
 *    Description:  implemention of IoRequestPacket
 *
 *        Version:  1.0
 *        Created:  06/27/2016 08:58:32 PM
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
#include "defines.h"
#include "port.h"
#include "frmwrk.h"

namespace rpcf
{

using namespace std;

IRP_CONTEXT::IRP_CONTEXT( IRP_CONTEXT* pRhs )
    : CObjBase()
{
    SetClassId( clsid( IRP_CONTEXT ) );

    if( pRhs != nullptr )
    {
        *this = *pRhs;
        return;
    }

    m_dwFlags = 0;

    m_iStatus = 0;

    // the major control code
    m_dwMajorCmd = 0;

    // the minor control code
    m_dwMinorCmd = 0;

    // the secondary control code
    m_dwCtrlCmd = 0;

}

IRP_CONTEXT& IRP_CONTEXT::operator=( const IRP_CONTEXT& oIrpCtx )
{
    m_dwFlags = oIrpCtx.m_dwFlags;

    m_iStatus = oIrpCtx.m_iStatus; 

    // the major control code
    m_dwMajorCmd = oIrpCtx.m_dwMajorCmd; 

    // the major control code
    m_dwMinorCmd = oIrpCtx.m_dwMinorCmd; 

    // the secondary control code
    m_dwCtrlCmd =  oIrpCtx.m_dwCtrlCmd;

    // the pointer of the request data
    m_pReqData = oIrpCtx.m_pReqData;

    // the output data buffer, 
    // the caller alloc/frees this buffer
    m_pRespData = oIrpCtx.m_pRespData;

    // context extension
    // m_pExtBuf is set via AllocIrpCtxExt

    return *this;
}

IRP_CONTEXT::~IRP_CONTEXT()
{
}

guint32 IRP_CONTEXT::GetMajorCmd() const
{
    return ( m_dwMajorCmd & IRP_MAJOR_MASK );
}

guint32 IRP_CONTEXT::GetMinorCmd() const
{
    return ( ( m_dwMinorCmd & IRP_MINOR_MASK) );
}

guint32 IRP_CONTEXT::GetCtrlCode() const
{
    return ( m_dwCtrlCmd & IRP_CTRLCODE_MASK );
}

void IRP_CONTEXT::SetMajorCmd( guint32 dwMajorCmd )
{
    m_dwMajorCmd &= ( ~IRP_MAJOR_MASK );
    m_dwMajorCmd |= ( dwMajorCmd & IRP_MAJOR_MASK );
}
void IRP_CONTEXT::SetMinorCmd( guint32 dwMinorCmd )
{
    m_dwMinorCmd &= ( ~IRP_MINOR_MASK );
    m_dwMinorCmd |= ( dwMinorCmd & IRP_MINOR_MASK );
}
void IRP_CONTEXT::SetCtrlCode( guint32 dwCtrlCode )
{
    m_dwCtrlCmd &= ( ~IRP_CTRLCODE_MASK );
    m_dwCtrlCmd |= ( dwCtrlCode & IRP_CTRLCODE_MASK );
}

gint32 IRP_CONTEXT::GetStatus() const
{
    return m_iStatus;
}

void IRP_CONTEXT::SetStatus( gint32 iStatus )
{
    m_iStatus = iStatus;
}

gint32 IRP_CONTEXT::SetExtBuf( const BufPtr& bufPtr )
{
    gint32 ret = 0;
    try{
        // make a content copy
        m_pExtBuf = bufPtr;
    }
    catch( std::invalid_argument& e )
    {
        ret = -EINVAL;
    }
    return ret;
}

gint32 IRP_CONTEXT::SetReqData( const BufPtr& bufPtr )
{
    gint32 ret = 0;
    try{
       m_pReqData = bufPtr;
    }
    catch( std::invalid_argument& e )
    {
       ret = -EINVAL; 
    }
    return ret;
}

gint32 IRP_CONTEXT::SetRespData( const BufPtr& bufPtr )
{
    gint32 ret = 0;
    try{
        if( bufPtr.IsEmpty() )
        {
            m_pRespData.Clear();
            return 0;
        }
        m_pRespData = bufPtr;
    }
    catch( std::invalid_argument& e )
    {
       ret = -EINVAL; 
    }
    return ret;
}

gint32 IRP_CONTEXT::GetRespAsCfg( CfgPtr& pCfg )
{
    gint32 ret = 0;
    try{
        if( m_pRespData.IsEmpty() )
        {
            ret = -EFAULT;
        }
        else
        {
            if( m_pReqData->GetDataType() !=
                DataTypeObjPtr )
                return -EINVAL;

            ObjPtr pObj = ( ObjPtr& )*m_pRespData;
            pCfg = pObj;
            if( pCfg.IsEmpty() )
                ret = -EFAULT;
        }
    }
    catch( std::invalid_argument& e )
    {
       ret = -EINVAL; 
    }

    return ret;
}

gint32 IRP_CONTEXT::GetReqAsCfg( CfgPtr& pCfg )
{
    gint32 ret = 0;
    try{
        if( m_pReqData.IsEmpty() )
        {
            ret = -EFAULT;
        }
        else
        {
            if( m_pReqData->GetDataType() !=
                DataTypeObjPtr )
                return -EINVAL;

            ObjPtr pObj = ( ObjPtr& )*m_pReqData;
            pCfg = pObj;
            if( pCfg.IsEmpty() )
                ret = -EFAULT;
        }
    }
    catch( std::runtime_error & e )
    {
        ret = ERROR_FAIL;
    }
    catch( std::invalid_argument& e )
    {
       ret = -EINVAL; 
    }
    return ret;
}

void IRP_CONTEXT::GetExtBuf( BufPtr& bufPtr ) const
{
    bufPtr = m_pExtBuf;
}


guint32 IRP_CONTEXT::GetIoDirection() const
{
    return ( m_dwFlags & IRP_DIR_MASK );
}
void IRP_CONTEXT::SetIoDirection( guint32 dwDir )
{
    m_dwFlags &= ~IRP_DIR_MASK;
    m_dwFlags |= ( dwDir & IRP_DIR_MASK );
    return;
}

bool IRP_CONTEXT::IsNonDBusReq() const
{
    return ( ( m_dwFlags & IRP_NON_DBUS ) > 0 );
}

void IRP_CONTEXT::SetNonDBusReq( bool bNonDBus )
{
    m_dwFlags &= ~IRP_NON_DBUS;
    m_dwFlags |= ( bNonDBus ? IRP_NON_DBUS : 0 );
    return;
}

IoRequestPacket::IoRequestPacket() :
    m_iStatus( STATUS_PENDING ),
    m_dwFlags( 0 ),
    m_atmState( IRP_STATE_READY ),
    m_pCallback( nullptr ),
    m_dwContext( 0 ),
    m_IrpThrdPtr( nullptr ),
    m_iTimerId( -1 ),
    m_pMgr( nullptr ),
    m_dwCurPos( 0 ),
    m_pMasterIrp( nullptr ),
    m_wMinSlaves( 0 )

{
    SetClassId( clsid( IoRequestPacket ) );
    Sem_Init( &m_semWait, 0, 0 );
}

bool IoRequestPacket::SetState(
    gint32 iCurState, gint32 iNewState )
{
    // can only be called after a successful call
    // of CanContinue
    bool ret =
        m_atmState.compare_exchange_strong(
                iCurState, iNewState );
    return ret;
}
IoRequestPacket::~IoRequestPacket()
{
    while( m_vecCtxStack.size() )
        PopCtxStack();
    RemoveTimer();
    sem_destroy( &m_semWait );
}

void IoRequestPacket::PopCtxStack()
{
    if( GetStackSize() == 0 )
        return;

    IPort* pPort = GetTopPort();
    IrpCtxPtr& pCtx = GetTopStack();

    if( pPort != nullptr )
    {
        // free the irp extension
        pPort->ReleaseIrpCtxExt(
            pCtx, nullptr );
    }

    m_vecCtxStack.pop_back(); 
}

gint32 IoRequestPacket::AllocNextStack(
    IPort* pPort,
    gint32 iAllocOption )
{
    // prepare the stack for the lower port
    IrpCtxPtr ctxPtr( true );
    gint32 ret = 0;

    do{
        if( pPort == nullptr && m_vecCtxStack.size() > 0 )
        {
            ret = -EINVAL;
            break;
        }
        else if( pPort == nullptr )
        {
            // we are ok with the nullptr for the first port
        }
        
        if( iAllocOption == IOSTACK_ALLOC_COPY )
        {
            // copy the stack top
            *ctxPtr = *m_vecCtxStack.back().second;
        }
        else if( iAllocOption != IOSTACK_ALLOC_NEW )
        {
            ret = -EINVAL;
            break;
        }

        // we will have a ref count to keep the port
        m_vecCtxStack.push_back(
            pair< PortPtr, IrpCtxPtr >( PortPtr( pPort ), ctxPtr ) );

    }while( 0 );

    return ret;
}

IrpCtxPtr& IoRequestPacket::GetTopStack() const
{
   return ( IrpCtxPtr& )m_vecCtxStack.back().second; 
}

IPort* IoRequestPacket::GetTopPort() const
{
   return m_vecCtxStack.back().first; 
}

bool IoRequestPacket::IsPending() const
{
    return ( 0 != ( m_dwFlags & IRP_PENDING ) );
}
void IoRequestPacket::MarkPending()
{
    m_dwFlags |= IRP_PENDING;
}

void IoRequestPacket::RemovePending()
{
    m_dwFlags &= ~IRP_PENDING;
}

bool IoRequestPacket::CanCancel()
{
    return CanComplete();
}

gint32 IoRequestPacket::CanContinue(
    gint32 iNewState )
{
    // we will wait till the irp return
    // to ready or completed or cancelled.
    //
    // since the IRP can be held anywhere
    // it is necessary to have a state flag
    // to indicate if the IRP is valid for
    // further processing. that is why this
    // method exists.
    gint32 iExpected = IRP_STATE_READY; 
    gint32 ret = 0;
    do{
        if( !m_atmState.compare_exchange_strong(
            iExpected, iNewState ) )
        {
            switch( iExpected )
            {
            case IRP_STATE_COMPLETED:
            case IRP_STATE_CANCELLED:
                {
                    //operation cannot continue
                    ret = -EPERM;
                    break;
                }
            case IRP_STATE_BUSY:
            case IRP_STATE_COMPLETING:
            default:
                ret = ERROR_STATE;
                break;
            }

        }
    }while( 0 );

    return ret;
}

bool IoRequestPacket::IsSyncCall() const
{
    return ( 0 != ( m_dwFlags & IRP_SYNC_CALL ) );
}

void IoRequestPacket::SetSyncCall( bool bSync ) 
{
    if( bSync )
        m_dwFlags |= IRP_SYNC_CALL;
    else
        m_dwFlags &= ~IRP_SYNC_CALL;
}

void IoRequestPacket::SetCbOnly( bool bCbOnly )
{
    if( bCbOnly )
        m_dwFlags |= IRP_CALLBACK_ONLY;
    else
        m_dwFlags &= ~IRP_CALLBACK_ONLY;
}

void IoRequestPacket::SetNoComplete(
    bool bNoComplete )
{
    if( bNoComplete )
        m_dwFlags |= IRP_NO_COMPLETE;
    else
        m_dwFlags &= ~IRP_NO_COMPLETE;
}

void IoRequestPacket::SetCompleteInPlace(
    bool bInPlace )
{
    if( bInPlace )
        m_dwFlags |= IRP_COMP_INPLACE;
    else
        m_dwFlags &= ~IRP_COMP_INPLACE;

}

gint32 IoRequestPacket::WaitForComplete()
{
    int ret = 0;
    while( sem_wait( &m_semWait ) < 0 )
    {
        if( errno == EINTR )
            continue;

        if( errno == EINVAL )
        {
            RemovePending();
            ret = -errno;
            break;
        }
    }
    return ret;
}

void IoRequestPacket::SetMinSlaves(
    guint16 wCount )
{
    m_wMinSlaves = wCount;
}

void IoRequestPacket::AddSlaveIrp(
    IRP* pSlave )
{
    if( m_mapSlaveIrps.find( pSlave ) !=
        m_mapSlaveIrps.end() )
        return;

    m_mapSlaveIrps[ pSlave ] = STATUS_PENDING;
    if( m_wMinSlaves > 0 )
        --m_wMinSlaves;
}

bool IoRequestPacket::FindSlaveIrp(
    IRP* pSlave ) const
{
    return m_mapSlaveIrps.find( pSlave )
        != m_mapSlaveIrps.end();
}

void IoRequestPacket::RemoveSlaveIrp(
    IRP* pSlave )
{
    m_mapSlaveIrps.erase( pSlave );
}

void IoRequestPacket::RemoveAllSlaves()
{
    m_mapSlaveIrps.clear(); 
}

void IoRequestPacket::SetMasterIrp(
    IRP* pMaster )
{
    m_pMasterIrp = pMaster;
}

IRP* IoRequestPacket::GetMasterIrp() const
{
    return m_pMasterIrp;
}

guint32 IoRequestPacket::GetSlaveCount() const
{
    return m_mapSlaveIrps.size() + m_wMinSlaves;
}

void IoRequestPacket::SetTimer(
    guint32 dwTimeoutSec,
    CIoManager* pMgr )
{
    m_dwTimeoutSec = dwTimeoutSec;
    if( dwTimeoutSec > 3600 * 72 )
        return;

    if( pMgr != nullptr )
    {
        m_pMgr = pMgr;
        CUtilities& oUtils = pMgr->GetUtils();

        gint32 iTimerId = oUtils.GetTimerSvc().AddTimer(
            m_dwTimeoutSec, this, ( LONGWORD )pMgr );

        if( iTimerId > 0 )
        {
            m_iTimerId = iTimerId;
        }
    }
}

void IoRequestPacket::RemoveTimer()
{
    if( m_pMgr != nullptr && m_iTimerId > 0 )
    {
        CUtilities& oUtils = m_pMgr->GetUtils();
        oUtils.GetTimerSvc().RemoveTimer( m_iTimerId );
        m_iTimerId = -1;
    }

}

void IoRequestPacket::ResetTimer()
{
    if( m_pMgr != nullptr && m_iTimerId > 0 )
    {
        CTimerService& oTimerSvc =
            m_pMgr->GetUtils().GetTimerSvc();

        oTimerSvc.ResetTimer( m_iTimerId );
    }
}

gint32 IoRequestPacket::OnEvent(
        EnumEventId iEvent,
        LONGWORD dwParam1,
        LONGWORD dwParam2,
        LONGWORD* pData )
{

    gint32 ret = 0;

    switch( iEvent )
    {
    case eventTimeout:
        {
            CIoManager* pMgr = reinterpret_cast
                < CIoManager* >( dwParam1 );

            if( pMgr == nullptr )
            {
                ret = -EINVAL;
                break;
            }

            CStdRMutex oLock( GetLock() );
            ret = CanContinue( IRP_STATE_READY );
            if( ERROR( ret ) )
                break;
            m_iTimerId = -1;
            IrpCtxPtr& pCtx = GetTopStack();
            pCtx->SetStatus( -ETIMEDOUT );
            oLock.Unlock();

            pMgr->CancelIrp(
                this, true, -ETIMEDOUT );

            break;
        }
    default:
        {
            break;
        }
    }
    return ret;
}

guint32 IoRequestPacket::GetStackSize() const
{
    return m_vecCtxStack.size();
}

guint32 IoRequestPacket::GetCurPos() const
{
    return m_dwCurPos;
}

bool IoRequestPacket::IsIrpHolder() const
{
    // this method should only be used
    // when the irp is being completed
    // or canceled
    return ( GetCurPos() + 1
            == GetStackSize() );
}

bool IoRequestPacket::SetCurPos( guint32 dwIdx )
{
    bool bRet = false;
    if( dwIdx < m_vecCtxStack.size() )
    {
        m_dwCurPos = dwIdx;
        bRet = true;
    }

    return bRet;;
}

IrpCtxPtr& IoRequestPacket::GetCtxAt( guint32 dwPos ) const
{
    if( dwPos >= m_vecCtxStack.size()
        || m_vecCtxStack.empty() )
        throw std::out_of_range( "no such element" );

    return ( IrpCtxPtr& )m_vecCtxStack[ dwPos ].second; 
}

IPort* IoRequestPacket::GetPortAt( guint32 dwPos ) const
{
    if( dwPos >= m_vecCtxStack.size()
        || m_vecCtxStack.empty() )
        throw std::out_of_range( "no such element" );

    return m_vecCtxStack[ dwPos ].first; 
}

void IoRequestPacket::SetCallback(
    const EventPtr& pEvent,
    LONGWORD dwContext )
{
    m_pCallback = pEvent;
    m_dwContext = dwContext;
}

void IoRequestPacket::RemoveCallback()
{
    m_pCallback.Clear();
    m_dwContext = 0;
}

bool IoRequestPacket::CanComplete()
{
    if( IsNoComplete() )
        return false;

    return ( m_dwTid != GetTid()
        || IsPending() );
}

guint32 IoRequestPacket::MajorCmd() const
{
    if( GetStackSize() == 0 )
        return IRP_MJ_INVALID;

    return GetCurCtx()->GetMajorCmd();
}
guint32 IoRequestPacket::MinorCmd() const
{
    if( GetStackSize() == 0 )
        return IRP_MN_INVALID;

    return GetCurCtx()->GetMinorCmd();
}
guint32 IoRequestPacket::CtrlCode() const
{
    if( GetStackSize() == 0 )
        return CTRLCODE_INVALID;

    return GetCurCtx()->GetCtrlCode();
}
guint32 IoRequestPacket::IoDirection() const
{
    if( GetStackSize() == 0 )
        return CTRLCODE_INVALID;

    return GetCurCtx()->GetIoDirection();
}

gint32 IoRequestPacket::SetIrpThread(
    CIoManager* pMgr )
{
    if( pMgr == nullptr )
        return -EINVAL;

    m_pMgr = pMgr;
    return STATUS_SUCCESS;
}

gint32 IoRequestPacket::GetIrpThread(
    ThreadPtr& pthrd )
{
    gint32 ret = 0;
    do{
        if( m_pMgr == nullptr )
        {
            ret = -ENOENT;
            break;
        }
        CThreadPool& oPool =
            m_pMgr->GetIrpThreadPool();
        ret = oPool.GetThread( pthrd );

    }while( 0 );

    return ret;
}

void IoRequestPacket::ClearIrpThread()
{
    m_IrpThrdPtr.Clear();
}

}
