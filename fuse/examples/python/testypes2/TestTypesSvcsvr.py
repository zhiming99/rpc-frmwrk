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
import select
class CITestTypessvr( IITestTypes_SvrImpl ):
    '''
    Synchronous request handler
    within which to run the business logic.
    Returning Err.STATUS_PENDING for an
    asynchronous operation and returning other
    error code will complete the request. if the
    method returns STATUS_PENDING, make sure to
    call "self.EchoCompleteCb" later to
    complete the request. Otherwise, the client
    will get a timeout error. The return value is a
    list, with the first element as error code and
    the second element as a list of the response
    parameters.
    '''
    def Echo( self, reqId : object,
        strText : str
        ) -> Tuple[ int, list ] :
        #Implement this method here
        return [ 0, [ strText ] ]
        
    '''
    Synchronous request handler
    within which to run the business logic.
    Returning Err.STATUS_PENDING for an
    asynchronous operation and returning other
    error code will complete the request. if the
    method returns STATUS_PENDING, make sure to
    call "self.EchoByteArrayCompleteCb" later to
    complete the request. Otherwise, the client
    will get a timeout error. The return value is a
    list, with the first element as error code and
    the second element as a list of the response
    parameters.
    '''
    def EchoByteArray( self, reqId : object,
        pBuf : bytearray
        ) -> Tuple[ int, list ] :
        #Implement this method here
        return [ 0, [ pBuf ] ]
        
    '''
    Synchronous request handler
    within which to run the business logic.
    Returning Err.STATUS_PENDING for an
    asynchronous operation and returning other
    error code will complete the request. if the
    method returns STATUS_PENDING, make sure to
    call "self.EchoArrayCompleteCb" later to
    complete the request. Otherwise, the client
    will get a timeout error. The return value is a
    list, with the first element as error code and
    the second element as a list of the response
    parameters.
    '''
    def EchoArray( self, reqId : object,
        arrInts : list
        ) -> Tuple[ int, list ] :
        #Implement this method here
        return [ 0, [ arrInts ] ]
        
    '''
    Synchronous request handler
    within which to run the business logic.
    Returning Err.STATUS_PENDING for an
    asynchronous operation and returning other
    error code will complete the request. if the
    method returns STATUS_PENDING, make sure to
    call "self.EchoMapCompleteCb" later to
    complete the request. Otherwise, the client
    will get a timeout error. The return value is a
    list, with the first element as error code and
    the second element as a list of the response
    parameters.
    '''
    def EchoMap( self, reqId : object,
        mapReq : map
        ) -> Tuple[ int, list ] :
        #Implement this method here
        return [ 0, [ mapReq ] ]
        
    '''
    Synchronous request handler
    within which to run the business logic.
    Returning Err.STATUS_PENDING for an
    asynchronous operation and returning other
    error code will complete the request. if the
    method returns STATUS_PENDING, make sure to
    call "self.EchoManyCompleteCb" later to
    complete the request. Otherwise, the client
    will get a timeout error. The return value is a
    list, with the first element as error code and
    the second element as a list of the response
    parameters.
    '''
    def EchoMany( self, reqId : object,
        i1 : int,
        i2 : int,
        i3 : int,
        i4 : float,
        i5 : float,
        szText : str
        ) -> Tuple[ int, list ] :
        #Implement this method here
        return [ 0, [ i1, i2, i3+1, i4 + 2.0, i5 + 5.0, szText ] ]
        
    '''
    Synchronous request handler
    within which to run the business logic.
    Returning Err.STATUS_PENDING for an
    asynchronous operation and returning other
    error code will complete the request. if the
    method returns STATUS_PENDING, make sure to
    call "self.EchoStructCompleteCb" later to
    complete the request. Otherwise, the client
    will get a timeout error. The return value is a
    list, with the first element as error code and
    the second element as a list of the response
    parameters.
    '''
    def EchoStruct( self, reqId : object,
        fi : object
        ) -> Tuple[ int, list ] :
        #Implement this method here
        return [ 0, [ fi ] ]
        
    '''
    Synchronous request handler
    within which to run the business logic.
    Returning Err.STATUS_PENDING for an
    asynchronous operation and returning other
    error code will complete the request. if the
    method returns STATUS_PENDING, make sure to
    call "self.EchoNoParamsCompleteCb" later to
    complete the request. Otherwise, the client
    will get a timeout error. The return value is a
    list, with the first element as error code and
    the second element as a list of the response
    parameters.
    '''
    def EchoNoParams( self, reqId : object
        ) -> Tuple[ int, None ] :
        return [ 0, None ]
        
    '''
    Synchronous request handler
    within which to run the business logic.
    Returning Err.STATUS_PENDING for an
    asynchronous operation and returning other
    error code will complete the request. if the
    method returns STATUS_PENDING, make sure to
    call "self.EchoStreamCompleteCb" later to
    complete the request. Otherwise, the client
    will get a timeout error. The return value is a
    list, with the first element as error code and
    the second element as a list of the response
    parameters.
    '''
    def EchoStream( self, reqId : object,
        hstm : str
        ) -> Tuple[ int, list ] :
        #open the stream
        stmPath = self.m_strPath + "/streams/" + hstm
        stmfp = open( stmPath, "r+b", buffering=0 )
        inputs = [stmfp]
        inBuf = bytearray()
        #read a 8kb block
        size = 8 * 1024
        while size > 0 :
            notifylist = select.select( inputs, [], [] )
            data = stmfp.read(8*1024)
            inBuf.extend( data )
            size -= len( data )
        pos = len(inBuf) - 2
        inBuf[ pos ] = 0x41
        inBuf[ pos + 1 ] = 0x42
        #send back with some changes at the tail bytes
        stmfp.write(inBuf)
        stmfp.close()
        return [ 0, [hstm] ]
        
    
