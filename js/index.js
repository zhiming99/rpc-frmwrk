var strObjDesc = "http://example.com/rpcf/asynctstdesc.json"
var strObjName = "AsyncTest"
var strAppName = ""

// index.js
const { randomInt } = require("./combase/defines")
const {EnumClsid, EnumPropId} = require("./combase/enums")
globalThis.g_iMsgIdx = randomInt( 100000000 )

const {CoCreateInstance}=require("./combase/factory")
globalThis.CoCreateInstance=CoCreateInstance
const {CIoManager} = require( "./ipc/iomgr")
const {CInterfaceProxy} = require( "./ipc/proxy")
var g_oIoMgr = new CIoManager()

var oParams = globalThis.CoCreateInstance( EnumClsid.CConfigDb2)
oParams.SetString(
    EnumPropId.propObjInstName, strObjName)

var oProxy = new CInterfaceProxy( g_oIoMgr,
    strObjDesc, strObjName, oParams )

g_oIoMgr.Start()
oProxy.Start()




