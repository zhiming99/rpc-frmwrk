const { unmarshall } = require("../dbusmsg/message.js")
const { SERI_HEADER_BASE } = require("./defines")
const Cid = require( "./enums.js").EnumClsid
const Pid = require( "./enums.js").EnumPropId
const Tid = require( "./enums.js").EnumTypeId
const CV = require( "./enums.js").constval
const { Buffer } = require( "buffer")
const { CoCreateInstance } = require("./factory.js")
const CBuffer = require( "./cbuffer").CBuffer
const marshall = require( "../dbusmsg/message.js").marshall
const { CObjBase } = require( "./objbase.js" )

const SERI_HEADER_CFG = class CConfigDb2_SERI_HEADER extends SERI_HEADER_BASE
{
    constructor()
    {
        super()
        this.dwClsid = Cid.CConfigDb2
        this.dwSize = 0
        this.bVersion = 2
        this.dwCount = 0
    }    
    static GetSeriSize()
    {
        return SERI_HEADER_BASE.GetSeriSize() + 4
    }

    Serialize( dstBuf, offset )
    {
        super.Serialize( dstBuf, offset )
        offset += SERI_HEADER_BASE.GetSeriSize()
        dstBuf.writeUint32BE( this.dwCount, offset)
    }

    Deserialize( srcBuf, offset )
    {
        super.Deserialize( srcBuf, offset )
        offset += SERI_HEADER_BASE.GetSeriSize()
        this.dwCount = srcBuf.readUint32BE( offset )
    }
}

