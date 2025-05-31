const { CConfigDb2 } = require("../combase/configdb")
const { ERROR, SYS_METHOD, SUCCEEDED } = require("../combase/defines")
const { errno, EnumPropId, EnumTypeId, EnumCallFlags } = require("../combase/enums")
const { AdminCmd, CIoReqMessage } = require("../combase/iomsg")
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

function LoginRequestCb_OAuth2( oContext, oResp )
{
    var oUserCb = oContext.m_oUserCb
    var ret = oResp.GetProperty(
        EnumPropId.propReturnValue)
    if( ret === null )
        ret = errno.ERROR_FAIL;
    if( !oUserCb )
        return
    if( SUCCEEDED( ret ))
    {
        var oEncrypted = oResp.GetProperty( 0 )

    }
    oUserCb( ret )
    if( ERROR( ret ))
        return

    var strHash = oResp.GetProperty(
        EnumPropId.propSessHash )
    if( !strHash )
        return
    UpdateSessHash.bind( this )( strHash )
    return
}

function bigintToUint8Array(bigInt) {
  let hex = bigInt.toString(16);
  if (hex.length % 2) {
    hex = '0' + hex;
  }
  const len = hex.length / 2;
  const u8 = new Uint8Array(len);
  for (let i = 0, j = 0; i < len; i++, j += 2) {
    u8[i] = parseInt(hex.slice(j, j + 2), 16);
  }
  return u8;
}

function LoginRequestCb_SimpAuth( oContext, oResp )
{
    var oUserCb = oContext.m_oUserCb
    var ret = oResp.GetProperty(
        EnumPropId.propReturnValue)
    if( ERROR( ret ) )
    {
        ret = errno.ERROR_FAIL;
        if( oUserCb )
            oUserCb( ret )
        return Promise.resolve( ret )
    }

    var qwTs = oContext.m_oToken.GetProperty(
        EnumPropId.propTimestamp )
    var strUser =
        oContext.m_oToken.GetProperty( EnumPropId.propUserName )
    var strSess = 
        oContext.m_oToken.GetProperty( EnumPropId.propSessHash )
    qwTs = BigInt( Number( qwTs ) + 1 )
    var qwTsBuf = bigintToUint8Array( qwTs )
    if( qwTsBuf.length < 8 )
        qwTsBuf = Buffer.concat( [ Buffer.alloc( 8 - qwTsBuf.length, 0 ), qwTsBuf ] )

    var tokenBuf = Buffer.concat(
        [ qwTsBuf, Buffer.from( strUser, 'utf8' ), Buffer.from( strSess, 'utf8' ) ] )

    var oSvrInfo = oResp.GetProperty( 0 )
    var oSvrToken = oSvrInfo.GetProperty( 0 )
    var iv = oSvrToken.slice( 0, 12)

    const pRawKey = Buffer.from(this.m_strKey, 'hex')
    return crypto.subtle.importKey(
        "raw", pRawKey, { name: "AES-GCM" }, false, ["encrypt", "decrypt"]).then((pKey) => {
        return crypto.subtle.decrypt( {name: "AES-GCM", iv:iv }, pKey, oSvrToken.slice( 12 ) )
            .then((decrypted) => {
                if( tokenBuf.compare( Buffer.from( decrypted ) ) !== 0 )
                {
                    if( oUserCb)
                        oUserCb( -errno.EACCES )
                    return Promise.resolve( -errno.EACCES )
                }
                var strHash = oResp.GetProperty(
                    EnumPropId.propSessHash )
                if( !strHash )
                {
                    ret = -errno.EACCES
                    oResp.SetUint32( propReturnValue, ret )
                }
                else
                {
                    UpdateSessHash.bind( this )( strHash )
                }
                for( var i = 0; i < this.m_strKey.length; i++ )
                    this.m_strKey[i] = ' '
                if( oUserCb )
                    oUserCb( ret )
                return Promise.resolve( ret )
            }).catch((err) => {
                console.error("Login failed:", err);
                if( oUserCb )
                    oUserCb( -errno.EACCES )
                return Promise.resolve( -errno.EACCES )
            })
    }).catch((err) => {
        console.error("Key import failed:", err);
        if( oUserCb )
            oUserCb( -errno.EACCES )
        return Promise.resolve( -errno.EACCES )
    });
}

