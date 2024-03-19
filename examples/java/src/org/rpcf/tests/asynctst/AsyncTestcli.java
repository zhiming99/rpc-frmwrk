// GENERATED BY RIDLC, MAKE SURE TO BACKUP BEFORE RUNNING RIDLC AGAIN
// Copyright (C) 2024  zhiming <woodhead99@gmail.com>
// This program can be distributed under the terms of the GNU GPLv3.
// ridlc -jO . -Porg.rpcf.tests ../../asynctst.ridl 
package org.rpcf.tests.asynctst;
import org.rpcf.rpcbase.*;
import java.util.Map;
import java.util.HashMap;
import java.lang.String;
import java.nio.ByteBuffer;
import java.util.concurrent.Semaphore;

public class AsyncTestcli extends AsyncTestclibase
{
    public Semaphore m_sem = new Semaphore(0);
    public AsyncTestcli( ObjPtr pIoMgr,
        String strDesc, String strSvrObj )
    { super( pIoMgr, strDesc, strSvrObj ); }
    
    // IAsyncTest::LongWait async callback
    public void onLongWaitComplete(
        Object oContext, int iRet,
        String i0r  )
    {
        JRetVal jret = (JRetVal)oContext;
        jret.setError(iRet);
        if (RC.SUCCEEDED(iRet)) {
            rpcbase.JavaOutputMsg("LongWait complete with response: " + i0r );
        } else {
            rpcbase.JavaOutputMsg("LongWaitNoParam failed with error " + iRet);
        }
        this.m_sem.release();
        return;
    }

    public void onLongWaitNoParamComplete(Object oContext, int iRet) {
        JRetVal jret = (JRetVal)oContext;
        jret.setError(iRet);
        if (RC.SUCCEEDED(iRet)) {
            rpcbase.JavaOutputMsg("LongWaitNoParam complete successfully");
        } else {
            rpcbase.JavaOutputMsg("LongWaitNoParam failed with error " + iRet);
        }
        this.m_sem.release();
    }
}
