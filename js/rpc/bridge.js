const { Pair, CConfigDb2 } = require("../combase/configdb")
const { randomInt } = require("../combase/defines")
const { constval, errno, EnumPropId, EnumProtoId, EnumStmId, EnumTypeId, EnumCallFlags } = require("../combase/enums")
const { marshall, unmarshall } = require("../dbusmsg/message")
const { CIncomingPacket, COutgoingPacket, CPacketHeader } = require("./protopkt")
const { CDBusMessage } = require("./dmsg")
const { IoCmd, IoMsgType, CAdminRespMessage, CIoRespMessage, CIoReqMessage, AdminCmd } = require("./iomsg")
const { messageType } = require( "../dbusmsg/constants")

function ERROR( iRet )
{
    return iRet < 0
}

function SUCCEEDED( iRet )
{ return iRet === errno.STATUS_SUCCESS }

function DBusIfName(
    strName )
{
    var strIfName = constval.DBUS_NAME_PREFIX
    return strIfName + "Interf." + strName
}
exports.DBusIfName = DBusIfName
function DBusDestination(
    strModName )
{
    var strDest = constval.DBUS_NAME_PREFIX
    strDest += strModName
}
exports.DBusDestination = DBusDestination

function DBusObjPath(
    strModName, strObjName )
{
    if( strModName.includes("./") )
        return ""
    if( strObjName.length === 0 )
        return ""
    var strPath = "." +
        constval.DBUS_NAME_PREFIX +
        strModName + ".objs." + strObjName

    return strPath.replace(/\./g,'/')
}
exports.DBusObjPath = DBusObjPath

function DBusDestination2(
    strModName, strObjName )
{
    if( strModName.includes("./") )
        return ""
    if( strObjName.length === 0 )
        return ""
    return constval.DBUS_NAME_PREFIX +
        strModName + ".objs." + strObjName
}

exports.DBusDestination2 = DBusDestination2

class CPendingRequest
{
    constructor( oReq )
    {
        this.m_oResolve = null
        this.m_oReject = null
        this.m_oReq = oReq
        this.m_oResp = null
        this.m_oObject = null
    }

    OnTaskComplete( oResp )
    {
        if( this.m_oResolve === null )
            return
        this.m_oResp = oResp
        this.m_oResolve( this )
    }

