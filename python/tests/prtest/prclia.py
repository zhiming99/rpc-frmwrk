#asynchronous version proxy for CPauseResumeServer

import sys
import numpy as np
import threading as tr
from rpcf.rpcbase import *

sys.path.insert(0, '../../')
from rpcf.proxy import PyRpcContext, PyRpcProxy

#1. define the interface 
class CPauseResumeClient:
    """mandatory class member to define the
    interface name, which will be used to invoke
    the event handler if any
    """
    ifName = "CPauseResumeServer"

    '''all the proxy methods have the similiar
    implementation, pass the callback function,
    the interface name, method name and all the
    input parameters to sendRequestAsync'''
    def Echoa(self, text ):
        tupRet = self.sendRequestAsync(
            CPauseResumeClient.EchoCb,
            self.ifName, "Echo", text )

        return tupRet;

    ''' Callback method has the first parameter
    as the self object, the second parameter as
    the return value, the rest parameters are
    parameters returned. The callback method's
    return value is ignored so far'''
    def EchoCb( self, iRet, textResp ) :
        print( "EchoCb" )
        if iRet < 0 :
            print( "error occured in Echo request", iRet )
        else :
            print( "Text Response is: ", textResp );
        self.iError = iRet 
        self.sem.release()

    def EchoUnknowna( self, byteBuf ):
        tupRet = self.sendRequestAsync(
            CPauseResumeClient.EchoUnknownCb,
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
            print( "error occured in EchoUnknown", iRet )
        else :
            print( "Buf response is: ", byteBuf )
        self.iError = iRet 
        self.sem.release()

    #return sum of i1+i2
    def EchoMany( self, i1, i2, i3, i4, i5, strText ):
        tupRet = self.sendRequestAsync(
            CPauseResumeClient.EchoManyCb,
            self.ifName, "EchoMany",
            np.int32( i1 ),
            np.int16( i2 ),
            np.int64( i3 ),
            np.float32( i4 ),
            np.float64( i5 ),
            strText )

        ret = tupRet[ 0 ]

        if ret == 65537 :
            self.sem.acquire()
            ret = 0

        return ret;

    '''EchoMany has 6 arguments to return if the
    request succeeds'''
    def EchoManyCb( self, iRet, o1, o2, o3, o4, o5, strReply ):
        print( "EchoManyCb" )
        if iRet < 0 :
            print( "error occured in EchoMany", iRet )
        else :
            print( "Response is: ",
                o1, o2, o3, o4, o5, strReply )

        self.iError = iRet 
        self.sem.release()



#2. aggregrate the interface class and the PyRpcProxy
# class by CPauseResumeProxy to pickup the python-cpp
# interaction support

class CPauseResumeProxy(CPauseResumeClient, PyRpcProxy):
    def __init__(self, pIoMgr, strCfgFile, strObjName) :
        super( CPauseResumeClient, self ).__init__(
            pIoMgr, strCfgFile, strObjName )

        '''a semaphore to synchronize between the
        main thread and the callback thread'''
        self.sem = tr.Semaphore( 0 )

        '''an error code to pass from callback to
        the main thread
        '''
        self.iError = 0

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
        oProxy = CPauseResumeProxy( oContext.pIoMgr,
            "../../../test/debug64/prdesc.json",
            "CPauseResumeServer" );

        ret = oProxy.GetError() 
        if ret < 0 :
            break;

        ret = oProxy.Start()
        if ret < 0 :
            break;

        print("The server is paused by default."
            "This example Demonstrates what to do",
            "when server is paused..." )

        print( "Echo..." )
        strText = "Are you OK?"
        tupRet = oProxy.Echoa( strText )
        ret = tupRet[ 0 ]
        if ret != 65537 :
            break;

        taskId = tupRet[ 1 ] 
        '''wait for 3 seconds
        '''
        bCompleted = oProxy.sem.acquire( True, 3 );
        if not bCompleted :
            print( "Server not responding for 3 seconds",
                " canceling" )
            ret = oProxy.oInst.CancelRequest( taskId )

            if ret < 0 :
                print( "CancelRequest failed", ret )
                break;
            else :
                print( "CancelRequest succeeded" )

            ''' the sem will be posted by the callback '''
            oProxy.sem.acquire()

        print( "Resume the server..." )
        ret = oProxy.oInst.Resume()
        if ret < 0 :
            break

        print( "Echo... again" )
        tupRet = oProxy.Echoa( "Are you ok" )
        if tupRet[ 0 ] == 65537 :
            oProxy.sem.acquire()
        else :
            print( "unknown error" )

        print( "EchoUnknown..." )
        f = open( "/usr/bin/sh", "rb" )
        f.seek(1024)
        o = f.read( 16 )
        print( o )
        f.close()
        oProxy.EchoUnknowna( o );

        print( "EchoMany..." )
        oProxy.EchoMany(
            1, 2, 3, 4, 5, strText )

        print( "Pause the server..." )
        oProxy.oInst.Pause()

        break

    ''' Stop the guys'''
    if ( oProxy is not None and 
        oProxy.GetError() == 0 ):
        oProxy.Stop();

    if oContext is not None:
        oContext.Stop()

    return ret


ret = test_main()
quit( ret )
