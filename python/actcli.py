from rpcfrmwrk import *
import time
from proxy import PyRpcContext, PyRpcProxy
import numpy as np

class CActcClient:
    """Mandatory class member to define the
    interface name, which will be used to invoke
    the event handler
    """
    ifName = "CActcServer"
    """
    this is the only method the server support
    """
    def LongWait( self, strText ) :
        tupRet = self.sendRequestAsync(
            CActcClient.LongWaitCb,
            self.ifName, "LongWait",
            strText )
        return tupRet

    def LongWaitCb( self, iRet, strResp ) :
        if iRet < 0 :
            print( "LongWaitCb returns with error ", iRet )
        else :
            print( "LongWaitCb returns successfully", strResp ) 

#aggregrate the interface class and the PyProxy
#class by CEchoProxy
class CActcProxy(CActcClient, PyRpcProxy):
    def __init__(self, pIoMgr, strDesc, strObjName) :
        super(CActcClient, self).__init__(
            pIoMgr, strDesc, strObjName )

def test_main() : 
    ret = 0
    oContext = PyRpcContext();
    with oContext :
        print( "start to work here..." )
        oProxy = CActcProxy( oContext.pIoMgr,
            "../test/debug64/actcdesc.json",
            "CActcServer" );

        ret = oProxy.GetError() 
        if ret < 0 :
            return ret

        ret = oProxy.Start()
        if ret < 0 :
            return ret

        while True :
            #wait if the server is not online
            while ( cpp.stateRecovery ==
                oProxy.oInst.GetState() ):
                time.sleep( 1 )

            tupRet = oProxy.LongWait( "Hello, World" )
            if tupRet[ 0 ] != 65537 :
                ret = tupRet[ 0 ]
                break

            qwTaskId = np.int64( tupRet[ 1 ] )
            time.sleep( 2 )
            ret = oProxy.oInst.CancelRequest( qwTaskId )
            if ret == 0 :
                print( "the request is canceled sucessfully" )
            else :
                print( "failed to cancel the request" )
            break;

        oProxy.Stop()

    return ret


ret = test_main()
quit( ret )
