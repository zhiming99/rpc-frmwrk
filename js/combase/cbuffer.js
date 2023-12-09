const { unmarshall } = require("../dbusmsg/message");
const { CConfigDb2 } = require("./configdb");
const { EnumTypeId, constval, EnumClsid } = require("./enums");

require( "./enums")
require( "./objbase" ).CObjBase
// import { Buffer } from "node:buffer"
require( "../dbusmsg/test/node_modules/buffer" ).Buffer
require( "../dbusmsg/message" ).marshall;
require( "../dbusmsg/message" ).unmarshall;
require ( "./defines" ).SERI_HEADER_BASE ;
const E = require ( 'syserrno' ).errors

class SERI_HEADER extends SERI_HEADER_BASE
{
    constructor()
    {
        super()
        this.dwType = EnumTypes.DataTypeMem 
        this.dwObjClsid = EnumClsid.Invalid
    }
    static GetSeriSize()
    { return SERI_HEADER_BASE.GetSeriSize() + 8 }
}
module.exports = class CBuffer extends CObjBase
{
    constructor()
    {
        this.m_dwClsid = EnumClsid.CBuffer;
        this.m_dwOffset = 0;
        this.m_arrBuf = null
        this.m_dwTail = 0
        this.m_dwTypeId = EnumTypeId.typeNone;
        this.m_value = null
    }

    get size()
    {
        if( this.type !== EnumTypeId.typeByteArr )
            return 0
        if( this.empty() )
            return 0
        return this.m_arrBuf.length
    }

    get offset()
    {
        if( this.type !== EnumTypeId.typeByteArr )
            return 0
        if( this.empty() )
            return 0
        return this.m_dwOffset;
    }

    set offset( dwOffset )
    {
        if( this.type !== EnumTypeId.typeByteArr )
            return
        if( this.empty() )
            return 0
        if( dwOffset < 0 || dwOffset > this.m_dwTail )
            return
        this.m_dwOffset = dwOffset
    }

    get tail()
    {
        if( this.type !== EnumTypeId.typeByteArr )
            return 0
        if( this.empty() )
            return 0
        return this.m_dwTail;
    }

    set tail( dwTail )
    {
        if( this.type !== EnumTypeId.typeByteArr )
            return
        if( this.empty() )
            return 0
        if( dwTail > this.m_arrBuf.length || dwTail <= 0 )
            return
        this.m_dwTail = dwTail
    }

    get buffer()
    {
        if( this.type !== EnumTypeId.typeByteArr )
            return null 
        if( this.empty() )
            return null 
        return this.m_arrBuf.buffer.slice(
            this.offset, this.m_dwTail )
    }

    get type()
    { return this.m_dwTypeId }

    set type( dwType )
    { this.m_dwTypeId = dwType }

    // return an arraybuffer with encoded string
    static StrToBuffer( str )
    {
        const encoder = new TextEncoder();
        const encodedData = encoder.encode(str);
        nullc = Buffer.alloc(1)
        nullc[0] = 0
        return Buffer.from( [encodedData, nullc] )
    }

    static BufferToStr( buf )
    {
        const decoder = new TextDecoder();
        return decoder.decode(buf);
    }

    Resize( dwNewSize )
    {
        do{
            if( dwNewSize === 0 )
            {
                this.m_arrBuf = null
                this.offset = 0
                this.m_dwTail = 0
            }
            else if( this.size === 0 )
            {
                this.m_arrBuf = Buffer.alloc( dwNewSize );
                this.offset = 0
                this.m_dwTail = newBuf.length
            }
            else if( dwNewSize === this.size )
            {
                break
            }
            else if( dwNewSize < this.size )
            {
                this.m_dwTail = this.offset + dwNewSize;
            }
            else
            {
                newBuf = Buffer.alloc( dwNewSize );
                if( this.empty() )
                    this.m_arrBuf = newBuf
                else
                    newBuf.set( this.buffer )
                        
                this.m_arrBuf = newBuf
                this.offset = 0
                this.m_dwTail = newBuf.length
            }
            this.type = EnumTypeId.typeByteArr;
        }while( 0 )
        return
    }

    Serialize()
    {
        var dstBuffer
        var header = Buffer.alloc( SERI_HEADER.GetSeriSize())
        var oHdrView = new DataView( header.buffer )
        dwType = EnumTypes.DataTypeMem 
        switch( this.type )
        {
        case EnumTypeId.typeByteArr:
            {
                dstBuffer = this.buffer
                break
            }
        case EnumTypeId.typeByte:
            {
                dstBuffer = Buffer.alloc(1)
                dstBuffer[0] = this.m_value
                break
            }
        case EnumTypeId.typeUInt16 :
            {
                dstBuffer = Buffer.alloc(2)
                const view = new DataView( dstBuffer.buffer )
                view.setUint16( 0, this.m_value )
                break
            }
        case EnumTypeId.typeUInt32 :
            {
                dstBuffer = Buffer.alloc(4)
                const view = new DataView( dstBuffer.buffer )
                view.setUint32( 0, this.m_value )
                break
            }
        case EnumTypeId.typeUInt64 :
            {
                dstBuffer = Buffer.alloc(8)
                const view = new DataView( dstBuffer.buffer )
                view.setBigUint64( 0, this.m_value )
            }
            break
        case EnumTypeId.typeFloat :
            {
                dstBuffer = Buffer.alloc(4)
                const view = new DataView( dstBuffer.buffer )
                view.setFloat32( 0, this.m_value )
                break
            }
        case EnumTypeId.typeDouble :
            {
                dstBuffer = Buffer.alloc(8)
                const view = new DataView( dstBuffer.buffer )
                view.setFloat64( 0, this.m_value )
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
                dwType = EnumTypes.DataTypeMsgPtr
                oHdrView.SetUint32( 16, this.m_value.type )
                break
            }
        case EnumTypeId.typeObj:
            {
                dstBuffer = this.m_value.Serialize()
                dwType = EnumTypes.DataTypeObjPtr 
                oHdrView.SetUint32( 16, this.m_value.GetClsid() )
                break
            }
        default:
            throw new TypeError(`Error unknown type id ${this.type}`)
        }
        oHdrView.setUint32( 0, EnumClsid.CBuffer )
        oHdrView.setUint32( 4, dstBuffer.length )
        oHdrView.setUint32( 8, 1, true )
        oHdrView.SetUint32( 12, dwType & ( this.type << 4 ))
        dstBuffer = Buffer.concat( header, dstBuffer )
        return dstBuffer
    }

    GetValue()
    {
        if( this.type === EnumTypeId.typeByteArr )
            return this.m_arrBuf
        return this.m_value
    }

    SetBool( val )
    {
        this.SetUint8( val )
    }
    SetUint8( val )
    {
        this.type = EnumTypeId.typeByte
        this.m_value = val
    }

    SetUint16( val )
    {
        this.type = EnumTypeId.typeUInt16
        this.m_value = val
    }

    SetUint32( val )
    {
        this.type = EnumTypeId.typeUInt32
        this.m_value = val
    }

    SetUint64( val )
    {
        this.type = EnumTypeId.typeUInt64
        this.m_value = val
    }

    SetFloat( val )
    {
        this.type = EnumTypeId.typeFloat
        this.m_value = val
    }

    SetDouble( val )
    {
        this.type = EnumTypeId.typeDouble
        this.m_value = val
    }

    SetMsgPtr( val )
    {
        this.type = EnumTypeId.typeDMsg
        this.m_value = val
    }

    SetObjPtr( val )
    {
        this.type = EnumTypeId.typeObj
        this.m_value = val
    }

    empty()
    {
        if( this.m_arrBuf === null ||
            this.m_dwOffset === this.m_dwTail )
            return true
        return false
    }

    Append( newBuf )
    {
        if( newBuf.constructor.name !== "Buffer" )
            throw new TypeError( "Error type of input param, expecting Buffer")

        if( this.type === EnumTypeId.typeNone )
        {
            this.m_arrBuf = newBuf
            this.type = EnumTypeId.typeByteArr
            return
        }
        else if( this.type !== EnumTypeId.typeByteArr )
        {
            throw new TypeError("Error not a bytearray")
        }

        if( this.empty() )
        {
            this.offset = 0
            this.m_dwTail = newBuf.length
            this.m_arrBuf = newBuf
        }
        else
        {
            this.m_arrBuf = Buffer.concat(
                [this.buffer, newBuf], 2)
        }
        return
    }

    Append2( newCBuf )
    {
        if( newCBuf.constructor.name !== "CBuffer" )
            throw new TypeError( "Error type of input param, expecting CBuffer")
        this.Append( newCBuf.buffer )
    }

    Clear()
    {
        this.type = EnumTypeId.typeNone
        this.m_arrBuf = null
        this.m_dwOffset = this.m_dwTail = 0
        this.m_value = null
    }

    DeserializeObj( ov, dwClsid )
    {
        offset = SERI_HEADER.GetSeriSize()
        dwSize = ov.getUint32( 4 )
        newObj = null
        switch( dwClsid )
        {
        case EnumClsid.CConfigDb2:
            {
                newObj = new CConfigDb2()
                break
            }
        case EnumClsid.CBuffer:
            {

            }
        default:
            throw new TypeError( "Error Unknown class id" )
        }
        this.m_value = newObj.Deserialize(
            ov.buffer.slice( offset, dwSize ) )
    }

    DeserializePrimeType( ov, dwTypeId )
    {
        offset = SERI_HEADER.GetSeriSize()
        dwSize = ov.getUint32( 4 )

        switch( dwTypeId )
        {
        case EnumTypeId.typeByteArr:
            {
                this.m_arrBuf = Buffer.from(
                    ov.buffer.slice( offset, dwSize ) )
                this.m_dwTail = this.m_arrBuf.length
                this.m_dwOffset = 0
                break
            }
        case EnumTypeId.typeByte:
            {
                this.m_value = ov.getUint8( offset, 1 )
                break
            }
        case EnumTypeId.typeUInt16:
            {
                this.m_value = ov.getUint16( offset, 2 )
                break
            }
        case EnumTypeId.typeUInt32:
            {
                this.m_value = ov.getUint32( offset, 4 )
                break
            }
        case EnumTypeId.typeUInt64:
            {
                this.m_value = ov.getUint64( offset, 8 )
                break
            }
        case EnumTypeId.typeFloat:
            {
                this.m_value = ov.getFloat32( offset, 4 )
                break
            }
        case EnumTypeId.typeDouble:
            {
                this.m_value = ov.getFloat64( offset, 8 )
                break
            }
        case EnumTypeId.typeString:
            {
                this.m_value =
                    BufferToStr( ov.buffer.slice( offset, dwSize ) )
                break
            }
        default:
            throw new TypeError( `Error unsupported typeid ${dwTypeId}`)
        }
        this.type = dwTypeId
    }

    Deserialize( srcBuffer )
    {
        if( newCBuf.constructor.name !== "Buffer" )
            throw new TypeError( "Error type of input param, expecting CBuffer")

        this.Clear()
        pos = 0
        ret = 0
        ov = new DataView( srcBuffer.buffer )
        offset = SERI_HEADER.GetSeriSize()
        do{
            val = ov.getUint32( pos )
            if( val !== EnumClsid.CBuffer )
            {
                ret = E.EINVAL.errno
                break
            }
            pos += 4
            dwSize = ov.getUint32( pos )
            if( dwSize > constval.BUF_MAX_SIZE )
            {
                throw new Error ( "Error invalid size to deserialize")
            }

            val = ov.getUint32( 12 )
            dwType = ( val & 0xf )
            if( dwType === EnumTypes.DataTypeMem )
            {
                dwTypeId = ( (val & 0xf0 ) >> 4 )
                this.DeserializePrimeType( ov, dwTypeId )
            }
            else if( dwType === EnumTypes.DataTypeMsgPtr )
            {
                this.m_value = unmarshall(
                    srcBuffer.slice( offset, dwSize ) )
            }
            else if( dwType === EnumTypes.DataTypeObjPtr )
            {
                dwClsid = ov.getUint32( 16 )
                this.m_value = DeserializeObj( ov, dwClsid )
            }
            this.type = dwTypeId
        }while( 0 )
    }
}