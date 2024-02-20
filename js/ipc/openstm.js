const { CConfigDb2 } = require("../combase/configdb")
const { ERROR, SYS_METHOD, Int32Value } = require("../combase/defines")
const { EnumPropId, errno, EnumCallFlags, EnumClsid, EnumTypeId, EnumProtoId } = require("../combase/enums")
const { IoCmd, CPendingRequest } = require("../combase/iomsg")
const { messageType } = require( "../dbusmsg/constants")
const { CIoReqMessage } = require("../combase/iomsg")
const { DBusIfName } = require("../rpc/dmsg")
const { DBusDestination2 } = require("../rpc/dmsg")

function OpenStreamLocal( oContext )
{
    return new Promise((resolve, reject)=>{

        var dwTimeout = this.m_dwFetchDataTimeout
        var oReq = new CConfigDb2()

        var oCallOpts = new CConfigDb2()
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
        /* oReq.SetUint32(
            EnumPropId.propTimeoutSec, dwTimeout)
        oReq.SetUint32( EnumPropId.propKeepAliveSec,
            Math.floor( dwTimeout / 2 ) )
        */

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
            this.m_setStreams.add( hStream )
            e.m_oContext.m_hStream = hStream
            e.m_oContext.m_iRet = 0
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
