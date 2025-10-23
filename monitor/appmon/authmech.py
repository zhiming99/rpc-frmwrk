#!/bin/python3
import os
import sys
import json

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

if __name__ == "__main__":
    if sys.argv.__len__() < 2:
        print("Error: missing <path to driver.json>", file=sys.stderr)
        print("Usage: python3 authmech.py <path to driver.json>", file=sys.stderr)
        sys.exit(1)
    drvPath = sys.argv[1]
    authMech = GetAuthMech( drvPath )
    if authMech == "":
        print("Error: authmech was not found", file=sys.stderr)
        sys.exit(1)
    print(authMech)