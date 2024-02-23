const Tid = require( "./enums.js").EnumTypeId
const {errno} = require( "./enums.js")

exports.randomInt = function randomInt( max )
{ return Math.floor( Math.random() * max ) }

exports.SUCCEEDED = function ( iRet )
{ return iRet === errno.STATUS_SUCCESS }

exports.InvalFunc = function()
{ console.log( "Error Invalid function" )}

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

function Int32Value( i ){
    if( i < 0x80000000 )
        return i
    else if( i < 0x100000000 )
        return i - 0x100000000
    throw new Error( "Error beyond max 32-bit signed integer")
}

exports.Int32Value = Int32Value

exports.ERROR = function ( iRet )
{
    return Int32Value( iRet ) < 0
}

exports.USER_METHOD = function( strName )
{
    return "UserMethod_" + strName
}

exports.SYS_METHOD = function( strName )
{
    return "RpcCall_" + strName
}

exports.NumberToUint32= function( i )
{ return Number( BigInt( i ) & BigInt( 0x00000000ffffffff ) ) }