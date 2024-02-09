var strObjDesc = "http://example.com/rpcf/asynctstdesc.json"
var strObjName = "AsyncTest"
var strAppName = "asynctst"

const { CConfigDb2 } = require("./combase/configdb")
// index.js
const { messageType } = require( "./dbusmsg/constants")
const { randomInt, ERROR, USER_METHOD, Pair } = require("./combase/defines")
const {EnumClsid, EnumPropId, EnumCallFlags, EnumTypeId, EnumSeriProto} = require("./combase/enums")
globalThis.g_iMsgIdx = randomInt( 100000000 )

const {CoCreateInstance}=require("./combase/factory")
globalThis.CoCreateInstance=CoCreateInstance
const {CIoManager} = require( "./ipc/iomgr")
const {CInterfaceProxy} = require( "./ipc/proxy")
const {CBuffer} = require("./combase/cbuffer")
const { DBusIfName, DBusDestination2, DBusObjPath } = require("./rpc/dmsg")
var g_oIoMgr = new CIoManager()

var oParams = globalThis.CoCreateInstance( EnumClsid.CConfigDb2)
oParams.SetString(
    EnumPropId.propObjInstName, strObjName)

class CAsyncTestCli extends CInterfaceProxy
{
    constructor( oIoMgr,
        strObjDesc, strObjName, oParams )
    {
        super( oIoMgr, strObjDesc, strObjName )
    }
    LongWaitCallback(oResp)
    {
        var ret = oResp.GetProperty(
            EnumPropId.propReturnValue)
        if( ERROR( ret) )
        {
            console.log( "error occurs")
            return
        }
        var ridlBuf = oResp.GetProperty( 0 )
        var oBuf = Buffer.from( ridlBuf )
        var dwSize = oBuf.readUint32BE( 0 )
        if( dwSize > oBuf.length - 4 )
            console.log( "Error invalid string")
        var strResp = CBuffer.BufferToStrNoNull( oBuf, 0 )
        console.log( `server returns ${strResp}`)
    }

    LongWait( strText, oCallback=this.LongWaitCallback )
    {
        var oReq = new CConfigDb2()
        var strBuf = CBuffer.StrToBufferNoNull( strText )
        var oSizeBuf = Buffer.alloc(4)
        oSizeBuf.writeUint32BE( strBuf.length )
        var ridlBuf = Buffer.concat([oSizeBuf,strBuf])
        oReq.Push( new Pair( {t: EnumTypeId.typeByteArr, v:ridlBuf} ) )
        oReq.SetString( EnumPropId.propIfName,
            DBusIfName( "IAsyncTest") )
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
            EnumPropId.propTimeoutsec, 97)
        oCallOpts.SetUint32(
            EnumPropId.propKeepAliveSec, 48 )
        oCallOpts.SetUint32( EnumPropId.propCallFlags,
            EnumCallFlags.CF_ASYNC_CALL |
            EnumCallFlags.CF_WITH_REPLY |
            messageType.methodCall )
        oReq.SetObjPtr(
            EnumPropId.propCallOptions, oCallOpts)
        return this.m_funcForwardRequest( oReq, oCallback )
    }
}
var oProxy = new CAsyncTestCli( g_oIoMgr,
    strObjDesc, strObjName, oParams )

g_oIoMgr.Start()
oProxy.Start().then((retval)=>{
    if( ERROR( retval ))
    {
        console.log(retval)
        return
    }
    oProxy.LongWait( "hello, World!" )

}).catch((retval)=>{
    console.log(retval)
    oProxy.Stop()
})




