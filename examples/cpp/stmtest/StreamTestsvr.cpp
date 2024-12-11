/****BACKUP YOUR CODE BEFORE RUNNING RIDLC***/
// Implement the following methods
// to get the RPC proxy/server work
#include "rpc.h"
using namespace rpcf;
#include "stmtest.h"
#include "StreamTestsvr.h"

// IStreamTest Server
/* Sync Req */
gint32 CStreamTest_SvrImpl::Echo(
    const std::string& i0 /*[ In ]*/,
    std::string& i0r /*[ Out ]*/ )
{
    i0r = i0;
    return STATUS_SUCCESS;
}

#define SET_CHANCTX( hChannel_, ptc_ ) \
({ \
    CfgPtr pCtx; \
    gint32 iRet = this->GetContext(hChannel_, pCtx); \
    if( SUCCEEDED( iRet ) ) \
    { \
        CCfgOpener oCtx( ( IConfigDb* )pCtx ); \
        if( ptc_ != nullptr ) \
            iRet = oCtx.SetIntPtr( \
                propChanCtx, ( guint32* )ptc_ ); \
        else\
            iRet = oCtx.RemoveProperty( propChanCtx );\
    }\
    iRet;\
})

#define GET_CHANCTX( hChannel_, ptr_ ) \
({ \
    guint32* ptc = nullptr; \
    CfgPtr pCtx; \
    ret = ( ptr_ )->GetContext(hChannel_, pCtx); \
    if( SUCCEEDED( ret ) ) \
    { \
        CCfgOpener oCtx( ( IConfigDb* )pCtx ); \
        ret = oCtx.GetIntPtr( propChanCtx, ptc ); \
        if( ERROR( ret ) ) \
            ptc = nullptr; \
    };\
    reinterpret_cast< TransferContext* >( ptc );\
})

gint32 CStreamTest_SvrImpl::OnReadStreamComplete(
    HANDLE hChannel, gint32 iRet,
    BufPtr& pBuf, IConfigDb* pCtx )
{
    if( ERROR( iRet ) )
    {
        OutputMsg( iRet,
            "ReadStreamAsync failed with error " );
        return 0;
    }

    stdstr strBuf = BUF2STR( pBuf );
    OutputMsg( iRet, "Proxy says %s",
        strBuf.c_str() );
    this->WriteAndReceive( hChannel );

    return 0;
}

gint32 CStreamTest_SvrImpl::OnWriteStreamComplete(
    HANDLE hChannel, gint32 iRet,
    BufPtr& pBuf, IConfigDb* pCtx )
{
    gint32 ret = 0;
    do{
        if( ERROR( iRet ) )
        {
            OutputMsg( iRet,
                "WriteStreamAsync failed with error " );
            break;
        }

        TransferContext* ptc =
            GET_CHANCTX( hChannel, this );
        if( ERROR( ret  ) ) 
            break;

        ptc->IncCounter();
        this->ReadAndReply( hChannel );

    }while( 0 );

    return 0;
}

gint32 CStreamTest_SvrImpl::OnStreamReady(
    HANDLE hChannel )
{
    gint32 ret = 0;
    do{
        OutputMsg( ret, "Haha" );
        stdstr strGreeting = "Hello, Proxy!";
        BufPtr pBuf( true );
        *pBuf = strGreeting;
        ret = this->WriteStreamNoWait(
            hChannel, pBuf );
        if( ERROR( ret ) )
            break;
        TransferContext* ptc =
            new TransferContext();

        ret = SET_CHANCTX( hChannel, ptc );
        if( ERROR( ret ) )
            break;
        // ret = ReadAndReply( hChannel );
        ret = RunReadWriteTasks( hChannel );

    }while( 0 );
    // error will cause the stream to close
    return ret;
}

gint32 CStreamTest_SvrImpl::OnStmClosing(
    HANDLE hChannel )
{
    gint32 ret = 0;
    do{
        CStdRMutex oIfLock( this->GetLock() );
        TransferContext* ptc =
            GET_CHANCTX( hChannel, this );
        if( ERROR( ret ) )
            break;

        OutputMsg( ptc->GetError(),
            "Stream 0x%x is closing with status ",
            hChannel );
        delete ptc;
        SET_CHANCTX( hChannel, nullptr );

    }while( 0 );
    return super::OnStmClosing( hChannel );
}

