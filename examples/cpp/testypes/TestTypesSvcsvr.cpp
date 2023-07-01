/****BACKUP YOUR CODE BEFORE RUNNING RIDLC***/
// ridlc -bsO . ../../testypes.ridl 
// Implement the following methods
// to get the RPC proxy/server work
#include "rpc.h"
using namespace rpcf;
#include "stmport.h"
#include "fastrpc.h"
#include "TestTypes.h"
#include "TestTypesSvcsvr.h"

/* Sync Req Handler*/
gint32 CTestTypesSvc_SvrImpl::Echo(
    const std::string& strText /*[ In ]*/,
    std::string& strResp /*[ Out ]*/ )
{
    // TODO: Process the sync request here
    // return code can be an Error or
    // STATUS_SUCCESS
    strResp = strText;
    return STATUS_SUCCESS;
}
/* Sync Req Handler*/
gint32 CTestTypesSvc_SvrImpl::EchoByteArray(
    BufPtr& pBuf /*[ In ]*/,
    BufPtr& pRespBuf /*[ Out ]*/ )
{
    // TODO: Process the sync request here
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}
/* Sync Req Handler*/
gint32 CTestTypesSvc_SvrImpl::EchoArray(
    std::vector<gint32>& arrInts /*[ In ]*/,
    std::vector<gint32>& arrIntsR /*[ Out ]*/ )
{
    // TODO: Process the sync request here
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}
/* Sync Req Handler*/
gint32 CTestTypesSvc_SvrImpl::EchoMap(
    std::map<gint32,std::string>& mapReq /*[ In ]*/,
    std::map<gint32,std::string>& mapResp /*[ Out ]*/ )
{
    // TODO: Process the sync request here
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}
/* Sync Req Handler*/
gint32 CTestTypesSvc_SvrImpl::EchoMany(
    gint32 i1 /*[ In ]*/,
    gint16 i2 /*[ In ]*/,
    gint64 i3 /*[ In ]*/,
    float i4 /*[ In ]*/,
    double i5 /*[ In ]*/,
    const std::string& szText /*[ In ]*/,
    gint32& i1r /*[ Out ]*/,
    gint16& i2r /*[ Out ]*/,
    gint64& i3r /*[ Out ]*/,
    float& i4r /*[ Out ]*/,
    double& i5r /*[ Out ]*/,
    std::string& szTextr /*[ Out ]*/ )
{
    // TODO: Process the sync request here
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}
/* Sync Req Handler*/
gint32 CTestTypesSvc_SvrImpl::EchoStruct(
    FILE_INFO& fi /*[ In ]*/,
    FILE_INFO& fir /*[ Out ]*/ )
{
    // TODO: Process the sync request here
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}
/* Sync Req Handler*/
gint32 CTestTypesSvc_SvrImpl::EchoNoParams()
{
    // TODO: Process the sync request here
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}
/* Sync Req Handler*/
gint32 CTestTypesSvc_SvrImpl::EchoStream(
    HANDLE hstm_h /*[ In ]*/,
    HANDLE& hstmr_h /*[ Out ]*/ )
{
    // TODO: Process the sync request here
    // return code can be an Error or
    // STATUS_SUCCESS
    return ERROR_NOT_IMPL;
}
/* RPC event sender */
gint32 CTestTypesSvc_SvrImpl::OnHelloWorld(
    const std::string& strMsg /*[ In ]*/ )
{
    std::vector< InterfPtr > vecSkels;
    gint32 ret = this->EnumStmSkels( vecSkels );
    if( ERROR( ret ) )
        return ret;
    for( auto& elem : vecSkels )
    {
        CTestTypesSvc_SvrSkel* pSkel = elem;
        ret = pSkel->IITestTypes_SImpl::OnHelloWorld(
            strMsg );
    }
    return ret;
}
gint32 CTestTypesSvc_SvrImpl::CreateStmSkel(
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
            strDesc,"TestTypesSvc_SvrSkel",
            true, oCfg.GetCfg() );
        if( ERROR( ret ) )
            break;
        oCfg.CopyProp( propSkelCtx, this );
        oCfg[ propPortId ] = dwPortId;
        ret = pIf.NewObj(
            clsid( CTestTypesSvc_SvrSkel ),
            oCfg.GetCfg() );
    }while( 0 );
    return ret;
}
gint32 CTestTypesSvc_SvrImpl::OnPreStart(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCtx;
        CCfgOpenerObj oIfCfg( this );
        oCtx[ propClsid ] = clsid( 
            CTestTypesSvc_ChannelSvr );
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
        strInstName = "TestTypesSvc_ChannelSvr";
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
