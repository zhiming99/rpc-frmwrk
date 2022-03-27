/*
 * =====================================================================================
 *
 *       Filename:  streamex.h
 *
 *    Description:  Declaration CStreamServerEx and CStreamProxyEx and the worker task
 *                  CIfStmReadWriteTask
 *
 *        Version:  1.0
 *        Created:  08/22/2019 03:23:25 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  Ming Zhi( woodhead99@gmail.com )
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

#include "configdb.h"
#include "defines.h"
#include "ifhelper.h"
#include "uxstream.h"
#include <exception>
#include <sys/eventfd.h>
#include <fcntl.h>
#include "seribase.h"

namespace rpcf
{

#define CTRLCODE_READMSG    0x1000
#define CTRLCODE_READBLK    0x1001

#define CTRLCODE_WRITEMSG   0x1002
#define CTRLCODE_WRITEBLK   0x1003

#define SYNC_STM_MAX_PENDING_PKTS ( STM_MAX_PACKETS_REPORT + 2 )

#define POST_LOOP_EVENT( _hChannel, _iEvt, _ret ) \
do{ \
    if( !IsLoopStarted() ) break;\
    gint32 _iRet1 = ( gint32 )( _ret ); \
    BufPtr pBuf( true ); \
    pBuf->Resize( sizeof( guint8 ) + \
        sizeof( _hChannel ) ); \
    pBuf->ptr()[ 0 ] = _iEvt; \
    memcpy( pBuf->ptr() + 1, \
        &_hChannel, \
        sizeof( _hChannel ) ); \
    if( _iEvt == stmevtSendDone || \
        _iEvt == stmevtRecvData ) \
        pBuf->Append( ( guint8* )&_iRet1, sizeof( _iRet1 ) ); \
    PostLoopEvent( pBuf ); \
}while( 0 )

#define POST_SEND_DONE_EVT( _hChannel1, _ret1 ) \
    POST_LOOP_EVENT( _hChannel1, stmevtSendDone, _ret1 )

#define POST_RECV_DATA_EVT( _hChannel1, _ret1 ) \
    POST_LOOP_EVENT( _hChannel1, stmevtRecvData, _ret1 )

enum EnumStreamEvent : guint8
{
    stmevtInvalid = 0,
    stmevtRecvData,
    stmevtLifted,
    stmevtSendDone,
    stmevtLoopStart,
    stmevtCloseChan,
};

class CIfStmReadWriteTask :
    public CIfUxTaskBase
{
    protected:

    std::deque< IrpPtr > m_queRequests;
    std::deque< BufPtr > m_queBufRead;

    struct IRPCTX_EXT
    {
        ObjPtr pCallback;
        guint32 dwOffset;
        guint32 dwTailOff;
        IRPCTX_EXT() :
            pCallback(),
            dwOffset( 0 ),
            dwTailOff( 0 )
        {
        }

        IRPCTX_EXT( const IRPCTX_EXT& rhs )
        {
            dwOffset = rhs.dwOffset;
            dwTailOff = rhs.dwTailOff;
            pCallback = rhs.pCallback;
        }

        ~IRPCTX_EXT()
        {
            pCallback.Clear();
        }
    };

    BufPtr  m_pSendBuf;
    bool    m_bIn;
    bool    m_bDiscard = false;
    HANDLE  m_hChannel = INVALID_HANDLE;

    gint32 ReadStreamInternal(
        IEventSink* pCallback,
        BufPtr& pBuf, bool bNoWait );

    gint32 SetRdRespForIrp(
        PIRP pIrp, BufPtr& pBuf );

    gint32 CompleteReadIrp(
        IrpPtr& pIrp, BufPtr& pBuf );

    gint32 WriteStreamInternal(
        IEventSink* pCallback,
        BufPtr& pBuf, bool bNoWait );

    gint32 OnIoIrpComplete( PIRP pIrp );
    gint32 OnWorkerIrpComplete( PIRP pIrp );

    gint32 AdjustSizeToWrite( IrpPtr& pIrp,
        BufPtr& pBuf, bool bSlide = false );

    public:
    typedef CIfUxTaskBase super;

    CIfStmReadWriteTask( const IConfigDb* pCfg );

    gint32 GetMsgCount() const
    {
        if( !IsReading() )
            return 0;

        CStdRTMutex oLock( GetLock() );
        return m_queBufRead.size();
    }

    gint32 GetReqCount() const
    {
        CStdRTMutex oLock( GetLock() );
        return m_queRequests.size();
    }

    // pause the task from read/write
    gint32 Pause()
    {
        if( IsReading() )
            PauseReading( true );
        return super::Pause();
    }
    gint32 Resume()
    {
        if( IsReading() )
            PauseReading( false );
        return super::Resume();
    }

    /**
    * @name SetDiscard
    * @{ set whether to discard the incoming
    * stream data. If you don't want to receive
    * the incoming data, but keep the stream
    * channel alive, call SetDiscard with true.
    * Don't use PauseReading which will block the
    * management events ping/pong or report to
    * happen and the channel will lose due to
    * timeout.
    * */
    /**
     * Parameter(s):
     *  bDiscard: true to let the task discard all
     *  the incoming data from OnStmRecv, and
     *  false to let the task queue the incoming
     *  data in the queue.
     * @} */

    // discard the incoming data.
    gint32 SetDiscard( bool bDiscard )
    {
        if( !IsReading() )
            return 0;

        CStdRTMutex oTaskLock( GetLock() );
        m_bDiscard = bDiscard;
        if( bDiscard )
        {
            if( m_queBufRead.size() )
                m_queBufRead.clear();
        }

        // resume the reading task, otherwise the
        // channel could be closed if ping/pong,
        // report message can not be received.
        if( IsPaused() )
            Resume();

        return 0;
    }

    gint32 OnIrpComplete( IRP* pIrp );
    gint32 OnCancel( guint32 dwContext );
    gint32 RunTask();

    inline bool IsReading() const
    { return m_bIn; }

    gint32 ReadStream( BufPtr& pBuf )
    {
        return ReadStreamInternal(
            nullptr, pBuf, false );
    }

    gint32 ReadStreamAsync(
        IEventSink* pCallback, BufPtr& pBuf )
    {
        return ReadStreamInternal(
            pCallback, pBuf, false );
    }

    gint32 ReadStreamNoWait(
        BufPtr& pBuf )
    {
        if( !pBuf.IsEmpty() )
            pBuf.Clear();
        return ReadStreamInternal(
            nullptr, pBuf, true );
    }

    gint32 WriteStream( BufPtr& pBuf )
    {
        return WriteStreamInternal(
            nullptr, pBuf, false );
    }

    gint32 WriteStreamAsync(
        IEventSink* pCallback, BufPtr& pBuf )
    {
        return WriteStreamInternal(
            pCallback, pBuf, false );
    }

    gint32 WriteStreamNoWait( BufPtr& pBuf )
    {
        return WriteStreamInternal(
            nullptr, pBuf, true );
    }

    gint32 OnFCLifted();
    gint32 PauseReading( bool bPause );
    gint32 OnStmRecv( BufPtr& pBuf );
    gint32 PostLoopEvent( BufPtr& pBuf );
    bool IsLoopStarted();
    bool CanSend();
    gint32 ReschedRead();
    gint32 PeekStream( BufPtr& pBuf );
    gint32 GetPendingBytes( guint32& dwBytes ) const;
    gint32 GetPendingReqs( guint32& dwCount ) const;
    inline bool IsReport( const BufPtr& pBuf ) const
    { return pBuf->GetExDataType() == typeObj; }
    gint32 SendProgress( BufPtr& pBuf ) const;
    gint32 GetUxStream( InterfPtr& pIf ) const;

};


