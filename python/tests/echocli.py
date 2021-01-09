import sys
import errno
import numpy as np
from rpcfrmwrk import *

sys.path.insert(0, '../')
from proxy import PyRpcContext, PyRpcProxy

#1. define the interface the CEchoServer provides
class CEchoClient:
    """Mandatory class member to define the
    interface name, which will be used to invoke
    the event handler if any
    """
    ifName = "CEchoServer"

    '''all the methods have the similiar
    #implementation, pass `self.sendRequest' with
    the interface name, method name and all the
    input parameters '''

    def Echo(self, text ):
        return self.sendRequest(
            self.ifName, "Echo", text )
 
    def EchoUnknown( self, byteBuf ):
        return self.sendRequest(
            self.ifName, "EchoUnknown", byteBuf )

    #return sum of i1+i2
    def Echo2( self, i1, i2 ):
        return self.sendRequest(
            self.ifName, "Echo2",
            np.int32( i1 ),
            np.float64( i2 ) )

    def EchoCfg( self, iCount, pObj ):
        return self.sendRequest(
            self.ifName, "EchoCfg",
            np.int32( iCount ), pObj )

#2. aggregrate the interface class and the PyRpcProxy
# class by CEchoProxy to pickup the python-cpp
# interaction support

class CEchoProxy(CEchoClient, PyRpcProxy):
    def __init__(self, pIoMgr, strDesc, strObjName) :
        super( CEchoClient, self ).__init__(
            pIoMgr, strDesc, strObjName )

def HandleResp( resp, hint ) :
    if resp is None :
        ret = -errno.EFAULT;
        return ret;
    ret = resp[ 0 ];
    if ret is None :
        ret = -errno.EFAULT
        return ret
    elif ret < 0 :
        print( "error occured in ", hint, ret );
    elif resp[ 0 ] < 0:
        print( "error occured from peer", resp[ 0 ] )
        ret = -errno.ENOENT
    else :
        respArgs = resp[ 1 ]
        if respArgs is None :
            return -errno.EFAULT
        if hint != "EchoCfg" :
            print( "response is \"",
                respArgs[ 0 ],"\"" )
        else :
            pObj = resp[ 1 ][ 1 ];
            pCfg = cpp.CastToCfg( pObj )
            oResp = cpp.CParamList( pCfg )
            ret = oResp.GetSize()
            if ret[ 0 ] == 0 and ret[ 1 ] > 0 :
                retText = oResp.GetStrProp( 0 )
                print( "response is \"",
                    retText[ 1 ], "\"" )
            ret = ret[ 0 ]

    return ret

def test_main() : 
    ret = 0
    oContext = PyRpcContext();
    with oContext :
        print( "start to work here..." )
        oProxy = CEchoProxy( oContext.pIoMgr,
            "../../test/debug64/echodesc.json",
            "CEchoServer" );

        ret = oProxy.GetError() 
        if ret < 0 :
            return ret

        with oProxy :
            for i in range( 100 ) :
                #Echo a plain text
                resp = oProxy.Echo( "Are you ok" );
                ret = HandleResp( resp, "Echo" );
                if ret < 0 :
                    break;

                #Add to numbers and retun the sum
                resp = oProxy.Echo2( 2, 3.0 );
                ret = HandleResp( resp, "Echo2" );
                if ret < 0 :
                    break;

                #Echo whatever a block of binary data
                f = open( "/usr/bin/sh", "rb" )
                f.seek(1024);
                o = f.read( 16 );
                print( o );
                f.close();
                resp = oProxy.EchoUnknown( o );
                ret = HandleResp( resp, "EchoUnknown" );
                if ret < 0 :
                    break;

                #Echo a serializable object held
                # by oParams
                oParams = cpp.CParamList();
                oParams.PushStr( "This is EchoCfg" );
                pObjSend = oParams.GetCfgAsObj();
                resp = oProxy.EchoCfg( 1, pObjSend );
                ret = HandleResp( resp, "EchoCfg" );
                if ret < 0 :
                    break;
    return ret 

ret = test_main()
quit( ret )
