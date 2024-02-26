const { CConfigDb2 } = require("../combase/configdb")
const { randomInt, Int32Value, ERROR, InvalFunc } = require("../combase/defines")
const { constval, errno, EnumPropId, EnumProtoId, EnumStmId, EnumTypeId, EnumCallFlags, EnumIfState } = require("../combase/enums")
const { marshall, unmarshall } = require("../dbusmsg/message")
const { CIncomingPacket, COutgoingPacket, CPacketHeader } = require("./protopkt")
const { CDBusMessage } = require("./dmsg")
const { IoCmd, IoMsgType, CAdminRespMessage, CIoRespMessage, CIoReqMessage, CIoEventMessage, CPendingRequest, AdminCmd, IoEvent, CAdminReqMessage } = require("../combase/iomsg")
const { messageType } = require( "../dbusmsg/constants")
const { Bdge_Handshake } = require("./handshak")
const { Bdge_EnableRemoteEvent } = require("./enablevtrmt")
const { Bdge_ForwardRequest } = require("./fwrdreqrmt")
const { Bdge_ForwardEvent } = require("./fwrdevtrmt")
const { Bdge_OnKeepAlive } = require("./keepalivermt")
const { Bdge_OpenStream } = require( "./openstmrmt")
const { CRpcStreamBase, Bdge_DataConsumed } = require( "./stream")
const { Bdge_StreamWrite } = require( "./stmwritermt")

class CRpcControlStream extends CRpcStreamBase
{
    constructor( oParent )
    {
        super( oParent )
        this.m_wProtoId = EnumProtoId.protoControl
    }

    ReceivePacket( oInPkt )
    {
        try{
            var oBuf = oInPkt.m_oBuf
            if( oBuf === null ||
                oBuf === undefined )
            {
                throw new Error( "Invalid message")
            }
            // post the data to the main thread
            var oReq = new CConfigDb2()
            oReq.Deserialize( oBuf, 0 )
            var oCallOpt = oReq.GetProperty(
                EnumPropId.propCallOptions )
            var oFlags = oCallOpt.GetProperty(
                EnumPropId.propCallFlags )
            if( ( oFlags & messageType.methodReturn ) === 0 )
            {
                throw new Error( "Invalid message type")
            }
            var dwSeqNo = oReq.GetProperty(
                EnumPropId.propSeqNo )
            var oPending = this.m_oParent.FindPendingReq( dwSeqNo )
            if( !oPending )
            {
                throw new Error( "unable to find the request for " + dwSeqNo )
            }
            oPending.OnTaskComplete( oReq )
            this.m_oParent.RemovePendingReq( dwSeqNo )
        }
        catch( e )
        {
            console.log( "ControlStream discarded bad packet with error: " + e )
        }
    }
}

