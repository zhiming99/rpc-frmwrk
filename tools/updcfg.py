import json
import os
import sys 
from shutil import move
from copy import deepcopy
from urllib.parse import urlparse
from typing import Dict
import errno

def GetTestPaths( path : str= None ) :
    dir_path = os.path.dirname(os.path.realpath(__file__))
    paths = []
    if path is None:
        curDir = dir_path
        paths.append( curDir + "/../../etc/rpcf" )
        paths.append( "/etc/rpcf")

        curDir += "/../test/"
        paths.append( curDir + "actcancel" )
        paths.append( curDir + "asynctst" )
        paths.append( curDir + "btinrt" )
        paths.append( curDir + "evtest" )
        paths.append( curDir + "helloworld" )
        paths.append( curDir + "iftest" )
        paths.append( curDir + "inproc" )
        paths.append( curDir + "katest" )
        paths.append( curDir + "prtest" )
        paths.append( curDir + "sftest" )
        paths.append( curDir + "stmtest" )
    else:
        paths.append( path )

    return paths

def ReadTestCfg( paths:list, name:str) :
    jsonVal = None
    for path in paths:
        try:
            cfgFile = path + "/" + name
            fp = open( cfgFile, "r" )
            jsonVal = [cfgFile, json.load(fp) ]
            fp.close()
        except Exception as err:
            continue

        return jsonVal
    return None

def GetPyTestPaths() :
    dir_path = os.path.dirname(os.path.realpath(__file__))
    paths = []
    paths.append( dir_path +
        "/../python/tests/sftest" )
    return paths

def UpdateTestCfg( jsonVal, cfgList:list ):
    try:
        ifCfg = cfgList[ 0 ]
        authInfo = cfgList[ 1 ]
        if not 'Objects' in jsonVal :
            return

        objs = jsonVal[ 'Objects' ]
        for elem in objs :
            elem[ 'IpAddress' ] = ifCfg[ 'BindAddr' ]
            elem[ 'PortNumber' ] = ifCfg[ 'PortNumber' ]
            elem[ 'Compression' ] = ifCfg[ 'Compression' ]
            elem[ 'EnableSSL' ] = ifCfg[ 'EnableSSL' ]
            elem[ 'EnableWS' ] = ifCfg[ 'EnableWS' ]
            elem[ 'RouterPath' ] = ifCfg[ 'RouterPath' ]
            if 'HasAuth' in ifCfg and ifCfg[ 'HasAuth' ] == 'true' :
                elem[ 'AuthInfo' ] = authInfo
            else :
                elem.pop( 'AuthInfo', None )
            if elem[ 'EnableWS' ] == 'true' :
                elem[ 'DestURL' ] = ifCfg[ 'DestURL' ]
                urlComp = urlparse( elem[ 'DestURL' ], scheme='https' )
                if urlComp.port is None :
                    elem[ 'PortNumber' ] = '443'
                else :
                    elem[ 'PortNumber' ] = urlComp.port
                if urlComp.hostname is not None:
                    elem[ 'IpAddress' ] = urlComp.hostname
                else :
                    raise Exception( 'web socket URL is not valid' ) 
            else :
                elem.pop( 'DestURL', None )

    except Exception as err:
        pass

    return

def WriteTestCfg( path, jsonVal ) :
    fp = open(path, "w")
    json.dump( jsonVal, fp, indent=4)
    fp.close()
    return

def ExportTestCfgsTo( cfgList:list, destPath:str ):
    testDescs = [ "actcdesc.json",
        "asyndesc.json",
        "btinrt.json",
        "evtdesc.json",
        "hwdesc.json",
        "echodesc.json",
        "kadesc.json",
        "prdesc.json",
        "sfdesc.json",
        "stmdesc.json" ]

    paths = GetTestPaths( destPath )
    pathVal = ReadTestCfg( paths, testDescs[ 0 ] )
    if pathVal is None :
        paths = GetTestPaths()

    for testDesc in testDescs :
        pathVal = ReadTestCfg( paths, testDesc )
        if pathVal is None :
            continue
        UpdateTestCfg( pathVal[ 1 ], cfgList )
        WriteTestCfg(
            destPath + "/" + testDesc,
            pathVal[ 1 ] )

    paths = GetPyTestPaths()
    pathVal = ReadTestCfg( paths, 'sfdesc.json' )
    if pathVal is not None :
        UpdateTestCfg( pathVal[ 1 ], cfgList )
        WriteTestCfg(
            destPath + "/sfdesc-py.json",
            pathVal[ 1 ] )

