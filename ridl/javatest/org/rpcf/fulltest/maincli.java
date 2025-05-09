// GENERATED BY RIDLC. MAKE SURE TO BACKUP BEFORE RE-COMPILING.
package org.rpcf.fulltest;

import org.rpcf.rpcbase.*;

import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.TimeUnit;
import java.util.Arrays;
public class maincli
{
    public static JavaRpcContext m_oCtx;
    public static String getDescPath( String strName )
    {
        String strDescPath =
            maincli.class.getProtectionDomain().getCodeSource().getLocation().getPath();
        String strDescPath2 = strDescPath + "/org/rpcf/fulltest/" + strName;
        java.io.File oFile = new java.io.File( strDescPath2 );
        if( oFile.isFile() )
            return strDescPath2;
        strDescPath += "/" + strName;
        oFile = new java.io.File( strDescPath );
        if( oFile.isFile() )
            return strDescPath;
        return "";
    }
    static int SyncTests( SimpFileSvccli oProxy )
    {
        JRetVal jret = null;
        do {
            // Ping
            jret = oProxy.Ping();
            if (jret.SUCCEEDED())
                rpcbase.JavaOutputMsg("Ping complete");
            else {
                rpcbase.JavaOutputMsg("Ping failed");
                break;
            }

            // Echo
            jret = oProxy.Echo("Hello, World!");
            if (jret.SUCCEEDED())
                rpcbase.JavaOutputMsg(
                        "Echo complete " + (String) jret.getAt(0));

            byte[] buf = "hello, Unknown".getBytes(StandardCharsets.UTF_8);
            jret = oProxy.EchoUnknown(buf);
            if (jret.SUCCEEDED()) {
                rpcbase.JavaOutputMsg("echo unknown complete");
                byte[] resp = (byte[]) jret.getAt(0);
                rpcbase.JavaOutputMsg(
                    new String( resp, StandardCharsets.UTF_8 ) );
            } else {
                rpcbase.JavaOutputMsg(
                        String.format("echo unknown error %d", jret.getError()));
                break;
            }
            CParamList oParams = new CParamList();
            jret = (JRetVal) oParams.GetCfg();
            if (jret.ERROR())
                break;

            ObjPtr pCfg = (ObjPtr) jret.getAt(0);
            oParams.PushStr("echoCfg");
            jret = oProxy.EchoCfg(pCfg);
            if (jret.SUCCEEDED()) {
                // check the response
                String strMsg = "echo cfg complete";
                ObjPtr pObj = (ObjPtr) jret.getAt(0);
                CfgPtr pResp = rpcbase.CastToCfg(pObj);
                CParamList oResp = new CParamList(pResp);
                jret = (JRetVal) oResp.GetStrProp(0);
                if (jret.SUCCEEDED()) {
                    rpcbase.JavaOutputMsg(
                            String.format("%s %s", strMsg, (String) jret.getAt(0)));
                } else {
                    rpcbase.JavaOutputMsg(
                            String.format("error echo cfg %d", jret.getError()));
                    break;
                }
            }

            jret = oProxy.EchoMany(3, (short) 1,
                    4, (float) 1.0, 5.0, "EchoMany");
            if (jret.SUCCEEDED()) {
                String strMsg = "echo many complete";
                int i1r = jret.getAtInt(0);
                short i2r = jret.getAtShort(1);
                long i3r = jret.getAtLong(2);
                float i4r = jret.getAtFloat(3);
                double i5r = jret.getAtDouble(4);
                String i6r = (String) jret.getAt(5);
                rpcbase.JavaOutputMsg(
                        String.format("%s, %d, %d, %d, %f, %g, %s",
                                strMsg, i1r, i2r, i3r, i4r, i5r, i6r));
            } else {
                rpcbase.JavaOutputMsg(
                        String.format("error echo many %d", jret.getError()));
                break;
            }

            FILE_INFO fi = new FILE_INFO();
            fi.szFileName = "EchoStruct";
            fi.fileSize = 123456;
            fi.bRead = true;
            fi.fileHeader = "EchoStructHeader".getBytes(StandardCharsets.UTF_8);
            fi.vecLines = new String[][]{
                    {"elem00", "elem01"}, {"elem10", "elem11"}};
            fi.vecBlocks = new HashMap<Integer, byte[]>();
            fi.vecBlocks.put(0, "haha".getBytes(StandardCharsets.UTF_8));
            fi.vecBlocks.put(1, "bobo".getBytes(StandardCharsets.UTF_8));
            fi.vecBlocks.put(2, "cici".getBytes(StandardCharsets.UTF_8));

            jret = oProxy.EchoStruct(fi);
            if (jret.SUCCEEDED()) {
                FILE_INFO fir = (FILE_INFO) jret.getAt(0);
                rpcbase.JavaOutputMsg("echo struct complete");
                rpcbase.JavaOutputMsg("\tszFileName = " + fir.szFileName);
                rpcbase.JavaOutputMsg("\tfileSize = " + Long.toString(fir.fileSize));
                rpcbase.JavaOutputMsg("\tbRead = " + Boolean.toString(fir.bRead));
                rpcbase.JavaOutputMsg("\tfileHeader = " + fir.fileHeader.toString());
                rpcbase.JavaOutputMsg("\tvecLines = " + fir.vecLines.toString());
                rpcbase.JavaOutputMsg("\tvecBlocks = {" );
                for( Map.Entry<Integer, byte[]> entry : fir.vecBlocks.entrySet())
                    rpcbase.JavaOutputMsg(
                            "\t\t" + new String( entry.getValue(), StandardCharsets.UTF_8) );
                rpcbase.JavaOutputMsg("\t}" );
            }
            else
            {
                rpcbase.JavaOutputMsg(
                        String.format("error echo struct %d", jret.getError()));
                break;
            }


        }while( false );
        if( jret != null)
            return jret.getError();
        return -RC.ERROR_FAIL;
    }

