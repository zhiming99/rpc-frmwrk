const { CConfigDb2 } = require("../combase/configdb")
const { Pair, ERROR, InvalFunc } = require("../combase/defines")
const { constval, errno, EnumPropId, EnumSeriProto, EnumStmId, EnumTypeId, EnumCallFlags, EnumIfState, EnumMatchType } = require("../combase/enums")
const { IoCmd, IoMsgType, CAdminReqMessage, CAdminRespMessage, CIoRespMessage, CIoReqMessage, CIoEventMessage, CPendingRequest, AdminCmd, IoEvent } = require("../combase/iomsg")
const { CDBusMessage, DBusIfName, DBusDestination, DBusDestination2, DBusObjPath } = require("../rpc/dmsg")
const { CMessageMatch } = require( "../combase/msgmatch")
const { CInterfaceProxy } = require("./proxy")
const { OpenStreamLocal, CloseStreamLocal } = require("./openstm")
const { StreamWriteLocal } = require( "./stmwrite")
const { KeepAliveRequest } = require("./keepalive")

const { marshall, unmarshall } = require("../dbusmsg/message")
const { messageType } = require( "../dbusmsg/constants")
const { createHash } = require('crypto-browserify')

class CFastRpcMsg
{
    constructor()
    {
        this.m_dwSize = 0;
        this.m_bType = EnumTypeId.typeObj;
        this.m_oReq = null;
    }

    Serialize()
    {
        var strObjPath = this.m_oReq.GetProperty(
            EnumPropId.propObjPath )
        if( strObjPath.length > 8 &&
            strObjPath.slice( strObjPath.length - 8 ) !== '_SvrSkel')
            strObjPath += "_SvrSkel";
        this.m_oReq.SetString(
            EnumPropId.propObjPath, strObjPath )

        var oHdr = Buffer.alloc( 8 );
        oHdr.writeUint8( this.m_bType, 4 );
        oHdr.writeUint8( 0x46, 5)
        oHdr.writeUint8( 0x52, 6)
        oHdr.writeUint8( 0x4d, 7)
        if( !this.m_oReq )
        {
            throw new Error( "Error, empty FASTRPC message" );
        }

        var oBody = this.m_oReq.Serialize();
        var dstBuf = Buffer.concat( [ oHdr, oBody ]);
        dstBuf.writeUint32BE( dstBuf.length - 4 );
        return dstBuf;
    }

    Deserialize( oBuf, offset )
    {
        if( offset === undefined )
            offset = 0;
        var dwSize = oBuf.readUint32BE( offset );
        if( dwSize + offset + 4 > oBuf.length )
            throw new Error( "Error, invalid FASTRPC message size" );
        offset += 4;
        var iType = oBuf.readUint8( offset )
        if( iType !== EnumTypeId.typeObj )
            throw new Error( "Error, invalid FASTRPC message type" );

        offset++;
        var szMag = [0,0,0]
        szMag[0] = oBuf.readUint8( offset );
        szMag[1] = oBuf.readUint8( offset + 1 );
        szMag[2] = oBuf.readUint8( offset + 2 );
        if( szMag[ 0 ] !== 0x46 ||
            szMag[ 1 ] !== 0x52 ||
            szMag[ 2 ] !== 0x4d )
            throw new Error( "Error, invalid FASTRPC message magic" );
        this.m_dwSize = dwSize;
        this.m_bType = EnumTypeId.typeObj;

        offset += 3;
        this.m_oReq = new CConfigDb2();
        var ret = this.m_oReq.Deserialize( oBuf, offset )
        if( this.GetType() === messageType.signal )
        {
            var strObjPath = this.m_oReq.GetProperty(
                EnumPropId.propObjPath )
            if( strObjPath )
            {
                const regex = /_SvrSkel/g;
                var idx = strObjPath.search( regex );
                if( idx >= 0 )
                {
                    this.m_oReq.SetString( EnumPropId.propObjPath,
                        strObjPath.slice(0, idx))
                }
            }
        }
        return ret;
    }

    GetType()
    {
        var oOpts = this.m_oReq.GetProperty(
            EnumPropId.propCallOptions );
        var dwFlags = oOpts.GetProperty(
            EnumPropId.propCallFlags )
        return ( dwFlags & 0x07 );
    }

    GetReturnValue()
    {
        return this.m_oReq.GetProperty(
            EnumPropId.propReturnValue );
    }

    IsNoReply()
    {
        var oOpts = this.m_oReq.GetProperty(
            EnumPropId.propCallOptions );
        var dwFlags = oOpts.GetProperty(
            EnumPropId.propCallFlags )
        if( dwFlags & EnumCallFlags.CF_WITH_REPLY )
            return false;
        return true;
    }

