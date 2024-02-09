const { CConfigDb2 } = require("../combase/configdb")
const { Pair, ERROR } = require("../combase/defines")
const { constval, errno, EnumPropId, EnumProtoId, EnumStmId, EnumTypeId, EnumCallFlags, EnumIfState } = require("../combase/enums")
const { marshall, unmarshall } = require("../dbusmsg/message")
const { CIncomingPacket, COutgoingPacket, CPacketHeader } = require("./protopkt")
const { CDBusMessage, DBusIfName, DBusObjPath, DBusDestination, DBusDestination2 } = require("./dmsg")
const { IoCmd, IoMsgType, CAdminRespMessage, CIoRespMessage, CPendingRequest, AdminCmd } = require("../combase/iomsg")
const { messageType } = require( "../dbusmsg/constants")

function BuildDBusMsgToFwrd( oReq )
{
    var dmsg = new CDBusMessage( messageType.methodCall )
    dmsg.SetDestination(
        oReq.GetProperty( EnumPropId.propDestDBusName ) )
    oReq.RemoveProperty( EnumPropId.propDestDBusName)
    dmsg.SetSender( oReq.GetProperty(
        EnumPropId.propSrcDBusName ) )
    oReq.RemoveProperty( EnumPropId.propSrcDBusName)
    dmsg.SetInterface( oReq.GetProperty(
        EnumPropId.propIfName ) )
    oReq.RemoveProperty( EnumPropId.propIfName)
    dmsg.SetObjPath( oReq.GetProperty(
        EnumPropId.propObjPath ) )
    oReq.RemoveProperty( EnumPropId.propObjPath)
    dmsg.SetMember( oReq.GetProperty(
            EnumPropId.propMethodName ) )
    oReq.RemoveProperty( EnumPropId.propMethodName )
    dmsg.SetSerial( oReq.GetProperty(
            EnumPropId.propSeqNo ) )
    oReq.RemoveProperty( EnumPropId.propSeqNo )

    var oCallOptions = oReq.GetProperty(
        EnumPropId.propCallOptions )
    var dwFlags = oCallOptions.GetProperty(
        EnumPropId.propCallFlags )
    if( ( dwFlags & EnumCallFlags.CF_WITH_REPLY ) === 0 )
        dmsg.SetNoReply()

    var oBuf = oReq.Serialize()
    var arrBody = []
    arrBody.push( new Pair( { t: "ay", v: oBuf }))
    dmsg.AppendFields( arrBody )
    return dmsg
}

function ForwardRequest( oMsg )
{
    var completion = OnForwardRequestComplete.bind( this )
    return new Promise( (resolve, reject)=>{

        oMsg.m_iMsgIdLocal = oMsg.m_iMsgId
        oMsg.m_iMsgId = globalThis.g_iMsgIdx++

        var oReq = new CConfigDb2()
        var oReqCtx = new CConfigDb2()

        var oInnerMsg = BuildDBusMsgToFwrd( oMsg.m_oReq )
        if( oInnerMsg === undefined  )
            throw new Error( "Error marshalling message")
        var oInnerBuf = marshall( oInnerMsg )
        var oInnerTotal = Buffer.alloc( 4 + oInnerBuf.length )
        oInnerTotal.writeUint32BE( oInnerBuf.length, 0)
        oInnerTotal.fill( oInnerBuf, 4, 4 + oInnerBuf.length )

        var strRouter = oMsg.m_oReq.GetProperty(
                EnumPropId.propRouterPath)
        oReqCtx.SetString(
            EnumPropId.propRouterPath, strRouter )
        if( oInnerMsg.IsNoReply())
            oReqCtx.SetBool( EnumPropId.propNoReply, true )

        var qwNow = Math.floor( Date.now()/1000 )
        oReqCtx.SetUint64( EnumPropId.propTimestamp,
            qwNow - this.m_qwStartTime + this.m_qwPeerTime )

        oReqCtx.SetUint64(
            EnumPropId.propTaskId, oMsg.m_iMsgIdLocal )
        oReqCtx.SetString(
            EnumPropId.propPath2, strRouter )

        var oCtxBuf = oReqCtx.Serialize()

        var dmsg = new CDBusMessage( messageType.methodCall )
        dmsg.SetInterface( DBusIfName(
            constval.IFNAME_MIN_BRIDGE ) )
        dmsg.SetMember( IoCmd.ForwardRequest[1] )
        dmsg.SetObjPath( DBusObjPath(
            this.m_oParent.GetRouterName(),
            constval.OBJNAME_TCP_BRIDGE ))
        dmsg.SetDestination( DBusDestination2(
            this.m_oParent.GetRouterName(),
            constval.OBJNAME_TCP_BRIDGE ) )
        dmsg.SetSender( DBusDestination(
            this.m_oParent.GetRouterName()))
        dmsg.SetSerial( oMsg.m_iMsgId )

        var arrBody = []
        arrBody.push( new Pair( { t: "ay", v: oCtxBuf }))
        arrBody.push( new Pair( { t: "ay", v: oInnerTotal }))
        arrBody.push( new Pair({t:"t", v: oMsg.m_iMsgIdLocal}))
        dmsg.AppendFields( arrBody )
        var oBufToSend = marshall( dmsg )
        var oBufSize = Buffer.alloc( 4 )
        oBufSize.writeUInt32BE( oBufToSend.length )

        var oStream = this.m_mapStreams.get(
            EnumStmId.TCP_CONN_DEFAULT_STM )

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

        var oPendingEe = new CPendingRequest( oMsg )
        oPendingEe.m_oResolve = resolve
        oPendingEe.m_oReject = reject
        oPendingEe.m_oObject = this
        this.m_mapPendingReqs.set(
            oMsg.m_iMsgId, oPendingEe )

    }).then( ( t )=>{
        completion( t )
    }).catch(( t )=>{
        completion( t )
    })
} 

function OnForwardRequestComplete( oPending )
{
    if( oPending.message !== undefined )
    {
        console.log( oPending )
        return
    }

    var dmsg = new CDBusMessage()
    dmsg.Restore( unmarshall(
        oPending.m_oResp.slice( 4 ) ) )
    
    var iRet = dmsg.GetArgAt( 0 )
    var oResp = new CConfigDb2()
    if( ERROR( iRet ) )
    {
        oResp.SetUint32(
            EnumPropId.propReturnValue, iRet )
        ret = iRet
    }
    else
    {
        var oBuf = dmsg.GetArgAt( 1 )
        oResp.Deserialize( oBuf, 0 )
    }

    var ret = oResp.GetProperty(
        EnumPropId.propReturnValue )
    if( ret === null || ret === undefined )
        return
    try{
        var oLocalResp = new CIoRespMessage( oPending.m_oReq )
        oLocalResp.m_oResp = oResp
        oLocalResp.m_oReq = undefined
        oLocalResp.m_iMsgId = oPending.m_oReq.m_iMsgIdLocal
        this.m_oParent.PostMessage( oLocalResp )
    }
    catch( e )
    {
    }
    finally
    {
        if( oPending.m_oResp !== undefined )
        {
            this.m_mapPendingReqs.delete(
                oPending.m_oReq.m_iMsgId )
        }
    }
}

exports.Bdge_ForwardRequest = ForwardRequest