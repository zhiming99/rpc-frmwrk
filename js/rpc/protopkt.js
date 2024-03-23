const { EnumPktFlag } = require("../combase/enums.js")

const CV = require( "../combase/enums.js" ).constval
const Buffer = require( "buffer").Buffer
require( "./lz4" )

const encodeBound = globalThis.encodeBound
const encodeBlock = globalThis.encodeBlock
const decodeBlock = globalThis.decodeBlock

class CPacketHeader
{
    constructor()
    {
        var u8 = new Uint8Array([0x50, 0x48, 0x64, 0x72])
        var arrUint32 = new Uint32Array(u8.buffer)
        this.m_dwMagic = arrUint32[0]
        this.m_dwSize = 0
        this.m_iStmId = 0
        this.m_iPeerStmId = 0
        if( encodeBound !== undefined )
            this.m_wFlags = EnumPktFlag.flagCompress 
        else
            this.m_wFlags = 0
        this.m_wProtoId = 0
        this.m_dwSessId = 0
        this.m_dwSeqNo = 0
    }
    static GetSeriSize()
    { return 28 }

    Serialize()
    {
        var oBuf = Buffer.alloc( CPacketHeader.GetSeriSize() )
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
        var dwMagic = srcBuf.readUInt32BE( offset )
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
exports.CPacketHeader = CPacketHeader
class CCarrierPacket
{
    constructor()
    {
        this.m_oBuf = null
        this.m_oHeader = new CPacketHeader()
    }

    GetStreamId()
    { return this.m_oHeader.m_iStmId }

    GetPeerStreamId()
    { return this.m_oHeader.m_iPeerStmId }

    GetProtoId()
    { return this.m_oHeader.m_wProtoId }

    GetSeqNo()
    { return this.m_oHeader.m_dwSeqNo }

    GetPayload()
    { return this.m_oBuf }

    SetPayload( oBuf )
    {
        this.m_oBuf = oBuf
        this.m_oHeader.m_dwSize = oBuf.length
    }

    GetFlags()
    { return this.m_oHeader.m_wFlags }

    GetHeader()
    { return this.m_oHeader }

    static GetSeriSize()
    { return 28 + m_oBuf.length }

    Serialize()
    {
        var hdrBuf = this.m_oHeader.Serialize()
        return Buffer.concat([hdrBuf, this.m_oBuf ])
    }

    Deserialize( srcBuf, offset )
    {
        if( srcBuf.length - offset < CPacketHeader.GetSeriSize() )
            return -1
        var pos = this.m_oHeader.Deserialize( srcBuf, offset )
        if( pos < 0 )
            throw new Error( "Error deserializing packet")

        if( srcBuf.length - pos < this.m_oHeader.m_dwSize )
            return -1

        this.m_oBuf = srcBuf.slice(
            pos, pos + this.m_oHeader.m_dwSize )

        return pos + this.m_oHeader.m_dwSize
    }
}

class COutgoingPacket extends CCarrierPacket 
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
        this.m_oBuf = oPayload
        this.m_oHeader.m_dwSize = oPayload.length
        if( oPayload.length <= 8 )
            this.m_oHeader.m_wFlags = 0;
    }

    SetHeader( oHeader )
    {
        this.m_oHeader.m_iStmId = oHeader.m_iStmId
        this.m_oHeader.m_iPeerStmId = oHeader.m_iPeerStmId
        this.m_oHeader.m_wFlags = oHeader.m_wFlags
        this.m_oHeader.m_dwSeqNo = oHeader.m_dwSeqNo
        this.m_oHeader.m_wProtoId = oHeader.m_wProtoId
        this.m_oHeader.m_dwSessId = oHeader.m_dwSessId
    }

    Serialize()
    {
        if( this.m_oBuf.length === 0 )
            return null

        if( ( this.m_oHeader.m_wFlags & EnumPktFlag.flagCompress ) === 0 )
            return super.Serialize()

        var compressed = Buffer.alloc(
            encodeBound( this.m_oBuf.length ) )
        var byteOutput = encodeBlock( this.m_oBuf, compressed )
        if( byteOutput === 0 )
        {
            console.log("warning: compress does not work properly")
            this.m_oHeader.m_wFlags = 0
            return super.Serialize()
        }

        var bufSize = Buffer.alloc( 4 )
        bufSize.writeUint32BE( this.m_oBuf.length )
        var backBuf = this.m_oBuf
        this.m_oBuf = Buffer.concat( [bufSize, compressed.slice( 0, byteOutput )] )
        this.m_oHeader.m_dwSize = byteOutput + 4

        var output = super.Serialize()

        this.m_oBuf = backBuf
        this.m_oHeader.m_dwSize = backBuf.length

        return output
    }

    Send()
    {
        var oSock = globalThis.g_oSock
        if( oSock.readyState !== 1 )
            throw new Error( "Error websocket is not ready")
        oSock.send( this.Serialize() )
    }
}
exports.COutgoingPacket = COutgoingPacket
class CIncomingPacket extends CCarrierPacket 
{
    constructor()
    { super() }

    Deserialize( srcBuf, offset )
    {
        var pos = super.Deserialize( srcBuf, offset )
        if( pos < 0 )
            return -1

        if( this.m_oBuf.length === 0 )
            return pos

        if( this.m_oHeader.m_wFlags & EnumPktFlag.flagCompress )
        {
            var actSize = this.m_oBuf.readUInt32BE(0);
            if( actSize > CV.MAX_BYTES_PER_TRANSFER )
                throw new Error( "Error invalid message size " + actSize )
            var output = Buffer.alloc( actSize )
            var byteOutput = decodeBlock( this.m_oBuf.slice( 4 ), output )
            if( byteOutput < 0 )
                throw new Error( "Error decompress incoming packets " + byteOutput )
            this.m_oBuf = output.slice( 0, byteOutput )
            this.m_oHeader.m_dwSize = this.m_oBuf.length
        }
        return pos
    }
}
exports.CIncomingPacket = CIncomingPacket
