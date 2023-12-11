const { SERI_HEADER_BASE } = require("./defines")
const Cid = require( "./enums.js").EnumClsid
const Pid = require( "./enums.js").EnumPropId
const Tid = require( "./enums.js").EnumTypeId
const CV = require( "./enums.js").constval
const Buffer = require( "buffer")
const CBuffer = require( "./cbuffer").CBuffer

const SERI_HEADER = class CConfigDb2_SERI_HEADER extends SERI_HEADER_BASE
{
    constructor()
    {
        super()
        this.dwCount = 0
    }    
    static GetSeriSize()
    {
        return SERI_HEADER_BASE.GetSeriSize() + 4
    }

    Serialize( offset, dstBuf )
    {
        super.Serialize( offset, dstBuf )
        offset += SERI_HEADER_BASE.GetSeriSize()
        dstBuf.setUint32( offset, this.dwCount)
    }

    Deserialize( offset, srcBuf )
    {
        super.Deserialize( offset, srcBuf )
        offset += SERI_HEADER_BASE.GetSeriSize()
        this.dwCount = srcBuf.getUint32( offset )
    }
}

const Pair = class Pair
{
    constructor()
    {
        this.t = Tid.typeNone
        this.v = null
    }
    constructor( src )
    {
        this.t = src.t
        this.v = src.v
    }
}
exports.CConfigDb2=class CConfigDb2 extends CObjBase
{
    constructor()
    {
        this.m_dwClsid = Cid.CConfigDb2
        this.m_props = new Map()
        this.m_dwCount = 0
    }

    Push( src )
    {
        this.m_props[ this.m_dwCount ] = src
        this.m_dwCount++
    }

    Push( tid, val )
    {
        this.Push( new Pair( { t : tid, v:val } ) )        
    }

    Pop()
    {
        if( m_dwCount == 0 )
            return null
        val = this.m_prop[ this.m_dwCount - 1 ]
        this.m_props.delete( this.m_dwCount - 1 )
        this.m_dwCount--
        return val
    }

    GetPropertyType( iProp )
    {
        if( !this.m_props.has( iProp ) )
            return Tid.typeNone
        return this.m_props[iProp].t
    }

    GetProperty( iProp )
    {
        if( !this.m_props.has( iProp ) )
            return null
        return this.m_props[iProp].v
    }

    SetProperty( iProp, val )
    {
        if( val.constructor.name !== "Pair" )
            throw new TypeError( "Error type of input param, expecting Buffer")
        this.m_props[ iProp ]  = val
    }

    SetBool( iProp, val )
    {
        this.SetUint8( iProp, val )
    }
    SetUint8( iProp, val )
    {
        this.m_props[ iProp ] =
            { t: Tid.typeByte, v: val }
    }

    SetUint16( iProp, val )
    {
        this.m_props[ iProp ] =
            { t: Tid.typeUInt16, v: val }
    }

    SetUint32( iProp, val )
    {
        this.m_props[ iProp ] =
            { t: Tid.typeUInt32, v: val }
    }

    SetUint64( iProp, val )
    {
        this.type = Tid.typeUInt64
        this.m_value = val
    }

    SetFloat( iProp, val )
    {
        this.m_props[ iProp ] =
            { t: Tid.typeUInt64, v: val }
    }

    SetDouble( iProp, val )
    {
        this.m_props[ iProp ] =
            { t: Tid.typeDouble, v: val }
    }

    SetMsgPtr( iProp, val )
    {
        this.m_props[ iProp ] =
            { t: Tid.typeDMsg, v: val }
    }

    SetObjPtr( iProp, val )
    {
        this.m_props[ iProp ] =
            { t: Tid.typeObj, v: val }
    }


    GetCount()
    { return this.m_dwCount }

    ClearParams()
    {
        count = this.GetCount()
        for(i=0;i<count;i++)
            Pop()
    }

    static ExtendBuffer( srcBuf, iExtSize )
    {
        return Buffer.concat(
            [ srcBuf, Buffer.alloc(iExtSize)])
    }

    Serialize()
    {
        oHdr = new SERI_HEADER()
        oHdr.m_dwClsid = Cid.CConfigDb2
        oHdr.m_dwSize = 0
        oHdr.m_bVersion = 2
        if( this.m_dwCount > 0 )
        {
            this.SetUint32(
                Pid.propParamCount, this.m_dwCount )
        }
        dstBuf = Buffer.alloc( CV.PAGE_SIZE )
        oHdr.m_dwCount = this.m_props.size 
        pos = 0
        dwBytes = 0
        pos += oHdr.GetSeriSize()

        for( const [ key, value ] of this.m_props )
        {
            if( pos + 1 > dstBuf.length )
            {
                dstBuf = CConfigDb2.ExtendBuffer( dstBuf, CV.PAGE_SIZE )
            }
            dstBuf.writeUint8( value.t, pos++ )
            switch( value.t )
            {
            case Tid.typeByte:
                {
                    if( pos + 1 > dstBuf.length )
                    {
                        dstBuf = CConfigDb2.ExtendBuffer(
                            dstBuf, CV.PAGE_SIZE )
                    }
                    dstBuf.writeUint8( value.v, pos++ )
                    break
                }
            case Tid.typeUInt16:
                {
                    if( pos + 2 > dstBuf.length )
                    {
                        dstBuf = CConfigDb2.ExtendBuffer(
                            dstBuf, CV.PAGE_SIZE )
                    }
                    dstBuf.writeUint16BE( value.v, pos )
                    pos += 2
                    break
                }               
            case Tid.typeUInt32:
                {
                    if( pos + 4 > dstBuf.length )
                    {
                        dstBuf = CConfigDb2.ExtendBuffer(
                            dstBuf, CV.PAGE_SIZE )
                    }
                    dstBuf.writeUint32BE( value.v, pos )
                    pos += 4
                    break
                }               
            case Tid.typeUInt64:
                {
                    if( pos + 8 > dstBuf.length )
                    {
                        dstBuf = CConfigDb2.ExtendBuffer(
                            dstBuf, CV.PAGE_SIZE )
                    }
                    dstBuf.writeUint64BE( value.v, pos )
                    pos += 8
                    break
                }               
            case Tid.typeFloat:
                {
                    if( pos + 4 > dstBuf.length )
                    {
                        dstBuf = CConfigDb2.ExtendBuffer(
                            dstBuf, CV.PAGE_SIZE )
                    }
                    dstBuf.writeFloatBE( value.v, pos )
                    pos += 4
                    break
                }
            case Tid.typeDouble:
                {
                    if( pos + 8 > dstBuf.length )
                    {
                        dstBuf = CConfigDb2.ExtendBuffer(
                            dstBuf, CV.PAGE_SIZE )
                    }
                    dstBuf.writeDoubleBE( value.v, pos )
                    pos += 8
                    break
                }
            case Tid.typeString:
                {
                    if( pos + strBuf.length > dstBuf.length )
                    {
                        dwSize = strBuf.length > CV.PAGE_SIZE ?
                            ( strBuf.length * 2 ) : CV.PAGE_SIZE
                        dstBuf = CConfigDb2.ExtendBuffer(
                            dstBuf, dwSize )
                    }
                    pos += CBuffer.StrToBuffer( dstBuf, pos, value.v ) 
                    break
                }
            case Tid.typeDMsg:
                {
                    break
                }
            case Tid.typeObj:
                {
                    break
                }
            case Tid.typeByteArr:
                {
                    break
                }
            }
        }


    }

    Deserialize( srcBuffer )
    {}
}