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
#include "stmcp.h"
#include "seqtgmgr.h"

namespace rpcf
{

void IStream::SetInterface( CRpcServices* pSvc )
{
    m_pSvc = pSvc;

    CCfgOpenerObj oCfg( pSvc );
    oCfg.GetBoolProp(
        propNonSockStm, m_bNonSockStm );

    bool bSeqTgMgr = false;
    gint32 ret = oCfg.GetBoolProp(
        propSeqTgMgr, bSeqTgMgr );
    if( ERROR( ret ) || !bSeqTgMgr )
        return;

    m_pSeqTgMgr.NewObj( clsid( CStmSeqTgMgr ) );
    CStmSeqTgMgr* pSeqMgr = GetSeqTgMgr();
    pSeqMgr->SetParent( GetInterface() );
}

ObjPtr IStream::GetSeqTgMgr()
{ return m_pSeqTgMgr; }

gint32 IStream::AddStartTask(
    HANDLE hChannel, TaskletPtr& pTask )
{
    if( hChannel == INVALID_HANDLE ||
        pTask.IsEmpty() )
        return -EINVAL;
    gint32 ret = 0;
    do{
        CRpcServices* pSvc = GetInterface();
        CStmSeqTgMgr* psmgr = GetSeqTgMgr();
        if( psmgr == nullptr ||
            !pSvc->IsServer() )
            return pSvc->AddSeqTask( pTask );

        ret = psmgr->AddStartTask(
            hChannel, pTask );

    }while( 0 );
    return ret;
}

gint32 IStream::AddStopTask(
    IEventSink* pCallback,
    HANDLE hChannel,
    TaskletPtr& pTask )
{
    if( hChannel == INVALID_HANDLE ||
        pTask.IsEmpty() )
        return -EINVAL;
    gint32 ret = 0;
    do{
        CRpcServices* pSvc = GetInterface();
        CStmSeqTgMgr* psmgr = GetSeqTgMgr();
        bool bSvr = pSvc->IsServer();
        if( psmgr == nullptr || !bSvr )
        {
            if( pCallback != nullptr )
            {
                CIfRetryTask* pRetry = pTask;
                pRetry->SetClientNotify( pCallback );
            }
            ret = pSvc->AddSeqTask( pTask );
            break;
        }

        ret = psmgr->AddStopTask(
            pCallback, hChannel, pTask );

    }while( 0 );
    return ret;
}

gint32 IStream::AddSeqTask2(
    HANDLE hChannel, TaskletPtr& pTask )
{
    if( hChannel == INVALID_HANDLE ||
        pTask.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CRpcServices* pSvc = GetInterface();
        CStmSeqTgMgr* psmgr = GetSeqTgMgr();
        if( psmgr == nullptr ||
            !pSvc->IsServer() )
            return pSvc->AddSeqTask( pTask );

        ret = psmgr->AddSeqTask(
            hChannel, pTask );

    }while( 0 );
    return ret;
}

gint32 IStream::StopSeqTgMgr(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        CStmSeqTgMgr* psmgr = GetSeqTgMgr();
        if( psmgr == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        ret = psmgr->Stop( pCallback );

    }while( 0 );
    return ret;
}

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
        iClsid == clsid( Invalid ) )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CObjBase* pParent = GetInterface();
        CCfgOpener oDataDesc( pDataDesc );
        CParamList oNewCfg;

        oNewCfg.SetBoolProp(
            propIsServer, bServer );

        if( unlikely( this->IsNonSockStm() ) )
        {
            oNewCfg.SetBoolProp(
                propStarter, true );
        }
        else if( unlikely( oDataDesc.exist(
            propStmConnPt ) ) )
        {
            oNewCfg.SetBoolProp(
                propStarter, false );
        }
        else if( unlikely( iFd < 0 ) )
        {
            ret = -EINVAL;
            break;
        }

        oNewCfg.SetIntProp(
            propFd, ( guint32& )iFd );

        oNewCfg.SetPointer(
            propParentPtr, pParent ); 

        guint32 dwTimeoutSec = 0;
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

        EnumClsid iClsid = clsid( Invalid );
        if( bServer )
            iClsid = clsid( CUnixSockStmServer );
        else
            iClsid = clsid( CUnixSockStmProxy );

        ObjPtr pStmCp;
        if( pStream->IsNonSockStm() )
        {
            oCfg.GetObjPtr(
                propStmConnPt, pStmCp );
            oCfg.RemoveProperty( propStmConnPt );
        }

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

        CRpcServices* pUxSvc = pUxIf;

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

        if( pStream->IsNonSockStm() )
        {
            CCfgOpenerObj oIfCfg(
                ( CObjBase* )pUxIf );
            oIfCfg.SetObjPtr(
                propStmConnPt, pStmCp );
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

        if( bServer )
            pStartTask->SetPriority( 1 );

        ret = pStream->AddUxStream(
            ( HANDLE )pUxSvc, pUxIf );
        if( ERROR( ret ) )
            break;

        // make sure the task runs sequentially
        ret = pStream->AddStartTask(
            ( HANDLE )pUxSvc, pStartTask );
        if( ERROR( ret ) )
        {
            InterfPtr pTemp;
            pStream->RemoveUxStream(
                ( HANDLE )pUxSvc, pTemp );

            ( *pStartTask )( eventCancelTask );
            break;
        }

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

CIfStartUxSockStmTask::CIfStartUxSockStmTask(
    const IConfigDb* pCfg ) : super( pCfg )
{ SetClassId( clsid( CIfStartUxSockStmTask ) ); }

CIfStartUxSockStmTask::~CIfStartUxSockStmTask()
{}

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

        gint32 (*func)( CTasklet* ,
            CRpcServices*, CIfStartUxSockStmTask* ) =
        ([]( CTasklet* pCb,
            CRpcServices* pIf,
            CIfStartUxSockStmTask* pTask )->gint32
        {
            if( pTask == nullptr )
                return -EINVAL;
            gint32 ret = 0;
            CCfgOpener oCfg(
                ( IConfigDb* )pCb->GetConfig() );
            IConfigDb* pResp = nullptr;
            ret = oCfg.GetPointer( propRespPtr, pResp );
            if( SUCCEEDED( ret ) )
            {
                CCfgOpener oResp( pResp );
                gint32 iRet = oResp.GetIntProp(
                    propReturnValue, ( guint32& )ret );
                if( ERROR( iRet ) )
                    ret = iRet;
            }

            EnumTaskState iState =
                    pTask->GetTaskState();
            if( pTask->IsStopped( iState ) )
                return ERROR_STATE;

            TaskletPtr pDefer;
            ret = DEFER_CALL_NOSCHED( pDefer,
                pTask, &IEventSink::OnEvent,
               eventTaskComp, ret, 0, nullptr ); 

            if( ERROR( ret ) )
                return ret;

            auto pMgr = pIf->GetIoMgr();
            return pMgr->RescheduleTaskByTid( pDefer );
        });
        // This task is to make sure 'StartTicking' runs 
        // outside the root group.
        TaskletPtr pStartCb;
        ret = NEW_COMPLETE_FUNCALL( 0,
            pStartCb, pSvc->GetIoMgr(),
            func, nullptr, pSvc, this );
        if( ERROR( ret ) )
            break;

        ret = pSvc->StartEx( pStartCb );
        if( ERROR( ret ) )
            ( *pStartCb )( eventCancelTask );

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

    InterfPtr pUxIf;
    ObjPtr pIf;
    bool bUnreg = false;
    HANDLE hcp = INVALID_HANDLE;

    do{
        if( SUCCEEDED( iRet ) )
        {
            EventPtr pCb;
            ret = GetClientNotify( pCb );
            if( SUCCEEDED( ret ) )
            {
                CIfInvokeMethodTask* pInv = pCb;
                if( pInv != nullptr )
                {
                    // reset timer, we don't want the
                    // start process be abrupted by
                    // unwanted timeout.
                    pInv->ResetTimer();
                }
            }
        }

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

        bool bServer = pParent->IsServer();

        ObjPtr pUxObj;
        ret = oParams.GetObjPtr( 0, pUxObj );
        if( ERROR( ret ) )
            break;

        CRpcServices* pSvc = pUxObj;
        if( pSvc == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        pUxIf = pUxObj;

        // all the input parameters are set
        if( ERROR( iRet ) )
        {
            ret = iRet;
            break;
        }

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

            guint64 qwId = pUxIf->GetObjId();
            guint64 qwHash = 0;
            ret = GetObjIdHash( qwId, qwHash );
            if( ERROR( ret ) )
                break;

            oDataDesc[ propPeerObjId ] = qwHash;

            oResp.Push( ObjPtr( pDesc ) );
            oResp.Push( dwFd );

            oResp.Push( 0 );
            oResp.Push( 0 );
            if( pStream->IsNonSockStm() )
            {
                // the fd is ignored
                oResp.SetBoolProp( propNonFd, true );
                CCfgOpenerObj oUxCfg( pSvc );
                CStmConnPoint* pscp;
                ret = oUxCfg.GetPointer(
                    propStmConnPt, pscp );
                if( ERROR( ret ) )
                    break;
                oDataDesc.SetQwordProp(
                    propStmConnPt, ( guint64 )pscp );
                pscp->RegForTransfer();
                bUnreg = true;
                hcp = ( HANDLE )pscp;
            }
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

        if( pSvc->IsServer() )
        {
            CUnixSockStmServer* pStm = pUxIf;
            ret = pStm->StartTicking( nullptr );
        }
        else
        {
            CUnixSockStmProxy* pStm = pUxIf;
            ret = pStm->StartTicking( nullptr );
        }

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

    if( ERROR( ret ) &&
        !pIf.IsEmpty() &&
        !pUxIf.IsEmpty() )
    {
        CRpcServices* pSvc = pIf;
        HANDLE hUxIf = ( HANDLE )
            ( CRpcServices* )pUxIf;
        if( pSvc->IsServer() )
        {
            CStreamServer* pStm = pIf;
            pStm->OnChannelError( hUxIf, ret );
        }
        else
        {
            CStreamProxy* pStm = pIf;
            pStm->OnChannelError( hUxIf, ret );
        }
    }
    if( ERROR( ret ) && bUnreg )
    {
        ObjPtr pObj;
        CStmConnPoint::RetrieveAndUnreg(
            hcp, pObj );
    }
    return ret;
}

gint32 CIfStartUxSockStmTask::OnCancel(
    guint32 dwContext )
{
    gint32 ret = 0;
    CParamList oParams(
        ( IConfigDb* )GetConfig() );

    do{
        DebugPrintEx( logWarning,
            -ECANCELED, "%s canceling",
            CoGetClassName( this->GetClsid() ) );
        ObjPtr pUxIf;
        ObjPtr pIf;
        ret = oParams.GetObjPtr( propIfPtr, pIf );
        if( ERROR( ret ) )
            break;
        
        CRpcServices* pParent = pIf;
        ret = oParams.GetObjPtr( 0, pUxIf );
        if( ERROR( ret ) )
            break;

        CRpcServices* pSvc = pIf;
        HANDLE hUxIf = ( HANDLE )
            ( CRpcServices* )pUxIf;
        if( pSvc->IsServer() )
        {
            CStreamServer* pStm = pIf;
            pStm->OnClose( hUxIf );
        }
        else
        {
            CStreamProxy* pStm = pIf;
            pStm->OnClose( hUxIf );
        }

    }while( 0 );

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
    bool bUnreg = false;
    HANDLE hcp = INVALID_HANDLE;
    fd = -1;
    do{
        CIoManager* pMgr = this->GetIoMgr();
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
        oParams[ propIoMgr ] = ObjPtr( pMgr );
        oParams[ propFd ] = ( guint32 )fd;
        oParams[ propIsServer ] = ( bool )false;
        oParams[ propEventSink ] = ObjPtr( pCallback );

        oParams.Push( ObjPtr( pDataDesc ) );

        ObjPtr pStmCp;
        if( this->IsNonSockStm() )
        {
            CCfgOpener oCpCfg;
            oCpCfg.SetPointer( propIoMgr, pMgr );
            ret = pStmCp.NewObj(
                clsid( CStmConnPoint ),
                oCpCfg.GetCfg() );
            if( ERROR( ret ) )
                break;

            CStmConnPoint* pcp = pStmCp;
            oDesc.SetQwordProp(
                propStmConnPt, ( guint64 )pcp );
            pcp->RegForTransfer();
            bUnreg = true;
            hcp = ( HANDLE )pcp;
            oParams.SetObjPtr(
                propStmConnPt, pStmCp );
        }

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

        // the 'create-sock-stm' task has not run yet,
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
        if( bUnreg && hcp != INVALID_HANDLE )
        {
            ObjPtr pObj;
            CStmConnPoint::RetrieveAndUnreg(
                hcp, pObj );
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

    gint32 ret = RemoveUxStream( hChannel, pIf );
    if( ERROR( ret ) )
    {
        DebugPrint( ret,
            "RemoveUxStream failed 0x%llx", hChannel );
        return ret;
    }

    do{
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

        TaskletPtr pStopTask;
        pStopTask.NewObj(
            clsid( CIfStopUxSockStmTask ),
            oParams.GetCfg() );

        ret = AddStopTask(
            pCallback, hChannel, pStopTask );

        if( ERROR( ret ) )
        {
            DebugPrint( ret,
                "Error OnClose failed to add "
                "stoptask for 0x%llx @state=%d",
                pSvc, pThisIf->GetState() );
            ( *pStopTask )( eventCancelTask );
        }
        else
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
        CIoManager* pMgr = this->GetIoMgr();
        ObjPtr pStmCp;
        CParamList oParams;
        if( !this->IsNonSockStm() )
        {
            ret = CreateSocket( iLocal, iRemote );
            if( ERROR( ret ) )
                break;
        }
        else
        {
            CCfgOpener oCpCfg;
            oCpCfg.SetPointer( propIoMgr, pMgr );
            ret = pStmCp.NewObj(
                clsid( CStmConnPoint ),
                oCpCfg.GetCfg() );
            if( ERROR( ret ) )
                break;
            oParams[ propStmConnPt ] = pStmCp;
        }

        oParams[ propIfPtr ] = ObjPtr( this );
        oParams[ propIoMgr ] = ObjPtr( pMgr );
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

        ret = this->AddAndRun( pTask );
        if( ERROR( ret ) )
            break;

        ret = STATUS_PENDING;

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

gint32 IStream::OnPreStopSharedParallel(
    IEventSink* pCallback )
{
    return StopSeqTgMgr( pCallback );
}

gint32 IStream::OnPreStopShared(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    CRpcServices* pThis = GetInterface(); 
    auto pMgr = pThis->GetIoMgr();

    do{
        CStdRMutex oIfLock( pThis->GetLock() );
        if( m_mapUxStreams.empty() )
            break;

        std::vector< HANDLE > vecStreams;
        for( auto elem : m_mapUxStreams )
            vecStreams.push_back( elem.first );

        oIfLock.Unlock();

        gint32 arrRets[ vecStreams.size() ];
        for( int i = 0; i < vecStreams.size(); i++ )
        {
            arrRets[ i ] = this->OnClose(
                vecStreams[ i ], nullptr );
        }

    }while( 0 );

    if( true )
    {
        // possibly there are stream stop tasks in the
        // m_pSeqTasks, so schedule a conclusion task
        // to make sure all the stop tasks are done.
        TaskletPtr pFinalCb;
        gint32 (*func1)( IEventSink*) =
        ([]( IEventSink* pNotify )->gint32
        {
            pNotify->OnEvent(
                eventTaskComp, 0, 0, nullptr );
            return 0;
        });
        NEW_FUNCCALL_TASK( pFinalCb,
            pMgr, func1, pCallback );
        ret = pThis->AddSeqTask( pFinalCb );
        if( ERROR( ret ) )
            ( *pFinalCb )( eventCancelTask );
        else
            ret = STATUS_PENDING;
    }

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

template<>
bool IsKeyInvalid( const HANDLE& key )
{ return key == INVALID_HANDLE; }

template<>
bool IsKeyInvalid( const stdstr& key )
{ return key.empty(); }

#if BUILD_64 == 1
template<>
bool IsKeyInvalid( const guint32& key )
{ return ( key == 0 || key == ( guint32 )-1 ); }
#endif

}
