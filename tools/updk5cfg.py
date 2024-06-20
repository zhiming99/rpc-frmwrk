#!/usr/bin/env python3
import json
import os
from typing import Dict
from typing import Tuple
import errno
import socket
import re
from krbparse import *
from updwscfg import IsSudoAvailable, rpcf_system
import traceback

def GetLocalIp( strIpAddr : str )->str:
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.connect(( strIpAddr, 88))
    ip = s.getsockname()[0]
    s.close()
    return ip
    
def CheckPrincipal(
    strPrinc : str,
    strRealm : str ) -> bool:
    ret = True
    try:
        strPrinc = strPrinc.strip()
        strRealm = strRealm.strip()
        if len( strPrinc ) > 256 or len( strRealm ) > 256:
            raise Exception( "Error principal too long" )
        components = strPrinc.strip().split( "@" )
        if len( components ) > 2:
            raise Exception( "Error principal should have only one '@'" )

    except Exception as err:
        print( err )
        if ret == True:
            ret = False

    return ret

def ValidateName( hostname : str ) -> bool:
    if len(hostname) > 255:
        return False
    if hostname[-1] == ".":
        hostname = hostname[:-1] # strip exactly one dot from the right, if present
    allowed = re.compile("(?!-)[A-Z\d-]{1,63}(?<!-)$", re.IGNORECASE)
    return all(allowed.match(x) for x in hostname.split(".")) 

def IsNameRegistred(
    strIpAddr : str,
    strName : str ) ->bool:
    try:
        if not ValidateName( strName ):
            return False
        addrInfo = socket.getaddrinfo(
            strName, 0,
            proto=socket.IPPROTO_TCP,
            type=socket.SOCK_STREAM )
        for elem in addrInfo:
            if elem[4][0] == strIpAddr:
                return True
    except Exception as err:
        print( err )
    return False

def AddEntryToHosts(
    strIpAddr : str,
    strNames : str ) -> str:

    cmdline = "{sudo} sh -c 'echo \"" + strIpAddr + '\t' + \
        strNames + "\" >> /etc/hosts'"
    return cmdline

def AddPrincipal(
    strPrinc : str,
    strPass : str,
    bLocal = True,
    strAdmin = None ) -> str:
    if bLocal:
        kadmin = "kadmin.local"
        sudostr = '{sudo} '
    else:
        if strAdmin is None:
            kadmin = "kadmin"
        else:
            kadmin = "kadmin -k -p " + strAdmin
        sudostr = ''
    cmdline = sudostr + kadmin + " -q 'addprinc "
    if len( strPass ) != 0:
        cmdline += "-pw \"" + strPass + "\" " + strPrinc + "'"
    else:
        cmdline += "-randkey " + strPrinc + "'"
    return cmdline

def DeletePrincipal(
    strPrinc : str,
    bLocal = True,
    strAdmin = None ) -> str:
    if bLocal:
        kadmin = "kadmin.local"
        sudostr = '{sudo} '
    else:
        if strAdmin is None:
            kadmin = "kadmin"
        else:
            kadmin = "kadmin -k -p " + strAdmin
        sudostr = ''
    cmdline = sudostr + kadmin + " -q 'delete_principal -force " + \
        strPrinc + "'"
    return cmdline

def AddToKeytabInternal(
    strPrinc : str,
    strKeytab : str,
    bAdd : bool,
    bLocal = True,
    strAdmin = None ) -> str:
    if bLocal:
        kadmin = "kadmin.local"
        sudostr = '{sudo} '
    else:
        if strAdmin is None:
            kadmin = "kadmin"
        else:
            kadmin = "kadmin -k -p " + strAdmin
        sudostr = ''

    strKtPath = os.path.dirname( strKeytab )
    cmdline = ""
    if not os.access( strKtPath, os.R_OK | os.X_OK ):
        cmdline += "mkdir -p " + strKtPath + ";"

    if len( strKeytab ) == 0:
        cmdline += sudostr + kadmin + " -q 'ktrem " + strPrinc + "'"
        if bAdd:
            cmdline += ';'
            cmdline += sudostr + kadmin + " -q 'ktadd " + strPrinc + "'"
    else:
        cmdline += sudostr + kadmin + \
            " -q 'ktrem -k " + strKeytab + " " + strPrinc + "'"
        if bAdd:
            cmdline += ';'
            cmdline += sudostr + kadmin + \
                " -q 'ktadd -k " + strKeytab + " " + strPrinc + "'"
    return cmdline

