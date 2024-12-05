const { CBuffer } = require("./cbuffer")
const { CConfigDb2 } = require("./configdb")
const { CMessageMatch } = require("./msgmatch")
const { CObjBase } = require( "./objbase")
const Cid = require("./enums").EnumClsid

exports.IClassFactory = class IClassFactory extends CObjBase 
{
    constructor()
    { super() }
    CreateInstance( iClsid )
    { return null }
}

class CClassFactoryBase extends exports.IClassFactory
{
    constructor()
    {
        super()
        this.SetClsid( Cid.CClassFactory )
    }

    CreateInstance( iClsid )
    {
        switch( iClsid )
        {
        case Cid.CBuffer:
            return new CBuffer()
        case Cid.CConfigDb2:
            return new CConfigDb2()
        case Cid.CMessageMatch:
            return new CMessageMatch()
        case Cid.CStlQwordVector:
        case Cid.CStlObjVector:
        default:
            return null
        }
    }
}

var g_Factories = [ new CClassFactoryBase(), ]

exports.RegisterFactory = function RegisterFactory( oFactory )
{ g_Factories.push( oFactory ) }

exports.CoCreateInstance = function CoCreateInstance( iClsid )
{
    var oObj = null
    for( var i = 0; i < g_Factories.length; i++ )
    {
        oObj = g_Factories[i].CreateInstance(iClsid)
        if( oObj !== null )
            break
    }
    return oObj
}