class CRpcDefaultStream extends CRpcStreamBase
{
    constructor( oParent )
    {
        super( oParent )
        this.m_wProtoId = EnumProtoId.protoDBusRelay
        var origSendBuf = this.SendBuf
        this.SendBuf = (( dmsg )=>{
            var oBufToSend = marshall( dmsg )
            var oBufSize = Buffer.alloc( 4 )
            oBufSize.writeUInt32BE( oBufToSend.length )
            return origSendBuf(
                Buffer.concat( [ oBufSize, oBufToSend] ) )
        }).bind(this)
    }
    /**
     * Receive data blocks from the server, and dispatch it to the proper
     * handler
     * @param {CIncomingPacket}oInPkt the incoming packet
     * @returns {undefined}
     * @api public
     */
    ReceivePacket( oInPkt )
    {
        var oBuf = oInPkt.m_oBuf
        if( oBuf === null ||
            oBuf === undefined )
            return
        var dmsgBuf = oBuf.slice( 4, oBuf.length )
        // post the data to the main thread
        var oMsg = unmarshall( dmsgBuf )
        var dmsg = new CDBusMessage()
        dmsg.Restore( oMsg )
        if( dmsg.GetType() === messageType.signal )
        {
            var strMethod = dmsg.GetMember()

            var key, value
            for( [key, value] of Object.entries(IoEvent) )
            {
                if( value[1] === strMethod )
                    this.m_oParent.m_arrDispEvtTable[ value[0]]( dmsg )
            }
        }
        else if( dmsg.GetType() === messageType.methodReturn )
        {
            var key = dmsg.GetReplySerial()
            var oPending = this.m_oParent.FindPendingReq( key )
            if( !oPending )
                return
            var oResp = new CConfigDb2()
            var iRet = dmsg.GetArgAt(0)
            if( ERROR( iRet ) )
            {
                oResp.SetUint32(
                    EnumPropId.propReturnValue, iRet )
            }
            else
            {
                var oBuf = dmsg.GetArgAt(1)
                if( oPending.m_oReq.m_iCmd === IoCmd.ForwardRequest[0])
                    oResp = oBuf
                else if( oPending.m_oReq.m_iCmd === IoCmd.FetchData[0])
                {
                    oResp.SetUint32( EnumPropId.propReturnValue, iRet )
                    var oDataDesc = new CConfigDb2()
                    oDataDesc.Deserialize( oBuf, 0 )
                    oResp.Push( {t: EnumTypeId.typeObj, v: oDataDesc})
                    oResp.Push( {t: EnumTypeId.typeUInt32, v:dmsg.GetArgAt(2)})
                    oResp.Push( {t: EnumTypeId.typeUInt32, v:dmsg.GetArgAt(3)})
                    oResp.Push( {t: EnumTypeId.typeUInt32, v:dmsg.GetArgAt(4)})
                }
                else
                    oResp.Deserialize( oBuf, 0 )
            }
            oPending.OnTaskComplete( oResp )
            this.m_oParent.RemovePendingReq( key )
        }
        return
    }
}

class CRpcTcpBridgeProxy
{
    BindFunctions()
    {
        var oIoTab = this.m_arrDispReqTable
        for( var i = 0; i< Object.keys(IoCmd).length;i++)
        { oIoTab.push(InvalFunc) }

        oIoTab[ IoCmd.Handshake[0] ] =
            Bdge_Handshake.bind( this )

        oIoTab[ IoCmd.EnableRemoteEvent[0]] =
            Bdge_EnableRemoteEvent.bind( this )

        oIoTab[ IoCmd.ForwardRequest[0] ] =
            Bdge_ForwardRequest.bind(this)

        oIoTab[ IoCmd.OpenStream[0]] =
            Bdge_OpenStream.bind(this)

        oIoTab[ IoCmd.StreamWrite[0]] =
            Bdge_StreamWrite.bind( this )

        oIoTab[ IoCmd.DataConsumed[0]] =
            Bdge_DataConsumed.bind( this )

        var oIoEvent = this.m_arrDispEvtTable
        oIoEvent[ IoEvent.ForwardEvent[0]] =
            Bdge_ForwardEvent.bind( this )

        oIoEvent[ IoEvent.OnKeepAlive[0]]  =
            Bdge_OnKeepAlive.bind( this )
    }

    NewStreamId()
    { return this.m_dwStreamId++ }

    constructor( oParent, oConnParams )
    {
        this.m_mapStreams = new Map()
        this.m_bCompress = false
        this.m_oConnParams = oConnParams
        this.m_strSess = ""
        this.m_dwPortId = 0
        this.m_mapPendingReqs = new Map()
        this.m_oSocket = null
        this.m_oParent = oParent
        this.m_arrDispReqTable = []
        this.m_iState = EnumIfState.stateStarting
        this.m_qwPeerTime = 0
        this.m_qwStartTime = 0
        this.m_arrDispEvtTable = []
        this.m_mapHStream2StmId = new Map()
        this.m_dwStreamId = 1024

        this.BindFunctions()

        var oStm = new CRpcControlStream( this )
        var iStmId = EnumStmId.TCP_CONN_DEFAULT_CMD
        oStm.SetStreamId( iStmId,
            EnumProtoId.protoControl )
        oStm.SetPeerStreamId( iStmId )
        this.AddStream( oStm )

        oStm = new CRpcDefaultStream( this )
        iStmId = EnumStmId.TCP_CONN_DEFAULT_STM
        oStm.SetStreamId( iStmId, 
            EnumProtoId.protoDBusRelay )

        oStm.SetPeerStreamId( iStmId )
        this.AddStream( oStm)
        this.m_iTimerId = 0

        this.m_oScanReqs = ()=>{
            var dwIntervalMs = 10 * 1000
            var expired = []
            var key, val
            for( [ key, val ] of this.m_mapPendingReqs )
            {
                if( val.m_oReq.DecTimer( dwIntervalMs ) > 0 )
                    continue;
                val.OnCanceled( -errno.ETIMEDOUT )
                expired.push( key )
            }
            for( key of expired )
                this.m_mapPendingReqs.delete( key )

            for( [key, val ] of this.m_mapStreams )
                val.AgeStream( 10 )

            this.m_iTimerId = setTimeout(
                this.m_oScanReqs, dwIntervalMs )
        }
    }

