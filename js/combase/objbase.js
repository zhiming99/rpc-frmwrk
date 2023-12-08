
import { EnumClsid } from "./enums"
exports.CObjBase=class CObjBase
{
    constructor()
    { this.m_dwClsid=EnumClsid.Invalid; }
    get m_dwClsid()
    { return this.m_dwClsid}

    set m_dwClsid( dwClsid )
    { this.m_dwClsid = dwClsid; }

    Serialize()
    { return null }

    Deserialize( srcBuffer )
    {}
}