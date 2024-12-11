const { CConfigDb2 } = require("../combase/configdb")
const { randomInt, ERROR, SUCCEEDED, NumberToUint32 } = require("../combase/defines")
const { constval, errno, EnumPropId, EnumProtoId, EnumFCState, EnumTypeId, EnumStmToken, } = require("../combase/enums")
const { CIoRespMessage, CIoEventMessage, IoEvent } = require("../combase/iomsg")
const { CIncomingPacket, COutgoingPacket, CPacketHeader } = require("./protopkt")

class CRpcStreamBase
{
    constructor( oParent )
    {
        this.m_iStmId = -1
        this.m_iPeerStmId = -1
        this.m_dwSeqNo = randomInt( 0xffffffff )
        this.m_dwAgeSec = 0
        this.m_oHeader = new CPacketHeader()
        this.m_oParent = oParent
        this.m_wProtoId = EnumProtoId.protoStream
        if( oParent.m_bCompress === false )
            this.m_oHeader.m_wFlags = 0

        this.SendBuf = (( oBuf )=>{
            var oHdr = this.m_oHeader
            oHdr.m_dwSeqNo = this.GetSeqNo()
            var oOut = new COutgoingPacket()
            oOut.SetHeader( oHdr )
            oOut.SetBufToSend( oBuf )

            // send
            return this.m_oParent.SendMessage( 
                oOut.Serialize() )
        }).bind(this)
    }

    SetStreamId( iStmId, wProtoId )
    {
        this.m_iStmId = iStmId
        this.m_wProtoId = wProtoId
        this.m_oHeader.m_iStmId = iStmId
        this.m_oHeader.m_wProtoId = wProtoId
    }

    GetStreamId()
    { return this.m_iStmId }

    GetProtoId()
    { return this.m_wProtoId }

    SetPeerStreamId( iPeerId )
    {
        this.m_iPeerStmId = iPeerId
        this.m_oHeader.m_iPeerStmId = iPeerId
    }

    GetPeerStreamId()
    { return this.m_iPeerStmId }

    AgeStream( dwSec )
    { this.m_dwAgeSec += dwSec }

    GetAge()
    { return this.m_dwAgeSec }

    Refresh()
    { this.m_dwAgeSec = 0 }

    GetSeqNo()
    { return this.m_dwSeqNo++ }

}

class CFlowControl
{
    constructor()
    {
        this.m_qwRxBytes = 0
        this.m_qwTxBytes = 0
        this.m_qwAckTxBytes = 0
        this.m_qwAckRxBytes = 0

        this.m_qwRxPkts = 0
        this.m_qwTxPkts = 0
        this.m_qwAckTxPkts = 0
        this.m_qwAckRxPkts = 0
        this.m_byFlowCtrl = 0
    }

    IsFlowCtrl()
    { return this.m_byFlowCtrl === 0 }

    IncTxBytes( dwBytes )
    {
        var ret = EnumFCState.fcsKeep
        if( dwBytes === 0 ||
            dwBytes > constval.MAX_BYTES_PER_TRANSFER)
            return ret
        var bAboveLastBytes = false
        var bAboveLastPkts = false
        var bAboveBytes = false
        var bAbovePkts = false

        if( this.m_qwTxBytes - this.m_qwAckTxBytes >=
            constval.STM_MAX_PENDING_WRITE )
            bAboveLastBytes = true

        if( this.m_qwTxPkts - this.m_qwAckTxPkts >=
            constval.STM_MAX_PACKETS_REPORT )
            bAboveLastPkts = true
        
        this.m_qwTxBytes += dwBytes
        this.m_qwTxPkts++

        if( this.m_qwTxBytes - this.m_qwAckTxBytes >=
            constval.STM_MAX_PENDING_WRITE )
            bAboveBytes = true

        if( this.m_qwTxPkts - this.m_qwAckTxPkts >=
            constval.STM_MAX_PACKETS_REPORT )
            bAbovePkts = true
        
        if( bAboveLastBytes !== bAboveBytes ||
            bAboveLastPkts !== bAbovePkts )
        {
            ret = this.IncFCCount()
        }
        return ret
    }

