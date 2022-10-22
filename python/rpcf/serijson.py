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
import errno
import json
import base64

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
        pass

    def SerialHStream( self, val : str ) -> str:
        return val

    def SerialStruct( self, val : object ) -> dict:
        return val.Serialize()

    def SerialArray( self, val : list, sig: str ) -> Tuple[ list, int ]:
        sigLen = len( sig )
        if sig[ 0 ] != '(' :
            return ( None, -errno.EINVAL )
        if sig[ sigLen -1 ] != ')':
            return ( None, -errno.EINVAL )
        if sigLen <= 2 :
            return ( None, -errno.EINVAL )
        sigElem = sig[ 1 : sigLen - 1 ]

        res = []
        ret = ( res, 0 )
        count = len( val )
        for elem in val :
            ret = self.SerialElem( elem, sigElem )
            if ret[ 1 ] < 0 :
                break
            res.append( ret[ 0 ] )
        if ret[ 1 ] < 0 :
            return ret
        return ( res, 0 )

    def SerialMap( self, val : dict, sig: str ) -> Tuple[ dict, int ]:
        sigLen = len( sig )
        if sig[ 0 ] != '[' :
            return ( None, -errno.EINVAL )
        if sig[ sigLen -1 ] != ']':
            return ( None, -errno.EINVAL )
        if sigLen <= 2 :
            return ( None, -errno.EINVAL )
        sigElem = sig[ 1 : sigLen - 1 ]

        count = len( val )
        res = dict()
        ret = ( res, 0 )
        for ( key, value ) in val.items() :
            ret = self.SerialElem( key, sigElem[ :1 ] )
            if ret[ 1 ] < 0 :
                break
            resKey = ret[ 0 ]
            ret = self.SerialElem( value, sigElem[ 1: ] )
            if ret[ 1 ] < 0 :
                break
            res[ resKey ] = ret[ 0 ]
        if ret[ 1 ] < 0 :
            return ret
        return ( res, 0 )

    def SerialElem( self, val : object, sig : str ) -> Tuple[ object, int]:
        if sig[ 0 ] == '(' :
            return self.SerialArray( val, sig )
        if sig[ 0 ] == '[' :
            return self.SerialMap( val, sig )

        resElem = self.SerialElemOpt[ sig[ 0 ] ]( self, val )
        return ( resElem, 0 )

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
        
    def DeserialObjPtr( self, val : object )->object:
        return val

    def DeserialStruct( self, val : dict ) -> object:
        structId = val.GetStructId()
        structInst = CStructFactoryBase.Create( structId )
        ret = structInst.Deserialize( val )
        if ret < 0 :
            return object()
        return structInst

    def DeserialArray( self, val : list, sig : str )-> Tuple[ list, int ]:
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
            return ( res, 0 )

        ret = ( res, 0 )
        for elem in val :
            ret = self.DeserialElem( elem, sigElem )
            if ret[ 1 ] < 0 :
                break
            res.append( resElem )
        return ( ret, res )

    def DeserialMap( self, val : list, sig : str ) -> Tuple[ dict, int ]:
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
            return ( res, 0 )

        ret = ( None, 0 )
        for key, value in val :
            ret = self.DeserialElem( key, sigElem[ :1 ], elemKey )
            if ret[ 1 ] < 0 :
                break
            ret = self.DeserialElem( value, sigElem[ 1: ], elemVal )
            if ret[ 1 ] < 0 :
                break
            res[ elemKey ] = elemVal

        return ( res, ret[ 1 ] )

    def DeserialElem( self, val : object, sig : str ) -> Tuple[ object, int ]:
        if sig[ 0 ] == '(' :
            return self.DeserialArray( val, sig )
        if sig[ 0 ] == '[' :
            return self.DeserialMap( val, sig )
        resElem = self.DeserialElemOpt[ sig[ 0 ] ]( self, buf, offset )
        return ( resElem, 0 )

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
        'O' : SerialStruct  # TOK_STRUCT,
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
        'O' : DeserialStruct  # TOK_STRUCT,
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

