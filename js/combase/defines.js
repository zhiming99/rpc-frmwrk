const { Buffer } = require( "buffer")
exports.SERI_HEADER_BASE = class SERI_HEADER_BASE
{
    constructor()
    {
        this.dwClsid = 0;
        this.dwSize = 0;
        this.bVersion = 1;
        this.bReserved = 0;
    }

    static GetSeriSize()
    { return 12 }

    Serialize( dstBuf, offset )
    {
        dstBuf.writeUint32BE( this.dwClsid, offset )
        dstBuf.writeUint32BE( this.dwSize, offset + 4)
        dstBuf.writeUint32BE( 0, offset + 8)
        dstBuf.writeUint8( this.bVersion, offset + 8 )
        return dstBuf
    }
    Deserialize( srcBuf, offset )
    {
        this.dwClsid = src.readUint32BE( 0 )
        this.dwSize = src.readUint32BE( 4 )
        this.bVersion = ov.readUint8( 8 )
        return offset + SERI_HEADER_BASE.GetSeriSize()
    }
}

exports.randomInt = function randomInt( max )
{ return Math.floor( Math.random() * max ) }