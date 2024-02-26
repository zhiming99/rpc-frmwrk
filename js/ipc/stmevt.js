const { CConfigDb2 } = require("../combase/configdb")
const { ERROR, SYS_METHOD, Int32Value } = require("../combase/defines")
const { EnumPropId, errno, EnumCallFlags, EnumClsid, EnumTypeId, EnumProtoId } = require("../combase/enums")
const { IoCmd, CPendingRequest, AdminCmd, CIoEventMessage } = require("../combase/iomsg")
const { CIoReqMessage } = require("../combase/iomsg")

/**
 * NotifyDataConsumed will send a message to webworker to notify the data block
 * is handled and can move on to the next incoming data block
 * @param {number} hStream the stream channel to notify
 * @returns {undefined}
 * @api public
 */
function NotifyDataConsumed( hStream )
{
    var oReq = new CIoReqMessage()
    oReq.m_dwPortId = this.GetPortId()
    oReq.m_iCmd = IoCmd.DataConsumed[0]
    oReq.m_bNoReply = true
    oReq.m_oReq.Push(
        {t: EnumTypeId.typeUInt64, v: hStream})
    oReq.m_iMsgId = globalThis.g_iMsgIdx++
    this.m_oIoMgr.PostMessage( oReq )
}

/**
 * OnDataReceived will be called when there is incoming data block from the
 * stream `hStream` It will call the user provided `OnDataReceived` callback if
 * present, and then notify the underlying webworker to move on to next incoming
 * data block. If the user provided `OnDataReceived` callback returns
 * STATUS_PENDING, the notifcation to the webworker won't happen, and receiving
 * from the stream channel will be hold back till the
 * `this.m_funcNotifyDataConsumed()` is called.
 * @param {number} hStream the stream channel the data comes in
 * @param {CIoEventMessage} oMsg the message from the webworker carrying the
 * data block.
 * @returns {undefined}
 * @api public
 */
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
 * OnStreamClosed will be called when the underlying stream channel is closed.
 * it will call user provided OnStreamClosed if present, and then claim all the
 * resources from the stream.
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
