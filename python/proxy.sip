%ModuleHeaderCode
enum EnumMyClsid2
{
    DECL_CLSID( CPythonProxyImpl ) =
        clsid( CStreamingClient ) + 1,
};

class CPythonProxyImpl : public CPythonProxy
{
    public:
    typedef CPythonProxy super;
    CPythonProxyImpl( const IConfigDb* pCfg ) 
        : super::_MyVirtBase( pCfg ), super( pCfg )
    { SetClassId( clsid( CPythonProxyImpl ) ); }

    gint32 ConvertBufToPyObj(
        BufPtr& pBuf,
        PyObject*& pObject );

    gint32 GetNumpyValue(
        gint32 iType,
        PyObject* pyInt,
        PyObject*& val );

    gint32 FillList( IConfigDb* pResp,
        PyObject* listResp );

    gint32 GetTypeId(
        PyObject* pVar );

    gint32 ConvertPyObjToBuf(
        PyObject* pObject,
        BufPtr& pBuf );

    gint32 List2Vector( PyObject *pList,
        std::vector< BufPtr >& vecArgs );

    gint32 OnProxyCallResp(
        IEventSink* pCallback, 
        IEventSink* pIoReq,
        IConfigDb* pReqCtx );

    gint32 AsyncCallVector(
        IEventSink* pTask,
        CfgPtr& pOptions,
        CfgPtr& pResp,
        const std::string& strcMethod,
        std::vector< BufPtr >& vecParams );

    gint32 PyProxyCall(
        PyObject* pCb,
        const std::string& strIfName,
        const std::string& strMethod,
        PyObject* listArgs,
        PyObject* listResp,
        guint64& qwTaskCancel );

    gint32 PyProxyCallSync(
        const std::string& strIfName,
        const std::string& strMethod,
        PyObject* listArgs,
        PyObject* listResp );

    gint32 SetPyHost(
        PyObject* pObj );

    gint32 GetPyHost(
        PyObject*& pObj );

    gint32 RemovePyHost();

    gint32 InvokeUserMethod(
        IConfigDb* pParams,
        IEventSink* pCallback );

    gint32 OnReadStreamComplete(
        HANDLE hChannel,
        gint32 iRet,
        BufPtr& pBuf,
        IConfigDb* pCtx );

    gint32 OnWriteStreamComplete(
        HANDLE hChannel,
        gint32 iRet,
        BufPtr& pBuf,
        IConfigDb* pCtx );
};
%End

%ModuleCode

gint32 CPythonProxyImpl::ConvertBufToPyObj(
    BufPtr& pBuf, PyObject*& pObj )
{
    gint32 ret = 0;
    if( pBuf.IsEmpty() || pBuf->empty() )
    {
        pObj = Py_None;
        return ret;
    }
    do{
        EnumTypeId iType = pBuf->GetExDataType();
        if( iType == typeNone )
        {
            ret = -ENOTSUP;
            break;
        }

        switch( iType )
        {
        case typeByte: 
            {
                guint8 val = *pBuf;
                PyObject* pTemp =
                    PyLong_FromLong( val );
                ret = GetNumpyValue(
                    iType, pTemp, pObj );
                break;
            }
        case typeUInt16:
            {
                guint16 val = *pBuf;
                PyObject* pTemp =
                    PyLong_FromLong( val );
                ret = GetNumpyValue(
                    iType, pTemp, pObj );
                break;
            }
        case typeUInt32:
            {
                guint32 val = *pBuf;
                PyObject* pTemp =
                    PyLong_FromLong( val );
                ret = GetNumpyValue(
                    iType, pTemp, pObj );
                break;
            }
        case typeUInt64:
            {
                guint64 val = *pBuf;
                PyObject* pTemp =
                    PyLong_FromLongLong( val );
                ret = GetNumpyValue(
                    iType, pTemp, pObj );
                break;
            }
        case typeFloat:
            {
                float val = *pBuf;
                PyObject* pTemp =
                    PyFloat_FromDouble( val );
                ret = GetNumpyValue(
                    iType, pTemp, pObj );
                break;
            }
        case typeDouble:
            {
                double val = *pBuf;
                PyObject* pTemp =
                    PyFloat_FromDouble( val );
                ret = GetNumpyValue(
                    iType, pTemp, pObj );
                break;
            }
        case typeString:
            {
                std::string val = *pBuf;
                pObj =
                    PyUnicode_FromString( val.c_str() );
                break;
            }
        case typeObj:
            {
                ObjPtr* val = ( ObjPtr*)pBuf->ptr();
                pObj = sipConvertFromNewType(
                    val,sipType_cpp_ObjPtr, SIP_NULLPTR );
                break;
            }
        case typeByteArr:
            {
                if( pBuf.IsEmpty() || pBuf->empty() )
                {
                    ret = -EINVAL;
                    break;
                }
                PyObject* pView = PyMemoryView_FromMemory(
                    pBuf->ptr(), pBuf->size(), PyBUF_READ );
                if( pView == nullptr )
                {
                    ret = -EFAULT;
                    break;
                }

                pObj = PyByteArray_FromObject( pView );
                break;
            }
        case typeDMsg:
            {
                ret = -ENOTSUP;
                break;
            }
        default:
            {
                ret = -EINVAL;
                break;
            }
        }

    }while( 0 );
    return ret;
}

