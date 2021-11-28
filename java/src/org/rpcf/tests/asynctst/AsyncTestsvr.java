package org.rpcf.tests.asynctst;

import java.util.concurrent.TimeUnit;
import org.rpcf.rpcbase.JRetVal;
import org.rpcf.rpcbase.JavaReqContext;
import org.rpcf.rpcbase.ObjPtr;
import org.rpcf.rpcbase.RC;
import org.rpcf.rpcbase.rpcbase;
import org.rpcf.rpcbase.JavaRpcServiceS.IDeferredCall;
import org.rpcf.rpcbase.JavaRpcServiceS.IUserTimerCb;

public class AsyncTestsvr extends AsyncTestsvrbase {
    public AsyncTestsvr(ObjPtr pIoMgr, String strDesc, String strSvrObj) {
        super(pIoMgr, strDesc, strSvrObj);
    }

    // asynchronous handler
    public int LongWait(JavaReqContext oReqCtx, final String i0) {
        // schedule a timer to complete this call
        IUserTimerCb oTimerCb = new IUserTimerCb() {
            public void onTimer(Object octx, int iRet) {
                JavaReqContext oReqCtx = (JavaReqContext)octx;
                Object oData = oReqCtx.getUserData();
                if (oData != null) {
                    ObjPtr var5 = (ObjPtr)oData;
                }

                rpcbase.JavaOutputMsg("LongWait complete with \"" + i0 + "\"");
                oReqCtx.onServiceComplete(0, new Object[]{i0});
            }
        };
        int ret = 0;
        JRetVal jret = this.addTimer(3, oTimerCb, oReqCtx);
        if (jret.SUCCEEDED()) {
            oReqCtx.setUserData(jret.getAt(0));
            ret = RC.STATUS_PENDING;
        } else {
            ret = jret.getError();
            oReqCtx.setResponse(ret, new Object[0]);
        }

        return ret;
    }

    public void onLongWaitCanceled(JavaReqContext oReqCtx, int iRet, String i0) {
        Object oData = oReqCtx.getUserData();
        if (oData != null) {
            ObjPtr oTimerObj = (ObjPtr)oData;
            this.disableTimer(oTimerObj);
        }
    }

    public int LongWaitNoParam(JavaReqContext oReqCtx) {
        try {
            TimeUnit.SECONDS.sleep(2L);
        } catch (InterruptedException var3) {
        }

        oReqCtx.setResponse(RC.STATUS_SUCCESS, new Object[0]);
        return RC.STATUS_SUCCESS;
    }

    // asynchronous handler
    public int LongWait2(final JavaReqContext oReqCtx, String i1) {
        // schedule a deferred call to complete this request
        IDeferredCall oDeferCall = new IDeferredCall() {
            public int getArgCount() {
                return 1;
            }

            public Class<?>[] getArgTypes() {
                return new Class[]{String.class};
            }

            public void call(Object[] oParams) {
                String strResp = (String)oParams[0];
                rpcbase.JavaOutputMsg("completing LongWait2 with \"" + strResp + "\"");
                oReqCtx.onServiceComplete(RC.STATUS_SUCCESS, new Object[]{strResp});
            }
        };
        int ret = this.deferCall(oDeferCall, new Object[]{i1});
        if (RC.ERROR(ret)) {
            oReqCtx.setResponse(ret, new Object[0]);
        } else {
            ret = RC.STATUS_PENDING;
        }

        return ret;
    }

    public void onLongWait2Canceled(JavaReqContext oReqCtx, int iRet, String i1) {
    }
}

