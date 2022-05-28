import os
import sys
import json
import io
import select
import errno

import time
from iolib import *

def fctest() :
    error = 0
    svcdir = sys.argv[1]

    idx = 123456
    try:
        stmFile = svcdir + "/streams/stream_0"
        stmfd = os.open( stmFile, os.O_WRONLY | os.O_CREAT | os.O_DIRECT )
        stmfp = os.fdopen(stmfd, "wb")

        for j in range( 101 ) :
            greeting = "hello, world" + str( j ) + "\n"

            try:
                os.write( stmfd, greeting.encode("UTF-8"))
            except Exception as err:
                #sending too fast
                #wait for peer to consume them by selecting the fd
                if err.errno == errno.EAGAIN :
                    print( "flow control @loop %d" % j )
                    outNotify = [stmfp]
                    notifyList = select.select([], outNotify, [])
                    j-=1
                    continue
                break

        stmfd.close()
    except Exception as err:
        print( err )
        print( "usage: fctest.py <service path>")
        print( "\t<service path> is /'path to mountpoint'/connection_X/ActiveCancel")
        stmfd.close()
    finally :
        return error


def main() :
    fctest()

if __name__ == "__main__":
    main()