    IncRxBytes( dwBytes )
    {
        var ret = EnumFCState.fcsKeep
        if( dwBytes === 0 ||
            dwBytes > constval.MAX_BYTES_PER_TRANSFER)
            return ret
        var bAboveLastBytes = false
        var bAboveLastPkts = false
        var bAboveBytes = false
        var bAbovePkts = false

        if( this.m_qwRxBytes - this.m_qwAckRxBytes >=
            constval.STM_MAX_PENDING_WRITE )
            bAboveLastBytes = true

        if( this.m_qwRxPkts - this.m_qwAckRxPkts >=
            constval.STM_MAX_PACKETS_REPORT )
            bAboveLastPkts = true
        
        this.m_qwRxBytes += dwBytes
        this.m_qwRxPkts++

        if( this.m_qwRxBytes - this.m_qwAckRxBytes >=
            constval.STM_MAX_PENDING_WRITE )
            bAboveBytes = true

        if( this.m_qwRxPkts - this.m_qwAckRxPkts >=
            constval.STM_MAX_PACKETS_REPORT )
            bAbovePkts = true
        
        if( bAboveLastBytes !== bAboveBytes ||
            bAboveLastPkts !== bAbovePkts )
        {
            ret = EnumFCState.fcsReport
            this.m_qwAckRxBytes = this.m_qwRxBytes
            this.m_qwAckRxPkts = this.m_qwRxPkts
        }
        return ret
    }

    IncFCCount()
    {
        var ret = EnumFCState.fcsKeep
        this.m_byFlowCtrl++
        if( this.m_byFlowCtrl === 1 )
            ret = EnumFCState.fcsFlowCtrl
        return ret
    }

    DecFCCount()
    {
        var ret = EnumFCState.fcsKeep
        this.m_byFlowCtrl--
        if( this.m_byFlowCtrl === 0 )
            ret = EnumFCState.fcsLift
        return ret
    }

    OnReportInternal( qwAckTxBytes, qwAckTxPkts )
    {
        var ret = EnumFCState.fcsKeep
        if( qwAckTxBytes <= this.m_qwAckTxBytes ||
            qwAckTxPkts < this.m_qwAckTxPkts )
            return ret
        
        var bAboveLast = false
        var bAbove = false

        if( this.m_qwTxBytes - this.m_qwAckTxBytes >=                                                                                                                                                                              
            constval.STM_MAX_PENDING_WRITE ||                                                                                                                                                                                     
            this.m_qwTxPkts - this.m_qwAckTxPkts >=                                                                                                                                                                                
            constval.STM_MAX_PACKETS_REPORT )                                                                                                                                                                                     
            bAboveLast = true;                                                                                                                                                                                           
                                                                                                                                                                                                                         
        if( this.m_qwTxBytes - qwAckTxBytes >=                                                                                                                                                                                
            constval.STM_MAX_PENDING_WRITE ||                                                                                                                                                                                     
            this.m_qwTxPkts - qwAckTxPkts >=                                                                                                                                                                                  
            constval.STM_MAX_PACKETS_REPORT )                                                                                                                                                                                     
            bAbove = true;                                                                                                                                                                                               
                                                                                                                                                                                                                         
        this.m_qwAckTxBytes = qwAckTxBytes;                                                                                                                                                                                   
        this.m_qwAckTxPkts = qwAckTxPkts;                                                                                                                                                                                     
                                                                                                                                                                                                                         
        if( bAboveLast != bAbove )                                                                                                                                                                                       
            ret = this.DecFCCount();                                 
        return ret
    }

    OnFCReport( oReport )
    {
        var qwAckTxBytes = oReport.GetProperty(
            EnumPropId.propRxBytes )
        var qwAckTxPkts = oReport.GetProperty(
            EnumPropId.propRxPkts )
        return this.OnReportInternal(
            Number( qwAckTxBytes ),
            Number( qwAckTxPkts) );
    }

