const { Pair, } = require("./defines")
const { SERI_HEADER_BASE } = require("./SERI_HEADER_BASE.js")
const Cid = require( "./enums.js").EnumClsid
const Pid = require( "./enums.js").EnumPropId
const Tid = require( "./enums.js").EnumTypeId
const CV = require( "./enums.js").constval
const { Buffer } = require( "buffer")
const { CBuffer } = require( "./cbuffer")
const { CObjBase } = require( "./objbase.js" )
const { EnumTypeId } = require("./enums.js")

class CSerialBase
{
    BuildFuncMaps()
    {
        this.m_oSerialMap = new Map( {
            'Q' : this.SerialUint64,
            'q' : this.SerialUint64,
            'D' : this.SerialUint32,
            'd' : this.SerialUint32,
            'W' : this.SerialUint16,
            'w' : this.SerialUint16,
            'F' : this.SerialDouble,
            'f' : this.SerialFloat,
            'B' : this.SerialUint8,
            'b' : this.SerialBool,
            'h' : this.SerialHStream,
            'a' : this.SerialByteArray,
            'O' : this.SerialStruct,
            'o' : this.SerialObjPtr,
        })

        this.m_oDeserialMap = new Map( {
            'Q' : this.DeserialUint64,
            'q' : this.DeserialUint64,
            'D' : this.DeserialUint32,
            'd' : this.DeserialUint32,
            'W' : this.DeserialUint16,
            'w' : this.DeserialUint16,
            'F' : this.DeserialDouble,
            'f' : this.DeserialFloat,
            'B' : this.DeserialUint8,
            'b' : this.DeserialBool,
            'h' : this.DeserialHStream,
            'a' : this.DeserialByteArray,
            'O' : this.DeserialStruct,
            'o' : this.DeserialObjPtr,
        })
    }
    /**
     * constructor of CSerialBase
     *
     * @param {bool}bSerialize to tell whether serialize or deserialize.
     * @param {object}oProxy the proxy object, which is used to serialize or deserialize the stream handle
     * @returns {undefined}
     * @api public
     */
    constructor( bSerialize, oProxy )
    {
        this.m_oBuf = null
        this.m_bSerial = bSerialize
        if( oProxy !== undefined)
            this.m_oProxy = oProxy
        else
            this.m_oProxy = null
    }

    /**
     * Get the ridl buffer.
     * @returns {Buffer} the accumulated ridl buffer after a set of serialization operations
     * @api public
     */
    GetRidlBuf()
    { return this.m_oBuf }

    SerialUint8( val )
    {
        var oBuf = Buffer.alloc( 1 )
        oBuf.writeUInt8( val )
        if( this.m_oBuf === null )
            this.m_oBuf = oBuf
        else
            this.m_oBuf =
                Buffer.concat( [ this.m_oBuf, oBuf ])
        return
    }
    SerialUint16( val )
    {
        var oBuf = Buffer.alloc( 2 )
        oBuf.writeUInt16BE( val )
        if( this.m_oBuf === null )
            this.m_oBuf = oBuf
        else
            this.m_oBuf =
                Buffer.concat( [ this.m_oBuf, oBuf ])
        return
    }
    SerialUint32( val )
    {
        var oBuf = Buffer.alloc( 4 )
        oBuf.writeUInt32BE( val )
        if( this.m_oBuf === null )
            this.m_oBuf = oBuf
        else
            this.m_oBuf =
                Buffer.concat( [ this.m_oBuf, oBuf ])
        return
    }
    SerialUint64( val )
    {
        var oBuf = Buffer.alloc( 8 )
        oBuf.writeBigUInt64BE( val )
        if( this.m_oBuf === null )
            this.m_oBuf = oBuf
        else
            this.m_oBuf =
                Buffer.concat( [ this.m_oBuf, oBuf ])
        return
    }

    SerialFloat( val )
    {
        var oBuf = Buffer.alloc( 4 )
        oBuf.writeFloatBE( val )
        if( this.m_oBuf === null )
            this.m_oBuf = oBuf
        else
            this.m_oBuf =
                Buffer.concat( [ this.m_oBuf, oBuf ])
        return
    }

    SerialDouble( val )
    {
        var oBuf = Buffer.alloc( 8 )
        oBuf.writeDoubleBE( val )
        if( this.m_oBuf === null )
            this.m_oBuf = oBuf
        else
            this.m_oBuf =
                Buffer.concat( [ this.m_oBuf, oBuf ])
        return
    }

    SerialBool( val )
    { this.SerialUint8( val ) }

    SerialHStream( val )
    { throw new Error( "Error support is not available yet") }

    SerialByteArray( val )
    {
        var oSizeBuf = Buffer.alloc( 4 )
        oSizeBuf.writeUint32BE( val.length )

        if( this.m_oBuf === null )
            this.m_oBuf =
                Buffer.concat( [ oSizeBuf, val ])
        else
            this.m_oBuf = Buffer.concat(
                [ this.m_oBuf, oSizeBuf, val ])
        return
    }

