/*
 * =====================================================================================
 *
 *       Filename:  fastrpc.i
 *
 *    Description:  the swig file as the wrapper of the fastrpc classes and
 *                  helper classes for Java
 *
 *        Version:  1.0
 *        Created:  06/05/2023 14:02:30
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2021 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */ 

%header{

enum EnumMyClsid
{
    DECL_CLSID( CSwigRosRpcSvc_CliSkel ) = 0xC62DFC66,
    DECL_CLSID( CSwigRosRpcSvc_SvrSkel ),
    DECL_CLSID( CSwigRosRpcSvc_ChannelCli ),
    DECL_CLSID( CSwigRosRpcSvc_ChannelSvr ),

    DECL_IID( ISwigRosRpcSvc ),

    DECL_CLSID( CJavaRpcSvc_CliImpl ) = Clsid_Invalid,
    DECL_CLSID( CJavaRpcSvc_SvrImpl ) = Clsid_Invalid,
    DECL_CLSID( CJavaRpcSvc_CliBase ) = Clsid_Invalid,
    DECL_CLSID( CJavaRpcSvc_SvrBase ) = Clsid_Invalid,

};

class ISwigRosRpcSvc_PImpl
    : public virtual CFastRpcSkelProxyBase
{
    public:
    typedef CFastRpcSkelProxyBase super;
    ISwigRosRpcSvc_PImpl( const IConfigDb* pCfg ) :
        super( pCfg )
        {}
    gint32 InitUserFuncs();
    
    const EnumClsid GetIid() const override
    { return iid( ISwigRosRpcSvc ); }

    gint32 InvokeUserMethod(
        IConfigDb* pParams,
        IEventSink* pCallback ) override
    {
        CRpcServices* pSvc = this->GetStreamIf();
        return pSvc->InvokeUserMethod(
            pParams, pCallback );
    }
};

class ISwigRosRpcSvc_CliApi
    : public virtual CAggInterfaceProxy
{
    public:
    typedef CAggInterfaceProxy super;
    ISwigRosRpcSvc_CliApi( const IConfigDb* pCfg ) :
        super( pCfg )
        {}
    inline ISwigRosRpcSvc_PImpl* GetSkelPtr()
    {
        auto pCli = dynamic_cast
            < CFastRpcProxyBase* >( this );
        if( pCli == nullptr )
            return nullptr;
        InterfPtr pIf = pCli->GetStmSkel();
        if( pIf.IsEmpty() )
            return nullptr;
        auto pSkel = dynamic_cast
            <ISwigRosRpcSvc_PImpl*>(
                ( CRpcServices* )pIf );
        return pSkel;
    }
};

class ISwigRosRpcSvc_SImpl
    : public virtual CFastRpcSkelSvrBase
{
    public:
    typedef CFastRpcSkelSvrBase super;
    ISwigRosRpcSvc_SImpl( const IConfigDb* pCfg ) :
        super( pCfg )
        {}
    gint32 InitUserFuncs();
    const EnumClsid GetIid() const override
    { return iid( ISwigRosRpcSvc ); }

    gint32 InvokeUserMethod(
        IConfigDb* pParams,
        IEventSink* pCallback ) override
    {
        CRpcServices* pSvc = this->GetStreamIf();
        return pSvc->InvokeUserMethod(
            pParams, pCallback );
    }
};

class ISwigRosRpcSvc_SvrApi
    : public virtual CAggInterfaceServer
{
    public:
    typedef CAggInterfaceServer super;
    ISwigRosRpcSvc_SvrApi( const IConfigDb* pCfg ) :
        super( pCfg )
        {}
    inline ISwigRosRpcSvc_SImpl* GetSkelPtr( HANDLE hstm )
    {
        auto pSvr = dynamic_cast< CFastRpcServerBase* >( this );
        if( pSvr == nullptr )
            return nullptr;
        InterfPtr pIf;
        gint32 ret = pSvr->GetStmSkel(
            hstm, pIf );
        if( ERROR( ret ) )
            return nullptr;
        auto pSkel = dynamic_cast<ISwigRosRpcSvc_SImpl*>(( CRpcServices* )pIf );
        return pSkel;
    }

};

DECLARE_AGGREGATED_SKEL_PROXY(
    CSwigRosRpcSvc_CliSkel,
    ISwigRosRpcSvc_PImpl );

DECLARE_AGGREGATED_SKEL_SERVER(
    CSwigRosRpcSvc_SvrSkel,
    ISwigRosRpcSvc_SImpl );


class CSwigRosRpcSvc_ChannelCli
    : public CRpcStreamChannelCli
{
    public:
    typedef CRpcStreamChannelCli super;
    CSwigRosRpcSvc_ChannelCli(
        const IConfigDb* pCfg ) :
        super::virtbase( pCfg ), super( pCfg )
    { SetClassId( clsid(
        CSwigRosRpcSvc_ChannelCli ) ); }
};

class CSwigRosRpcSvc_ChannelSvr
    : public CRpcStreamChannelSvr
{
    public:
    typedef CRpcStreamChannelSvr super;
    CSwigRosRpcSvc_ChannelSvr(
        const IConfigDb* pCfg ) :
        super::virtbase( pCfg ), super( pCfg )
    { SetClassId( clsid(
        CSwigRosRpcSvc_ChannelSvr ) ); }
};

DECLARE_AGGREGATED_PROXY(
    CJavaRpcSvc_CliBase,
    CStatCountersProxy,
    CJavaProxy,
    ISwigRosRpcSvc_CliApi,
    CFastRpcProxyBase );

class CJavaRpcSvc_CliImpl
    : public CJavaRpcSvc_CliBase
{
    public:
    typedef CJavaRpcSvc_CliBase super;
    CJavaRpcSvc_CliImpl( const IConfigDb* pCfg ) :
        super::virtbase( pCfg ), super( pCfg )
    { SetClassId( clsid(CJavaRpcSvc_CliImpl ) ); }

    /* The following 2 methods are important for */
    /* streaming transfer. rewrite them if necessary */
    gint32 OnStreamReady( HANDLE hChannel ) override
    { return CJavaProxy::OnStreamReady( hChannel ); } 
    
    gint32 OnStmClosing( HANDLE hChannel ) override
    { return CJavaProxy::OnStmClosing( hChannel ); }
    
    gint32 CreateStmSkel(
        InterfPtr& pIf ) override;
    
    gint32 OnPreStart(
        IEventSink* pCallback ) override;

    gint32 AsyncCallVector(
        IEventSink* pTask,
        CfgPtr& pOptions,
        CfgPtr& pResp,
        const std::string& strcMethod,
        std::vector< Variant >& vecParams ) override;

    gint32 InvokeUserMethod(
        IConfigDb* pParams,
        IEventSink* pCallback ) override;
};

DECLARE_AGGREGATED_SERVER(
    CJavaRpcSvc_SvrBase,
    CStatCountersServer,
    CJavaServer,
    ISwigRosRpcSvc_SvrApi,
    CFastRpcServerBase );

class CJavaRpcSvc_SvrImpl
    : public CJavaRpcSvc_SvrBase
{
    public:
    typedef CJavaRpcSvc_SvrBase super;
    CJavaRpcSvc_SvrImpl( const IConfigDb* pCfg ) :
        super::virtbase( pCfg ), super( pCfg )
    { SetClassId( clsid( CJavaRpcSvc_SvrImpl ) ); }

    /* The following 3 methods are important for */
    /* streaming transfer. rewrite them if necessary */
    gint32 OnStreamReady( HANDLE hChannel ) override
    { return CJavaServer::OnStreamReady( hChannel ); } 
    
    gint32 OnStmClosing( HANDLE hChannel ) override
    { return CJavaServer::OnStmClosing( hChannel ); }
    
    gint32 AcceptNewStream(
        IEventSink* pCb, IConfigDb* pDataDesc ) override
    { return CJavaServer::AcceptNewStream( pCb, pDataDesc ); }
    
    gint32 CreateStmSkel(
        HANDLE, guint32, InterfPtr& ) override;
    
    gint32 OnPreStart(
        IEventSink* pCallback ) override;

    gint32 InvokeUserMethod(
        IConfigDb* pParams,
        IEventSink* pCallback ) override;

    gint32 SendEvent(
        JNIEnv *jenv,
        jobject pCallback,
        const std::string& strCIfName,
        const std::string& strMethod,
        const std::string& strDest,
        jobject pArgs,
        guint32 dwSeriProto ) override;
};

gint32 ISwigRosRpcSvc_PImpl::InitUserFuncs()
{
    BEGIN_IFPROXY_MAP( ISwigRosRpcSvc, false );
    END_IFPROXY_MAP;
    return STATUS_SUCCESS;
}

gint32 ISwigRosRpcSvc_SImpl::InitUserFuncs()
{
    BEGIN_IFHANDLER_MAP( ISwigRosRpcSvc );
    END_IFHANDLER_MAP;
    return STATUS_SUCCESS;
}

gint32 CJavaRpcSvc_CliImpl::CreateStmSkel(
    InterfPtr& pIf )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg;
        auto pMgr = this->GetIoMgr();
        oCfg[ propIoMgr ] = ObjPtr( pMgr );
        oCfg[ propIsServer ] = false;
        oCfg.SetPointer( propParentPtr, this );
        oCfg.CopyProp( propSkelCtx, this );

        stdstr strCfg;
        CCfgOpenerObj oIfCfg( this ); 
        ret = oIfCfg.GetStrProp(
            propObjDescPath, strCfg );
        if( ERROR( ret ) )
            break;

        stdstr strSvcName;
        ret = oIfCfg.GetStrProp(
            propObjName, strSvcName );
        if( ERROR( ret ) )
            break;

        ret = CRpcServices::LoadObjDesc(
            strCfg, strSvcName + "_SvrSkel",
            false, oCfg.GetCfg() );
        if( ERROR( ret ) )
            break;
        ret = pIf.NewObj(
            clsid( CSwigRosRpcSvc_CliSkel ),
            oCfg.GetCfg() );
    }while( 0 );
    return ret;
}

