const { CConfigDb2 } = require("../combase/configdb")
const { randomInt, ERROR } = require("../combase/defines")
const { constval, errno, EnumPropId, EnumProtoId, EnumStmId, EnumTypeId, EnumCallFlags, EnumIfState } = require("../combase/enums")
const { IoCmd, IoMsgType, CAdminReqMessage, CAdminRespMessage, CIoRespMessage, CIoReqMessage, CIoEventMessage, CPendingRequest, AdminCmd, IoEvent } = require("../combase/iomsg")

exports.CIoManager = class CIoManager
{
    constructor()
    {
        this.m_oWorker = null
        this.m_mapPendingReqs = new Map()
        this.m_mapProxies = new Map()
    }

    Start()
    {
        var oWorker =
            new Worker(new URL("../rpc/ioworker.js", import.meta.url));
        oWorker.onmessage=(e)=>{
            this.DispatchMessage( e.data )
        }
        oWorker.onerror=(e)=>{
            console.log(e)
        }
        oWorker.onmessageerror=(e)=>{
            console.log(e)
        }
        this.m_oWorker = oWorker
    }

    Stop()
    {
        this.m_oWorker.terminate()
    }

    SetRouterConfig( oCfg )
    {
        var oMsg = new CAdminReqMessage()
        oMsg.m_iMsgId = globalThis.g_iMsgIdx++
        oMsg.m_iCmd = AdminCmd.SetConfig[0]
        oMsg.m_oReq.SetString(
            EnumPropId.propRouterName,
            "rpcrouter" )
        this.PostMessage( oMsg )
    }

    AddPendingReq( iKey, oPending )
    { this.m_mapPendingReqs.set(iKey, oPending) }

    RemovePendingReq( iKey )
    { this.m_mapPendingReqs.delete(iKey)}

    GetPendingReq( iKey )
    {
        var o = this.m_mapPendingReqs.get( iKey )
        if( o === undefined )
            o = null
        return o
    }

    PostMessage( oMsg )
    { this.m_oWorker.postMessage(oMsg) }

    DispatchMessage( oMsg )
    {
        try{
            if( oMsg.m_iType === IoMsgType.EvtMsg )
            {
                if( oMsg.m_iCmd === IoEvent.OnRmtSvrOffline[0])
                {
                    console.log( "connection is down")
                    var oEvt = new CIoEventMessage()
                    oEvt.Restore( oMsg )
                    var oProxy = this.m_mapProxies.get( oEvt.m_dwPortId )
                    if( oProxy !== undefined )
                        oProxy.Stop( errno.ERROR_PORT_STOPPED)
                }
                else if( oMsg.m_iCmd === IoEvent.ForwardEvent[0])
                {
                    var oEvt = new CIoEventMessage()
                    oEvt.Restore( oMsg )
                    var oProxy = this.m_mapProxies.get( oEvt.m_dwPortId )
                    if( oProxy !== undefined )
                        oProxy.DispatchEventMsg( oMsg )
                }
                else
                    throw new Error( `Error unsupported Message (${oMsg})`)
            }
            else if( oMsg.m_iType === IoMsgType.AdminResp ||
                oMsg.m_iType === IoMsgType.RespMsg )
            {
                var oPending = this.m_mapPendingReqs.get( oMsg.m_iMsgId )
                var oResp = new CConfigDb2()
                oResp.Restore( oMsg.m_oResp )
                oPending.m_oResp = oResp
                oPending.m_oResolve( oPending )
                this.m_mapPendingReqs.delete( oMsg.m_iMsgId )
            }
            else
                throw new Error( `Error unknown Message type (${oMsg.m_iType})`)
        } catch(e) {
            console.log( e.message )
        }
    }

    RegisterProxy( oProxy )
    {
        try{
            var dwPortId = oProxy.GetPortId()
            this.m_mapProxies.set( dwPortId, oProxy )
        }catch( e ) {
            console.log( "Register proxy failed")
        }
    }

    UnregisterProxy( oProxy, ret )
    {
        try{
            var dwPortId = oProxy.GetPortId()
            var key, oPending
            if( ret === undefined )
                ret = -errno.ECANCELED
            var oResp = new CConfigDb2()
            oResp.SetUint32(
                EnumPropId.propReturnValue, ret )
            var arrKeys = []
            for( [key, oPending] of this.m_mapPendingReqs )
            {
                oPending.m_oResp = oResp
                var oReq = oPending.m_oReq
                if( oReq.m_dwPortId === dwPortId )
                {
                    oPending.m_oReject( oPending )
                    arrKeys.push( key )
                }
            }
            for( key of arrKeys )
                this.m_mapPendingReqs.delete( key )

            this.m_mapProxies.delete( dwPortId )
        }catch( e ) {
            console.log( "Unregister proxy failed")
        }
    }
}