    async Connect( strUrl, oPending )
    {
        try {
            const res = await new Promise((resolve, reject) => {
                var socket = new WebSocket(strUrl)
                socket.binaryType = 'arraybuffer'
                socket.onopen = (event) => {
                    this.m_oSocket = socket
                    this.m_iState = EnumIfState.stateStarted
                    resolve(this)
                }

                socket.onmessage = (event_1) => {
                    this.ReceiveMessage(event_1.data)
                }

                socket.onclose = (e) => {
                    console.log(`${this.m_oSocket} closed`)
                    if( this.m_iState !== EnumIfState.stateStopping &&
                        this.m_iState !== EnumIfState.stateStopped )
                    {
                        var oFunc = this.m_oParent.OnRmtSvrOffline.bind(
                            this.m_oParent, this.m_dwPortId, "/")
                        setTimeout( oFunc, 0 )
                    }
                }

                socket.onerror = (event_2) => {
                    reject(this)
                    console.log(event_2)
                }
            })
            var oMsg = new CIoReqMessage()
            oMsg.m_iCmd = IoCmd.Handshake
            oMsg.m_iMsgId = globalThis.g_iMsgIdx++
            oMsg.m_iType = IoMsgType.ReqMsg
            oMsg.m_dwTimerLeftMs =
                constval.IFSTATE_DEFAULT_IOREQ_TIMEOUT * 1000
            this.m_arrDispReqTable[ IoCmd.Handshake[0] ](oMsg, oPending)

        } catch ( oProxy ) {
            oPending.m_oResp.SetUint32(
                EnumPropId.propReturnValue, -errno.EFAULT )
            oPending.m_oReject( oPending )
        }
    }

    IsConnected()
    {
        if( this.m_oSocket === null )
            return false
        if( this.m_oSocket.readyState !== WebSocket.OPEN)
            return false
        return true
    }

    Start( oPending )
    {
        var strUrl = this.m_oConnParams.GetProperty(
            EnumPropId.propDestUrl )
        this.m_iTimerId = setTimeout(
            this.m_oScanReqs, 10 * 1000 )
        return this.Connect( strUrl, oPending )
    }

    Stop()
    {
        var hStream
        var arrKeys = this.m_mapHStream2StmId.keys()
        for( hStream of arrKeys )
        {
            var oStm = this.GetStreamByHandle( hStream )
            oStm.Stop()
        }
        
        this.m_iState = EnumIfState.stateStopping
        for( [ key, val ] of this.m_mapPendingReqs )
            val.OnCanceled( -errno.ERROR_PORT_STOPPED )
        if( this.m_mapPendingReqs.size > 0 )
            this.m_mapPendingReqs.clear()
        if( this.IsConnected() )
            this.m_oSocket.close()

        if( this.m_iTimerId != 0 )
        {
            clearTimeout( this.m_iTimerId )
            this.m_iTimerId = 0
        }
        this.m_oParent = null
        this.m_iState = EnumIfState.stateStopped
    }

    // encode the message and send it through websocket
    SendMessage( oBuf )
    {
        if( !this.IsConnected() )
        {
            console.log(
            `WebSocket ${this.m_oSocket} is not ready to send`)
            return errno.ERROR_STATE
        }
        this.m_oSocket.send( oBuf )
        return errno.STATUS_SUCCESS
    }

