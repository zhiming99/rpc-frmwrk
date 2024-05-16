'''
 * =====================================================================================
 *
 *       Filename:  proxy.py
 *
 *    Description:  implementations of PyRpcProxy and PyRpcServer
 *
 *        Version:  1.0
 *        Created:  11/04/2020 01:10:51 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2021 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
''' 
from rpcf.rpcbase import *
from inspect import signature
import numpy as np

import errno
import inspect
import types
import platform
import pickle
import os
from enum import IntEnum
from typing import Union, Tuple, Optional

class ErrorCode( IntEnum ) :
    INVALID_HANDLE = np.int32( 0 )
    STATUS_SUCCESS = np.int32( 0 )
    STATUS_PENDING = np.int32( 0x10001 )
    STATUS_MORE_PROCESS_NEEDED = np.int32( 0x10002 )
    STATUS_CHECK_RESP = np.int32( 0x10003 )
    ERROR_FAIL = np.int32( np.uint32( 0x80000001 ) )
    ERROR_ADDRESS = np.int32( np.uint32( 0x80010002 ) )
    ERROR_STATE = np.int32( np.uint32( 0x80010003 ) )
    ERROR_WRONG_THREAD = np.int32( np.uint32( 0x80010004 ) )
    ERROR_CANNOT_CANCEL = np.int32( np.uint32( 0x80010005 ) )
    ERROR_PORT_STOPPED = np.int32( np.uint32( 0x80010006 ) )
    ERROR_FALSE = np.int32( np.uint32( 0x80010007 ) )
    ERROR_REPEAT = np.int32( np.uint32( 0x80010008 ) )
    ERROR_PREMATURE = np.int32( np.uint32( 0x80010009 ) )
    ERROR_NOT_HANDLED = np.int32( np.uint32( 0x8001000a ) )
    ERROR_CANNOT_COMP = np.int32( np.uint32( 0x8001000b ) )
    ERROR_USER_CANCEL = np.int32( np.uint32( 0x8001000c ) )
    ERROR_PAUSED = np.int32( np.uint32( 0x8001000d ) )
    ERROR_NOT_IMPL = np.int32( np.uint32( 0x80010010 ) )
    ERROR_CANCEL_INSTEAD = np.int32( np.uint32( 0x8001000f ) )
    ERROR_QUEUE_FULL = np.int32( np.uint32( 0x8001000e ) )

def DebugPrint( strMsg : str, logLevel = 3 ) :
    PyDbgPrint( strMsg.encode(), logLevel )

def OutputMsg( strMsg : str ) :
    PyOutputMsg( strMsg.encode() )

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
        return cpp.typeUInt32
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
        arch = platform.architecture()[ 0 ]
        if arch == '32bit' :
            return cpp.typeUInt32
        elif arch == '64bit' :
            return cpp.typeUInt64
    elif isinstance( var, float ) :
        return cpp.typeDouble
    return cpp.typeNone

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

def UpdateObjDesc(
    strDesc : str, params : dict )->Tuple[ int, str ]:
    try:
        ret = 0
        oCfg = cpp.CParamList()
        if 'Role' in params:
            oCfg.SetIntProp( 101, params[ 'Role' ] )
        else:
            ret = -errno.EINVAL
            raise Exception(
                "Error UpdateObjDesc cannot find router role" )
        if 'bAuth' in params:
            oCfg.SetBoolProp( 102, params[ 'bAuth' ] )
        if 'AppName' in params:
            oCfg.SetStrProp( 104, params[ 'AppName' ] )
        else:
            ret = -errno.EINVAL
            raise Exception(
                "Error UpdateObjDesc cannot find AppName" )
        if 'instname' in params and 'sainstname' in params:
            ret = -errno.EINVAL
            raise Exception(
                "Error specifying both 'instname' and 'sainstname'" )

        if 'instname' in params:
            oCfg.SetStrProp( 107, params[ 'instname' ] )

        if 'sainstname' in params:
            oCfg.SetStrProp( 108, params[ 'sainstname' ] )

        if 'ipaddr' in params:
            oCfg.SetStrProp( 109, params[ 'ipaddr' ] )

        if 'portnum' in params:
            oCfg.SetStrProp( 110, params[ 'portnum' ] )

        p2 = oCfg.GetCfg()
        resp = cpp.UpdateObjDesc( strDesc, p2 )
        if not 'instname' in params and not 'sainstname' in params:
            ret, instName = oCfg.GetStrProp( 107 )
            if ret < 0:
                raise Exception(
                    "Error find 'instname' in response" )
            params[ 'instname' ] = instName
        return resp
    except Exception as err:
        if ret == 0:
            ret = -errno.EFAULT
            return ( ret, None )

