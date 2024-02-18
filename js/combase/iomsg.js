const { EnumPropId, EnumCallFlags } = require("./enums.js");
const { CConfigDb2 } = require("./configdb.js");

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
    OnKeepAlive : [ 1, "RpcEvt_KeepAlive" ],
    ForwardEvent : [ 2, "RpcEvt_ForwardEvent" ],
    OnRmtSvrOffline : [ 3, "OnRmtSvrOffline" ],
    StreamRead: [ 4, "StreamRead" ],
    StreamClosed: [ 5, "StreamClosed" ],
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
        this.m_iCmd = -1
    }

    Restore( oMsg ){
        this.m_iMsgId = oMsg.m_iMsgId
        this.m_iType = oMsg.m_iType
        this.m_dwPortId = oMsg.m_dwPortId
        this.m_iCmd = oMsg.m_iCmd
        if( oMsg.m_oReq !== null )
        {
            var oReq = new CConfigDb2()
            oReq.Restore( oMsg.m_oReq )
            this.m_oReq = oReq
        }
        if( oMsg.m_oResp !== null )
        {
            var oResp = new CConfigDb2()
            oReq.Restore( oMsg.m_oResp )
            this.m_oResp = oResp
        }
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

    GetPortId()
    { return this.m_dwPortId }
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

class CIoReqMessage extends CIoMessageBase
{
    constructor()
    {
        super()
        this.m_iType = IoMsgType.ReqMsg
        this.m_iCmd = IoCmd.Invalid[0]
        this.m_dwTimerLeftMs = 0
        this.m_oReq = new CConfigDb2()
    }

    Restore(oMsg)
    {
        if( oMsg === null || oMsg === undefined )
            return
        super.Restore(oMsg)
        this.m_dwTimerLeftMs = oMsg.m_dwTimerLeftMs
    }

    DecTimer( delta )
    {
        if( this.m_dwTimerLeftMs > delta )
            this.m_dwTimerLeftMs -= delta
        else
        {
            this.m_dwTimerLeftMs = 0
        }
        return this.m_dwTimerLeftMs
    }

    ResetTimer()
    {
        this.m_dwTimerLeftMs = this.GetTimeoutSec() * 1000
    }

    GetTimeoutSec()
    {
        var oCallOpt = this.m_oReq.GetProperty( 
            EnumPropId.propCallOptions )
        if( oCallOpt === null )
            return 0
        return  oCallOpt.GetProperty(
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
exports.CIoReqMessage = CIoReqMessage
class CIoRespMessage extends CIoMessageBase
{
    constructor( oReqMsg )
    {
        super()
        if( oReqMsg !== undefined )
        {
            this.m_iMsgId = oReqMsg.m_iMsgId
            this.m_iType = IoMsgType.RespMsg
            this.m_iCmd = oReqMsg.m_iCmd
            this.m_oResp = new CConfigDb2()
            this.m_oReq = oReqMsg
        }
        else
        {
            this.m_iType = IoMsgType.RespMsg
            this.m_oResp = new CConfigDb2()
        }

    }

    Restore(oMsg){
        this.m_iMsgId = oMsg.m_iMsgId
        this.m_iType = oMsg.m_iType
        this.m_dwPortId = oMsg.m_dwPortId
        this.m_iCmd = oMsg.m_iCmd
        if( oMsg.m_oReq !== null )
        {
            var oReq = new CIoReqMessage()
            oReq.Restore( oMsg.m_oReq )
            this.m_oReq = oReq
        }
        if( oMsg.m_oResp !== null )
        {
            var oResp = new CConfigDb2()
            oResp.Restore( oMsg.m_oResp )
            this.m_oReq = oResp
        }
    }
    GetReturnValue()
    {
        if( this.m_oResp === null )
            return null
        return this.m_oResp.GetProperty(
            EnumPropId.propReturnValue )
    }
}
exports.CIoRespMessage = CIoRespMessage
class CIoEventMessage extends CIoMessageBase
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
exports.CIoEventMessage = CIoEventMessage
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