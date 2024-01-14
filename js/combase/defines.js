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
        Buffer.alloc( this.GetSeriSize())
        dstBuf.setUint32( offset, this.dwClsid )
        dstBuf.setUint32( offset + 4, this.dwSize )
        dstBuf.setUint32( offset + 8, 0 )
        dstBuf.setUint8( offset, this.bVersion )
        return dstBuf
    }
    Deserialize( srcBuf, offset )
    {
        ov = new DataView( srcBuf.buffer, offset )
        this.dwClsid = ov.getUint32( 0 )
        this.dwSize = ov.getUint32( 4 )
        this.bVersion = ov.getUint8( 8 )
    }
}