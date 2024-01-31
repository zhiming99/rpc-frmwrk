const { CConfigDb2 } = require("../combase/configdb")
const { randomInt, ERROR } = require("../combase/defines")
const { constval, errno, EnumPropId, EnumProtoId, EnumStmId, EnumTypeId, EnumCallFlags, EnumIfState } = require("../combase/enums")
const { marshall, unmarshall } = require("../dbusmsg/message")
const { CIncomingPacket, COutgoingPacket, CPacketHeader } = require("./protopkt")
const { CDBusMessage } = require("./dmsg")
const { IoCmd, IoMsgType, CAdminRespMessage, CIoRespMessage, CIoReqMessage, CIoEventMessage, CPendingRequest, AdminCmd, IoEvent } = require("./iomsg")
const { messageType } = require( "../dbusmsg/constants")
const { Bdge_Handshake } = require("./handshak")

class CRpcStreamBase
{
    constructor( oParent )
    {
        this.m_iStmId = -1
        this.m_iPeerStmId = -1
        this.m_wProtoId = EnumProtoId.protoStream
        this.m_dwSeqNo = randomInt( 0xffffffff )
        this.m_dwAgeSec = 0
        this.m_oHeader = new CPacketHeader()
        this.m_oParent = oParent
    }

    SetStreamId( iStmId, wProtoId )
    {
        this.m_iStmId = iStmId
        this.m_wProtoId = wProtoId
        this.m_oHeader.m_iStmId = iStmId
        this.m_oHeader.m_wProtoId = wProtoId
        this.SendBuf = function ( oBuf )
        {
            var oHdr = this.m_oHeader
            oHdr.m_dwSeqNo = this.GetSeqNo()
            var oOut = new COutgoingPacket()
            oOut.SetHeader( oHdr )
            oOut.SetBufToSend( oBuf )

            // send
            return this.m_oParent.SendMessage( 
                oOut.Serialize() )
        }
    }

    GetStreamId()
    { return this.m_iStmId }

    GetProtoId()
    { return this.m_wProtoId }

    SetPeerStreamId( iPeerId )
    {
        this.m_iPeerStmId = iPeerId
        this.m_oHeader.m_iPeerStmId = iPeerId
    }

    GetPeerStreamId()
    { return this.m_iPeerStmId }

    AgeStream( dwSec )
    { this.m_dwAgeSec += dwSec }

    GetAge()
    { return this.m_dwAgeSec }

    Refresh()
    { this.m_dwAgeSec = 0 }

    GetSeqNo()
    { return this.m_dwSeqNo++ }

}

class CRpcControlStream extends CRpcStreamBase
{
    constructor( oParent )
    { super( oParent ) }

    ReceivePacket( oInPkt )
    {
        var oBuf = oInPkt.m_oBuf
        if( oBuf === null ||
            oBuf === undefined )
            return
        // post the data to the main thread
        var oReq = new CConfigDb2()
        oReq.Deserialize( oBuf, 0 )
    }
}

class CRpcDefaultStream extends CRpcStreamBase
{
    constructor( oParent )
    {
        super( oParent )
        var origSendBuf = this.SendBuf
        this.SendBuf = function ( dmsg )
        {
            var oBufToSend = marshall( dmsg )
            var oBufSize = Buffer.alloc( 4 )
            oBufSize.writeUInt32BE( oBufToSend.length )
            origSendBuf(
                Buffer.concat( [ oBufSize, oBufToSend] ) )
        }
    }

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
        dmsg.Copy( oMsg )
        if( dmsg.GetType() === messageType.signal )
            return
        else if( dmsg.GetType() === messageType.methodReturn )
        {
            var key = dmsg.GetReplySerial()
            var oReq = this.m_oParent.FindPendingReq( key )
            if( !oReq )
                return
            var oResp = new CConfigDb2()
            var iRet = dmsg.GetArgAt(0)
            if( iRet < 0 )
            {
                oResp.SetUint32(
                    EnumPropId.propReturnValue, iRet )
            }
            else
            {
                var oBuf = dmsg.GetArgAt(1)
                oResp.Deserialize( oBuf, 0 )
            }
            oReq.OnTaskComplete( oResp )
        }
        return
    }
}

class CRpcTcpBridgeProxy
{
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
        this.m_arrDispTable = []
        this.m_iState = EnumIfState.stateStarting

        var oIoTab = this.m_arrDispTable
        for( var i = 0; i< Object.keys(IoCmd).length;i++)
        { oIoTab.push(()=>{console.log("Invalid function call")}) }

        oIoTab[ IoCmd.Handshake[0] ] =
            Bdge_Handshake.bind( this )

        var oStm = new CRpcControlStream( this )
        var iStmId = EnumStmId.TCP_CONN_DEFAULT_CMD
        oStm.SetStreamId( iStmId,
            EnumProtoId.protoControl )
        oStm.SetPeerStreamId( iStmId )
        this.m_mapStreams.set( iStmId, oStm )

