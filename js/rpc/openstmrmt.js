const { CConfigDb2 } = require("../combase/configdb")
const { Pair, ERROR } = require("../combase/defines")
const { constval, errno, EnumPropId, EnumProtoId, EnumStmId, EnumTypeId, EnumCallFlags, EnumIfState } = require("../combase/enums")
const { marshall, unmarshall } = require("../dbusmsg/message")
const { CIncomingPacket, COutgoingPacket, CPacketHeader } = require("./protopkt")
const { CDBusMessage, DBusIfName, DBusObjPath, DBusDestination, DBusDestination2 } = require("./dmsg")
const { IoCmd, IoMsgType, CAdminRespMessage, CIoRespMessage, CPendingRequest, AdminCmd, CIoReqMessage } = require("../combase/iomsg")
const { messageType } = require( "../dbusmsg/constants")
const { CRpcStream } = require( "./stream")

function FetchData( oContext, oMsg )
{
    return new Promise( (resolve, reject)=>{
        var oBdgeMsg = new CIoReqMessage()
        var oDataDesc = oMsg.m_oReq
        var oCallOpt = oDataDesc.GetProperty(
            EnumPropId.propCallOptions )
        oBdgeMsg.m_iCmd = IoCmd.FetchData[0]
        oBdgeMsg.m_iMsgId = globalThis.g_iMsgIdx++
        oBdgeMsg.m_iType = IoMsgType.ReqMsg
        var dwTimeoutSec =
            oCallOpt.GetProperty( EnumPropId.propTimeoutSec )
        oBdgeMsg.m_dwTimerLeftMs = dwTimeoutSec * 1000
        var dmsg = new CDBusMessage()
        dmsg.SetInterface(
            oDataDesc.GetProperty( EnumPropId.propIfName) )
        dmsg.SetMember(
            oDataDesc.GetProperty( EnumPropId.propMethodName))
        dmsg.SetObjPath(
            DBusObjPath( this.m_oParent.GetRouterName(), 
            constval.OBJNAME_TCP_BRIDGE))
        dmsg.SetSerial( oBdgeMsg.m_iMsgId )
        dmsg.SetSender( DBusDestination(
            this.m_oParent.GetRouterName()))
        dmsg.SetDestination(
            DBusDestination2( this.m_oParent.GetRouterName(), 
            constval.OBJNAME_TCP_BRIDGE))
        var oTransctx = oDataDesc.GetProperty(
            EnumPropId.propTransCtx)
        oTransctx.SetString( EnumPropId.propSessHash,
            this.m_strSess)
        oTransctx.SetUint64( EnumPropId.propTimestamp,
            this.GetPeerTimestamp() )
        dwTimeoutSec = oDataDesc.GetProperty(
            EnumPropId.propTimeoutSec )
        oContext.m_oStream.SetPingInterval( dwTimeoutSec )
        var oBuf = oDataDesc.Serialize()
        dmsg.AppendFields( [{t: "ay", v:oBuf},
            {t: "u", v:oContext.m_oStream.GetPeerStreamId()},
            {t: "u", v:0 },
            {t: "u", v:0 },
            {t: "t", v:oBdgeMsg.m_iMsgId}])
        oBdgeMsg.m_dmsg = dmsg
        var ret = this.GetStreamById(
            EnumStmId.TCP_CONN_DEFAULT_STM ).SendBuf( dmsg )
        var oPending = new CPendingRequest()
        oPending.m_oObject = this
        oPending.m_oReq = oBdgeMsg
        oPending.m_oContext = oContext
        if( ERROR( ret ) )
        {
            var oResp = new CConfigDb2()
            oResp.SetUint32(
                EnumPropId.propReturnValue, ret)
            oPending.m_oResp = oResp
            return resolve( oPending )
        }
        oPending.m_oReject = reject
        oPending.m_oResolve = resolve
        this.AddPendingReq( oBdgeMsg.m_iMsgId, oPending )
    })
}

