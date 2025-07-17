#!/bin/python3
#*
#* =====================================================================================
#*
#*       Filename:  geninitcfg.py
#*
#*    Description:  implementation of command line tool to generate initcfg file from
#*                  driver.json
#*
#*        Version:  1.0
#*        Created:  07/13/2025 07:10:00 AM
#*       Revision:  none
#*       Compiler:  gcc
#*
#*         Author:  Ming Zhi( woodhead99@gmail.com )
#*   Organization:
#*
#*      Copyright:  2021 Ming Zhi( woodhead99@gmail.com )
#*
#*        License:  Licensed under GPL-3.0. You may not use this file except in
#*                  compliance with the License. You may find a copy of the
#*                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
#*
#* =====================================================================================
#*

import json
import sys
from pathlib import Path 
import os
import ipaddress

def GetIpVersion(ipAddress:str)->str:
    try:
        ip_obj = ipaddress.ip_address(ipAddress)
        if isinstance(ip_obj, ipaddress.IPv4Address):
            return "ipv4"
        elif isinstance(ip_obj, ipaddress.IPv6Address):
            return "ipv6"
    except ValueError:
        return ""

def GenInitCfgFromDrv( cfgList : list )->object:
    jsonVal = dict()
    connList = []
    jsonVal[ "Connections" ] = connList
    bAuth = False
    bSSL = False
    authMech = None
    drvCfg = cfgList[0]
    maxConns = 0
    bWS = False
    for port in drvCfg.get( "Ports", [] ):
        if port.get( "PortClass" ) == "RpcTcpBusPort":
            params = port.get("Parameters", [])
            connElem = dict()
            for param in params:
                ipAddr = param.get( "BindAddr" )
                connElem[ "IpAddress" ] = ipAddr
                connElem[ "BindTo" ] = param[ "BindTo" ]
                connElem[ "PortNumber" ] = param[ "PortNumber" ]
                connElem[ "Compression" ] = param[ "Compression" ]
                connElem[ "EnableSSL" ] = param[ "EnableSSL" ]
                if connElem[ "EnableSSL" ] == "true":
                    bSSL = True
                connElem[ "HasAuth" ] = param[ "HasAuth" ]
                if connElem[ "HasAuth" ] == "true":
                    connElem["AuthMech"] = param[ "AuthMech" ]
                    authMech = param[ "AuthMech" ]
                    bAuth = True
                connElem[ "EnableWS" ] = param[ "EnableWS" ]
                if connElem[ "EnableWS" ] == "true":
                    connElem[ "DestURL" ] = param[ "DestURL" ]
                    bWS = True
                connElem[ "RouterPath" ] = param[ "RouterPath" ]
                connList.append( connElem )
            maxConns = port.get( "MaxConnections", "" )

    if bSSL:
        for match in drvCfg.get( "Match", [] ):
            if match.get( "PortClass" ) == "TcpStreamPdo2":
                if "UsingGmSSL" in match and match.get( "UsingGmSSL" ) == "true":
                    bUsingGmSSL = True
                else:
                    bUsingGmSSL = False

    elemSecs = dict()
    jsonVal[ "Security" ] = elemSecs
    sslFiles = dict()
    elemSecs[ "SSLCred" ] = sslFiles

    if bSSL:
        portClass = "RpcOpenSSLFido" if not bUsingGmSSL else "RpcGmSSLFido"
        for port in drvCfg.get( "Ports", [] ):
            if port.get( "PortClass" ) == portClass:
                params = port.get("Parameters", {})
                sslFiles[ "CertFile" ] = params.get( "CertFile", "" )
                sslFiles[ "KeyFile" ] = params.get( "KeyFile", "" )
                sslFiles[ "CACertFile" ] = params.get( "CACertFile", "" )
                sslFiles[ "SecretFile" ] = params.get( "SecretFile", "" )
                if bUsingGmSSL:
                    sslFiles[ "UsingGmSSL" ] = "true"
                else:
                    sslFiles[ "UsingGmSSL" ] = "false"
                verifyPeer = port.get( "VerifyPeer", "" )
                if verifyPeer == "true":
                    sslFiles[ "VerifyPeer" ] = "true"
                else:
                    sslFiles[ "VerifyPeer" ] = "false"
                break

    authInfo = dict()
    if bAuth:
        echodesc = cfgList[ 1 ]
        for elem in echodesc.get( "Objects", [] ):
            if elem.get( "ObjectName" ) == "CEchoServer":
                authInfo[ "AuthMech" ] = authMech
                ai = elem.get( "AuthInfo", {} )
                if authMech == "krb5":
                    authInfo[ "Realm" ] = ai[ "Realm" ]
                    authInfo[ "ServceName" ] = ai[ "ServceName" ]
                    authInfo[ "SignMessage" ] = ai[ "SignMessage" ]
                    authInfo[ "UserName" ] = ai[ "UserName" ]
                elif authMech == "SimpAuth":
                    if "UserName" in ai:
                        authInfo[ "UserName" ] = ai[ "UserName" ]
                elif authMech == "OAuth2":
                    if "AuthUrl" in ai:
                        authInfo[ "AuthUrl" ] = ai[ "AuthUrl" ]
                break

        if authMech == "krb5":
            authPrxy = cfgList[ 3 ]
            for elem in authPrxy.get( "Objects", [] ):
                if elem.get( "ObjectName" ) == "KdcRelayServer":
                    authInfo[ "KdcIp" ] = elem[ "IpAddress" ]

        if authMech == "OAuth2":
            oa2check = cfgList[ 4 ]
            for elem in oa2check.get( "Objects", [] ):
                if elem.get( "ObjectName" ) == "OA2Proxy":
                    authInfo[ "OA2ChkIp" ] = elem[ "IpAddress" ]
                    authInfo[ "OA2ChkPort" ] = elem[ "PortNumber" ]
                    authInfo[ "OA2SSL" ] = elem[ "EnableSSL" ]
    elemSecs[ "AuthInfo" ] = authInfo

    oMisc = dict()
    oMisc[ "MaxConnections" ] = maxConns
    if bWS:
        oMisc[ "ConfigWebServer" ] = "true"

    if authMech == "krb5":
        oMisc[ "ConfigKrb5" ] = "true"
    oMisc[ "KinitProxy" ] = "false"

    elemSecs[ "misc" ] = oMisc

    rtauth = cfgList[ 2 ]
    count = 0
    for elem in rtauth.get( "Objects", [] ):
        objName = elem.get( "ObjectName" )
        if elem.get( "ObjectName" ) == "RpcRouterBridgeAuthImpl":
            nodeList = []
            jsonVal[ "Multihop" ] = nodeList
            for node in elem.get( "Nodes", [] ):
                n = dict() 
                n[ "NodeName" ] = node[ "NodeName" ]
                n[ "IpAddress" ] = node[ "IpAddress" ]
                n[ "Enabled" ] = node[ "Enabled" ]
                n[ "Compression" ] = node[ "Compression" ]
                n[ "EnableSSL" ] = node[ "EnableSSL" ]
                n[ "EnableWS" ] = node[ "EnableWS" ]
                if n[ "EnableWS" ] == "true":
                    n[ "DestURL" ] = node[ "DestURL" ]
                n[ "Protocol" ] = "native"
                n[ "ConnRecover" ] = "false"
                n[ "AddrFormat" ] = GetIpVersion( n[ "IpAddress" ] )
                nodeList.append( n )
            jsonVal[ "LBGroup" ] = elem.get( "LBGroup", [] )
            count += 1
        elif ( objName == "RpcReqForwarderAuthImpl" or
            objName == "RpcReqForwarderImpl" ):
            oMisc[ 'TaskScheduler' ] = elem[ 'TaskScheduler' ]
            count += 1
        if count >= 2:
            break

    jsonVal[ 'IsServer' ] = "true"
    return jsonVal

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: geninitcfg.py <driver.json>")
        sys.exit(1)

    json_path = sys.argv[1]
    if not Path(json_path).is_file():
        print(f"File {json_path} does not exist.")
        sys.exit(1)

    try:
        cfgs = []
        with open(json_path, 'r', encoding='utf-8') as f:
            cfgs.append(json.load(f))

        echoDesc=os.path.dirname(json_path) + "/echodesc.json"
        with open(echoDesc, 'r', encoding='utf-8') as f:
            cfgs.append(json.load(f))

        rtauth=os.path.dirname(json_path) + "/rtauth.json"
        with open(rtauth, 'r', encoding='utf-8') as f:
            cfgs.append(json.load(f))

        authprxy=os.path.dirname(json_path) + "/authprxy.json"
        with open(authprxy, 'r', encoding='utf-8') as f:
            cfgs.append(json.load(f))

        oa2check =os.path.dirname(json_path) + "/oa2checkdesc.json"
        with open(authprxy, 'r', encoding='utf-8') as f:
            cfgs.append(json.load(f))

        initcfg = GenInitCfgFromDrv( cfgs )

        print(json.dumps(initcfg, indent=4))

    except Exception as e:
        print(f"Error processing file: {e}")
        sys.exit(1)
