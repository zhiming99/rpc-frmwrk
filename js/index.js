// index.js
const dmsg = require( "./dbusmsg/message.js" )
const buffer = require( "./combase/cbuffer.js" )
const { Connect, CloseWs } = require('./rpc/startws.js')
var sock = Connect("wss://example.com/chat")
alert("hello,webpack!")
CloseWs( sock )



