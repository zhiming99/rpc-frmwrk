# GENERATED BY RIDLC, MAKE SURE TO BACKUP BEFORE RUNNING RIDLC AGAIN
# Copyright (C) 2024  zhiming <woodhead99@gmail.com>
# This program can be distributed under the terms of the GNU GPLv3.
# ridlc -pO . -l regfs.ridl 
from typing import Tuple
from rpcf.rpcbase import *
from rpcf.proxy import *
from seribase import CSerialBase
from RegistryFsstructs import *
from weblogin import *
import errno

from RegFsSvcLocalcli import CRegFsSvcLocalProxy
import os
import time
import gc
import json
import hashlib as hl
from pathlib import Path
import sys
from encdec import *
from datetime import datetime

import signal
bExit = False
authUrl = None
clientId = None
redirectUrl = None
scope = None
cEncrypt = ""
bRemove = False

def IsEncrypt()->bool:
    global cEncrypt
    return cEncrypt=="z"

def IsEncrypted( filePath : str )->bool:
    base = os.path.basename( filePath )
    return base[0:1] == "z"

def ERROR( ret ) :
    return ret[0] < 0

def SigHandler( signum, frame ):
    global bExit
    bExit = True

def LoadParam( oReg: CRegFsSvcLocalProxy, strPath: str) ->list:
    try:
        ret = oReg.GetAttr(strPath)
        if ERROR( ret ):
            raise Exception( "Error read from {file}".format(file=strPath))
        stBuf = ret[1][0]
        fileSize = stBuf.st_size
        ret = oReg.OpenFile( strPath, os.R_OK )
        if ERROR(ret):
            raise Exception( "Error read from {file}".format(file=strPath))
        hFile = ret[1][0]
        ret = oReg.ReadFile( hFile, fileSize, 0)
        if ERROR(ret):
            raise Exception( "Error read from {file}".format(file=strPath))

        bytesRead = ret[1][0]
        o = json.loads(bytesRead)
        global authUrl
        global clientId
        global redirectUrl
        global scope
        if 'AuthUrl' in o:
            authUrl = o['AuthUrl']
        if 'ClientId' in o:
            clientId = o['ClientId']
        if 'RedirectUrl' in o:
            redirectUrl = o['RedirectUrl']
        if 'Scope' in o:
            scope = o['Scope']
        if authUrl is None or clientId is None or redirectUrl is None or scope is None:
            return [None] * 4
        return [authUrl, clientId, redirectUrl, scope]
    except Exception as err:
        return [None] * 4
    finally:
        if hFile is not None:
            oReg.CloseFile( hFile )

def GetOA2ParamsFromDesc( fileName : str )->list:
    try:
        file=open(fileName, "r" )
        strJson = file.read()
        if len( strJson ) == 0:
            raise Exception( "Error empty description file")
        oDesc = json.loads(strJson)
        if not "Objects" in oDesc:
            raise Exception( "Error bad description file")
        arrObjs = oDesc["Objects"]
        for elem in arrObjs:
            if not "AuthInfo" in elem:
                continue
            oAuth = elem["AuthInfo"]
            if not "AuthMech" in oAuth:
                continue
            strMech = oAuth[ "AuthMech"]
            if strMech != "OAuth2":
                continue
            if not "AuthUrl" in oAuth:
                continue
            if not "Scope" in oAuth:
                continue
            return [oAuth["AuthUrl"], oAuth["ClientId"], oAuth["RedirectUrl"], oAuth[ "Scope"]]
        raise Exception( "Error OAuth2 parameters not found")
    except Exception as err:
        print( err )
        return [None] * 4
    finally:
        if file is not None:
            file.close()

