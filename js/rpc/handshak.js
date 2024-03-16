const { CConfigDb2 } = require("../combase/configdb")
const { Pair, ERROR } = require("../combase/defines")
const { constval, errno, EnumPropId, EnumProtoId, EnumStmId, EnumTypeId, EnumCallFlags, EnumIfState } = require("../combase/enums")
const { marshall, unmarshall } = require("../dbusmsg/message")
const { CIncomingPacket, COutgoingPacket, CPacketHeader } = require("./protopkt")
const { CDBusMessage, DBusIfName, DBusObjPath, DBusDestination2 } = require("./dmsg")
const { IoCmd, IoMsgType, CAdminRespMessage, CIoRespMessage, CPendingRequest, AdminCmd } = require("../combase/iomsg")
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
        EnumPropId.propTimeoutSec, 
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
    /*var oBufToSend = marshall( dmsg )
    var oBufSize = Buffer.alloc( 4 )
    oBufSize.writeUInt32BE( oBufToSend.length )*/

    var oStream =
        this.m_mapStreams.get( EnumStmId.TCP_CONN_DEFAULT_STM )

    ret = oStream.SendBuf( dmsg )

    if( ret < 0 )
    {
        var oResp = new CIoRespMessage( oMsg )
        oResp.m_oResp.SetUint32(
            EnumPropId.propReturnValue, ret )
        this.m_oParent.PostMessage( oResp )
        return null
    }

    oMsg.m_oReq = oParams
    return new Promise( (resolve, reject) =>{
            var oPendingHs = new CPendingRequest( oMsg )
            oPendingHs.m_oResolve = resolve
            oPendingHs.m_oReject = reject
            oPendingHs.m_oObject = oPending
            this.AddPendingReq(
                oMsg.m_iMsgId, oPendingHs )
        }).then( ( t )=>{
            OnHandshakeComplete( this, t, t.m_oResp )
        }).catch(( t )=>{
            OnHandshakeComplete( this, t, t.m_oResp )
        })
}

function OnHandshakeComplete( oProxy, oPending, oResp )
{
    var ret = oResp.GetProperty(
        EnumPropId.propReturnValue )
    if( ret === null || ret === undefined )
        return

    var oOuter = oPending.m_oObject
    try{
        if( ERROR( ret ) )
        {
            throw new Error( "Error handshake from server")
        }

        var oInfo = oResp.GetProperty( 0 )
        if( !oInfo )
        {
            ret = -errno.EINVAL
            throw new Error( "Error bad response parameters")
        }

        var strGreet = oInfo.GetProperty(0)
        if( strGreet.substr( 0, constval.BRIDGE_GREETINGS.length ) !==
            constval.BRIDGE_GREETINGS )
        {
            ret = -errno.EACCES
            throw new Error( "Error bad handshake string")
        }

        var strSess = oResp.GetProperty(
            EnumPropId.propSessHash )
        oProxy.m_strSess = strSess

        oProxy.m_qwPeerTime = Number( oInfo.GetProperty(
            EnumPropId.propTimestamp ) )

        var oInfo2 = oPending.m_oReq.m_oReq.GetProperty( 0 )
        var qwStartTime1 = Number( oInfo2.GetProperty(
            EnumPropId.propTimestamp ) )
        var qwStartTime2 = Math.floor( Date.now()/1000 )
        oProxy.m_qwStartTime =
            Math.floor( ( qwStartTime2 + qwStartTime1 ) / 2 )

        oOuter.m_oResp.Push(
            {t:EnumTypeId.typeUInt32, v: oProxy.m_dwPortId} )

        oOuter.m_oResp.SetUint32(
            EnumPropId.propReturnValue,
            ret )
        oOuter.m_oResolve( oOuter )

        oProxy.m_iState = EnumIfState.stateConnected
        console.log( "Handshake completed successfully" )

    }
    catch( e )
    {
        oOuter.m_oResp.SetUint32(
            EnumPropId.propReturnValue,
            ret )
        oOuter.m_oReject( oOuter )
    }
}

exports.Bdge_Handshake = Handshake
exports.Bdge_OnHandshakeComplete = OnHandshakeComplete