exports.Pair = class Pair
{
    constructor( src )
    {
        if( src === null )
        {
            this.t = Tid.typeNone
            this.v = null
        }
        else
        {
            this.t = src.t
            this.v = src.v
        }
    }
}
exports.CConfigDb2=class CConfigDb2 extends CObjBase
{
    constructor()
    {
        super()
        this.m_dwClsid = Cid.CConfigDb2
        this.m_props = new Map()
        this.m_dwCount = 0
    }

    Recover( oMsg )
    {
        this.m_props = oMsg.m_props
        this.m_dwCount = oMsg.m_dwCount
    }

    Push( src )
    {
        if( this.m_dwCount > 1024 )
            return
        this.m_props.set( this.m_dwCount, src )
        this.m_dwCount++
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
        return this.m_props.get(iProp).t
    }

    GetProperty( iProp )
    {
        if( !this.m_props.has( iProp ) )
            return null
        return this.m_props.get( iProp).v
    }

    SetProperty( iProp, val )
    {
        if( val.constructor.name !== "Pair" )
            throw new TypeError( "Error type of input param, expecting Buffer")
        this.m_props.set( iProp, val )
    }

    SetBool( iProp, val )
    {
        this.SetUint8( iProp, val )
    }
    SetUint8( iProp, val )
    {
        this.m_props.set( iProp, 
            { t: Tid.typeByte, v: val } )
    }

    SetUint16( iProp, val )
    {
        this.m_props.set( iProp, 
            { t: Tid.typeUInt16, v: val } )
    }

    SetUint32( iProp, val )
    {
        this.m_props.set( iProp, 
            { t: Tid.typeUInt32, v: val } )
    }

    SetUint64( iProp, val )
    {
        this.type = Tid.typeUInt64
        this.m_value = val
    }

    SetFloat( iProp, val )
    {
        this.m_props.set( iProp,
            { t: Tid.typeUInt64, v: val } )
    }

    SetDouble( iProp, val )
    {
        this.m_props.set( iProp, 
            { t: Tid.typeDouble, v: val } )
    }

    SetString( iProp, val )
    {
        this.m_props.set( iProp, 
            { t: Tid.typeString, v: val } )
    }

    SetByteArr( iProp, val )
    {
        if( !Buffer.isBuffer( val ) )
            throw new TypeError( "Error not a value of Buffer type" )

        this.m_props.set( iProp, 
            { t: Tid.typeByteArr, v: val } )
    }
    SetMsgPtr( iProp, val )
    {
        this.m_props.set( iProp, 
            { t: Tid.typeDMsg, v: val } )
    }

    SetObjPtr( iProp, val )
    {
        this.m_props.set( iProp, 
            { t: Tid.typeObj, v: val } )
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
        var oHdr = new SERI_HEADER_CFG()
        if( this.m_dwCount > 0 )
        {
            this.SetUint32(
                Pid.propParamCount, this.m_dwCount )
        }

        var dstBuf = Buffer.alloc( CV.PAGE_SIZE )
        oHdr.dwCount = this.m_props.size 

        var pos = SERI_HEADER_CFG.GetSeriSize()
        var key, value
        for( const [ key, value ] of this.m_props )
        {
            if( pos + 5 > dstBuf.length )
            {
                dstBuf = CConfigDb2.ExtendBuffer( dstBuf, CV.PAGE_SIZE )
            }
            dstBuf.writeUint32BE( key, pos )
            pos += 4
            dstBuf.writeUint8( value.t, pos++ )
            var dwSize = 0
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
                    if( value.v === null || value.v === "")
                    {
                        dstBuf.writeUint32BE( 0, pos )
                        pos += 4
                        break
                    }
                    var strBuf = CBuffer.StrToBuffer( value.v ) 
                    if( pos + strBuf.length > dstBuf.length )
                    {
                        dwSize = strBuf.length > CV.PAGE_SIZE ?
                            strBuf.length  : CV.PAGE_SIZE
                        dstBuf = CConfigDb2.ExtendBuffer(
                            dstBuf, dwSize )
                    }
                    dstBuf.writeUint32BE( strBuf.length, pos )
                    pos += 4
                    dstBuf.fill( strBuf, pos, pos + strBuf.length )
                    pos += strBuf.length
                    break
                }
            case Tid.typeByteArr:
                {
                    dwSize = value.v.length
                    dstBuf.writeUint32BE( dwSize , pos )
                    pos += 4
                    if( dwSize === 0 )
                        break
                    dstBuf.fill( value.v, pos, pos + dwSize )
                    pos += dwSize
                    break
                }
            case Tid.typeDMsg:
                {
                    var msgBuf = marshall( value.v )
                    dwSize = msgBuf.length
                    dstBuf.writeUint32BE( dwSize + 4, pos )
                    pos += 4
                    dstBuf.writeUint32BE( dwSize, pos )
                    pos += 4
                    dstBuf.fill( msgBuf, pos, pos + dwSize )
                    pos += dwSize
                    break
                }
            case Tid.typeObj:
                {
                    if( value.v === null )
                    {
                        var o = new SERI_HEADER_BASE()
                        o.Serialize( dstBuf, pos )
                        pos += SERI_HEADER_BASE.GetSeriSize()
                        break
                    }
                    var objBuf = value.v.Serialize() 
                    dwSize = objBuf.length
                    dstBuf.fill( objBuf, pos, pos + dwSize )
                    pos += dwSize
                    break
                }
            }
        }
        oHdr.dwSize = pos - SERI_HEADER_BASE.GetSeriSize()
        oHdr.Serialize( dstBuf, 0 )
        return dstBuf.slice( 0, pos )
    }

    Deserialize( srcBuffer, offset )
    {
        this.m_props.clear()
        if( !Buffer.isBuffer( srcBuffer ) )
            throw new TypeError( "Error invald buffer type")
        if( srcBuffer.length < offset )
            throw new Error( "Error buffer too small to deserialize")

        var oHdr = new SERI_HEADER_CFG()
        var pos = offset
        oHdr.Deserialize( srcBuffer, pos )
        if( oHdr.dwClsid !== Cid.CConfigDb2 ||
            oHdr.dwSize > CV.CFGDB_MAX_SIZE ||
            oHdr.bVersion !== 2 ||
            oHdr.dwCount > CV.CFGDB_MAX_ITEMS ||
            oHdr.dwSize < srcBuffer.length - offset -
                SERI_HEADER_BASE.GetSeriSize() )
            throw new Error( "Error invalid CConfigDb2")
        pos += SERI_HEADER_CFG.GetSeriSize()
        for( i = 0; i < oHdr.dwCount; i++ )
        {
            var key = srcBuffer.readUint32BE( pos )
            pos += 4
            var value = new Pair(null)
            value.t = srcBuffer.readUint8( pos )
            pos++
            var dwSize = 0
            switch( value.t )
            {
            case Tid.typeByte:
                {
                    if( pos + 1 > srcBuf.length )
                    {
                        throw new Error( "Error buffer is too small" )
                    }
                    value.v = srcBuf.readUint8( pos++ )
                    break
                }
            case Tid.typeUInt16:
                {
                    if( pos + 2 > srcBuf.length )
                    {
                        throw new Error( "Error buffer is too small" )
                    }
                    value.v = srcBuf.readUint16BE( pos )
                    pos += 2
                    break
                }               
            case Tid.typeUInt32:
                {
                    if( pos + 4 > srcBuf.length )
                    {
                        throw new Error( "Error buffer is too small" )
                    }
                    value.v = srcBuf.readUint32BE( pos )
                    pos += 4
                    break
                }               
            case Tid.typeUInt64:
                {
                    if( pos + 8 > srcBuf.length )
                    {
                        throw new Error( "Error buffer is too small" )
                    }
                    value.v = srcBuf.readUint64BE( pos )
                    pos += 8
                    break
                }               
            case Tid.typeFloat:
                {
                    if( pos + 4 > srcBuf.length )
                    {
                        throw new Error( "Error buffer is too small" )
                    }
                    srcBuf.readFloatBE( value.v, pos )
                    pos += 4
                    break
                }
            case Tid.typeDouble:
                {
                    if( pos + 8 > srcBuf.length )
                    {
                        throw new Error( "Error buffer is too small" )
                    }
                    value.v = srcBuf.readDoubleBE( pos )
                    pos += 8
                    break
                }
            case Tid.typeString:
                {
                    dwSize = srcBuf.readUint32BE( pos )
                    pos += 4
                    if( pos + dwSize > srcBuf.length )
                        throw new Error( "Error buffer is too small" )
                    if( dwSize = 0 )
                        value.v = ""
                    else
                    {
                        value.v = CBuffer.BufferToStr(
                            srcBuf.slice( pos, dwSize ) ) 
                        pos += dwSize
                    }
                    break
                }
            case Tid.typeByteArr:
                {
                    dwSize = srcBuf.readUint32BE( pos )
                    pos += 4
                    if( pos + dwSize > srcBuf.length )
                        throw new Error( "Error buffer is too small" )
                    if( dwSize === 0 )
                    {
                        value.v = Buffer.alloc( 0 )
                    }
                    else
                    {
                        value.v = srcBuf.slice( pos, pos + dwSize )
                        pos += dwSize
                    }
                    break
                }
            case Tid.typeDMsg:
                {
                    if( pos + 8 > srcBuf.length )
                        throw new Error( "Error buffer is too small" )

                    dwSize = srcBuf.readUint32BE( pos + 4 )
                    pos += 8
                    if( dwSize == 0 )
                    {
                        value.v = null
                        break
                    }
                    if( pos + dwSize > srcBuf.length )
                        throw new Error( "Error buffer is too small" )
                    value.v = unmarshall( srcBuf.slice( pos, dwSize ) )
                    pos += dwSize
                    break
                }
            case Tid.typeObj:
                {
                    if( pos + SERI_HEADER_BASE.GetSeriSize() > srcBuf.length )
                        throw new Error( "Error buffer is too small" )

                    oHdr = new SERI_HEADER_BASE()
                    oHdr.Deserialize( srcBuf, pos )
                    if( oHdr.dwSize + pos > srcBuf.length )
                        throw new Error( "Error buffer is too small" )
                    value.v = CoCreateInstance( oHdr.dwClsid )
                    value.v.Deserialize( srcBuf, pos )
                    pos += oHdr.dwSize + oHdr.GetSeriSize()
                    break
                }
            default:
                {
                    throw new Error( "Error unknown type")
                    break
                }
            }
            this.SetProperty( key, value )
            if( key === Pid.propParamCount )
                this.m_dwCount = value.v
        }
        return offset + oHdr.dwSize +
            SERI_HEADER_BASE.GetSeriSize();
    }
}