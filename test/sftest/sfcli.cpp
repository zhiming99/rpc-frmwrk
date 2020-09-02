/*
 * =====================================================================================
 *
 *       Filename:  sfcli.cpp
 *
 *    Description:  implementation of the proxy interface class CEchoClient.
 *
 *        Version:  1.0
 *        Created:  07/15/2018 01:15:23 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com ), 
 *   Organization:  
 *
 * =====================================================================================
 */


#include <rpc.h>
#include <string>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cppunit/TestFixture.h>
#include "sfsvr.h"

using namespace std;

CEchoClient::CEchoClient(
    const IConfigDb* pCfg )
    : super( pCfg )
{
}

gint32 CEchoClient::InitUserFuncs()
{
    BEGIN_IFPROXY_MAP( CEchoServer, false );

    ADD_USER_PROXY_METHOD_EX( 1,
        CEchoClient::Echo,
        METHOD_Echo );

    END_PROXY_MAP;

    return 0;
}

gint32 CEchoClient::Echo(
    const std::string& strText, // [ in ]
    std::string& strReply ) // [ out ]
{
    return FORWARD_IF_CALL( iid( CEchoServer ),
        1, __func__, strText, strReply  );
}
/**
* @name InitUserFuncs
* @{
* Init the proxy's proxy method map.
* */
/** 
 *  Usually it is not necessary to call
 *  super::InitUserFuncs unless this class
 *  inherits from another interface class. In this
 *  case, the IStream interface.
 * @} */

gint32 CMyFileProxy::InitUserFuncs()
{
    // IStream's interface
    super::InitUserFuncs();
    BEGIN_IFPROXY_MAP( CMyFileServer, false );

    ADD_USER_PROXY_METHOD_EX( 1,
        CMyFileProxy::GetFileInfo,
        "GetFileInfo" );

    END_PROXY_MAP;
    return 0;
}

/**
* @name OnRecvData_Loop
* @{ parameters:
*   hChannel: handle to the channel on which there
*   is data ready or event pending.
*
*   iRet: STATUS_SUCCESS when there is data ready
*   for receiving. And an error code if the last
*   receive request failed.
*
* return value: ignored.
* */
/**
 * An event handler when there is data arrives
 * to the channel `hChannel'. It will iterate till
 * all the data packets in the pending queue are
 * consumed (written to the storage).
 * @} */
gint32 CMyFileProxy::OnRecvData_Loop(
    HANDLE hChannel, gint32 iRet )
{
    if( hChannel == INVALID_HANDLE )
        return -EINVAL;

    gint32 ret = 0;
    ObjPtr pObj;
    ret = GetTransCtx( hChannel, pObj );
    if( ERROR( ret ) )
    {
        SetErrorAndClose( hChannel, ret );
        return ret;
    }

    CTransferContext* ptctx = pObj;
    if( ptctx->m_bUpload )
        return 0;

    BufPtr pBuf;
    do{
        if( ERROR( iRet ) )
        {
            ret = iRet;
            break;
        }
        pBuf.Clear();
        ret = this->ReadMsg(
            hChannel, pBuf, -1 );
        if( ret == -EAGAIN )
        {
            ret = 0;
            break;
        }
        else if( ERROR( ret ) )
        {
            break;
        }

        ret = write( ptctx->m_iFd,
            pBuf->ptr(), pBuf->size() );
        if( ret == -1 )
        {
            ret = -errno;
            break;
        }
        ptctx->m_qwCurOffset += pBuf->size();
        guint32 dwRecved = ptctx->m_qwCurOffset -
            ptctx->m_qwStart;

        if( dwRecved == ptctx->m_dwReqSize )
        {
            // all the data received. we can close
            // the channel now.
            ret = ptctx->SetError( 0 );
            close( ptctx->m_iFd );
            ptctx->m_iFd = -1;
            SetErrorAndClose( hChannel );
            break;
        }
        if( dwRecved > ptctx->m_dwReqSize )
        {
            // more data received beyond the
            // request.
            ret = dwRecved - ptctx->m_dwReqSize;
            DebugPrint( ret, "File corrupted " );
            ret = -EFBIG;
            break;
        }

    }while( 1 );

    if( ERROR( ret ) )
    {
        ptctx->SetError( ret );
        SetErrorAndClose( hChannel, ret );
    }

    return ret;
}

