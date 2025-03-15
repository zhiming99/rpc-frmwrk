/****BACKUP YOUR CODE BEFORE RUNNING RIDLC***/
// ../../ridl/.libs/ridlc --server -sO . ./appmon.ridl 
// Implement the following methods
// to get the RPC proxy/server work
#include "rpc.h"
using namespace rpcf;
#include "stmport.h"
#include "fastrpc.h"
#include "blkalloc.h"
#include "appmon.h"
#include "AppMonitorsvr.h"
#include "SimpleAuthsvr.h"

#define GETAC( _pContext ) \
    CAccessContext oac; \
    ret = m_pAppMon->GetAccessContext(\
        ( _pContext ), oac ); \
    if( ret == -EACCES ) \
        break; \
    if( SUCCEEDED( ret ) ) \
    { ret = -EACCES; break; } \
    if( ret != -ENOENT && ERROR( ret ) ) \
        break; \
    ret = 0

/* Async Req Handler*/
gint32 CSimpleAuth_SvrImpl::GetUidByOAuth2Name( 
    IConfigDb* pContext, 
    const std::string& strOA2Name /*[ In ]*/, 
    gint32& dwUid /*[ Out ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'GetUidByOAuth2NameComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CSimpleAuth_SvrImpl::GetUidByKrb5Name( 
    IConfigDb* pContext, 
    const std::string& strKrb5Name /*[ In ]*/, 
    gint32& dwUid /*[ Out ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'GetUidByKrb5NameComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CSimpleAuth_SvrImpl::GetUidByUserName( 
    IConfigDb* pContext, 
    const std::string& strUserName /*[ In ]*/, 
    gint32& dwUid /*[ Out ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'GetUidByUserNameComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CSimpleAuth_SvrImpl::GetPasswordSalt( 
    IConfigDb* pContext, 
    gint32 dwUid /*[ In ]*/, 
    std::string& strSalt /*[ Out ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'GetPasswordSaltComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}

gint32 CSimpleAuth_SvrImpl::CreateStmSkel(
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
        std::string strDesc;
        CCfgOpenerObj oIfCfg( this );
        ret = oIfCfg.GetStrProp(
            propObjDescPath, strDesc );
        if( ERROR( ret ) )
            break;
        ret = CRpcServices::LoadObjDesc(
            strDesc,"SimpleAuth_SvrSkel",
            true, oCfg.GetCfg() );
        if( ERROR( ret ) )
            break;
        oCfg.CopyProp( propSkelCtx, this );
        oCfg[ propPortId ] = dwPortId;
        ret = pIf.NewObj(
            clsid( CSimpleAuth_SvrSkel ),
            oCfg.GetCfg() );
    }while( 0 );
    return ret;
}
gint32 CSimpleAuth_SvrImpl::OnPreStart(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCtx;
        CCfgOpenerObj oIfCfg( this );
        oCtx[ propClsid ] = clsid( 
            CSimpleAuth_ChannelSvr );
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
        oCtx[ propIsServer ] = true;
        oIfCfg.SetPointer( propSkelCtx,
            ( IConfigDb* )oCtx.GetCfg() );
        ret = super::OnPreStart( pCallback );
    }while( 0 );
    return ret;
}
