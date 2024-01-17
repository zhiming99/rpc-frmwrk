const EnumClsid = require("./enums" ).EnumClsid
exports.CObjBase = class CObjBase
{
    constructor()
    { this.m_dwClsid=EnumClsid.Invalid; }

    GetClsid()
    { return this.m_dwClsid }

    SetClsid( dwClsid )
    { this.m_dwClsid = dwClsid; }

    Serialize()
    { return null }

    Deserialize( srcBuffer )
    {}
};