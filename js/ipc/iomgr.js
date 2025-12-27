const { CConfigDb2 } = require("../combase/configdb")
const { randomInt, ERROR } = require("../combase/defines")
const { constval, errno, EnumPropId, EnumProtoId, EnumStmId, EnumTypeId, EnumCallFlags, EnumIfState, EnumStmToken } = require("../combase/enums")
const { IoCmd, IoMsgType, CIoMessageBase, CAdminReqMessage, CAdminRespMessage, CIoRespMessage, CIoReqMessage, CIoEventMessage, CPendingRequest, AdminCmd, IoEvent } = require("../combase/iomsg")
const { CInterfaceProxy } = require("./proxy")
const { createHash } = require('crypto-browserify')

exports.CIoManager = class CIoManager
{
    constructor()
    {
        this.m_oWorker = null
        this.m_mapPendingReqs = new Map()
        this.m_mapProxies = new Map()
        this.m_mapStreams = new Map()
        this.m_iTimerId = 0

        this.m_oScanReqs = ()=>{
            var dwIntervalMs = 5 * 1000
            var expired = []
            var key, oPending
            for( [ key, oPending ] of this.m_mapPendingReqs )
            {
                if( !oPending.m_iTimeoutSec )
                    continue;
                if( ! typeof oPending.m_iTimeoutSec === "number" )
                    continue
                oPending.m_iTimeoutSec -= 5
                if( oPending.m_iTimeoutSec > 0 )
                    continue
                if( oPending.m_oReject )
                {
                    var oResp = new CConfigDb2()
                    oResp.SetUint32(
                        EnumPropId.propReturnValue, 
                        -errno.ETIMEDOUT )
                    oPending.m_oResp = oResp
                }
                expired.push( [ key, oPending ] )
            }
            for( [ key, oPending ] of expired )
            {
                this.m_mapPendingReqs.delete( key )
                oPending.m_oReject( oPending  )
            }

            this.m_iTimerId = setTimeout(
                this.m_oScanReqs, dwIntervalMs )
        }
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

        this.m_iTimerId = setTimeout(
            this.m_oScanReqs, 10 * 1000 )
    }

    Stop()
    {
        this.m_oScanReqs()
        clearTimeout(this.m_iTimerId)
        this.m_iTimerId = 0
        this.m_oWorker.terminate()
    }

    SetRouterConfig( oCfg )
    {
        var oMsg = new CAdminReqMessage()
        oMsg.m_iMsgId = globalThis.g_iMsgIdx++
        oMsg.m_iCmd = AdminCmd.SetConfig[0]
        var strDefRtName = 'rpcrouter';
        var strVal = oCfg.GetProperty(
            EnumPropId.propRouterName )
        if( strVal === null )
            strVal = strDefRtName;
        oMsg.m_oReq.SetString(
            EnumPropId.propRouterName,
            strVal )
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

    /**
     * DispatchMessage dispatches the messages from the webworker to the handlers.
     * @param {CIoMessageBase}oMsg the message from the webworker
     * @returns {undefined}
     * @api public
     */
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
                    var oProxies = this.m_mapProxies.get( oEvt.m_dwPortId )
                    if( oProxies !== undefined )
                    {
                        oProxies.forEach( ( oProxy, v2, set )=>{
                            oProxy.Stop( errno.ERROR_PORT_STOPPED)
                        })
                    }
                }
                else if( oMsg.m_iCmd === IoEvent.ForwardEvent[ 0 ] ||
                    oMsg.m_iCmd === IoEvent.OnKeepAlive[0])
                {
                    var oEvt = new CIoEventMessage()
                    oEvt.Restore( oMsg )
                    var oProxies = this.m_mapProxies.get( oEvt.m_dwPortId )
                    if( oProxies !== undefined )
                    {
                        oProxies.forEach( ( oProxy, v2, set )=>{
                            oProxy.m_arrDispTable[ oMsg.m_iCmd ]( oEvt )
                        })
                    }
                }
                else if( oMsg.m_iCmd === IoEvent.StreamRead[ 0 ] ||
                    oMsg.m_iCmd === IoEvent.StreamClosed[0])
                {
                    var oEvt = new CIoEventMessage()
                    oEvt.Restore( oMsg )
                    var hStream = oEvt.m_oReq.GetProperty( 0 )
                    if( !hStream )
                    {
                        console.log( "Error invalid stream event" + oEvt)
                        return
                    }

                    var oProxy = this.GetStreamOwner( hStream )
                    if( oProxy === undefined )
                    {
                        console.log( "Error stream event cannot find owner " + oEvt)
                        return
                    }
                    oProxy.m_arrDispTable[ oEvt.m_iCmd ]( hStream, oEvt )
                }
            }
            else if( oMsg.m_iType === IoMsgType.AdminResp ||
                oMsg.m_iType === IoMsgType.RespMsg )
            {
                var oPending = this.m_mapPendingReqs.get( oMsg.m_iMsgId )
                if( oPending === undefined )
                    throw new Error( "Error cannot find pending req for response " + oMsg)
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

    RegisterStream( hStream, oProxy )
    { this.m_mapStreams.set( hStream, oProxy ) }

    UnregisterStream( hStream )
    { this.m_mapStreams.delete( hStream ) }

    GetStreamOwner( hStream )
    { return this.m_mapStreams.get( hStream )}

    RegisterProxy( oProxy )
    {
        try{
            var dwPortId = oProxy.GetPortId()
            var oProxies = this.m_mapProxies.get( dwPortId )
            if( oProxies !== undefined )
                oProxies.add( oProxy )
            else
            {
                oProxies = new Set()
                oProxies.add( oProxy )
                this.m_mapProxies.set( dwPortId, oProxies )
            }
        }catch( e ) {
            console.log( "Register proxy failed")
        }
    }

    /**
     * UnregisterProxy and release the resources associated with the proxy
     * @param {CInterfaceProxy}oProxy the proxy to unregister with the iomgr
     * @param {number}ret the status code to pass to the pending request
     * @returns {Promise}
     * @api public
     */
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
                if( oPending.m_oObject === oProxy )
                {
                    oPending.m_oResp = oResp
                    oPending.m_oReject( oPending )
                    arrKeys.push( key )
                }
            }
            for( key of arrKeys )
                this.m_mapPendingReqs.delete( key )

            var oProxies = this.m_mapProxies.get( dwPortId )
            if( oProxies === undefined )
                return Promise.resolve(0)
            oProxies.delete( oProxy )
            if( oProxies.size > 0 )
                return Promise.resolve(0)
            this.m_mapProxies.delete( dwPortId )

            return new Promise( (resolve, reject)=>{
                var oMsg = new CAdminReqMessage()
                oMsg.m_iMsgId = globalThis.g_iMsgIdx++
                oMsg.m_iCmd = AdminCmd.CloseRemotePort[0]
                oMsg.m_dwPortId = dwPortId
                var oPending = new CPendingRequest(oMsg)
                oPending.m_oReq = oMsg
                oPending.m_oObject = this
                oPending.m_oResolve = resolve
                oPending.m_oReject = reject
                oPending.m_iTimeoutSec = 5
                oProxy.PostMessage( oPending )
            }).then((e)=>{
                return Promise.resolve(0)
            }).catch((e)=>{
                return Promise.reject( e )
            })
        }catch( e ) {
            console.log( "Unregister proxy failed")
            return Promise.resolve( -errno.EFAULT )
        }
    }

    async SetRouterName(
        strAppName, strObjName, strObjDesc, strAltName )
    {
        try {
            if( strAltName !== undefined )
            {
                var oCfg = new CConfigDb2();
                oCfg.SetProperty(
                    EnumPropId.propRouterName, strAltName );
                this.SetRouterConfig( oCfg );
                return Promise.resolve( ret )
            }
            var ret = errno.STATUS_SUCCESS
            const response = await fetch(strObjDesc)
            const o = await response.json()
            var arrObjs = o[ "Objects" ]
            var elem
            for( elem of arrObjs )
            {
                if( elem[ "ObjectName"] !== strObjName )
                    continue
                break
            }
            if( elem["DestURL"] === undefined )
                return Promise.reject( -errno.ENOENT );

            var strToHash = "t_" + elem["DestURL"] + "_0";

            var shasum = createHash('sha1');
            shasum.update( strToHash );
            var strSum = shasum.digest( 'hex' );
            this.m_strAppHash = 
                strSum.slice( 3 * 8, 4 * 8 ).toUpperCase();
            var strRouterName = strAppName + "_rt_"
                + this.m_strAppHash; 
            
            var oCfg = new CConfigDb2();
            oCfg.SetString(
                EnumPropId.propRouterName, strRouterName );
            this.SetRouterConfig( oCfg );
            return Promise.resolve( ret )
        } catch (e) {
            console.log(e.message)
            ret = -errno.EFAULT
            return Promise.reject( ret )
        }
    }
}