gint32 CJavaRpcSvc_CliImpl::OnPreStart(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCtx;
        CCfgOpenerObj oIfCfg( this );
        oCtx[ propClsid ] = clsid( 
            CSwigRosRpcSvc_ChannelCli );
        oCtx.CopyProp( propObjDescPath, this );

        stdstr strSvcName;
        ret = oIfCfg.GetStrProp(
            propObjName, strSvcName );
        if( ERROR( ret ) )
            break;

        stdstr strInstName = strSvcName;
        oCtx[ 1 ] = strInstName;
        guint32 dwHash = 0;
        ret = GenStrHash( strInstName, dwHash );
        if( ERROR( ret ) )
            break;
        char szBuf[ 16 ];
        sprintf( szBuf, "_%08X", dwHash );
        strInstName = strSvcName + "_ChannelSvr";
        oCtx[ 0 ] = strInstName;
        strInstName += szBuf;
        oCtx[ propObjInstName ] = strInstName;
        oCtx[ propIsServer ] = false;
        oIfCfg.SetPointer( propSkelCtx,
            ( IConfigDb* )oCtx.GetCfg() );
        ret = super::OnPreStart( pCallback );
    }while( 0 );
    return ret;
}

gint32 CJavaRpcSvc_CliImpl::AsyncCallVector(
    IEventSink* pTask,
    CfgPtr& pOptions,
    CfgPtr& pResp,
    const std::string& strcMethod,
    std::vector< Variant >& vecParams )
{
    gint32 ret = 0;

    do{ 
        if( pTask == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        guint64 qwIoTaskId = 0; 
        CCfgOpenerObj oCfg( pTask );
        if( pResp.IsEmpty() )
            pResp.NewObj();

        CopyUserOptions( pTask, pOptions );

        std::string strMethod( strcMethod );
        bool bNoReply = false;
        if( !pOptions.IsEmpty() )
        {
            bool bSysMethod = false;
            CCfgOpener oOptions(
                ( IConfigDb* )pOptions );

            ret = oOptions.GetBoolProp(
                propSysMethod, bSysMethod );

            if( SUCCEEDED( ret ) && bSysMethod )
                strMethod = SYS_METHOD( strMethod );
            else
                strMethod = USER_METHOD( strMethod );

            ret = oOptions.GetBoolProp(
                propNoReply, bNoReply );
            if( ERROR( ret ) )
            {
                bNoReply = false;
                ret = 0;
            }
        }
        InterfPtr pIf = this->GetStmSkel();
        CInterfaceProxy* pSkel = pIf;
        if( pSkel == nullptr )
        {
            ret = -EFAULT;
            DebugPrintEx( logErr, ret,
                "Error, the underlying StreamSkel is gone" );
            break;
        }
        ret = pSkel->SendProxyReq( pTask, false,
             strMethod, vecParams, qwIoTaskId ); 

        if( SUCCEEDED( ret ) && !bNoReply )
            ret = STATUS_PENDING;

        if( ret == STATUS_PENDING ) 
        {
            // for canceling purpose
            CParamList oResp( ( IConfigDb* )pResp );
            oResp[ propTaskId ] = qwIoTaskId;
            break;
        }

        if( ERROR( ret ) ) 
            break; 

    }while( 0 );

    return ret;
}

gint32 CJavaRpcSvc_CliImpl::InvokeUserMethod(
        IConfigDb* pParams,
        IEventSink* pCallback )
{
    return CJavaProxy::InvokeUserMethod(
        pParams, pCallback );
}

gint32 CJavaRpcSvc_SvrImpl::SendEvent(
    JNIEnv *jenv,
    jobject pCallback,
    const std::string& strCIfName,
    const std::string& strMethod,
    const std::string& strDest,
    jobject pArgs,
    guint32 dwSeriProto )
{
    guint32 ret = 0;
    TaskletPtr pTask;
    jobject pjCb = nullptr;
    jobject pjret = nullptr;
    do{
        if( strMethod.empty() ||
            strCIfName.empty() )
        {
            ret = -EINVAL;
            break;
        }

        std::vector< Variant > vecArgs;
        ret = List2Vector( jenv, pArgs, vecArgs );
        if( ERROR( ret ) )
            break;

        IEventSink* pCb = nullptr;
        if( pCallback == nullptr )
        {
            pTask.NewObj( clsid( CIfDummyTask ) );
        }
        else
        {
            CParamList oReqCtx;
            ret = NEW_PROXY_RESP_HANDLER2(
                pTask, ObjPtr( this ),
                &CJavaServer::OnAsyncCallResp,
                nullptr, oReqCtx.GetCfg() );

            if( ERROR( ret ) )
                break;

            pjCb = jenv->NewGlobalRef( pCallback );
            jobject jret = NewJRet( jenv );
            pjret = jenv->NewGlobalRef( jret );

            oReqCtx.Push( ( intptr_t ) pjCb );
            oReqCtx.Push( ( intptr_t )pjret );
            oReqCtx.Push( strMethod );
            oReqCtx.Push( seriNone );
        }
        pCb = pTask;

        std::string strIfName =
           DBUS_IF_NAME( strCIfName );

        std::vector< InterfPtr > vecIfs;
        ret = this->EnumStmSkels( vecIfs );
        if( SUCCEEDED( ret ) )
        {
            CRpcServices* pSkel = vecIfs[ 0 ];
            bool bHasMatch = true;
            MatchPtr pIfMatch;

            CReqBuilder oReq( pSkel );
            oReq.SetIntProp(
                propSeriProto, dwSeriProto );

            pSkel->FindMatch(
                strIfName, pIfMatch );

            if( !pIfMatch.IsEmpty() )
                oReq.SetIfName( strIfName );
            else
                bHasMatch = false;

            if( bHasMatch )
            {
                if( !strDest.empty() )
                    oReq.SetDestination( strDest );
                
                // we do not expect a response
                oReq.SetCallFlags( 
                   DBUS_MESSAGE_TYPE_SIGNAL
                   | CF_ASYNC_CALL );

                oReq.SetMethodName( strMethod );

                for( auto elem : vecArgs )
                    oReq.Push<BufPtr&>( elem );

                ret = BroadcastEvent(
                    oReq.GetCfg(), pCb );
            }
        }
        
    }while( 0 );

    if( ret != STATUS_PENDING )
    {
        if( !pTask.IsEmpty() )
            ( *pTask )( eventCancelTask );

        if( pjCb != nullptr )
            jenv->DeleteGlobalRef( pjCb );
        if( pjret != nullptr )
            jenv->DeleteGlobalRef( pjret );
    }

    return ret;
}

gint32 CJavaRpcSvc_SvrImpl::CreateStmSkel(
    HANDLE hStream,
    guint32 dwPortId,
    InterfPtr& pIf )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg;
        auto pMgr = this->GetIoMgr();
        oCfg[ propIoMgr ] = ObjPtr( pMgr );
        oCfg[ propIsServer ] = true;
        oCfg.SetIntPtr( propStmHandle,
            ( guint32*& )hStream );
        oCfg.SetPointer( propParentPtr, this );

        stdstr strCfg;
        CCfgOpenerObj oIfCfg( this ); 
        ret = oIfCfg.GetStrProp(
            propObjDescPath, strCfg );
        if( ERROR( ret ) )
            break;

        stdstr strSvcName;
        ret = oIfCfg.GetStrProp(
            propObjName, strSvcName );
        if( ERROR( ret ) )
            break;

        ret = CRpcServices::LoadObjDesc(
            strCfg, strSvcName + "_SvrSkel",
            true, oCfg.GetCfg() );
        if( ERROR( ret ) )
            break;
        oCfg.CopyProp( propSkelCtx, this );
        oCfg[ propPortId ] = dwPortId;
        ret = pIf.NewObj(
            clsid( CSwigRosRpcSvc_SvrSkel ),
            oCfg.GetCfg() );
    }while( 0 );
    return ret;
}