def GetOA2ParamsFromReg( oReg : CRegFsSvcLocalProxy )->list:
    try:
        ret = oReg.Access("/cookies", os.X_OK | os.R_OK)
        if ERROR( ret ):
            raise Exception("no login info in storage")
        ret = oReg.OpenDir( "/cookies", os.R_OK )
        if ERROR( ret ):
            raise Exception("no login info in storage")
        hDir = ret[1][0]
        ret = oReg.ReadDir( hDir )
        if ERROR( ret ):
            raise Exception("no login info in storage")
        vecFileEnt = ret[1][0]
        numCookies = len( vecFileEnt) 
        if numCookies == 0:
            raise Exception("no login info in storage")
        if numCookies == 1:
            fileName = "/cookies/" + vecFileEnt[0].st_name
            return LoadParam( oReg, fileName )
        oFileList = []
        idx = 0
        for ent in vecFileEnt:
            if not ( ent.st_mode & 0o100000 ): 
                continue
            fileName = "/cookies/" + ent.st_name
            ret = oReg.GetAttr(fileName)
            if ERROR( ret ):
                raise Exception( "Error read from {file}".format(file=fileName))
            fileAttr = ret[1][0]
            timestamp = fileAttr.st_mtim.tv_sec
            mtime = datetime.fromtimestamp(timestamp)

            quad = LoadParam( oReg, fileName )
            if quad[0] is None:
                continue
            oFileList.append( (fileName, quad)) 
            if IsEncrypted( fileName ):
                prefix = "enc "
            else:
                prefix = "clr "
            idx+=1
            print( prefix + " {time} {idx}: [AuthUrl]={authUrl},[ClientId]={clientId},[RedirectUrl]={redirectUrl}, [Scope]={scope}".format(
                  time=str(mtime), idx=idx, authUrl=quad[0], clientId=quad[1],redirectUrl=[quad[2]], scope=[quad[3]]) )

        if len( oFileList ) == 0:
            raise Exception("no login info in storage")
        if len( oFileList ) == 1:
            fileName = oFileList[0][0]
            return LoadParam( oReg, fileName )

        userInput = input("Please choose from above the quad to login (1-{idx}):".format( idx=idx ))
        i = int( userInput )
        if i < 1 or i > idx:
            print( "Error invalid index")
            return -errno.EINVAL
        fileName = oFileList[ i - 1 ][0]
        return LoadParam( oReg, fileName )

    except Exception as err:
        print( err )
        return [None] * 4
    finally:
        if hDir is not None:
            oReg.CloseDir(hDir)

def OAuth2Login( oRegistry : CRegFsSvcLocalProxy ) -> int:
    global authUrl, clientId, redirectUrl, scope, cEncrypt
    try:
        if authUrl is None or clientId is None or redirectUrl is None or scope is None:
            authUrl, clientId, redirectUrl, scope  = \
                GetOA2ParamsFromReg( oRegistry )
            if authUrl is None:
                return -errno.ENODATA

        ret = fetch_rpcf_code(
            authUrl, clientId, redirectUrl, scope )
        if ERROR( ret ) :
            return ret[0]
        oCookie = ret[1]
        h = hl.sha1()
        h.update(
            (authUrl + clientId + redirectUrl + scope ).encode() )
        fileName = cEncrypt + h.hexdigest()
        oData = dict()
        oData['oCookie'] = oCookie
        oData['AuthUrl'] = authUrl
        oData['ClientId'] = clientId
        oData['RedirectUrl'] = redirectUrl
        oData['Scope'] = scope
        return SaveCookie( oRegistry, oData, fileName )
    except Exception as err:
        print( err )
        return -errno.EFAULT

def SaveCookie( oRegistry : CRegFsSvcLocalProxy,
    oData : object, fileName : str ):
    try:
        ret = oRegistry.Access("/cookies", os.X_OK | os.R_OK)
        if ERROR(ret):
            ret = oRegistry.MakeDir( "/cookies", 0o755)
            if ERROR(ret) :
                return ret[0]
        strFile = "/cookies/" + fileName
        ret = oRegistry.Access( strFile, os.F_OK )
        if ERROR(ret):
            ret = oRegistry.CreateFile(
                strFile, 0o600, os.O_RDWR )
        else:
            ret = oRegistry.OpenFile(
                strFile, os.W_OK | os.O_TRUNC )
        if ERROR(ret) :
            return ret[ 0 ]

        hFile = ret[ 1 ][0]
        if IsEncrypt():
            strCookie = oData['oCookie']['value']
            byteEnc = Encrypt_Bytes( strCookie.encode() )
            if byteEnc is None:
                raise Exception( "Error encrypting the file") 
            oData['oCookie']['value'] = byteEnc.hex()

        strCookie = json.dumps( oData )
        byteCookie = strCookie.encode()

        ret = oRegistry.WriteFile(
            hFile, byteCookie, 0 )
        oRegistry.CloseFile( hFile )
        if ERROR(ret):
            return ret[0]
        return 0
    except Exception as err:
        print( err )
        return -errno.EFAULT
    
