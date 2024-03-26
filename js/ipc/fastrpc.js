const { CConfigDb2 } = require("../combase/configdb")
const { Pair, ERROR, InvalFunc } = require("../combase/defines")
const { constval, errno, EnumPropId, EnumProtoId, EnumStmId, EnumTypeId, EnumCallFlags, EnumIfState, EnumMatchType } = require("../combase/enums")
const { IoCmd, IoMsgType, CAdminReqMessage, CAdminRespMessage, CIoRespMessage, CIoReqMessage, CIoEventMessage, CPendingRequest, AdminCmd, IoEvent } = require("../combase/iomsg")
const { CDBusMessage, DBusIfName, DBusDestination, DBusDestination2, DBusObjPath } = require("../rpc/dmsg")
const { CMessageMatch } = require( "../combase/msgmatch")
const { ForwardRequestLocal } = require("./fwrdreq")
const { ForwardEventLocal } = require("./fwrdevt")
const { CInterfaceProxy } = require("./proxy")
const { OpenStreamLocal, CloseStreamLocal } = require("./openstm")
const { OnDataReceivedLocal, OnStreamClosedLocal, NotifyDataConsumed} = require( "./stmevt")
const { StreamWriteLocal } = require( "./stmwrite")

const { marshall, unmarshall } = require("../dbusmsg/message")
const { messageType } = require( "../dbusmsg/constants")

class CFastRpcChanProxy extends CInterfaceProxy
{
    constructor( oMgr, strObjDesc, strObjName, oParams )
    {
        var oParent = oParams.GetPropery(
            EnumPropId.propParentPtr)
        if( !oOwner )
            throw new Error( "Error parent object is not given")
        this.m_oParent = oParent;
        this.OnDataReceived = this.OnDataReceivedImpl.bind(this);
        this.OnStreamClosed = this.OnStmClosedImpl.bind(this);
        this.m_mapPendingReqs = new Map();
        this.m_funcForwardRequest = this.ForwardRequest.bind( this );
    }

    OnDataReceivedImpl( hStream, oBuf )
    {
        var ret = 0;
        try{
            var dmsg = new CDBusMessage()
            dmsg.Restore( unmarshall(
                oBuf.slice( 4 ) ) )
            
            var dmsg = new CDBusMessage()
            dmsg.Restore( unmarshall(
                oPending.m_oResp.slice( 4 ) ) )
            
            if( dmsg.GetType() === messageType.methodReturn )
            {
                var iRet = dmsg.GetArgAt( 0 )
                oResp = new CConfigDb2()
                if( ERROR( iRet ) )
                {
                    oResp.SetUint32(
                        EnumPropId.propReturnValue, iRet )
                    ret = iRet
                }
                else
                {
                    var oBuf = dmsg.GetArgAt( 1 )
                    oResp.Deserialize( oBuf, 0 )
                }

                var ret = oResp.GetProperty(
                    EnumPropId.propReturnValue )
                if( ret === null || ret === undefined )
                    return

                var iMsgId = dmsg.GetReplySerial();
                var oPending = this.m_mapPendingReqs.get( iMsgId );
                if( oPending === undefined )
                    return;
                oPending.m_oResp = oResp;
                this.OnRespReceived( oPending );
            }
            else if( dmsg.GetType() === messageType.signal )
            {
                this.OnEventReceived( dmsg );
            }
            else
            {
                throw new ERROR( "unknown message");
            }

        }catch( e ){
            console.log( e );
        }
    }

    OnStmClosedImpl( hStream )
    {}

    OnRespReceived( oPending )
    {
        try{
            if( oPending.m_oCallback !== undefined )
                oPending.m_oCallback( oPending.m_oResp );
        }catch( e ){
        }
    }

    OnEventReceived( dmsg, oEvent )
    {
        var oEvtReq = new CConfigDb2();
        var strVal = dmsg.GetSender()
        if( strVal != undefined )
            oEvtReq.SetString(
                EnumPropId.propSrcDBusName, strVal )
        strVal = dmsg.GetObjPath()
        if( strVal != undefined )
            oEvtReq.SetString(
                EnumPropId.propObjPath, strVal )
        strVal = dmsg.GetMember()
        if( strVal != undefined )
            oEvtReq.SetString(
                EnumPropId.propMethodName, strVal )
        strVal = dmsg.GetDestination() 
        if( strVal != undefined )
            oEvtReq.SetString(
                EnumPropId.propDestDBusName, strVal )
        strVal = dmsg.GetSerial()
        if( strVal !== undefined )
            oEvtReq.SetUint32(
                EnumPropId.propSeqNo, strVal )
        strVal = dmsg.GetInterface()
        if( strVal !== undefined )
            oEvtReq.SetString(
                EnumPropId.propIfName, strVal )

        var oParamBuf = dmsg.GetArgAt(0)
        var oParams = new CConfigDb2()
        oParams.Deserialize( oParamBuf )
        oEvtReq.Push( {t: EnumTypeId.typeObj, v:oParams })

        var ioEvt = new CIoEventMessage();
        var oParent = this.m_oParent;
        ioEvt.m_oReq = oEvtReq;
        oParent.m_arrDispTable[ IoEvent.ForwardEvent[0] ]( ioEvt );
    }