def UpdateDrvCfg(
    strDriver : str, params : dict )->Tuple[ int, str ]:
    try:
        ret = 0
        oCfg = cpp.CParamList()
        if 'Role' in params:
            oCfg.SetIntProp( 101, params[ 'Role' ] )
        else:
            ret = -errno.EINVAL
            raise Exception(
                "Error UpdateDrvCfg cannot find router role" )
        if 'AppName' in params:
            oCfg.SetStrProp( 104, params[ 'AppName' ] )

        if 'ipaddr' in params:
            oCfg.SetStrProp( 109, params[ 'ipaddr' ] )

        if 'portnum' in params:
            oCfg.SetStrProp( 110, params[ 'portnum' ] )

        p2 = oCfg.GetCfg()
        return cpp.UpdateDrvCfg( strDriver, p2 )
    except Exception as err:
        if ret == 0:
            ret = -errno.EFAULT
            return ( ret, None )

class PyReqContext :
    def __init__( self ) :
        self.oCallback = object()
        self.oContext = object()

class PyRpcContext :
    def StartIoMgr( self, oInitParams ) :
        cpp.CoInitialize(0)
        p1=cpp.CParamList()
        ret = 0
        try:
            if type( oInitParams ) is str:
                ret = p1.SetStrProp( 0, oInitParams )
            elif type( oInitParams ) is dict:
                if 'ModName' in oInitParams:
                    p1.SetStrProp( 0,
                        oInitParams[ 'ModName' ] )
                else:
                    ret = -errno.EINVAL
                    raise Exception(
                        "Error cannot find Module name")
                dwRole = None
                if 'Role' in oInitParams:
                    dwRole = oInitParams[ 'Role' ]
                    p1.SetIntProp( 101, dwRole )
                else:
                    ret = -errno.EINVAL
                    raise Exception(
                        "Error cannot find router role" )
                if 'bAuth' in oInitParams:
                    p1.SetBoolProp( 102,
                        oInitParams[ 'bAuth' ] )
                if 'bDaemon' in oInitParams:
                    p1.SetBoolProp( 103,
                        oInitParams[ 'bDaemon' ] )
                if 'AppName' in oInitParams:
                    p1.SetStrProp( 104,
                        oInitParams[ 'AppName' ] )
                if 'driver' in oInitParams:
                    p1.SetStrProp( 105,
                        oInitParams[ 'driver' ] )
                if 'router' in oInitParams:
                    p1.SetStrProp( 106,
                        oInitParams[ 'router' ] )
                if 'instname' in oInitParams:
                    p1.SetStrProp( 107,
                        oInitParams[ 'instname' ] )
                elif 'sainstname' in oInitParams:
                    p1.SetStrProp( 107,
                        oInitParams[ 'sainstname' ] )
                if 'bKProxy' in oInitParams and dwRole == 1:
                    p1.SetBoolProp( 111,
                        oInitParams[ 'bKProxy' ] )
                if 'nodbus' in oInitParams:
                    p1.SetBoolProp( 112, True )

        except Exception as err:
            print( err )
            if ret == 0:
                ret = -errno.EFAULT

        if ret < 0 :
            return None

        p2 = cpp.CastToCfg( p1.GetCfgAsObj() )
        p3=cpp.StartIoMgr( p2 )
        return p3

    def StopIoMgr( self, pIoMgr ) :
        ret = cpp.StopIoMgr( pIoMgr )
        return ret

    def CleanUp( self ) :
        cpp.CoUninitialize()

    def DestroyRpcCtx( self ) :
        print( "proxy.py: about to quit..." )
        self.CleanUp()

    def __init__( self, oInitParams = "PyRpcProxy" ) :
        if oInitParams is None:
            raise Exception(
                "Error init parameter cannot be None" )
        self.oInitParams = oInitParams
        self.status = 0

    def Start( self, oInitParams ) :
        ret = 0
        print( "entering..." )
        self.pIoMgr = self.StartIoMgr( oInitParams )
        if self.pIoMgr is None :
            ret = -errno.EFAULT
            raise Exception(
                "Error Start IoManager" )
        return ret

    def Stop( self ) :
        print( "exiting..." )
        if self.pIoMgr is not None :
            self.StopIoMgr( self.pIoMgr )
            self.pIoMgr = None

    def __enter__( self ) :
        self.Start( self.oInitParams )
        return self

    def __exit__( self, type, val, traceback ) :
        if self.status < 0:
            print( os.getpid(), "__exit__():",
                type, val, traceback,
                self.status )
        self.Stop()
        self.DestroyRpcCtx()

