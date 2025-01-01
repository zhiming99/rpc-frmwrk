# Generated by RIDLC, make sure to backup before recompile
# ridlc -spf -O . ../../../../examples/testypes.ridl 
import sys
from rpcf import iolib
from rpcf import serijson
from rpcf.serijson import Variant
import errno
from rpcf.proxy import ErrorCode as Err
from typing import Union, Tuple, Optional
from TestTypesstructs import *
from ifimpl import *
import os
import time
import threading
import select
import json
from TestTypesSvccli import CTestTypesSvcProxy

class CliEvtThread(threading.Thread):
    def __init__(self , threadName, oProxies ):
        super(CliEvtThread,self).__init__(name=threadName)
        self.m_oProxies = oProxies
        self.m_bExit = False
        self.m_oLock = threading.Lock()

    def IsExiting( self ) -> bool:
        self.m_oLock.acquire()
        bExit = self.m_bExit
        self.m_oLock.release()
        return bExit; 
    
    def SetExit( self ) -> bool:
        self.m_oLock.acquire()
        self.m_bExit = True
        self.m_oLock.release()
    
    def run(self):
        try:
            fps = []
            fMap = dict()
            oProxy = self.m_oProxies[ 0 ]
            fp = oProxy.getEvtFp()
            fps.append( fp )
            fMap[ fp.fileno() ] = 0
            
            while True:
                ioevents = select.select( fps, [], [], 1 )
                readable = ioevents[ 0 ]
                for s in readable:
                    ret = iolib.recvMsg( s )
                    if ret[ 0 ] < 0:
                        raise Exception( "Error read @%d" % s.fileno() )
                    for oMsg in ret[ 1 ] :
                        idx = fMap[ s.fileno() ]
                        self.m_oProxies[ idx ].DispatchMsg( oMsg )
                bExit = self.IsExiting()
                if bExit: 
                    break
        except Exception as err:
            print( err )
            return
        
def Usage() :
    print( "Usage: python3 maincli.py < SP1 Path > < SP2 Path >..." )
    print( "\t< SP1 path > is the path to the first service's service point. The order" )
    print( "\tof the < SP path > is the same as services declared in the ridl file" )
    
