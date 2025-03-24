package org.rpcf.rpcbase;
import java.util.Map;
import java.util.List;
import java.util.ArrayList;

/*
 * Note: we need a C preprocessor to spawn the final
 * java classes.
 */

abstract public class JavaRpcService implements IRpcService
{
    protected int m_iError = 0;
    protected ObjPtr m_pIoMgr = null;
    protected InstType m_oInst = null;

    @Override
    public boolean IsServer;
    public abstract Map< String, IReqHandler > initMaps();

    @Override
    public int getError()
    { return m_iError; }

    @Override
    public void setError( int iError )
    { m_iError = iError; }

    public InstType getInst()
    { return m_oInst; }

    public void setInst( InstType oInst )
    { m_oInst = oInst; }

    @Override
    public int getState()
    {
        InstType oInst = getInst();
        if( oInst != null )
            return oInst.GetState();
        return RC.stateInvalid;
    }

    @Override
    public int start()
    {
        InstType oInst = getInst();
        if( oInst == null )
            return -RC.EFAULT;
        oInst.SetJavaHost( this );
        int ret = oInst.Start();

        if( ret < 0 )
        {
            if( isServer() )
                System.out.println(
                    "Failed start server..." + ret );
            else
                System.out.println(
                    "Failed start proxy..." + ret );
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

    @Override
    public int stop()
    {
        InstType oInst = getInst();
        if( oInst == null )
            return -RC.EFAULT;
        oInst.RemoveJavaHost();
        return oInst.Stop();
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
    @Override
    public JRetVal addTimer( int timeoutSec,
        Object callback,  Object context )
    {
        InstType oInst = getInst();
        if( oInst == null )
        {
            JRetVal jret = new JRetVal();
            jret.setError( -RC.EFAULT );
            return jret;
        }
        Object jret = oInst.AddTimer(
            timeoutSec, callback, context );
        return ( JRetVal )jret;
    }

    /*Remove a previously scheduled timer.
    */
    @Override
    public int disableTimer( ObjPtr timerObj )
    {
        InstType oInst = getInst();
        if( oInst == null )
            return -RC.EFAULT;

        return oInst.DisableTimer( timerObj );
    }

    /* attach a complete notification to the task
     * to complete. When the task is completed or
     * canceled, this notification will be triggered.
     * the notification has the following layout
     * callback( ret, *listArgs ), as is the
     * same as the response callback. The `ret' is
     * the error code of the task.
     */
    @Override
    public int installCancelNotify( ObjPtr task,
        ICancelNotify callback,
        Object[] listArgs )
    {
        InstType oInst = getInst();
        if( oInst == null )
            return -RC.EFAULT;

        JRetVal jret = new JRetVal();
        for( Object i : listArgs )
            jret.addElem( i );
        return oInst.InstallCancelNotify(
            task, callback, jret );
    }

    /* called from the underlying C++ code on
     * response arrival.
     */
    @Override
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
                    new ArrayList<>();

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
                    for( int i = 0; i < iSize; i++ )
                        listArgs.set( i + 2, jret.getAt( i ) );
                    invokeCallback( asyncb, listArgs );
                    break;
                }
                if( seriProto == RC.seriNone )
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
                else if( seriProto == RC.seriRidl )
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
        catch( IllegalArgumentException | NullPointerException | IndexOutOfBoundsException | ClassCastException e )
        {
        }
        catch( Exception e )
        {
        }
    }

    /* establish a stream channel and
     * return a handle to the channel on success
     */
    @Override
    public JRetVal startStream( CParamList oDesc )
    {
        if( isServer() )
            return null;

        JRetVal jret;
        ObjPtr o = null;

        if( oDesc != null )
        { 
            jret = ( JRetVal )oDesc.GetCfg();
            if( jret.ERROR() )
                return jret;
            o = ( ObjPtr )jret.getAt( 0 );
        }

        InstType oInst = getInst();
        if( oInst == null )
        {
            jret = new JRetVal();
            jret.setError( -RC.EFAULT );
            return jret;
        }

        jret = ( JRetVal )oInst.StartStream( o );

        if( jret == null )
        {
            jret = new JRetVal();
            jret.setError( -RC.EFAULT );
        }
        return jret;
    }

    @Override
    public int closeStream( long hChannel )
    {
        InstType oInst = getInst();
        if( oInst == null )
            return -RC.EFAULT;
        return oInst.CloseStream( hChannel );
    }

    @Override
    public int writeStream( long hChannel, byte[] pBuf )
    {
        InstType oInst = getInst();
        if( oInst == null )
            return -RC.EFAULT;
        return oInst.WriteStream( hChannel, pBuf );
    }

    @Override
    public void onWriteStreamComplete( int iRet,
        long hChannel, byte[] buf )
    {}

    IAsyncRespCb m_oWriteStmCallback = new IAsyncRespCb() {
        @Override
        public int getArgCount()
        { return 1; }
        @Override
        public Class<?>[] getArgTypes()
        { return new Class<?>[]{ byte[].class }; }

        @Override
        public void onAsyncResp( Object oContext,
            int iRet, Object[] oParams ) 
        {
            long hChannel;
            if( !( oContext instanceof Long ) )
                return;
            hChannel = ( Long )oContext;
            byte[] buf = null;
            if( iRet == RC.STATUS_SUCCESS )
                buf = ( byte[] )oParams[ 0 ];
            onWriteStreamComplete( iRet, hChannel, buf );
        }
    };

    @Override
    public int writeStreamAsync( long hChannel,
        byte[] pBuf, IAsyncRespCb callback )
    {
        InstType oInst = getInst();
        if( oInst == null )
            return -RC.EFAULT;
        return oInst.WriteStreamAsync(
            hChannel, pBuf, callback );
    }

    @Override
    public int writeStreamAsync( long hChannel, byte[] pBuf )
    {
        InstType oInst = getInst();
        if( oInst == null )
            return -RC.EFAULT;
        return oInst.WriteStreamAsync(
            hChannel, pBuf, m_oWriteStmCallback );
    }

    @Override
    public int writeStreamNoWait( long hChannel, byte[] pBuf )
    {
        InstType oInst = getInst();
        if( oInst == null )
            return -RC.EFAULT;
        return oInst.WriteStreamNoWait( hChannel, pBuf );
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
    @Override
    public JRetVal readStream( long hChannel )
    {
        InstType oInst = getInst();
        if( oInst == null )
        {
            JRetVal jret = new JRetVal();
            jret.setError( -RC.EFAULT );
            return jret;
        }
        return ( JRetVal )
            oInst.ReadStream( hChannel, 0 );
    }

    @Override
    public JRetVal readStream( long hChannel, int size )
    {
        InstType oInst = getInst();
        if( oInst == null )
        {
            JRetVal jret = new JRetVal();
            jret.setError( -RC.EFAULT );
            return jret;
        }
        return ( JRetVal )
            oInst.ReadStream( hChannel, size );
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

    @Override
    public void onReadStreamComplete( int iRet,
        long hChannel, byte[] buf )
    {}

    IAsyncRespCb m_oReadStmCallback = new IAsyncRespCb() {
        @Override
        public int getArgCount()
        { return 1; }
        @Override
        public Class<?>[] getArgTypes()
        { return new Class<?>[]{ byte[].class }; }
        @Override
        public void onAsyncResp( Object oContext,
            int iRet, Object[] oParams ) 
        {
            long hChannel;
            if( ! ( oContext instanceof Long ) )
                return;
            hChannel = ( Long )oContext;
            byte[] buf = null;
            if( iRet == RC.STATUS_SUCCESS )
                buf = ( byte[] )oParams[ 0 ];
            onReadStreamComplete( iRet, hChannel, buf );
        }
    };

    @Override
    public JRetVal readStreamAsync( long hChannel, int size )
    {
        InstType oInst = getInst();
        if( oInst == null )
        {
            JRetVal jret = new JRetVal();
            jret.setError( -RC.EFAULT );
            return jret;
        }
        return ( JRetVal )oInst.ReadStreamAsync(
            hChannel, m_oReadStmCallback, size );
    }

    @Override
    public JRetVal readStreamAsync( long hChannel,
        int size, IAsyncRespCb callback )
    {
        InstType oInst = getInst();
        if( oInst == null )
        {
            JRetVal jret = new JRetVal();
            jret.setError( -RC.EFAULT );
            return jret;
        }
        return ( JRetVal )oInst.ReadStreamAsync(
            hChannel, callback, size );
    }
    /*
     * ReadStreamNoWait to read the first block of
     * bufferrd data from the stream. If there are
     * not data bufferred, error -EAGIN will be
     * returned, and will not blocked to wait.
     */
    @Override
    public JRetVal readStreamNoWait( long hChannel)
    {
        InstType oInst = getInst();
        if( oInst == null )
        {
            JRetVal jret = new JRetVal();
            jret.setError( -RC.EFAULT );
            return jret;
        }
        return ( JRetVal )
            oInst.ReadStreamNoWait( hChannel );
    }

    /* event called when the stream `hChannel' is
     * ready
     */
    @Override
    public int onStmReady( long hChannel )
    { return 0; }

    /* event called when the stream `hChannel' is
     * about to close
     */
    @Override
    public int onStmClosing( long hChannel ) 
    { return 0; }

    /* Convert the arguments in the `oParams' from c++
     * object to a list of java object.
     *
     * The return value is a two element list. The
     * first element is the error code and the second
     * element is a list of java object. The second
     * element should be None if error occurs during
     * the conversion.
     */

    JRetVal argObjToList( int seriProto, CParamList oParams ) 
    {
        JRetVal jret = new JRetVal();
        if( oParams == null )
        {
            jret.setError( -RC.EINVAL );
            return jret;
        }
        do{
            jret = ( JRetVal )oParams.GetSize();
            if( jret.ERROR() ||
                jret.getParamCount() == 0 )
                break;

            int iSize = jret.getAtInt( 0 );

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

                jret.clear();
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
                        oParams.GetBufPtr( i );
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
    @Override
    public JRetVal invokeMethod( ObjPtr callback,
        String ifName, String methodName,
        int seriProto, CParamList cppargs )
    {
        JRetVal oResp = new JRetVal();
        do{
            JRetVal jret;
            jret = argObjToList(
                seriProto, cppargs );
            if( jret.ERROR() )
            {
                oResp = jret;
                break;
            }

            Object[] argList =
                jret.getParamArray();

            String[] nameComps =
                methodName.split( "_" );

            if( isServer() )
            {
                if( nameComps[0].equals("RpcCall") )
                {
                    oResp.setError(-RC.ENOTSUP);
                    break;
                }
                if( !nameComps[ 0 ].equals("UserMethod") )
                {
                    oResp.setError( -RC.EINVAL );
                    break;
                }
            }
            else 
            {
                if( nameComps[0].equals("RpcEvt") )
                {
                    oResp.setError(-RC.ENOTSUP);
                    break;
                }
                if( !nameComps[ 0 ].equals("UserEvent" ))
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
            catch( IllegalArgumentException | ClassCastException e )
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

        if( !oResp.isPending() )
        {
            if( callback != null )
            {
                callback.Clear();
                callback = null;
            }
        }
        if( cppargs != null )
        {
            cppargs.Reset();
            cppargs = null;
        }

        return oResp;
    }

    /* Invoke a callback function depending
     * it is bound function or not
     */
    @Override
    public int invokeCallback( IAsyncRespCb callback,
        List< Object > listArgs )
    {
        int ret = 0;
        try{
            Object context = listArgs.get( 0 );
            int iRet = ( Integer )listArgs.get( 1 );

            listArgs.remove( 0 );
            listArgs.remove( 0 );

            callback.onAsyncResp(
                context, iRet, listArgs.toArray() );
        }
        catch( IllegalArgumentException | IndexOutOfBoundsException | ClassCastException e )
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
    @Override
    public int deferCall(
        IDeferredCall callback, Object[] args )
    {
        InstType oInst = getInst();
        if( oInst == null )
            return -RC.EFAULT;
        return oInst.DeferCall( callback, args );
    }

    @Override
    public void deferCallback(
        Object callback, Object listArgs )
    {
        IDeferredCall dc = ( IDeferredCall )callback;
        if( !Helpers.isArray( listArgs ) )
            return;
        Object[] arrArgs = ( Object[] )listArgs;
        dc.call( arrArgs );
    }

    @Override
    public int setChanCtx(
        long hChannel, Object pCtx)
    {
        InstType oInst = getInst();
        if( oInst == null )
            return -RC.EFAULT;
        return oInst.SetChanCtx(
            hChannel, pCtx );
    }

    @Override
    public int removeChanCtx(
        long hChannel )
    {
        InstType oInst = getInst();
        if( oInst == null )
            return -RC.EFAULT;
        return oInst.RemoveChanCtx( hChannel );
    }

    @Override
    public JRetVal getChanCtx( long hChannel )
    {
        InstType oInst = getInst();
        if( oInst == null )
        {
            JRetVal jret = new JRetVal();
            jret.setError( -RC.EFAULT );
            return jret;
        }
        return ( JRetVal )oInst.GetChanCtx(
            hChannel );
    }

    @Override
    public int onPostStop()
    {
        System.out.println(
            "Service is stopped" );
        return 0;
    }

    public JRetVal getProperty( int iProp )
    {
        InstType oInst = getInst();
        if( oInst == null )
        {
            JRetVal jret = new JRetVal();
            jret.setError( -RC.EFAULT );
            return jret;
        }
        return ( JRetVal )oInst.GetProperty( iProp );
    }

    public int setProperty( int iProp, Object oVal )
    {
        InstType oInst = getInst();
        if( oInst == null )
        {
            JRetVal jret = new JRetVal();
            return -RC.EFAULT;
        }
        Variant oVar = new Variant();
        if( oVal instanceof Byte )
        {
            Byte bval = ( Byte )oVal;
            oVar.SetByte( bval.byteValue() );
        }
        else if( oVal instanceof Integer )
        {
            Integer ival = ( Integer )oVal;
            oVar.SetInt( ival.intValue() );
        }
        else if( oVal instanceof Boolean )
        {
            oVar.SetByte( (byte)
                (( (Boolean)oVal ) ? 1 : 0) );
        }
        else if( oVal instanceof Short )
        {
            Short sval = ( Short )oVal;
            oVar.SetShort( sval.shortValue() );
        }
        else if( oVal instanceof Float )
        {
            Float fval = ( Float ) oVal;
            oVar.SetFloat( fval.floatValue() );
        }
        else if( oVal instanceof Double )
        {
            Double dval = ( Double )oVal;
            oVar.SetDouble( dval.doubleValue() );
        }
        else if( oVal instanceof String )
            oVar.SetString( ( String )oVal );
        else if( oVal instanceof byte[] )
            oVar.SetByteArray( ( byte[] )oVal );
        return oInst.SetProperty( iProp, oVar );
    }
}