gint32 CStreamTest_SvrImpl::ReadAndReply(
    HANDLE hChannel )
{
    gint32 ret = 0;
    do{
        TransferContext* ptc =
            GET_CHANCTX( hChannel, this );
        if( ERROR( ret ) )
            break;

        while( true )
        {
            BufPtr pBuf;
            ret = this->ReadStreamAsync(
                hChannel, pBuf,
                ( IConfigDb* )nullptr ); 
            if( ERROR( ret ) )
                break;
            if( ret == STATUS_PENDING )
            {
                ret = 0;
                break;
            }

            stdstr strBuf = BUF2STR( pBuf );
            OutputMsg( ret, "Proxy says: %s",
                strBuf.c_str() );

            gint32 idx = ptc->GetCounter();
            stdstr strMsg = "This is message ";
            strMsg += std::to_string( idx );
            pBuf.NewObj();
            *pBuf = strMsg;
            IConfigDb* pCtx = nullptr;
            ret = this->WriteStreamAsync(
                hChannel, pBuf, pCtx );
            if( ERROR( ret ) )
                break;

            if( ret == STATUS_PENDING )
            {
                ret = 0;
                break;
            }
            ptc->IncCounter();
        }
        if( ERROR( ret ) )
        {
            ptc->SetError( ret );
        }
    }while( 0 );
    if( ERROR( ret ) )
        OutputMsg( ret,
            "ReadAndReply failed with error " );
    return ret;
}

gint32 CStreamTest_SvrImpl::WriteAndReceive(
    HANDLE hChannel )
{
    gint32 ret = 0;
    do{
        TransferContext* ptc =
            GET_CHANCTX( hChannel, this  );
        if( ERROR( ret ) )
            break;

        while( true )
        {
            gint32 idx = ptc->GetCounter();
            stdstr strMsg = "This is message ";
            strMsg += std::to_string( idx );
            BufPtr pBuf( true );
            *pBuf = strMsg;
            ret = this->WriteStreamAsync(
                hChannel, pBuf, 
                ( IConfigDb* )nullptr );
            if( ERROR( ret ) )
                break;

            if( ret == STATUS_PENDING )
            {
                ret = 0;
                break;
            }
            ptc->IncCounter();
            pBuf.Clear();
            ret = this->ReadStreamAsync(
                hChannel, pBuf,
                ( IConfigDb* )nullptr );
            if( ERROR( ret ) )
                break;

            if( ret == STATUS_PENDING )
            {
                ret = 0;
                break;
            }
            stdstr strBuf = BUF2STR( pBuf );
            OutputMsg( ret, "Proxy says: %s",
                strBuf.c_str() );
        }
        if( ERROR( ret ) )
            ptc->SetError( ret );

    }while( 0 );

    if( ERROR( ret ) )
        OutputMsg( ret,
            "ReadAndReply failed with error " );
    return ret;
}


