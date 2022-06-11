keydir=$(pwd)/tools/testcfgs
if [ ! -e /etc/ssl/certs/rpcf.key ]; then
    ln -s $keydir/server.key /etc/ssl/certs/rpcf.key
fi
if [ ! -e /etc/ssl/certs/rpcf.crt ]; then
    ln -s $keydir/server.crt /etc/ssl/certs/rpcf.crt
fi

echo "==================================================================================="
echo "==== /etc/nginx/conf.d/ssl.conf ==================================================="
echo "==================================================================================="

if [ ! -e /etc/nginx/conf.d ]; then
    mkdir -p /etc/nginx/conf.d
fi

echo '127.0.0.1	example.com' >> /etc/hosts

tee /etc/nginx/conf.d/ssl.conf << EOF
server {
    listen 443 http2 ssl;
    listen [::]:443 http2 ssl;

    server_name example.com;

    ssl_certificate /etc/ssl/certs/rpcf.crt;
    ssl_certificate_key /etc/ssl/private/rpcf.key;
    ssl_dhparam /etc/ssl/certs/dhparam.pem;
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
   
        proxy_read_timeout  90;

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
EOF

echo "==================================================================================="
echo "==== /etc/nginx/default.d/ssl-redirect.conf ======================================="
echo "==================================================================================="

if [ ! -e /etc/nginx/default.d ]; then
    mkdir -p /etc/nginx/default.d
fi

tee /etc/nginx/default.d/ssl-redirect.conf <<EOF
return 301 https://$host$request_uri/;
EOF

kill -3 `pidof nginx`
nginx
