'''
 * =====================================================================================
 *
 *       Filename:  seribase.py
 *
 *    Description:  serialization utilities for Python
 *
 *        Version:  1.0
 *        Created:  10/24/2021 17:37:00 PM
 *       Revision:  none
 *    Interpreter:  python3
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
from typing import Tuple
from rpcf.rpcbase import *
import struct
import errno
from rpcf.proxy import *

class Variant :
    mapType2Sig = {
        cpp.typeUInt64 : 'Q',   # TOK_UINT64,
        cpp.typeUInt32 : 'D',   # TOK_UINT32,
        cpp.typeUInt16 : 'W',   # TOK_UINT16,
        cpp.typeFloat :  'f',    # TOK_FLOAT,
        cpp.typeDouble:  'F',    # TOK_DOUBLE,
        cpp.typeByte :   'b',     # TOK_BYTE,
        cpp.typeString:  's',    # TOK_STRING,
        cpp.typeByteArr: 'a',   # TOK_BYTEARR,
        cpp.typeObj:     'O',       # TOK_STRUCT,
    }

    def __init__( self, osb : Union[ "CSerialBase", None ] = None ) :
        self.iType = cpp.typeNone
        self.val = None
        self.osb = osb

    def Serialize( self, buf : bytearray )->int :
        if self.osb is None:
            return -errno.EFAULT
        osb = self.osb
        iType = self.iType
        if iType in self.mapType2Sig:
            osb.SerialInt8( buf, iType )
            sig = self.mapType2Sig[ iType ]
            return osb.SerialElem( buf, self.val, sig )
        elif iType == cpp.typeNone:
            osb.SerialInt8( buf, iType )
            return 0
        return -errno.ENOTSUP

    def Deserialize( self, buf, offset )->Tuple[ int, int ]:
        if self.osb is None:
            return ( None, offset ) 
        osb = self.osb
        ret = osb.DeserialInt8( buf, offset )
        iType = ret[ 0 ]
        newOff = ret[ 1 ]
        if iType != cpp.typeObj:
            if iType in self.mapType2Sig:
                sig = self.mapType2Sig[ iType ]
                val = osb.DeserialElem( buf, newOff, sig )
            elif iType ==  cpp.typeNone:
                self.iType = iType
                return ( 0, newOff )
            else:
                return ( None, offset )
        elif iType == cpp.typeObj:
            ret = osb.DeserialInt32( buf, newOff );
            if ret[ 0 ] == 0x73747275:
                val = osb.DeserialStruct( buf, newOff )
                if val[ 0 ] is None:
                    return ( None, offset )
            else:
                val = osb.DeserialObjPtr( buf, newOff )
                if val[ 0 ] is None:
                    return ( None, offset )
        self.iType = iType
        self.val = val[ 0 ]
        return ( 0, val[ 1 ] )

    def SetBool( self, val ):
        self.iType = cpp.typeByte
        if val is True:
            self.val = 1
        else:
            self.val = 0

    def SetByte( self, val ):
        self.iType = cpp.typeByte
        self.val = val

    def SetUInt16( self, val ):
        self.iType = cpp.typeUInt16
        self.val = val

    def SetInt16( self, val ):
        self.SetUInt16( val )

    def SetUInt32( self, val ):
        self.iType = cpp.typeUInt32
        self.val = val

    def SetInt32( self, val ):
        self.SetUInt32( val )

    def SetUInt64( self, val ):
        self.iType = cpp.typeUInt64
        self.val = val

    def SetInt64( self, val ):
        self.SetUInt64( val )

    def SetFloat( self, val ):
        self.iType = cpp.typeFloat
        self.val = val

    def SetDouble( self, val ):
        self.iType = cpp.typeDouble
        self.val = val

    def SetStruct( self, val ):
        self.iType = cpp.typeObj
        self.val = val

    def SetObjPtr( self, val ):
        self.iType = cpp.typeObj
        self.val = val

    def SetByteArray( self, val ):
        self.iType = cpp.typeByteArr
        self.val = val

    def SetString( self, val ):
        self.iType = cpp.typeString
        self.val = val

    def GetType( self ):
        return self.iType

    def GetVal( self ):
        return self.val

class CSerialBase :

    def __init__( self, pIf : PyRpcServices = None ) :
        self.pIf = pIf

    def SerialBool( self, buf : bytearray, val : int ) :
        buf.extend( struct.pack( "!B", val ) )

    def SerialInt8( self, buf : bytearray, val : int ) :
        buf.extend( struct.pack( "!B", val ) )

    def SerialInt16( self, buf : bytearray, val : int ) :
        buf.extend( struct.pack( "!H", val ) )

    def SerialInt32( self, buf : bytearray, val : int ) :
        buf.extend( struct.pack( "!I", val ) )

    def SerialInt64( self, buf : bytearray, val : int ) :
        buf.extend( struct.pack( "!Q", val ) )

    def SerialFloat( self, buf : bytearray, val : float ) :
        buf.extend( struct.pack( "!f", val ) )

    def SerialDouble( self, buf : bytearray, val : float ) :
        buf.extend( struct.pack( "!d", val ) )

    def SerialString( self, buf : bytearray, val : str ) :
        buf.extend( struct.pack(
            "!I%ds" % len( val ), len( val ), val.encode() ) )

    def SerialBuf( self, buf : bytearray, val : bytearray ) :
        buf.extend( struct.pack(
            "!I%ds" % len( val ), len( val ), val ) )

    def SerialObjPtr( self, buf : bytearray, val : cpp.ObjPtr )-> int :
        objBuf = val.SerialToByteArray()
        if objBuf is None :
            return -errno.EFAULT
        if type( objBuf ) is not bytearray :
            return -errno.EFAULT
        buf.extend( objBuf )
        return 0

    def SerialHStream( self, buf : bytearray, val : int ) -> int:
        if self.pIf is None :
            return -errno.EFAULT
        pIf = self.pIf
        isServer = pIf.IsServer()
        if isServer :
            ret = pIf.oInst.GetIdHashByChan( val )
        else :
            ret = pIf.oInst.GetPeerIdHash( val )

        if ret[ 0 ] <  0 :
            return ret[ 0 ]

        self.SerialInt64( buf, ret[ 1 ] )
        return 0 

    def SerialStruct( self, buf: bytearray, val : object ) -> int:
        try:
            val.osb = self
            return val.Serialize( buf )
        except :
            return -errno.EFAULT

    def SerialVariant( self, buf: bytearray, val : Variant ) -> int:
        try:
            if val.osb is None:
                val.osb = self
            return val.Serialize( buf )
        except :
            return -errno.EFAULT

    def SerialArray( self, buf: bytearray, val : list, sig: str ) -> int:
        sigLen = len( sig )

        if sig[ 0 ] != '(' :
            return -errno.EINVAL
        if sig[ sigLen -1 ] != ')':
            return -errno.EINVAL
        if sigLen <= 2 :
            return -errno.EINVAL
        sigElem = sig[ 1 : sigLen - 1 ]

        sizeOff = len( buf )
        count = len( val )
        self.SerialInt32( buf, 0 )
        self.SerialInt32( buf, count )
        if count == 0 :
            return 0
        for elem in val :
            ret = self.SerialElem( buf, elem, sigElem )
            if ret < 0 :
                return ret

        arrSize = len( buf ) - sizeOff - 8 
        if arrSize <= 0 :
            return -errno.ERANGE

        struct.pack_into( "!I", buf, sizeOff, arrSize )
        return 0

    def SerialMap( self, buf: bytearray, val : dict, sig: str ) -> int:
        sigLen = len( sig )
        if sig[ 0 ] != '[' :
            return -errno.EINVAL
        if sig[ sigLen -1 ] != ']':
            return -errno.EINVAL
        if sigLen <= 2 :
            return -errno.EINVAL
        sigElem = sig[ 1 : sigLen - 1 ]

        sizeOff = len( buf )
        count = len( val )
        self.SerialInt32( buf, 0 )
        self.SerialInt32( buf, count )
        if count == 0 :
            return 0

        if type( val ) is not dict :
            return -errno.EINVAL

        for ( key, value ) in val.items() :
            ret = self.SerialElem( buf, key, sigElem[ :1 ] )
            if ret < 0 :
                return ret
            ret = self.SerialElem( buf, value, sigElem[ 1: ] )
            if ret < 0 :
                return ret

        mapSize = len( buf ) - sizeOff - 8 
        if mapSize <= 0 :
            return -errno.ERANGE

        struct.pack_into( "!I", buf, sizeOff, mapSize )
        return 0

    def SerialElem( self, buf: bytearray, val : object, sig : str ) -> int:
        if sig[ 0 ] == '(' :
            return self.SerialArray( buf, val, sig )
        if sig[ 0 ] == '[' :
            return self.SerialMap( buf, val, sig )

        ret = self.SerialElemOpt[ sig[ 0 ] ]( self, buf, val )
        if ret is None:
            return 0
        return ret

    def DeserialInt64( self, buf : bytearray, offset : int ) -> Tuple[ int, int ] :
        val = struct.unpack_from( "!Q", buf, offset )
        return ( val[ 0 ], offset + 8 )

    def DeserialInt32( self, buf : bytearray, offset : int ) -> Tuple[ int, int ] :
        val = struct.unpack_from( "!I", buf, offset )
        return ( val[ 0 ], offset + 4 )

    def DeserialInt16( self, buf : bytearray, offset : int ) -> Tuple[ int, int ] :
        val = struct.unpack_from( "!H", buf, offset )
        return ( val[ 0 ], offset + 2 )

    def DeserialBool( self, buf : bytearray, offset : int ) -> Tuple[ bool, int ] :
        val = struct.unpack_from( "!B", buf, offset )
        return ( val[ 0 ], offset + 1 )

    def DeserialInt8( self, buf : bytearray, offset : int ) -> Tuple[ int, int ] :
        val = struct.unpack_from( "!B", buf, offset )
        return ( val[ 0 ], offset + 1 )

    def DeserialFloat( self, buf : bytearray, offset : int ) -> Tuple[ float, int ] :
        val = struct.unpack_from( "!f", buf, offset )
        return ( val[ 0 ], offset + 4 )

    def DeserialDouble( self, buf : bytearray, offset : int ) -> Tuple[ float, int ] :
        val = struct.unpack_from( "!d", buf, offset )
        return ( val[ 0 ], offset + 8 )

    def DeserialHStream( self, buf : bytearray, offset : int ) -> Tuple[ int, int ] :
        if self.pIf is None :
            return ( 0, 0 )
        pIf = self.pIf
        val = self.DeserialInt64( buf, offset )
        ret = pIf.oInst.GetChanByIdHash( val[ 0 ] )
        if ret == ErrorCode.INVALID_HANDLE :
            return ( ret, 0 )
        return ( ret, offset + 8 )

    def DeserialString( self, buf : bytearray, offset : int ) -> Tuple[ str, int ] :
        ret = self.DeserialInt32( buf, offset )
        offset += 4
        iSize = ret[ 0 ]
        if iSize is None :
            return ( None, 0 )
        if  iSize == 0:
            return ( "", offset )
        return ( buf[ offset : offset + iSize ].decode(), offset + iSize )

    def DeserialBuf( self, buf : bytearray, offset : int ) -> Tuple[ bytearray, int ] :
        ret = self.DeserialInt32( buf, offset )
        offset += 4
        iSize = ret[ 0 ]
        if iSize is None :
            return ( None, 0 )
        if iSize == 0 :
            return ( bytearray(), offset )
        return ( buf[ offset : offset + iSize ], offset + iSize )
        
    def DeserialObjPtr( self, buf : bytearray, offset : int ) -> Tuple[ cpp.ObjPtr, int ] :
        pNewObj = cpp.ObjPtr()
        listRet = pNewObj.DeserialObjPtr( buf, offset )
        if listRet[ 0 ] < 0 : 
            return ( None, 0 ) 
        return ( listRet[ 1 ], listRet[ 2 ] ) 

    def DeserialStruct( self, buf : bytearray, offset : int ) -> Tuple[ object, int ] :
        ret = self.DeserialInt32( buf, offset + 4 )
        structId = ret[ 0 ]
        structInst = CStructFactoryBase.Create( structId, self.pIf )
        ret = structInst.Deserialize( buf, offset )
        if ret[ 0 ] < 0 :
            return ( None, 0 )
        return ( structInst, ret[ 1 ] )

    def DeserialVariant( self, buf : bytearray, offset : int ) -> Tuple[ Variant, int ]:
        var = Variant( self )
        ret = var.Deserialize( buf, offset )
        if ret[ 0 ] < 0:
            return ( None, 0 )
        return ( var, ret[ 1 ] )

    def DeserialArray( self, buf : bytearray, offset : int, sig : str )-> Tuple[list, int ]:
        sigLen = len( sig )
        if sig[ 0 ] != '(' :
            return ( None, 0 )
        if sig[ sigLen -1 ] != ')':
            return ( None, 0 )
        if sigLen <= 2 :
            return ( None, 0 )
        sigElem = sig[ 1 : sigLen - 1 ]

        sizeOff = len( buf )
        ret = self.DeserialInt32( buf, offset )
        arrBytes = ret[ 0 ]
        ret = self.DeserialInt32( buf, offset + 4 )
        count = ret [ 0 ]
        if count == 0 :
            return ( list(), offset + 8 )

        arrRet = list()
        elemOff = offset + 8
        for idx in range( count ) :
            ret = self.DeserialElem( buf, elemOff, sigElem )
            if ret[ 0 ] is None:
                return ( None, 0 )
            elemOff = ret[ 1 ]
            arrRet.append( ret[ 0 ] )

        return ( arrRet, elemOff )

    def DeserialMap( self, buf : bytearray, offset : int, sig : str ) -> Tuple[ dict, int ]:
        sigLen = len( sig )
        if sig[ 0 ] != '[' :
            return ( None, 0 )
        if sig[ sigLen -1 ] != ']':
            return ( None, 0 )
        if sigLen <= 2 :
            return ( None, 0 )
        sigElem = sig[ 1 : sigLen - 1 ]

        sizeOff = len( buf )
        ret = self.DeserialInt32( buf, offset )
        arrBytes = ret[ 0 ]
        ret = self.DeserialInt32( buf, offset + 4 )
        count = ret [ 0 ]
        if count == 0 :
            return ( dict(), offset + 8 )

        mapRet = dict()
        elemOff = offset + 8
        for idx in range( count ) :
            ret = self.DeserialElem( buf, elemOff, sigElem[ :1 ] )
            if ret[ 0 ] is None:
                return ( None, 0 )
            elemOff = ret[ 1 ]
            key = ret[ 0 ]
            ret = self.DeserialElem( buf, elemOff, sigElem[ 1: ] )
            if ret[ 0 ] is None:
                return ( None, 0 )
            elemOff = ret[ 1 ]
            val = ret[ 0 ]
            mapRet[ key ] = val
        return ( mapRet, elemOff )

    def DeserialElem( self, buf : bytearray, offset : int, sig : str ) -> Tuple[ object, int ]:
        if sig[ 0 ] == '(' :
            return self.DeserialArray( buf, offset, sig )
        if sig[ 0 ] == '[' :
            return self.DeserialMap( buf, offset, sig )
        return self.DeserialElemOpt[ sig[ 0 ] ]( self, buf, offset )

    SerialElemOpt = {
        'Q' : SerialInt64,    # TOK_UINT64,
        'q' : SerialInt64,    # TOK_INT64,
        'D' : SerialInt32,    # TOK_UINT32,
        'd' : SerialInt32,    # TOK_INT32,
        'W' : SerialInt16,    # TOK_UINT16,
        'w' : SerialInt16,    # TOK_INT16,
        'f' : SerialFloat,    # TOK_FLOAT,
        'F' : SerialDouble,    # TOK_DOUBLE,
        'b' : SerialBool,    # TOK_BOOL,
        'B' : SerialInt8,    # TOK_BYTE,
        'h' : SerialHStream,    # TOK_HSTREAM,
        's' : SerialString,    # TOK_STRING,
        'a' : SerialBuf,    # TOK_BYTEARR,
        'o' : SerialObjPtr,    # TOK_OBJPTR,
        'O' : SerialStruct,  # TOK_STRUCT,
        'v' : SerialVariant # TOK_VARIANT
    }

    DeserialElemOpt = {
        'Q' : DeserialInt64,    # TOK_UINT64,
        'q' : DeserialInt64,    # TOK_INT64,
        'D' : DeserialInt32,    # TOK_UINT32,
        'd' : DeserialInt32,    # TOK_INT32,
        'W' : DeserialInt16,    # TOK_UINT16,
        'w' : DeserialInt16,    # TOK_INT16,
        'f' : DeserialFloat,    # TOK_FLOAT,
        'F' : DeserialDouble,    # TOK_DOUBLE,
        'b' : DeserialBool,    # TOK_BOOL,
        'B' : DeserialInt8,    # TOK_BYTE,
        'h' : DeserialHStream,    # TOK_HSTREAM,
        's' : DeserialString,    # TOK_STRING,
        'a' : DeserialBuf,    # TOK_BYTEARR,
        'o' : DeserialObjPtr,    # TOK_OBJPTR,
        'O' : DeserialStruct,  # TOK_STRUCT,
        'v' : DeserialVariant # TOK_VARIANT
    }

g_mapStructs = dict()

class CStructFactoryBase:
    global g_mapStructs
    def __init__( self ) : 
        pass

    @staticmethod
    def Create( iStructId : int, pIf : PyRpcServices = None ) -> object :
        if iStructId in g_mapStructs :
            return g_mapStructs[ iStructId ]( pIf )
        return None

    @staticmethod
    def AddStruct( iStructId : int, clsObj : object ) -> int :
        if iStructId in g_mapStructs :
            return -errno.EEXIST
        g_mapStructs[ iStructId ] = clsObj
        return 0

class CStructFactory( CStructFactoryBase ) :
    pass

