const { CConfigDb2 } = require("../combase/configdb")
const { randomInt, ERROR, SUCCEEDED } = require("../combase/defines")
const { constval, errno, EnumPropId, EnumProtoId, EnumFCState, EnumTypeId, EnumStmToken, } = require("../combase/enums")
const { CPendingRequest, CIoEventMessage, IoEvent, CIoRespMessage } = require("../combase/iomsg")
const { CIncomingPacket, COutgoingPacket, CPacketHeader } = require("./protopkt")

function StreamWrite( oMsg )
{
    var ret = errno.STATUS_SUCCESS
    try{
        var hStream = oMsg.m_oReq.GetProperty( 0 )
        var oStm = this.GetStreamByHandle( hStream )
        if( !oStm )
        {
            ret = -errno.ENOENT
            return ret
        }
        if( oStm.m_arrPendingWriteReqs.length > 0 ||
            !oStm.CanSend() )
        {
            oStm.m_arrPendingWriteReqs.push( [ oMsg, 0])
            ret = errno.STATUS_PENDING
            return ret
        }
        if( oStm.m_arrPendingWriteReqs.length > 0 )
        {
            ret = errno.ERROR_STATE
            return ret
        }
        oStm.m_arrPendingWriteReqs.push( [ oMsg, 0])
        ret = oStm.SendHeadReq()
        if( ret === errno.STATUS_PENDING )
            return ret
        oStm.m_arrPendingWriteReqs.shift()
        return ret
    }
    catch(e) {
        
    }
    finally{
        if( ret !== errno.STATUS_PENDING )
        {
            var oResp = new CIoRespMessage( oMsg )
            oResp.m_oResp.SetUint32(
                EnumPropId.propReturnValue, ret)
            this.PostMessage( oResp )
        }
    }

}
exports.Bdge_StreamWrite = StreamWrite