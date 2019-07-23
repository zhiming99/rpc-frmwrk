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

#include "../ipc/uxstream.h"
#include "../ipc/stream.h"
#include "tcpport.h"

template< class T >
class CStreamRelayBase :
    public T
{
    protected:

    // the parent object, the CRpcTcpBridgeProxy or
    // CRpcTcpBridge
    InterfPtr m_pParent;

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

    // stream over unix sock
    guint32 GetPendingWrite(
        HANDLE hWatch ) const;

    // data is ready for reading
    virtual gint32 OnStmRecv(
        HANDLE hChannel, BufPtr& pBuf )
    { return 0; }

    // the local sock is closed
    virtual gint32 OnClose( HANDLE hChannel,
        IEventSink* pCallback = nullptr )
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
                guint32 dwError = *pBuf;
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

    using IStream::OnConnected;
    gint32 OnConnected( HANDLE hChannel )
    { return 0; }

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
            ObjPtr pPortObj = this->GetPort();
            CPort* pPort = pPortObj;
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
            IPort* pPort = this->GetPort();
            TaskletPtr pCloseStream;
            ret = DEFER_IFCALL_NOSCHED(
                pCloseStream, ObjPtr( this ),
                &CRpcTcpBridgeShared::CloseLocalStream,
                pPort, iStmId );

            if( ERROR( ret ) )
                break;
                
            // write a close token to the 
            TaskletPtr pSendClose;
            BufPtr pBuf( true );
            *pBuf = ( guint8 )tokClose;
            ret = DEFER_IFCALL_NOSCHED(
                pSendClose, ObjPtr( this ),
                &CRpcTcpBridgeShared::WriteStream,
                iStmId, *pBuf, 1, ( IEventSink* )nullptr );

            if( ERROR( ret ) )
                break;

            // Let the WriteStream to notify this task when
            // completed
            CIfDeferCallTask* pTemp = pSendClose;
            BufPtr pwscb( true );
            *pwscb = ObjPtr( pSendClose );
            pTemp->UpdateParamAt( 3, pwscb );

            CParamList oParams;
            oParams[ propIfPtr ] = ObjPtr( this );
            TaskGrpPtr pTaskGrp;

            ret = pTaskGrp.NewObj(
                clsid( CIfTaskGroup ),
                oParams.GetCfg() );

            if( ERROR( ret ) )
                break;
     
            pTaskGrp->SetRelation( logicNONE );
            pTaskGrp->AppendTask( pSendClose );
            pTaskGrp->AppendTask( pCloseStream );

            TaskletPtr pTask( pTaskGrp );
            ret = this->AddSeqTask( pTask );

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
    protected:
    gint32 FetchData_Server(
        IConfigDb* pDataDesc,           // [in]
        gint32& fd,                     // [out]
        guint32& dwOffset,              // [in, out]
        guint32& dwSize,                // [in, out]
        IEventSink* pCallback );

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

    // request to the remote server completed
    gint32 OnFetchDataComplete(
        IEventSink* pCallback,
        IEventSink* pIoReqTask,
        IConfigDb* pContext );

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

    gint32 RunTask()
    { return STATUS_PENDING; }

    gint32 OnTaskComplete( gint32 iRet );
};

