%ModuleHeaderCode

class CPythonServerImpl :
    public CPythonInterfBase< CPythonServer >
{
    public:
    typedef CPythonInterfBase< CPythonServer > super;
    CPythonServerImpl( const IConfigDb* pCfg ) 
        : super::_MyVirtBase( pCfg ), super( pCfg )
    { SetClassId( clsid( CPythonServerImpl ) ); }

    gint32 OnStreamReady( HANDLE hChannel )
    {
        gint32 ret = 0;
        do{
            Py_BEGIN_ALLOW_THREADS;
            PyObject* pHost = nullptr;
            ret = GetPyHost( pHost );
            if( ERROR( ret ) )
                break;
            PyObject_CallMethod(
             pHost, "OnStmReady", 
             "(L)", hChannel );
            Py_END_ALLOW_THREADS;
        }while( 0 );
        return ret;
    }

    gint32 OnStmClosing( HANDLE hChannel )
    {
        if( hChannel == INVALID_HANDLE )
            return -EINVAL;

        RemoveChanCtx( hChannel );
        return super::OnStmClosing( hChannel );
    }

    gint32 SendEvent(
        PyObject* pCallback,
        const std::string& strCIfName,
        const std::string& strMethod,
        const std::string& strDest,
        PyObject* pArgs )
    {
        guint32 ret = 0;
        do{
            if( strMethod.empty() ||
                strCIfName.empty() )
            {
                ret = -EINVAL;
                break;
            }

            CReqBuilder oReq( this );
            std::string strIfName =
               DBUS_IF_NAME( strCIfName );
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

                if( pIfMatch.IsEmpty() )
                {
                    ret = -ENOENT;
                    break;
                }
                
                oReq.SetIfName( strIfName );
            }

            if( !strDest.empty() )
                oReq.SetDestination( strDest );
            
            // we don't expect a response
            oReq.SetCallFlags( 
               DBUS_MESSAGE_TYPE_SIGNAL
               | CF_ASYNC_CALL );

            oReq.SetMethodName( strMethod );

            std::vector< BufPtr > vecArgs;
            ret = List2Vector( pArgs, vecArgs );
            if( ERROR( ret ) )
                break;

            for( auto elem : vecArgs )
                oReq.Push<BufPtr&>( elem );

            IEventSink* pCb = nullptr;
            TaskletPtr pTask;
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
                    &CPythonServerImpl::OnAsyncCallResp,
                    nullptr, oReqCtx.GetCfg() );

                if( ERROR( ret ) )
                    break;

                oReqCtx.Push(
                    ( intptr_t ) pCallback );
                Py_INCREF( pCallback );

                PyObject* listResp = PyList_New( 2 );
                oReqCtx.Push( ( intptr_t )listResp );
            }
            pCb = pTask;
            Py_BEGIN_ALLOW_THREADS;
            BroadcastEvent( oReq.GetCfg(), pCb );
            Py_END_ALLOW_THREADS;
            
        }while( 0 );

        return ret;
    }
};

%End

