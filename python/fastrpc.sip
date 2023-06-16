/*
 * =====================================================================================
 *
 *       Filename:  fastrpc.sip
 *
 *    Description:  a sip file as the wrapper of fastrpc related classes for python
 *
 *        Version:  1.0
 *        Created:  05/23/2023 11:44:51 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2023 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */ 
%ModuleHeaderCode

#include "stmport.h"
#include "fastrpc.h"

enum EnumMyClsid
{
    DECL_CLSID( CPythonRpcSvc_CliSkel ) = 0x5095AA9D,
    DECL_CLSID( CPythonRpcSvc_SvrSkel ),
    DECL_CLSID( CPythonRpcSvc_ChannelCli ),
    DECL_CLSID( CPythonRpcSvc_ChannelSvr ),

    DECL_IID( IPythonRpcSvc ),
};

class IPythonRpcSvc_PImpl
    : public virtual CFastRpcSkelProxyBase
{
    public:
    typedef CFastRpcSkelProxyBase super;
    IPythonRpcSvc_PImpl( const IConfigDb* pCfg ) :
        super( pCfg )
        {}
    gint32 InitUserFuncs();
    
    const EnumClsid GetIid() const override
    { return iid( IPythonRpcSvc ); }

    gint32 InvokeUserMethod(
        IConfigDb* pParams,
        IEventSink* pCallback ) override
    {
        CRpcServices* pSvc = this->GetStreamIf();
        return pSvc->InvokeUserMethod(
            pParams, pCallback );
    }
};

class IPythonRpcSvc_CliApi
    : public virtual CAggInterfaceProxy
{
    public:
    typedef CAggInterfaceProxy super;
    IPythonRpcSvc_CliApi( const IConfigDb* pCfg ) :
        super( pCfg )
        {}
    inline IPythonRpcSvc_PImpl* GetSkelPtr()
    {
        auto pCli = dynamic_cast< CFastRpcProxyBase* >( this );
        if( pCli == nullptr )
            return nullptr;
        InterfPtr pIf = pCli->GetStmSkel();
        if( pIf.IsEmpty() )
            return nullptr;
        auto pSkel = dynamic_cast<IPythonRpcSvc_PImpl*>(( CRpcServices* )pIf );
        return pSkel;
    }
};

class IPythonRpcSvc_SImpl
    : public virtual CFastRpcSkelSvrBase
{
    public:
    typedef CFastRpcSkelSvrBase super;
    IPythonRpcSvc_SImpl( const IConfigDb* pCfg ) :
        super( pCfg )
        {}
    gint32 InitUserFuncs();
    const EnumClsid GetIid() const override
    { return iid( IPythonRpcSvc ); }

    gint32 InvokeUserMethod(
        IConfigDb* pParams,
        IEventSink* pCallback ) override
    {
        CRpcServices* pSvc = this->GetStreamIf();
        return pSvc->InvokeUserMethod(
            pParams, pCallback );
    }
};

class IPythonRpcSvc_SvrApi
    : public virtual CAggInterfaceServer
{
    public:
    typedef CAggInterfaceServer super;
    IPythonRpcSvc_SvrApi( const IConfigDb* pCfg ) :
        super( pCfg )
        {}
    inline IPythonRpcSvc_SImpl* GetSkelPtr( HANDLE hstm )
    {
        auto pSvr = dynamic_cast< CFastRpcServerBase* >( this );
        if( pSvr == nullptr )
            return nullptr;
        InterfPtr pIf;
        gint32 ret = pSvr->GetStmSkel(
            hstm, pIf );
        if( ERROR( ret ) )
            return nullptr;
        auto pSkel = dynamic_cast<IPythonRpcSvc_SImpl*>(( CRpcServices* )pIf );
        return pSkel;
    }

};

DECLARE_AGGREGATED_SKEL_PROXY(
    CPythonRpcSvc_CliSkel,
    IPythonRpcSvc_PImpl );

