
const { CConfigDb2 } = require("../combase/configdb")
const { ERROR } = require("../combase/defines")
const { EnumPropId, errno, EnumSeriProto } = require("../combase/enums")
const { IoCmd, CPendingRequest } = require("../combase/iomsg")
const { messageType } = require( "../dbusmsg/constants")
const { CIoReqMessage } = require("../combase/iomsg")
const { CSerialBase } = require("../combase/seribase")

/**
 * ForwardEventLocal to pass the event to the client handler
 * @param {bool}bSerialize to tell whether serialize or deserialize.
 * @param {object}oProxy the proxy object, which is used to serialize or deserialize the stream handle
 * @returns {undefined}
 * @api public
 */
exports.ForwardEventLocal = function ForwardEventLocal( oMsg )
{
    var oEvent = oMsg.m_oReq
    var strIfName = oEvent.GetProperty(
        EnumPropId.propIfName )
    var strObjPath = oEvent.GetProperty(
        EnumPropId.propObjPath )
    var strMethod = oEvent.GetProperty(
        EnumPropId.propMethodName )
    var strKey = ""
    for( var elem of this.m_arrMatches )
    {
        if( elem.GetIfName() !== strIfName )
            continue
        if( elem.GetObjPath() !== strObjPath )
            continue
        strKey = strIfName.slice( 16 ) +
            '::' + strMethod.slice( 10 )
        break
    }
    if( strKey === "" )
        return
    var oRmtEvt = oEvent.GetProperty( 0 );
    try{
        var bSeriProto = oRmtEvt.GetProperty(
            EnumPropId.propSeriProto )
        if( bSeriProto === null )
            throw new Error( "Error serialization proto missing")
        if( bSeriProto !== EnumSeriProto.seriRidl )
            throw new Error( "Error serialization proto not supported")
        var ridlBuf = oRmtEvt.GetProperty( 0 )
        if( ridlBuf === null )
            throw new Error( "Error bad event message")
        this.m_mapEvtHandlers.get( strKey )( ridlBuf )
    }
    catch(e)
    {

    }
}
