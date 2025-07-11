#!/bin/python3
import os
import sys
import json
from pathlib import Path

def update_keys(json_path, ssl_type, priv_key, pub_cert, cacert):
    try:
        with open(json_path, 'r', encoding='utf-8') as f:
            data = json.load(f)

        ssl_enabled = False
        for port in data.get("Ports", []):
            if port.get("PortClass") == "RpcTcpBusPort":
                params = port.get("Parameters", [])
                for param in params:
                    if param.get( "EnableSSL" ) == "true":
                        ssl_enabled = True
                        break
                if ssl_enabled:
                    break

        # Determine which PortClass to update
        if ssl_type.lower() == "openssl":
            port_class = "RpcOpenSSLFido"
        elif ssl_type.lower() == "gmssl":
            port_class = "RpcGmSSLFido"
        else:
            print("Unknown ssl_type:", ssl_type)
            sys.exit(1)

        # Find the matching port entry
        found = False
        keyfile_path = [ priv_key, pub_cert  ]
        if cacert is not None:
             keyfile_path.append( cacert )

        for port in data.get("Ports", []):
            if port.get("PortClass") == port_class:
                params = port.get("Parameters", {})
                params["CertFile"] = keyfile_path[1]
                params["KeyFile"] = keyfile_path[0]
                if len( keyfile_path ) >= 3 :
                    if os.path.getsize( keyfile_path[2] ) > 0:
                        params["CACertFile"] = keyfile_path[2]
                else:
                    params["CACertFile"] = ""
                port["Parameters"] = params
                found = True

        if not found:
            print(f"PortClass {port_class} not found in Ports.")
            sys.exit(1)

        if not ssl_enabled:
            return 0

        for match in data.get("Match", []):
            if match.get("PortClass") == "TcpStreamPdo2":
                ps = match.get("ProbeSequence", [])
                if ssl_type.lower() == "openssl":
                    ssldrv ="RpcOpenSSLFidoDrv"
                else:
                    ssldrv ="RpcGmSSLFidoDrv"
                if ( ps[ 0 ] == "RpcOpenSSLFidoDrv" or
                    ps[ 0 ] == "RpcGmSSLFidoDrv" ):
                    ps[ 0 ] = ssldrv
                else:
                    ps.insert( 0, ssldrv )
                match[ "ProbeSequence" ] = ps
                if ssl_type.lower() == "openssl":
                    match[ "UsingGmSSL" ] = "false"
                else:
                    match[ "UsingGmSSL" ] = "true"
        with open(json_path, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=4)
            print(f"Updated keys successfully")
    except Exception as err:
        print( err )
        sys.exit(1)
    return 0

if __name__ == "__main__":
    if len(sys.argv) < 5:
        print("Usage: python updatekeys.py <path to driver.json> <openssl|gmssl> <priv_key> <pub_cert> [<ca cert>]")
        sys.exit(1)
    if len(sys.argv) == 5:
        update_keys(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[ 4 ], None)
    else:
        update_keys(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[ 4 ], sys.argv[ 5 ])