function OpenChannel( oContext )
{
    return new Promise( (resolve, reject)=>{
        var oMsg = new CIoReqMessage()
        oMsg.m_iCmd = IoCmd.OpenStream
        oMsg.m_iMsgId = globalThis.g_iMsgIdx++
        oMsg.m_iType = IoMsgType.ReqMsg
        oMsg.m_dwTimerLeftMs =
            constval.IFSTATE_DEFAULT_IOREQ_TIMEOUT * 1000
        var oUserReq = oContext.m_oUserReq

        var oReq = new CConfigDb2()
        oReq.SetUint32( EnumPropId.propTimeoutSec,
            constval.IFSTATE_DEFAULT_IOREQ_TIMEOUT)
        oReq.SetUint32( EnumPropId.propKeepAliveSec,
            Math.floor( constval.IFSTATE_DEFAULT_IOREQ_TIMEOUT / 2 ) )
        oReq.SetString( EnumPropId.propMethodName,
            IoCmd.OpenStream[1] )
        oReq.SetString( EnumPropId.propIfName,
            DBusIfName( constval.IFNAME_TCP_BRIDGE ))
        oReq.SetUint32( EnumPropId.propPortId,
            EnumProtoId.protoControl )
        oReq.SetUint32( EnumPropId.propSeqNo,
            oMsg.m_iMsgId)
        oReq.SetUint32( EnumPropId.propStreamId,
            EnumStmId.TCP_CONN_DEFAULT_CMD )

        var oCallOpts = new CConfigDb2()
        oCallOpts.SetUint32( EnumPropId.propTimeoutSec,
            constval.IFSTATE_DEFAULT_IOREQ_TIMEOUT)

        oCallOpts.SetUint32( EnumPropId.propKeepAliveSec,
            Math.floor( constval.IFSTATE_DEFAULT_IOREQ_TIMEOUT / 2 ) )

        oCallOpts.SetUint32( EnumPropId.propCallFlags,
            EnumCallFlags.CF_ASYNC_CALL |
            EnumCallFlags.CF_WITH_REPLY |
            EnumCallFlags.CF_NON_DBUS |
            messageType.methodCall )

        oReq.SetObjPtr( EnumPropId.propCallOptions,
            oCallOpts)

        oReq.SetUint32( EnumPropId.propCmdId,
            constval.CTRLCODE_CLOSE_STREAM_PDO )

        oReq.SetBool(
            EnumPropId.propSubmitPdo, true)

        var oTransctx = new CConfigDb2()

        oTransctx.SetUint64( EnumPropId.propTimestamp,
            this.GetPeerTimestamp() )

        oReq.SetObjPtr( EnumPropId.propContext,
            oTransctx )

        var dwProto = oUserReq.m_oReq.GetProperty( 0 )
        oReq.Push({t:EnumTypeId.typeUInt32,v:this.NewStreamId()})
        oReq.Push( {t:EnumTypeId.typeUInt32,v:dwProto} )

        oMsg.m_oReq = oReq
        var oBuf = oReq.Serialize()

        var oPending = new CPendingRequest()
        oPending.m_oReject = reject
        oPending.m_oResolve = resolve
        oPending.m_oObject = this
        oPending.m_oReq = oMsg
        oPending.m_oContext = oContext

        var oStream = this.GetStreamById(
            EnumStmId.TCP_CONN_DEFAULT_CMD )
        var ret = oStream.SendBuf( oBuf )
        if( ERROR( ret ) )
        {
            var oResp = new CConfigDb2()
            oResp.SetUint32(
                EnumPropId.propReturnValue, ret )
            oPending.m_oResp = oResp
            return reject( oPending )
        }
        this.AddPendingReq( oMsg.m_iMsgId, oPending )
    })
}

function CloseChannel()
{}

/**
 * OpenStream to open a stream channel between the server and the proxy.
 * @param {CIoReqMessage}oMsg the `OpenStream` request from the main thread.
 * @returns {Promise} On success, a response package will be returned to the
 * caller, including a datadesc object with a stream handle, as can be used to
 * read/write to the stream.
 * @api public
 */
function OpenStream( oMsg )
{
    var oContext = new Object()
    oContext.m_oUserReq = oMsg
    return OpenChannel.bind( this )( oContext ).then(
        ( oPending ) =>{
            var oContext = oPending.m_oContext
            var oMsg = oContext.m_oUserReq
            oPending.m_oContext = undefined
            oContext.m_oUserReq = undefined
            var oResp = oPending.m_oResp
            var ret = oResp.GetProperty(EnumPropId.propReturnValue )
            if( ret === null )
            {
                oResp.SetUint32(EnumPropId.propReturnValue,
                    -errno.EINVAL )
                return Promise.reject( oPending )
            }

            var iPeerStmId = oResp.GetProperty( 0 )
            var iStmId = oResp.GetProperty( 2 )
            var dwProto = oResp.GetProperty( 1 )
            var oReq = oPending.m_oReq.m_oReq
            if( iStmId !== oReq.GetProperty( 0 ) ||
                dwProto != oReq.GetProperty( 1 ) )
            {
                oResp.SetUint32(EnumPropId.propReturnValue,
                    -errno.EINVAL )
                return Promise.reject( oPending )
            }

            var oStream = new CRpcStream(this)
            oStream.SetStreamId( iStmId, dwProto)
            oStream.SetPeerStreamId( iPeerStmId )
            oContext.m_oStream = oStream
            oStream.Start()

            return FetchData.bind( this )(oContext, oMsg ).then(
                (oPending)=>{
                    var oResp = oPending.m_oResp
                    var ret = oResp.GetProperty( EnumPropId.propReturnValue )
                    if( ERROR( ret ) )
                        return Promise.reject( oPending )
                    var oDataDesc = oResp.GetProperty( 0 )
                    var hStream = oDataDesc.GetProperty(
                        EnumPropId.propPeerObjId )
                    var oCtx = oPending.m_oContext
                    var oStream = oCtx.m_oStream
                    oStream.m_hStream = hStream
                    this.BindHandleStream(
                        hStream, oStream.m_iStmId ) 
                    // send message to main thread
                    var oRespMsg = new CIoRespMessage( oMsg )
                    oRespMsg.m_oResp = oResp
                    this.PostMessage( oRespMsg )
                    oStream.SendPing()
                    return Promise.resolve( oPending )
                }).catch((oPending)=>{
                    console.log( "FetchData failed")
                    return Promise.reject( oPending )
                })
        }).then((oPending)=>{
            console.log( "OpenStream completed successfully")
            return Promise.resolve( oPending.m_oContext )
        }).catch((oPending)=>{
            var oCtx = oPending.m_oContext
            if( oCtx !== undefined )
            {
                oCtx.m_oStream.Stop()
            }
            console.log( "OpenStream failed")
            return Promise.resolve( oCtx )
        })
}
exports.Bdge_OpenStream = OpenStream

function CloseStream( oMsg )
{
    var hStream = oMsg.m_oReq.GetProperty( 0 );
    if( hStream !== null )
    {
        var oStm = this.GetStreamByHandle( hStream );
        if( oStm )
        {
            oStm.Stop();
        }
    }
}

exports.Bdge_CloseStream = CloseStream