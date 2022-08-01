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
    print( "Usage: send a file through the rpc-frmwrk stream channel." )
    print( "\tpython3 sendFile.py <path to file to send> <path to stream file >" )
    print( "" )

def sendStream() :
    error = 0

    try:
        srcFile = sys.argv[ 1 ]
        stmFile = sys.argv[ 2 ]
    except Exception as err:
        Usage()
        error = 22
        return error

    try:
        srcfp = open( srcFile, "rb" )
        stmfp = open( stmFile, "wb", buffering=0 )

        print( time.time())
        size = 65536 * 2
        count = 0
        while True:
            data = srcfp.read(size)
            if len( data ) > 0:
                stmfp.write(data)
            else:
                break
            count += size
            data=bytes()
            if count % ( 4 * 1024 * 1024 ) :
                print( "%d bytes transfered " % count )

        print( "done!")
        print( time.time())
        stmfp.close()
    except Exception as err:
        print( err )
        srcfp.close()
        stmfp.close()
    finally :
        return error


def main() :
    return sendStream()

if __name__ == "__main__":
    main()
