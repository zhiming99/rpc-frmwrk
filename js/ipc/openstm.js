const { CConfigDb2 } = require("../combase/configdb")
const { ERROR, SYS_METHOD, Int32Value } = require("../combase/defines")
const { EnumPropId, errno, EnumCallFlags, EnumClsid, EnumTypeId, EnumProtoId } = require("../combase/enums")
const { IoCmd, CPendingRequest } = require("../combase/iomsg")
const { messageType } = require( "../dbusmsg/constants")
const { CIoReqMessage } = require("../combase/iomsg")
const { DBusIfName } = require("../rpc/dmsg")
const { DBusDestination2 } = require("../rpc/dmsg")

/**
 * OpenStreamLocal will open a stream channel for read/write
 * @param {Object} oContext an object which will be returned after OpenStream is
 * completed
 * @returns {Promise<Object>} `oContext`, which would have a member `m_hStream`
 * to as the opened stream. The `m_hStream` can be passed as a parametr to
 * `OnDataReceived`, `OnStreamClosed, or be passed to proxy method
 * `StreamWrite`. the `oContext` also has a member `m_iRet` when error occurs.
 * @api public
 */
function OpenStreamLocal( oContext )
{
    return new Promise((resolve, reject)=>{

        var oReq = new CConfigDb2()

        var oCallOpts = new CConfigDb2()
        // this timeout is for the FetchData request
        oCallOpts.SetUint32( EnumPropId.propTimeoutSec,
            this.m_dwFetchDataTimeout)

        oCallOpts.SetUint32( EnumPropId.propKeepAliveSec,
            this.m_dwKeepAliveSec )

        oCallOpts.SetUint32( EnumPropId.propCallFlags,
            EnumCallFlags.CF_ASYNC_CALL |
            EnumCallFlags.CF_WITH_REPLY |
            messageType.methodCall )

        oReq.SetObjPtr(
            EnumPropId.propCallOptions, oCallOpts)
        oReq.SetString( EnumPropId.propDestDBusName,
            this.m_strDest )
        oReq.SetString( EnumPropId.propMethodName,
            SYS_METHOD( "FetchData"))
        oReq.SetString( EnumPropId.propObjPath,
            this.m_strObjPath )
        oReq.SetString( EnumPropId.propSrcDBusName,
            this.m_strSender )
        oReq.SetString( EnumPropId.propIfName,
            DBusIfName( "IStream") )
        oReq.SetUint32( EnumPropId.propIid,
            EnumClsid.IStream )
        var oTransctx = new CConfigDb2()
        oTransctx.SetString( EnumPropId.propRouterPath,
            this.m_strRouterPath )
        oReq.SetObjPtr( EnumPropId.propTransCtx,
            oTransctx )

        // this timeout is for the stream object
        var dwTimeout = this.m_dwTimeoutSec > 50 ?
            50 : this.m_dwTimeoutSec
        oReq.SetUint32( EnumPropId.propTimeoutSec,
            dwTimeout)
        oReq.SetUint32( EnumPropId.propKeepAliveSec,
            Math.floor( dwTimeout / 2 ) )

        oReq.Push({t: EnumTypeId.typeUInt32,
            v: EnumProtoId.protoStream})

        var oMsg = new CIoReqMessage()
        oMsg.m_iMsgId = globalThis.g_iMsgIdx++
        oMsg.m_iCmd = IoCmd.OpenStream[0]
        oMsg.m_dwPortId = this.GetPortId()
        oMsg.m_oReq = oReq

        var oPending = new CPendingRequest()
        oPending.m_oObject = this
        oPending.m_oReject = reject
        oPending.m_oResolve = resolve
        oPending.m_oReq = oMsg
        oPending.m_oContext = oContext
        this.PostMessage( oPending )
        oContext.m_qwTaskId = oMsg.m_iMsgId
    }).then((e)=>{
        do{
            var ret = 0
            if( e.m_oResp === undefined )
            {
                ret = -errno.EINVAL
                break
            }
            ret = e.m_oResp.GetProperty(
                EnumPropId.propReturnValue )
            if( ERROR( ret ) )
                break

            var oDesc = e.m_oResp.GetProperty( 0 )
            if( oDesc === null )
            {
                ret = -errno.EFAULT
                break
            }
            var hStream = oDesc.GetProperty(
                EnumPropId.propPeerObjId )
            if( hStream === null )
            {
                ret = -errno.EFAULT
                break
            }
            var oCtx = e.m_oContext
            oCtx.m_hStream = hStream
            oCtx.m_iRet = 0
            this.AddStream( hStream )
            // kick off the incoming stream
            this.m_funcNotifyDataConsumed( hStream )
            return Promise.resolve( e.m_oContext)
        }while( 0 )
        if( ERROR( ret ))
        {
            ret = -errno.EINVAL
            e.m_oResp = new CConfigDb2()
            e.m_oResp.SetUint32(
                EnumPropId.propReturnValue, ret )
            return Promise.reject( e )
        }
    }).catch((e)=>{
        var ret = e.m_oResp.GetProperty(
            EnumPropId.propReturnValue )
        this.DebugPrint( "Error OpenStreamLocal failed (" + Int32Value( ret ) + ")")
        e.m_oContext.m_iRet = ret
        return Promise.resolve( e.m_oContext )
    })
}

exports.OpenStreamLocal = OpenStreamLocal

function CloseStreamLocal( hStream )
{
    this.RemoveStream( hStream )
}
exports.CloseStreamLocal = CloseStreamLocal