gint32 CPythonProxyImpl::FillList(
    IConfigDb* pResp, PyObject* listResp )
{
    gint32 ret = 0;
    do{
        CParamList oResp( pResp );

        guint32 dwSize = 0;
        ret = oResp.GetSize( dwSize );
        if( ERROR( ret ) )
            break;

        if( dwSize > 20 )
        {
            ret = -ERANGE;
            break;
        }

        if( PyList_Size( listResp ) < 2 )
        {
            ret = -ERANGE;
            break;
        }

        PyObject* listArgs = PyList_New( dwSize );
        if( listArgs == nullptr )
        {
            ret = -ENOMEM;
            break;
        }

        if( dwSize == 0 )
            break;

        // note: the first parameter for the
        // callback is the return code 
        for( gint32 i = 0; i < ( gint32 )dwSize; ++i )
        {
            gint32 iType = ( gint32 )typeNone;
            ret = oResp.GetType( i, iType );
            if( ERROR( ret ) )
                break;

            switch( iType )
            {
            case typeByte: 
                {
                    guint8 val = 0;
                    ret = oResp.GetByteProp( i, val );
                    if( ERROR( ret ) )
                        break;
                    PyObject* pObj = PyLong_FromLong( val );
                    PyList_SET_ITEM( listArgs, i, pObj );
                    break;
                }
            case typeUInt16:
                {
                    guint16 val = 0;
                    ret = oResp.GetShortProp( i, val );
                    if( ERROR( ret ) )
                        break;
                    PyObject* pObj = PyLong_FromLong( val );
                    PyList_SET_ITEM( listArgs, i, pObj );
                    break;
                }
            case typeUInt32:
                {
                    guint32 val = 0;
                    ret = oResp.GetIntProp( i, val );
                    if( ERROR( ret ) )
                        break;
                    PyObject* pObj = PyLong_FromLong( val );
                    PyList_SET_ITEM( listArgs, i, pObj );
                    break;
                }
            case typeUInt64:
                {
                    guint64 val = 0;
                    ret = oResp.GetQwordProp( i, val );
                    if( ERROR( ret ) )
                        break;
                    PyObject* pObj =
                        PyLong_FromLongLong( val );
                    PyList_SET_ITEM( listArgs, i, pObj );
                    break;
                }
            case typeFloat:
                {
                    float val = 0;
                    ret = oResp.GetFloatProp( i, val );
                    if( ERROR( ret ) )
                        break;
                    PyObject* pObj =
                        PyFloat_FromDouble( val );
                    PyList_SET_ITEM( listArgs, i, pObj );
                    break;
                }
            case typeDouble:
                {
                    double val = 0;
                    ret = oResp.GetDoubleProp( i, val );
                    if( ERROR( ret ) )
                        break;
                    PyObject* pObj =
                        PyFloat_FromDouble( val );
                    PyList_SET_ITEM( listArgs, i, pObj );
                    break;
                }
            case typeString:
                {
                    std::string val;
                    ret = oResp.GetStrProp( i, val );
                    if( ERROR( ret ) )
                        break;
                    PyObject* pObj =
                        PyUnicode_FromString( val.c_str() );
                    PyList_SET_ITEM( listArgs, i, pObj );
                    break;
                }
            case typeObj:
                {
                    ObjPtr* val = new ObjPtr();
                    ret = oResp.GetObjPtr( i, *val );
                    if( ERROR( ret ) )
                        break;

                    PyObject* pObj = sipConvertFromNewType(
                        val,sipType_cpp_ObjPtr, SIP_NULLPTR );

                    if( pObj == nullptr )
                    {
                        ret = -EFAULT;
                        break;
                    }
                    PyList_SET_ITEM( listArgs, i, pObj );
                    break;
                }
            case typeByteArr:
                {
                    BufPtr pBuf;
                    ret = oResp.GetProperty( i, pBuf );
                    if( ERROR( ret ) )
                        break;
                    if( pBuf.IsEmpty() || pBuf->empty() )
                    {
                        ret = -EINVAL;
                        break;
                    }
                    PyObject* pView = PyMemoryView_FromMemory(
                        pBuf->ptr(), pBuf->size(), PyBUF_READ );
                    if( pView == nullptr )
                    {
                        ret = -EFAULT;
                        break;
                    }

                    PyObject* pByteArr =
                        PyByteArray_FromObject( pView );

                    if( pByteArr == nullptr )
                    {
                        ret = -EFAULT;
                        break;
                    }
                    PyList_SET_ITEM( listArgs, i, pByteArr );
                    break;
                }
            case typeDMsg:
                {
                    ret = -ENOTSUP;
                    break;
                }
            default:
                {
                    ret = -EINVAL;
                    break;
                }
            }

            if( ERROR( ret ) )
                break;
        }

        PyList_SetItem( listResp, 1, listArgs );

    }while( 0 );

    if( ERROR( ret ) )
    {
        PyList_SetItem( listResp,
            0, PyLong_FromLong( ret ) );
    }

    return ret;
}

