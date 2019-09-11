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

#define CTRLCODE_READMSG    0x1000
#define CTRLCODE_READBLK    0x1001

#define CTRLCODE_WRITEMSG   0x1002
#define CTRLCODE_WRITEBLK   0x1003

#define SYNC_STM_MAX_PENDING_PKTS ( STM_MAX_PACKATS_REPORT + 2 )

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

    BufPtr  m_pSendBuf;
    bool    m_bIn;
    bool    m_bDiscard = false;
    HANDLE  m_hChannel = INVALID_HANDLE;

    gint32 ReadStreamInternal(
        bool bMsg, BufPtr& pBuf,
        guint32 dwTimeoutSec );

    gint32 SetRdRespForIrp(
        PIRP pIrp, BufPtr& pBuf );

    gint32 CompleteReadIrp(
        IrpPtr& pIrp, BufPtr& pBuf );

    gint32 WriteStreamInternal(
        bool bMsg, BufPtr& pBuf,
        guint32 dwTimeoutSec );

    gint32 OnIoIrpComplete( PIRP pIrp );
    gint32 OnWorkerIrpComplete( PIRP pIrp );

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

    /**
    * @name ReadMsg
    * @{ Read a message from the stream. It is any
    * thing the peer send via a WriteMsg/Write
    * call. it will wait till the message arrives
    * or error occurs on the underlying stream. It
    * should be fastest, since there is no copy
    * work to the user specified places.
    * */
    /** 
     * Parameter:
     * pBuf: an empty buffer for receiving the
     * incoming data.
     *
     * dwTimeoutSec: time to wait before return.
     * if dwTimeoutSec == -1, ReadMsg will return
     * immediately. If there is pending incoming
     * msg, it will be returned via pBuf. If there
     * is no data pending, -EAGAIN is returned.
     *
     * return value:
     * STATUS_SUCCESS: the pBuf contains the
     * payload from the peer side
     *
     * @} */
    gint32 ReadMsg( BufPtr& pBuf,
        guint32 dwTimeoutSec )
    {
        return ReadStreamInternal( true,
            pBuf, dwTimeoutSec );
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
     * pBuf: a buffer for receiving the incoming
     * data with the desired size. If it is empty,
     * -EINVAL will returns
     *
     * return value:
     * STATUS_SUCCESS: the pBuf contains the
     * payload from the peer side
     *
     * @} */
    gint32 ReadBlock( BufPtr pBuf,
        guint32 dwTimeoutSec )
    {
        return ReadStreamInternal( false,
            pBuf, dwTimeoutSec );
    }

    /**
    * @name WriteMsg
    * @{ Write a message to the stream. It is any
    * thing to peer end. It will wait till the
    * message is out of the box or error occurs on
    * the underlying stream. It should be fastest,
    * since there is no copy work to the user
    * specified places. If the flow-control
    * happens, it will wait till flow-control is
    * lifted and the the pBuf is sent successfully
    * */
    /** 
     * Parameter:
     * pBuf: a buffer for the outgoing data
     *
     * return vale:
     * STATUS_SUCCESS: the content is sent
     * successfully
     *
     * @} */
    gint32 WriteMsg( BufPtr& pBuf,
        guint32 dwTimeoutSec )
    {
        return WriteStreamInternal( true,
            pBuf, dwTimeoutSec );
    }

    /**
    * @name WriteBlock
    * @{ Write a block of data to the stream. It is any
    * thing to peer end. It will wait till the all
    * the bytes of the block is out of the box or
    * error occurs on the underlying stream. If the
    * flow-control happens, it will wait till
    * flow-control is lifted and the the pBuf is
    * sent successfully. It will chop the block of
    * data to smaller blocks if it is too big to
    * transfer in one shot. the upper limit for a
    * one-shot transfer is STM_MAX_BYTES_PER_BUF(
    * 32KB so far ).
    * 
    * */
    /** 
     * Parameter:
     * pBuf: a buffer for the outgoing data. an
     * empty buf will cause an error of -EINVAL.
     *
     * return vale:
     * STATUS_SUCCESS: the content is sent
     * successfully
     *
     * @} */
    gint32 WriteBlock( BufPtr& pBuf,
        guint32 dwTimeoutSec )
    {
        return WriteStreamInternal( true,
            pBuf, dwTimeoutSec );
    }

    gint32 OnFCLifted();
    gint32 PauseReading( bool bPause );
    gint32 OnStmRecv( BufPtr& pBuf );
    gint32 PostLoopEvent( BufPtr& pBuf );
    bool IsLoopStarted();
    bool CanSend();
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

    gint32 StopLoop()
    {
        guint32 ret = 0;
        if( !IsLoopStarted() )
            return 0;

        CMainIoLoop* pLoop = m_pRdLoop;
        if( pLoop == nullptr )
            return -EFAULT;

        pLoop->WakeupLoop( aevtStop );
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
     * you can call ReadMsg/ReadBlock with this
     * parameter to get the buffer containing the
     * incoming message or data block.
     *
     * Return value:
     * 
     * @} */

    virtual gint32 OnRecvData_Loop(
        HANDLE hChannel ) = 0;
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
        if( dwParam1 != POLL_IN )
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
                    OnRecvData_Loop( hChannel );
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
            guint32 dwParam1 = 0,
            guint32 dwParam2 = 0,
            guint32* pData = NULL  )
    {
        if( iEvent == eventIoWatch )
        {
            return DispatchReadWriteEvent(
                dwParam1 );
        }

        return super::OnEvent( iEvent,
            dwParam1, dwParam2, pData );
    }

    gint32 StartLoop()
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

            ret = m_pRdLoop->Start();

            RemoveIoWatch();

            if( ERROR( ret ) )
                break;
            
            
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
        return 0;
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
            CStdRTMutex oTaskLock(
                pWriter->GetLock() );

            ret = pWriter->OnFCLifted();
            if( ERROR( ret ) )
                break;

        }while( 0 );

        return ret;
    }

    gint32 StopWorkers( HANDLE hChannel )
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

    gint32 OnConnected( HANDLE hChannel )
    {
        WORKER_ELEM oWorker;
        gint32 ret = 0;
        DebugPrint( 0, "OnConnected..." );

        TaskGrpPtr pTaskGrp;
        do{
            CStdRMutex oIfLock( this->GetLock() );
            if( !this->IsConnected( nullptr ) )
            {
                ret = ERROR_STATE;
                break;
            }

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

            ret = oWorker.pReader.NewObj(
                clsid( CIfStmReadWriteTask ),
                oParams.GetCfg() );

            if( ERROR( ret ) )
                break;

            ret = oWorker.pContext.NewObj();
            if( ERROR( ret ) )
                break;

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
     
            pTaskGrp->SetRelation( logicNONE );
            TaskletPtr pStartReader;
            ret = DEFER_IFCALL_NOSCHED(
                pStartReader,
                ObjPtr( this ),
                &CStreamSyncBase::RunManagedTaskWrapper,
                ( IEventSink* )oWorker.pReader,
                false );

            if( ERROR( ret ) )
                break;

            pTaskGrp->AppendTask( pStartReader );

            TaskletPtr pStartWriter;
            ret = DEFER_IFCALL_NOSCHED( 
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

                ret = DEFER_IFCALL_NOSCHED(
                    pFirstEvent,
                    ObjPtr( this ),
                    &CStreamSyncBase::PostLoopEventWrapper,
                    *pBuf );

                if( ERROR( ret ) )
                    break;

                pTaskGrp->AppendTask( pFirstEvent );

                TaskletPtr pEnableRead;
                bool bPause = false;
                ret = DEFER_IFCALL_NOSCHED(
                    pEnableRead,
                    ObjPtr( this ),
                    &CStreamSyncBase::PauseReadNotify,
                    hChannel, bPause );

                if( ERROR( ret ) )
                    break;

                pTaskGrp->AppendTask( pEnableRead );
            }

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
        gint32 ret = GetWorker(
            hChannel, true, pTask );
        if( ERROR( ret ) )
            return ret;

        CIfStmReadWriteTask* pReader = pTask;
        if( pReader == nullptr )
            return -EFAULT;

        ret = pReader->OnStmRecv( pBuf );
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
        if( IsLoopStarted() && !bPause )
        {
            BufPtr pEvtBuf( true );
            pEvtBuf->Resize( sizeof( guint8 ) +
                sizeof( hChannel ) );
            pEvtBuf->ptr()[ 0 ] = stmevtRecvData;
            memcpy( pEvtBuf->ptr() + 1,
                &hChannel, sizeof( hChannel ) );
            PostLoopEvent( pEvtBuf );
        }

        return ret;
    }

    gint32 ReadWriteInternal( bool bRead,
        bool bMsg, HANDLE hChannel,
        BufPtr& pBuf, guint32 dwTimeoutSec )
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
            if( bRead && bMsg )
            {
                ret = pRwTask->ReadMsg(
                    pBuf, dwTimeoutSec );
            }
            else if( bRead && !bMsg )
            {
                ret = pRwTask->ReadBlock(
                    pBuf, dwTimeoutSec );
            }
            else if( !bRead && bMsg )
            {
                ret = pRwTask->WriteMsg(
                    pBuf, dwTimeoutSec );
            }
            else
            {
                ret = pRwTask->WriteBlock(
                    pBuf, dwTimeoutSec );
            }

        }while( 0 );

        return ret;
    }

    gint32 ReadMsg( HANDLE hChannel,
        BufPtr& pBuf, guint32 dwTimeoutSec = 0 )
    {
        return ReadWriteInternal( true, true,
            hChannel, pBuf, dwTimeoutSec );
    }

    gint32 ReadBlock( HANDLE hChannel,
        BufPtr& pBuf, guint32 dwTimeoutSec = 0 )
    {
        return ReadWriteInternal( true, false,
            hChannel, pBuf, dwTimeoutSec );
    }

    gint32 WriteMsg( HANDLE hChannel,
        BufPtr& pBuf, guint32 dwTimeoutSec = 0 )
    {
        return ReadWriteInternal( false, true,
            hChannel, pBuf, dwTimeoutSec );
    }

    gint32 WriteBlock( HANDLE hChannel,
        BufPtr& pBuf, guint32 dwTimeoutSec = 0 )
    {
        return ReadWriteInternal( false, false,
            hChannel, pBuf, dwTimeoutSec );
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

        CIfStmReadWriteTask* pReader = pTask;

        if( bPause )
            return 0;

        // post a event to the loop
        BufPtr pBuf( true );
        pBuf->Resize( sizeof( guint8 ) +
            sizeof( HANDLE ) );

        pBuf->ptr()[ 0 ] = stmevtRecvData;
        memcpy( pBuf->ptr() + 1, &hChannel,
            sizeof( hChannel ) );

        CStdRTMutex oTaskLock(
            pReader->GetLock() );

        guint32 dwCount = 
            pReader->GetMsgCount();

        if( dwCount > 0 )
            return PostLoopEvent( pBuf );

        return 0;
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
        IConfigDb* pDesc = nullptr )
    {
        gint32 ret = 0;
        hChannel = INVALID_HANDLE;
        do{
            if( !IsConnected() )
            {
                ret = ERROR_STATE;
                break;
            }
            ret = super::StartStream(
                hChannel, pDesc );

            if( ERROR( ret ) )
                break;

        }while( 0 );

        return ret;
    }
};

class CStreamServerSync :
    public CStreamSyncBase< CStreamServer >
{
    public:

    typedef CStreamSyncBase< CStreamServer > super;
    CStreamServerSync( const IConfigDb* pCfg ) :
       _MyVirtBase( pCfg ), super( pCfg )
    {;}
};
