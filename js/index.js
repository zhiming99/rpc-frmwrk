const { randomInt, ERROR, Int32Value, USER_METHOD, Pair } = require("./combase/defines")
globalThis.g_iMsgIdx = randomInt( 100000000 )

const {CoCreateInstance}=require("./combase/factory")
const {CIoManager} = require( "./ipc/iomgr.js")
globalThis.CoCreateInstance=CoCreateInstance

function setLogger() {
    var old = console.log;
    var logger = document.getElementById('log');
    if( !logger )
        return
    console.log = function (message) {
        if (typeof message == 'object') {
            logger.innerHTML += (JSON && JSON.stringify ? JSON.stringify(message) : message) + '<br />';
        } else {
            logger.innerHTML += message + '<br />';
        }
    }
}
setLogger()
globalThis.g_oIoMgr = new CIoManager()
globalThis.g_oIoMgr.Start()
require( "./test/actcancel.js" )
require( "./test/evtest.js")
// require( "./test/asynctst.js")
require( "./test/katest.js")
