#!/bin/python3
import os
import sys
import json
import getopt

def GetAuthMech( drvPath:str )->str:
    authMech = ""
    fp=open( drvPath, "r" )
    drvCfg = json.load( fp )
    fp.close()
    for port in drvCfg.get( "Ports", [] ):
        if port.get( "PortClass" ) == "RpcTcpBusPort":
            params = port.get("Parameters", [])
            for param in params:
                if param.get( "HasAuth" ) == "true":
                    authMech = param.get( "AuthMech" )
                    break
        if authMech != "":
            break

    return authMech

def GetAuthMechInitCfg( drvPath:str )->str:
    authMech = ""
    fp=open( drvPath, "r" )
    drvCfg = json.load( fp )
    fp.close()
    oSec = drvCfg.get( "Security", None )
    if oSec is None:
        return authMech
    oAuthInfo = oSec.get( "AuthInfo", None )
    if oAuthInfo is None:
        return authMech
    return oAuthInfo.get( "AuthMech", "" )

def usage():
    print( "Usage: python3 authmech.py [-hi] <driver.json|initcfg.json" )
    print( "\t-i: the file is an initcfg.json." )
    print( "\t\tOtherwise it is a driver.json" )
    print( "\t-h: print this help." )

if __name__ == "__main__":
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hi" )
    except getopt.GetoptError as err:
        # print help information and exit:
        print(err)  # will print something like "option -a not recognized"
        usage()
        sys.exit(-errno.EINVAL)

    bInitCfg = False
    output = None
    verbose = False
    for o, a in opts:
        if o == "-h" :
            usage()
            sys.exit( 0 )
        elif o == "-i":
            bInitCfg = True
        else:
            assert False, "unhandled option"

    if len( args ) == 0 :
        usage()
        sys.exit( -errno.EINVAL )

    drvPath = args[0]
    if bInitCfg :
        authMech = GetAuthMechInitCfg( drvPath )
    else:
        authMech = GetAuthMech( drvPath )
    if authMech == "":
        print("Error: authmech was not found", file=sys.stderr)
        sys.exit(1)
    print(authMech)
