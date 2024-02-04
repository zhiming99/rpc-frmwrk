// index.js
const { randomInt } = require("./combase/defines")
const {EnumClsid, EnumPropId} = require("./combase/enums")
globalThis.g_iMsgIdx = randomInt( 100000000 )

const {CoCreateInstance}=require("./combase/factory")
globalThis.CoCreateInstance=CoCreateInstance
const {CIoManager} = require( "./ipc/iomgr")
const {CInterfaceProxy} = require( "./ipc/proxy")
var oIoMgr = new CIoManager()

var oParams = globalThis.CoCreateInstance( EnumClsid.CConfigDb2)
oParams.SetString(
    EnumPropId.propObjInstName, "CEchoServer")

var val = oParams.GetProperty( EnumPropId.propObjInstName )
var oProxy = new CInterfaceProxy( oIoMgr,
    "http://example.com/rpcf/echodesc.json",
    "CEchoServer", oParams )

oIoMgr.Start()
oProxy.Start()




