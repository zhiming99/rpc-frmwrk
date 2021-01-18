from rpcfrmwrk import *
from inspect import signature
import numpy as np

import errno
import inspect
import types
import platform
import pickle
from enum import IntEnum
from typing import Union, Callable

class ErrorCode( IntEnum ) :
    STATUS_SUCCESS = np.int32( 0 )
    STATUS_PENDING = np.int32( 0x10001 )
    STATUS_MORE_PROCESS_NEEDED = np.int32( 0x10002 )
    STATUS_CHECK_RESP = np.int32( 0x10003 )
    ERROR_ADDRESS = np.int32( 0x80010002 )
    ERROR_STATE = np.int32( 0x80010003 )
    ERROR_WRONG_THREAD = np.int32( 0x80010004 )
    ERROR_CANNOT_CANCEL = np.int32( 0x80010005 )
    ERROR_PORT_STOPPED = np.int32( 0x80010006 )
    ERROR_FALSE = np.int32( 0x80010007 )
    ERROR_REPEAT = np.int32( 0x80010008 )
    ERROR_PREMATURE = np.int32( 0x80010009 )
    ERROR_NOT_HANDLED = np.int32( 0x8001000a )
    ERROR_CANNOT_COMP = np.int32( 0x8001000b )
    ERROR_USER_CANCEL = np.int32( 0x8001000c )
    ERROR_PAUSED = np.int32( 0x8001000d )
    ERROR_NOT_IMPL = np.int32( 0x8001000e )
    ERROR_CANCEL_INSTEAD = np.int32( 0x8001000f )
    ERROR_QUEUE_FULL = np.int32( 0x8001000e )

def GetNpValue( typeid, val ) :
    if typeid == cpp.typeUInt32 :
        return np.uint32( val )

    elif typeid == cpp.typeUInt64 :
        return np.uint64( val )

    elif typeid == cpp.typeFloat :
        return np.float32( val )

    elif typeid == cpp.typeDouble :
        return np.float64( val )

    elif typeid == cpp.typeUInt16 :
        return np.uint16( val )

    elif typeid ==  cpp.typeUInt8 :
        return np.uint8( val )

    return None

def GetObjType( var ) :
    if ( isinstance( var, np.int32 ) or
        isinstance( var, np.uint32 ) ) :
        return cpp.typeUInt32;
    elif ( isinstance( var, np.int64 ) or
        isinstance( var, np.uint64 ) ) :
        return cpp.typeUInt64
    elif isinstance( var, np.float32 ) :
        return cpp.typeFloat
    elif isinstance( var, np.float64 ) :
        return cpp.typeDouble
    elif ( isinstance( var, np.int16 ) or
        isinstance( var, np.uint16 ) ) :
        return cpp.typeUInt16
    elif ( isinstance( var, np.int8 ) or
        isinstance( var, np.uint8 ) ) :
        return cpp.typeUInt8
    elif isinstance( var, str ) :
        return cpp.typeString
    elif isinstance( var, cpp.ObjPtr ) :
        return cpp.typeObj
    elif isinstance( var, bytearray ) :
        return cpp.typeByteArr
    elif isinstance( var, bytes ) :
        return cpp.typeByteArr
    elif isinstance( var, np.bool ) :
        return cpp.typeByte
    elif isinstance( var, cpp.BufPtr ) :
        return var.GetExDataType()
    elif isinstance( var, int ) :
        arch = platform.architecture()[ 0 ];
        if arch == '32bit' :
            return cpp.typeUInt32
        elif arch == '64bit' :
            return cpp.typeUInt64
    elif isinstance( var, float ) :
        return cpp.typeDouble
    return cpp.typeNone;

def GetTypeObj( typeid ) :
    if typeid == cpp.typeUInt32 :
        return np.uint32

    elif typeid == cpp.typeUInt64 :
        return np.uint64

    elif typeid == cpp.typeFloat :
        return np.float32

    elif typeid == cpp.typeDouble :
        return np.float64

    elif typeid == cpp.typeUInt16 :
        return np.uint16

    elif typeid ==  cpp.typeUInt8 :
        return np.uint8

    elif typeid == cpp.typeString :
        return str

    elif typeid == cpp.typeObj :
        return cpp.ObjPtr

    elif typeid == cpp.typeByteArr :
        return bytearray

    elif typeid == cpp.typeByte :
        return bool
    return None

