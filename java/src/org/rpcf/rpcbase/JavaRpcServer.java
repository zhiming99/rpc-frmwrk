package org.rpcf.rpcbase;
import java.lang.String;
import java.util.HashMap;
import java.util.Map;
import java.util.ArrayList;
import java.util.List;

abstract public class JavaRpcServer extends JavaRpcServiceS 
{
    public JavaRpcServer( ObjPtr pIoMgr,
        String strDesc, String strSvrObj )
    {
        JRetVal jret = null;
        do{
            m_pIoMgr = pIoMgr;
            CParamList oParams = new CParamList();
            jret = ( JRetVal )oParams.GetCfg();
            if( jret.ERROR() )
                break;
            ObjPtr pCfg = ( ObjPtr )jret.getAt( 0 );
            jret = ( JRetVal )rpcbase.CreateServer(
                pIoMgr, strDesc, strSvrObj, pCfg );

            if( jret.ERROR() )
                break;

            ObjPtr obj = ( ObjPtr )jret.getAt( 0 );
            jret = ( JRetVal )
                rpcbase.CastToServer( obj );
            if( jret.ERROR() )
                break;

            m_oInst = ( CJavaServer )
                jret.getAt( 0 );

        }while( false );

        if( jret.ERROR() )
            setError( jret.getError() );

        return;
    }

    /* callback should be the one passed to the
     * InvokeMethod. This is for asynchronous invoke,
     * that is, the invoke method returns pending and
     * the task will complete later in the future
     * with a call to this method. the callback is
     * the glue between InvokeMethod and
     * OnServiceComplete.
     */
    public int onServiceComplete(
        ObjPtr callback, int ret, Object[] args )
    {
        return m_oInst.OnServiceComplete(
            callback, ret,
            RC.seriNone,
            args );
    }

    public int setResponse(
        ObjPtr callback, int ret, Object[] args )
    {
        return m_oInst.SetResponse(
            callback, ret,
            RC.seriNone,
            args );
    }

    public int ridlOnServiceComplete(
        ObjPtr callback, int ret, BufPtr pBuf )
    {
        List<Object> lstObj =
            new ArrayList< Object >();
        if( pBuf != null )
            lstObj.add( pBuf );
        return m_oInst.OnServiceComplete(
            callback, ret,
            RC.seriRidl,
            lstObj.toArray() );
    }

    public int ridlSetResponse(
        ObjPtr callback, int ret, byte[] pBuf )
    {
        List<Object> lstObj =
            new ArrayList< Object >();
        lstObj.add( pBuf );
        return m_oInst.SetResponse(
            callback, ret,
            RC.seriRidl,
            lstObj.toArray() );
    }

    /* callback can be none if it is not
     * necessary to get notified of the completion.
     * destName can be none if not needed.
     */
    public int sendEvent( Object callback,
        String ifName, String evtName,
        String destName, Object[] arrArgs )
    {
        evtName = "UserEvent_" + evtName;
        return m_oInst.SendEvent( callback,
            ifName, evtName, destName, arrArgs,
            RC.seriNone );
    }

    /* callback can be none if it is not
     * necessary to get notified of the completion.
     * destName can be none if not needed.
     */
    public int ridlSendEvent( Object callback,
        String ifName, String evtName,
        String destName, BufPtr pBuf )
    {
        evtName = "UserEvent_" + evtName;
        Object[] argsArr = new Object[]{ pBuf };
        return m_oInst.SendEvent( callback,
            ifName, evtName, destName,
            argsArr, RC.seriRidl );
    }

    /* event called when the stream channel
     * described by pDataDesc is about to establish
     * If accepted, the OnStmReady will be called,
     * and the datadesc can be retrieved by the
     * stream handle.
     */
    public int acceptNewStream(
        ObjPtr pCtx, CfgPtr pDataDesc )
    { return 0; }

    public boolean isServer()
    { return true; }
}
