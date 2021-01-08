import sys
import time
import numpy as np
from rpcfrmwrk import *

sys.path.insert(0, '../')
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
    layout, a self object comes first, and then a
    callback for async call, and then the
    parameters from the proxy.
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

    def EchoCfg( self, callback, iCount, pObj ):
        listResp = [ 0 ]
        listParams = [ iCount, pObj ]
        listResp.append( listParams )
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
            "../../test/debug64/echodesc.json",
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
