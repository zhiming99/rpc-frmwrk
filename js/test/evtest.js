const { ERROR, Int32Value, USER_METHOD, Pair } = require("../combase/defines")
const {EnumClsid, errno, EnumPropId, EnumCallFlags, EnumTypeId, EnumSeriProto} = require("../combase/enums")
const {CSerialBase} = require("../combase/seribase")
const {CInterfaceProxy} = require( "../ipc/proxy")

var strObjDesc = "http://example.com/rpcf/evtestdesc.json"
var strObjName = "EventTest"
var strAppName = "evtest"

var oParams = globalThis.CoCreateInstance( EnumClsid.CConfigDb2)
oParams.SetString(
    EnumPropId.propObjInstName, strObjName)


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
        this.DebugPrint( strEvent )
    }
}

var oProxy = new CEventTestCli(
    globalThis.g_oIoMgr,
    strObjDesc, strObjName, oParams )

var count = 0
oProxy.Start().then((retval)=>{
    if( ERROR( retval ))
    {
        console.log(retval)
        return
    }
    var func = ()=>{
        oProxy.DebugPrint( count++ )
        setTimeout( func, 6000 )
    }
    var timerId = setTimeout( func , 6000 )
}).catch((retval)=>{
    console.log(retval)
    oProxy.Stop()
})


