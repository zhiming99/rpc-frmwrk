/*
 * =====================================================================================
 *
 *       Filename:  swig.i
 *
 *    Description:  the swig file as the wrapper of CJavaServerImpl class and
 *                  helper classes for Java
 *
 *        Version:  1.0
 *        Created:  10/17/2021 04:39:30 PM
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

class CJavaServerImpl :
    public CJavaInterfBase<CJavaServer>
{
    public:
    typedef CJavaInterfBase< CJavaServer > super;
    CJavaServerImpl( const IConfigDb* pCfg ) 
        : super::_MyVirtBase( pCfg ), super( pCfg )
    { SetClassId( clsid( CJavaServerImpl ) ); }

    gint32 SendEvent(
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

            std::vector< BufPtr > vecArgs;
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
                    &CJavaServerImpl::OnAsyncCallResp,
                    nullptr, oReqCtx.GetCfg() );

                if( ERROR( ret ) )
                    break;

                pjCb = jenv->NewGlobalRef( pCallback );
                jobject jret = NewJRet( jenv );
                pjret = jenv->NewGlobalRef( jret );

                oReqCtx.Push(
                    ( intptr_t ) pjCb );
                oReqCtx.Push( ( intptr_t )pjret );
                oReqCtx.Push( strMethod );
                oReqCtx.Push( seriNone );
            }
            pCb = pTask;

            CReqBuilder oReq( this );
            std::string strIfName =
               DBUS_IF_NAME( strCIfName );

            oReq.SetIntProp(
                propSeriProto, dwSeriProto );

            bool bHasMatch = true;
            if( true )
            {
                CStdRMutex oIfLock( GetLock() );
                MatchPtr pIfMatch;
                for( auto pMatch : m_vecMatches )
                {
                    CCfgOpenerObj oMatch(
                        ( CObjBase* )pMatch );

                    std::string strRegIf;

                    ret = oMatch.GetStrProp(
                        propIfName, strRegIf );

                    if( ERROR( ret ) )
                        continue;

                    if( strIfName == strRegIf )
                    {
                        pIfMatch = pMatch;
                        break;
                    }
                }

                if( !pIfMatch.IsEmpty() )
                    oReq.SetIfName( strIfName );
                else
                    bHasMatch = false;
            }

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

    gint32 AcceptNewStream(
        IEventSink* pCallback,
        IConfigDb* pCfg )
    {
        if( !this->IsServer() )
            return -EINVAL;

        gint32 ret = 0;
        bool bAttach = false;
        JNIEnv *jenv = nullptr;
        ret = GetJavaEnv( jenv, bAttach );
        if( ERROR( ret ) )
            return ret;

        do{
            jobject pHost = nullptr;
            ret = GetJavaHost( pHost );
            if( ERROR( ret ) )
                break;

            ObjPtr* ppObj =
                new ObjPtr( pCallback );
            jobject pCb = NewObjPtr(
                jenv, ( jlong )ppObj );
            if( pCb == nullptr )
            {
                delete ppObj;
                ret = -EFAULT;
                break;
            }
            CfgPtr* ppCfg = new CfgPtr( pCfg );
            jobject pjCfg = NewCfgPtr(
                jenv, ( jlong )ppCfg );
            if( pjCfg == nullptr )
            {
                delete ppCfg;
                ret = -EFAULT;
                break;
            }

            jclass cls =
                jenv->GetObjectClass( pHost );

            jmethodID ans = jenv->GetMethodID(
                cls, "acceptNewStream",
                "(Lorg/rpc/rpcbase/ObjPtr;"
                "Lorg/rpc/rpcbase/CfgPtr;)I" );

            ret = jenv->CallIntMethod( pHost,
                ans, pCb, pjCfg );

        }while( 0 );

        if( bAttach )
            PutJavaEnv();

        return ret;
    }

    jobject GetPeerIdHash(
        JNIEnv *jenv,
        jlong hChannel )
    { return nullptr; }

    jlong GetIdHashByChan( jlong hChannel )
    {
        InterfPtr pIf;
        gint32 ret = this->GetUxStream(
            ( HANDLE )hChannel, pIf );
        if( ERROR( ret ) )
            return 0;

        guint64 qwHash = 0;
        guint64 qwId = pIf->GetObjId();
        ret = GetObjIdHash( qwId, qwHash );
        if( ERROR( ret ) )
            return 0;
         return qwHash;
    } 
};

gint32 ChainTasks(
    ObjPtr& pObj1,
    ObjPtr& pObj2 )
{
    gint32 ret = 0;
    CIfRetryTask* pTask1 = pObj1;
    CIfRetryTask* pTask2 = pObj2;
    if( pTask1 == nullptr || pTask2 == nullptr )
    {
        ret = -EINVAL;
    }
    else
    {
        // Risk alert: potential race condition if
        // one of the two tasks is already running
        // use it carefully
        ret = pTask1->SetClientNotify( pTask2 );
    }
    return ret;
}

jobject CreateServer(
    JNIEnv *jenv,
    ObjPtr& pMgr,
    const std::string& strDesc,
    const std::string& strObjName,
    ObjPtr& pCfgObj )
{
    gint32 ret = 0;
    jobject jret = NewJRet( jenv );

    do{
        CfgPtr pCfg = pCfgObj;

        if( pCfg.IsEmpty() )
        {
            CParamList oParams;
            oParams.SetObjPtr(
                propIoMgr, pMgr );
            pCfg = oParams.GetCfg();
        }
        else
        {
            CParamList oParams( pCfg );
            oParams.SetObjPtr(
                propIoMgr, pMgr );
        }

        ret = CRpcServices::LoadObjDesc(
            strDesc, strObjName, true, pCfg );
        if( ERROR( ret ) )
            break;

        ObjPtr* ppIf = new ObjPtr();
        ret = ppIf->NewObj(
            clsid( CJavaServerImpl ),
            pCfg );
        if( ERROR( ret ) )
        {
            delete ppIf;
            break;
        }

        jobject pjObj = NewObjPtr(
            jenv, ( jlong )ppIf );
        if( pjObj == nullptr )
        {
            delete ppIf;
            ret = -EFAULT;
            break;
        }
        AddElemToJRet( jenv, jret, pjObj );

    }while( 0 );

    SetErrorJRet( jenv, jret, ret );
    return jret;
}

CJavaServerImpl* CastToServer(
    ObjPtr& pObj )
{
    CJavaServerImpl* pSvr = pObj;
    return pSvr;
}

}

%nodefaultctor;
%template(CJavaInterfBaseS) CJavaInterfBase<CJavaServer>;
%typemap(javadestruct_derived, methodname="delete", methodmodifiers="public synchronized") CJavaServerImpl {
    if (swigCPtr != 0) {
      if (swigCMemOwn) {
        swigCMemOwn = false;
      }
      swigCPtr = 0;
    }
    super.delete();
  }
class CJavaServerImpl :
    public CJavaInterfBase<CJavaServer>
{
    public:
%extend{ 
    gint32 SetInvTimeout(
        ObjPtr& pCallback,
        gint32 dwTimeoutSec )
    {
        gint32 ret = 0;
        do{
            CJavaServerImpl* pImpl = static_cast
                < CJavaServerImpl* >( $self );
            if( pCallback.IsEmpty() )
            {
                ret = -EINVAL;
                break;
            }
            ret = pImpl->SetInvTimeout(pCallback,
                ( guint32 )dwTimeoutSec );

        }while( 0 );
        return ret;
    }

    gint32 SetResponse(
        JNIEnv *jenv,
        ObjPtr& pCallback,
        gint32 iRet,
        gint32 dwSeriProto,
        jobject arrArgs )
    {
        gint32 ret = 0;
        CParamList oResp;
        CJavaServerImpl* pImpl = static_cast
            < CJavaServerImpl* >( $self );
        do{

            if( pCallback.IsEmpty() )
            {
                ret = -EINVAL;
                break;
            }
            oResp[ propReturnValue ] =
                ( guint32 )iRet;

            if( ERROR( iRet ) )
                break;

            std::vector< BufPtr > vecResp;
            ret = pImpl->List2Vector(
                jenv, arrArgs, vecResp);
            if( ERROR( ret ) )
            {
                oResp[ propReturnValue ] = ret;
                ret = 0;
                break;
            }
            for( auto elem : vecResp )
                oResp.Push( elem );

            oResp[ propSeriProto ] =
                ( guint32 )dwSeriProto;

        }while( 0 );
        if( SUCCEEDED( ret ) )
        {
            pImpl->SetResponse(
                pCallback, oResp.GetCfg() );
        }
        return ret;
    }

    gint32 OnServiceComplete(
        JNIEnv *jenv,
        ObjPtr& pCallback,
        gint32 iRet,
        gint32 dwSeriProto,
        jobject arrArgs )
    {
        gint32 ret = 0;
        CParamList oResp;
        CJavaServerImpl* pImpl = static_cast
            < CJavaServerImpl* >( $self );
        do{

            if( pCallback.IsEmpty() )
            {
                ret = -EINVAL;
                break;
            }
            oResp[ propReturnValue ] =
                ( guint32 )iRet;
            if( ERROR( iRet ) )
                break;

            std::vector< BufPtr > vecResp;
            ret = pImpl->List2Vector(
                jenv, arrArgs, vecResp );
            if( ERROR( ret ) )
            {
                oResp[ propReturnValue ] = ret;
                ret = 0;
                break;
            }
            for( auto elem : vecResp )
                oResp.Push( elem );
            oResp[ propSeriProto ] =
                ( guint32 )dwSeriProto;

        }while( 0 );
        if( SUCCEEDED( ret ) )
        {
            pImpl->OnServiceComplete(
                oResp.GetCfg(), pCallback );
        }
        return ret;
    }

    gint32 SendEvent(
        JNIEnv *jenv,
        jobject pCallback,
        const std::string& strIfName,
        const std::string& strMethod,
        const std::string& strDest,
        jobject pListArgs,
        gint32 dwSeriProto )
    {
        CJavaServerImpl* pImpl = static_cast
            < CJavaServerImpl* >( $self );
        return pImpl->SendEvent( jenv,
            pCallback, strIfName,
            strMethod, strDest,
            pListArgs,
            ( guint32 )dwSeriProto );
    }

    }

    jobject CastToObjPtr( JNIEnv *jenv );
    jlong GetIdHashByChan( jlong hChannel );
    jobject GetPeerIdHash(
        JNIEnv *jenv,
        jlong hChannel );
};
%clearnodefaultctor;

gint32 ChainTasks( ObjPtr& pObj1, ObjPtr& pObj2 );
jobject CreateServer(
    JNIEnv *jenv,
    ObjPtr& pMgr,
    const std::string& strDesc,
    const std::string& strObjName,
    ObjPtr& pCfgObj );

CJavaServerImpl* CastToServer(
    ObjPtr& pObj );

