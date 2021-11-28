package org.rpcf.tests.asynctst;

import java.util.concurrent.TimeUnit;
import org.rpcf.rpcbase.JRetVal;
import org.rpcf.rpcbase.JavaRpcContext;
import org.rpcf.rpcbase.RC;
import org.rpcf.rpcbase.rpcbase;

public class maincli {
    public static JavaRpcContext m_oCtx;

    public maincli() {
    }

    public static void main(String[] args) {
        m_oCtx = JavaRpcContext.createProxy();
        if (m_oCtx == null)
            return;
        int ret = 0;
        do{
            AsyncTestcli oSvcCli = new AsyncTestcli(
                m_oCtx.getIoMgr(), "./asynctstdesc.json", "AsyncTest");

            if (RC.ERROR(oSvcCli.getError()))
                break;

            ret = oSvcCli.start();
            if (RC.ERROR(ret))
                break;

            while(oSvcCli.getState() == RC.stateRecovery) {
                try {
                    TimeUnit.SECONDS.sleep(1L);
                } catch (InterruptedException var5) {
                }
            }

            if (oSvcCli.getState() != RC.stateConnected)
            {
                ret = RC.ERROR_STATE;
                break;
            }

            JRetVal jret = oSvcCli.LongWaitNoParam(null);
            if (jret.ERROR()) 
            {
                ret = jret.getError();
                break;
            }

            while(true)
            {
                try {
                    if (jret.isPending()) {
                        oSvcCli.m_sem.acquire();
                        break;
                    }
                } catch (InterruptedException var6) {
                }
            }

            rpcbase.JavaOutputMsg("LongWaitNoParam completed");
            jret = oSvcCli.LongWait("hello, LongWait");
            if (jret.ERROR()) 
            {
                ret = jret.getError();
                break;
            }
            
            String strResp = (String)jret.getAt(0);
            rpcbase.JavaOutputMsg(
                String.format("LongWait completed with resp %s", strResp));
            if (jret.ERROR()) 
            {
                ret = jret.getError();
                break;
            }

            jret = oSvcCli.LongWait2("hello, LongWait2");
            if (jret.ERROR()) 
            {
                ret = jret.getError();
                break;
            }
            strResp = (String)jret.getAt(0);
            rpcbase.JavaOutputMsg(
                String.format("LongWait2 completed with resp %s", strResp));

            oSvcCli.stop();
            m_oCtx.stop();
        }while( false );
    }
}