    static int AsyncTests( SimpFileSvccli oProxy )
    {
        JRetVal jret = null;
        do{
            jret = oProxy.KAReq2( "haha", 1 );
            if( jret.SUCCEEDED()) {
                rpcbase.JavaOutputMsg(
                        String.format("KAReq2 sent %d", jret.getError()));
                break;
            }
            else if( jret.ERROR() ) {
                rpcbase.JavaOutputMsg(
                        String.format("KAReq2 failed %d", jret.getError()));
                break;
            }
            else if( !jret.isPending() )
                break;
            rpcbase.JavaOutputMsg( "KAReq2 is pending");
            try {
                TimeUnit.SECONDS.sleep(3);
            } catch (InterruptedException e) {
            }
        }while( false );
        if( jret != null )
            return jret.getError();
        return -RC.ERROR_FAIL;
    }

    static void WaitForSem( StreamSvccli oProxy )
    {
        while( true )
            try {
                oProxy.m_sem.acquire();
                break;
            } catch (InterruptedException e) {}
    }
    static int UploadTest( StreamSvccli oProxy, long hStream )
    {
        if( oProxy == null || hStream == RC.INVALID_HANDLE )
            return -RC.EINVAL;
         JRetVal jret = null;
         for( int i = 0; i < 15; i++)
         {
             int blockSize = ( 1 << i ) * 1024;
             jret = oProxy.UploadFile(
                     i, "block", hStream, 0, blockSize );
             if( jret.ERROR() )
                 break;
             else if( jret.isPending()) {
                 // just for testing purpose. you can
                 // customize to issue the next request
                 // immediately without waiting for
                 // completion
                 WaitForSem(oProxy);
                 if (RC.ERROR(oProxy.getError())) {
                     jret.setError(oProxy.getError());
                     break;
                 }
             }
             byte[] buf = new byte[ blockSize];
             int ret = oProxy.writeStream(hStream, buf);
             if( RC.ERROR(ret )) {
                 jret.setError(ret);
                 rpcbase.JavaOutputMsg(
                         String.format("upload %d bytes failed(%d)", blockSize, ret ));
                 break;
             }
             rpcbase.JavaOutputMsg(
                     String.format("uploaded %d bytes", blockSize));
             jret = oProxy.readStream(hStream);
             if( jret.ERROR())
                 break;
             byte[] oResp = (byte[])jret.getAt(0);
             rpcbase.JavaOutputMsg(
                     String.format("peer received message %s", oResp.toString() ));
         }
         if( jret != null )
             return jret.getError();
         return RC.ERROR_FAIL;
    }

    static int DownloadTest( StreamSvccli oProxy, long hStream )
    {
        JRetVal jret = null;
        int ret = 0;

        for( int i = 0; i < 15; i++ )
        {
            long blockSize = ( 1 << i ) * 1024;
            jret = oProxy.DownloadFile(
                    i, "block", hStream, 0, blockSize );
            if(jret.ERROR())
                break;
            if(jret.isPending()) {
                WaitForSem(oProxy);
                if (RC.ERROR(oProxy.getError())) {
                    jret.setError(oProxy.getError());
                    break;
                }
            }
            rpcbase.JavaOutputMsg(String.format("downloading %d bytes", blockSize));
            while( blockSize > 0 )
            {
                jret = oProxy.readStream(hStream);
                if( jret.ERROR())
                    break;
                byte[] buf = (byte[]) jret.getAt(0);
                blockSize -= buf.length;
            }
            if( jret.ERROR())
                break;
            byte[]buf = "ROK".getBytes(StandardCharsets.UTF_8);
            ret = oProxy.writeStream(hStream,buf);
            if( RC.ERROR(ret))
            {
                rpcbase.JavaOutputMsg("send ACK failed");
                jret.setError(ret);
                break;
            }
            rpcbase.JavaOutputMsg(
                    String.format("download test %d complete", i));
        }

        if( jret != null )
            return jret.getError();
        return RC.ERROR_FAIL;
    }

