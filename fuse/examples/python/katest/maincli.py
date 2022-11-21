# Generated by RIDLC, make sure to backup before recompile
# ridlc -spf --async_proxy -O . ../../../../examples/katest.ridl 
import sys
from rpcf import iolib
from rpcf import serijson
import errno
from rpcf.proxy import ErrorCode as Err
from typing import Union, Tuple, Optional
from kateststructs import *
from ifimpl import *
import os
import time
import threading
import select
from KeepAlivecli import CKeepAliveProxy

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
            
            fp = oProxy.getRespFp()
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
        oProxy = CKeepAliveProxy(
            strSvcPt, num )
        oProxies.append( oProxy )
        
        oMsgThrd = CliEvtThread( "katestcliThrd", oProxies )
        oMsgThrd.start()
        
        '''
        adding your code here
        '''
        #make sure the reqId is unique to the given service point
        reqId = 0x12345 + num * 0x100000
        iRet = oProxy.LongWait( reqId, "Hello, KeepAlive!" )
        if iRet[ 0 ] < 0 :
            error = iRet[ 0 ]
            raise Exception( "LongWait failed with error %d" % error )

        if iRet[ 0 ] == 0:
            return 0

        oProxy.m_semWait.acquire()
        
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
quit( -ret )
