import sys
import time
import numpy as np

from rpcf.rpcbase import *

from rpcf.proxy import PyRpcContext, PyRpcServer, ErrorCode

class CKeepAliveServer:
    """Mandatory class member to define the
    interface name, which will be used to invoke
    the event handler
    """
    _ifName_ = "CKeepAliveServer"
    """
    Method: LongWait
    Description: this request is a long request,
    it will be completed in 30 seconds. For some
    reason we want to cancel it in 2 seconds after
    the request is sent, we can call CancelRequest
    to cancel this request.

    Return value: the return value is a list with
    1 elememnt, elem 0 is the error code, and
    if it is 0, 
    """
    def LongWait( self, callback, strText ) :
        #schedule a timer to call EchoCfgCb in
        #2 seconds
        context = [ callback, strText ]
        ret = self.AddTimer( 300,
            CKeepAliveServer.LongWaitCb,
            context )
        if ret[ 0 ] < 0 :
            return [ ret[ 0 ],  ]

        timerObj = ret[ 1 ]

        # Make sure the timerObj be freed if the
        # request is canceled
        self.InstallCompNotify( callback,
            CKeepAliveServer.LongWaitCleanup,
            timerObj )
        listResp = [ 65537, ]
        return listResp

    def LongWaitCleanup( self, ret, timerObj ) :
        if ret == ErrorCode.ERROR_USER_CANCEL :
            self.DisableTimer( timerObj )
            print( "remove the timer",
                "at user's request" )
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
        strText += " 2"
        self.OnServiceComplete(
            callback, 0, strText )
        return


#aggregrate the interface class and the PyRpcServer
class CKaSvrObj( CKeepAliveServer, PyRpcServer ):
    def __init__(self, pIoMgr, strDesc, strObjName) :
        super(CKeepAliveServer, self).__init__(
            pIoMgr, strDesc, strObjName )

def test_main() : 
    ret = 0
    oContext = PyRpcContext()
    with oContext :
        print( "start to work here..." )
        oServer = CKaSvrObj( oContext.pIoMgr,
            "../../../test/debug64/kadesc.json",
            "CKeepAliveServer" )

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

