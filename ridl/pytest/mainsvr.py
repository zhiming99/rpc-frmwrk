from rpcf.rpcbase import *
from rpcf.proxy import *
import errno
import time
from SimpFileSvcsvr import CSimpFileSvcServer
from StreamSvcsvr import CStreamSvcServer

def mainsvr() :
    ret = 0
    oContext = PyRpcContext( 'PyRpcServer' )
    with oContext :
        print( "start to work here..." )
        oFileSvr = CSimpFileSvcServer( oContext.pIoMgr,
            './exampledesc.json',
            'SimpFileSvc' )
        ret = oFileSvr.GetError()
        if ret < 0 :
            return ret

        oStmSvr = CStreamSvcServer( oContext.pIoMgr,
            './exampledesc.json',
            'StreamSvc' )
        ret = oStmSvr.GetError()
        if ret < 0 :
            return ret
        
        with oFileSvr, oStmSvr :
            '''
            made change to the following code
            snippet for your own purpose
            '''
            count = 0
            while ( cpp.stateConnected == oFileSvr.oInst.GetState() and
                    cpp.stateConnected == oStmSvr.oInst.GetState() ):
                count+=1
            
                if oStmSvr.hChannel == 0 :
                    time.sleep( 1 )
                    oFileSvr.OnHelloWorld( None, "hello, world" )
                    continue

                if oStmSvr.curTest == 0 :
                    ret = oStmSvr.ReadStreamNoWait(
                        oStmSvr.hChannel )
                    if ret[ 0 ] < 0:
                        print( "error ReadStreamNoWait", ret )
                        time.sleep( 1 )
                        oFileSvr.OnHelloWorld( None, "hello, world" )
                        continue

                    buf = ret[ 1 ].decode()
                    OutputMsg( "received msg '%s'" % buf )

                    buf = "greeting from server"
                    oStmSvr.WriteStream(
                        oStmSvr.hChannel, buf )
                    continue

                if oStmSvr.curTest == 1 :
                    #upload test
                    ret = oStmSvr.ReadStream(
                        oStmSvr.hChannel )
                    if ret[ 0 ] < 0:
                        time.sleep( 1 )
                        oFileSvr.OnHelloWorld( None, "hello, world" )
                        continue

                    size = len( ret[ 1 ] )

                    oStmSvr.curSize -= size
                    if oStmSvr.curSize == 0:
                        OutputMsg( "test %d complete" % oStmSvr.numTest )
                        OutputMsg( "%d bytes uploaded" % oStmSvr.totalSize )
                        oStmSvr.curTest = 0
                        buf = "ROK"
                        oStmSvr.WriteStream(
                            oStmSvr.hChannel, buf )

                    elif oStmSvr.curSize < 0 :
                        print( "error size is %d" % oStmSvr.size )
                        ret = -errno.ERANGE
                        break

                    continue

                if oStmSvr.curTest == 2 :

                    #upload test
                    ret = 0
                    while oStmSvr.curSize > 0 :
                        size = oStmSvr.curSize
                        if size > 4 * 1024 * 1024 :
                            size = 4 * 1024 * 1024

                        buf = bytearray( size )
                        ret = oStmSvr.WriteStream(
                            oStmSvr.hChannel, buf )

                        if ret < 0:
                            break

                        oStmSvr.curSize -= size
                        #OutputMsg( "send bytes %d" % size )

                    if ret < 0 :
                        print( "error send bytes", ret )
                        time.sleep( 1 )
                        oFileSvr.OnHelloWorld( None, "hello, world" )
                        oStmSvr.curTest = 0
                        continue

                    if oStmSvr.curSize == 0:
                        OutputMsg( "test %d complete" % oStmSvr.numTest )
                        OutputMsg( "%d bytes downloaded" % oStmSvr.totalSize )
                        oStmSvr.curTest = 0
                        ret = oStmSvr.ReadStream( oStmSvr.hChannel )
                        if  ret[ 0 ] < 0 :
                            OutputMsg( "receiving ACK failed" )
                        else :
                            OutputMsg( ret[ 1 ].decode() )

                    elif oStmSvr.curSize < 0 :
                        print( "error size is %d" % oStmSvr.size )
                        ret = -errno.ERANGE
                        break
                    

    return ret

from examplestructs import *
ret = mainsvr()
quit( ret )
