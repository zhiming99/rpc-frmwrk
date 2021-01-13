import sys
import time
import numpy as np
from rpcfrmwrk import *

sys.path.insert(0, '../../')
from proxy import PyRpcContext, PyRpcServer

#1. define the interface the CPauseResumeServer provides
class CPauseResumeServer:
    """Mandatory class member to define the
    interface name, which will be used to invoke
    the event handler if any
    """
    ifName = "CPauseResumeServer"

    '''the server side implementation of the
    interface method share the same parameter
    layout, the `self' object comes first, and
    then a `callback' object for async call, and
    then the parameters from the proxy. You can
    ignore the `callback' if the request is
    complete in the synchronous fashion.

    the return value is a two-element list, the
    first element is the error code, and the
    second is the argument list if the error code
    is STATUS_SUCCESS. If no argument to return,
    the second element should be an empty list.

    if the error code is STATUS_PENDING, which
    means the request will be completed in the
    future time. The parameter `callback' should
    be kept and pass to OnServiceComplete when the
    requests is truly completed, as shows by
    method `EchoCfg'
    '''
    def Echo(self, callback, text ):
        listResp = [ 0 ]
        listParams = [ text ]
        listResp.append( listParams )
        return listResp
 
    def EchoUnknown( self, callback, byteBuf ):
        listResp = [ 0 ]
        listParams = [ byteBuf ]
        listResp.append( listParams )
        return listResp

    #return sum of i1+i2
    def EchoMany( self,
        callback, i1, i2, i3, i4, i5, strText ) :
        listResp = [ 0 ]
        listParams = [
            np.int32( i1 + 1 ),
            np.int16( i2 + 1 ),
            np.int64( i3 + 1 ),
            np.float32( i4 + 1 ),
            np.float64( i5 + 1 ),
            strText + " 2" ]
        listResp.append( listParams )
        return listResp

    def LongWait( self, callback, strText ) :
        #schedule a timer to call EchoCfgCb in
        #2 seconds
        context = [ callback, strText ]
        ret = self.AddTimer( 20,
            CPauseResumeServer.LongWaitCb,
            context )
        if ret[ 0 ] < 0 :
            return [ ret[ 0 ],  ]

        timerObj = ret[ 1 ]

        # Make sure the timerObj be freed if the
        # request is canceled or completed by
        # other events
        self.InstallCompNotify( callback,
            CPauseResumeServer.LongWaitCleanup,
            timerObj )
        listResp = [ 65537, ]
        return listResp

    def LongWaitCleanup( self, ret, timerObj ) :
        self.DisableTimer( timerObj )
        print( "remove the timer",
            "at user's request" );
        return

    '''
    Method: LongWaitCb
    Description: the callback of `LongWait'
    request. It will be called only if the request
    succeeds. In this test case, the request will
    be canceled, so no chance to go to here.
    Return Value: none
    '''
    def LongWaitCb( self, context ) :
        callback = context[ 0 ]
        strText = context[ 1 ] + " 2"
        self.OnServiceComplete(
            callback, 0, strText );
        return

#2. aggregrate the interface class and the PyRpcProxy
# class by CEchoProxy to pickup the python-cpp
# interaction support

class CPrSvrObj( CPauseResumeServer, PyRpcServer ):
    def __init__( self, pIoMgr, strDesc, strObjName ) :
        super( CPauseResumeServer, self ).__init__(
            pIoMgr, strDesc, strObjName )

def test_main() : 
    ret = 0
    oContext = PyRpcContext( "PyRpcServer" );
    with oContext :
        print( "start to work here..." )
        oServer = CPrSvrObj( oContext.pIoMgr,
            "../../test/debug64/prdesc.json",
            "CPauseResumeServer" );

        ret = oServer.GetError() 
        if ret < 0 :
            print( "error start server..." )
            return ret

        with oServer :
            #keep waiting for server event,
            #till disconnection occurs
            while True :
                ret = oServer.oInst.GetState()
                if ( ret == cpp.stateConnected or
                    ret == cpp.statePaused or 
                    ret == cpp.stateRecovery ) :
                    time.sleep( 1 )
                    continue
                break

    return ret 

ret = test_main()
quit( ret )

