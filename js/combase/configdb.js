import { EnumClsid } from "./enums"
import {CObjBase} from "./objbase"
import { SERI_HEADER_BASE } from "./defines"

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