class PyRpcContext :
    def CreateIoMgr( self, strModName ) :
        cpp.CoInitialize(0)
        p1=cpp.CParamList()
        ret = p1.SetStrProp( 0, strModName );
        if ret < 0 :
            return None
        p2 = cpp.CastToCfg( p1.GetCfgAsObj() );
        p3=cpp.CreateObject(
            cpp.Clsid_CIoManager, p2 )
        return p3

    def StartIoMgr( self, pIoMgr ) :
        p = cpp.CastToSvc( pIoMgr );
        if p is None :
            return -errno.EFAULT
        ret = p.Start();
        return ret

    def StopIoMgr( self, pIoMgr ) :
        p = cpp.CastToSvc( pIoMgr );
        if p is None :
            return -errno.EFAULT
        ret = p.Stop();
        return ret

    def CleanUp( self ) :
        cpp.CoUninitialize();

    def DestroyRpcCtx( self ) :
        print( "proxy.py: about to quit..." )
        self.CleanUp();

    def __init__( self, strModName= "PyRpcProxy" ) :
        self.strModName = strModName

    def Start( self, strModName="PyRpcProxy" ) :
        ret = 0
        print( "entering..." );
        self.pIoMgr = self.CreateIoMgr( strModName );
        if self.pIoMgr is not None :
            ret = self.StartIoMgr( self.pIoMgr );
            if ret > 0 :
                self.StopIoMgr( self.pIoMgr );
            else :
                ret = LoadPyFactory( self.pIoMgr );
        return ret

    def Stop( self ) :
        print( "exiting..." );
        if self.pIoMgr is not None :
            self.StopIoMgr( self.pIoMgr );
        return self.DestroyRpcCtx();

    def __enter__( self ) :
        self.Start( self.strModName )

    def __exit__( self, type, val, traceback ) :
        self.Stop();

