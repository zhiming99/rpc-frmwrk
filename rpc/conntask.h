/*
 * =====================================================================================
 *
 *       Filename:  conntask.h
 *
 *    Description:  defination of the template class CStmSockConnectTaskBase 
 *
 *        Version:  1.0
 *        Created:  11/13/2019 04:30:02 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi(woodhead99@gmail.com )
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
#include <vector>

namespace rpcfrmwrk
{

template< class SockClass >
class CStmSockConnectTaskBase
    : public CTasklet
{
    protected:
    // NOTE: this task is not meant to be
    // scheduled. it is an object for all the
    // related resources to play when the
    // connection is being established
    //
    // parameters:
    // propIoMgr
    // propSockPtr
    // propRetries
    // propIrpPtr
    enum ConnStat
    {
        connInit,
        connProcess,
        connCompleted,
        connStopped,
    };

    ConnStat m_iConnState;
    mutable stdrmutex m_oLock;
    gint32 m_iTimerId;
    sem_t  m_semInitSync;

    // eventStart from Pdo's PostStart
    gint32 OnStart( IRP* pIrp )
    {
        if( pIrp == nullptr ||
            pIrp->GetStackSize() == 0 )
            return -EINVAL;

        SetConnState( connProcess );

        gint32 ret = 0;
        do{
            CCfgOpener oCfg( ( IConfigDb* )m_pCtx );
            ret = oCfg.SetObjPtr( propIrpPtr,
                ObjPtr( pIrp ) );

            SockClass* pSock = nullptr;
            ret = GetSockPtr( pSock );
            if( ERROR( ret ) )
                break;

            ret = AddConnTimer();
            if( ERROR( ret ) )
                break;

            ret = pSock->Start();
            if( ret != STATUS_PENDING )
            {
                RemoveConnTimer();
            }

        }while( 0 );
        
        return ret;
    }

    // request from the CRpcStreamSock::OnError
    gint32 OnError( gint32 iRet )
    {
        gint32 ret = 0;
        switch( -iRet )
        {
        case ETIMEDOUT:
            {
                ret = Retry();
                break;
            }
        /* Connection refused */
        case ECONNREFUSED:
        /* Host is down */
        case EHOSTDOWN:
        /* No route to host */
        case EHOSTUNREACH:
            {
                // retry
                ret = iRet;
                break;
            }
        /* Operation already in progress */
        case EALREADY:
        /* Operation now in progress */
        case EINPROGRESS:
            {
                ret = STATUS_PENDING;
                break;
            }
        default:
            {
                ret = iRet;
            }
            break;;
        }
        return ret;
    }

    // eventNewConn from the
    // CRpcStreamSock::DispatchSockEvent
    gint32 OnConnected()
    {
        SockClass* pSock = nullptr;
        gint32 ret = GetSockPtr( pSock );
        if( ERROR( ret ) )
            return ret;

        return pSock->Start_bh();
    }

    // internal helpers
    gint32 Retry()
    {
        gint32 ret = DecRetries( nullptr );

        if( ERROR( ret ) )
            return -ETIMEDOUT;

        SockClass* pSock = nullptr;
        ret = GetSockPtr( pSock );
        if( ERROR( ret ) )
            return ret;

        AddConnTimer();
        ret = pSock->ActiveConnect();
        if( ret != STATUS_PENDING )
        {
            RemoveConnTimer();
        }

        return ret;
    }

    gint32 SetSockState(
        EnumSockState iState )
    {
        SockClass* pSock = nullptr;
        gint32 ret = GetSockPtr( pSock );
        if( ERROR( ret ) )
            return ret;

        return pSock->SetState( iState );
    }

    gint32 CompleteTask( gint32 iRet )
    {
        IRP* pIrp = nullptr;
        gint32 ret = 0;

        if( GetConnState() != connProcess )
            return ERROR_STATE;

        do{
            RemoveConnTimer();
            ret = GetMasterIrp( pIrp );
            if( ERROR( ret ) )
                break;

            if( pIrp == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            else
            {
                CStdRMutex oIrpLock(
                    pIrp->GetLock() );

                ret = pIrp->CanContinue(
                    IRP_STATE_READY);

                if( ERROR( ret ) )
                    break;

                IrpCtxPtr& pCtx =
                    pIrp->GetTopStack();

                pCtx->SetStatus( iRet );
            }

            CIoManager* pMgr = GetIoMgr();
            if( pMgr == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            pMgr->CompleteIrp( pIrp );

        }while( 0 );

        if( SUCCEEDED( iRet ) )
            SetConnState( connCompleted );
        else
            SetConnState( connStopped );

        m_pCtx->RemoveProperty( propIrpPtr );
        m_pCtx->RemoveProperty( propSockPtr );
        m_pCtx->RemoveProperty( propIoMgr );

        return ret;
    }
    gint32 StopTask( gint32 iRet )
    {
        if( GetConnState() != connProcess )
            return ERROR_STATE;

        SockClass* pSock = nullptr;
        gint32 ret = GetSockPtr( pSock );

        if( SUCCEEDED( ret ) )
        {
            pSock->Stop();
        }

        CompleteTask( iRet );
        return 0;
    }

    gint32 GetMasterIrp( IRP*& pIrp )
    { 
        CCfgOpener oCfg( ( IConfigDb* )m_pCtx );
        ObjPtr pObj;

        gint32 ret = oCfg.GetObjPtr(
            propIrpPtr, pObj );

        if( ERROR( ret ) )
            return ret;

        pIrp = pObj;
        if( pIrp == nullptr )
            return -EFAULT;

        return 0;
    }

    gint32 DecRetries( guint32* pdwRetries )
    {
        CCfgOpener oCfg( ( IConfigDb* )m_pCtx );

        guint32 dwRetries = 0;

        gint32 ret = oCfg.GetIntProp(
            propRetries, dwRetries );

        if( ERROR( ret ) )
            return ret;

        if( dwRetries == 0 )
            return -EOVERFLOW;

        --dwRetries;

        ret = oCfg.SetIntProp(
            propRetries, dwRetries );

        if( ERROR( ret ) )
            return ret;

        if( pdwRetries != nullptr )
            *pdwRetries = dwRetries;

        return 0;
    }
    gint32 GetRetries( guint32& dwRetries )
    {
        CCfgOpener oCfg( ( IConfigDb* )m_pCtx );

        gint32 ret = oCfg.GetIntProp(
            propRetries, dwRetries );

        if( ERROR( ret ) )
            return ret;

        return 0;
    }

    gint32 GetSockPtr( SockClass*& pSock )
    {
        CCfgOpener oCfg( ( IConfigDb* )m_pCtx );
        ObjPtr pObj;

        gint32 ret = oCfg.GetObjPtr(
            propSockPtr, pObj );

        if( ERROR( ret ) )
            return ret;

        pSock = pObj;
        if( pSock == nullptr )
            return -EFAULT;

        return 0;
    }

    gint32 SetConnState( ConnStat iStat )
    {
        m_iConnState = iStat;
        return 0;
    }

    gint32 RemoveConnTimer()
    {
        gint32 ret = 0;
        do{
            if( m_iTimerId == 0 )
                break;

            CIoManager* pMgr = GetIoMgr();
            if( unlikely( pMgr == nullptr ) )
            {
                ret = -EFAULT;
                break;
            }

            CTimerService& otsvc =
                pMgr->GetUtils().GetTimerSvc();

            otsvc.RemoveTimer( m_iTimerId );
            m_iTimerId = 0;

        }while( 0 );

        return ret;
    }

    gint32 AddConnTimer()
    {
        gint32 ret = 0;
        do{
            // the connection could be long, restart
            // the irp timer.
            PIRP pIrp = nullptr;
            CCfgOpener oCfg(
                ( IConfigDb* )GetConfig() );

            ret = oCfg.GetPointer( propIrpPtr, pIrp );
            if( ERROR( ret ) )
                break;

            pIrp->ResetTimer();
            CIoManager* pMgr = GetIoMgr();
            if( unlikely( pMgr == nullptr ) )
            {
                ret = -EFAULT;
                break;
            }

            guint32 dwTimeWait =
                pIrp->m_dwTimeoutSec;

            if( unlikely( dwTimeWait == 0 ) )
                dwTimeWait = PORT_START_TIMEOUT_SEC - 5;

            CTimerService& otsvc =
                pMgr->GetUtils().GetTimerSvc();

            m_iTimerId = otsvc.AddTimer(
                dwTimeWait, this, eventRetry );

        }while( 0 );

        return ret;
    }

    ConnStat GetConnState()
    { return m_iConnState; }

    public:
    typedef CTasklet super;

    CStmSockConnectTaskBase( const IConfigDb* pCfg )
        : super( pCfg )
    {
        m_iConnState = connInit;
        m_iTimerId = 0;
        Sem_Init( &m_semInitSync, 0, 0 );
        SetError( STATUS_PENDING );
    }
    ~CStmSockConnectTaskBase()
    {
        if( m_iTimerId != 0 )
            RemoveConnTimer();
        sem_destroy( &m_semInitSync );
    }

    stdrmutex& GetLock() const
    { return m_oLock; }

    CIoManager* GetIoMgr()
    {
        CCfgOpener oCfg( ( IConfigDb* )m_pCtx );
        ObjPtr pObj;

        CIoManager* pMgr = nullptr;
        gint32 ret = oCfg.GetPointer( propIoMgr, pMgr );

        if( ERROR( ret ) )
            return nullptr;

        return pMgr;
    }

    // event accepted:
    //  ---pdo events---
    //  eventStart
    //  eventCancelTask
    //  eventStop
    //
    // ---libc socket events ---
    //  eventConnErr
    //  eventNewConn
    //
    gint32 operator()( guint32 dwContext )
    {
        gint32 ret = 0;

        EnumEventId iEvent =
            ( EnumEventId )dwContext; 

        CStdRMutex oTaskLock( GetLock() );

        gint32 iState = GetConnState();

        if( iState == connStopped ||
            iState == connCompleted )
            return ERROR_STATE;

        SockClass* pSock = nullptr;
        ret = GetSockPtr( pSock );

        if( ERROR( ret ) )
            return ret;

        switch( iEvent )
        {
        case eventStart:
            {
                IrpPtr pIrp;
                if( iState != connInit )
                {
                    ret = ERROR_STATE;
                    break;
                }
                ret = GetIrpFromParams( pIrp );
                if( ERROR( ret ) )
                    break;

                ret = OnStart( pIrp );
                if( SUCCEEDED( ret ) )
                {
                    ret = OnConnected();
                }
                break;
            }
        case eventNewConn:
            {
                if( iState != connProcess )
                {
                    ret = ERROR_STATE;
                    break;
                }
                guint32 dwConnFlags = ( G_IO_OUT );

                std::vector< LONGWORD > vecParams;
                ret = GetParamList( vecParams );
                if( ERROR( ret ) )
                    break;

                guint32 dwCondition = vecParams[ 1 ];

                if( dwCondition == dwConnFlags )
                    ret = OnConnected();
                else
                    ret = ERROR_STATE;

                break;
            }
        case eventConnErr:
            {
                if( iState != connProcess )
                {
                    ret = ERROR_STATE;
                    break;
                }
                std::vector< LONGWORD > vecParams;
                ret = GetParamList( vecParams );
                if( ERROR( ret ) )
                    break;

                guint32 dwParam1 = vecParams[ 1 ];
                ret = OnError( ( gint32 )dwParam1 ); 

                break;
            }

        // from CPort::PreStop
        case eventStop:
            {
                ret = ERROR_PORT_STOPPED;
                break;
            }
        case eventTimeout:
            {
                m_iTimerId = 0;
                std::vector< LONGWORD > vecParams;
                ret = GetParamList( vecParams );
                if( ERROR( ret ) )
                    break;

                EnumEventId iSubEvt =
                    ( EnumEventId )vecParams[ 1 ];
                if( iSubEvt != eventRetry )
                {
                    ret = -ENOTSUP;
                    break;
                }

                ret = OnError( -ETIMEDOUT );
                break;
            }
        // only from CTcpStreamPdo::CancelStartIrp
        case eventCancelTask:
            {
                // remove the master irp, to prevent
                // the irp from being completed again
                m_pCtx->RemoveProperty( propIrpPtr );

                std::vector< LONGWORD > vecParams;
                ret = GetParamList( vecParams );
                if( ERROR( ret ) )
                    break;
                ret = ( gint32 )vecParams[ 1 ];
                break;
            }
        default:
            {
                ret = -ENOTSUP;
                break;
            }
        }

        if( SUCCEEDED( ret ) )
        {
            CompleteTask( ret );
        }
        else if( ERROR( ret ) )
        {
            StopTask( ret );
        }

        // BUGBUG: even STATE_PENDING needs to be set
        // to let the caller know the call result. the
        // return value is not passed on by
        // CTasklet::OnEvent 
        return SetError( ret );
    }

    static gint32 NewTask( CIoManager* pMgr,
        SockClass* pSock,
        gint32 iRetries,
        TaskletPtr& pTask,
        EnumClsid iClsid )
    {
        CCfgOpener oCfg;
        if( pMgr == nullptr ||
            pSock == nullptr ||
            iRetries <= 0 )
            return -EINVAL;

        gint32 ret = 0;

        do{
            oCfg.SetPointer( propIoMgr, pMgr );

            ret = oCfg.SetObjPtr(
                propSockPtr, ObjPtr( pSock ) );

            if( ERROR( ret ) )
                break;

            ret = oCfg.SetIntProp(
                propRetries, iRetries );

            if( ERROR( ret ) )
                break;

            ret = pTask.NewObj(
                iClsid, oCfg.GetCfg() );

        }while( 0 );

        return ret;
    }

    void SetInitDone()
    { Sem_Post( &m_semInitSync ); }

    void WaitInitDone()
    { Sem_Wait( &m_semInitSync ); }

};

}