def ExportTestCfgs( cfgList:list ):
    testDescs = [ "actcdesc.json",
        "asyndesc.json",
        "btinrt.json",
        "evtdesc.json",
        "hwdesc.json",
        "echodesc.json",
        "kadesc.json",
        "prdesc.json",
        "sfdesc.json",
        "stmdesc.json" ]

    paths = GetTestPaths()
    for testDesc in testDescs :
        pathVal = ReadTestCfg( paths, testDesc )
        if pathVal is None :
            continue
        UpdateTestCfg( pathVal[ 1 ], cfgList )
        WriteTestCfg( pathVal[ 0 ], pathVal[ 1 ] )

    paths = GetPyTestPaths()
    pathVal = ReadTestCfg( paths, 'sfdesc.json' )
    if pathVal is not None :
        UpdateTestCfg( pathVal[ 1 ], cfgList )
        WriteTestCfg( pathVal[ 0 ], pathVal[ 1 ] )

def LoadConfigFiles( path : str) :
    dir_path = os.path.dirname(os.path.realpath(__file__))
    paths = []
    if path is None:
        curDir = dir_path
        paths.append( curDir + "/../../etc/rpcf" )
        paths.append( "/etc/rpcf")

        paths.append( curDir + "/../ipc" )
        paths.append( curDir + "/../rpc/router" )
        paths.append( curDir + "/../rpc/security" )
    else:
        paths.append( path )
    
    jsonvals = [ None ] * 4
    for strPath in paths :
        try:
            if jsonvals[ 0 ] is None :
                drvfile = strPath + "/driver.json"
                fp = open( drvfile, "r" )
                jsonvals[ 0 ] = [drvfile, json.load(fp) ]
                fp.close()
        except Exception as err :
            pass
        try:
            if jsonvals[ 1 ] is None :
                rtfile = strPath + "/router.json"
                fp = open( rtfile, "r" )
                jsonvals[ 1 ] = [rtfile, json.load(fp)]
                fp.close()
        except Exception as err :
            pass

        try:
            if jsonvals[ 2 ] is None :
                rtaufile = strPath + "/rtauth.json"
                fp = open( rtaufile, "r" )
                jsonvals[ 2 ] = [rtaufile, json.load(fp)]
                fp.close()
        except Exception as err :
            pass
        
        try:
            if jsonvals[ 3 ] is None :
                auprxyfile = strPath + "/authprxy.json"
                fp = open( auprxyfile, "r" )
                jsonvals[ 3 ] = [auprxyfile, json.load(fp)]
                fp.close()
        except Exception as err :
            pass            

    return jsonvals

