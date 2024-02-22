const { CConfigDb2 } = require("../combase/configdb")
const { ERROR, SYS_METHOD, Int32Value } = require("../combase/defines")
const { EnumPropId, errno, EnumCallFlags, EnumClsid, EnumTypeId, EnumProtoId } = require("../combase/enums")
const { IoCmd, CPendingRequest, AdminCmd, CIoEventMessage } = require("../combase/iomsg")
const { CIoReqMessage } = require("../combase/iomsg")
function NotifyDataConsumed( hStream )
{
    var oReq = CIoReqMessage()
    oReq.m_dwPortId = this.GetPortId()
    oReq.m_iCmd = IoCmd.DataConsumed[0]
    oReq.m_bNoReply = true
    oReq.m_oReq.Push(
        {t: EnumTypeId.typeUInt64, v: hStream})
    oReq.m_iMsgId = globalThis.g_iMsgIdx++
    this.m_oIoMgr.PostMessage( oReq )
}

function OnDataReceived( hStream, oMsg )
{
    var oBuf = oMsg.m_oReq.GetProperty( 1 )
    var oCb = this.OnDataReceived
    if( !oCb )
        return
    var ret = oCb( hStream, oBuf )
    if( ret !== errno.STATUS_PENDING )
        NotifyDataConsumed.bind( this )( hStream )
    return
}

/**
 * OnStreamClosed will close the stream given by hStream
 * @param {number} hStream the keepalive event message
 * @param {CIoEventMessage} oMsg not used
 * @returns {undefined}
 * @api public
 */
function OnStreamClosed( hStream, oMsg )
{
    this.DebugPrint( `stream@${hStream} will be closed: on message` + oMsg)
    var oCb = this.OnStreamClosed
    if( !oCb )
        return
    oCb( hStream, oMsg )
    this.m_funcCloseStream( hStream )
}

exports.OnDataReceivedLocal = OnDataReceived
exports.OnStreamClosedLocal = OnStreamClosed
exports.NotifyDataConsumed = NotifyDataConsumed