class CReadWriteWatchTask : public CTasklet
{
    public:

    typedef CTasklet super;
    CReadWriteWatchTask ( const IConfigDb* pCfg )
        : super( pCfg )
    {
        SetClassId( clsid( CReadWriteWatchTask ) );
    }
    gint32 operator()( guint32 dwContext );
};

/**
* @name CStreamSyncBase
* @{ */
/**
 * this class is for ux stream object management
 * and I/O management. 
 * It provides easier API for upper user.
 * @} */

template < class T >
struct CStreamSyncBase :
    public T
{
    struct WORKER_ELEM
    {
        CfgPtr pContext;
        TaskletPtr pReader;
        TaskletPtr pWriter;
        bool    bPauseNotify = false;

        WORKER_ELEM()
        {;}

        WORKER_ELEM( const WORKER_ELEM& rhs )
        {
            pReader = rhs.pReader;
            pWriter = rhs.pWriter;
            pContext = rhs.pContext;
            bPauseNotify = rhs.bPauseNotify;
        }

        ~WORKER_ELEM()
        {
            pReader.Clear();
            pWriter.Clear();
            pContext.Clear();
        }
    };

    typedef std::map< HANDLE, WORKER_ELEM > WORKER_MAP;
    WORKER_MAP m_mapStmWorkers;

    typedef T super;
    CStreamSyncBase( const IConfigDb* pCfg ) :
        super::_MyVirtBase( pCfg ), super( pCfg )
    {;}

    TaskletPtr m_pRecvWatch;
    TaskletPtr m_pSendWatch;

    MloopPtr m_pRdLoop;
    int m_iPiper = -1, m_iPipew = 1;
    HANDLE m_hEvtWatch = INVALID_HANDLE;

    gint32 StopLoop( gint32 iRet = 0 )
    {
        guint32 ret = 0;
        if( !IsLoopStarted() )
            return 0;

        CMainIoLoop* pLoop = m_pRdLoop;
        if( pLoop == nullptr )
            return -EFAULT;

        if( pLoop->GetError() !=
            STATUS_PENDING )
            iRet = pLoop->GetError();
        pLoop->WakeupLoop( aevtStop, iRet );
        return ret;
    }

    gint32 InstallIoWatch()
    {
        gint32 ret = 0;
        int pipefds[ 2 ] = {-1};
        do{
            gint32 ret = pipe( pipefds );
            if( ret == -1 )
            {
                ret = -errno;
                break;
            }

            m_iPiper = pipefds[ 0 ];
            m_iPipew = pipefds[ 1 ];

            int iFlags = fcntl(
                m_iPiper, F_GETFL );

            ret = fcntl( m_iPiper, F_SETFL,
                iFlags | O_NONBLOCK );

            if( ERROR( ret ) )
            {
                ret = -errno;
                break;
            }

            CParamList oParams;

            // the fd to watch on
            oParams.Push(
                ( guint32 )m_iPiper );

            // watch on the read events
            oParams.Push(
                ( guint32 )POLLIN );

            // start watch immediately
            oParams.Push( true );

            oParams.SetPointer(
                propIfPtr, this );

            TaskletPtr pTask;
            ret = pTask.NewObj(
                clsid( CReadWriteWatchTask ),
                oParams.GetCfg() );

            if( ERROR( ret ) )
                break;

            CMainIoLoop* pLoop = m_pRdLoop;
            if( pLoop == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            ret = pLoop->AddIoWatch(
                pTask, m_hEvtWatch );

            if( ERROR( ret ) )
                break;

            BufPtr pBuf( true );
            *pBuf = stmevtLoopStart;
            ret = PostLoopEvent( pBuf );

        }while( 0 );

        if( ERROR( ret ) )
        {
            if( pipefds[ 0 ] >= 0 )
                close( pipefds[ 0 ] );

            if( pipefds[ 1 ] >= 0 )
                close( pipefds[ 1 ] );
        }

        return ret;
    }

    gint32 RemoveIoWatch()
    {
        if( m_hEvtWatch == INVALID_HANDLE )
            return 0;

        CMainIoLoop* pLoop = m_pRdLoop;
        if( pLoop != nullptr )
        {
            pLoop->RemoveIoWatch(
                m_hEvtWatch );

            m_hEvtWatch = INVALID_HANDLE;

            if( m_iPiper >= 0 )
                close( m_iPiper );

            if( m_iPipew >= 0 )
                close( m_iPipew );

            m_iPiper = m_iPipew = -1;
        }

        return 0;
    }

    /**
    * @name OnRecvData_Loop @{ an event handler
    * for the server/proxy to handle the incoming
    * data. NOTE that, the suffix _Loop indicating
    * it is a event handler triggered only if
    * IsLoopStarted is true, that is, this stream
    * object currently is running a user created
    * mainloop. And the following XXX_Loop methods
    * apply to the same condition.
    * */
    /**
     * Parameter(s):
     * hChannel: the handle to the stream channel.
     * you can call ReadStream with this parameter
     * to get the buffer containing the incoming
     * message or data block.
     *
     * Return value:
     * 
     * @} */

    virtual gint32 OnRecvData_Loop(
        HANDLE hChannel, gint32 iRet ) = 0;
    /**
    * @name OnSendDone_Loop
    * @{ an event handler called when the current
    * WriteMsg/WriteBlock requests are completed.
    * The WriteMsg/WriteBlock usually return
    * STATUS_PENDING after the request is
    * submitted. and when the request is done,
    * OnSendDone_Loop will be called to indicate
    * the write request is finished, with the
    * error code in iRet. All the write requests
    * are services in the FIFO manner. so if you
    * submit more than one WriteXXX requests,
    * which returns pending all the time. The
    * current OnSendDone_Loop must be the first
    * write request which is yet to complete.
    * */
    /** 
     * Parameter(s):
     * hChannel: the handle to the stream channel
     * on the the Write request happens.
     * iRet: the error code of the earliest write
     * requests, which returns pending.
     * @} */
    virtual gint32 OnSendDone_Loop(
        HANDLE hChannel, gint32 iRet ) = 0;
    /**
    * @name OnWriteEnabled_Loop
    * @{ an event handler called since the
    * latest WriteMsg/WriteBlock returns
    * ERROR_QUEUE_FULL, a notification to the
    * caller, whether server/proxy, to resume the
    * earlier failed request.
    * */
    /** 
     * Parameter(s):
     * hChannel: the handle to the stream channel
     * on which the flow control is lifted.
     * @} */
    virtual gint32 OnWriteEnabled_Loop(
        HANDLE hChannel ) = 0;

    /**
    * @name OnCloseChannel_Loop
    * @{ an event handler called to notify that a
    * stream channel is closing in process. To
    * give client a chance to do some cleanup
    * work.
    * */
    /** 
     * Parameter(s):
     * hChannel: the handle to the stream channel
     * which will be closed.
     *
     * return value:
     * ignored.
     * @} */
    
    virtual gint32 OnCloseChannel_Loop(
        HANDLE hChannel ) = 0;
    /**
    * @name OnStart_Loop
    * @{ an event handler which is the first event
    * to the server/proxy after the loop is
    * started. For the usage, please refer to
    * stmcli.cpp. */
    /**  @} */
    virtual gint32 OnStart_Loop() = 0;

#define HANDLE_RETURN_VALUE( _ret, iErrNoData ) \
{ \
    if( _ret == -1 && errno == EAGAIN ) \
    { \
        _ret = iErrNoData; \
        break; \
    } \
    else if( _ret == -1 ) \
    { \
        _ret = -errno; \
        break; \
    } \
    else if( _ret == 0 ) \
    { \
        _ret = -ENODATA; \
        break; \
    } \
    else _ret = 0; \
}

    gint32 DispatchReadWriteEvent(
        guint32 dwParam1 )
    {
        if( dwParam1 != G_IO_IN )
            return ERROR_FAIL;

        if( m_iPiper < 0 )
            return ERROR_STATE;

        gint32 ret = 0;
        guint8 byEvent = stmevtInvalid;

        do{
            ret = read( m_iPiper, &byEvent,
                sizeof( byEvent ) );

            HANDLE_RETURN_VALUE(
                ret, STATUS_PENDING );

            switch( byEvent )
            {
            case stmevtLoopStart:
                {
                    ret = OnStart_Loop();
                    break;
                }
            case stmevtRecvData:
                {
                    HANDLE hChannel = INVALID_HANDLE;
                    ret = read( m_iPiper, &hChannel,
                        sizeof( hChannel ) );

                    HANDLE_RETURN_VALUE(
                        ret, -EBADMSG );

                    gint32 iRet = 0;
                    ret = read( m_iPiper, &iRet,
                        sizeof( iRet ) );

                    HANDLE_RETURN_VALUE(
                        ret, -EBADMSG );

                    OnRecvData_Loop( hChannel, iRet );
                    break;
                }
            case stmevtCloseChan:
                {
                    HANDLE hChannel = INVALID_HANDLE;
                    ret = read( m_iPiper, &hChannel,
                        sizeof( hChannel ) );

                    HANDLE_RETURN_VALUE(
                        ret, -EBADMSG );
                    OnCloseChannel_Loop( hChannel );
                    break;
                }
            case stmevtLifted:
                {
                    HANDLE hChannel = INVALID_HANDLE;
                    ret = read( m_iPiper, &hChannel,
                        sizeof( hChannel ) );

                    HANDLE_RETURN_VALUE(
                        ret, -EBADMSG );
                    OnWriteEnabled_Loop( hChannel );
                    break;
                }
            case stmevtSendDone:
                {
                    HANDLE hChannel = INVALID_HANDLE;
                    ret = read( m_iPiper, &hChannel,
                        sizeof( hChannel ) );

                    HANDLE_RETURN_VALUE(
                        ret, -EBADMSG );

                    gint32 iRet = 0;
                    ret = read( m_iPiper, &iRet,
                        sizeof( iRet ) );

                    HANDLE_RETURN_VALUE(
                        ret, -EBADMSG );

                    OnSendDone_Loop( hChannel, iRet );
                    break;
                }
            default:
                    ret = -ENOTSUP;
                    break;
            }

        }while( 0 );

        return ret;
    }

    gint32 OnEvent( EnumEventId iEvent,
            LONGWORD dwParam1 = 0,
            LONGWORD dwParam2 = 0,
            LONGWORD* pData = NULL  )
    {
        if( iEvent == eventIoWatch )
        {
            return DispatchReadWriteEvent(
                dwParam1 );
        }

        return super::OnEvent( iEvent,
            dwParam1, dwParam2, pData );
    }

    gint32 StartLoop(
        IEventSink* pStartTask = nullptr )
    {
        gint32 ret = 0;
        do{
            CParamList oCfg;

            oCfg.SetPointer( propIoMgr,
                this->GetIoMgr() );
            std::string strName( "MainLoop-2" );

            // thread name
            oCfg.Push( strName );

            // don't start new thread
            oCfg.Push( false );

            ret = m_pRdLoop.NewObj(
                clsid( CMainIoLoop ),
                oCfg.GetCfg() );

            if( ERROR( ret ) )
                break;

            ret = InstallIoWatch();
            if( ERROR( ret ) )
                break;

            CMainIoLoop* pLoop = m_pRdLoop;
            if( unlikely( pLoop == nullptr ) )
            {
                ret = -EFAULT;
                break;
            }
            if( pStartTask != nullptr )
            {
                TaskletPtr pTask = 
                    ObjPtr( pStartTask );
                pLoop->AddTask( pTask );
            }

            pLoop->Start();
            RemoveIoWatch();
            // get the reason the loop quits
            ret = pLoop->GetError();

        }while( 0 );

        return ret;
    }

    bool IsLoopStarted()
    {
        if( m_pRdLoop.IsEmpty() )
            return false;

        CMainIoLoop* pLoop = m_pRdLoop;
        if( pLoop == nullptr )
            return false;

        return ( 0 !=
            pLoop->GetThreadId() );
    }

    gint32 RunManagedTaskWrapper(
        IEventSink* pTask,
        const bool& bRoot )
    {
        gint32 ret = this->
            RunManagedTask( pTask, bRoot );
        if( ret == STATUS_PENDING )
            ret = 0;
        return ret;
    }

    gint32 PostLoopEventWrapper( CBuffer* pBuf )
    {
        if( pBuf == nullptr )
            return -EINVAL;

        BufPtr ptrBuf( ( CBuffer* )pBuf );
        return PostLoopEvent( ptrBuf );
    }

    gint32 PostLoopEvent( BufPtr& pBuf )
    {
        if( !IsLoopStarted() && 
            m_hEvtWatch == INVALID_HANDLE )
            return ERROR_STATE;

        if( pBuf.IsEmpty() || pBuf->empty() )
            return -EINVAL;

        gint32 ret = write( m_iPipew,
            pBuf->ptr(), pBuf->size() );

        if( ret == -1 )
            return -errno;

        return 0;
    }
    
    gint32 CloseStream( HANDLE hChannel )
    {
        if( hChannel == INVALID_HANDLE )
            return -EINVAL;

        gint32 ret = 0;
        do{
            CParamList oParams;
            oParams.SetPointer( propIoMgr,
                this->GetIoMgr() );

            TaskletPtr pSyncTask;
            ret = pSyncTask.NewObj(
                clsid( CSyncCallback ),
                oParams.GetCfg() );

            if( ERROR( ret ) )
                break;

            ret = this->CloseChannel(
                hChannel, pSyncTask );
            if( ret == STATUS_PENDING )
            {
                CSyncCallback* pscb = pSyncTask;
                pscb->WaitForComplete();
                ret = pSyncTask->GetError();
            }

        }while( 0 );

        return ret;
    }

    gint32 OnFCLifted( HANDLE hChannel )
    {
        TaskletPtr pTask;

        gint32 ret = 0;
        do{
            GetWorker( hChannel, false, pTask );
            if( ERROR( ret ) )
                break;

            CIfStmReadWriteTask* pWriter = pTask;
            ret = pWriter->OnFCLifted();
            if( ERROR( ret ) )
                break;

        }while( 0 );

        return ret;
    }

    virtual gint32 StopWorkers( HANDLE hChannel )
    {
        if( hChannel == INVALID_HANDLE )
            return -EINVAL;

        gint32 ret = 0;
        do{
            CStdRMutex oIfLock( this->GetLock() );
            typename WORKER_MAP::iterator itr = 
                m_mapStmWorkers.find( hChannel );

            if( itr == m_mapStmWorkers.end() )
            {
                ret = -ENOENT;
                break;
            }

            WORKER_ELEM oWorker = itr->second;
            oIfLock.Unlock();

            if( !oWorker.pReader.IsEmpty() )
            {
                ( *oWorker.pReader )(
                    eventCancelTask );
            }
            if( !oWorker.pWriter.IsEmpty() )
            {
                ( *oWorker.pWriter )(
                    eventCancelTask );
            }

            oIfLock.Lock();
            itr = m_mapStmWorkers.find( hChannel );
            if( itr == m_mapStmWorkers.end() )
                break;

            m_mapStmWorkers.erase( hChannel );

        }while( 0 );

        return ret;
    }

    /**
    * @name OnStmClosing 
    * event handler called whenever a uxstream is
    * about to be closed.
    * @{ */
    /**
     * Override this method for more cleanup
     * works.
     * @} */
    virtual gint32 OnStmClosing( HANDLE hChannel )
    {
        gint32 ret = StopWorkers( hChannel );
        if( IsLoopStarted() )
        {
            BufPtr pBuf( true );
            pBuf->Resize( sizeof( guint8 ) +
                sizeof( hChannel ) );
            pBuf->ptr()[ 0 ] = stmevtCloseChan;
            memcpy( pBuf->ptr() + 1,
                &hChannel, sizeof( hChannel ) );
            PostLoopEvent( pBuf );
        }
        return ret;
    }
    /**
    * @name OnClose 
    * event handler called when the underlying ux
    * stream received an tokClose signal weither
    * from the uxport or due to an internal fatal
    * error.
    * @{ */
    /**
     * It won't be called when active Stop
     * happens.
     * @} */
    
    gint32 OnClose( HANDLE hChannel,
        IEventSink* pCallback = nullptr )
    {
        if( hChannel == INVALID_HANDLE )
            return -EINVAL;

        OnStmClosing( hChannel );

        return super::OnClose(
            hChannel, pCallback );
    }

    /**
    * @name OnStreamReady
    * event when the stream is fully initialized
    * by the CStreamSyncBase, that is, the
    * read/write tasks are running. if the loop is
    * started, this event won't be sent, since the
    * loop has its own start event
    * @{ */
    /**  @} */
    
    virtual gint32 OnStreamReady(
        HANDLE hStream )
    { return 0; }

    gint32 OnStmStartupComplete(
        IEventSink* pCallback,
        gint32 iRet, HANDLE hStream )
    {
        if( SUCCEEDED( iRet ) ) 
            return 0;

        TaskletPtr pTask;
        pTask.NewObj( clsid( CIfDummyTask ) ); 
        OnClose( hStream, pTask );    
        return 0;
    }

    /**
    * @name OnConnected
    * event when a new stream is setup between the
    * proxy and the server. At this moment, both
    * ends can send/receive. But for
    * CStreamSyncBase, there remains some more
    * initialization work to do.
    * @{ */
    /**  @} */

    gint32 OnConnected( HANDLE hCfg )
    {
        WORKER_ELEM oWorker;
        gint32 ret = 0;
        DebugPrint( 0, "OnConnected..." );

        TaskGrpPtr pTaskGrp;
        HANDLE hChannel = INVALID_HANDLE;
        do{
            CStdRMutex oIfLock( this->GetLock() );
            if( !this->IsConnected( nullptr ) )
            {
                ret = ERROR_STATE;
                break;
            }

            IConfigDb* pResp = reinterpret_cast
                < IConfigDb* >( hCfg );
            if( pResp == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            CCfgOpener oResp( pResp );
            hChannel = ( HANDLE& )oResp[ 1 ];

            typename WORKER_MAP::iterator itr = 
                m_mapStmWorkers.find( hChannel );

            if( itr != m_mapStmWorkers.end() )
            {
                ret = -EEXIST;
                break;
            }

            CIoManager* pMgr = this->GetIoMgr();
            CParamList oParams;

            // reader task;
            oParams.Push( true );
            oParams.Push( hChannel );

            oParams.SetPointer(
                propIfPtr, this );

            oParams.SetPointer( propIoMgr, pMgr );

            InterfPtr pIf;
            ret = this->GetUxStream( hChannel, pIf );
            if( ERROR( ret ) )
                break;

            ret = oParams.CopyProp(
                propTimeoutSec, (CObjBase* )pIf );
            if( ERROR( ret ) )
                break;

            ret = oWorker.pReader.NewObj(
                clsid( CIfStmReadWriteTask ),
                oParams.GetCfg() );

            if( ERROR( ret ) )
                break;

            ret = oWorker.pContext.NewObj();
            if( ERROR( ret ) )
                break;

            if( !this->IsServer() )
            {
                ObjPtr& pObj = oResp[ 0 ];
                IConfigDb* pDataDesc = pObj;

                CCfgOpener oCtx( ( IConfigDb* )
                    oWorker.pContext );

                oCtx.CopyProp(
                    propPeerObjId, pDataDesc );
            }

            // writer task
            oParams.SetBoolProp( 0, false );
            ret = oWorker.pWriter.NewObj(
                clsid( CIfStmReadWriteTask ),
                oParams.GetCfg() );

            if( ERROR( ret ) )
                break;

            oWorker.bPauseNotify = true;
            m_mapStmWorkers.insert(
                std::pair< HANDLE, WORKER_ELEM >
                ( hChannel, oWorker ) );

            oIfLock.Unlock();

            CParamList oGrpCfg;
            oGrpCfg[ propIfPtr ] = ObjPtr( this );
            ret = pTaskGrp.NewObj(
                clsid( CIfTaskGroup ),
                oGrpCfg.GetCfg() );

            if( ERROR( ret ) )
                break;
     
            pTaskGrp->SetRelation( logicAND );
            TaskletPtr pStartReader;
            ret = DEFER_IFCALLEX_NOSCHED(
                pStartReader,
                ObjPtr( this ),
                &CStreamSyncBase::RunManagedTaskWrapper,
                ( IEventSink* )oWorker.pReader,
                false );

            if( ERROR( ret ) )
                break;

            pTaskGrp->AppendTask( pStartReader );

            TaskletPtr pStartWriter;
            ret = DEFER_IFCALLEX_NOSCHED( 
                pStartWriter,
                ObjPtr( this ),
                &CStreamSyncBase::RunManagedTaskWrapper,
                ( IEventSink* )oWorker.pWriter,
                false );

            if( ERROR( ret ) )
                break;

            pTaskGrp->AppendTask( pStartWriter );

            TaskletPtr pFirstEvent;
            if( IsLoopStarted() )
            {
                BufPtr pBuf( true );
                pBuf->Resize( sizeof( guint8 )
                    + sizeof( HANDLE ) );

                pBuf->ptr()[ 0 ] = stmevtLifted;
                memcpy( pBuf->ptr() + 1, &hChannel,
                    sizeof( hChannel ) );

                ret = DEFER_IFCALLEX_NOSCHED(
                    pFirstEvent,
                    ObjPtr( this ),
                    &CStreamSyncBase::PostLoopEventWrapper,
                    *pBuf );

                if( ERROR( ret ) )
                    break;

                pTaskGrp->AppendTask( pFirstEvent );

                TaskletPtr pEnableRead;
                bool bPause = false;
                ret = DEFER_IFCALLEX_NOSCHED(
                    pEnableRead,
                    ObjPtr( this ),
                    &CStreamSyncBase::PauseReadNotify,
                    hChannel, bPause );

                if( ERROR( ret ) )
                    break;

                pTaskGrp->AppendTask( pEnableRead );
            }
            else
            {

                TaskletPtr pReadyNotify;
                ret = DEFER_IFCALLEX_NOSCHED( 
                    pReadyNotify,
                    ObjPtr( this ),
                    &CStreamSyncBase::OnStreamReady,
                    hChannel );
                if( ERROR( ret ) )
                    break;
                pTaskGrp->AppendTask( pReadyNotify );
            }

            TaskletPtr pCleanup;
            gint32 ret = DEFER_CANCEL_HANDLER2(
                -1, pCleanup, this,
                &CStreamSyncBase::OnStmStartupComplete,
                ( IEventSink* )pTaskGrp, 0, hChannel );

            if( SUCCEEDED( ret ) )
            {
                CIfAsyncCancelHandler* pasc = pCleanup;
                pasc->SetSelfCleanup();
            }

            pTaskGrp->AppendTask( pStartWriter );
            // prevent the OnRecvData_Loop to be
            // the first event
            TaskletPtr pSeqTasks = pTaskGrp;
            ret = pMgr->RescheduleTask( pSeqTasks );

        }while( 0 );

        if( ERROR( ret ) )
        {
            if( !pTaskGrp.IsEmpty() )
                ( *pTaskGrp )( eventCancelTask );

            CStdRMutex oIfLock( this->GetLock() );
            if( hChannel != INVALID_HANDLE )
                m_mapStmWorkers.erase( hChannel );
            oIfLock.Unlock();

            if( !oWorker.pReader.IsEmpty() )
            {
                ( *oWorker.pReader )(
                    eventCancelTask );
            }
            if( !oWorker.pWriter.IsEmpty() )
            {
                ( *oWorker.pWriter )(
                    eventCancelTask );
            }
        }

        return ret;
    }

    gint32 GetWorker( HANDLE hChannel, bool bReader,
        TaskletPtr& pTask )
    {
        CStdRMutex oIfLock( this->GetLock() );

        typename WORKER_MAP::iterator itr = 
            m_mapStmWorkers.find( hChannel );

        if( itr == m_mapStmWorkers.end() )
            return -ENOENT;

        if( bReader )
            pTask = itr->second.pReader;
        else
            pTask = itr->second.pWriter;

        oIfLock.Unlock();

        return 0;
    }

    gint32 SetDiscard( HANDLE hChannel,
        bool bDiscard )
    {
        TaskletPtr pTask;
        gint32 ret = GetWorker(
            hChannel, true, pTask );
        if( ERROR( ret ) )
            return ret;

        CIfStmReadWriteTask* pReader = pTask;
        if( pReader == nullptr )
            return -EFAULT;

        ret = pReader->SetDiscard( bDiscard );

        return ret;
    }

    gint32 OnStmRecv( HANDLE hChannel,
        BufPtr& pBuf )
    {
        TaskletPtr pTask;
        if( hChannel == INVALID_HANDLE )
            return -EINVAL;

        gint32 ret = GetWorker(
            hChannel, true, pTask );
        if( ERROR( ret ) )
            return ret;

        CIfStmReadWriteTask* pReader = pTask;
        if( pReader == nullptr )
            return -EFAULT;

        // NOTE: the pBuf could contain a progress
        // report to send if the content is an
        // objptr rather than a byte array
        bool bEmitEvt = false;
        ret = pReader->OnStmRecv( pBuf );
        if( ret == STATUS_MORE_PROCESS_NEEDED )
            bEmitEvt = true;

        bool bPause = false;
        ret = IsReadNotifyPaused(
            hChannel, bPause );

        // It is possible the WORKER_ELEM is not
        // ready yet
        if( ret == -ENOENT )
            return ret;

        // if bPause == true, the client does
        // not want to receive the stmevtRecvData
        // event currently.
        if( IsLoopStarted() &&
            !bPause && bEmitEvt )
        {
            POST_RECV_DATA_EVT( hChannel, 0 );
        }

        return ret;
    }


    gint32 OnPreStop(
        IEventSink* pCallback )
    {
        do{
            std::vector< HANDLE > vecHandles;
            CStdRMutex oIfLock( this->GetLock() );
            for( auto elem : m_mapStmWorkers )
                vecHandles.push_back( elem.first );
            oIfLock.Unlock();

            for( auto elem : vecHandles )
                OnStmClosing( elem );

            StopLoop();

        }while( 0 );

        return super::OnPreStop( pCallback );
    }

    gint32 GetContext( HANDLE hChannel,
        CfgPtr& pCtx )
    {
        // a place to store channel specific data.
        if( hChannel == INVALID_HANDLE )
            return -EINVAL;

        CStdRMutex oIfLock( this->GetLock() );
        typename WORKER_MAP::iterator itr = 
            m_mapStmWorkers.find( hChannel );

        if( itr == m_mapStmWorkers.end() )
            return -ENOENT;

        pCtx = itr->second.pContext;
        return 0;
    }

    gint32 IsReadNotifyPaused(
        HANDLE hChannel, bool& bPaused )
    {
        if( hChannel == INVALID_HANDLE )
            return -EINVAL;

        TaskletPtr pTask;
        CStdRMutex oIfLock( this->GetLock() );

        typename WORKER_MAP::iterator itr = 
            m_mapStmWorkers.find( hChannel );

        if( itr == m_mapStmWorkers.end() )
            return -ENOENT;

        bPaused = itr->second.bPauseNotify; 
        oIfLock.Unlock();

        return 0;
    }

    /**
    * @name PauseReadNotify @{ to prevent the
    * OnRecvData_Loop to happen if bPause is true.
    * It does NOT stop receiving and queuing the
    * incoming stream data, which is important to
    * keep the ping/pong and report to coming in,
    * as keep the channel alive. But the queue
    * size is limited. therefore, if the queue is
    * full, the stream channel will finally be
    * blocked and disconnection will happen if
    * PauseReadNotify cannot be released very
    * soon. If you don't wont' to receiving the
    * data, use SetDiscard instead.
    * */
    /**
     * Parameter(s):
     *  hChannel: the stream channel to deny or
     *  allowing the incoming stream data.
     *
     *  bPause: true to mute the OnRecvData_Loop
     *  and false to allow OnRecvData_Loop to
     *  happen.
     * @} */
    
    gint32 PauseReadNotify( HANDLE hChannel,
        const bool& bPause )
    {
        if( hChannel == INVALID_HANDLE )
            return -EINVAL;

        TaskletPtr pTask;
        CStdRMutex oIfLock( this->GetLock() );

        typename WORKER_MAP::iterator itr = 
            m_mapStmWorkers.find( hChannel );

        if( itr == m_mapStmWorkers.end() )
            return -ENOENT;

        itr->second.bPauseNotify = bPause;
        pTask = itr->second.pReader;
        oIfLock.Unlock();

        if( bPause )
            return 0;

        // post a event to the loop
        CIfStmReadWriteTask* pReader = pTask;
        CStdRTMutex oTaskLock(
            pReader->GetLock() );

        guint32 dwCount = 
            pReader->GetMsgCount();

        if( dwCount > 0 )
           POST_RECV_DATA_EVT( hChannel, 0 );

        return 0;
    }

    protected:

    gint32 ReadWriteInternal(
        HANDLE hChannel, BufPtr& pBuf,
        IEventSink* pCallback,
        bool bRead, bool bNoWait )
    {
        gint32 ret = 0;
        do{
            TaskletPtr pTask;

            if( !this->IsConnected( nullptr ) )
                return ERROR_STATE;

            ret = GetWorker(
                hChannel, bRead, pTask );
            if( ERROR( ret ) )
                return ret;


            CIfStmReadWriteTask* pRwTask = pTask;
            if( pRwTask == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            if( bRead && bNoWait )
            {
                ret = pRwTask->
                    ReadStreamNoWait( pBuf );
            }
            else if( bRead && !bNoWait )
            {
                if( pCallback == nullptr )
                    ret = pRwTask->ReadStream( pBuf );
                else
                    ret = pRwTask->ReadStreamAsync(
                        pCallback, pBuf );
            }
            else if( !bRead && bNoWait )
            {
                ret = pRwTask->
                    WriteStreamNoWait( pBuf );
            }
            else
            {
                if( pCallback == nullptr )
                    ret = pRwTask->WriteStream( pBuf );
                else
                    ret = pRwTask->WriteStreamAsync(
                        pCallback, pBuf );

            }

        }while( 0 );

        return ret;
    }

    public:
    // ----methods as the streaming interface ----
    /**
    * @name ReadStream
    * @{ Read a message from the stream. It is any
    * thing the peer send via a WriteMsg/Write
    * call. it will wait till the message arrives
    * or error occurs on the underlying stream. It
    * should be faster than ReadBlock, since there
    * is zero copy along the path from the `read'
    * call to the caller.
    * */
    /** 
     * Parameter:
     * pBuf: an empty buffer for receiving the
     * incoming data.
     *
     * dwTimeoutSec: time to wait before return.
     * if dwTimeoutSec == -1, ReadStream will return
     * immediately. If there are pending msgs in
     * the receiving queue, the first msg will be
     * returned via pBuf. And if there is no data
     * pending, error code -EAGAIN is returned.
     *
     * return value:
     * STATUS_SUCCESS: the pBuf contains the
     * payload from the peer side
     *
     * @} */
    gint32 ReadStream(
        HANDLE hChannel, BufPtr& pBuf )
    {
        return ReadWriteInternal( hChannel,
            pBuf, nullptr, true, false );
    }

    /**
    * @name ReadBlock
    * @{ Read a required size of data block from
    * the stream. It is any thing the peer send
    * via a WriteMsg/Write call. it will wait till
    * the specified size of bytes arrives or error
    * occurs on the underlying stream. 
    * */
    /** 
     * Parameter:
     * hChannel: handle to the stream channel to
     * read
     *
     * pBuf: a buffer for receiving the incoming
     * data. The pBuf must not be empty. And the
     * method will try to fill the buffer at the
     * best effort. If the time is up before the
     * requested amount of data can be received,
     * the request will be canceled.
     *
     * return value:
     * STATUS_SUCCESS: the pBuf contains the
     * payload from the peer side
     *
     * @} */

    gint32 ReadStreamNoWait(
        HANDLE hChannel, BufPtr& pBuf )
    {
        return ReadWriteInternal( hChannel,
            pBuf, nullptr, true, true );
    }

    gint32 ReadWriteAsync( HANDLE hChannel,
        BufPtr& pBuf, IConfigDb* pCtx, bool bRead )
    {
        CParamList oReqCtx;
        oReqCtx.Push( hChannel );
        IConfigDb* pReqCtx = oReqCtx.GetCfg();

        TaskletPtr pCallback;
        gint32 ret = NEW_PROXY_RESP_HANDLER2(
            pCallback, ObjPtr( this ),
            &CStreamSyncBase::OnRWComplete,
            nullptr, pReqCtx );

        if( ERROR( ret ) )
            return ret;

        ObjPtr pObj( pCtx, true );
        oReqCtx.Push( pObj );

        // whether a read/write request
        oReqCtx.Push( bRead );
        if( !bRead )
            oReqCtx.Push( pBuf );

        return ReadWriteInternal( hChannel,
            pBuf, pCallback, bRead, false );
    }

    /**
    * @name ReadStreamAsync
    * @{ Read a block of data from the stream. It
    * is any thing from peer end. It will return
    * with STATUS_PENDING if there is no requested
    * size of data ready for reading. When the
    * request size of buffer is filled, the event
    * OnReadStreamComplete will be called.  The
    * callback will also  report error if the
    * write failed not. 
    * */
    /** 
     * Parameter:
     *
     * hChannel: handle to the stream channel to
     * read
     *
     * pBuf: a buffer for receiving the incoming
     * data. If the pBuf is empty, the first
     * incoming data block is returned. And if
     * pBuf is not empty, the method will try to
     * fill the buffer at the best effort. If the
     * time is up before the requested amount of
     * data can be received, the request will be
     * canceled.
     *
     * pCtx: context which will be passed to the
     * callback method OnReadStreamComplete.
     *
     * return value:
     * STATUS_SUCCESS: the pBuf contains the
     * payload from the peer side
     *
     * @} */

    gint32 ReadStreamAsync( HANDLE hChannel,
        BufPtr& pBuf, IConfigDb* pCtx )
    {
        return ReadWriteAsync(
            hChannel, pBuf, pCtx, true );
    }

    /**
    * @name ReadStreamAsync
    * @{ Read a block of data from the stream. It
    * is any thing from peer end. It will return
    * with STATUS_PENDING if there is no requested
    * size of data ready for reading. When the
    * request size of buffer is filled, the
    * callback pCb will be called. The callback
    * will also be called to report error if the
    * read failed finally. 
    * */
    /** 
     * Parameter:
     *
     * hChannel: handle to the stream channel to
     * read
     *
     * pBuf: a buffer for receiving the incoming
     * data. The pBuf must not be empty. And the
     * method will try to fill the buffer at the
     * best effort. If the time is up before the
     * requested amount of data can be received,
     * the request will be canceled.
     *
     * pCb: a pointer to the callback when the
     * read request is done. The pCb must conform
     * to the behavor as CIfResponseHandler does
     * in OnTaskComplete.
     *
     *
     * return value:
     * STATUS_SUCCESS: the pBuf contains the
     * payload from the peer side. It can be
     * returned immediately or by callback pCb.
     *
     * STATUS_PENDING: the request is accepted,
     * but not complete yet. Wait till the
     * callback is called.
     *
     * ETIMEDOUT : the read fails due to time's
     * up.
     *
     * @} */

    gint32 ReadStreamAsync( HANDLE hChannel,
        BufPtr& pBuf, IEventSink* pCb )
    {
        if( pCb == nullptr )
            return -EINVAL;

        return ReadWriteInternal( hChannel,
            pBuf, pCb, true, false );
    }
    /**
    * @name WriteStream
    * @{ Write a message to the stream
    * synchronously. It can be any thing to peer
    * end. The method will wait till the message
    * is sent successfully or error occurs.
    * */
    /** 
     * Parameter:
     * hChannel: handle to the stream channel to
     * write.
     *
     * pBuf: a buffer for the outgoing data. an
     * empty buf will cause -EINVAL to return.
     * The buffer size can be up to the CBuffer's
     * upper limit, MAX_BYTES_PER_BUFFER.
     *
     * return vale:
     *
     * STATUS_SUCCESS: the content in the buffer
     * is sent successfully.
     *
     * ERROR_QUEUE_FULL: the request is not
     * accepted, due to the send queue full.
     * Resend it later when OnWriteResumed occurs.
     *
     * ETIMEDOUT: the request fails to complete
     * before time is up.
     *
     * @} */
    gint32 WriteStream(
        HANDLE hChannel, BufPtr& pBuf )
    {
        return ReadWriteInternal( hChannel,
            pBuf, nullptr, false, false );
    }

    /**
    * @name WriteStreamNoWait
    * @{ Write a block of data to the stream. It
    * can be any thing to peer end. And the method
    * will return immediately without waiting till
    * the message is sent. It will always return
    * STATUS_PENDING. While you can still
    * repeatedly call this method till
    * ERROR_QUEUE_FULL is returned, which means
    * the request is refused due to the send queue
    * is full. And it also indicate to resend the
    * request after the next OnWriteResumed event
    * happens.
    * */
    /** 
    * Parameter:
     *
     * hChannel: handle to the stream channel to
     * write.
     *
     * pBuf: a buffer for the outgoing data. an
     * empty buf will cause an error of -EINVAL.
     * The buffer size can be up to the CBuffer's
     * upper limit, MAX_BYTES_PER_BUFFER.
     *
     * return vale:
     *
     * STATUS_PENDING: the request is accepted,
     * but not complete yet. Wait till the
     * callback is called.
     *
     * ERROR_QUEUE_FULL: the request is not
     * accepted, due to the send queue full.
     * Resend it later when OnWriteResumed occurs.
     *
     * @} */
    gint32 WriteStreamNoWait(
        HANDLE hChannel, BufPtr& pBuf )
    {
        return ReadWriteInternal( hChannel,
            pBuf, nullptr, false, true );
    }

    /**
    * @name WriteStreamAsync
    * @{ Write a block of data to the stream. It
    * is any thing to peer end. It will return
    * with STATUS_PENDING if the message is queued
    * sucessfully. When the buffer is sent, the
    * event OnWriteStreamComplete is called. It
    * will report if the write succeeds or not.
    * If the flowcontrol happens, ERROR_QUEUE_FULL
    * will return and the request has to be resend
    * after OnWriteResumed event happens
    * */
    /** 
     * Parameter:
     * hChannel: handle to the stream channel to
     * write.
     *
     * pBuf: a buffer for the outgoing data. an
     * empty buf will cause an error of -EINVAL.
     * The buffer size can be up to the CBuffer's
     * upper limit, MAX_BYTES_PER_BUFFER.
     *
     * pCtx: a pointer to user supplied context
     * which will be passed to the callback method
     * OnWriteStreamComplete.
     *
     * return vale:
     *
     * STATUS_SUCCESS: the content in the buffer
     * is sent successfully.
     *
     * STATUS_PENDING: the request is accepted,
     * but not complete yet. Wait till the
     * callback is called.
     *
     * ERROR_QUEUE_FULL: the request is not
     * accepted, due to the send queue full.
     * Resend it later when OnWriteResumed occurs.
     *
     * ETIMEDOUT: the request fails to complete
     * before time is up. This error should only
     * be returned by the callback
     * OnWriteStreamComplete, after this method
     * returns STATUS_PENDING.
     *
     * @} */
    gint32 WriteStreamAsync( HANDLE hChannel,
        BufPtr& pBuf, IConfigDb* pCtx )
    {
        return ReadWriteAsync(
            hChannel, pBuf, pCtx, false );
    }

    /**
    * @name WriteStreamAsync ( version 2 )
    * @{ Write a block of data to the stream. It
    * is any thing to peer end. It will return
    * with STATUS_PENDING if the message is queued
    * sucessfully. Whether or not the buffer is
    * sent, the callback pCb will be called later
    * to notify the status of the request.
    * */
    /** 
     * Parameter:
     * hChannel: handle to the stream channel to
     * write.
     *
     * pBuf: a buffer for the outgoing data. an
     * empty buf will cause an error of -EINVAL.
     * The buffer size can be up to the CBuffer's
     * upper limit, MAX_BYTES_PER_BUFFER.
     *
     * pCb: a pointer to the callback when the
     * write request is done. The pCb must conform
     * to the same behavor as CIfResponseHandler
     * does in OnTaskComplete.
     *
     * return vale:
     *
     * STATUS_SUCCESS: the content in the buffer
     * is sent successfully.
     *
     * STATUS_PENDING: the request is accepted,
     * but not complete yet. Wait till the
     * callback is called.
     *
     * ERROR_QUEUE_FULL: the request is not
     * accepted, due to the send queue full.
     * Resend it later when OnWriteResumed occurs.
     *
     * ETIMEDOUT: the request fails to complete
     * before time is up. This error should only
     * be returned by the callback
     * OnWriteStreamComplete, after this method
     * returns STATUS_PENDING;
     *
     * @} */
    gint32 WriteStreamAsync( HANDLE hChannel,
        BufPtr& pBuf, IEventSink* pCb )
    {
        if( pCb == nullptr )
            return -EINVAL;

        return ReadWriteInternal( hChannel,
            pBuf, pCb, false, false );
    }
    // event to notify the flow control is lifted,
    // and write request is allowed.
    virtual gint32 OnWriteResumed(
        HANDLE hChannel )
    { return 0; }

    // callback to notify the read request
    // identified as lwTid is done with status
    // code in iRet on channel hChannel.
    virtual gint32 OnReadStreamComplete(
        HANDLE hChannel,
        gint32 iRet,
        BufPtr& pBuf,
        IConfigDb* pCtx )
    { return 0; }

    // callback to notify the write request
    // identified as lwTid is done with status
    // code in iRet on channel hChannel.
    virtual gint32 OnWriteStreamComplete(
        HANDLE hChannel,
        gint32 iRet,
        BufPtr& pBuf,
        IConfigDb* pCtx ) 
    { return 0; }

    gint32 PeekStream(
        HANDLE hChannel, BufPtr& pBuf )
    {
        gint32 ret = 0;
        do{
            TaskletPtr pTask;

            if( !this->IsConnected( nullptr ) )
                return ERROR_STATE;

            ret = GetWorker(
                hChannel, true, pTask );
            if( ERROR( ret ) )
                return ret;


            CIfStmReadWriteTask* pRdTask = pTask;
            if( pRdTask == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            ret = pRdTask->PeekStream( pBuf );

        }while( 0 );

        return ret;
    }

    gint32 GetPendingBytes(
        HANDLE hChannel, guint32& dwBytes )
    {
        gint32 ret = 0;
        do{
            TaskletPtr pTask;

            if( !this->IsConnected( nullptr ) )
                return ERROR_STATE;

            ret = GetWorker(
                hChannel, true, pTask );
            if( ERROR( ret ) )
                return ret;


            CIfStmReadWriteTask* pRdTask = pTask;
            if( pRdTask == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            ret = pRdTask->GetPendingBytes(
                dwBytes );

        }while( 0 );

        return ret;
    }

    gint32 GetPendingReqs(
        HANDLE hChannel, guint32& dwCount )
    {
        gint32 ret = 0;
        do{
            TaskletPtr pTask;

            if( !this->IsConnected( nullptr ) )
                return ERROR_STATE;

            ret = GetWorker(
                hChannel, true, pTask );
            if( ERROR( ret ) )
                return ret;


            CIfStmReadWriteTask* pRdTask = pTask;
            if( pRdTask == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            ret = pRdTask->GetPendingReqs(
                dwCount );

        }while( 0 );

        return ret;
    }

    // helper callback
    protected:
    gint32 OnRWComplete(
        IEventSink* pCallback,
        IEventSink* pIoReq,
        IConfigDb* pReqCtx )
    {
        if( pIoReq == nullptr ||
            pReqCtx == nullptr )
            return -EINVAL;

        gint32 ret = 0; 

        do{
            CCfgOpenerObj oReq( pIoReq );
            IConfigDb* pResp = nullptr;
            ret = oReq.GetPointer(
                propRespPtr, pResp );
            if( ERROR( ret ) )
                break;

            CCfgOpener oResp( pResp );
            guint32 iRet = 0;
            ret = oResp.GetIntProp(
                propReturnValue, iRet );

            if( ERROR( ret ) )
                break;

            
            CParamList oReqCtx( pReqCtx );
            HANDLE hChannel = INVALID_HANDLE;
            guint32* pTemp = nullptr;
            ret = oReqCtx.GetIntPtr( 0, pTemp );
            if( ERROR( ret ) )
                break;
            hChannel = ( HANDLE )pTemp;
            IConfigDb* pCtx = nullptr;
            ret = oReqCtx.GetPointer(
                1, pCtx );

            if( ERROR( ret ) )
                pCtx = nullptr;

            if( pCtx != nullptr && pCtx->size() == 0 )
                pCtx = nullptr;

            bool bRead = false;
            ret = oReqCtx.GetBoolProp(
                2, bRead );
            if( ERROR( ret ) )
                break;

            BufPtr pBuf;
            if( SUCCEEDED( iRet ) && bRead ) 
                oResp.GetProperty( 0, pBuf );
            else if( !bRead )
            {
                oReqCtx.GetProperty( 3, pBuf );
            }

            if( bRead )
                OnReadStreamComplete( hChannel,
                    ( gint32& )iRet, pBuf, pCtx );
            else
                OnWriteStreamComplete( hChannel,
                    ( gint32& )iRet, pBuf, pCtx );

        }while( 0 );

        return ret;
    }
};

class CStreamProxySync :
    public CStreamSyncBase< CStreamProxy >
{
    public:
    typedef CStreamSyncBase< CStreamProxy > super;

    CStreamProxySync( const IConfigDb* pCfg ) :
       _MyVirtBase( pCfg ), super( pCfg )
    {;}

    gint32 StartStream( HANDLE& hChannel,
        IConfigDb* pDesc = nullptr,
        IEventSink* pCallback = nullptr );

    gint32 GetPeerIdHash( HANDLE hChannel,
        guint64& qwPeerIdHash );

    HANDLE GetChanByIdHash( guint64 ) const;
};

class CStreamServerSync :
    public CStreamSyncBase< CStreamServer >
{
    std::map< guint64, HANDLE > m_mapIdHashToHandle;


    public:
    typedef CStreamSyncBase< CStreamServer > super;
    CStreamServerSync( const IConfigDb* pCfg ) :
       _MyVirtBase( pCfg ), super( pCfg )
    {;}

    virtual gint32 OnConnected( HANDLE hCfg );
    virtual gint32 StopWorkers( HANDLE hChannel );
    HANDLE GetChanByIdHash( guint64 ) const;

    virtual gint32 AcceptNewStream(
        IEventSink* pCallback,
        IConfigDb* pDataDesc ) override;
};

class CStreamProxyWrapper :
    public virtual CAggInterfaceProxy
{
    public:
    typedef CAggInterfaceProxy super;
    CStreamProxyWrapper( const IConfigDb* pCfg )
        : super( pCfg )
    {;}

    inline gint32 ReadStream(
        HANDLE hChannel, BufPtr& pBuf )
    {
        CStreamProxySync* pStm = ObjPtr( this );
        if( pStm == nullptr )
            return -EFAULT;
        return pStm->ReadStream(
            hChannel, pBuf );
    }

    inline gint32 ReadStreamNoWait(
        HANDLE hChannel, BufPtr& pBuf )
    {
        CStreamProxySync* pStm = ObjPtr( this );
        if( pStm == nullptr )
            return -EFAULT;
        return pStm->ReadStreamNoWait(
            hChannel, pBuf );
    }

    inline gint32 ReadStreamAsync(
        HANDLE hChannel, BufPtr& pBuf,
        IEventSink* pCb )
    {
        CStreamProxySync* pStm = ObjPtr( this );
        if( pStm == nullptr )
            return -EFAULT;
        return pStm->ReadStreamAsync(
            hChannel, pBuf, pCb );
    }

    inline gint32 WriteStream(
        HANDLE hChannel, BufPtr& pBuf )
    {
        CStreamProxySync* pStm = ObjPtr( this );
        if( pStm == nullptr )
            return -EFAULT;
        return pStm->WriteStream(
            hChannel, pBuf );
    }

    inline gint32 WriteStreamNoWait(
        HANDLE hChannel, BufPtr& pBuf )
    {
        CStreamProxySync* pStm = ObjPtr( this );
        if( pStm == nullptr )
            return -EFAULT;
        return pStm->WriteStreamNoWait(
            hChannel, pBuf );
    }

    inline gint32 WriteStreamAsync(
        HANDLE hChannel, BufPtr& pBuf,
        IEventSink* pCb )
    {
        CStreamProxySync* pStm = ObjPtr( this );
        if( pStm == nullptr )
            return -EFAULT;
        return pStm->WriteStreamAsync(
            hChannel, pBuf, pCb );
    }
};

class CStreamServerWrapper :
    public virtual CAggInterfaceServer
{
    public:
    typedef CAggInterfaceServer super;
    CStreamServerWrapper( const IConfigDb* pCfg )
        : super( pCfg )
    {;}

    inline gint32 ReadStream(
        HANDLE hChannel, BufPtr& pBuf )
    {
        CStreamServerSync* pStm = ObjPtr( this );
        if( pStm == nullptr )
            return -EFAULT;
        return pStm->ReadStream(
            hChannel, pBuf );
    }

    inline gint32 ReadStreamNoWait(
        HANDLE hChannel, BufPtr& pBuf )
    {
        CStreamServerSync* pStm = ObjPtr( this );
        if( pStm == nullptr )
            return -EFAULT;
        return pStm->ReadStreamNoWait(
            hChannel, pBuf );
    }

    inline gint32 ReadStreamAsync(
        HANDLE hChannel, BufPtr& pBuf,
        IEventSink* pCb )
    {
        CStreamServerSync* pStm = ObjPtr( this );
        if( pStm == nullptr )
            return -EFAULT;
        return pStm->ReadStreamAsync(
            hChannel, pBuf, pCb );
    }

    inline gint32 WriteStream(
        HANDLE hChannel, BufPtr& pBuf )
    {
        CStreamServerSync* pStm = ObjPtr( this );
        if( pStm == nullptr )
            return -EFAULT;
        return pStm->WriteStream(
            hChannel, pBuf );
    }

    inline gint32 WriteStreamNoWait(
        HANDLE hChannel, BufPtr& pBuf )
    {
        CStreamServerSync* pStm = ObjPtr( this );
        if( pStm == nullptr )
            return -EFAULT;
        return pStm->WriteStreamNoWait(
            hChannel, pBuf );
    }

    inline gint32 WriteStreamAsync(
        HANDLE hChannel, BufPtr& pBuf,
        IEventSink* pCb )
    {
        CStreamServerSync* pStm = ObjPtr( this );
        if( pStm == nullptr )
            return -EFAULT;
        return pStm->WriteStreamAsync(
            hChannel, pBuf, pCb );
    }
};

class CStreamProxyAsync :
    public CStreamProxySync
{
    public:
    typedef CStreamProxySync super;
    CStreamProxyAsync( const IConfigDb* pCfg ) :
        _MyVirtBase( pCfg ), super( pCfg )
    {}

    gint32 OnRecvData_Loop(
        HANDLE hChannel, gint32 iRet )
    { return -ENOTSUP; }

    gint32 OnSendDone_Loop(
        HANDLE hChannel, gint32 iRet )
    { return -ENOTSUP; }

    gint32 OnWriteEnabled_Loop(
        HANDLE hChannel )
    { return -ENOTSUP; }

    gint32 OnCloseChannel_Loop(
        HANDLE hChannel )
    { return -ENOTSUP; }

    gint32 OnStart_Loop()
    { return -ENOTSUP; }
};

class CStreamServerAsync :
    public CStreamServerSync
{
    public:
    typedef CStreamServerSync super;
    CStreamServerAsync( const IConfigDb* pCfg ) :
        _MyVirtBase( pCfg ), super( pCfg )
    {}

    gint32 OnRecvData_Loop(
        HANDLE hChannel, gint32 iRet )
    { return -ENOTSUP; }

    gint32 OnSendDone_Loop(
        HANDLE hChannel, gint32 iRet )
    { return -ENOTSUP; }

    gint32 OnWriteEnabled_Loop(
        HANDLE hChannel )
    { return -ENOTSUP; }

    gint32 OnCloseChannel_Loop(
        HANDLE hChannel )
    { return -ENOTSUP; }

    gint32 OnStart_Loop()
    { return -ENOTSUP; }
};

struct HSTREAM final : public HSTREAM_BASE
{
    HANDLE m_hStream = INVALID_HANDLE;
    guint64 m_qwStmHash = 0;
    CRpcServices* m_pIf = nullptr;
    gint32 Serialize( BufPtr& pBuf ) const;
    gint32 Deserialize( BufPtr& );
};

}