    // decode the message and pass to mainthread
    ReceiveMessage( data )
    {
        var oBuf = Buffer.from( data )
        var oInPkt = new CIncomingPacket()
        oInPkt.Deserialize( oBuf, 0 )

        var iStmId = oInPkt.GetPeerStreamId()
        var stm = this.GetStreamById( iStmId )
        if( stm === undefined )
        {
            console.log( `cannot find stream for ${oInPkt}`)
            return -errno.ENOENT
        }
        return stm.ReceivePacket( oInPkt )
    }

    FindPendingReq( key )
    { return this.m_mapPendingReqs.get( key ) }

    RemovePendingReq( key )
    { this.m_mapPendingReqs.delete( key )}

    AddPendingReq( key, oPending )
    { this.m_mapPendingReqs.set( key, oPending)}

    GetPortId()
    { return this.m_dwPortId }

    SetPortId( dwPortId )
    { this.m_dwPortId = dwPortId }

    DispatchMsg( oMsg )
    {
        if( oMsg.m_iType !== IoMsgType.ReqMsg )
            return -errno.EINVAL

        var oCmdTab = this.m_arrDispReqTable
        if( oMsg.m_iCmd >= oCmdTab.length )
            return -errno.ENOTSUP
        try{
            return oCmdTab[ oMsg.m_iCmd ]( oMsg )
        } catch( Error )
        { return  -errno.EFAULT}
    }

    PostMessage( oMsg )
    {
        this.m_oParent.PostMessage( oMsg )
    }

    GetPeerTimestamp()
    {
        var qwNow = Math.floor( Date.now()/1000 )
        return qwNow - this.m_qwStartTime + this.m_qwPeerTime
    }

    AddStream( oStream )
    { 
        this.m_mapStreams.set( oStream.m_iStmId, oStream )
    }

    GetStreamById( iStmId )
    {
        return this.m_mapStreams.get( iStmId )
    }

    GetStreamByHandle( hStream )
    {
        var iStmId = this.m_mapHStream2StmId.get( hStream )
        if( iStmId === undefined )
            return undefined
        return this.m_mapStreams.get( iStmId )
    }

    RemoveStreamById( iStmId )
    {
        this.m_mapStreams.delete( iStmId )
    }

    RemoveStreamByHandle( hStream )
    {
        var iStmId = this.m_mapHStream2StmId.get( hStream )
        if( iStmId === undefined )
            return
        this.RemoveStreamById( iStmId )
        this.m_mapHStream2StmId.delete( hStream )
    }

    BindHandleStream( hStream, iStmId )
    {
        this.m_mapHStream2StmId.set( hStream, iStmId )
    }
}
exports.CRpcTcpBridgeProxy=CRpcTcpBridgeProxy

class CConnParams extends CConfigDb2
{
    constructor()
    { super() }

    IsEqual( rhs )
    {
        var rval = rhs.GetProperty(
            EnumPropId.propDestUrl )
        var lval = this.GetProperty(
            EnumPropId.propDestUrl )
        if( lval !== rval )
            return false
        return true
    }
}

class CRpcRouter
{
    constructor()
    {
        this.m_mapBdgeProxies = new Map()
        this.m_mapConnParams = new Map()
        this.m_dwPortIdCounter = 1
        this.m_oWorker = self
        this.m_oWorker.onmessage =
            this.DispatchMsg.bind( this )

        this.m_strRouterName = "rpcrouter"
        this.m_bOpenPortInProgress = false
        this.m_arrOpenPortQueue = []

        this.m_arrDispTable = []
        var oAdminTab = this.m_arrDispTable
        for( var i = 0; i< Object.keys(AdminCmd).length;i++)
        { oAdminTab.push(InvalFunc) }

        oAdminTab[ AdminCmd.SetConfig[0] ] = (oMsg)=>{
            this.SetConfig( oMsg ) }

        oAdminTab[ AdminCmd.OpenRemotePort[0]] = (oMsg)=>{
            this.OpenRemotePort( oMsg ) }

        oAdminTab[ AdminCmd.CloseRemotePort[0]] = (oMsg)=>{
            this.CloseRemotePort( oMsg ) }
    }

