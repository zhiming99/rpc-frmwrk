// GENERATED BY RIDLC. MAKE SURE TO BACKUP BEFORE RE-COMPILING.
package org.rpcf.fulltest;
import org.rpcf.rpcbase.*;

import java.nio.charset.StandardCharsets;
import java.util.Map;
import java.util.HashMap;
import java.lang.String;
import java.nio.ByteBuffer;

public class StreamSvcsvr extends StreamSvcsvrbase
{
    public StreamSvcsvr( ObjPtr pIoMgr,
        String strDesc, String strSvrObj )
    { super( pIoMgr, strDesc, strSvrObj ); }

    FILE_INFO BuildFF( String fileName )
    {
        FILE_INFO fi = new FILE_INFO();
        // the content does not make sense
        fi.szFileName = fileName;
        fi.fileSize = 123;
        fi.bRead = true;
        fi.fileHeader = (fileName + "Header").getBytes(StandardCharsets.UTF_8);
        String[] listStrs = new String[]{ "alem00", "alem01"};
        String[] listStrs1 = new String[]{ "alem10", "alem11"};
        fi.vecLines = new String[][]{listStrs, listStrs1};

        byte[] val = "bibi".getBytes(StandardCharsets.UTF_8);
        fi.vecBlocks = new HashMap<>();
        fi.vecBlocks.put( 0, val );
        val = "nono".getBytes(StandardCharsets.UTF_8);
        fi.vecBlocks.put( 1, val );
        return fi;
    }

    public int m_curTest = 1;
    public int m_numTest = 0;
    public long m_curSize = 0;
    public long m_totalSize = 0;
    public long m_hChannel = 0;

    // IFileTransfer::UploadFile async-handler
    public int UploadFile(
        JavaReqContext oReqCtx,
        String szFilePath,
        long hChannel,
        long offset,
        long size )
    {
        m_curTest = 1;
        m_curSize = size;
        m_numTest++;
        m_totalSize = size;
        oReqCtx.setResponse( RC.STATUS_SUCCESS );
        return RC.STATUS_SUCCESS;
    }
    public void onUploadFileCanceled(
        JavaReqContext oReqCtx, int iRet,
        String szFilePath,
        long hChannel,
        long offset,
        long size )
    {
        m_curTest = 0;
        m_curSize = 0;
        m_totalSize = 0;
        return;
    }
    
    // IFileTransfer::GetFileInfo sync-handler
    public int GetFileInfo(
        JavaReqContext oReqCtx,
        String szFileName,
        boolean bRead )
    {
        // Asynchronouse handler. Make sure to call
        // oReqCtx.SetResponse with response
        // parameters if return immediately or call
        // oReqCtx.onServiceComplete in the future
        // if returning RC.STATUS_PENDING.
        return RC.ERROR_NOT_IMPL;
    }
    public void onGetFileInfoCanceled(
        JavaReqContext oReqCtx, int iRet,
        String szFileName,
        boolean bRead )
    {
        // IFileTransfer::GetFileInfo is canceled.
        // Optionally make change here to do the cleanup
        // work if the request was timed-out or canceled
        return;
    }
    
    // IFileTransfer::DownloadFile async-handler
    public int DownloadFile(
        JavaReqContext oReqCtx,
        String szFileName,
        long hChannel,
        long offset,
        long size )
    {
        // Asynchronouse handler. Make sure to call
        m_curTest = 2;
        m_curSize = size;
        m_numTest++;
        m_totalSize = size;
        oReqCtx.setResponse( RC.STATUS_SUCCESS );
        return RC.STATUS_SUCCESS;
    }
    public void onDownloadFileCanceled(
        JavaReqContext oReqCtx, int iRet,
        String szFileName,
        long hChannel,
        long offset,
        long size )
    {
        m_curTest = 0;
        m_curSize = 0;
        m_totalSize = 0;
        return;
    }
    
    // IChat::SpecifyChannel sync-handler
    public int SpecifyChannel(
        JavaReqContext oReqCtx,
        long hChannel )
    {
        // Synchronous handler. Make sure to call
        int ret = RC.STATUS_SUCCESS;
        if( hChannel == 0 )
        {
            ret = -RC.EINVAL;
            rpcbase.JavaOutputMsg( "hChannel is zero" );
            return ret;
        }
        m_hChannel = hChannel;
        m_curTest = 0;
        Map< String, FILE_INFO > fis = new HashMap<>();
        fis.put( "hello", BuildFF("SpecifyChannel "));
        oReqCtx.setResponse( ret, fis );
        return ret;
    }


    // override the default version of acceptNewStream
    public int acceptNewStream( ObjPtr pCtx, CfgPtr pDataDesc )
    {
        rpcbase.JavaOutputMsg("acceptNewStream " + pDataDesc.toString() );
        return RC.STATUS_SUCCESS;
    }

    public int onStmReady( long hChannel )
    {
        rpcbase.JavaOutputMsg( "onStmReady " + Long.toString( hChannel) );
        // send a notification to notify the server side is ready
        writeStream( hChannel, "RDY".getBytes(StandardCharsets.UTF_8));
        return 0;
    }
    public int onStmClosing( long hChannel )
    {
        if( m_hChannel == hChannel ) {
            rpcbase.JavaOutputMsg("onStmClosing " +
                    Long.toString(hChannel));
            m_hChannel = 0;
        }
        else
        {
            rpcbase.JavaOutputMsg( "stream closing, but not me " +
                    Long.toString( m_hChannel ) +
                    " " +
                    Long.toString( hChannel ));
        }
        return 0;
    }
}
