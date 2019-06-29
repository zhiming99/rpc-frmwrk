/*
 * =====================================================================================
 *
 *       Filename:  uxstream.h
 *
 *    Description:  declaration and implementation of
 *                  CUnixSockStream and related classes
 *
 *        Version:  1.0
 *        Created:  06/25/2019 09:32:00 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:  
 *
 * =====================================================================================
 */
#pragma once

#include "stream.h"

class CIfCallbackInterceptor :
    public CIfInterceptTaskProxy
{
    TaskletPtr m_pCallAhead;
    TaskletPtr m_pCallAfter;
    public: 
    typedef CIfInterceptTaskProxy super;

    CIfCallbackInterceptor( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CIfCallbackInterceptor ) ); }

    // for those tasks such as DEFER_CALL's output, who
    // cannot call propEventSink on complete
    //
    // Run pTask ahead of propEventSink
    void InsertCall( TaskletPtr& pTask )
    { m_pCallAhead = pTask; }

    // Run pTask after propEventSink 
    void InsertCallAfter( TaskletPtr& pTask )
    {m_pCallAfter = pTask; }

    gint32 RunTask()
    { return STATUS_PENDING; }

    gint32 OnTaskComplete( gint32 iRet )
    {
        if( ERROR( iRet ) )
            return iRet;

        if( m_pCallAhead.IsEmpty() )
            return iRet;

        ( *m_pCallAhead )( eventZero );

        return iRet;
    }

    gint32 OnComplete( gint32 iRet )
    {
        super::OnComplete( iRet );

        if( !m_pCallAhead.IsEmpty() )
            ( *m_pCallAfter )( eventZero );

        return iRet;
    }
};

class CIfUxPingTask :
    public CIfParallelTask
{
    public:
    typedef CIfParallelTask super;
    
    CIfUxPingTask( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CIfUxPingTask ) ); }

    gint32 RunTask();
    gint32 OnRetry();
    gint32 OnCancel( guint32 dwContext )
    {
        RemoveTimer();
        return super::OnCancel( dwContext );
    }
};

class CIfUxPingTicker :
    public CIfParallelTask
{
    public:
    typedef CIfParallelTask super;
    CIfUxPingTicker( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CIfUxPingTicker ) ); }

    gint32 RunTask();
    gint32 OnRetry();
};

class CIfUxSockTransTask :
    public CIfParallelTask
{

    bool m_bIn = true;

    InterfPtr m_pUxStream;
    BufPtr m_pPayload;
    guint32 m_dwTimeoutSec;
    guint32 m_dwKeepAliveSec;

    public:
    typedef CIfParallelTask super;

    CIfUxSockTransTask( const IConfigDb* pCfg );
    gint32 OnIrpComplete( IRP* pIrp );
    gint32 OnComplete( gint32 iRet );
    gint32 RunTask();

    gint32 OnCancel( guint32 dwContext )
    { return super::OnCancel( dwContext ); }

    inline void SetIoDirection( bool bIn )
    { m_bIn = bIn; }

    inline gint32 SetBufToSend( BufPtr pBuf )
    {
        if( pBuf.IsEmpty() )
            return -EINVAL;

        if( ( *pBuf ).empty() )
            return -EINVAL;

        m_pPayload = pBuf;
        return STATUS_SUCCESS;
    }

    inline void GetBufReceived( BufPtr pBuf )
    { pBuf = m_pPayload; }

};

class CIfUxListeningTask :
    public CIfParallelTask
{
    public:
    typedef CIfParallelTask super;

    CIfUxListeningTask( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CIfUxListeningTask ) ); }

    gint32 RunTask();
    gint32 PostEvent( BufPtr& pBuf );
    gint32 OnIrpComplete( IRP* pIrp );
};

#define UXSOCK_MOD_SERVER_NAME "UnixSockStmServer"
#define UXSOCK_OBJNAME_SERVER "CUnixSockStmServer"               
#define UXSOCK_OBJNAME_PROXY  "CUnixSockStmProxy"               