def Update_AuthPrxy( initCfg: dict, drvFile : list,
    bServer: bool, destDir : str ) -> int :

    ret = 0
    apVal = drvFile[ 1 ]
    proxies = apVal['Objects']
    if proxies is None or len( proxies ) == 0:
        raise Exception("bad content in authprxy.json") 

    oConns = None
    oConn0 = None
    if 'Connections' in initCfg :
        oConns = initCfg[ 'Connections' ]
        if len( oConns ) > 0 :
            oConn0 = oConns[ 0 ]

    authInfo = None
    kdcIp = None
    userName = None
    if ( 'Security' in initCfg and
        'AuthInfo' in initCfg[ 'Security' ] ):
        authInfo = deepcopy(
            initCfg[ 'Security' ][ 'AuthInfo' ] )
        if bServer :
            kdcIp = authInfo[ 'KdcIp' ]
        authInfo.pop( 'KdcIp', None )
        if 'UserName' in authInfo :
            userName = authInfo[ 'UserName' ]

    for proxy in proxies :
        objName = proxy[ 'ObjectName' ]
        if objName == 'K5AuthServer' or objName == 'KdcChannel':
            if oConn0 is None:
                continue
            elem = proxy
            elem[ 'IpAddress' ] = oConn0[ 'IpAddress' ]
            elem[ 'PortNumber' ] = oConn0[ 'PortNumber' ]
            elem[ 'Compression' ] = oConn0[ 'Compression' ]
            elem[ 'EnableSSL' ] = oConn0[ 'EnableSSL' ]

            if objName == 'K5AuthServer' :
                elem.pop( 'HasAuth', None )
            else :
                if authInfo is not None:
                    authInfo[ 'UserName' ] = 'kdcclient'
                    elem[ 'AuthInfo' ] = authInfo

            elem[ 'EnableWS' ] = oConn0[ 'EnableWS' ]
            if elem[ 'EnableWS' ] == 'true' :
                elem[ 'DestURL'] = oConn0[ 'DestURL' ]
                urlComp = urlparse( elem[ 'DestURL' ], scheme='https' )
                if urlComp.port is None :
                    elem[ 'PortNumber' ] = '443'
                else :
                    elem[ 'PortNumber' ] = urlComp.port
                if urlComp.hostname is not None:
                    elem[ 'IpAddress' ] = urlComp.hostname
                else :
                    raise Exception( 'web socket URL is not valid' ) 

        elif objName == 'KdcRelayServer' :
            if kdcIp is not None and bServer :
                proxy[ 'IpAddress' ] = kdcIp
            break

    if destDir is None :
        apPath = drvFile[ 0 ]
    else :
        apPath = destDir + "/authprxy.json"

    fp = open( apPath, "w" )
    json.dump( apVal, fp, indent = 4  )
    fp.close()

    # update test configs
    if oConn0 is None :
        return ret

    if userName is not None :
        authInfo[ 'UserName' ] = userName

    oConn0[ 'BindAddr' ] = oConn0[ 'IpAddress' ]
    cfgList = [ oConn0, authInfo ]
    if destDir is None :
        ExportTestCfgs( cfgList )
    else:
        ExportTestCfgsTo( cfgList, destDir )

    oConn0.pop( 'BindAddr', None )

    return ret

def Update_Rtauth( initCfg: dict, drvFile: list,
    bServer: bool, destDir : str ) -> int :

    ret = 0
    if not bServer :
        return 0

    lbCfgs = []
    if 'LBGroup' in initCfg :
        lbCfgs = initCfg[ 'LBGroup' ]

    authInfo = None
    taskSched = 'RR'
    maxConn = 512
    if 'Security' in initCfg :
        security = initCfg[ 'Security' ]
        if 'AuthInfo' in security :
            authInfo = deepcopy( security[ 'AuthInfo' ] )
            authInfo.pop( 'UserName', None ) 
            authInfo.pop( 'KdcIp', None )

        if 'misc' in security :
            misc = security[ 'misc' ]
            if 'MaxConnections' in misc :
                maxConns = int( misc[ 'MaxConnections' ] )
            if 'TaskScheduler' in misc :
                taskSched = misc[ 'TaskScheduler' ]

    rtDesc = drvFile[ 1 ]
    svrObjs = rtDesc[ 'Objects' ]
    mhNodes = None

    if 'Multihop' in initCfg :
        mhNodes = initCfg[ 'Multihop' ]

    if svrObjs is not None and len( svrObjs ) > 0 :
        for svrObj in svrObjs :
            objName = svrObj[ 'ObjectName']
            if objName == 'RpcRouterBridgeAuthImpl' :
                if authInfo is not None :
                    svrObj[ 'AuthInfo'] = authInfo
                else :
                    svrObj.pop( 'AuthInfo', None )

            if ( objName == 'RpcRouterBridgeAuthImpl' or
                 objName == 'RpcRouterBridgeImpl' ) :

                if lbCfgs is None or len( lbCfgs ) == 0:
                    svrObj.pop( 'LBGroup', None )
                else:
                    svrObj[ 'LBGroup' ] = lbCfgs

                if mhNodes is None or len( mhNodes ) == 0:
                    svrObj.pop( 'Nodes', None )
                elif 'Nodes' in svrObj :
                    nodes = svrObj[ 'Nodes' ]
                    if len( mhNodes ) < len( nodes ) :
                        nodes = nodes[ :len( mhNodes ) ]
                        svrObj[ 'Nodes' ] = nodes
                    elif len( mhNodes ) > len( nodes ) :
                        nodes += ( [ nodes[ 0 ] ] *
                            ( len( mhNodes ) - len( nodes ) ) )
                    for i in range( len( nodes ) ):
                        mhNode = mhNodes[ i ]
                        node = nodes[ i ]
                        for key, val in mhNode.items():
                            node[ key ] = val
                        if mhNode[ 'EnableWS' ] == "false" :
                            node.pop( 'DestURL', None )
                else :
                    svrObj[ 'Nodes'] = mhNodes

            elif objName == 'RpcRouterManagerImpl' :
                if not 'MaxPendingRequests' in svrObj:
                    svrObj[ 'MaxPendingRequests' ] = str( maxConns * 16  )
            elif ( objName == 'RpcReqForwarderAuthImpl' or
                objName == 'RpcReqForwarderImpl' ):
                svrObj[ 'TaskScheduler' ] = taskSched

    if destDir is None :
        rtauPath = drvFile[ 0 ]
    else :
        rtauPath = destDir + "/"
        rtauPath += os.path.basename( drvFile[ 0 ] )

    fp = open(rtauPath, "w")
    json.dump( rtDesc, fp, indent = 4 )
    fp.close()
    return ret

