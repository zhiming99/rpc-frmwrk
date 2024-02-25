const { CConfigDb2 } = require("../combase/configdb")
const { messageType } = require( "../dbusmsg/constants")
const { randomInt, ERROR, Int32Value, USER_METHOD, Pair } = require("../combase/defines")
const {EnumClsid, errno, EnumPropId, EnumCallFlags, EnumTypeId, EnumSeriProto} = require("../combase/enums")
const {CSerialBase} = require("../combase/seribase")
const {CInterfaceProxy} = require( "../ipc/proxy")
const {Buffer} = require( "buffer")
const { DBusIfName, DBusDestination2, DBusObjPath } = require("../rpc/dmsg")

var strObjDesc = "http://example.com/rpcf/stmtestdesc.json"
var strObjName = "StreamTest"
var strAppName = "stmtest"

var oParams = globalThis.CoCreateInstance( EnumClsid.CConfigDb2)
oParams.SetString( EnumPropId.propObjInstName, strObjName)

class CStreamTestCli extends CInterfaceProxy
{
    constructor( oIoMgr,
        strObjDesc, strObjName, oParams )
    {
        super( oIoMgr, strObjDesc, strObjName )
    }
    EchoCallback(oContext, ret, i0r)
    {
        if( ERROR( ret) )
        {
            this.DebugPrint( `error occurs ${Int32Value(ret)}`)
            return
        }
        this.DebugPrint( `server returns ${i0r}`)
    }

    Echo( oContext, i0, oCallback=( oContext, oResp )=>{
        try{
            var ret = oResp.GetProperty(
                EnumPropId.propReturnValue)
            if( ERROR( ret ) )
            {
                this.EchoCallback( oContext, ret )
            }
            else
            {
                var args = [ oContext, ret ]
                var ridlBuf = Buffer.from( oResp.GetProperty( 0 ) )
                var osb = new CSerialBase( false, this )
                var retVal = osb.DeserialString( ridlBuf, 0 )
                args.push( retVal[0])
                this.EchoCallback.apply( this, args )
            }
            oContext.m_iRet = Int32Value( ret )
            if( ERROR( ret ))
                oContext.m_oReject( oContext )
            else
                oContext.m_oResolve( oContext )
        }
        catch( e )
        {
            console.log(e)
            oContext.m_iRet = -errno.EFAULT
            oContext.m_oReject( oContext )
        }
    })
    {
        var oReq = new CConfigDb2()
        var osb = new CSerialBase(true, this )
        osb.SerialString( i0 ) 
        var ridlBuf = osb.GetRidlBuf()
        oReq.Push( {t: EnumTypeId.typeByteArr, v:ridlBuf} )
        oReq.SetString( EnumPropId.propIfName,
            DBusIfName( "IStreamTest") )
        oReq.SetString( EnumPropId.propObjPath,
            DBusObjPath( strAppName, strObjName))
        oReq.SetString( EnumPropId.propSrcDBusName,
            this.m_strSender)
        oReq.SetUint32( EnumPropId.propSeriProto,
            EnumSeriProto.seriRidl )
        oReq.SetString( EnumPropId.propDestDBusName,
            DBusDestination2(strAppName, strObjName ))
        oReq.SetString( EnumPropId.propMethodName, 
            USER_METHOD("Echo"))
        var oCallOpts = new CConfigDb2()
        oCallOpts.SetUint32(
            EnumPropId.propTimeoutSec, 300)
        oCallOpts.SetUint32(
            EnumPropId.propKeepAliveSec, 3 )
        oCallOpts.SetUint32( EnumPropId.propCallFlags,
            EnumCallFlags.CF_ASYNC_CALL |
            EnumCallFlags.CF_WITH_REPLY |
            EnumCallFlags.CF_KEEP_ALIVE |
            messageType.methodCall )
        oReq.SetObjPtr(
            EnumPropId.propCallOptions, oCallOpts)
        var ret =  this.m_funcForwardRequest(
            oReq, oCallback, oContext )
        this.DebugPrint(`taskid is ${oContext.m_qwTaskId}`)
        return ret
    }
}

var oProxy = new CStreamTestCli(
    globalThis.g_oIoMgr,
    strObjDesc, strObjName, oParams )

var oStmCtx = new Object()

// stream callbacks
oProxy.OnDataReceived = ( function (hStream, oBuf ){

    const decoder = new TextDecoder();
    var strResp = decoder.decode( oBuf )
    strResp = strResp.replace('\0', '')
    this.DebugPrint( `stream@${hStream} received: ` + strResp )
    const encoder = new TextEncoder()
    var oBuf = encoder.encode( Date.now() + " proxy: Hello, world!")
    this.m_funcStreamWrite( hStream, oBuf ).then((e)=>{
        if( ERROR( e ))
            this.DebugPrint( `Error ${e}`)
    })
}).bind( oProxy )

oProxy.OnStmClosed = (function(hStream, oMsg){
    this.DebugPrint( `stream@${hStream} closed ` + oMsg)
}).bind( oProxy )

oProxy.Start().then((retval)=>{
    if( ERROR( retval ))
    {
        console.log(retval)
        return
    }
    return new Promise( (resolve, reject)=>{
        var oContext = new Object()
        oContext.m_oResolve = resolve
        oContext.m_oReject = reject
        return oProxy.Echo( oContext, "hello, World!" )
    }).then((e)=>{
        oStmCtx.m_oProxy = oProxy
        return oProxy.m_funcOpenStream( oStmCtx ).then((oCtx)=>{
            console.log( oCtx )
            return Promise.resolve( oCtx )
        }).catch((oCtx)=>{
            console.log( oCtx )
            return Promise.resolve( oCtx )
        })
    }).catch( (e )=>{
        console.log( e )
        return Promise.resolve( e )
    })

}).then((retval)=>{
    setTimeout( ()=>{
        console.log("Stopping the proxy")
        oProxy.Stop()
    }, 60000 )
})