template< class T >
class CUnixSockStream:
    public T
{
    InterfPtr m_pParent;

    TaskletPtr m_pPingTicker;
    TaskletPtr m_pListeningTask;
    TaskletPtr m_pReadingTask;

    // TaskletPtr m_pReadUxStmTask;
    // TaskletPtr m_pWriteUxStmTask;

    std::atomic< guint64 >   m_qwRxBytes;
    std::atomic< guint64 >   m_qwTxBytes;
    bool                     m_bFirstPing = true;

    // flag to allow or deny outgoing data
    std::atomic< guint8 > m_byFlowCtrl;
    std::deque< std::pair< ObjPtr, ObjPtr > > m_queWaitLift;

    protected:
    gint32 OnUxSockEventWrapper(
        guint8 byToken,
        CBuffer* pBuf )
    {
        return OnUxSockEvent( byToken, pBuf );
    }

    static CfgPtr InitCfg( const IConfigDb* pCfg )
    {
        // since it is a system provided interface,
        // we make a config manually at present.
        CCfgOpener oNewCfg; 
        *oNewCfg.GetCfg() = *pCfg;
        gint32 ret = 0;

        do{
            ret = oNewCfg.SetIntProp(
                propIfStateClass, 
                clsid( CUnixSockStmState ) ) ;

            guint32 dwFd;
            ret = oNewCfg.GetIntProp( propFd, dwFd );
            if( ERROR( ret ) )
                break;

            bool bServer = true;
            ret = oNewCfg.GetBoolProp(
                propIsServer, bServer );
            if( ERROR( ret ) )
                break;

            oNewCfg.RemoveProperty( propIsServer );

            CRpcServices* pIf = nullptr;
            ret = oNewCfg.GetPointer(
                propIfPtr, pIf );
            if( ERROR( ret ) )
                break;

            CIoManager* pMgr = pIf->GetIoMgr();

            std::string strVal;
            if( bServer )
                strVal = UXSOCK_OBJNAME_SERVER;
            else
                strVal = UXSOCK_OBJNAME_PROXY;

            // actually this object does not use 
            // dbus's mechanism, so the names is
            // not useful for this object.
            std::string strSvrName =
                pMgr->GetModName();

            strVal = DBUS_OBJ_PATH(
                strSvrName, strVal );

            oNewCfg.SetStrProp(
                propObjPath, strVal );

            std::string strDest( strVal );
            std::replace( strDest.begin(),
                strDest.end(), '/', '.');

            if( strDest[ 0 ] == '.' )
                strDest = strDest.substr( 1 );

            if( bServer )
            {
                oNewCfg.SetStrProp(
                    propSrcDBusName, strDest );
            }
            else
            {
                oNewCfg.SetStrProp(
                    propDestDBusName, strDest );
            }

            if( true )
            {
                // build a match
                MatchPtr pMatch;
                pMatch.NewObj( clsid( CMessageMatch ) );

                CCfgOpenerObj oMatch(
                    ( CObjBase* )pMatch );

                oMatch.SetStrProp(
                    propObjPath, strVal );

                oMatch.SetIntProp( propMatchType,
                    bServer ? matchServer : matchClient );

                std::string strIfName =
                    UXSOCK_OBJNAME_SERVER;

                oMatch.SetStrProp( propIfName,
                    DBUS_IF_NAME( strIfName ) );

                oMatch.SetBoolProp(
                    propPausable, false );

                ObjVecPtr pObjVec( true );
                ( *pObjVec )().push_back( pMatch );

                oNewCfg.SetObjPtr(
                    propObjList, ObjPtr( pObjVec ) );
            }


            strVal = "UnixSockBusPort_0";
            oNewCfg.SetStrProp(
                propBusName, strVal );

            strVal = "UnixSockStmPdo";
            oNewCfg.SetStrProp(
                propPortClass, strVal );

            oNewCfg.SetIntProp(
                propPortId, dwFd );

        }while( 0 );
        
        if( ERROR( ret ) )
        {
            throw std::invalid_argument(
                DebugMsg( -EINVAL, "Error occurs" ) );
        }
        return oNewCfg.GetCfg();
    }

    public:
    typedef T super;

    CUnixSockStream( const IConfigDb* pCfg ) :
        super( InitCfg( pCfg ) ),
        m_qwRxBytes( 0 ),
        m_qwTxBytes( 0 ),
        m_byFlowCtrl( 0 )
    {
        if( pCfg == nullptr )
            return;

        gint32 ret = 0;
        do{
            CCfgOpenerObj oParams( this );
            ObjPtr pObj;
            oParams.GetObjPtr( propIfPtr, pObj );

            m_pParent = pObj;
            if( m_pParent.IsEmpty() )
            {
                ret = -EFAULT;
                break;
            }

            ret = oParams.GetObjPtr( 0, pObj );
            if( ERROR( ret ) )
                break;

            CCfgOpener oDataDesc(
                ( IConfigDb* )pObj );

            oParams.CopyProp( propTimeoutSec,
                oDataDesc.GetCfg() );

            oParams.CopyProp( propKeepAliveSec,
                oDataDesc.GetCfg() );

            this->RemoveProperty( propIfPtr );
            this->RemoveProperty( 0 );
            this->RemoveProperty( propParamCount );

        }while( 0 );

        if( ERROR( ret ) )
        {
            std::string strMsg = DebugMsg( ret,
                "Error in CUnixSockStream ctor" );
            throw std::runtime_error( strMsg );
        }
    }

    inline IStream* GetParent()
    { return dynamic_cast< IStream* >( m_pParent ); }

    virtual gint32 OnUxSockEvent(
        guint8 byToken,
        CBuffer* pBuf )
    {
        gint32 ret = 0;

        switch( byToken )
        {
        case tokPing:
        case tokPong:
            {
                ret = OnPingPong(
                    tokPing == byToken );
                break;
            }
        case tokProgress:
            {
                ret = OnProgress( pBuf );
                break;
            }
        case tokLift:
            {
                // happens when the sending queue has
                // space for new writes or the unacked
                // bytes is less than
                // STM_MAX_PENDING_WRITE
                ret = OnFCLifted();
                break;
            }
        case tokFlowCtrl:
            {
                // happens when the sending queue is
                // full
                ret = OnFlowControl();
                break;
            }
        case tokData:
            {
                if( pBuf == nullptr || pBuf->empty() )
                {
                    ret = -EINVAL;
                    break;
                }
                guint64 qwOldRxBytes = m_qwRxBytes;
                m_qwRxBytes += pBuf->size();
                guint64 qwNewRxBytes = m_qwRxBytes;

                BufPtr bufPtr( pBuf );
                PostUxStreamEvent( tokData, bufPtr );

                guint64 qwThresh = ( qwNewRxBytes &
                    ~( STM_MAX_PENDING_WRITE - 1 ) );

                if( qwOldRxBytes < qwThresh &&
                    qwNewRxBytes >= qwThresh )
                {
                    // every STM_MAX_PENDING_WRITE
                    // bytes received, we emit a
                    // progress report, as a flow
                    // control hint.
                    //
                    // send a progress notification
                    CParamList oProgress;
                    oProgress.SetQwordProp(
                        propRxBytes, m_qwRxBytes );

                    BufPtr pBuf( true );
                    oProgress.GetCfg()->Serialize( *pBuf );

                    CParamList oReq;
                    oReq.SetStrProp(
                        propMethodName, "SendProgress" );
                    oReq.Push( pBuf );

                    CParamList oCallback;

                    oCallback.CopyProp( propKeepAliveSec, this );
                    oCallback.SetPointer( propIfPtr, this );
                    oCallback.SetPointer( propIoMgr, this );

                    TaskletPtr pTask;
                    pTask.NewObj( clsid( CIfDummyTask ),
                        oCallback.GetCfg() );

                    ret = SubmitRequest(
                        oReq.GetCfg(), pTask, true );
                    
                }
                break;
            }
        case tokClose:
        case tokError:
            {
                BufPtr bufPtr( pBuf );
                ret = PostUxStreamEvent(
                    byToken, bufPtr );
                break;
            }
        }

        return ret;
    }

    gint32 PostUxStreamEvent(
        guint8 byToken, BufPtr& pBuf )
    {
        // forward to the parent IStream object
        gint32 ret = 0;
        do{
            HANDLE hChannel = ( HANDLE )this;
            CParamList oParams;
            BufPtr ptrBuf = pBuf;
            oParams.Push( byToken );
            oParams.Push( ptrBuf );

            TaskletPtr pTask;
            DEFER_CALL_NOSCHED( pTask,
                ObjPtr( m_pParent ),
                &IStream::OnUxStmEvtWrapper,
                hChannel,
                ( IConfigDb* )oParams.GetCfg() );

            CRpcServices* pIf = m_pParent;
            ret = pIf->AddSeqTask( pTask );

        }while( 0 );

        return ret;
    }

    gint32 PostUxSockEvent(
        guint8 byToken, BufPtr& pBuf )
    {
        TaskletPtr pTask;
        DEFER_CALL_NOSCHED( pTask,
            ObjPtr( this ),
            &CUnixSockStream::OnUxSockEventWrapper,
            byToken, *pBuf );

        return this->AddSeqTask( pTask );
    }

    // events from the ux port
    virtual gint32 OnPingPong( bool bPing )
    {
        gint32 ret = 0;
        do{
            ResetPingTicker();
            if( this->IsServer() && bPing )
            {
                // echo pong to the client
                TaskletPtr pDummyTask;
                ret = pDummyTask.NewObj(
                    clsid( CIfDummyTask ) );

                if( ERROR( ret ) )
                    break;

                ret = SendPingPong(
                    tokPong, pDummyTask );

                if( ret == ERROR_QUEUE_FULL )
                {
                    // what to do here?
                }
                else if( ERROR( ret ) )
                {
                    BufPtr pNullTask;
                    PostUxSockEvent(
                        tokClose, pNullTask );
                }
                else if( m_bFirstPing )
                {
                    m_bFirstPing = false;
                    HANDLE hChannel = ( HANDLE )
                        ( ( CObjBase* )this );

                    // send the OnConnected event to
                    // the parent stream object
                    TaskletPtr pConnTask;
                    ret = DEFER_CALL_NOSCHED(
                        pConnTask,
                        ObjPtr( m_pParent ),
                        &IStream::OnConnected,
                        hChannel ); 

                    if( SUCCEEDED( ret ) )
                    {
                        CRpcServices* pSvc = m_pParent;
                        DEFER_CALL( pSvc->GetIoMgr(),
                            ObjPtr( m_pParent ),
                            &CRpcServices::RunManagedTask,
                            pConnTask, false );
                    }

                    ret = 0;
                }
            }
            else if( !this->IsServer() && !bPing )
            {
                // received a pong
                // schedule another ping task in 
                // `propKeepAliveSec' second

                CParamList oParams;
                oParams.SetPointer(
                    propIoMgr, this->GetIoMgr() );

                oParams.SetPointer(
                    propIfPtr, this );

                oParams.CopyProp(
                    propKeepAliveSec, this );

                oParams.CopyProp(
                    propTimeoutSec, this );

                TaskletPtr pPingTask;
                ret = pPingTask.NewObj(
                    clsid( CIfUxPingTask ),
                    oParams.GetCfg() );

                if( ERROR( ret ) )
                    break;

                ret = this->RunManagedTask( pPingTask );
            }
            else
            {
                ret = -EINVAL;
            }

        }while( 0 );

        return ret;
    }

    gint32 OnProgress( CBuffer* pBuf )
    {
        gint32 ret = 0;
        do{
            CCfgOpener oCfg;
            ret = oCfg.GetCfg()->Deserialize( *pBuf );
            if( ERROR( ret ) )
                break;

            guint64 qwTxBytesAcked = 0;
            ret = oCfg.GetQwordProp(
                propRxBytes, qwTxBytesAcked );

            if( ERROR( ret ) )
                break;

            if( m_qwTxBytes < qwTxBytesAcked )
            {
                ret = -ERANGE;
                break;
            }

            if( m_qwTxBytes - qwTxBytesAcked <
                STM_MAX_PENDING_WRITE &&
                m_byFlowCtrl > 0 )
            {
                ret = OnFCLifted();
            }
            else if( m_qwTxBytes - qwTxBytesAcked >=
                STM_MAX_PENDING_WRITE &&
                m_byFlowCtrl == 0 )
            {
                ret = OnFlowControl();
            }

        }while( 0 );

        return ret;
    }

    // only happens on the router
    gint32 OnFlowControl()
    {
        m_byFlowCtrl++;
        return 0;
    }

    gint32 OnFCLifted()
    {
        gint32 ret = 0;

        do{
            CStdRMutex oIfLock( this->GetLock() );
            m_byFlowCtrl--;
            if( m_byFlowCtrl > 0 )
                break;

            // submit those blocked requests
            while( !m_queWaitLift.empty() )
            {
                if( !IsConnected( nullptr ) )
                    break;

                std::pair< ObjPtr, ObjPtr > 
                    oFirst = m_queWaitLift.front();

                oIfLock.Unlock();

                ret = SubmitRequest( oFirst.first,
                    oFirst.second, true );

                if( ret != STATUS_PENDING )
                    break;

                oIfLock.Lock();
                m_queWaitLift.pop_front();
            }
        
        }while( 0 );

        return ret;
    }

    gint32 ResetPingTicker()
    {
        if( m_pPingTicker.IsEmpty() )
            return ERROR_STATE;

        CIfParallelTask* pTask = m_pPingTicker;

        CStdRTMutex oTaskLock(
            pTask->GetLock() );

        if( stateStarted !=
            pTask->GetTaskState() )
            return ERROR_STATE;

        pTask->ResetRetries();
        pTask->ResetTimer();

        return 0;
    }

    gint32 RebuildMatches()
    {
        this->m_vecMatches.clear();
        return 0;
    }

    // IO methods
    gint32 SendPingPong( guint8 byToken,
        IEventSink* pCallback )
    {
        gint32 ret = 0;
        if( pCallback == nullptr )
            return -EINVAL;
        do{
            BufPtr pBuf( true );
            *pBuf = byToken;
            CParamList oParams;
            oParams[ propMethodName ] = __func__;
            oParams.Push( pBuf );
            ret = SubmitRequest( oParams.GetCfg(),
                pCallback, true );

        }while( 0 );

        return ret;
    }

    gint32 PauseReading( bool bPause )
    {
        gint32 ret = 0;
        do{
            CParamList oParams;
            BufPtr pBuf( true );
            *pBuf = bPause ? tokFlowCtrl : tokLift;
            oParams[ propMethodName ] = __func__;
            oParams.Push( pBuf );

            TaskletPtr pCallback;

            ret = pCallback.NewObj(
                clsid( CIfDummyTask ) );

            if( ERROR( ret ) )
                break;

            ret = SubmitRequest( oParams.GetCfg(),
                pCallback, true );

        }while( 0 );

        return ret;
    }

    gint32 SendProgress( CBuffer* pBuf,
        IEventSink* pCallback )
    {
        gint32 ret = 0;
        if( pBuf == nullptr )
            return -EINVAL;

        if( pCallback == nullptr )
            return -EINVAL;

        do{
            CParamList oParams;
            oParams[ propMethodName ] = __func__;
            BufPtr pNewBuf( true );
            ret = pNewBuf->Resize(
                1 + sizeof( guint32 ) + pBuf->size() );

            if( ERROR( ret ) )
                break;

            pNewBuf->ptr()[ 0 ] = tokProgress;
            guint32 dwSize = htonl( pBuf->size() );

            pNewBuf->Append( ( guint8* )&dwSize,
                sizeof( guint32 ) );

            memcpy( pNewBuf->ptr() + 1 + sizeof( guint32 ),
                pBuf->ptr(), pBuf->size() );

            pNewBuf->Append( ( guint8* )pBuf->ptr(),
                pBuf->size() );

            oParams.Push( pBuf );
            ret = SubmitRequest( oParams.GetCfg(),
                pCallback, true );

        }while( 0 );

        return ret;
    }

    gint32 WriteStream( BufPtr& pBuf,
        IEventSink* pCallback )
    {
        gint32 ret = 0;
        if( pBuf.IsEmpty() || pBuf->empty() )
            return -EINVAL;

        if( pCallback == nullptr )
            return -EINVAL;

        do{
            CParamList oParams;
            oParams[ propMethodName ] = __func__;
            oParams.Push( pBuf );
            ret = SubmitRequest( oParams.GetCfg(),
                pCallback, true );

        }while( 0 );

        return ret;
    }

    gint32 StartListening(
        IEventSink* pCallback )
    {
        gint32 ret = 0;

        if( pCallback == nullptr )
            return -EINVAL;

        do{
            CParamList oParams;
            oParams[ propMethodName ] = __func__;
            ret = SubmitRequest( oParams.GetCfg(),
                pCallback, false );

        }while( 0 );

        return ret;
    }

    gint32 StartReading(
        IEventSink* pCallback )
    {
        gint32 ret = 0;

        if( pCallback == nullptr )
            return -EINVAL;

        do{
            CParamList oParams;
            oParams[ propMethodName ] = __func__;
            ret = SubmitRequest( oParams.GetCfg(),
                pCallback, false );

        }while( 0 );

        return ret;
    }

    gint32 SetupReqIrp( IRP* pIrp,
        IConfigDb* pReqCall,
        IEventSink* pCallback )
    {
        gint32 ret = 0;
        if( pIrp == nullptr ||
            pIrp->GetStackSize() == 0 )
            return -EINVAL;

        if( pReqCall == nullptr ||
            pCallback == nullptr )
            return -EINVAL;

        do{
            CParamList oParams( pReqCall );
            std::string strMethod;
            ret = oParams.GetStrProp(
                propMethodName, strMethod );

            if( ERROR( ret ) )
                break;

            CCfgOpenerObj oCfg( pCallback );
            IrpCtxPtr& pCtx = pIrp->GetTopStack(); 
            pCtx->SetMajorCmd( IRP_MJ_FUNC );
            pIrp->SetCallback( pCallback, 0 );

            if( strMethod == "StartReading" )
            {
                pCtx->SetMinorCmd( IRP_MN_READ );
                pCtx->SetIoDirection( IRP_DIR_IN );
            }
            else if( strMethod == "WriteStream" )
            {
                BufPtr pBuf;
                oParams.GetProperty( 0, pBuf );
                pCtx->SetMinorCmd( IRP_MN_WRITE );
                pCtx->SetIoDirection( IRP_DIR_OUT );
                pCtx->SetReqData( pBuf );

                guint32 dwTimeoutSec = 0;
                ret = oCfg.GetIntProp(
                    propKeepAliveSec, dwTimeoutSec );
                if( ERROR( ret ) )
                    break;

                pIrp->SetTimer(
                    dwTimeoutSec, this->GetIoMgr() );
            }
            else if( strMethod == "StartListening" )
            {
                IrpCtxPtr& pCtx = pIrp->GetTopStack(); 
                pCtx->SetMinorCmd( IRP_MN_IOCTL );
                pCtx->SetCtrlCode( CTRLCODE_LISTENING );
                pCtx->SetIoDirection( IRP_DIR_IN );

                guint32 dwTimeoutSec = 0;
                ret = oCfg.GetIntProp(
                    propTimeoutSec, dwTimeoutSec );
                if( ERROR( ret ) )
                    break;

                pIrp->SetTimer(
                    dwTimeoutSec, this->GetIoMgr() );
            }
            else if( strMethod == "SendPingPong" ||
                strMethod== "SendProgress" )
            {
                BufPtr pBuf;
                oParams.GetProperty( 0, pBuf );
                pCtx->SetMinorCmd( IRP_MN_IOCTL );
                pCtx->SetCtrlCode( CTRLCODE_STREAM_CMD );
                pCtx->SetIoDirection( IRP_DIR_OUT );
                pCtx->SetReqData( pBuf );

                guint32 dwTimeoutSec = 0;
                ret = oCfg.GetIntProp(
                    propKeepAliveSec, dwTimeoutSec );
                if( ERROR( ret ) )
                    break;

                pIrp->SetTimer(
                    dwTimeoutSec, this->GetIoMgr() );
            }
            else if( strMethod == "PauseReading" )
            {
                BufPtr pBuf;
                oParams.GetProperty( 0, pBuf );
                pCtx->SetMinorCmd( IRP_MN_IOCTL );
                pCtx->SetCtrlCode( CTRLCODE_STREAM_CMD );
                pCtx->SetIoDirection( IRP_DIR_IN );
                pCtx->SetReqData( pBuf );
                // this is an immediate request, no
                // timer
            }

            // save the irp for canceling
            oCfg.SetPointer( propIrpPtr, pIrp );

        }while( 0 );

        return ret;
    }

    gint32 SubmitRequest( 
        IConfigDb* pReqCall,
        IEventSink* pCallback,
        bool bSend )
    {
        gint32 ret = 0;
        if( pCallback == nullptr ||
            pReqCall == nullptr )
            return -EINVAL;

        do{
            CStdRMutex oIfLock( this->GetLock() );
            if( bSend && m_byFlowCtrl > 0 )
            {
                // those calls will be stored in the queue
                std::pair< ObjPtr, ObjPtr > oPair;
                oPair.first = ObjPtr( pReqCall );
                oPair.second = ObjPtr( pCallback );

                std::pair< ObjPtr, ObjPtr >&
                    oHead = m_queWaitLift.front();

                if( !( oPair.first == oHead.first ) &&
                    !( oPair.second == oHead.second ) )
                {
                    m_queWaitLift.push_back( oPair );
                    ret = STATUS_PENDING;
                }
                else
                {
                    ret = -EAGAIN;
                }
                break;
            }
            oIfLock.Unlock();

            CCfgOpenerObj oCfg( pCallback );

            IPort* pPort = this->GetPort();
            pPort = pPort->GetTopmostPort();

            IrpPtr pIrp( true );
            ret = pIrp->AllocNextStack( nullptr );
            if( ERROR( ret ) )
                break;

            IrpCtxPtr& pCtx = pIrp->GetTopStack(); 
            pPort->AllocIrpCtxExt( pCtx, ( PIRP )pIrp );

            ret = SetupReqIrp(
                pIrp, pReqCall, pCallback );
            if( ERROR( ret ) )
                break;

            CPort* pPort2 = ( CPort* )pPort;
            CIoManager* pMgr = pPort2->GetIoMgr();

            ret = pMgr->SubmitIrp(
                PortToHandle( pPort ), pIrp );
            if( ret == STATUS_PENDING )
                break;

            pIrp->RemoveTimer();
            pIrp->RemoveCallback();
            oCfg.RemoveProperty( propIrpPtr );

            if( bSend && ret == ERROR_QUEUE_FULL )
            {
                oIfLock.Lock();
                std::pair< ObjPtr, ObjPtr > oPair;

                oPair.first = ObjPtr( pReqCall );
                oPair.second = ObjPtr( pCallback );

                std::pair< ObjPtr, ObjPtr >&
                    oHead = m_queWaitLift.front();

                if( !( oPair.first == oHead.first ) &&
                    !( oPair.second == oHead.second ) )
                {
                    m_queWaitLift.push_back( oPair );
                    ret = STATUS_PENDING;
                }
                else
                {
                    ret = -EAGAIN;
                }

                oIfLock.Unlock();
                break;
            }

            if( ret == ERROR_STATE )
                break;

            if( ret == -EAGAIN )
                ret = STATUS_MORE_PROCESS_NEEDED;

            if( ERROR( ret ) )
                break;

        }while( 0 );

        return ret;
    }

    virtual gint32 OnReceiveData( CBuffer* pBuf )
    { return 0; }

    virtual gint32 StartEx( IEventSink* pCallback )
    {
        gint32 ret = 0;

        if( pCallback == nullptr )
            return -EINVAL;

        TaskletPtr pTask;
        CParamList oParams;
        oParams.SetPointer( propIfPtr, this );
        oParams[ propEventSink ] =
            ObjPtr( pCallback );

        ret = pTask.NewObj(
            clsid( CIfCallbackInterceptor ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            return ret;

        TaskletPtr pPostStart;
        ret = DEFER_CALL_NOSCHED( pPostStart,
            ObjPtr( this ),
            &CUnixSockStream::OnPostStart );

        if( ERROR( ret ) )
            return ret;

        CIfCallbackInterceptor* pCbi = pTask;
        pCbi->InsertCall( pPostStart );

        ret = this->super::StartEx( pCbi );

        if( ERROR( ret ) )
            return ret;

        if( ret == STATUS_PENDING )
            return ret;

        return OnPostStart();
    }

    virtual bool IsConnected(
        const char* szDestAddr = nullptr )
    {
        CRpcServices* pIf = m_pParent;
        if( !pIf->IsConnected( szDestAddr ) )
            return false;

        if( this->GetState() != stateConnected )
            return false;

        return true;
    }

    virtual gint32 OnPostStart()
    {
        gint32 ret = 0;

        do{
            // start the event listening
            CParamList oParams;

            oParams.CopyProp( propIfPtr, this );
            oParams.CopyProp( propIoMgr, this );
            oParams.CopyProp( propTimeoutSec, this );

            ret = m_pListeningTask.NewObj(
                clsid( CIfUxListeningTask ),
                oParams.GetCfg() );

            if( ERROR( ret ) )
                break;

            ret = this->RunManagedTask(
                m_pListeningTask );

            if( ERROR( ret ) )
                break;


            // start the ping ticker
            ret = m_pPingTicker.NewObj(
                clsid( CIfUxPingTicker ),
                oParams.GetCfg() );

            if( ERROR( ret ) )
                break;

            ret = this->RunManagedTask(
                m_pPingTicker );

            if( ERROR( ret ) )
                break;

            // start the data reading
            oParams.CopyProp(
                propKeepAliveSec, this );

            oParams.Push( ( bool )true );

            ret = m_pReadingTask.NewObj(
                clsid( CIfUxSockTransTask ),
                oParams.GetCfg() );
            if( ERROR( ret ) )
                break;

            ret = this->RunManagedTask(
                m_pReadingTask );

        }while( 0 );

        return ret;
    }

    virtual gint32 OnPostStop( IEventSink* pCallback ) 
    {
        gint32 ret = 0;

        do{
            CStdRMutex oIfLock( this->GetLock() );
            std::deque< std::pair< ObjPtr, ObjPtr > >
                queWaitLift = m_queWaitLift;
            m_queWaitLift.clear();
            oIfLock.Unlock();

            // clear the flowcontrol queue
            if( queWaitLift.empty() )
                break;

            for( auto oElem : queWaitLift )
            {
                TaskletPtr pTask = oElem.second;
                if( pTask.IsEmpty() )
                    continue;
                ( *pTask )( eventCancelTask );
            }

            if( !m_pPingTicker.IsEmpty() )
                ( *m_pPingTicker )( eventCancelTask );

            if( !m_pListeningTask.IsEmpty() )
                ( *m_pListeningTask )( eventCancelTask );

            if( !m_pReadingTask.IsEmpty() )
                ( *m_pReadingTask )( eventCancelTask );

        }while( 0 );

        return ret;
    }
};

class CUnixSockStmProxy : 
    public CUnixSockStream< CInterfaceProxy >
{
    public:
    typedef CUnixSockStream< CInterfaceProxy > super;
    CUnixSockStmProxy( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CUnixSockStmProxy ) ); }

    virtual gint32 OnPostStart()
    {
        gint32 ret = 0;
        do{
            CParamList oParams;

            oParams.CopyProp( propIfPtr, this );
            oParams.CopyProp( propIoMgr, this );
            oParams.CopyProp( propKeepAliveSec, this );

            TaskletPtr pTask;
            gint32 ret = pTask.NewObj(
                clsid( CIfDummyTask ),
                oParams.GetCfg() );

            if( ERROR( ret ) )
                break;

            ret = SendPingPong( tokPing, pTask );
            if( ret == STATUS_PENDING )
                ret = 0;

        }while( 0 );

        return ret;
    }
};