    GetReport()
    {
        var oReport = new CConfigDb2()
        oReport.SetUint64( EnumPropId.propRxBytes,
            this.m_qwRxBytes)
        oReport.SetUint64( EnumPropId.propRxPkts,
            this.m_qwRxPkts)
        return oReport
    }
}

class CRpcStream extends CRpcStreamBase
{
    constructor( oParent )
    {
        super( oParent )
        this.m_oFlowCtrl = new CFlowControl()
        this.m_hStream = null
        this.m_dwPingIntervalSec = 0
        this.m_iPingTimer = 0 
        this.m_iPongTimer = 0
        this.m_arrPendingBufs = [ Object(), ]
        this.m_oPendingReadReq = null
        this.m_arrPendingWriteReqs = []

        var originSend = this.SendBuf
        this.SendBuf = (( token, oBuf )=>{
            var ret = 0
            switch( token )
            {
            case EnumStmToken.tokData:
                {
                    if( !oBuf )
                        break
                    if( !this.CanSend() )
                    {
                        ret = errno.ERROR_FAIL
                        break
                    }
                    var hdr = Buffer.alloc( 5 )
                    hdr.writeUint8( token, 0 )
                    hdr.writeUint32BE( oBuf.length, 1 )
                    ret = originSend(
                        Buffer.concat( [hdr, oBuf ] ))
                    if( ERROR( ret ) )
                        break
                    var fcs = this.m_oFlowCtrl.IncTxBytes(
                        oBuf.length)
                    if( fcs !== EnumFCState.fcsFlowCtrl )
                        break
                    console.log( "Warning: follow-control activated")
                    // ret = errno.ERROR_QUEUE_FULL
                    break
                }
            case EnumStmToken.tokProgress:
                {
                    var hdr = Buffer.alloc( 5 )
                    hdr.writeUint8( token, 0 )
                    hdr.writeUint32BE( hdr.length, 1 )
                    ret = originSend(
                        Buffer.concat( [hdr, oBuf ] ))
                    break
                }
            case EnumStmToken.tokPing:
            case EnumStmToken.tokPong:
            case EnumStmToken.tokClose:
            case EnumStmToken.tokLift:
            case EnumStmToken.tokFlowCtrl:
                {
                    oBuf = Buffer.alloc(1)
                    oBuf.writeUint8( token )
                    ret = originSend( oBuf )
                    break
                }
            case EnumStmToken.tokError:
            default:
                {
                    ret = -errno.EINVAL
                    break
                }
            }
            return ret
        }).bind( this )
    }

    CanSend()
    { return this.m_oFlowCtrl.IsFlowCtrl() }

    SetPingInterval( dwIntervalSec)
    { this.m_dwPingIntervalSec = dwIntervalSec }

    GetPingInterval()
    { return this.m_dwPingIntervalSec }

    EnqueuePayload( oPayload )
    { this.m_arrPendingBufs.push( oPayload ) }

    OutquePayload()
    { return this.m_arrPendingBufs.shift() }

    GetHeadPayload()
    {
        if( this.m_arrPendingBufs.length === 0 )
            return undefined
        return this.m_arrPendingBufs[0]
    }