gint32 CStreamTest_SvrImpl::BuildAsyncTask(
    HANDLE hChannel, bool bRead, TaskletPtr& pTask )
{
    if( hChannel == INVALID_HANDLE )
        return -EINVAL;
    gint32 ret = 0;
    do{
        using CompType = gint32 ( * )( IEventSink*,
            CStreamTest_SvrImpl*, HANDLE, bool );
        CompType asyncComp = ([]( IEventSink* pCb,
            CStreamTest_SvrImpl* pIf,
            HANDLE hChannel, bool bRead )->gint32
        {
            gint32 ret = 0;
            gint32 iRet = 0;
            if( pIf == nullptr ||
                pCb == nullptr ||
                hChannel == INVALID_HANDLE )
                return -EINVAL;
            do{
                CCfgOpenerObj oTaskCfg( pCb );
                IConfigDb* pResp;

                ret = oTaskCfg.GetPointer(
                    propRespPtr, pResp );
                if( ERROR( ret ) )
                    break;
                CCfgOpener oResp(
                    ( IConfigDb*)pResp );
                ret = oResp.GetIntProp(
                    propReturnValue, ( guint32& )iRet );
                if( ERROR( ret ) )
                    break;
                if( ERROR( iRet ) )
                {
                    ret = iRet;
                    break;
                }
                if( bRead )
                {    
                    BufPtr pBuf;
                    ret = oResp.GetBufPtr( 0, pBuf );
                    if( ERROR( ret ) )
                        break;

                    stdstr strBuf = BUF2STR( pBuf );
                    OutputMsg( ret, "Proxy says: %s",
                        strBuf.c_str() );
                }
                else
                {
                    {
                        CStdRMutex oIfLock( pIf->GetLock() );
                        TransferContext* ptc =
                            GET_CHANCTX( hChannel, pIf );
                        if( ERROR( ret ) )
                        {
                            OutputMsg( ret,
                                "Cannot find stream 2@0x%llx",
                                hChannel );
                            break;
                        }
                        ptc->IncCounter();
                    }
                    ret = pIf->RunReadWriteTasks(
                        hChannel );
                }

            }while( 0 );

            Variant oVar;
            iRet = pCb->GetProperty( 0x12345, oVar );
            if( SUCCEEDED( iRet ) )
            {
                CIfRetryTask* pTask =
                    ( ObjPtr& )oVar;

                pTask->OnEvent( eventTaskComp,
                    0, 0, nullptr );
                pCb->RemoveProperty( 0x12345 );
            }
            return ret;
        });
        
        gint32 ( *rwFunc )( IEventSink* pCb,
            CStreamTest_SvrImpl*, HANDLE, bool,
            intptr_t ) =
        ([]( IEventSink* pCb,
            CStreamTest_SvrImpl* pIf,
            HANDLE hChannel,
            bool bRead,
            intptr_t compFunc )->gint32
        {
            gint32 ret = 0;
            if( pIf == nullptr ||
                hChannel == INVALID_HANDLE ||
                compFunc == 0 )
                return -EINVAL;
            do{
                BufPtr pBuf;
                auto asyncComp = ( CompType )compFunc;
                TaskletPtr pCompTask;
                ret = NEW_COMPLETE_FUNCALL(
                    0, pCompTask, pIf->GetIoMgr(),
                    asyncComp, nullptr, pIf, hChannel,
                    bRead );
                if( ERROR( ret ) )
                    break;

                Variant oVar = ObjPtr( pCb );
                pCompTask->SetProperty( 0x12345, oVar );

                if( bRead )
                {
                    ret = pIf->ReadStreamAsync( 
                        hChannel, pBuf, pCompTask );
                }
                else
                {
                    CStdRMutex oIfLock( pIf->GetLock() );
                    TransferContext* ptc =
                        GET_CHANCTX( hChannel, pIf );
                    if( ERROR( ret ) )
                    {
                        OutputMsg( ret,
                            "Cannot find stream @0x%llx",
                            hChannel );
                    }
                    else
                    {
                        gint32 idx = ptc->GetCounter();
                        oIfLock.Unlock();
                        stdstr strMsg = "This is message ";
                        strMsg += std::to_string( idx );
                        pBuf.NewObj();
                        *pBuf = strMsg;
                        ret = pIf->WriteStreamAsync(
                            hChannel, pBuf, pCompTask );
                    }
                }

                if( ret == STATUS_PENDING )
                    break;

                pCompTask->RemoveProperty( 0x12345 );
                CParamList oResp;
                oResp.SetIntProp(
                    propReturnValue, ret );

                if( SUCCEEDED( ret ) )
                    oResp.Push( pBuf );

                oVar = ObjPtr( oResp.GetCfg() );

                pCompTask->SetProperty(
                    propRespPtr, oVar );

                ret = asyncComp( pCompTask,
                    pIf, hChannel, bRead );
                ( *pCompTask )( eventCancelTask );

            }while( 0 );
            return ret;
        });

        TaskletPtr pRWTask;
        ret = NEW_FUNCCALL_TASKEX2( 0, pRWTask,
            this->GetIoMgr(), rwFunc, nullptr,
            this, hChannel, bRead,
            ( intptr_t )asyncComp );
        if( ERROR( ret ) )
            break;

        pTask = pRWTask;

    }while( 0 );

    return ret;
}

gint32 CStreamTest_SvrImpl::RunReadWriteTasks(
    HANDLE hChannel )
{
    gint32 ret = 0;
    do{
        TaskGrpPtr pGrp;
        CParamList oGrpCfg;
        oGrpCfg[ propIfPtr ] = ObjPtr( this );
        ret = pGrp.NewObj(
            clsid( CIfTaskGroup ),
            oGrpCfg.GetCfg() );
        if( ERROR( ret ) )
            break;
        pGrp->SetRelation( logicAND );

        TaskletPtr pTask;
        ret = BuildAsyncTask( hChannel, true, pTask );
        if( ERROR( ret ) )
            break;
        pGrp->AppendTask( pTask );
        ret = BuildAsyncTask( hChannel, false, pTask );
        if( ERROR( ret ) )
            break;
        pGrp->AppendTask( pTask );
        TaskletPtr pGrpTask = pGrp;
        InterfPtr pUxIf;
        ret = this->GetUxStream(
            hChannel, pUxIf );
        if( ERROR( ret ) )
            break;
        CRpcServices* pSvc = pUxIf;
        ret = pSvc->RunManagedTask2( pGrpTask );
        // GetIoMgr()->RescheduleTask( pGrpTask );
    }while( 0 );
    return ret;
}        