def AddToKeytab(
    strPrinc : str,
    strKeytab : str,
    bLocal = True,
    strAdmin = None ) -> str:
    return AddToKeytabInternal(
        strPrinc, strKeytab, True, bLocal, strAdmin )

def DelFromKeytab(
    strPrinc : str,
    strKeytab : str,
    bLocal = True,
    strAdmin = None ) -> str:
    return AddToKeytabInternal(
        strPrinc, strKeytab, False, bLocal, strAdmin )

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

def GetKadminSvcName() -> str:
    strDist = GetDistName()
    if strDist == "debian":
        return 'krb5-admin-server'
    elif strDist == 'fedora':
        return 'kadmin'
    return ""

def IsKinitProxyEnabled()->bool:
    strDist = GetDistName()
    if strDist == "debian":
        cmdline = "destPath=`dpkg -L libkrb5-3 | grep 'plugins/libkrb5'| head -n 1`/libauth.so"
    elif strDist == 'fedora':
        cmdline = "destPath=`rpm -ql krb5-libs | grep 'plugins/libkrb5' | head -n 1`/libauth.so"
    else:
        return False
    cmdline += " && ls $destPath 2>/dev/null"
    ret = rpcf_system( cmdline )
    if ret == 0:
        return True
    return False

def EnableKinitProxy( bEnable : bool ) -> bool:
    strDist = GetDistName()
    if strDist == "debian":
        cmdline = "destPath=`dpkg -L libkrb5-3 | grep 'plugins/libkrb5'| head -n 1`"
    elif strDist == 'fedora':
        cmdline = "destPath=`rpm -ql krb5-libs | grep 'plugins/libkrb5' | head -n 1`"
    else:
        return False
    if bEnable:
        cmdline += ";srcPath=`echo $destPath | sed 's:krb5/plugins/libkrb5::'`libauth.so;"
        cmdline += "if [ ! -f $srcPath ]; then srcPath='/usr/local/lib/libauth.so';fi;"
        cmdline += "if [ -f $srcPath ]; then {sudo} ln -s $srcPath $destPath;"
        cmdline += "else echo cannot find libauth.so; fi"
    else:
        cmdline += ";if [ -f $destPath/libauth.so ]; then {sudo} rm $destPath/libauth.so;fi"

    if os.geteuid() == 0:
        actCmd = cmdline.format( sudo = '' )
    elif IsSudoAvailable():
        actCmd = cmdline.format( sudo = 'sudo' )
    else:
        actCmd = "su -c '" + cmdline.format(
            sudo = "" ) + "'"
    print( actCmd )
    ret = rpcf_system( actCmd )
    if ret == 0:
        return True
    return False

def GetKdcSvcName() -> str:
    strDist = GetDistName()
    if strDist == "debian":
        return 'krb5-kdc'
    elif strDist == 'fedora':
        return 'krb5kdc'
    return ""

def GetKdcConfPath() -> str:
    strDist = GetDistName()
    if strDist == 'debian':
        return '/etc/krb5kdc'
    elif strDist == 'fedora':
        return '/var/kerberos/krb5kdc'
    return ""

def GetTestKeytabPath()->str:
    return os.path.expanduser( "~" ) + "/.rpcf/krb5/krb5.keytab"

def GetTestAdminKeytabPath()->str:
    return os.path.expanduser( "~" ) + "/.rpcf/krb5/krb5adm.keytab"

