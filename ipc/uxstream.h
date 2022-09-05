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
 *      Copyright:  2019 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#pragma once

#include "stream.h"

namespace rpcf
{

class CIfUxTaskBase :
    public CIfParallelTask
{
    protected:
    guint32 m_dwFCState = stateStarted;

    public:
    typedef CIfParallelTask super;

    CIfUxTaskBase( const IConfigDb* pCfg ) :
        super( pCfg )
    {;}

    bool IsPaused()
    {
        CStdRTMutex oLock( GetLock() );
        return m_dwFCState == statePaused;
    }

    virtual gint32 Pause()
    {
        gint32 ret = 0;
        do{
            CStdRTMutex oTaskLock( GetLock() );
            guint32 dwState = GetTaskState();
            if( dwState != stateStarted )
            {
                ret = ERROR_STATE;
                break;
            }

            if( IsPaused() )
                break;

            m_dwFCState = statePaused;

            SetTaskState( stateStarting );

            CCfgOpener oCfg(
                ( IConfigDb* )GetConfig() );

            CIoManager* pMgr = nullptr;
            ret = GET_IOMGR( oCfg, pMgr );
            if( ERROR( ret ) )
                break;

            ObjPtr pIrpObj;
            ret = oCfg.GetObjPtr(
                propIrpPtr, pIrpObj );

            if( ERROR( ret ) )
                break;

            oCfg.RemoveProperty( propIrpPtr );
            IrpPtr pIrp = pIrpObj;

            oTaskLock.Unlock();

            CStdRMutex oIrpLock( pIrp->GetLock() );
            ret = pIrp->CanContinue( IRP_STATE_READY );
            if( ERROR( ret ) )
                break;

            pIrp->RemoveCallback();
            pIrp->RemoveTimer();

            oIrpLock.Unlock();

            ret = pMgr->CancelIrp(
                pIrp, true, -ECANCELED );

        }while( 0 );

        return ret;
    }

    virtual gint32 Resume()
    {
        gint32 ret = 0;
        do{
            CStdRTMutex oLock( GetLock() );
            guint32 dwState = GetTaskState();
            if( dwState != stateStarting )
            {
                ret = ERROR_STATE;
                break;
            }

            if( !IsPaused() )
                break;

            m_dwFCState = stateStarted;

            CCfgOpener oCfg(
                ( IConfigDb* )GetConfig() );

            CIoManager* pMgr = nullptr;
            ret = GET_IOMGR( oCfg, pMgr );
            if( ERROR( ret ) )
                break;
            oLock.Unlock();

            ret = DEFER_CALL( pMgr, this,
                &IEventSink::OnEvent,
                eventZero, 0, 0, nullptr );

        }while( 0 );

        return ret;
    }

    gint32 ReRun()
    {
        gint32 ret = 0;
        do{
            CStdRTMutex oLock( GetLock() );
            SetTaskState( stateStarting );
            CCfgOpener oCfg(
                ( IConfigDb* )GetConfig() );

            if( IsPaused() )
                break;

            CIoManager* pMgr = nullptr;
            ret = GET_IOMGR( oCfg, pMgr );
            if( ERROR( ret ) )
                break;

            oLock.Unlock();
            ret = DEFER_CALL( pMgr, this,
                &IEventSink::OnEvent,
                eventZero, 0, 0, nullptr );

        }while( 0 );

        return ret;
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
    gint32 OnCancel( guint32 dwContext );
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
    gint32 OnCancel( guint32 dwContext );
};

class CIfUxSockTransTask :
    public CIfUxTaskBase
{

    bool m_bIn = true;

    InterfPtr m_pUxStream;
    BufPtr m_pPayload;
    guint32 m_dwTimeoutSec;
    guint32 m_dwKeepAliveSec;

    gint32 HandleResponse( IRP* pIrp );
    gint32 StartNewListening();
    gint32 PostData( BufPtr& pPayload );

    public:
    typedef CIfUxTaskBase super;

    CIfUxSockTransTask( const IConfigDb* pCfg );
    gint32 OnIrpComplete( IRP* pIrp );
    gint32 OnComplete( gint32 iRet );
    gint32 RunTask();

    gint32 OnCancel( guint32 dwContext )
    { return super::OnCancel( dwContext ); }

    inline void SetIoDirection( bool bIn )
    { m_bIn = bIn; }

    // whether reading from the uxsock or writing to
    // the uxsock
    inline bool IsReading()
    { return m_bIn; }

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

    gint32 OnTaskComplete( gint32 iRet );
};

class CIfUxListeningTask :
    public CIfUxTaskBase
{
    public:
    typedef CIfUxTaskBase super;

    CIfUxListeningTask( const IConfigDb* pCfg ) :
        super( pCfg )
    { SetClassId( clsid( CIfUxListeningTask ) ); }

    gint32 RunTask();
    gint32 PostEvent( BufPtr& pBuf );
    gint32 OnIrpComplete( IRP* pIrp );
    gint32 OnTaskComplete( gint32 iRet );
};

#define UXSOCK_MOD_SERVER_NAME "UnixSockStmServer"
#define UXSOCK_OBJNAME_SERVER "CUnixSockStmServer"               
#define UXSOCK_OBJNAME_PROXY  "CUnixSockStmProxy"               

enum EnumFCState : gint32
{
    fcsKeep,
    fcsFlowCtrl,
    fcsLift,
    fcsReport,
};

class CFlowControl
{
    // flow control for uxstream only
    stdrmutex   m_oFCLock;

    guint64     m_qwRxBytes = 0;
    guint64     m_qwTxBytes = 0;
    guint64     m_qwAckTxBytes = 0;
    guint64     m_qwAckRxBytes = 0;

    guint64     m_qwRxPkts = 0;
    guint64     m_qwTxPkts = 0;
    guint64     m_qwAckTxPkts = 0;
    guint64     m_qwAckRxPkts = 0;

    gint8       m_byFlowCtrl = 0;
    bool        m_bMonitor = false;

    EnumFCState OnReportInternal(
        guint64 qwAckTxBytes, guint64 qwAckTxPkts );

    public:

    inline stdrmutex& GetLock()
    { return m_oFCLock; }

    inline void SetAsMonitor( bool bMonitor )
    { m_bMonitor = bMonitor; }

    bool CanSend();
    EnumFCState IncTxBytes( guint32 dwSize );
    EnumFCState IncTxBytes( const BufPtr& pBuf );
    EnumFCState IncRxBytes( guint32 dwSize );
    EnumFCState IncRxBytes( const BufPtr& pBuf );
    EnumFCState IncFCCount();
    EnumFCState DecFCCount();
    EnumFCState OnFCReport( CfgPtr& pReport );
    CfgPtr GetReport();
    void GetBytesTransfered(
        guint64& qwRxBytes, guint64& qwTxBytes );
};

template< class T >
class CUnixSockStream:
    public T
{
    protected:
    InterfPtr   m_pParent;

    TaskletPtr  m_pPingTicker;
    TaskletPtr  m_pListeningTask;
    TaskletPtr  m_pReadingTask;

    // TaskletPtr m_pReadUxStmTask;
    // TaskletPtr m_pWriteUxStmTask;

    // bool        m_bFirstPing = true;

    CFlowControl m_oFlowCtrl;

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
            if( !oNewCfg.exist( propIfStateClass ) )
            {
                ret = oNewCfg.SetIntProp(
                    propIfStateClass, 
                    clsid( CUnixSockStmState ) ) ;
            }

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
                propParentPtr, pIf );
            if( ERROR( ret ) )
                break;

            CIoManager* pMgr = pIf->GetIoMgr();
            oNewCfg.SetPointer( propIoMgr, pMgr );

            std::string strVal;
            if( bServer )
                strVal = UXSOCK_OBJNAME_SERVER;
            else
                strVal = UXSOCK_OBJNAME_PROXY;

            // actually this object does not use 
            // dbus's mechanism, so the names are 
            // ignored for this object.
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

            // build a match
            MatchPtr pMatch;
            pMatch.NewObj( clsid( CMessageMatch ) );

            CCfgOpenerObj oMatch(
                ( CObjBase* )pMatch );

            oMatch.SetStrProp(
                propObjPath, strVal );

            gint32 iType = matchClient;
            if( bServer )
                iType = matchServer;

            oMatch.SetIntProp(
                propMatchType, iType );

            std::string strIfName = DBUS_IF_NAME(
                UXSOCK_OBJNAME_SERVER );

            oMatch.SetStrProp(
                propIfName, strIfName );

            oMatch.SetBoolProp(
                propPausable, false );

            ObjVecPtr pObjVec( true );
            ( *pObjVec )().push_back( pMatch );

            oNewCfg.SetObjPtr(
                propObjList, ObjPtr( pObjVec ) );

            // for the creation of CInterfaceState
            oNewCfg[ propIfName ] = strIfName;
            oNewCfg[ propMatchType ] = iType;
            oNewCfg[ propObjPath ] = strVal;

            strVal = "UnixSockBusPort_0";
            if( !oNewCfg.exist( propBusName ) )
            {
                oNewCfg.SetStrProp(
                    propBusName, strVal );
            }

            strVal = "UnixSockStmPdo";
            if( !oNewCfg.exist( propPortClass ) )
            {
                oNewCfg.SetStrProp(
                    propPortClass, strVal );
            }

            if( !oNewCfg.exist( propPortId ) )
            {
                oNewCfg.SetIntProp(
                    propPortId, dwFd );
            }

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
        super( InitCfg( pCfg ) )
    {
        if( pCfg == nullptr )
            return;

        gint32 ret = 0;
        do{
            CCfgOpenerObj oParams( this );
            ObjPtr pObj;
            oParams.GetObjPtr( propParentPtr, pObj );

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

            oParams.SetObjPtr(
                propDataDesc, pObj );

            this->RemoveProperty( propParentPtr );
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

    virtual IStream* GetParent()
    {
        return dynamic_cast< IStream* >
            ( ( IGenericInterface* )m_pParent );
    }

    inline void GetParent( InterfPtr& pIf ) const
    { pIf = m_pParent; }

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
                ret = OnDataReceived( pBuf );
                break;
            }
        case tokClose:
        case tokError:
            {
                BufPtr bufPtr( pBuf );
                ret = SendUxStreamEvent(
                    byToken, bufPtr );
                break;
            }
        }

        return ret;
    }

