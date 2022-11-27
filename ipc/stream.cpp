/*
 * =====================================================================================
 *
 *       Filename:  stream.cpp
 *
 *    Description:  implementation of stream related classes
 *
 *        Version:  1.0
 *        Created:  11/22/2018 11:21:25 AM
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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "frmwrk.h"
#include "stream.h"
#include "uxstream.h"
#include "sha1.h"

namespace rpcf
{

bool IStream::CanSend( HANDLE hChannel )
{
    InterfPtr pIf;
    gint32 ret = GetUxStream( hChannel, pIf );
    if( ERROR( ret ) )
        return false;

    if( pIf->IsServer() )
    {
        CUnixSockStmServer* pSvr = ObjPtr( pIf );
        if( pSvr == nullptr )
            return false;

        return pSvr->CanSend();
    }

    CUnixSockStmProxy* pProxy = ObjPtr( pIf );
    if( pProxy == nullptr )
        return false;

    return pProxy->CanSend();
}

gint32 IStream::CreateUxStream(
    IConfigDb* pDataDesc,
    gint32 iFd, EnumClsid iClsid,
    bool bServer,
    InterfPtr& pIf )
{
    if( pDataDesc == nullptr ||
        iClsid == clsid( Invalid ) ||
        iFd < 0 )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CObjBase* pParent = GetInterface();

        CParamList oNewCfg;

        oNewCfg.SetBoolProp(
            propIsServer, bServer );

        oNewCfg.SetIntProp(
            propFd, ( guint32& )iFd );

        oNewCfg.SetPointer(
            propParentPtr, pParent ); 

        guint32 dwTimeoutSec = 0;
        CCfgOpener oDataDesc( pDataDesc );
        ret = oDataDesc.GetIntProp(
            propTimeoutSec, dwTimeoutSec );
        if( ERROR( ret ) )
            break;

        if( dwTimeoutSec > MAX_TIMEOUT_VALUE )
        {
            ret = oNewCfg.CopyProp(
                propTimeoutSec, pParent );
            if( ERROR( ret ) )
                break;
        }
        else
        {
            oNewCfg.SetIntProp(
                propTimeoutSec, dwTimeoutSec );
        }

        guint32 dwKeepAliveSec = 0;
        ret = oDataDesc.GetIntProp(
            propKeepAliveSec, dwKeepAliveSec );
        if( ERROR( ret ) )
            break;

        if( dwKeepAliveSec > MAX_TIMEOUT_VALUE )
        {
            ret = oNewCfg.CopyProp(
                propKeepAliveSec, pParent );
            if( ERROR( ret ) )
                break;
        }
        else
        {
            oNewCfg.SetIntProp(
            propKeepAliveSec, dwKeepAliveSec );
        }

        oNewCfg.Push( ObjPtr( pDataDesc ) );

        ret = pIf.NewObj( iClsid,
            oNewCfg.GetCfg() );

        if( ERROR( ret ) )
            break;

    }while( 0 );
    
    return ret;
}

gint32 IStream::OnUxStreamEvent(
    HANDLE hChannel,
    guint8 byToken,
    CBuffer* pBuf )
{
    if( hChannel == INVALID_HANDLE )
        return -EINVAL;

    gint32 ret = 0;
    switch( byToken )
    {
    case tokClose:
        {
            ret = OnClose( hChannel );
            break;
        }
    case tokError:
        {
            if( pBuf == nullptr || pBuf->empty() )
                break;

            guint32 dwError = ntohl( *pBuf );
            ret = OnChannelError(
                hChannel, ( gint32 )dwError );

            break;
        }
    case tokPing:
    case tokPong:
        {
            ret = OnPingPong( hChannel,
                byToken == tokPing );
            break;
        }
    case tokData:
        {
            BufPtr bufPtr( pBuf );
            ret = OnStmRecv( hChannel, bufPtr );
            break;
        }
    case tokFlowCtrl:
        {
            ret = OnFlowControl( hChannel );
            break;
        }
    case tokLift:
        {
            ret = OnFCLifted( hChannel );
            break;
        }
    default:
        break;
    }
    return ret;
}

gint32 IStream::OnUxStmEvtWrapper(
    HANDLE hChannel,
    IConfigDb* pCfg )
{
    InterfPtr pIf =
        dynamic_cast< CRpcServices* >( this );

    if( pIf.IsEmpty() )
        return -EFAULT;

    CParamList oParams( pCfg );
    return OnUxStreamEvent( hChannel,
        ( guint8& )oParams[ 0 ],
        *( BufPtr& )oParams[ 1 ] );
}

gint32 IStream::WriteStream(
    HANDLE hChannel, BufPtr& pBuf )
{
    if( hChannel == INVALID_HANDLE ||
        pBuf.IsEmpty() )
        return -EINVAL;
    
    gint32 ret = 0;
    do{
        CParamList oParams;
        oParams.SetPointer( propIoMgr, 
            GetInterface()->GetIoMgr() );

        TaskletPtr pTask;
        ret = pTask.NewObj(
            clsid( CSyncCallback ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        ret = WriteStream(
            hChannel, pBuf, pTask );

        if( ret == STATUS_PENDING )
        {
            CSyncCallback* pSyncTask = pTask;
            pSyncTask->WaitForComplete();
            ret = pSyncTask->GetError();
        }

    }while( 0 );

    return ret;
}

gint32 IStream::WriteStream(
    HANDLE hChannel,
    BufPtr& pBuf,
    IEventSink* pCallback )
{
    if( hChannel == INVALID_HANDLE ||
        pBuf.IsEmpty() )
        return -EINVAL;
    
    TaskletPtr pDummy;
    if( pCallback == nullptr )
    {
        pDummy.NewObj( clsid( CIfDummyTask ) );
        pCallback = pDummy;
    }

    gint32 ret = 0;

    do{
        InterfPtr pIf;
        ret = GetUxStream( hChannel, pIf );
        if( ERROR( ret ) )
            break;

        CRpcServices* pSvc = pIf;
        if( !pSvc->IsConnected() )
        {
            ret = ERROR_STATE;
            break;
        }

        if( pIf->IsServer() )
        {
            CUnixSockStmServer* pSvr = pIf;
            ret = pSvr->WriteStream(
                pBuf, pCallback );
        }
        else
        {
            CUnixSockStmProxy* pProxy = pIf;
            ret = pProxy->WriteStream(
                pBuf, pCallback );
        }

    }while( 0 );

    return ret;
}

gint32 IStream::GetDataDesc(
    HANDLE hChannel,
    CfgPtr& pDataDesc ) const
{
    if( hChannel == INVALID_HANDLE )
        return -EINVAL;

    gint32 ret = 0;
    do{
        InterfPtr pIf;
        ret = GetUxStream( hChannel, pIf );
        if( ERROR( ret ) )
            break;

        CCfgOpenerObj oCfg(
            ( const CObjBase* )pIf );

        ObjPtr pObj;
        ret = oCfg.GetObjPtr(
            propDataDesc, pObj );

        if( ERROR( ret ) )
            break;

        pDataDesc = pObj;
        if( pDataDesc.IsEmpty() )
            ret = -EFAULT;

    }while( 0 );

    return ret;
}

gint32 IStream::SendPingPong(
    HANDLE hChannel, bool bPing,
    IEventSink* pCallback )
{
    TaskletPtr pTask;
    if( hChannel == INVALID_HANDLE ||
        pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        InterfPtr pIf;
        ret = GetUxStream( hChannel, pIf );
        if( ERROR( ret ) )
            break;

        CRpcServices* pSvc = pIf;
        if( !pSvc->IsConnected() )
        {
            ret = ERROR_STATE;
            break;
        }

        if( pSvc->IsServer() )
        {
            CUnixSockStmServer* pSvr = pIf;
            ret = pSvr->SendPingPong(
                bPing ? tokPing : tokPong,
                pCallback );
        }
        else
        {
            CUnixSockStmProxy* pProxy = pIf;
            ret = pProxy->SendPingPong(
                bPing ? tokPing : tokPong,
                pCallback );
        }

    }while( 0 );

    return ret;
}

gint32 IStream::SendPingPong(
    HANDLE hChannel, bool bPing )
{
    if( hChannel == INVALID_HANDLE )
        return -EINVAL;
    
    gint32 ret = 0;
    do{
        CParamList oParams;
        oParams.SetPointer( propIoMgr,
            GetInterface()->GetIoMgr() );

        TaskletPtr pTask;
        ret = pTask.NewObj(
            clsid( CSyncCallback ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        ret = SendPingPong(
            hChannel, bPing, pTask );

        if( ret == STATUS_PENDING )
        {
            CSyncCallback* pSyncTask = pTask;
            pSyncTask->WaitForComplete();
            ret = pSyncTask->GetError();
        }

    }while( 0 );

    return ret;
}

gint32 CStreamProxy::OnChannelError(
    HANDLE hChannel, gint32 iError )
{
    if( hChannel == INVALID_HANDLE )
        return -EINVAL;

    return OnClose( hChannel  );
}

gint32 CStreamProxy::SendSetupReq(
    IConfigDb* pDataDesc,
    gint32& fd, IEventSink* pCallback )
{
    if( pDataDesc == nullptr ||
        pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        guint32 dwOffset = 0;
        guint32 dwSize = 0;
        ret = FetchData( pDataDesc,
            fd, dwOffset, dwSize, pCallback );

    }while( 0 );

    return ret;
}

gint32 CIfCreateUxSockStmTask::GetResponse()
{
    gint32 ret = 0;
    do{
        std::vector< LONGWORD > vecParams;
        ret = GetParamList( vecParams,
            propParamList );

        if( ERROR( ret ) )
            break;

        ObjPtr pObj(
            ( CObjBase* )vecParams[ 3 ] );
        if( pObj.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        CCfgOpenerObj oCfg(
            ( CObjBase* )pObj );

        IConfigDb* pResp = nullptr;
        ret = oCfg.GetPointer(
            propRespPtr, pResp );

        if( ERROR( ret ) )
            break;

        CParamList oResp( pResp );
        gint32 iRet = oResp[ propReturnValue ];
        if( ERROR( iRet ) )
        {
            ret = iRet;
            break;
        }
        guint32 dwVal = 0;
        ret = oResp.GetIntProp( 1, dwVal );
        if( ERROR( ret ) )
            break;

        CCfgOpener oThisCfg(
            ( IConfigDb* )GetConfig() );
        oThisCfg[ propFd ] = dwVal;

        IConfigDb* pDataDescSvr = nullptr;
        ret = oResp.GetPointer( 0, pDataDescSvr );
        if( ERROR( ret ) )
            break;
        
        IConfigDb* pDataDesc = nullptr;
        ret = oThisCfg.GetPointer( 0, pDataDesc );
        if( ERROR( ret ) )
            break;

        // the peer stream object's id
        CCfgOpener oDataDesc( pDataDesc );
        ret = oDataDesc.CopyProp(
            propPeerObjId, pDataDescSvr );

    }while( 0 );

    return ret;
}

gint32 CIfCreateUxSockStmTask::RunTask()
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );

        ret = oCfg.GetBoolProp(
            propIsServer, m_bServer );

        if( ERROR( ret ) )
            break;

        RemoveProperty( propIsServer );

        if( m_bServer )
        {
            OnTaskComplete( 0 );
        }
        else
        {
            // the proxy side needs to wait till the
            // server reply arrives
            ret = STATUS_PENDING;
        }

    }while( 0 );

    return ret;
}

gint32 CIfCreateUxSockStmTask::OnTaskComplete(
    gint32 iRet )
{
    gint32 ret = 0;

    do{
        if( ERROR( iRet ) )
        {
            OutputMsg( iRet,
                "CIfCreateUxSockStmTask: "
                "failure checkpoint 5" );
            ret = iRet;
            break;
        }

        if( !m_bServer )
        {
            ret = GetResponse();
            if( ERROR( ret ) )
                break;
        }

        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );

        CRpcServices* pIf = nullptr;
        ret = oCfg.GetPointer( propIfPtr, pIf );
        if( ERROR( ret ) )
            break;
        
        IStream* pStream =
            dynamic_cast< IStream* >( pIf );

        if( pStream == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        bool& bServer = m_bServer;

        EnumClsid iClsid =
            clsid( CUnixSockStmProxy );

        if( bServer )
            iClsid = clsid( CUnixSockStmServer );

        guint32 dwFd = 0;
        ret = oCfg.GetIntProp( propFd, dwFd );
        if( ERROR( ret ) )
            break;

        IConfigDb* pDataDesc = nullptr;
        ret = oCfg.GetPointer( 0, pDataDesc );
        if( ERROR( ret ) )
            break;

        InterfPtr pUxIf;
        ret = pStream->CreateUxStream( pDataDesc,
            dwFd, iClsid, bServer, pUxIf );

        if( ERROR( ret ) )
            break;

        TaskletPtr pStartTask;
        CParamList oStartParams;

        oStartParams.Push( ObjPtr( pUxIf ) );

        // copy the fd for client
        if( bServer )
        {
            ret = oCfg.GetIntProp( 1, dwFd );
            if( ERROR( ret ) )
                break;

        }

        // dataDesc for response
        oStartParams.Push(
            ObjPtr( pDataDesc ) );

        CCfgOpenerObj oIfCfg(
            ( CObjBase* )pUxIf );

        oIfCfg.SetBoolProp(
            propListenOnly, true );

        oStartParams[ propFd ] = dwFd;

        oStartParams.SetPointer( propIfPtr, pIf );
        oStartParams.CopyProp( propEventSink, this );
        oStartParams[ propIsServer ] = bServer;

        // transfer the callback to the start task
        ret = pStartTask.NewObj( 
            clsid( CIfStartUxSockStmTask ),
            oStartParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        // make sure the task runs sequentially
        ret = pIf->AddSeqTask( pStartTask );
        if( ERROR( ret ) )
            break;

        ClearClientNotify();

    }while( 0 );

    if( ERROR( ret ) )
    {
        CParamList oResp;
        oResp[ propReturnValue ] = ret;

        EventPtr pEvt;
        gint32 iRet = GetInterceptTask( pEvt );
        if( SUCCEEDED( iRet ) )
        {
            CCfgOpenerObj oCfg(
                ( CObjBase* )pEvt );

            oCfg.SetPointer( propRespPtr,
                ( CObjBase* )oResp.GetCfg() );
        }
    }

    return ret;
}

gint32 CIfStartUxSockStmTask::RunTask()
{
    gint32 ret = 0;

    do{
        ObjPtr pIf;
        CParamList oParams(
            ( IConfigDb* )GetConfig() );

        ret = oParams.GetObjPtr( 0, pIf );
        if( ERROR( ret ) )
            break;

        CRpcServices* pSvc = pIf;
        if( pSvc == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ret = pSvc->StartEx( this );
        if( ERROR( ret ) )
            break;

        if( ret == STATUS_PENDING )
            break;

    }while( 0 );

    if( ret == STATUS_PENDING )
        return ret;

    OnTaskComplete( ret );

    return ret;
}

gint32 CIfStartUxSockStmTask::OnTaskComplete(
    gint32 iRet )
{
    gint32 ret = 0;

    CParamList oResp;
    CParamList oParams(
        ( IConfigDb* )GetConfig() );

    do{
        if( ERROR( iRet ) )
        {
            ret = iRet;
            break;
        }
        ObjPtr pIf;
        ret = oParams.GetObjPtr( propIfPtr, pIf );
        if( ERROR( ret ) )
            break;
        
        CRpcServices* pParent = pIf;
        IStream* pStream =
            dynamic_cast< IStream* >( pParent );

        if( pStream == nullptr ||
            pParent == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        bool bServer = false;
        ret = oParams.GetBoolProp(
            propIsServer, bServer );
        if( ERROR( ret ) )
            break;

        ret = oParams.GetObjPtr( 0, pIf );
        if( ERROR( ret ) )
            break;

        CRpcServices* pSvc = pIf;
        if( pSvc == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        InterfPtr pInterf = pIf;
        ret = pStream->AddUxStream(
            ( HANDLE )pSvc, pInterf );

        if( ERROR( ret ) )
            break;

        ret = 0;
        oResp[ propReturnValue ] = 0;
        guint32 dwFd = oParams[ propFd ];
        ObjPtr& pObj = ( ObjPtr& )oParams[ 1 ];

        if( bServer )
        {
            CfgPtr pDesc( true );
            pDesc->Clone( *( IConfigDb* )pObj );
            {
                CParamList oConnParam;
                oConnParam.Push( pObj );
                oConnParam.Push( ( HANDLE )pSvc );
                IConfigDb* pCfg = oConnParam.GetCfg();
                HANDLE hCfg = ( HANDLE )pCfg;
                ret = pStream->OnConnected( hCfg );
                if( ERROR( ret ) )
                    break;
            }

            // set the response for FETCH_DATA request
            CCfgOpener oDataDesc(
                ( IConfigDb* )pDesc );

            guint64 qwId = pIf->GetObjId();
            guint64 qwHash = 0;
            ret = GetObjIdHash( qwId, qwHash );
            if( ERROR( ret ) )
                break;

            oDataDesc[ propPeerObjId ] = qwHash;

            oResp.Push( ObjPtr( pDesc ) );
            oResp.Push( dwFd );

            oResp.Push( 0 );
            oResp.Push( 0 );

        }
        else
        {
            // set the response for OpenChannel
            oResp.Push( pObj );
            oResp.Push( ( HANDLE )pSvc );
            IConfigDb* pCfg = oResp.GetCfg();

            CStreamProxy* pProxy = ObjPtr( pParent );
            ret = pProxy->OnConnected( ( HANDLE )pCfg );
            if( ERROR( ret ) )
            {
                HANDLE hStm;
                oResp.Pop( hStm );
                oResp.Pop( pObj );
                break;
            }
        }

        gint32 ( *func )( CRpcServices*, CRpcServices* ) =
            ([]( CRpcServices* pUxIf, CRpcServices* pParent )->gint32
            {
                gint32 ret = 0;
                if( pUxIf == nullptr || pParent == nullptr )
                    return -EINVAL;
                do{
                    ObjPtr pObj = pUxIf;
                    if( pUxIf->IsServer() )
                    {
                        CUnixSockStmServer* pSvc = pObj;
                        ret = pSvc->StartTicking( nullptr );
                    }
                    else
                    {
                        CUnixSockStmProxy* pSvc = pObj;
                        ret = pSvc->StartTicking( nullptr );
                    }
                    if( SUCCEEDED( ret ) )
                        break;

                    HANDLE hstm = ( HANDLE )pUxIf;
                    pObj = pParent;
                    if( pParent->IsServer() )
                    {
                        CStreamServer* pSvc = pObj;
                        pSvc->OnChannelError( hstm, ret );
                    }
                    else
                    {
                        CStreamProxy* pSvc = pObj;
                        pSvc->OnChannelError( hstm, ret );
                    }

                }while( 0 );
                return ret;
            });

        auto pMgr = pSvc->GetIoMgr();
        TaskletPtr pListenTask;
        ret = NEW_FUNCCALL_TASK( pListenTask,
            pMgr, func, pSvc, pParent );
        if( ERROR( ret ) )
            break;

        ret = pMgr->RescheduleTask( pListenTask );
        if( ERROR( ret ) )
            ( *pListenTask )( eventCancelTask );
        
    }while( 0 );

    oParams.ClearParams();

    if( ERROR( ret ) )
        oResp[ propReturnValue ] = ret;

    EventPtr pEvt;
    iRet = GetInterceptTask( pEvt );
    if( SUCCEEDED( iRet ) )
    {
        CCfgOpenerObj oCfg( ( IEventSink* )pEvt );
        iRet = oCfg.SetPointer( propRespPtr,
            ( CObjBase* )oResp.GetCfg() );
    }
    if( !IsPending() )
        DebugPrint( ret, "Warning, "
            "the task is not pending" );

    return ret;
}

gint32 CIfStopUxSockStmTask::RunTask()
{
    gint32 ret = 0;

    do{
        ObjPtr pIf;
        CParamList oParams(
            ( IConfigDb* )GetConfig() );

        ret = oParams.GetObjPtr( 0, pIf );
        if( ERROR( ret ) )
            break;

        CRpcServices* pSvc = pIf;
        if( pSvc == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ret = pSvc->Shutdown( this );
        if( ERROR( ret ) )
            break;

        if( ret == STATUS_PENDING )
            break;

    }while( 0 );

    if( ret == STATUS_PENDING )
        return ret;

    OnTaskComplete( ret );

    return ret;
}

gint32 CIfStopUxSockStmTask::OnTaskComplete(
    gint32 iRet )
{
    CParamList oParams(
        ( IConfigDb* )GetConfig() );
    oParams.ClearParams();
    oParams.RemoveProperty( propIfPtr );

    return iRet;
}

gint32 CStreamProxy::OpenChannel(
    IConfigDb* pDataDesc,
    int& fd, HANDLE& hChannel,
    IEventSink* pCallback )
{
    if( pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    fd = -1;
    do{
        CCfgOpener oDesc( pDataDesc );
        // add the iid to the data desc
        oDesc.SetIntProp(
            propIid, iid( IStream ) );

        oDesc.SetBoolProp(
            propStreaming, true );

        // prepare for the creation of interface
        // CUnixSockStream
        CParamList oParams;
        oParams[ propIfPtr ] = ObjPtr( this );
        oParams[ propIoMgr ] = ObjPtr( GetIoMgr() );
        oParams[ propFd ] = ( guint32 )fd;
        oParams[ propIsServer ] = ( bool )false;
        oParams[ propEventSink ] = ObjPtr( pCallback );

        oParams.Push( ObjPtr( pDataDesc ) );

        // creation will happen when the FETCH_DATA
        // request succeeds
        TaskletPtr pTask;
        ret = pTask.NewObj(
            clsid( CIfCreateUxSockStmTask ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        // set the task to started state
        ret = ( *pTask )( eventZero );
        if( ret != STATUS_PENDING )
            break;

        OutputMsg( ret, 
            "CStreamProxy OpenChannel: "
            "checkpoint 1" );

        // send the request
        ret = SendSetupReq(
            pDataDesc, fd, pTask );

        if( ret == STATUS_PENDING )
            break;

        if( ERROR( ret ) )
        {
            ( *pTask )( eventCancelTask );
            break;
        }

        gint32 iRet = pTask->GetError();
        if( ERROR( iRet ) )
            break;

        if( iRet != STATUS_PENDING )
        {
            // some other guy is doing the work
            ret = STATUS_PENDING;
            break;
        }

        // the create task is not run,
        // let's reschedule it
        gint32 ( *func )( IEventSink*,
            IConfigDb*, guint32 ) = ([](
            IEventSink* pTask,
            IConfigDb* pDesc,
            guint32 dwFd )->gint32
        {
            CParamList oResp;
            oResp.Push( ObjPtr( pDesc ) );
            oResp.Push( dwFd );
            oResp[ propReturnValue ] =
                ( guint32 )STATUS_SUCCESS;
            TaskletPtr pDummy;
            gint32 ret = pDummy.NewObj(
                clsid( CIfDummyTask ) );
            if( ERROR( ret ) )
                return ret;

            CCfgOpener oCfg( ( IConfigDb* )
                pDummy->GetConfig() );
            oCfg[ propRespPtr ] =
                ObjPtr( oResp.GetCfg() );

            IEventSink* pCaller = pDummy;
            LONGWORD* pData = ( LONGWORD* )pCaller;
            if( pTask != nullptr )
            {
                pTask->OnEvent(
                    eventTaskComp, 0, 0, pData );
            }
            return 0;
        });

        OutputMsg( 0, "OpenChannel: Immediate "
            "actively schedule "
            "the createstmtask" );

        CIoManager* pMgr = GetIoMgr();
        TaskletPtr pActCall;
        ret = NEW_FUNCCALL_TASK(
            pActCall, pMgr, func,
            ( IEventSink* )pTask, 
            pDataDesc, ( guint32 )fd ); 
        if( ERROR( ret ) )
            break;

        ret = pMgr->RescheduleTask(
            pActCall );

        if( SUCCEEDED( ret ) )
            ret = STATUS_PENDING;
 
    }while( 0 );

    if( ERROR( ret ) )
    {
        if( fd >= 0 )
        {
            close( fd );
            fd = -1;
        }
    }

    return ret;
}

// call this helper to start a stream channel
gint32 CStreamProxy::StartStream(
    HANDLE& hChannel,
    IConfigDb* pDesc,
    IEventSink* pCb )
{
    gint32 ret = 0;
    do{
        int fd = -1;

        CfgPtr pParams;
        if( pDesc == nullptr )
        {
            pParams.NewObj();
            pDesc = pParams;
        }

        CParamList oParams( pDesc );
        oParams.SetPointer( propIoMgr, GetIoMgr() );

        bool bAsync = true;
        TaskletPtr pCallback;
        if( pCb == nullptr )
        {
            bAsync = false;
            ret = pCallback.NewObj(
                clsid( CSyncCallback ),
                pDesc );

            if( ERROR( ret ) )
                break;
        }
        else
        {
            pCallback = ObjPtr( pCb );
        }

        oParams.RemoveProperty( propIoMgr );

        if( !oParams.exist( propKeepAliveSec ) )
            oParams.CopyProp(
                propKeepAliveSec, this );

        if( !oParams.exist( propTimeoutSec ) )
            oParams.CopyProp(
                propTimeoutSec, this );


        ret = OpenChannel( oParams.GetCfg(),
            fd, hChannel, pCallback );

        if( ERROR( ret ) )
            break;

        if( ret == STATUS_PENDING && bAsync )
        {
            break;
        }
        else if( ret == STATUS_PENDING )
        {
            CSyncCallback* pscb = pCallback;
            pscb->WaitForComplete();
            ret = pscb->GetError();
            if( ERROR( ret ) )
                break;
        }

        CCfgOpenerObj oTaskCfg(
            ( CObjBase* )pCallback );

        IConfigDb* pCfg = nullptr;
        ret = oTaskCfg.GetPointer(
            propRespPtr, pCfg );
        if( ERROR( ret ) )
            break;

        CParamList oResp( pCfg );
        ret = oResp[ propReturnValue ];
        if( ERROR( ret ) )
            break;

        hChannel = oResp[ 1 ];

    }while( 0 );

    return ret;
}

// call this method when the proxy want to end the
// stream actively
gint32 IStream::CloseChannel(
    HANDLE hChannel,
    IEventSink* pCallback )
{
    InterfPtr pIf;

    gint32 ret = GetUxStream( hChannel, pIf );
    if( ERROR( ret ) )
        return ret;

    do{
        RemoveUxStream( hChannel );

        CRpcServices* pSvc = pIf;
        CRpcServices* pThisIf = GetInterface();

        if( pSvc == nullptr ||
            pThisIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        CParamList oParams;
        oParams.Push( ObjPtr( pIf ) );
        oParams[ propIfPtr ] = ObjPtr( pThisIf );
        if( pCallback != nullptr )
        {
            oParams[ propEventSink ] =
                ObjPtr( pCallback );
        }

        TaskletPtr pStopTask;
        pStopTask.NewObj(
            clsid( CIfStopUxSockStmTask ),
            oParams.GetCfg() );

        ret = pThisIf->AddSeqTask( pStopTask );

        if( SUCCEEDED( ret ) )
            ret = STATUS_PENDING;

    }while( 0 );

    return ret;
}

// call this method when the proxy encounters
// fatal error
gint32 CStreamProxy::CancelChannel(
    HANDLE hChannel )
{
    if( hChannel == INVALID_HANDLE )
        return -EINVAL;

    TaskletPtr pTask;
    gint32 ret = 0;
    do{
        CParamList oParams;
        oParams.SetPointer( propIoMgr, GetIoMgr() );

        TaskletPtr pSyncTask;
        ret = pSyncTask.NewObj(
            clsid( CSyncCallback ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        ret = CloseChannel( hChannel, pSyncTask );
        if( ret == STATUS_PENDING )
        {
            CSyncCallback* pscb = pSyncTask;
            pscb->WaitForComplete();
            ret = pSyncTask->GetError();
        }

    }while( 0 );

    return ret;
}

gint32 IStream::CreateSocket(
    int& fdLocal, int& fdRemote )
{
    gint32 ret = 0;
    do{
        int fds[ 2 ];
        ret = socketpair( AF_UNIX,
            SOCK_STREAM | SOCK_NONBLOCK,
            0, fds );
        if( ret == -1 )
        {
            ret = -errno;
            break;
        }

        fdLocal = fds[ 0 ];
        fdRemote = fds[ 1 ];

    }while( 0 );

    return ret;
}

gint32 CStreamServer::OnChannelError(
    HANDLE hChannel, gint32 iError )
{
    // we will close the channel by complete the
    // invoke task
    if( hChannel == INVALID_HANDLE )
        return -EINVAL;

    return OnClose( hChannel );
}

gint32 CStreamServer::CanContinue()
{
    if( !IsConnected() )
        return ERROR_STATE;
    
    EnumClsid iid = iid( IStream );

    const std::string& strIfName =
        CoGetIfNameFromIid( iid, "s" );

    if( strIfName.empty() )
        return -ENOTSUP;

    if( IsPaused( strIfName ) )
        return ERROR_STATE;

    return 0;
}

gint32 CStreamServer::OpenChannel(
    IConfigDb* pDataDesc,
    int& fd, HANDLE& hChannel,
    IEventSink* pCallback )
{
    if( pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    int iRemote = -1, iLocal = -1;

    do{
        ret = CreateSocket( iLocal, iRemote );
        if( ERROR( ret ) )
            break;

        CParamList oParams;
        oParams[ propIfPtr ] = ObjPtr( this );
        oParams[ propIoMgr ] = ObjPtr( GetIoMgr() );
        oParams[ propFd ] = iLocal;
        oParams[ propEventSink ] = ObjPtr( pCallback );
        oParams[ propIsServer ] = ( bool )true;

        oParams.Push( ObjPtr( pDataDesc ) );
        oParams.Push( iRemote );

        TaskletPtr pTask;
        ret = pTask.NewObj(
            clsid( CIfCreateUxSockStmTask ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        ret = this->AddSeqTask( pTask );
        if( ERROR( ret ) )
            break;

        ret = pTask->GetError();

    }while( 0 );

    if( ERROR( ret ) )
    {
        if( iLocal >= 0 )
        {
            close( iLocal );
            iLocal = -1;
        }
    }

    if( ret != STATUS_PENDING )
    {
        if( iRemote >= 0 )
        {
            close( iRemote );
            iRemote = -1;
        }
    }
    return ret;
}

gint32 CStreamServer::FetchData_Server(
    IConfigDb* pDataDesc,           // [in]
    gint32& fd,                     // [out]
    guint32& dwOffset,              // [in, out]
    guint32& dwSize,                // [in, out]
    IEventSink* pCallback )
{
    HANDLE hChannel = 0;

    gint32 ret = AcceptNewStream(
        pCallback, pDataDesc );
    if( ERROR( ret ) )
        return ret;

    ret = OpenChannel( pDataDesc,
        fd, hChannel, pCallback );

    if( ret == STATUS_PENDING )
        return ret;

    return ret;
}

gint32 IStream::OnClose(
    HANDLE hChannel,
    IEventSink* pCallback )
{
    gint32 ret = CloseChannel(
        hChannel, pCallback );

    if( ret != STATUS_PENDING )
        return ret;

    return 0;
}

gint32 IStream::OnPreStopShared(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    CRpcServices* pThis = GetInterface(); 

    do{
        CStdRMutex oIfLock( pThis->GetLock() );
        if( m_mapUxStreams.empty() )
            break;

        UXSTREAM_MAP mapStreams = m_mapUxStreams;
        m_mapUxStreams.clear();
        oIfLock.Unlock();
        CParamList oGrpParams;
        TaskGrpPtr pTaskGrp;
        oGrpParams[ propIfPtr ] = ObjPtr( pThis );

        ret = pTaskGrp.NewObj(
            clsid( CIfTaskGroup ),
            oGrpParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        if( pCallback != nullptr )
            pTaskGrp->SetClientNotify( pCallback );

        pTaskGrp->SetRelation( logicNONE );

        for( auto elem : mapStreams )
        {
            CParamList oParams;
            oParams.Push( ObjPtr( elem.second ) );
            oParams[ propIfPtr ] = ObjPtr( pThis );

            TaskletPtr pStopTask;
            ret = pStopTask.NewObj(
                clsid( CIfStopUxSockStmTask ),
                oParams.GetCfg() );

            if( ERROR( ret ) )
            {
                ret = -ENOMEM;
                break;
            }
            pTaskGrp->AppendTask( pStopTask );
        }

        if( ERROR( ret ) )
            break;

        if( pTaskGrp->GetTaskCount() == 0 )
            break;

        TaskletPtr pTask = ObjPtr( pTaskGrp );
        ret = pThis->AddSeqTask( pTask );
        if( SUCCEEDED( ret ) )
            ret = STATUS_PENDING;

    }while( 0 );

    return ret;
}

gint32 GetObjIdHash(
    guint64 qwObjId, guint64& qwHash )
{
    SHA1 sha;
    sha.Input( ( const char* )&qwObjId,
        sizeof( qwObjId ) );

    guint32 arrDwords[ 5 ];
    if( !sha.Result( arrDwords ) )
        return ERROR_FAIL;

    guint32* ptr = ( guint32* )&qwHash;

    ptr[ 0 ] = arrDwords[ 0 ];
    ptr[ 1 ] = arrDwords[ 4 ];

    return 0;
}

}