    SerialString( val )
    {
        var oStrBuf = CBuffer.StrToBufferNoNull( val )
        var oSizeBuf = Buffer.alloc( 4 )
        oSizeBuf.writeUint32BE( oStrBuf.length )

        if( this.m_oBuf === null )
            this.m_oBuf =
                Buffer.concat( [ oSizeBuf, oStrBuf ])
        else
            this.m_oBuf = Buffer.concat(
                [ this.m_oBuf, oSizeBuf, oStrBuf ])
        return
    }

    SerialObjPtr( val )
    { throw Error( "Error not supported") }

    SerialStruct( val )
    {
        var oBuf = val.Serialize()
        if( this.m_oBuf === null )
            this.m_oBuf = oBuf
        else
            this.m_oBuf = Buffer.concat(
                [ this.m_oBuf, oBuf ])
        return
    }

    SerialArray( val, sig )
    {
        var dwSigLen = sig.length
        if( sig[0] !== '(' ||
            sig[ dwSigLen - 1 ] != ')' ||
            dwSigLen <= 2 )
            throw new Error( "Error invalid array signature")
        var sigElem = sig.slice(1, dwSigLen-1)
        var oSizeBuf = Buffer.alloc( 8 )
        oSizeBuf.writeUInt32BE( 0, 0 )
        oSizeBuf.writeUint32BE( val.length, 4 )
        if( this.m_oBuf === null )
            this.m_oBuf = oSizeBuf
        else
            this.m_oBuf = Buffer.concat( [this.m_oBuf, oSizeBuf])
        if( val.length === 0 )
            return
        for( var elem of val )
            this.SerialElem( elem, sigElem )
        return
    }

    FindContainerSig( signature, bMap )
    {
        var balance = 1
        sigRest = signature.slice(1)
        var count = 1
        var openChar, closeChar
        if( bMap )
        {
            openChar = '['
            closeChar = ']'
        }
        else
        {
            openChar = '('
            closeChar = ')'
        }
        for( var elem of sigRest )
        {
            count++
            if( elem === openChar)
            {
                balance++
                continue
            }
            if( elem === closeChar)
            {
                balance-- 
                if( balance === 0 )
                    break
            }
        }
        if( balance > 0 )
            throw new Error( "Error invalid array/map signature")
        return signature.slice( 0, count )
    }

    FindStandaloneSig( signature )
    {
        if( signature[0] === '(')
            return this.FindContainerSig( signature, false )
        else if( signature[0] === '[')
            return this.FindContainerSig( signature, true )
        return signature[0]
    }

    SerialMap( val, sig )
    {
        var dwSigLen = sig.length
        if( sig[0] !== '[' ||
            sig[ dwSigLen - 1 ] != ']' ||
            dwSigLen <= 2 )
            throw new Error( "Error invalid array signature")
        var sigElem = sig.slice(1, dwSigLen-1)
        var oSizeBuf = Buffer.alloc( 8 )
        oSizeBuf.writeUInt32BE( 0, 0 )
        oSizeBuf.writeUint32BE( val.length, 4 )
        var oBackBuf = this.m_oBuf
        this.m_oBuf = oSizeBuf
        if( val.length === 0 )
        {
            if( oBackBuf.length > 0 )
                this.m_oBuf = Buffer.concat( [oBackBuf, oSizeBuf])
            return
        }

        sigKey = this.FindStandaloneSig( sigElem )
        if( sigKey.length >= sigElem.length )
            throw new Error( "Error invalid map key signature")
        var sigRest = 
            sigElem.slice( sigKey.length )
        sigValue = this.FindStandaloneSig(sigRest )
        if( sigValue !== sigRest )
            throw new Error( "Error invalid map value signature")

        var key, value
        for( [key, value ] of val )
        {
            this.SerialElem( key, sigKey )
            this.SerialElem( value, sigValue)
        }
        this.m_oBuf.writeUInt32BE(
            this.m_oBuf.length - 8 )
        this.m_oBuf =
            Buffer.concat([oBackBuf, this.m_oBuf])
        return
    }

    SerialElem( val, sig )
    {
        if( sig[0] === '(' )
        {
            this.SerialArray( val, sig )
        }
        else if( sig[0] === '[' )
        {
            this.SerialMap( val, sig )
        }
        else
        {
            
            this.m_oSerialMap.get( sig[0])( val )
        }
        return
    }

    DeserialUint8( oBuf, offset )
    {
        return [ oBuf.readUint8( offset ), offset + 1 ]
    }

    DeserialUint16( oBuf, offset )
    {
        return [ oBuf.readUint16BE( offset ), offset + 2 ]
    }
    DeserialUint32( oBuf, offset )
    {
        return [ oBuf.readUint32BE( offset ), offset + 4 ]
    }
    DeserialUint64( oBuf, offset )
    {
        return [ oBuf.readBigUInt64BE( offset ), offset + 8 ]
    }

