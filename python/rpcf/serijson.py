'''
 * =====================================================================================
 *
 *       Filename:  serijson.py
 *
 *    Description:  serialization utilities for rpcfs skelton in Python
 *
 *        Version:  1.0
 *        Created:  10/8/2022 14:52:00 PM
 *       Revision:  none
 *    Interpreter:  python3
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2022 Ming Zhi( woodhead99@gmail.com )
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
import errno
import json
import base64

class Variant :
    def __init__( self ) :
        self.iType = cpp.typeNone 
        self.val = None
    
class CSerialBase :

    def __init__( self ) :
        pass

    def SerialBool( self, val : bool ) -> bool :
        return val

    def SerialInt8( self, val : int ) -> int :
        return val

    def SerialInt16( self, val : int ) -> int :
        return val

    def SerialInt32( self, val : int ) -> int :
        return val

    def SerialInt64( self, val : int ) -> int:
        return val

    def SerialFloat( self, val : float ) -> float:
        return val

    def SerialDouble( self, val : float ) -> float:
        return val

    def SerialString( self, val : str ) -> str :
        return val

    def SerialBuf( self, val : bytearray ) -> str :
        return base64.b64encode(val).decode( 'UTF-8' )

    def SerialObjPtr( self, val : object )-> int :
        raise Exception( "ObjPtr is not supported by rpcfs" )

    def SerialHStream( self, val : str ) -> str:
        return val

    def SerialStruct( self, val : object ) -> Tuple[ int, dict ]:
        return val.Serialize()

    def SerialVariant( self, val : Variant )->dict :
        ret = dict()
        iType = val.iType
        if iType == cpp.typeByte:
            oVal = self.SerialInt8( val.val )
        elif iType == cpp.typeUInt16:
            oVal = self.SerialInt16( val.val )
        elif iType == cpp.typeUInt32:
            oVal = self.SerialInt32( val.val )
        elif iType == cpp.typeUInt64:
            oVal = self.SerialInt64( val.val )
        elif iType == cpp.typeFloat:
            oVal = self.SerialFloat( val.val )
        elif iType == cpp.typeDouble:
            oVal = self.SerialDouble( val.val )
        elif iType == cpp.typeString:
            oVal = self.SerialString( val.val )
        elif iType == cpp.typeObj:
            iRet, oVal = self.SerialStruct( val.val )
            if iRet < 0:
                raise Exception(
                    "Serialize struct failed " + str( ret ) )
        elif iType == cpp.typeNone:
            oVal = None
        else:
            raise Exception( "unsupported datatype by rpcfs" )
        if oVal is None:
            raise Exception( "Error occurs in SerialVariant" )

        ret[ 'v' ] = oVal
        ret[ 't'] = iType
        return ret

    def SerialArray( self, val : list, sig: str ) -> Tuple[ int, list ]:
        sigLen = len( sig )
        if sig[ 0 ] != '(' :
            return ( -errno.EINVAL, None )
        if sig[ sigLen -1 ] != ')':
            return ( -errno.EINVAL, None )
        if sigLen <= 2 :
            return ( -errno.EINVAL, None )
        sigElem = sig[ 1 : sigLen - 1 ]

        res = []
        ret = ( 0, res )
        count = len( val )
        for elem in val :
            ret = self.SerialElem( elem, sigElem )
            if ret[ 0 ] < 0 :
                break
            res.append( ret[ 1 ] )
        if ret[ 0 ] < 0 :
            return ret
        return ( 0, res )

    def SerialMap( self, val : dict, sig: str ) -> Tuple[ int, dict ]:
        sigLen = len( sig )
        if sig[ 0 ] != '[' :
            return ( -errno.EINVAL, None )
        if sig[ sigLen -1 ] != ']':
            return ( -errno.EINVAL, None )
        if sigLen <= 2 :
            return ( -errno.EINVAL, None )
        sigElem = sig[ 1 : sigLen - 1 ]

        count = len( val )
        res = dict()
        ret = ( 0, res )
        for ( key, value ) in val.items() :
            ret = self.SerialElem( key, sigElem[ :1 ] )
            if ret[ 0 ] < 0 :
                break
            resKey = ret[ 1 ]
            ret = self.SerialElem( value, sigElem[ 1: ] )
            if ret[ 0 ] < 0 :
                break
            res[ resKey ] = ret[ 1 ]
        if ret[ 0 ] < 0 :
            return ret
        return ( 0, res )

    def SerialElem( self, val : object, sig : str ) -> Tuple[ int, object]:
        if sig[ 0 ] == '(' :
            return self.SerialArray( val, sig )
        if sig[ 0 ] == '[' :
            return self.SerialMap( val, sig )
        if sig[ 0 ] == 'O' :
            return self.SerialStruct( val )

        resElem = self.SerialElemOpt[ sig[ 0 ] ]( self, val )
        return ( 0, resElem )

    def DeserialInt64( self, val : int ) -> int:
        return val

    def DeserialInt32( self, val : int ) -> int:
        return val

    def DeserialInt16( self, val : int ) -> int:
        return val

    def DeserialBool( self, val : bool ) -> bool:
        return val

    def DeserialInt8( self, val : int ) -> int:
        return val

    def DeserialFloat( self, val : float ) -> float:
        return val

    def DeserialDouble( self, val : float ) -> float:
        return val

    def DeserialHStream( self, val : str ) -> str:
        return val

    def DeserialString( self, val : str ) -> str:
        return val

    def DeserialBuf( self, val : str ) -> bytearray:
        return base64.b64decode( val )

    def DeserialVariant( self, val : dict )->Variant:
        iType = val[ 't' ]
        if iType == cpp.typeByte:
            oVal = self.DeserialInt8( val[ 'v' ] )
        elif iType == cpp.typeUInt16:
            oVal = self.DeserialInt16( val[ 'v' ] )
        elif iType == cpp.typeUInt32:
            oVal = self.DeserialInt32( val[ 'v' ] )
        elif iType == cpp.typeUInt64:
            oVal = self.DeserialInt64( val[ 'v' ] )
        elif iType == cpp.typeFloat:
            oVal = self.DeserialFloat( val[ 'v' ] )
        elif iType == cpp.typeDouble:
            oVal = self.DeserialDouble( val[ 'v' ] )
        elif iType == cpp.typeString:
            oVal = self.DeserialString( val[ 'v' ] )
        elif iType == cpp.typeObj:
            ret, oVal = self.DeserialStruct( val[ 'v' ] )
            if ret < 0 :
                raise Exception(
                    "Error Deserialize struct failed " + str(ret) )
        elif iType == cpp.typeNone:
            oVal = None
        else:
            raise Exception( "unsupport data type by rpcfs" )
        var = Variant()
        var.val = oVal
        var.iType = iType
        return var
        
    def DeserialObjPtr( self, val : object )->object:
        raise Exception( "ObjPtr is not supported by rpcfs" )

    def DeserialStruct( self, val : dict ) -> Tuple[ int, object ]:
        try:
            structId = val[ "StructId" ]
            structInst = CStructFactoryBase.Create( structId )
            ret = structInst.Deserialize( val )
            if ret < 0 :
                return ( ret, None )
            return ( 0, structInst )
        except Exception as err:
            return ( -errno.EFAULT, None );

    def DeserialArray( self, val : list, sig : str )-> Tuple[ int, list ]:
        sigLen = len( sig )
        if sig[ 0 ] != '(' :
            return ( None, -errno.EINVAL )
        if sig[ sigLen -1 ] != ')':
            return ( None, -errno.EINVAL )
        if sigLen <= 2 :
            return ( None, -errno.EINVAL )
        sigElem = sig[ 1 : sigLen - 1 ]

        res = []
        count = len( val )
        if count == 0 :
            return ( 0, res )

        ret = ( 0, res )
        for elem in val :
            ret = self.DeserialElem( elem, sigElem )
            if ret[ 0 ] < 0 :
                break
            res.append( ret[ 1 ] )
        if ret[ 0 ] < 0:
            return ret
        return ( 0, res )

    def DeserialMap( self, val : dict, sig : str ) -> Tuple[ int, object ]:
        sigLen = len( sig )
        if sig[ 0 ] != '[' :
            return ( None, -errno.EINVAL )
        if sig[ sigLen -1 ] != ']':
            return ( None, -errno.EINVAL )
        if sigLen <= 2 :
            return ( None, -errno.EINVAL )
        sigElem = sig[ 1 : sigLen - 1 ]

        res = dict()
        count = len( val )
        if count == 0 :
            res  = dict()
            return ( 0, res )

        ret = [ 0, None ]
        for key, value in val.items() :
            ret = self.DeserialElem( key, sigElem[ :1 ] )
            if ret[ 0 ] < 0 :
                break
            elemKey = ret[ 1 ]
            ret = self.DeserialElem( value, sigElem[ 1: ] )
            if ret[ 0 ] < 0 :
                break
            res[ elemKey ] = ret[ 1 ]

        if ret[ 0 ] < 0 :
            return ret
        return ( 0, res )

    def DeserialElem( self, val : object, sig : str ) -> Tuple[ int, object ]:
        if sig[ 0 ] == '(' :
            return self.DeserialArray( val, sig )
        if sig[ 0 ] == '[' :
            return self.DeserialMap( val, sig )
        if sig[ 0 ] == 'O' :
            return self.DeserialStruct( val )
        resElem = self.DeserialElemOpt[ sig[ 0 ] ]( self, val )
        return ( 0, resElem )

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
        'v' : SerialVariant,  # TOK_VARIANT,
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
        'v' : DeserialVariant,  # TOK_VARIANT,
    }

g_mapStructs = dict()

class CStructFactoryBase:
    global g_mapStructs
    def __init__( self ) : 
        pass

    @staticmethod
    def Create( iStructId : int ) -> object :
        if iStructId in g_mapStructs :
            return g_mapStructs[ iStructId ]()
        return None

    @staticmethod
    def AddStruct( iStructId : int, clsObj : object ) -> int :
        if iStructId in g_mapStructs :
            return -errno.EEXIST
        g_mapStructs[ iStructId ] = clsObj
        return 0

class CStructFactory( CStructFactoryBase ) :
    pass

