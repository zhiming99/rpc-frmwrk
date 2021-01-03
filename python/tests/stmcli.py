#asynchronous version of echocli
import sys
import numpy as np
import threading as tr
from rpcfrmwrk import *

sys.path.insert(0, '../')
from proxy import PyRpcContext, PyRpcProxy

#1. define the interface the CEchoServer provides
class CEchoClient:
    """mandatory class member to define the
    interface name, which will be used to invoke
    the event handler if any
    """
    ifName = "CEchoServer"

    '''all the proxy methods have the similiar
    implementation, pass the callback function,
    the interface name, method name and all the
    input parameters to sendRequestAsync'''
    def Echoa(self, text ):
        tupRet = self.sendRequestAsync(
            CEchoClient.EchoCb,
            self.ifName, "Echo", text )

        ret = tupRet[ 0 ]

        if ret == 65537 :
            self.sem.acquire()
            ret = 0

        return ret;

    ''' Callback method has the first parameter
    as the self object, the second parameter as
    the return value, the rest parameters are
    parameters returned. The callback method's
    return value is ignored so far'''
    def EchoCb( self, iRet, textResp ) :
        if iRet < 0 :
            print( "error occured in EchoAsyncCb", iRet )
        else :
            print( "Text Response is: ", textResp );
        self.sem.release()

#2. aggregrate the interface class and the PyRpcProxy
# class by CStreamingClient to pickup the python-cpp
# interaction support

class CStreamingClient(CEchoClient, PyRpcProxy):
    def __init__(self, pIoMgr, strCfgFile, strObjName) :
        super( CEchoClient, self ).__init__(
            pIoMgr, strCfgFile, strObjName )

        '''a semaphore to synchronize between the
        main thread and the callback thread'''
        self.sem = tr.Semaphore( 0 )
        self.iError = 0

    def WriteStmCallback( self, iRet, hChannel, pBuf ) :
        if iRet < 0 :
            print( "Write failed with error", iRet )
        else :
            print( " server says( async ): ", pBuf )
        self.iError = iRet
        self.sem.release()

    '''a callback for async stream read
    '''
    def ReadStmCallback( self, iRet, hChannel, pBuf ) :
        if iRet < 0 :
            print( "Read failed with error", iRet )
        else :
            print( " server says(async): ", pBuf )
        self.iError = iRet
        self.sem.release()

    def GetError( self ) :
        return self.iError

def test_main() : 
    while( True ) :
        '''create the transfer context, and start
        it'''
        oContext = PyRpcContext();
        ret = oContext.Start();
        if ret < 0 :
            break;

        '''create the proxy object, and start
        it'''
        print( "start to work here..." )
        oProxy = CStreamingClient( oContext.pIoMgr,
            "../../test/debug64/stmdesc.json",
            "CStreamingServer" );

        ret = oProxy.GetError() 
        if ret < 0 :
            break

        ret = oProxy.Start()
        if ret < 0 :
            break

        ''' make the RPC calls
        '''
        #Echo a plain text
        print( "Echo..." )
        ret = oProxy.Echoa( "Are you ok" )
        if ret < 0 :
            break

        '''to setup a stream channel between the
        client and the server '''
        oDesc = cpp.CParamList()
        tupRet = oProxy.StartStream(
            oDesc.GetCfgAsObj() ) 

        ret = tupRet[ 0 ]
        if ret < 0 :
            break;

        hChannel = tupRet[ 1 ]
        
        print( "start talking..." )
        for i in range( 10000 ) :
            strFmt = "a message to server {} " 
            strMsg = strFmt.format( i )
            ret = oProxy.WriteStream(
                hChannel, strMsg )
            if ret < 0 :
                break

            tupRet = oProxy.ReadStream( hChannel )
            ret = tupRet[ 0 ]
            if ret < 0 :
                break

            pBuf = tupRet[ 1 ]
            print( "Server says( sync ): ", pBuf )

            b =  i + 0.1;
            strMsg = strFmt.format( b ) 
            ret = oProxy.WriteStreamAsync(
                hChannel, strMsg,
                CStreamingClient.WriteStmCallback )
            if ret < 0 :
                break
            if ret == 65537 :
                oProxy.sem.acquire()    

            ret = oProxy.GetError()
            if ret < 0 :
                break

            ret = oProxy.ReadStreamAsync( hChannel,
                CStreamingClient.ReadStmCallback )
            if ret < 0 :
                break
            
        break

    ''' Stop the guys'''
    if ( oProxy is not None and 
        oProxy.GetError() == 0 ):
        oProxy.Stop();

    if oContext is not None:
        oContext.Stop()

    return ret;


ret = test_main()
quit( ret )
