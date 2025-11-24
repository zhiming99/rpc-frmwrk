
const { CConfigDb2 } = require("../combase/configdb")
const { Pair, ERROR } = require("../combase/defines")
const { constval, errno, EnumPropId, EnumProtoId, EnumStmId, EnumTypeId, EnumCallFlags, EnumIfState } = require("../combase/enums")
const { marshall, unmarshall } = require("../dbusmsg/message")
const { CIncomingPacket, COutgoingPacket, CPacketHeader } = require("./protopkt")
const { CDBusMessage, DBusIfName, DBusObjPath, DBusDestination2 } = require("./dmsg")
const { IoCmd, IoMsgType, CAdminRespMessage, CIoRespMessage, CPendingRequest, AdminCmd } = require("../combase/iomsg")
const { messageType } = require( "../dbusmsg/constants")

function CheckRouterPathRemote( oMsg )
{
    var completion = OnCheckRouterPathComplete.bind( this )
    return new Promise( (resolve, reject)=>{

        var dmsg = new CDBusMessage( messageType.methodCall )

        dmsg.SetInterface( DBusIfName(
            constval.IFNAME_TCP_BRIDGE ) )

        dmsg.SetMember( IoCmd.CheckRouterPath[1] )
        dmsg.SetObjPath( this.m_strObjPath )
        dmsg.SetDestination( this.m_strDestination )
            
        dmsg.SetSerial( oMsg.m_iMsgId )
        var oParams = oMsg.m_oReq

        var oBuf = oParams.Serialize()
        var arrBody = []
        arrBody.push( new Pair( { t: "ay", v: oBuf }))
        dmsg.AppendFields( arrBody )

        var oStream = this.m_mapStreams.get(
            EnumStmId.TCP_CONN_DEFAULT_STM )

        var ret = oStream.SendBuf( dmsg )
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
        this.AddPendingReq(
            oMsg.m_iMsgId, oPendingEe )

    }).then( ( t )=>{
        completion( t )
    }).catch(( t )=>{
        completion( t )
    })
} 

function OnCheckRouterPathComplete( oPending )
{
    var oResp = oPending.m_oResp
    var ret = oResp.GetProperty(
        EnumPropId.propReturnValue )
    if( ret === null || ret === undefined )
        return
    if( ERROR( ret ) )
        ret = -ret;
    try{
        var oRespToLocal = new CIoRespMessage( oPending.m_oReq )
        oRespToLocal.m_oResp.SetUint32(
            EnumPropId.propReturnValue, ret )
        oRespToLocal.m_oReq = undefined
        this.m_oParent.PostMessage( oRespToLocal )
    }
    catch( e )
    {
    }
}
exports.Bdge_CheckRouterPathRemote = CheckRouterPathRemote