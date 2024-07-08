package org.rpcf.rpcbase;
import java.util.ArrayList;
import java.util.List;

abstract public class JavaRpcServer extends JavaRpcServiceS 
{
    public JavaRpcServer( ObjPtr pIoMgr,
        String strDesc, String strSvrObj )
    {
        JRetVal jret;
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

            setInst( ( CJavaServer )
                jret.getAt( 0 ) );
            obj.Clear();

        }while( false );

        if( jret.ERROR() )
            setError( jret.getError() );
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
        if( getInst() == null )
            return -RC.EFAULT;

        return getInst().OnServiceComplete(
            callback, ret,
            RC.seriNone,
            args );
    }

    public int setResponse(
        ObjPtr callback, int ret, Object[] args )
    {
        if( getInst() == null )
            return -RC.EFAULT;

        return getInst().SetResponse(
            callback, ret,
            RC.seriNone,
            args );
    }

    public int ridlOnServiceComplete(
        ObjPtr callback, int ret, BufPtr pBuf )
    {
        if( getInst() == null )
            return -RC.EFAULT;

        List<Object> lstObj =
            new ArrayList<>();
        if( pBuf != null )
            lstObj.add( pBuf );
        return getInst().OnServiceComplete(
            callback, ret,
            RC.seriRidl,
            lstObj.toArray() );
    }

    public int ridlSetResponse(
        ObjPtr callback, int ret, byte[] pBuf )
    {
        if( getInst() == null )
            return -RC.EFAULT;
        List<Object> lstObj =
            new ArrayList<>();
        lstObj.add( pBuf );
        return getInst().SetResponse(
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
        return getInst().SendEvent( callback,
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
        if( getInst() == null )
            return -RC.EFAULT;
        evtName = "UserEvent_" + evtName;
        Object[] argsArr = new Object[]{ pBuf };
        return getInst().SendEvent( callback,
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

    public int LogMessage( int dwLogLevel,
        int ret, String strMsg )
    {
        StackTraceElement[] se =
            Thread.currentThread().getStackTrace();
        String strFile = se[ 3 ].getFileName();
        int lineNo = se[ 3 ].getLineNumber();
        return getInst().LogMessage(
            dwLogLevel, strFile, lineNo, ret, strMsg );
    }

    public int LogInfo( int ret, String strMsg )
    { return LogMessage( RC.logInfo, ret, strMsg ); }

    public int LogError( int ret, String strMsg )
    { return LogMessage( RC.logErr, ret, strMsg ); }

    public int LogAlert( int ret, String strMsg )
    { return LogMessage( RC.logAlert, ret, strMsg ); }

    public int LogWarn( int ret, String strMsg )
    { return LogMessage( RC.logWarning, ret, strMsg ); }

    public int LogCritical( int ret, String strMsg )
    { return LogMessage( RC.logCrit, ret, strMsg ); }

    public int LogEmerg( int ret, String strMsg )
    { return LogMessage( RC.logEmerg, ret, strMsg ); }

    public int LogNotice( int ret, String strMsg )
    { return LogMessage( RC.logNotice, ret, strMsg ); }
}
