import sys
import time
from rpcf import *

sys.path.insert(0, '../../')
from proxy import PyRpcContext, PyRpcProxy

class CEventClient:
    """Mandatory class member to define the
    interface name, which will be used to invoke
    the event handler
    """
    ifName = "CEventServer"
    """
    this is a event handler, it print the event
    string from the remote server. The first two
    parameter must be self and callback, the rest
    parameters depends what the server send.
    """
    def OnHelloWorld( self, callback, strSvrEvt ):
        print( strSvrEvt )
        # return a list, with the first element is
        # the error number
        return [ 0, ];

#aggregrate the interface class and the PyProxy
#class by CEchoProxy
class CEventProxy(CEventClient, PyRpcProxy):
    def __init__(self, pIoMgr, strDesc, strObjName) :
        super(CEventClient, self).__init__(
            pIoMgr, strDesc, strObjName )

def test_main() : 
    oContext = PyRpcContext();
    with oContext :
        print( "start to work here..." )
        oProxy = CEventProxy( oContext.pIoMgr,
            "../../../test/debug64/evtdesc.json",
            "CEventServer" );

        ret = oProxy.GetError() 
        if ret < 0 :
            return ret

        with oProxy :

            #wait if the server is not online
            while ( cpp.stateRecovery ==
                oProxy.oInst.GetState() ):
                time.sleep( 1 )

            #keep waiting for server event,
            #till disconnection occurs
            while ( cpp.stateConnected ==
                oProxy.oInst.GetState() ):
                time.sleep( 1 )

ret = test_main()
quit( ret )