    gint32 SendUxStreamEvent(
        guint8 byToken, BufPtr& pBuf )
    {
        // forward to the parent IStream object
        gint32 ret = 0;
        do{
            HANDLE hChannel = ( HANDLE )this;
            IStream* pStream = GetParent();
            ret = pStream->OnUxStreamEvent(
                hChannel, byToken, *pBuf );

        }while( 0 );

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
            DEFER_IFCALLEX_NOSCHED( pTask,
                ObjPtr( m_pParent ),
                &IStream::OnUxStmEvtWrapper,
                hChannel,
                ( IConfigDb* )oParams.GetCfg() );

            // NOTE: the istream method is running on
            // the seqtask of the uxstream
            ret = this->AddSeqTask( pTask );
            if( ERROR( ret ) )
                ( *pTask )( eventCancelTask );

        }while( 0 );

        return ret;
    }

    virtual gint32 PostUxSockEvent(
        guint8 byToken, BufPtr& pBuf )
    {
        gint32 ret = 0;
        TaskletPtr pTask;
        ret = DEFER_IFCALLEX_NOSCHED( pTask,
            ObjPtr( this ),
            &CUnixSockStream::OnUxSockEventWrapper,
            byToken, *pBuf );

        if( ERROR( ret ) )
            return ret;

        ret = this->AddSeqTask( pTask );
        if( ERROR( ret ) )
            ( *pTask )( eventCancelTask );

        return ret;
    }

    // events from the ux port
    virtual gint32 OnDataReceived( CBuffer* pBuf )
    {
        gint32 ret = 0;

        if( !IsConnected() )
            return ERROR_STATE;

        do{
            if( pBuf == nullptr || pBuf->empty() )
            {
                ret = -EINVAL;
                break;
            }

            ret = m_oFlowCtrl.IncRxBytes( pBuf );

            BufPtr bufPtr( pBuf );
            SendUxStreamEvent( tokData, bufPtr );

            if( ret == fcsReport )
            {
                // send a progress notification
                CfgPtr pProgress =
                    m_oFlowCtrl.GetReport();

                BufPtr pRptBuf( true );
                *pRptBuf = ObjPtr( pProgress );
                SendUxStreamEvent( tokData, pRptBuf );
                ret = 0;
            }
            else
            {
                ret = 0;
            }

        }while( 0 );

        return ret;
    }
    // events from the ux port
    virtual gint32 OnPingPong( bool bPing )
    {
        gint32 ret = 0;

        if( !IsConnected() )
            return ERROR_STATE;

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
                    SendUxStreamEvent(
                        tokClose, pNullTask );
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
                if( ret == STATUS_PENDING )
                    ret = 0;
            }
            else
            {
                ret = -EINVAL;
            }

        }while( 0 );

        return ret;
    }

    virtual gint32 OnProgress( CBuffer* pBuf )
    {
        gint32 ret = 0;

        if( !IsConnected() )
            return ERROR_STATE;

        do{
            CfgPtr pReport( true );
            ret = pReport->Deserialize( *pBuf );
            if( ERROR( ret ) )
                break;

            ret = m_oFlowCtrl.OnFCReport( pReport );
            if( ret == fcsKeep )
            {
                ret = 0;
                break;
            }
            else if( ret == fcsLift )
            {
                BufPtr pBuf( true );
                ret = SendUxStreamEvent(
                    tokLift, pBuf );
            }
            else
            {
                ret = ERROR_FAIL;
            }

        }while( 0 );

        return ret;
    }

    // only happens when the underlying ux port's
    // output queue is full.
    virtual gint32 OnFlowControl()
    {
        gint32 ret =
            m_oFlowCtrl.IncFCCount();

        if( ret == fcsFlowCtrl )
        {
            BufPtr pBuf( true );
            SendUxStreamEvent(
                tokFlowCtrl, pBuf );
        }
        
        return 0;
    }

    // only happens when the underlying ux port's
    // output queue is full.
    virtual gint32 OnFCLifted()
    {
        gint32 ret = 0;

        if( !IsConnected() )
            return ERROR_STATE;

        ret = m_oFlowCtrl.DecFCCount();
        if( ret == fcsLift )
        {
            BufPtr pBuf( true );
            SendUxStreamEvent(
                tokLift, pBuf );
        }

        return 0;
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

        if( !IsConnected() )
            return ERROR_STATE;

        do{
            BufPtr pBuf( true );
            *pBuf = byToken;
            pBuf->Resize( sizeof( byToken ) );
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

        if( !IsConnected() )
            return ERROR_STATE;

        do{
            CParamList oParams;
            BufPtr pBuf( true );
            *pBuf = bPause ? tokFlowCtrl : tokLift;
            pBuf->Resize( sizeof( tokLift ) );
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

        if( !IsConnected() )
            return ERROR_STATE;

        do{
            CParamList oParams;
            oParams[ propMethodName ] = __func__;
            BufPtr pNewBuf( true );
            pNewBuf->Resize( UXBUF_OVERHEAD +
                pBuf->size() );

            if( ERROR( ret ) )
                break;

            pNewBuf->SetOffset( UXBUF_OVERHEAD -
                UXPKT_HEADER_SIZE );

            pNewBuf->ptr()[ 0 ] = tokProgress;
            guint32 dwSize = htonl( pBuf->size() );

            memcpy( pNewBuf->ptr() + 1,
                ( guint8* )&dwSize,
                sizeof( guint32 ) );

            memcpy( pNewBuf->ptr() + UXPKT_HEADER_SIZE, 
                ( guint8* )pBuf->ptr(),
                pBuf->size() );

            oParams.Push( pNewBuf );
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

        if( !IsConnected() )
            return ERROR_STATE;

        if( !CanSend() )
            return ERROR_QUEUE_FULL;

        do{
            CParamList oParams;
            oParams[ propMethodName ] = __func__;
            oParams.Push( pBuf );

            CStdRMutex oLock(
                m_oFlowCtrl.GetLock() );

            ret = SubmitRequest( oParams.GetCfg(),
                pCallback, true );

            if( ERROR( ret ) )
                break;

            gint32 iRet =
                m_oFlowCtrl.IncTxBytes( pBuf );

            if( iRet == fcsFlowCtrl )
            {
                oLock.Unlock();
                BufPtr pEmptyBuf( true );
                iRet = PostUxStreamEvent(
                    tokFlowCtrl, pEmptyBuf );
                if( ERROR( iRet ) )
                {
                    ret = iRet;
                    break;
                }
            }

        }while( 0 );

        return ret;
    }

    virtual gint32 StartListening(
        IEventSink* pCallback )
    {
        gint32 ret = 0;

        if( pCallback == nullptr )
            return -EINVAL;

        if( !IsConnected() )
            return ERROR_STATE;

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

        if( !IsConnected() )
            return ERROR_STATE;

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

            CCfgOpenerObj oIfCfg( this );
            CCfgOpenerObj oCfg( pCallback );
            IrpCtxPtr& pCtx = pIrp->GetTopStack(); 
            pCtx->SetMajorCmd( IRP_MJ_FUNC );
            pIrp->SetCallback( pCallback, 0 );
            pIrp->SetIrpThread(
                this->GetIoMgr() );

            if( strMethod == "StartReading" )
            {
                pCtx->SetMinorCmd( IRP_MN_READ );
                pCtx->SetIoDirection( IRP_DIR_IN );
            }
            else if( strMethod == "WriteStream" )
            {
                BufPtr pBuf;
                oParams.GetBufPtr( 0, pBuf );
                pCtx->SetMinorCmd( IRP_MN_WRITE );
                pCtx->SetIoDirection( IRP_DIR_OUT );
                pCtx->SetReqData( pBuf );

                guint32 dwTimeoutSec = 0;
                ret = oIfCfg.GetIntProp(
                    propKeepAliveSec, dwTimeoutSec );
                if( ERROR( ret ) )
                    break;

                pIrp->SetTimer( dwTimeoutSec,
                    this->GetIoMgr() );
            }
            else if( strMethod == "StartListening" )
            {
                IrpCtxPtr& pCtx = pIrp->GetTopStack(); 
                pCtx->SetMinorCmd( IRP_MN_IOCTL );
                pCtx->SetCtrlCode( CTRLCODE_LISTENING );
                pCtx->SetIoDirection( IRP_DIR_IN );

                guint32 dwTimeoutSec = 0;
                ret = oIfCfg.GetIntProp(
                    propTimeoutSec, dwTimeoutSec );
                if( ERROR( ret ) )
                    break;

                pIrp->SetTimer( dwTimeoutSec,
                    this->GetIoMgr() );
            }
            else if( strMethod == "SendPingPong" ||
                strMethod== "SendProgress" )
            {
                BufPtr pBuf;
                oParams.GetBufPtr( 0, pBuf );
                pCtx->SetMinorCmd( IRP_MN_IOCTL );
                pCtx->SetCtrlCode( CTRLCODE_STREAM_CMD );
                pCtx->SetIoDirection( IRP_DIR_OUT );
                pCtx->SetReqData( pBuf );

                guint32 dwTimeoutSec = 0;
                ret = oIfCfg.GetIntProp(
                    propKeepAliveSec, dwTimeoutSec );
                if( ERROR( ret ) )
                    break;

                pIrp->SetTimer( dwTimeoutSec,
                    this->GetIoMgr() );
            }
            else if( strMethod == "PauseReading" )
            {
                BufPtr pBuf;
                oParams.GetBufPtr( 0, pBuf );
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

        if( !IsConnected() )
            return ERROR_STATE;

        do{
            CCfgOpenerObj oCfg( pCallback );

            IPort* pPort = this->GetPort();
            pPort = pPort->GetTopmostPort();

            IrpPtr pIrp( true );
            ret = pIrp->AllocNextStack( nullptr );
            if( ERROR( ret ) )
                break;

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

            if( ERROR( ret ) )
                break;

            if( !bSend )
            {
                IrpCtxPtr& pCtx = pIrp->GetTopStack();
                CCfgOpenerObj oCallback( pCallback );
                CParamList oParams;
                oParams[ propReturnValue ] = ret;
                if( !pCtx->m_pRespData.IsEmpty() &&
                    !pCtx->m_pRespData->empty() )
                    oParams.Push( pCtx->m_pRespData );

                oCallback.SetObjPtr( propRespPtr,
                    ObjPtr( oParams.GetCfg() ) );
            }

        }while( 0 );

        if( ret == -EAGAIN )
            ret = STATUS_MORE_PROCESS_NEEDED;

        return ret;
    }

    virtual bool IsConnected(
        const char* szDestAddr = nullptr )
    {
        if( this->GetState() != stateConnected )
            return false;

        CRpcServices* pIf = m_pParent;
        if( !pIf->IsConnected( szDestAddr ) )
            return false;

        return true;
    }

    virtual gint32 StartTicking(
        IEventSink* pContext )
    {
        gint32 ret = 0;

        do{
            // start the event listening
            CParamList oParams;
            oParams.SetPointer( propIfPtr, this );

            oParams.SetPointer(
                propIoMgr, this->GetIoMgr() );

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
            oParams.SetPointer( propIfPtr, this );
            oParams.SetPointer( propIoMgr, this );
            oParams.CopyProp( propTimeoutSec, this );
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

            if( !ERROR( ret ) )
                ret = 0;

        }while( 0 );

        return ret;
    }

    virtual gint32 OnPreStop( IEventSink* pCallback ) 
    {
        gint32 ret = 0;

        do{
            if( !m_pPingTicker.IsEmpty() )
                ( *m_pPingTicker )( eventCancelTask );

            if( !m_pListeningTask.IsEmpty() )
                ( *m_pListeningTask )( eventCancelTask );

            if( !m_pReadingTask.IsEmpty() )
                ( *m_pReadingTask )( eventCancelTask );

        }while( 0 );

        return ret;
    }

    gint32 OnPostStop( IEventSink* pCallback )
    {
        super::OnPostStop( pCallback );
        m_pParent.Clear();
        m_pListeningTask.Clear();
        m_pPingTicker.Clear();
        m_pReadingTask.Clear();
        DebugPrintEx( logInfo, 0,
            "a CUnixSockStream object"
            "@0x%x is over", this );
        return 0;
    }

    inline bool CanSend()
    {
        return m_oFlowCtrl.CanSend();
    }

    inline void GetBytesTransfered(
        guint64& qwRxBytes, guint64& qwTxBytes )
    {
        return m_oFlowCtrl.GetBytesTransfered(
            qwRxBytes, qwTxBytes );
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

    virtual gint32 OnPostStart(
        IEventSink* pCallback )
    {
        gint32 ret = 0;
        do{
            ret = super::OnPostStart( pCallback );
            if( ERROR( ret ) )
                break;

            CParamList oParams;
            CIoManager* pMgr = GetIoMgr();
            oParams.SetPointer( propIfPtr, this );
            oParams.SetPointer( propIoMgr, pMgr );
            oParams.CopyProp(
                propKeepAliveSec, this );

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

}