def IsTestKdcSet() :
    strPath = os.path.dirname( GetTestKeytabPath() )
    strPath += "/kdcinited" 
    if os.access( strPath, os.F_OK ):
        return True
    return False

def ChangeKeytabOwner(
    strKeytab: str,
    strUser : str = "" ) -> str:
    try:
        if len( strUser ) == 0:
            strUser = os.getlogin()
        cmdline = "{sudo} chown " + strUser + " " + strKeytab + ";"
        cmdline += "{sudo} chmod 600 " + strKeytab
    except:
        print( "getlogin failed")
        return ""
    return cmdline

def IsLocalIpAddr(
    strIpAddr : str ) -> bool:
    bRet = True
    try:
        s = socket.socket(
            socket.AF_INET, socket.SOCK_STREAM )
        s.bind( ( strIpAddr, 54312 ) )
        s.listen()
        return True
    except Exception as err:
        #print(err)
        return False
    finally:
        s.close()

def ReplaceKdcAddr( astRoot : AstNode, strAddr : str ) -> int:
    ret = 0
    try:
        for i in astRoot.children:
            if i.strVal != '[realms]':
                continue
            for realm in i.children:
                block = realm.children[1]
                for kv in block.children:
                    if kv.children[0].strVal == 'kdc' :
                        kv.children[1].strVal = strAddr
                    elif kv.children[0].strVal == 'admin_server' :
                        kv.children[1].strVal = strAddr
    except Exception as err :
        print( err )
        if ret == 0:
            ret = -errno.EFAULT
    return ret

def GetNewRealmAstNode(
    strRealm : str,
    strKdcAddr : str ) ->Tuple[ int, AstNode ]:
    newRealm = '''
[realms]
{RealmUpper} = {{
kdc = {KdcServer}
admin_server = {KdcServer}
default_domain = {RealmLower}
}}

[domain_realm]
.{RealmLower} = {RealmUpper}
{RealmLower} = {RealmUpper}

'''
    ret = 0
    try:
        strFile = '/tmp/sfdaf122'
        fp = open( strFile , "w" )
        fp.write( newRealm.format(
            RealmUpper = strRealm.upper(),
            RealmLower=strRealm.lower(),
            KdcServer = strKdcAddr ) )
        fp.close()
        fp = None
        return ParseKrb5Conf( strFile )
    except Exception as err:
        print( err )
        if ret == 0:
            ret = -errno.EFAULT
        return ( ret, None )
    finally:
        if fp is not None:
            fp.close()
        if strFile is not None:
            os.unlink( strFile )

def AddNewRealm(
    astRoot : AstNode,
    strRealm : str,
    strKdcAddr : str ) -> int:
    try:
        ret, astNew = GetNewRealmAstNode(
            strRealm, strKdcAddr )
        if ret < 0:
            return ret

        realmToAdd = None
        for i in astNew.children:
            if i.strVal != '[realms]':
                continue
            realmToAdd = i.children[0]
            break

        realmsDest = None
        for i in astRoot.children:
            if i.strVal != '[realms]':
                continue
            realmsDest = i
            break

        realmsDest.AddChild( realmToAdd )

        drDest = None
        for i in astRoot.children:
            if i.strVal != '[domain_realm]':
                continue
            drDest = i
            break

        drkvToAdd = []
        for i in astNew.children:
            if i.strVal != '[domain_realm]':
                continue
            drkvToAdd.extend( i.children )
            break
        for i in drkvToAdd:
            drDest.AddChild( i )

    except Exception as err:
        print( err )
        if ret == 0:
            ret = -errno.EFAULT
    return ret

