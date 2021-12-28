#Generated by RIDLC, make sure to backup before running RIDLC again
from typing import Tuple
from rpcf.rpcbase import *
from rpcf.proxy import *
from seribase import CSerialBase
from kateststructs import *
import errno

from KeepAlivesvrbase import *
class CIKeepAlivesvr( IIKeepAlive_SvrImpl ):

    def LongWaitCb( self, oReqCtx : PyReqContext ) :
        #timer callback, complete the request
        context = oReqCtx.oContext
        strText = context[ 1 ]
        context = None
        self.OnLongWaitComplete(
            oReqCtx, 0, strText )
        print( "LongWait request completed " )
        return
    '''
    Asynchronous request handler
    '''
    def LongWait( self, oReqCtx : PyReqContext,
        i0 : str
        ) -> Tuple[ int, list ] :
        '''
        the response parameters includes
        i0r : str
        '''

        #let's complete the request in 295 seconds
        ret = self.AddTimer( 295,
            CIKeepAlivesvr.LongWaitCb,
            oReqCtx )

        if ret[ 0 ] < 0 :
            return [ ret[ 0 ],  ]

        # store timerObj to the oReqCtx so that
        # we can use it in the complete callback
        timerObj = ret[ 1 ]
        oReqCtx.oContext = [ timerObj, i0 ]

        # return pending, and the keep-alive will
        # be enabled automatically
        return [ 65537, ]
        
    '''
    This method is called when the async
    request is cancelled due to timeout
    or at client's request. Add cleanup
    code here.
    '''
    def OnLongWaitCanceled( self,
        oReqCtx : PyReqContext, iRet : int,
        i0 : str ):
        if oReqCtx.oContext is not None:
            timerObj = oReqCtx.oContext[ 0 ]
            ret = self.DisableTimer( timerObj )
            print( "timer removed " + str( ret ) )
            oReqCtx.oContext = None
            print( "LongWait request canceled with " + i0 )
        
    
class CKeepAliveServer(
    CIKeepAlivesvr,
    PyRpcServer ) :
    def __init__( self, pIoMgr, strDesc, strObjName ) :
        PyRpcServer.__init__( self,
            pIoMgr, strDesc, strObjName )
