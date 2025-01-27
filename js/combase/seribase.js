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

class Variant
{
    constructor( osb = null )
    {
        this.m_iType = EnumTypeId.typeNone
        this.m_val = null
        this.m_osb = osb
    }

    SetOsb( osb )
    { this.m_osb = osb }


    Serialize()
    {
        this.m_osb.SerialUint8( this.m_iType )
        switch( this.m_iType )
        {
        case EnumTypeId.typeByte:
            this.m_osb.SerialUint8( this.m_val )
            break
        case EnumTypeId.typeUInt16:
            this.m_osb.SerialUint16( this.m_val )
            break
        case EnumTypeId.typeUInt32:
            this.m_osb.SerialUint32( this.m_val )
            break
        case EnumTypeId.typeUInt64:
            this.m_osb.SerialUint64( this.m_val )
            break
        case EnumTypeId.typeFloat:
            this.m_osb.SerialFloat( this.m_val )
            break
        case EnumTypeId.typeDouble:
            this.m_osb.SerialDouble( this.m_val )
            break
        case EnumTypeId.typeByteArr:
            this.m_osb.SerialByteArray( this.m_val )
            break
        case EnumTypeId.typeString:
            this.m_osb.SerialString( this.m_val )
            break
        case EnumTypeId.typeObj:
            if( this.m_val instanceof CStructBase )
                this.m_osb.SerialStruct( this.m_val )
            else
                this.m_osb.SerialObjPtr( this.m_val )
            break
        case EnumTypeId.typeNone:
            break
        default:
            throw new Error( "Error unknown type to serialize")
        }
    }

    Deserialize( srcBuf, offset )
    {
        var ret = this.m_osb.DeserialUint8(
                srcBuf, offset )
        this.m_iType = ret[0]
        offset = ret[1]
        switch( this.m_iType )
        {
        case EnumTypeId.typeByte:
            ret = this.m_osb.DeserialUint8( srcBuf, offset )
            break
        case EnumTypeId.typeUInt16:
            ret = this.m_osb.DeserialUint16( srcBuf, offset )
            break
        case EnumTypeId.typeUInt32:
            ret = this.m_osb.DeserialUint32( srcBuf, offset )
            break
        case EnumTypeId.typeUInt64:
            ret = this.m_osb.DeserialUint64( srcBuf, offset )
            break
        case EnumTypeId.typeFloat:
            ret = this.m_osb.DeserialFloat( srcBuf, offset )
            break
        case EnumTypeId.typeDouble:
            ret = this.m_osb.DeserialDouble( srcBuf, offset )
            break
        case EnumTypeId.typeByteArr:
            ret = this.m_osb.DeserialByteArray( srcBuf, offset )
            break
        case EnumTypeId.typeString:
            ret = this.m_osb.DeserialString( srcBuf, offset )
            break
        case EnumTypeId.typeObj:
            ret = this.m_osb.DeserialUint32( srcBuf, offset )
            if( ret[ 0 ] === 0x73747275 )
                ret = this.m_osb.DeserialStruct( srcBuf, offset )
            else
                ret = this.m_osb.DeserialObjPtr( srcBuf, offset )
            break
        case EnumTypeId.typeNone:
            return [ this, offset ]
        default:
            throw new Error( "Error unknown type to serialize")
        }
        this.m_val = ret[0]
        return [ this, ret[1]]
    }

    static Restore( oVal )
    {
        var oVar = new Variant()
        if( oVal.m_iType !== undefined )
            oVar.m_iType = oVal.m_iType
        if( oVal.m_val !== undefined )
            oVar.m_val = oVal.m_val
        return oVar
    }
}

