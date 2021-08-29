%ModuleHeaderCode

class CPythonServerImpl :
    public CPythonInterfBase< CPythonServer >
{
    public:
    typedef CPythonInterfBase< CPythonServer > super;
    CPythonServerImpl( const IConfigDb* pCfg ) 
        : super::_MyVirtBase( pCfg ), super( pCfg )
    { SetClassId( clsid( CPythonServerImpl ) ); }

    gint32 SendEvent(
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

            std::vector< BufPtr > vecArgs;
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
                    &CPythonServerImpl::OnAsyncCallResp,
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
                
                // we don't expect a response
                oReq.SetCallFlags( 
                   DBUS_MESSAGE_TYPE_SIGNAL
                   | CF_ASYNC_CALL );

                oReq.SetMethodName( strMethod );

                for( auto elem : vecArgs )
                    oReq.Push<BufPtr&>( elem );

                ret = BroadcastEvent( oReq.GetCfg(), pCb );
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

    gint32 AcceptNewStream(
        IEventSink* pCallback,
        IConfigDb* pCfg ) override
    {
        if( !this->IsServer() )
            return -EINVAL;

        gint32 ret = 0;
        PyGILState_STATE gstate;
        gstate = PyGILState_Ensure();
        do{
            PyObject* pHost = nullptr;
            ret = GetPyHost( pHost );
            if( ERROR( ret ) )
                break;

            ObjPtr* ppObj = new ObjPtr();
            *ppObj = pCfg;
            PyObject* pPyObj = sipConvertFromNewType(
                ppObj, sipType_cpp_ObjPtr, SIP_NULLPTR );
            if( unlikely( pPyObj == nullptr ) )
            {
                ret = -EFAULT;
                break;
            }

            PyObject* pPyRet = nullptr;
            pPyRet = PyObject_CallMethod(
                 pHost, "AcceptNewStream", 
                 "(O)", pPyObj );
            ret = PyLong_AsLong( pPyRet );
            Py_DECREF( pPyObj );

        }while( 0 );
        PyGILState_Release( gstate );
        return ret;
    }

};

%End

class CPythonServer :
    public cpp::CInterfaceServer /Abstract/
{
    public:

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

    gint32 WriteStreamNoWait(
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
        sipRes = pImpl->
            WriteStreamNoWait( a0, pBuf );
        Py_END_ALLOW_THREADS;

    }while( 0 );
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

    PyObject* ReadStreamNoWait( HANDLE hChannel );
    %MethodCode
    gint32 ret = 0;

    PyObject* pResp = PyList_New( 2 );
    sipRes = pResp;

    do{

        CPythonServerImpl* pImpl = static_cast
            < CPythonServerImpl* >( sipCpp );

        if( pImpl == nullptr ) 
        {
            ret = -EFAULT;
            break;
        }

        BufPtr pBuf;

        Py_BEGIN_ALLOW_THREADS;
        ret = pImpl->ReadStreamNoWait( a0, pBuf );
        Py_END_ALLOW_THREADS;

        if( ERROR( ret ) )
            break;

        PyObject* pPyBuf = nullptr;
        ret = pImpl->ConvertBufToPyObj( pBuf, pPyBuf );
        if( ERROR( ret ) )
            break;

        BufPtr* ppBuf = new BufPtr( pBuf );
        PyObject* pPyBufBak = sipConvertFromNewType(
            ppBuf, sipType_cpp_BufPtr, SIP_NULLPTR );

        if( pPyBufBak == nullptr )
        {
            delete ppBuf;
            ret = -ENOMEM;
            break;
        }

        PyList_SetItem( pResp, 1, pPyBuf );
        PyList_Append( pResp, pPyBufBak );
        Py_DECREF( pPyBufBak );

    }while( 0 );

    PyList_SetItem( pResp, 0,
        PyLong_FromLong( ret ) );

    %End

    PyObject* ReadStream( HANDLE hChannel, guint32 dwSize );
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

        if( a1 > MAX_BYTES_PER_FILE )
        {
            ret = -EINVAL;
            break;
        }

        BufPtr pBuf;
        if( a1 > 0 )
        {
            pBuf.NewObj();
            pBuf->Resize( a1 );
        }

        Py_BEGIN_ALLOW_THREADS;
        ret = pImpl->ReadStream( a0, pBuf );
        Py_END_ALLOW_THREADS;
        if( ERROR( ret ) )
        {
            ret = -EFAULT;
            break;
        }

        PyObject* pPyBuf = nullptr;
        ret = pImpl->ConvertBufToPyObj( pBuf, pPyBuf );
        if( ERROR( ret ) )
            break;

        BufPtr* ppBuf = new BufPtr( pBuf );
        return sipBuildResult(0,"(iRN)",
            0, pPyBuf, ppBuf,sipType_cpp_BufPtr,SIP_NULLPTR);

        break;

    }while( 0 );
    if( ERROR( ret ) )
    {
        sipRes = sipBuildResult(
            0,"(i)",ret );
    }
    %End

    PyObject* ReadStreamAsync(
        HANDLE hChannel, PyObject* pCb, guint32 dwSize );
    %MethodCode
    PyObject* pResp = PyList_New( 2 );
    sipRes = pResp;
    gint32 ret = 0;
    do{
        CPythonServerImpl* pImpl = static_cast
            < CPythonServerImpl* >( sipCpp );

        if( pImpl == nullptr || a1 == nullptr ) 
        {
            ret = -EFAULT;
            break;
        }

        if( a2 > MAX_BYTES_PER_FILE )
        {
            ret = -EINVAL;
            break;
        }

        ret = PyCallable_Check( a1 );
        if( ret == 0 )
        {
            ret = -EINVAL;
            break;
        }

        CParamList oReqCtx;
        oReqCtx.Push( ( intptr_t ) a1 );
        Py_INCREF( a1 );

        cpp::BufPtr pBuf;
        if( a2 > 0 )
        {
            ret = pBuf.NewObj();
            if( ERROR( ret ) )
                break;

            pBuf->Resize( a2 );
        }
        IConfigDb* pCtx = oReqCtx.GetCfg();
        Py_BEGIN_ALLOW_THREADS;
        ret = pImpl->ReadStreamAsync(
            a0, pBuf, pCtx );
        Py_END_ALLOW_THREADS;

        if( ret != STATUS_PENDING )
            Py_DECREF( a1 );

        if( ret == STATUS_PENDING || ERROR( ret ) )
            break;

        PyObject* pPyBuf = nullptr;
        ret = pImpl->ConvertBufToPyObj( pBuf, pPyBuf );
        if( ERROR( ret ) )
            break;

        BufPtr* ppBuf = new BufPtr( pBuf );
        PyObject* pPyBufBak = sipConvertFromNewType(
            ppBuf, sipType_cpp_BufPtr, SIP_NULLPTR );
        if( unlikely( pPyBufBak == nullptr ) )
        {
            ret = -ENOMEM;
            delete ppBuf;
            break;
        }

        PyList_SetItem( pResp, 1, pPyBuf );
        PyList_Append( pResp, pPyBufBak );
        Py_DECREF( pPyBufBak );

    }while( 0 );

    PyList_SetItem( pResp, 0,
        PyLong_FromLong( ret ) );

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
        if( sipRes != STATUS_PENDING )
        {
            Py_DECREF( a1 );
            Py_DECREF( a2 );
        }
        break;

    }while( 0 );
    %End

    gint32 SetResponse(
        cpp::ObjPtr& pCallback,
        gint32 iRet,
        guint32 dwSeriProto,
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
                a3, vecResp);
            if( ERROR( ret ) )
            {
                oResp[ propReturnValue ] = ret;
                ret = 0;
                break;
            }
            for( auto elem : vecResp )
                oResp.Push( elem );

            oResp[ propSeriProto ] = a2;

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
        guint32 dwSeriProto,
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
                a3, vecResp );
            if( ERROR( ret ) )
            {
                oResp[ propReturnValue ] = ret;
                ret = 0;
                break;
            }
            for( auto elem : vecResp )
                oResp.Push( elem );

            oResp[ propSeriProto ] = a2;

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
        PyObject* pListArgs,
        guint32 dwSeriProto );
    %MethodCode
        CPythonServerImpl* pImpl =
        static_cast< CPythonServerImpl* >
            ( sipCpp );
        sipRes = pImpl->SendEvent(
            a0, *a1, *a2, *a3, a4, a5 );
    %End
    bool IsServer();
    %MethodCode
        sipRes = sipCpp->IsServer();
    %End

    gint32 AddTimer(
        guint32 dwTimeoutSec,
        PyObject* pCb,
        PyObject* pCtx,
        cpp::ObjPtr& pTimer /Out/ );
    %MethodCode
        CPythonServerImpl* pImpl =
        static_cast< CPythonServerImpl* >
            ( sipCpp );
        sipRes = pImpl->AddTimer(
            a0, a1, a2, *a3 );
    %End

    gint32 DisableTimer(
        cpp::ObjPtr& pTimer );
    %MethodCode
        CPythonServerImpl* pImpl =
        static_cast< CPythonServerImpl* >
            ( sipCpp );
        sipRes = pImpl->DisableTimer( *a0 );
    %End

    gint32 InstallCompNotify(
        cpp::ObjPtr& pCallback,
        PyObject* pCb,
        PyObject* pListResp );
    %MethodCode
        CPythonServerImpl* pImpl =
        static_cast< CPythonServerImpl* >
            ( sipCpp );
        sipRes = pImpl->InstallCompNotify(
            *a0, a1, a2 );
    %End

    guint64 GetIdHashByChan( HANDLE );
    %MethodCode
    do{
        sipRes = 0;
        CPythonServerImpl* pImpl =
        static_cast< CPythonServerImpl* >
            ( sipCpp );
        
        InterfPtr pIf;
        gint32 ret = pImpl->GetUxStream( a0, pIf );
        if( ERROR( ret ) )
            break;
        guint64 qwHash = 0;
        guint64 qwId = pIf->GetObjId();
        ret = GetObjIdHash( qwId, qwHash );
        if( ERROR( ret ) )
            break;
        sipRes = qwHash;
    }while( 0 );
    %End
    HANDLE GetChanByIdHash( guint64 );
    %MethodCode
        CPythonServerImpl* pImpl =
        static_cast< CPythonServerImpl* >
            ( sipCpp );
        sipRes = pImpl->GetChanByIdHash( a0 );
    %End

    gint32 PyDeferCall( PyObject* pPyCb,
        PyObject* pPyArgs );
    %MethodCode
        CPythonServerImpl* pImpl =
        static_cast< CPythonServerImpl* >
            ( sipCpp );
        sipRes = pImpl->PyDeferCall( a0, a1 );
    %End
};

gint32 ChainTasks(
    cpp::ObjPtr& pObj1,
    cpp::ObjPtr& pObj2 );
%MethodCode
    CIfRetryTask* pTask1 = *a0;
    CIfRetryTask* pTask2 = *a1;
    if( pTask1 == nullptr || pTask2 == nullptr )
    {
        sipRes = -EINVAL;
    }
    else
    {
        // Risk alert: potential race condition if
        // one of the two tasks is already running
        sipRes = pTask1->SetClientNotify(
            pTask2 );
    }
%End

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

