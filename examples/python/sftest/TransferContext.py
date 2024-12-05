from rpcf.proxy import *
import threading as tr
import TransFileContext

class TransferContext :
    def __init__(self, oInst : PyRpcServices ) :
        self.m_oInst = oInst
        self.m_mapChanToCtx = dict()

    def AddContext(self, hChannel : int, o : TransFileContext )->int:
        if hChannel == ErrorCode.INVALID_HANDLE :
            return -errno.EINVAL
        old = None
        if hChannel in self.m_mapChanToCtx:
            old = self.m_mapChanToCtx[ hChannel ]

        if old is not None and not old.IsDone():
            return -errno.EEXIST

        self.m_mapChanToCtx[hChannel] = o
        return ErrorCode.STATUS_SUCCESS

    def RemoveContext( self, hChannel : int):
        self.m_mapChanToCtx.pop(hChannel)

    def OnTransferDone( self, hChannel : int, iRet : int) :
        if not hChannel in self.m_mapChanToCtx:
            return
        o = self.m_mapChanToCtx[hChannel]
        o.CleanUp()
        self.RemoveContext(hChannel)

    def ReadFileAndSend( self, hChannel : int)->int:
        ret = 0
        if not hChannel in self.m_mapChanToCtx:
            return -errno.ENOENT
        iSizeLimit = 2 * 1024 * 1024
        o = self.m_mapChanToCtx[hChannel]
        while True:
            iSize = o.m_iBytesLeft
            if iSize > iSizeLimit:
                iSize = iSizeLimit
            elif iSize == 0 :
                self.OnTransferDone(hChannel, 0)
                break
            pBuf = o.m_fp.read(iSize)
            ret = self.m_oInst.WriteStreamAsync2(
                hChannel, pBuf)
            if ret == ErrorCode.STATUS_PENDING:
                break
            if ret < 0:
                self.OnTransferDone(hChannel, ret)
                break
            o.m_iBytesLeft -= iSize
            o.m_iBytesSent += iSize

        return ret

    def SendToken( self, hChannel : int, buf : bytearray) -> int :
        return self.m_oInst.WriteStreamNoWait(hChannel,buf)

    def WriteFileAndRecv( self, hChannel:int, buf : bytearray ) -> int :
        ret = 0
        if hChannel == ErrorCode.INVALID_HANDLE:
            return -errno.EINVAL
        iSizeLimit = 2 * 1024 * 1024
        if not hChannel in self.m_mapChanToCtx:
            return -errno.ENOENT
        o = self.m_mapChanToCtx[hChannel]
        while True:
            iSize = len(buf)
            if o.m_iBytesLeft < iSize:
                ret = -errno.ERANGE
                break
            if iSize > 0:
                o.m_fp.write(buf)
                o.m_iBytesLeft -= iSize
                o.m_iBytesSent += iSize
                OutputMsg( "Received %d, To receive %d" %
                    ( o.m_iBytesSent, o.m_iBytesLeft ) )

            iSizeRecv = o.m_iBytesLeft
            if iSizeRecv > iSizeLimit:
                iSizeRecv = iSizeLimit
            elif iSizeRecv == 0:
                self.OnTransferDone(hChannel, 0)
                if self.m_oInst.IsServer() :
                    print("send over token")
                    self.m_oInst.DeferCall(
                        self.SendToken, hChannel, "over".encode())
                break

            pret = self.m_oInst.ReadStreamAsync2(
                hChannel, iSizeRecv )
            ret = pret[ 0 ]
            if ret < 0 or ret == ErrorCode.STATUS_PENDING:
                break
            buf = pret[1]
        
        if ret < 0:
            self.OnTransferDone(hChannel, ret)
        
        return ret

    def WriteStmCallback(self, hChannel : int, buf : bytearray) :
        if hChannel == ErrorCode.INVALID_HANDLE or buf is None:
            return
        if not hChannel in self.m_mapChanToCtx:
            return
        o = self.m_mapChanToCtx[ hChannel]
        o.m_iBytesLeft -= len(buf)
        o.m_iBytesSent += len(buf)
        OutputMsg( "Sent %d, To send %d" %
            (o.m_iBytesSent, o.m_iBytesLeft))
        self.ReadFileAndSend(hChannel)

    def ReadStmCallback(self, hChannel : int, buf : bytearray) :
        if hChannel == ErrorCode.INVALID_HANDLE or buf is None:
            return
        self.WriteFileAndRecv(hChannel, buf)

    def SetError( self, hChannel : int, iError : int) :
        if hChannel == ErrorCode.INVALID_HANDLE :
            return
        if not hChannel in self.m_mapChanToCtx:
            return
        o = self.m_mapChanToCtx[ hChannel ]
        o.SetError( iError )

    def GetError( self, hChannel : int ) -> int:
        if hChannel == ErrorCode.INVALID_HANDLE :
            return
        if not hChannel in self.m_mapChanToCtx:
            return -errno.ENOENT
        o = self.m_mapChanToCtx[ hChannel ]
        return o.GetError()

class TransferContextCli( TransferContext ):
    def __init__(self, oInst : PyRpcServices) :
        super().__init__(oInst)
        self.m_sem = tr.Semaphore(0)

    def OnTransferDone(self, hChannel: int, iRet: int):
        if hChannel == ErrorCode.INVALID_HANDLE:
            return
        if not hChannel in self.m_mapChanToCtx:
            return
        o = self.m_mapChanToCtx[ hChannel]
        if o.IsDone() :
            return
        o.SetError( iRet )
        o.CleanUp()
        self.NotifyComplete()

    def NotifyComplete( self ) :
        self.m_sem.release()

    def WaitForComplete( self ) :
        self.m_sem.acquire()
        
