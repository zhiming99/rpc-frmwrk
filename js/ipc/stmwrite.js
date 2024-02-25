const { CConfigDb2 } = require("../combase/configdb")
const { ERROR, SYS_METHOD, Int32Value } = require("../combase/defines")
const { EnumPropId, errno, EnumCallFlags, EnumClsid, EnumTypeId, EnumProtoId, constval } = require("../combase/enums")
const { IoCmd, CPendingRequest, AdminCmd, CIoEventMessage } = require("../combase/iomsg")
const { CIoReqMessage } = require("../combase/iomsg")
/**
 * StreamWrite will write a buffer to the stream `hStream`
 * @param {number} hStream the stream channel to write to
 * @param {Buffer} oBuf  the byte block to write
 * @returns {number} on Success the return value is errno.STATUS_SUCCESS, otherwise an error returned
 * @api public
 */
function StreamWrite( hStream, oBuf )
{
    return new Promise( (resolve, reject)=>{
        var oMsg = new CIoReqMessage()
        oMsg.m_iCmd = IoCmd.StreamWrite[0]
        oMsg.m_iMsgId = globalThis.g_iMsgIdx++
        oMsg.m_dwPortId = this.GetPortId()
        oMsg.m_oReq.Push( {t: EnumTypeId.typeUInt64, v: hStream })
        oMsg.m_oReq.Push( {t: EnumTypeId.typeByteArr, v: oBuf })
        var oPending = new CPendingRequest()
        oPending.m_oReq = oMsg
        oPending.m_oReject = reject
        oPending.m_oResolve = resolve
        oPending.m_oObject = this
        oPending.m_oResp = new CConfigDb2()
        if( oBuf.length === 0 )
        {
            oPending.m_oResp.SetUint32(
                EnumPropId.propReturnValue, -errno.EINVAL )
            reject( oPending )
        }
        if( oBuf.lenght > constval.MAX_BYTES_PER_TRANSFER )
        {
            oPending.m_oResp.SetUint32(
                EnumPropId.propReturnValue, -errno.E2BIG )
            return reject( oPending )
        }
        this.PostMessage( oPending )
    }).then((oPending)=>{
        return Promise.resolve( errno.STATUS_SUCCESS )
    }).catch((oPending)=>{
        var oResp = oPending.m_oResp
        var ret = oResp.GetProperty( EnumPropId.propReturnValue )
        this.DebugPrint( `Error occurs when sending write request (${Int32Value(ret)})` )
        return Promise.resolve( ret )
    })
}

exports.StreamWriteLocal = StreamWrite