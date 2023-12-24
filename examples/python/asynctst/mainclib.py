# GENERATED BY RIDLC, MAKE SURE TO BACKUP BEFORE RUNNING RIDLC AGAIN
# Copyright (C) 2023  zhiming <woodhead99@gmail.com>
# This program can be distributed under the terms of the GNU GPLv3.
# ridlc -pbO . ../../asynctst.ridl 
from typing import Tuple
from rpcf.rpcbase import *
from rpcf.proxy import *
from seribase import CSerialBase
from asynctststructs import *
import errno

from AsyncTestcli import CAsyncTestProxy
import os
import time
import sys
import getopt
import stat

def Usage():
    print( ( "Usage: %s [OPTION]\n" +
        "\t [ -a to enable authentication ]\n" +
        "\t [ -d to run as a daemon ]\n" +
        "\t [ -i <ip address> to specify the destination ip address. ]\n"
        "\t [ -p <port number> to specify the tcp port number to ]\n"
        "\t [ -k to run as a kinit proxy. ]\n"
        "\t [ -l <user name> login with the user name and then quit. ]\n"
        "\t [ --driver <path> to specify the path to the customized 'driver.json'. ]\n"
        "\t [ --objdesc <path> to specify the path to the object description file. ]\n"
        "\t [ --router <path> to specify the path to the customized 'router.json'. ]\n"
        "\t [ --instname <name> to specify the server instance name to connect'. ]\n"
        "\t [ --sainstname <name> to specify the stand-alone router instance name to connect'. ]\n"
        "\t [ -h this help ]\n" ) % ( sys.argv[ 0 ] ),
        file=sys.stderr )

import signal
bExit = False
def SigHandler( signum, frame ):
    global bExit
    bExit = True

def KProxyLoop( strUserName : str = None ) -> int:
    global bExit
    if strUserName is None:
        while not bExit:
            time.sleep( 3 )
        print( "\n" )
        return 0
    strCmd = "kinit -ki " + strUserName
    return -os.system( strCmd )

