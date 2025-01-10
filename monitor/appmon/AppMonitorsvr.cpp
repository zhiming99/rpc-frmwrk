/****BACKUP YOUR CODE BEFORE RUNNING RIDLC***/
// ridlc -sO . ./appmon.ridl 
// Implement the following methods
// to get the RPC proxy/server work
#include "rpc.h"
using namespace rpcf;
#include "stmport.h"
#include "fastrpc.h"
#include "appmon.h"
#include "AppMonitorsvr.h"

/* Sync Req Handler*/
gint32 CAppMonitor_SvrImpl::CreateFile(
    const std::string& strPath /*[ In ]*/,
    guint32 dwMode /*[ In ]*/,
    guint32 dwFlags /*[ In ]*/,
    guint64& hFile /*[ Out ]*/ )
{
    // TODO: Process the sync request here
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}
/* Sync Req Handler*/
gint32 CAppMonitor_SvrImpl::MakeDir(
    const std::string& strPath /*[ In ]*/,
    guint32 dwMode /*[ In ]*/ )
{
    // TODO: Process the sync request here
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}
/* Sync Req Handler*/
gint32 CAppMonitor_SvrImpl::OpenFile(
    const std::string& strPath /*[ In ]*/,
    guint32 dwFlags /*[ In ]*/,
    guint64& hFile /*[ Out ]*/ )
{
    // TODO: Process the sync request here
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}
/* Sync Req Handler*/
gint32 CAppMonitor_SvrImpl::CloseFile(
    guint64 hFile /*[ In ]*/ )
{
    // TODO: Process the sync request here
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}
/* Sync Req Handler*/
gint32 CAppMonitor_SvrImpl::RemoveFile(
    const std::string& strPath /*[ In ]*/ )
{
    // TODO: Process the sync request here
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}
/* Sync Req Handler*/
gint32 CAppMonitor_SvrImpl::ReadFile(
    guint64 hFile /*[ In ]*/,
    guint32 dwSize /*[ In ]*/,
    guint32 dwOff /*[ In ]*/,
    BufPtr& buffer /*[ Out ]*/ )
{
    // TODO: Process the sync request here
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}
/* Sync Req Handler*/
gint32 CAppMonitor_SvrImpl::WriteFile(
    guint64 hFile /*[ In ]*/,
    BufPtr& buffer /*[ In ]*/,
    guint32 dwOff /*[ In ]*/,
    guint32& dwSizeWrite /*[ Out ]*/ )
{
    // TODO: Process the sync request here
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}
/* Sync Req Handler*/
gint32 CAppMonitor_SvrImpl::Truncate(
    guint64 hFile /*[ In ]*/,
    guint32 dwOff /*[ In ]*/ )
{
    // TODO: Process the sync request here
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}
/* Sync Req Handler*/
gint32 CAppMonitor_SvrImpl::CloseDir(
    guint64 hFile /*[ In ]*/ )
{
    // TODO: Process the sync request here
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}
/* Sync Req Handler*/
gint32 CAppMonitor_SvrImpl::RemoveDir(
    const std::string& strPath /*[ In ]*/ )
{
    // TODO: Process the sync request here
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}
/* Sync Req Handler*/
gint32 CAppMonitor_SvrImpl::SetGid(
    const std::string& strPath /*[ In ]*/,
    guint32 wGid /*[ In ]*/ )
{
    // TODO: Process the sync request here
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}
/* Sync Req Handler*/
gint32 CAppMonitor_SvrImpl::SetUid(
    const std::string& strPath /*[ In ]*/,
    guint32 wUid /*[ In ]*/ )
{
    // TODO: Process the sync request here
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}
/* Sync Req Handler*/
gint32 CAppMonitor_SvrImpl::GetGid(
    const std::string& strPath /*[ In ]*/,
    guint32& gid /*[ Out ]*/ )
{
    // TODO: Process the sync request here
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}
/* Sync Req Handler*/
gint32 CAppMonitor_SvrImpl::GetUid(
    const std::string& strPath /*[ In ]*/,
    guint32& uid /*[ Out ]*/ )
{
    // TODO: Process the sync request here
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}
/* Sync Req Handler*/
gint32 CAppMonitor_SvrImpl::SymLink(
    const std::string& strSrcPath /*[ In ]*/,
    const std::string& strDestPath /*[ In ]*/ )
{
    // TODO: Process the sync request here
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}
/* Sync Req Handler*/
gint32 CAppMonitor_SvrImpl::GetValue(
    const std::string& strPath /*[ In ]*/,
    std::string& strJson /*[ Out ]*/ )
{
    // TODO: Process the sync request here
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}
/* Sync Req Handler*/
gint32 CAppMonitor_SvrImpl::SetValue(
    const std::string& strPath /*[ In ]*/,
    const std::string& strJson /*[ In ]*/ )
{
    // TODO: Process the sync request here
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}
/* Sync Req Handler*/
gint32 CAppMonitor_SvrImpl::Chmod(
    const std::string& strPath /*[ In ]*/,
    guint32 dwMode /*[ In ]*/ )
{
    // TODO: Process the sync request here
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}
/* Sync Req Handler*/
gint32 CAppMonitor_SvrImpl::Chown(
    const std::string& strPath /*[ In ]*/,
    guint32 dwUid /*[ In ]*/,
    guint32 dwGid /*[ In ]*/ )
{
    // TODO: Process the sync request here
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}
/* Sync Req Handler*/
gint32 CAppMonitor_SvrImpl::ReadLink(
    const std::string& strPath /*[ In ]*/,
    BufPtr& buf /*[ Out ]*/ )
{
    // TODO: Process the sync request here
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}
/* Sync Req Handler*/
gint32 CAppMonitor_SvrImpl::Rename(
    const std::string& szFrom /*[ In ]*/,
    const std::string& szTo /*[ In ]*/ )
{
    // TODO: Process the sync request here
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}
/* Sync Req Handler*/
gint32 CAppMonitor_SvrImpl::Flush(
    guint32 dwFlags /*[ In ]*/ )
{
    // TODO: Process the sync request here
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}
/* Sync Req Handler*/
gint32 CAppMonitor_SvrImpl::Access(
    const std::string& strPath /*[ In ]*/,
    guint32 dwFlags /*[ In ]*/ )
{
    // TODO: Process the sync request here
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}
/* Sync Req Handler*/
gint32 CAppMonitor_SvrImpl::GetAttr(
    const std::string& strPath /*[ In ]*/,
    FileStat& oStat /*[ Out ]*/ )
{
    // TODO: Process the sync request here
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}
/* Sync Req Handler*/
gint32 CAppMonitor_SvrImpl::ReadDir(
    guint64 hDir /*[ In ]*/,
    std::vector<FileStat>& vecDirEnt /*[ Out ]*/ )
{
    // TODO: Process the sync request here
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}
/* Sync Req Handler*/
gint32 CAppMonitor_SvrImpl::OpenDir(
    const std::string& strPath /*[ In ]*/,
    guint32 dwFlags /*[ In ]*/,
    guint64& hDir /*[ Out ]*/ )
{
    // TODO: Process the sync request here
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}
/* Sync Req Handler*/
gint32 CAppMonitor_SvrImpl::ExecBat(
    const std::string& strJson /*[ In ]*/ )
{
    // TODO: Process the sync request here
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::RegisterListener( 
    IConfigDb* pContext, 
    HANDLE hStream_h /*[ In ]*/,
    std::vector<std::string>& arrApps /*[ In ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'RegisterListenerComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::RemoveListener( 
    IConfigDb* pContext, 
    HANDLE hStream_h /*[ In ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'RemoveListenerComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::ListApps( 
    IConfigDb* pContext, 
    std::vector<std::string>& arrApps /*[ Out ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'ListAppsComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::ListPoints( 
    IConfigDb* pContext, 
    const std::string& strAppPath /*[ In ]*/, 
    std::vector<std::string>& arrPoints /*[ Out ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'ListPointsComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::ListAttributes( 
    IConfigDb* pContext, 
    const std::string& strPtPath /*[ In ]*/, 
    std::vector<std::string>& arrAttributes /*[ Out ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'ListAttributesComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::SetPointValue( 
    IConfigDb* pContext, 
    const std::string& strPtPath /*[ In ]*/,
    const Variant& value /*[ In ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'SetPointValueComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::GetPointValue( 
    IConfigDb* pContext, 
    const std::string& strPtPath /*[ In ]*/, 
    Variant& rvalue /*[ Out ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'GetPointValueComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::SetAttrValue( 
    IConfigDb* pContext, 
    const std::string& strAttrPath /*[ In ]*/,
    const Variant& value /*[ In ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'SetAttrValueComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::GetAttrValue( 
    IConfigDb* pContext, 
    const std::string& strAttrPath /*[ In ]*/, 
    Variant& rvalue /*[ Out ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'GetAttrValueComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::SetPointValues( 
    IConfigDb* pContext, 
    std::vector<KeyValue>& arrValues /*[ In ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'SetPointValuesComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* Async Req Handler*/
gint32 CAppMonitor_SvrImpl::GetPointValues( 
    IConfigDb* pContext, 
    std::vector<std::string>& arrPtPaths /*[ In ]*/, 
    std::vector<KeyValue>& arrKeyVals /*[ Out ]*/ )
{
    // TODO: Emit an async operation here.
    // And make sure to call 'GetPointValuesComplete'
    // when the service is done
    return ERROR_NOT_IMPL;
}
/* RPC event sender */
gint32 CAppMonitor_SvrImpl::OnPointChanged(
    const std::string& strPtPath /*[ In ]*/,
    const Variant& value /*[ In ]*/ )
{
    std::vector< InterfPtr > vecSkels;
    gint32 ret = this->EnumStmSkels( vecSkels );
    if( ERROR( ret ) )
        return ret;
    for( auto& elem : vecSkels )
    {
        CAppMonitor_SvrSkel* pSkel = elem;
        ret = pSkel->IIAppMonitor_SImpl::OnPointChanged(
            strPtPath,
            value );
    }
    return ret;
}
/* RPC event sender */
gint32 CAppMonitor_SvrImpl::OnPointsChanged(
    std::vector<KeyValue>& arrKVs /*[ In ]*/ )
{
    std::vector< InterfPtr > vecSkels;
    gint32 ret = this->EnumStmSkels( vecSkels );
    if( ERROR( ret ) )
        return ret;
    for( auto& elem : vecSkels )
    {
        CAppMonitor_SvrSkel* pSkel = elem;
        ret = pSkel->IIAppMonitor_SImpl::OnPointsChanged(
            arrKVs );
    }
    return ret;
}
gint32 CAppMonitor_SvrImpl::CreateStmSkel(
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
            strDesc,"AppMonitor_SvrSkel",
            true, oCfg.GetCfg() );
        if( ERROR( ret ) )
            break;
        oCfg.CopyProp( propSkelCtx, this );
        oCfg[ propPortId ] = dwPortId;
        ret = pIf.NewObj(
            clsid( CAppMonitor_SvrSkel ),
            oCfg.GetCfg() );
    }while( 0 );
    return ret;
}
gint32 CAppMonitor_SvrImpl::OnPreStart(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCtx;
        CCfgOpenerObj oIfCfg( this );
        oCtx[ propClsid ] = clsid( 
            CAppMonitor_ChannelSvr );
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
        strInstName = "AppMonitor_ChannelSvr";
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