exports.Variant = Variant
class CSerialBase
{
    BuildFuncMaps()
    {
        this.m_oSerialMap = new Map([ 
            ['Q' , this.SerialUint64.bind( this )],
            ['q' , this.SerialUint64.bind( this )],
            ['D' , this.SerialUint32.bind( this )],
            ['d' , this.SerialUint32.bind( this )],
            ['W' , this.SerialUint16.bind( this )],
            ['w' , this.SerialUint16.bind( this )],
            ['F' , this.SerialDouble.bind( this )],
            ['f' , this.SerialFloat.bind( this )],
            ['B' , this.SerialUint8.bind( this )],
            ['b' , this.SerialBool.bind( this )],
            ['h' , this.SerialHStream.bind( this )],
            ['a' , this.SerialByteArray.bind( this )],
            ['O' , this.SerialStruct.bind( this )],
            ['o' , this.SerialObjPtr.bind( this )],
            ['s' , this.SerialString.bind( this )],
            ['v' , this.SerialVariant.bind( this )],
        ])

        this.m_oDeserialMap = new Map([
            ['Q' , this.DeserialUint64.bind( this )],
            ['q' , this.DeserialUint64.bind( this )],
            ['D' , this.DeserialUint32.bind( this )],
            ['d' , this.DeserialUint32.bind( this )],
            ['W' , this.DeserialUint16.bind( this )],
            ['w' , this.DeserialUint16.bind( this )],
            ['F' , this.DeserialDouble.bind( this )],
            ['f' , this.DeserialFloat.bind( this )],
            ['B' , this.DeserialUint8.bind( this )],
            ['b' , this.DeserialBool.bind( this )],
            ['h' , this.DeserialHStream.bind( this )],
            ['a' , this.DeserialByteArray.bind( this )],
            ['O' , this.DeserialStruct.bind( this )],
            ['o' , this.DeserialObjPtr.bind( this )],
            ['s' , this.DeserialString.bind( this )],
            ['v' , this.DeserialVariant.bind( this )],
        ])
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
        this.BuildFuncMaps();
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
        oBuf.writeBigUInt64BE( BigInt( val ) )
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
    { this.SerialUint64( val ) }

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
    {
        var oObjBuf = val.Serialize()
        if( this.m_oBuf === null )
            this.m_oBuf = oObjBuf
        else
            this.m_oBuf = Buffer.concat(
                [ this.m_oBuf, oObjBuf ])
        return

    }

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

    SerialVariant( val )
    {
        val.SetOsb( this )
        val.Serialize()
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
        var oBackBuf = this.m_oBuf
        this.m_oBuf = oSizeBuf

        for( var elem of val )
            this.SerialElem( elem, sigElem )

        this.m_oBuf.writeUInt32BE(
            this.m_oBuf.length - 8 )
        if( oBackBuf !== null )
        {
            this.m_oBuf =
                Buffer.concat([oBackBuf, this.m_oBuf])
        }
        return
    }

    static FindContainerSig( signature, bMap )
    {
        var balance = 1
        var sigRest = signature.slice(1)
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

    static FindStandaloneSig( signature )
    {
        if( signature[0] === '(')
            return CSerialBase.FindContainerSig( signature, false )
        else if( signature[0] === '[')
            return CSerialBase.FindContainerSig( signature, true )
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
        oSizeBuf.writeUint32BE( val.size, 4 )
        var oBackBuf = this.m_oBuf
        this.m_oBuf = oSizeBuf
        if( val.size === 0 )
        {
            if( oBackBuf.length > 0 )
                this.m_oBuf = Buffer.concat( [oBackBuf, oSizeBuf])
            return
        }

        var sigKey = CSerialBase.FindStandaloneSig( sigElem )
        if( sigKey.length >= sigElem.length )
            throw new Error( "Error invalid map key signature")
        var sigRest = 
            sigElem.slice( sigKey.length )
        var sigValue = CSerialBase.FindStandaloneSig(sigRest )
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
        if( oBackBuf !== null )
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
        var qwBigInt = 
            oBuf.readBigUInt64BE( offset );
        return [ qwBigInt, offset + 8 ]
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
    { return this.DeserialUint8( val ) }

    DeserialHStream( oBuf, offset )
    { return this.DeserialUint64( oBuf, offset ) }

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
        var strRet = CBuffer.BufferToStrNoNull( oBuf, offset )
        if( strRet[ strRet.length - 1] === '\u0000' )
            strRet = strRet.slice( 0, strRet.length - 1)
        return [ strRet, offset + dwSize + 4 ]
    }

    DeserialObjPtr( oBuf, offset )
    {
        var dwClsid = oBuf.readUint32BE( offset )
        var dwSize = oBuf.readUint32BE( offset + 4 )
        var oObj = globalThis.CoCreateInstance( dwClsid )
        if( oObj === null )
            throw new Error( "Error no such cobject")
        if( dwSize === 0 || dwSize >
            oBuf.length - offset - SERI_HEADER_BASE.GetSeriSize())
            throw new Error( "Error object size is out of range")
        oObj.Deserialize( oBuf, offset)
        return [ oObj, offset + dwSize + SERI_HEADER_BASE.GetSeriSize()]
    } 

    DeserialStruct( oBuf, offset )
    {
        var dwClsid = oBuf.readUint32BE( offset + 4 )
        var oObj = globalThis.CoCreateInstance( dwClsid )
        if( oObj === null )
            throw new Error( "Error no such struct")
        var retOff = oObj.Deserialize( oBuf, offset )
        return [ oObj, retOff[1] ]
    }

    DeserialVariant( oBuf, offset )
    {
        var oVar = new Variant( this )    
        var ret = oVar.Deserialize( oBuf, offset )
        return [ oVar, ret[1] ]
    }

    DeserialElem( oBuf, offset, sig )
    {
        if( sig[0] === '(')
            return this.DeserialArray( oBuf, offset, sig )
        else if( sig[0] === '[')
            return this.DeserialArray( oBuf, offset, sig )
        else
            return this.m_oDeserialMap.get( sig[0] )( oBuf, offset )
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
        var arrValues = []
        for( var i = 0; i < dwCount; i++ )
        {
            var ret = this.DeserialElem( oBuf, offset, sigElem )
            arrValues.push( ret[0])
            offset = ret[1]
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

        var sigKey = CSerialBase.FindStandaloneSig( sigElem )
        if( sigKey.length >= sigElem.length )
            throw new Error( "Error invalid map key signature")
        var sigRest = 
            sigElem.slice( sigKey.length )
        var sigValue = CSerialBase.FindStandaloneSig(sigRest )
        if( sigValue !== sigRest )
            throw new Error( "Error invalid map value signature")

        var mapValues = new Map()
        for( var i = 0; i < dwCount; i++ )
        {
            var retKey = this.DeserialElem( oBuf, offset, sigKey )
            offset = retKey[1]
            var retVal = this.DeserialElem( oBuf, offset, sigValue)
            offset = retVal[1]
            mapValues.set( retKey[0], retVal[0])
        }
        return [mapValues, offset]

    }
}

exports.CSerialBase=CSerialBase

class CStructBase extends CObjBase
{
    constructor( pIf )
    {
        super();
        if( pIf !== undefined )
            this.m_pIf = pIf;
        else
            this.m_pIf = null;
        return;
    }
    SetInterface( pIf )
    { this.m_pIf = pIf; }

    Restore( oVal )
    {}

    static RestoreArray( oArray, strSig )
    {
        if( strSig[0] !== '(' ||
            strSig[ strSig.length - 1 ] != ')' )
            return null;
        const regex = /O/g;
        var idx = strSig.search( regex );
        if( idx === -1 )
            return oArray;

        var dwSigLen = strSig.length
        var sigElem = strSig.slice(1, dwSigLen-1);
        var oNewArr = [];
        for( var i = 0; i < oArray.length; i++ )
        {
            oNewArr.push( 
                CStructBase.RestoreElem( oArray[ i ], sigElem ) );
        }
        return oNewArr;
    }

    static RestoreMap( oMap, strSig )
    {
        if( strSig[0] !== '(' ||
            strSig[ strSig.length - 1 ] != ')' )
            return null;

        const regex = /O/g;
        var idx = strSig.search( regex );
        if( idx === -1 )
            return oMap;
        
        var dwSigLen = strSig.length
        var sigElem = strSig.slice(1, dwSigLen-1);
        var strKeySig = CStructBase.FindStandaloneSig( sigElem );
        var strValSig = sigElem.slice( strKeySig.length );
        var bRestoreKey = true, bRestoreValue = true;
        idx = strKeySig.search( regex );
        if( idx === -1 )
            bRestoreKey = false;
        idx = strValSig.search( regex );
        if( idx === -1 )
            bRestoreValue = false;

        var oNewMap = new Map();
        for( const [ key, value ] of oMap )
        {
            var newKey, newVal;
            if( bRestoreKey )
                newKey = RestoreElem( key, strKeySig );
            if( bRestoreValue )
                newVal = RestoreElem( value, strKeySig );
            oNewMap.set( newKey, newVal )
        }
        return oNewMap;
    }

    static RestoreStruct( oStruct, strSig )
    {
        if( oStruct.m_pIf !== undefined )
            return oStruct;
        const regex = /[0-9A-Fa-f]{8}\*/g;
        var idx = strSig.search( regex );
        if( idx !== 1 )
            throw new Error( "Error unable to find clsid to restore struct")
        var strClsid = "0x" + strSig.slice( 1, 9 );
        var iClsid = Number( strClsid );
        var oObj = globalThis.CoCreateInstance( iClsid );
        oObj.Restore( oStruct );
        return oObj;
    }

    static RestoreElem( oElem, strSig )
    {
        if( strSig[ 0 ] === '(')
            return CStructBase.RestoreArray( oElem, strSig );
        else if( strSig[ 0 ] === '[')
            return CStructBase.RestoreMap( oElem, strSig );
        else if( strSig[ 0 ] === 'O' )
            return CStructBase.RestoreStruct( oElem, strSig );
        else if( strSig[ 0 ] === 'v' )
            return Variant.Restore( oElem );
        return oElem;
    }

    static FindStandaloneSig( signature )
    {
        if( signature[ 0 ] === 'O' )
        {
            if( signature.length < 10 )
                throw new Error( "Error bad signature '" + signature + "'");
            return signature.slice( 0, 10 );
        }
        return CSerialBase.FindStandaloneSig( signature );
    }
}

exports.CStructBase = CStructBase
