const CV = require( "../combase/enums.js" ).constval
const Buffer = require( "buffer").Buffer
const { EnumTypeId, EnumTypes, EnumPropId, EnumCallFlags } = require("../combase/enums.js");
const { CConfigDb2 } = require("../combase/configdb");

const IoMsgType =
{
    Invalid : 0,
    ReqMsg : 1,
    RespMsg : 2,
    EvtMsg : 3,
    AdminReq : 4,
    AdminResp : 5,
}

exports.IoMsgType = IoMsgType

const AdminCmd =
{
    Invalid : [ 0, "Invalid" ],
    SetConfig : [ 1, "SetConfig" ],
    OpenRemotePort : [ 2, "OpenRemotePort" ],
    CloseRemotePort : [ 3, "CloseRemotePort" ],
}
exports.AdminCmd = AdminCmd

const IoCmd = {
    Invalid : [ 0, "Invalid" ],
    ForwardRequest : [ 1, "RpcCall_ForwardRequest" ],
    FetchData: [ 2, "RpcCall_FetchData" ],
    OpenStream : [ 3, "Brdg_Req_OpenStream" ],
    CloseStream : [ 4, "Brdg_Req_CloseStream" ],
    EnableRemoteEvent : [ 5, "RpcCall_EnableRemoteEvent" ],
    DisableRemoteEvent : [ 6, "RpcCall_DisableRemoteEvent" ],
    CheckRouterPath : [ 7, "RpcCall_CheckRouterPath" ],
    UserCancelRequest : [ 8, "RpcCall_UserCancelRequest" ],
    Login : [ 9, "RpcCall_Login" ],
    Handshake: [10, "RpcCall_Handshake"],
    KeepAliveRequest: [11, "RpcCall_KeepAliveRequest"],
    StreamWrite: [ 12, "StreamWrite" ],
}
exports.IoCmd = IoCmd

const IoEvent = {
    Invalid : [ 0, "Invalid" ],
    OnKeepAlive : [ 1, "RpcEvt_OnKeepAlive" ],
    ForwardEvent : [ 2, "RpcEvt_ForwardEvent" ],
    OnRmtSvrOffline : [ 3, "OnRmtSvrOffline" ],
    StreamRead: [ 4, "StreamRead" ],
}

exports.IoEvent = IoEvent

class CIoMessageBase
{
    constructor()
    {
        this.m_oReq = null
        this.m_iMsgId = -1 
        this.m_oResp = null
        this.m_iType = null
        this.m_dwPortId = -1
    }

    IsRequest()
    {
        return ( this.m_iType === IoMsgType.ReqMsg )
    }

    IsResponse()
    {
        return ( this.m_iType === IoMsgType.RespMsg )
    }

    IsEvent()
    {
        return ( this.m_iType === IoMsgType.EvtMsg )
    }

    GetType()
    { return this.m_iType }
}

exports.CAdminReqMessage = class CAdminReqMessage extends CIoMessageBase
{
    constructor()
    {
        super()
        this.m_iType = IoMsgType.AdminReq
        this.m_iCmd = AdminCmd.Invalid[0]
        this.m_oReq = new CConfigDb2()
    }
}
exports.CAdminRespMessage = class CAdminRespMessage extends CIoMessageBase
{
    constructor( oReqMsg )
    {
        super()
        this.m_iType = IoMsgType.AdminResp
        this.m_iCmd = oReqMsg.m_iCmd
        this.m_iMsgId = oReqMsg.m_iMsgId
        this.m_oResp = new CConfigDb2()
    }
}

