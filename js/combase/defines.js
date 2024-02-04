const Tid = require( "./enums.js").EnumTypeId

exports.randomInt = function randomInt( max )
{ return Math.floor( Math.random() * max ) }

exports.ERROR = function ( iRet )
{
    return iRet < 0
}
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

exports.Int32Value = ( i )=>{
    if( i < 0x80000000 )
        return i
    else if( i < 0x100000000 )
        return i - 0x100000000
    throw new Error( "Error beyond max 32-bit signed integer")
}