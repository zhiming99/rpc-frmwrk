#Generated by RIDLC, make sure to backup if there are important changes
from rpcf.rpcbase import *
from rpcf.proxy import PyRpcServices, ErrorCode
from seribase import CSerialBase
import errno

from rpcf.proxy import PyRpcProxy
from StreamSvcclibase import *
import threading as tr

class CIFileTransfercli( IIFileTransfer_CliImpl ):

    '''
    Asynchronous callback to receive the 
    request status, and reponse parameters
    if any. And add code here to process the
    request response
    '''
    def UploadFileCb( self,
        context : object, ret : int ):
    
        self.status = ret
        self.sem.release()
    
        
    '''
    Asynchronous callback to receive the 
    request status, and reponse parameters
    if any. And add code here to process the
    request response
    '''
    def DownloadFileCb( self,
        context : object, ret : int ):
    
        self.status = ret
        self.sem.release()
    
class CIChatcli( IIChat_CliImpl ):

    pass
    
class CStreamSvcProxy(
    CIFileTransfercli,
    CIChatcli,
    PyRpcProxy ) :
    def __init__( self, pIoMgr, strDesc, strObjName ) :
        PyRpcProxy.__init__( self,
            pIoMgr, strDesc, strObjName )
        self.sem = tr.Semaphore( 0 )
        self.status = 0