    GetSerial()
    {
        return this.m_oReq.GetProperty(
            EnumPropId.propSeqNo );
    }

    GetReplySerial()
    {
        return this.m_oReq.GetProperty(
            EnumPropId.propSeqNo2 );
    }

    GetSender()
    {
        return this.m_oReq.GetProperty(
            EnumPropId.propSrcDBusName );
    }

    GetObjPath()
    {
        return this.m_oReq.GetProperty(
            EnumPropId.propObjPath )
    }

    GetMember()
    {
        return this.m_oReq.GetProperty(
            EnumPropId.propMethodName )
    }

    GetDestination()
    {
        return this.m_oReq.GetProperty(
            EnumPropId.propDestDBusName )
    }

    GetSerial()
    {
        return this.m_oReq.GetProperty(
            EnumPropId.propSeqNo )
    }

    GetInterface()
    {
        return this.m_oReq.GetProperty(
            EnumPropId.propIfName )
    }

    GetArgAt( idx )
    {
        return this.m_oReq.GetProperty( idx )
    }
}
class CFastRpcChanProxy extends CInterfaceProxy
{
    constructor( oMgr, strObjDesc, strObjName, oParams )
    {
        super( oMgr, strObjDesc, strObjName, oParams )
        var oParent = oParams.GetProperty(
            EnumPropId.propParentPtr)
        if( !oParent )
            throw new Error( "Error parent object is not given")
        this.m_oParent = oParent;
        this.OnDataReceived = this.OnDataReceivedImpl.bind(this);
        this.OnStreamClosed = this.OnStmClosedImpl.bind(this);
        this.m_mapPendingReqs = new Map();
        this.m_funcForwardRequestNonStream = this.m_funcForwardRequest
        this.m_funcForwardRequest = this.ForwardRequest.bind( this );
        this.m_hStream = null;

        this.m_oScanReqs = ()=>{
            var dwIntervalMs = 10 * 1000
            var expired = []
            var key, val
            for( [ key, val ] of this.m_mapPendingReqs )
            {
                if( val.m_oReq.DecTimer( dwIntervalMs ) > 0 )
                    continue;
                val.OnCanceled( -errno.ETIMEDOUT )
                expired.push( key )
            }
            for( key of expired )
                this.m_mapPendingReqs.delete( key )

            this.m_iTimerId = setTimeout(
                this.m_oScanReqs, dwIntervalMs )
        }
    }

    OnDataReceivedImpl( hStream, oBuf )
    {
        var ret = 0;
        try{
            if( hStream != this.m_hStream )
                return;
            var dmsg = new CFastRpcMsg()
            dmsg.Deserialize( oBuf )
            
            if( dmsg.GetType() === messageType.methodReturn )
            {
                ret = dmsg.GetReturnValue()
                if( ret === null || ret === undefined )
                    return;

                var iMsgId = dmsg.GetReplySerial();
                var oPending = this.m_mapPendingReqs.get( iMsgId );
                if( oPending === undefined )
                    return;
                oPending.m_oResp = dmsg;
                this.OnRespReceived( oPending );
                this.m_mapPendingReqs.delete(iMsgId);
            }
            else if( dmsg.GetType() === messageType.signal )
            {
                this.OnEventReceived( dmsg );
            }
            else
            {
                throw new Error( "unknown message");
            }

        }catch( e ){
            console.log( e );
        }
    }

    OnStmClosedImpl( hStream )
    {
        if( hStream != this.m_hStream )
            return;
        setTimeout(()=>{
            console.log(
                "Warning: the stream channel closed");
            this.m_oParent.Stop(
                errno.ERROR_PORT_STOPPED );
        }, 0 );
    }

    OnRespReceived( oPending )
    {
        try{
            if( oPending.m_oCallback !== undefined )
                oPending.m_oCallback(
                    oPending.m_oResp.m_oReq );
            oPending.OnTaskComplete( oPending.m_oResp )
        }catch( e ){
        }
    }

