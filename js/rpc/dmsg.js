const { messageType } = require("../dbusmsg/constants")

require( "../dbusmsg/constants")

exports.CDBusMessage = class CDBusMessage
{
    constructor( iType, oReqMsg )
    {

        this.type = iType
        if( iType === messageType.methodReturn )
        {
            this.SetReplySerial( oReqMsg.GetSerial() )
            this.SetSender( oReqMsg.GetDestination() )
            this.SetInterface( oReqMsg.GetInterface() )
            this.SetObjPath( oReqMsg.GetObjPath() )
        }
        this.path = null
        this.interface = null
        this.member = null
        this.errorName = null
        this.replySerial = null
        this.destination = null
        this.sender = null
        this.signature = ""
        this.serial = null
        this.body = []
    }

    GetObjPath()
    { return this.path }

    SetObjPath( strPath )
    { this.path = strPath }

    GetInterface()
    { return this.interface }

    SetInterface( strIfName )
    { this.interface = strIfName }

    GetMember()
    { return this.member }

    SetMember( strMember )
    { this.member = strMember }

    GetError()
    { return this.errorName }

    SetError( strError )
    { this.errorName = strError }

    GetReplySerial()
    { return this.replySerial }

    SetReplySerial( iSerial )
    { this.replySerial = iSerial }

    GetSerial()
    { return this.serial }

    SetSerial( iSerial )
    { this.serial = iSerial }

    GetDestination()
    { return this.destination }

    SetDestination( strDest )
    { this.destination = strDest }

    GetSender()
    { return this.sender }

    SetSender( strSender )
    { this.sender = strSender }
    // arrFileds is an array of pairs
    // each pair contains a type character
    // and a value
    AppendFields( arrFields )
    {
        var sig = ""
        if( this.signature !== "")
            sig = this.signature
        arrFields.forEach(e => {
            sig += e.t
            this.body.push( e.v )
        });
        this.signature = sig
    }

    GetArgCount()
    {
        if( !this.body )
            return 0
        return this.body.length
    }

    GetArgAt( idx )
    {
        if( idx >= this.GetArgCount() )
            return null
        return this.body[ idx ]
    }

    Copy( oMsg )
    {
        this.type = oMsg.type
        this.path = oMsg.path
        this.interface = oMsg.interface
        this.member = oMsg.member
        this.errorName = oMsg.errorName
        this.replySerial = oMsg.replySerial
        this.destination = oMsg.destination
        this.sender = oMsg.destination
        this.signature = oMsg.signature 
        this.serial = oMsg.serial
        this.body = oMsg.body
    }
}