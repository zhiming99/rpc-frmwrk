# Generated by RIDLC, make sure to backup before recompile
# ridlc -spf --async_proxy -O . ../../../../examples/actcancel.ridl 
import sys
from rpcf import iolib
from rpcf import serijson
import errno
from rpcf.proxy import ErrorCode as Err
from typing import Union, Tuple, Optional
from actcancelstructs import *
from ifimpl import *
import os
import time
import threading
import select
import random
from ActiveCancelcli import CActiveCancelProxy

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
        oProxy = CActiveCancelProxy(
            strSvcPt, num )
        oProxies.append( oProxy )
        
        oMsgThrd = CliEvtThread( "actcancelcliThrd", oProxies )
        oMsgThrd.start()
        
        '''
        adding your code here
        using 'ActiveCancel' as an example
        Calling a proxy method like

        '''
        reqId = random.randint( 0, 0x100000000 )
        iRet = oProxy.LongWait( reqId, "Hello, ActiveCancel!" )
        if iRet[ 0 ] < 0:
            error = iRet[ 0 ]
            raise Exception( "LongWait failed with error %d" % error )
        if iRet[ 0 ] == Err.STATUS_PENDING:
            time.sleep( 3 )
            iRet = oProxy.CancelRequest( reqId )
            if iRet == Err.STATUS_PENDING:
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
