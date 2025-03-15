package org.rpcf.rpcbase;
import java.util.List;

public interface IRpcService
{
    int getError();

    void setError( int iError );

    int getState();

    int start();

    int stop();
    // types of callback
    //
    interface IAsyncRespCb {
        int getArgCount();
        Class<?>[] getArgTypes();
        void onAsyncResp(
                Object oContext, int iRet, Object[] oParams );
    }

    interface IReqHandler {
        int getArgCount();
        Class<?>[] getArgTypes();
        JRetVal invoke(
                Object oHost,
                ObjPtr callback,
                Object[] oParams );
    }

    interface IEvtHandler extends IReqHandler{
    }

    interface IDeferredCall{
        int getArgCount();
        Class<?>[] getArgTypes();
        void call( Object[] oParams );
    }

    interface ICancelNotify extends IAsyncRespCb{
    }

    interface IUserTimerCb {
        void onTimer( Object octx, int iRet );
    }

    JRetVal addTimer(
        int timeoutSec, Object callback,  Object context );

    /*Remove a previously scheduled timer.
     */
    int disableTimer( ObjPtr timerObj );

    int installCancelNotify(
        ObjPtr task, ICancelNotify callback,
        Object[] listArgs );

    /* called from the underlying C++ code on
     * response arrival.
     */
    void handleAsyncResp(
        Object callback, int seriProto,
        Object listResp, Object context );

    /* establish a stream channel and
     * return a handle to the channel on success
     */
    JRetVal startStream( CParamList oDesc );

    int closeStream( long hChannel );

    int writeStream( long hChannel, byte[] pBuf );

    void onWriteStreamComplete(
        int iRet, long hChannel, byte[] buf );

    int writeStreamAsync(
        long hChannel, byte[] pBuf, IAsyncRespCb callback );

    int writeStreamAsync( long hChannel, byte[] pBuf );

    int writeStreamNoWait( long hChannel, byte[] pBuf );
    JRetVal readStream( long hChannel );

    JRetVal readStream( long hChannel, int size );

    void onReadStreamComplete(
        int iRet, long hChannel, byte[] buf );

    JRetVal readStreamAsync( long hChannel, int size );

    JRetVal readStreamAsync(
        long hChannel, int size, IAsyncRespCb callback );
    /*
     * ReadStreamNoWait to read the first block of
     * buffered data from the stream. If there are
     * no data buffered, error -AGAIN will be
     * returned, and will not block to wait.
     */
    JRetVal readStreamNoWait( long hChannel);

    /* event called when the stream `hChannel' is
     * ready
     */
    int onStmReady( long hChannel );

    /* event called when the stream `hChannel' is
     * about to close
     */
    int onStmClosing( long hChannel ) ;

    //for event handler
    JRetVal invokeMethod(
        ObjPtr callback, String ifName, String methodName,
        int seriProto, CParamList cppargs );

    /* Invoke a callback function depending on
     * it is bound function or not
     */
    int invokeCallback(
        IAsyncRespCb callback, List< Object > listArgs ) ;

    int deferCall(
            IDeferredCall callback, Object[] args ) ;

    void deferCallback(
            Object callback, Object listArgs );

    int setChanCtx( long hChannel, Object pCtx);

    int removeChanCtx( long hChannel ) ;
    JRetVal getChanCtx( long hChannel ) ;
    boolean isServer();
    Object getSerialBase();
    int onPostStop();
}

