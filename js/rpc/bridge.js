const { CConfigDb2 } = require("../combase/configdb")
const { randomInt } = require("../combase/defines")
const { errno, EnumPropId, EnumProtoId, EnumStmId } = require("../combase/enums")
const { unmarshall } = require("../dbusmsg/message")
const { CIncomingPacket, COutgoingPacket, CPacketHeader } = require("./protopkt")
const { CDBusMessage } = require("./dmsg")

class CPendingRequest
{
    constructor()
    {
        this.m_oResolve = null
        this.m_oReject = null
        this.m_oIoReq = null
        this.m_oObj = null
    }

    OnTaskComplete( oResp )
    {
        if( this.m_oResolve === null )
            return
        this.m_oIoReq.m_oResp = oResp
        this.m_oResolve.call(
            this.m_oObj, this.m_oIoReq )
    }

    OnCanceled( iRet )
    {
        if( this.m_oReject === null)
            return
        oResp = new CConfigDb2()
        oResp.SetUint32(
            EnumPropId.propReturnValue, iRet )
        this.m_oIoReq.m_oResp = oResp
        this.m_oReject.call(
            this.m_oObj, this.m_oIoReq )
    }
}

exports.CRpcStreamBase = class CRpcStreamBase
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
        oHdr = this.m_oHeader
        oHdr.m_dwSeqNo = this.GetSeqNo()
        oOut = new COutgoingPacket()
        oOut.SetHeader( oHdr )
        oOut.SetBufToSend( oBuf )

        // send
        this.m_oParent.SendMessage( 
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
        oReq = new CConfigDb2()
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
        // post the data to the main thread
        var oMsg = unmarshall( oBuf )
        var dmsg = new CDBusMessage()
    }
}

exports.CRpcTcpBridgeProxy = class CRpcTcpBridgeProxy
{
    constructor( oConnParams )
    {
        this.m_mapStreams = new Map()
        this.m_bCompress = false
        this.m_oConnParams = oConnParams
        this.m_strRouterName = ""
        this.m_strSess = ""
        this.m_dwPortId = 0
        this.m_mapPendingReqs = new Map()
        this.m_oSocket = null

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
            for( [key, val ] of this.m_mapPendingReqs )
            {
                if( val.m_oIoReq.DecTimer( dwIntervalMs ) > 0 )
                    continue;
                val.OnCanceled( -errno.ETIMEDOUT )
            }
            for( [key, val ] of this.m_mapStreams )
                val.AgeStream( 10 )

            this.m_iTimerId = setTimeout(
                this.m_oScanReqs, dwIntervalMs )
        }
        this.m_iTimerId = setTimeout(
            this.m_oScanReqs, 10 * 1000 )
    }

    Connect( strUrl )
    {
        var socket = new WebSocket( strUrl );
        socket.binaryType = 'arraybuffer'
        socket.onopen = ()=>{
            this.m_oSocket = socket
            console.log('connected!');
        };

        socket.onmessage = (event) => {
            this.ReceiveMessage( event.data )
        };

        socket.onclose = ()=>{
            console.log(`${this.m_oSocket} closed`);
        };
        return socket
    }

    IsConnected()
    {
        if( this.m_oSocket === null )
            return false
        if( this.m_oSocket.readyState !== WebSocket.OPEN)
            return false
        return true
    }

    Start()
    {
        var strUrl = this.m_oConnParams.GetProperty(
            EnumPropId.propDestUrl )
        Connect( strUrl )
        this.m_iTimerId =
            setTimeout( this.m_oScanReqs )
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
        oBuf = Buffer.from( data )
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

    GetPortId()
    { return this.m_dwPortId }

    SetPortId( dwPortId )
    { this.m_dwPortId = dwPortId }
}

exports.CConnParams = class CConnParams extends CConfigDb2
{
    constructor()
    {
        super()
        this.SetBool(
            EnumPropId.propEnableSSL, true )
        this.SetBool(
            EnumPropId.propEnableWebSock, true )
    }

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

exports.CRpcRouter = class CRpcRouter
{
    constructor()
    {
        this.m_mapBdgeProxies = new Map()
        this.m_mapConnParams = new Map()
        this.m_dwPortId = 1
    }

    StartBridgeProxy( oConnParams )
    {
        bFound = false
        for( [ dwPortId, oConn ] of this.m_mapBdgeProxies )
        {
            if( oConn.IsEqual( oConnParams ) )
            {
                bFound = true
                break
            }
        }

        if( bFound )
            return oConn

        oProxy = new CRpcTcpBridgeProxy( oConnParams )
        dwPortId = this.m_dwPortId++ 
        oProxy.SetPortId( dwPortId )

        this.m_mapBdgeProxies.set(
            dwPortId, oProxy )

        this.m_mapConnParams.set(
            dwPortId, oConnParams )

        oProxy.Start()
    }

    GetProxy( key )
    {
        oProxy = undefined
        if( typeof key === "number" )
            oProxy = this.m_mapBdgeProxies.get( key )
        else if( typeof key === "string" )
        {
            for( [key, oConn] of this.m_mapConnParams )
            {
                strUrl = oConn.GetProperty(
                    EnumPropId.propDestUrl)
                if( strUrl === key )
                    oProxy = this.m_mapBdgeProxies.get( key )
            }
        }
        if( oProxy === undefined )
            return null
        return oProxy
    }
}