class CUnixSockStmServer : 
    public CUnixSockStream< CInterfaceServer >
{
    public:
    typedef CUnixSockStream< CInterfaceServer > super;
    CUnixSockStmServer( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CUnixSockStmServer ) ); }
};

class CUnixSockStmServerRelay :
    public CUnixSockStmServer
{
    public:
    typedef CUnixSockStmServer super;

    CUnixSockStmServerRelay( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CUnixSockStmServerRelay ) ); }
};

class CUnixSockStmProxyRelay :
    public CUnixSockStmProxy
{
    public:
    typedef CUnixSockStmProxy super;

    CUnixSockStmProxyRelay( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CUnixSockStmProxyRelay ) ); }
};

class CIfCreateUxSockStmTask :
    public CIfInterceptTaskProxy
{
    bool m_bServer = false;
    public:
    typedef CIfInterceptTaskProxy super;

    CIfCreateUxSockStmTask( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CIfCreateUxSockStmTask ) ); }

    gint32 RunTask();
    gint32 OnTaskComplete( gint32 iRet );
    gint32 GetResponse();
};

class CIfStartUxSockStmTask :
    public CIfInterceptTaskProxy
{
    bool m_bServer = false;
    public:
    typedef CIfInterceptTaskProxy super;

    CIfStartUxSockStmTask( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CIfStartUxSockStmTask ) ); }

    gint32 RunTask();
    gint32 OnTaskComplete( gint32 iRet );
};

class CIfStopUxSockStmTask :
    public CIfInterceptTaskProxy
{
    bool m_bServer = false;

    public:
    typedef CIfInterceptTaskProxy super;

    CIfStopUxSockStmTask( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CIfStopUxSockStmTask ) ); }

    gint32 RunTask();
    gint32 OnTaskComplete( gint32 iRet );
};