def MainEntryCli() :
    signal.signal( signal.SIGINT, SigHandler)
    oContext = PyRpcContext( 'PyRpcProxy' )
    with oContext as ctx:
        if ctx.status < 0:
            ret = ctx.status
            print( os.getpid(), 
                "Error start PyRpcContext %d" % ret )
            return ret
        
        print( "start to work here..." )
        strPath_ = os.path.dirname( os.path.realpath( __file__) )
        strPath_ += '/RegistryFsdesc.json'

        # start a cpp server
        oCfg = cpp.CParamList()
        oCfg.SetObjPtr( cpp.propIoMgr, ctx.pIoMgr )
        strHome = str( Path.home() )

        cfgFile = strHome + "/.rpcf/registry.dat"
        strRpcfDir = strHome + "/.rpcf"
        bFormat = False
        if not os.path.exists( strRpcfDir ):
            os.makedirs( strRpcfDir )
            bFormat = True
        elif not os.path.exists( cfgFile ):
            bFormat = True
        elif os.stat( cfgFile ).st_size == 0:
            bFormat = True

        if bFormat :
            oCfg.SetBoolProp( 0, bFormat )
        oCfg.SetStrProp( cpp.propConfigPath, cfgFile )

        print( "starting RegistrySvr..." )
        ret = cpp.CRpcServices.LoadObjDesc( strPath_,
            "RegFsSvcLocal", True,
            oCfg.GetCfg() )
        if ret < 0 :
            return ret

        iClsid = cpp.CoGetClassId(
            "CRegFsSvcLocal_SvrImpl" )
        if iClsid == 0xffffffff :
            return -errno.ENOENT
        pObj = oCfg.GetCfgAsObj()
        pCfg = cpp.CastToCfg( pObj )
        oSvrInst = cpp.CreateObject(
            iClsid, pCfg )
        if not oSvrInst :
            return ErrorCode.ERROR_FAIL

        oSvc = cpp.CastToSvc( oSvrInst )
        try:
            ret = oSvc.Start()
            if ret < 0:
                oSvc = None
                return ret
            print( "Starting the proxy" )
            oProxy_RegFsSvcLocal = CRegFsSvcLocalProxy( ctx.pIoMgr,
                strPath_, 'RegFsSvcLocal' )
            ret = oProxy_RegFsSvcLocal.GetError()
            if ret < 0 :
                return ret
            
            with oProxy_RegFsSvcLocal as oProxy:
                global bExit
                try:
                    ret = oProxy.GetError()
                    if ret < 0 :
                        raise Exception( 'start proxy failed' )
                    state = oProxy.oInst.GetState()
                    while state == cpp.stateRecovery :
                        time.sleep( 1 )
                        state = oProxy.oInst.GetState()
                        if bExit:
                            break
                    if state != cpp.stateConnected or bExit:
                        return ErrorCode.ERROR_STATE
                    ret = maincli( oProxy )
                except Exception as err:
                    print( err )
                
            oProxy_RegFsSvcLocal = None
        finally:
            if oSvc:
                oSvc.Stop()
                oSvc = None
            oSvrInst.Clear()
            pObj.Clear()
            gc.collect()
    oContext = None
    return ret
    
