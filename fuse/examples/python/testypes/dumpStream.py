import os
import sys
import json
import base64
import io
import select
import time
import termios
import fcntl
import array
from rpcf.iolib import *

def Usage():
    print( "Usage: dump the data to a file as from the peer to stream_0 " )
    print( "\tpython3 dumpStream.py  <path to service point > [path to the dump file]" )
    print( "\tif the path to the dump file is not given, it will dump to file /tmp/stmdump" )

def dumpStream() :
    error = 0
    try:
        svcdir = sys.argv[ 1 ] + "/streams"
        if len( sys.argv ) > 2 :
            dumpFile = sys.argv[ 2 ]
        else:
            dumpFile = "/tmp/stmdump"
    except Exception as err:
        Usage()
        error = 22
        return error

    try:
        stmFile = svcdir + "/stream_0"
        stmfp = open( stmFile, "rb" )
        dumpfp = open( dumpFile, "wb")

        pollobj = select.poll()
        pollobj.register(stmfp, select.POLLIN)
        print( time.time())

        while True:
            #receive response if any
            fdlist = pollobj.poll()
            if fdlist == [] :
                continue
            fd, flags = fdlist[0]
            if ( flags & select.POLLHUP or
                flags & select.POLLERR ):
                break
            size = getBytesAvail(stmfp)
            if size == 0 :
                break
            data = stmfp.read(size)
            while len( data ) > 0:
                dumpfp.write(data)
                data = stmfp.read(size)

        print( "done!")
        print( time.time())
        stmfp.close()
        dumpfp.close()
    except Exception as err:
        print( err )
        dumpfp.close()
        stmfp.close()
    finally :
        return error


def main() :
    return dumpStream()

if __name__ == "__main__":
    main()
