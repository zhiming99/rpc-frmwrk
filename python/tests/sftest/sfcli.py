'''This test case demonstrates 
    . Double-direction big data transfer via
    streams

    . How to enable python's built-in
    serialization

    . Multiple interfaces support

    . Note: this server is dedicated for python
    client due to python specific serilization
'''
import sys
import time
import numpy as np
import threading as tr
import getpass
import errno
import os
import io
from rpcfrmwrk import *

import types
sys.path.insert(0, '../../')
from proxy import PyRpcContext, PyRpcServer
from proxy import ErrorCode as EC
from typing import Union
from sfsvr import CTransContext, CFileInfo,
from sfsvr import PyFileTransfer, PyFileTransferBase

#1. define the interfaces to support
#CEchoServer interface
class CEchoClient:
    """mandatory class member to define the
    interface name, which will be used to invoke
    the event handler if any
    """
    ifName = "CEchoServer"

    def Echo(self, text ):
        return self.sendRequest(
            CEchoClient.ifName, "Echo", text )

class PyFileTransProxy(
    CEchoClient, PyFileTransferBase, PyRpcProxy):

    def __init__(self, pIoMgr, strCfgFile, strObjName) :
        super( PyFileTransferBase, self ).__init__(
            pIoMgr, strCfgFile, strObjName )
        super( CEchoClient, self ).__init__()

    ''' rpc method
    '''
    def UploadFile( self,
        fileName:   str,
        chanHash:   np.int64, 
        offset:     np.int64,
        size:       np.int64 )->[ int, list ]:

        resp = PySendRequest( self,
            ifName, "UploadFile",
            fileName, hChannel,
            offset, size )

        return resp

    ''' rpc method
    '''
    def GetFileInfo( self,
        fileName : str,
        bRead : bool ) -> [ int, list ]
        return self.PySendRequest(
           PyFileTransfer.ifName, "GetFileInfo", 
            fileName, bRead )

    ''' rpc method
    '''
    def DownloadFile( self,
        fileName:   str,
        chanHash:   np.int64,
        offset:     np.int64,
        size:       np.int64 )->[ int, list ]:

        resp = self.PySendRequest(
           PyFileTransfer.ifName, "DownloadFile", 
           fileName, chanHash, offset, size )

        return resp

    '''OnStmReady is a system defined event
    handler, called when the stream channel is
    ready to send/receive. There must be an
    implementation of OnStmReady for the server to
    support streaming related things.
    '''
    def OnStmReady( self, hChannel ) :
        self.SetTransCtx( hChannel,
            CTransContext() )

    def OnStmClosing( self, hChannel )
        self.OnTransferDone( hChannel )
        self.mapChannels.pop( hChannel )

    def DoUploadFile( self,
        fileName:   str,
        hChannel:   np.int64, 
        offset:     np.int64,
        size:       np.int64 )->[ int, list ]:
        resp = [ EC.STATUS_SUCCESS, list() ]
        while True :
            if not os.access( fileName, os.W_OK ) :
                resp[ 0 ] = -errno.EACCES
                break

            oCtx = self.mapChannels[ hChannel ]
            if oCtx is None :
                resp[ 0 ] = -errno.ENOENT
                break;

            if size == 0 :
                resp[ 0 ] = -errno.EINVAL
                break;
            '''The channel can be full-duplex.
            But for simplicity, just one direction
            at one time
            '''
            if oCtx.fp is not None:
                resp[ 0 ] = -error.EBUSY
                break

            try:
                fp = open( fileName, "rb+" )
                iSize = fp.seek( 0, SEEK_END )
                if iSize < offset :
                    resp[ 0 ] = -errno.ERANGE
                    break
                fp.seek( offset, SEEK_SET )

            except OSError( eno, strerr ) : 
                resp[ 0 ] = -eno
                break

            if resp[ 0 ] < 0 :
                break

            ret = self.oInst.GetPeerIdHash( hChannel )
            if ret[ 0 ] < 0 :
                resp[ 0 ] = ret[ 0 ]
                break;

            chanHash = ret[ 1 ]
            ret = self.UploadFile( ifName,
                chanHash, offset, size )
            if ret[ 0 ] < 0 :
                resp[ 0 ] = ret[ 0 ]
                break;

            oCtx.fp = fp
            oCtx.iOffset = offset
            oCtx.iSize = size
            oCtx.iBytesLeft = size
            oCtx.byDir = 'u'
            oCtx.strPath = fileName

            '''start the transfer a bit later
            '''
            ret = self.DeferCall(
                self.ReadFileAndSend, hChannel )
            if ret < 0 :
                resp[ 0 ] = ret;
            else
                '''transfer will start immediately
                and complete this request with
                success
                '''
                resp[ 0 ] = EC.STATUS_SUCCESS

        return resp

    def DoDownloadFile( self,
        fileName:   str,
        hChannel:   np.int64,
        offset:     np.int64,
        size:       np.int64 )->[ int, list ]:
        resp = [ 0, list() ]
        while True :
            if not os.access( fileName, os.W_OK ) :
                resp[ 0 ] = -errno.EACCES
                break

            oCtx = self.mapChannels[ hChannel ]
            if oCtx is None :
                resp[ 0 ] = -errno.ENOENT
                break;

            if size == 0 :
                resp[ 0 ] = -errno.EINVAL
                break;
            '''The channel can be full-duplex.
            But for simplicity, just one direction
            at one time
            '''

            if oCtx.fp is not None:
                resp[ 0 ] = -error.EBUSY
                break
            
            try:
                fp = open( fileName, "wb+" )
                iSize = fp.seek( 0, SEEK_END )
                if iSize < offset + size :
                    resp[ 0 ] = -errno.ERANGE
                else : 
                    fp.truncate( offset )
                    fp.seek( offset, SEEK_SET ) 

            except OSError( eno, strerr ) : 
                resp[ 0 ] = -eno
                break

            if resp[ 0 ] < 0
                break

            oCtx.fp = fp
            oCtx.iOffset = offset
            oCtx.iSize = size
            oCtx.iBytesLeft = size
            oCtx.byDir = 'd'
            oCtx.strPath = fileName

            '''start the transfer a bit later
            '''
            pBuf = bytearray()
            ret = self.DeferCall(
                self.WriteFileAndRecv, hChannel,
                pBuf )
            if ret < 0 :
                resp[ 0 ] = ret;
            else
                '''transfer will start immediately
                this request is successful
                '''
                resp[ 0 ] = EC.STATUS_SUCCESS

        return resp


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
        oServer = PyFileTransProxy( oContext.pIoMgr,
            "./sfdesc.json",
            "PyFileTransSvr" );

        ret = oServer.GetError() 
        if ret < 0 :
            break

        ret = oServer.Start()
        if ret < 0 :
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

