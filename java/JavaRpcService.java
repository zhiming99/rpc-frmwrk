package org.rpcf.rpcbase;
import java.lang.String;
import java.util.HashMap;
import java.util.Map;
import java.util.List;
import java.util.ArrayList;
import java.lang.ClassCastException;

/*
 * Hilarious, I need C preprocessor to spawn the java
 * classes and cannot find native way to do it yet.
 */

abstract public class JavaRpcService
{
    protected int m_iError = 0;
    protected ObjPtr m_pIoMgr = null;
    protected InstType m_oInst = null;

    public boolean IsServer;
    public abstract Map< String, IReqHandler > initMaps();

    public int GetError()
    { return m_iError; }

    public InstType getInst() { return m_oInst; }

    public void SetError( int iErr )
    { m_iError = iErr; }

    public int start()
    {
        if( m_oInst == null )
            return -RC.EFAULT;
        m_oInst.SetJavaHost( this );
        int ret = m_oInst.Start();

        if( ret < 0 )
        {
            if( isServer() )
                System.out.println(
                    "Failed start server..." );
            else
                System.out.println(
                    "Failed start proxy..." );
        }
        else
        {
            if( isServer() )
                System.out.println(
                    "Server started..." );
            else
                System.out.println(
                    "Proxy started..." );
        }
        return ret;
    }

    public int stop()
    {
        if( m_oInst == null )
            return 0;
        m_oInst.RemoveJavaHost();
        return m_oInst.Stop();
    }

    // types of callback
    //
    public interface IAsyncRespCb {
        public abstract int getArgCount();
        public abstract Class<?>[] getArgTypes();
        public abstract void onAsyncResp( Object oContext,
            int iRet, Object[] oParams ); 
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

    public void timerCallback(
        Object callback, Object context, int iRet )
    {
        IUserTimerCb tcb = ( IUserTimerCb )callback; 
        tcb.onTimer( context, iRet );
    }

    /*create a timer object, which is due in timeoutSec
     * seconds. And the `callback' will be called with
     * the parameter context.

     * addTimer returns JRetVal object, includes the
     * error code, and a timerObj if the error code is
     * STATUS_SUCCESS.
     */
    public JRetVal addTimer( int timeoutSec,
        Object callback,  Object context )
    {
        Object jret = m_oInst.AddTimer(
            timeoutSec, callback, context );
        return ( JRetVal )jret;
    }

    /*Remove a previously scheduled timer.
    */
    public int disableTimer( ObjPtr timerObj )
    {
        return m_oInst.DisableTimer( timerObj );
    }

    /* attach a complete notification to the task
     * to complete. When the task is completed or
     * canceled, this notification will be triggered.
     * the notification has the following layout
     * callback( ret, *listArgs ), as is the
     * same as the response callback. The `ret' is
     * the error code of the task.
     */
    public int installCancelNotify( ObjPtr task,
        ICancelNotify callback,
        Object[] listArgs )
    {
        JRetVal jret = new JRetVal();
        for( Object i : listArgs )
            jret.addElem( i );
        return m_oInst.InstallCancelNotify(
            task, callback, listArgs );
    }

    /* called from the underlying C++ code on
     * response arrival.
     */
    public void handleAsyncResp( Object callback,
        int seriProto, Object listResp,
        Object context )
    {
        if( callback == null )
            return;

        try{ 

            JRetVal jret = ( JRetVal )listResp;
            do{
                IAsyncRespCb asyncb =
                    ( IAsyncRespCb )callback;

                List<Object> listArgs =
                    new ArrayList< Object >();

                int iArgNum = asyncb.getArgCount();
                for( int i = 0; i < iArgNum + 2; i++ ) 
                    listArgs.add( null );

                if( listResp == null )
                {
                    listArgs.set( 1, -RC.EBADMSG );
                    invokeCallback( asyncb, listArgs );
                    break;
                }

                int ret = jret.getError();

                listArgs.set( 0, context );
                listArgs.set( 1, ret );
                int iSize = jret.getParamCount();
                if( jret.ERROR() || iSize == 0 )
                {
                    invokeCallback( asyncb, listArgs );
                    break;
                }
                if( seriProto ==
                    RC.seriNone )
                {
                    if( iArgNum != iSize )
                    {
                        ret = -RC.EINVAL;
                        listArgs.set( 1, ret );
                    }
                    if( ret == RC.STATUS_SUCCESS )
                        for( int i = 0; i < iArgNum; i++ )
                            listArgs.set( i + 2, jret.getAt( i ) );

                    invokeCallback( asyncb, listArgs );
                }
                else if( seriProto ==
                        RC.seriRidl )
                {
                    if( iArgNum == 1 && iArgNum == iSize ) 
                        listArgs.set( 2, jret.getAt( 0 ) );
                    else
                    {
                        ret = -RC.EINVAL;
                        listArgs.set( 1, ret );
                    }
                    invokeCallback( asyncb, listArgs );
                }
                else
                {
                    break;
                }
            }while( false );
        }
        catch( IllegalArgumentException e )
        {
        }
        catch( NullPointerException e )
        {
        }
        catch( IndexOutOfBoundsException e )
        {
        }
        catch( ClassCastException e )
        {
        }
        catch( Exception e )
        {
        }

        return;
    }

