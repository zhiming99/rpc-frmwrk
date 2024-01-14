const { messageType } = require("../dbusmsg/constants");
const { CConfigDb2 } = require("./configdb");
const { SERI_HEADER_BASE } = require("./defines");
const CObjBase = require("./objbase");
const MT = requie( "./enums").EnumMatchType
require ("../dbusmsg/message")

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

    GetType()
    { return this.m_iMatchType }

    SetType( iType )
    {
        this.m_iMatchType = iType
        return
    }

    GetIfName()
    { return this.m_strIfName }

    SetIfName( strIfName )
    { 
        this.m_strIfName = strIfName
        return
    }

    GetObjPath()
    { return this.m_strObjPath }

    SetObjPath( strObjPath )
    { 
        this.m_strObjPath = strObjPath
        return
    }

    ToDBusRules( iMsgType )
    {
        strRules = "type="
        switch( iMsgType  )
        {
        case messageType.signal:
            {
                strRules += "'signal'"
                break
            }
        case messageType.methodCall:
            {
                strRules += "'method_call'"
                break
            }
        case messageType.methodReturn:
            {
                strRules += "'method_return'"
                break
            }
        case messageType.error:
            {
                strRules += "'error'"
                break
            }
        case messageType.invalid:
        default:
            {
                strRules = ""
            }
        }
        strMatch = ""
        if( strRules.length > 0 )
            strMatch = strRules
        strIfName = this.GetIfName()
        if( strIfName.length > 0 )
            strMatch += ",interface='" +
                strIfName + "'"

    }

    IsEqual( rhs )
    {
        if( this.m_strIfName !== rhs.m_strIfName ||
            this.m_strObjPath != rhs.m_strObjPath )
            return false
        if( this.GetType() != rhs.GetType() )
            return false

        return true
    }

    ToString()

}