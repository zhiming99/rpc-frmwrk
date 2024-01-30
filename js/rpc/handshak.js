const { CConfigDb2 } = require("../combase/configdb")
const { Pair } = require("../combase/defines")
const { constval, errno, EnumPropId, EnumProtoId, EnumStmId, EnumTypeId, EnumCallFlags } = require("../combase/enums")
const { marshall, unmarshall } = require("../dbusmsg/message")
const { CIncomingPacket, COutgoingPacket, CPacketHeader } = require("./protopkt")
const { CDBusMessage, DBusIfName, DBusObjPath, DBusDestination2 } = require("./dmsg")
const { IoCmd, IoMsgType, CAdminRespMessage, CIoRespMessage, CPendingRequest, AdminCmd } = require("./iomsg")
const { messageType } = require( "../dbusmsg/constants")

function Handshake( oMsg, oPending )
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
            OnHandshakeComplete( this, t.m_oObject, t.m_oResp )
        }).catch(( t )=>{
            OnHandshakeComplete( this, t.m_oObject, t.m_oResp )
        })
}

function OnHandshakeComplete( oProxy, oPending, oResp )
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
        {t:EnumTypeId.typeUInt32, v: oProxy.m_dwPortId} )

    oPending.m_oResolve( oPending )
}

exports.Bdge_Handshake = Handshake
exports.Bdge_OnHandshakeComplete = OnHandshakeComplete