const { CConfigDb2 } = require("../combase/configdb")
const { Pair, ERROR, InvalFunc, SUCCEEDED } = require("../combase/defines")
const { constval, errno, EnumPropId, EnumProtoId, EnumStmId, EnumTypeId, EnumCallFlags, EnumIfState, EnumMatchType } = require("../combase/enums")
const { IoCmd, IoMsgType, CAdminReqMessage, CAdminRespMessage, CIoRespMessage, CIoReqMessage, CIoEventMessage, CPendingRequest, AdminCmd, IoEvent } = require("../combase/iomsg")
const { DBusIfName, DBusDestination, DBusDestination2, DBusObjPath } = require("../rpc/dmsg")
const { CMessageMatch } = require( "../combase/msgmatch")
const { EnableEventLocal } = require( "./enablevt")
const { ForwardRequestLocal } = require("./fwrdreq")
const { ForwardEventLocal } = require("./fwrdevt")
const { UserCancelRequest } = require("./cancelrq")
const { OnKeepAliveLocal } = require("./keepalive")
const { OpenStreamLocal, CloseStreamLocal } = require("./openstm")
const { OnDataReceivedLocal, OnStreamClosedLocal, NotifyDataConsumed} = require( "./stmevt")
const { StreamWriteLocal } = require( "./stmwrite")
const { LoginLocal } = require( "./login")

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
        this.m_dwFetchDataTimeout = 0
        this.m_setStreams = new Set()
        this.m_bCompression = false
        /**
         * This function is called when there are data blocks coming from the
         * stream Channel `hStream`. You need override this callback to process
         * the incoming data blocks. If the callback returns
         * `errno.STATUS_PENDING`, the stream receiving will be held back till
         * function `this.m_funcNotifyDataConsumed` is called
         * @param {number} hStream the stream channel that has incoming data
         * @param {Buffer} oBuf  the byte block arrived
         * @returns {none}          
         */
        this.OnDataReceived = ( hStream, oBuf )=>{}
        /**
         * This function is called when the stream channel `hStream` is closed.
         * override this callback when it is necessary.
         * @param {number} hStream the stream channel closed
         * @returns {none}          
         */
        this.OnStmClosed = (hStream)=>{}
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
        this.m_funcOpenStream = OpenStreamLocal.bind( this )
        this.m_funcCloseStream = CloseStreamLocal.bind( this )
        this.m_funcNotifyDataConsumed = NotifyDataConsumed.bind( this )
        this.m_funcStreamWrite = StreamWriteLocal.bind( this )
        this.m_funcLogin = LoginLocal.bind( this )

        var oEvtTab = this.m_arrDispTable
        for( var i = 0; i< Object.keys(IoEvent).length;i++)
        { oEvtTab.push(InvalFunc) }

        oEvtTab[ IoEvent.ForwardEvent[0]] = ForwardEventLocal.bind( this)
        oEvtTab[ IoEvent.OnKeepAlive[0]] = OnKeepAliveLocal.bind( this)
        oEvtTab[ IoEvent.StreamRead[0]] = OnDataReceivedLocal.bind( this )
        oEvtTab[ IoEvent.StreamClosed[0]] = OnStreamClosedLocal.bind( this )
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

        var oParams = new CConfigDb2()
        oParams.SetUint32( EnumPropId.propTimeoutSec,
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
        var ret = errno.STATUS_SUCCESS
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

            if( elem[ "Compression"] === "true")
            { this.m_bCompression = true }

            if( elem[ "AuthInfo"] !== undefined )
            {
                var oAuth = elem[ "AuthInfo" ]
                if( oAuth[ "AuthMech"] === "krb5")
                    throw new Error(
                        `Error krb5 authentication is not supported in this version`)
                if( oAuth[ "AuthMech"] === "SimpAuth" ||
                    oAuth[ "AuthMech" ] === "OAuth2" )
                {
                    globalThis.g_bAuth = true
                    globalThis.g_strAuthMech = oAuth[ "AuthMech" ]
                }
            }

            for( var interf of elem[ "Interfaces"])
            {
                var bDummy = interf[ "DummyInterface"]
                if( bDummy !== undefined && bDummy === "true")
                    continue
                var strVal = interf[ "InterfaceName"]
                if( strVal === "IUnknown")
                    continue
                if( strVal === "IStream" &&
                    interf[ "FetchDataTimeout"] !== undefined )
                {
                    this.m_dwFetchDataTimeout = Number( interf[ "FetchDataTimeout"] )
                }
                else
                {
                    this.m_dwFetchDataTimeout = this.m_dwTimeoutSec
                }
                var strIfName = DBusIfName( strVal )
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
            ret = -errno.EFAULT
        }
        return ret
    }

    async LoadObjDesc( strObjDesc )
    {
        try {
            var ret = errno.STATUS_SUCCESS
            const response = await fetch(strObjDesc)
            const o = await response.json()
            ret = this.LoadParams(o)
            return Promise.resolve( ret )
        } catch (e) {
            console.log(e.message)
            ret = -errno.EFAULT
            return Promise.reject( ret )
        }
    }

    GetPortId()
    { return this.m_dwPortId }

    GetUrl()
    { return this.m_strUrl }

    EnableEvents()
    {
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

            }).catch((e)=>{
                var oResp = e.m_oResp
                var ret = oResp.GetProperty(
                    EnumPropId.propReturnValue )
                this.DebugPrint("Error, EnableEvent failed (" + ret + " )")
                this.m_iState = EnumIfState.stateStartFailed
                return Promise.resolve( ret)
            })
    }

    GetLoginCredentialFromInput()
    {
        return new Promise((resolve) => {
            // Create modal background
            const modalBg = document.createElement('div');
            modalBg.style.position = 'fixed';
            modalBg.style.top = 0;
            modalBg.style.left = 0;
            modalBg.style.width = '100vw';
            modalBg.style.height = '100vh';
            modalBg.style.background = 'rgba(0,0,0,0.3)';
            modalBg.style.display = 'flex';
            modalBg.style.alignItems = 'center';
            modalBg.style.justifyContent = 'center';
            modalBg.style.zIndex = 9999;

            // Create modal form
            const modal = document.createElement('div');
            modal.style.background = '#fff';
            modal.style.padding = '24px';
            modal.style.borderRadius = '8px';
            modal.style.boxShadow = '0 2px 12px rgba(0,0,0,0.2)';
            modal.innerHTML = `
                <h3>Login</h3>
                <form id="loginForm" autocomplete="off">
                    <div style="margin-bottom:10px;">
                        <input id="loginUser" type="text" placeholder="Username" style="width:200px;padding:6px;" required />
                    </div>
                    <div style="margin-bottom:10px;">
                        <input id="loginPass" type="password" placeholder="Password" style="width:200px;padding:6px;" required />
                    </div>
                    <div style="text-align:right;">
                        <button type="submit" style="margin-right:8px;">OK</button>
                        <button type="button" id="loginCancel">Cancel</button>
                    </div>
                </form>
            `;
            modalBg.appendChild(modal);
            document.body.appendChild(modalBg);

            // Focus username input
            setTimeout(() => document.getElementById('loginUser').focus(), 0);

            // Handle form submit
            modal.querySelector('#loginForm').onsubmit = function(e) {
                e.preventDefault();
                const user = document.getElementById('loginUser').value;
                const pass = document.getElementById('loginPass').value;
                document.body.removeChild(modalBg);
                resolve({ userName: user, password: pass });
            };

            // Handle cancel
            modal.querySelector('#loginCancel').onclick = function() {
                document.body.removeChild(modalBg);
                resolve(null);
            };
        });
    }

    async generateSHA256Hash( input )
    {
        const encoder = new TextEncoder();
        const data = encoder.encode(input);
        const hashBuffer = await crypto.subtle.digest('SHA-256', data);
        const hashArray = Array.from(new Uint8Array(hashBuffer));
        const hashHex = hashArray.map(b => b.toString(16).padStart(2, '0')).join('').toUpperCase();
        return hashHex;
    }

    async generateSHA256Block( input )
    {
        const encoder = new TextEncoder();
        const data = encoder.encode(input);
        const hashBuffer = await crypto.subtle.digest('SHA-256', data);
        return hashBuffer;
    }

    Start()
    {
        return this.LoadObjDesc( this.m_strObjDesc).then((retval)=>{
            if(ERROR(retval))
                return Promise.reject(retval)
            if( globalThis.g_bAuth &&
                globalThis.g_strAuthMech === "SimpAuth" )
            {
                return this.GetLoginCredentialFromInput().then((oCred)=>{
                    if( oCred === null )
                    {
                        this.DebugPrint("Error, Login canceled by user")
                        return Promise.reject( -errno.EFAULT )
                    }
                    this.m_strUserName = oCred.userName
                    return this.generateSHA256Hash( oCred.password ).then((hash)=>{
                        this.m_strKey = hash.toUpperCase()
                        return this.generateSHA256Hash( oCred.userName ).then((userHash)=>{
                            this.m_strKey += userHash.toUpperCase() 
                            return this.generateSHA256Hash( this.m_strKey ).then((finalHash)=>{
                                this.m_strKey = finalHash.toUpperCase()
                                return this.OpenRemotePort( this.m_strUrl ).then((e)=>{
                                    if( globalThis.g_bAuth )
                                        return this.m_funcLogin( undefined, e.m_oResp ).then((retval)=>{
                                            if( ERROR( retval ) )
                                                return Promise.resolve( retval )
                                            return this.EnableEvents().then((e)=>{
                                                return Promise.resolve(e)
                                            }).catch((e)=>{
                                                return Promise.resolve( -errno.EFAULT )
                                            })
                                        }).catch((e)=>{
                                            this.DebugPrint("Error, Login failed ( " + e + " )")
                                            return Promise.resolve(-errno.EFAULT)
                                        })
                                    else
                                        return this.EnableEvents().then((e)=>{
                                            return Promise.resolve(e)
                                        }).catch((e)=>{
                                            this.DebugPrint("Error, EnableEvent failed ( " + e + " )")
                                            return Promise.resolve( -errno.EFAULT )
                                        })
                                }).catch((e)=>{
                                    this.DebugPrint("Error, OpenRemotePort failed ( " + e + " )")
                                    return Promise.resolve(-errno.EFAULT)
                                })
                            })
                        })
                    }).catch((e)=>{
                        this.DebugPrint("Error, GetLoginCredentialFromInput failed ( " + e + " )")
                        return Promise.reject( -errno.EFAULT )
                    })
                })
            }

            return this.OpenRemotePort( this.m_strUrl ).then((e)=>{
                if( globalThis.g_bAuth )
                    return this.m_funcLogin( undefined, e.m_oResp ).then((retval)=>{
                        if( ERROR( retval ) )
                            return Promise.resolve( retval )
                        return this.EnableEvents().then((e)=>{
                            return Promise.resolve(e)
                        }).catch((e)=>{
                            return Promise.resolve( -errno.EFAULT )
                        })
                    }).catch((e)=>{
                        this.DebugPrint("Error, Login failed ( " + e + " )")
                        return Promise.resolve(e)
                    })
                else
                    return this.EnableEvents().then((e)=>{
                        return Promise.resolve(e)
                    }).catch((e)=>{
                        this.DebugPrint("Error, EnableEvent failed ( " + e + " )")
                        return Promise.resolve( -errno.EFAULT )
                    })
                })
        }).catch((e)=>{
            this.DebugPrint("Error, LoadObjDesc failed ( " + e + " )")
            return Promise.resolve(e)
        })
    }

    Stop( ret )
    {
        var stms = []
        for( var hStream of this.m_setStreams )
            stms.push( hStream )

        for( var hStream of stms )
            this.m_funcCloseStream( hStream )
        this.m_setStreams.clear()
        return this.m_oIoMgr.UnregisterProxy(
            this, ret ).then((e)=>{
            this.m_iState = EnumIfState.stateStopped
            return Promise.resolve(e);
        }).catch((e)=>{
            return Promise.reject(e);
        });
    }

    OpenRemotePort( strUrl )
    {
        var oMsg = new CAdminReqMessage()
        oMsg.m_iMsgId = globalThis.g_iMsgIdx++
        oMsg.m_iCmd = AdminCmd.OpenRemotePort[0]
        oMsg.m_oReq.SetString(
            EnumPropId.propDestUrl, strUrl)
        var bAuth = false
        if( globalThis.g_bAuth )
            bAuth = globalThis.g_bAuth
        oMsg.m_oReq.SetBool(
            EnumPropId.propHasAuth, bAuth )
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

    DebugPrint( strMsg )
    {
        console.log( "[ " + Date.now() + "-" + this.m_strSvrName + " ]: " + strMsg  )
    }

    AddStream( hStream )
    {
        this.m_setStreams.add( hStream )
        this.m_oIoMgr.RegisterStream( hStream, this )
    }

    StopStream( hStream )
    {
        var oMsg = new CIoReqMessage()
        oMsg.m_iCmd = IoCmd.CloseStream[0]
        oMsg.m_iMsgId = globalThis.g_iMsgIdx
        oMsg.m_dwPortId = this.GetPortId()
        oMsg.m_oReq.Push(
            {t: EnumTypeId.typeUInt64, v: BigInt( hStream )})
        this.m_oIoMgr.PostMessage( oMsg )
    }

    RemoveStream( hStream )
    {
        this.StopStream( hStream )
        this.m_setStreams.delete( hStream )
        this.m_oIoMgr.UnregisterStream( hStream )
    }

}
