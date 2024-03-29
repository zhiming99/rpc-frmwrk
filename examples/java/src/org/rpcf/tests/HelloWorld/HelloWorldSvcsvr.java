// GENERATED BY RIDLC. MAKE SURE TO BACKUP BEFORE RE-COMPILING.
package org.rpcf.tests.HelloWorld;

import org.rpcf.rpcbase.JavaReqContext;
import org.rpcf.rpcbase.ObjPtr;
import org.rpcf.rpcbase.RC;

public class HelloWorldSvcsvr extends HelloWorldSvcsvrbase
{
    public HelloWorldSvcsvr( ObjPtr pIoMgr,
        String strDesc, String strSvrObj )
    { super( pIoMgr, strDesc, strSvrObj ); }
    
    // IHelloWorld::Echo sync-handler
    public int Echo(
        JavaReqContext oReqCtx,
        String strText )
    {
        // Synchronous handler. Make sure to call
        // oReqCtx.setResponse before return
        int ret = RC.STATUS_SUCCESS;
        System.out.println("Received message '" + strText + "'");
        oReqCtx.setResponse(ret, strText + " from server");
        return ret;
    }
    
    
}