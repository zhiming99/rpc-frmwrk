const { CConfigDb2 } = require("../combase/configdb")
const { ERROR } = require("../combase/defines")
const { constval, errno, EnumPropId, EnumProtoId, EnumStmId, EnumTypeId, EnumCallFlags, EnumIfState, EnumSeriProto } = require("../combase/enums")
const { IoCmd, IoMsgType, CAdminRespMessage, CIoRespMessage, CPendingRequest, AdminCmd } = require("../combase/iomsg")
const { messageType } = require( "../dbusmsg/constants")
const { CIoReqMessage } = require("../combase/iomsg")

exports.EnableEventLocal = function EnableEventLocal( idx )
{
    if( idx >= this.m_arrMatches.length )
        return

    var oMatch = this.m_arrMatches[ idx ]
    return new Promise( ( resolve, reject )=>{
        var oMsg = new CIoReqMessage()
        oMsg.m_iMsgId = globalThis.g_iMsgIdx++
        oMsg.m_iCmd = IoCmd.EnableRemoteEvent[0]
        oMsg.m_dwPortId = this.GetPortId()
        oMsg.m_dwTimerLeftMs = this.m_dwTimeoutSec * 1000
        var oReq = oMsg.m_oReq
        var oParams = new CConfigDb2()
        oParams.SetUint32( EnumPropId.propTimeoutSec,
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
        oPending.m_oObject = this
        oPending.m_oResolve = resolve
        oPending.m_oReject = reject
        this.PostMessage( oPending )
        // this.m_oIoMgr.AddPendingReq( oMsg.m_iMsgId, oPending)
    }).then(( e )=>{
        const oResp = e.m_oResp
        var ret = oResp.GetProperty(
            EnumPropId.propReturnValue )
        if( ret === null || ERROR( ret ) )
            return Promise.reject( oResp )
        return Promise.resolve( oResp )
    }).catch(( e )=>{
        if( e.message !== undefined)
        {
            console.log( `Error EnableEventLocal failed: ${e.message}` )
            return Promise.reject( e.message )
        }
        else if( e.m_oResp !== undefined )
        {
            var ret = e.m_oResp.GetProperty(
                EnumPropId.propReturnValue )
            console.log( `Error EnableEventLocal failed with error ${ret}` )
            return Promise.reject( e.m_oResp )
        }
    })
}