    /* establish a stream channel and
     * return a handle to the channel on success
     */
    public JRetVal startStream( CParamList oDesc )
    {
        if( isServer() )
            return null;

        if( oDesc == null )
            oDesc = new CParamList();
        JRetVal jret = ( JRetVal )
            m_oInst.StartStream(
                ( ObjPtr )oDesc.GetCfg() );
        if( jret == null )
        {
            jret = new JRetVal();
            jret.setError( -RC.EFAULT );
        }
        return jret;
    }

    public int closeStream( long hChannel )
    {
        return m_oInst.CloseStream( hChannel );
    }

    public int writeStream( long hChannel, byte[] pBuf )
    {
        return m_oInst.WriteStream( hChannel, pBuf );
    }

    public void onWriteStreamComplete( int iRet,
        long hChannel, byte[] buf )
    { return; }

    IAsyncRespCb m_oWriteStmCallback = new IAsyncRespCb() {
        public int getArgCount()
        { return 1; }
        public Class<?>[] getArgTypes()
        { return new Class<?>[]{ byte[].class }; }

        public void onAsyncResp( Object oContext,
            int iRet, Object[] oParams ) 
        {
            long hChannel;
            if( !( oContext instanceof Long ) )
                return;
            hChannel = ( ( Long )oContext ).longValue();
            byte[] buf = null;
            if( iRet == RC.STATUS_SUCCESS )
                buf = ( byte[] )oParams[ 0 ];
            onWriteStreamComplete( iRet, hChannel, buf );
        }
    };

    public int writeStreamAsync( long hChannel,
        byte[] pBuf, IAsyncRespCb callback )
    {
        return m_oInst.WriteStreamAsync(
            hChannel, pBuf, callback );
    }

    public int writeStreamAsync( long hChannel, byte[] pBuf )
    {
        return m_oInst.WriteStreamAsync(
            hChannel, pBuf, m_oWriteStmCallback );
    }

    /* ReadStream to read `size' bytes from the
     * stream `hChannel'.  If `size' is zero, the
     * number of bytes read depends on the first
     * message received. If you are not sure what
     * will come from the stream, you are recommended
     * to use zero size.

     * The return value is a list of 3 elements,
     * element 0 is the error code. If it is
     * negative, no data is returned. If it is
     * STATUS_SUCCESS, element 1 is a bytearray
     * object read from the stream. element 2 is a
     * hidden object to keep element 1 valid, just
     * leave it alone.
    */
    public JRetVal readStream( long hChannel )
    {
        return ( JRetVal )
            m_oInst.ReadStream( hChannel, 0 );
    }

    public JRetVal readStream( long hChannel, int size )
    {
        return ( JRetVal )
            m_oInst.ReadStream( hChannel, size );
    }

    /*
     * ReadStreamAsync to read `size' bytes from the
     * stream `hChannel', and give control to
     * callback after the data is read from the
     * stream. If `size' is zero, the number of bytes
     * read depends on the first message in the
     * pending message queue. If you are not sure
     * what will come from the stream, you are
     * recommended to use zero size.

     * The return value is a list of 3 elements,
     * element 0 is the error code. If it is
     * STATUS_PENDING, there is no data in the
     * pending message queue, and wait till the
     * callback is called in the future time.

     * If the return value is STATUS_SUCCESS, element
     * 1 holds a bytearray object of the bytes read
     * from the stream. And element 2 is a hidden
     * object, that you don't want to access.
     * Actually it controls the life time of element
     * 1, so make sure not to access elemen 1 if
     * element2 is no longer valid.
     */

    public void onReadStreamComplete( int iRet,
        long hChannel, byte[] buf )
    { return; }