    OnKeepAlive( oMsg )
    {
        try{
            var oEvent = oMsg.m_oReq
            var strVal = oEvent.GetProperty(
                EnumPropId.propIfName )
            if( strVal !== DBusIfName( "IInterfaceServer") )
                return;
            strVal = oEvent.GetProperty(
                EnumPropId.propObjPath );
            if( strVal !== this.m_oParent.m_strObjPath )
                return;
            var qwTaskId = oEvent.GetProperty( 0 );
            var oPending = this.m_mapPendingReqs.get( Number( qwTaskId ) );
            if( oPending === null )
                return
            var oReq = oPending.m_oReq
            var val = oReq.m_oReq.GetProperty(
                EnumPropId.propObjPath )
            if( val !== this.m_oParent.m_strObjPath + "_SvrSkel" )
                return
            KeepAliveRequest.bind( this.m_oParent )(
                oReq.m_oReq, qwTaskId )
        }
        catch(e)
        {
    
        }
    }
    OnEventReceived( dmsg, oEvent )
    {
        var strMethod = dmsg.GetMember()
        if( strMethod === IoEvent.OnKeepAlive[1] )
        {
            this.OnKeepAlive( dmsg );
        }
        else
        {
            this.ForwardEvent( dmsg );
        }
    }

    BuildFastRpcMsg( oReq )
    {
        var dmsg = new CFastRpcMsg()
        dmsg.m_oReq = oReq;
        return dmsg
    }

    ForwardEvent( oMsg )
    {
        var oParent = this.m_oParent;
        var oEvent = oMsg.m_oReq
        var strIfName = oEvent.GetProperty(
            EnumPropId.propIfName )
        var strObjPath = oEvent.GetProperty(
            EnumPropId.propObjPath )
        var strMethod = oEvent.GetProperty(
            EnumPropId.propMethodName )
        var strKey = ""
        for( var elem of oParent.m_arrMatches )
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
        try{
            var bSeriProto = oEvent.GetProperty(
                EnumPropId.propSeriProto )
            if( bSeriProto === null )
                throw new Error( "Error serialization proto missing")
            if( bSeriProto === EnumSeriProto.seriRidl )
            {
                var ridlBuf = oEvent.GetProperty( 0 )
                if( ridlBuf === null )
                    throw new Error( "Error bad event message")
                oParent.m_mapEvtHandlers.get( strKey )( ridlBuf )
            }
            else if( bSeriProto === EnumSeriProto.seriNone )
            {
                var arrArgs = []
                var oCount = oEvent.GetProperty(
                    EnumPropId.propParamCount )
                if( oCount === null )
                    oCount = 0
                for( var i = 0; i < oCount; i++ )
                {
                    arrArgs.push( oRmtEvt.GetProperty( i ))
                }
                oParent.m_mapEvtHandlers.get( strKey ).apply( this, arrArgs )
            }
            else
            {
                this.DebugPrint( "Error unsupported serialization protocol")
            }
        }
        catch(e)
        {
            console.log( "Error in ForwardEventLocal")
        }
    }
    