def DefaultIfCfg() -> dict :
    defCfg = dict()
    defCfg[ "AddrFormat" ]= "ipv4"
    defCfg[ "Protocol" ]= "native"
    defCfg[ "PortNumber" ]= "4132"
    defCfg[ "BindAddr"] = "127.0.0.1"
    defCfg[ "PdoClass"] = "TcpStreamPdo2"
    defCfg[ "Compression" ]= "true"
    defCfg[ "EnableWS" ]= "false"
    defCfg[ "EnableSSL" ]= "false"
    defCfg[ "ConnRecover" ]= "false"
    defCfg[ "HasAuth" ]= "false"
    defCfg[ "DestURL" ]= "https://www.example.com"
    return defCfg

def Update_Drv( initCfg: dict, drvFile : list,
    bServer : bool, destDir : str ) -> int :

    ret = 0
    drvVal = drvFile[ 1 ]

    if 'Ports' in drvVal :
        ports = drvVal['Ports']
    else :
        return -errno.ENOENT

    while 'Security' in initCfg :
        security = initCfg[ 'Security' ]
        if 'SSLCred' in security :
            sslCred = security[ 'SSLCred' ]
            if 'KeyFile' in sslCred :
                keyPath = sslCred[ 'KeyFile' ]

            if 'CertFile' in sslCred :
                certPath = sslCred[ 'CertFile' ]

            for port in ports :
                if port[ 'PortClass'] != 'RpcOpenSSLFido' :
                    continue
                sslFiles = port[ 'Parameters']
                if sslFiles is None :
                    continue
                sslFiles[ "KeyFile"] = keyPath
                sslFiles[ "CertFile"] = certPath
                break

        if 'misc' in security and bServer :
            misc = security[ 'misc' ]
            for port in ports :
                if port[ 'PortClass'] != 'RpcTcpBusPort' :
                    continue
                if 'MaxConnections' in misc :
                    port[ 'MaxConnections' ] = misc[ 'MaxConnections' ]
                else :
                    port[ 'MaxConnections' ] = '512'
                break
        break


    defCfg = DefaultIfCfg()

    defConn = dict()
    defConn[ "PortNumber" ]= "4132"
    defConn[ "IpAddress"] = "127.0.0.1" 
    defConn[ "Compression" ]= "true"
    defConn[ "EnableWS" ]= "false"
    defConn[ "EnableSSL" ]= "false"
    defConn[ "HasAuth" ]= "false"

    oConns = None
    if bServer and 'Connections' not in initCfg:
        oConns = [ defConn, ]

    while bServer and 'Connections' in initCfg:
        oConns = initCfg[ 'Connections' ]
        if len( oConns ) == 0 :
            oConns.append( defConn )

        for port in ports :
            if port[ 'PortClass'] != 'RpcTcpBusPort' :
                continue
            ifList = port[ 'Parameters' ]
            if len( ifList ) < len( oConns ):
                ifList += ( [ defCfg ] *
                    ( len(oConns ) - len( ifList ) ) )
            elif len( ifList ) > len( oConns ) :
                ifList = ifList[ : len(oConns )]

            for i in range( len( ifList ) ) :
                ifCfg = ifList[ i ]
                oConn = oConns[ i ]
                for key, val in oConn.items() :
                    if key == 'IpAddress' :
                        ifCfg[ 'BindAddr' ] = val
                    else :
                        ifCfg[ key ] = val
                if oConn[ 'EnableWS' ] == "false" :
                    ifCfg.pop( "DestURL", None )
            port[ 'Parameters'] = ifList
            break
        break

    if not bServer and 'Connections' in initCfg:
        oConns = initCfg[ 'Connections' ]

    bAuth = False
    bSSL = False
    if oConns is not None:
        for oConn in oConns:
            if 'HasAuth' in oConn :
                if oConn[ 'HasAuth' ] == 'true' :
                    bAuth = True
            if 'EnableSSL' in oConn:
                if oConn[ 'EnableSSL' ] == 'true' :
                    bSSL = True

    try:
        if 'Match' in drvVal :
            oMatches = drvVal[ 'Match' ]
            for oMatch in oMatches:
                if oMatch[ 'PortClass' ] != "TcpStreamPdo2":
                    continue
                oSeq = [ "RpcNatProtoFdoDrv", "RpcTcpFidoDrv" ]
                if bAuth :
                    oSeq.insert( 0, "RpcSecFidoDrv" )
                if bSSL :
                    oSeq.insert( 0, "RpcWebSockFidoDrv" )
                    oSeq.insert( 0, "RpcOpenSSLFidoDrv" )
                oMatch[ 'ProbeSequence' ] = oSeq
                break

        if 'Modules' in drvVal :
            oModules = drvVal[ 'Modules' ]
            for oModule in oModules :
                if oModule[ 'ModName' ] != 'rpcrouter':
                    continue
                oFactories = []
                if bAuth :
                    oFactories.append ( './libauth.so' )
                if bSSL:
                    oFactories.append( './libsslpt.so' )
                    oFactories.append( './libwspt.so' )
                oModule[ "ClassFactories" ] = oFactories
                break

    except Exception as err:
        pass

    if destDir is None:
        drvPath = drvFile[ 0 ]
    else:
        drvPath = destDir + "/driver.json"
    fp = open(drvPath, "w")
    json.dump( drvVal, fp, indent=4)
    fp.close()

    return ret