def UpdateLibDefaults( ast : AstNode,
    strNewRealm : str, bInstall : bool ) -> int:
    ret = 0
    try:
        strPath = os.path.dirname( GetTestKeytabPath() )
        libDef = None
        for i in ast.children:
            if i.strVal != '[libdefaults]':
                continue
            libDef = i
        bKeytab = False
        bKeytabCli = False
        for i in libDef.children:
            if i.children[0].strVal == 'default_keytab_name' :
                i.children[1].strVal = strPath + "/krb5.keytab"
                bKeytab = True
            elif i.children[0].strVal == 'default_client_keytab_name':
                i.children[1].strVal = strPath + "/krb5cli.keytab"
                bKeytabCli = True
            elif i.children[0].strVal == 'default_realm':
                i.children[1].strVal = strNewRealm.upper()

        if bInstall:
            return ret

        if not bKeytab:
            assign = AstNode()
            assign.strVal = '='
            assign.id = NodeType.assign
            assign.lineNum = libDef.children[-1].lineNum + 1
            
            key = AstNode()
            key.strVal = 'default_keytab_name'
            key.id = NodeType.key
            key.lineNum = assign.lineNum
            assign.AddChild( key )

            value = AstNode()
            value.strVal = strPath + "krb5.keytab"
            value.id = NodeType.value
            value.lineNum = assign.lineNum
            assign.AddChild( value )
            libDef.AddChild( assign )

        if not bKeytabCli:
            assign = AstNode()
            assign.strVal = '='
            assign.id = NodeType.assign
            assign.lineNum = libDef.children[-1].lineNum + 1
            
            key = AstNode()
            key.strVal = 'default_client_keytab_name'
            key.id = NodeType.key
            key.lineNum = assign.lineNum
            assign.AddChild( key )

            value = AstNode()
            value.strVal = strPath + "krb5cli.keytab"
            value.id = NodeType.value
            value.lineNum = assign.lineNum
            assign.AddChild( value )
            libDef.AddChild( assign )

    except Exception as err:
        print( err )
        if ret == 0:
            ret = -errno.EFAULT

    return ret

def UpdateKrb5Cfg( ast : AstNode,
    strRealm : str,
    strKdcAddr : str,
    strDestPath : str,
    bInstall : bool ) -> int:
    ret = 0
    try:
        ret = ReplaceKdcAddr( ast, strKdcAddr )
        if ret < 0:
            return ret
        ret, realm = FindRealm( ast, strRealm )
        if not ret:
            AddNewRealm( ast, strRealm, strKdcAddr )
        ret = UpdateLibDefaults( ast, strRealm, bInstall )
        if ret < 0:
            return ret
        strRet = ast.Dumps( 0 )
        if len( strDestPath ) > 0:
            fp = open( strDestPath, 'w')
            fp.write( strRet )
            fp.close()
        else:
            print( strRet )
    except Exception as err:
        print( err )
        if ret == 0:
            ret = -errno.EFAULT
    return ret

def GenNewKeytabSvr(
    strSvcHost : str, strRealm : str, strKeytab : str ) -> str:
    components = strSvcHost.strip().split( "@" )
    if len( components ) != 2:
        ret = -errno.EINVAL
        raise Exception(
            'Error invalid service principal' )
    strHostPrinc = components[ 0 ] + "/" + \
        components[ 1 ].lower() + "@" + strRealm

    strAdminPrinc = components[ 0 ] + "/admin" + \
         "@" + strRealm
    cmdline = AddPrincipal(
        strHostPrinc, "", False, strAdminPrinc )
    cmdline += ";"
    cmdline += AddToKeytab(
        strHostPrinc, strKeytab, False, strAdminPrinc )
    cmdline += ";"
    cmdline += DelFromKeytab(
        strAdminPrinc, strKeytab, False, strAdminPrinc )
    return cmdline