class PyRpcServices :
    def GetError( self ) :
        if self.iError is None :
            return 0
        return self.iError

    def SetError( self, iErr ) :
        self.iError = iErr

    def Start( self ) :
        self.oInst.SetPyHost( self )
        ret = self.oInst.Start()
        isServer = self.IsServer()

        if ret < 0 :
            self.SetError( ret )
            if isServer :
                print( "Failed to start server %d(%d), state=%d" % 
                    ( os.getpid(), ret, self.oInst.GetState() ) )
            else :
                print( "Failed to start proxy %d(%d), state=%d" %
                    ( os.getpid(), ret, self.oInst.GetState() ) )
            return ret
        else :
            if isServer :
                print( "Server started..." )
            else :
                print( "Proxy started..." )

        oCheck = self.oInst.GetPyHost()
        return ret

    def Stop( self ) :
        ret = self.oInst.Stop()
        self.oInst.RemovePyHost()
        return ret

    def __enter__( self ) :
        self.Start()
        return self

    def __exit__( self, type, val, traceback ) :
        self.Stop()
        self.oInst = None
        self.oObj = None
        self.pIoMgr = None

    def TimerCallback( self, callback, context )->None :
        sig =signature( callback )
        iCount = len( sig.parameters )
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
        callback, context )->Tuple[int,cpp.ObjPtr] :
        return self.oInst.AddTimer(
            timeoutSec, callback, context )

    '''Remove a previously scheduled timer.
    '''
    def DisableTimer( self, timerObj ) :
        return self.oInst.DisableTimer(
            timerObj )

    '''attach a complete notification to the task
    to complete. When the task is completed or
    canceled, this notification will be triggered.
    the notification has the following layout
    callback( self, ret, *listArgs ), as is the
    same as the response callback. The `ret' is
    the error code of the task.
    '''
    def InstallCancelNotify(
        self, task, callback, *listArgs ):
        listResp = [ 0, [ *listArgs ] ]
        self.oInst.InstallCancelNotify(
            task, callback, listResp )

    def HandleAsyncResp( self,
        callback, seriProto, listResp, context ) :
        listArgs = []
        sig =signature( callback )
        iArgNum = len( sig.parameters ) - 2
        for i in range( iArgNum ):
            listArgs.append( None )

        if listResp is None :
            print( "HandleAsyncResp: bad response" )
            listArgs.insert( 0, -cpp.EBADMSG )
            self.InvokeCallback( callback, listArgs )
            return

        if iArgNum < 0 :
            print( "HandleAsyncResp: bad callback")
            listArgs.insert( 0, -cpp.EBADMSG )
            self.InvokeCallback( callback, listArgs )
            return

        ret = listResp[ 0 ]
        if seriProto == cpp.seriNone :
            if len( listResp ) <= 1 :
                listArgs.insert( 0, ret )
                self.InvokeCallback( callback, listArgs )
            elif isinstance( listResp[ 1 ], list ) :
                listArgs = listResp[ 1 ]
                listArgs.insert( 0, ret )
                self.InvokeCallback( callback, listArgs )
            else :
                listArgs.insert( 0, ret )
                self.InvokeCallback( callback, listArgs )

        elif seriProto == cpp.seriPython :
            if len( listResp ) <= 1 :
                listArgs.insert( 0, ret )
                self.InvokeCallback( callback, listArgs )
            elif isinstance( listResp[ 1 ], bytearray ) :
                try:
                    trueResp = pickle.loads( listResp[ 1 ] )
                    if isinstance( trueResp, list ) :
                        trueResp.insert( 0, ret )
                        self.InvokeCallback( callback, trueResp )
                except PickleError:
                    listArgs.insert( 0, -cpp.EBADMSG )
                    self.InvokeCallback( callback, listArgs )
            else :
                listArgs.insert( 0, -cpp.EBADMSG )
                self.InvokeCallback( ret, listArgs )

        elif seriProto == cpp.seriRidl :
            listArgs.insert( 0, context )
            listArgs.insert( 1, ret )
            if len( listResp ) <= 1 :
                self.InvokeCallback( callback, listArgs )
            elif isinstance( listResp[ 1 ], list ) :
                if len( listResp[ 1 ] ) > 0 and iArgNum == 1 :
                    listArgs[ 2 ] = listResp[ 1 ][ 0 ]
                self.InvokeCallback( callback, listArgs )
            else :
                self.InvokeCallback( callback, listArgs )
        return

    def GetObjType( self, pObj ) :
        return GetObjType( pObj )

    def GetNumpyValue( typeid, val ) :
        return GetNpValue( typeid, val )

    ''' establish a stream channel and
    return a handle to the channel on success
    '''
    def StartStream( self ) -> int :
        oDesc = cpp.CParamList()
        tupRet = self.oInst.StartStream(
            oDesc.GetCfgAsObj() )
        ret = tupRet[ 0 ]
        if ret < 0 :
            print( "Error start stream", ret )
            return 0
        return tupRet[ 1 ]
        

    def CloseStream( self, hChannel ) :
        return self.oInst.CloseStream( hChannel )

    def WriteStream( self, hChannel, pBuf ) :
        return self.oInst.WriteStream( hChannel, pBuf )

    '''Write pBuf to stream hChannel, and return
    immediately and no waiting for whether pBuf is sent
    successfully or not
    '''
    def WriteStreamNoWait( self, hChannel, pBuf ) :
        if len( pBuf ) == 0:
            return -errno.EINVAL
        return self.oInst.WriteStreamNoWait(
            hChannel, pBuf )

    ''' callback declaration for async stream write
    '''
    def WriteStmCallback( self, iRet, hChannel, pBuf ) :
        pass

    def WriteStreamAsync2( self,
        hChannel, pBuf )->int :
        return self.oInst.WriteStreamAsync(
            hChannel, pBuf, self.WriteStmCallback )

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
    def ReadStream( self, hChannel, size = 0
        )-> Tuple[ int, bytearray, cpp.BufPtr ] :
        return self.oInst.ReadStream( hChannel, size )

    ''' callback declaration for async stream read
    '''
    def ReadStmCallback( self, iRet, hChannel, pBuf ) :
        pass
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
        hChannel, callback, size = 0)->Tuple[
            int, Optional[ bytearray ], Optional[ cpp.ObjPtr] ]:

        return self.oInst.ReadStreamAsync(
            hChannel, callback, size )

    def ReadStreamAsync2( self,
        hChannel, size = 0)->Tuple[
            int, Optional[ bytearray ], Optional[ cpp.ObjPtr] ]:
        return self.oInst.ReadStreamAsync(
            hChannel, self.ReadStmCallback, size )
    '''
    ReadStreamNoWait to read the first block of
    bufferrd data from the stream. If there are
    not data bufferred, error -EAGIN will be
    returned, and will not blocked to wait.
    '''
    def ReadStreamNoWait( self, hChannel
        )-> Tuple[ int, bytearray, cpp.BufPtr ] :
        return self.oInst.ReadStreamNoWait( hChannel )

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
        pObj:cpp.ObjPtr )->Tuple[ int, list ] :

        pCfg = cpp.CastToCfg( pObj )
        if pCfg is None :
            return [ -errno.EFAULT, ]
        oParams = cpp.CParamList( pCfg )
        ret = oParams.GetSize()
        if ret[ 0 ] < 0 :
            return ret
        iSize = ret[ 1 ]

        if iSize < 0 :
            return [ -errno.EINVAL, ]

        argList = []
        if seriProto == cpp.seriPython :
            ret = oParams.GetBufPtr( 0 )
            if ret[ 0 ] < 0 :
                return [ 0, argList ]
            val = ret[ 1 ]
            if val.IsEmpty() :
                return [ -errno.EFAULT, ]

            pyBuf = bytearray( val.size() )
            iRet = val.CopyToPython( pyBuf )
            if iRet < 0 : 
                return [ iRet, ]

            try :
                argList = pickle.loads( pyBuf )
            except PickleError :
                return [ -errno.EBADMSG, ]

            if not isinstance( argList, list ):
                return [ -errno.EBADMSG, ]

            return [ 0, argList ]

        elif seriProto == cpp.seriRidl :
            ret = oParams.GetBufPtr( 0 )
            if ret[ 0 ] < 0 :
                return [ 0, argList ]

            val = ret[ 1 ]
            if val.IsEmpty() :
                return [ 0, argList ]

            pyBuf = bytearray( val.size() )
            iRet = val.CopyToPython( pyBuf )
            if iRet < 0 : 
                return [ iRet, ]

            argList.append( pyBuf )
            return [ 0, argList ]

        elif seriProto != cpp.seriNone :
            return [ -errno.EBADMSG, ]
            
        for i in range( iSize ) :
            ret = oParams.GetPropertyType( i )
            if ret[ 0 ] < 0 :
                break
            iType = ret[ 1 ]
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
                ret = oParams.GetBufPtr( i )
                if ret[ 0 ] < 0 :
                    break
                val = ret[ 1 ]
                if val.IsEmpty() :
                    ret = [ -errno.EFAULT, ]
                    break
                pyBuf = bytearray( val.size() )
                iRet = val.CopyToPython( pyBuf )
                if iRet < 0 : 
                    ret = [ iRet, ]
                    break

                val = pyBuf
            argList.append( val )

        if ret[ 0 ] < 0 :
            return ret

        return [ 0, argList ]

    def GetMethod( self, strIfName : str, strMethod : str ) -> object :
        while True :
            found = False
            typeFound = None
            bases = type( self ).__bases__ 
            for iftype in bases :
                if not hasattr( iftype, "_ifName_" ) :
                    continue
                if iftype._ifName_ != strIfName :
                    continue
                found = True
                typeFound = iftype
                break
            if not found :
                break

            found = False
            oMembers = inspect.getmembers(
                typeFound, inspect.isfunction)

            for oMethod in oMembers :
                if strMethod != oMethod[ 0 ]: 
                    continue
                targetMethod = oMethod[ 1 ]
                found = True
                break

            break

        if found :
            return targetMethod

        return None


    #for event handler 
    def InvokeMethod( self, callback,
        ifName, methodName,
        seriProto:int,
        cppargs:cpp.ObjPtr ) ->Tuple[ int, list ]:
        resp = [ 0 ]
        while True :

            ret = self.ArgObjToList( seriProto, cppargs )
            if ret[ 0 ] < 0 :
                resp[ 0 ] = ret
                break

            argList = ret[ 1 ]
            found = False
            typeFound = None
            bases = type( self ).__bases__ 
            for iftype in bases :
                if not hasattr( iftype, "_ifName_" ) :
                    continue
                if iftype._ifName_ != ifName :
                    continue
                found = True
                typeFound = iftype
                break
            if not found :
                resp[ 0 ] = -errno.ENOTSUP
                break

            isServer = self.oInst.IsServer()
            nameComps = methodName.split( '_' )
            if not isServer :
                if nameComps[ 0 ] != "UserEvent" :
                    resp[ 0 ] = -errno.EINVAL
                    break
            else :
                if nameComps[ 0 ] != "UserMethod" :
                    resp[ 0 ] = -errno.EINVAL
                    break

            found = False
            oMembers = inspect.getmembers(
                typeFound, inspect.isfunction)

            if( seriProto != cpp.seriRidl ) :
                for oMethod in oMembers :
                    if nameComps[ 1 ] != oMethod[ 0 ]: 
                        continue
                    targetMethod = oMethod[ 1 ]
                    found = True
                    break
            else :
                wrapMethod = nameComps[ 1 ] + "Wrapper"
                for oMethod in oMembers :
                    if wrapMethod != oMethod[ 0 ]: 
                        continue
                    targetMethod = oMethod[ 1 ]
                    found = True
                    break

            if not found :
                resp[ 0 ] = -errno.EINVAL
                break

            if targetMethod is None :
                resp[ 0 ] = -errno.EFAULT
                break

            try:
                resp = targetMethod( self, callback, *argList )
            except:
                resp[ 0 ] = -errno.EFAULT
                
            if seriProto == cpp.seriNone :
                break

            if seriProto == cpp.seriRidl :
                break

            if seriProto != cpp.seriPython :
                resp[ 0 ] = -errno.EINVAL
                break

            ret = resp[ 0 ]
            if ret < 0 :
                break

            if len( resp ) <= 1 : 
                break

            if not isinstance( resp[ 1 ], list ) :
                resp[ 0 ] = -errno.EBADMSG
                break

            listResp = resp[ 1 ]
            pBuf = pickle.dumps( listResp )
            resp[ 1 ] = [ pBuf, ]
            break #while True

        return resp

    '''Invoke a callback function depending
    whether it is bound function or not
    '''
    def InvokeCallback( self, callback, listArgs):
        ret = None
        if inspect.isfunction( callback ) :
            ret = callback( self, *listArgs )
        elif inspect.ismethod( callback ) :
            ret = callback( *listArgs )
        return ret

    ''' run the callback on a new stack context
    instead of the current one

    DeferCall, like a functor object bind to the
    callback function with the arguments. And it
    can be scheduled run on a new stack context.
    The purpose is to reduce the depth of the call
    stack and provide a locking free or light
    locking context as the current one cannot.
    '''
    def DeferCall( self, callback, *args ) :
        return self.oInst.PyDeferCall(
            callback, [ *args ] )

    def DeferCallback( self, callback, listArgs ) :
        self.InvokeCallback( callback, listArgs )