    static int StreamTests( StreamSvccli oProxy )
    {
        int ret = 0;
        JRetVal jret = null;
        long hStream = RC.INVALID_HANDLE;
        do{
            jret = oProxy.startStream( null );
            if( jret.ERROR()) {
                rpcbase.JavaOutputMsg("StartStream failed " + jret.getError() );
                break;
            }
            hStream = jret.getAtLong(0);
            rpcbase.JavaOutputMsg("Start stream test...");
            jret = oProxy.readStream(hStream);
            if( jret.ERROR()) {
                rpcbase.JavaOutputMsg(
                        String.format("Channel READ ACK failed %d", jret.getError()));
                break;
            }
            String strMsg = jret.getAt(0).toString();
            rpcbase.JavaOutputMsg(String.format("peer Channel %s",strMsg));

            jret = oProxy.SpecifyChannel(hStream);
            if( jret.ERROR())
            {
                rpcbase.JavaOutputMsg(
                        String.format("SpecifyChannel failed %d", jret.getError()));
                break;
            }
            rpcbase.JavaOutputMsg("SpecifyChannel succeeded");
            ret = oProxy.writeStream(
                    hStream, "hello, SpecifyChannel".getBytes(StandardCharsets.UTF_8));
            if( RC.ERROR(ret))
            {
                jret.setError(ret);
                rpcbase.JavaOutputMsg(
                        String.format("writeStream failed %d", jret.getError()));
                break;
            }
            jret = oProxy.readStream(hStream);
            if( jret.ERROR())
            {
                rpcbase.JavaOutputMsg(
                        String.format("readStream failed %d", jret.getError()));
                break;
            }
            strMsg = jret.getAt(0).toString();
            rpcbase.JavaOutputMsg(String.format("%s",strMsg));

            ret = UploadTest(oProxy, hStream);
            if( RC.ERROR(ret))
                break;
            ret = DownloadTest(oProxy, hStream);
        }while( false );

        if( hStream != RC.INVALID_HANDLE)
            oProxy.closeStream(hStream);

        return ret;
    }

    public static void main( String[] args )
    {
        m_oCtx = JavaRpcContext.createProxy(); 
        if( m_oCtx == null )
            System.exit( RC.EFAULT );

        int ret = 0;
        JRetVal jret = null;
        SimpFileSvccli oSvcCli = null;
        StreamSvccli oStmCli = null;
	    do{
            String strDescPath =
                getDescPath( "fulltestdesc.json" );
            if( strDescPath.isEmpty() )
                System.exit( RC.ENOENT );
            // create the service object
            oSvcCli = new SimpFileSvccli(
                m_oCtx.getIoMgr(),
                strDescPath,
                "SimpFileSvc" );

            // check if there are errors
            if( RC.ERROR( oSvcCli.getError() ) )
                System.exit( -oSvcCli.getError() );

            // create the service object
            oStmCli = new StreamSvccli(
                    m_oCtx.getIoMgr(),
                    strDescPath,
                    "StreamSvc");

            // check if there are errors
            if (RC.ERROR(oSvcCli.getError()))
                return;

            // start the proxy
            ret = oSvcCli.start();
            if (RC.ERROR(ret))
                break;

            ret = oStmCli.start();
            if (RC.ERROR(ret))
                break;

            // test remote server is not online
            while (oSvcCli.getState() == RC.stateRecovery ||
                oStmCli.getState() == RC.stateRecovery )
                try {
                    TimeUnit.SECONDS.sleep(1);
                } catch (InterruptedException e) {
                }

            while (oSvcCli.getState() != RC.stateConnected ||
                oStmCli.getState() != RC.stateConnected ) {
                ret = RC.ERROR_STATE;
                break;
            }

            jret = oSvcCli.Ping();
            if( jret.ERROR())
            {
                ret = jret.getError();
                break;
            }

            ret = SyncTests( oSvcCli );
            if( RC.ERROR( ret ))
                break;
            try{
                TimeUnit.SECONDS.sleep(1);
            }catch( InterruptedException e){}
            ret = AsyncTests( oSvcCli );
            if( RC.ERROR(ret))
                break;
            ret = StreamTests( oStmCli );

         }while( false );

        if( RC.ERROR( ret ))
            rpcbase.JavaOutputMsg(
                    String.format( "Error running tests (%d)", ret));
        return;
    }
}
