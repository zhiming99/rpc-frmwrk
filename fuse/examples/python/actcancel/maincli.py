import os
import sys
import json
import io

import time
from iolib import *

def BuildReqHdr( methodName : str, idx : int, ifName : str="IActiveCancel" ) -> dict:
        req = dict()
        # request header, the information can be found
        # in examples/actcancel.ridl.

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

        for i in range( 10 ) :
            reqId = idx
            idx += 1
            echoReq = BuildReqHdr("LongWait", reqId)
            
            params = dict()
            params[ "i0" ] = "你好, 世界!"
            echoReq[ "Parameters"] = params
            print( "the request is :\n", echoReq )
            # send request
            sendReq(reqfp, echoReq)
            time.sleep(1)
            # try to cancel it via a built-in request
            # from interface 'IInterfaceServer'
            cancelReq = BuildReqHdr(
                'UserCancelRequest', idx,
                'IInterfaceServer')
            idx += 1
            params2 = dict()

            # specify the request to cancel
            params2['RequestId'] = reqId
            cancelReq[ 'Parameters'] = params2
            sendReq( reqfp, cancelReq)

            # there will be two responses to recv, one
            # is 'LongWait' and the other is
            # "UserCancelRequest". Usually the first is
            # the canceled task, and the second is
            # "UserCancelRequest" itself.  if the
            # 'UserCancelRequest' fails, the another
            # response may not happen
            i = 0
            while i < 2 :
                respList = recvResp(respfp)
                for respObj in respList:
                    i+=1
                    respObj = respList[0]
                    respId = respObj[ 'RequestId']
                    error = respObj[ 'ReturnCode']
                    if respId == idx - 1:
                        # the 'UserCancelRequest' request
                        if error < 0 :
                            print( "the cancel request failed with error %d" % error  )
                        else :
                            print( "the request is canceled successfully" )
                    elif respId == reqId :
                        print( "LongWait returned error code %d" % error )

        reqfp.close()
        respfp.close()
    except Exception as err:
        print( err )
        print( "usage: maincli.py <service path>")
        print( "\t<service path> is /'path to mountpoint'/connection_X/ActiveCancel")
        reqfp.close()
        respfp.close()
    finally :
        return error


def main() :
    Echo()

if __name__ == "__main__":
    main()
