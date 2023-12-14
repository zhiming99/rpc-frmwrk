const { CConfigDb2 } = require("./configdb");
const { SERI_HEADER_BASE } = require("./defines");
const CObjBase = require("./objbase");
const MT = requie( "./enums").EnumMatchType

const SERI_HEADER = class SERI_HEADER extends SERI_HEADER_BASE
{
    constructor()
    {
        super()
        this.iMatchType = MT.matchInvalid
    }
    static GetSeriSize()
    {
        return SERI_HEADER_BASE.GetSeriSize() + 4
    }

    Serialize( dstBuf, offset )
    {
        super.Serialize( dstBuf, offset )
        offset += SERI_HEADER_BASE.GetSeriSize()
        dstBuf.setUint32( offset, this.iMatchType)
    }

    Deserialize( srcBuf, offset )
    {
        super.Deserialize( srcBuf, offset )
        offset += SERI_HEADER_BASE.GetSeriSize()
        this.iMatchType = srcBuf.getUint32( offset )
    }
}
exports.CMessageMatch = class CMessageMatch extends CObjBase
{
    constructor()
    {
        this.m_strObjPath = ""
        this.m_strIfName = ""
        this.m_iMatchType = MT.matchInvalid
        this.m_pCfg = new CConfigDb2()
    }

}