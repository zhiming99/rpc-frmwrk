const { CConfigDb2 } = require("../combase/configdb")
const { messageType } = require( "../dbusmsg/constants")
const { randomInt, ERROR, Int32Value, USER_METHOD, Pair } = require("../combase/defines")
const {EnumClsid, errno, EnumPropId, EnumCallFlags, EnumTypeId, EnumSeriProto} = require("../combase/enums")
globalThis.g_iMsgIdx = randomInt( 100000000 )

const {CoCreateInstance}=require("../combase/factory")
const {CSerialBase} = require("../combase/seribase")
globalThis.CoCreateInstance=CoCreateInstance
const {CIoManager} = require( "../ipc/iomgr")
const {CInterfaceProxy} = require( "../ipc/proxy")
const {CBuffer} = require("../combase/cbuffer")
const { DBusIfName, DBusDestination2, DBusObjPath } = require("../rpc/dmsg")
var g_oIoMgr = new CIoManager()

var oParams = globalThis.CoCreateInstance( EnumClsid.CConfigDb2)
oParams.SetString(
    EnumPropId.propObjInstName, strObjName)

var strObjDesc = "http://example.com/rpcf/asynctstdesc.json"
var strObjName = "AsyncTest"
var strAppName = "asynctst"

class CAsyncTestCli extends CInterfaceProxy
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
            console.log( `error occurs ${Int32Value(ret)}`)
            return
        }
        console.log( `server returns ${strResp}`)
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
            var ret = osb.DeserialString( ridlBuf, 0 )
            args.push( ret[0])
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
        var ret =  this.m_funcForwardRequest(
            oReq, oCallback, oContext )
        console.log( `taskid is ${oContext.m_qwTaskId}`)
        return ret
    }
}
g_oIoMgr.Start()

var oProxy = new CAsyncTestCli( g_oIoMgr,
    strObjDesc, strObjName, oParams )

oProxy.Start().then((retval)=>{
    if( ERROR( retval ))
    {
        console.log(retval)
        return
    }
    oContext = new Object()
    oProxy.LongWait( oContext, "hello, World!" )

}).catch((retval)=>{
    console.log(retval)
    oProxy.Stop()
})