function LoginRequestCb( oContext, oResp )
{
    if( globalThis.g_strAuthMech === "OAuth2" )
    {
        return LoginRequestCb_OAuth2.bind( this )(
            oContext, oResp )
    }
    else if( globalThis.g_strAuthMech === "SimpAuth" )
    {
        return LoginRequestCb_SimpAuth.bind( this )(
            oContext, oResp )
    }
}

/**
 * LoginRequest will ask the server to login with the token available
 * @param {function}oUserCallback the callback to receive the login status code
 * @returns {Promise}
 * @api public
 */
function LoginRequest_OAuth2( oUserCallback )
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
        SYS_METHOD("AuthReq_Login"))
    var strCookie = getCookie( 'rpcf_code' )
    if( strCookie === null )
        return Promise.resolve( -errno.EACCES )
    var oInfo = new CConfigDb2()
    oInfo.Push( {t: EnumTypeId.typeString, v: strCookie })

    oReq.Push( {t: EnumTypeId.typeObj, v: oInfo })
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

function LoginRequest_SimpAuth( oUserCallback, oHsInfo, oCred )
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
        SYS_METHOD("AuthReq_Login"))

    var strUser = this.m_strUserName
    var oInfo = new CConfigDb2()
    oInfo.SetBool( EnumPropId.propGmSSL, false )
    oInfo.SetString( EnumPropId.propUserName, strUser )

    var oToken = new CConfigDb2()
    oToken.SetString( EnumPropId.propSessHash,
        oHsInfo.GetProperty( EnumPropId.propSessHash ) )
    oToken.SetUint64( EnumPropId.propTimestamp,
        oHsInfo.GetProperty( EnumPropId.propTimestamp ) )
    oToken.SetString( EnumPropId.propUserName, strUser )

    oReq.Push( {t: EnumTypeId.typeObj, v: oInfo })
    // oInfo.Push( {t: EnumTypeId.typeObj, v: oToken })
    var ciphertxt = oToken.Serialize()

    const iv = crypto.getRandomValues(new Uint8Array(12));
    const pRawKey = Buffer.from(this.m_strKey, 'hex')
    return crypto.subtle.importKey(
        "raw", pRawKey, { name: "AES-GCM" }, false, ["encrypt", "decrypt"] ).then((pKey) => {
        return crypto.subtle.encrypt( {name: "AES-GCM", iv:iv},
            pKey, ciphertxt ).then((encrypted) => {
            var oAesBuf = Buffer.concat(
                [ Buffer.from(iv), Buffer.from(encrypted) ] )
            oInfo.Push( {t: EnumTypeId.typeByteArr, v: oAesBuf } )
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
            oContext.m_oReq = oReq
            oContext.m_strSessHash = oHsInfo.GetProperty(
                EnumPropId.propSessHash )
            oContext.m_oToken = oToken

            return this.m_funcForwardRequest(
                oReq, LoginRequestCb, oContext )
        }).catch((err) => {
            console.error("Login failed:", err);
            return Promise.resolve( -errno.EACCES )
        })
    }).catch((err) => {
        console.error("Key import failed:", err);
        return Promise.resolve( -errno.EACCES )
    });
}

function LoginRequest( oUserCallback, oResp )
{
    if( globalThis.g_strAuthMech === "OAuth2" )
    {
        return LoginRequest_OAuth2.bind( this )(
            oUserCallback )
    }
    else if( globalThis.g_strAuthMech === "SimpAuth" )
    {
        return LoginRequest_SimpAuth.bind( this )(
            oUserCallback, oResp )
    }
    return Promise.resolve( -errno.ENOTSUP ) 
}

exports.LoginLocal = LoginRequest
