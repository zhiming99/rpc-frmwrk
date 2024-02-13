const { CConfigDb2 } = require("../combase/configdb")
const { ERROR, SYS_METHOD } = require("../combase/defines")
const { constval, errno, EnumPropId, EnumProtoId, EnumStmId, EnumTypeId, EnumCallFlags, EnumIfState, EnumSeriProto } = require("../combase/enums")
const { messageType } = require( "../dbusmsg/constants")
const { DBusIfName, DBusDestination2, DBusObjPath } = require("../rpc/dmsg")

function UserCancelRequestCb( oContext, oResp )
{
    var oUserCb = oContext.m_oUserCb
    var ret = oResp.GetProperty(
        EnumPropId.propReturnValue)
    if( ret === null )
    {
        oUserCb( errno.ERROR_FAIL )
        return
    }
    var qwTaskId = oContext.m_qwUserTaskId
    oUserCb( ret, qwTaskId )
    return
}

/**
 * UserCancelRequest will ask the server to cancel a pending request.
 * @param {number}qwTaskId the unique task id indicating the request to cancel
 * @param {function}oUserCallback the callback when UserCancelRequest is
 * completed asynchronously. oUserCallback accepts two parameters, one is the
 * error code, 0 indicating the request in question is canceled successfully,
 * otherwise, the request is not canceled successfully. And the other one is the
 * qwTaskId to cancel.
 * @returns {Promise}
 * @api public
 */
function UserCancelRequest( qwTaskId, oUserCallback )
{
    var oReq = new CConfigDb2()
    oReq.SetString( EnumPropId.propIfName,
        DBusIfName( "IInterfaceServer") )
    oReq.SetString( EnumPropId.propObjPath,
        this.m_strObjPath )
    oReq.SetString( EnumPropId.propSrcDBusName,
        this.m_strSender)
    oReq.SetString( EnumPropId.propDestDBusName,
        this.m_strDest )
    oReq.SetString( EnumPropId.propMethodName, 
        SYS_METHOD("UserCancelRequest"))
    oReq.Push( {t: EnumTypeId.typeUInt64, v: qwTaskId})
    var oCallOpts = new CConfigDb2()
    oCallOpts.SetUint32(
        EnumPropId.propTimeoutsec, this.m_dwTimeoutSec)
    oCallOpts.SetUint32(
        EnumPropId.propKeepAliveSec, this.m_dwKeepAliveSec )
    oCallOpts.SetUint32( EnumPropId.propCallFlags,
        EnumCallFlags.CF_ASYNC_CALL |
        EnumCallFlags.CF_WITH_REPLY |
        messageType.methodCall )
    oReq.SetObjPtr(
        EnumPropId.propCallOptions, oCallOpts)
    var oContext = new Object()
    oContext.m_oUserCb = oUserCallback
    oContext.m_qwUserTaskId = qwTaskId
    return this.m_funcForwardRequest(
        oReq, UserCancelRequestCb, oContext )
}

exports.UserCancelRequest = UserCancelRequest
