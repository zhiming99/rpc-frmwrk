import os
import sys
import json
import io

import time
from rpcf.iolib import *

def BuildReqHdr( methodName : str, idx : int, ifName : str="IKeepAlive" ) -> dict:
        req = dict()
        # request header, the information can be found
        # in examples/hellwld.ridl.

        # the interface name this request belongs to
        req[ "Interface" ] = ifName
        # the request type include "req" "resp" and
        # "evt". In this case, it is of type "req"
        req[ "MessageType"] = "req"
        # the request name
        req[ "Method"] = methodName
        # request id is a 64bit number to uniquely
        # identify this request
        req[ "RequestId"] = idx
        req[ 'ReturnCode'] = 0
        return req

def LongWait() :
    error = 0

    reqfp = object()
    respfp = object()
    svcdir = sys.argv[1]
    idx = 123456
    try:
        reqFile = svcdir + "/jreq_0"
        reqfp = open( reqFile, "wb", buffering=0 )

        respFile = svcdir + "/jrsp_0"
        respfp = open( respFile, "rb", buffering=0)


        reqId = idx
        idx += 1
        echoReq = BuildReqHdr("LongWait", reqId)
        
        params = dict()
        params[ "i0" ] = "你好, 世界!"
        echoReq[ "Parameters"] = params
        print( "the request is :\n", echoReq )
        # send request
        sendReq(reqfp, echoReq)

        respObj = recvResp(respfp)
        error = respObj[0][ 'ReturnCode']

        if error < 0 :
            print( "the cancel request failed with error %d" % error  )
        else :
            print( "the request is completed with response %s" % respObj[0]['Parameters']['i0r'] )

        reqfp.close()
        respfp.close()
    except Exception as err:
        print( err )
        print( "usage: maincli.py <service path>")
        print( "\t<service path> is /'path to mountpoint'/connection_X/KeepAlive")
        reqfp.close()
        respfp.close()
    finally :
        return error


def main() :
    LongWait()

if __name__ == "__main__":
    main()
