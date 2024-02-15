const { CConfigDb2 } = require("../combase/configdb")
const { Pair, ERROR, InvalFunc } = require("../combase/defines")
const { constval, errno, EnumPropId, EnumProtoId, EnumStmId, EnumTypeId, EnumCallFlags, EnumIfState, EnumMatchType } = require("../combase/enums")
const { IoCmd, IoMsgType, CAdminReqMessage, CAdminRespMessage, CIoRespMessage, CIoReqMessage, CIoEventMessage, CPendingRequest, AdminCmd, IoEvent } = require("../combase/iomsg")
const { DBusIfName, DBusDestination, DBusDestination2, DBusObjPath } = require("../rpc/dmsg")
const { CMessageMatch } = require( "../combase/msgmatch")
const { EnableEventLocal } = require( "./enablevt")
const { ForwardRequestLocal } = require("./fwrdreq")
const { ForwardEventLocal } = require("./fwrdevt")
const { UserCancelRequest } = require("./cancelrq")
const { OnKeepAliveLocal } = require("./keepalive")

exports.CInterfaceProxy = class CInterfaceProxy
{
    constructor( oMgr, strObjDesc, strObjName, oParams )
    {
        this.m_strUrl = ""
        this.m_iState = EnumIfState.stateStarting
        this.m_oIoMgr = oMgr
        this.m_dwPortId = 0
        this.m_arrDispTable = []
        this.m_strObjDesc = strObjDesc
        this.m_strObjPath = ""
        this.m_strDest = ""
        this.m_strRouterPath = ""
        this.m_arrStreams = []
        this.m_strSvrName = ""
        this.m_strObjInstName = ""
        this.m_strObjName = strObjName
        this.m_arrMatches = []
        this.m_strSender = ""
        this.m_dwTimeoutSec = 0
        this.m_dwKeepAliveSec = 0
        this.m_mapEvtHandlers = new Map()
        if( oParams !== undefined )
        {
            var oVal = oParams.GetProperty(
                EnumPropId.propSvrInstName )
            if( oVal !== null && oVal.length > 0 )
                this.m_strSvrName = oVal
            oVal = oParams.GetProperty(
                EnumPropId.propObjInstName )
            if( oVal !== null && oVal.length > 0 )
                this.m_strObjInstName = oVal
        }
        if( this.m_strObjInstName.length === 0)
            this.m_strObjInstName = strObjName
        this.BindFunctions()
    }

    BindFunctions()
    {
        this.m_funcEnableEvent = EnableEventLocal.bind( this )
        this.m_funcForwardRequest = ForwardRequestLocal.bind( this )
        this.m_funcCancelRequest = UserCancelRequest.bind( this )

        var oEvtTab = this.m_arrDispTable
        for( var i = 0; i< Object.keys(IoEvent).length;i++)
        { oEvtTab.push(InvalFunc) }

        oEvtTab[ IoEvent.ForwardEvent[0]] = ForwardEventLocal.bind( this)
        oEvtTab[ IoEvent.OnKeepAlive[0]] = OnKeepAliveLocal.bind( this)
    }

    InitReq( oReq, strMethodName )
    {
        oReq.SetString( EnumPropId.propRouterPath,
            this.m_strRouterName )
        oReq.SetUint32( EnumPropId.propSeriProto,
            EnumSeriProto.seriNone ) 
        oReq.SetString( EnumPropId.propDestDBusName,
            this.m_strDest )
        oReq.SetString( EnumPropId.propSrcDBusName,
            this.m_strSender )
        oReq.SetString( EnumPropId.propIfName,
           this.m_strIfName )
        oReq.SetString( EnumPropId.propObjPath,
            this.m_strObjPath )
        if( strMethodName !== undefined)
            oReq.SetString( EnumPropId.propMethodName,
                strMethodName )

        oParams = new CConfigDb2()
        oParams.SetUint32( EnumPropId.propTimeoutsec,
            this.m_dwTimeoutSec )
        oParams.SetUint32( EnumPropId.propKeepAliveSec,
            this.m_dwKeepAliveSec )
        oParams.SetUint32( EnumPropId.propCallFlags,
            EnumPropId.propCallFlags,
            EnumCallFlags.CF_ASYNC_CALL |
            EnumCallFlags.CF_WITH_REPLY |
            messageType.methodCall )
        oReq.SetObjPtr(
            EnumPropId.propCallOptions, oParams )
    }

    LoadParams( o )
    {
        var oVal
        try{
            if( this.m_strSvrName.length === 0 )
            {
                oVal = o["ServerName"]
                if( oVal !== undefined )
                {
                    this.m_strSvrName = oVal
                }
                else
                {
                    throw new Error(
                        `Error 'ServerName' not defined in objdesc file:${this.m_strObjDesc}`)
                }
            }
            var arrObjs = o[ "Objects" ]
            var elem
            for( elem of arrObjs )
            {
                if( elem[ "ObjectName"] !== this.m_strObjName )
                    continue
                break
            }
            if( elem["DestURL"] === undefined )
            {
                throw new Error(
                        `Error 'DestURL' not defined in objdesc file:${this.m_strObjDesc}.${this.m_strObjName}`)
            }
            this.m_strUrl = elem["DestURL"]
            this.m_strUrl =
                this.m_strUrl.replace("http:", "ws:")
            this.m_strUrl =
                this.m_strUrl.replace("https:", "wss:")
            this.m_strObjPath = DBusObjPath(
                this.m_strSvrName, this.m_strObjInstName)

            this.m_strDest = DBusDestination2(
                this.m_strSvrName, this.m_strObjInstName )

            this.m_strSender = this.m_strDest + "_proxy" 
            if( elem[ "Interfaces"] === undefined )
            {
                throw new Error(
                        `Error 'Interfaces' not defined in objdesc file:${this.m_strObjDesc}.${this.m_strObjName}`)
            }
            if( elem[ "RouterPath"] === undefined )
            {
                throw new Error(
                        `Error 'RouterPath' not defined in objdesc file:${this.m_strObjDesc}.${this.m_strObjName}`)
            }
            this.m_strRouterPath = elem[ "RouterPath" ]
            if( elem[ "RequestTimeoutSec"] === undefined )
                this.m_dwTimeoutSec = constval.IFSTATE_DEFAULT_IOREQ_TIMEOUT
            else
                this.m_dwTimeoutSec = Number( elem["RequestTimeoutSec"] )
            if( elem[ "KeepAliveTimeoutSec"] === undefined )
                this.m_dwKeepAliveSec = constval.IFSTATE_DEFAULT_IOREQ_TIMEOUT / 2
            else
                this.m_dwKeepAliveSec = Number( elem[ "KeepAliveTimeoutSec"] )

            for( var interf of elem[ "Interfaces"])
            {
                var bDummy = interf[ "DummyInterface"]
                if( bDummy !== undefined && bDummy === "true")
                    continue
                var strIfName = DBusIfName( interf[ "InterfaceName" ] )
                var match = new CMessageMatch()
                match.SetType( EnumMatchType.matchClient)
                match.SetObjPath( this.m_strObjPath )
                match.SetIfName( strIfName )
                match.SetProperty( EnumPropId.propRouterPath,
                    new Pair({t:EnumTypeId.typeString, v:this.m_strRouterPath}) )
                match.SetProperty( EnumPropId.propDestDBusName,
                    new Pair({t:EnumTypeId.typeString, v:this.m_strDest}) )
                this.m_arrMatches.push( match )
            }
        }
        catch(e)
        {

        }
    }

    LoadObjDesc( strObjDesc )
    {
        return fetch( strObjDesc )
        .then( response=>response.json())
        .then( (o)=>{
            this.LoadParams( o )
        })
        .catch( (e)=>{
            console.log(e.message)
        })
    }

    GetPortId()
    { return this.m_dwPortId }

    GetUrl()
    { return this.m_strUrl }

    Start()
    {
        return this.LoadObjDesc( this.m_strObjDesc).then((e)=>{
            return this.OpenRemotePort( this.m_strUrl ).then((e)=>{
                var proms = []
                for( var i =0; i< this.m_arrMatches.length; i++ )
                    proms.push( this.m_funcEnableEvent( i ) )
                return Promise.all( proms ).then((e)=>{
                    var ret = 0
                    for( var oResp of e )
                    {
                        ret = oResp.GetProperty(
                            EnumPropId.propReturnValue )
                        if( ERROR( ret ) )
                            break
                    }
                    if( ERROR( ret ))
                        return Promise.reject( ret )
                    else
                        return Promise.resolve( ret )
                })
            }).catch((e)=>{
                console.log(e)
                var oResp = e.m_oResp
                var ret = oResp.GetProperty(
                    EnumPropId.propReturnValue )
                return Promise.resolve( errno.ERROR_STATE )
            })
        })
    }

    Stop( ret )
    {
        if( this.GetPortId() === 0 )
        {
            this.m_oIoMgr.UnregisterProxy( this, ret )
            return Promise.resolve(0)
        }
        return new Promise( (resolve, reject)=>{
            var oMsg = new CAdminReqMessage()
            oMsg.m_iMsgId = globalThis.g_iMsgIdx++
            oMsg.m_iCmd = AdminCmd.CloseRemotePort[0]
            oMsg.m_dwPortId = this.GetPortId()
            var oPending = new CPendingRequest(oMsg)
            oPending.m_oReq = oMsg
            oPending.m_oObject = this
            oPending.m_oResolve = resolve
            oPending.m_oReject = reject
            this.PostMessage( oPending )
        }).then((e)=>{
            this.m_oIoMgr.UnregisterProxy( this, ret)
            return Promise.resolve(ret)
        }).catch((e)=>{
            console.log(e)
        })

    }

    OpenRemotePort( strUrl )
    {
        var oMsg = new CAdminReqMessage()
        oMsg.m_iMsgId = globalThis.g_iMsgIdx++
        oMsg.m_iCmd = AdminCmd.OpenRemotePort[0]
        oMsg.m_oReq.SetString(
            EnumPropId.propDestUrl, strUrl)
        return new Promise((resolve, reject) =>{
                var oPending = new CPendingRequest(oMsg)
                oPending.m_oReq = oMsg
                oPending.m_dwPortId = -1
                oPending.m_oObject = this
                oPending.m_oResolve = resolve
                oPending.m_oReject = reject
                this.PostMessage( oPending )
                this.m_oIoMgr.AddPendingReq(
                    oMsg.m_iMsgId, oPending)
        }).then(( e )=>{
            var ret = 0
            const oResp = e.m_oResp
            var ret = oResp.GetProperty(
                EnumPropId.propReturnValue )
            if( ret === null || ERROR( ret ) )
            {
                console.log( "Error OpenRemotePort failed")
                return Promise.reject( e )
            }

            ret = oResp.GetProperty(0)
            if( ret === null || ERROR( ret ))
            {
                console.log( "Error OpenRemotePort invalid response")
                return Promise.reject( e)
            }

            this.m_dwPortId = ret
            this.m_iState = EnumIfState.stateStarted
            this.m_oIoMgr.RegisterProxy( this )
            return Promise.resolve( e )
        }).catch(( e )=>{
            this.m_iState = EnumIfState.stateStartFailed
            return Promise.reject( e )
        })
    }

    DispatchMessage( e )
    {
        var oMsg = e.data
        var oResp = new CConfigDb2()
        oResp.Restore( oMsg.m_oResp )

        if( oMsg.GetType() === IoMsgType.AdminResp ||
            oMsg.GetType() === IoMsgType.RespMsg )
        {
            var oPending =
                this.m_oIoMgr.GetPendingReq( oMsg.m_iMsgId )
            if( oPending !== null )
            {
                oPending.m_oResp = oResp
                oPending.m_oResolve( oPending )
                this.m_oIoMgr.RemovePendingReq(
                    m_oReq.m_dwMsgId )
            }
        }
    }

    PostMessage( oPending )
    {
        var oMsg = oPending.m_oReq
        this.m_oIoMgr.PostMessage( oMsg )
        if( oMsg.GetType() === IoMsgType.AdminReq ||
            oMsg.GetType() === IoMsgType.ReqMsg )
        {
            this.m_oIoMgr.AddPendingReq(
                oMsg.m_iMsgId, oPending)
        }
    }

}
