const { CConfigDb2 } = require("../combase/configdb")
const { ERROR, Int32Value } = require("../combase/defines")
const { constval, errno, EnumPropId, EnumProtoId, EnumStmId, EnumTypeId, EnumCallFlags, EnumIfState, EnumSeriProto } = require("../combase/enums")
const { CDBusMessage, DBusIfName, DBusObjPath, DBusDestination2 } = require("../rpc/dmsg")
const { IoCmd, IoMsgType, CAdminRespMessage, CIoRespMessage, CPendingRequest, AdminCmd } = require("../combase/iomsg")
const { messageType } = require( "../dbusmsg/constants")
const { CIoReqMessage } = require("../combase/iomsg")

exports.EnableEventLocal = function EnableEventLocal( idx )
{
    var ret = 0
    if( idx >= this.m_arrMatches.length )
    {
        console.log( "EnableEventLocal completed")
        return ret
    }
    var oMatch = this.m_arrMatches[ idx ]
    return new Promise( ( resolve, reject )=>{
        var oMsg = new CIoReqMessage()
        oMsg.m_iMsgId = globalThis.g_iMsgIdx++
        oMsg.m_iCmd = IoCmd.EnableRemoteEvent[0]
        oMsg.m_dwPortId = this.GetPortId()
        oMsg.m_dwTimeLeftSec = this.m_dwTimeoutSec * 1000
        var oReq = oMsg.m_oReq
        oParams = new CConfigDb2()
        oParams.SetUint32( EnumPropId.propTimeoutsec,
            this.m_dwTimeoutSec )
        oParams.SetUint32( EnumPropId.propKeepAliveSec,
            this.m_dwKeepAliveSec )
        oParams.SetUint32( EnumPropId.propCallFlags,
            EnumPropId.propCallFlags,
            EnumCallFlags.CF_ASYNC_CALL |
            EnumCallFlags.CF_WITH_REPLY |
            messageType.methodCall )
        oReq.SetObjPtr(
            EnumPropId.propCallOptions, oParams )
        oReq.Push( {t: EnumTypeId.typeObj, v: oMatch})

        var oPending = new CPendingRequest(oMsg)
        oPending.m_oReq = oMsg
        // oPending.m_dwPortId = this.GetPortId()
        oPending.m_oObject = this
        oPending.m_oResolve = resolve
        oPending.m_oReject = reject
        this.PostMessage( oPending )
        this.m_oIoMgr.AddPendingReq(
            oMsg.m_iMsgId, oPending)
    }).then(( e )=>{
        var ret = 0
        const oResp = e.m_oResp
        var ret = Int32Value( oResp.GetProperty(
            EnumPropId.propReturnValue ) )
        if( ret === null || ERROR( ret ) )
            throw new Error( `Error returned ${ret}` )
        idx++
        if( idx >= this.m_arrMatches.length )
            return
        this.m_funcEnableEvent( idx )
    }).catch(( e )=>{
        if( e.message !== undefined)
            console.log( `Error EnableEventLocal failed: ${e.message}` )
        else if( e.m_oResp !== undefined )
        {
            var ret = e.m_oResp.GetProperty(
                EnumPropId.propReturnValue )
            console.log( `Error EnableEventLocal failed with error ${ret}` )
        }
    })
}