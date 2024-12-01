const { messageType } = require("../dbusmsg/constants");
const { CConfigDb2 } = require("./configdb");
const { SERI_HEADER_BASE } = require("./SERI_HEADER_BASE");
const {CObjBase} = require("./objbase");
const MT = require( "./enums").EnumMatchType
const {EnumPropId, EnumClsid} = require( "./enums")
require ("../dbusmsg/message")
const {Buffer} = require( "buffer")

class SERI_HEADER_MSGMATCH extends SERI_HEADER_BASE
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
        dstBuf.writeUint32BE( this.iMatchType, offset )
    }

    Deserialize( srcBuf, offset )
    {
        super.Deserialize( srcBuf, offset )
        offset += SERI_HEADER_BASE.GetSeriSize()
        this.iMatchType = srcBuf.readUint32BE( offset )
    }
}
exports.CMessageMatch = class CMessageMatch extends CObjBase
{
    constructor()
    {
        super()
        this.m_dwClsid = EnumClsid.CMessageMatch
        this.m_strObjPath = ""
        this.m_strIfName = ""
        this.m_iMatchType = MT.matchInvalid
        this.m_oCfg = new CConfigDb2()
    }

    Restore( oObj )
    {
        if( oObj === null || oObj === undefined )
            return
        this.m_strObjPath = oObj.m_strObjPath
        this.m_strIfName = oObj.m_strIfName
        this.m_iMatchType = oObj.m_iMatchType
        this.m_oCfg = new CConfigDb2()
        if( oObj.m_oCfg !== null )
            this.m_oCfg.Restore( oObj.m_oCfg )
    }

    SetProperty( iProp, o )
    {
        if( iProp === EnumPropId.propObjPath )
        {
            this.SetObjPath( o.v )
        }
        else if( iProp === EnumPropId.propIfName )
        {
            return this.SetIfName( o.v )
        }
        else
            this.m_oCfg.SetProperty( iProp, o )
    }

    GetProperty( iProp )
    {
        if( iProp === EnumPropId.propObjPath )
            return this.GetObjPath()
        else if( iProp === EnumPropId.propIfName )
            return this.GetIfName()
        return this.m_oCfg.GetProperty( iProp ).v
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

        strPath = this.GetObjPath()
        if( strPath.length > 0 )
            strMatch += ",path='" +
                strPath + "'"

        strMethod = this.m_oCfg(
            EnumPropId.propMethodName )
        if( strMethod !== null )
            strMatch += ",member='" +
                strMethod + "'"
        return strMatch
    }

    IsEqual( rhs )
    {
        if( this.m_strIfName !== rhs.m_strIfName ||
            this.m_strObjPath !== rhs.m_strObjPath )
            return false
        if( this.GetType() !== rhs.GetType() )
            return false

        return true
    }

    ToString()
    {
        strAll = this.ToDBusRules( messageType.invalid )
        strAll += ",match="
        if( this.GetType() == MT.matchClient )
            strAll += "matchClient"
        else if( this.GetType() == MT.matchServer )
            strAll += "matchServer"
        return strAll
    }

    Serialize()
    {
        this.m_oCfg.SetString(
            EnumPropId.propIfName, this.m_strIfName)
        this.m_oCfg.SetString(
            EnumPropId.propObjPath, this.m_strObjPath )
        
        var oHeader = new SERI_HEADER_MSGMATCH()
        var oCfgBuf = this.m_oCfg.Serialize()

        oHeader.dwClsid = this.m_dwClsid
        oHeader.iMatchType = this.m_iMatchType

        var dwTotal =
            oCfgBuf.length + SERI_HEADER_MSGMATCH.GetSeriSize()

        oHeader.dwSize = dwTotal -
            SERI_HEADER_BASE.GetSeriSize()
        
        var hdrBuf = Buffer.alloc(
            SERI_HEADER_MSGMATCH.GetSeriSize() )
        oHeader.Serialize( hdrBuf, 0 )
        return Buffer.concat( [ hdrBuf, oCfgBuf ])
    } 

    Deserialize( srcBuf, offset )
    {
        var oHdr = new SERI_HEADER_MSGMATCH()
        oHdr.Deserialize( srcBuf, offset )
        if( oHdr.dwSize + SERI_HEADER_BASE.GetSeriSize() >
            srcBuf.length - offset )
            throw new Error( "Error buffer too small to deserialize")

        this.m_iMatchType = oHdr.iMatchType
        if( this.m_iMatchType != MT.matchClient &&
            this.m_iMatchType != MT.matchServer )
            throw new Error( "Error invalid MessageMatch")

        this.m_oCfg.Deserialize( srcBuf,
            offset + SERI_HEADER_MSGMATCH.GetSeriSize() )
        this.m_strIfName = this.m_oCfg.GetString(
            EnumPropId.propIfName )
        this.m_strObjPath = this.m_oCfg.GetString(
            EnumPropId.propObjPath )
        if( this.m_strIfName === null ||
            this.m_strObjPath === null )
            throw new Error( "Error invalid MessageMatch")
        return offset + SERI_HEADER_BASE.GetSeriSize() + oHdr.dwSize
    }
}
