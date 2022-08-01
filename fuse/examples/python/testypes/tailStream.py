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

def Usage():
    print( "Usage: dump the data over the stream channel to standard output" )
    print( "\tpython3 tailFile.py  <path to stream file >" )
    print( "" )

def tailStream() :
    error = 0
    try:
        stmFile = sys.argv[ 1 ]
    except Exception as err:
        Usage()
        error = 22
        return
    try:
        stmfp = open( stmFile, "rb" )

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
            size = 65536
            data = stmfp.read(size)
            while len( data ) > 0:
                print(data.decode(),end='')
                data = stmfp.read(size)

        print( "done!")
        print( time.time())
        stmfp.close()
    except Exception as err:
        print( err )
        stmfp.close()
    finally :
        return error


def main() :
    return tailStream()

if __name__ == "__main__":
    main()
