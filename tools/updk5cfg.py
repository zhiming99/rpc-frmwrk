#!/usr/bin/env python3
import json
import os
from typing import Dict
from typing import Tuple
import errno
import socket
import re
from krbparse import *
from updwscfg import IsSudoAvailable

    
def AddEntryToHosts(
    strIpAddr : str,
    strDomain : str ) -> str:

    bExist = False
    pattern = strIpAddr + ".*" + strDomain
    with open('/etc/hosts', 'r') as fp:
        for line in fp:
            if re.search( pattern, line ):
                bExist = True
                break
    if bExist :
        return ""

    cmdline = "{sudo} sh -c 'echo \"" + strIpAddr + '\t' + \
        strDomain + "\" >> /etc/hosts'"
    return cmdline

def AddPrincipal(
    strPrinc : str,
    strPass : str,
    bLocal = True ) -> str:
    if bLocal:
        kadmin = "kadmin.local"
    else:
        kadmin = "kadmin"
    cmdline = "{sudo} " + kadmin + " -q 'addprinc "
    if len( strPass ) != 0:
        cmdline += "-pw \"" + strPass + "\" " + strPrinc + "'"
    else:
        cmdline += "-randkey " + strPrinc + "'"
    return cmdline

def DeletePrincipal(
    strPrinc : str,
    bLocal = True ) -> str :
    if bLocal:
        kadmin = "kadmin.local"
    else:
        kadmin = "kadmin"
    cmdline = "{sudo} " + kadmin + " -q 'delete_principal -force " + \
        strPrinc + "'"
    return cmdline

def AddToKeytab(
    strPrinc : str,
    strKeytab : str,
    bLocal = True ) -> str:
    if bLocal:
        kadmin = "kadmin.local"
    else:
        kadmin = "kadmin"
    strKtPath = os.path.dirname( strKeytab )
    cmdline = ""
    if not os.access( strKtPath, os.R_OK | os.X_OK ):
        cmdline += "mkdir -p " + strKtPath + "&&"

    if len( strKeytab ) == 0:
        cmdline += "{sudo} " + kadmin + " -q 'ktrem " + strPrinc + "';"
        cmdline += "{sudo} " + kadmin + " -q 'ktadd " + strPrinc + "'"
    else:
        cmdline += "{sudo} " + kadmin + \
            " -q 'ktrem -k " + strKeytab + " " + strPrinc + "';"
        cmdline += "{sudo} " + kadmin + \
            " -q 'ktadd -k " + strKeytab + " " + strPrinc + "'"
    return cmdline

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

def IsTestKdcSet() :
    strPath = os.path.dirname( GetTestKeytabPath() )
    strPath += "/kdcinited" 
    if os.access( strPath, os.F_OK ):
        return True
    return False

def ChangeKeytabOwner(
    strKeytab: str,
    strUser : str = "" ) -> str:
    if len( strUser ) == 0:
        strUser = os.getlogin()
    cmdline = "{sudo} chown " + strUser + " " + strKeytab + ";"
    cmdline += "{sudo} chmod 600 " + strKeytab
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
        print(err)
        return False
    finally:
        s.close()

def Config_Kerberos2( initCfg : object ) -> int:
    ret = 0
    try:
        bServer = False
        if initCfg[ 'IsServer' ] == 'true' :
            bServer = True

    except Exception as err:
        print( err )
        if ret == 0:
            ret = -errno.EFAULT
        return ret

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

def ConfigKrb5( initCfg : dict, curDir : str )-> int:
    ret = 0
    bServer = False
    try:
        if initCfg[ 'IsServer' ] == 'true':
            bServer = True

        oMisc = initCfg[ 'Security' ]['misc']
        if not 'ConfigKrb5' in oMisc:
            return ret

        if oMisc['ConfigKrb5'] != 'true':
            return ret

        bAuth = False
        oConns = initCfg['Connections']
        for oConn in oConns:
            if not 'HasAuth' in oConn :
                continue
            if not oConn['HasAuth'] == 'true' :
                continue
            bAuth = True
            break

        if not bAuth:
            return ret

        oAuth = initCfg[ 'Security' ][ 'AuthInfo' ]
        if oAuth[ 'AuthMech' ] != 'krb5':
            return ret

        curDir = curDir.strip()
        if curDir == '' :
            curDir = '.'

        destPath = GetTestKeytabPath()
        if destPath == '' :
            raise Exception( "Error empty keytab path" )

        if bServer:
            strKeytab = curDir + '/krb5.keytab'
            destPath += '/krb5.keytab'
        else:
            strKeytab += curDir + '/krb5cli.keytab'
            destPath += '/krb5cli.keytab'

        cmdline = "install -m 600 " + \
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

        if os.geteuid() == 0:
            actCmd = cmdline.format( sudo = '' )
        elif IsSudoAvailable():
            actCmd = cmdline.format( sudo = 'sudo' )
        else:
            actCmd = "su -c '" + cmdline.format(
                sudo = "" ) + "'"

        ret = rpcf_system( actCmd )

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

