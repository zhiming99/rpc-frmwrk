import os
import errno
#from numpy import true_divide
from rpcf.proxy import *

class TransFileContext :
    def __init__(self, strPath : str = "") :
        self.m_iError = 0
        self.m_iBytesLeft = 0
        self.m_iBytesSent = 0
        self.m_bServer = False
        self.m_strPath = strPath
        self.m_iOffset = 0
        self.m_iSize = 0
        self.m_bRead = False
        self.m_fp = None
        self.m_cDirection='u'

    def SetError( self, iError ):
        self.m_iError = iError

    def GetError( self ) -> int:
        return self.m_iError

    def IsDone( self ) -> bool:
        return self.m_fp == None

    def GetFileName(self) ->str:
        return os.path.basename(self.m_strPath)

    def GetDirName(self)->str:
        return os.path.dirname(self.m_strPath)

    def OpenFile(self)->int:
        if self.m_strPath is None or self.m_strPath == "":
            return ErrorCode.ERROR_STATE
        ret = 0
        try:
            while True:
                if ( self.m_cDirection == 'u' and not self.m_bServer ) or (
                    self.m_cDirection == 'd' and self.m_bServer ) :
                    self.m_bRead = True
                    self.m_fp = open(self.m_strPath, "rb")
                    iSize = self.m_fp.seek( 0, os.SEEK_END )
                    self.m_fp.seek(0, os.SEEK_SET)
                    if self.m_iOffset >= iSize :
                        ret = -errno.ERANGE
                        break
                    if self.m_iOffset > 0:
                        self.m_fp.seek(self.m_iOffset, os.SEEK_SET)
                    if self.m_iSize > 0 :
                        if self.m_iSize > iSize - self.m_iOffset:
                            self.m_iSize = iSize = self.m_iOffset
                    else :
                        self.m_iSize = iSize - self.m_iOffset

                    if self.m_iSize > 128 * 1024 * 1024 :
                        ret = -errno.ERANGE
                        break
                else :
                    if self.m_iSize == 0 :
                        ret = -errno.ERANGE
                        break

                    self.m_fp = open(self.m_strPath, "wb+")
                    self.m_fp.seek(0, os.SEEK_END)
                    self.m_fp.truncate(self.m_iOffset)

                self.m_iBytesLeft = self.m_iSize
                break

        except Exception as e:
            print(str(e))

        return ret

    def CleanUp(self):
        if self.m_fp is not None:
            self.m_fp.close()
            self.m_fp = None
