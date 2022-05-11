import os
import sys
import json
import base64
import io

import time
from iolib import *
def BuildRespHdr( methodName : str, idx : int ) -> dict:
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
    print( "Echo ", req['Parameters']['strText'])
    return resp

def EchoByteArray( req : object )->object :
    try:
        resp = BuildRespHdr('EchoByteArray', req['RequestId'])
        res = req[ 'Parameters']['pBuf'].encode()
        bytearr = base64.b64decode(res)
        print(bytearr)
        AddParameter(resp, 'pRespBuf', req['Parameters']['pBuf'])
        return resp
    except Exception as err:
        print( err )
        return None

def EchoArray( req : object )->object:
    resp = BuildRespHdr('EchoArray', req['RequestId'])
    res = req[ 'Parameters']['arrInts']
    print(res)
    AddParameter(resp, 'arrIntsR', res)
    return resp

def EchoMap( req : object )->object:
    resp = BuildRespHdr('EchoMap', req['RequestId'])
    res = req[ 'Parameters']['mapReq']
    print(res)
    AddParameter(resp, 'mapResp', res)
    return resp

def EchoStruct( req : object)->object:
    resp = BuildRespHdr('EchoStruct', req['RequestId'])
    res = req[ 'Parameters']['fi']
    print(res)
    AddParameter(resp, 'fir', res)
    return resp

def EchoNoParams( req : object)->object:
    resp = BuildRespHdr('EchoNoParams', req['RequestId'])
    print("EchoNoParams arrives")
    return resp

def EchoStream( req : object)->object:
    resp = BuildRespHdr('EchoStream', req['RequestId'])
    res = req[ 'Parameters']['hstm']
    print("EchoStream ", res)
    AddParameter(resp, 'hstmr', res)
    return resp

def EchoMany( req : object )->object:
    resp = BuildRespHdr('EchoMany', req['RequestId'])
    params = req[ 'Parameters']
    AddParameter( resp, 'i1r', params['i1'] + 1)
    AddParameter( resp, 'i2r', params['i2'] + 2)
    AddParameter( resp, 'i3r', params['i3'] + 3)
    AddParameter( resp, 'i4r', params['i4'] + 4)
    AddParameter( resp, 'i5r', params['i5'] + 5)
    AddParameter( resp, 'szTextr', params['szText'] + 'll')
    print("EchoMany ", resp)
    return resp

def test() :
    error = 0
    mp = '/home/zhiming/mywork/github/rpc-frmwrk/ridl/ftest1/mpsvr'
    srcdir = mp + "/TestTypesSvc"
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
        num = sys.argv[1]
        reqFile = srcdir + "/jreq_" + num
        reqfp = open( reqFile, "rb" )

        respFile = srcdir + "/jrsp_" + num
        respfp = open( respFile, "wb")
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
                except Exception as err :
                    continue

                sendResp( respfp, resp )


        reqfp.close()
        respfp.close()
        evtfp.close()
        stmfp.close()

    except Exception as err:
        print( err )
        reqfp.close()
        respfp.close()
        evtfp.close()
        stmfp.close()

    finally :
        return error


def main() :
    test()

if __name__ == "__main__":
    main()
