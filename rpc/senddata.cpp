/*
 * =====================================================================================
 *
 *       Filename:  senddata.cpp
 *
 *    Description:  SendData/FetchData related implementations for RPC 
 *        Version:  1.0
 *        Created:  04/16/2018 08:12:27 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi ( woodhead99@gmail.com ), 
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

#include "configdb.h"
#include "defines.h"
#include "proxy.h"
#include "rpcroute.h"
#include "dbusport.h"
#include "tcpport.h"
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

gint32 CBdgeProxyOpenStreamTask::RunTask() 
{
    gint32 ret = 0;

    try{
        CParamList oParams( 
            ( IConfigDb* )GetConfig() );

        CRpcTcpBridgeProxy* pIf = nullptr;
        pIf = oParams[ propIfPtr ];

        gint32 iStmId = -1;
        ret = pIf->OpenStream(
            protoStream, iStmId, this );

    }
    catch( std::runtime_error& e )
    {
        ret = -ENOENT;
    }

    if( ret == STATUS_PENDING ) 
        return ret;

    if( Retriable( ret ) )
        return STATUS_MORE_PROCESS_NEEDED;

    OnTaskComplete( ret );

    return ret;
}

gint32 CBdgeProxyOpenStreamTask::OnTaskComplete(
    gint32 iRetVal )
{
    gint32 ret = 0;

    EventPtr pEvent;
    CCfgOpener oTaskCfg( ( IConfigDb* )*this );
    do{
        ObjPtr pObj;

        ret = oTaskCfg.GetObjPtr( propIfPtr, pObj );
        if( ERROR( ret ) )
            break;

        CRpcTcpBridgeProxy* pIfProxy = pObj;
        if( pIfProxy == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        TaskletPtr pCallerTask;
        ret = GetCallerTask( pCallerTask );
        if( ERROR( ret ) )
            break;

        // grab the response from caller
        CCfgOpenerObj oCallerCfg(
            ( CObjBase* )pCallerTask );

        ret = oCallerCfg.GetObjPtr(
            propRespPtr, pObj );
        if( ERROR( ret ) )
        {
            // an immediate return, almost immpossible
            ret = oTaskCfg.GetObjPtr(
                propRespPtr, pObj );

            if( ERROR( ret ) && SUCCEEDED( iRetVal ) )
                iRetVal = ret;
        }

        ret = GetInterceptTask( pEvent );
        if( ERROR( ret ) )
            break;

        if( ERROR( iRetVal ) )
        {
            ret = iRetVal;
            break;
        }
        
        // continue to send out the SEND_DATA request
        gint32 iStmId = 0;
        CCfgOpener oCfg( ( IConfigDb* )pObj );

        iStmId = ( gint32& )oCfg[ 0 ];

        if( ERROR( ret ) )
            break;

        ret = EmitSendDataRequest( iStmId );

        if( ret == STATUS_PENDING )
            ret = 0;

    }while( 0 );

    if( ERROR( ret ) && !pEvent.IsEmpty() )
    {
        // complete the intercepted task with error 
        CParamList oParams;

        oParams[ propReturnValue ] = ret;
        oTaskCfg[ propRespPtr ] =
            oParams.GetCfg();

        pEvent->OnEvent( eventTaskComp,
            ret, 0, ( LONGWORD* )this ); 
    }

    // remove the event sink
    oTaskCfg.RemoveProperty( propEventSink );

    if( true )
    {
        // clear the context
        CParamList oParams( m_pCtx );
        oParams.ClearParams();
        oParams.RemoveProperty( propReqPtr );
        oParams.RemoveProperty( propRespPtr );
    }

    // return non-pending status to finish this task
    return ret;
}

gint32 CBdgeProxyOpenStreamTask::
    EmitSendDataRequest( gint32 iStmId )
{
    gint32 ret = 0;
    if( iStmId < 0 || IsReserveStm( iStmId ) )
        return -EINVAL;

    do{
        CCfgOpener oTaskCfg( 
            ( IConfigDb* )GetConfig() );

        ObjPtr pObj;
        ret = oTaskCfg.GetObjPtr(
            propReqPtr, pObj );

        if( ERROR( ret ) )
            break;

        IConfigDb* pArgs = pObj;

        CParamList oStartParams;
        oStartParams.SetObjPtr(
            propReqPtr, pArgs );

        oStartParams.CopyProp(
            propIoMgr, pArgs );

        oStartParams.CopyProp(
            propIfPtr, pArgs );

        oStartParams.CopyProp(
            propEventSink, pArgs );

        oStartParams.SetIntProp(
            propStreamId, iStmId );

        CIoManager* pMgr = nullptr;
        ret = oTaskCfg.GetPointer(
            propIoMgr, pMgr );

        if( ERROR( ret ) )
            break;

        ret = pMgr->ScheduleTask(
            clsid( CBdgeProxyStartSendTask ),
            oStartParams.GetCfg() );

        if( SUCCEEDED( ret ) )
            ret = STATUS_PENDING;

    }while( 0 );

    return ret;
}

// CRpcTcpBridgeProxy side Send/Fetch task 
gint32 CBdgeProxyStartSendTask::RunTask()
{
    gint32 ret = 0;
    do{
        if( GetState() != statInit ) 
        {
            ret = ERROR_STATE;
            break;
        }

        SetState( statSendReq );
        CCfgOpener oTaskCfg(
            ( IConfigDb* )GetConfig() );

        ObjPtr pObj;
        ret = oTaskCfg.GetObjPtr(
            propReqPtr, pObj );

        if( ERROR( ret ) )
            break;

        CfgPtr pArgs = pObj;
        CParamList oSendArgs(
            ( IConfigDb* )pArgs );

        oSendArgs.Pop( m_dwCurSize );
        oSendArgs.Pop( m_dwCurOffset );
        oSendArgs.Pop( m_iFd );
        oSendArgs.Pop( pObj );
        m_dwSize = m_dwCurSize;
        m_pDataDesc = pObj;

        oTaskCfg.RemoveProperty( propReqPtr );

        ret = oTaskCfg.GetIntProp(
            propStreamId, ( guint32& )m_iStmId );

        if( ERROR( ret ) )
            break;

       ret = oTaskCfg.GetObjPtr(
            propEventSink, pObj ); 

       if( ERROR( ret ) )
           break;

        m_pCallback = pObj;
        oTaskCfg.RemoveProperty(
            propEventSink );

        ret = oTaskCfg.GetObjPtr(
            propIfPtr, pObj ); 

        if( ERROR( ret ) )
            break;

        CRpcTcpBridgeProxy* pProxy = pObj;
        if( pProxy == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        CCfgOpener oDesc(
            ( IConfigDb* )m_pDataDesc );

        string strMethod;
        strMethod = oDesc[ propMethodName ];
        oDesc[ propNonFd ] = ( guint32 )true;

        if( strMethod == SYS_METHOD_SENDDATA )
        {
            ret = pProxy->super::SendData_Proxy(
                m_pDataDesc, m_iStmId,
                m_dwCurOffset, m_dwCurSize,
                this );
        }
        else if( strMethod == SYS_METHOD_FETCHDATA )
        {
            ret = pProxy->super::FetchData_Proxy(
                m_pDataDesc, m_iStmId,
                m_dwCurOffset, m_dwCurSize,
                this );
        }
        else
        {
            ret = -EINVAL;
        }

    }while( 0 );

    if( ret == STATUS_PENDING )
    {
        SetState( statWaitNotify );
        return ret;
    }

    OnTaskComplete( ret );

    return ret;
}

gint32 CBdgeProxyStartSendTask::
    OnTaskCompleteWithError( gint32 iRetVal )
{
    if( !ERROR( iRetVal ) )
        return -EINVAL;

    CCfgOpener oCfg(
        ( IConfigDb* )GetConfig() );

    gint32 ret = 0;
    do{
        SetState( statComplete );

        CParamList oResp;
        oResp.SetIntProp(
            propReturnValue, iRetVal );

        oCfg.SetObjPtr( propRespPtr,
            ObjPtr( ( IConfigDb* )
                oResp.GetCfg() ) );

        EventPtr pEvent = m_pCallback;
        if( pEvent.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        pEvent->OnEvent( eventTaskComp,
            iRetVal, 0, ( LONGWORD* )this );

        CloseStream();

    }while( 0 );

    oCfg.RemoveProperty( propRespPtr );

    if( SUCCEEDED( iRetVal ) )
        return ret;

    return iRetVal;
}

gint32 CBdgeProxyStartSendTask::
    OnTaskComplete( gint32 iRetVal )
{

    if( ERROR( iRetVal ) )
    {
        OnTaskCompleteWithError( iRetVal );
        return iRetVal;
    }
    if( iRetVal == STATUS_PENDING )
        return iRetVal;

    gint32 ret = 0;

    CCfgOpener oCfg(
        ( IConfigDb* )GetConfig() );

    do{

        bool bSend = true;
        string strMethod = oCfg[ propMethodName ];
        if( strMethod == SYS_METHOD_FETCHDATA )
            bSend = false;

        CParamList oResp;

        TaskletPtr pCallerTask;
        ret = GetCallerTask( pCallerTask );
        if( ERROR( ret ) )
            break;

        // get the response
        CCfgOpenerObj oCallerCfg(
            ( CObjBase* )pCallerTask );

        ObjPtr pObj;
        ret = oCallerCfg.GetObjPtr(
            propRespPtr, pObj );

        if( ERROR( ret ) )
        {
            if( SUCCEEDED( iRetVal ) )
                iRetVal = ret;
            
            // if error, generate a response
            oResp.SetIntProp(
                propReturnValue, iRetVal );

            oCfg.SetObjPtr( propRespPtr,
                oResp.GetCfg() );
        }
        else
        {
            if( !bSend )
            {
                // correct the responses for fetched
                // data
                CCfgOpener oResp( ( IConfigDb* )pObj );
                CCfgOpener oErrorResp;
                guint32 dwRet = ( guint32 )oResp[ propReturnValue ];
                if( SUCCEEDED( ( gint32 ) dwRet ) )
                {
                    ObjPtr pDesc = oResp[ 0 ];
                    CCfgOpener oDesc( ( IConfigDb* )pDesc );

                    // replace the file path with the one we received
                    oDesc[ propFilePath1 ] = m_strFile;
                    oDesc.SwapProp( propFilePath,
                        propFilePath1 );

                    // set the fd to the local fd
                    oResp.SetIntProp( 1, ( guint32 )m_iFd );
                    // set the offset to zero
                    oResp.SetIntProp( 2, 0 );

                    oResp.RemoveProperty( propNonFd );
                    oDesc.RemoveProperty( propNonFd );

                    guint32 dwSize = oResp[ 3 ];
                    if( dwSize != m_dwSize )
                    {
                        ret = -EBADMSG;
                        oErrorResp[ propReturnValue ] = ret;
                        pObj = oErrorResp.GetCfg();
                    }
                }
            }
            oCfg.SetObjPtr( propRespPtr, pObj );
        }


        EventPtr pEvent = m_pCallback;
        if( pEvent.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        pEvent->OnEvent( eventTaskComp,
            iRetVal, 0, ( LONGWORD* )this );

        CloseStream();

    }while( 0 );

    oCfg.RemoveProperty( propRespPtr );
    oCfg.RemoveProperty( propReqPtr );
    oCfg.RemoveProperty( propEventSink );
    m_pDataDesc.Clear();
    m_pCallback.Clear();

    return iRetVal;
}

gint32 CBdgeProxyStartSendTask::OnNotify(
    LONGWORD dwEvent,
    LONGWORD dwParam1,
    LONGWORD dwParam2,
    LONGWORD* pData )
{
    gint32 ret = STATUS_PENDING;
    if( dwParam1 != eventProgress )
        return ret;

    // we only care about the first notification so far
    // for both SendData and FetchData
    if( dwParam2 != 0 )
        return ret;

    // progress 0 arrived
    if( GetState() != statWaitNotify )
    {
        ret = ERROR_STATE;
        OnTaskComplete( ret );
        return ret;
    }

    // now start send/fetch the content via the stream
    bool bFinished = false;

    CCfgOpener oCfg( ( IConfigDb* )m_pDataDesc );
    bool bSend = true;

    string strMethod = oCfg[ propMethodName ];

    if( strMethod == SYS_METHOD_FETCHDATA )
        bSend = false;

    do{
        if( bSend )
        {
            SetState( statWriteStream );
            ret = WriteStream( bFinished );
        }
        else
        {
            SetState( statReadStream );
            ret = ReadStream( bFinished );
        }

        if( ret == STATUS_PENDING )
            break;

    }while( SUCCEEDED( ret ) && !bFinished );

    if( ret == STATUS_PENDING )
        return ret; 

    if( !bFinished )
    {
        // error occurs
        SetState( statComplete );
    }
    else
     {
         SetState( statWaitResp );
         ret = STATUS_PENDING;
     }
    OnTaskComplete( ret );

    return ret;
}

gint32 CBdgeProxyStartSendTask::CloseStream()
{
    gint32 ret = 0;

    do{
        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );

        ObjPtr pObj;
        ret = oCfg.GetObjPtr(
            propIfPtr, pObj );
        if( ERROR( ret ) )
            break;

        CRpcTcpBridgeProxy* pIf = pObj;
        TaskletPtr pTask;

        ret = pTask.NewObj(
            clsid( CIfDummyTask ) );

        if( ERROR( ret ) )
            break;

        ret = pIf->CloseStream( m_iStmId,
            ObjPtr( pTask ) );

    }while( 0 );

    return ret;
}

gint32 CBdgeProxyStartSendTask::
    OnIrpComplete( PIRP pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    if( GetState() == statComplete ||
        GetState() == statWaitResp )
        return ERROR_STATE;

    gint32 ret = super::OnIrpComplete( pIrp );
    if( ret == STATUS_MORE_PROCESS_NEEDED ||
        ret == STATUS_PENDING )
        return ret;

    do{
        ret = pIrp->GetStatus();
        if( ERROR( ret ) )
            break;
        IrpCtxPtr& pCtx = pIrp->GetTopStack();

        if( pCtx->GetMajorCmd() != IRP_MJ_FUNC )
            break;

        bool bSend = false;
        if( pCtx->GetMinorCmd() != IRP_MN_WRITE )
            bSend = true;

        if( pCtx->GetMinorCmd() != IRP_MN_READ )
        {
            // unknown command
            ret = ERROR_FAIL;
            break;
        }

        bool bFinished = false;
        if( bSend  )
        {
            if( GetState() != statWriteStream )
            {
                // possibly the peer side may have
                // finished the senddata request with
                // error
                ret = ERROR_STATE;
                break;
            }
            ret = WriteStream( bFinished );
        }
        else 
        {
            if( GetState() != statReadStream )
            {
                // possibly the peer side may have
                // finished the senddata request with
                // error
                ret = ERROR_STATE;
                break;
            }
            ret = ReadStream( bFinished );
        }

    }while( 0 );

    if( ret != STATUS_PENDING )
    {
        if( SUCCEEDED( ret ) )
        {
            // we are still waiting for the SENDDATA or
            // FETCHDATA response from the server side.
            SetState( statWaitResp );
            ret = STATUS_PENDING;
        }
        else
        {
            SetState( statComplete );
            // error occured
            OnTaskComplete( ret );
        }
    }

    return ret;
}

gint32 CBdgeReadWriteStreamTask::
    ReadStream( bool bFinished )
{
    gint32 ret = 0;

    do{

        if( m_dwCurSize == 0 )
        {
            // transfer completed
            bFinished = true;
            break;
        }

        BufPtr pBuf( true );
        pBuf->Resize( MAX_BYTES_PER_TRANSFER );

        guint32 dwSize = MAX_BYTES_PER_TRANSFER ;
        if( dwSize > m_dwCurSize )
            dwSize = m_dwCurSize;

        CCfgOpener oTaskCfg(
            ( IConfigDb* )GetConfig() ); 

        CRpcTcpBridge* pServer =
            oTaskCfg[ propIfPtr ];

        CRpcTcpBridgeProxy* pProxy =
            oTaskCfg[ propIfPtr ];

        // this method could be called by the
        // bridge or the bridge proxy
        CRpcTcpBridgeShared* pShared = nullptr;
        if( pServer != nullptr )
        {
            pShared = pServer;
        }
        else if( pProxy != nullptr )
        {
            pShared = pProxy;
        }
        else
        {
            ret = -EFAULT;
        }

        ret = pShared->ReadStream(
            m_iStmId, pBuf, dwSize, this );

        if( ERROR( ret ) )
            break;

        if( ret == STATUS_PENDING )
            break;

        dwSize = pBuf->size();
        m_dwCurSize -= dwSize;
        m_dwCurOffset += dwSize;

        if( SUCCEEDED( ret ) )
        {
            if( m_iFd < 0 )
            {
                ret = OpenTempFile();
                if( ERROR( ret ) )
                    break;
            }
            ret = write( m_iFd, pBuf->ptr(), dwSize );
            if( ret == -1 )
            {
                ret = -errno;
                break;
            }
            if( ret != ( gint32 )dwSize )
            {
                ret = -errno;
                break;
            }
        }

    }while( 0 );

    return ret;
}

gint32 CBdgeReadWriteStreamTask::
    WriteStream( bool bFinished )
{
    gint32 ret = 0;
    do{
        if( m_iFd < 0 )
        {
            CCfgOpener oCfg(
                ( IConfigDb* )m_pDataDesc );

            string strPath;
            strPath = oCfg[ propFilePath ];
            if( ERROR( ret ) )
                break;

            ret = open( strPath.c_str(),
                O_RDONLY );

            if( ret == -1 )
            {
                ret = -errno;
                break;
            }
            m_iFd = ret;
        }
        if( m_dwSize == m_dwCurSize )
        {
            ret = lseek( m_iFd,
                m_dwCurOffset, SEEK_SET );

            if( ret == -1 )
            {
                ret = -errno;
                break;
            }
        }

        if( m_dwCurSize == 0 )
        {
            // transfer completed
            bFinished = true;
            break;
        }

        BufPtr pBuf( true );
        pBuf->Resize( MAX_BYTES_PER_TRANSFER );

        guint32 dwSize = MAX_BYTES_PER_TRANSFER ;
        if( dwSize > m_dwCurSize )
            dwSize = m_dwCurSize;

        ret = read( m_iFd, pBuf->ptr(), dwSize );
        if( ret == -1 )
        {
            ret = -errno;
            break;
        }
        if( ret != ( gint32 )dwSize )
        {
            ret = ERROR_FAIL;
            break;
        }

        CCfgOpener oTaskCfg(
            ( IConfigDb* )GetConfig() ); 

        ObjPtr pObj =
            ( ObjPtr& )oTaskCfg[ propIfPtr ];

        CRpcTcpBridgeProxy* pProxy = pObj;
        CRpcTcpBridge* pServer = pObj;
        CRpcTcpBridgeShared* pShared = nullptr;
        if( pProxy != nullptr )
        {
            pShared = pProxy;
        }
        else if( pServer != nullptr )
        {
            pShared = pServer;
        }

        ret = pShared->WriteStream(
            m_iStmId, pBuf, dwSize, this );

        m_dwCurSize -= dwSize;
        m_dwCurOffset += dwSize;
        // we don't set the finished flag here if
        // m_dwCurSize == 0 because the last datablock is
        // still on the way

    }while( 0 );

    if( ret == STATUS_PENDING )
        return ret;

    return ret;
}

gint32 CBdgeReadWriteStreamTask::OpenTempFile()
{
    gint32 ret = 0;
    do{
        char szFile[ 32 ] = TMP_FILE_PREFIX"XXXXXX";
        ret = mkstemp( szFile );
        if( ret == -1 )
        {
            ret = -errno;
            break;
        }

        m_strFile = szFile;
        m_bTempFile = true;
        m_iFd = ret;

        ret = 0;

    }while( 0 );

    return ret;
}

gint32 CBdgeStartRecvDataTask::RunTask()
{
    gint32 ret = 0;
    do{
        SetState( statReadStream );
        CCfgOpener oTaskCfg(
            ( IConfigDb* )GetConfig() );

        ObjPtr pObj;
        ret = oTaskCfg.GetObjPtr(
            propReqPtr, pObj );

        if( ERROR( ret ) )
            break;

        CfgPtr pArgs = pObj;
        CParamList oSendArgs(
            ( IConfigDb* )pArgs );

        oSendArgs.Pop( m_dwSize );
        oSendArgs.Pop( m_dwCurOffset );
        oSendArgs.Pop( m_iStmId );
        oSendArgs.Pop( pObj );

        m_dwCurSize = m_dwSize;
        m_pDataDesc = pObj;
        oTaskCfg.RemoveProperty( propReqPtr );

        m_qwTaskId = oTaskCfg[ propTaskId ];

        if( ERROR( ret ) )
            break;

        ret = oTaskCfg.GetObjPtr(
            propEventSink, pObj ); 

        if( ERROR( ret ) )
           break;

        m_pCallback = pObj;
        bool bFinished = false;

        while( SUCCEEDED( ret ) && !bFinished )
        {
            ret = ReadStream( bFinished );
        }

        if( SUCCEEDED( ret ) ||
            ret == STATUS_PENDING )
        {
            SendProgressNotify();
        }

    }while( 0 );

    if( ret == STATUS_PENDING )
        return ret;

    // this request should not complete immediately
    OnTaskComplete( ret );

    return ret;

}

gint32 CBdgeStartRecvDataTask::
    SendProgressNotify()
{
    gint32 ret = 0;
    if( GetState() != statReadStream )
        return ERROR_STATE;

    do{
        CCfgOpener oTaskCfg(
            ( IConfigDb* )GetConfig() ); 

        CRpcTcpBridge* pIf =
            ( ObjPtr& )oTaskCfg[ propIfPtr ];

        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ret = pIf->OnProgressNotify(
            ++m_dwProgress,
            m_qwTaskId );

    }while( 0 );

    return ret;
}

gint32 CBdgeStartRecvDataTask::
    OnTaskComplete( gint32 iRetVal )
{
    if( iRetVal == STATUS_PENDING )
        return iRetVal;

    gint32 ret = 0;

    do{
        SetState( statComplete );
        CParamList oResp;

        CCfgOpener oTaskCfg(
            ( IConfigDb* )GetConfig() ); 

        CRpcTcpBridge* pIf =
            ( ObjPtr& )oTaskCfg[ propIfPtr ];

        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ObjPtr pResp;
        if( SUCCEEDED( iRetVal ) )
        {
            pResp = ( ObjPtr& )
                oTaskCfg[ propRespPtr ];

            if( pResp.IsEmpty() )
                ret = -EBADMSG;
        }

        if( pResp.IsEmpty() )
        {
            oResp[ propReturnValue ] = iRetVal;
            pResp = oResp.GetCfg();
        }

        // the caller task has set the resp property
        // for this task, so we can get the resp from
        // this task

        pIf->OnServiceComplete( 
            pResp, m_pCallback ); 

        // CloseStream ? 

    }while( 0 );

    RemoveProperty( propRespPtr );
    RemoveProperty( propReqPtr );
    m_pDataDesc.Clear();
    m_pCallback.Clear();

    return ret;
}

gint32 CBdgeStartRecvDataTask::
    OnIrpComplete( PIRP pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = super::OnIrpComplete( pIrp );

    // in case EAGAIN returned
    if( ret == STATUS_MORE_PROCESS_NEEDED )
    {
        // not support retry yet since the state has
        // changed a lot
        return ERROR_FAIL;
    }

    do{
        if( GetState() != statReadStream )
        {
            ret = ERROR_STATE;
            break;
        }

        SendProgressNotify();

        ret = pIrp->GetStatus();
        if( ERROR( ret ) )
            break;
        IrpCtxPtr& pCtx = pIrp->GetTopStack();

        if( pCtx->GetMajorCmd() != IRP_MJ_FUNC )
            break;

        if( pCtx->GetMinorCmd() != IRP_MN_READ )
            break;

        bool bFinished = false;
        ret = ReadStream( bFinished );

    }while( 0 );

    if( ret == STATUS_PENDING )
        return ret;

    // relay the senddata request to the proxy
    if( SUCCEEDED( ret ) )
        ret = RelaySendData();

    if( ERROR( ret ) )
        OnTaskComplete( ret );

    return ret;
}

gint32 CBdgeStartRecvDataTask::RelaySendData()
{
    gint32 ret = 0;
    do{
        SetState( statRelayReq );
        CCfgOpener oTaskCfg(
            ( IConfigDb* )GetConfig() ); 

        CRpcTcpBridge* pIf =
            ( ObjPtr& )oTaskCfg[ propIfPtr ];

        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        CCfgOpener oDataDesc(
            ( IConfigDb* )m_pDataDesc );

        // replace the file path with the one we received
        oDataDesc[ propFilePath1 ] = m_strFile;
        oDataDesc.SwapProp(
            propFilePath, propFilePath1 );

        // remove the propNonFd we have placed in
        // datadesc in the proxy side
        oDataDesc.RemoveProperty( propNonFd );
        ret = pIf->super::SendData_Server(
           m_pDataDesc, m_iFd, 0, m_dwSize, this );

    }while( 0 );

    return ret;
}

// --- CBdgeStartFetchDataTask ---
gint32 CBdgeStartFetchDataTask ::RunTask()
{
    gint32 ret = 0;
    do{
        SetState( statRelayReq );
        CCfgOpener oTaskCfg(
            ( IConfigDb* )GetConfig() );

        ObjPtr pObj;
        ret = oTaskCfg.GetObjPtr(
            propReqPtr, pObj );

        if( ERROR( ret ) )
            break;

        CfgPtr pArgs = pObj;
        CParamList oSendArgs(
            ( IConfigDb* )pArgs );

        oSendArgs.Pop( m_dwSize );
        oSendArgs.Pop( m_dwOffset );
        oSendArgs.Pop( m_iStmId );
        oSendArgs.Pop( pObj );
        m_dwCurOffset = m_dwOffset;

        m_dwCurSize = m_dwSize;
        m_pDataDesc = pObj;
        oTaskCfg.RemoveProperty( propReqPtr );

        m_qwTaskId = oTaskCfg[ propTaskId ];

        if( ERROR( ret ) )
            break;

        ret = oTaskCfg.GetObjPtr(
            propEventSink, pObj ); 

        if( ERROR( ret ) )
           break;

        m_pCallback = pObj;

        ret = RelayFetchData();

        if( SUCCEEDED( ret ) ||
            ret == STATUS_PENDING )
        {
            SendProgressNotify();
        }

    }while( 0 );

    if( ret == STATUS_PENDING )
        return ret;

    // this request should not complete immediately
    if( SUCCEEDED( ret ) )
        ret = ERROR_FAIL;

    OnTaskComplete( ret );

    return ret;
}

gint32 CBdgeStartFetchDataTask::
    SendProgressNotify()
{
    gint32 ret = 0;
    if( GetState() != statRelayReq )
        return ERROR_STATE;

    do{
        CCfgOpener oTaskCfg(
            ( IConfigDb* )GetConfig() ); 

        CRpcTcpBridge* pIf =
            ( ObjPtr& )oTaskCfg[ propIfPtr ];

        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ret = pIf->OnProgressNotify(
            ++m_dwProgress,
            m_qwTaskId );

    }while( 0 );

    return ret;
}

gint32 CBdgeStartFetchDataTask::
    OnTaskComplete( gint32 iRetVal )
{
    if( iRetVal == STATUS_PENDING )
        return iRetVal;

    gint32 ret = 0;

    do{
        CCfgOpener oTaskCfg(
            ( IConfigDb* )GetConfig() ); 

        CRpcTcpBridge* pIf =
            ( ObjPtr& )oTaskCfg[ propIfPtr ];

        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        EnumTaskStat iTaskState = GetState();
        if( iTaskState == statRelayReq &&
            SUCCEEDED( iRetVal ) )
        {
            // not finished yet
            if( !oTaskCfg.exist( propRespPtr ) )
            {
                ret = ERROR_FAIL;
                break;
            }

            IConfigDb* pResp = oTaskCfg[ propRespPtr ];
            if( pResp == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            CParamList oParams( pResp );
            m_pDataDesc = ( ObjPtr& )oParams[ 0 ];

            m_iFd = oParams[ 1 ];

            m_dwOffset = m_dwCurOffset =
                oParams[ 2 ];

            m_dwSize = oParams[ 3 ];
            m_dwCurSize = m_dwSize;

            ret = pIf->OnProgressNotify(
                ++m_dwProgress,
                m_qwTaskId );

            SetState( statWriteStream );

            bool bFinished = false;
            ret = WriteStream( bFinished );
            if( ret == STATUS_PENDING )
                return ret;
        }
        else if( iTaskState != statWriteStream &&
            iTaskState != statRelayReq )
        {
            // don't go further
            return ERROR_STATE;
        }

        SetState( statComplete );

        CParamList oResp;
        ObjPtr pResp;
        if( SUCCEEDED( iRetVal ) )
        {
            oResp[ propReturnValue ] = iRetVal;
            oResp.Push( ObjPtr( m_pDataDesc ) );
            // no fd to send across the internet
            oResp.Push( -1 );
            oResp.Push( m_dwOffset );
            oResp.Push( m_dwSize );
            pResp = oResp.GetCfg();
        }

        if( ERROR( iRetVal )   )
        {
            oResp[ propReturnValue ] = iRetVal;
            pResp = oResp.GetCfg();
        }
        else if( pResp.IsEmpty() )
        {
            oResp[ propReturnValue ] = -EFAULT;
            pResp = oResp.GetCfg();
        }

        // the caller task has set the resp property
        // for this task, so we can get the resp from
        // this task

        pIf->OnServiceComplete( 
            pResp, m_pCallback ); 

        // CloseStream ? 

    }while( 0 );

    RemoveProperty( propRespPtr );
    RemoveProperty( propReqPtr );
    m_pDataDesc.Clear();
    m_pCallback.Clear();

    return ret;
}

gint32 CBdgeStartFetchDataTask::
    OnIrpComplete( PIRP pIrp )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    gint32 ret = super::OnIrpComplete( pIrp );

    // in case EAGAIN returned
    if( ret == STATUS_MORE_PROCESS_NEEDED )
    {
        // not support retry yet since the state has
        // changed a lot
        return ERROR_FAIL;
    }

    do{
        if( GetState() != statWriteStream )
        {
            ret = ERROR_STATE;
            break;
        }

        SendProgressNotify();

        ret = pIrp->GetStatus();
        if( ERROR( ret ) )
            break;
        IrpCtxPtr& pCtx = pIrp->GetTopStack();

        if( pCtx->GetMajorCmd() != IRP_MJ_FUNC )
            break;

        if( pCtx->GetMinorCmd() != IRP_MN_WRITE )
            break;

        bool bFinished = false;
        ret = WriteStream( bFinished );

    }while( 0 );

    if( ret == STATUS_PENDING )
        return ret;

    OnTaskComplete( ret );
    return ret;
}

gint32 CBdgeStartFetchDataTask::RelayFetchData()
{
    gint32 ret = 0;
    do{
        SetState( statRelayReq );
        CCfgOpener oTaskCfg(
            ( IConfigDb* )GetConfig() ); 

        CRpcTcpBridge* pIf =
            ( ObjPtr& )oTaskCfg[ propIfPtr ];

        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        CCfgOpener oDataDesc(
            ( IConfigDb* )m_pDataDesc );

        oDataDesc.RemoveProperty( propNonFd );

        ret = pIf->super::FetchData_Server(
            m_pDataDesc, m_iFd, m_dwOffset,
            m_dwSize, this );

    }while( 0 );

    return ret;
}

