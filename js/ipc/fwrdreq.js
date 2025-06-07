const { CConfigDb2 } = require("../combase/configdb")
const { ERROR } = require("../combase/defines")
const { EnumPropId, errno, EnumCallFlags } = require("../combase/enums")
const { IoCmd, CPendingRequest } = require("../combase/iomsg")
const { messageType } = require( "../dbusmsg/constants")
const { CIoReqMessage } = require("../combase/iomsg")

/**
 * ForwardRequestLocal will send a user request to the worker thread
 * @param {CConfigDb2}oReq the parameters of the request
 * 
 * @param {function}oCallback the callback when the call is completed
 * asynchronously. oCallback is declared as `oCallback( oContext, oResp )`.  See
 * below for information about `oContext`. and `oResp` is a `CConfigDb2` object,
 * contains the reponse information.
 * 
 * @param {Object}oContext the parameter to pass to `oCallback`. It will have a
 * m_qwTaskId after the ForwardRequestLocal returns, which can be used to cancel
 * the request.
 * 
 * @returns {Promise}
 * @api public
 */
exports.ForwardRequestLocal = function ForwardRequestLocal( oReq, oCallback, oContext )
{
    var ret = 0
    return new Promise( ( resolve, reject )=>{
        var oMsg = new CIoReqMessage()
        oMsg.m_iMsgId = globalThis.g_iMsgIdx++
        oMsg.m_iCmd = IoCmd.ForwardRequest[0]
        oMsg.m_dwPortId = this.GetPortId()
        oMsg.m_oReq = oReq
        var oOpts = oReq.GetProperty( EnumPropId.propCallOptions )
        ret = oOpts.GetProperty( EnumPropId.propTimeoutSec )
        if( ret !== null )
        {
            oMsg.m_dwTimerLeftMs = ret * 1000
        }
        else
        {
            oMsg.m_dwTimerLeftMs = this.m_dwTimeoutSec * 1000
        }

        oReq.SetString( EnumPropId.propRouterPath,
            this.m_strRouterPath )
        oReq.SetUint32(
            EnumPropId.propSeqNo, oMsg.m_iMsgId)
        oReq.SetUint64(
            EnumPropId.propTaskId, oMsg.m_iMsgId)

        var oCallOpt = oReq.GetProperty(
            EnumPropId.propCallOptions )
        var dwFlags = oCallOpt.GetProperty(
            EnumPropId.propCallFlags )
        var bReply = true
        if( ( dwFlags & EnumCallFlags.CF_WITH_REPLY ) === 0 )
            bReply = false
        var oPending = new CPendingRequest(oMsg)
        oPending.m_oReq = oMsg
        oPending.m_oObject = this
        oPending.m_oResolve = resolve
        oPending.m_oReject = reject
        if( oContext !== undefined )
            oContext.m_qwTaskId = oMsg.m_iMsgId
        oPending.m_oCallback = oCallback.bind( this, oContext )
        this.PostMessage( oPending )
        if( bReply )
        {
            this.m_oIoMgr.AddPendingReq( oMsg.m_iMsgId, oPending)
        }
        else
        {
            var oResp = new CConfigDb2()
            oResp.SetUint32( EnumPropId.propReturnValue, 0)
            oResp.SetBool( EnumPropId.propNoReply, true )
            oPending.m_oResp = oResp
            resolve( oPending )
        }
    }).then(( e )=>{
        var oResp = e.m_oResp
        if( oResp === undefined )
        {
            oResp = new CConfigDb2()
            oResp.SetUint32( EnumPropId.propReturnValue,
                errno.ERROR_FAIL )
        }
        var ret;
        try{
            ret = e.m_oCallback( oResp )
        } catch( err ){
        }
        if( ret === undefined || ret === null )
        {
            ret = oResp.GetProperty(
                EnumPropId.propReturnValue )
        }
        return Promise.resolve( ret )
    }).catch(( e )=>{
        var oResp = e.m_oResp
        var ret = -errno.EFAULT 
        if( e.message !== undefined)
        {
            console.log( `Error ForwardRequestLocal failed: ${e.message}` )
        }
        else if( e.m_oCallback !== undefined )
        {
            console.log( `Error ForwardRequestLocal failed: ${e.m_oResp}` )
            ret = e.m_oCallback( oResp )
            if( ret === undefined || ret === null )
            {
                ret = oResp.GetProperty(
                    EnumPropId.propReturnValue )
            }
        }
        return Promise.resolve( ret )
    })
}
