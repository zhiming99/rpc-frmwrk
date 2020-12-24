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

    def EchoUnknowna( self, byteBuf ):
        tupRet = self.sendRequestAsync(
            CEchoClient.EchoUnknownCb,
            self.ifName, "EchoUnknown", byteBuf )
        ret = tupRet[ 0 ]
        if ret == 65537 :
            self.sem.acquire()
            ret = 0
        return ret;

    '''EchoUnknown has a bytearray to return if
    the request succeeds as iRet is 0'''
    def EchoUnknownCb( self, iRet, byteBuf ):
        if iRet < 0 :
            print( "error occured in EchoUnknownCb", iRet )
        else :
            print( "Buf response is: ", byteBuf )
        self.sem.release()

    #return sum of i1+i2
    def Echo2a( self, i1, i2 ):
        tupRet = self.sendRequestAsync(
            CEchoClient.Echo2Cb,
            self.ifName, "Echo2",
            np.int32( i1 ),
            np.float64( i2 ) )

        ret = tupRet[ 0 ]

        if ret == 65537 :
            self.sem.acquire()
            ret = 0

        return ret;

    '''Echo2 has a double value to return if the
    request succeeds'''
    def Echo2Cb( self, iRet, i ):
        if iRet < 0 :
            print( "error occured in Echo2Cb", iRet )
        else :
            print( "sum is: ", i );

        self.sem.release()

    def EchoCfga( self, iCount, pObj ):
        tupRet = self.sendRequestAsync(
            CEchoClient.EchoCfgCb,
            self.ifName, "EchoCfg",
            np.int32( iCount ), pObj )

        ret = tupRet[ 0 ]

        if ret == 65537 :
            self.sem.acquire()
            ret = 0

        return ret;

    #unpack the config object
    def HandleResp( self, pObj ) :
        pCfg = cpp.CastToCfg( pObj )
        oResp = cpp.CParamList( pCfg )
        ret = oResp.GetSize()
        if ret[ 0 ] == 0 and ret[ 1 ] > 0 :
            retText = oResp.GetStrProp( 0 )
            print( "response is \"",
                retText[ 1 ], "\"" )

        return ret[ 0 ]

    '''EchoCfgCb will have two parameters to
    return as the response if the request
    succeeds. one is the property count in the
    pCfg, and the other is an objptr object
    carrying some properties.'''
    def EchoCfgCb( self, iRet, iCount, pCfg ) :
        if iRet < 0 :
            print( "error occured in EchoCfgCb", iRet )
        else :
            ret = self.HandleResp( pCfg )
            if ret < 0 :
                print( "error occured in EchoCfgCb 2", ret )
        self.sem.release()


#2. aggregrate the interface class and the PyRpcProxy
# class by CEchoProxyAsync to pickup the python-cpp
# interaction support

class CEchoProxyAsync(CEchoClient, PyRpcProxy):
    def __init__(self, pIoMgr, strCfgFile, strObjName) :
        super( CEchoClient, self ).__init__(
            pIoMgr, strCfgFile, strObjName )

        '''a semaphore to synchronize between the
        main thread and the callback thread'''
        self.sem = tr.Semaphore( 0 )

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
        oProxy = CEchoProxyAsync( oContext.pIoMgr,
            "../../test/debug64/echodesc.json",
            "CEchoServer" );

        ret = oProxy.GetError() 
        if ret < 0 :
            break;

        ret = oProxy.Start()
        if ret < 0 :
            break;

        ''' make the RPC calls
        '''
        for i in range( 10 ) :
            #Echo a plain text
            print( "Echo" )
            ret = oProxy.Echoa( "Are you ok" );
            if ret < 0 :
                break;

            #Add to numbers and retun the sum
            print( "Echo2" )
            ret = oProxy.Echo2a( 2, 3.0 );
            if ret < 0 :
                break;

            #Echo whatever a block of binary data
            print( "EchoUnknown" )
            f = open( "/usr/bin/sh", "rb" )
            f.seek(1024);
            o = f.read( 16 );
            print( o );
            f.close();
            ret = oProxy.EchoUnknowna( o );
            if ret < 0 :
                break;

            #Echo a serializable object held
            # by oParams
            print( "EchoCfg" )
            oParams = cpp.CParamList();
            oParams.PushStr( "This is EchoCfg" );
            pObjSend = oParams.GetCfgAsObj();
            ret = oProxy.EchoCfga( 1, pObjSend );
            if ret < 0 :
                break;
        break;

    ''' Stop the guys'''
    if ( oProxy is not None and 
        oProxy.GetError() == 0 ):
        oProxy.Stop();

    if oContext is not None:
        oContext.Stop()

    return ret;


ret = test_main()
quit( ret )
