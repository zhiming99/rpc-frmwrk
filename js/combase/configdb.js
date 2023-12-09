require ( "./enums" ).EnumClsid
require ( "./objbase" ).CObjBase
require ( "./defines" ).SERI_HEADER_BASE

class SERI_HEADER extends SERI_HEADER_BASE
{
    constructor()
    {
        super()
        this.dwCount = 0
    }    
}
exports.CConfigDb2=class CConfigDb2 extends CObjBase
{
    constructor()
    {
        this.m_dwClsid = EnumClsid.CConfigDb2
        this.m_mapProps = new Map()
    }

    Serialize()
    {

    }

    Deserialize( srcBuffer )
    {}
}