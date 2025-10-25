#!/usr/bin/env python3
import json
import os
from urllib.parse import urlparse
from typing import List
from typing import Tuple
import errno
import re
import subprocess
import glob

class InstPkg :
    def __init__( self ):
        self.pkgName = ""
        self.startIdx = ""
        self.isServer = True
        self.instName = ""

def GetDistName() -> str:
    with open('/etc/os-release', 'r') as fp:
        for line in fp:
            if re.search( 'debian', line, flags=re.IGNORECASE ):
                return 'debian'
            elif re.search( 'ubuntu', line, flags=re.IGNORECASE ):
                return 'debian'
            elif re.search( 'fedora', line, flags=re.IGNORECASE ):
                return 'fedora'
    return ""

def GetNewerFile( strPath, pattern, bRPM )->str:
    try:
        curdir = os.getcwd()
        os.chdir( strPath )
        files = glob.glob( pattern )
        if len( files ) == 0 :
            return ""
        if len( files ) == 1 :
            return files[ 0 ]
        if bRPM :
            files = [s for s in files if not 'src.rpm' in s]
        files.sort(key=lambda x: os.path.getmtime(os.path.join('', x)))
        return files[-1]
    finally:
        os.chdir( curdir )

def IsRpcfSelfGenKey( bGmSSL:bool, strCert:str )->bool:
    if not os.path.exists( strCert ):
        return False
    if bGmSSL:
        cmdline = "gmssl certparse < " + strCert + " | grep 'Yanta'"
    else:
        cmdline = "openssl x509 -in " + strCert + " -noout -text | grep 'Yanta'"
    try:
        process = subprocess.Popen( cmdline, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE )
        output = process.stdout.readline().decode('utf-8')
        if output == '' and process.poll() is not None:
            return False
        return True
    finally:
        process.stdout.close()

def AddInstallPackages( strPath, destPkg, bServer : bool )->str :
    cmdline = "" 
    try:
        strDist = GetDistName()
        strCmd = "touch " + strDist + ";"
        strCmd += "tar rf " + destPkg + " " + strDist + ";"
        strCmd += "tar rf " + destPkg + " -C " + strPath + " "
        appmoncli = ""
        oinit = ""
        if strDist == 'debian' :
            mainPkg = GetNewerFile(
                 strPath, 'rpcf_*.deb', False )
            devPkg = GetNewerFile(
                strPath, 'rpcf-dev_*.deb', False )
            if bServer :
                appmoncli = GetNewerFile(
                    strPath, 'python3-appmoncli*.deb', False )
            else:
                oinit = GetNewerFile(
                    strPath, 'python3-oinit*.deb', False )
        elif strDist == 'fedora' :     
            devPkg = GetNewerFile(
                strPath, 'rpcf-devel-[0-9]*.rpm', True )
            mainPkg = GetNewerFile(
                strPath, 'rpcf-[0-9]*.rpm', True )
            if bServer :
                appmoncli = GetNewerFile(
                    strPath, 'python3-appmoncli*.rpm', False )
            else:
                oinit = GetNewerFile(
                    strPath, 'python3-oinit*.rpm', False )

        if len( mainPkg ) == 0 or len( devPkg ) == 0:
            return cmdline

        strCmd += mainPkg + " " + devPkg 
        if len( appmoncli ) > 0 :
            strCmd += " " + appmoncli
        if len( oinit ) > 0:
            strCmd += " " + oinit
        strCmd += ";"
        strCmd += ( "md5sum " + strPath + "/" + mainPkg + " "
            + strPath + "/" + devPkg + ";")
        if len( appmoncli ) > 0:
            strCmd += "md5sum " + strPath + "/" + appmoncli + ";"
        if len( oinit ) > 0:
            strCmd += "md5sum " + strPath + "/" + oinit + ";"
        strCmd += "rm " + strDist + ";"
        cmdline = strCmd
    except Exception as err:
        print( err )

    return cmdline