gint32 CPythonProxyImpl::OnProxyCallResp(
    IEventSink* pCallback, 
    IEventSink* pIoReq,
    IConfigDb* pReqCtx )
{
    gint32 ret = 0;
    if( pIoReq == nullptr ||
        pReqCtx == nullptr )
        return -EINVAL;

    
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    PyObject* pPyCb = nullptr;
    PyObject* pPyResp = nullptr;

    do{
        CCfgOpenerObj oCbCfg( pIoReq );
        IConfigDb* pResp = nullptr;
        ret = oCbCfg.GetPointer(
            propRespPtr, pResp );
        if( ERROR( ret ) )
            break;

        guint32* ptrVal = nullptr;
        CCfgOpener oReqCtx( pReqCtx );
        ret = oReqCtx.GetIntPtr( 0, ptrVal );
        if( ERROR( ret ) )
            break;
        PyObject* pPyCb =
            ( PyObject* )ptrVal;

        ret = oReqCtx.GetIntPtr( 1, ptrVal );
        if( ERROR( ret ) )
            break;

        PyObject* pPyResp =
            ( PyObject* )ptrVal;  

        guint32 iRet = 0;
        CParamList oResp( pResp );
        ret = oResp.GetIntProp(
            propReturnValue, iRet );

        if( SUCCEEDED( ret ) &&
            ERROR( ( gint32 )iRet ) )
            ret = iRet;

        if( ERROR( ret ) )
        {
            PyObject* pRet =
                PyLong_FromLong( ret );
            PyList_SET_ITEM( pPyResp, 0, pRet );
        }
        else
        {
            PyObject* pRet = PyLong_FromLong( 0 );
            PyList_SET_ITEM( pPyResp, 0, pRet );
            ret = FillList( pResp, pPyResp );
        }

        PyObject* pHost = nullptr;
        ret = GetPyHost( pHost );
        if( ERROR( ret ) )
            break;

        PyObject_CallMethod(
             pHost, "HandleAsyncResp", 
             "(OO)", pPyCb, pPyResp );

    }while( 0 );

    if( pPyCb != nullptr )
        Py_DECREF( pPyCb );

    if( pPyResp != nullptr )
        Py_DECREF( pPyResp );

    PyGILState_Release(gstate);

    return 0;
}

gint32 CPythonProxyImpl::AsyncCallVector(
    IEventSink* pTask,
    CfgPtr& pOptions,
    CfgPtr& pResp,
    const std::string& strcMethod,
    std::vector< BufPtr >& vecParams )
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

        oCfg.CopyProp( propIfName,
            ( CObjBase* )pOptions );

        std::string strMethod( strcMethod );
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
        }

        ret = this->SendProxyReq( pTask, false,
             strMethod, vecParams, qwIoTaskId ); 

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

