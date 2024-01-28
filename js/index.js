// index.js
const dmsg = require( "./dbusmsg/message.js" )
const buffer = require( "./combase/cbuffer.js" )
const { randomInt } = require("./combase/defines")
const { CAdminReqMessage, AdminCmd } = require("./rpc/iomsg.js")
const { EnumPropId } = require("./combase/enums.js")

globalThis.g_iMsgIdx = randomInt( 100000000 )
require('./rpc/iomsg')

var g_count = 0
const worker = new Worker(new URL('./rpc/ioworker.js', import.meta.url));
worker.onmessage=(e)=>{
    console.log(e)
    if( g_count > 0 )
        return

    g_count += 1
    var oMsg = new CAdminReqMessage()
    oMsg.m_iMsgId = g_iMsgIdx++
    oMsg.m_iCmd = AdminCmd.OpenRemotePort[0]
    oMsg.m_oReq.SetString(
        EnumPropId.propDestUrl,
        "ws://example.com/chat" )
    worker.postMessage(oMsg)
}
worker.onerror=(e)=>{
    console.log(e)
}
worker.onmessageerror=(e)=>{
    console.log(e)
}

//alert("hello,webpack!")
var oMsg = new CAdminReqMessage()
oMsg.m_iMsgId = globalThis.g_iMsgIdx++
oMsg.m_iCmd = AdminCmd.SetConfig[0]
oMsg.m_oReq.SetString(
    EnumPropId.propRouterName,
    "rpcrouter" )
worker.postMessage(oMsg)