class PyRpcProxy( PyRpcServices ) :

    def __init__( self, pIoMgr, strDesc, strSvrObj ) :
        self.pIoMgr = pIoMgr
        self.iError = 0
        oParams = cpp.CParamList()
        oObj = CreateProxy( pIoMgr,
            strDesc, strSvrObj,
            oParams.GetCfgAsObj() )
        if oObj is None or oObj.IsEmpty() :
            print( "failed to create proxy..." )
            self.SetError( -errno.EFAULT )
            return

        oInst = CastToProxy( oObj )
        if oInst is None :
            print( "failed to create proxy 2..." )
        self.oInst = oInst
        self.oObj = oObj
        return

    def sendRequest( self,
        strIfName, strMethod, *args )->Tuple[int,list] :
        resp = [ 0, None ]

        listArgs = list( args )
        self.MakeCall( strIfName, strMethod,
            listArgs, resp, cpp.seriNone )

        return resp

    ''' sendRequestAsync returns a tuple with two
    elements, 0 is the return code, and 1 is a
    np.int64 as a taskid, which can be used to
    cancel the request sent'''
    def sendRequestAsync( self,
        callback, strIfName,
        strMethod, *args )->Tuple[int, np.uint64]:

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
        strMethod, *args )->Tuple[ int, list] :

        resp = [ 0, None ]
        listArgs = list( args )
        pBuf = pickle.dumps( listArgs )
        ret = self.MakeCall( strIfName, strMethod,
            [ pBuf, ], resp, cpp.seriPython )
        if len( resp ) <= 1 :
            return resp

        if resp[ 0 ] == 0 :
            pPyBuf = resp[ 1 ][ 0 ]
            if not isinstance( pPyBuf, bytearray ) :
                resp[ 0 ] = -errno.EBADMSG
                return resp
            listArgs = pickle.loads( pPyBuf )
            if not isinstance( listArgs, list ) :
                resp[ 0 ] = -errno.EBADMSG
                return resp
            resp[ 1 ] = listArgs

        return resp

    def PySendRequestAsync(
        self, callback,
        strIfName, strMethod,
        *args )->Union[ int, Optional[ int ], Optional[list]]:

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

    def MakeCall( self, strIfName,
        strMethod, args, resp, seriProto )->int :

        self.oInst.PyProxyCallSync(
            strIfName, strMethod, args,
            resp, seriProto )

        return resp

    def MakeCallAsync( self, callback,
        strIfName, strMethod, args, resp, seriProto
        )->Tuple[int,int]:

        ret = self.oInst.PyProxyCall( callback,
            strIfName, strMethod, args, resp, seriProto )

        return ret

    ''' MakeCallWithOpt: similiar to MakeCall, but with
        more parameters in pCfg. It is a synchronous call.
        the parameters in pCfg includes:
        [ propIfName ]: string, interface name
        [ propMethodName ]: string, method name
        [ propSeriProto ]: guint32, serialization protocol
        [ propNoReply ]: bool, one-way request (optional)
        [ propTimeoutSec ]: guint32, timeout in second
            specific to this request (optional)
        [ propKeepAliveSec ]: guint32, timeout in second
            for keep-alive heartbeat (optional)

        the return value is a list includes:
        [ 0 ] : error code
        [ 1 ] : depending on the element[ 0 ]
            if [ 0 ] is STATUS_PENDING
                [ 1 ] is the taskid for canceling
            if [ 0 ] is STATUS_SUCCESS and propNoReply is false,
                [ 1 ] is a list with a bytearray as the
                only element
            if [ 0 ] is ERROR or propNoReply is true
                [ 1 ] is None
    '''
    def MakeCallWithOpt( self,
        pCfg : cpp.CfgPtr, args : list, resp : list
        )->Tuple[int,list]:

        return self.oInst.PyProxyCall2(
            None, None, pCfg, args, resp )

    def MakeCallWithOptAsync( self, callback,
        context: object,
        pCfg : cpp.CfgPtr,
        args : list, resp : list
        )->Tuple[int,list]:

        return self.oInst.PyProxyCall2(
            callback, context, pCfg, args, resp )

    def IsServer( self ) :
        return False