    OnCanceled( iRet )
    {
        if( this.m_oReject === null)
            return
        var oResp = new CConfigDb2()
        oResp.SetUint32(
            EnumPropId.propReturnValue, iRet )
        this.m_oResp = oResp
        this.m_oReject( this )
    }
}

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

    Refresh()
    { this.m_dwAgeSec = 0 }

    GetSeqNo()
    { return this.m_dwSeqNo++ }

    SendBuf( oBuf )
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
    { super( oParent ) }

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

        var oIoTab = this.m_arrDispTable
        for( var i = 0; i< Object.keys(IoCmd).length;i++)
        { oIoTab.push(()=>{console.log("Invalid function call")}) }

        oIoTab[ IoCmd.Handshake[0] ] =
            this.Handshake.bind( this )

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
        this.m_iTimerId = setTimeout(
            this.m_oScanReqs, 10 * 1000 )
    }

    async Connect( strUrl, oPending )
    {
        try {
            const res = await new Promise((resolve, reject) => {
                var socket = new WebSocket(strUrl)
                socket.binaryType = 'arraybuffer'
                socket.onopen = (event) => {
                    this.m_oSocket = socket
                    resolve(this)
                }

                socket.onmessage = (event_1) => {
                    this.ReceiveMessage(event_1.data)
                }

                socket.onclose = () => {
                    console.log(`${this.m_oSocket} closed`)
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
            this.Handshake(oMsg, oPending)
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
        this.m_iTimerId =
            setTimeout( this.m_oScanReqs )
        return this.Connect( strUrl, oPending )
    }

    Stop()
    {
        this.m_oSocket.close()
        this.m_oSocket = null
        if( this.m_iTimerId != 0 )
        {
            clearTimeout( this.m_iTimerId )
            this.m_iTimerId = 0
        }
        this.m_oParent = null
    }

    Handshake( oMsg, oPending )
    {
        var ret = -errno.EINVAL
        var dmsg = new CDBusMessage( messageType.methodCall )

        dmsg.SetInterface( DBusIfName(
            constval.IFNAME_MIN_BRIDGE ) )
        dmsg.SetMember( IoCmd.Handshake[1] )
        dmsg.SetObjPath( DBusObjPath(
            this.m_oParent.GetRouterName(),
            constval.OBJNAME_TCP_BRIDGE ))

        dmsg.SetDestination( DBusDestination2(
            this.m_oParent.GetRouterName(),
            constval.OBJNAME_TCP_BRIDGE ) )
            
        dmsg.SetSerial( oMsg.m_iMsgId )
        var oParams = new CConfigDb2()
        var oInfo = new CConfigDb2()
        
        oInfo.SetUint64(
            EnumPropId.propTimestamp,
            Math.floor( Date.now() / 1000 ))

        oInfo.Push( new Pair(
            {t: EnumTypeId.typeString,
             v: constval.BRIDGE_PROXY_GREETINGS }) )

        oParams.Push( new Pair(
            {t: EnumTypeId.typeObj, v: oInfo }) )

        var oCallOpts = new CConfigDb2()
        oCallOpts.SetUint32(
            EnumPropId.propCallFlags,
            EnumCallFlags.CF_ASYNC_CALL |
            EnumCallFlags.CF_WITH_REPLY |
            messageType.methodCall )

        oCallOpts.SetUint32(
            EnumPropId.propTimeoutsec, 
            constval.IFSTATE_DEFAULT_IOREQ_TIMEOUT )

        oCallOpts.SetUint32(
            EnumPropId.propKeepAliveSec, 
            3600 )

        oParams.SetObjPtr(
            EnumPropId.propCallOptions, oCallOpts )
        var oBuf = oParams.Serialize()
        var arrBody = []
        arrBody.push( new Pair( { t: "ay", v: oBuf }))
        dmsg.AppendFields( arrBody )
        var oBufToSend = marshall( dmsg )
        var oBufSize = Buffer.alloc( 4 )
        oBufSize.writeUInt32BE( oBufToSend.length )

        var oStream =
            this.m_mapStreams.get( EnumStmId.TCP_CONN_DEFAULT_STM )

        ret = oStream.SendBuf(
            Buffer.concat([oBufSize, oBufToSend]) )

        if( ret < 0 )
        {
            var oResp = new CIoRespMessage( oMsg )
            oResp.m_oResp.SetUint32(
                EnumPropId.propReturnValue, ret )
            this.m_oParent.PostMessage( oResp )
            return null
        }
        return new Promise( (resolve, reject) =>{
                var oPendingHs = new CPendingRequest( oMsg )
                oPendingHs.m_oResolve = resolve
                oPendingHs.m_oReject = reject
                oPendingHs.m_oObject = oPending
                this.m_mapPendingReqs.set(
                    oMsg.m_iMsgId, oPendingHs )
            }).then( ( t )=>{
                this.OnHandshakeComplete( t.m_oObject, t.m_oResp )
            }).catch(( t )=>{
                this.OnHandshakeComplete( t.m_oObject, t.m_oResp )
            })
    }

    OnHandshakeComplete( oPending, oResp )
    {
        var ret = oResp.GetProperty(
            EnumPropId.propReturnValue )
        if( ret === null || ret === undefined )
            return
        if( ret < 0 )
        {
            oPending.m_oResp.SetUint32(
                EnumPropId.propReturnValue, ret )
            oPending.m_oReject( oPending )
            return
        }

        var strGreet = oResp.GetProperty( 0 )
        if( strGreet.substr( 0, constval.BRIDGE_GREETINGS.length ) !==
            constval.BRIDGE_GREETINGS )
        {
            oPending.m_oResp.SetUint32(
                EnumPropId.propReturnValue, -errno.EPROTO )
            oPending.m_oReject( oPending )
            return
        }
  
        console.log( "bridge returns " + strGreet )

        oPending.m_oResp.Push(
            {t:EnumTypeId.typeUInt32, v: this.m_dwPortId} )

        oPending.m_oResolve( oPending )
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

        this.m_arrDispTable = []
        var oAdminTab = this.m_arrDispTable
        for( var i = 0; i< Object.keys(AdminCmd).length;i++)
        { oAdminTab.push(()=>{console.log("Invalid function")}) }

        oAdminTab[ AdminCmd.SetConfig[0] ] = (oMsg)=>{
            this.SetConfig( oMsg ) }

        oAdminTab[ AdminCmd.OpenRemotePort[0]] = (oMsg)=>{
            this.OpenRemotePort( oMsg ) }
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