template< class T >
struct CUnixSockStmRelayBase :
    public T
{
    typedef T super;

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
    inline guint32 QueueLimit()
    { return 5; }

    gint32 PostBdgeStmEvent(
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
                ret = DEFER_IFCALL_NOSCHED( pTask,
                    ObjPtr( pSvc ),
                    &CStreamServerRelay::OnBdgeStmEvent,
                    iStmId, byToken, ( CBuffer* )pBuf );
            }
            else
            {
                ret = DEFER_IFCALL_NOSCHED( pTask,
                    ObjPtr( pSvc ),
                    &CStreamProxyRelay::OnBdgeStmEvent,
                    iStmId, byToken, ( CBuffer* )pBuf );
            }

            if( ERROR( ret ) )
                break;

            // NOTE: the istream method is running on
            // the seqtask of the uxstream
            ret = this->AddSeqTask( pTask );

        }while( 0 );

        return ret;
    }

    gint32 PauseResumeTasks(
        ObjPtr pTasks, bool bResume )
    {
        ObjVecPtr pvecTasks = pTasks;
        if( pvecTasks.IsEmpty() )
            return -EINVAL;

        for( auto elem : ( *pvecTasks )() )
        {
            CIfUxTaskBase* pTask = elem;
            if( pTask == nullptr )
                continue;
            if( bResume )
                pTask->Resume();
            else
                pTask->Pause();
        }

        return 0;
    }

    gint32 ForwardToLocal(
        guint8 byToken, BufPtr& pBuf )
    {
        gint32 ret = 0;

        do{
            CStdRMutex oIfLock( this->GetLock() );
            SENDQUE* pQueue =  &m_queToLocal;
            bool bEmpty = pQueue->empty();

            pQueue->push_back(
                QUE_ELEM( byToken, pBuf ) ); 

            if( pQueue->size() >= QueueLimit() )
            {
                // return error, but the request is
                // queued still.
                TaskletPtr pTask;
                BufPtr pEmptyBuf( true );

                // make sure the tokFlowCtrl is sent
                // ahead of tokLift
                DEFER_IFCALL_NOSCHED( pTask,
                    ObjPtr( this ),
                    &CUnixSockStmRelayBase::ForwardToRemoteWrapper,
                    tokFlowCtrl, ( CBuffer* )pEmptyBuf );

                ret = this->AddSeqTask( pTask );
                if( ERROR( ret ) )
                    break;

                ret = ERROR_QUEUE_FULL;
                break;
            }

            if( bEmpty )
            {
                // wake up the writer task
                CIoManager* pMgr = this->GetIoMgr();
                ret = DEFER_CALL( pMgr,
                    ObjPtr( m_pWritingTask ),
                    &CIfUxTaskBase::Resume );
            }

        }while( 0 );

        return ret;
    }

    gint32 ForwardToRemoteWrapper(
        gint8 byToken, CBuffer* pBuf )
    {
        if( pBuf != nullptr )
        {
            BufPtr ptrBuf( pBuf );
            return ForwardToRemote(
                byToken, ptrBuf );
        }

        BufPtr ptrBuf( true );
        return ForwardToRemote(
            byToken, ptrBuf );
    }

    gint32 ForwardToRemote(
        guint8 byToken, BufPtr& pBuf )
    {
        gint32 ret = 0;

        do{
            CStdRMutex oIfLock( this->GetLock() );
            SENDQUE* pQueue = &m_queToRemote;
            bool bEmpty = pQueue->empty();

            pQueue->push_back(
                QUE_ELEM( byToken, pBuf ) ); 

            if( m_bTcpFlowCtrl )
            {
                ret = ERROR_QUEUE_FULL;
                break;
            }

            if( pQueue->size() >= QueueLimit() )
            {
                // return the error, but the request is
                // queued still.
                ret = ERROR_QUEUE_FULL;
                break;
            }

            if( bEmpty )
            {
                CIoManager* pMgr = this->GetIoMgr();
                // wake up the writer task
                ret = DEFER_CALL( pMgr,
                    ObjPtr( m_pWrTcpStmTask ),
                    &CIfUxTaskBase::Resume );
            }

        }while( 0 );

        return ret;
    }

    gint32 GetReqToRemote(
        guint8& byToken, BufPtr& pBuf )
    {
        gint32 ret = 0;

        do{
            CStdRMutex oIfLock( this->GetLock() );
            SENDQUE* pQueue = &m_queToRemote;

            if( m_bTcpFlowCtrl )
            {
                ret = -ENOENT;
                break;
            }

            if( pQueue->empty() )
            {
                ret = -ENOENT;
                break;
            }

            bool bFull =
                pQueue->size() >= QueueLimit();

            byToken = pQueue->front().first;
            pBuf = pQueue->front().second;
            pQueue->pop_front();

            if( !bFull )
                break;

            if( pQueue->size() >= QueueLimit() )
                break;

            CIoManager* pMgr = this->GetIoMgr();
            ObjVecPtr pvecTasks( true );

            ( *pvecTasks )().push_back(
                ObjPtr( this->m_pReadingTask ) );

            ( *pvecTasks )().push_back(
                ObjPtr( this->m_pListeningTask ) );

            ObjPtr pTasks = pvecTasks;

            // wake up ux stream reader task
            DEFER_CALL( pMgr,
                ObjPtr( this ),
                &CUnixSockStmRelayBase::PauseResumeTasks,
                pTasks, ( bool )true );

        }while( 0 );

        return ret;
    }

    inline gint32 GetReqToLocal(
        guint8& byToken, BufPtr& pBuf )
    {
        gint32 ret = 0;

        do{
            CStdRMutex oIfLock( this->GetLock() );
            SENDQUE* pQueue = &m_queToLocal;

            if( this->m_byFlowCtrl > 0 ||
                pQueue->empty() )
            {
                ret = -ENOENT;
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
                TaskGrpPtr pTaskGrp;
                CParamList oParams;
                ObjPtr pThis( this );
                oParams[ propIfPtr ] = pThis;

                ret = pTaskGrp.NewObj(
                    clsid( CIfTaskGroup ),
                    oParams.GetCfg() );

                if( ERROR( ret ) )
                    break;

                TaskletPtr pRmtResumeSend;
                BufPtr pEmptyBuf( true );
                // send out the tokLift to the peer
                ret = DEFER_IFCALL_NOSCHED(
                    pRmtResumeSend, pThis,
                    &CUnixSockStmRelayBase::ForwardToRemoteWrapper,
                    tokLift, pEmptyBuf );

                if( ERROR( ret ) )
                    break;

                // wake up the writer task
                TaskletPtr pResumeRead;
                ret = DEFER_IFCALL_NOSCHED(
                    pResumeRead,
                    ObjPtr( m_pRdTcpStmTask ),
                    &CIfUxTaskBase::Resume );

                if( ERROR( ret ) )
                    break;

                pTaskGrp->AppendTask( pResumeRead );
                pTaskGrp->AppendTask( pRmtResumeSend );
                TaskletPtr pTasks = ObjPtr( pTaskGrp );

                ret = this->AddSeqTask( pTasks );
            }

        }while( 0 );

        return ret;
    }

    CUnixSockStmRelayBase( const IConfigDb* pCfg ) :
        super( pCfg )
    {;}

    gint32 PostTcpStmEvent(
        guint8 byToken, BufPtr& pBuf )
    {
        TaskletPtr pTask;
        DEFER_IFCALL_NOSCHED( pTask,
            ObjPtr( this ),
            &CUnixSockStmRelayBase::OnTcpStmEvent,
            byToken, ( CBuffer* )pBuf );

        return this->AddSeqTask( pTask );
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
                ret = PostTcpStmEvent(
                    byToken, bufPtr );
                break;
            }
        }

        return ret;
    }

    virtual gint32 OnPingPong( bool bPing )
    {
        // to cloak super::OnPingPong
        return -ENOTSUP;
    }

    virtual gint32 OnDataReceivedRemote(
        CBuffer* pBuf )
    {
        return 0;
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
        if( m_queToRemote.size() < QueueLimit() )
        {
            ( *pvecTasks )().push_back(
                ObjPtr( this->m_pReadingTask ) );

            ( *pvecTasks )().push_back(
                ObjPtr( this->m_pListeningTask ) );

        }

        if( !m_queToRemote.empty() )
        {
            ( *pvecTasks )().push_back(
                ObjPtr( m_pWrTcpStmTask ) );
        }

        ObjPtr pTasks = pvecTasks;

        // wake up ux stream reader task
        TaskletPtr pDeferTask;
        gint32 ret = DEFER_IFCALL_NOSCHED( 
            pDeferTask,
            ObjPtr( this ),
            &CUnixSockStmRelayBase::PauseResumeTasks,
            pTasks, ( bool )true );

        if( ERROR( ret ) )
            return ret;

        ret = this->AddSeqTask( pDeferTask );

        return ret;
    }

    virtual gint32 OnDataReceived( CBuffer* pBuf )
    {
        if( pBuf == nullptr || pBuf->empty() )
            return -EINVAL;
        this->m_qwRxBytes += pBuf->size();
        return 0;
    }

    virtual gint32 OnProgressRemote( CBuffer* pBuf )
    {
        return 0;
    }

    virtual gint32 OnProgress( CBuffer* pBuf )
    {
        BufPtr bufPtr( pBuf );
        return this->PostUxStreamEvent(
            tokProgress, bufPtr );
    }

    virtual gint32 StartTicking(
        IEventSink* pContext )
    {
        gint32 ret = 0;

        do{
            // start the event listening
            CParamList oParams;

            oParams.CopyProp( propIfPtr, this );
            oParams.CopyProp( propIoMgr, this );
            oParams.CopyProp( propTimeoutSec, this );

            ret = this->m_pListeningTask.NewObj(
                clsid( CIfUxListeningRelayTask ),
                oParams.GetCfg() );

            if( ERROR( ret ) )
                break;

            CIoManager* pMgr = this->GetIoMgr();
            ret = pMgr->RescheduleTask(
                this->m_pListeningTask );

            if( ERROR( ret ) )
                break;

            // start the ping ticker
            ret = this->m_pPingTicker.NewObj(
                clsid( CIfUxPingTicker ),
                oParams.GetCfg() );

            if( ERROR( ret ) )
                break;

            ret = pMgr->RescheduleTask(
                this->m_pPingTicker );

            if( ERROR( ret ) )
                break;

        }while( 0 );

        return ret;
    }

    virtual gint32 OnPostStart(
        IEventSink* pContext )
    {
        gint32 ret = 0;

        do{
            ret = StartTicking( pContext );
            if( ERROR( ret ) )
                break;

            // start the data reading
            CParamList oParams;
            oParams.CopyProp( propIfPtr, this );
            oParams.CopyProp( propIoMgr, this );
            oParams.CopyProp( propTimeoutSec, this );
            oParams.CopyProp(
                propKeepAliveSec, this );

            // ux stream reader
            oParams.Push( ( bool )true );
            ret = this->m_pReadingTask.NewObj(
                clsid( CIfUxSockTransRelayTask ),
                oParams.GetCfg() );
            if( ERROR( ret ) )
                break;

            ret = this->GetIoMgr()->
                RescheduleTask( this->m_pReadingTask );

            // ux stream writer
            oParams.SetBoolProp( 0, false );
            ret = m_pWritingTask.NewObj(
                clsid( CIfUxSockTransRelayTask ),
                oParams.GetCfg() );
            if( ERROR( ret ) )
                break;

            ret = this->GetIoMgr()->
                RescheduleTask( this->m_pWritingTask );

            if( ERROR( ret ) )
                break;

            // tcp stream writer
            ret = m_pWrTcpStmTask.NewObj(
                clsid( CIfTcpStmTransTask ),
                oParams.GetCfg() );
            if( ERROR( ret ) )
                break;

            ret = this->GetIoMgr()->
                RescheduleTask( m_pWrTcpStmTask );

            if( ERROR( ret ) )
                break;

            // tcp stream reader
            oParams.SetBoolProp( 0, true );
            ret = m_pRdTcpStmTask.NewObj(
                clsid( CIfTcpStmTransTask ),
                oParams.GetCfg() );
            if( ERROR( ret ) )
                break;

            ret = this->GetIoMgr()->
                RescheduleTask( m_pRdTcpStmTask );

        }while( 0 );

        if( ERROR( ret ) )
        {
            // cancel all the tasks
            OnPreStop( pContext );
        }

        return ret;
    }

    virtual gint32 OnPreStop( IEventSink* pCallback ) 
    {
        super::OnPreStop( pCallback );

        if( !m_pWrTcpStmTask.IsEmpty() )
            ( *m_pWrTcpStmTask )( eventCancelTask );

        if( !m_pRdTcpStmTask.IsEmpty() )
            ( *m_pRdTcpStmTask )( eventCancelTask );

        if( !m_pWritingTask.IsEmpty() )
            ( *m_pWritingTask )( eventCancelTask );

        return 0;
    }
};