    IAsyncRespCb m_oReadStmCallback = new IAsyncRespCb() {
        public int getArgCount()
        { return 1; }
        public Class<?>[] getArgTypes()
        { return new Class<?>[]{ byte[].class }; }
        public void onAsyncResp( Object oContext,
            int iRet, Object[] oParams ) 
        {
            long hChannel;
            if( ! ( oContext instanceof Long ) )
                return;
            hChannel = ( ( Long )oContext ).longValue();
            byte[] buf = null;
            if( iRet == RC.STATUS_SUCCESS )
                buf = ( byte[] )oParams[ 0 ];
            onReadStreamComplete( iRet, hChannel, buf );
        }
    };

    public JRetVal readStreamAsync( long hChannel, int size )
    {
        return ( JRetVal )m_oInst.ReadStreamAsync(
            hChannel, m_oReadStmCallback, size );
    }

    public JRetVal readStreamAsync( long hChannel,
        int size, IAsyncRespCb callback )
    {
        return ( JRetVal )m_oInst.ReadStreamAsync(
            hChannel, callback, size );
    }
    /*
     * ReadStreamNoWait to read the first block of
     * bufferrd data from the stream. If there are
     * not data bufferred, error -EAGIN will be
     * returned, and will not blocked to wait.
     */
    public JRetVal readStreamNoWait( long hChannel)
    {
        return ( JRetVal )
            m_oInst.ReadStreamNoWait( hChannel );
    }

    /* event called when the stream `hChannel' is
     * ready
     */
    public int onStmReady( long hChannel )
    { return 0; }

    /* event called when the stream `hChannel' is
     * about to close
     */
    public int onStmClosing( long hChannel ) 
    { return 0; }

    /* Convert the arguments in the `pObj' c++
     * object to a list of python object.
     *
     * The return value is a two element list. The
     * first element is the error code and the second
     * element is a list of python object. The second
     * element should be None if error occurs during
     * the conversion.
     */

    JRetVal argObjToList( int seriProto, ObjPtr pObj ) 
    {
        JRetVal jret = new JRetVal();
        do{
            CfgPtr pCfg = rpcbase.CastToCfg( pObj );
            if( pCfg == null )
            {
                jret.setError( -RC.EFAULT );
                break;
            }

            CParamList oParams =
                new CParamList( pCfg );

            jret = ( JRetVal )oParams.GetSize();
            if( jret.ERROR() ||
                jret.getParamCount() == 0 )
                break;

            int iSize = ( ( Integer )
                jret.getAt( 0 ) ).intValue();

            jret.clear();

            Object val;
            if( seriProto == RC.seriRidl )
            {
                jret = ( JRetVal )
                    oParams.GetByteArray( 0 );
                if( jret.getError() == -RC.ENOENT )
                {
                    // fine, OK with 0 parameter.
                    jret.clear();
                    break;
                }

                if( jret.ERROR() )
                    break;

                val = jret.getAt( 0 );
                if( val == null )
                {
                    jret.clear();
                    jret.setError( -RC.EFAULT );
                    break;
                }

                byte[] bytearr = ( byte[] )val;
                jret.addElem( bytearr );
                break;
            }
            else if( seriProto != RC.seriNone )
            {
                jret.setError( 
                    -RC.EBADMSG );
                break;
            }
                
            for( int i = 0; i < iSize; i++ )
            {
                jret = ( JRetVal )
                    oParams.GetPropertyType( i );

                if( jret.ERROR() )
                    break;

                val = jret.getAt( 0 );
                if( val == null )
                {
                    jret.setError(
                        -RC.ENOENT );
                    break;
                }

                int iType = ( Integer )val;

                JRetVal jret2 = new JRetVal();
                switch( iType )
                {
                case RC.typeUInt32 :
                    jret2 = ( JRetVal )
                        oParams.GetIntProp( i );
                    break;
                case RC.typeDouble :
                    jret2  = ( JRetVal )
                        oParams.GetDoubleProp( i );
                    break;
                case RC.typeByte :
                    jret2 = ( JRetVal )
                        oParams.GetByteProp( i );
                    break;
                case RC.typeUInt16 :
                    jret2 = ( JRetVal )
                        oParams.GetShortProp( i );
                    break;
                case RC.typeUInt64 :
                    jret2 = ( JRetVal )
                        oParams.GetQwordProp( i );
                    break;
                case RC.typeFloat :
                    jret2 = ( JRetVal )
                        oParams.GetFloatProp( i );
                    break;
                case RC.typeString :
                    jret2 = ( JRetVal )
                        oParams.GetStrProp( i );
                    break;
                case RC.typeObj :
                    jret2 = ( JRetVal )
                        oParams.GetObjPtr( i );
                    break;
                case RC.typeByteArr :
                    jret2 = ( JRetVal )
                        oParams.GetProperty( i );
                    break;
                }
                if( jret2.ERROR() )
                {
                    jret.clear();
                    jret.setError( jret2.getError() );
                    break;
                }

                val = jret2.getAt( 0 );
                if( val == null )
                {
                    jret.clear();
                    jret.setError(
                        -RC.ENOENT );
                    break;
                }
                jret2.clear();
                jret.addElem( val );
            }

        }while( false );

        return jret;
    }

