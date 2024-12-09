const { CConfigDb2 } = require("../combase/configdb")
const { messageType } = require( "../dbusmsg/constants")
const { randomInt, ERROR, Int32Value, USER_METHOD, Pair } = require("../combase/defines")
const {EnumClsid, errno, EnumPropId, EnumCallFlags, EnumTypeId, EnumSeriProto} = require("../combase/enums")
const {CoCreateInstance}=require("../combase/factory")
const {CSerialBase} = require("../combase/seribase")
const {CIoManager} = require( "../ipc/iomgr")
const {CInterfaceProxy} = require( "../ipc/proxy")
const {CBuffer} = require("../combase/cbuffer")
const { DBusIfName, DBusDestination2, DBusObjPath } = require("../rpc/dmsg")

var strObjDesc = "http://example.com/rpcf/katestdesc.json"
var strObjName = "KeepAlive"
var strAppName = "katest"

var oParams = globalThis.CoCreateInstance( EnumClsid.CConfigDb2)
oParams.SetString(
    EnumPropId.propObjInstName, strObjName)

class CKeepAliveCli extends CInterfaceProxy
{
    constructor( oIoMgr,
        strObjDesc, strObjName, oParams )
    {
        super( oIoMgr, strObjDesc, strObjName )
    }
    LongWaitCallback(oContext, ret, strResp)
    {
        if( ERROR( ret) )
        {
            this.DebugPrint( `error occurs ${Int32Value(ret)}`)
            return
        }
        this.DebugPrint( `server returns ${strResp}`)
    }

    LongWait( oContext, strText, oCallback=( oContext, oResp )=>{
        try{
            var ret = oResp.GetProperty(
                EnumPropId.propReturnValue)
            if( ERROR( ret ) )
            {
                this.LongWaitCallback( oContext, ret )
                return
            }
            var args = [ oContext, ret ]
            var ridlBuf = Buffer.from( oResp.GetProperty( 0 ) )
            var osb = new CSerialBase( false, this )
            var strResp = osb.DeserialString( ridlBuf, 0 )
            args.push( strResp[0])
            this.LongWaitCallback.apply( this, args )
        }
        catch( e )
        {
            console.log(e)
        }
    })
    {
        var oReq = new CConfigDb2()
        var osb = new CSerialBase(true, this )
        osb.SerialString( strText ) 
        var ridlBuf = osb.GetRidlBuf()
        oReq.Push( new Pair( {t: EnumTypeId.typeByteArr, v:ridlBuf} ) )
        oReq.SetString( EnumPropId.propIfName,
            DBusIfName( "IKeepAlive") )
        oReq.SetString( EnumPropId.propObjPath,
            DBusObjPath( strAppName, strObjName))
        oReq.SetString( EnumPropId.propSrcDBusName,
            this.m_strSender)
        oReq.SetUint32( EnumPropId.propSeriProto,
            EnumSeriProto.seriRidl )
        oReq.SetString( EnumPropId.propDestDBusName,
            DBusDestination2(strAppName, strObjName ))
        oReq.SetString( EnumPropId.propMethodName, 
            USER_METHOD("LongWait"))
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

var oProxy = new CKeepAliveCli(
    globalThis.g_oIoMgr,
    strObjDesc, strObjName, oParams )

oProxy.Start().then((retval)=>{
    if( ERROR( retval ))
    {
        console.log(retval)
        return
    }
    var oContext = new Object()
    oProxy.LongWait( oContext, "hello, World!" )

}).catch((retval)=>{
    console.log(retval)
    oProxy.Stop()
})