class CUnixSockStmServerRelay :
    public CUnixSockStmRelayBase< CUnixSockStmServer >
{
    public:
    typedef CUnixSockStmRelayBase< CUnixSockStmServer > super;

    CUnixSockStmServerRelay( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CUnixSockStmServerRelay ) ); }

    virtual gint32 OnPingPong( bool bPing )
    {
        BufPtr pBuf( true );
        return PostUxStreamEvent(
            bPing ? tokPing : tokPong,
            pBuf );
    }

    virtual gint32 OnPingPongRemote( bool bPing )
    {
        if( !bPing )
            ResetPingTicker();

        BufPtr pBuf( true );
        return PostBdgeStmEvent(
            bPing ? tokPing : tokPong,
            pBuf );
    }
};

class CUnixSockStmProxyRelay :
    public CUnixSockStmRelayBase< CUnixSockStmProxy >
{
    public:
    typedef CUnixSockStmRelayBase< CUnixSockStmProxy > super;

    CUnixSockStmProxyRelay( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CUnixSockStmProxyRelay ) ); }

    virtual gint32 OnPingPong( bool bPing )
    {
        if( !bPing )
            ResetPingTicker();

        BufPtr pBuf( true );
        return PostUxStreamEvent(
            bPing ? tokPing : tokPong,
            pBuf );
    }

    virtual gint32 OnPingPongRemote( bool bPing )
    {
        BufPtr pBuf( true );
        return PostBdgeStmEvent(
            bPing ? tokPing : tokPong,
            pBuf );
    }
};