def Update_InitCfg(
    cfgPath : str, destDir : str ) -> int :
    ret = 0
    try:
        fp = open( cfgPath, "r" )
        initCfg = json.load( fp )
        fp.close()

        strVal = initCfg[ 'IsServer' ]
        if strVal == 'true' :
            bServer = True
        elif strVal == 'false' :
            bServer = False
        else:
            return -errno.EINVAL

        jsonFiles = LoadConfigFiles( None )
        if jsonFiles is None :
            return -errno.EFAULT

        ret = Update_Drv(
            initCfg, jsonFiles[ 0 ], bServer, destDir )
        if ret < 0 :
            return ret

        ret = Update_Rtauth(
            initCfg, jsonFiles[ 1 ], bServer, destDir )
        if ret < 0 :
            return ret

        ret = Update_Rtauth(
            initCfg, jsonFiles[ 2 ], bServer, destDir )
        if ret < 0 :
            return ret

        ret = Update_AuthPrxy(
            initCfg, jsonFiles[ 3 ], bServer, destDir )
        if ret < 0 :
            return ret

    except Exception as err:
        text = "Update_InitCfg failed:" + str( err )
        exc_type, exc_obj, exc_tb = sys.exc_info()
        fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
        second_text = "@" + fname + ":" + str(exc_tb.tb_lineno)
        print( text, second_text )
        ret = -errno.EFAULT

    return ret