    ReceiveBuf( oBuf )
    {
        var ret = errno.STATUS_SUCCESS
        var token = oBuf.readUint8( 0 )
        switch( token )
        {
        case EnumStmToken.tokData:
            {
                var dwSize = oBuf.readUint32BE( 1 )        
                if( dwSize > oBuf.length - 5 )
                {
                    ret = -errno.ERANGE
                    break
                }
                var oPayload = oBuf.slice( 5, 5 + dwSize )
                if( oPayload.length >
                    constval.STM_MAX_PENDING_WRITE )
                {
                    ret = -errno.E2BIG
                    break
                }
                var fcs = this.m_oFlowCtrl.IncRxBytes(
                    oPayload.length )
                if( fcs != EnumFCState.fcsReport)
                    this.EnqueuePayload( oPayload )
                else
                {
                    var oReport = this.m_oFlowCtrl.GetReport()
                    this.EnqueuePayload( [oPayload, oReport] )
                }
                if( this.m_arrPendingBufs.length === 1 )
                    this.PassHeadToClient()
                break
            }
        case EnumStmToken.tokProgress:
            {
                var dwSize = oBuf.readUint32BE( 1 )        
                if( dwSize > oBuf.length - 5 )
                {
                    ret = -errno.ERANGE
                    break
                }
                var oPayload = oBuf.slice( 5, 5 + dwSize )
                oReport = new CConfigDb2()
                oReport.Deserialize( oPayload, 0 )
                var fcs = this.m_oFlowCtrl.OnFCReport( oReport )
                if( fcs === EnumFCState.fcsLift )
                    ret = this.ResumeWrite()
                break
            }
        case EnumStmToken.tokPong:
            {
                if( this.m_iPongTimer !== 0 )
                {
                    clearTimeout( this.m_iPongTimer )
                    this.m_iPongTimer = 0
                }
                if( this.m_iPingTimer != 0 )
                {
                    clearTimeout( this.m_iPingTimer )
                }
                this.m_iPingTimer = setTimeout(
                    this.PingTimerCb.bind( this ),
                    this.GetPingInterval() * 1000 )
                break
            }
        case EnumStmToken.tokError:
            {
                ret = oBuf.readUint32BE( 1 )
                // fall through
            }
        case EnumStmToken.tokClose:
        case EnumStmToken.tokLift:
        case EnumStmToken.tokFlowCtrl:
        case EnumStmToken.tokPing:
        default:
            {
                if( token != EnumStmToken.tokError &&
                    token != EnumStmToken.tokClose )
                    token = EnumStmToken.tokInvalid
                if( this.m_hStream === null )
                {
                    ret = errno.ERROR_STATE
                    break
                }
                if( this.m_iPongTimer !== 0 )
                {
                    clearTimeout( this.m_iPongTimer )
                    this.m_iPongTimer = 0
                }
                if( this.m_iPingTimer != 0 )
                {
                    clearTimeout( this.m_iPingTimer )
                    this.m_iPingTimer = 0
                }
                var oMsg = new CIoEventMessage()
                oMsg.m_iCmd = IoEvent.StreamClosed[0]
                oMsg.m_dwPortId = this.m_oParent.GetPortId()
                oMsg.m_iMsgIdx = globalThis.g_iMsgIdx++
                oMsg.m_oReq.Push({t: EnumTypeId.typeUInt64, v: BigInt( this.m_hStream )} )
                oMsg.m_oReq.Push({t: EnumTypeId.typeByte, v: token} )
                if( token === EnumStmToken.tokError )
                    oMsg.m_oReq.Push({t: EnumTypeId.typeUInt32, v: ret} )
                this.m_oParent.PostMessage( oMsg )
                ret = -errno.ESHUTDOWN
                break
            }
        }
        return ret
    }

    ReceivePacket( oInPkt )
    {
        if( oInPkt.m_oBuf === null ||
            oInPkt.m_oBuf === undefined )
            return
        // post the data to the main thread
        this.ReceiveBuf( oInPkt.m_oBuf )
    }

    SendClose()
    { return this.SendBuf( EnumStmToken.tokClose)}

    PongTimerCb()
    {
        var oBuf = Buffer.alloc( 5 )
        oBuf.writeUint8( EnumStmToken.tokError)
        oBuf.writeUint32BE(
            NumberToUint32( -errno.ETIMEDOUT ), 1 )
        this.ReceiveBuf( oBuf )
    }

    SendPing()
    {
        var ret = this.SendBuf( EnumStmToken.tokPing )
        if( SUCCEEDED( ret ))
        {
            if( this.m_iPongTimer !== 0 )
                clearTimeout( this.m_iPongTimer )
            this.m_iPongTimer = setTimeout(
                this.PongTimerCb.bind(this),
                this.GetPingInterval() * 1000 )
        }
        return ret
    }