class PyRpcServices :
    def GetError( self ) :
        if self.iError is None :
            return 0
        return self.iError

    def SetError( self, iErr ) :
        self.iError = iErr;

    def Start( self ) :
        self.oInst.SetPyHost( self );
        ret = self.oInst.Start()
        if ret < 0 :
            print( "Failed start proxy..." )
            return ret;
        else :
            print( "Proxy started..." )

        oCheck = self.oInst.GetPyHost()
        return 0

    def Stop( self ) :
        self.oInst.RemovePyHost();
        return self.oInst.Stop()

    def __enter__( self ) :
        self.Start()

    def __exit__( self, type, val, traceback ) :
        self.Stop()

    def TimerCallback( self, callback, context )->None :
        sig =signature( callback )
        iCount = len( sig.parameters );
        if iCount != 2 :
            print( "TimerCallback: params number",
                "does not match,", iCount )
            return
        ret = callback( self, context )
        return

    ''' create a timer object, which is due in
    timeoutSec seconds. And the `callback' will be
    called with the parameter context.

    it returns a two element list, the first
    is the error code, and the second is a
    timerObj if the error code is
    STATUS_SUCCESS.
    '''
    def AddTimer( self, timeoutSec,
        callback, context )->[int,cpp.ObjPtr] :
        return self.oInst.AddTimer(
            np.uint32( timeoutSec ),
            callback, context )

    '''Remove a previously scheduled timer.
    '''
    def DisableTimer( self, timerObj ) :
        return self.oInst.DisableTimer(
            timerObj );

    '''attach a complete notification to the task
    to complete. When the task is completed or
    canceled, this notification will be triggered.
    the notification has the following layout
    callback( self, ret, *listArgs ), as is the
    same as the response callback. The `ret' is
    the error code of the task.
    '''
    def InstallCompNotify(
        self, task, callback, *listArgs ):
        listResp = [ 0, [ *listArgs ] ]
        self.oInst.InstallCompNotify(
            task, callback, listResp )

    def HandleAsyncResp( self,
        callback, seriProto, listResp ) :
        listArgs = []
        sig =signature( callback )
        iArgNum = len( sig.parameters ) - 2
        for i in range( iArgNum ):
            listArgs.append( None );

        if listResp is None :
            print( "HandleAsyncResp: bad response" )
            callback( self, -cpp.EBADMSG, *listArgs )
            return

        if iArgNum < 0 :
            print( "HandleAsyncResp: bad callback")
            callback( self, -cpp.EBADMSG, *listArgs )
            return

        ret = listResp[ 0 ];
        if seriProto == cpp.seriNone :
            if len( listResp ) <= 1 :
                callback( self, ret, *listArgs )
            elif isinstance( listResp[ 1 ], list ) :
                callback( self, ret, *listResp[ 1 ] )
            else :
                callback( self, ret, *listArgs )
        elif seriProto == cpp.seriPython :
            if len( listResp ) <= 1 :
                callback( self, ret, *listArgs )
            elif isinstance( listResp[ 1 ], bytearray ) :
                try:
                    trueResp = pickle.loads( listResp[ 1 ] )
                    if isinstance( trueResp, list ) :
                        callback( self, ret, *trueResp )
                except PickleError:
                    callback( self, -errno.EBADMSG, *listArgs )
            else :
                callback( self, -errno.EBADMSG, *listArgs )

        return

    def GetObjType( self, pObj ) :
        return GetObjType( pObj )

    def GetNumpyValue( typeid, val ) :
        return GetNpValue( typeid, val )

    def StartStream( self, pDesc ) :
        return self.oInst.StartStream( pDesc )

    def CloseStream( self, hChannel ) :
        return self.oInst.CloseStream( hChannel )

    def WriteStream( self, hChannel, pBuf ) :
        return self.oInst.WriteStream( hChannel, pBuf )

    def WriteStreamAsync( self,
        hChannel, pBuf, callback )->int :
        return self.oInst.WriteStreamAsync(
            hChannel, pBuf, callback )

    '''ReadStream to read `size' bytes from the
    stream `hChannel'.  If `size' is zero, the
    number of bytes read depends on the first
    message received. If you are not sure what
    will come from the stream, you are recommended
    to use zero size.

    The return value is a list of 3 elements,
    element 0 is the error code. If it is
    negative, no data is returned. If it is
    STATUS_SUCCESS, element 1 is a bytearray
    object read from the stream. element 2 is a
    hidden object to keep element 1 valid, just
    leave it alone.
    '''
    def ReadStream( self, hChannel, size = 0 ) :
        tupRet = self.oInst.ReadStream( hChannel, size )
        ret = tupRet[ 0 ]
        if ret < 0 :
            return tupRet;
        #elem 1 is a BufPtr object on success
        pBuf = tupRet[ 1 ]
        #transfer the content to a bytes object
        pBytes = pBuf.TransToBytes();
        if pBytes == None :
            return ( -errno.ENODATA, )
        return ( 0, pBytes, pBuf )

    '''
    ReadStreamAsync to read `size' bytes from the
    stream `hChannel', and give control to
    callback after the data is read from the
    stream. If `size' is zero, the number of bytes
    read depends on the first message in the
    pending message queue. If you are not sure
    what will come from the stream, you are
    recommended to use zero size.

    The return value is a list of 3 elements,
    element 0 is the error code. If it is
    STATUS_PENDING, there is no data in the
    pending message queue, and wait till the
    callback is called in the future time.

    If the return value is STATUS_SUCCESS, element
    1 holds a bytearray object of the bytes read
    from the stream. And element 2 is a hidden
    object, that you don't want to access.
    Actually it controls the life time of element
    1, so make sure not to access elemen 1 if
    element2 is no longer valid.  '''

    def ReadStreamAsync( self,
        hChannel, callback, size = 0)
        ->[int, optional[ bytearray ], optional[ cpp.ObjPtr] ]:

        return self.oInst.ReadStreamAsync(
            hChannel, callback, size )

    '''event called when the stream `hChannel' is
    ready '''
    def OnStmReady( self, hChannel ) :
        pass

    '''event called when the stream `hChannel' is
    about to close '''
    def OnStmClosing( self, hChannel ) :
        pass

    '''Convert the arguments in the `pObj' c++
    object to a list of python object.

    The return value is a two element list. The
    first element is the error code and the second
    element is a list of python object. The second
    element should be None if error occurs during
    the conversion.  '''

    def ArgObjToList( self,
        seriProto:int,
        pObj:cpp.ObjPtr )->[ int, list ] :

        pCfg = cpp.CastToCfg( pObj )
        if pCfg is None :
            return [ -errno.EFAULT, ]
        oParams = cpp.CParamList( pCfg )
        ret = oParams.GetSize()
        if ret[ 0 ] < 0 :
            resp[ 0 ] = ret[ 0 ];
        iSize = ret[ 1 ]

        if iSize < 0 :
            return [ -errno.EINVAL, ]

        if seriProto == cpp.seriPython :
            ret = pCfg.GetProperty( 0 );
            if ret[ 0 ] < 0 :
                return [ ret[ 0 ], ]
            val = ret[ 1 ]
            if val.IsEmpty() :
                return [ -errno.EFAULT, ]

            pyBuf = bytearray( val.size() );
            iRet = val.CopyToPython( pyBuf );
            if iRet < 0 : 
                return [ iRet, ]

            try :
                argList = pickle.loads( pyBuf )
            except PickleError :
                return [ -errno.EBADMSG, ]

            if not isinstance( argList, list ):
                return [ -errno.EBADMSG, ]

            return [ 0, argList ]
        elif seriProto != cpp.seriNone :
            return [ -errno.EBADMSG, ]
            
        argList = []
        for i in range( iSize ) :
            ret = oParams.GetPropertyType( i )
            if ret[ 0 ] < 0 :
                break
            iType = ret[ 1 ];
            if iType == cpp.typeUInt32 :
                ret = oParams.GetIntProp( i )
                if ret[ 0 ] < 0 :
                    break
                val = ret[ 1 ]
            elif iType == cpp.typeDouble :
                ret = oParams.GetDoubleProp( i )
                if ret[ 0 ] < 0 :
                    break
                val = ret[ 1 ]

            elif iType == cpp.typeByte :
                ret = oParams.GetByteProp( i )
                if ret[ 0 ] < 0 :
                    break
                val = ret[ 1 ]

            elif iType == cpp.typeUInt16 :
                ret = oParams.GetShortProp( i )
                if ret[ 0 ] < 0 :
                    break
                val = ret[ 1 ]

            elif iType == cpp.typeUInt64 :
                ret = oParams.GetQwordProp( i )
                if ret[ 0 ] < 0 :
                    break
                val = ret[ 1 ]

            elif iType == cpp.typeFloat :
                ret = oParams.GetFloatProp( i )
                if ret[ 0 ] < 0 :
                    break
                val = ret[ 1 ]

            elif iType == cpp.typeString :
                ret = oParams.GetStrProp( i )
                if ret[ 0 ] < 0 :
                    break
                val = ret[ 1 ]

            elif iType == cpp.typeObj :
                ret = oParams.GetObjPtr( i )
                if ret[ 0 ] < 0 :
                    break
                val = ret[ 1 ]
                if val.IsEmpty() :
                    ret = [ -errno.EFAULT, ]
                    break
            elif iType == cpp.typeByteArr :
                ret = pCfg.GetProperty( i );
                if ret[ 0 ] < 0 :
                    break
                val = ret[ 1 ]
                if val.IsEmpty() :
                    ret = [ -errno.EFAULT, ]
                    break
                pyBuf = bytearray( val.size() );
                iRet = val.CopyToPython( pyBuf );
                if iRet < 0 : 
                    ret = [ iRet, ]
                    break

                val = pyBuf;
            argList.append( val )

        if ret[ 0 ] < 0 :
            return ret;

        return [ 0, argList ]

    #for event handler 
    def InvokeMethod( self, callback,
        ifName, methodName,
        seriProto:int,
        cppargs:cpp.ObjPtr ) ->[ int, list ]:
        resp = [ 0 ];
        while True :

            ret = self.ArgObjToList( seriProto, cppargs )
            if ret[ 0 ] < 0 :
                resp[ 0 ] = ret;
                break

            argList = ret[ 1 ]
            found = False
            for iftype in type(self).__bases__ :
                if not hasattr( iftype, "ifName" ) :
                    prevType = iftype
                    continue;
                if iftype.ifName != ifName :
                    prevType = iftype
                    continue;
                found = True
                typeFound = iftype;
                break
            if not found :
                resp[ 0 ] = -errno.ENOTSUP;
                break

            isServer = self.oInst.IsServer();
            nameComps = methodName.split( '_' )
            if not isServer :
                if nameComps[ 0 ] != "UserEvent" :
                    resp[ 0 ] = -errno.EINVAL;
                    break
            else :
                if nameComps[ 0 ] != "UserMethod" :
                    resp[ 0 ] = -errno.EINVAL;
                    break

            found = False
            oMembers = inspect.getmembers( typeFound,
                predicate=inspect.isfunction);
            for oMethod in oMembers :
                if nameComps[ 1 ] != oMethod[ 0 ]: 
                    continue;
                targetMethod = oMethod[ 1 ];
                found = True;               
                break

            if not found :
                resp[ 0 ] = -errno.EINVAL;
                break

            if targetMethod is None :
                resp[ 0 ] = -error.EFAULT;
                break

            resp = targetMethod( self, callback, *argList )
            if seriProto == cpp.seriNone :
                break;

            if seriProto != cpp.seriPython :
                resp[ 0 ] = -errno.ENOTSUP;
                break;

            ret = resp[ 0 ]
            if ret < 0 :
                break

            if len( resp ) <= 1 : 
                break;

            if not isinstance( resp[ 1 ], list ) :
                resp[ 0 ] = -errno.EBADMSG;
                break;

            listResp = resp[ 1 ]
            pBuf = pickle.dumps( listResp )
            resp[ 1 ] = [ pBuf, ]
            break #while True

        return resp

    ''' run the callback on a new context instead
    of the current context
    '''
    def DeferCall( self, callback, *args ) :
        self.oInst.DeferCall( callback, *args )

    def DeferCallback( self, callback, *args )
        callback( self, *args )
    
