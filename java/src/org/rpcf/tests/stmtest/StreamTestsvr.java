// GENERATED BY RIDLC. MAKE SURE TO BACKUP BEFORE RE-COMPILING.
package org.rpcf.tests.stmtest;
import org.rpcf.rpcbase.*;

import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;
import java.util.Map;
import java.util.HashMap;
import java.lang.String;
import java.nio.ByteBuffer;

public class StreamTestsvr extends StreamTestsvrbase {
    public StreamTestsvr(ObjPtr pIoMgr,
                         String strDesc, String strSvrObj) {
        super(pIoMgr, strDesc, strSvrObj);
    }

    // IStreamTest::Echo sync-handler
    public int Echo(
            JavaReqContext oReqCtx,
            String i0) {
        // Synchronous handler. Make sure to call
        // oReqCtx.setResponse before return
        oReqCtx.setResponse(RC.STATUS_SUCCESS, i0);
        return RC.STATUS_SUCCESS;
    }

    // asynchronous readStream callback,
    // this method can run on multiple threads
    // at the same time. so be careful about the
    // concurrent issues.
    public void onReadStreamComplete(
            int iRet, long hChannel, byte[] buf)
    {
        if(RC.ERROR(iRet))
        {
            rpcbase.JavaOutputMsg(
                    "readStreamAsync failed with errror " + iRet);
            return;
        }
        rpcbase.JavaOutputMsg(
                "proxy says: " + new String(buf, StandardCharsets.UTF_8));
        writeAndReceive(hChannel);
        return;
    }

    public void onWriteStreamComplete(
            int iRet, long hChannel, byte[] buf) {
        if(RC.ERROR(iRet))
        {
            rpcbase.JavaOutputMsg(
                    "writeStreamAsync failed with errror " + iRet);
            return;
        }
        JRetVal jret = getChanCtx(hChannel);
        if(jret.SUCCEEDED())
        {
            TransferContext o = (TransferContext) jret.getAt(0);
            o.incCounter();
        }
        readAndReply(hChannel);
        return;
    }

    int readAndReply(long hChannel)
    {
        int ret = 0;
        JRetVal jret = getChanCtx(hChannel);
        if(jret.ERROR())
        {
            ret = jret.getError();
            return ret;
        }
        TransferContext o = (TransferContext)jret.getAt(0);
        do {
            // read anything from the channel
            jret = readStreamAsync(hChannel, 0);
            if(jret.ERROR()) {
                ret = jret.getError();
                break;
            }
            else if(jret.isPending())
            {
                ret = 0;
                break;
            }
            // something already arrives
            byte[] byResp = (byte[]) jret.getAt(0);
            String strResp =
                    new String(byResp, StandardCharsets.UTF_8);
            rpcbase.JavaOutputMsg("proxy says: " + strResp);

            int i = o.getCounter();
            String strMsg = String.format("This is message %d", i );
            ret = writeStreamAsync(
                    hChannel,strMsg.getBytes(StandardCharsets.UTF_8));
            if( RC.ERROR(ret) )
                break;
            if(RC.isPending(ret))
            {
                ret = 0;
                break;
            }
            o.incCounter();

        }while(true);
        if(RC.ERROR(ret))
        {
            rpcbase.JavaOutputMsg(
                    "readAndReply failed with error " + ret);
            o.setError(ret);
        }
        return ret;
    }

    int writeAndReceive( long hChannel )
    {
        int ret = 0;
        JRetVal jret = getChanCtx(hChannel);
        if (jret.ERROR()) {
            ret = jret.getError();
            return ret;
        }
        TransferContext o = (TransferContext) jret.getAt(0);

        do {
            int idx = o.getCounter();
            String strMsg = String.format("This is message %d", idx);
            ret = writeStreamAsync(
                    hChannel, strMsg.getBytes(StandardCharsets.UTF_8));
            if (RC.ERROR(ret))
                break;
            if (RC.isPending(ret)) {
                ret = 0;
                break;
            }
            o.incCounter();
            jret = readStreamAsync(hChannel, 0);
            if (jret.ERROR()) {
                ret = jret.getError();
                break;
            }
            if (jret.isPending()){
                ret = 0;
                break;
            }

            byte[] byResp = (byte[]) jret.getAt(0);
            String strResp =
                    new String(byResp, StandardCharsets.UTF_8);
            rpcbase.JavaOutputMsg("proxy says: " + strResp);

        }while( true );
        if(RC.ERROR(ret))
        {
            o.setError(ret);
            rpcbase.JavaOutputMsg(
                    "writeAndReceive failed with error " + ret);
        }
        return ret;
    }
    public int onStmReady( long hChannel )
    {
        String greeting = "Hello, Proxy";
        writeStreamNoWait(hChannel,
                greeting.getBytes(StandardCharsets.UTF_8));
        TransferContext tc = new TransferContext();
        setChanCtx(hChannel,tc);
        return readAndReply(hChannel);
    }
    public int onStmClosing( long hChannel )
    {
        int ret = 0;
        JRetVal jret = getChanCtx(hChannel);
        if (jret.ERROR()) {
            return 0;
        }
        TransferContext o = (TransferContext) jret.getAt(0);
        rpcbase.JavaOutputMsg(String.format(
                "Stream %d is closing with status code %d", hChannel,o.getError()));
        return 0;
    }
}