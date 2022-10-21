import os
import sys
import json
import io

import time
from rpcf.iolib import *

def Echo() :
    error = 0
    svcdir = sys.argv[1]
    reqfp = object()
    respfp = object()
    idx = 123456
    try:
        reqFile = svcdir + "/jreq_0"
        reqfp = open( reqFile, "wb" )

        respFile = svcdir + "/jrsp_0"
        respfp = open( respFile, "rb")

        echoReq = dict()
        # request header, the information can be found
        # in examples/hellwld.ridl
        # the interface name this request belongs to
        echoReq[ "Interface" ] = "IHelloWorld"
        # the request type include "req" "resp" and
        # "evt". In this case, it is of type "req"
        echoReq[ "MessageType"] = "req"
        # the request name
        echoReq[ "Method"] = "Echo"
        # request id is a 64bit number to uniquely
        #  identify this request

        echoReq[ "RequestId"] = idx
        idx += 1
        echoParams = dict()
        echoParams[ "strText" ] = "Hello, World!"
        echoReq[ "Parameters"] = echoParams
        print( "the request is :\n", echoReq )
        # send request
        sendReq(reqfp, echoReq)
        #receive response
        respObj = recvResp(respfp)
        print( "the response is :\n", respObj )

        reqfp.close()
        respfp.close()
    except Exception as err:
        print( err )
        print( "usage: maincli.py <service path>")
        print( "\t<service path> is /'path to mountpoint'/HelloWorldSvc")
        eqfp.close()
        respfp.close()
    finally :
        return error


def main() :
    Echo()

if __name__ == "__main__":
    main()
