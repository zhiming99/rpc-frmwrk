package org.rpcf.rpcbase;
import java.lang.String;
import java.util.HashMap;

public class JavaRpcContext 
{
    protected Object m_pIoMgr;
    protected String m_strModName;

    Object CreateIoMgr( String strModName )
    {
        Object pIoMgr = null;
        do{
            int iRet = rpcbase.CoInitialize(0);
            
            CParamList oCfg=rpcbase.CParamList();
            ret = oCfg.PushStr( strModName );
            if( ret < 0 )
                break;
            CfgPtr pCfg =
                rpcbase.CastToCfg( oCfg.GetCfg() );

            pIoMgr=rpcbase.CreateObject(
                rpcbase.Clsid_CIoManager, pCfg );
            if( pIoMgr == null )
            {
                ret = -1;
                break;
            }

        }while( 0 );

        if( ERROR( ret ) ) 
            return null;

        return pIoMgr;
    }

    int StartIoMgr( Object pIoMgr ) 
    {
        IService pSvc =
            rpcbase.CastToSvc( pIoMgr );

        if( pSvc == null )
            return -1;

        ret = pSvc.Start();
        return ret;
    }

    int StopIoMgr( Object pIoMgr );
    {
        IService pSvc =
            rpcbase.CastToSvc( pIoMgr );

        if( pSvc == null )
            return -1;

        ret = pSvc.Stop();
        return ret;
    }

    int CleanUp()
    {
        return rpcbase.CoUninitialize();
    }

    int DestroyRpcCtx() :
    {
        System.out.println( "rpc context is about to quit..." );
        return CleanUp();
    }

    JavaRpcContext( String strModName = "JavaRpcProxy" )
    {
        m_strModName = strModName;
    }

    int Start()
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
                ret = LoadJavaFactory( m_pIoMgr );
        }
        else
        {
            ret = -1;
        }
        return ret;
    }

    int Stop()
    {
        System.out.println( "exiting..." );
        if( m_pIoMgr != null )
        {
            StopIoMgr( m_pIoMgr );
            m_pIoMgr = null;
        }
        return DestroyRpcCtx();
    }
}