class PyRpcServer( PyRpcServices ) :

    def __init__( self, pIoMgr, strDesc, strSvrObj ) :
        self.pIoMgr = pIoMgr
        self.iError = 0
        oParams = cpp.CParamList()
        oObj = CreateServer( pIoMgr,
            strDesc, strSvrObj,
            oParams.GetCfgAsObj() )
        if oObj is None or oObj.IsEmpty() :
            print( "failed to create server..." )
            self.SetError( -errno.EFAULT )
            return

        oInst = CastToServer( oObj )
        if oInst is None :
            print( "failed to create server 2..." )
        self.oInst = oInst
        self.oObj = oObj
        return

    def SetChanCtx( self, hChannel, oContext ) :
        return self.oInst.SetChanCtx(
            hChannel, oContext )

    def GetChanCtx( self, hChannel ) :
        return self.oInst.GetChanCtx( hChannel )

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

    def RidlOnServiceComplete(
        self, callback, ret, pBuf : bytearray ) :
        listResp = []
        if ret == 0 and pBuf is not None :
            listResp.append( pBuf )
        return self.oInst.OnServiceComplete(
            callback, ret, cpp.seriRidl, listResp )

    def RidlSetResponse(
        self, callback, ret, pBuf : bytearray ) :
        listResp = []
        if ret == 0 and pBuf is not None :
            listResp.append( pBuf )
        return self.oInst.SetResponse(
            callback, ret, cpp.seriRidl, listResp )

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

    ''' callback can be none if it is not
    necessary to get notified of the completion.
    destName can be none if not needed.
    '''
    def RidlSendEvent( self, callback,
        ifName, evtName, destName, pBuf : bytearray ) :
        evtName = "UserEvent_" + evtName
        pListArgs = []
        if pBuf is not None:
            pListArgs.append( pBuf )
        return self.oInst.SendEvent( callback,
            ifName, evtName, destName, pListArgs,
            cpp.seriRidl )

    '''event called when the stream channel
    described by pDataDesc is about to establish
    If accepted, the OnStmReady will be called,
    and the datadesc can be retrieved by the
    stream handle.
    '''
    def AcceptNewStream( self,
        pCtx: cpp.ObjPtr, pDataDesc: cpp.ObjPtr ):
        return 0

    def IsServer( self ) :
        return True