        oStm = new CRpcDefaultStream( this )
        iStmId = EnumStmId.TCP_CONN_DEFAULT_STM
        oStm.SetStreamId( iStmId, 
            EnumProtoId.protoDBusRelay )

        oStm.SetPeerStreamId( iStmId )
        this.m_mapStreams.set( iStmId, oStm)
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

                socket.onclose = () => {
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
            oMsg.m_dwTimeLeftSec =
                constval.IFSTATE_DEFAULT_IOREQ_TIMEOUT * 1000
            this.m_arrDispTable[ IoCmd.Handshake[0] ](oMsg, oPending)

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
        var key, val
        this.m_iState = EnumIfState.stateStopping
        for( [ key, val ] of this.m_mapPendingReqs )
            val.OnCanceled( -errno.ERROR_PORT_STOPPED )
        if( this.m_mapPendingReqs.size > 0 )
            this.m_mapPendingReqs.clear()
        if( this.IsConnected() )
            this.m_oSocket.close()
        this.m_oSocket = null

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
        var stm = this.m_mapStreams.get( iStmId )
        if( stm === undefined )
        {
            console.log( `cannot find stream for ${oInPkt}`)
            return -errno.ENOENT
        }
        return stm.ReceivePacket( oInPkt )
    }

    FindPendingReq( key )
    { return this.m_mapPendingReqs.get( key ) }

    GetPortId()
    { return this.m_dwPortId }

    SetPortId( dwPortId )
    { this.m_dwPortId = dwPortId }

    DispatchMsg( oMsg )
    {
        if( oMsg.m_iType !== IoMsgType.ReqMsg )
            return -errno.EINVAL
        console.log(oMsg)

        var oCmdTab = this.m_arrDispTable
        if( oMsg.m_iCmd >= oCmdTab.length )
            return -errno.ENOTSUP
        try{
            return oCmdTab[ oMsg.m_iCmd ]( oMsg )
        } catch( Error )
        { return  -errno.EFAULT}
    }
}

class CConnParams extends CConfigDb2
{
    constructor()
    { super() }

    IsEqual( rhs )
    {
        rval = rhs.GetProperty(
            EnumPropId.propDestUrl )
        lval = this.GetProperty(
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
        this.m_strSess = ""

        this.m_arrDispTable = []
        var oAdminTab = this.m_arrDispTable
        for( var i = 0; i< Object.keys(AdminCmd).length;i++)
        { oAdminTab.push(()=>{console.log("Invalid function")}) }

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
        var bFound = false
        var oConnParams = new CConnParams();
        oConnParams.Recover( oReq.m_oReq )
        var dwPortId, oConn
        for( [ dwPortId, oConn ] of this.m_mapBdgeProxies )
        {
            if( oConn.IsEqual( oConnParams ) )
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

    CloseRemotePort( oReq )
    {
        var ret = errno.STATUS_SUCCESS
        oResp = new CAdminRespMessage( oReq )
        try{
            var dwPortId = oReq.m_oReq.GetProperty(
                EnumPropId.propPortId )
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
            if( strRoute === "/" )
            {
                var oProxy =
                    this.m_mapBdgeProxies.get( dwPortId )
                if( !oProxy )
                    throw new Error( "Error no such proxy" )

                oProxy.Stop()
                this.m_mapBdgeProxies.delete( dwPortId )
                this.m_mapConnParams.delete( dwPortId )
            }
            var oEvt = new CIoEventMessage()
            oEvt.m_dwPortId = dwPortId
            oEvt.m_iCmd = IoEvent.OnRmtSvrOffline[0]
            oEvt.m_iMsgId = globalThis.g_iMsgIdx++
            oEvt.m_oReq.SetString(
                EnumPropId.propRouterPath, strRoute )
            this.PostMessage( oEvt )
        }
        catch( e )
        {
            console.log( e )
        }
    }

    GetProxy( key )
    {
        oProxy = undefined
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
        var oMsg = e.data
        var oReq = new CConfigDb2()
        oReq.Recover( oMsg.m_oReq )
        oMsg.m_oReq = oReq
        if( oMsg.m_iType === IoMsgType.AdminReq)
        {
            if( oMsg.m_iCmd >=
                this.m_arrDispTable.length )
                return -errno.ENOTSUP
            return this.m_arrDispTable[ oMsg.m_iCmd ]( oMsg )
        }
        else if( oMsg.m_iType === IoMsgType.ReqMsg )
        {
            var dwPortId = oMsg.GetPortId()
            if( dwPortId === null || dwPortId === undefined )
                return -errno.EINVAL
            var oProxy = this.GetProxy( dwPortId )
            if( !oProxy )
            {
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
