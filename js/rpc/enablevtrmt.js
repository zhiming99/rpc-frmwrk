const { CConfigDb2 } = require("../combase/configdb")
const { Pair, ERROR } = require("../combase/defines")
const { constval, errno, EnumPropId, EnumProtoId, EnumStmId, EnumTypeId, EnumCallFlags, EnumIfState } = require("../combase/enums")
const { marshall, unmarshall } = require("../dbusmsg/message")
const { CIncomingPacket, COutgoingPacket, CPacketHeader } = require("./protopkt")
const { CDBusMessage, DBusIfName, DBusObjPath, DBusDestination2 } = require("./dmsg")
const { IoCmd, IoMsgType, CAdminRespMessage, CIoRespMessage, CPendingRequest, AdminCmd } = require("../combase/iomsg")
const { messageType } = require( "../dbusmsg/constants")

function EnableRemoteEvent( oMsg )
{
    var completion = OnEnableEventComplete.bind( this )
    return new Promise( (resolve, reject)=>{

        var dmsg = new CDBusMessage( messageType.methodCall )

        dmsg.SetInterface( DBusIfName(
            constval.IFNAME_TCP_BRIDGE ) )

        dmsg.SetMember( IoCmd.EnableRemoteEvent[1] )
        dmsg.SetObjPath( DBusObjPath(
            this.m_oParent.GetRouterName(),
            constval.OBJNAME_TCP_BRIDGE ))

        dmsg.SetDestination( DBusDestination2(
            this.m_oParent.GetRouterName(),
            constval.OBJNAME_TCP_BRIDGE ) )
            
        dmsg.SetSerial( oMsg.m_iMsgId )
        var oParams = oMsg.m_oReq

        var oBuf = oParams.Serialize()
        var arrBody = []
        arrBody.push( new Pair( { t: "ay", v: oBuf }))
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

function OnEnableEventComplete( oPending )
{
    var oResp = oPending.m_oResp
    var ret = oResp.GetProperty(
        EnumPropId.propReturnValue )
    if( ret === null || ret === undefined )
        return
    try{
        var oResp = new CIoRespMessage( oPending.m_oReq )
        oResp.m_oResp.SetUint32(
            EnumPropId.propReturnValue, ret )
        oResp.m_oReq = undefined
        this.m_oParent.PostMessage( oResp )
    }
    catch( e )
    {
    }
    finally
    {
        this.m_mapPendingReqs.delete(
            oPending.m_oReq.m_iMsgId )
    }
}
exports.Bdge_EnableRemoteEvent = EnableRemoteEvent