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
from iolib import *

def dumpStream() :
    error = 0
    svcdir = sys.argv[ 1 ] + "/streams"
    if len( sys.argv ) > 2 :
        dumpFile = sys.argv[ 2 ]
    else:
        dumpFile = "/tmp/stmdump"
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
    dumpStream()

if __name__ == "__main__":
    main()
