import os
import sys
import json
import io

import errno
import termios
import time
import select
import fcntl
import array

def setBlocking( fp : object )->int:
    try:
        return fcntl.ioctl( fp.fileno(), 0x4a00 )
    except Exception as err:
        print( err )

def setNonBlocking( fp : object )->int:
    try:
        return fcntl.ioctl( fp.fileno(), 0x4a01 )
    except Exception as err:
        print( err )

def getBytesAvail( fp : object )->int:
    buf=array.array('i',[0])
    res = fcntl.ioctl( fp.fileno(), 0x80044a02, buf, 1 )
    if res == -1:
        return 0
    return buf[0]

def recvResp( respfp : object)->[object] :
    inputs = [respfp]
    # wait for the reponse, it could take
    # up to 2 min to return when something
    # wrong
    res = []
    inBuf = bytearray()
    while len( inBuf ) == 0:
        resp = select.select( inputs, [], [] )
        # read at the page boundary
        data = respfp.read(4096)
        if len( data ) > 0 and len( data ) < 4096:
            inBuf = data
            break
        while True:
            inBuf.extend(data)
            if len( data ) < 4096:
                break
            data = respfp.read(4096)
    
    pos = 0
    while pos < len( inBuf ):
        size = int.from_bytes(inBuf[pos:pos+4], 'big')
        if size == 0 :
            error = errno.ENODATA
            raise Exception( 'response with error %d' % error )
        payload = inBuf[ pos + 4 : pos + 4 + size ]
        if len( payload ) == 0:
            error = errno.EBADMSG
            raise Exception( 'partial response with error %d' % error )
        strResp = payload.decode( "UTF-8" )
        res.append( json.loads( strResp ) )
        pos += 4 + size
    
    return res

    
def sendReq( reqfp : object, reqObj : object ):
    strReq = json.dumps(reqObj)
    reqBytes = strReq.encode()
    iSize = len( reqBytes )
    try:
        # write the size to the req file
        reqfp.write( iSize.to_bytes(4, 'big') )
        # write the message body
        error = reqfp.write(reqBytes)
        if error < 0 :
            raise Exception( 'request failed with error %d' % error )
        # must flush to push the things
        # down to rpc-frmwrk
        reqfp.flush()
    except Exception as err:
        print( err )
    

def peekResp( respfp : object)->object :
    # read a four bytes integer as the
    # size of the response
    data = respfp.read(4)
    if len( data ) == 0 :
        return None
    size = int.from_bytes(data, 'big')
    if size == 0 :
        error = errno.ENODATA
        raise Exception( 'response with error %d' % error )

    # read the data from the file as is
    # the body of the response
    data = respfp.read(size)
    strResp = data.decode( "UTF-8")
    return json.loads(strResp)

def recvReq( reqfp : object ) -> [object]:
    return recvResp( reqfp )

def sendResp( respfp : object, resp:object ) :
    sendReq( respfp, resp )

def sendEvent( respfp : object, evt:object ) :
    sendReq( respfp, evt )
