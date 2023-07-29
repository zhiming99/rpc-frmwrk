#!/usr/bin/env python3
import json
import os
import sys 
from shutil import move
from copy import deepcopy
from urllib.parse import urlparse
from typing import Dict
from typing import Tuple
import errno

def IsNginxInstalled()->bool:
    if os.access( '/usr/sbin/nginx', os.X_OK | os.R_OK ):
        return True
    return False

def IsApacheInstalled()->bool:
    if os.access( '/usr/sbin/httpd', os.X_OK | os.R_OK ):
        return True
    return False

def InstallKeys( oResps : [] ) -> int:
    srcKeys = {}
    ret = 0
    try:
        oResp = oResps[0]
        keyFile = oResp[ 'KeyFile']
        certFile = oResp[ 'CertFile']
        cacertFile = oResp[ 'CACertFile']
        cmdline = ''
        keyName = 'rpcf_' + os.path.basename( keyFile )
        cmdline = 'sudo install -D -m 600 ' + keyFile + ' /etc/ssl/private/' + keyName + ";"
        keyPath = '/etc/ssl/private/' + keyName
        certName = 'rpcf_' + os.path.basename( certFile )
        cmdline += 'sudo install -D -m 644 ' + certFile + ' /etc/ssl/certs/' + certName + ";"
        certPath = '/etc/ssl/certs/' + certName
        cacertName = 'rpcf_' + os.path.basename( cacertFile )
        cmdline += 'sudo install -D -m 644 ' + cacertFile + ' /etc/ssl/certs/' + cacertName + ";"
        cacertPath = '/etc/ssl/certs/' + cacertName
        for oResp in oResps:
            oResp[ 'KeyFile'] = keyPath
            oResp[ 'CertFile'] = certPath
            oResp[ 'CACertFile'] = cacertPath
        ret = -os.system(cmdline)
    except Exception as err:
        print( err )
        if ret == 0:
            ret = errno.EFAULT

    return ret

def ExtractParams( initCfg : object) ->( dict, ):
    try:
        if not 'Connections' in initCfg:
            ret = -errno.ENOENT
            raise Exception( "Error unabled to find connection settings" )
        if len( initCfg[ 'Connections'] ) == 0:
            ret = -errno.ENOENT
            raise Exception( "Error unabled to find connection settings" )

        oResps = []
        for oConn in initCfg[ 'Connections' ]:
            oResp = {}
            if ( oConn[ 'EnableSSL' ] != 'true' or 
                oConn[ 'EnableWS' ] != 'true' ):
                continue
            oResp[ 'IpAddress' ] = oConn[ 'IpAddress' ]
            oResp[ 'PortNumber' ] = oConn[ 'PortNumber' ]

            urlComp = urlparse( oConn[ 'DestURL' ], scheme='https')
            if urlComp.port is None:
                oResp[ 'WsPortNumber' ] = 443
            else:
                oResp[ 'WsPortNumber' ] = urlComp.port

            oResp[ 'ServerName' ] = urlComp.hostname

            path = os.path.basename( urlComp.path )
            if not path.isidentifier() :
                raise Exception( "Error the basename of the path must be a valid identifier")
            
            oResp[ 'AppName' ] = urlComp.path[1:]
            oResps.append( oResp )

        if ( not 'Security' in initCfg or
            not 'SSLCred' in initCfg['Security'] ):
            raise Exception( "Error unable to find SSL key and certs")

        sslCred = initCfg['Security']['SSLCred']
        for oResp in oResps:
            oResp['KeyFile'] = sslCred['KeyFile']
            oResp['CertFile'] = sslCred['CertFile']
            oResp['CACertFile'] = sslCred['CACertFile']

        return ( oResps )

    except Exception as err:
        print( err )
        return ()

