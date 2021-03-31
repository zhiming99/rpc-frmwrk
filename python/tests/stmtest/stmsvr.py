import sys
import time
import numpy as np
import threading as tr
from rpcf.rpcbase import *

from rpcf.proxy import PyRpcContext, PyRpcServer
from rpcf.proxy import ErrorCode as EC

#1. define the interface the CEchoServer provides
class CEchoServer:
    """mandatory class member to define the
    interface name, which will be used to invoke
    the event handler if any
    """
    ifName = "CEchoServer"

    def Echo(self, callback, text ):
        listResp = [ 0 ]
        listParams = [ text ]
        listResp.append( listParams )
        return listResp

class CTransContext :
    def __init__( self ) :
        self.iCounter = 0
        self.iError = 0
    
#2. aggregrate the interface class and the PyRpcServer
# class by CStreamingServer to pickup the python-cpp
# interaction support

class CStreamingServer(CEchoServer, PyRpcServer):
    def __init__(self, pIoMgr, strCfgFile, strObjName) :
        super( CEchoServer, self ).__init__(
            pIoMgr, strCfgFile, strObjName )

    def GetCounter( self, hChannel ) :
        context = self.GetChanCtx( hChannel )
        if context is None :
            return None
        return context.iCounter

    def SetCounter( self, hChannel, i ) :
        context = self.GetChanCtx( hChannel )
        if context is None :
            return 
        context.iCounter = i 

    def ReadAndReply( self, hChannel ) :
        while True :
            listResp = self.ReadStreamAsync(
                hChannel,
                CStreamingServer.ReadStmCallback )
            ret = listResp[ 0 ]
            if ret < 0 :
                print( "Error occurs", ret )
                break
            elif ret == EC.STATUS_PENDING :
                break
            print( "proxy says: ", listResp[ 1 ] )
            i = self.GetCounter( hChannel )
            strMsg = "This is message {}"
            ret = self.WriteStreamAsync(
                hChannel, strMsg.format( i ),
                CStreamingServer.WriteStmCallback )
            i += 1
            self.SetCounter( hChannel, i )
            if ret < 0 or ret == EC.STATUS_PENDING:
                break;

    def WriteAndReceive( self, hChannel ) :
        while True :
            ret = 0
            i = self.GetCounter( hChannel )
            strMsg = "This is message {}"
            ret = self.WriteStreamAsync(
                hChannel, strMsg.format( i ),
                CStreamingServer.WriteStmCallback )
            i += 1
            self.SetCounter( hChannel, i )
            if ret < 0 :
                print( "WriteAndReceive error...", ret )
                break
            if ret == EC.STATUS_PENDING:
                break
            listResp = self.ReadStreamAsync(
                hChannel,
                CStreamingServer.ReadStmCallback )
            ret = listResp[ 0 ]
            if ret < 0 :
                print( "Error occurs", ret )
                break
            elif ret == EC.STATUS_PENDING :
                break
            print( "proxy says: ", listResp[ 1 ] )

    '''OnStmReady is a system defined event
    handler, called when the stream channel is
    ready to send/receive. There must be an
    implementation of OnStmReady for the server to
    support streaming related things.
    '''
    def OnStmReady( self, hChannel ) :
        greeting = "Hello, Proxy"
        self.oInst.WriteStreamNoWait(
            hChannel, greeting )
        context = CTransContext()
        self.SetChanCtx( hChannel, context )
        ret = self.ReadAndReply( hChannel )

    def WriteStmCallback( self, iRet, hChannel, pBuf ) :
        if iRet < 0 :
            print( "Write failed with error",
                hChannel, iRet )
            return
        ret = self.ReadAndReply( hChannel )

    '''a callback for async stream read
    '''
    def ReadStmCallback( self, iRet, hChannel, pBuf ) :
        if iRet < 0 :
            print( "Read failed with error",
                hChannel, iRet )
            return
        print( "proxy says: ", pBuf )
        self.WriteAndReceive( hChannel )

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

        '''create the server object, and start
        it'''
        print( "start to work here..." )
        oServer = CStreamingServer( oContext.pIoMgr,
            "../../../test/debug64/stmdesc.json",
            "CStreamingServer" );

        ret = oServer.GetError() 
        if ret < 0 :
            break

        ret = oServer.Start()
        if ret < 0 :
            break

        if ret == EC.STATUS_PENDING :
            break

        #wait if the server is not online
        while ( cpp.stateRecovery ==
            oServer.oInst.GetState() ):
            time.sleep( 1 )

        #keep wait till offline
        while ( cpp.stateConnected ==
            oServer.oInst.GetState() ):
            time.sleep( 1 )
            
        break

    ''' Stop the guys'''
    if oServer is not None :
        oServer.Stop();

    if oContext is not None:
        oContext.Stop()

    return ret;


ret = test_main()
quit( ret )

