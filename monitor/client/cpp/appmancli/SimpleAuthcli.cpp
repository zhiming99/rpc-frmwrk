/****BACKUP YOUR CODE BEFORE RUNNING RIDLC***/
// ../../../../ridl/.libs/ridlc --sync_mode IAppStore=async_p --sync_mode IUserManager=async_p --services=AppManager,SimpleAuth --client -slO . ../../../../monitor/appmon/appmon.ridl 
// Implement the following methods
// to get the RPC proxy/server work
#include "rpc.h"
using namespace rpcf;
#include "stmport.h"
#include "fastrpc.h"
#include "appmon.h"
#include "AppManagercli.h"

InterfPtr g_pSAcli;
stdrmutex g_oSALock;
#include "SimpleAuthcli.h"
gint32 GetSimpleAuthcli( InterfPtr& pCli )
{
    gint32 ret = 0;
    do{
        CStdRMutex oLock( g_oSALock );
        InterfPtr pIf = g_pSAcli; 
        if( pIf.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }
        pCli = pIf;
    }while( 0 );
    return ret;
}
gint32 DestroySimpleAuthcli(
    CIoManager* pMgr, IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        InterfPtr pCli;
        ret = GetSimpleAuthcli( pCli );
        if( ERROR( ret ) )
            break;

        if( pCli->GetState() == stateStopping ||
            pCli->GetState() == stateStopped )
            break;

        if( pCallback == nullptr )
        {
            ret = pCli->Stop();
            break;
        }
        gint32 (*func)( IEventSink* pCb ) =
        ([]( IEventSink* pCb )->gint32
        {
            if( pCb != nullptr )
                pCb->OnEvent(
                    eventTaskComp, 0, 0, nullptr );
            CStdRMutex oLock( g_oSALock );
            g_pSAcli.Clear();
            return 0;
        });
        TaskletPtr pStopTask;
        ret = NEW_FUNCCALL_TASKEX(
            pStopTask, pMgr, func, pCallback );
        if( ERROR( ret ) )
            break;
        CRpcServices* pSvc = pCli;
        ret = pSvc->QueueStopTask( pCli, pStopTask );
        if( ERROR( ret ) )
        {
            ( *pStopTask )( eventCancelTask );
            OutputMsg( ret,
                "Error stop CSimpleAuth_CliImpl" );
        }
        if( SUCCEEDED( ret ) )
            ret = STATUS_PENDING;
    }while( 0 );
    if( ret != STATUS_PENDING )
        g_pSAcli.Clear();
    return ret;
}
gint32 CreateSimpleAuthcli( CIoManager* pMgr,
    EnumClsid iClsid, IEventSink* pCallback,
    IConfigDb* pCfg )
{
    gint32 ret = 0;
    do{
        gint32 (*func)( IEventSink*, IEventSink* ) =
        ([]( IEventSink* pTask, IEventSink* pCb )->gint32
        {
            gint32 ret = 0;
            ObjPtr ptrResp;
            do{
                IConfigDb* pResp = nullptr;
                CCfgOpenerObj oReq( pTask );
                ret = oReq.GetObjPtr(
                    propRespPtr, ptrResp );
                if( ERROR( ret ) )
                    break;
                pResp = ptrResp;
                CCfgOpener oResp( pResp );
                gint32 iRet = 0;
                ret = oResp.GetIntProp(
                    propReturnValue,
                    ( guint32& ) iRet );
                if( ERROR( ret ) )
                    break;
                if( ERROR( iRet ) )
                {
                    ret = iRet;
                    break;
                }

                ObjPtr pIf;
                ret = oResp.GetObjPtr( 0, pIf );
                if( SUCCEEDED( ret ) )
                {
                    CStdRMutex oLock( g_oSALock );
                    g_pSAcli = pIf;
                }
            }while( 0 );
            if( ptrResp.IsEmpty() )
            {
                CCfgOpener oCfg;
                oCfg.SetIntProp( propReturnValue, ret );
                ptrResp = oCfg.GetCfg();
            }
            Variant oVar( ptrResp );
            pCb->SetProperty( propRespPtr, oVar );
            pCb->OnEvent( eventTaskComp,
                ret, 0, ( LONGWORD* )pTask );
            return ret;
        });

        TaskletPtr pStartCb;
        ret = NEW_COMPLETE_FUNCALL( 0, pStartCb,
            pMgr, func, nullptr, pCallback );
        if( ERROR( ret ) )
            break;

        ret = AsyncCreateIf<CSimpleAuth_CliImpl>(
            pMgr, pStartCb, pCfg, iClsid,
            "invalidpath/appmondesc.json", "SimpleAuth",
            g_pSAcli, false );

        if( ERROR( ret ) )
            ( *pStartCb )( eventCancelTask );
    }while( 0 );
    return ret;
}

