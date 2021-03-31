import sys
import time
import numpy as np
import threading as tr
import getpass
import errno
import os
import io
from rpcf.rpcbase import *

import types
from rpcf.proxy import PyRpcContext, PyRpcProxy
from rpcf.proxy import ErrorCode as EC
from typing import Union

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

class CFileInfo :
    def __init__(self ) :
        self.fileName = ""
        self.size = 0
        self.bRead = True

# PyFileTransfer, an RPC interface
class PyFileTransfer :

    ifName = "PyFileTransfer"

    def __init__( self ) :
        pass

    ''' rpc method
    '''
    def UploadFile( self,
        callback,
        fileName:   str,
        chanHash:   np.uint64, 
        offset:     np.uint64,
        size:       np.uint64 )->[ int, list ]:
        pass

    ''' rpc method
    '''
    def DownloadFile( self,
        callback,
        fileName:   str,
        chanHash:   np.uint64,
        offset:     np.uint64,
        size:       np.uint64 )->[ int, list ]:
        pass

    ''' rpc method
    '''
    def GetFileInfo( self,
        callback,
        fileName : str ) -> [ int, list ]:
        pass

class PyFileTransferBase( PyFileTransfer ):

    def __init__( self ):
        self.mapChannels = dict()

    def GetTransCtx( self, hChannel )->Union[
        CTransContext, None ] :
        if hChannel in self.mapChannels :
            context = self.mapChannels[ hChannel ]
            return context
        return None

    def SetTransCtx( self, hChannel, ctx )->None :
        self.mapChannels[ hChannel ] = ctx

    def ReadFileAndSend( self, hChannel ) :
        ret = 0
        sizeLimit = 2 * 1024 * 1204
        ctx = self.GetTransCtx( hChannel )
        while True :
            iSize = ctx.iBytesLeft
            if iSize > sizeLimit :
                iSize = sizeLimit
            elif iSize == 0 :
                self.OnTransferDone( hChannel )
                break

            pBuf = ctx.fp.read( iSize )
            ret = self.WriteStreamAsync(
                hChannel, pBuf,
                self.WriteStmCallback )
            if ret < 0 or ret == EC.STATUS_PENDING :
                break

            ctx.iBytesSent += len( pBuf )
            ctx.iBytesLeft -= len( pBuf )

        return ret

    def SendToken( self, hChannel, pBuf ) :
        self.oInst.WriteStreamNoWait( hChannel, pBuf )

    def WriteFileAndRecv( self, hChannel, pBuf ) :
        ctx = self.GetTransCtx( hChannel )
        if ctx is None :
            return -errno.EFAULT

        ret = 0
        sizeLimit = 2 * 1024 * 1204
        while True :
            if len( pBuf ) > 0 :
                ctx.fp.write( pBuf )
                ctx.iBytesLeft -= len( pBuf )
                ctx.iBytesSent += len( pBuf )

            print( f"Received {ctx.iBytesSent},",
                f"To receive {ctx.iBytesLeft} " )

            iSize = ctx.iBytesLeft
            if iSize > sizeLimit :
                iSize = sizeLimit
            elif iSize == 0 :
                self.OnTransferDone( hChannel )
                if self.oInst.IsServer() :
                    self.DeferCall(
                        self.SendToken, hChannel,
                        "over" )
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

    def OnTransferDone( self, hChannel ) :
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

        ctx = self.GetTransCtx( hChannel )
        if ctx is None :
            return

        iSize = len( pBuf )
        ctx.iBytesSent += iSize
        ctx.iBytesLeft -= iSize
        print( f"Sent {ctx.iBytesSent},",
            f"To send {ctx.iBytesLeft} " )

        ret = self.ReadFileAndSend( hChannel )

    '''a callback for async stream read
    '''
    def ReadStmCallback( self, iRet, hChannel, pBuf ) :
        if pBuf is None :
            DebugPrint( "incoming buffer is None" )
        if iRet < 0 :
            print( "Upload failed with error",
                hChannel, iRet )
            return
        self.WriteFileAndRecv( hChannel, pBuf )
    
