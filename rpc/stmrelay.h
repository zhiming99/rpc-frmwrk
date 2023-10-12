/*
 * =====================================================================================
 *
 *       Filename:  stmrelay.h
 *
 *    Description:  declaration of CStreamProxyRelay and CStreamServerRelay
 *
 *        Version:  1.0
 *        Created:  07/10/2019 04:19:46 PM
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

#include "uxstream.h"
#include "stream.h"
#include "stmcp.h"
#include "tcpport.h"


namespace rpcf
{

template< class T >
class CStreamRelayBase :
    public T
{
    protected:

    // maps between the stream id and the stream handle
    std::map< HANDLE, gint32 > m_mapHandleToStmId;
    std::map< gint32, HANDLE > m_mapStmIdToHandle;

    // the irps to receive the packet from a stream
    std::map< gint32, guint32 > m_mapStmPendingBytes;

    public:

    typedef T super;
    using IStream::OnClose;

    CStreamRelayBase( const IConfigDb* pCfg )
        : super::_MyVirtBase( pCfg ), super( pCfg )
    {}

    // data is ready for reading
    virtual gint32 OnStmRecv(
        HANDLE hChannel, BufPtr& pBuf )
    { return 0; }

    // the local sock is closed
    gint32 OnClose( HANDLE hChannel,
        IEventSink* pCallback = nullptr ) override
    {
        gint32 ret = 0;
        if( hChannel == INVALID_HANDLE )
            return -EINVAL;

        do{
            gint32 iStmId = -1;
            CStdRMutex oIfLock( this->GetLock() );

            std::map< HANDLE, gint32 >::iterator
                itr = m_mapHandleToStmId.find( hChannel );

            if( itr != m_mapHandleToStmId.end() )
            {
                iStmId = itr->second;
                RemoveBinding( hChannel, iStmId );
            }
            oIfLock.Unlock();

            if( iStmId >= 0 )
            {
                CloseTcpStream( iStmId, true );
                ret = this->CloseChannel(
                    hChannel, pCallback );
                // don't return pending since the
                // caller is an IFCALL task. it may
                // cause caller task wait endlessly
                //
                if( ret == STATUS_PENDING )
                    ret = 0;
            }

        }while( 0 );

        return ret;
    }

    // the peer request to close
    virtual gint32 OnClose( gint32 iStmId,
        IEventSink* pCallback = nullptr )
    {
        gint32 ret = 0;
        if( iStmId < 0 )
            return -EINVAL;

        if( iStmId == TCP_CONN_DEFAULT_CMD ||
            iStmId == TCP_CONN_DEFAULT_STM )
            return -EINVAL;

        do{
            HANDLE hChannel = INVALID_HANDLE;
            CStdRMutex oIfLock( this->GetLock() );
            std::map< gint32, HANDLE >::iterator
                itr = m_mapStmIdToHandle.find( iStmId );

            if( itr != m_mapStmIdToHandle.end() )
            {
                hChannel = itr->second;
                RemoveBinding( hChannel, iStmId );
            }
            oIfLock.Unlock();

            // no need to send
            if( hChannel != INVALID_HANDLE )
            {
                CloseTcpStream( iStmId, false );
                ret = this->CloseChannel(
                    hChannel, pCallback );
                if( ret == STATUS_PENDING )
                    ret = 0;
            }

        }while( 0 );

        return ret;
    }

    // error happens on the local sock 
    virtual gint32 OnChannelError(
        HANDLE hChannel, gint32 iError )
    { return OnClose( hChannel ); }

    // error happens on the remote( tcp ) sock 
    gint32 OnChannelError(
        gint32 iStmId, gint32 iError )
    { return OnClose( iStmId ); }

    // stream over tcp sock
    guint32 GetPendingWrite(
        gint32 iStmId ) const
    {
        CStdRMutex oIfLock( this->GetLock() );
        std::map< gint32, guint32 >::iterator
            itr = m_mapStmPendingBytes.find( iStmId );
        if( itr != m_mapStmPendingBytes.end() )
            return itr->second;
        return 0;
    }

    // stream over tcp sock
    gint32 AddPendingWrite(
        gint32 iStmId, guint32 dwBytes )
    {
        CStdRMutex oIfLock( this->GetLock() );
        std::map< gint32, guint32 >::iterator
            itr = m_mapStmPendingBytes.find( iStmId );
        if( itr == m_mapStmPendingBytes.end() )
            return -ENOENT;
        itr->second += dwBytes;
        return 0;
    }

    gint32 SubPendingWrite(
        gint32 iStmId, guint32 dwBytes )
    {
        CStdRMutex oIfLock( this->GetLock() );
        std::map< gint32, guint32 >::iterator
            itr = m_mapStmPendingBytes.find( iStmId );
        if( itr == m_mapStmPendingBytes.end() )
            return -ENOENT;
        itr->second -= dwBytes;
        return 0;
    }

    gint32 OnBdgeStmEvent( gint32 iStmId,
        guint8 byToken, CBuffer* pBuf )
    {
        if( iStmId == TCP_CONN_DEFAULT_STM ||
            iStmId == TCP_CONN_DEFAULT_CMD )
            return -EINVAL;

        gint32 ret = 0;
        switch( byToken )
        {
        case tokError:
            {
                if( pBuf == nullptr ||
                    pBuf->empty() )
                {
                    ret = -EINVAL;
                    break;
                }
                guint32 dwError = ntohl(
                    ( guint32&)*pBuf );
                ret = OnChannelError( iStmId,
                    ( gint32& )dwError );
                break;
            }
        case tokClose:
            {
                ret = OnClose( iStmId );
                break;
            }
        default:
            ret = -ENOTSUP;
            break;
        }
        return ret;
    }

    // return STATUS_SUCCESS if iStmId is a valid
    // stream
    gint32 IsTcpStmExist( gint32 iStmId )
    {
        if( iStmId < 0 )
            return -EINVAL;

        if( iStmId == TCP_CONN_DEFAULT_STM ||
            iStmId == TCP_CONN_DEFAULT_CMD )
            return STATUS_SUCCESS;

        gint32 ret = 0;
        do{
            PortPtr ptrPort;
            CCfgOpener oReq;
            oReq.SetBoolProp( propSubmitPdo, true );
            ret = this->GetPortToSubmit(
                oReq.GetCfg(), ptrPort );

            CPort* pPort = ptrPort;
            if( unlikely( pPort == nullptr ) )
            {
                ret = -EFAULT;
                break;
            }

            IrpPtr pIrp( true );
            ret = pIrp->AllocNextStack( nullptr );
            if( ERROR( ret ) )
                break;

            IrpCtxPtr& pIrpCtx = pIrp->GetTopStack(); 
            pIrpCtx->SetMajorCmd( IRP_MJ_FUNC );
            pIrpCtx->SetMinorCmd( IRP_MN_IOCTL );
            pIrpCtx->SetCtrlCode( CTRLCODE_GET_RMT_STMID );
            BufPtr pBuf( true );
            CParamList oParams;
            oParams.Push( iStmId );
            *pBuf = ObjPtr( oParams.GetCfg() );
            pIrpCtx->SetReqData( pBuf );
            pPort->AllocIrpCtxExt( pIrpCtx );
            CIoManager* pMgr = this->GetIoMgr();

            ret = pMgr->SubmitIrpInternal(
                pPort, pIrp, false );

        }while( 0 );

        return ret;
    }

    gint32 BindUxTcpStream(
        HANDLE hChannel, gint32 iStmId )
    {
        gint32 ret = 0;
        do{
            CStdRMutex oLock( this->GetLock() );
            if( m_mapHandleToStmId.find( hChannel ) ==
                m_mapHandleToStmId.end() )
            {
                m_mapHandleToStmId[ hChannel ] = iStmId;
                m_mapStmIdToHandle[ iStmId ] = hChannel;
                break;
            }
            ret = -EEXIST;

        }while( 0 );

        return ret;
    }

    gint32 RemoveBinding(
        HANDLE hChannel, gint32 iStmId )
    {
        gint32 ret = 0;
        do{
            CStdRMutex oLock( this->GetLock() );
            std::map< HANDLE, gint32 >::iterator
                itr = m_mapHandleToStmId.find( hChannel );
            if( itr == m_mapHandleToStmId.end() )
            {
                ret = -ENOENT;
                break;
            }
            else if( itr->second != iStmId )
            {
                ret = -ENOENT;
                break;
            }

            m_mapHandleToStmId.erase( itr );
            m_mapStmIdToHandle.erase( iStmId );
            break;

        }while( 0 );

        return ret;
    }

    gint32 CloseTcpStream( gint32 iStmId, bool bSend = true )
    {
        if( iStmId < 0 )
            return -EINVAL;

        gint32 ret = 0;
        if( !bSend )
        {
            if( this->IsServer() )
            {
                CRpcTcpBridge* pSvr =
                    ObjPtr( this );

                ret = pSvr->CloseLocalStream(
                    nullptr, iStmId );
            }
            else
            {
                CRpcTcpBridgeProxy* pProxy =
                    ObjPtr( this );

                ret = pProxy->CloseLocalStream(
                    nullptr, iStmId );
            }
            return ret;
        }

        do{
            IPort* pPort = nullptr;
            TaskletPtr pCloseStream;
            ret = DEFER_IFCALLEX_NOSCHED(
                pCloseStream, ObjPtr( this ),
                &CRpcTcpBridgeShared::CloseLocalStream,
                pPort, iStmId );

            if( ERROR( ret ) )
                break;
                
            // write a close token to the 
            TaskletPtr pSendClose;
            BufPtr pBuf( true );
            *pBuf = ( guint8 )tokClose;
            ret = DEFER_IFCALLEX_NOSCHED2(
                3, pSendClose, ObjPtr( this ),
                &CRpcTcpBridgeShared::WriteStream,
                iStmId, *pBuf, 1, ( IEventSink* )nullptr );

            if( ERROR( ret ) )
            {
                ( *pCloseStream )( eventCancelTask );
                break;
            }

            ret = this->AddSeqTask( pSendClose );
            if( ERROR( ret ) )
            {
                ( *pSendClose )( eventCancelTask );
                break;
            }
            ret = this->AddSeqTask( pCloseStream );
            if( ERROR( ret ) )
            {
                ( *pSendClose )( eventCancelTask );
                ( *pCloseStream )( eventCancelTask );
            }

        }while( 0 );

        return ret;
    }

};

// this interface will be hosted by
// CRpcTcpBridgeImpl
class CRpcRouter;
class CStreamServerRelay :
    public CStreamRelayBase< CStreamServer >
{
    public:

    typedef CStreamRelayBase super;
    CStreamServerRelay( const IConfigDb* pCfg )
        : super::_MyVirtBase( pCfg ), super( pCfg )
    {;}

    CRpcRouter* GetParent() const;

    virtual gint32 OpenChannel(
        IConfigDb* pDataDesc,
        int& fd, HANDLE& hChannel,
        IEventSink* pCallback ) 
    { return 0; }

    gint32 FetchData_Server(
        IConfigDb* pDataDesc,           // [in]
        gint32& fd,                     // [out]
        guint32& dwOffset,              // [in, out]
        guint32& dwSize,                // [in, out]
        IEventSink* pCallback );

    // request to the remote server completed
    gint32 OnFetchDataComplete(
        IEventSink* pCallback,
        IEventSink* pIoReqTask,
        IConfigDb* pContext );

    using IStream::OnConnected;
    gint32 OnConnected( HANDLE hChannel ) override
    { return 0; }

    gint32 OnPreStop(
        IEventSink* pContext ) override
    { return super::OnPreStop( pContext ); }
};

// this interface will be hosted by
// CRpcTcpBridgeProxyImpl
class CStreamProxyRelay :
    public CStreamRelayBase< CStreamProxy >
{
    protected:
    gint32 FetchData_Proxy(
        IConfigDb* pDataDesc,           // [in, out]
        gint32& fd,                     // [out]
        guint32& dwOffset,              // [in, out]
        guint32& dwSize,                // [in, out]
        IEventSink* pCallback );

    bool SupportIid( EnumClsid iid ) const
    {
        // support IStreamMH to allow the call to
        // FetchData_Proxy can come to this class.
        // the call is made only by the
        // CRpcReqForwarder
        if( iid == iid( IStreamMH ) )
            return true;
        return false;
    }

    public:

    typedef CStreamRelayBase< CStreamProxy > super;
    CStreamProxyRelay( const IConfigDb* pCfg )
        : super::_MyVirtBase( pCfg ), super( pCfg )
    {;}

    CRpcRouter* GetParent() const;
    const EnumClsid GetIid() const
    { return iid( IStream ); }

    virtual gint32 OpenChannel(
        IConfigDb* pDataDesc,
        int& fd, HANDLE& hChannel,
        IEventSink* pCallback ) 
    { return 0; }

    // request to the remote bridge completed
    virtual gint32 OnFetchDataComplete(
        IEventSink* pCallback,
        IEventSink* pIoReqTask,
        IConfigDb* pContext );

    virtual gint32 OnOpenStreamComplete(
        IEventSink* pCallback,
        IEventSink* pIoReqTask,
        IConfigDb* pContext );

    using IStream::OnConnected;
    gint32 OnConnected( HANDLE hChannel ) override
    { return 0; }

    gint32 OnPostStart(
        IEventSink* pContext ) override
    {
        CStatCountersProxy* psc = ObjPtr( this );    
        if( psc == nullptr )
            return -EFAULT;
        psc->SetCounter(
            propRxBytes, ( guint64 )0 );
        psc->SetCounter(
            propTxBytes, ( guint64 )0 );
        return 0;
    }
    gint32 OnPreStop(
        IEventSink* pContext ) override
    { return super::OnPreStop( pContext ); }
};

class CIfStartUxSockStmRelayTask :
    public CIfInterceptTaskProxy
{
    bool m_bServer = false;
    public:
    typedef CIfInterceptTaskProxy super;

    CIfStartUxSockStmRelayTask( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CIfStartUxSockStmRelayTask ) ); }

    gint32 RunTask();
    gint32 OnTaskComplete( gint32 iRet );
    gint32 OnCancel( guint32 dwContext ) override;
    void Cleanup();
};

template< class T >
class CUnixSockStmRelayBase :
    public T
{
    protected:

    TaskletPtr m_pWrTcpStmTask;
    TaskletPtr m_pRdTcpStmTask;
    TaskletPtr m_pWritingTask;

    // flow control stuffs
    //
    typedef std::pair< guint8, BufPtr > QUE_ELEM;
    typedef std::deque< std::pair< guint8, BufPtr > > SENDQUE;
    SENDQUE m_queToLocal;
    SENDQUE m_queToRemote;
    bool  m_bTcpFlowCtrl = false;

    public:
    typedef T super;

    inline guint32 QueueLimit()
    { return STM_MAX_PACKETS_REPORT + 4; }

    virtual gint32 SendBdgeStmEvent(
        guint8 byToken, BufPtr& pBuf )
    {
        // forward to the parent IStream object
        gint32 ret = 0;
        do{
            CParamList oParams;
            BufPtr ptrBuf = pBuf;
            oParams.Push( byToken );
            oParams.Push( ptrBuf );

            CCfgOpenerObj oCfg(
                ( CObjBase* )this );
            gint32 iStmId = -1;
            ret = oCfg.GetIntProp( propStreamId,
                ( guint32& )iStmId );
            
            if( ERROR( ret ) )
                break;

            TaskletPtr pTask;

            IStream* pStm = this->GetParent();
            CRpcServices* pSvc =
                pStm->GetInterface();

            if( pSvc->IsServer() )
            {
                CStreamServerRelay* pRelay =
                    ObjPtr( pSvc );

                ret = pRelay->OnBdgeStmEvent(
                    iStmId, byToken, pBuf );
            }
            else
            {
                CStreamProxyRelay* pRelay =
                    ObjPtr( pSvc );

                ret = pRelay->OnBdgeStmEvent(
                    iStmId, byToken, pBuf );
            }

        }while( 0 );

        return ret;
    }

    gint32 PauseResumeTask(
        IEventSink* pTask, bool bResume )
    {
        if( pTask == nullptr )
            return -EINVAL;

        CIfUxTaskBase* pTaskBase =
            ObjPtr( pTask );

        if( pTaskBase == nullptr )
            return -EFAULT;

        if( bResume )
            pTaskBase->Resume();
        else
            pTaskBase->Pause();

        return 0;
    }

    gint32 PauseResumeTasks(
        ObjPtr pTasks, bool bResume )
    {
        ObjVecPtr pvecTasks = pTasks;
        if( pvecTasks.IsEmpty() )
            return -EINVAL;

        for( auto elem : ( *pvecTasks )() )
        {
            IEventSink* pTask = elem;
            PauseResumeTask( pTask, bResume );
        }

        return 0;
    }

    gint32 ForwardToUrgent( SENDQUE* pQueue,
        guint8 byToken,
        BufPtr& pBuf,
        TaskletPtr& pResumeTask )
    {
        gint32 ret = 0;

        do{
            if( byToken != tokFlowCtrl &&
                byToken != tokLift &&
                byToken != tokClose )
            {
                ret = -EINVAL;
                break;
            }

            guint8 byHead = tokInvalid;

            if( !pQueue->empty() )
                byHead = pQueue->front().first;

            do{
                ret = 0;

                if( byHead == tokInvalid )
                    break;

                // continue to queue the token
                if( byToken == tokClose )
                    break;

                if( byHead == tokClose )
                {
                    // `close' is to send, no need of
                    // flow control any more
                    ret = -EEXIST;
                    break;
                }

                if( byHead != tokFlowCtrl &&
                    byHead != tokLift )
                    break;

                SENDQUE::iterator pre =
                    pQueue->begin();

                SENDQUE::iterator itr =
                    pre + 1;

                while( itr != pQueue->end() )
                {
                    byHead = itr->first;

                    if( byHead != tokFlowCtrl &&
                        byHead != tokLift )
                        break;

                    pre = itr;
                    ++itr;
                }

                byHead = pre->first;

                // stop to queue the token
                if( byHead == byToken )
                {
                    ret = -EEXIST;
                    break;
                }

                // the two are canceled
                if( byHead == tokFlowCtrl &&
                    byToken == tokLift )
                {
                    pre = pQueue->erase( pre );
                    ret = -EEXIST;
                    break;
                }

                // insert the token after all the
                // urgent tokens
                if( byHead == tokLift &&
                    byToken == tokFlowCtrl )
                {
                    if( itr == pQueue->end() )
                    {
                        pQueue->push_back(
                            QUE_ELEM( byToken, pBuf ) );
                    }
                    else
                    {
                        pQueue->insert( itr,
                            QUE_ELEM( byToken, pBuf ) );
                    }
                    ret = -EEXIST;
                    break;
                }

            }while( 0 );

            if( SUCCEEDED( ret ) )
            {
                pQueue->push_front(
                    QUE_ELEM( byToken, pBuf ) ); 
            }
            else if( ret != -EEXIST )
            {
                ret = 0;
                break;
            }

            // resume the writing task to send the
            // flowctrl token.
            CIoManager* pMgr = this->GetIoMgr();
            ret = DEFER_CALL( pMgr,
                ObjPtr( pResumeTask ),
                &CIfUxTaskBase::Resume );

        }while( 0 );

        return ret;
    }

    gint32 ForwardToRemoteUrgent(
        guint8 byToken, BufPtr& pBuf )
    {
        return ForwardToUrgent( &m_queToRemote,
            byToken, pBuf, m_pWrTcpStmTask );
    }

    gint32 ForwardToLocalUrgent(
        gint8 byToken, BufPtr& pBuf )
    {
        return ForwardToUrgent( &m_queToLocal,
            byToken, pBuf, m_pWritingTask );
    }

    gint32 ForwardToQueue(
        guint8 byToken, BufPtr& pBuf,
        SENDQUE& oQueue, SENDQUE& oQueUrgent,
        TaskletPtr& pTaskResume,
        TaskletPtr& pTaskUrgent )
    {
        gint32 ret = 0;

        do{
            CStdRMutex oIfLock( this->GetLock() );
            SENDQUE* pQueue =  &oQueue;
            bool bEmpty = pQueue->empty();

            pQueue->push_back(
                QUE_ELEM( byToken, pBuf ) ); 

            if( pQueue->size() > QueueLimit() )
            {
                ret = ERROR_QUEUE_FULL;
                break;
            }
            else if( pQueue->size() == QueueLimit() )
            {
                // return error, but the request is
                // queued still.
                TaskletPtr pTask;
                BufPtr pEmptyBuf( true );

                ForwardToUrgent( &oQueUrgent,
                    tokFlowCtrl, pEmptyBuf, pTaskUrgent );

                ret = ERROR_QUEUE_FULL;
                break;
            }

            if( bEmpty )
            {
                // wake up the writer task
                // use defer call to avoid lock nesting
                // among the tasks
                TaskletPtr pResumeTask;
                ret = DEFER_IFCALLEX_NOSCHED(
                    pResumeTask,
                    ObjPtr( this ),
                    &CUnixSockStmRelayBase::PauseResumeTask,
                    ( IEventSink* )pTaskResume, true );

                if( ERROR( ret ) )
                    break;

                oIfLock.Unlock();
                // cannot run directly, could deadlock
                TaskletPtr pDeferTask;
                ret = DEFER_CALL_NOSCHED( pDeferTask,
                    ObjPtr( this ),
                    &CRpcServices::RunManagedTask,
                    pResumeTask, false );

                if( ERROR( ret ) )
                {
                    ( *pResumeTask )( eventCancelTask );
                    break;
                }

                ret = this->GetIoMgr()->RescheduleTask(
                    pDeferTask );

                if( ERROR( ret ) )
                    ( *pResumeTask )( eventCancelTask );
            }

        }while( 0 );

        return ret;
    }

    gint32 ForwardToLocal(
        guint8 byToken, BufPtr& pBuf )
    {
        return ForwardToQueue( byToken, pBuf,
            m_queToLocal, m_queToRemote,
            m_pWritingTask, m_pWrTcpStmTask );
    }

    virtual gint32 ForwardToRemote(
        guint8 byToken, BufPtr& pBuf )
    {
        gint32 ret = 0;

        do{
            CStdRMutex oIfLock( this->GetLock() );
            SENDQUE* pQueue = &m_queToRemote;
            bool bEmpty = pQueue->empty();

            pQueue->push_back(
                QUE_ELEM( byToken, pBuf ) ); 

            if( pQueue->size() >= QueueLimit() )
            {
                // return the error, but the
                // request is queued still.
                //
                // unlike the remote end flow
                // control, there is no
                // ForwardToLocalUrgent since the
                // PauseReading will stop pipe
                // reading and thus the incoming
                // message will stop.
                ret = ERROR_QUEUE_FULL;
                break;
            }

            if( bEmpty )
            {
                // wake up the writer task
                // use defer call to avoid lock nesting
                // among the tasks
                //
                CIoManager* pMgr = this->GetIoMgr();
                // use defer call to avoid lock nesting
                // among the tasks
                ret = DEFER_CALL( pMgr,
                    ObjPtr( m_pWrTcpStmTask ),
                    &CIfUxTaskBase::Resume );
            }

        }while( 0 );

        return ret;
    }

    virtual gint32 GetReqToRemote(
        guint8& byToken, BufPtr& pBuf )
    {
        gint32 ret = 0;

        do{
            CIfUxTaskBase* pReadTask =
                this->m_pListeningTask;

            CStdRTMutex oTaskLock(
                pReadTask->GetLock() );

            CStdRMutex oIfLock( this->GetLock() );
            SENDQUE* pQueue = &m_queToRemote;

            if( pQueue->empty() )
            {
                ret = -ENOENT;
                break;
            }

            byToken = pQueue->front().first;
            pBuf = pQueue->front().second;

            if( m_bTcpFlowCtrl )
            {
                if( byToken != tokFlowCtrl &&
                    byToken != tokLift &&
                    byToken != tokClose )
                {
                    ret = -ENOENT;
                    break;
                }
            }

            pQueue->pop_front();

            if( pQueue->size() >= QueueLimit() )
                break;

            if( !pReadTask->IsPaused() )
                break;

            oIfLock.Unlock();

            PauseResumeTask( pReadTask, true );

        }while( 0 );

        return ret;
    }

    virtual gint32 PeekReqToLocal(
        guint8& byToken, BufPtr& pBuf )
    {
        gint32 ret = 0;

        do{
            CStdRMutex oIfLock( this->GetLock() );
            SENDQUE* pQueue = &m_queToLocal;

            if( !this->CanSend()  ||
                pQueue->empty() )
            {
                ret = -ENOENT;
                break;
            }

            byToken = pQueue->front().first;
            pBuf = pQueue->front().second;

        }while( 0 );

        return ret;
    }

    virtual gint32 GetReqToLocal(
        guint8& byToken, BufPtr& pBuf )
    {
        gint32 ret = 0;

        do{
            CIfUxTaskBase* pReadTask =
                m_pRdTcpStmTask;

            CStdRTMutex oReadLock(
                pReadTask->GetLock() );

            CStdRMutex oIfLock( this->GetLock() );
            SENDQUE* pQueue = &m_queToLocal;

            if( !this->CanSend()  ||
                pQueue->empty() )
            {
                ret = -ENOENT;
                break;
            }

            bool bFull =
                pQueue->size() >= QueueLimit();

            byToken = pQueue->front().first;
            pBuf = pQueue->front().second;
            pQueue->pop_front();

            if( pQueue->size() >= QueueLimit() )
                break;

            if( bFull )
            {
                // reading from tcp stream can resume
                BufPtr pEmptyBuf( true );
                ForwardToRemoteUrgent( tokLift, pEmptyBuf );
                oIfLock.Unlock();

                PauseResumeTask( pReadTask, true );

            }

        }while( 0 );

        return ret;
    }

    CUnixSockStmRelayBase( const IConfigDb* pCfg ) :
        super( pCfg )
    { this->m_oFlowCtrl.SetAsMonitor( true ); }

    gint32 PostTcpStmEvent(
        guint8 byToken, BufPtr& pBuf )
    {
        gint32 ret = 0;
        TaskletPtr pTask;
        ret = DEFER_IFCALLEX_NOSCHED( pTask,
            ObjPtr( this ),
            &CUnixSockStmRelayBase::OnTcpStmEvent,
            byToken, ( CBuffer* )pBuf );

        if( ERROR( ret ) )
            return ret;

        ret = this->AddSeqTask( pTask );
        if( ERROR( ret ) )
            ( *pTask )( eventCancelTask );

        return ret;
    }

    virtual gint32 OnPingPongRemote(
        bool bPing ) = 0;

    virtual gint32 OnTcpStmEvent(
        guint8 byToken, CBuffer* pBuf )
    {
        gint32 ret = 0;

        switch( byToken )
        {
        case tokPing:
        case tokPong:
            {
                ret = OnPingPongRemote(
                    tokPing == byToken );
                break;
            }
        case tokProgress:
            {
                ret = OnProgressRemote( pBuf );
                break;
            }
        case tokLift:
            {
                ret = OnFCLiftedRemote();
                break;
            }
        case tokFlowCtrl:
            {
                ret = OnFlowControlRemote();
                break;
            }
        case tokData:
            {
                ret = OnDataReceivedRemote( pBuf );
                break;
            }
        case tokClose:
        case tokError:
            {
                BufPtr bufPtr( pBuf );
                ret = SendBdgeStmEvent(
                    byToken, bufPtr );
                break;
            }
        }

        return ret;
    }

    gint32 OnPingPong( bool bPing ) override
    {
        // to cloak super::OnPingPong
        return 0;
    }

    virtual gint32 OnDataReceivedRemote(
        CBuffer* pBuf )
    { return 0; }

    gint32 OnFlowControl() override
    {
        this->m_oFlowCtrl.IncFCCount();
        CStdRMutex oIfLock( this->GetLock() );
        ObjPtr pObj = m_pWritingTask;
        if( pObj.IsEmpty() )
            return -EFAULT;

        CIfUxTaskBase* pTask =
            m_pWritingTask;

        if( pTask == nullptr )
            return -EFAULT;

        oIfLock.Unlock();
        return pTask->Pause();
    }

    gint32 OnFCLifted() override
    {
        this->m_oFlowCtrl.DecFCCount();
        CStdRMutex oIfLock( this->GetLock() );
        ObjPtr pObj = m_pWritingTask;
        if( pObj.IsEmpty() )
            return -EFAULT;

        CIfUxTaskBase* pTask =
            m_pWritingTask;

        if( pTask == nullptr )
            return -EFAULT;

        oIfLock.Unlock();
        return pTask->Resume();
    }

    virtual gint32 OnFlowControlRemote()
    {
        // this event comes over tcp stream
        CStdRMutex oIfLock( this->GetLock() );
        m_bTcpFlowCtrl = true;
        return 0;
    }

    virtual gint32 OnFCLiftedRemote()
    {
        // this event comes over tcp stream
        CStdRMutex oIfLock( this->GetLock() );
        m_bTcpFlowCtrl = false;

        ObjVecPtr pvecTasks( true );

        if( !m_queToRemote.empty() )
        {
            ( *pvecTasks )().push_back(
                ObjPtr( m_pWrTcpStmTask ) );
        }

        oIfLock.Unlock();
        ObjPtr pTasks = pvecTasks;

        // wake up ux stream reader task
        if( ( *pvecTasks )().empty() )
            return 0;

        return PauseResumeTasks( pTasks, true );
    }

    gint32 OnDataReceived( CBuffer* pBuf ) override
    {
        return 0;
    }

    virtual gint32 OnProgressRemote( CBuffer* pBuf )
    {
        return 0;
    }

    gint32 OnProgress( CBuffer* pBuf ) override
    {
        gint32 ret = 0;
        do{
            CfgPtr pReport( true );
            ret = pReport->Deserialize( *pBuf );
            if( ERROR( ret ) )
                break;

            this->m_oFlowCtrl.OnFCReport( pReport );

        }while( 0 );

        return ret;
    }

    gint32 StartTicking(
        IEventSink* pContext ) override
    {
        gint32 ret = 0;

        do{
            // start the event listening
            CParamList oParams;
            auto pMgr = this->GetIoMgr();

            oParams.SetPointer( propIfPtr, this );
            oParams.SetPointer( propIoMgr, pMgr );
            oParams.CopyProp( propTimeoutSec, this );

            ret = this->m_pListeningTask.NewObj(
                clsid( CIfUxListeningRelayTask ),
                oParams.GetCfg() );

            if( ERROR( ret ) )
                break;

            ret = this->RunManagedTask(
                this->m_pListeningTask );

            if( ERROR( ret ) )
                break;

            // start the ping ticker
            ret = this->m_pPingTicker.NewObj(
                clsid( CIfUxPingTicker ),
                oParams.GetCfg() );

            if( ERROR( ret ) )
                break;

            // RunManagedTask makes the
            // m_pPingTicker as a place holder in
            // the parallel task group, in order
            // which can avoid continuously
            // destroyed when the group is
            // temporarily empty, and waiting for
            // new tasks.
            ret = this->RunManagedTask(
                this->m_pPingTicker );

            if( ERROR( ret ) )
                break;

        }while( 0 );

        if( ERROR( ret ) )
        {
            if( !this->m_pListeningTask.IsEmpty() )
                ( *this->m_pListeningTask )(
                    eventCancelTask );

            if( !this->m_pPingTicker.IsEmpty() )
                ( *this->m_pPingTicker )(
                    eventCancelTask );
        }
        return ret;
    }

    gint32 OnPostStart(
        IEventSink* pContext ) override
    { return 0; }

    gint32 OnPostStartDeferred(
        IEventSink* pContext )
    {
        gint32 ret = 0;

        do{
            CIoManager* pMgr = this->GetIoMgr();
            ret = StartTicking( pContext );
            if( ERROR( ret ) )
                break;

            // start the data reading
            CParamList oParams;
            oParams.SetPointer( propIfPtr, this );
            oParams.SetPointer( propIoMgr, pMgr );

            oParams.CopyProp( propTimeoutSec, this );
            oParams.CopyProp(
                propKeepAliveSec, this );

            // ux stream writer
            oParams.Push( false );
            ret = m_pWritingTask.NewObj(
                clsid( CIfUxSockTransRelayTask ),
                oParams.GetCfg() );
            if( ERROR( ret ) )
                break;

            ret = this->RunManagedTask(
                this->m_pWritingTask );

            if( ERROR( ret ) )
                break;

            oParams.CopyProp( propStreamId, this );
            // tcp stream writer
            ret = m_pWrTcpStmTask.NewObj(
                clsid( CIfTcpStmTransTask ),
                oParams.GetCfg() );
            if( ERROR( ret ) )
                break;

            ret = this->RunManagedTask(
                m_pWrTcpStmTask );

            if( ERROR( ret ) )
                break;

            // tcp stream reader
            oParams.SetBoolProp( 0, true );
            ret = m_pRdTcpStmTask.NewObj(
                clsid( CIfTcpStmTransTask ),
                oParams.GetCfg() );
            if( ERROR( ret ) )
                break;

            ret = this->RunManagedTask(
                m_pRdTcpStmTask );

        }while( 0 );

        return ret;
    }

    gint32 ReportByteStat()
    {
        gint32 ret = 0;
        do{
            InterfPtr pParent;
            this->GetParent( pParent );
            if( pParent.IsEmpty() )
            {
                ret = -EFAULT;
                break;
            }
            guint64 qwRx, qwTx, qwVal;
            this->GetBytesTransfered(
                qwRx, qwTx );

            if( qwRx == 0 && qwTx == 0 )
                break;

            if( pParent->IsServer() )
            {
                CStatCountersServer* psc =
                    pParent;

                psc->GetCounter2(
                    propRxBytes, qwVal );
                qwVal += qwRx;

                psc->SetCounter(
                    propRxBytes, qwVal );

                psc->GetCounter2(
                    propTxBytes, qwVal );

                qwVal += qwTx;

                psc->SetCounter(
                    propTxBytes, qwVal );
            }
            else
            {
                CStatCountersProxy* psc =
                    pParent;

                psc->GetCounter2(
                    propRxBytes, qwVal );
                qwVal += qwRx;

                psc->SetCounter(
                    propRxBytes, qwVal );

                psc->GetCounter2(
                    propTxBytes, qwVal );

                qwVal += qwTx;

                psc->SetCounter(
                    propTxBytes, qwVal );
            }

        }while( 0 );

        return ret;
    }

    virtual gint32 OnPreStop( IEventSink* pCallback ) 
    {
        ReportByteStat();
        if( this->GetPortHandle() == INVALID_HANDLE )
        {
            CCfgOpenerObj oIfCfg( this );
            CStmConnPoint* pscp = nullptr;
            gint32 ret = oIfCfg.GetPointer(
                propStmConnPt, pscp );
            if( SUCCEEDED( ret ) )
                pscp->Stop( false );
        }

        super::OnPreStop( pCallback );

        if( !m_pWrTcpStmTask.IsEmpty() )
            ( *m_pWrTcpStmTask )( eventCancelTask );

        if( !m_pRdTcpStmTask.IsEmpty() )
            ( *m_pRdTcpStmTask )( eventCancelTask );

        if( !m_pWritingTask.IsEmpty() )
            ( *m_pWritingTask )( eventCancelTask );

        return 0;
    }

    gint32 WriteStream( BufPtr& pBuf,
        IEventSink* pCallback )
    {
        // this is a version without flow-control
        gint32 ret = 0;
        if( pBuf.IsEmpty() || pBuf->empty() )
            return -EINVAL;

        if( pCallback == nullptr )
            return -EINVAL;

        if( !this->IsConnected() )
            return ERROR_STATE;

        do{
            CParamList oParams;
            oParams[ propMethodName ] = __func__;
            oParams.Push( pBuf );

            ret = this->SubmitRequest(
                oParams.GetCfg(), pCallback, true );

            IncTxBytes( pBuf->size() );

        }while( 0 );

        return ret;
    }

    inline EnumFCState IncTxBytes( guint32 dwSize )
    { return this->m_oFlowCtrl.IncTxBytes( dwSize ); }

    inline EnumFCState IncRxBytes( guint32 dwSize )
    { return this->m_oFlowCtrl.IncRxBytes( dwSize ); }
    
    gint32 OnPostStop(
        IEventSink* pContext ) override
    { return super::OnPostStop( pContext ); }
};

class CUnixSockStmServerRelay :
    public CUnixSockStmRelayBase< CUnixSockStmServer >
{
    public:
    typedef CUnixSockStmRelayBase< CUnixSockStmServer > super;

    CUnixSockStmServerRelay( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CUnixSockStmServerRelay ) ); }

    gint32 OnPingPong( bool bPing ) override;
    gint32 OnPingPongRemote( bool bPing ) override;
    IStream* GetParent() override;
};

class CUnixSockStmProxyRelay :
    public CUnixSockStmRelayBase< CUnixSockStmProxy >
{
    public:
    typedef CUnixSockStmRelayBase< CUnixSockStmProxy > super;

    CUnixSockStmProxyRelay( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CUnixSockStmProxyRelay ) ); }

    gint32 OnPingPong( bool bPing ) override;
    gint32 OnPingPongRemote( bool bPing ) override;
    IStream* GetParent() override;

    inline bool GetOvershoot( guint64& qwBytes,
        guint64& qwPkts ) const
    {
        return m_oFlowCtrl.GetOvershoot(
            qwBytes, qwPkts );
    }
    gint32 OnDataReceivedRemote(
        CBuffer* pBuf ) override;
};

struct CIfUxRelayTaskHelper
{
    CRpcServices* m_pSvc;
    CIfUxRelayTaskHelper( IEventSink* pTask );

    gint32 SetTask( IEventSink* pTask );
    virtual gint32 GetBridgeIf( InterfPtr& pIf );

    // CUnixSockStmRelayBase methods
    gint32 ForwardToRemote(
        guint8 byToken, BufPtr& pBuf );

    gint32 ForwardToLocal(
        guint8 byToken, BufPtr& pBuf );

    gint32 GetReqToRemote(
        guint8& byToken, BufPtr& pBuf );

    gint32 PeekReqToLocal(
        guint8& byToken, BufPtr& pBuf );

    gint32 GetReqToLocal(
        guint8& byToken, BufPtr& pBuf );

    gint32 PostTcpStmEvent(
        guint8 byToken, BufPtr& pBuf );

    // CUnixSockStream methods
    gint32 PostUxSockEvent(
        guint8 byToken, BufPtr& pBuf );

    gint32 StartListening(
        IEventSink* pCallback );

    gint32 StartReading(
        IEventSink* pCallback );

    gint32 WriteStream( BufPtr& pBuf,
        IEventSink* pCallback );

    gint32 SendProgress( CBuffer* pBuf,
        IEventSink* pCallback );

    gint32 SendPingPong( guint8 byToken,
        IEventSink* pCallback );

    // CRpcTcpBridgeShared method
    virtual gint32 WriteTcpStream(
        gint32 iStreamId,
        CBuffer* pSrcBuf,
        guint32 dwSizeToWrite,
        IEventSink* pCallback );

    virtual gint32 ReadTcpStream(
        gint32 iStreamId,
        BufPtr& pSrcBuf,
        guint32 dwSizeToWrite,
        IEventSink* pCallback );

    gint32 PauseReading( bool bPause );

    EnumFCState IncRxBytes( guint32 dwSize );
    EnumFCState IncTxBytes( guint32 dwSize );
};

class CIfUxSockTransRelayTask :
    public CIfUxTaskBase
{
    protected:

    bool m_bIn = true;

    InterfPtr m_pUxStream;
    guint32 m_dwTimeoutSec = 0;
    guint32 m_dwKeepAliveSec = 0;

    public:
    typedef CIfUxTaskBase super;

    CIfUxSockTransRelayTask(
        const IConfigDb* pCfg );

    gint32 OnIrpComplete( IRP* pIrp );
    gint32 OnComplete( gint32 iRet );
    gint32 RunTask();

    // whether reading from the uxsock or writing to
    // the uxsock
    inline bool IsReading()
    { return m_bIn; }

    gint32 OnTaskComplete( gint32 iRet );
    gint32 HandleIrpResp( IRP* pIrp );

    gint32 Pause();
    gint32 Resume();
};

class CIfUxListeningRelayTask :
    public CIfUxTaskBase 
{
    public:
    typedef CIfUxTaskBase  super;

    CIfUxListeningRelayTask( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CIfUxListeningRelayTask ) ); }

    gint32 RunTask();
    gint32 OnIrpComplete( IRP* pIrp );
    virtual gint32 PostEvent( BufPtr& pBuf );
    gint32 OnTaskComplete( gint32 iRet );

    gint32 Pause()
    {
        CIfUxRelayTaskHelper oHelper( this );

        // stop watching the uxsock and
        // uxsock's buffer will do the
        // rest work
        oHelper.PauseReading( true );
        return super::Pause();
    }

    gint32 Resume()
    {
        CIfUxRelayTaskHelper oHelper( this );
        // resume watching the uxsock
        oHelper.PauseReading( false );
        return super::Resume();
    }
};

class CIfTcpStmTransTask :
    public CIfUxSockTransRelayTask
{

    InterfPtr m_pBdgeIf;

    protected:
    gint32 ResumeWriting();

    public:
    typedef CIfUxSockTransRelayTask super;

    CIfTcpStmTransTask( const IConfigDb* pCfg );

    gint32 OnIrpComplete( IRP* pIrp );
    gint32 OnComplete( gint32 iRet );
    gint32 RunTask();

    gint32 HandleIrpResp( IRP* pIrp );
    virtual gint32 PostEvent( BufPtr& pBuf );

    gint32 LocalToRemote( guint8 byToken,
        BufPtr& pLocal, BufPtr& pRemote );

    gint32 RemoteToLocal( BufPtr& pRemote,
        guint8& byToken, BufPtr& pLocal );

    gint32 Pause();
    gint32 Resume();
};

}
