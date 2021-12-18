#Generated by RIDLC, make sure to backup before running RIDLC again
from typing import Tuple
from rpcf.rpcbase import *
from rpcf.proxy import *
from seribase import CSerialBase
from ifteststructs import *
import errno

from IfTestsvrbase import *
class CIEchoThingssvr( IIEchoThings_SvrImpl ):

    '''
    Synchronous request handler
    '''
    def Echo( self, oReqCtx : PyReqContext,
        i0 : object
        ) -> Tuple[ int, list ] :
        '''
        the response parameters includes
        i0r : GlobalFeatureList
        '''
        #Implement this method here
        return [ ErrorCode.STATUS_SUCCESS, [i0] ]
        
    
class CIfTestServer(
    CIEchoThingssvr,
    PyRpcServer ) :
    def __init__( self, pIoMgr, strDesc, strObjName ) :
        PyRpcServer.__init__( self,
            pIoMgr, strDesc, strObjName )
