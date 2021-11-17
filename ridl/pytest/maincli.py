from rpcf.rpcbase import *
from rpcf.proxy import PyRpcProxy, PyRpcContext
from rpcf.proxy import ErrorCode as EC
import errno
from rpcf.proxy import OutputMsg
from SimpFileSvccli import CSimpFileSvcProxy
from StreamSvccli import CStreamSvcProxy
from fullteststructs import *
import time
import threading as tr

def SyncTests( oProxy ) :
    ret = oProxy.Ping();
    if ret[ 0 ] == 0 :
        OutputMsg( "Ping complete" )
    
    ret = oProxy.Echo( "hello, world" )
    if ret[ 0 ] == 0 :
        OutputMsg( "echo complete" )
    
    buf = bytearray(
        "hello, Unknown".encode() )
    ret = oProxy.EchoUnknown( buf )
    if ret[ 0 ] == 0 :
        strMsg = "echo unknown complete"
        OutputMsg( strMsg  )
        print( ret[ 1 ][ 0 ].decode() )
    else :
        strMsg = "echo unknown error %d"
        OutputMsg( strMsg % ret[ 0 ] )

    oParams = cpp.CParamList();
    pCfg = oParams.GetCfgAsObj();

    #set a string to property 0
    oParams.PushStr( "echoCfg" );
    ret = oProxy.EchoCfg( pCfg );

    if ret[ 0 ] == 0 :
        strMsg = "echo cfg complete"
        pObj = ret[ 1 ][ 0 ]
        pCfg = cpp.CastToCfg( pObj )
        oResp = cpp.CParamList( pCfg )
        ret = oResp.GetStrProp( 0 );
        OutputMsg( strMsg )
        print( ret[ 1 ] )
    else :
        strMsg = "echo cfg error %d"
        OutputMsg( strMsg % ret[ 0 ] )
    
    ret = oProxy.EchoMany(
        3, 1, 4, 1.0, 5.0, "EchoMany" )
    if ret[ 0 ] == 0 :
        strMsg = "echo many complete"
        plist = ret[ 1 ]
        OutputMsg( strMsg )
        print( plist )
    else :
        strMsg = "echo many failed"
        OutputMsg( strMsg % ret[ 0 ] )

    fi = FILE_INFO()
    fi.szFileName = "EchoStruct"
    fi.fileSize = 123
    fi.bRead = True
    fi.fileHeader.extend(
        "EchoStructHeader".encode() )
    listStrs = list()
    listStrs.append( "elem00" );
    listStrs.append( "elem01" );
    fi.vecLines.append( listStrs )
    listStrs1 = list()
    listStrs1.append( "elem10" );
    listStrs1.append( "elem11" );
    fi.vecLines.append( listStrs1 )

    val = bytearray()
    val.extend( "haha".encode() )
    fi.vecBlocks[ 0 ] = val
        
    val1 = bytearray()
    val1.extend( "bobo".encode() )
    fi.vecBlocks[ 1 ] = val1

    ret = oProxy.EchoStruct( fi );
    if ret[ 0 ] == 0 :
        strMsg = "echo struct complete"
        OutputMsg( strMsg )
    else :
        strMsg = "echo struct failed %d"
        OutputMsg( strMsg % ret[ 0 ] )

    ret = oProxy.KAReq( 2 )
    if ret[ 0 ] == 0 :
        strMsg = "KAReq complete"
        OutputMsg( strMsg )
    else :
        strMsg = "KAReq failed %d"
        OutputMsg( strMsg % ret[ 0 ] )


def AsyncTests( oProxy ) :
    
    ret = oProxy.KAReq2( 365, 1 )
    if ret[ 0 ] == 0 :
        strMsg = "KAReq2 succeeded %d"
        OutputMsg( strMsg % ret[ 0 ] )
    elif ret[ 0 ] < 0 :
        strMsg = "KAReq2 failed %d"
        OutputMsg( strMsg % ret[ 0 ] )

    if ret[ 0 ] != EC.STATUS_PENDING :
        return
    
    strMsg = "KAReq2 is pending %d"
    OutputMsg( strMsg % ret[ 1 ] )
    time.sleep( 3 )
    return

