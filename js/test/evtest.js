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


var strObjDesc = "http://example.com/rpcf/evtestdesc.json"
var strObjName = "EventTest"
var strAppName = "evtest"

class CEventTestCli extends CInterfaceProxy
{
    constructor( oIoMgr,
        strObjDesc, strObjName, oParams )
    {
        super( oIoMgr, strObjDesc, strObjName )
        var func =( ridlBuf )=>{
            var offset = 0
            var arrParams = []
            var oRidlBuf = Buffer.from( ridlBuf )
            var osb = new CSerialBase()
            var ret = osb.DeserialString( oRidlBuf, offset )
            arrParams.push( ret[0 ])
            offset += ret[1]
            this.OnHelloWorld.apply( this, arrParams )
        }
        this.m_mapEvtHandlers.set(
            "IEventTest::OnHelloWorld", func)
    }

    OnHelloWorld( strEvent )
    {
        console.log( Date.now() + ":" + strEvent )
    }
}

var oProxy = new CEventTestCli( g_oIoMgr,
    strObjDesc, strObjName, oParams )

var count = 0
oProxy.Start().then((retval)=>{
    if( ERROR( retval ))
    {
        console.log(retval)
        return
    }
    var func = ()=>{
        console.log( count++ )
        setTimeout( func, 6000 )
    }
    var timerId = setTimeout( func , 6000 )
}).catch((retval)=>{
    console.log(retval)
    oProxy.Stop()
})