class CPythonServer :
    public cpp::CInterfaceServer /Abstract/
{
    public:
    %TypeHeaderCode
    #include "stmsvr.h"
    %End

    gint32 SetPyHost( PyObject* pObj );
    %MethodCode
        CPythonServerImpl* pImpl = static_cast
            < CPythonServerImpl* >( sipCpp );
        if( pImpl == nullptr ) 
            sipRes = -EFAULT;
        else
            sipRes = pImpl->SetPyHost( a0 );
    %End

    PyObject* GetPyHost();
    %MethodCode
        CPythonServerImpl* pImpl = static_cast
            < CPythonServerImpl* >( sipCpp );
        PyObject* pObj = nullptr;
        sipRes = SIP_NULLPTR;
        if( pImpl != nullptr ) 
        {
            gint32 ret = 
                pImpl->GetPyHost( pObj );
            if( SUCCEEDED( ret ) )
                sipRes = pObj;
        }
    %End

    gint32 RemovePyHost();
    %MethodCode
        CPythonServerImpl* pImpl = static_cast
            < CPythonServerImpl* >( sipCpp );
        if( pImpl == nullptr ) 
            sipRes = -EFAULT;
        else
            sipRes = pImpl->RemovePyHost();
    %End

    gint32 CloseStream( HANDLE hChannel );
    %MethodCode
        Py_BEGIN_ALLOW_THREADS;
        CPythonServerImpl* pImpl = static_cast
            < CPythonServerImpl* >( sipCpp );
        if( pImpl == nullptr ) 
            sipRes = -EFAULT;
        else
        {
            sipRes = pImpl->
                CloseStream( a0 );
        }
        Py_END_ALLOW_THREADS;
    %End

    gint32 WriteStream(
        HANDLE hChannel, PyObject* pPyBuf );
    %MethodCode
    do{
        CPythonServerImpl* pImpl = static_cast
            < CPythonServerImpl* >( sipCpp );

        if( pImpl == nullptr ) 
        {
            sipRes = -EFAULT;
            break;
        }

        BufPtr pBuf( true );
        sipRes = pImpl->ConvertPyObjToBuf(
            a1, pBuf );
        if( ERROR( sipRes ) )
            break;

        Py_BEGIN_ALLOW_THREADS;
        sipRes = pImpl->WriteStream( a0, pBuf );
        Py_END_ALLOW_THREADS;

    }while( 0 );
    %End

    PyObject* ReadStream( HANDLE hChannel );
    %MethodCode
    gint32 ret = 0;
    do{
        CPythonServerImpl* pImpl = static_cast
            < CPythonServerImpl* >( sipCpp );

        if( pImpl == nullptr ) 
        {
            ret = -EFAULT;
            break;
        }

        BufPtr* ppBuf = new BufPtr() ;
        Py_BEGIN_ALLOW_THREADS;
        ret = pImpl->ReadStream( a0, *ppBuf );
        Py_END_ALLOW_THREADS;
        if( ERROR( ret ) )
        {
            ret = -EFAULT;
            break;
        }

        sipRes = sipBuildResult(0,"(iN)",
            ret,ppBuf,sipType_cpp_BufPtr,SIP_NULLPTR);

        break;

    }while( 0 );
    if( ERROR( ret ) )
    {
        sipRes = sipBuildResult(
            0,"(i)",ret );
    }
    %End

    gint32 ReadStreamAsync(
        HANDLE hChannel, PyObject* pCb );
    %MethodCode
    do{
        sipRes = 0;
        CPythonServerImpl* pImpl = static_cast
            < CPythonServerImpl* >( sipCpp );

        if( pImpl == nullptr || a1 == nullptr ) 
        {
            sipRes = -EFAULT;
            break;
        }

        gint32 ret = PyCallable_Check( a1 );
        if( ret == 0 )
        {
            sipRes = -EINVAL;
            break;
        }

        CParamList oReqCtx;
        oReqCtx.Push( ( intptr_t ) a1 );
        Py_INCREF( a1 );

        BufPtr pBuf;
        IConfigDb* pCtx = oReqCtx.GetCfg();
        Py_BEGIN_ALLOW_THREADS;
        sipRes = pImpl->ReadStreamAsync(
            a0, pBuf, pCtx );
        Py_END_ALLOW_THREADS;
        break;

    }while( 0 );

    %End

    gint32 WriteStreamAsync(
        HANDLE hChannel,
        PyObject* pPyBuf,
        PyObject* pCb );
    %MethodCode
    do{
        sipRes = 0;
        CPythonServerImpl* pImpl = static_cast
            < CPythonServerImpl* >( sipCpp );

        if( pImpl == nullptr ) 
        {
            sipRes = -EFAULT;
            break;
        }

        BufPtr pBuf( true );
        gint32 ret = PyCallable_Check( a2 );
        if( ret == 0 )
        {
            sipRes = -EINVAL;
            break;
        }

        sipRes = pImpl->ConvertPyObjToBuf(
            a1, pBuf );
        if( ERROR( sipRes ) )
            break;

        CParamList oReqCtx;
        oReqCtx.Push( ( intptr_t ) a1 );
        oReqCtx.Push( ( intptr_t ) a2 );
        Py_INCREF( a1 );
        Py_INCREF( a2 );

        IConfigDb* pCtx = oReqCtx.GetCfg();
        Py_BEGIN_ALLOW_THREADS;
        sipRes = pImpl->WriteStreamAsync(
            a0, pBuf, pCtx );
        Py_END_ALLOW_THREADS;
        break;

    }while( 0 );
    %End

    gint32 SetResponse(
        cpp::ObjPtr& pCallback, gint32 iRet,
        PyObject* pListResp );
    %MethodCode
        gint32 ret = 0;
        CParamList oResp;
        do{
            CPythonServerImpl* pImpl =
            static_cast< CPythonServerImpl* >
                ( sipCpp );

            if( a0->IsEmpty() )
            {
                ret = -EINVAL;
                break;
            }
            oResp[ propReturnValue ] = a1;
            if( ERROR( a1 ) )
                break;

            std::vector< BufPtr > vecResp;
            ret = pImpl->List2Vector(
                a2, vecResp);
            if( ERROR( ret ) )
            {
                oResp[ propReturnValue ] = ret;
                ret = 0;
                break;
            }
            for( auto elem : vecResp )
                oResp.Push( elem );
        }while( 0 );
        if( SUCCEEDED( ret ) )
        {
            sipCpp->SetResponse(
                *a0, oResp.GetCfg() );
        }
        sipRes = ret;
    %End

    gint32 OnServiceComplete(
        cpp::ObjPtr& pCallback,
        gint32 iRet,
        PyObject* pListResp );
    %MethodCode
        gint32 ret = 0;
        CParamList oResp;
        do{
            CPythonServerImpl* pImpl =
            static_cast< CPythonServerImpl* >
                ( sipCpp );

            if( a0->IsEmpty() )
            {
                ret = -EINVAL;
                break;
            }
            oResp[ propReturnValue ] = a1;
            if( ERROR( a1 ) )
                break;

            std::vector< BufPtr > vecResp;
            ret = pImpl->List2Vector(
                a2, vecResp );
            if( ERROR( ret ) )
            {
                oResp[ propReturnValue ] = ret;
                ret = 0;
                break;
            }
            for( auto elem : vecResp )
                oResp.Push( elem );
        }while( 0 );
        if( SUCCEEDED( ret ) )
        {
            Py_BEGIN_ALLOW_THREADS;
            sipCpp->OnServiceComplete(
                oResp.GetCfg(), *a0 );
            Py_END_ALLOW_THREADS;
        }
        sipRes = ret;
    %End

    PyObject* GetChanCtx( HANDLE hChannel );
    %MethodCode
        CPythonServerImpl* pImpl =
        static_cast< CPythonServerImpl* >
            ( sipCpp );
        sipRes = pImpl->GetChanCtx( a0 );
    %End

    gint32 SetChanCtx(
        HANDLE hChannel, PyObject* pCtx );
    %MethodCode
        CPythonServerImpl* pImpl =
        static_cast< CPythonServerImpl* >
            ( sipCpp );
        sipRes = pImpl->SetChanCtx( a0, a1 );
    %End

    gint32 RemoveChanCtx( HANDLE hChannel );
    %MethodCode
        CPythonServerImpl* pImpl =
        static_cast< CPythonServerImpl* >
            ( sipCpp );
        sipRes = pImpl->RemoveChanCtx( a0 );
    %End

    gint32 SendEvent( PyObject* pCallback,
        const std::string& strIfName,
        const std::string& strMethod,
        const std::string& strDest,
        PyObject* pListArgs );
    %MethodCode
        Py_BEGIN_ALLOW_THREADS;
        CPythonServerImpl* pImpl =
        static_cast< CPythonServerImpl* >
            ( sipCpp );
        sipRes = pImpl->SendEvent(
            a0, *a1, *a2, *a3, a4 );
        Py_END_ALLOW_THREADS;
    %End
    bool IsServer();
    %MethodCode
        sipRes = sipCpp->IsServer();
    %End
};

cpp::ObjPtr CreateRpcServer(
    cpp::ObjPtr& pMgr,
    const std::string& strDesc,
    const std::string& strObjName,
    cpp::ObjPtr& pCfgObj );
%MethodCode
    gint32 ret = 0;
    do{
        cpp::CfgPtr pCfg = *a3;

        if( pCfg.IsEmpty() )
        {
            CParamList oParams;
            oParams.SetObjPtr(
                propIoMgr, *a0 );
            pCfg = oParams.GetCfg();
        }
        else
        {
            CParamList oParams( pCfg );
            oParams.SetObjPtr(
                propIoMgr, *a0 );
        }

        ret = CRpcServices::LoadObjDesc(
            *a1, *a2, true, pCfg );
        if( ERROR( ret ) )
            break;

        cpp::ObjPtr pIf;
        ret = pIf.NewObj(
            clsid( CPythonServerImpl ),
            pCfg );
        if( ERROR( ret ) )
            break;

        sipRes = new cpp::ObjPtr(
            nullptr, false );
        *sipRes = pIf;

    }while( 0 );
%End

CPythonServer* CastToServer(
    cpp::ObjPtr& pObj );
%MethodCode
    CPythonServer* pSvr = *a0;
    sipRes = pSvr;
%End