class PyRpcProxy( PyRpcServices ) :

    def __init__( self, pIoMgr, strDesc, strSvrObj ) :
        self.pIoMgr = pIoMgr;
        self.iError = 0
        oParams = cpp.CParamList();
        oObj = CreateProxy( pIoMgr,
            strDesc, strSvrObj,
            oParams.GetCfgAsObj() );
        if oObj is None or oObj.IsEmpty() :
            print( "failed to create proxy..." )
            self.SetError( -errno.EFAULT );
            return

        oInst = CastToProxy( oObj );
        if oInst is None :
            print( "failed to create proxy 2..." )
        self.oInst = oInst;
        self.oObj = oObj;
        return

    def sendRequest( self,
        strIfName, strMethod, *args )->[int,list] :
        resp = [ 0, None ];

        listArgs = list( args )
        self.MakeCall( strIfName, strMethod,
            listArgs, resp, cpp.seriNone )

        return resp;

    ''' sendRequestAsync returns a tuple with two
    elements, 0 is the return code, and 1 is a
    np.int64 as a taskid, which can be used to
    cancel the request sent'''
    def sendRequestAsync( self,
        callback, strIfName,
        strMethod, *args )->[int, np.uint64]:

        resp = [ 0, None ]
        listArgs = list( args )
        tupRet = self.MakeCallAsync( callback,
            strIfName, strMethod, listArgs, resp,
            cpp.seriNone )

        return tupRet

    '''Send a request with pickle as the
    serialization protocol
    '''
    def PySendRequest(
        self, strIfName,
        strMethod, *args )->[ int, list] :

        resp = [ 0, None ];
        listArgs = list( args )
        pBuf = pickle.dumps( listArgs )
        ret = self.MakeCall( strIfName, strMethod,
            [ pBuf, ], resp, cpp.seriPython )
        if len( resp ) <= 1 :
            return resp

        if pResp[ 0 ] == 0 :
            pPyBuf = resp[ 1 ][ 0 ]
            if not isinstance( pPyBuf, bytearray ) :
                resp[ 0 ] = -errno.EBADMSG
                return resp
            listArgs = pickle.loads( pPyBuf )
            if not isinstance( listArgs, list ) :
                resp[ 0 ] = -errno.EBADMSG
                return resp
            resp[ 1 ] = listArgs;

        return resp;

    def PySendRequestAsync(
        self, callback,
        strIfName, strMethod,
        *args )->Union[[ int, int ],[int,list]]:

        resp = [ 0, None ]
        listArgs = list( args )
        pBuf = pickle.dumps( listArgs )
        tupRet = self.MakeCallAsync( callback,
            strIfName, strMethod, [ pBuf, ],
            resp, cpp.seriPython )
        ret = tupRet[ 0 ]
        if ret == ErrorCode.STATUS_PENDING :
            return tupRet

        if ret < 0 :
            return tupRet

        if len( resp ) <= 1 :
            return tupRet

        if ret == 0 :
            pPyBuf = resp[ 1 ][ 0 ]
            if not isinstance( pPyBuf, bytearray ) :
                tupRet[ 0 ] = -errno.EBADMSG
                return tupRet
            listArgs = pickle.loads( pPyBuf )
            if not isinstance( listArgs, list ) :
                tupRet[ 0 ] = -errno.EBADMSG
                return tupRet

            tupRet[ 1 ] = listArgs

        return tupRet

    def MakeCall(
        self, strIfName,
        strMethod, args,
        resp, seriProto )->int :

        self.oInst.PyProxyCallSync(
            strIfName, strMethod, args,
            resp, seriProto )

        return resp

    def MakeCallAsync(
        self, callback,
        strIfName, strMethod, args,
        resp, seriProto )->[int,int]:

        ret = self.oInst.PyProxyCall( callback,
            strIfName, strMethod, args, resp, seriProto )

        return ret;