    DeserialFloat( oBuf, offset )
    {
        return [ oBuf.readFloatBE( offset ), offset + 4 ]
    }

    DeserialDouble( oBuf, offset )
    {
        return [ oBuf.readDoubleBE( offset ), offset + 8 ]
    }

    DeserialBool( oBuf, offset )
    { this.DeserialUint8( val ) }

    DeserialHStream( oBuf, offset )
    { throw new Error( "Error support is not available yet") }

    DeserialByteArray( oBuf, offset )
    {
        var dwSize = oBuf.readUint32BE( offset )
        if( dwSize === 0 || dwSize > CV.MAX_BYTES_PER_BUFFER )
        {
            throw new Error( "Error bytearray out of range")
        }
        return [ oBuf.slice( offset + 4, offset + 4 + dwSize ),
            offset + 4 + dwSize ]
    }

    DeserialString( oBuf, offset )
    {
        var dwSize = oBuf.readUint32BE( offset )
        if( dwSize === 0 || dwSize > CV.MAX_BYTES_PER_BUFFER )
        {
            throw new Error( "Error bytearray size is out of range")
        }
        var strRet = CBuffer.BufferToStrNoNull( oBuf )
        if( strRet[ strRet.length - 1] === '\u0000' )
            strRet = strRet.slice( 0, strRet.length - 1)
        return [ strRet, offset + dwSize + 4 ]
    }

    DeserialObjPtr( oBuf, offset )
    { throw new Error( "Error not supported") }

    DeserialStruct( oBuf, offset )
    {
        var dwClsid = oBuf.readUint32BE( offset )
        var oObj = globalThis.CoCreateInstance( dwClsid )
        if( oObj === null )
            throw new Error( "Error no such struct")
        var dwSize = oBuf.readUint32BE( offset + 4 )
        if( dwSize === 0 || dwSize > CV.MAX_BYTES_PER_BUFFER )
            throw new Error( "Error object size is out of range")
        oObj.Deserialize( oBuf, offset )
        return [ oObj, offset + dwSize + SERI_HEADER_BASE.GetSeriSize()]
    }

    DeserialElem( oBuf, offset, sig )
    {
        if( sig[0] === '(')
            return this.DeserialArray( oBuf, offset, sig )
        else if( sig[0] === '[')
            return this.DeserialArray( oBuf, offset, sig )
        else
            return this.m_oDeserialMap[ sig[0]]( oBuf, offset )
    }

    DeserialArray( oBuf, offset, sig )
    {
        var dwSigLen = sig.length
        if( sig[0] !== '(' ||
            sig[ dwSigLen - 1 ] != ')' ||
            dwSigLen <= 2 )
            throw new Error( "Error invalid array signature")
        var sigElem = sig.slice(1, dwSigLen-1)
        var dwSize = this.DeserialUint32( oBuf, offset)[0]
        var dwCount = this.DeserialUint32( oBuf, offset + 4)[0]
        if( dwSize === 0 || dwCount === 0 )
            return [ [], offset + 8 ]
        if( dwSize > CV.MAX_BYTES_PER_BUFFER || dwCount > 1000000 )
            throw new Error( "Error array size out of range")
        offset += 8
        var arrValues
        for( var i = 0; i < dwCount; i++ )
        {
            var ret = this.DeserialElem( oBuf, offset, sigElem )
            arrValues.push( ret[0])
            offset += ret[1]
        }
        return [arrValues, offset]

    }

    DeserialMap( oBuf, offset, sig )
    {
        var dwSigLen = sig.length
        if( sig[0] !== '[' ||
            sig[ dwSigLen - 1 ] != ']' ||
            dwSigLen <= 2 )
            throw new Error( "Error invalid array signature")
        var sigElem = sig.slice(1, dwSigLen-1)
        var dwSize = this.DeserialUint32( oBuf, offset)[0]
        var dwCount = this.DeserialUint32( oBuf, offset + 4)[0]
        if( dwSize === 0 || dwCount === 0 )
            return [ new Map(), offset + 8 ]
        if( dwSize > CV.MAX_BYTES_PER_BUFFER || dwCount > 1000000 )
            throw new Error( "Error array size out of range")
        offset += 8

        sigKey = this.FindStandaloneSig( sigElem )
        if( sigKey.length >= sigElem.length )
            throw new Error( "Error invalid map key signature")
        var sigRest = 
            sigElem.slice( sigKey.length )
        sigValue = this.FindStandaloneSig(sigRest )
        if( sigValue !== sigRest )
            throw new Error( "Error invalid map value signature")

        var mapValues = new Map()
        for( var i = 0; i < dwCount; i++ )
        {
            var retKey = this.DeserialElem( oBuf, offset, sigKey )
            offset += retKey[1]
            var retVal = this.DeserialElem( oBuf, offset, sigValue)
            offset += retVal[1]
            mapValues.set( retKey[0], retVal[0])
        }
        return [mapValues, offset]

    }
}

exports.CSerialBase=CSerialBase