def UploadTest( oProxy, hStream : int ) :

    for i in range( 15 ) :
        blockSize = ( 1<<i ) * 1024
        ret = oProxy.UploadFile( i,
            "block", hStream, 0, blockSize )

        if ret[ 0 ] < 0 :
            oProxy.status = ret[ 0 ]
            break
        if ret[ 0 ] == EC.STATUS_PENDING :
            oProxy.sem.acquire()
    
        buf = bytearray( blockSize )
        ret = oProxy.WriteStream( hStream, buf )
        if ret < 0 :
            oProxy.status = ret
            strMsg = "upload %dKB bytes failed(%d)" % ( blockSize, ret )
            OutputMsg( strMsg )
            break

        OutputMsg( "uploaded %d Bytes" % blockSize );
        ret = oProxy.ReadStream( hStream )
        if ret[ 0 ] < 0 :
            oProxy.status = ret
            break
        OutputMsg(
            "peer received message %s" % ret[ 1 ].decode() )

    return oProxy.status

def DownloadTest( oProxy, hStream : int ) :

    for i in range( 15 ) :
        blockSize = ( 1<< i ) * 1024
        ret = oProxy.DownloadFile( i,
            "block", hStream, 0, blockSize )

        if ret[ 0 ] < 0 :
            oProxy.status = ret[ 0 ]
            break
        if ret[ 0 ] == EC.STATUS_PENDING :
            oProxy.sem.acquire()
    
        OutputMsg( "downloading %d Bytes" % blockSize );
        while blockSize > 0 :
            ret = oProxy.ReadStream( hStream )
            if ret[ 0 ] < 0 :
                oProxy.status = ret
                break

            count = len( ret[ 1 ] )
            blockSize -= count


        buf = "ROK".encode()
        ret = oProxy.WriteStream( hStream, buf )
        if ret < 0 :
            oProxy.status = ret
            strMsg = "send ACK failed"
            OutputMsg( strMsg )
            break

        OutputMsg( "download complete"  )

    return oProxy.status

def StreamTests( oProxy )  :

    ret = oProxy.StartStream()
    if ret == 0 :
        OutputMsg( "StartStream failed" )
        return oProxy.GetError()

    hStream = ret;
    while True:
        OutputMsg( "Start stream test..." )
        listRet = oProxy.ReadStream( hStream )
        if listRet[ 0 ] < 0 :
            OutputMsg(
                "Channel READY ACK failed %d" % listRet[ 0 ] )
        else :
            OutputMsg(
                "peer Channel %s" % listRet[ 1 ].decode() )

        ret = oProxy.SpecifyChannel( hStream )
        if ret[ 0 ] < 0 :
            OutputMsg(
                "SpecifyChannel failed %d" % ret[ 0 ] )
            break

        OutputMsg( "SpecifyChannel Succeeded" )

        ret = oProxy.WriteStream( hStream,
            "hello, SpecifyChannel".encode() )
        if ret < 0 :
            OutputMsg(
                "WriteStream failed %d" % ret )
            break

        ret = oProxy.ReadStream( hStream )
        if ret[ 0 ] < 0 :
            ret = ret[ 0 ]
            OutputMsg(
                "ReadStream failed %d" % ret )
            break

        content = ret[ 1 ]
        OutputMsg( content.decode() )

        ret = UploadTest( oProxy, hStream )
        if ret < 0 :
            break

        ret = DownloadTest( oProxy, hStream )
        break

    oProxy.CloseStream( hStream )
    oProxy.status = ret
    return ret


def maincli() :
    ret = 0
    try:
        oContext = PyRpcContext( 'PyRpcProxy' )
        with oContext :
            print( "start to work here..." )
            oProxy = CSimpFileSvcProxy( oContext.pIoMgr,
                './fulltestdesc.json',
                'SimpFileSvc' )
            ret = oProxy.GetError()
            if ret < 0 :
                return ret
            
            oProxy2 = CStreamSvcProxy( oContext.pIoMgr,
                './fulltestdesc.json',
                'StreamSvc' )
            ret = oProxy2.GetError()
            if ret < 0 :
                OutputMsg( "error creating CStreamSvcProxy %d" % ret  )
                return ret

            with oProxy, oProxy2 :
                '''
                adding your code here
                Calling a proxy method like
                '''
                oProxy.Ping()
                SyncTests( oProxy )
                time.sleep( 1 )
                AsyncTests( oProxy )
                StreamTests( oProxy2 )
    except:
        return -errno.EFAULT

    return 0
    
ret = maincli()
quit( ret )
