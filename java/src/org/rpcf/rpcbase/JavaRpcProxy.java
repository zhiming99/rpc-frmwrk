package org.rpcf.rpcbase;


abstract public class JavaRpcProxy extends JavaRpcServiceP
{
    public JavaRpcProxy(
        ObjPtr pIoMgr,
        String strDesc,
        String strSvrObj )
    {
        JRetVal jret;
        do{
            m_pIoMgr = pIoMgr;
            CParamList oParams = new CParamList();
            jret = ( JRetVal )oParams.GetCfg();
            if( jret.ERROR() )
                break;

            ObjPtr pCfg = ( ObjPtr )jret.getAt( 0 );
            jret = ( JRetVal )rpcbase.CreateProxy(
                pIoMgr, strDesc, strSvrObj, pCfg );

            if( jret.ERROR() )
                break;

            ObjPtr obj = ( ObjPtr )jret.getAt( 0 );
            jret = ( JRetVal )
                rpcbase.CastToProxy( obj );

            if( jret.ERROR() )
                break;

            setInst( ( CJavaProxy )
                jret.getAt( 0 ) );
            obj.Clear();

        }while( false );

        if( jret.ERROR() )
            setError( jret.getError() );
    }

    public JRetVal sendRequest(
        String strIfName,
        String strMethod,
        Object[] args )
    {
        return makeCall( strIfName, strMethod,
            args, RC.seriNone );
    }
    /* sendRequestAsync returns a tuple with two
     * elements, 0 is the return code, and 1 is a
     * np.int64 as a taskid, which can be used to
     * cancel the request sent
     */

    public JRetVal sendRequestAsync(
        IAsyncRespCb callback,
        String strIfName,
        String strMethod,
        Object[] args )
    {
        return makeCallAsync( callback,
            strIfName, strMethod, args,
            RC.seriNone );
    }

    public JRetVal makeCall(
        String strIfName,
        String strMethod,
        Object[] args,
        int seriProto )
    {
        return makeCallAsync( null,
            strIfName, strMethod,
            args, seriProto );
    }

    public JRetVal makeCallAsync(
        IAsyncRespCb callback,
        String strIfName,
        String strMethod,
        Object[] args,
        int seriProto )
    {
        JRetVal jret = new JRetVal();
        int ret;
        do{
            CParamList oParams = new CParamList();
            ret = oParams.SetStrProp(
                RC.propIfName, strIfName );
            if( ret < 0 )
                break;

            ret = oParams.SetStrProp(
                RC.propMethodName, strMethod );
            if( ret < 0 )
                break;

            ret = oParams.SetIntProp(
                RC.propSeriProto, seriProto );
            if( ret < 0 )
                break;

            jret = ( JRetVal )oParams.GetCfg(); 
            if( jret.ERROR() )
                break;

            ObjPtr pParams = ( ObjPtr )jret.getAt( 0 );
            CfgPtr pCfg =
                rpcbase.CastToCfg( pParams );

            jret = makeCallWithOptAsync(
                callback, null, pCfg, args );

        }while( false );

        if( ret < 0 )
            jret.setError( ret );

        return jret;
    }

    /* MakeCallWithOpt: similiar to MakeCall, but with
        more parameters in pCfg. It is a synchronous call.
        the parameters in pCfg includes:
        [ propIfName ]: string, interface name
        [ propMethodName ]: string, method name
        [ propSeriProto ]: guint32, serialization protocol
        [ propNoReply ]: bool, one-way request (optional)
        [ propTimeoutSec ]: guint32, timeout in second
            specific to this request (optional)
        [ propKeepAliveSec ]: guint32, timeout in second
            for keep-alive heartbeat (optional)

        the return value is a list includes:
        [ 0 ] : error code
        [ 1 ] : depending on the element[ 0 ]
            if [ 0 ] is STATUS_PENDING
                [ 1 ] is the taskid for canceling
            if [ 0 ] is STATUS_SUCCESS and propNoReply is false,
                [ 1 ] is a list with a bytearray as the
                only element
            if [ 0 ] is ERROR or propNoReply is true
                [ 1 ] is None
    */
    public JRetVal makeCallWithOpt(
        CfgPtr pCfg, Object[] args )
    {
        return makeCallWithOptAsync(
            null, null, pCfg, args );
    }

    public JRetVal makeCallWithOptAsync(
        IAsyncRespCb callback,
        Object context,
        CfgPtr pCfg,
        Object[] args )
    {
        if( getInst() == null )
        {
            JRetVal jret = new JRetVal();
            jret.setError( -RC.EFAULT );
            return jret;
        }
        return ( JRetVal )getInst().JavaProxyCall2(
            ( Object )callback,
            context, pCfg,
            ( Object )args );
    }

}
