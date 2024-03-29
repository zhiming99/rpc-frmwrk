// GENERATED BY RIDLC. MAKE SURE TO BACKUP BEFORE RE-COMPILING.
package org.rpcf.fulltest;
import org.rpcf.rpcbase.*;
import java.util.Map;
import java.util.HashMap;
import java.lang.String;
import java.nio.ByteBuffer;

import static org.rpcf.rpcbase.rpcbase.JavaOutputMsg;

public class SimpFileSvcsvr extends SimpFileSvcsvrbase
{
    public SimpFileSvcsvr( ObjPtr pIoMgr,
        String strDesc, String strSvrObj )
    { super( pIoMgr, strDesc, strSvrObj ); }
    
    // ITinyInterface::Ping sync-handler
    public int Ping(
        JavaReqContext oReqCtx )
    {
        oReqCtx.setResponse( RC.STATUS_SUCCESS );
        return RC.STATUS_SUCCESS;
    }
    
    // ITinyInterface::KAReq sync-handler no-reply
    public int KAReq(
        JavaReqContext oReqCtx,
        long qwTaskId )
    {
        // Synchronous handler with no-reply
        // return value and response is ignored.
        return RC.STATUS_SUCCESS;
    }
    
    // ITinyInterface::KAReq2 async-handler no-reply
    public int KAReq2(
        JavaReqContext oReqCtx,
        long qwTaskId )
    {
        // Asynchronouse handler with no-reply. 
        // Make sure to call
        // oReqCtx.onServiceComplete in the future
        // if returning RC.STATUS_PENDING.
        return RC.STATUS_SUCCESS;
    }
    public void onKAReq2Canceled(
        JavaReqContext oReqCtx, int iRet,
        long qwTaskId )
    {
        // ITinyInterface::KAReq2 is canceled.
        // Optionally make change here to do the cleanup
        // here if the request was timed-out or canceled
        return;
    }
    
    // IEchoThings::Echo sync-handler
    public int Echo(
        JavaReqContext oReqCtx,
        String strText )
    {
        // Synchronous handler. Make sure to call
        JavaOutputMsg(String.format("strText is %s", strText));
        oReqCtx.setResponse( RC.STATUS_SUCCESS, strText );
        return RC.STATUS_SUCCESS;
    }
    
    // IEchoThings::EchoUnknown sync-handler
    public int EchoUnknown(
        JavaReqContext oReqCtx,
        byte[] pBuf )
    {
        // Synchronous handler. Make sure to call
        oReqCtx.setResponse( RC.STATUS_SUCCESS, pBuf );
        return RC.STATUS_SUCCESS;
    }
    
    // IEchoThings::EchoCfg sync-handler
    public int EchoCfg(
        JavaReqContext oReqCtx,
        ObjPtr pObj )
    {
        // Synchronous handler. Make sure to call
        oReqCtx.setResponse( RC.STATUS_SUCCESS, pObj );
        return RC.STATUS_SUCCESS;
    }
    
    // IEchoThings::EchoMany sync-handler
    public int EchoMany(
        JavaReqContext oReqCtx,
        int i1,
        short i2,
        long i3,
        float i4,
        double i5,
        String szText )
    {
        oReqCtx.setResponse( RC.STATUS_SUCCESS,
                i1, i2, i3, (float)(i4 + .1), i5 +.2, szText );
        return RC.STATUS_SUCCESS;
    }
    public void onEchoManyCanceled(
        JavaReqContext oReqCtx, int iRet,
        int i1,
        short i2,
        long i3,
        float i4,
        double i5,
        String szText )
    {
        // IEchoThings::EchoMany is canceled.
        // Optionally make change here to do the cleanup
        // work if the request was timed-out or canceled
        return;
    }
    
    // IEchoThings::EchoStruct 
    public int EchoStruct(
        JavaReqContext oReqCtx,
        FILE_INFO fi )
    {
        // Asynchronouse handler. Make sure to call
        oReqCtx.setResponse( RC.STATUS_SUCCESS, fi );
        return RC.STATUS_SUCCESS;
    }
    public void onEchoStructCanceled(
        JavaReqContext oReqCtx, int iRet,
        FILE_INFO fi )
    {
        // IEchoThings::EchoStruct is canceled.
        // Optionally make change here to do the cleanup
        // work if the request was timed-out or canceled
        return;
    }
    
    // IEchoThings::EchoTypedef async-handler
    public int EchoTypedef(
        JavaReqContext oReqCtx,
        OBJPTR[] stm )
    {
        // Asynchronouse handler. Make sure to call
        // oReqCtx.SetResponse with response
        // parameters if return immediately or call
        // oReqCtx.onServiceComplete in the future
        // if returning RC.STATUS_PENDING.
        return RC.ERROR_NOT_IMPL;
    }
    public void onEchoTypedefCanceled(
        JavaReqContext oReqCtx, int iRet,
        OBJPTR[] stm )
    {
        // IEchoThings::EchoTypedef is canceled.
        // Optionally make change here to do the cleanup
        // work if the request was timed-out or canceled
        return;
    }
    
    // IEchoThings::EchoHandle async-handler
    public int EchoHandle(
        JavaReqContext oReqCtx,
        STMHANDLE stm )
    {
        // Asynchronouse handler. Make sure to call
        oReqCtx.setResponse( RC.STATUS_SUCCESS, stm );
        return RC.STATUS_SUCCESS;
    }
    public void onEchoHandleCanceled(
        JavaReqContext oReqCtx, int iRet,
        STMHANDLE stm )
    {
        // IEchoThings::EchoHandle is canceled.
        // Optionally make change here to do the cleanup
        // work if the request was timed-out or canceled
        return;
    }
}
