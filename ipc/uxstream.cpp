/*
 * =====================================================================================
 *
 *       Filename:  uxstream.cpp
 *
 *    Description:  implementations of CUnixSockStream and related classes
 *
 *        Version:  1.0
 *        Created:  06/28/2019 07:22:19 PM
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
#include "uxstream.h"

namespace rpcf
{

EnumFCState CFlowControl::OnReportInternal(
    guint64 qwAckTxBytes, guint64 qwAckTxPkts )
{
    EnumFCState ret = fcsKeep;

    bool bAboveLast = false;
    bool bAbove = false;

    CStdRMutex oLock( GetLock() );
    if( qwAckTxBytes <= m_qwAckTxBytes ||
        qwAckTxPkts <= m_qwAckTxPkts )
        return ret;

    if( m_qwTxBytes - m_qwAckTxBytes >=
        STM_MAX_PENDING_WRITE ||
        m_qwTxPkts - m_qwAckTxPkts >=
        STM_MAX_PACKETS_REPORT )
        bAboveLast = true;

    if( m_qwTxBytes - qwAckTxBytes >=
        STM_MAX_PENDING_WRITE ||
        m_qwTxPkts - qwAckTxPkts >=
        STM_MAX_PACKETS_REPORT )
        bAbove = true;

    m_qwAckTxBytes = qwAckTxBytes;
    m_qwAckTxPkts = qwAckTxPkts;

    if( bAboveLast != bAbove )
        ret = DecFCCount();

    return ret;
}

EnumFCState CFlowControl::IncTxBytes(
    guint32 dwSize )
{
    EnumFCState ret = fcsKeep;
    if( dwSize == 0 ||
        dwSize > MAX_BYTES_PER_TRANSFER )
        return ret;

    bool bAboveLastBytes = false;
    bool bAboveLastPkts = false;
    bool bAboveBytes = false;
    bool bAbovePkts = false;

    CStdRMutex oLock( GetLock() );

    if( m_qwTxBytes - m_qwAckTxBytes >=
        STM_MAX_PENDING_WRITE )
        bAboveLastBytes = true;

    if( m_qwTxPkts - m_qwAckTxPkts >=
        STM_MAX_PACKETS_REPORT )
        bAboveLastPkts = true;

    m_qwTxBytes += dwSize;
    m_qwTxPkts++;

    if( m_qwTxBytes - m_qwAckTxBytes >=
        STM_MAX_PENDING_WRITE )
        bAboveBytes = true;

    if( m_qwTxPkts - m_qwAckTxPkts >=
        STM_MAX_PACKETS_REPORT )
        bAbovePkts = true;

    // get the flow control to take effect
    // immediately
    if( bAboveLastBytes != bAboveBytes ||
        bAboveLastPkts != bAbovePkts )
        ret = IncFCCount();

    return ret;
}

EnumFCState CFlowControl::IncTxBytes(
    const BufPtr& pBuf )
{
    if( pBuf.IsEmpty() || pBuf->empty() )
        return fcsKeep;

    return IncTxBytes( pBuf->size() );
}


EnumFCState CFlowControl::IncRxBytes(
    guint32 dwSize )
{
    EnumFCState ret = fcsKeep;
    if( dwSize == 0 ||
        dwSize > MAX_BYTES_PER_TRANSFER )
        return ret;

    bool bAboveLastBytes = false;
    bool bAboveLastPkts = false;
    bool bAboveBytes = false;
    bool bAbovePkts = false;

    CStdRMutex oLock( GetLock() );

    if( m_qwRxBytes - m_qwAckRxBytes >=
        STM_MAX_PENDING_WRITE )
        bAboveLastBytes = true;

    if( m_qwRxPkts - m_qwAckRxPkts >=
        STM_MAX_PACKETS_REPORT )
        bAboveLastPkts = true;

    m_qwRxBytes += dwSize;
    m_qwRxPkts++;

    if( m_qwRxBytes - m_qwAckRxBytes >=
        STM_MAX_PENDING_WRITE  )
        bAboveBytes = true;

    if( m_qwRxPkts - m_qwAckRxPkts >=
        STM_MAX_PACKETS_REPORT )
        bAbovePkts = true;

    if( bAboveLastBytes != bAboveBytes ||
        bAboveLastPkts != bAbovePkts )
    {
        ret = fcsReport;
        m_qwAckRxBytes = m_qwRxBytes;
        m_qwAckRxPkts = m_qwRxPkts;
    }

    return ret;
}

EnumFCState CFlowControl::IncRxBytes(
    const BufPtr& pBuf )
{
    if( pBuf.IsEmpty() || pBuf->empty() )
        return fcsKeep;

    return IncRxBytes( pBuf->size() );
}

EnumFCState CFlowControl::IncFCCount()
{
    EnumFCState ret = fcsKeep;
    CStdRMutex oLock( GetLock() );
    ++m_byFlowCtrl;
    if( m_byFlowCtrl == 1 )
        ret = fcsFlowCtrl;
    return ret;
}

EnumFCState CFlowControl::DecFCCount()
{
    EnumFCState ret = fcsKeep;
    CStdRMutex oLock( GetLock() );
    --m_byFlowCtrl;
    if( m_byFlowCtrl == 0 )
        ret = fcsLift;
    return ret;
}

EnumFCState CFlowControl::OnFCReport(
    CfgPtr& pReport )
{
    CCfgOpener oCfg(
        ( IConfigDb* )pReport );

    guint64 qwAckTxPkts = 0;
    guint64 qwAckTxBytes = 0;
    gint32 ret = oCfg.GetQwordProp(
        propRxBytes, qwAckTxBytes );
    if( ERROR( ret ) )
        return fcsKeep;

    ret = oCfg.GetQwordProp(
        propRxPkts, qwAckTxPkts );
    if( ERROR( ret ) )
        return fcsKeep;

    return OnReportInternal(
        qwAckTxBytes, qwAckTxPkts );
}

bool CFlowControl::CanSend() const
{
    if( m_bMonitor )
        return true;

    CStdRMutex oLock( GetLock() );
    return IsFlowCtrl();
}

bool CFlowControl::IsFlowCtrl() const
{ return m_byFlowCtrl == 0; }

bool CFlowControl::GetOvershoot(
    guint64& qwBytes,
    guint64& qwPkts ) const
{
    CStdRMutex oLock( GetLock() );
    if( !IsFlowCtrl() )
        return false;
    qwBytes = qwPkts = 0;
    guint64 qwDelta =
        m_qwTxPkts - m_qwAckTxPkts;
    if( qwDelta > STM_MAX_PACKETS_REPORT )
    {
        qwPkts = qwDelta - STM_MAX_PACKETS_REPORT;
        return true;
    }
    qwDelta = m_qwTxBytes - m_qwAckTxBytes;
    if( qwDelta > STM_MAX_PENDING_WRITE )
    {
        qwBytes = qwDelta - STM_MAX_PENDING_WRITE;
        return true;
    }
    return false;
}

CfgPtr CFlowControl::GetReport()
{
    CParamList oProgress;
    CStdRMutex oLock( GetLock() );
    oProgress.SetQwordProp(
        propRxBytes, m_qwRxBytes );

    oProgress.SetQwordProp(
        propRxPkts, m_qwRxPkts );

    return oProgress.GetCfg();
}

void CFlowControl::GetBytesTransfered(
    guint64& qwRxBytes, guint64& qwTxBytes )
{
    CStdRMutex oLock( GetLock() );
    qwRxBytes = m_qwRxBytes;
    qwTxBytes = m_qwTxBytes;
}

gint32 CIfUxPingTask::RunTask()
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );

        if( !oCfg.exist( propIfPtr ) )
        {
            ret = -ENOENT;
            break;
        }

        guint32 dwInterval = 0;
        ret = oCfg.GetIntProp(
            propKeepAliveSec, dwInterval );

        if( ERROR( ret ) )
            break;

        oCfg.SetIntProp(
            propIntervalSec, dwInterval );

        ret = STATUS_MORE_PROCESS_NEEDED;

    }while( 0 );

    return ret;
}

gint32 CIfUxPingTask::OnCancel(
    guint32 dwContext )
{
    RemoveTimer();
    return super::OnCancel( dwContext );
}

gint32 CIfUxPingTask::OnRetry()
{
    // Note: we start a ping request in this
    // OnRetry method. 
    gint32 ret = 0;
    do{
        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );

        ObjPtr pObj;
        ret = oCfg.GetObjPtr(
            propIfPtr, pObj );

        if( ERROR( ret ) )
            break;

        CRpcServices* pIf = pObj;
        if( unlikely( pIf == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }

        CParamList oParams;
        oParams.SetPointer( propIoMgr,
            pIf->GetIoMgr() );

        oParams.SetPointer(
            propIfPtr, this );

        oParams.CopyProp(
            propKeepAliveSec, this );

        oParams.CopyProp(
            propTimeoutSec, this );

        TaskletPtr pDummyTask;
        ret = pDummyTask.NewObj(
            clsid( CIfDummyTask ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        BufPtr pBuf;
        pIf = pObj;
        if( pIf->IsServer() )
        {
            CUnixSockStmServer* pSvr = pObj;
            ret = pSvr->SendPingPong(
                tokPing, pDummyTask  );
        }
        else
        {
            CUnixSockStmProxy* pProxy = pObj;
            ret = pProxy->SendPingPong(
                tokPing, pDummyTask  );
        }

        if( ret == STATUS_PENDING )
        {
            // complete current task
            ret = 0;
        }

    }while( 0 );

    return ret;
}

gint32 CIfUxPingTicker::OnCancel(
    guint32 dwContext )
{
    RemoveTimer();
    return super::OnCancel( dwContext );
}

gint32 CIfUxPingTicker::RunTask()
{
    return STATUS_PENDING;
}

gint32 CIfUxPingTicker::OnRetry()
{
    // Note: we terminate the stream in this
    // OnRetry method. Don't get confused
    // by the method name.
    // 
    gint32 ret = 0;
    do{
        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );

        ObjPtr pObj;
        ret = oCfg.GetObjPtr(
            propIfPtr, pObj );

        if( ERROR( ret ) )
            break;

        BufPtr pBuf;
        InterfPtr pIf = pObj;
        if( pIf->IsServer() )
        {
            CUnixSockStmServer* pSvr = pIf;
            ret = pSvr->PostUxSockEvent(
                tokClose, pBuf );
        }
        else
        {
            CUnixSockStmProxy* pProxy = pIf;
            ret = pProxy->PostUxSockEvent(
                tokClose, pBuf );
        }

    }while( 0 );

    return ret;
}

CIfUxSockTransTask::CIfUxSockTransTask(
    const IConfigDb* pCfg ) :
    super( pCfg )
{
    gint32 ret = 0;
    SetClassId( clsid( CIfUxSockTransTask ) );

    CParamList oParams(
        ( IConfigDb* )GetConfig() );
    do{
        CRpcServices* pIf;
        ret = oParams.GetPointer(
            propIfPtr, pIf );

        if( ERROR( ret ) )
            break;
        m_pUxStream = pIf;

        ret = oParams.GetIntProp(
            propTimeoutSec,
            m_dwTimeoutSec );

        if( ERROR( ret ) )
            break;

        ret = oParams.GetIntProp(
            propKeepAliveSec,
            m_dwKeepAliveSec );

        if( ERROR( ret ) )
            break;

        m_bIn = ( bool& )oParams[ 0 ];

    }while( 0 );

    if( ERROR( ret ) )
    {
        std::string strMsg = DebugMsg( ret,
            "Error in CIfUxSockTransTask ctor" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CIfUxSockTransTask::PostData(
    BufPtr& pPayload )
{
    gint32 ret = 0;
    CCfgOpenerObj oCallback( this );
    do{
        if( pPayload.IsEmpty() ||
            pPayload->empty() )
        {
            ret = -EFAULT;
            break;
        }

        if( m_pUxStream->IsServer() )
        {
            CUnixSockStmServer* pSvr = m_pUxStream;
            pSvr->PostUxSockEvent(
                tokData, pPayload );
        }
        else
        {
            CUnixSockStmProxy* pProxy = m_pUxStream;
            pProxy->PostUxSockEvent(
                tokData, pPayload );
        }

    }while( 0 );

    return ret;
}

gint32 CIfUxSockTransTask::OnTaskComplete(
    gint32 iRet )
{
    gint32 ret = 0;
    if( ERROR( iRet ) )
        return iRet;

    CCfgOpenerObj oCallback( this );
    do{
        IConfigDb* pResp = nullptr;
        ret = oCallback.GetPointer(
            propRespPtr, pResp );

        if( ERROR( ret ) )
            break;

        CParamList oResp( pResp );
        BufPtr pPayload;
        oResp.GetProperty( 0, pPayload );
        if( ERROR( ret ) )
            break;
        
        ret = PostData( pPayload );
        if( SUCCEEDED( ret ) )
        {
            // don't read immediately, to
            // let other tasks to have
            // chance to run
            ret = StartNewListening();
        }

    }while( 0 );

    return ret;
}

gint32 CIfUxSockTransTask::HandleResponse(
    IRP* pIrp )
{
    gint32 ret = 0;

    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    ret = pIrp->GetStatus();
    if( ERROR( ret ) )
        return ret;

    do{
        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        BufPtr pPayload = pCtx->m_pRespData;

        ret = PostData( pPayload );

    }while( 0 );

    return ret;
}

gint32 CIfUxSockTransTask::StartNewListening()
{
    gint32 ret = 0;
    do{
        // start another listening task
        CParamList oParams;
        oParams.SetPointer( propIfPtr,
            ( CObjBase* )m_pUxStream );

        oParams.SetIntProp(
            propKeepAliveSec, m_dwKeepAliveSec );

        oParams.SetIntProp(
            propTimeoutSec, m_dwTimeoutSec );

        oParams.CopyProp( propIoMgr, this );
        oParams.Push( m_bIn );

        TaskletPtr pTask;
        ret = pTask.NewObj(
            clsid( CIfUxSockTransTask ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        CRpcServices* pIf = m_pUxStream;
        pIf->RunManagedTask( pTask );

    }while( 0 );

    return ret;
}

gint32 CIfUxSockTransTask::OnIrpComplete( IRP* pIrp )
{
    gint32 ret = 0;

    do{
        ret = HandleResponse( pIrp );
        if( ERROR( ret ) )
            break;

        ret = StartNewListening();

    }while( 0 );

    return ret;
}

gint32 CIfUxSockTransTask::OnComplete( gint32 iRet )
{
    super::OnComplete( iRet );
    m_pUxStream.Clear();
    return iRet;
}

gint32 CIfUxSockTransTask::RunTask()
{
    gint32 ret = 0;
    do{
        if( m_pUxStream->IsServer() )
        {
            CUnixSockStmServer* pSvr = m_pUxStream;
            if( m_bIn )
            {
                ret = pSvr->StartReading( this );
                if( SUCCEEDED( ret ) )
                    ret = OnTaskComplete( ret );
            }
            else
            {
                if( m_pPayload.IsEmpty() ||
                    m_pPayload->empty() )
                {
                    ret = ERROR_STATE;
                    break;
                }
                ret = pSvr->WriteStream(
                    m_pPayload, this );
            }
        }
        else
        {
            CUnixSockStmProxy* pProxy = m_pUxStream;
            if( m_bIn )
            {
                ret = pProxy->StartReading( this );
                if( SUCCEEDED( ret ) )
                    ret = OnTaskComplete( ret );
            }
            else
            {
                if( m_pPayload.IsEmpty() ||
                    m_pPayload->empty() )
                {
                    ret = ERROR_STATE;
                    break;
                }
                ret = pProxy->WriteStream(
                    m_pPayload, this );
            }
        }
        break;

    }while( 1 );

    return ret;
}

gint32 CIfUxListeningTask::RunTask()
{
    gint32 ret = 0;
    do{
        CRpcServices* pIf = nullptr;
        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );

        ret = oCfg.GetPointer( propIfPtr, pIf );
        if( ERROR( ret ) )
            break;

        ObjPtr pObj( pIf );
        if( pIf->IsServer() )
        {
            CUnixSockStmServer* pSvr = pObj;
            ret = pSvr->StartListening( this );
        }
        else
        {
            CUnixSockStmProxy* pProxy = pObj;
            ret = pProxy->StartListening( this );
        }

        if( ret == STATUS_PENDING )
            break;

        if( ERROR( ret ) )
            break;

        ret = OnTaskComplete( ret );
        if( ERROR( ret ) )
            break;

    }while( 1 );

    return ret;
}

gint32 CIfUxListeningTask::PostEvent(
    BufPtr& pBuf )
{
    if( pBuf.IsEmpty() ||
        pBuf->empty() )
        return -EINVAL;

    CRpcServices* pIf = nullptr;
    CCfgOpener oCfg(
        ( IConfigDb* )GetConfig() );

    gint32 ret = oCfg.GetPointer(
        propIfPtr, pIf );

    if( ERROR( ret ) )
        return ret;

    guint8 byToken = pBuf->ptr()[ 0 ];
    BufPtr pNewBuf( true );
    switch( byToken )
    {
    case tokPing:
    case tokPong:
    case tokClose:
    case tokFlowCtrl:
    case tokLift:
        {
            break;
        }
    case tokData:
    case tokProgress:
        {
            pBuf->SetOffset( pBuf->offset() +
                UXPKT_HEADER_SIZE );
            pNewBuf = pBuf;
            break;
        }
    case tokError:
        {
            pBuf->SetOffset(
                pBuf->offset() + 1 );
            pNewBuf = pBuf;
            break;
        }
    default:
        {
            ret = -EINVAL;
            break;
        }
    }

    if( ERROR( ret ) )
        return ret;

    ObjPtr pObjIf( pIf );
    if( pIf->IsServer() )
    {
        CUnixSockStmServer* pSvr = pObjIf;
        ret = pSvr->OnUxSockEvent(
            byToken, pNewBuf );
    }
    else
    {
        // to make sure the first tokData to receive
        // happens after OnStreamReady is called.
        CUnixSockStmProxy* pProxy = pObjIf;
        ret = pProxy->OnUxSockEvent(
            byToken, pNewBuf );
    }
    return ret;
}

gint32 CIfUxListeningTask::OnTaskComplete(
    gint32 iRet )
{
    gint32 ret = 0;

    do{
        if( ERROR( iRet ) )
        {
            ret = iRet;
            break;
        }
        IConfigDb* pCfg = GetConfig();
        CCfgOpener oCfg( pCfg );

        ObjPtr pIf;
        ret = oCfg.GetObjPtr( propIfPtr, pIf );
        if( ERROR( ret ) )
            break;

        IConfigDb* pResp = nullptr;
        ret = oCfg.GetPointer(
            propRespPtr, pResp );

        if( ERROR( ret ) )
            break;

        CParamList oResp( pResp );
        BufPtr pPayload;
        oResp.GetProperty( 0, pPayload );
        if( ERROR( ret ) )
            break;
        
        if( pPayload.IsEmpty() ||
            pPayload->empty() )
        {
            ret = -EFAULT;
            break;
        }

        ret = PostEvent( pPayload );

    }while( 0 );

    if( ERROR( ret ) )
    {
        DebugPrint( ret, "Error, the channel will "
            "be closed from OnTaskComplete" );
        BufPtr pErrBuf( true );
        *pErrBuf = tokError;
        gint32 iRet = htonl( ret );
        pErrBuf->Append(
            ( guint8* )&iRet, sizeof( iRet ) );
        PostEvent( pErrBuf );
    }

    return ret;
}

gint32 CIfUxListeningTask::OnIrpComplete( IRP* pIrp )
{
    gint32 ret = 0;

    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    do{
        IConfigDb* pCfg = GetConfig();
        CCfgOpener oCfg( pCfg );
        CRpcServices* pIf = nullptr;
        ret = oCfg.GetPointer( propIfPtr, pIf );
        if( ERROR( ret ) )
            break;

        ret = pIrp->GetStatus();
        if( ERROR( ret ) )
            break;

        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        BufPtr pPayload = pCtx->m_pRespData;
        if( pPayload.IsEmpty() ||
            pPayload->empty() )
        {
            ret = -EFAULT;
            break;
        }
        ret = PostEvent( pCtx->m_pRespData );
        if( ERROR( ret ) )
            break;

        // start another listening task
        oCfg.RemoveProperty( propIrpPtr );
        oCfg.RemoveProperty( propRespPtr );

        ret = ReRun();
        if( ERROR( ret ) )
            break;

        ret = STATUS_PENDING;

    }while( 0 );

    if( ERROR( ret ) )
    {
        DebugPrint( ret, "Error, the channel will "
            "be closed from OnIrpComplete" );
        BufPtr pErrBuf( true );
        *pErrBuf = tokError;
        gint32 iRet = htonl( ret );
        pErrBuf->Append(
            ( guint8* )&iRet, sizeof( iRet ) );
        PostEvent( pErrBuf );
    }

    return ret;
}

}
