const { CConfigDb2 } = require("../combase/configdb")
const { SYS_METHOD } = require("../combase/defines")
const { constval, errno, EnumPropId, EnumProtoId, EnumStmId, EnumTypeId, EnumCallFlags, EnumIfState } = require("../combase/enums")
const { marshall, unmarshall } = require("../dbusmsg/message")
const { CIncomingPacket, COutgoingPacket, CPacketHeader } = require("./protopkt")
const { CDBusMessage, DBusIfName, DBusObjPath, DBusDestination, DBusDestination2 } = require("./dmsg")
const { IoCmd, IoMsgType, CAdminRespMessage, CIoRespMessage, CPendingRequest, AdminCmd, CIoEventMessage, IoEvent } = require("../combase/iomsg")
const { messageType } = require( "../dbusmsg/constants")

/**
 * KeepAlive event handler 
 * @param {CDBusMessage}dmsg the 'KeepAlive' message from the remote server
 * @returns {undefined}
 * @api public
 */
function OnKeepAlive( dmsg )
{
    try{
        if( !dmsg )
            throw new Error( "Error invalid parameter" )
        var dwSerial = dmsg.GetSerial()
        var oEvt = new CConfigDb2()
        oEvt.Deserialize( dmsg.GetArgAt(0) )
        var qwTaskId = oEvt.GetProperty( 0 )
        var oPending, key
        var bFound = false
        for( [key, oPending ] of this.m_mapPendingReqs )
        {
            var qwPendingTaskId = oPending.m_oReq.m_iMsgIdLocal
            if( qwTaskId === qwPendingTaskId )
            {
                oPending.m_oReq.ResetTimer()
                bFound = true
                break
            }
        }
        if( !bFound )
            return
        var oEvent = new CIoEventMessage()
        oEvent.m_iCmd = IoEvent.OnKeepAlive[0]
        oEvent.m_iMsgId = globalThis.g_iMsgIdx++
        oEvent.m_oReq = oEvt
        oEvent.m_dwPortId = this.GetPortId()
        this.PostMessage( oEvent )
    }
    catch(e)
    {}
}

exports.Bdge_OnKeepAlive = OnKeepAlive