    GetRouterName()
    { return this.m_strRouterName }

    SetConfig( oMsg )
    {
        var ret = -errno.EINVAL
        var strName = oMsg.m_oReq.GetProperty(
            EnumPropId.propRouterName )
        if( strName.length > 0 )
            this.m_strRouterName = strName

        var oRet = new CAdminRespMessage( oMsg )
        oRet.m_oResp.SetUint32(
            EnumPropId.propReturnValue, ret )
        this.PostMessage( oRet )
    }

    GetNewPortId()
    { return this.m_dwPortIdCounter++ }

    OpenRemotePort( oReq )
    {
        if( this.m_bOpenPortInProgress )
        {
            console.log( "new OpenRemotePort request queued")
            this.m_arrOpenPortQueue.push( oReq )
            return
        }
        this.m_bOpenPortInProgress = true
        this.OpenRemotePortInternal( oReq )
    }

    UpdateOpenPortQueue()
    {
        if( this.m_arrOpenPortQueue.length > 0 )
        {
            console.log( "resume queued OpenRemotePort request")
            this.m_bOpenPortInProgress = true
            var newReq = this.m_arrOpenPortQueue.shift()
            setTimeout( ()=>{
                this.OpenRemotePortInternal( newReq )
            }, 0)
        }
        else
        {
            this.m_bOpenPortInProgress = false
        }
    }

    OpenRemotePortInternal( oReq )
    {
        var bFound = false
        var oConnParams = new CConnParams();
        oConnParams.Restore( oReq.m_oReq )
        var dwPortId, oProxy
        for( [ dwPortId, oProxy ] of this.m_mapBdgeProxies )
        {
            if( oProxy.m_oConnParams.IsEqual( oConnParams ) )
            {
                bFound = true
                break
            }
        }

        if( bFound )
        {
            var oRet = new CAdminRespMessage( oReq )
            oRet.m_oResp.SetUint32(
                EnumPropId.propReturnValue,
                errno.STATUS_SUCCESS )
            oRet.m_oResp.Push(
                {t: EnumTypeId.typeUInt32, v: dwPortId})
            this.PostMessage(oRet)
            this.UpdateOpenPortQueue()
            return null
        }

        return new Promise( (resolve, reject)=>{
            var oProxy = new CRpcTcpBridgeProxy(
                this, oConnParams )
            var dwPortId = this.GetNewPortId()
            oProxy.SetPortId( dwPortId )
            var oPending = new CPendingRequest( oReq )
            oPending.m_oResolve = resolve
            oPending.m_oReject = reject
            oPending.m_oObject = oProxy
            oPending.m_oResp = new CConfigDb2()
            oProxy.Start( oPending )
        }).then( ( t )=>{
            this.OnOpenRemotePortComplete(
                t.m_oObject, t.m_oReq, t.m_oResp )
        }).catch(( t )=>{
            this.OnOpenRemotePortComplete(
                t.m_oObject, t.m_oReq, t.m_oResp )
        })
    }

    OnOpenRemotePortComplete( oProxy, oReq, oResp )
    {
        try{
            var iRet = oResp.GetProperty(
                EnumPropId.propReturnValue )
            if( iRet === null || iRet === undefined )
                return

            var oRet = new CAdminRespMessage( oReq )
            oRet.m_oResp = oResp

            this.PostMessage( oRet )

            var dwPortId = oResp.GetProperty( 0 )
            if( ERROR( iRet ) )
            {
                if( dwPortId === null || dwPortId === undefined )
                    return
                oProxy.Stop()
                return
            }
            
            // register the bridge proxy
            var dwPortId = oResp.GetProperty( 0 )
            this.m_mapBdgeProxies.set(
                dwPortId, oProxy )

            var oConnParams = oReq.m_oReq
            this.m_mapConnParams.set(
                dwPortId, oConnParams )
        }
        finally
        {
            this.UpdateOpenPortQueue()
        }
    }

