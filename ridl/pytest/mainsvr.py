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
                    OutputMsg( "received msg size %d" % size )

                    oStmSvr.curSize -= size
                    if oStmSvr.curSize == 0:
                        OutputMsg( "test %d complete" % oStmSvr.numTest )
                        OutputMsg( "%d bytes transfered" % oStmSvr.totalSize )
                        oStmSvr.curTest = 0
                        buf = "ROK"
                        oStmSvr.WriteStream(
                            oStmSvr.hChannel, buf )

                    elif oStmSvr.curSize < 0 :
                        print( "error size is %d" % oStmSvr.size )
                        ret = -errno.ERANGE
                        break

                    continue
                    

    return ret

from examplestructs import *
ret = mainsvr()
quit( ret )