DECLARE_AGGREGATED_SKEL_SERVER(
    CPythonRpcSvc_SvrSkel,
    IPythonRpcSvc_SImpl );


class CPythonRpcSvc_ChannelCli
    : public CRpcStreamChannelCli
{
    public:
    typedef CRpcStreamChannelCli super;
    CPythonRpcSvc_ChannelCli(
        const IConfigDb* pCfg ) :
        super::virtbase( pCfg ), super( pCfg )
    { SetClassId( clsid(
        CPythonRpcSvc_ChannelCli ) ); }
};

class CPythonRpcSvc_ChannelSvr
    : public CRpcStreamChannelSvr
{
    public:
    typedef CRpcStreamChannelSvr super;
    CPythonRpcSvc_ChannelSvr(
        const IConfigDb* pCfg ) :
        super::virtbase( pCfg ), super( pCfg )
    { SetClassId( clsid(
        CPythonRpcSvc_ChannelSvr ) ); }
};

#define Clsid_CPythonRpcSvc_CliBase    Clsid_Invalid

DECLARE_AGGREGATED_PROXY(
    CPythonRpcSvc_CliBase,
    CStatCountersProxy,
    CPythonProxy,
    IPythonRpcSvc_CliApi,
    CFastRpcProxyBase );

class CPythonProxyRosImpl
    : public CPythonRpcSvc_CliBase
{
    public:
    typedef CPythonRpcSvc_CliBase super;
    CPythonProxyRosImpl( const IConfigDb* pCfg ) :
        super::virtbase( pCfg ), super( pCfg )
    { SetClassId( clsid(CPythonProxyRosImpl ) ); }

    /* The following 2 methods are important for */
    /* streaming transfer. rewrite them if necessary */
    gint32 OnStreamReady( HANDLE hChannel ) override
    { return CPythonProxy::OnStreamReady( hChannel ); } 
    
    gint32 OnStmClosing( HANDLE hChannel ) override
    { return CPythonProxy::OnStmClosing( hChannel ); }
    
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

#define Clsid_CPythonRpcSvc_SvrBase    Clsid_Invalid

DECLARE_AGGREGATED_SERVER(
    CPythonRpcSvc_SvrBase,
    CStatCountersServer,
    CPythonServer,
    IPythonRpcSvc_SvrApi,
    CFastRpcServerBase );

class CPythonServerRosImpl
    : public CPythonRpcSvc_SvrBase
{
    public:
    typedef CPythonRpcSvc_SvrBase super;
    CPythonServerRosImpl( const IConfigDb* pCfg ) :
        super::virtbase( pCfg ), super( pCfg )
    { SetClassId( clsid( CPythonServerRosImpl ) ); }

    /* The following 3 methods are important for */
    /* streaming transfer. rewrite them if necessary */
    gint32 OnStreamReady( HANDLE hChannel ) override
    { return CPythonServer::OnStreamReady( hChannel ); } 
    
    gint32 OnStmClosing( HANDLE hChannel ) override
    { return CPythonServer::OnStmClosing( hChannel ); }
    
    gint32 AcceptNewStream(
        IEventSink* pCb, IConfigDb* pDataDesc ) override
    { return CPythonServer::AcceptNewStream( pCb, pDataDesc ); }
    
    gint32 CreateStmSkel(
        HANDLE, guint32, InterfPtr& ) override;
    
    gint32 OnPreStart(
        IEventSink* pCallback ) override;

    gint32 InvokeUserMethod(
        IConfigDb* pParams,
        IEventSink* pCallback ) override;

    gint32 SendEvent(
        PyObject* pCallback,
        const std::string& strCIfName,
        const std::string& strMethod,
        const std::string& strDest,
        PyObject* pArgs,
        guint32 dwSeriProto ) override;
};

%End

%ModuleCode

gint32 IPythonRpcSvc_PImpl::InitUserFuncs()
{
    BEGIN_IFPROXY_MAP( IPythonRpcSvc, false );
    END_IFPROXY_MAP;
    return STATUS_SUCCESS;
}

gint32 IPythonRpcSvc_SImpl::InitUserFuncs()
{
    BEGIN_IFHANDLER_MAP( IPythonRpcSvc );
    END_IFHANDLER_MAP;
    return STATUS_SUCCESS;
}

gint32 CPythonProxyRosImpl::CreateStmSkel(
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
            clsid( CPythonRpcSvc_CliSkel ),
            oCfg.GetCfg() );
    }while( 0 );
    return ret;
}

