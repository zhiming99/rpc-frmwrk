package org.rpcf.tests.iftest;

import org.rpcf.rpcbase.JavaReqContext;
import org.rpcf.rpcbase.ObjPtr;
import org.rpcf.rpcbase.RC;

public class IfTestsvr extends IfTestsvrbase {
    public IfTestsvr(ObjPtr pIoMgr, String strDesc, String strSvrObj)
    {
        super(pIoMgr, strDesc, strSvrObj);
    }

    public int Echo(JavaReqContext oReqCtx, GlobalFeatureList i0)
    {
        oReqCtx.setResponse(
            RC.STATUS_SUCCESS, new Object[]{i0});
        return RC.STATUS_SUCCESS;
    }
}
