/*
 * =====================================================================================
 *
 *       Filename:  stmcp.cpp
 *
 *    Description:  Definitions of the classes as the connection point between 
 *                  proxy/server stream objects
 *
 *        Version:  1.0
 *        Created:  04/23/2023 05:56:34 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *
 *      Copyright:  2023 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#include "stmcp.h"

namespace rpcf
{

void CStreamQueue::SetState(
    EnumTaskState dwState )
{
    CStdMutex oLock( this->GetLock() );
    m_dwState = dwState;
}

gint32 CStreamQueue::Start() 
{
    SetState( stateStarted );
    return 0;
}

gint32 CStreamQueue::Stop( bool bStarter ) 
{
    if( true )
    {
        CStdMutex oLock( this->GetLock() );
        if( GetState() == stateStopped )
            return ERROR_STATE;
        if( m_byStoppedBy == -1 )
            m_byStoppedBy = bStarter ? 0 : 1;
    }
    SetState( stateStopping );
    CancelAllIrps( ERROR_PORT_STOPPED );
    SetState( stateStopped );
    GetParent()->OnEvent( eventStop,
        0, 0, ( LONGWORD* )this );
    return 0;
}

CIoManager* CStreamQueue::GetIoMgr()
{
    if( m_pParent == nullptr )
        return nullptr;
    return m_pParent->GetIoMgr();
}

gint32 CStreamQueue::QueuePacket( BufPtr& pBuf )
{
    gint32 ret = 0;
    bool bNotify = false;
    if( pBuf.IsEmpty() || pBuf->empty() )
        return -EINVAL;

    do{
        CStdMutex oLock( GetLock() );
        if( GetState() != stateStarted )
        {
            ret = ERROR_STATE;
            break;
        }

        if( m_quePkts.size() >=
            STM_MAX_QUEUE_SIZE )
        {
            ret = ERROR_QUEUE_FULL;
            break;
        }

        guint8& byToken =
            ( guint8& )pBuf->ptr()[ 0 ];
        m_quePkts.push_back( pBuf );
        m_qwBytesIn += pBuf->size();
        m_qwPktsIn++;

        if( m_quePkts.size() ==
            STM_MAX_QUEUE_SIZE )
        {
            if( byToken != tokFlowCtrl &&
                byToken != tokLift )
                bNotify = true;
        }

        if( m_bInProcess )
            break;

        if( m_queIrps.empty() )
            break;

        gint32 (*func )( intptr_t ) = 
            ( []( intptr_t iQue )->gint32
            {
                auto pQue = reinterpret_cast
                    < CStreamQueue* >( iQue );
                return pQue->ProcessIrps();
            });

        TaskletPtr pTask;
        CIoManager* pMgr = this->GetIoMgr();
        ret = NEW_FUNCCALL_TASK(
            pTask, pMgr, func,
            ( intptr_t )this );
        if( ERROR( ret ) )
            break;

        m_bInProcess = true;
        ret = this->RescheduleTask( pTask );
        if( ERROR( ret ) )
        {
            m_bInProcess = false;
            break;
        }

    }while( 0 );

    if( bNotify )
    {
        GetParent()->OnEvent( eventPaused,
            0, 0, ( LONGWORD* )this );
    }

    return ret;
}

gint32 CStreamQueue::SubmitIrp( PIRP pIrp )
{
    gint32 ret = 0;
    if( pIrp == nullptr )
        return -EINVAL;
    do{
        CStdMutex oLock( GetLock() );
        if( GetState() != stateStarted )
        {
            ret = ERROR_STATE;
            break;
        }

        if( m_queIrps.size() >= STM_MAX_QUEUE_SIZE )
        {
            ret = ERROR_QUEUE_FULL;
            break;
        }

        IrpPtr irpPtr = pIrp;
        if( m_bInProcess || m_quePkts.empty() )
        {
            m_queIrps.push_back( irpPtr );
            ret = STATUS_PENDING;
            break;
        }

        if( m_queIrps.size() )
        {
            m_queIrps.push_back( irpPtr );
            gint32 (*func )( intptr_t ) = 
                ( []( intptr_t iQue )->gint32
                {
                    auto pQue = reinterpret_cast
                        < CStreamQueue* >( iQue );
                    return pQue->ProcessIrps();
                });

            TaskletPtr pTask;
            CIoManager* pMgr = this->GetIoMgr();
            ret = NEW_FUNCCALL_TASK(
                pTask, pMgr, func,
                ( intptr_t )this );
            if( ERROR( ret ) )
                break;
            m_bInProcess = true;
            ret = this->RescheduleTask( pTask );
            if( SUCCEEDED( ret ) )
                ret = STATUS_PENDING;
            else
                m_bInProcess = false;
            break;
        }

        BufPtr pResp = m_quePkts.front();
        m_quePkts.pop_front();
        m_qwPktsOut++;
        m_qwBytesOut += pResp->size();
        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        pCtx->SetRespData( pResp );
        pCtx->SetStatus( STATUS_SUCCESS );
        if( m_quePkts.size() ==
            STM_MAX_QUEUE_SIZE - 1 )
        {
            oLock.Unlock();
            GetParent()->OnEvent( eventResumed,
                0, 0, ( LONGWORD* )this );
        }

    }while( 0 );

    return ret;
}

gint32 CStreamQueue::ProcessIrps()
{
    gint32 ret = 0;
    do{
        CIoManager* pMgr = GetIoMgr();
        CStdMutex oLock( this->GetLock() );
        if( GetState() == stateStopped )
        {
            ret = ERROR_STATE;
            m_bInProcess = false;
            break;
        }
        if( m_quePkts.empty() || m_queIrps.empty() )
        {
            m_bInProcess = false;
            break;
        }

        m_bInProcess = true;
        while( m_queIrps.size() &&
            m_quePkts.size() )
        {
            IrpPtr pIrp = m_queIrps.front();
            m_queIrps.pop_front();
            BufPtr pResp = m_quePkts.front();
            m_quePkts.pop_front();
            m_qwPktsOut++;
            m_qwBytesOut += pResp->size();
            oLock.Unlock();

            bool bPutBack = false;
            CStdRMutex oIrpLock( pIrp->GetLock() );
            ret = pIrp->CanContinue( IRP_STATE_READY );
            if( SUCCEEDED( ret ) )
            {
                IrpCtxPtr& pCtx = pIrp->GetTopStack();
                pCtx->SetRespData( pResp );
                pCtx->SetStatus( STATUS_SUCCESS );
                oIrpLock.Unlock();
                pMgr->CompleteIrp( pIrp );
            }
            else
            {
                oIrpLock.Unlock();
                bPutBack = true;
            }
            oLock.Lock();
            if( bPutBack )
            {
                m_quePkts.push_front( pResp );
                m_qwPktsOut--;
                m_qwBytesOut -= pResp->size();
            }
            guint32 dwCount = m_quePkts.size();
            if( !bPutBack &&
                dwCount == STM_MAX_QUEUE_SIZE - 1 )
            {
                oLock.Unlock();
                GetParent()->OnEvent( eventResumed,
                    0, 0, ( LONGWORD* )this );
                oLock.Lock();
            }
        }
        m_bInProcess = false;

    }while( 0 );


    return ret;
}

gint32 CStreamQueue::RemoveIrpFromMap(
    PIRP pIrp )
{
    gint32 ret = 0;
    if( pIrp == nullptr )
        return -EINVAL;

    bool bFound = false;
    do{
        CStdMutex oLock( this->GetLock() );
        if( GetState() == stateStopped )
        {
            ret = ERROR_STATE;
            break;
        }
        if( m_queIrps.empty() )
        {
            ret = -ENOENT;
            break;
        }
        guint64 id = pIrp->GetObjId();
        auto itr = m_queIrps.begin();
        while( itr != m_queIrps.end() )
        {
            if( ( *itr )->GetObjId() == id )
            {
                m_queIrps.erase( itr );
                bFound = true;
                break;
            }
            itr++;
        }
        oLock.Unlock();
        if( !bFound )
            ret = -ENOENT;

    }while( 0 );

    return ret;
}

gint32 CStreamQueue::CancelAllIrps(
    gint32 iError )
{
    gint32 ret = 0;
    do{
        CStdMutex oLock( this->GetLock() );
        if( GetState() == stateStopped )
        {
            ret = ERROR_STATE;
            break;
        }
        std::vector< IrpPtr > vecIrps;
        vecIrps.insert( vecIrps.begin(),
            m_queIrps.begin(),
            m_queIrps.end() );

        m_queIrps.clear();
        m_quePkts.clear();

        oLock.Unlock();

        for( auto& elem : vecIrps )
        {
            GetIoMgr()->CancelIrp(
                elem, false, iError );
        }

    }while( 0 );

    return ret;
}

std::hashmap< HANDLE, ObjPtr > CStmConnPoint::m_mapConnPts;
stdmutex CStmConnPoint::m_oLock;

gint32 CStmConnPoint::RegForTransfer()
{
    CStdMutex oLock( GetLock() );
    m_mapConnPts.insert(
        { ( HANDLE )this, ObjPtr( this ) } );
    return 0;
}

gint32 CStmConnPoint::RetrieveAndUnreg(
    HANDLE hConn, ObjPtr& pConn )
{
    CStdMutex oLock( GetLock() );
    auto itr = m_mapConnPts.find( hConn );
    if( itr == m_mapConnPts.end() )
        return -ENOENT;
    pConn = itr->second;
    m_mapConnPts.erase( itr );
    return STATUS_SUCCESS;
}

CStmConnPoint::CStmConnPoint(
    const IConfigDb* pCfg )
{
    gint32 ret = 0;
    do{
        SetClassId( clsid( CStmConnPoint ) );
        CCfgOpener oCfg( pCfg );
        ret = oCfg.GetPointer(
            propIoMgr, m_pMgr );

        gint32 iCount = sizeof( m_arrQues ) /
            sizeof( m_arrQues[ 0 ] );

        for( gint32 i = 0; i < iCount; i++ )
            m_arrQues[ i ].SetParent( this );

        ret = Start();

    }while( 0 );

    if( SUCCEEDED( ret ) )
        return;

    stdstr strMsg = DebugMsg( ret,
        "cannot find iomgr in "
        "CStmConnPoint's ctor" );
    throw std::runtime_error( strMsg );
}

gint32 CStmConnPoint::Start()
{
    gint32 ret = 0;

    gint32 iCount = sizeof( m_arrQues ) /
        sizeof( m_arrQues[ 0 ] );

    for( gint32 i = 0; i < iCount; i++ )
        m_arrQues[ i ].Start();

    return ret;
}

gint32 CStmConnPoint::Stop( bool bStarter )
{
    ObjPtr pObj;
    gint32 ret = 0;
    do{
        gint32 iCount = sizeof( m_arrQues ) /
            sizeof( m_arrQues[ 0 ] );

        for( gint32 i = 0; i < iCount; i++ )
            m_arrQues[ i ].Stop( bStarter );

    }while( 0 );
    RetrieveAndUnreg( ( HANDLE )this, pObj ); 
    return ret;
}

gint32 CStmConnPoint::OnEvent(
    EnumEventId iEvent,
    LONGWORD dwParam1,
    LONGWORD dwParam2,
    LONGWORD* pData )
{
    gint32 ret = 0;
    switch( iEvent )
    {
    case eventResumed:
    case eventPaused:
        {
            auto pQue = reinterpret_cast
                < CStreamQueue* >( pData );

            guint32 dwIdx = pQue - m_arrQues;
            if( dwIdx >= 2 )
            {
                ret = -ERANGE;
                break;
            }

            dwIdx ^= 1;

            BufPtr pBuf( true );
            pBuf->Resize( 1 );
            if( iEvent == eventResumed )
                pBuf->ptr()[ 0 ] = tokLift;
            else
                pBuf->ptr()[ 0 ] = tokFlowCtrl;
            m_arrQues[ dwIdx ].QueuePacket( pBuf );
            break;
        }
    case eventStop:
        {
            BufPtr pBuf( true );
            pBuf->Resize( 1 );
            pBuf->ptr()[ 0 ] = tokClose;
            for( int i = 0; i < 2; i++ )
                m_arrQues[ i ].QueuePacket( pBuf );
            break;
        }
    default:
        break;
    }
    return ret;
}

CStmCpPdo::CStmCpPdo( const IConfigDb* pCfg ) :
    super( pCfg )
{
    SetClassId( clsid( CStmCpPdo ) );
    this->m_dwFlags &= ~PORTFLG_TYPE_MASK;
    this->m_dwFlags |= PORTFLG_TYPE_PDO;

    CCfgOpener oCfg( pCfg );
    gint32 ret = 0;

    do{
        ret = oCfg.GetPointer(
            propBusPortPtr, m_pBusPort );

        if( ERROR( ret ) )
            break;

        ret = oCfg.GetObjPtr(
            propStmConnPt, m_pStmCp );
        if( ERROR( ret ) )
            break;

        oCfg.GetBoolProp(
            propStarter, m_bStarter );

        CCfgOpenerObj oPortCfg( this );
        oPortCfg.RemoveProperty(
            propStmConnPt );

    }while( 0 );

    if( ERROR( ret ) )
    {
        throw std::runtime_error(
            "Error in CStmCpPdo ctor" );
    }
}

gint32 CStmCpPdo::RemoveIrpFromMap(
    PIRP pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    bool bStarter = this->IsStarter();
    CStmConnPointHelper oh(
        m_pStmCp, bStarter );
    return oh.RemoveIrpFromMap( pIrp );
}

gint32 CStmCpPdo::CancelFuncIrp(
    IRP* pIrp, bool bForce )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        ret = RemoveIrpFromMap( pIrp );
        if( ERROR( ret ) )
            break;

        CStdRMutex oIrpLock( pIrp->GetLock() );
        ret = pIrp->CanContinue( IRP_STATE_READY );
        if( ERROR( ret ) )
            break;

        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        pCtx->SetStatus( ret = ERROR_CANCEL );

        oIrpLock.Unlock();
        super::CancelFuncIrp( pIrp, bForce );

    }while( 0 );

    return ret;
}

gint32 CStmCpPdo::SubmitIoctlCmd(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    // let's process the func irps
    IrpCtxPtr pCtx = pIrp->GetCurCtx();
    do{
        if( pIrp->MajorCmd() != IRP_MJ_FUNC
            || pIrp->MinorCmd() != IRP_MN_IOCTL )
        {
            ret = -EINVAL;
            break;
        }

        switch( pIrp->CtrlCode() )
        {
        case CTRLCODE_LISTENING:
            {
                // server side I/O
                ret = HandleListening( pIrp );
                break;
            }
        case CTRLCODE_STREAM_CMD:
            {
                ret = HandleStreamCommand( pIrp );
                break;
            }
        default:
            {
                ret = -ENOTSUP;
                break;
            }
        }

    }while( 0 );

    if( ret != STATUS_PENDING )
        pCtx->SetStatus( ret );

    return ret;
}

gint32 CStmCpPdo::HandleListening(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    // let's process the func irps
    IrpCtxPtr& pCtx = pIrp->GetTopStack();
    bool bStarter = this->IsStarter();
    CStmConnPointHelper oh(
        m_pStmCp, bStarter );

    return  oh.SubmitListeningIrp( pIrp );
}

gint32 CStmCpPdo::HandleStreamCommand(
    IRP* pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = 0;
    // let's process the func irps
    do{

        CStdRMutex oPortLock( GetLock() );
        IrpCtxPtr& pCtx = pIrp->GetTopStack();

        bool bStarter = this->IsStarter(); 
        BufPtr pBuf = pCtx->m_pReqData;
        if( pBuf.IsEmpty() || pBuf->empty() )
        {
            ret = -EINVAL;
            break;
        }

        CStmConnPointHelper oh(
            m_pStmCp, bStarter );

        guint8 cToken = pBuf->ptr()[ 0 ];
        switch( cToken )
        {
        case tokPong:
        case tokPing:
        case tokProgress:
        case tokClose:
            ret = oh.Send( pBuf );
            break;

        case tokLift:
        case tokFlowCtrl:
        default:
            {
                ret = -EINVAL;
                break;
            }
        }

    }while( 0 );

    return ret;
}

gint32 CStmCpPdo::OnSubmitIrp(
    IRP* pIrp )
{

    gint32 ret = 0;
    do{
        if( pIrp == nullptr ||
            pIrp->GetStackSize() == 0 )
        {
            ret = -EINVAL;
            break;
        }

        // let's process the func irps
        if( pIrp->MajorCmd() != IRP_MJ_FUNC )
        {
            ret = -EINVAL;
            break;
        }

        switch( pIrp->MinorCmd() )
        {
        case IRP_MN_READ:
            {
                ret = SubmitReadIrp( pIrp );
                break;
            }
        case IRP_MN_WRITE:
            {
                ret = SubmitWriteIrp( pIrp );
                break;
            }
        case IRP_MN_IOCTL:
            {
                switch( pIrp->CtrlCode() )
                {
                case CTRLCODE_LISTENING:
                case CTRLCODE_STREAM_CMD:
                    {
                        ret = SubmitIoctlCmd( pIrp );
                        break;
                    }
                default:
                    {
                        ret = -ENOTSUP;
                        break;
                    }
                }

                if( ret != -ENOTSUP )
                    break;
                // fall through
            }
        default:
            {
                ret = PassUnknownIrp( pIrp );
                break;
            }
        }

    }while( 0 );

    if( ret != STATUS_PENDING )
    {
        pIrp->GetCurCtx()->SetStatus( ret );
    }

    return ret;
}

gint32 CStmCpPdo::SubmitWriteIrp(
    PIRP pIrp )
{
    gint32 ret = 0;
    bool bNotify = false;

    do{
        if( pIrp == nullptr ||
            pIrp->GetStackSize() == 0 )
        {
            ret = -EINVAL;
            break;
        }
        bool bStarter = IsStarter();
        CStmConnPointHelper oh(
            m_pStmCp, bStarter );

        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        BufPtr& pPayload = pCtx->m_pReqData;
        if( pPayload.IsEmpty() ||
            pPayload->empty() )
        {
            ret = -EINVAL;
            break;
        }

        BufPtr pNewBuf;
        guint8 byToken = tokData;
        guint32 dwSize =
            htonl( pPayload->size() ); 

        if( bStarter ||
            pPayload->offset() < UXPKT_HEADER_SIZE )
        {
            pNewBuf.NewObj();

            ret = pNewBuf->Append( &byToken, 1 );
            if( ERROR( ret ) )
                break;

            ret = pNewBuf->Append(
                ( guint8* )&dwSize,
                sizeof( dwSize ) );

            if( ERROR( ret ) )
                break;

            ret = pNewBuf->Append(
                pPayload->ptr(),
                pPayload->size() );

            if( ERROR( ret ) )
                break;
        }
        else
        {
            // non-starter, that is, from router 
            guint32 dwNewOff = 
                pPayload->offset() - UXPKT_HEADER_SIZE;
            pPayload->SetOffset( dwNewOff );

            pPayload->ptr()[ 0 ] = byToken;
            char* dest = pPayload->ptr() + 1;
            char* src = ( char* )&dwSize;
            dest[ 0 ] = src[ 0 ];
            dest[ 1 ] = src[ 1 ];
            dest[ 2 ] = src[ 2 ];
            dest[ 3 ] = src[ 3 ];
            pNewBuf = pPayload;
        }

        ret = oh.Send( pNewBuf );

    }while( 0 );

    return ret;
}

gint32 CStmCpPdo::PreStop( IRP* pIrp )
{
    CStmConnPoint* pscp = m_pStmCp;
    if( pscp != nullptr )
        pscp->Stop( IsStarter() );

    return super::PreStop( pIrp );
}

gint32 CStmCpPdo::PostStart( IRP* pIrp ) 
{
    return super::PostStart( pIrp );
}

gint32 CStreamQueue::RescheduleTask(
    TaskletPtr pTask )
{
    if( pTask.IsEmpty() )
        return -EINVAL;
    CIoManager* pMgr =
        GetParent()->GetIoMgr();
    
    CThreadPools& oPools =
        pMgr->GetThreadPools();

    ThreadPtr pth;
    gint32 ret = oPools.GetThread(
        UXSOCK_TAG, pth );

    if( ERROR( ret ) )
        return ret;

    CTaskThread* ptth = pth;
    if( ptth == nullptr )
        return -EFAULT;
    pTask->MarkPending();
    ptth->AddTask( pTask );
    return STATUS_SUCCESS;
}

}
