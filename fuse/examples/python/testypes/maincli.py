#!/bin/python3
import os
import sys
import json
import base64
import io
import errno

import time
from rpcf.iolib import *
from rpcf.proxy import ErrorCode as Err

def BuildReqHdr( methodName : str, idx : int ) -> dict:
        echoReq = dict()
        # request header, the information can be found
        # in examples/hellwld.ridl
        # the interface name this request belongs to
        echoReq[ "Interface" ] = "ITestTypes"
        # the request type include "req" "resp" and
        # "evt". In this case, it is of type "req"
        echoReq[ "MessageType"] = "req"
        # the request name
        echoReq[ "Method"] = methodName
        # request id is a 64bit number to uniquely
        #  identify this request
        echoReq[ "RequestId"] = idx
        echoReq[ 'Parameters'] = dict()
        return echoReq

def AddParameter( req : dict, paramName : str, val : object ) :
    req[ 'Parameters'][paramName] = val

def test() :
    error = 0
    reqfp = object()
    respfp = object()

    try:
        svcdir = sys.argv[1]
        num = sys.argv[2]

        reqFile = svcdir + "/jreq_" + num
        reqfp = open( reqFile, "wb", buffering=0 )

        respFile = svcdir + "/jrsp_" + num
        respfp = open( respFile, "rb", buffering=0)

        evtFile = svcdir + "/jevt_" + num
        evtfp = open( evtFile, "rb", buffering=0)

        stmFile = svcdir + "/streams/stream_" + num

        print( "openStream %s" % num )
        stmfp = open( stmFile, "w+b", buffering=0)

        idx = 1 + int( num ) * 1000
        while True:
            print( time.time() )
            #receive response if any
            ret = peekResp(evtfp)
            if ret[ 0 ] == 0:
                evtObj = ret[ 1 ]
                print( "the event is :\n", evtObj )

            #EchoMany
            req = BuildReqHdr( "EchoMany", idx )
            idx += 1
            AddParameter(req, "i1", 1 )
            AddParameter(req, "i2", 2 )
            AddParameter(req, "i3", 3 )
            AddParameter(req, "i4", 4.0 )
            AddParameter(req, "i5", 5.0 )
            AddParameter(req, "szText", 'Hello, you' )

            sendReq( reqfp, req )
            ret = recvResp( respfp )
            if ret[ 0 ] < 0 :
                error = ret[ 0 ]
                raise Exception( 'EchoMany recv failed with error %d@%s' % ( error, num) )
            objResp = ret[ 1 ][ 0 ]
            error = objResp[ "ReturnCode"]
            if error < 0 :
                raise Exception( 'EchoMany failed with error %d@%s' % ( error, num) )
            i1r = objResp['Parameters']["i1r"]
            i2r = objResp['Parameters']["i2r"]
            i3r = objResp['Parameters']["i3r"]
            i4r = objResp['Parameters']["i4r"]
            i5r = objResp['Parameters']["i5r"]
            szTextr = objResp['Parameters']["szTextr"]
            print( "EchoMany response ", i1r, i2r, i3r, i4r, i5r, szTextr )

            # Echo
            req = BuildReqHdr( "Echo", idx )
            idx += 1
            strText = "Hello, World!"
            AddParameter( req, "strText", strText)
            sendReq( reqfp, req )
            ret = recvResp( respfp )
            if ret[ 0 ] < 0 :
                error = ret[ 0 ]
                raise Exception( 'Echo recv failed with error %d@%s' % ( error, num) )
            objResp = ret[ 1 ][ 0 ]
            error = objResp[ "ReturnCode"]
            if error < 0 :
                raise Exception( 'Echo failed with error %d@%s' % ( error, num) )
            print( "Echo completed with response '%s'" %
                objResp['Parameters']['strResp'])

            # EchoByteArray
            req = BuildReqHdr( "EchoByteArray", idx )
            idx += 1
            binBuf = bytearray("Hello, World!".encode( "UTF-8" ) )
            binBuf.extend( idx.to_bytes(4, "big") )
            binBuf.extend( bytearray( 8 * 1024 ) )
            pos = len(binBuf) - 2
            binBuf[ pos ] = 0x31
            binBuf[ pos + 1 ] = 0x32

            # bytearray must be base64 encoded before adding
            # to json
            res = base64.b64encode(binBuf)
            
            AddParameter( req, "pBuf", res.decode())
            sendReq( reqfp, req )
            ret = recvResp( respfp )
            if ret[ 0 ] < 0 :
                error = ret[ 0 ]
                raise Exception( 'EchoByteArray recv failed with error %d@%s' % ( error, num) )
            objResp = ret[ 1 ][ 0 ]
            error = objResp[ "ReturnCode"]
            if error < 0 :
                raise Exception( 'EchoByteArray failed with error %d@%s' % ( error, num) )
            strBufr = objResp[ "Parameters"]["pRespBuf"]
            res = strBufr.encode()
            binBufr = base64.b64decode(res)
            bufsize = len( binBuf )
            print( "EchoByteArray ", binBuf[bufsize - 128:bufsize] )

            # EchoArray
            req = BuildReqHdr( "EchoArray", idx )
            idx += 1
            arrInts = [ 1, 2, 3, 4 ]
            AddParameter(req, "arrInts", arrInts )
            sendReq( reqfp, req )
            ret = recvResp( respfp )
            if ret[ 0 ] < 0 :
                error = ret[ 0 ]
                raise Exception( 'EchoArray recv failed with error %d@%s' % ( error, num) )
            objResp = ret[ 1 ][ 0 ]
            error = objResp[ "ReturnCode"]
            if error < 0 :
                raise Exception( 'EchoArray failed with error %d@%s' % ( error, num) )
            
            arrIntsR = objResp['Parameters']["arrIntsR"]
            print( arrIntsR )

            #EchoMap
            req = BuildReqHdr( "EchoMap", idx )
            idx += 1
            mapReq = { 1 : "apple", 2 : "banana", 3 : "pear", 4 : "grape" }
            AddParameter(req, "mapReq", mapReq )
            sendReq( reqfp, req )
            ret = recvResp( respfp )
            if ret[ 0 ] < 0 :
                error = ret[ 0 ]
                raise Exception( 'EchoMap recv failed with error %d@%s' % ( error, num) )
            objResp = ret[ 1 ][ 0 ]
            error = objResp[ "ReturnCode"]
            if error < 0 :
                raise Exception( 'EchoMap failed with error %d@%s' % ( error, num) )
            mapResp = objResp['Parameters']["mapResp"]
            print( mapResp )

            #EchoStruct
            req = BuildReqHdr( "EchoStruct", idx )
            idx += 1
            fileInfo = dict()
            # look up the StructId from TestTypes.h generated by ridlc
            fileInfo[ "StructId"] = 0x62DC83DD
            fields = dict()
            fields[ "szFileName"] = "test.dat"
            fields[ "fileSize"] = 0
            fields[ "bRead"] = True
            fileInfo[ "Fields" ] = fields
            AddParameter(req, "fi", fileInfo )
            sendReq( reqfp, req )
            ret = recvResp( respfp )
            if ret[ 0 ] < 0 :
                error = ret[ 0 ]
                raise Exception( 'EchoStruct recv failed with error %d@%s' % ( error, num) )
            objResp = ret[ 1 ][ 0 ]
            error = objResp[ "ReturnCode"]
            if error < 0 :
                raise Exception( 'EchoStruct failed with error %d@%s' % ( error, num) )
            mapResp = objResp['Parameters']["fir"]
            print( mapResp )
            
            #EchoNoParams
            req = BuildReqHdr( "EchoNoParams", idx )
            idx += 1
            sendReq( reqfp, req )
            ret = recvResp( respfp )
            if ret[ 0 ] < 0 :
                error = ret[ 0 ]
                raise Exception( 'EchoNoParams recv failed with error %d@%s' % ( error, num) )
            objResp = ret[ 1 ][ 0 ]
            error = objResp[ "ReturnCode"]
            print( 'EchoNoParams returned with status %d@%s' % ( error, num) )

            #EchoStream
            req = BuildReqHdr( "EchoStream", idx )
            idx += 1
            AddParameter(req, "hstm", "stream_" + num )
            sendReq( reqfp, req )
            stmfp.write(binBuf[0:8*1024])
            ret = recvResp( respfp )
            if ret[ 0 ] < 0 :
                error = ret[ 0 ]
                raise Exception( 'EchoStream recv failed with error %d@%s' % ( error, num) )
            objResp = ret[ 1 ][ 0 ]
            error = objResp[ "ReturnCode"]
            if error < 0 :
                raise Exception( 'EchoStream %s failed with error %d' % ( num, error ) )
            else :
                inputs = [stmfp]
                notifylist = select.select( inputs, [], [] )
                binBuf = stmfp.read(8*1024)
            hstmr = objResp[ 'Parameters']["hstmr"]
            bufsize = len( binBuf )
            print(binBuf[ bufsize - 128 : bufsize ])
            print( 'EchoStream %s completed with %s' % ( num, hstmr ))
            break

        reqfp.close()
        respfp.close()
        evtfp.close()
        stmfp.close()
        
    except Exception as err:
        print( "error is", err )
        print( "usage: maincli.py <service path> <req num>")
        print( "\t<service path> is /'path to mountpoint'/connection_X/TestTypesSvc")
        print( "\t<req num> is the suffix of the req file under <service path>" )
        if error == 0 :
            error = -errno.EFAULT
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