/**
* @name OnSendDone_Loop
* @{
* Parameters:
*   hChannel: the handle of the stream channel
*   whose last send request is done.
*
*   iRet: the return code for the last send
*   request. STATUS_SUCCESS for successful
*   transfer. and error code for failed transfer.
* */
/**
 * an event handler called when the last pending
 * write is done on the channel. All the write
 * requests are guaranteed to be services in the
 * FIFO manner. a short transfer indicates the
 * end of the upload/download.
 * @} */
gint32 CMyFileProxy::OnSendDone_Loop(
    HANDLE hChannel, gint32 iRet )
{
    if( hChannel == INVALID_HANDLE )
        return -EINVAL;

    gint32 ret = 0;
    ObjPtr pObj;
    ret = GetTransCtx( hChannel, pObj );
    if( ERROR( ret ) )
    {
        SetErrorAndClose( hChannel, ret );
        return ret;
    }

    CTransferContext* ptctx = pObj;
    if( !ptctx->m_bUpload )
        return 0;

    BufPtr pBuf( true );
    do{
        if( ERROR( iRet ) )
        {
            ret = iRet;
            break;
        }
        guint32 dwSent =
            ptctx->m_qwCurOffset -
            ptctx->m_qwStart;

        if( dwSent + STM_MAX_BYTES_PER_BUF >
            ptctx->m_dwReqSize )
        {
            // upload is done
            ptctx->m_qwCurOffset = ptctx->m_qwStart
                + ptctx->m_dwReqSize;
            ret = ptctx->SetError( 0 );
            // don't quit immediately, waiting the
            // peer side to close the stream
            CMainIoLoop* pLoop = m_pRdLoop;
            if( unlikely( pLoop == nullptr ) )
                break;
            pLoop->SetError( 0 );
            break;
        }

        ptctx->m_qwCurOffset +=
            STM_MAX_BYTES_PER_BUF;

        // read a block from the file and send out
        ret = ReadAndSend( hChannel, ptctx );

    }while( 0 );

    if( ERROR( ret ) )
    {
        ptctx->SetError( ret );
        SetErrorAndClose( hChannel, ret );
    }

    return ret;
}
/**
* @name OnWriteEnabled_Loop
* @{
* Parameters:
*   hChannel: the handle of the channel, which
*   resumes sending from the flow control.
*
* return value:
*   ignored.
* */
/**
 * It is called when the flow control is lifted
 * for the underlying streaming object since the
 * last ERROR_QUEUE_FULL is returned. It will try
 * to resend the blocked data buffer as held in
 * the transfer context.
 *
 * On successful return of ReadAndWrite or
 * ResumeBlockedSend, the sending request should
 * still in the sending queue waiting for
 * transmission, or blocked by flowcontrol. In
 * either case, the method does not attempt to
 * move on to send another block, unlike what
 * OnRecvData_Loop does.
 * @} */