gint32 CJavaRpcSvc_SvrImpl::OnPreStart(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCtx;
        CCfgOpenerObj oIfCfg( this );
        oCtx[ propClsid ] = clsid( 
            CSwigRosRpcSvc_ChannelSvr );
        oCtx.CopyProp( propObjDescPath, this );

        stdstr strSvcName;
        ret = oIfCfg.GetStrProp(
            propObjName, strSvcName );
        if( ERROR( ret ) )
            break;
        stdstr strInstName = strSvcName;
        oCtx[ 1 ] = strInstName;
        guint32 dwHash = 0;
        ret = GenStrHash( strInstName, dwHash );
        if( ERROR( ret ) )
            break;
        char szBuf[ 16 ];
        sprintf( szBuf, "_%08X", dwHash );
        strInstName = strSvcName + "_ChannelSvr";
        oCtx[ 0 ] = strInstName;
        strInstName += szBuf;
        oCtx[ propObjInstName ] = strInstName;
        oCtx[ propIsServer ] = true;
        oIfCfg.SetPointer( propSkelCtx,
            ( IConfigDb* )oCtx.GetCfg() );
        ret = super::OnPreStart( pCallback );
    }while( 0 );
    return ret;
}

gint32 CJavaRpcSvc_SvrImpl::InvokeUserMethod(
        IConfigDb* pParams,
        IEventSink* pCallback )
{
    return CJavaServer::InvokeUserMethod(
        pParams, pCallback );
}

}