    BuildDBusMsgToFwrd( oReq )
    {
        var dmsg = new CDBusMessage( messageType.methodCall )
        dmsg.SetDestination(
            oReq.GetProperty( EnumPropId.propDestDBusName ) )
        oReq.RemoveProperty( EnumPropId.propDestDBusName)
        dmsg.SetSender( oReq.GetProperty(
            EnumPropId.propSrcDBusName ) )
        oReq.RemoveProperty( EnumPropId.propSrcDBusName)
        dmsg.SetInterface( oReq.GetProperty(
            EnumPropId.propIfName ) )
        oReq.RemoveProperty( EnumPropId.propIfName)
        dmsg.SetObjPath( oReq.GetProperty(
            EnumPropId.propObjPath ) )
        oReq.RemoveProperty( EnumPropId.propObjPath)
        dmsg.SetMember( oReq.GetProperty(
                EnumPropId.propMethodName ) )
        oReq.RemoveProperty( EnumPropId.propMethodName )
        dmsg.SetSerial( oReq.GetProperty(
                EnumPropId.propSeqNo ) )
        oReq.RemoveProperty( EnumPropId.propSeqNo )

        var oCallOptions = oReq.GetProperty(
            EnumPropId.propCallOptions )
        var dwFlags = oCallOptions.GetProperty(
            EnumPropId.propCallFlags )
        if( ( dwFlags & EnumCallFlags.CF_WITH_REPLY ) === 0 )
            dmsg.SetNoReply()

        var oBuf = oReq.Serialize()
        var arrBody = []
        arrBody.push( new Pair( { t: "ay", v: oBuf }))
        dmsg.AppendFields( arrBody )
        return dmsg
    }

    ForwardRequest( oReq, oCallback, oContext )
    {
        return new Promise( (resolve, reject)=>{
                var dmsg = this.BuildDBusMsgToFwrd( oReq );
                var oMsgBuf = marshall( dmsg );
                var oPending = new CPendingRequest();
                oPending.m_oResolve = resolve;
                oPending.m_oReject = reject;
                oPending.m_oReq = oReq;
                oPending.m_oCallback =
                    oCallback.bind( this, oContext);
                var oSizeBuf = Buffer.alloc(4);
                oSizeBuf.writeUint32BE( oMsgBuf.length );
                return this.m_funcStreamWrite( this.m_hStream,
                    Buffer.concat( [ oSizeBuf, oMsgBuf ]) ).then((e)=>{
                    if( ERROR( e ) )
                    {
                        oPending.OnCanceled( e );
                    }
                    else
                    {
                        if( dmsg.IsNoReply() )
                        {
                            var oResp = new CConfigDb2();
                            oResp.SetUint32(
                                EnumPropId.propReturnValue, 0);
                            return oPending.OnTaskComplete( oResp );
                        }
                        var iMsgId = dmsg.GetSerial();
                        this.m_mapPendingReqs.set( iMsgId, oPending )
                    }
                }).cache((e)=>{
                    var ret = 0;
                    if( e.m_oResp === undefined )
                    {
                        ret = -errno.EFAULT;
                    }
                    else
                    {
                        ret = e.m_oResp.GetProperty(
                            EnumPropId.propReturnValue )
                    }
                    oPending.OnCanceled( ret );
                })
        })
    }

    Stop( reason )
    {
        var arrKeys = []
        var oResp = new CConfigDb2();
        if( reason == undefined )
            reason = errno.ERROR_PORT_STOPPED;
        oResp.SetUint32(
            EnumPropId.propReturnValue, reason)
        for( [key, oPending] of this.m_mapPendingReqs )
        {
            if( oPending.m_oObject === oProxy )
            {
                oPending.m_oResp = oResp
                oPending.m_oReject( oPending )
                arrKeys.push( key )
            }
        }
        for( key of arrKeys )
            this.m_mapPendingReqs.delete( key )
        super.Stop( reason );
    }
}

class CFastRpcProxy extends CInterfaceProxy
{
    constructor( oMgr, strObjDesc, strObjName, oParams )
    {
        super( oMgr, strObjDesc, strObjName, oParams );
        var strChannName = strObjName + "_ChannelSvr";
        this.m_oChanProxy = new CFastRpcChanProxy(
            oMgr, strObjDesc, strChannName, oParams );
        this.m_funcForwardRequest =
            this.ForwardRequest.bind(this);
        this.m_funcEnableEvent =
            this.EnableEvent.bind( this );
    }

    Start()
    {
        // start the channel client first
        var oChanCb = function( retVal )
        {
            if( ERROR( retVal ) )
                return retVal;
            
            var oStmCtx;
            oStmCtx.m_oProxy = this;
            return this.m_funcOpenStream(oStmCtx).then((oCtx)=>{
                if( ERROR( oCtx.m_iRet ) )
                    return Promise.reject( oCtx );
                this.m_hStream = oCtx.m_hStream;
                // start the master proxy
                return this.super.Start();
            }).then((retVal)=>{
                if( ERROR( retVal))
                {
                    oStmCtx.m_iRet = retVal;
                    return Promise.reject( oStmCtx );
                }
                return Promise.resolve( retVal );
            }).catch((e)=>{
                this.m_iState = EnumIfState.stateStartFailed;
                var ret = -errno.EFAULT
                if( e.m_iRet !== undefined )
                    ret = e.m_iRet;
                else
                    ret = -errno.EFAULT
                return Promise.resolve( ret );
            })
        }
        return this.m_oChanProxy.Start().then(
            oChanCb.bind( this.m_oChanProxy )
            ).catch( (retVal)=>{
                this.m_iState = EnumIfState.stateStartFailed;
                console.log( "Error, Channel setup failed" );
                return Promise.resolve( retVal );
            })
    }

    ForwardRequest(  oReq, oCallback, oContext )
    {
        return this.m_oChanProxy.ForwardRequest(
            oReq, oCallback, oContext );
    }

    EnableEvent( idx )
    {
        var oResp = CConfigDb2();
        oResp.SetUint32( EnumPropId.propReturnValue, 0);
        return Promise.resolve(oResp);
    }

    Stop( reason )
    {
        this.m_oChanProxy.Stop( reason );
        super.Stop( reason )
    }
}

exports.CFastRpcProxy = CFastRpcProxy