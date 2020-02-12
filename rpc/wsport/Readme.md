### Technical Information:   

  1. The data is transferred in binary format in a `BINARY_FRAME`.
  2. The max bytes per frame is 1MB, or 63KB per the streaming packet.
  3. Support to echo `PING_FRAME`, but will not actively pinging from the client.   
  4. Support to respond `CLOSE_FRAME`, but will not actively `CLOSE_FRAME` at present.   
  5. The `WebSocket handshake`'s request/response header field `Sec-WebSocket-Protocol` are both set to `chat` in the handshake so far, and ignored. As following shows
  ```
GET / HTTP/1.1
Host: localhost:4133
Sec-WebSocket-Key: BVk27NoJaxQ=
Sec-WebSocket-Protocol: chat
Sec-WebSocket-Version: 13
Origin: https://example.com
X-Forwarded-For: 192.168.3.2
X-Forwarded-Host: server.example.com
X-Forwarded-Server: server.example.com
Upgrade: WebSocket
Connection: Upgrade


  ```
  ```
HTTP/1.1 101 Switching Protocols
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Accept: /mYE/17pbLxAW2R1S+Bb4/zsPZY=
Sec-WebSocket-Protocol: chat


  ```
  6. The option `DestURL` in the XXXdesc.json along with options `EnableSSL`, `EnableWS`, etc, specifies the URI to request if exists. If `DestURL` is not specified, `"https://example.com/chat"` is used. And you can set up an Apache `virtualhost` following the settings below. Otherwise, please make the changes accordingly.
  7. Example of `Apache` configuration :
  ```
  <VirtualHost 192.168.0.1:443>
        ServerName "server.example.com"
        SSLEngine on
        SSLCertificateFile "/etc/pki/tls/certs/httpd.crt"
        SSLCertificateKeyFile "/etc/pki/tls/private/httpd.key"

        RewriteEngine on
        RewriteCond ${HTTP:Upgrade} websocket [NC]
        RewriteCond ${HTTP:Connection} upgrade [NC]
        RewriteRule .* "wss://localhost:4133/$1" [P,L]

        SSLProxyEngine on
        SSLProxyVerify none
        SSLProxyCheckPeerCN off
        SSLProxyCheckPeerExpire off

        ProxyPass /chat wss://localhost:4133/
        ProxyPassReverse /chat wss://localhost:4133/
        ProxyRequests off
</VirtualHost>
```


  
  
