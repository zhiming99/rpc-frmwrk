from rpcfrmwrk import *
import time
from proxy import PyRpcContext, PyRpcProxy
import numpy as np
import threading as tr

class CKeepAliveClient:
    """Mandatory class member to define the
    interface name, which will be used to invoke
    the event handler
    """
    ifName = "CKeepAliveServer"
    """
    Method: LongWait
    Description:
    this is a long request, and the client would
    wait for about 5 minutes before the reply gets
    back. Meanwhile, the server will send a
    keep-alive event to notify the client the
    request is still normal, and the client will
    continue to wait. Otherwise, the client will
    cancel the request in 2 minutes.
    """
    def LongWait( self, strText ) :

        tupRet = self.sendRequestAsync(
            CKeepAliveClient.LongWaitCb,
            self.ifName, "LongWait",
            strText )

        ret = tupRet[ 0 ]

        if ret == 65537 :
            self.sem.acquire()
            ret = 0
        elif ret < 0 :
            print( "LongWait: the request failed with", ret );

        return ret;

    '''
    Description:
    the callback of LongWait Request. It will be
    called whether or not the request succeeds. If
    `iRet' is STATUS_SUCCESS, the parameter
    `strReply' contains the text string sent by
    LongWait appended with a " 2" by the server.
    '''
    def LongWaitCb( self, iRet, strReply ) :
        if iRet < 0 :
            print( "error occured in Echo2Cb", iRet )
        else :
            print( "Reponse is: ", strReply );

        self.sem.release()
    


'''aggregrate the interface class and the PyProxy
class to pick up the underlying rpc support
'''
class CKeepAliveProxy(CKeepAliveClient, PyRpcProxy):
    def __init__(self, pIoMgr, strDesc, strObjName) :
        super(CKeepAliveClient, self).__init__(
            pIoMgr, strDesc, strObjName )

        '''a semaphore to synchronize between the
        main thread and the callback thread'''
        self.sem = tr.Semaphore( 0 )

def test_main() : 
    ret = 0
    oContext = PyRpcContext();
    with oContext :
        print( "start to work here..." )
        oProxy = CKeepAliveProxy( oContext.pIoMgr,
            "../test/debug64/kadesc.json",
            "CKeepAliveServer" );

        ret = oProxy.GetError() 
        if ret < 0 :
            return ret

        ret = oProxy.Start()
        if ret < 0 :
            return ret

        #wait if the server is not online
        while ( cpp.stateRecovery ==
            oProxy.oInst.GetState() ):
            time.sleep( 1 )

        ret = oProxy.LongWait( "Hello, World" )

        oProxy.Stop()

    return ret


ret = test_main()
quit( ret )