class CTestTypesSvcServer(
    CITestTypessvr ) :
    def __init__( self, strSvcPoint : str, num : int ) :
        if num is None:
            num = 0
        error = 0
        self.m_strPath = strSvcPoint
        reqFile = strSvcPoint + "/jreq_" + str( num )
        self.m_reqFp = open( reqFile, "rb" )
        respFile = strSvcPoint + "/jrsp_" + str( num )
        self.m_respFp = open( respFile, "wb" )
        
    def sendResp( self, oResp : object ) -> int:
        return iolib.sendResp( self.m_respFp, oResp )
    
    def sendEvent( self, oEvent : object ) -> int:
        return iolib.sendEvent( self.m_respFp, oEvent )
    
    def recvReq( self ) -> Tuple[ int, object ]:
        return iolib.recvReq( self.m_reqFp )
    
    def getReqFp( self ) ->  object:
        return self.m_reqFp
    
    def OnKeepAlive( self, reqId : object ) -> int:
        if reqId is None:
            return -errno.EINVAL
        oResp = dict()
        oResp[ "RequestId" ] = reqId
        oResp[ "Method" ] = "OnKeepAlive"
        oResp[ "Interface" ] = "IInterfaceServer"
        oResp[ "MessageType" ] = "evt"
        return self.sendEvent( oResp )
    
    def DispatchMsg( self, oReq : dict ):
        try:
            if "ITestTypes" == oReq[ "Interface" ] :
                IITestTypes_SvrImpl.DispatchIfMsg( self, oReq )
                return
            if "IInterfaceServer" == oReq[ "Interface" ] and "UserCancelRequest" == oReq[ "Method" ] :
                reqId = oReq[ "RequestId" ]
                oParams = oReq[ "Parameters" ]
                reqIdToCancel = oParams[ "RequestId" ]
                self.UserCancelRequest( reqId, reqIdToCancel )
                return
            
        except Exception as err:
            print( err )
            return
        
    def UserCancelRequest( self, reqId : object,
        reqIdToCancel : object ) -> int:
        # change this function for customized behavor
        print( "request", reqIdToCancel, " is canceled" )
        oParams = dict()
        oParams[ "RequestId" ] = reqIdToCancel
        oResp = BuildReqHeader( reqId,
            "UserCancelRequest",
            "IInterfaceServer",
            0, False, True )
        oResp[ "Parameters" ] = oParams
        return self.sendResp( oResp )
    
