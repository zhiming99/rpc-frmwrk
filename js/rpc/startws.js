const {CIncomingPacket, COutgoingPacket} = require( "./protopkt")

exports.Connect = function Connect( strWsUrl )
{
    socket = globalThis.g_oSock = new WebSocket( strWsUrl );
    socket.binaryType = 'arraybuffer'
    socket.onopen = function() {
        console.log('connected!');
    };
    socket.onmessage = function(event) {
        oBuf = Buffer.from( event.data )
        oInPkt = new CIncomingPacket()
        oInPkt.Deserialize( oBuf, 0 )
        console.log(`msg arrives：${message}`);
    };

    socket.onclose = function() {
        console.log('connection closed');
    };
    return socket
}

exports.sendMessageToServer = function sendMessageToServer( socket, message)
{
    if (socket.readyState === WebSocket.OPEN) {
        socket.send(message);
    } else {
        console.error('Connection is not ready, and unable to send');
    }
}

exports.CloseWs = function closeConnectionWithServer( socket ) {
    socket.close();
}