def maincli() :
    ret = 0
    error = 0
    oMsgThrd = None
    try:
        oProxies = []
        if len( sys.argv ) < 2 :
            Usage()
            return -errno.EINVAL
        num = 0
        if len( sys.argv ) >= 3 :
            num = int( sys.argv[ 2 ] )
        '''
        Note: this is a reference design
        you are encouraged to make changes
        for your own purpse
        '''
        
        print( "Start to work here..." )
        strSvcPt = sys.argv[ 1 ]
        oProxy = CTestTypesSvcProxy(
            strSvcPt, num )
        oProxies.append( oProxy )
        
        oMsgThrd = CliEvtThread( "TestTypescliThrd", oProxies )
        oMsgThrd.start()

        '''
        adding your code here
        '''
        print( "Start Echo...", os.getpid() )
        #make sure the reqid is not the same across
        #all the local clients of the same service point
        reqId = 0x12345 + num * 0x100000
        iRet = oProxy.Echo( reqId, "Hello, World!" )
        if iRet[ 0 ] < 0:
            error = iRet[ 0 ]
            raise Exception( "Echo failed with error %d" % error )
        oResp = iRet[ 1 ]
        print( "Echo completed with response", oResp, os.getpid() )

        print( "Start EchoMany...", os.getpid() )
        reqId += 1
        iRet = oProxy.EchoMany( reqId, 1, 2, 3, 4.0, 5.0, "hello, many!" )
        if iRet[ 0 ] < 0:
            error = iRet[ 0 ]
            raise Exception( "EchoMany failed with error %d" % error )
        oResp = iRet[ 1 ]
        print( "EchoMany completed with response", oResp, os.getpid() )

        print( "Start EchoNoParams...",
        os.getpid() )
        reqId += 1
        iRet = oProxy.EchoNoParams( reqId )
        if iRet[ 0 ] < 0:
            error = iRet[ 0 ]
            raise Exception( "EchoMany failed with error %d" % error )
        print( "EchoNoParams completed", os.getpid() )

        print( "Start EchoArray...", os.getpid() )
        reqId += 1
        arrInts = [ 1, 2, 3, 4 ]        
        iRet = oProxy.EchoArray( reqId, arrInts )
        if iRet[ 0 ] < 0:
            error = iRet[ 0 ]
            raise Exception( "EchoArray failed with error %d" % error )
        oResp = iRet[ 1 ]
        print( "EchoArray completed with response", oResp, os.getpid() )

        print( "Start EchoByteArray...",
        os.getpid() )
        reqId += 1
        binBuf = bytearray("Hello, World!".encode( "UTF-8" ) )
        binBuf.extend( reqId.to_bytes(4, "big") )
        binBuf.extend( bytearray( 8 * 1024 ) )
        pos = len(binBuf) - 2
        binBuf[ pos ] = 0x31
        binBuf[ pos + 1 ] = 0x32
        iRet = oProxy.EchoByteArray( reqId, binBuf )
        if iRet[ 0 ] < 0:
            error = iRet[ 0 ]
            raise Exception( "EchoByteArray failed with error %d" % error )
        oResp = iRet[ 1 ]
        binBufr = oResp[ 0 ]
        bufSize = len( binBufr )
        if bufSize < 128:
            print( "EchoByteArray completed with response", binBufr, os.getpid() )
        else:
            print( "EchoByteArray completed with response", binBufr[ bufSize - 128 : bufSize ], os.getpid())

        print( "Start EchoMap...", os.getpid() )
        reqId += 1
        mapReq = { 1 : "apple", 2 : "banana", 3 : "pear", 4 : "grape" }
        iRet = oProxy.EchoMap( reqId, mapReq )
        if iRet[ 0 ] < 0:
            error = iRet[ 0 ]
            raise Exception( "EchoMap failed with error %d" % error )
        oResp = iRet[ 1 ]
        print( "EchoMap completed with response", oResp, os.getpid() )

        print( "Start EchoStruct...", os.getpid() )
        reqId += 1
        fileInfo = FILE_INFO()
        fileInfo.bRead = True
        fileInfo.szFileName = "test.dat"
        fileInfo.fileSize = 0
        iRet = oProxy.EchoStruct( reqId, fileInfo )
        if iRet[ 0 ] < 0:
            error = iRet[ 0 ]
            raise Exception( "EchoStruct failed with error %d" % error )
        oResp = iRet[ 1 ]
        fir = oResp[ 0 ]
        firDict = fir.Serialize()[1]
        strDump = json.dumps(firDict)
        print( "EchoStruct completed with response %s" % strDump, os.getpid() )

        print( "Start EchoStream...", os.getpid() )
        reqId += 1
        hstmIn = "stream_" + str( num )
        stmFile = strSvcPt + "/streams/" + hstmIn
        
        #create the stream if not exist
        stmfp = open( stmFile, "w+b", buffering=0)

        #write 8kb block to the stream
        stmfp.write(binBuf[0:8*1024])

        #stream handle is the same as the file name of the stream
        iRet = oProxy.EchoStream( reqId, hstmIn )
        if iRet[ 0 ] < 0:
            error = iRet[0]
            raise Exception( "EchoStream failed with error %d" % error )
        oResp = iRet[1]
        #get the stream returned
        hstmr = oResp[0]
        if hstmr != hstmIn:
            error = -errno.EBADF
            raise Exception( "EchoStream returned different stream '%s'(returned):'%s'(origin)" %
                ( hstmr, hstmIn ), os.getpid() )

        #read the datablock echoed back
        inputs = [stmfp]
        notifylist = select.select( inputs, [], [], 10 )
        binBuf = stmfp.read(8*1024)
        bufsize = len( binBuf )
        print(binBuf[ bufsize - 128 : bufsize ])
        print( 'EchoStream completed with %s' % hstmr, os.getpid() )

    except Exception as err:
        print( err )
        if error < 0 :
            return error
        return -errno.EFAULT
    finally:
        if oMsgThrd is not None:
            oMsgThrd.SetExit()
            threading.Thread.join( oMsgThrd )
    return ret
    
ret = maincli()
quit( ret )
