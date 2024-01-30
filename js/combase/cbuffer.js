const { marshall, unmarshall } = require("../dbusmsg/message");
const Cid = require( "./enums.js").EnumClsid
const Tid = require( "./enums.js").EnumTypeId
const CV = require( "./enums.js").constval
const { CoCreateInstance } = require("./factory");
const E = require ( 'syserrno' ).errors
const { SERI_HEADER_BASE } = require("./SERI_HEADER_BASE.js");

const CObjBase = require( "./objbase" ).CObjBase
const { Buffer } = require( "buffer" )

const SERI_HEADER = class CBuffer_SERI_HEADER extends SERI_HEADER_BASE
{
    constructor()
    {
        super()
        this.dwType = EnumTypes.DataTypeMem 
        this.dwObjClsid = Cid.Invalid
    }
    static GetSeriSize()
    { return super.GetSeriSize() + 8 }

    Serialize( dstBuf, offset )
    {
        super.Serialize( dstBuf, offset )
        offset += super.GetSeriSize()
        dstBuf.setUint32( offset, this.dwType)
        dstBuf.setUint32( offset + 4, this.dwObjClsid)
    }

    Deserialize( srcBuf, offset )
    {
        super.Deserialize( srcBuf, offset )
        offset += super.GetSeriSize()
        this.dwType = srcBuf.getUint32( offset )
        this.dwObjClsid = 
            srcBuf.getUint32( offset + 4 )
    }
}
exports.CBuffer = class CBuffer extends CObjBase
{
    constructor()
    {
        this.m_dwClsid = Cid.CBuffer;
        this.m_dwOffset = 0;
        this.m_arrBuf = null
        this.m_dwTail = 0
        this.m_dwTypeId = Tid.typeNone;
        this.m_value = null
    }

    get size()
    {
        if( this.type !== Tid.typeByteArr )
            return 0
        if( this.empty() )
            return 0
        return this.m_arrBuf.length
    }

    get offset()
    {
        if( this.type !== Tid.typeByteArr )
            return 0
        if( this.empty() )
            return 0
        return this.m_dwOffset;
    }

    set offset( dwOffset )
    {
        if( this.type !== Tid.typeByteArr )
            return
        if( this.empty() )
            return 0
        if( dwOffset < 0 || dwOffset > this.m_dwTail )
            return
        this.m_dwOffset = dwOffset
    }

    get tail()
    {
        if( this.type !== Tid.typeByteArr )
            return 0
        if( this.empty() )
            return 0
        return this.m_dwTail;
    }

    set tail( dwTail )
    {
        if( this.type !== Tid.typeByteArr )
            return
        if( this.empty() )
            return 0
        if( dwTail > this.m_arrBuf.length || dwTail <= 0 )
            return
        this.m_dwTail = dwTail
    }

    get buffer()
    {
        if( this.type !== Tid.typeByteArr )
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

    static StrToBuffer( dstBuf, start, str )
    {
        const encoder = new TextEncoder();
        const encodedData = encoder.encode(str);
        var offset = start
        dstBuf.writeUint32LE( offset, encodedData.length + 1 )
        offset += 4
        dstBuf.fill( encodedData, offset, offset + encodedData.length )
        offset += encodedData.length
        dstBuf.writeUint8( offset, 0 )
        offset+=1
        return offset - start
    }

    // return an arraybuffer with encoded string
    static StrToBuffer( str )
    {
        const encoder = new TextEncoder();
        const encodedData = encoder.encode(str);
        var nullc = Buffer.alloc(1)
        nullc[0] = 0
        return Buffer.concat( [encodedData, nullc] )
    }

    static BufferToStr( buf, offset )
    {
        const decoder = new TextDecoder();
        var dwSize = buf.getUint32( offset )
        return decoder.decode(buf.slice(offset + 4))
    }

    static BufferToStr( buf )
    {
        return this.BufferToStr( buf, 0 )
    }

    ConvertToByteArr()
    {
        if( this.m_value === null )
            return

        switch( this.type )
        {
        case Tid.typeByte:
            {
                this.m_arrBuf = Buffer.alloc( 1 )
                this.m_arrBuf.writeUint8( this.m_value )
                break
            }
        case Tid.typeUInt16:
            {
                this.m_arrBuf = Buffer.alloc( 2 )
                this.m_arrBuf.writeUint16BE( this.m_value )
                break
            }
        case Tid.typeUInt32:
            {
                this.m_arrBuf = Buffer.alloc( 4 )
                this.m_arrBuf.writeUint32BE( this.m_value )
                break
            }
        case Tid.typeUInt64:
            {
                this.m_arrBuf = Buffer.alloc( 8 )
                this.m_arrBuf.writeUint64BE( this.m_value )
                break
            }
        case Tid.typeFloat:
            {
                this.m_arrBuf = Buffer.alloc( 4 )
                this.m_arrBuf.writeFloatBE( this.m_value )
                break
            }
        case Tid.typeDouble:
            {
                this.m_arrBuf = Buffer.alloc( 8 )
                this.m_arrBuf.writeDoubleBE( this.m_value )
                break
            }
        case Tid.typeString:
            {
                const encoder = new TextEncoder();
                this.m_arrBuf = encoder.encode(this.m_value);
                break
            }
        case Tid.typeByteArr:
            {
                return
            }
        case Tid.typeNone:
        default:
            {
                break            
            }
        }
        this.m_value = null
        this.type = Tid.typeByteArr
        this.m_dwOffset = 0
        if( this.m_arrBuf !== null )
            this.m_dwTail = this.m_arrBuf.length
        else
            this.m_dwTail = 0
        return
    }

    Resize( dwNewSize )
    {
        do{
            this.ConvertToByteArr()

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
            this.type = Tid.typeByteArr;

        }while( 0 )
        return
    }

    Serialize()
    {
        var dstBuffer
        var header = Buffer.alloc( SERI_HEADER.GetSeriSize())
        var oHdrView = new DataView( header.buffer )
        var dwType = EnumTypes.DataTypeMem 
        switch( this.type )
        {
        case Tid.typeByteArr:
            {
                dstBuffer = this.buffer
                break
            }
        case Tid.typeByte:
            {
                dstBuffer = Buffer.alloc(1)
                dstBuffer[0] = this.m_value
                break
            }
        case Tid.typeUInt16 :
            {
                dstBuffer = Buffer.alloc(2)
                const view = new DataView( dstBuffer.buffer )
                view.setUint16( 0, this.m_value )
                break
            }
        case Tid.typeUInt32 :
            {
                dstBuffer = Buffer.alloc(4)
                const view = new DataView( dstBuffer.buffer )
                view.setUint32( 0, this.m_value )
                break
            }
        case Tid.typeUInt64 :
            {
                dstBuffer = Buffer.alloc(8)
                const view = new DataView( dstBuffer.buffer )
                view.setBigUint64( 0, this.m_value )
            }
            break
        case Tid.typeFloat :
            {
                dstBuffer = Buffer.alloc(4)
                const view = new DataView( dstBuffer.buffer )
                view.setFloat32( 0, this.m_value )
                break
            }
        case Tid.typeDouble :
            {
                dstBuffer = Buffer.alloc(8)
                const view = new DataView( dstBuffer.buffer )
                view.setFloat64( 0, this.m_value )
                break
            }
        case Tid.typeString :
            {
                dstBuffer = CBuffer.StrToBuffer(
                    this.m_value )
                break
            }
        case Tid.typeDMsg:
            {
                dstBuffer = marshall( this.m_value )
                dwType = EnumTypes.DataTypeMsgPtr
                oHdrView.SetUint32( 16, this.m_value.type )
                break
            }
        case Tid.typeObj:
            {
                dstBuffer = this.m_value.Serialize()
                dwType = EnumTypes.DataTypeObjPtr 
                oHdrView.SetUint32( 16, this.m_value.GetClsid() )
                break
            }
        default:
            throw new TypeError(`Error unknown type id ${this.type}`)
        }
        oHdrView.setUint32( 0, Cid.CBuffer )
        oHdrView.setUint32( 4, dstBuffer.length )
        oHdrView.setUint32( 8, 1, true )
        oHdrView.SetUint32( 12, dwType & ( this.type << 4 ))
        dstBuffer = Buffer.concat( header, dstBuffer )
        return dstBuffer
    }

    GetValue()
    {
        if( this.type === Tid.typeByteArr )
            return this.m_arrBuf
        return this.m_value
    }

    SetBool( val )
    {
        this.SetUint8( val )
    }
    SetUint8( val )
    {
        this.type = Tid.typeByte
        this.m_value = val
    }

    SetUint16( val )
    {
        this.type = Tid.typeUInt16
        this.m_value = val
    }

    SetUint32( val )
    {
        this.type = Tid.typeUInt32
        this.m_value = val
    }

    SetUint64( val )
    {
        this.type = Tid.typeUInt64
        this.m_value = val
    }

    SetFloat( val )
    {
        this.type = Tid.typeFloat
        this.m_value = val
    }

    SetDouble( val )
    {
        this.type = Tid.typeDouble
        this.m_value = val
    }

    SetMsgPtr( val )
    {
        this.type = Tid.typeDMsg
        this.m_value = val
    }

    SetObjPtr( val )
    {
        this.type = Tid.typeObj
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

        if( this.type === Tid.typeNone )
        {
            this.m_arrBuf = newBuf
            this.type = Tid.typeByteArr
            return
        }
        else
        {
            this.ConvertToByteArr()
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
        this.type = Tid.typeNone
        this.m_arrBuf = null
        this.m_dwOffset = this.m_dwTail = 0
        this.m_value = null
    }

    DeserializeObj( ov, dwClsid )
    {
        var offset = SERI_HEADER.GetSeriSize()
        var dwSize = ov.getUint32( 4 )
        var newObj = CoCreateInstance( dwClsid )

        if( newObj === null )
            throw new TypeError( "Error unknown class id")

        this.m_value = newObj.Deserialize(
            ov.buffer.slice( offset, dwSize ) )
    }

    DeserializePrimeType( ov, dwTypeId )
    {
        var offset = SERI_HEADER.GetSeriSize()
        var dwSize = ov.getUint32( 4 )

        switch( dwTypeId )
        {
        case Tid.typeByteArr:
            {
                this.m_arrBuf = Buffer.from(
                    ov.buffer.slice( offset, dwSize ) )
                this.m_dwTail = this.m_arrBuf.length
                this.m_dwOffset = 0
                break
            }
        case Tid.typeByte:
            {
                this.m_value = ov.getUint8( offset, 1 )
                break
            }
        case Tid.typeUInt16:
            {
                this.m_value = ov.getUint16( offset, 2 )
                break
            }
        case Tid.typeUInt32:
            {
                this.m_value = ov.getUint32( offset, 4 )
                break
            }
        case Tid.typeUInt64:
            {
                this.m_value = ov.getUint64( offset, 8 )
                break
            }
        case Tid.typeFloat:
            {
                this.m_value = ov.getFloat32( offset, 4 )
                break
            }
        case Tid.typeDouble:
            {
                this.m_value = ov.getFloat64( offset, 8 )
                break
            }
        case Tid.typeString:
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
        if( srcBuffer.constructor.name !== "Buffer" )
            throw new TypeError( "Error type of input param, expecting CBuffer")

        this.Clear()
        var pos = 0
        var ret = 0
        var ov = new DataView( srcBuffer.buffer )
        var offset = SERI_HEADER.GetSeriSize()
        do{
            var val = ov.getUint32( pos )
            if( val !== Cid.CBuffer )
            {
                ret = E.EINVAL.errno
                break
            }
            pos += 4
            var dwSize = ov.getUint32( pos )
            if( dwSize > CV.BUF_MAX_SIZE )
            {
                throw new Error ( "Error invalid size to deserialize")
            }

            val = ov.getUint32( 12 )
            var dwType = ( val & 0xf )
            var dwTypeId = ( (val & 0xf0 ) >> 4 )
            if( dwType === EnumTypes.DataTypeMem )
            {
                this.DeserializePrimeType( ov, dwTypeId )
                this.type = dwTypeId
            }
            else if( dwType === EnumTypes.DataTypeMsgPtr )
            {
                this.m_value = unmarshall(
                    srcBuffer.slice( offset, dwSize ) )
                this.type = dwTypeId
            }
            else if( dwType === EnumTypes.DataTypeObjPtr )
            {
                var dwClsid = ov.getUint32( 16 )
                if( dwClsid != Cid.CBuffer )
                    this.m_value = this.DeserializeObj( ov, dwClsid )
                else
                {
                    this.Deserialize(
                        srcBuffer.slice( offset, dwSize ))
                }
            }
            else
            {
                throw new TypeError( "Error invalid type" )
            }
        }while( 0 )
    }
}
