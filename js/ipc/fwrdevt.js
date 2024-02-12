
const { CConfigDb2 } = require("../combase/configdb")
const { ERROR } = require("../combase/defines")
const { EnumPropId, errno, EnumSeriProto } = require("../combase/enums")
const { IoCmd, CPendingRequest } = require("../combase/iomsg")
const { messageType } = require( "../dbusmsg/constants")
const { CIoReqMessage } = require("../combase/iomsg")
const { CSerialBase } = require("../combase/seribase")

exports.ForwardEventLocal = function ForwardEventLocal( oEvent )
{
    var strIfName = oEvent.GetProperty(
        EnumPropId.propIfName )
    var strObjPath = oEvent.GetProperty(
        EnumPropId.propObjPath )
    var strSender = oEvent.GetProperty(
        EnumPropId.propDestDBusName )
    var strMethod = oEvent.GetProperty(
        EnumPropId.propMethodName )
    var strKey = ""
    for( var elem of this.m_arrMatches )
    {
        if( elem.GetIfName() !== strIfName )
            continue
        if( elem.GetObjPath() !== strObjPath )
            continue
        if( elem.GetSender() !== strSender )
            continue
        strKey = strIfName + '::' + strMethod
        break
    }
    if( strKey === "" )
        return
    try{
        var bSeriProto = oEvent.GetProperty(
            EnumPropId.propSeriProto )
        if( bSeriProto === null )
            throw new Error( "Error serialization proto missing")
        if( bSeriProto !== EnumSeriProto.seriRidl )
            throw new Error( "Error serialization proto not supported")
        var ridlBuf = oEvent.GetProperty( 0 )
        if( ridlBuf === null )
            throw new Error( "Error bad event message")
        this.m_mapEvtHandlers.get( strKey ).apply( this, [ridlBuf] )
    }
    catch(e)
    {

    }
}