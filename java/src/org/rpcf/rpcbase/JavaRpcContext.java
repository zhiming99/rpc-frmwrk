package org.rpcf.rpcbase;
import java.lang.String;
import java.util.Map;
import java.util.HashMap;
import java.util.concurrent.TimeUnit;

public class JavaRpcContext 
{
    protected ObjPtr m_pIoMgr;
    protected Map< Integer, Object > m_oInitParams;
    protected int m_iError = 0;

    public ObjPtr getIoMgr()
    { return m_pIoMgr; }

    public String getModName()
    { return ( String )m_oInitParams.get( 0 ); }

    public void setError( int iErr )
    { m_iError = iErr; }

    public int getError()
    { return m_iError; }

    /**
     * @param oInit A map with multiple input parameters.
     * The valid key value is one of the following.
     *      <p>0 : a string of the module name, mandatory</p>
     *
     *      <p>101: role of the rpcrouter, only valid when
     *      builti-in router is enabled.</p>
     *
     *      <p>102: a boolean telling whether the authoration
     *      enabled or not, optional.</p>
     *
     *      <p>103: a boolean telling whether running as a
     *      daemon, optional.</p>
     *
     *      <p>104: a string of the application name, optional.</p>
     *
     *      <p>105: a string as the path of 'driver.json', which
     *      can override the default path of the driver.json.
     *      It is a must-have attribute when fastrpc or
     *      built-in router is enabled.</p>
     * @return an ObjPtr object holding the instance of IoMgr on success
     * and null on error, with m_iError set.
     */
    public ObjPtr StartIoMgr(Map<Integer, Object> oInit)
    {
        ObjPtr pIoMgr = null;
        int ret = 0;
        do{
            try{
                System.loadLibrary("rpcbaseJNI");
            }
            catch (UnsatisfiedLinkError e)
            {
                System.err.println(
                    "Native code library failed to load." +
                    " See the chapter on Dynamic Linking " +
                    "Problems in the SWIG Java " +
                    "documentation for help.\n" + e);
                System.exit(1);
            }

            ret = rpcbase.CoInitialize(0);
            if( RC.ERROR( ret ) )
                break;
            
            CParamList oCfg= new CParamList();
            String strModName =
                ( String )oInit.get( 0 );
            if( strModName == null )
            {
                ret = RC.ENOENT;
                break;
            }
            ret = oCfg.SetStrProp( 0, strModName );
            if( RC.ERROR( ret ) )
                break;

            String strCfgPath =
                ( String )oInit.get( 105 );

            if( strCfgPath != null &&
                strCfgPath.length() > 0 )
            {
                oCfg.SetStrProp(
                    rpcbaseConstants.propConfigPath,
                    strCfgPath );

                Integer iVal =
                    ( Integer )oInit.get( 101 );
                if( iVal != null )
                    oCfg.SetIntProp( 101, iVal );
                Boolean bVal =
                    ( Boolean )oInit.get( 102 );
                if( bVal != null )
                    oCfg.SetBoolProp( 102, bVal );
                bVal =
                    ( Boolean )oInit.get ( 103 );
                if( bVal != null )
                    oCfg.SetBoolProp( 103, bVal );

                String strVal =
                    ( String )oInit.get ( 104 );
                if( strVal != null )
                    oCfg.SetStrProp( 104, strVal );
            }

            JRetVal jret =
                ( JRetVal )oCfg.GetCfg();

            if( jret.ERROR() )
            {
                ret = jret.getError();
                break;
            }

            ObjPtr pObj =
                ( ObjPtr )jret.getAt( 0 );

            CfgPtr pCfg =
                rpcbase.CastToCfg( pObj );

            pIoMgr=rpcbase.StartIoMgr( pCfg );
            if( pIoMgr == null )
            {
                ret = -RC.EFAULT;
                break;
            }

        }while( false );

        if( RC.ERROR( ret ) ) 
            setError( ret );

        return pIoMgr;
    }

    public int StopIoMgr(ObjPtr pIoMgr)
    {
        return rpcbase.StopIoMgr( pIoMgr );
    }

    int CleanUp()
    {
        // tell the garbage collector to release
        // the 'proxy' objects.
        System.gc();
        try{
            TimeUnit.SECONDS.sleep(2);
        }
        catch( InterruptedException e )
        {};
        return rpcbase.CoUninitializeEx();
    }

    int DestroyRpcCtx()
    {
        System.out.println( "rpc context is about to quit..." );
        return CleanUp();
    }

    public JavaRpcContext( Map< Integer, Object > oInitParams )
    {
        m_oInitParams = oInitParams;
    }

    public int start()
    {
        int ret = 0;
        System.out.println( "entering..." );
        m_pIoMgr = StartIoMgr( m_oInitParams );
        if( m_pIoMgr == null )
        {
            ret = -RC.EFAULT;
            setError( ret );
        }
        return ret;
    }

    public int stop()
    {
        System.out.println( "exiting..." );
        if( m_pIoMgr != null )
        {
            StopIoMgr( m_pIoMgr );
            m_pIoMgr.Clear();
        }
        return DestroyRpcCtx();
    }

    static JavaRpcContext create(
        boolean bServer, Map<Integer, Object> oInitParams )
    {
        JavaRpcContext o;
        o = new JavaRpcContext( oInitParams );

        int ret = o.start();
        if( RC.ERROR( ret ) )
            return null;

        return o;
    }

    public static JavaRpcContext createServer()
    {
        Map<Integer, Object> oInitParams =
            new HashMap< Integer, Object >();
        
        oInitParams.put( 0, "JavaRpcServer" );
        return create( true, oInitParams );
    }

    public static JavaRpcContext createServer(
        Map<Integer, Object> oInitParams )
    {
        return create( true, oInitParams );
    }

    public static JavaRpcContext createProxy()
    {
        Map<Integer, Object> oInitParams =
            new HashMap< Integer, Object >();
        oInitParams.put( 0, "JavaRpcProxy" );
        return create( false, oInitParams );
    }

    public static JavaRpcContext createProxy(
        Map<Integer, Object> oInitParams )
    { return create( false,  oInitParams ); }

}