gint32 CMyFileProxy::OnWriteEnabled_Loop(
    HANDLE hChannel )
{
    if( hChannel == INVALID_HANDLE )
        return -EINVAL;

    gint32 ret = 0;
    CTransferContext* ptctx = nullptr;
    do{
        ObjPtr pObj;
        bool bFirst = false;
        ret = GetTransCtx( hChannel, pObj );
        if( ret == -ENOENT )
        {
            // this is the first event after the
            // stream channel is established.
            // let's create the transfer context
            // before the transfer begins.
            ret = OnStreamStarted( hChannel );
            if( ERROR( ret ) )
                break;

            ret = GetTransCtx( hChannel, pObj );
            if( ERROR( ret ) )
                break;

            bFirst = true;
        }

        ptctx = pObj;
        if( ptctx == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        // for receiving end, do nothing here
        if( ptctx->m_bUpload == this->IsServer() )
            break;

        if( unlikely( bFirst ) )
        {
            // transfer starts, we don't have
            // blocked transfer at this moment.
            //
            // just send out the first block.
            ret = ReadAndSend( hChannel, pObj );
        }
        else
        {
            // resend the blocked transfer
            ret = ResumeBlockedSend(
                hChannel, ptctx );
        }

    }while( 0 );

    if( ERROR( ret ) )
    {
        if( ptctx != nullptr )
            ptctx->SetError( ret );
        SetErrorAndClose( hChannel, ret );
    }

    return ret;
}

gint32 CMyFileProxy::OnCloseChannel_Loop(
    HANDLE hChannel )
{
    gint32 ret = 0;
    do{
        CMainIoLoop* pLoop = m_pRdLoop;
        if( unlikely( pLoop == nullptr ) )
            break;
        ret = pLoop->GetError();
        if( ret == STATUS_PENDING )
            pLoop->SetError( 0 );

    }while( 0 );

    return StopLoop( ret );
}

void CMyFileProxy::SetErrorAndClose(
    HANDLE hChannel, gint32 iError )
{
    do{
        CMainIoLoop* pLoop = m_pRdLoop;
        if( unlikely( pLoop == nullptr ) )
            break;

        // set the error code
        pLoop->SetError( iError );
        ScheduleToClose( hChannel, true );

        // stop the loop ASAP, to prevent handling
        // the remaining events in the queue,
        // which could overwrite the error code
        // due to the on-going stop process.
        StopLoop( iError );

    }while( 0 );

    return;
}

gint32 CMyFileProxy::GetFileInfo(
    const std::string& strFile, // [ in ]
    CfgPtr& pResp )    // [ out ]
{
    return FORWARD_IF_CALL( iid( CMyFileServer ),
        1, __func__, strFile, pResp  );
}

gint32 CMyFileProxy::OnOpenChanComplete(
    IEventSink* pCallback,
    IEventSink* pIoReqTask,
    IConfigDb* pContext )
{
    if( pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    HANDLE hChannel = INVALID_HANDLE;
    do{
        CCfgOpenerObj oCfg( pCallback );
        IConfigDb* pResp = nullptr;
        ret = oCfg.GetPointer(
            propRespPtr, pResp );

        if( ERROR( ret ) )
            break;

        CCfgOpener oResp( pResp );
        gint32 iRet = 0;

        ret = oResp.GetIntProp(
            propReturnValue, ( guint32& )iRet );
        if( ERROR( ret ) )
            break;

        guint32* pval = nullptr;
        ret = oResp.GetIntPtr( 1, pval );
        if( ERROR( ret ) )
            break;

        hChannel = ( HANDLE )pval;

    }while( 0 );

    if( ERROR( ret ) )
    {
        if( hChannel != INVALID_HANDLE )
        {
            SetErrorAndClose( hChannel, ret );
        }
        else
        {
            StopLoop( ret );
        }
    }
    return ret;
}

gint32 CMyFileProxy::StartTransfer(
    CTransferContext* pCtx )
{
    if( pCtx == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    TaskletPtr pOpenComp;

    do{
        HANDLE hChannel = INVALID_HANDLE; 
        CParamList oDesc;

        if( !oDesc.exist( propKeepAliveSec ) )
            oDesc.CopyProp(
                propKeepAliveSec, this );

        if( !oDesc.exist( propTimeoutSec ) )
            oDesc.CopyProp(
                propTimeoutSec, this );

        BufPtr pBuf( true );
        ret = pCtx->Serialize( *pBuf );
        if( ERROR( ret ) )
            break;

        oDesc.Push( pBuf );

        ObjPtr pEmpty;
        ret = NEW_PROXY_RESP_HANDLER(
            pOpenComp, this,
            &CMyFileProxy::OnOpenChanComplete,
            this, pEmpty );

        if( ERROR( ret ) )
            break;

        CIfResponseHandler* pHandler = pOpenComp;
        if( unlikely( pHandler == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }

        // correct the first parameter to
        // OnOpenChanComplete
        BufPtr pParam0( true );
        *pParam0 = ObjPtr( pHandler );
        ret = pHandler->UpdateParamAt( 0, pParam0 );
        if( ERROR( ret ) )
            break;

        gint32 fd = -1;
        ret = OpenChannel( oDesc.GetCfg(),
            fd, hChannel, pOpenComp );

    }while( 0 );

    if( ret != STATUS_PENDING )
    {
        if( !pOpenComp.IsEmpty() )
            ( *pOpenComp )( eventCancelTask );
    }
    else
    {
        // transferred to pOpenComp
        ret = 0;
    }

    if( ERROR( ret ) )
        StopLoop( ret );

    return ret;
}

gint32 CMyFileProxy::UploadDownload(
    const std::string& strSrcFile, bool bUpload )
{
    gint32 ret = 0;
    do{
        if( strSrcFile.size() > 512 )
        {
            ret = -EINVAL;
            break;
        }
        std::string strFileName =
            basename( strSrcFile.c_str() );
        if( strFileName.find( '/' ) !=
            std::string::npos )
        {
            ret = -EINVAL;
            break;
        }

        ObjPtr pObj;
        ret = pObj.NewObj(
            clsid( CTransferContext ) );
        if( ERROR( ret ) )
            break;

        CTransferContext* ptctx = pObj;
        ptctx->m_strFileName = strFileName;
        ptctx->m_qwStart = 0;
        ptctx->m_bUpload = bUpload;
        ptctx->m_strUserName = getlogin();
        ptctx->m_strPathName = strSrcFile;

        if( !bUpload )
        {
            CfgPtr pResp( true );
            ret = GetFileInfo(
                strFileName, pResp );
            if( ERROR( ret ) )
                break;

            CCfgOpener oResp(
                ( IConfigDb* )pResp );

            ret = oResp.GetIntProp(
                SF_REQSIZE_IDX,
                ptctx->m_dwReqSize );
            if( ERROR( ret ) )
                break;

            if( ptctx->m_dwReqSize >
                MAX_BYTES_PER_FILE )
            {
                ret = -EFBIG;
                break;
            }
        }
        else
        {
            struct stat sBuf;
            ret = stat( strSrcFile.c_str(), &sBuf );
            if( unlikely( ret == -1 ) )
            {
                ret = -errno;
                break;
            }
            if( unlikely( sBuf.st_size == 0 ) )
            {
                ret = 0;
                break;
            }
            ptctx->m_dwReqSize =
                ( guint32 )sBuf.st_size;
        }

        TaskletPtr pTask;
        ret = DEFER_IFCALL_NOSCHED(
            pTask, ObjPtr( this ),
            &CMyFileProxy::StartTransfer,
            ptctx );

        if( ERROR( ret ) )
            break;
        // we don't use OnStart_Loop because there
        // is no way to pass in the parameters of
        // the UploadFile. Use a start task
        // instead.
        ret = StartLoop( pTask );

    }while( 0 );

    return ret;
}

gint32 CMyFileProxy::UploadFile(
    const std::string& strSrcFile )
{
    return UploadDownload( strSrcFile, true );
}

gint32 CMyFileProxy::DownloadFile(
    const std::string& strSrcFile )
{
    return UploadDownload( strSrcFile, false );
}

template<>
gint32 GetIidOfType< const CMyFileProxy >(
    std::vector< guint32 >& vecIids, const CMyFileProxy* pType )
{
    vecIids.push_back( pType->CMyFileProxy::super::GetIid() );
    vecIids.push_back( pType->CMyFileProxy::GetIid() );
    return 0;
}