def RemoveFromReg( oReg : CRegFsSvcLocalProxy )->list:
    try:
        ret = oReg.Access("/cookies", os.X_OK | os.R_OK)
        if ERROR( ret ):
            raise Exception("Info no login info in storage")
        ret = oReg.OpenDir( "/cookies", os.R_OK )
        if ERROR( ret ):
            raise Exception("Info no login info in storage")
        hDir = ret[1][0]
        ret = oReg.ReadDir( hDir )
        if ERROR( ret ):
            raise Exception("Info no login info in storage")
        vecFileEnt = ret[1][0]
        numCookies = len( vecFileEnt) 
        if numCookies == 0:
            raise Exception("Info no login info in storage")

        oFileList = []
        idx = 0
        for ent in vecFileEnt:
            if not ( ent.st_mode & 0o100000 ): 
                continue
            fileName = "/cookies/" + ent.st_name
            ret = oReg.GetAttr(fileName)
            if ERROR( ret ):
                raise Exception( "Error read from {file}".format(file=fileName))
            fileAttr = ret[1][0]
            timestamp = fileAttr.st_mtim.tv_sec
            mtime = datetime.fromtimestamp(timestamp)

            quad = LoadParam( oReg, fileName )
            if quad[0] is None:
                continue
            oFileList.append( (fileName, quad)) 
            if IsEncrypted( fileName ):
                prefix = "enc "
            else:
                prefix = "clr "
            idx+=1
            print( prefix + " {time} {idx}: [AuthUrl]={authUrl},[ClientId]={clientId},[RedirectUrl]={redirectUrl}, [Scope]={scope}".format(
                  time=str(mtime), idx=idx, authUrl=quad[0], clientId=quad[1],redirectUrl=[quad[2]], scope=[quad[3]]) )

        if len( oFileList ) == 0:
            raise Exception("Info no login info in storage")

        userInput = input("Please choose from above the list to login (1-{idx}):".format( idx=idx ))
        i = int( userInput )
        if i < 1 or i > idx:
            print( "Error invalid index")
            return
        fileName = oFileList[ i - 1 ][0]
        userInput = input("Confirm to remove login infomation {idx}(y/n):".format( idx=i ))
        if userInput == "y" :
            oReg.RemoveFile(fileName)
        else:
            print( "The login information is kept.")
        return
    except Exception as err:
        print( err )
        return
    finally:
        if hDir is not None:
            oReg.CloseDir(hDir)

import getopt
def usage():
    print( "Usage: python3 oinit.py [options] [<auth url> <client id> <redirect url> <scope>]" )
    print( "\t To perform an OAuth2 authorization code login to an rpc-frmwrk server for rpc-frmwrk non-js clients." )
    print( "\t -d <description file>. To find the login information from the given desc file")
    print( "\t -c 1 Using the example container 'django_oa2check_https' for OAuth2 login")
    print( "\t -c 2 Using the example container 'springboot_oa2check_https' for OAuth2 login")
    print( "\t -e to encrypt the login cookie in the storage")
    print( "\t -r to remove from a list of stored login information.")
    print(" \t If " +
         "'authorize url', 'client id', 'redirect url' and 'scope' are specified, " +
         "oinit.py will use the four to perform the OAuth2 login" )
    print( "\t If neither options nor parameters are given, oinit will try to use the stored login info " + \
          "in the registry to perform the OAuth2 login.")
    print( "\t And you only need to run this command next time when the access token expired" )

def maincli( oProxy: CRegFsSvcLocalProxy ):
    try:
        opts, args = getopt.getopt(sys.argv[1:], "c:d:ehr" )
    except getopt.GetoptError as err:
        # print help information and exit:
        print(err)  # will print something like "option -a not recognized"
        usage()
        return -errno.EINVAL

    global authUrl
    global clientId
    global redirectUrl
    global scope
    global cEncrypt
    global bRemove

    bHasOptions = False
    for o, a in opts:
        if o == "-d" :
            authUrl, clientId, redirectUrl, scope = \
                GetOA2ParamsFromDesc( a )
        elif o == "-c" :
            clientId = "IpCMFeunGnjKb0FhSMxnI4MWH8tEycSTpIlQkFP3"
            authUrl ="https://172.17.0.3/o/authorize/"
            if int( a ) == 1:
                redirectUrl = "https://172.17.0.2/oa2check/callback/"
                scope = "read+write"
            elif int( a ) == 2:
                redirectUrl = "https://172.17.0.2/login/oauth2/code/oa2check"
                scope = "profile+email"
        elif o == "-e":
            cEncrypt = "z"
        elif o == "-r":
            RemoveFromReg( oProxy )
            bRemove = True
            return 0
        elif o == "-h" :
            usage()
            sys.exit( 0 )
        else:
            assert False, "unknown option"
        bHasOptions = True

    argc = len( args )

    if not bHasOptions and argc == 4:
        authUrl = args[0]
        clientId = args[1]
        redirectUrl = args[2]
        scope = args[3]
    elif bHasOptions and argc > 0:
        print( "Error, found unexpected parameters")
        usage()
        sys.exit( -errno.EINVAL )

    return OAuth2Login( oProxy )
    

if __name__ == '__main__' :
    ret = MainEntryCli()
    if bRemove:
        quit( 0 )
    if ret == 0 :
        print( "Login Completed" )
    elif ret < 0:
        print( "Error occurs during login({error})".format( error=-ret ) )
    quit( -ret )
