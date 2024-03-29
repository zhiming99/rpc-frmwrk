#Generated by RIDLC, make sure to backup before running RIDLC again
from typing import Tuple
from rpcf.rpcbase import *
from rpcf.proxy import *
from seribase import CSerialBase
from actcancelstructs import *
import errno

from ActiveCancelcli import CActiveCancelProxy
import os
import time


def maincli() :
    ret = 0
    oContext = PyRpcContext( 'PyRpcProxy' )
    with oContext :
        print( "start to work here..." )
        strPath_ = os.path.dirname( os.path.realpath( __file__ ) )
        strPath_ += '/actcanceldesc.json'
        oProxy = CActiveCancelProxy( oContext.pIoMgr,
            strPath_, 'ActiveCancel' )
        ret = oProxy.GetError()
        if ret < 0 :
            return ret
        
        with oProxy :
            state = oProxy.oInst.GetState()
            while state == cpp.stateRecovery :
                time.sleep( 1 )
                state = oProxy.oInst.GetState()
            if state != cpp.stateConnected :
                return ErrorCode.ERROR_STATE
            
            '''
            adding your code here
            Calling a proxy method like
            '''
            i0 = "hello, actctest!"
            pret = oProxy.LongWait( "haha", i0 )
            ret = pret[ 0 ]
            if ret < 0 :
                print( "LongWait failed with error " +  str(ret) ) 
                return ret

            if ret == 0 :
                print( "LongWait succeeded so quickly " + str( pret[ 1 ][ 0 ] ) )
                return ret

            if ret != ErrorCode.STATUS_PENDING :
                print( "LongWait returned with strange error " + str( pret[ 0 ] ) )
                return ret

            print( "LongWait is pending, wait 3 seconds " ) 
            time.sleep( 1 )
            print( "canceling LongWait request... " ) 
            iTaskId = pret[ 1 ]
            ret = oProxy.oInst.CancelRequest( iTaskId )
            if ret < 0 :
                print( "CancelRequest failed, wait it to complete or timeout" )
                oProxy.WaitForComplete()
            else:
                oProxy.WaitForComplete()
                ret = oProxy.GetError()
                if ret == ErrorCode.ERROR_USER_CANCEL:
                    ret = 0
                    print( "CancelRequest succeeded " + str( 0 ) )
                else:
                    print( "CancelRequest failed with error " + str( ret ) )

        oProxy = None            
    oContext = None
    return ret
    
ret = maincli()
quit( ret )
