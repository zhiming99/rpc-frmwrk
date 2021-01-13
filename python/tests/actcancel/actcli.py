import sys
import time
import numpy as np

from rpcfrmwrk import *

sys.path.insert(0, '../../')
from proxy import PyRpcContext, PyRpcProxy

class CActcClient:
    """Mandatory class member to define the
    interface name, which will be used to invoke
    the event handler
    """
    ifName = "CActcServer"
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
    def LongWait( self, strText ) :
        tupRet = self.sendRequestAsync(
            CActcClient.LongWaitCb,
            self.ifName, "LongWait",
            strText )
        return tupRet

    '''
    Method: LongWaitCb

    Description: the callback of `LongWait'
    request. It will be called whether  the
    request succeeds or not. If `iRet' is
    STATUS_SUCCESS, the parameter `strReply'
    contains the text string sent by LongWait
    appended with a " 2" by the server.

    Return Value: none
    '''
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
            "../../test/debug64/actcdesc.json",
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
