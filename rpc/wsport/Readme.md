### Technical Spec:   

  1. The data is transferred in binary format in a `BINARY_FRAME`.
  2. The max bytes per frame is 1MB.
  3. Support to echo `PING_FRAME`, but will not actively pinging from the client.   
  4. Support to respond `CLOSE_FRAME`, but will not actively `CLOSE_FRAME` at present.   
  5. The `protocol` header option is set to `rpc-frmwrk` in the handshake so far.
  6. The option `DestURL` in the XXXdesc.json along with `EnableSSL`, `EnableWS`, etc, specifies the URI to request if   exists. If `DestURL` is not specified, `"https://example.com/chat"` is used. And you can set up an Apache `virtualhost` following the settings below. Otherwise, please make the changes accordingly.
  6. Example of `Apache` configuration :
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


  
  