struct CIfUxRelayTaskHelper
{
    CRpcServices* m_pSvc;
    CIfUxRelayTaskHelper( IEventSink* pTask );

    gint32 SetTask( IEventSink* pTask );
    gint32 GetBridgeIf( InterfPtr& pIf );

    // CUnixSockStmRelayBase methods
    gint32 ForwardToRemote(
        guint8 byToken, BufPtr& pBuf );

    gint32 ForwardToLocal(
        guint8 byToken, BufPtr& pBuf );

    gint32 GetReqToRemote(
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

    // CRpcTcpBridgeShared method
    gint32 WriteTcpStream(
        gint32 iStreamId,
        CBuffer* pSrcBuf,
        guint32 dwSizeToWrite,
        IEventSink* pCallback );

    gint32 ReadTcpStream(
        gint32 iStreamId,
        CBuffer* pSrcBuf,
        guint32 dwSizeToWrite,
        IEventSink* pCallback );

    gint32 PauseReading( bool bPause );
};

class CIfUxSockTransRelayTask :
    public CIfUxTaskBase
{
    protected:

    bool m_bIn = true;

    InterfPtr m_pUxStream;
    BufPtr m_pPayload;
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

    gint32 Pause()
    {
        CIfUxRelayTaskHelper oHelper( this );

        // stop watching the uxsock and
        // uxsock's buffer will do the
        // rest work
        if( IsReading() )
            oHelper.PauseReading( true );

        return super::Pause();
    }
    gint32 Resume()
    {
        CIfUxRelayTaskHelper oHelper( this );
        // resume watching the uxsock
        if( IsReading() )
            oHelper.PauseReading( false );

        return super::Pause();
    }
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
    gint32 PostEvent( BufPtr& pBuf );
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
        return super::Pause();
    }
};


class CIfTcpStmTransTask :
    public CIfUxSockTransRelayTask
{

    InterfPtr m_pBdgeIf;

    public:
    typedef CIfUxSockTransRelayTask super;

    CIfTcpStmTransTask( const IConfigDb* pCfg );

    gint32 OnIrpComplete( IRP* pIrp );
    gint32 OnComplete( gint32 iRet );
    gint32 RunTask();

    // whether reading from the uxsock or writing to
    // the uxsock
    inline bool IsReading()
    { return m_bIn; }

    gint32 HandleIrpResp( IRP* pIrp );
    gint32 PostEvent( BufPtr& pBuf );

    gint32 LocalToRemote( guint8 byToken,
        BufPtr& pLocal, BufPtr& pRemote );

    gint32 RemoteToLocal( BufPtr& pRemote,
        guint8& byToken, BufPtr& pLocal );
};