/* Async callback handler */
gint32 CSimpleAuth_CliImpl::GetUidByOAuth2NameCallback( 
    IConfigDb* context, gint32 iRet,
    gint32 dwUid /*[ In ]*/ )
{
    PSAACBS pCbs;
    gint32 ret = GetAsyncCallbacks( pCbs );
    if( ERROR( ret ) )
        return ret;
    return pCbs->GetUidByOAuth2NameCallback(
        context, iRet, dwUid );
}
/* Async callback handler */
gint32 CSimpleAuth_CliImpl::GetUidByKrb5NameCallback( 
    IConfigDb* context, gint32 iRet,
    gint32 dwUid /*[ In ]*/ )
{
    PSAACBS pCbs;
    gint32 ret = GetAsyncCallbacks( pCbs );
    if( ERROR( ret ) )
        return ret;
    return pCbs->GetUidByKrb5NameCallback(
        context, iRet, dwUid );
}
/* Async callback handler */
gint32 CSimpleAuth_CliImpl::GetUidByUserNameCallback( 
    IConfigDb* context, gint32 iRet,
    gint32 dwUid /*[ In ]*/ )
{
    PSAACBS pCbs;
    gint32 ret = GetAsyncCallbacks( pCbs );
    if( ERROR( ret ) )
        return ret;
    return pCbs->GetUidByUserNameCallback(
        context, iRet, dwUid );
}

/* Async callback handler */
gint32 CSimpleAuth_CliImpl::GetPasswordSaltCallback( 
    IConfigDb* context, gint32 iRet,
    const std::string& strSalt /*[ In ]*/ )
{
    PSAACBS pCbs;
    gint32 ret = GetAsyncCallbacks( pCbs );
    if( ERROR( ret ) )
        return ret;
    return pCbs->GetPasswordSaltCallback(
        context, iRet, strSalt );
}

/* Async callback handler */
gint32 CSimpleAuth_CliImpl::CheckClientTokenCallback( 
    IConfigDb* context, gint32 iRet,
    ObjPtr& oInfo /*[ In ]*/ )
{
    PSAACBS pCbs;
    gint32 ret = GetAsyncCallbacks( pCbs );
    if( ERROR( ret ) )
        return ret;
    return pCbs->CheckClientTokenCallback(
        context, iRet, oInfo );
    return 0;
}

gint32 CSimpleAuth_CliImpl::CreateStmSkel(
    InterfPtr& pIf )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg;
        auto pMgr = this->GetIoMgr();
        oCfg[ propIoMgr ] = ObjPtr( pMgr );
        oCfg[ propIsServer ] = false;
        oCfg.SetPointer( propParentPtr, this );
        oCfg.CopyProp( propSkelCtx, this );
        std::string strDesc;
        CCfgOpenerObj oIfCfg( this );
        ret = oIfCfg.GetStrProp(
            propObjDescPath, strDesc );
        if( ERROR( ret ) )
            break;
        ret = CRpcServices::LoadObjDesc(
            strDesc,"SimpleAuth_SvrSkel",
            false, oCfg.GetCfg() );
        if( ERROR( ret ) )
            break;
        ret = pIf.NewObj(
            clsid( CSimpleAuth_CliSkel ),
            oCfg.GetCfg() );
    }while( 0 );
    return ret;
}
gint32 CSimpleAuth_CliImpl::OnPreStart(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCtx;
        CCfgOpenerObj oIfCfg( this );
        oCtx[ propClsid ] = clsid( 
            CSimpleAuth_ChannelCli );
        oCtx.CopyProp( propObjDescPath, this );
        oCtx.CopyProp( propSvrInstName, this );
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
        strInstName = "SimpleAuth_ChannelSvr";
        oCtx[ 0 ] = strInstName;
        strInstName += szBuf;
        oCtx[ propObjInstName ] = strInstName;
        oCtx[ propIsServer ] = false;
        oIfCfg.SetPointer( propSkelCtx,
            ( IConfigDb* )oCtx.GetCfg() );
        ret = super::OnPreStart( pCallback );
    }while( 0 );
    return ret;
}

gint32 CSimpleAuth_CliImpl::OnPostStop(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        ret = super::OnPostStop( pCallback );
        LOGWARN( this->GetIoMgr(), ret, 
            "Warning CSimpleAuth_CliImpl stopped" );
        PSAACBS pCbs;
        CfgPtr pContext;
        gint32 ret = GetAsyncCallbacks( pCbs, pContext );
        if( ERROR( ret ) )
            break;
        pCbs->OnSvrOffline( pContext, this );
    }while( 0 );
    return ret;
}