    // map for request handlers
    Map< String, IReqHandler >
        m_mapReqHandlers = initMaps();

    //for event handler 
    public JRetVal invokeMethod( ObjPtr callback,
        String ifName, String methodName,
        int seriProto, ObjPtr cppargs )
    {
        JRetVal oResp = new JRetVal();
        do{
            JRetVal jret = new JRetVal();
            jret = argObjToList(
                seriProto, cppargs );
            if( jret.ERROR() )
            {
                oResp = jret;
                break;
            }

            Object[] argList =
                jret.getParamArray();

            boolean found = false;
            String[] nameComps =
                methodName.split( "_" );

            if( isServer() )
            {
                if( nameComps[ 0 ] != "UserMethod" )
                {
                    oResp.setError( -RC.EINVAL );
                    break;
                }
            }
            else 
            {
                if( nameComps[ 0 ] != "UserEvent" )
                {
                    oResp.setError( -RC.EINVAL );
                    break;
                }
            }

            String strKey =
                ifName + "::" + nameComps[ 1 ]; 
            IReqHandler oMethod =
                m_mapReqHandlers.get( strKey );
            if( oMethod == null )
            {
                // tell the native code to search
                // other places
                oResp.setError( -RC.ENOTSUP );
                break;
            }
                
            try
            {
                oResp = oMethod.invoke(
                    this, callback, argList );
            }
            catch( IllegalArgumentException e )
            {
                oResp.setError( -RC.EINVAL );
            }
            catch( NullPointerException e )
            {
                oResp.setError( -RC.EFAULT );
            }
            catch( IndexOutOfBoundsException e )
            {
                oResp.setError( -RC.ERANGE );
            }
            catch( Exception e )
            {
                oResp.setError( -RC.EFAULT );
            }
                
        }while( false );

        return oResp;
    }

    /* Invoke a callback function depending
     * it is bound function or not
     */
    public int invokeCallback( IAsyncRespCb callback,
        List< Object > listArgs )
    {
        int ret = 0;
        try{
            Object context = listArgs.get( 0 );
            int iRet =
                ( ( Integer )listArgs.get( 1 ) ).intValue();

            listArgs.remove( 0 );
            listArgs.remove( 0 );

            callback.onAsyncResp(
                context, iRet, listArgs.toArray() );
        }
        catch( IllegalArgumentException e )
        {
            ret = -RC.ERANGE;
        }
        catch( IndexOutOfBoundsException e )
        {
            ret = -RC.ERANGE;
        }
        catch( NullPointerException e )
        {
            ret = -RC.EFAULT;
        }
        catch( Exception e )
        {
            ret = -RC.EFAULT;
        }
        return ret;
    }

    /* run the callback on a new context instead
     * of the current context
     * 
     * DeferCall, like a functor object bind to the
     * callback function with the arguments. And it
     * can be scheduled run on a new stack context.
     * The purpose is to reduce the depth of the call
     * stack and provide a locking free or light
     * locking context as the current one cannot.
     */
    public int deferCall(
        IDeferredCall callback, Object[] args )
    {
        return m_oInst.DeferCall( callback, args );
    }

    public void deferCallback(
        Object callback, Object listArgs )
    {
        IDeferredCall dc = ( IDeferredCall )callback;
        if( !Helpers.isArray( listArgs ) )
            return;
        Object[] arrArgs = ( Object[] )listArgs;
        dc.call( arrArgs );
        return;
    }

    public int setChanCtx(
        long hChannel, Object pCtx)
    {
        return m_oInst.SetChanCtx(
            hChannel, pCtx );
    }

    public int removeChanCtx(
        long hChannel )
    {
        return m_oInst.RemoveChanCtx( hChannel );
    }

    public JRetVal getChanCtx( long hChannel )
    {
        return ( JRetVal )m_oInst.GetChanCtx(
            hChannel );
    }
}

