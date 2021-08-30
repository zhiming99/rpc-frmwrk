import sys
import time
import numpy as np

from rpcf.rpcbase import *

from rpcf.proxy import PyRpcContext, PyRpcServer, ErrorCode

class CActcServer:
    """Mandatory class member to define the
    interface name, which will be used to invoke
    the event handler
    """
    _ifName_ = "CActcServer"
    """
    Method: LongWait

    Description: this request is a long request,
    it will be completed in 30 seconds. For some
    reason we want to cancel it in 2 seconds after
    the request is sent, we can call CancelRequest
    to cancel this request.

    Return value: the return value is a tuple with
    two elememnts, 0 is the error code, and if it
    is 65537, element 1 is the task id, which can
    be passed to CancelRequest.
    """
    def LongWait( self, callback, strText ) :
        #schedule a timer to call EchoCfgCb in
        #2 seconds
        context = [ callback, strText ]
        ret = self.AddTimer( 30,
            CActcServer.LongWaitCb,
            context )
        if ret[ 0 ] < 0 :
            return [ ret[ 0 ],  ]

        timerObj = ret[ 1 ]

        # Make sure the timerObj be freed if the
        # request is canceled
        self.InstallCompNotify( callback,
            CActcServer.LongWaitCleanup,
            timerObj )
        listResp = [ 65537, ]
        return listResp

    def LongWaitCleanup( self, ret, timerObj ) :
        if ret == ErrorCode.ERROR_USER_CANCEL :
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
        strText = context[ 1 ]
        self.OnServiceComplete(
            callback, 0, strText );
        return


#aggregrate the interface class and the PyRpcServer
class CActcSvrObj(CActcServer, PyRpcServer):
    def __init__(self, pIoMgr, strDesc, strObjName) :
        super(CActcServer, self).__init__(
            pIoMgr, strDesc, strObjName )

def test_main() : 
    ret = 0
    oContext = PyRpcContext( "PyRpcServer" );
    with oContext :
        print( "start to work here..." )
        oServer = CActcSvrObj( oContext.pIoMgr,
            "../../../test/debug64/actcdesc.json",
            "CActcServer" );

        ret = oServer.GetError() 
        if ret < 0 :
            return ret

        ret = oServer.Start()
        if ret < 0 :
            return ret

        #wait if the server is not online
        while ( cpp.stateRecovery ==
            oServer.oInst.GetState() ):
            time.sleep( 1 )

        #wait till the server is offline
        while ( cpp.stateConnected ==
            oServer.oInst.GetState() ):
            time.sleep( 1 )

        oServer.Stop()

    return ret


ret = test_main()
quit( ret )