    ForwardRequest( oReq, oCallback, oContext )
    {
        if( this.m_iState !== EnumIfState.stateStarted )
        {
            var oPending = new CPendingRequest();
            oPending.m_oReq = oReq;
            oPending.m_oCallback = oCallback;
            oPending.m_oResp = new CConfigDb2();
            oPending.m_oResp.SetUint32(
                EnumPropId.propReturnValue, errno.ERROR_STATE)
            return Promise.reject( oPending )
        }

        var dmsg = this.BuildFastRpcMsg( oReq );
        var oMsgBuf = dmsg.Serialize();
        var ioReq = new CIoReqMessage();
        ioReq.m_iCmd = IoCmd.ForwardRequest[0];
        var oOpts = oReq.GetProperty(
            EnumPropId.propCallOptions );
        var ret = oOpts.GetProperty(
            EnumPropId.propTimeoutSec )
        if( ret !== null )
        {
            ioReq.m_dwTimerLeftMs = ret * 1000;
        }
        else
        {
            ioReq.m_dwTimerLeftMs =
                this.m_oParent.m_dwTimeoutSec * 1000;
        }

        if( oContext !== undefined )
            oContext.m_qwTaskId = dmsg.GetSerial();

        ioReq.m_oReq = oReq;
        ioReq.m_iMsgId = oCallback.m_qwTaskId;

        return new Promise( (resolve, reject)=>{
            var oPending = new CPendingRequest();
            oPending.m_oReq = ioReq;
            oPending.m_oCallback =
                oCallback.bind( this.m_oParent, oContext);
            oPending.m_oResolve = resolve;
            oPending.m_oReject = reject;

            return this.m_funcStreamWrite(
                this.m_hStream, oMsgBuf).then((e)=>{
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
            }).catch((e)=>{
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

    Start()
    {
        return super.Start().then((e)=>{
            this.m_iTimerId = setTimeout(
                this.m_oScanReqs, 10 * 1000 )
            return Promise.resolve(e);
        }).catch((e)=>{
            return Promise.reject(e);
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
        var key, oPending;
        for( [key, oPending] of this.m_mapPendingReqs )
        {
            if( oPending.m_oObject === this )
            {
                oPending.m_oResp = oResp
                oPending.m_oReject( oPending )
                arrKeys.push( key )
            }
        }
        for( key of arrKeys )
            this.m_mapPendingReqs.delete( key )
        if( this.m_iTimerId !== 0 )
        {
            clearTimeout( this.m_iTimerId );
            this.m_iTimerId = 0;
        }
        return super.Stop( reason );
    }

    OpenRequestChannel()
    {
        var oStmCtx = new Object();
        oStmCtx.m_oProxy = this;
        return this.m_funcOpenStream(oStmCtx).then((oCtx)=>{
            if( ERROR( oCtx.m_iRet ) )
                return Promise.reject( oCtx );
            this.m_hStream = oCtx.m_hStream;
            return Promise.resolve( oCtx.m_iRet );
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
}

class CFastRpcProxy extends CInterfaceProxy
{
    constructor( oMgr, strObjDesc, strObjName, oParams )
    {
        super( oMgr, strObjDesc, strObjName, oParams );
        var strChannName = strObjName + "_ChannelSvr";
        if( !oParams )
            oParams = new CConfigDb2();
        oParams.SetObjPtr(
            EnumPropId.propParentPtr, this )

        var strSuffix = "";
        if( this.m_strObjInstName.length > 0 )
        {
            strSuffix = this.m_strObjInstName;
        }
        else
        {
            strSuffix = this.m_strObjName;
        }
        var shasum = createHash('sha1');
        shasum.update( strSuffix );
        var strSum = shasum.digest( 'hex' );
        var strObjInstName = strChannName +
            '_' + strSum.slice( 3 * 8, 4 * 8 ).toUpperCase();
        oParams.SetString( EnumPropId.propObjInstName,
            strObjInstName );

        this.m_oChanProxy = new CFastRpcChanProxy(
            oMgr, strObjDesc, strChannName, oParams );
        this.m_funcForwardRequest =
            this.ForwardRequest.bind(this);
    }

    LoadSkelMatches( o )
    {
        var elem
        var arrObjs = o['Objects']
        var bFound = false;
        for( elem of arrObjs )
        {
            if( elem[ "ObjectName"] !== this.m_strObjName + "_SvrSkel" )
                continue
            bFound = true;
            break
        }
        if( !bFound )
            throw new Error( "Error cannot find the svrskel object")

        for( var interf of elem[ "Interfaces"])
        {
            var bDummy = interf[ "DummyInterface"]
            if( bDummy !== undefined && bDummy === "true")
                continue
            var strVal = interf[ "InterfaceName"]
            if( strVal === "IUnknown")
                continue
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
        return 0;
    }
    Start()
    {
        // start the channel client first
        var oChan = this.m_oChanProxy;
        return oChan.Start().then((retVal)=>{
                if( ERROR(retVal))
                    return Promise.reject( retVal )
                return oChan.OpenRequestChannel()
            }).then((retVal)=>{
                if( ERROR( retVal ) )
                    return Promise.reject( retVal );
                return CInterfaceProxy.prototype.Start.bind( this)();
            }).then((retVal)=>{
                // load matches from the skel object
                if( ERROR( retVal ) )
                    return Promise.reject( retVal );
                var ret = errno.STATUS_SUCCESS
                return fetch(this.m_strObjDesc).then((response)=>{
                    return response.json().then((o)=>{
                        return this.LoadSkelMatches(o);
                    })
                })
            }).catch( (retVal)=>{
                this.m_iState = EnumIfState.stateStartFailed;
                console.log( "Error, Channel setup failed" );
                return Promise.resolve( retVal );
            })
    }

    ForwardRequest(  oReq, oCallback, oContext )
    {
        oReq.SetString( EnumPropId.propRouterPath,
            this.m_strRouterPath )
        var iMsgId = globalThis.g_iMsgIdx++;
        oReq.SetUint32(
            EnumPropId.propSeqNo, iMsgId )
        oReq.SetUint64(
            EnumPropId.propTaskId, iMsgId )

        return this.m_oChanProxy.ForwardRequest(
            oReq, oCallback, oContext );
    }

    Stop( reason )
    {
        return this.m_oChanProxy.Stop( reason ).then((e)=>{
            return super.Stop( reason )
        }).catch((e)=>{
            return super.Stop( reason )
        })
    }
}

exports.CFastRpcProxy = CFastRpcProxy
