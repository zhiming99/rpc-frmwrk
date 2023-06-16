#!/bin/python3
import os
import sys
import json
import base64
import io

import time
from rpcf.iolib import *
svcdir = str()

def BuildRespHdr( methodName : str, idx : object ) -> dict:
        resp = dict()
        # request header, the information can be found
        # in examples/hellwld.ridl
        # the interface name this request belongs to
        resp[ "Interface" ] = "ITestTypes"
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
def Echo( req : object )->object :
    resp = BuildRespHdr('Echo', req['RequestId'])
    AddParameter(resp, 'strResp', req['Parameters']['strText'])
    print( os.getpid(), "Echo ", req['Parameters']['strText'])
    return resp

def EchoByteArray( req : object )->object :
    try:
        resp = BuildRespHdr('EchoByteArray', req['RequestId'])
        res = req[ 'Parameters']['pBuf'].encode()
        bytearr = base64.b64decode(res)
        bufsize = len(bytearr)
        print(os.getpid(), "EchoByteArray ", bytearr[bufsize-128:bufsize])
        AddParameter(resp, 'pRespBuf', req['Parameters']['pBuf'])
        return resp
    except Exception as err:
        print( err )
        return None

def EchoArray( req : object )->object:
    resp = BuildRespHdr('EchoArray', req['RequestId'])
    res = req[ 'Parameters']['arrInts']
    print( os.getpid(), "EchoArray ", res)
    AddParameter(resp, 'arrIntsR', res)
    return resp

def EchoMap( req : object )->object:
    resp = BuildRespHdr('EchoMap', req['RequestId'])
    res = req[ 'Parameters']['mapReq']
    print( os.getpid(), "EchoMap ", res)
    AddParameter(resp, 'mapResp', res)
    return resp

def EchoStruct( req : object)->object:
    resp = BuildRespHdr('EchoStruct', req['RequestId'])
    res = req[ 'Parameters']['fi']
    print( os.getpid(), "EchoStruct ", res)
    AddParameter(resp, 'fir', res)
    return resp

def EchoNoParams( req : object)->object:
    resp = BuildRespHdr('EchoNoParams', req['RequestId'])
    print(os.getpid(), "EchoNoParams arrives")
    return resp

def EchoStream( req : object)->object:
    global svcdir
    resp = BuildRespHdr('EchoStream', req['RequestId'])
    res = req[ 'Parameters']['hstm']
    print( os.getpid(), "EchoStream ", res)
    AddParameter(resp, 'hstmr', res)
    #read content in the stream and echo back
    try:
        stmfp = open( svcdir + "/streams/" + res, "r+b", buffering=0 )
        inputs = [stmfp]
        inBuf = bytearray()
        size = 8 * 1024
        while size > 0 :
            notifylist = select.select( inputs, [], [] )
            data = stmfp.read(8*1024)
            inBuf.extend( data )
            size -= len( data )
        stmfp.write(inBuf)
        print( "stream@", res, inBuf[8160:8192] )
        stmfp.close()

    except Exception as err:
        print( "EchoStream ", err, res )

    return resp

def EchoMany( req : object )->object:
    resp = BuildRespHdr('EchoMany', req['RequestId'])
    params = req[ 'Parameters']
    AddParameter( resp, 'i1r', params['i1'] )
    AddParameter( resp, 'i2r', params['i2'] )
    AddParameter( resp, 'i3r', params['i3'] )
    AddParameter( resp, 'i4r', params['i4'] )
    AddParameter( resp, 'i5r', params['i5'] )
    AddParameter( resp, 'szTextr', params['szText'] )
    print( os.getpid(), "EchoMany ", resp)
    return resp

def test() :
    error = 0
    global svcdir
    reqfp = object()
    respfp = object()

    validReqs = {
        "Echo" : lambda req : Echo( req ),
        "EchoByteArray" : lambda req : EchoByteArray( req ),
        "EchoArray" : lambda req : EchoArray( req ),
        "EchoMap" : lambda req : EchoMap( req ),
        'EchoMany' : lambda req : EchoMany( req ),
        "EchoStruct" : lambda req : EchoStruct( req ),
        'EchoNoParams' : lambda req : EchoNoParams( req ),
        'EchoStream' : lambda req : EchoStream( req )
         }

    try:
        num = sys.argv[2]
        svcdir = sys.argv[1]
        reqFile = svcdir + "/jreq_" + num
        reqfp = open( reqFile, "rb" )

        respFile = svcdir + "/jrsp_" + num
        respfp = open( respFile, "wb")
        count = 0

        while True:
            #print( time.time() )
            #receive response if any
            ret = recvReq(reqfp)
            if ret[ 0 ] < 0 :
                raise Exception( os.getpid(),
                    "recvReq: unexpected error %d" % ret[ 0 ]  )

            reqs = ret[ 1 ]
            if reqs is None or len( reqs ) == 0:
                print( "the req is empty ", reqs )
                continue

            while len( reqs ) > 0 :
                req = reqs[0]
                reqs.pop(0)
                print( count )
                count += 1
                if req[ "Interface"] != "ITestTypes" :
                    print( "the req is invalid ", req )
                    continue

                if req[ 'Method' ] not in validReqs.keys():
                    print( "the req is invalid ", req )
                    continue

                try:
                    resp = validReqs[ req['Method']](req)
                    if resp is None :
                        print( "the req failed ", req )
                        continue

                    sendResp( respfp, resp )

                except Exception as err :
                    print( os.getpid(), "mainloop(mainsvr.py):", err )
                    continue

        reqfp.close()
        respfp.close()

    except Exception as err:
        print( os.getpid(), "test()(mainsvr.py):", err )
        print( "usage: mainsvr.py <service path> <req num>")
        print( "\t<service path> is /'path to mountpoint'/TestTypesSvc")
        print( "\t<req num> is the suffix of the req file under <service path>" )
        reqfp.close()
        respfp.close()

    finally :
        return error


def main() :
    test()

if __name__ == "__main__":
    main()
