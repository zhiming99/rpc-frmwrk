package org.rpcf.rpcbase;
import java.lang.String;
import java.util.HashMap;

public class JavaRpcContext 
{
    protected ObjPtr m_pIoMgr;
    protected String m_strModName;
    protected int m_iError = 0;

    public ObjPtr getIoMgr()
    { return m_pIoMgr; }

    public String getModName()
    { return m_strModName; }

    public void setError( int iErr )
    { m_iError = iErr; }

    public int getError()
    { return m_iError; }

    ObjPtr CreateIoMgr( String strModName )
    {
        ObjPtr pIoMgr = null;
        int ret = 0;
        do{
            ret = rpcbase.CoInitialize(0);
            if( RC.ERROR( ret ) )
                break;
            
            CParamList oCfg= new CParamList();
            ret = oCfg.PushStr( strModName );
            if( RC.ERROR( ret ) )
                break;

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

            pIoMgr=rpcbase.CreateObject(
                rpcbase.Clsid_CIoManager, pCfg );
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

    int StartIoMgr( ObjPtr pIoMgr ) 
    {
        int ret = 0;
        IService pSvc =
            rpcbase.CastToSvc( pIoMgr );

        if( pSvc == null )
        {
            ret = -RC.EFAULT;
            setError( ret );
            return ret;
        }

        ret = pSvc.Start();
        if( RC.ERROR( ret ) )
            setError( ret );

        return ret;
    }

    int StopIoMgr( ObjPtr pIoMgr )
    {
        int ret = 0;
        IService pSvc =
            rpcbase.CastToSvc( pIoMgr );

        if( pSvc == null )
        {
            ret = -RC.EFAULT;
            setError( ret );
            return ret;
        }

        ret = pSvc.Stop();
        if( RC.ERROR( ret ) )
            setError( ret );

        return ret;
    }

    int CleanUp()
    {
        return rpcbase.CoUninitialize();
    }

    int DestroyRpcCtx()
    {
        System.out.println( "rpc context is about to quit..." );
        return CleanUp();
    }

    public JavaRpcContext( String strModName )
    {
        m_strModName = strModName;
    }

    public int start()
    {
        int ret = 0;
        System.out.println( "entering..." );
        m_pIoMgr = CreateIoMgr( m_strModName );
        if( m_pIoMgr != null )
        {
            ret = StartIoMgr( m_pIoMgr );
            if( ret < 0 )
                StopIoMgr( m_pIoMgr );
            else
                ret = rpcbase.LoadJavaFactory(
                    m_pIoMgr );
        }
        else
        {
            ret = -RC.EFAULT;
        }
        if( RC.ERROR( ret ) )
            setError( ret );

        return ret;
    }

    public int stop()
    {
        System.out.println( "exiting..." );
        if( m_pIoMgr != null )
        {
            StopIoMgr( m_pIoMgr );
            m_pIoMgr = null;
        }
        return DestroyRpcCtx();
    }

    static JavaRpcContext create( boolean bServer )
    {
        JavaRpcContext o;
        if( bServer )
            o = new JavaRpcContext( "JavaRpcServer" );
        else
            o = new JavaRpcContext( "JavaRpcProxy" );

        int ret = o.start();
        if( RC.ERROR( ret ) )
            return null;

        return o;
    }

    public static JavaRpcContext createServer()
    { return create( true ); }

    public static JavaRpcContext createProxy()
    { return create( false ); }
}

