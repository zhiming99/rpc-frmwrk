var strObjDesc = "http://example.com/rpcf/asynctstdesc.json"
var strObjName = "AsyncTest"
var strAppName = "asynctst"

const { CConfigDb2 } = require("./combase/configdb")
// index.js
const { messageType } = require( "./dbusmsg/constants")
const { randomInt, ERROR, USER_METHOD } = require("./combase/defines")
const {EnumClsid, EnumPropId, EnumCallFlags} = require("./combase/enums")
globalThis.g_iMsgIdx = randomInt( 100000000 )

const {CoCreateInstance}=require("./combase/factory")
globalThis.CoCreateInstance=CoCreateInstance
const {CIoManager} = require( "./ipc/iomgr")
const {CInterfaceProxy} = require( "./ipc/proxy")
const { DBusIfName, DBusDestination2 } = require("./rpc/dmsg")
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
        var strResp = oResp.GetProperty( 0 )
        console.log( `client returns ${strResp}`)
    }

    LongWait( strText, oCallback=this.LongWaitCallback )
    {
        var oReq = new CConfigDb2()
        oReq.Push( strText )
        oReq.SetString( EnumPropId.propIfName,
            DBusIfName( "IAsyncTest") )
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
            EnumPropId.propCallFlags,
            EnumCallFlags.CF_ASYNC_CALL |
            EnumCallFlags.CF_WITH_REPLY |
            messageType.methodCall )
        oReq.SetObjPtr(
            EnumPropId.propCallOptions, oCallOpts)
        return this.m_funcForwardRequest( oReq )
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
    oProxy.LongWait( "hello, World" )

}).catch((retval)=>{
    console.log(retval)
    oProxy.Stop()
})




