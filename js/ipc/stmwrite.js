const { CConfigDb2 } = require("../combase/configdb")
const { ERROR, SYS_METHOD, Int32Value } = require("../combase/defines")
const { EnumPropId, errno, EnumCallFlags, EnumClsid, EnumTypeId, EnumProtoId, constval } = require("../combase/enums")
const { IoCmd, CPendingRequest, AdminCmd, CIoEventMessage } = require("../combase/iomsg")
const { CIoReqMessage } = require("../combase/iomsg")

function StreamWrite( hStream, oBuf )
{
    return new Promise( (resolve, reject)=>{
        var oMsg = CIoReqMessage()
        oMsg.m_iCmd = IoCmd.StreamWrite[0]
        oMsg.m_iMsgId = globalThis.g_iMsgIdx++
        oMsg.m_dwPortId = this.GetPortId()
        oMsg.m_oReq.Push( {t: EnumTypeId.typeUInt64, v: hStream })
        oMsg.m_oReq.Push( {t: EnumTypeId.typeByteArr, v: oBuf })
        oPending.m_oReq = oMsg
        var oPending = new CPendingRequest()
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
    }).then((e)=>{
        return Promise.resolve( errno.STATUS_SUCCESS )
    }).catch((oPending)=>{
        this.DebugPrint( `Error occurs when sending write request (${Int32Value(e)})` )
        var oResp = oPending.m_oResp
        var ret = oResp.GetProperty( EnumPropId.propReturnValue )
        return Promise.resolve( ret )
    })
}

exports.StreamWriteLocal = StreamWrite