gint32 CPythonProxyImpl::GetNumpyValue(
    gint32 iType,
    PyObject* pyInt,
    PyObject*& pNpVal )
{
    gint32 ret = 0;
    do{
        PyObject* pHost = nullptr;
        ret = GetPyHost( pHost );
        if( ERROR( ret ) )
            break;

        pNpVal =
        PyObject_CallMethod( pHost,
            "GetNumpyValue", "(iO)",
            iType, pyInt );

    }while( 0 );
    return ret;
}

gint32 CPythonProxyImpl::GetTypeId(
    PyObject* pVar )
{
    EnumTypeId ret = typeNone;
    gint32 iRet = 0;
    do{
        PyObject* pHost = nullptr;
        iRet = GetPyHost( pHost );
        if( ERROR( iRet ) )
            break;

        PyObject* pRet =
        PyObject_CallMethod( pHost,
            "GetObjType", "(O)", pVar );
        if( pRet == nullptr )
            break;

        ret = ( EnumTypeId )
            PyLong_AsLong( pRet );

    }while( 0 );

    return ret;
}

gint32 CPythonProxyImpl::ConvertPyObjToBuf(
    PyObject* pObject,
    BufPtr& pBuf )
{
    if( pObject == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    if( pBuf.IsEmpty() )
    {
        ret = pBuf.NewObj();
        if( ERROR( ret ) )
            return ret;
    }

    EnumTypeId iType =
        ( EnumTypeId )GetTypeId( pObject );

    switch( iType )
    {
    case typeUInt32:
        {
            *pBuf = ( guint32 )
                PyLong_AsLong( pObject );
            break;
        }
    case typeUInt64:
        {
            *pBuf = ( guint64 )
                PyLong_AsLong( pObject );
            break;
        }
    case typeFloat:
        {
            *pBuf = ( float )
                PyFloat_AsDouble( pObject );
            break;
        }
    case typeDouble:
        {
            *pBuf =
                PyFloat_AsDouble( pObject );
            break;
        }
    case typeByte:
        {
            *pBuf = ( guint8 )
                PyLong_AsLong( pObject );
            break;
        }
    case typeUInt16:
        {
            *pBuf = ( guint16 )
                PyLong_AsLong( pObject );
            break;
        }
    case typeString:
        {
            std::string strVal =
            convertPythonUnicodeObjectToStdString(
            pObject );
            *pBuf = strVal;
            break;
        }
    case typeByteArr:
        {
            Py_buffer *view = (Py_buffer *)
                malloc(sizeof(*view));
            ret = PyObject_GetBuffer(
                pObject, view, PyBUF_READ );
            if( ret < 0 )
            {
                ret = -ENOMEM;
                break;
            }
            pBuf->Append( ( char* )view->buf,
                ( guint32 )view->len);
            PyBuffer_Release( view );
            break;
        }
    case typeObj:
        {
            if(sipCanConvertToType( pObject,
                sipType_cpp_ObjPtr, 0 ))
            {
                gint32 iState = 0, iErr = 0;
                ObjPtr* pObj = ( ObjPtr* )
                sipConvertToType( pObject,
                    sipType_cpp_ObjPtr,
                    nullptr, 0, &iState,
                    &iErr );
                if( iErr != 0 )
                {
                    ret = ERROR_FAIL;
                    break;
                }
                *pBuf = *pObj;
            }
            else
            {
                // an empty argument
            }
            break;
        }
    default:
        ret = -ENOTSUP;
        break;
    }

    return ret;
}

gint32 CPythonProxyImpl::List2Vector(
    PyObject *pList,
    std::vector< BufPtr >& vecArgs )
{
    gint32 ret = 0;
    if( pList == nullptr )
        return -EFAULT;
    do{
        if( !PyList_Check( pList ) )
        {
            ret = -EINVAL;
            break;
        }
        gint32 iSize = PyList_Size( pList );
        if( iSize == 0 )
            break;

        for( int i = 0; i < iSize; ++i )
        {
            PyObject* pElem =
                PyList_GetItem( pList, i );

            BufPtr pBuf( true );
            ret = ConvertPyObjToBuf( pElem, pBuf );
            if( ERROR( ret ) )
                break;
            vecArgs.push_back( pBuf );
        }
        
    }while( 0 );

    if( ERROR( ret ) )
        vecArgs.clear();

    return ret;
}

gint32 CPythonProxyImpl::PyProxyCall(
    PyObject* pCb,
    const std::string& strIfName,
    const std::string& strMethod,
    PyObject* listArgs,
    PyObject* listResp,
    guint64& qwTaskCancel )
{
    gint32 ret = 0;
    do{
        bool bSync = false;
        if( pCb == nullptr || pCb == Py_None )
            bSync = true;
        if( !bSync )
        {
            ret = PyCallable_Check( pCb );
            if( ret == 0 )
            {
                ret = -EINVAL;
                break;
            }
        }
        std::vector< BufPtr > vecParams;
        ret = List2Vector(listArgs, vecParams );
        if( ERROR( ret ) )
            break;

        IEventSink* pRespCb = nullptr;
        TaskletPtr pWrapperCb;
        CParamList oReqCtx;
        if( !bSync )
        {
            ret = NEW_PROXY_RESP_HANDLER2(
                pWrapperCb, ObjPtr( this ),
                &CPythonProxyImpl::OnProxyCallResp,
                nullptr, oReqCtx.GetCfg() );

            if( ERROR( ret ) )
                break;

            oReqCtx.Push( ( intptr_t ) pCb );
            oReqCtx.Push( ( intptr_t ) listResp );
            oReqCtx.Push( strMethod );
            Py_INCREF( pCb );
            Py_INCREF( listResp );
        }
        else
        {
            ret = pWrapperCb.NewObj(
                clsid( CIoReqSyncCallback ) );
            if( ERROR( ret ) )
                break;
        }
        pRespCb = pWrapperCb;

        CCfgOpener oOptions;
        std::string strDBusIfName =
            DBUS_IF_NAME( strIfName );
        oOptions.SetStrProp(
            propIfName, strDBusIfName );
        oOptions.SetBoolProp(
            propSysMethod, false );

        CCfgOpener oResp;

        PyThreadState *_save;
        Py_UNBLOCK_THREADS;

        ret = this->AsyncCallVector(
            pRespCb, oOptions.GetCfg(),
            oResp.GetCfg(), strMethod,
            vecParams );

        Py_BLOCK_THREADS;

        if( ERROR( ret ) )
            break;

        if( ret == STATUS_PENDING && !bSync )
        {
            oResp.GetQwordProp(
                propTaskId, qwTaskCancel );
            break;
        }
        else if( ret == STATUS_PENDING )
        {
            cpp::CIoReqSyncCallback*
                pSyncCb = pWrapperCb;
            ret = pSyncCb->WaitForComplete();
        }

        if( ERROR( ret ) )
            break;

        CCfgOpenerObj oCbCfg( pRespCb );
        IConfigDb* pRmtResp = nullptr;
        ret = oCbCfg.GetPointer(
            propRespPtr, pRmtResp );
        if( ERROR( ret ) )
            break;

        guint32 iRet = 0;
        CParamList oRmtResp( pRmtResp );

        ret = oRmtResp.GetIntProp(
            propReturnValue, iRet );

        if( ERROR( ret ) )
            break;

        if( ERROR( ( gint32 )iRet ) )
        {
            ret = iRet;
            break;
        }

        ret = FillList( pRmtResp, listResp );
        if( ERROR( ret ) )
            break;

        qwTaskCancel = 0;
        
    }while( 0 );

    return ret;
}

gint32 CPythonProxyImpl::PyProxyCallSync(
    const std::string& strIfName,
    const std::string& strMethod,
    PyObject* listArgs,
    PyObject* listResp )
{
    guint64 qwTaskCancel = 0;
    return this->PyProxyCall( nullptr,
        strIfName, strMethod, listArgs,
        listResp, qwTaskCancel );
}

gint32 CPythonProxyImpl::SetPyHost(
    PyObject* pObj )
{
    gint32 ret = 0;
    Py_INCREF( pObj );
    CCfgOpenerObj oCfg( this );
    ret = oCfg.SetIntPtr( propPyObj,
        ( guint32* )pObj );
    if( ERROR( ret ) )
        Py_DECREF( pObj );
    return ret;
}

gint32 CPythonProxyImpl::GetPyHost(
    PyObject*& pObj )
{
    gint32 ret = 0;
    CCfgOpenerObj oCfg( this );
    ret = oCfg.GetIntPtr( propPyObj,
        ( guint32*& )pObj );
    return ret;
}

gint32 CPythonProxyImpl::RemovePyHost()
{
    guint32* pObj = nullptr;
    DebugPrint( 0, "RemovePyHost()..." );
    CCfgOpenerObj oCfg( this );
    gint32 ret = oCfg.GetIntPtr(
        propPyObj, pObj );
    if( ERROR( ret ) )
        return ret;
    oCfg.RemoveProperty( propPyObj );
    PyObject* pPyObj = ( PyObject* )pObj;
    if( pPyObj != nullptr )
         Py_DECREF( pPyObj );
    return ret;
}

gint32 CPythonProxyImpl::InvokeUserMethod(
    IConfigDb* pParams,
    IEventSink* pCallback )
{
    if( pParams == nullptr )
        return -EINVAL;

    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    CParamList oResp;
    gint32 ret = 0;
    PyObject* listResp = nullptr;

    PyObject* pPyCb = nullptr;
    PyObject* pPyParams = nullptr;

    do{
        CReqOpener oReq( pParams );
        std::string strMethod;
        ret = oReq.GetMethodName( strMethod );
        if( ERROR( ret ) )
            break;

        TaskletPtr pTask;

        CCfgOpenerObj oCfg( pCallback );
        std::string strIfName;

        ret = oCfg.GetStrProp(
            propIfName, strIfName );
        if( ERROR( ret ) ) 
            break;
        
        strIfName = IF_NAME_FROM_DBUS(
            strIfName );

        PyObject* pHost = nullptr;
        ret = GetPyHost( pHost );
        if( ERROR( ret ) )
            break;

        ObjPtr* pCb = new ObjPtr( pCallback );
        ObjPtr* pArgs = new ObjPtr( pParams );

        pPyCb = sipConvertFromNewType(
            pCb, sipType_cpp_ObjPtr, SIP_NULLPTR );
        if( unlikely( pPyCb == nullptr ) )
        {
            ret = -ENOMEM;
            break;
        }
        pPyParams = sipConvertFromNewType(
            pArgs, sipType_cpp_ObjPtr, SIP_NULLPTR );
        if( unlikely( pPyParams == nullptr ) ) 
        {           
            ret = -ENOMEM;
            break;
        }       

        listResp = PyObject_CallMethod(
             pHost, "InvokeMethod", 
             "(OssO)", pPyCb, strIfName.c_str(),
             strMethod.c_str(), pPyParams );

        if( unlikely( listResp == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }

        if( PyList_Check( listResp ) == false ||
            PyList_Size( listResp ) == 0 )
        {
            ret = ERROR_FAIL;
            break;
        }

        PyObject* pyRet =
            PyList_GetItem( listResp, 0 );
        if( unlikely( pyRet == nullptr ) )
        {
            ret = ERROR_FAIL;
            break;
        }

        ret = PyLong_AS_LONG( pyRet );
        if( ret == STATUS_PENDING )
        {
            break;
        }

        if( ERROR( ret ) ) 
            break;

        PyObject* pyRespArgs =
            PyList_GetItem( listResp, 1 );
        if( pyRespArgs == nullptr )
        {
            // fine, no resp params
            break;
        }

        if( PyList_Check( pyRespArgs ) == false ||
            PyList_Size( pyRespArgs ) == 0 )
            break;

        std::vector< BufPtr > vecResp;
        ret = List2Vector( pyRespArgs, vecResp );
        if( ERROR( ret ) )
            break;

        if( vecResp.empty() )
            break;

        for( auto elem : vecResp )
            oResp.Push( elem );

    }while( 0 );
    if( listResp != nullptr )
        Py_DECREF( listResp );

    if( ret != STATUS_PENDING )
    {
        oResp[ propReturnValue ] = ret;
        CCfgOpenerObj oCfg( pCallback );
        oCfg.SetPointer( propRespPtr,
            ( IConfigDb* )oResp.GetCfg() );
        Py_DECREF( pPyCb );
        Py_DECREF( pPyParams );
    }

    PyGILState_Release(gstate);

    return ret;
}

gint32 CPythonProxyImpl::OnReadStreamComplete(
    HANDLE hChannel,
    gint32 iRet,
    BufPtr& pBuf,
    IConfigDb* pCtx )
{
    if( pCtx == nullptr )
        return -EINVAL;
    CParamList oReqCtx( pCtx );
    guint32* pCb = nullptr;
    gint32 ret =
        oReqCtx.GetIntPtr( 0, pCb );

    if( ERROR( ret ) || pCb == nullptr )
        return ret;

    PyObject* pPyCb = nullptr;
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();
    do{
        PyObject* pPyResp = PyList_New( 2 );

        PyObject* pRet = PyLong_FromLong( iRet );
        PyList_SET_ITEM( pPyResp, 0, pRet );
        PyObject* pArgs = PyList_New( 1 );
        PyList_SET_ITEM( pPyResp, 1, pArgs );

        PyObject* pPyBuf = nullptr;
        ret = ConvertBufToPyObj( pBuf, pPyBuf );
        if( ERROR( ret ) )
        {
            pRet = PyLong_FromLong( ret );
            PyList_SET_ITEM( pPyResp, 0, pRet );
        }
        else
        {
            PyList_SET_ITEM( pArgs, 0, pPyBuf );
        }

        pPyCb = ( PyObject* )pCb;

        PyObject* pHost = nullptr;
        ret = GetPyHost( pHost );
        if( ERROR( ret ) )
            break;

        PyObject_CallMethod(
             pHost, "HandleAsyncResp", 
             "(OO)", pPyCb, pPyResp );

    }while( 0 );

    if( pPyCb != nullptr )
        Py_DECREF( pPyCb );

    PyGILState_Release( gstate );
    return ret;
}

gint32 CPythonProxyImpl::OnWriteStreamComplete(
    HANDLE hChannel,
    gint32 iRet,
    BufPtr& pBuf,
    IConfigDb* pCtx )
{
    if( pCtx == nullptr )
        return -EINVAL;
    CParamList oReqCtx( pCtx );
    PyObject* pPyBuf = nullptr; 
    PyObject* pPyCb = nullptr;
    gint32 ret = 0;
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();
    do{
        guint32* pVal = nullptr;
        ret = oReqCtx.GetIntPtr( 0, pVal );
        if( ERROR( ret ) )
            break;
        pPyBuf = ( PyObject* )pVal;

        ret = oReqCtx.GetIntPtr( 1, pVal );
        if( ERROR( ret ) )
            break;
        pPyCb = ( PyObject* )pVal;

        PyObject* pPyResp = PyList_New( 2 );
        PyObject* pPyArgs = PyList_New( 0 );

        PyObject* pRet = PyLong_FromLong( iRet );
        PyList_SET_ITEM( pPyResp, 0, pRet );

        if( SUCCEEDED( iRet ) )
            PyList_SET_ITEM( pPyArgs, 0, pPyBuf );

        PyList_SET_ITEM( pPyResp, 1, pPyArgs );

        PyObject* pHost = nullptr;
        ret = GetPyHost( pHost );
        if( ERROR( ret ) )
            break;

        PyObject_CallMethod(
             pHost, "HandleAsyncResp", 
             "(OO)", pPyCb, pPyResp );

    }while( 0 );

    if( pPyBuf != nullptr )
        Py_DECREF( pPyBuf );

    if( pPyCb != nullptr )
        Py_DECREF( pPyCb );

    PyGILState_Release( gstate );
    return ret;
}

static FactoryPtr InitClassFactory()
{
    BEGIN_FACTORY_MAPS;
    INIT_MAP_ENTRYCFG( CPythonProxyImpl );
    END_FACTORY_MAPS;
};

extern "C"
gint32 DllLoadFactory( FactoryPtr& pFactory )
{
    pFactory = InitClassFactory();
    if( pFactory.IsEmpty() )
        return -EFAULT;

    return 0;
}

gint32 LoadThisLib( ObjPtr& pIoMgr )
{
    gint32 ret = 0;
    if( pIoMgr.IsEmpty() )
        return -EINVAL;
    do{
        std::string strResult;
        const char* szLib = "rpcfrmwrk.cpython";
        ret = GetLibPathName( strResult, szLib );
        if( ERROR( ret ) )
            break;
        CIoManager* pMgr = pIoMgr;
        if( pMgr == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ret = pMgr->TryLoadClassFactory(
            strResult );

    }while( 0 );
    return ret;
}
%End

enum EnumMyClsid
{
    Clsid_CStreamingServer,
    Clsid_CStreamingClient,
    Iid_CEchoServer
};

class CPythonProxy :
    public cpp::CInterfaceProxy /Abstract/
{
    public:
    %TypeHeaderCode
    #include "stmsvr.h"
    %End

    gint32 PyProxyCall(
        PyObject* pCb /In/,
        const std::string& strIfName /In/,
        const std::string& strMethod /In/,
        PyObject* listArgs /In/,
        PyObject* listResp,
        guint64& qwTaskCancel /Out/ );

    gint32 PyProxyCallSync(
        const std::string& strIfName /In/,
        const std::string& strMethod /In/,
        PyObject* listArgs /In/,
        PyObject* listResp );
    %MethodCode
        guint64 qwTaskCancel = 0;
        sipRes = sipCpp->PyProxyCall( Py_None,
            *a0, *a1, a2, a3, qwTaskCancel );
    %End

    gint32 SetPyHost( PyObject* pObj );
    %MethodCode
        CPythonProxyImpl* pImpl = static_cast
            < CPythonProxyImpl* >( sipCpp );
        if( pImpl == nullptr ) 
            sipRes = -EFAULT;
        else
            sipRes = pImpl->SetPyHost( a0 );
    %End

    PyObject* GetPyHost();
    %MethodCode
        CPythonProxyImpl* pImpl = static_cast
            < CPythonProxyImpl* >( sipCpp );
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
        CPythonProxyImpl* pImpl = static_cast
            < CPythonProxyImpl* >( sipCpp );
        if( pImpl == nullptr ) 
            sipRes = -EFAULT;
        else
            sipRes = pImpl->RemovePyHost();
    %End

    gint32 StartStream( cpp::ObjPtr pObj,
        HANDLE& hChannel /Out/ );
    %MethodCode
        Py_BEGIN_ALLOW_THREADS;
        CPythonProxyImpl* pImpl = static_cast
            < CPythonProxyImpl* >( sipCpp );
        if( pImpl == nullptr ) 
            sipRes = -EFAULT;
        else
        {
            IConfigDb* pDesc = *a0;
            sipRes = pImpl->
                StartStream( a1, pDesc );
        }
        Py_END_ALLOW_THREADS;
    %End

    gint32 WriteStream(
        HANDLE hChannel, PyObject* pPyBuf );
    %MethodCode
    Py_BEGIN_ALLOW_THREADS;
    do{
        CPythonProxyImpl* pImpl = static_cast
            < CPythonProxyImpl* >( sipCpp );

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
        sipRes = pImpl->
            WriteStream( a0, pBuf );

    }while( 0 );
    Py_END_ALLOW_THREADS;
    %End

    PyObject* ReadStream( HANDLE hChannel );
    %MethodCode
    gint32 ret = 0;
    Py_BEGIN_ALLOW_THREADS;
    do{
        CPythonProxyImpl* pImpl = static_cast
            < CPythonProxyImpl* >( sipCpp );

        if( pImpl == nullptr ) 
        {
            ret = -EFAULT;
            break;
        }

        BufPtr* ppBuf = new BufPtr() ;
        ret = pImpl->ReadStream( a0, *ppBuf );
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
    Py_END_ALLOW_THREADS;
    %End

    gint32 ReadStreamAsync(
        HANDLE hChannel, PyObject* pCb );
    %MethodCode
    Py_BEGIN_ALLOW_THREADS;
    do{
        sipRes = 0;
        CPythonProxyImpl* pImpl = static_cast
            < CPythonProxyImpl* >( sipCpp );

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
        sipRes = pImpl->ReadStreamAsync(
            a0, pBuf, pCtx );

        break;

    }while( 0 );

    Py_END_ALLOW_THREADS;
    %End

    gint32 WriteStreamAsync(
        HANDLE hChannel,
        PyObject* pPyBuf,
        PyObject* pCb );
    %MethodCode
    Py_BEGIN_ALLOW_THREADS;
    do{
        sipRes = 0;
        CPythonProxyImpl* pImpl = static_cast
            < CPythonProxyImpl* >( sipCpp );

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
        sipRes = pImpl->WriteStreamAsync(
            a0, pBuf, pCtx );

        break;

    }while( 0 );
    Py_END_ALLOW_THREADS;

    %End
};

cpp::ObjPtr CreateProxy(
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
            *a1, *a2, false, pCfg );
        if( ERROR( ret ) )
            break;

        ObjPtr pIf;
        ret = pIf.NewObj(
            clsid( CPythonProxyImpl ),
            pCfg );
        if( ERROR( ret ) )
            break;

        sipRes = new cpp::ObjPtr(
            nullptr, false );
        *sipRes = pIf;

    }while( 0 );
%End

gint32 LoadPyFactory( cpp::ObjPtr& pMgr );
%MethodCode
    sipRes = LoadThisLib( *a0 );
%End

CPythonProxy* CastToProxy(
    cpp::ObjPtr& pObj );
%MethodCode
    CPythonProxy* pProxy = *a0;
    sipRes = pProxy;
%End