def ConfigKrb5( initCfg : dict, curDir : str )-> int:
    ret = 0
    bServer = False
    strSvcHost = None
    try:
        if initCfg[ 'InstToSvr' ] == 'true':
            bServer = True

        oMisc = initCfg[ 'Security' ]['misc']
        if not 'ConfigKrb5' in oMisc:
            return 1

        if oMisc['ConfigKrb5'] != 'true':
            return 2

        bAuth = False
        oConns = initCfg['Connections']
        strIpAddr = ""
        for oConn in oConns:
            if not 'HasAuth' in oConn :
                continue
            if not oConn['HasAuth'] == 'true' :
                continue
            bAuth = True
            strIpAddr = oConn[ 'IpAddress' ]
            break

        if not bAuth:
            return 3

        oAuth = initCfg[ 'Security' ][ 'AuthInfo' ]
        if oAuth[ 'AuthMech' ] != 'krb5':
            return 4

        strSvcHost = oAuth[ 'ServiceName' ]
        strRealm = oAuth[ 'Realm' ]
        strKdcIp = oAuth[ 'KdcIp' ]

        curDir = curDir.strip()
        if curDir == '' :
            curDir = '.'

        destPath = os.path.dirname( GetTestKeytabPath() )
        if destPath == '' :
            raise Exception( "Error empty keytab path" )

        if bServer:
            strKeytab = curDir + '/krb5.keytab'
            destPath += '/krb5.keytab'
        else:
            strKeytab = curDir + '/krb5cli.keytab'
            destPath += '/krb5cli.keytab'

        cmdline = "install -D -m 600 " + \
            strKeytab + " " +  destPath + ";"

        cmdline += ChangeKeytabOwner( destPath )
        cmdline += ";"

        strKrbConf = curDir + "/krb5.conf"
        ret, ast = ParseKrb5Conf( strKrbConf )
        if ret < 0:
            return ret

        ret, section = FindSection( ast, '[libdefaults]' )
        if not ret:
            ret = -errno.ENOENT
            raise Exception( "Error cannot find section [libdefaults]" )

        if bServer:
            strKey = 'default_keytab_name'
        else:
            strKey = 'default_client_keytab_name'

        for kv in section.children:
            if kv.children[ 0 ].strVal == strKey:
                kv.children[ 1 ].strVal = "FILE:" + destPath
                break

        strConf = ast.Dumps( 0 )
        fp = open( strKrbConf, "w" )
        fp.write( strConf )
        fp.close()

        cmdline += '{sudo} install -bm 644 ' + strKrbConf + \
            ' /etc/krb5.conf'

        if bServer:
            cmdline += ";"
            cmdline += GenNewKeytabSvr(
                strSvcHost, strRealm, destPath )

        components = strSvcHost.split( '@' )
        if not IsNameRegistred( components[ 1 ] ):
            strCmd = AddEntryToHosts(
                strIpAddr, components[ 1 ] )
            if len( strCmd ) > 0:
                cmdline += ";" + strCmd

        if not IsNameRegistred( strRealm ):
            strCmd = AddEntryToHosts( strKdcIp,
                "kdc." + strRealm + " " + strRealm)
            if len( strCmd ) > 0:
                cmdline += ";" + strCmd

        if os.geteuid() == 0:
            actCmd = cmdline.format( sudo = '' )
        elif IsSudoAvailable():
            actCmd = cmdline.format( sudo = 'sudo' )
        else:
            actCmd = "su -c '" + cmdline.format(
                sudo = "" ) + "'"

        ret = rpcf_system( actCmd )
        if ret < 0:
            return ret

        if 'KinitProxy' in oMisc and \
            oMisc['KinitProxy'] == 'true':
            EnableKinitProxy( True )

    except Exception as err:
        print( err )
        if ret == 0:
            ret = -errno.EFAULT

    return ret

def ConfigAuthServer( initFile : str ) -> int:
    try:
        ret = 0
        fp = open( initFile, 'r' )
        cfgVal = json.load( fp )
        fp.close()
        ret = ConfigKrb5( cfgVal,
            os.path.dirname( initFile ) )

    except Exception as err:
        print(traceback.format_exc())
        print( err )
        if ret == 0:
            ret = -errno.EFAULT
    return ret

if __name__ == "__main__":
    ret, ast = ParseKrb5Conf( "/etc/krb5.conf" )
    if ret < 0:
        quit( -ret )
    ret = UpdateKrb5Cfg(
        ast, "rpc.org", "127.0.0.1", "", False)
    ast.RemoveChildren()

