const { CConfigDb2 } = require("../combase/configdb")
const { ERROR } = require("../combase/defines")
const { EnumPropId, errno } = require("../combase/enums")
const { IoCmd, CPendingRequest } = require("../combase/iomsg")
const { messageType } = require( "../dbusmsg/constants")
const { CIoReqMessage } = require("../combase/iomsg")

exports.ForwardRequestLocal = function ForwardRequestLocal( oReq, oCallback )
{
    var ret = 0
    return new Promise( ( resolve, reject )=>{
        var oMsg = new CIoReqMessage()
        oMsg.m_iMsgId = globalThis.g_iMsgIdx++
        oMsg.m_iCmd = IoCmd.ForwardRequest[0]
        oMsg.m_dwPortId = this.GetPortId()
        oMsg.m_oReq = oReq
        var oOpts = oReq.GetProperty( EnumPropId.propCallOptions )
        ret = oOpts.GetProperty( EnumPropId.propTimeoutsec )
        if( ret !== null )
        {
            oMsg.m_dwTimeLeftSec = ret * 1000
        }
        else
        {
            oMsg.m_dwTimeLeftSec = this.m_dwTimeoutSec * 1000
        }

        oReq.SetString( EnumPropId.propRouterPath,
            this.m_strRouterPath )
        oReq.SetUint32(
            EnumPropId.propSeqNo, oMsg.m_iMsgId)
        oReq.SetUint64(
            EnumPropId.propTaskId, oMsg.m_iMsgId)

        var oPending = new CPendingRequest(oMsg)
        oPending.m_oReq = oMsg
        oPending.m_oObject = this
        oPending.m_oResolve = resolve
        oPending.m_oReject = reject
        oPending.m_oCallback = oCallback
        this.PostMessage( oPending )
        this.m_oIoMgr.AddPendingReq( oMsg.m_iMsgId, oPending)
    }).then(( e )=>{
        var oResp = e.m_oResp
        if( oResp === undefined )
        {
            oResp = new CConfigDb2()
            oResp.SetUint32( EnumPropId.propReturnValue,
                errno.ERROR_FAIL )
        }
        try{
            e.m_oCallback( oResp )
        } catch( e ){
        }
    }).catch(( e )=>{
        if( e.message !== undefined)
        {
            console.log( `Error ForwardRequestLocal failed: ${e.message}` )
        }
        else if( e.m_oCallback !== undefined )
        {
            console.log( `Error ForwardRequestLocal failed: ${e.m_oResp}` )
            var ret = e.m_oResp.GetProperty(
                EnumPropId.propReturnValue )
            e.m_oCallback( e.m_oResp )
        }
    })
}