gint32 CPythonProxyRosImpl::OnPreStart(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCtx;
        CCfgOpenerObj oIfCfg( this );
        oCtx[ propClsid ] = clsid( 
            CPythonRpcSvc_ChannelCli );
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

gint32 CPythonProxyRosImpl::AsyncCallVector(
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

gint32 CPythonProxyRosImpl::InvokeUserMethod(
        IConfigDb* pParams,
        IEventSink* pCallback )
{
    return CPythonProxy::InvokeUserMethod(
        pParams, pCallback );
}


gint32 CPythonServerRosImpl::SendEvent(
    PyObject* pCallback,
    const std::string& strCIfName,
    const std::string& strMethod,
    const std::string& strDest,
    PyObject* pArgs,
    guint32 dwSeriProto )
{
    guint32 ret = 0;
    bool bDec = false;
    TaskletPtr pTask;
    do{
        if( strMethod.empty() ||
            strCIfName.empty() )
        {
            ret = -EINVAL;
            break;
        }

        std::vector< Variant > vecArgs;
        ret = List2Vector( pArgs, vecArgs );
        if( ERROR( ret ) )
            break;

        IEventSink* pCb = nullptr;
        if( pCallback == nullptr ||
            pCallback == Py_None )
        {
            pTask.NewObj( clsid( CIfDummyTask ) );
        }
        else
        {
            CParamList oReqCtx;
            ret = NEW_PROXY_RESP_HANDLER2(
                pTask, ObjPtr( this ),
                &CPythonInterfBase::OnAsyncCallResp,
                nullptr, oReqCtx.GetCfg() );

            if( ERROR( ret ) )
                break;

            oReqCtx.Push(
                ( intptr_t ) pCallback );
            Py_INCREF( pCallback );

            PyObject* listResp = PyList_New( 2 );
            oReqCtx.Push( ( intptr_t )listResp );
            bDec = true;
        }
        pCb = pTask;

        Py_BEGIN_ALLOW_THREADS;

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
                
                // we don't expect a response
                oReq.SetCallFlags( 
                   DBUS_MESSAGE_TYPE_SIGNAL
                   | CF_ASYNC_CALL );

                oReq.SetMethodName( strMethod );

                for( auto elem : vecArgs )
                    oReq.Push( elem );

                ret = BroadcastEvent(
                    oReq.GetCfg(), pCb );
            }
        }
        Py_END_ALLOW_THREADS;
        
    }while( 0 );

    if( ret != STATUS_PENDING && bDec )
    {
        Py_DECREF( pCallback );
        if( !pTask.IsEmpty() )
            ( *pTask )( eventCancelTask );
    }

    return ret;
}

gint32 CPythonServerRosImpl::CreateStmSkel(
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
            clsid( CPythonRpcSvc_SvrSkel ),
            oCfg.GetCfg() );
    }while( 0 );
    return ret;
}

gint32 CPythonServerRosImpl::OnPreStart(
    IEventSink* pCallback )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCtx;
        CCfgOpenerObj oIfCfg( this );
        oCtx[ propClsid ] = clsid( 
            CPythonRpcSvc_ChannelSvr );
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

gint32 CPythonServerRosImpl::InvokeUserMethod(
        IConfigDb* pParams,
        IEventSink* pCallback )
{
    return CPythonServer::InvokeUserMethod(
        pParams, pCallback );
}

%End
