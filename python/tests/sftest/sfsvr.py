'''This test case demonstrates 
    . Double-direction big data transfer via
    stream channel

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

from sfcommon import CTransContext, CFileInfo
from sfcommon import PyFileTransfer, PyFileTransferBase

#1. define the interfaces to support
#CEchoServer interface
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

#2. aggregrate the interface class and the PyRpcServer
# class by PyFileTransSvr to pickup the python-cpp
# interaction support

class PyFileTransSvr( PyFileTransferBase ):

    strRootDir = "/tmp/sfsvr-root/" + getpass.getuser()

    def __init__(self ) :
        PyFileTransferBase.__init__( self )

    ''' rpc method
    '''
    def UploadFile( self,
        callback,
        fileName:   str,
        chanHash:   np.uint64, 
        offset:     np.uint64,
        size:       np.uint64 )->[ int, list ]:
        resp = [ 0, list() ]
        while True :
            fileName = self.strRootDir + "/" + fileName + "-1"

            hChannel = self.oInst.GetChanByIdHash(
                np.uint64( chanHash ) )
            if hChannel == 0 :
                resp[ 0 ] = -errno.EINVAL
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
                iSize = fp.seek( 0, os.SEEK_END )
                if iSize < offset :
                    resp[ 0 ] = -errno.ERANGE
                    break
                fp.seek( offset, os.SEEK_SET )
                fp.truncate()

            except OSError as err :
                resp[ 0 ] = -err.errno
                break

            if resp[ 0 ] < 0 :
                break

            oCtx.fp = fp
            oCtx.iOffset = offset
            oCtx.iSize = size
            oCtx.iBytesLeft = size
            oCtx.byDir = 'u'
            oCtx.strPath = fileName

            '''start the transfer a bit later
            '''
            pBuf = bytearray()
            ret = self.DeferCall(
                self.WriteFileAndRecv,
                hChannel, pBuf )
            if ret < 0 :
                resp[ 0 ] = ret;
            else :
                '''transfer will start immediately
                and complete this request with
                success
                '''
                resp[ 0 ] = EC.STATUS_SUCCESS

            break

        return resp

    ''' rpc method
    '''
    def GetFileInfo( self,
        callback,
        fileName : str,
        bRead : bool ) -> [ int, list ] :
        resp = [ 0, None ]
        strPath = self.strRootDir + "/" + fileName
        if not os.path.isfile( strPath ) :
            print( "{} does not exist" )
            resp[ 0 ] = -errno.ENOENT
            return resp

        fileInfo = CFileInfo()
        fileInfo.fileName = fileName
        fileInfo.bRead = bRead

        if bRead :
            flag = "rb+"
        else :
            flag = "wb+" 
        try:
            fp = open( strPath, flag )
            fileInfo.size = fp.seek( 0, os.SEEK_END )
            fp.close()
            resp[ 1 ] = [ fileInfo, ]
        except OSError as err :
            resp[ 0 ] = -err.errno

        return resp

    ''' rpc method
    '''
    def DownloadFile( self,
        callback,
        fileName:   str,
        chanHash:   np.uint64,
        offset:     np.uint64,
        size:       np.uint64 )->[ int, list ]:
        resp = [ 0, list() ]
        while True :
            fileName = self.strRootDir + "/" + fileName
            if not os.access( fileName, os.R_OK ) :
                resp[ 0 ] = -errno.EACCES
                break

            hChannel = self.oInst.GetChanByIdHash(
                np.uint64( chanHash ) )
            if hChannel == 0 :
                resp[ 0 ] = -errno.EINVAL
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
                fp = open( fileName, "rb" )
                iSize = fp.seek( 0, os.SEEK_END )
                if iSize < offset + size :
                    resp[ 0 ] = -errno.ERANGE
                else : 
                    fp.seek( offset, os.SEEK_SET ) 
            except OSError as err :
                resp[ 0 ] = -err.errno
                break

            if resp[ 0 ] < 0 :
                break

            oCtx.fp = fp
            oCtx.iOffset = offset
            oCtx.iSize = size
            oCtx.iBytesLeft = size
            oCtx.byDir = 'd'
            oCtx.strPath = fileName

            '''start the transfer a bit later
            '''
            ret = self.DeferCall(
                self.ReadFileAndSend, hChannel )
            if ret < 0 :
                resp[ 0 ] = ret;
            else :
                '''transfer will start immediately
                this request is successful
                '''
                resp[ 0 ] = EC.STATUS_SUCCESS

            break

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
        self.DeferCall(
            self.SendToken,
            hChannel, "rdy" )

    '''OnStmClosing is a system defined event
    handler, called when the stream channel has
    been closed by the peer, or this proxy/server
    will shutdown, or actively calls `CloseStream'
    '''
    def OnStmClosing( self, hChannel ) :
        self.OnTransferDone( hChannel )
        self.mapChannels.pop( hChannel )

class PyFileTransSvrObj(
    CEchoServer, PyFileTransSvr, PyRpcServer ) :
    def __init__(self, pIoMgr, strCfgFile, strObjName) :
        PyRpcServer.__init__(
            self, pIoMgr, strCfgFile, strObjName )
        PyFileTransSvr.__init__( self )
        CEchoServer.__init__( self )

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
        oServer = PyFileTransSvrObj( oContext.pIoMgr,
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

