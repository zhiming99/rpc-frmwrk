package org.rpcf.tests.asynctst;

import org.rpcf.rpcbase.JRetVal;
import org.rpcf.rpcbase.JavaRpcContext;
import org.rpcf.rpcbase.RC;
import org.rpcf.rpcbase.rpcbase;

import java.util.concurrent.TimeUnit;

public class maincli {
    public static JavaRpcContext m_oCtx;

    public static String getDescPath( String strName )
    {
        String strDescPath =
            maincli.class.getProtectionDomain().getCodeSource().getLocation().getPath();
        String strDescPath2 = strDescPath + "/org/rpcf/tests/asynctst/" + strName;
        java.io.File oFile = new java.io.File( strDescPath2 );
        if( oFile.isFile() )
            return strDescPath2;
        strDescPath += "/" + strName;
        oFile = new java.io.File( strDescPath );
        if( oFile.isFile() )
            return strDescPath;
        return "";
    }

    public static void main(String[] args) {
        m_oCtx = JavaRpcContext.createProxy();
        if (m_oCtx == null)
            System.exit( RC.EFAULT );;
        int ret = 0;
        AsyncTestcli oSvcCli = null;
        do{
            String strDescPath =
                    getDescPath( "asynctstdesc.json" );
            if( strDescPath.isEmpty() )
                System.exit( RC.ENOENT );

            oSvcCli = new AsyncTestcli(
                m_oCtx.getIoMgr(),
                    strDescPath,
                    "AsyncTest");

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

        }while( false );
        if(oSvcCli != null)
            oSvcCli.stop();
        m_oCtx.stop();
    }
}