def CopyInstPkg( keyPath : str, destPath: str, bServer : bool )->int:
    ret = 0

    sf = keyPath + "/rpcf_serial.old"
    if bServer :
        files = [ 'instsvr.tar', 'instcli.tar' ]
        idxFiles = [ keyPath + "/svridx", keyPath + "/clidx" ]
    else:
        files = [ 'instcli.tar' ]
        idxFiles = [ keyPath + "/clidx" ]

    try:
        if os.access( sf, os.R_OK ):
            idxFiles.append( sf )

        indexes = []
        for i in idxFiles:
            fp = open( i, "r" )
            idx = int( fp.read( 4 ) )
            fp.close()
            indexes.append( idx )

        if len( indexes ) == len( files ):
            indexes.append( 0 )
        
        '''
        if indexes[ 0 ] < indexes[ 2 ]:
            files.remove('instsvr.tar')
        if indexes[ 1 ] < indexes[ 0 ]:
            files.remove('instcli.tar')
        if len( files ) == 0 :                
            raise Exception( "InstPkgs too old to copy" )
        '''

        for i in files:
            cmdLine = "cp " + keyPath + "/" + i + " " + destPath
            ret = rpcf_system( cmdLine )
            if ret != 0:
                raise Exception( "failed to copy " + i  )
    except Exception as err:
        if ret == 0:
            ret = -errno.ENOENT
    return ret
    
def IsValidPassword(
    password : str ) -> bool:
    if password is None:
        return False
    ret = re.match( "^[a-zA-Z0-9#?!@$%^&*-]{2,}$", password )
    if ret is None:
        return False
    return True

def rpcf_system(
    command : str ) -> int:
    ret = os.system( command )
    if os.WIFEXITED( ret ):
        return -os.WEXITSTATUS( ret )
    return -errno.EFAULT

def IsInDevTree() -> bool:
    strPath = os.path.dirname(os.path.realpath(__file__))
    stat_info = os.stat( strPath )
    uid = stat_info.st_uid
    if uid != os.geteuid() :
        return False
    strPath += '/../configure.ac'
    if os.access( strPath, os.R_OK ):
        return True
    return False

def IsNginxInstalled()->bool:
    if os.access( '/usr/sbin/nginx', os.X_OK | os.R_OK ):
        return True
    return False

def IsApacheInstalled()->bool:
    if os.access( '/usr/sbin/httpd', os.X_OK | os.R_OK ):
        return True
    return False

def IsSudoAvailable()->bool:
    ret = rpcf_system( "which sudo > /dev/null" )
    if ret != 0:
        return False
    return True

def InstallKeys( oResps : List ) -> Tuple[ int, str ]:
    srcKeys = {}
    try:
        cmdline = ''
        oResp = oResps[0]
        keyFile = oResp[ 'KeyFile']
        certFile = oResp[ 'CertFile']
        cacertFile = oResp[ 'CACertFile']
        keyName = 'rpcf_' + os.path.basename( keyFile )
        cmdline = '{sudo} install -D -m 600 ' + keyFile + ' /etc/ssl/private/' + keyName + ";"
        keyPath = '/etc/ssl/private/' + keyName
        certName = 'rpcf_' + os.path.basename( certFile )
        cmdline += '{sudo} install -D -m 644 ' + certFile + ' /etc/ssl/certs/' + certName + ";"
        certPath = '/etc/ssl/certs/' + certName
        if cacertFile is not None and len(cacertFile) > 0:
            cacertName = 'rpcf_' + os.path.basename( cacertFile )
            cmdline += '{sudo} install -D -m 644 ' + cacertFile + ' /etc/ssl/certs/' + cacertName + ";"
            cacertPath = '/etc/ssl/certs/' + cacertName
        else :
            cacertPath = ''
        for oResp in oResps:
            oResp[ 'KeyFile'] = keyPath
            oResp[ 'CertFile'] = certPath
            if len( cacertPath ) > 0:
                oResp[ 'CACertFile'] = cacertPath
        return ( 0, cmdline )
    except Exception as err:
        print( err )
        return ( -errno.EFAULT, None )

