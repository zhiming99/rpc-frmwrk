# GENERATED BY RIDLC, MAKE SURE TO BACKUP BEFORE RUNNING RIDLC AGAIN
# Copyright (C) 2024  zhiming <woodhead99@gmail.com>
# This program can be distributed under the terms of the GNU GPLv3.
# ridlc -pO . ../../asynctst.ridl 
from typing import Tuple
from rpcf.rpcbase import *
from rpcf.proxy import *
from seribase import CSerialBase
from asynctststructs import *
import errno
import threading as tr

from AsyncTestclibase import *
class CIAsyncTestcli( IIAsyncTest_CliImpl ):

    sem = tr.Semaphore(0)
    def WaitForComplete( self ) :
        self.sem.acquire()
    
    def NotifyComplete( self ) :
        self.sem.release()

    '''
    Asynchronous callback to receive the 
    request status, and reponse parameters
    if any. And add code here to process the
    request response
    '''
    def LongWaitCb( self,
        context : object, ret : int, 
        i0r : str ) :
        if ret < 0:
            OutputMsg( "LongWait failed with error " +
                str(ret) )
        else:
            OutputMsg( "LongWait completed with response " +
                i0r )
        self.SetError(ret)    
        self.NotifyComplete()
        
    '''
    Asynchronous callback to receive the 
    request status, and reponse parameters
    if any. And add code here to process the
    request response
    '''
    def LongWaitNoParamCb( self,
        context : object, ret : int ):
        OutputMsg( "LongWaitNoParamCb completed with status " +
            str(ret) )
        self.SetError(ret)    
        self.NotifyComplete()
    
class CAsyncTestProxy(
    CIAsyncTestcli,
    PyRpcProxy ) :
    def __init__( self, pIoMgr, strDesc, strObjName ) :
        PyRpcProxy.__init__( self,
            pIoMgr, strDesc, strObjName )
