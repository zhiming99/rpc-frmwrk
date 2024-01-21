
exports.Connect = function Connect( strWsUrl )
{
    socket = globalThis.g_oSock = new WebSocket( strWsUrl );
    socket.binaryType = 'arraybuffer'
    socket.onopen = function() {
        console.log('connected!');
    };
    socket.onmessage = function(event) {
        const message = event.data;
        console.log(`msg arrives：${message}`);
    };

    socket.onclose = function() {
        console.log('connection closed');
    };
    return socket
}

exports.SendBuf = function sendMessageToServer( socket, message)
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