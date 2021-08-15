from rpcf.rpcbase import *
import struct
import errno
from rpcf.proxy import *
from . import g_mapStructs

class CStructFactoryBase:
    global mapStructs
    def __init__( self ) : 
        pass

    @staticmethod
    def Create( iMsgId : int ) -> object :
        if iMsgId in g_mapStructs :
            return g_mapStructs[ iMsgId ]()
        return None

    @staticmethod
    def AddStruct( iMsgId : int, clsObj : object ) -> int :
        if iMsgId in g_mapStructs :
            return -errno.EEXIST
        g_mapStructs[ iMsgId ] = clsObj
        return 0

class CStructFactory( metaclass = CStructFactoryBase ) :
    pass

class CSerialBase :

    def __init__( self, pIf : PyRpcServices = None ) :
        self.pIf = pIf

    def SerialBool( self, buf : bytearray, val : int ) :
        offset = len( buf )
        struct.pack_into( "!B", buf, offset, val )

    def SerialInt8( self, buf : bytearray, val : int ) :
        offset = len( buf )
        struct.pack_into( "!B", buf, offset, val )

    def SerialInt16( self, buf : bytearray, val : int ) :
        offset = len( buf )
        struct.pack_into( "!H", buf, offset, val )

    def SerialInt32( self, buf : bytearray, val : int ) :
        offset = len( buf )
        struct.pack_into( "!I", buf, offset, val )

    def SerialInt64( self, buf : bytearray, val : int ) :
        offset = len( buf )
        struct.pack_into( "!Q", buf, offset, val )

    def SerialFloat( self, buf : bytearray, val : float ) :
        offset = len( buf )
        struct.pack_into( "!f", buf, offset, val )

    def SerialDouble( self, buf : bytearray, val : float ) :
        offset = len( buf )
        struct.pack_into( "!d", buf, offset, val )

    def SerialString( self, buf : bytearray, val : bytes ) :
        offset = len( buf )
        struct.pack_into( "!I", buf, offset, len( val ) );
        offset = len( buf )
        struct.pack_into( "!s", buf, offset, val );

    def SerialBuf( self, buf : bytearray, val : bytearray ) :
        offset = len( buf )
        struct.pack_into( "!I", buf, offset, len( val ) );
        offset = len( buf )
        struct.pack_into( "!s", buf, offset, val );

    def SerialObjPtr( self, buf : bytearray, val : cpp.ObjPtr )-> int :
        objBuf = val.SerialToByteArray();
        if objBuf is None :
            return -errno.EFAULT
        if type( objBuf ) is not bytearray :
            return -errno.EFAULT
        buf.append( objBuf )
        return 0

    def SerialHStream( self, buf : bytearray, val : int ) -> int:
        if self.pIf is None :
            return -errno.EFAULT
        oInst = self.oInst
        isServer = self.oInst.IsServer()
        if isServer :
            ret = oInst.GetIdHashByChan( val )
        else :
            ret = oInst.GetPeerIdHash( val )

        if ret == 0 :
            return -errno.EFAULT
        self.SerialInt64( buf, val )
        return 0 

    def SerialStruct( self, buf: bytearray, val : object ) -> int:
        try:
            val.pIf = self.pIf
            return val.Serialize( buf )
        except :
            return -errno.EFAULT

    def SerialArray( self, buf: bytearray, val : list, sig: str ) -> int:
        sigLen = len( sig )

        if sig[ 0 ] is not '(' :
            return -errno.EINVAL
        if sig[ sigLen -1 ] is not ')':
            return -errno.EINVAL
        if len( sig ) < 2 :
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
        if sig[ 0 ] is not '[' :
            return -errno.EINVAL
        if sig[ sigLen -1 ] is not ']':
            return -errno.EINVAL
        if len( sig ) < 2 :
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

        for ( key, value ) in val :
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

        self.SerialElemOpt[ sig[ 0 ] ]( buf, val )
        return 0

    def DeserialInt64( self, buf : bytearray, offset : int ) -> tuple( int, int ) :
        val = struct.unpack_from( "!Q", buf, offset )
        return ( val[ 0 ], offset + 8 )

    def DeserialInt32( self, buf : bytearray, offset : int ) -> tuple( int, int ) :
        val = struct.unpack_from( "!I", buf, offset )
        return ( val[ 0 ], offset + 4 )

    def DeserialInt16( self, buf : bytearray, offset : int ) -> tuple( int, int ) :
        val = struct.unpack_from( "!H", buf, offset )
        return ( val[ 0 ], offset + 2 )

    def DeserialBool( self, buf : bytearray, offset : int ) -> tuple( bool, int ) :
        val = struct.unpack_from( "!B", buf, offset )
        return ( val[ 0 ], offset + 1 )

    def DeserialInt8( self, buf : bytearray, offset : int ) -> tuple( int, int ) :
        val = struct.unpack_from( "!B", buf, offset )
        return ( val[ 0 ], offset + 1 )

    def DeserialFloat( self, buf : bytearray, offset : int ) -> tuple( float, int ) :
        val = struct.unpack_from( "!f", buf, offset )
        return ( val[ 0 ], offset + 4 )

    def DeserialDouble( self, buf : bytearray, offset : int ) -> tuple( float, int ) :
        val = struct.unpack_from( "!d", buf, offset )
        return ( val[ 0 ], offset + 8 )

    def DeserialHStream( self, buf : bytearray, offset : int ) -> tuple( int, int ) :
        if self.pIf is None :
            return -errno.EFAULT
        val = self.DeserialInt64( self, buf, offset )
        oInst = self.oInst
        ret = oInst.GetChanByIdHash( val[ 0 ] )
        if ret == ErrorCode.INVALID_HANDLE :
            return -errno.EFAULT
        return ( ret, offset + 8 )

    def DeserialString( self, buf : bytearray, offset : int ) -> tuple( str, int ) :
        ret = self.DeserialInt32( self, buf, offset );
        offset += 4
        iSize = ret[ 0 ]
        if iSize is None :
            return ( None, 0 )
        if  iSize == 0:
            return ( "", offset )
        return ( str( buf[ offset : offset + iSize ] ), offset + iSize )

    def DeserialBuf( self, buf : bytearray, offset : int ) -> tuple( bytearray, int ) :
        ret = self.DeserialInt32( self, buf, offset )
        offset += 4
        iSize = ret[ 0 ]
        if iSize is None :
            return ( None, 0 )
        if iSize == 0 :
            return ( bytearray(), offset )
        return ( buf[ offset : offset + iSize ], offset + iSize )
        
    def DeserialObjPtr( self, buf : bytearray, offset : int ) -> tuple( cpp.ObjPtr, int ) :
        pNewObj = cpp.ObjPtr()
        listRet = pNewObj.DeserialObjPtr( buf, offset )
        if listRet[ 0 ] < 0 :
            return ( None, 0 )
        return ( listRet[ 0 ], listRet[ 1 ] )

    def DeserialStruct( self, buf : bytearray, offset : int ) -> tuple( object, int ) :
        ret = struct.unpack_from( "!I", buf, offset )
        structId = ret[ 0 ]
        structInst = CStructFactoryBase.Create( structId )
        structInst.pIf = self.pIf
        ret = structInst.Deserialize( buf, offset )
        if ret[ 0 ] < 0 :
            return ( None, 0 )
        return ( structInst, ret[ 1 ] )

    def DeserialArray( self, buf : bytearray, offset : int, sig : str )->tuple( list, int ):
        sigLen = len( sig )
        if sig[ 0 ] is not '(' :
            return -errno.EINVAL
        if sig[ sigLen -1 ] is not ')':
            return -errno.EINVAL
        if len( sig ) < 2 :
            return -errno.EINVAL
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

    def DeserialMap( self, buf : bytearray, offset : int, sig : str ) -> tuple( dict, int ):
        sigLen = len( sig )
        if sig[ 0 ] is not '(' :
            return -errno.EINVAL
        if sig[ sigLen -1 ] is not ')':
            return -errno.EINVAL
        if len( sig ) < 2 :
            return -errno.EINVAL
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

    def DeserialElem( self, buf : bytearray, offset : int, sig : str ) -> tuple( object, int ):
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
