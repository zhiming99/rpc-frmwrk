# Generated by RIDLC, make sure to backup before recompile
# ridlc -spf -O . ../../../../examples/testypes.ridl 
import sys
from rpcf import iolib
from rpcf import serijson
from rpcf.serijson import Variant
import errno
from rpcf.proxy import ErrorCode as Err
from typing import Union, Tuple, Optional
from TestTypesstructs import *
from ifimpl import *
import random
class CITestTypescli( IITestTypes_CliImpl ):
    '''
    Event handler
    Add code here to process the event
    '''
    def OnHelloWorld( self, evtId : int,
        strMsg : str
        ) :
        print( "OnHelloWorld event comes %s" % strMsg )
        return
    
class CTestTypesSvcProxy(
    CITestTypescli) :
    def __init__( self, strSvcPoint : str, num : int ) :
        if num is None:
            num = 0
        self.m_strPath = strSvcPoint
        reqFile = strSvcPoint + "/jreq_" + str( num )
        self.m_reqFp = open( reqFile, "wb" )
        respFile = strSvcPoint + "/jrsp_" + str( num )
        self.m_respFp = open( respFile, "rb" )
        evtFile = strSvcPoint + "/jevt_" + str( num )
        self.m_evtFp = open( evtFile, "rb" )
        
    def sendReq( self, oReq : object ) -> int:
        return iolib.sendReq( self.m_reqFp, oReq )
    
    def recvResp( self ) -> Tuple[ int, object ]:
        return iolib.recvResp( self.m_respFp )
    
    def getRespFp( self ) -> object:
        return self.m_respFp
    
    def getEvtFp( self ) ->  object:
        return self.m_evtFp
    
    def DispatchMsg( self, oResp : dict ):
        try:
            if "ITestTypes" == oResp[ "Interface" ] :
                IITestTypes_CliImpl.DispatchIfMsg( self, oResp )
                return
            
        except Exception as err:
            print( err )
            return
        
    
