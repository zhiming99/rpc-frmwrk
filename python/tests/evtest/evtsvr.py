import sys
import time
from rpcfrmwrk import *

sys.path.insert(0, '../../')
from proxy import PyRpcContext, PyRpcServer

class CEventServer:
    """Mandatory class member to define the
    interface name, which will be used to invoke
    the event handler
    """
    ifName = "CEventServer"
    """
    this is a event handler, it broadcast event
    message to the remote subscriber. 
    """
    def OnHelloWorld( self, strSvrEvt ):
        ret = self.SendEvent( None,
            CEventServer.ifName,
            "OnHelloWorld",
            "", strSvrEvt )


#aggregrate the interface class and the PyRpcServer
#class by CEventServer
class CEvtSvrObj(CEventServer, PyRpcServer):
    def __init__(self, pIoMgr, strDesc, strObjName) :
        super(CEventServer, self).__init__(
            pIoMgr, strDesc, strObjName )

def test_main() : 
    oContext = PyRpcContext();
    with oContext :
        print( "start to work here..." )
        oServer = CEvtSvrObj( oContext.pIoMgr,
            "../../test/debug64/evtdesc.json",
            "CEventServer" );

        ret = oServer.GetError() 
        if ret < 0 :
            return ret

        with oServer :

            #wait if the server is not online
            while ( cpp.stateRecovery ==
                oServer.oInst.GetState() ):
                time.sleep( 1 )

            #keep sending event every second,
            #till disconnection occurs
            i = 0;
            while ( cpp.stateConnected ==
                oServer.oInst.GetState() ):
                strMsg = "This is message {}"
                oServer.OnHelloWorld( 
                    strMsg.format( i ) )
                i+=1
                time.sleep( 1 )

ret = test_main()
quit( ret )