def MainEntryCli() :
    ret = 0
    signal.signal( signal.SIGINT, SigHandler)
    try:
        params=dict()
        argv = sys.argv[1:]
        try:
            opts, args = getopt.getopt(argv, "hadi:p:kl:",
                [ "driver=", "objdesc=", "router=", "instname=", "sainstname=" ] )
        except:
            Usage()
            return -errno.EINVAL
        for opt, arg in opts:
            if opt in ('-a'):
                params[ 'bAuth' ] = True
            elif opt in ('-k'):
                params[ 'bKProxy' ] = True
            elif opt in ('-l'):
                params[ 'bKProxy' ] = True
                params[ 'UserName' ] = arg
            elif opt in ('-d'):
                params[ 'bDaemon' ] = True
            elif opt in ('-i'):
                params[ 'ipaddr' ] = arg
            elif opt in ('-p'):
                params[ 'portnum' ] = arg
            elif opt in ('--driver', '--router', '--objdesc' ):
                try:
                    stinfo = os.stat( arg )
                    if not stat.S_ISREG( stinfo.st_mode ) and not stat.S_ISLNK( stinfo.st_mode):
                        raise Exception( "Error invalid file '%s'" % arg )
                except Exception as err:
                    print( err )
                    Usage()
                    sys.exit( 1 )
                if opt == '--driver':
                    params[ 'driver' ] = arg
                elif opt == '--router':
                    params[ 'router' ] = arg
                else:
                    params[ 'objdesc' ] = arg
            elif opt == '--instname':
                params[ 'instname' ] = arg
                if 'sainstname' in params:
                    print( 'Error specifying both instname and sainstname')
                    Usage()
                    sys.exit( 1 )
            elif opt == '--sainstname':
                params[ 'sainstname' ] = arg
                if 'instname' in params:
                    print( 'Error specifying both instname and sainstname')
                    Usage()
                    sys.exit( 1 )
            elif opt in ('-h'):
                Usage()
                sys.exit( 0 )
            else:
                Usage()
                sys.exit( 1 )
        if 'bKProxy' in params and not 'bAuth' in params:
            print( "Error '-k' or '-l' is only valid when '-a' is present ")
            Usage()
            sys.exit( 1 )
        params[ 'ModName' ] = "asynctstcli"
        params[ 'AppName' ] = "asynctst"
        params[ 'Role' ] = 1
        if 'objdesc' in params:
            strPath_ = params[ 'objdesc' ]
        else:
            strPath_ = os.path.dirname( os.path.realpath( __file__ ) )
            strPath_ += '/asynctstdesc.json'
        ret, strNewDesc = \
            UpdateObjDesc( strPath_, params )
        if ret < 0:
            print( 'Error UpdateObjDesc', ret )
            return ret
        if isinstance( strNewDesc, str ) and len( strNewDesc ):
            strPath_ = strNewDesc
            params[ 'objdesc' ] = strNewDesc
        oContext = PyRpcContext( params )
        with oContext as ctx:
            if ctx.status < 0:
                ret = ctx.status
                print( os.getpid(), 
                    "Error start PyRpcContext %d" % ret )
                return ret
            
            print( "start to work here..." )
            if 'bKProxy' in params and params[ 'bKProxy' ]:
                if 'UserName' in params:
                    strUser = params[ 'UserName' ]
                else:
                    strUser = None
                return KProxyLoop( strUser )
            oProxy_AsyncTest = CAsyncTestProxy( ctx.pIoMgr,
                strPath_, 'AsyncTest' )
            ret = oProxy_AsyncTest.GetError()
            if ret < 0 :
                return ret
            
            with oProxy_AsyncTest as oProxy:
                global bExit
                try:
                    ret = oProxy.GetError()
                    if ret < 0 :
                        raise Exception( 'start proxy failed' )
                    state = oProxy.oInst.GetState()
                    while state == cpp.stateRecovery :
                        time.sleep( 1 )
                        state = oProxy.oInst.GetState()
                        if bExit:
                            break
                    if state != cpp.stateConnected or bExit:
                        return ErrorCode.ERROR_STATE
                    ret = maincli( oProxy )
                except Exception as err:
                    print( err )
                
            oProxy_AsyncTest = None
        oContext = None
        
    finally:
        if 'objdesc' in params:
            strDesc = params[ 'objdesc' ]
            if strDesc[:12] == '/tmp/rpcfod_':
                os.unlink( strDesc )
    return ret
    
#------customize the method below for your own purpose----
def maincli( 
    oProxy: CAsyncTestProxy ) -> int:
    '''
    adding your code here
    Calling a proxy method like
    'oProxy.LongWait( i0 )'
    '''
    while True:
        i0 = "hello, LongWait " + str( os.getpid() )
        pret = oProxy.LongWait( i0 )
        if pret[ 0 ] < 0 :
            ret = pret[ 0 ]
            OutputMsg( "LongWait failed with error " + str(ret) )
            break
        OutputMsg( "LongWait completed with response " +
            pret[ 1 ][ 0 ] )
            
        pret = oProxy.LongWaitNoParam( "no context" )
        if pret[ 0 ] < 0:    
            ret = pret[ 0 ]
            OutputMsg( "LongWaitNoParam failed with error " +
                str(ret) )
            break

        if pret[ 0 ] == 65537 :
            oProxy.WaitForComplete()
            ret = oProxy.GetError()
            if ret == 0 :
                OutputMsg( "LongWaitNoParam completed" )
            else :
                OutputMsg( "LongWaitNoParam failed with error " +
                    str(ret) )
        else :
            OutputMsg( "LongWaitNoParam complete with " +
                pret[ 1 ][ 0 ] )
                
        i0 = "hello, LongWait2"
        pret = oProxy.LongWait2( i0 )    
        if pret[ 0 ] < 0 :
            ret = pret[ 0 ]
            OutputMsg( "LongWait2 failed with error " + str(ret) )
            break
        OutputMsg( "LongWait2 completed with response " +
            pret[ 1 ][ 0 ] )
        ret = 0
        break
    return 0
    
if __name__ == '__main__' :
    ret = MainEntryCli()
    quit( -ret )