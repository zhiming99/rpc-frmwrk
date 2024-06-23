#!/usr/bin/env python3
# This script disables SSL settings of the rpc-frmwrk for debug purpose.
import json
import os
import sys 
from urllib.parse import urlsplit
import errno
from updcfg import *

def GetTestPaths2( path : str= None ) :
    dir_path = os.path.dirname(os.path.realpath(__file__))
    paths = []

    curDir = dir_path
    paths.append( curDir + "/../../etc/rpcf" )
    paths.append( "/etc/rpcf")
    paths.append( "/usr/local/etc/rpcf")

    return paths

def ChangeUrlToHttp( url ):
    urlComp = urlsplit( url )
    urlComp = urlComp._replace( scheme='http' )
    if urlComp.port == 443 :
        urlComp = urlComp._replace(
            netloc = urlComp.netloc.replace(
                str( urlComp.port ), "80" ) )
    return urlComp.geturl()

def DisableSSLTestCfg( jsonVal ):
    try:
        objs = jsonVal[ 'Objects' ]
        for elem in objs :
            if elem[ 'EnableSSL' ] == 'false' :
                continue
            elem[ 'EnableSSL' ] = 'false'
            if elem[ 'EnableWS' ] == 'false' :
                continue

            if 'DestURL' not in elem :
                continue
            elem[ 'DestURL' ] = ChangeUrlToHttp( 
                elem[ 'DestURL' ] )

    except Exception as err:
        print( "Error DisableSSLTestCfg", file=sys.stderr )
    return

def DisableSSLTestCfgs():
    testDescs = [ 'echodesc.json', 'btinrt.json' ]
    if IsFeatureEnabled( 'testcases' ):
        testDescs.extend(  [ "actcdesc.json",
            "asyndesc.json",
            "evtdesc.json",
            "hwdesc.json",
            "kadesc.json",
            "prdesc.json",
            "sfdesc.json",
            "stmdesc.json" ] )

    paths = GetTestPaths2()
    pathVal = ReadTestCfg( paths, testDescs[ 0 ] )
    if pathVal is None :
        paths = GetTestPaths2()

    ret = 0
    for testDesc in testDescs :
        pathVal = ReadTestCfg( paths, testDesc )
        if pathVal is None :
            continue
        DisableSSLTestCfg( pathVal[ 1 ] )
        ret = WriteTestCfg(
            pathVal[ 0 ], pathVal[ 1 ] )
        if ret < 0 :
            break

    if ret < 0 :
        return ret
    paths = GetPyTestPaths()
    pathVal = ReadTestCfg( paths, 'sfdesc.json' )
    if pathVal is not None :
        DisableSSLTestCfg( pathVal[ 1 ] )
        ret = WriteTestCfg(
            pathVal[ 0 ], pathVal[ 1 ] )
    return ret

def LoadDriverFile() :
    dir_path = os.path.dirname(os.path.realpath(__file__))
    parent_dir = os.path.basename( dir_path )
    paths = []
    curDir = dir_path

    paths.append( "." )
    paths.append( curDir + "/../../etc/rpcf" )
    paths.append( "/etc/rpcf")
    paths.append( "/usr/local/etc/rpcf")
    
    count = 1
    jsonvals = [ None ] * 1
    for strPath in paths :
        try:
            if jsonvals[ 0 ] is None :
                drvfile = strPath + "/driver.json"
                fp = open( drvfile, "r" )
                jsonvals[ 0 ] = [drvfile, json.load(fp) ]
                fp.close()
                count-=1
        except Exception as err :
            pass

        if count == 0:
            break

    if count == 0:
        return jsonvals

    raise Exception( "Error open driver.json" )

def DisableSSLDrv() -> int :

    jsonFiles = LoadDriverFile()
    if jsonFiles is None :
        return -errno.EFAULT

    drvFile = jsonFiles[ 0 ]
    drvVal = drvFile[ 1 ]

    if 'Ports' in drvVal :
        ports = drvVal['Ports']
    else :
        return -errno.ENOENT

    for port in ports :
        if port[ 'PortClass'] != 'RpcTcpBusPort' :
            continue
        for elem in port[ 'Parameters' ] :
            if elem[ 'EnableSSL' ] == "false" :
                continue

            elem[ 'EnableSSL' ] = 'false'
            if elem[ 'EnableWS' ] == 'false' :
                continue

            if 'DestURL' not in elem :
                continue

            elem[ 'DestURL' ] = ChangeUrlToHttp( 
                elem[ 'DestURL' ] )
        break

    drvPath = drvFile[ 0 ]
    return OverwriteJsonFile( drvPath, drvVal )

def DisableSSL() -> int :
    ret = 0
    try:
        ret = DisableSSLDrv()
        if ret < 0 :
            return ret

        ret = DisableSSLTestCfgs()

    except Exception as err:
        text = "DisableSSL failed:" + str( err )
        exc_type, exc_obj, exc_tb = sys.exc_info()
        fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
        second_text = "@" + fname + ":" + str(exc_tb.tb_lineno)
        print( text, second_text )
        ret = -errno.EFAULT

    return ret

if __name__ == "__main__":
    DisableSSL()
