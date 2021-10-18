package org.rpcf.rpcbase;
import java.lang.String;
import java.util.HashMap;
import java.util.Map;
import java.util.List;


class JavaRpcProxy extends JavaRpcServiceP
{
    public JavaRpcProxy(
        ObjPtr pIoMgr,
        String strDesc,
        String strSvrObj )
    {
        JRetVal jret = new JRetVal();
        do{
            m_pIoMgr = pIoMgr;
            m_iError = 0;
            CParamList oParams = new CParamList();
            jret = ( JRetVal )oParams.GetCfg();
            if( jret.ERROR() )
                break;

            ObjPtr pCfg = ( ObjPtr )jret.getAt( 0 );
            jret = ( JRetVal )rpcbase.CreateProxy(
                pIoMgr, strDesc, strSvrObj,
                ( ObjPtr )oParams.GetCfg() );

            if( jret.ERROR() )
                break;

            ObjPtr obj = ( ObjPtr )jret.getAt( 0 );
            jret = ( JRetVal )
                rpcbase.CastToProxy( obj );

            if( jret.ERROR() )
                break;

            m_oInst = ( CJavaProxyImpl )
                jret.getAt( 0 );

        }while( false );

        if( jret.ERROR() )
        {
            throw new RuntimeException(
                "Error JavaRpcProxy ctor" +
                jret.getError() );
        }
        return;
    }

    JRetVal sendRequest(
        String strIfName,
        String strMethod,
        Object[] args )
    {
        return makeCall( strIfName, strMethod, args,
            rpcbaseConstants.seriNone );
    }
    /* sendRequestAsync returns a tuple with two
     * elements, 0 is the return code, and 1 is a
     * np.int64 as a taskid, which can be used to
     * cancel the request sent
     */

    JRetVal sendRequestAsync(
        IAsyncRespCb callback,
        String strIfName,
        String strMethod,
        Object[] args )
    {
        return makeCallAsync( callback,
            strIfName, strMethod, args,
            rpcbaseConstants.seriNone );
    }

    JRetVal makeCall(
        String strIfName,
        String strMethod,
        Object[] args,
        int seriProto )
    {
        return makeCallAsync( null,
            strIfName, strMethod,
            args, seriProto );
    }

    JRetVal makeCallAsync(
        IAsyncRespCb callback,
        String strIfName,
        String strMethod,
        Object[] args,
        int seriProto )
    {
        JRetVal jret = new JRetVal();
        int ret = 0;
        do{
            CParamList oParams = new CParamList();
            ret = oParams.SetStrProp(
                rpcbaseConstants.propIfName,
                strIfName );
            if( ret < 0 )
                break;

            ret = oParams.SetStrProp(
                rpcbaseConstants.propMethodName,
                strMethod );
            if( ret < 0 )
                break;

            ret = oParams.SetIntProp(
                rpcbaseConstants.propSeriProto,
                seriProto );
            if( ret < 0 )
                break;

            jret = ( JRetVal )oParams.GetCfg(); 
            if( jret.ERROR() )
                break;

            ObjPtr pParams = ( ObjPtr )jret.getAt( 0 );
            CfgPtr pCfg =
                rpcbase.CastToCfg( pParams );

            jret = makeCallWithOptAsync(
                callback, null,
                pCfg, args );

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
    JRetVal makeCallWithOpt(
        CfgPtr pCfg, Object[] args )
    {
        return makeCallWithOptAsync(
            null, null, pCfg, args );
    }

    JRetVal makeCallWithOptAsync(
        IAsyncRespCb callback,
        Object context,
        CfgPtr pCfg,
        Object[] args )
    {
        return ( JRetVal )m_oInst.JavaProxyCall2(
            ( Object )callback,
            context, pCfg,
            ( Object )args );
    }

    public boolean isServer()
    { return false; }

    public gint32 initMaps()
    { return rpcbaseConstants.ERROR_NOT_IMPL; }
}