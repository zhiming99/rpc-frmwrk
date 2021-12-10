package org.rpcf.tests.sftest;

import org.rpcf.rpcbase.IRpcService;
import org.rpcf.rpcbase.JRetVal;
import org.rpcf.rpcbase.RC;
import org.rpcf.rpcbase.rpcbase;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;

public class TransferContext {
    HashMap<Long,TransFileContext> m_mapChanToCtx = new HashMap<>();
    IRpcService m_oInst;

    TransferContext(IRpcService oInst)
    {m_oInst = oInst;}

    synchronized int addContext(long hChannel, TransFileContext o)
    {
        if(hChannel == RC.INVALID_HANDLE)
            return -RC.EINVAL;
        TransFileContext old =
            m_mapChanToCtx.get(hChannel);
        if(old != null && !old.isDone())
            return -RC.EEXIST;
        m_mapChanToCtx.put(hChannel,o);
        return RC.STATUS_SUCCESS;
    }

    synchronized void removeContext(long hChannel)
    { m_mapChanToCtx.remove(hChannel); }

    synchronized void onTransferDone(long hChannel, int iRet)
    {
        TransFileContext o = m_mapChanToCtx.get(hChannel);
        if(o == null)
            return;
        o.cleanUp();
        removeContext(hChannel);
    }

    synchronized int readFileAndSend(long hChannel)
    {
        int ret = 0;
        TransFileContext o = m_mapChanToCtx.get(hChannel);
        if(o == null)
            return -RC.ENOENT;
        int iSizeLimit = 2 * 1024 * 1024;
        try {
            while (true) {
                int iSize = o.m_iBytesLeft;
                if (iSize > iSizeLimit)
                    iSize = iSizeLimit;
                else if (iSize == 0) {
                    onTransferDone(hChannel, 0 );
                    break;
                }
                ByteBuffer bbuf = ByteBuffer.allocate(iSize);
                o.m_oChannel.read(bbuf);
                bbuf.flip();
                byte[] bufToSend = bbuf.array();
                ret = m_oInst.writeStreamAsync(hChannel, bufToSend);
                if(RC.isPending(ret)) {
                    ret = RC.STATUS_PENDING;
                    break;
                }
                if(RC.ERROR(ret))
                {
                    onTransferDone(hChannel,ret);
                    break;
                }
                o.m_iBytesLeft -= bufToSend.length;
                o.m_iBytesSent += bufToSend.length;

            }
        }catch( IOException e){
            ret = -RC.EIO;
            onTransferDone(hChannel,ret);
        }
        return ret;
    }

    int sendToken(long hChannel, byte[]buf )
    {
        return m_oInst.writeStreamNoWait(hChannel, buf);
    }
    synchronized int writeFileAndRecv(long hChannel, byte[] buf)
    {
        int ret = 0;
        if(hChannel == -RC.INVALID_HANDLE)
            return -RC.EINVAL;
        int iSizeLimit = 2 * 1024 * 1024;
        TransFileContext o = m_mapChanToCtx.get(hChannel);
        if(o == null)
            return -RC.ENOENT;

        try {
            while (true) {
                if(o.m_iBytesLeft < buf.length) {
                    ret = -RC.ERANGE;
                    break;
                }
                if(buf.length > 0) {
                    o.m_oFile.write(buf);
                    o.m_iBytesLeft -= buf.length;
                    o.m_iBytesSent += buf.length;

                    String strMsg = String.format(
                            "Received %d, To Receive %d",
                            o.m_iBytesSent, o.m_iBytesLeft);
                    rpcbase.JavaOutputMsg(strMsg);
                }
                int iSizeRecv = o.m_iBytesLeft;
                if(iSizeRecv > iSizeLimit)
                    iSizeRecv = iSizeLimit;
                else if(iSizeRecv == 0 ) {
                    onTransferDone(hChannel, 0 );
                    if(m_oInst.isServer())
                    {
                        IRpcService.IDeferredCall odc = new IRpcService.IDeferredCall() {
                            @Override
                            public int getArgCount() {
                                return 2;
                            }

                            @Override
                            public Class<?>[] getArgTypes() {
                                return new Class[]{
                                    long.class,
                                    byte[].class
                                };
                            }

                            @Override
                            public void call(Object[] oParams) {
                                long hChannel = (Long)oParams[0];
                                byte[] buf = (byte[]) oParams[1];
                                sendToken(hChannel, buf);
                            }
                        };
                        ret = m_oInst.deferCall(
                                odc, new Object[]{hChannel,
                                        "over".getBytes(StandardCharsets.UTF_8)});
                    }
                    break;
                }
                JRetVal jret = m_oInst.readStreamAsync(
                        hChannel, iSizeRecv );
                if(jret.ERROR()) {
                    ret = jret.getError();
                    break;
                }
                if(jret.isPending()) {
                    ret = RC.STATUS_PENDING;
                    break;
                }
                buf = (byte[]) jret.getAt(0);
            }
        }catch( IOException e){
            ret = -RC.EIO;
        }
        if(RC.ERROR(ret))
        {
            onTransferDone(hChannel,ret);
        }
        return ret;
    }

    synchronized public void onWriteStreamComplete(
            long hChannel, byte[] buf)
    {
        if(hChannel == RC.INVALID_HANDLE || buf == null)
            return;
        TransFileContext o = m_mapChanToCtx.get(hChannel);
        if(o == null)
            return;
        o.m_iBytesLeft -= buf.length;
        o.m_iBytesSent += buf.length;
        String strMsg = String.format(
                "Sent %d, To send %d",
                o.m_iBytesSent, o.m_iBytesLeft);
        rpcbase.JavaOutputMsg(strMsg);
        readFileAndSend(hChannel);
    }

    synchronized public void onReadStreamComplete(
            long hChannel, byte[]buf ) {
        if (hChannel == RC.INVALID_HANDLE || buf == null)
            return;
        TransFileContext o = m_mapChanToCtx.get(hChannel);
        if(o == null)
            return;
        writeFileAndRecv(hChannel,buf);
    }

    synchronized void setError(long hChannel, int iError)
    {
        if(hChannel == RC.INVALID_HANDLE)
            return;
        TransFileContext o = m_mapChanToCtx.get(hChannel);
        if(o == null)
            return;
        o.setError(iError);
    }

    synchronized int getError(long hChannel)
    {
        if(hChannel == RC.INVALID_HANDLE)
            return -RC.EINVAL;
        TransFileContext o = m_mapChanToCtx.get(hChannel);
        if(o == null)
            return -RC.EFAULT;
        return o.getError();
    }
}