exports.CIoReqMessage = class CIoReqMessage extends CIoMessageBase
{
    constructor()
    {
        super()
        this.m_iType = IoMsgType.ReqMsg
        this.m_iCmd = IoCmd.Invalid[0]
        this.m_dwTimeLeftSec = 0
        this.m_oReq = new CConfigDb2()
        this.m_oReqCtx = new CConfigDb2()
    }

    DecTimer( delta )
    {
        if( this.m_dwTimeLeftSec > delta )
            this.m_dwTimeLeftSec -= delta
        else
        {
            this.m_dwTimeLeftSec = 0
        }
        return this.m_dwTimeLeftSec
    }

    ResetTimer()
    {
        this.m_dwTimeLeftSec = this.GetTimeoutSec()
    }

    GetTimeoutSec()
    {
        return this.m_oReq.GetProperty(
                EnumPropId.propTimeoutsec )
    }

    GetObjPath()
    {
        return this.m_oReq.GetProperty(
            EnumPropId.propObjPath )
    }

    GetIfName()
    {
        return this.m_oReq.GetProperty(
            EnumPropId.propIfName )
    }

    GetDestDBusName()
    {
        return this.m_oReq.GetProperty(
            EnumPropId.propDestDBusName )
    }

    GetSender()
    {
        return this.m_oReq.GetProperty(
            EnumPropId.propSrcDBusName )
    }

    GetMethod()
    {
        return this.m_oReq.GetProperty(
            EnumPropId.propMethodName )
    }

    GetCallOptions()
    {
        return this.m_oReq.GetProperty(
            EnumPropId.propCallOptions )
    }

    GetCallFlags()
    {
        opt = this.m_oReq.GetProperty(
            EnumPropId.propCallOptions )
        return opt.GetProperty(
            EnumPropId.propCallFlags )
    }

    GetSerial()
    { return this.m_iMsgId }

    HasReply()
    {
        flags = this.GetCallFlags()
        if( flags & EnumCallFlags.CF_WITH_REPLY )
            return true
        return false
    }

    IsNonDBus()
    {
        flags = this.GetCallFlags()
        if( flags & EnumCallFlags.CF_NON_DBUS )
            return true
        return false
    }

    IsKeepAlive()
    {
        flags = this.GetCallFlags()
        if( flags & EnumCallFlags.CF_KEEP_ALIVE )
            return true
        return false
    }

    GetKeepAliveSec()
    {
        return this.m_oReq.GetProperty(
            EnumPropId.propKeepAliveSec )
    }

    GetTaskId()
    {
        return this.m_oReq.GetProperty(
            EnumPropId.propTaskId )
    }
}

exports.CIoRespMessage = class CIoRespMessage extends CIoMessageBase
{
    constructor( oReqMsg )
    {
        super()
        this.m_iMsgId = oReqMsg.m_iMsgId
        this.m_iType = IoMsgType.RespMsg
        this.m_iCmd = IoCmd.Invalid[0]
        this.m_dwTimeLeftSec = 0
        this.m_oResp = new CConfigDb2()
        this.m_oReqMsg = oReqMsg
    }

    GetReturnValue()
    {
        if( this.m_oResp === null )
            return null
        return this.m_oResp.GetProperty(
            EnumPropId.propReturnValue )
    }
}

exports.CIoEventMessage = class CIoEventMessage extends CIoMessageBase
{
    constructor()
    {
        super()
        this.m_iMsgId = 0
        this.m_iType = IoMsgType.EvtMsg
        this.m_iCmd = IoEvent.Invalid
        this.m_oReq = new CConfigDb2()
    }
}

class CPendingRequest
{
    constructor( oReq )
    {
        this.m_oResolve = null
        this.m_oReject = null
        this.m_oReq = oReq
        this.m_oResp = null
        this.m_oObject = null
    }

    OnTaskComplete( oResp )
    {
        if( this.m_oResolve === null )
            return
        this.m_oResp = oResp
        this.m_oResolve( this )
    }

    OnCanceled( iRet )
    {
        if( this.m_oReject === null)
            return
        var oResp = new CConfigDb2()
        oResp.SetUint32(
            EnumPropId.propReturnValue, iRet )
        this.m_oResp = oResp
        this.m_oReject( this )
    }
}
exports.CPendingRequest = CPendingRequest