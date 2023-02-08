import os
import json

import errno
import select
import fcntl
import array

def setBlocking( fp : object )->int:
    try:
        return fcntl.ioctl( fp.fileno(), 0x4a00 )
    except Exception as err:
        print( os.getpid(), "iolib.setBlocking ",
            err, ":", fp )

def setNonBlocking( fp : object )->int:
    try:
        return fcntl.ioctl( fp.fileno(), 0x4a01 )
    except Exception as err:
        print( os.getpid(), "iolib.setNonBlocking ",
            err, ":", fp )

def getBytesAvail( fp : object )->int:
    buf=array.array('i',[0])
    res = fcntl.ioctl( fp.fileno(), 0x80044a02, buf, 1 )
    if res == -1:
        return 0
    return buf[0]

def recvResp( respfp : object)->[int, list] :
    inputs = [respfp]
    # wait for the reponse, it could take
    # up to 2 min to return when something
    # wrong
    res = []
    inBuf = bytearray()
    error = 0
    try:
        while len( inBuf ) == 0:
            resp = select.select( inputs, [], [], 2.0 )
            # read at the page boundary
            data = respfp.read(8192)
            if len( data ) == 0 :
                #timeout
                continue
            if len( data ) > 0 and len( data ) < 8192:
                inBuf = data
                break
            while True:
                inBuf.extend(data)
                if len( data ) < 8192:
                    break
                data = respfp.read(8192)
    
        pos = 0
        while pos < len( inBuf ):
            size = int.from_bytes(inBuf[pos:pos+4], 'big')
            if size == 0 :
                error = -errno.ENODATA
                raise Exception( 'response with error %d' % error )
            if pos + 4 + size > len( inBuf ):
                error = -errno.ERANGE
                raise Exception( 'partial message with error %d' % error )
            payload = inBuf[ pos + 4 : pos + 4 + size ]
            strResp = payload.decode( "UTF-8" )
            res.append( json.loads( strResp ) )
            pos += 4 + size
        return [ 0, res ]
    except Exception as err:
        print( os.getpid(),
            "iolib.recvResp", err, ": ", respfp )
        if error == 0 :
            error = -errno.EFAULT
        return [ error, None ]
    
def sendReq( reqfp : object, reqObj : object ) -> int:
    strReq = json.dumps(reqObj)
    reqBytes = strReq.encode()
    iSize = len( reqBytes )
    try:
        # write the size to the req file
        reqfp.write( iSize.to_bytes(4, 'big') )
        # write the message body
        reqfp.write(reqBytes)
        # must flush to push the things
        # down to rpc-frmwrk
        reqfp.flush()
        return 0
    except Exception as err:
        print( os.getpid(),'iolib.sendReq ',
            err, ": ", reqfp, "---", reqObj )
        return -errno.EFAULT
    
'''
peekResp: read the non-blocking 'respfp' and return
immediately if there is no data. Unlike recvResp,
which wait in select till there is something to
read.
'''
def peekResp( respfp : object)->[ int, object ] :
    # read a four bytes integer as the
    # size of the response
    data = respfp.read(4)
    if len( data ) == 0 :
        return [ -errno.EAGAIN, None ]
    size = int.from_bytes(data, 'big')
    if size == 0 :
        return [ errno.ENODATA, None ]

    # read the data from the file as is
    # the body of the response
    data = respfp.read(size)
    strResp = data.decode( "UTF-8")
    return [ 0, json.loads(strResp) ]

def recvReq( reqfp : object ) -> [int, object]:
    return recvResp( reqfp )

def sendResp( respfp : object, resp:object )->int :
    return sendReq( respfp, resp )

def sendEvent( respfp : object, evt:object )->int :
    return sendReq( respfp, evt )

def recvMsg( respfp : object)->[int, list] :
    inputs = [respfp]
    # wait for the reponse, it could take
    # up to 2 min to return when something
    # wrong
    res = []
    inBuf = bytearray()
    error = 0
    try:
        
        # read at the page boundary
        data = respfp.read(8192)
        if len( data ) > 0 and len( data ) < 8192:
            inBuf = data
        elif len( data ) == 8192:
            while True:
                inBuf.extend(data)
                if len( data ) < 8192:
                    break
                data = respfp.read(8192)
        else:
            error = -errno.ENOMSG
            raise Exception( "Error no msg found %d" % error )
    
        pos = 0
        while pos < len( inBuf ):
            size = int.from_bytes(inBuf[pos:pos+4], 'big')
            if size == 0 :
                error = -errno.ENODATA
                raise Exception( 'response with error %d' % error )
            if pos + 4 + size > len( inBuf ):
                error = -errno.ERANGE
                raise Exception( 'partial message with error %d' % error )
            payload = inBuf[ pos + 4 : pos + 4 + size ]
            strResp = payload.decode( "UTF-8" )
            res.append( json.loads( strResp ) )
            pos += 4 + size
        return [ 0, res ]
    except Exception as err:
        print( os.getpid(),
            "iolib.recvMsg", err, ": ", respfp )
        if error == 0 :
            error = -errno.EFAULT
        return [ error, None ]
