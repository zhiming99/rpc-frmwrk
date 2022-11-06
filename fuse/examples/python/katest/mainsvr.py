#!/bin/python3
import os
import sys
import json
import base64
import io

import time
from rpcf.iolib import *
svcdir = str()

def BuildRespHdr( methodName : str, idx : int ) -> dict:
        resp = dict()
        # request header, the information can be found
        # in examples/hellwld.ridl
        # the interface name this request belongs to
        resp[ "Interface" ] = "IKeepAlive"
        # the request type include "req" "resp" and
        # "evt". In this case, it is of type "req"
        resp[ "MessageType"] = "resp"
        # the request name
        resp[ "Method"] = methodName
        # request id is a 64bit number to uniquely
        #  identify this request
        resp[ "RequestId"] = idx
        resp[ 'Parameters'] = dict()
        resp[ 'ReturnCode'] = 0
        return resp

def AddParameter( req : dict, paramName : str, val : object ) :
    req[ 'Parameters'][paramName] = val

#definitions of the request handlers
def LongWait( params : tuple )->object :
    req,respfp = params
    resp = BuildRespHdr('LongWait', req['RequestId'])
    AddParameter(resp, 'i0r', req['Parameters']['i0'])
    print( "LongWait ", req['Parameters']['i0'])

    #let's wait 2 minutes before return
    #build a keep-alive event to send, since normal request
    #cannot last 2 min before timeout
    kaevt = dict()
    kaevt[ "Interface" ] = "IInterfaceServer"
    kaevt[ "MessageType" ] = "evt"
    kaevt[ "Method" ] = "KeepAlive"
    kaevt[ "RequestId" ] = req[ 'RequestId' ]

    seconds = 0
    while seconds < 120 :
        time.sleep(3)
        seconds += 3
        #send keepAlive event to prevent timeout
        sendEvent(respfp, kaevt)

    return resp

def test() :
    error = 0
    global svcdir
    reqfp = object()
    respfp = object()

    validReqs = {
        "LongWait" : lambda req : LongWait( params )
         }

    try:

        svcdir = sys.argv[1]
        reqFile = svcdir + "/jreq_0"
        reqfp = open( reqFile, "rb", buffering=0 )

        respFile = svcdir + "/jrsp_0"
        respfp = open( respFile, "wb", buffering=0)
        count = 0

        while True:
            #print( time.time() )
            #receive response if any
            reqs = recvReq(reqfp)
            if reqs is None or len( reqs ) == 0:
                print( "the req is empty ", reqs )
                continue

            while len( reqs ) > 0 :
                req = reqs[0]
                reqs.pop(0)
                print( count )
                count += 1
                if req[ "Interface"] != "IKeepAlive" :
                    print( "the req is invalid ", req )
                    continue

                if req[ 'Method' ] not in validReqs.keys():
                    print( "the req is invalid ", req )
                    continue

                try:
                    params = (req, respfp)
                    resp = validReqs[ req['Method']](params)
                    if resp is None :
                        print( "the req failed ", req )
                        continue
                except Exception as err :
                    continue

                sendResp( respfp, resp )


        reqfp.close()
        respfp.close()

    except Exception as err:
        print( err )
        print( "usage: mainsvr.py <service path>")
        print( "\t<service path> is /'path to mountpoint'/KeepAlive")
        reqfp.close()
        respfp.close()

    finally :
        return error


def main() :
    test()

if __name__ == "__main__":
    main()