def ExtractParams( initCfg : object) ->Tuple[ dict ]:
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
            
            oResp[ 'AppName' ] = path
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

    root {RootPath};

    location /{AppName} {{
        proxy_set_header        Host $host;
        proxy_set_header        X-Real-IP $remote_addr;
        proxy_set_header        X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header        X-Forwarded-Proto $scheme;
        proxy_set_header        X-Forwarded-Port $remote_port;


        proxy_pass          https://{AppName};
        proxy_ssl_certificate     {CertFile};
        proxy_ssl_certificate_key {KeyFile};
   
        proxy_read_timeout  300s;

        # WebSocket support
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
    }}

    location /rpcf {{
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
    cmdline = ""
    ret, keyCmds = InstallKeys( oResps )
    if ret < 0:
        pass

    with open('/etc/nginx/sites-available/default') as f:
        for line in f:
            m = re.search(r'root\s+([^\s;]*html)', line)
            if m:
                strRootPath = m.group(1)
                break 

    if not strRootPath:
        strRootPath = '/usr/share/nginx/html'

    cmdline += keyCmds
    cfgFile = "/tmp/rpcf_nginx.conf"
    fp = open( cfgFile, "w" )
    for o in oResps :
        cfg = cfgText.format( IpAddress = o['IpAddress'],
            PortNum = o['PortNumber'],
            WsPortNum = o['WsPortNumber'],
            ServerName = o['ServerName'],
            AppName = o['AppName'],
            CertFile = o['CertFile'],
            KeyFile = o['KeyFile'],
            RootPath = strRootPath
            )
        fp.write( cfg )
    fp.close()
    cmdline += "{sudo} install -m 644 " + cfgFile + " /etc/nginx/sites-available/ &&"
    cmdline += "cd /etc/nginx/sites-enabled && ( {sudo} rm ./rpcf_nginx.conf;"
    cmdline += "{sudo} ln -s /etc/nginx/sites-available/rpcf_nginx.conf ) && "
    cmdline += "rm " + cfgFile + " && echo nginx setup complete ;"
    cmdline += "{sudo} systemctl restart nginx || {sudo} nginx -s reload"
    if IsSudoAvailable() :
        actCmd = cmdline.format( sudo='sudo' )
    else:
        if os.geteuid() != 0:
            actCmd = "su -c '" + cmdline.format( sudo='' ) + "'"
        else:
            actCmd = cmdline.format( sudo='' )
    ret = rpcf_system( actCmd )
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
      ProxyWebsocketIdleTimeout 300
      RequestHeader set X-Forwarded-For %${{REMOTE_ADDR}}s
      RequestHeader set X-Forwarded-Port %${{REMOTE_PORT}}s

      Alias /rpcf /var/www/rpcf
      <Directory /var/www/rpcf>
          Options Indexes FollowSymLinks
          AllowOverride None
          Require all granted
      </Directory>

</VirtualHost>

'''
    oResps = ExtractParams( initCfg )
    cmdline = ""
    ret, keyCmds = InstallKeys( oResps )
    if ret < 0:
        return ret
    cmdline += keyCmds
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
    cmdline += "{sudo} install -m 644 " + cfgFile + " /etc/httpd/conf.d && rm " + cfgFile + ";"
    cmdline += "{sudo} systemctl restart httpd"
    if IsSudoAvailable() :
        actCmd = cmdline.format( sudo='sudo' )
    else:
        if os.geteuid() != 0:
            actCmd = "su -c '" + cmdline.format( sudo='' ) + "'"
        else:
            actCmd = cmdline.format( sudo='' )
    ret = rpcf_system( actCmd )
    return ret

def ConfigWebServer2( initCfg : object )->int:
    ret = 0
    try:
        if initCfg[ 'IsServer' ] != 'true':
            return ret

        if "InstToSvr" in initCfg and \
            initCfg[ 'InstToSvr' ] == 'false':
            return ret

        oMisc = initCfg[ 'Security' ]['misc']
        if not 'ConfigWebServer' in oMisc:
            return ret
        if oMisc['ConfigWebServer'] != 'true':
            return ret

        bEnableWS = False
        oConns = initCfg['Connections']
        for oConn in oConns:
            if not ( 'EnableWS' in oConn and
                'EnableSSL' in oConn ):
                continue
            if not ( oConn['EnableWS'] == 'true' and
                oConn['EnableSSL'] == 'true' ):
                continue
            bEnableWS = True
            break

        if not bEnableWS:
            return ret

        if IsNginxInstalled():
            ret = Config_Nginx( initCfg )
        elif IsApacheInstalled():
            ret = Config_Apache( initCfg )
            if ret == 0:
                print( "Apache httpd is configured. And " +
                    "make sure mod_ssl is installed" )
    except Exception as err:
        print( err )
        if ret == 0:
            ret = -errno.EFAULT
    return ret

def ConfigWebServer( initFile : str ) -> int :
    ret = 0
    try:
        fp = open( initFile, 'r' )
        cfgVal = json.load( fp )
        fp.close()
        ret = ConfigWebServer2( cfgVal )
    except Exception as err:
        print( err )
        if ret == 0:
            ret = -errno.EFAULT
    return ret


if __name__ == "__main__":
    import sys
    if len(sys.argv) < 2:
        print("Usage: python3 updwscfg.py <initcfg path>")
        sys.exit(1)

    try:
        ConfigWebServer( sys.argv[1] )
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)
