package org.rpcf.rpcbase;
import java.util.List;

public interface IRpcService
{
    public int getError();

    public void setError( int iError );

    public int getState();

    public int start();

    public int stop();
    // types of callback
    //
    public interface IAsyncRespCb {
        public abstract int getArgCount();
        public abstract Class<?>[] getArgTypes();
        public abstract void onAsyncResp(
                Object oContext, int iRet, Object[] oParams );
    }

    public interface IReqHandler {
        public abstract int getArgCount();
        public abstract Class<?>[] getArgTypes();
        public abstract JRetVal invoke(
                Object oHost,
                ObjPtr callback,
                Object[] oParams );
    }

    public interface IEvtHandler extends IReqHandler{
    }

    public interface IDeferredCall {
        public abstract int getArgCount();
        public abstract Class<?>[] getArgTypes();
        public abstract void call( Object[] oParams );
    }

    public interface ICancelNotify extends IAsyncRespCb{
    }

    public interface IUserTimerCb {
        public abstract void onTimer( Object octx, int iRet );
    }

    public JRetVal addTimer(
        int timeoutSec, Object callback,  Object context );

    /*Remove a previously scheduled timer.
     */
    public int disableTimer( ObjPtr timerObj );

    public int installCancelNotify(
        ObjPtr task, ICancelNotify callback,
        Object[] listArgs );

    /* called from the underlying C++ code on
     * response arrival.
     */
    public void handleAsyncResp(
        Object callback, int seriProto,
        Object listResp, Object context );

    /* establish a stream channel and
     * return a handle to the channel on success
     */
    public JRetVal startStream( CParamList oDesc );

    public int closeStream( long hChannel );

    public int writeStream( long hChannel, byte[] pBuf );

    public void onWriteStreamComplete(
        int iRet, long hChannel, byte[] buf );

    public int writeStreamAsync(
        long hChannel, byte[] pBuf, IAsyncRespCb callback );

    public int writeStreamAsync( long hChannel, byte[] pBuf );

    public int writeStreamNoWait( long hChannel, byte[] pBuf );
    public JRetVal readStream( long hChannel );

    public JRetVal readStream( long hChannel, int size );

    public void onReadStreamComplete(
        int iRet, long hChannel, byte[] buf );

    public JRetVal readStreamAsync( long hChannel, int size );

    public JRetVal readStreamAsync(
        long hChannel, int size, IAsyncRespCb callback );
    /*
     * ReadStreamNoWait to read the first block of
     * buffered data from the stream. If there are
     * no data buffered, error -AGAIN will be
     * returned, and will not block to wait.
     */
    public JRetVal readStreamNoWait( long hChannel);

    /* event called when the stream `hChannel' is
     * ready
     */
    public int onStmReady( long hChannel );

    /* event called when the stream `hChannel' is
     * about to close
     */
    public int onStmClosing( long hChannel ) ;

    //for event handler
    public JRetVal invokeMethod(
        ObjPtr callback, String ifName, String methodName,
        int seriProto, CParamList cppargs );

    /* Invoke a callback function depending on
     * it is bound function or not
     */
    public int invokeCallback(
        IAsyncRespCb callback, List< Object > listArgs ) ;

    public int deferCall(
            IDeferredCall callback, Object[] args ) ;

    public void deferCallback(
            Object callback, Object listArgs );

    public int setChanCtx( long hChannel, Object pCtx);

    public int removeChanCtx( long hChannel ) ;
    public JRetVal getChanCtx( long hChannel ) ;
}

