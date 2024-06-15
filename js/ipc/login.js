const { CConfigDb2 } = require("../combase/configdb")
const { ERROR, SYS_METHOD } = require("../combase/defines")
const { errno, EnumPropId, EnumTypeId, EnumCallFlags } = require("../combase/enums")
const { AdminCmd } = require("../combase/iomsg")
const { messageType } = require( "../dbusmsg/constants")
const { DBusIfName, DBusDestination2, DBusObjPath } = require("../rpc/dmsg")
function UpdateSessHash( strHash )
{
    var oMsg = new CIoReqMessage()
    oMsg.m_iCmd = AdminCmd.UpdateSessHash[0]
    oMsg.m_iMsgId = globalThis.g_iMsgIdx++
    oMsg.m_dwPortId = this.GetPortId()
    oMsg.m_oReq.Push(
        {t: EnumTypeId.typeString, v: strHash})
    this.m_oIoMgr.PostMessage( oMsg )
}

function getCookie(name) {
    const value = `; ${document.cookie}`;
    const parts = value.split(`; ${name}=`);
    if (parts.length === 2){
      return parts.pop().split(';').shift();
   }
   return null
}

function LoginRequestCb( oContext, oResp )
{
    var oUserCb = oContext.m_oUserCb
    var ret = oResp.GetProperty(
        EnumPropId.propReturnValue)
    if( ret === null )
        ret = errno.ERROR_FAIL;
    if( !oUserCb )
        return
    oUserCb( ret )
    if( ERROR( ret ))
        return

    strHash = oResp.GetProperty(
        EnumPropId.propSessHash )
    if( !strHash )
        return
    UpdateSessHash.bind( this )( strHash )
    return
}

/**
 * LoginRequest will ask the server to login with the token available
 * @param {function}oUserCallback the callback to receive the login status code
 * @returns {Promise}
 * @api public
 */
function LoginRequest( oUserCallback )
{
    var oReq = new CConfigDb2()
    oReq.SetString( EnumPropId.propIfName,
        DBusIfName( "IAuthenticate") )
    oReq.SetString( EnumPropId.propObjPath,
        DBusObjPath( "rpcrouter", "RpcRouterBridgeAuthImpl") )
    oReq.SetString( EnumPropId.propSrcDBusName,
        this.m_strSender)
    oReq.SetString( EnumPropId.propDestDBusName,
        DBusDestination2( "rpcrouter", "RpcRouterBridgeAuthImpl") )
    oReq.SetString( EnumPropId.propMethodName, 
        SYS_METHOD("Login"))
    strCookie = getCookie( 'rpcf_code' )
    if( strCookie === null )
        return Promise.resolve( -errno.EACCES )

    oReq.Push( {t: EnumTypeId.typeString, v: strCookie })
    var oCallOpts = new CConfigDb2()
    oCallOpts.SetUint32(
        EnumPropId.propTimeoutSec, this.m_dwTimeoutSec)
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
    return this.m_funcForwardRequest(
        oReq, LoginRequestCb, oContext )
}

exports.LoginLocal = LoginRequest
