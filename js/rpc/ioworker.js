const { randomInt } = require("../combase/defines")
const {CoCreateInstance}=require("../combase/factory")
const { CRpcRouter } = require("./bridge")

globalThis.g_iMsgIdx = randomInt( 0xffffffff )
globalThis.g_oRouter = new CRpcRouter()
globalThis.CoCreateInstance=CoCreateInstance