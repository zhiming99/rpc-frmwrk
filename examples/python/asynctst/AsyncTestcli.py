#Generated by RIDLC, make sure to backup before running RIDLC again
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
    def LongWaitNoParamCb( self,
        context : object, ret : int ):
        print( "LongWaitNoParamCb completed with status " +
            str(ret) )
        self.SetError(ret)    
        self.NotifyComplete()
    
class CAsyncTestProxy(
    CIAsyncTestcli,
    PyRpcProxy ) :
    def __init__( self, pIoMgr, strDesc, strObjName ) :
        PyRpcProxy.__init__( self,
            pIoMgr, strDesc, strObjName )
