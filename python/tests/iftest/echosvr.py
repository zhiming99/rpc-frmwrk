import sys
import time
import numpy as np
from rpcfrmwrk import *

sys.path.insert(0, '../../')
from proxy import PyRpcContext, PyRpcServer

#1. define the interface the CEchoServer provides
class CEchoServer:
    """Mandatory class member to define the
    interface name, which will be used to invoke
    the event handler if any
    """
    ifName = "CEchoServer"

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
    def Echo2( self, callback, i1, i2 ):
        listResp = [ 0 ]
        result = np.int32( i1 ) + np.float64( i2 )
        listParams = [  result ]
        listResp.append( listParams )
        return listResp

    def EchoCfgCb( self, context ) :
        self.OnServiceComplete( context[ 0 ],
            0, *context[ 1: ] )

    ''' this method demonstrates a request
    completed asynchronously'''
    def EchoCfg( self, callback, iCount, pObj ):
        #schedule a timer to call EchoCfgCb in
        #2 seconds
        context = [ callback, iCount, pObj ]
        ret = self.oInst.AddTimer(
            np.int32( 2 ),
            CEchoServer.EchoCfgCb,
            context )
        if ret[ 0 ] < 0 :
            return [ ret[ 0 ],  ]
        listResp = [ 65537, ]
        return listResp

#2. aggregrate the interface class and the PyRpcProxy
# class by CEchoProxy to pickup the python-cpp
# interaction support

class CEchoSvrObj( CEchoServer, PyRpcServer ):
    def __init__( self, pIoMgr, strDesc, strObjName ) :
        super( CEchoServer, self ).__init__(
            pIoMgr, strDesc, strObjName )

def test_main() : 
    ret = 0
    oContext = PyRpcContext( "PyRpcServer" );
    with oContext :
        print( "start to work here..." )
        oServer = CEchoSvrObj( oContext.pIoMgr,
            "../../../test/debug64/echodesc.json",
            "CEchoServer" );

        ret = oServer.GetError() 
        if ret < 0 :
            return ret

        with oServer :
            #keep waiting for server event,
            #till disconnection occurs
            while ( cpp.stateConnected ==
                oServer.oInst.GetState() ):
                time.sleep( 1 )
    return ret 

ret = test_main()
quit( ret )
