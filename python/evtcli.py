from rpcfrmwrk import *
import time
from proxy import PyRpcContext, PyRpcProxy

class CEventClient:
    """
    this is a event handler, it print the event
    string from the remote server
    """
    def OnHelloWorld( self, strSvrEvt ):
        print( strSvrEvt )

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
            "../test/debug64/evtdesc.json",
            "CEventServer" );

        ret = oProxy.GetError() 
        if ret < 0 :
            return ret

        with oProxy :
            while True :
                state = oProxy.oInst.GetState();
                if state == cpp.stateConnected or state == cpp.stateRecovery :
                    time.sleep( 1 )
                    continue

                break;

ret = test_main()
quit( ret )