    CloseRemotePort( oReq )
    {
        var ret = errno.STATUS_SUCCESS
        var oResp = new CAdminRespMessage( oReq )
        try{
            var dwPortId = oReq.m_dwPortId
            if( dwPortId === null ||
                dwPortId === undefined )
            {
                ret = -errno.EINVAL
                throw new Error( "Error missing port id" )
            }
            var oProxy =
                this.m_mapBdgeProxies.get( dwPortId )
            if( !oProxy )
            {
                ret = -errno.ENOENT
                throw new Error( "Error no such proxy" )
            }

            oProxy.Stop()
            this.m_mapBdgeProxies.delete( dwPortId )
            this.m_mapConnParams.delete( dwPortId )
        }
        catch( e )
        {
            oResp.m_oResp.Push(
                {t: EnumTypeId.typeString, v: e.message })
        }
        finally
        {
            oResp.m_oResp.SetUint32(
                EnumPropId.propReturnValue, ret )
            this.PostMessage( oResp )
        }
    }

    OnRmtSvrOffline( dwPortId, strRoute )
    {
        try{
            var oProxy 
            if( strRoute !== "/" )
                return
            oProxy =
                this.m_mapBdgeProxies.get( dwPortId )

            var oEvt = new CIoEventMessage()
            oEvt.m_dwPortId = dwPortId
            oEvt.m_iCmd = IoEvent.OnRmtSvrOffline[0]
            oEvt.m_iMsgId = globalThis.g_iMsgIdx++
            oEvt.m_oReq.SetString(
                EnumPropId.propRouterPath, strRoute )
            if( oProxy )
                oEvt.m_oReq.SetObjPtr(
                    EnumPropId.propConnParams,
                    oProxy.m_oConnParams )
            this.PostMessage( oEvt )
            if( !oProxy )
                console.log( "Error no such proxy" )
            else
            {
                oProxy.Stop()
                this.m_mapBdgeProxies.delete( dwPortId )
                this.m_mapConnParams.delete( dwPortId )
            }
        }
        catch( e )
        {
            console.log( e.message )
        }
    }

    GetProxy( key )
    {
        var oProxy = undefined
        if( typeof key === "number" )
            oProxy = this.m_mapBdgeProxies.get( key )
        else if( typeof key === "string" )
        {
            var key, oConn
            for( [key, oConn] of this.m_mapConnParams )
            {
                strUrl = oConn.GetProperty(
                    EnumPropId.propDestUrl)
                if( strUrl !== key )
                    continue
                oProxy = this.m_mapBdgeProxies.get( key )
                break
            }
        }
        if( !oProxy )
            return null
        return oProxy
    }

    DispatchMsg( e )
    {
        var oOrigMsg = e.data
        if( oOrigMsg === undefined )
            return
        if( oOrigMsg.m_iType === IoMsgType.AdminReq)
        {
            var oMsg = new CAdminReqMessage()
            oMsg.Restore( oOrigMsg )
            if( oMsg.m_iCmd >=
                this.m_arrDispTable.length )
                return -errno.ENOTSUP
            return this.m_arrDispTable[ oMsg.m_iCmd ]( oMsg )
        }
        else if( oOrigMsg.m_iType === IoMsgType.ReqMsg )
        {
            var oMsg = new CIoReqMessage()
            oMsg.Restore( oOrigMsg )
            var dwPortId = oMsg.GetPortId()
            if( dwPortId === null || dwPortId === undefined )
                return -errno.EINVAL
            var oProxy = this.GetProxy( dwPortId )
            if( !oProxy )
            {
                if( oMsg.m_bNoReply )
                    return ret
                var ret = -errno.ENOENT
                var oResp = new CIoRespMessage( oMsg )
                oResp.m_oResp.SetUint32(
                    EnumPropId.propReturnValue, ret )
                this.PostMessage( oResp )
                return ret
            }
            return oProxy.DispatchMsg( oMsg )
        }
        return -errno.EINVAL
    }

    PostMessage( oMsg )
    { self.postMessage( oMsg ) }
}

exports.CRpcRouter = CRpcRouter
exports.CRpcStreamBase = CRpcStreamBase
