const { CConfigDb2 } = require("../combase/configdb")
const { Pair, ERROR } = require("../combase/defines")
const { constval, errno, EnumPropId, EnumProtoId, EnumStmId, EnumTypeId, EnumCallFlags, EnumIfState } = require("../combase/enums")
const { marshall, unmarshall } = require("../dbusmsg/message")
const { CIncomingPacket, COutgoingPacket, CPacketHeader } = require("./protopkt")
const { CDBusMessage, DBusIfName, DBusObjPath, DBusDestination, DBusDestination2 } = require("./dmsg")
const { IoCmd, IoMsgType, CAdminRespMessage, CIoRespMessage, CPendingRequest, AdminCmd, CIoEventMessage, IoEvent } = require("../combase/iomsg")
const { messageType } = require( "../dbusmsg/constants")

/**
 * Forward a remote event to the main thread
 * @param {CDBusMessage}dmsg the event message to forward
 * @returns {errno}
 * @api public
 */
function ForwardEvent( dmsg )
{
    try{
        if( !dmsg )
            throw new Error( "Error invalid parameter" )
        var oCtxBuf = dmsg.GetArgAt(0)
        var oCtx = new CConfigDb2()
        oCtx.Deserialize( oCtxBuf )
        var strRouter = oCtx.GetProperty( EnumPropId.propRouterPath )
        var oEvtBuf = dmsg.GetArgAt(1)
        var strIfName = DBusIfName(
            constval.IFNAME_TCP_BRIDGE )
        if( dmsg.GetInterface() !== strIfName )
            throw new Error( "Error invalid interface")
        var strObjPath = this.m_strObjPath 
        if( dmsg.GetObjPath() !== strObjPath )
            throw new Error( "Error invalid Object path")
        var oInnerMsg= unmarshall( oEvtBuf.slice(4)  )
        var oInner = new CDBusMessage()
        oInner.Restore( oInnerMsg )
        var oEvent = new CIoEventMessage()
        oEvent.m_iCmd = IoEvent.ForwardEvent[0]
        oEvent.m_iMsgId = oInner.GetSerial()
        var oEvtReq = oEvent.m_oReq

        var strVal = oInner.GetSender()
        if( strVal != undefined )
            oEvtReq.SetString(
                EnumPropId.propSrcDBusName, strVal )
        strVal = oInner.GetObjPath()
        if( strVal != undefined )
            oEvtReq.SetString(
                EnumPropId.propObjPath, strVal )
        strVal = oInner.GetMember()
        if( strVal != undefined )
            oEvtReq.SetString(
                EnumPropId.propMethodName, strVal )
        strVal = oInner.GetDestination() 
        if( strVal != undefined )
            oEvtReq.SetString(
                EnumPropId.propDestDBusName, strVal )
        strVal = oInner.GetSerial()
        if( strVal !== undefined )
            oEvtReq.SetUint32(
                EnumPropId.propSeqNo, strVal )
        strVal = oInner.GetInterface()
        if( strVal !== undefined )
            oEvtReq.SetString(
                EnumPropId.propIfName, strVal )

        var oParamBuf = oInner.GetArgAt(0)
        var oParams = new CConfigDb2()
        oParams.Deserialize( oParamBuf )
        oEvtReq.Push( {t: EnumTypeId.typeObj, v:oParams })
        oEvtReq.SetString( EnumPropId.propRouterPath, strRouter)
        oEvent.m_dwPortId = this.GetPortId()
        this.PostMessage( oEvent )
    }
    catch(e)
    {}
} 

exports.Bdge_ForwardEvent = ForwardEvent
