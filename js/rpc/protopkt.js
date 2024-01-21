const CV = require( "../combase/enums.js" ).constval
const Buffer = require( "buffer").Buffer


class CPacketHeader
{
    constructor()
    {
        u8 = new Uint8Array(['P', 'H', 'd', 'r'])
        this.m_dwMagic = new Uint32Array( u8 )[0]
        this.m_dwSize = 0
        this.m_iStmId = 0
        this.m_iPeerStmId = 0
        this.m_wFlags = 0
        this.m_wProtoId = 0
        this.m_dwSessId = 0
        this.m_dwSeqNo = 0
    }
    static GetSeriSize()
    { return 28 }

    Serialize()
    {
        oBuf = Buffer.alloc( GetSeriSize() )
        var pos = 0
        oBuf.writeUint32BE( this.m_dwMagic, pos )
        pos += 4
        oBuf.writeUint32BE( this.m_dwSize, pos )
        pos += 4
        oBuf.writeUint32BE( this.m_iStmId, pos )
        pos += 4
        oBuf.writeUint32BE( this.m_iPeerStmId, pos )
        pos += 4
        oBuf.writeUint16BE( this.m_wFlags, pos )
        pos += 2
        oBuf.writeUint16BE( this.m_wProtoId, pos )
        pos += 2
        oBuf.writeUint32BE( this.m_dwSessId, pos )
        pos += 4
        oBuf.writeUint32BE( this.m_dwSeqNo, pos )
        return oBuf
    }

    Deserialize( srcBuf, offset )
    {
        if( srcBuf.length - offset <
            CPacketHeader.GetSeriSize() )
            return -1
        dwMagic = srcBuf.readUInt32BE( offset )
        if( this.m_dwMagic !== dwMagic )
            throw new Error( "Error invalid packets magic")
        offset += 4
        this.m_dwSize = srcBuf.readUInt32BE( offset )
        offset += 4
        if( this.m_dwSize > CV.MAX_BYTES_PER_TRANSFER )
            throw new Error( "Error bad packet length")
        this.m_iStmId = srcBuf.readUInt32BE( offset )
        offset += 4
        this.m_iPeerStmId = srcBuf.readUInt32BE( offset )
        offset += 4
        this.m_wFlags = srcBuf.readUInt16BE( offset ) 
        offset += 2
        this.m_wProtoId = srcBuf.readUInt16BE( offset ) 
        offset += 2
        this.m_dwSessId = srcBuf.readUInt32BE( offset )
        offset += 4
        this.m_dwSeqNo = srcBuf.readUInt32BE( offset )
        offset += 4
        return offset
    }
}

class CCarrierPacket
{
    constructor()
    {
        this.m_oBuf = null
        this.m_oHeader = new CPacketHeader()
    }

    GetStreamId()
    { return this.m_oHeader.m_iStmId }

    GetPeerStmId()
    { return this.m_oHeader.m_iPeerStmId }

    GetProtoId()
    { return this.m_oHeader.m_wProtoId }

    GetSeqNo()
    { return this.m_oHeader.m_dwSeqNo }

    GetPayload()
    { return this.m_oBuf }

    SetPayload( oBuf )
    { this.m_oBuf = oBuf }

    GetFlags()
    { return this.m_oHeader.m_wFlags }

    GetHeader()
    { return this.m_oHeader }

    static GetSeriSize()
    { return 28 + m_oBuf.length }

    Serialize()
    {
        hdrBuf = this.m_oHeader.Serialize()
        return Buffer.concat([hdrBuf, this.m_oBuf ])
    }

    Deserialize( srcBuf, offset )
    {
        if( srcBuf.length - offset < this.m_oHeader.GetSeriSize() )
            return -1
        pos = this.m_oHeader.Deserialize( srcBuf, offset )
        if( pos < 0 )
            throw new Error( "Error deserializing packet")

        if( srcBuf.length - pos < this.m_oHeader.m_dwSize )
            return -1

        this.m_oBuf = srcBuf.slice(
            pos, pos + this.m_oHeader.m_dwSize )

        return pos + this.m_oHeader.m_dwSize
    }

}

exports.COutgoingPacket = class COutgoingPacket extends CCarrierPacket 
{
    constructor()
    { super() }

    static GetSeriSize()
    { return super.GetSeriSize() }

    SetBufToSend( oPayload )
    {
        if( oPayload === null ||
            oPayload.length === 0 )
            throw new Error( "Error Invalid Buffer")
        super.m_oBuf = oPayload
        super.m_oHeader.m_dwSize = oPayload.length
    }

    SetHeader( oHeader )
    {
        super.m_oHeader.m_iStmId = oHeader.m_iStmId
        super.m_oHeader.m_iPeerStmId = oHeader.m_iPeerStmId
        super.m_oHeader.m_wFlags = oHeader.m_wFlags
        super.m_oHeader.m_dwSeqNo = oHeader.m_dwSeqNo
        super.m_oHeader.m_wProtoId = oHeader.m_wProtoId
        super.m_oHeader.m_dwSessId = oHeader.m_dwSessId
    }

    Send()
    {
        oSock = globalThis.g_oSock
        if( oSock.readyState !== 1 )
            throw new Error( "Error websocket is not ready")
        oSock.send( super.Serialize() )
    }
}

exports.CIncomingPacket = class CIncomingPacket extends CCarrierPacket 
{
    constructor()
    {
        super()
        this.m_dwBytesToRecv = 0
        this.m_dwOffset = 0
    }

    Receive( srcBuf, offset )
    {
        pos = super.Deserialize( srcBuf, offset )
        if( pos < 0 )
            return -1
        return pos - offset
    }
}