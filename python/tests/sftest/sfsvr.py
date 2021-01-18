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

class CTransContext :
    def __init__( self ) :
        self.iError = 0
        self.iBytesLeft = 0
        self.iBytesSent = 0
        self.fp = None 
        self.byDir = 0
        self.strPath = ""
        self.iOffset = 0
        self.iSize = 0

# PyFileTransfer, another interface as a
# python-specific interface
class PyFileTransfer :

    ifName = "PyFileTransfer"

    def __init__( self ) :
        pass

    ''' rpc method
    '''
    def UploadFile( self,
        fileName:   str,
        chanHash:   np.int64, 
        offset:     np.int64,
        size:       np.int64 )->[ int, list ]:
        pass

    ''' rpc method
    '''
    def DownloadFile( self,
        fileName:   str,
        chanHash:   np.int64,
        offset:     np.int64,
        size:       np.int64 )->[ int, list ]:
        pass

    ''' rpc method
    '''
    def GetFileInfo( self,
        fileName : str ) -> [ int, list ]
        pass

#2. aggregrate the interface class and the PyRpcServer
# class by PyFileTransSvr to pickup the python-cpp
# interaction support

class CFileInfo :
    def __init__(self ) :
        self.fileName = ""
        self.size = 0
        self.bRead = True

class PyFileTransferBase( PyFileTransfer ):

    def __init__( self ):
        self.mapChannels = dict()

    def GetTransCtx( self, hChannel )->
        Union[ CTransContext, None ] :
        context = self.mapChannels[ hChannel ]
        if context is None :
            return None
        return context

    def SetTransCtx( self, hChannel, ctx )->None :
        self.mapChannels[ hChannel ] = ctx

    def ReadFileAndSend( self, hChannel ) :
        ret = 0
        sizeLimit = 2 * 1024 * 1204
        while True :
            ctx = self.GetTransCtx( hChannel )
            iSize = ctx.iBytesLeft
            if iSize > sizeLimit :
                iSize = sizeLimit
            elif iSize == 0 :
                self.OnTransferDone( hChannel )
                break

            pBuf = ctx.fp.read( iSize )
            ctx.iBytesLeft -= iSize

            ret = self.WriteStreamAsync(
                hChannel, pBuf,
                self.WriteStmCallback )
            if ret < 0 or ret == EC.STATUS_PENDING :
                break;

        return ret

    def WriteFileAndRecv( self, hChannel, pBuf ) :
        while True :
            ret = 0

            if len( pBuf ) > 0 :
                ctx.fp.write( pBuf )
                ctx.iBytesLeft -= len( pBuf )

            sizeLimit = 2 * 1024 * 1204
            ctx = self.GetTransCtx( hChannel )
            iSize = ctx.iBytesLeft
            if iSize > sizeLimit :
                iSize = sizeLimit
            elif iSize == 0 :
                self.OnTransferDone( hChannel )
                break

            listResp = self.ReadStreamAsync(
                hChannel, self.ReadStmCallback,
                iSize )

            ret = listResp[ 0 ]
            if ret < 0 :
                print( "Error occurs", ret )
                break
            elif ret == EC.STATUS_PENDING :
                break
            pBuf = listResp[ 1 ]

        return ret

    def OnTransferDone( self, hChannel )
        oCtx = self.mapChannels[ hChannel ]
        if oCtx is None :
            return

        if oCtx.fp is not None :
            oCtx.fp.close()

        self.mapChannels[ hChannel ] = CTransContext()

    def WriteStmCallback( self, iRet, hChannel, pBuf ) :
        if iRet < 0 :
            print( "Download failed with error",
                hChannel, iRet )
            return
        ret = self.ReadFileAndSend( hChannel )

    '''a callback for async stream read
    '''
    def ReadStmCallback( self, iRet, hChannel, pBuf ) :
        if iRet < 0 :
            print( "Upload failed with error",
                hChannel, iRet )
            return
        self.WriteFileAndRecv( hChannel, pBuf )
    
class PyFileTransSvr(
    CEchoServer, PyFileTransferBase, PyRpcServer):

    strRootDir =
        "/tmp/sfsvr-root/" + getpass.getuser()

    def __init__(self, pIoMgr, strCfgFile, strObjName) :
        super( PyFileTransferBase, self ).__init__(
            pIoMgr, strCfgFile, strObjName )
        super( CEchoServer, self ).__init__()

    ''' rpc method
    '''
    def UploadFile( self,
        fileName:   str,
        chanHash:   np.int64, 
        offset:     np.int64,
        size:       np.int64 )->[ int, list ]:
        resp = [ 0, list() ]
        while True :
            fileName =
                self.strRootDir + "/" + fileName
            if not os.access( fileName, os.W_OK ) :
                resp[ 0 ] = -errno.EACCES
                break

            hChannel = self.oInst.
                GetChanByIdHash( np.int64( chanHash ) )
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
                iSize = fp.seek( 0, SEEK_END )
                if iSize < offset :
                    resp[ 0 ] = -errno.ERANGE
                    break
                fp.seek( offset, SEEK_SET )
                fp.truncate()

            except OSError( eno, strerr ) : 
                resp[ 0 ] = -eno
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
            else
                '''transfer will start immediately
                and complete this request with
                success
                '''
                resp[ 0 ] = EC.STATUS_SUCCESS

        return resp

    ''' rpc method
    '''
    def GetFileInfo( self,
        fileName : str,
        bRead : bool ) -> [ int, list ]
        resp = [ 0, list() ]
        strPath = self.strRootDir +
            "/" + fileName
        if not os.path.isfile( strPath ) :
            print( "{} does not exist" )
            resp[ 0 ] = -errno.ENOENT
            return resp

        fileInfo = CFileInfo()
        fileInfo.fileName = fileName
        fileInfo.bRead = bRead

        if bRead :
            flag = "rb+"
        else
            flag = "wb+" 
        try:
            fp = open( strPath, flag )
            fileInfo.size = fp.seek( 0, SEEK_END )
            fp.close()
            resp.append( [ fileInfo, ] )
        except OSError( eno, strError )
            resp[ 0 ] = -eno

        return resp

    ''' rpc method
    '''
    def DownloadFile( self,
        fileName:   str,
        chanHash:   np.int64,
        offset:     np.int64,
        size:       np.int64 )->[ int, list ]:
        resp = [ 0, list() ]
        while True :
            fileName =
                self.strRootDir + "/" + fileName
            if not os.access( fileName, os.R_OK ) :
                resp[ 0 ] = -errno.EACCES
                break

            hChannel = self.oInst.
                GetChanByIdHash( np.int64( chanHash ) )
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
                iSize = fp.seek( 0, SEEK_END )
                if iSize < offset + size :
                    resp[ 0 ] = -errno.ERANGE
                else : 
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
            ret = self.DeferCall(
                self.ReadFileAndSend, hChannel )
            if ret < 0 :
                resp[ 0 ] = ret;
            else
                '''transfer will start immediately
                this request is successful
                '''
                resp[ 0 ] = EC.STATUS_SUCCESS

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
        oServer = PyFileTransSvr( oContext.pIoMgr,
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

