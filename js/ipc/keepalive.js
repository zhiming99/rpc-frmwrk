const { CConfigDb2 } = require("../combase/configdb")
const { ERROR, SYS_METHOD } = require("../combase/defines")
const { EnumPropId, EnumCallFlags, EnumTypeId, } = require("../combase/enums")
const { IoCmd, CPendingRequest, CIoRespMessage } = require("../combase/iomsg")
const { messageType } = require( "../dbusmsg/constants")
const { CIoReqMessage } = require("../combase/iomsg")
const { CSerialBase } = require("../combase/seribase")
const { ForwardRequestLocal } = require( "./fwrdreq")
const { DBusIfName} = require( "../rpc/dmsg")

function KeepAliveRequest( oOrigMsg, qwTaskId )
{
    var oReq = new CConfigDb2()
    oReq.SetString( EnumPropId.propIfName,
        DBusIfName( "IInterfaceServer") )
    var val = oOrigMsg.GetProperty(
        EnumPropId.propObjPath )
    if( val !== null )
        oReq.SetString( 
            EnumPropId.propObjPath, val )
    val = oOrigMsg.GetProperty(
            EnumPropId.propSrcDBusName )
    if( val !== null )
        oReq.SetString(
            EnumPropId.propSrcDBusName, val )
    val = oOrigMsg.GetProperty(
        EnumPropId.propDestDBusName )
    if( val !== null )
        oReq.SetString(
            EnumPropId.propDestDBusName, val )
    oReq.SetString( EnumPropId.propMethodName, 
        SYS_METHOD("KeepAliveRequest"))
    oReq.Push( {t: EnumTypeId.typeUInt64, v: BigInt( qwTaskId )})
    var oCallOpts = new CConfigDb2()
    oCallOpts.SetUint32(
        EnumPropId.propTimeoutSec, 120)
    oCallOpts.SetUint32(
        EnumPropId.propKeepAliveSec, 60 )
    oCallOpts.SetUint32( EnumPropId.propCallFlags,
        EnumCallFlags.CF_ASYNC_CALL |
        messageType.methodCall )
    oReq.SetObjPtr(
        EnumPropId.propCallOptions, oCallOpts)

    var oContext = new Object()
    return this.m_funcForwardRequest.bind( this )(
        oReq, (oContext, oResp )=>{
            this.DebugPrint( ": KeepAliveRequest is sent")
        }, oContext )
}

/**
 * OnKeepAliveLocal is called when server's OnKeepAlive event arrives. It will
 * send a one-way KeepAliveRequest back to server.
 * @param {CIoEventMessage} oMsg the keepalive event message
 * @returns {undefined}
 * @api public
 */
function OnKeepAliveLocal( oMsg )
{
    try{
        var oEvent = oMsg.m_oReq
        var qwTaskId = oEvent.GetProperty( 0 );
        var oPending = this.m_oIoMgr.GetPendingReq( Number( qwTaskId ) )
        if( oPending === null )
            return
        var oReq = oPending.m_oReq
        var val = oReq.m_oReq.GetProperty(
            EnumPropId.propObjPath )
        if( val !== this.m_strObjPath )
            return
        KeepAliveRequest.bind( this )( oReq.m_oReq, qwTaskId )
    }
    catch(e)
    {

    }
}

exports.KeepAliveRequest = KeepAliveRequest
exports.OnKeepAliveLocal = OnKeepAliveLocal;