    SendReport( oReport )
    {
        var oBuf = oReport.Serialize()
        return this.SendBuf(
            EnumStmToken.tokProgress, oBuf )
    }

    PingTimerCb()
    {
        this.SendPing()
    }

    Start()
    {
        this.m_oParent.AddStream( this )
    }

    Stop()
    {
        if( this.m_iPingTimer !== 0 )
            clearTimeout( this.m_iPingTimer )
        if( this.m_iPongTimer !== 0 )
            clearTimeout( this.m_iPongTimer )
        this.SendClose()
        if( this.m_hStream !== null )
            this.m_oParent.RemoveStreamByHandle( this.m_hStream )
        else 
            this.m_oParent.RemoveStreamById( this.m_iStmId )
    }
    ResumeWrite()
    {
        if( this.m_arrPendingWriteReqs.length === 0 )
            return 0
        var ret = 0;
        while( this.m_arrPendingWriteReqs.length > 0)
        {
            ret = this.SendHeadReq()
            if( ret === errno.STATUS_PENDING )
                break;
            var oMsg = this.m_arrPendingWriteReqs.shift()[0]
            var oResp = new CIoRespMessage( oMsg )
            oResp.m_oResp.SetUint32(
                EnumPropId.propReturnValue, ret)
            this.m_oParent.PostMessage( oResp )
        }
        return ret;
    }

    SendHeadReq()
    {
        var ret = errno.STATUS_SUCCESS
        var oMsg = this.m_arrPendingWriteReqs[0][0]
        var offset = this.m_arrPendingWriteReqs[0][1]
        var oBuf = oMsg.m_oReq.GetProperty( 1 )
        while( offset < oBuf.length )
        {
            var dwSize = oBuf.length - offset >
                constval.STM_MAX_BYTES_PER_BUF ?
                constval.STM_MAX_BYTES_PER_BUF : oBuf.length - offset

            var iRet = this.SendBuf( EnumStmToken.tokData,
                oBuf.slice( offset, offset + dwSize ) )

            if( iRet === errno.ERROR_FAIL ||
                iRet === errno.ERROR_QUEUE_FULL )
            {
                this.m_arrPendingWriteReqs[0][1] = offset
                ret = errno.STATUS_PENDING
                break
            }
            if( dwSize === oBuf.length - offset )
            {
                ret = errno.STATUS_SUCCESS
                break
            }
            else
            {
                offset += dwSize
            }
        }
        return ret
    }

    PassHeadToClient( )
    {
        var oPayload = this.GetHeadPayload()
        if( oPayload === undefined )
            return
        var oBuf, oReport
        if( !Array.isArray( oPayload ) )
        {
            oBuf = oPayload
        }
        else
        {
            oBuf = oPayload[0]
            oReport = oPayload[1]
        }
        var oMsg = new CIoEventMessage()
        oMsg.m_iCmd = IoEvent.StreamRead[0]
        oMsg.m_iMsgIdx = globalThis.g_iMsgIdx++
        oMsg.m_dwPortId = this.m_oParent.GetPortId()
        oMsg.m_oReq.Push({t: EnumTypeId.typeUInt64,
            v: BigInt( this.m_hStream )} )
        oMsg.m_oReq.Push({t: EnumTypeId.typeByteArr, v: oBuf} )
        this.m_oParent.PostMessage( oMsg )
        if( oReport )
            this.SendReport( oReport )
    }
}
exports.CRpcStreamBase = CRpcStreamBase
exports.CRpcStream = CRpcStream

function DataConsumed( oReqMsg )
{
    var hStream = oReqMsg.m_oReq.GetProperty( 0 )
    var oStm = this.GetStreamByHandle( hStream )
    if( !oStm )
        return
    var oPayload = oStm.OutquePayload()
    if( oPayload === undefined )
        return
    oStm.PassHeadToClient()
    return
}
exports.Bdge_DataConsumed = DataConsumed
