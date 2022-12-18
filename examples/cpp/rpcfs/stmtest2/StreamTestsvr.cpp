/****BACKUP YOUR CODE BEFORE RUNNING RIDLC***/
// Implement the following methods
// to get the RPC proxy/server work
#include "rpc.h"
using namespace rpcf;
#include "stmport.h"
#include "fastrpc.h"
#include "stmtest.h"
#include "StreamTestsvr.h"

/* Sync Req Handler*/
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

#define GET_CHANCTX( hChannel_ ) \
({ \
    guint32* ptc = nullptr; \
    CfgPtr pCtx; \
    ret = this->GetContext(hChannel_, pCtx); \
    if( SUCCEEDED( ret ) ) \
    { \
        CCfgOpener oCtx( ( IConfigDb* )pCtx ); \
        ret = oCtx.GetIntPtr( propChanCtx, ptc ); \
        if( ERROR( ret ) ) \
            ptc = nullptr; \
    };\
    ( TransferContext* )ptc;\
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

        TransferContext* ptc = GET_CHANCTX( hChannel );
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
        ReadAndReply( hChannel );

    }while( 0 );
    // error will cause the stream to close
    return ret;
}

gint32 CStreamTest_SvrImpl::OnStmClosing(
    HANDLE hChannel )
{
    gint32 ret = 0;
    do{
        TransferContext* ptc =
            GET_CHANCTX( hChannel );
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
            GET_CHANCTX( hChannel );
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
            GET_CHANCTX( hChannel );
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

gint32 CStreamTest_SvrImpl::CreateStmSkel(
    HANDLE hStream, guint32 dwPortId, InterfPtr& pIf )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg;
        auto pMgr = this->GetIoMgr();
        oCfg[ propIoMgr ] = ObjPtr( pMgr );
        oCfg[ propIsServer ] = true;
        oCfg.SetIntPtr( propStmHandle,
            ( guint32*& )hStream );
        oCfg.SetPointer( propParentPtr, this );
        ret = CRpcServices::LoadObjDesc(
            "./stmtestdesc.json",
            "StreamTest_SvrSkel",
            true, oCfg.GetCfg() );
        if( ERROR( ret ) )
            break;
        oCfg.CopyProp( propSkelCtx, this );
        oCfg[ propPortId ] = dwPortId;
        ret = pIf.NewObj(
            clsid( CStreamTest_SvrSkel ),
            oCfg.GetCfg() );
    }while( 0 );
    return ret;
}
gint32 CStreamTest_SvrImpl::OnPreStart(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCtx;
        CCfgOpenerObj oIfCfg( this );
        oCtx[ propClsid ] = clsid( 
            CStreamTest_ChannelSvr );
        oCtx.CopyProp( propObjDescPath, this );
        stdstr strInstName;
        ret = oIfCfg.GetStrProp(
            propObjName, strInstName );
        if( ERROR( ret ) )
            break;
        oCtx[ 1 ] = strInstName;
        guint32 dwHash = 0;
        ret = GenStrHash( strInstName, dwHash );
        if( ERROR( ret ) )
            break;
        char szBuf[ 16 ];
        sprintf( szBuf, "_%08X", dwHash );
        strInstName = "StreamTest_ChannelSvr";
        oCtx[ 0 ] = strInstName;
        strInstName += szBuf;
        oCtx[ propObjInstName ] = strInstName;
        oCtx[ propIsServer ] = true;
        oIfCfg.SetPointer( propSkelCtx,
            ( IConfigDb* )oCtx.GetCfg() );
        ret = super::OnPreStart( pCallback );
    }while( 0 );
    return ret;
}
