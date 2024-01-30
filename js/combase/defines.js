const Tid = require( "./enums.js").EnumTypeId

exports.randomInt = function randomInt( max )
{ return Math.floor( Math.random() * max ) }

exports.ERROR = function ( iRet )
{
    return iRet < 0
}
exports.SUCCEEDED = function ( iRet )
{ return iRet === errno.STATUS_SUCCESS }

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