def Config_Nginx( initCfg : object ) -> int:
    cfgText = '''server {{
    listen {WsPortNum} http2 ssl;
    listen [::]:{WsPortNum} http2 ssl;

    server_name {ServerName};

    ssl_certificate {CertFile};
    ssl_certificate_key {KeyFile};
    #ssl_dhparam /etc/ssl/certs/dhparam.pem;
    ssl_protocols       TLSv1.2 TLSv1.3;
    ssl_ciphers         HIGH:!aNULL:!MD5;

    root /usr/share/nginx/html;

    location /{AppName} {{
        proxy_set_header        Host \$host;
        proxy_set_header        X-Real-IP \$remote_addr;
        proxy_set_header        X-Forwarded-For \$proxy_add_x_forwarded_for;
        proxy_set_header        X-Forwarded-Proto \$scheme;


      # Fix the “It appears that your reverse proxy set up is broken" error.
        proxy_pass          https://{AppName};
        proxy_ssl_certificate     {CertFile};
        proxy_ssl_certificate_key {KeyFile};
   
        proxy_read_timeout  300s;

        # WebSocket support
        proxy_http_version 1.1;
        proxy_set_header Upgrade \$http_upgrade;
        proxy_set_header Connection "upgrade";
    }}

    error_page 404 /404.html;
    location = /404.html {{
    }}


    error_page 500 502 503 504 /50x.html;
    location = /50x.html {{
    }}
 
}}
upstream {AppName} {{
    # enable sticky session based on IP
    ip_hash;

    server {IpAddress}:{PortNum};
}}

'''
    oResps = ExtractParams( initCfg )
    ret = InstallKeys( oResps )
    if ret < 0:
        pass
    cfgFile = "/tmp/rpcf_nginx.conf"
    fp = open( cfgFile, "w" )
    for o in oResps :
        cfg = cfgText.format( IpAddress = o['IpAddress'],
            PortNum = o['PortNumber'],
            WsPortNum = o['WsPortNumber'],
            ServerName = o['ServerName'],
            AppName = o['AppName'],
            CertFile = o['CertFile'],
            KeyFile = o['KeyFile']
            )
        fp.write( cfg )
    fp.close()
    cmdline = "sudo install -m 644 " + cfgFile + " /etc/nginx/sites-available/ &&"
    cmdline += "cd /etc/nginx/sites-enabled && sudo ln -s /etc/nginx/sites-available/rpcf_nginx.conf &&"
    cmdline += "rm " + cfgFile
    ret = -os.system( cmdline )
    if ret < 0:
        return ret
    cmdline = "sudo systemctl restart nginx"
    ret = -os.system( cmdline )
    return ret

def Config_Apache( initCfg : object )->int:
    cfgText = '''<VirtualHost *:{WsPortNum}>
      ServerName "{ServerName}"
      SSLEngine on
      SSLCertificateFile "{CertFile}"
      SSLCertificateKeyFile "{KeyFile}"

      RewriteEngine on
      RewriteCond ${{HTTP:Upgrade}} websocket [NC]
      RewriteCond ${{HTTP:Connection}} upgrade [NC]
      RewriteRule .* "wss://{IpAddress}:{PortNum}/$1" [P,L]

      SSLProxyEngine on
      SSLProxyVerify none
      SSLProxyCheckPeerCN off
      SSLProxyCheckPeerExpire off

      ProxyPass /{AppName} wss://{IpAddress}:{PortNum}/
      ProxyPassReverse /{AppName} wss://{IpAddress}:{PortNum}/
      ProxyRequests off
</VirtualHost>

'''
    oResps = ExtractParams( initCfg )
    ret = InstallKeys( oResps )
    if ret < 0:
        return ret
    cfgFile = "/tmp/rpcf_apache.conf"
    fp = open( cfgFile, "w" )
    for o in oResps :
        cfg = cfgText.format( IpAddress = o['IpAddress'],
            PortNum = o['PortNumber'],
            WsPortNum = o['WsPortNumber'],
            ServerName = o['ServerName'],
            AppName = o['AppName'],
            CertFile = o['CertFile'],
            KeyFile = o['KeyFile']
            )
        fp.write( cfg )
    fp.close()
    cmdline = "sudo install -m 644 " + cfgFile + " /etc/httpd/conf.d && rm " + cfgFile
    ret = -os.system( cmdline )
    if ret < 0:
        return ret
    cmdline = "sudo systemctl restart httpd"
    ret = -os.system( cmdline )
    return ret

def ConfigWebServer( initCfg : object )->int:
    ret = 0
    try:
        oMisc = initCfg[ 'Security' ]['misc']
        if not 'ConfigWebServer' in oMisc:
            return ret
        if oMisc['ConfigWebServer'] != 'true':
            return ret
        if IsNginxInstalled():
            ret = Config_Nginx( initCfg )
        elif IsApacheInstalled():
            ret = Config_Apache( initCfg )

    except Exception as err:
        print( err )
        if ret == 0:
            ret = -errno.EFAULT
    return ret

def test():
    try:
        drvfile = "/home/zhiming/wsssl.json"
        fp = open( drvfile, "r" )                                                                                                                   
        initCfg = json.load(fp)
        fp.close()                                   
        Config_Nginx( initCfg )

    except Exception as err:
        print( err.with_traceback( None ) )

if __name__ == '__main__' :
    test()

