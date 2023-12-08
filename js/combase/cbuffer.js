import { EnumClsid, EnumTypeId } from "./enums";
import { CObjBase } from "./objbase"
// import { Buffer } from "node:buffer"
import { Buffer } from "../dbusmsg/test/node_modules/buffer"
import { marshall } from "../dbusmsg/message";

exports.CBuffer = class CBuffer extends CObjBase
{
    constructor()
    {
        this.m_dwClsid = EnumClsid.CBuffer;
        this.m_dwOffset = 0;
        this.m_arrBuf = new Buffer( 64 );
        this.m_dwTail = this.m_arrBuf.length;
        this.m_dwType = EnumTypeId.typeByteArr;
        this.m_value = null
    }

    get size()
    {
        if( m_dwType != EnumTypeId.typeByteArr )
            return 0
        return this.m_arrBuf.length
    }

    get offset()
    {
        if( m_dwType != EnumTypeId.typeByteArr )
            return 0
        return this.m_dwOffset;
    }

    // return an arraybuffer with encoded string
    static StrToBuffer( str )
    {
        const encoder = new TextEncoder();
        const encodedData = encoder.encode(str);
        return encodedData.buffer;
    }

    static BufferToStr( buf )
    {
        const decoder = new TextDecoder();
        return decoder.decode(new Buffer(buf) );
    }

    Resize( dwNewSize )
    {
        do{
            this.m_dwType = EnumTypeId.typeByteArr;
            if( dwNewSize === this.size )
                break
            else if( dwNewSize < this.size )
            {
                this.m_dwTail = this.m_dwOffset + dwNewSize;
            }
            else
            {
                newBuf = new Buffer( dwNewSize ).set(
                    this.m_arrBuf.slice(
                        this.m_dwOffset, this.m_dwTail ) )
                this.m_arrBuf = newBuf
                this.m_dwOffset = 0
                this.m_dwTail = newBuf.length
            }
        }while( 0 )
        return this
    }

    Serialize()
    {
        switch( this.m_dwType )
        {
        case EnumTypeId.typeByteArr:
            dstBuffer = this.m_arrBuf
            break
        case EnumTypeId.typeByte:
            newBuf = new Buffer(1)
            newBuf[0] = this.m_value
            dstBuffer = newBuf
            break
        case EnumTypeId.typeUInt16 :
            {
                newBuf = new Buffer(2)
                const view = new DataView( newBuf.buffer )
                view.setUint16( 0, this.m_value )
                dstBuffer = newBuf
                break
            }
        case EnumTypeId.typeUInt32 :
            {
                newBuf = new Buffer(4)
                const view = new DataView( newBuf.buffer )
                view.setUint32( 0, this.m_value )
                dstBuffer = newBuf
                break
            }
        case EnumTypeId.typeUInt64 :
            {
                newBuf = new Buffer(8)
                const view = new DataView( newBuf.buffer )
                view.setBigUint64( 0, this.m_value )
                dstBuffer = newBuf
            }
            break
        case EnumTypeId.typeFloat :
            {
                newBuf = new Buffer(4)
                const view = new DataView( newBuf.buffer )
                view.setFloat32( 0, this.m_value )
                dstBuffer = newBuf
                break
            }
        case EnumTypeId.typeDouble :
            {
                newBuf = new Buffer(8)
                const view = new DataView( newBuf.buffer )
                view.setFloat64( 0, this.m_value )
                dstBuffer = newBuf
                break
            }
        case EnumTypeId.typeString :
            {
                dstBuffer = CBuffer.StrToBuffer(
                    this.m_value )
                break
            }
        case EnumTypeId.typeDMsg:
            {
                dstBuffer = marshall( this.m_value )
                break
            }
        case EnumTypeId.typeObj:
            {
                this.m_value.Serialize( dstBuffer )
                break
            }
        default:
            throw new TypeError(`Error unknown type id ${this.m_dwTypeId}`)
        }
        return dstBuffer
    }

    Append( newBuf )
    {
        if( newBuf.constructor.name !== "Buffer" )
            throw new TypeError( "Error type of input param, expecting Buffer")

        if( this.m_dwType === EnumTypeId.typeNone )
        {
            this.m_arrBuf = newBuf
            this.m_dwType = EnumTypeId.typeByteArr
            return
        }
        else if( this.m_dwType != EnumTypeId.typeByteArr )
        {
            throw new TypeError("Error not a bytearray")
        }
        this.m_arrBuf = Buffer.concat([this.m_arrBuf, newBuf], 2)
        return
    }
}