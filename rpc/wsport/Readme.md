### Technical Information:   

  1. The data is transferred in binary format in a `BINARY_FRAME`.
  2. The max bytes per frame is 1MB, or 63KB per the streaming packet.
  3. Support to echo `PING_FRAME`, but will not actively pinging from the client.   
  4. Support to respond `CLOSE_FRAME`, but will not actively `CLOSE_FRAME` at present.   
  5. The `WebSocket handshake`'s request/response header field `Sec-WebSocket-Protocol` are both set to `chat` in the handshake so far, and ignored. As following shows
  ```
[Apache request]
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
[rpc-frmwrk response]  
HTTP/1.1 101 Switching Protocols
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Accept: /mYE/17pbLxAW2R1S+Bb4/zsPZY=
Sec-WebSocket-Protocol: chat


  ```
  6. The option `DestURL` in the XXXdesc.json along with options `EnableSSL`, `EnableWS`, etc, specifies the URI to request if exists. If `DestURL` is not specified, `"https://example.com/chat"` is used. And you can set up an Apache `virtualhost` following the settings below. Otherwise, please make the changes accordingly.
  7. Example configuration for `apache`:
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

  8. Example configuration for `nginx`

/etc/nginx/conf.d/ssl.conf
```
server {
    listen 443 http2 ssl;
    listen [::]:443 http2 ssl;

    server_name example.com;

    ssl_certificate /etc/ssl/certs/rpcf.crt;
    ssl_certificate_key /etc/ssl/private/rpcf.key;
    #ssl_dhparam /etc/ssl/certs/dhparam.pem;
    ssl_protocols       TLSv1.2 TLSv1.3;
    ssl_ciphers         HIGH:!aNULL:!MD5;


    root /usr/share/nginx/html;

    location /chat {
        proxy_set_header        Host $host;
        proxy_set_header        X-Real-IP $remote_addr;
        proxy_set_header        X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header        X-Forwarded-Proto $scheme;


      # Fix the “It appears that your reverse proxy set up is broken" error.
        proxy_pass          https://ws_backend;
        proxy_ssl_certificate     /etc/ssl/certs/rpcf.crt;
        proxy_ssl_certificate_key /etc/ssl/private/rpcf.key;
   
        proxy_read_timeout  300s;

        # WebSocket support
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
    }

    error_page 404 /404.html;
    location = /404.html {
    }


    error_page 500 502 503 504 /50x.html;
    location = /50x.html {
    }
 
}

upstream ws_backend {
    # enable sticky session based on IP
    ip_hash;

    server 127.0.0.1:4132;
}
```
/etc/nginx/default.d/ssl-redirect.conf
```
return 301 https://$host$request_uri/;
```

  9. Setup Nginx or Apache with `rpcfg.py`   
  `rpcfg.py` can automatically generate and deploy the config file for an Nginx or Apache server when `websocket` transfer enabled. Check the last checkbox `Config Web Server` on the ![security page](https://github.com/zhiming99/rpc-frmwrk/blob/master/pics/rpcfg2.png), and press button `OK` or `Export`, all will be set. For Apache, the SSL module `mod_ssl` is seperate from the `httpd` package, and make sure it is also installed.
