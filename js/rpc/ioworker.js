const { randomInt } = require("../combase/defines")
const { CRpcRouter } = require("./bridge")

globalThis.g_iMsgIdx = randomInt( 100000000 )
globalThis.g_oRouter = new CRpcRouter()