class PyRpcServer( PyRpcServices ) :

    def __init__( self, pIoMgr, strDesc, strSvrObj ) :
        self.pIoMgr = pIoMgr;
        self.iError = 0
        oParams = cpp.CParamList();
        oObj = CreateRpcServer( pIoMgr,
            strDesc, strSvrObj,
            oParams.GetCfgAsObj() );
        if oObj is None or oObj.IsEmpty() :
            print( "failed to create server..." )
            self.SetError( -errno.EFAULT );
            return

        oInst = CastToServer( oObj );
        if oInst is None :
            print( "failed to create server 2..." )
        self.oInst = oInst;
        self.oObj = oObj;
        return

    def SetChanCtx( self, hChannel, oContext ) :
        return self.oInst.SetChanCtx(
            hChannel, oContext );

    def GetChanCtx( self, hChannel ) :
        return self.oInst.GetChanCtx( hChannel );

    '''callback should be the one passed to the
    InvokeMethod. This is for asynchronous invoke,
    that is, the invoke method returns pending and
    the task will complete later in the future
    with a call to this method. the callback is
    the glue between InvokeMethod and
    OnServiceComplete.
    '''
    def OnServiceComplete(
        self, callback, ret, *args ) :
        listResp = list( args )
        return self.oInst.OnServiceComplete(
            callback, ret, cpp.seriNone, listResp )

    def SetResponse(
        self, callback, ret, *args ) :
        listResp = list( args )
        return self.oInst.SetResponse(
            callback, ret, cpp.seriNone, listResp )

    def PyOnServiceComplete(
        self, callback, ret, *args ) :
        listResp = list( args )
        pBuf = pickle.dumps( listResp )
        return self.oInst.OnServiceComplete(
            callback, ret, cpp.seriPython,
            [ pBuf, ] )

    def PySetResponse(
        self, callback, ret, *args ) :
        listResp = list( args )
        pBuf = pickle.dumps( listResp )
        return self.oInst.SetResponse(
            callback, ret, cpp.seriPython,
            [ pBuf, ] )

    ''' callback can be none if it is not
    necessary to get notified of the completion.
    destName can be none if not needed.
    '''
    def SendEvent( self, callback,
        ifName, evtName, destName, *args ) :
        evtName = "UserEvent_" + evtName
        return self.oInst.SendEvent( callback,
            ifName, evtName, destName, [ *args ],
            cpp.seriNone )

    ''' this is a python native event sender and
    the receiver must be python proxy only.  but
    it can accept parameters of complex types '''
    def PySendEvent( self, callback,
        ifName, evtName, destName, *args ) :
        evtName = "UserEvent_" + evtName
        listArgs = [ *args ]
        pBuf = pickle.dump( listArgs )
        return self.oInst.SendEvent( callback,
            ifName, evtName, destName, [ pBuf, ],
            cpp.seriPython )
