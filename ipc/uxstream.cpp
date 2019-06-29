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
 * =====================================================================================
 */
#include "uxstream.h"

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

gint32 CIfUxPingTask::OnRetry()
{
    // Note: we start a ping request in this
    // OnRetry method. 
    gint32 ret = 0;
    do{
        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );

        CRpcServices* pIf = nullptr;
        ObjPtr pObj;
        ret = oCfg.GetObjPtr(
            propIfPtr, pObj );

        if( ERROR( ret ) )
            break;

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

gint32 CIfUxPingTicker::RunTask()
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
            propTimeoutSec, dwInterval );

        if( ERROR( ret ) )
            break;

        oCfg.SetIntProp(
            propIntervalSec, dwInterval );

        ret = STATUS_MORE_PROCESS_NEEDED;

    }while( 0 );

    return ret;
}

gint32 CIfUxPingTicker::OnRetry()
{
    // Note: we terminate the stream in this
    // OnRetry method. Don't get confused
    // literally.
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

gint32 CIfUxSockTransTask::OnIrpComplete( IRP* pIrp )
{
    gint32 ret = 0;

    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    ret = pIrp->GetStatus();
    if( ERROR( ret ) )
        return ret;

    do{
        IrpCtxPtr& pCtx =
            pIrp->GetTopStack();
        m_pPayload = pCtx->m_pRespData;

        if( m_pUxStream->IsServer() )
        {
            CUnixSockStmServer* pSvr = m_pUxStream;
            pSvr->PostUxSockEvent(
                tokData, m_pPayload );
        }
        else
        {
            CUnixSockStmProxy* pProxy = m_pUxStream;
            pProxy->PostUxSockEvent(
                tokData, m_pPayload );
        }

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
                pSvr->StartReading( this );
            else
            {
                if( m_pPayload.IsEmpty() ||
                    m_pPayload->empty() )
                {
                    ret = ERROR_STATE;
                    break;
                }
                pSvr->WriteStream( m_pPayload, this );
            }
        }
        else
        {
            CUnixSockStmProxy* pProxy = m_pUxStream;
            if( m_bIn )
                pProxy->StartReading( this );
            else
            {
                if( m_pPayload.IsEmpty() ||
                    m_pPayload->empty() )
                {
                    ret = ERROR_STATE;
                    break;
                }
                pProxy->WriteStream( m_pPayload, this );
            }
        }

    }while( 0 );

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

    }while( 0 );

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
    BufPtr pNewBuf;
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
    case tokProgress:
        {
            guint32 dwSize;
            memcpy( &dwSize, pBuf->ptr() + 1,
                sizeof( guint32 ) );
            dwSize = ntohl( dwSize );
            BufPtr pNewBuf;
            pNewBuf->Resize( dwSize );
            if( ERROR( ret ) )
                break;

            if( dwSize > pBuf->size()- 1 - sizeof( guint32 ) )
            {
                ret = -EBADMSG;
                break;
            }

            memcpy( pNewBuf->ptr(),
                pBuf->ptr() + 1 + sizeof( guint32 ),
                dwSize );
            break;
        }
    case tokError:
        {
            gint32 iError;
            memcpy( &iError, pBuf->ptr() + 1,
                sizeof( gint32 ) );

            iError = ntohl( iError );
            BufPtr pNewBuf;
            pNewBuf->Resize( sizeof( gint32 ) );
            memcpy( pNewBuf->ptr(),
                &iError, sizeof( iError ) );
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
        ret = pSvr->PostUxSockEvent(
            byToken, pNewBuf );
    }
    else
    {
        CUnixSockStmProxy* pProxy = pObjIf;
        ret = pProxy->PostUxSockEvent(
            byToken, pNewBuf );
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
        CRpcServices* pIf = nullptr;
        CCfgOpener oCfg( pCfg );
        ret = oCfg.GetPointer( propIfPtr, pIf );
        if( ERROR( ret ) )
            break;

        ret = pIrp->GetStatus();
        if( ERROR( ret ) )
        {
            BufPtr pCloseBuf( true );
            *pCloseBuf = tokClose;
            PostEvent( pCloseBuf );
            break;
        }
        IrpCtxPtr& pCtx = pIrp->GetTopStack();
        ret = PostEvent( pCtx->m_pRespData );
        if( ERROR( ret ) )
            break;

        // start another listening task
        CParamList oParams;
        oParams.CopyProp( propIfPtr, this );
        oParams.CopyProp( propIoMgr, this );

        TaskletPtr pTask;

        ret = pTask.NewObj(
            clsid( CIfUxListeningTask ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        pIf->RunManagedTask( pTask );


    }while( 0 );

    return ret;
}
