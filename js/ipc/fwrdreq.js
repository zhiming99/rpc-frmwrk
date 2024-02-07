const { CConfigDb2 } = require("../combase/configdb")
const { ERROR, Int32Value } = require("../combase/defines")
const { constval, errno, EnumPropId, EnumProtoId, EnumStmId, EnumTypeId, EnumCallFlags, EnumIfState, EnumSeriProto } = require("../combase/enums")
const { CDBusMessage, DBusIfName, DBusObjPath, DBusDestination2 } = require("../rpc/dmsg")
const { IoCmd, IoMsgType, CAdminRespMessage, CIoRespMessage, CPendingRequest, AdminCmd } = require("../combase/iomsg")
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
        ret = oReq.GetProperty( EnumPropId.propTimeoutsec )
        if( ret !== null )
        {
            oMsg.m_dwTimeLeftSec = ret * 1000
        }
        else
        {
            oMsg.m_dwTimeLeftSec = this.m_dwTimeoutSec * 1000
        }
        var rp = oReq.GetProperty( EnumPropId.propRouterPath )
        if( rp === null )
            oReq.SetString( EnumPropId.propRouterPath,
                this.m_strRouterPath )

        oReq.SetProperty(
            EnumPropId.propSeqNo, oMsg.m_iMsgId)
        oReq.SetProperty(
            EnumPropId.propTaskId, oMsg.m_iMsgId)

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
        try{
            if( oCallback !== undefined )
                oCallback( oResp )
        } catch( e ){
        }
    }).catch(( e )=>{
        if( e.message !== undefined)
        {
            console.log( `Error ForwardRequestLocal failed: ${e.message}` )
            var oResp = new CConfigDb2()
            oResp.SetUint32(
                EnumPropId.propReturnValue, -errno.EFAULT)
            oCallback( oResp )
        }
        else if( e.m_oResp !== undefined )
        {
            console.log( `Error ForwardRequestLocal failed: ${e.m_oResp}` )
            var ret = e.m_oResp.GetProperty(
                EnumPropId.propReturnValue )
            oCallback( e.m_Resp )
        }
    })
}