%header {
typedef enum 
{
    // a pointer to pyobject of the python context
    propJavaVM = propReservedEnd + 120,

} EnumJavaPropId;

#include "streamex.h"
#include "counters.h"

class CJavaProxyBase :
    public CStreamProxyAsync
{
    public:
    typedef CStreamProxyAsync super;
    CJavaProxyBase( const IConfigDb* pCfg ) :
        super::_MyVirtBase( pCfg ), super( pCfg )
    {}

    virtual jobject JavaProxyCall2(
        jobject pCb,
        jobject pContext,
        CfgPtr& pOptions,
        jobject listArgs,
        jobject listResp )
    { return nullptr; }
};

DECLARE_AGGREGATED_PROXY(
    CJavaProxy,
    CStatCountersProxy,
    CJavaProxyBase );

DECLARE_AGGREGATED_SERVER(
    CJavaServer,
    CStatCountersServer,
    CStreamServerAsync );

template< class T >
class CJavaInterfBase : public T
{
    public:
    typedef T super;
    CJavaInterfBase( const IConfigDb* pCfg ) 
        : super::_MyVirtBase( pCfg ), super( pCfg )
    {}

    gint32 ConvertBufToByteArray( JNIEnv* jenv,
        BufPtr& pBuf, jobject& pObj )
    {
        gint32 ret = 0;
        if( pBuf.IsEmpty() || pBuf->empty() )
            return -EINVAL;

        if( jenv == nullptr )
            return -EINVAL;

        do{
            EnumTypeId iType =
                pBuf->GetExDataType();
            if( iType != typeByteArr )
            {
                ret = -EINVAL;
                break;
            }

            jbyteArray bytes = (*jenv)->
                NewByteArray( pBuf->size() );

            ( *jenv )->SetByteArrayRegion(
                jenv, bytes, 0, pBuf->size(),
                pBuf->ptr() );

            pObj = bytes;

        }while( 0 );

        return ret;
    }

    gint32 TimerCallback(
        IEventSink* pCallback,
        intptr_t pCb,
        intptr_t pCtx )
    {
        if( pCb == 0 )
            return -EINVAL;

        gint32 ret = 0;
        bool bAttach = false;
        JNIEnv* jenv = nullptr;
        ret = GetJavaEnv( jenv, bAttach );
        if( ERROR( ret ) )
            return ret;
        do{
            jobject pHost = nullptr;
            ret = GetJavaHost( jenv, pHost );
            if( ERROR( ret ) )
                break;

            jobject* pgcb = ( jobject )pCb;
            jobject* pgctx = ( jobject )pCtx;

            ( *jenv )->CallVoidMethod( pHost,
                "TimerCallback",
                "(Ljava/lang/Object;Ljava/lang/Object;)V",
                pgcb, pgtx );

            if( pgcb != nullptr )
                ( *jenv )->DeleteGlobalRef( pgcb );

            if( pgctx != nullptr )
                ( *jenv )->DeleteGlobalRef( pgctx );

        }while( 0 );

        if( bAttach )
            PutJavaEnv();

        return ret;
    }

    gint32 DisableTimer(
        JNIEnv* jenv, ObjPtr& pTimer )
    {
        CIfDeferCallTaskEx* pTaskEx = pTimer;
        if( pTaskEx == nullptr )
            return -EINVAL;
        jobject* pCb = nullptr;
        jobject* pCtx = nullptr;
        gint32 ret = 0;

        do{
            ret = pTaskEx->DisableTimer();

            CStdRTMutex oTaskLock(
                pTaskEx->GetLock() );

            BufPtr pBuf;
            ret = pTaskEx->GetParamAt( 1, pBuf );
            if( ERROR( ret ) )
                break;

            intptr_t ptrval = ( intptr_t& )*pBuf;
            if( ptrval == 0 )
                break;

            pCb = ( jobject )ptrval;
            ret = pTaskEx->GetParamAt( 2, pBuf );
            if( ERROR( ret ) )
                break;

            ptrval = ( intptr_t& )*pBuf;
            if( ptrval == 0 )
                break;

            pCtx = ( jobject ) ptrval ;

        }while( 0 );

        if( pCb != nullptr && jenv != nullptr )
            ( *jenv )->DeleteGlobalRef( pCb );

        if( pCtx != nullptr && jenv != nullptr )
            ( *jenv )->DeleteGlobalRef( pCtx );

        ( *pTaskEx )( eventCancelTask );
        return ret;
    }

    gint32 AddTimer(
        JNIEnv* jenv,
        guint32 dwTimeoutSec,
        jobject pCb,
        jobject pCtx,
        ObjPtr& pTimer )
    {
        if( pCb == nullptr || jenv == nullptr )
            return -EINVAL;

        if( dwTimeoutSec > MAX_TIMEOUT_VALUE )
            return -EINVAL;

        gint32 ret = 0;
        jobject pgcb = nullptr;
        jobject pgctx = nullptr;
        do{
            TaskletPtr pTask;
            pgcb =
                ( *jenv )->NewGlobalRef( pCb );

            if( pCtx != nullptr )
            {
                pgctx = ( *jenv )->
                    NewGlobalRef( pCtx );
            }

            ret = DEFER_IFCALLEX_NOSCHED2(
                0, pTask, ObjPtr( this ),
                &CJavaInterfBase::TimerCallback,
                nullptr, ( intptr_t )pgcb,
                ( intptr_t )pgctx );

            if( ERROR( ret ) )
                break;

            CIfDeferCallTaskEx* pTaskEx = pTask;
            ret = pTaskEx->EnableTimer(
                dwTimeoutSec, eventRetry );
            if( ERROR( ret ) )
                 break;

            pTimer = pTask;

        }while( 0 );

        if( ERROR( ret ) )
        {
            if( pgcb )
                ( *jenv )->DeleteGlobalRef( pgcb );
            if( pgctx )
                ( *jenv )->DeleteGlobalRef( pgctx );
        }

        return ret;
    }

    gint32 FillList( IConfigDb* pResp,
        PyObject* listResp )
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
                        PyList_SetItem( listArgs, i, pObj );
                        break;
                    }
                case typeUInt16:
                    {
                        guint16 val = 0;
                        ret = oResp.GetShortProp( i, val );
                        if( ERROR( ret ) )
                            break;
                        PyObject* pObj = PyLong_FromLong( val );
                        PyList_SetItem( listArgs, i, pObj );
                        break;
                    }
                case typeUInt32:
                    {
                        guint32 val = 0;
                        ret = oResp.GetIntProp( i, val );
                        if( ERROR( ret ) )
                            break;
                        PyObject* pObj = PyLong_FromLong( val );
                        PyList_SetItem( listArgs, i, pObj );
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
                        PyList_SetItem( listArgs, i, pObj );
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
                        PyList_SetItem( listArgs, i, pObj );
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
                        PyList_SetItem( listArgs, i, pObj );
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
                        PyList_SetItem( listArgs, i, pObj );
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
                        PyList_SetItem( listArgs, i, pObj );
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
                        Py_DECREF( pView );
                        if( pByteArr == nullptr )
                        {
                            ret = -EFAULT;
                            break;
                        }
                        PyList_SetItem( listArgs, i, pByteArr );
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

    gint32 GetTypeId( PyObject* pVar )
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

    gint32 ConvertPyObjToBuf(
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
                convertJavaUnicodeObjectToStdString(
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
                ret = pBuf->Append( ( char* )view->buf,
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

    gint32 List2Vector(
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

    gint32 SetJavaHost(
        JNIEnv* jenv, jobject pObj )
    {
        gint32 ret = 0;
        if( jenv == nullptr || pObj == nullptr )
            return -EINVAL;
        do{
            Java_VM* pvm;
            ( *jenv )->GetJavaVM( &pvm );
            jobject pgobj =
                ( *jenv )->NewGlobalRef( pObj );

            CCfgOpenerObj oCfg( this );
            oCfg.SetIntPtr(
                propJavaObj, ( guint32* )pgobj );
            oCfg.SetIntPtr(
                propJavaVM, ( guint32* )pvm );

        }while( 0 );

        return ret;
    }

    gint32 GetJavaHost(
        JNIEnv* jenv, jobject& pObj )
    {
        gint32 ret = 0;
        CCfgOpenerObj oCfg( this );
        ret = oCfg.GetIntPtr( propJavaObj,
            ( guint32*& )pObj );
        return ret;
    }

    gint32 GetVmPtr( Java_VM* pvm )
    {
        guint32* pVal = nullptr;
        CCfgOpenerObj oCfg( this );
        gint32 ret = oCfg.GetIntPtr(
            propJavaVM, pVal );
        if( ERROR( ret ) )
            return ret;
        pvm = ( Java_VM* )pVal );
        return STATUS_SUCCESS;
    }

    gint32 GetJavaEnv(
        JNIEnv*& jenv, bool& bAttach )
    {
        Java_VM pvm = nullptr;
        gint32 ret = 0;
        bAttach = false;
        do{
            ret = GetVmPtr( pvm );
            if( ERROR( ret ) )
                break;

            ret = pvm->GetEnv(
                (void **)&jenv, JNI_VERSION_1_6);

            if (ret == JNI_EDETACHED)
            {
                if( pvm->AttachCurrentThread(
                    &jenv, NULL )!= 0 )
                {
                    ret = ERROR_FAIL;
                    break;
                }
                bAttach = true;
                ret = STATUS_SUCCESS;
            }
            else if (ret == JNI_OK)
            {
                ret = STATUS_SUCCESS;
            }
            else if (ret == JNI_EVERSION)
            {
                ret = -EINVAL;
                break;
            }

        }while( 0 );

        return ret;
    }

    gint32 PutJavaEnv()
    {
        Java_VM pvm = nullptr;
        gint32 ret = STATUS_SUCCESS;
        do{
            ret = GetVmPtr( pvm );
            if( ERROR( ret ) )
                break;
            pvm->DetachCurrentThread();
        }while( 0 );

        return ret;
    }

    gint32 RemoveJavaHost()
    {
        gint32 ret = 0;
        bool bAttach = false;
        CCfgOpenerObj oCfg( this );
        do{
            JNIEnv* e = nullptr;
            ret = GetJavaEnv( e, bAttach );
            if( ERROR( ret ) )
                break;
            guint32* pObj = nullptr;
            DebugPrint( 0, "RemoveJavaHost()..." );
            ret = oCfg.GetIntPtr(
                propJavaObj, pObj );
            if( ERROR( ret ) )
                break;
            oCfg.RemoveProperty( propJavaObj );
            jobject pjObj = ( jobject )pObj;
            if( pjObj != nullptr )
                 ( *e )->DeleteGlobalRef( pjObj );

        }while( 0 );

        if( bAttach )
            PutJavaEnv();

        oCfg.RemoveProperty( propJavaVM );

        return ret;
    }

    gint32 InvokeUserMethod(
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
        bool bNoReply = false;
        guint32 dwSeriProto = seriNone;

        do{
            CReqOpener oReq( pParams );
            std::string strMethod;
            ret = oReq.GetMethodName( strMethod );
            if( ERROR( ret ) )
                break;

            ret = oReq.GetIntProp(
                propSeriProto, dwSeriProto );
            if( ERROR( ret ) )
                dwSeriProto = seriNone;

            guint32 dwCallFlags = 0;
            ret = oReq.GetCallFlags( dwCallFlags );
            if( SUCCEEDED( ret ) )
            {
                if( !( dwCallFlags & CF_WITH_REPLY ) )
                    bNoReply = true;
            }

            ret = 0;
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
            {
                ret = -ENOTSUP;
                break;
            }

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
                 "(OssiO)", pPyCb, strIfName.c_str(),
                 strMethod.c_str(), dwSeriProto,
                 pPyParams );

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
                break;

            if( ERROR( ret ) || bNoReply ) 
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

            oResp[ propSeriProto ] = dwSeriProto;

        }while( 0 );

        if( listResp != nullptr )
            Py_DECREF( listResp );

        if( pPyCb != nullptr )
            Py_DECREF( pPyCb );

        if( pPyParams != nullptr )
            Py_DECREF( pPyParams );

        PyGILState_Release(gstate);

        if( ret != STATUS_PENDING &&
            ret != -ENOTSUP)
        {
            if( bNoReply )
                return ret;

            oResp[ propReturnValue ] = ret;
            oResp[ propSeriProto ] = dwSeriProto;
            CCfgOpenerObj oCfg( pCallback );
            oCfg.SetPointer( propRespPtr,
                ( IConfigDb* )oResp.GetCfg() );
        }
        else if( ret == -ENOTSUP )
        {
            return super::InvokeUserMethod(
                pParams, pCallback );
        }

        return ret;
    }

    gint32 OnReadStreamComplete(
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
            PyList_SetItem( pPyResp, 0, pRet );
            PyObject* pArgs = PyList_New( 2 );

            PyObject* pPyBuf = nullptr;
            ret = ConvertBufToPyObj( pBuf, pPyBuf );
            if( SUCCEEDED( ret ) )
            {
                PyObject* pyHandle = nullptr;
                if( sizeof( HANDLE ) ==
                    sizeof( gint64 ) )
                {
                    pyHandle =
                        PyLong_FromLongLong( hChannel );
                }
                else
                {
                    pyHandle =
                        PyLong_FromLong( hChannel );
                }

                // Note: callback dose not to worry 
                // the pPyBuf become invalid if pBuf is released.
                // so we don't put pBuf to the list here.
                // but do pay attention to consume the pPyBuf
                // with the call. It would become invalid when
                // the callback returns
                PyList_SetItem( pArgs, 0, pyHandle );
                PyList_SetItem( pArgs, 1, pPyBuf );
                PyList_SetItem( pPyResp, 1, pArgs );
            }
            else
            {
                if( SUCCEEDED( iRet ) )
                    PyList_SetItem( pPyResp,
                        0, PyLong_FromLong( ret ) );
                PyList_SetItem( pPyResp, 1, Py_None );
            }

            pPyCb = ( PyObject* )pCb;

            PyObject* pHost = nullptr;
            ret = GetPyHost( pHost );
            if( ERROR( ret ) )
                break;

            PyObject_CallMethod(
                 pHost, "HandleAsyncResp", 
                 "(OiOO)", pPyCb, seriNone,
                 pPyResp, Py_None );

        }while( 0 );

        if( pPyCb != nullptr )
            Py_DECREF( pPyCb );

        PyGILState_Release( gstate );
        return ret;
    }

    gint32 OnWriteStreamComplete(
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
            PyObject* pPyArgs = nullptr; 

            PyObject* pRet = PyLong_FromLong( iRet );
            PyList_SetItem( pPyResp, 0, pRet );

            if( SUCCEEDED( iRet ) )
            {
                pPyArgs = PyList_New( 2 );
                PyObject* pyHandle = nullptr;
                if( sizeof( HANDLE ) ==
                    sizeof( gint64 ) )
                {
                    pyHandle =
                        PyLong_FromLongLong( hChannel );
                }
                else
                {
                    pyHandle =
                        PyLong_FromLong( hChannel );
                }

                PyList_SetItem( pPyArgs, 0, pyHandle );
                PyList_SetItem( pPyArgs, 1, pPyBuf );
                pPyBuf = nullptr;
            }
            else
            {
                pPyArgs = PyList_New( 0 );
            }

            PyList_SetItem( pPyResp, 1, pPyArgs );

            PyObject* pHost = nullptr;
            ret = GetPyHost( pHost );
            if( ERROR( ret ) )
                break;

            PyObject_CallMethod(
                 pHost, "HandleAsyncResp", 
                 "(OiOO)", pPyCb, seriNone,
                 pPyResp, Py_None );

        }while( 0 );

        if( pPyBuf != nullptr )
            Py_DECREF( pPyBuf );

        if( pPyCb != nullptr )
            Py_DECREF( pPyCb );

        PyGILState_Release( gstate );
        return ret;
    }

    gint32 OnAsyncCallResp(
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
        PyObject* pContext = nullptr;

        do{
            CCfgOpenerObj oCbCfg( pIoReq );
            IConfigDb* pResp = nullptr;
            ret = oCbCfg.GetPointer(
                propRespPtr, pResp );
            if( ERROR( ret ) )
                break;

            guint32* ptrVal = nullptr;
            guint32 dwReqProto = seriNone;

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

            oReqCtx.GetIntProp( 3, dwReqProto );

            ret = oReqCtx.GetIntPtr( 4, ptrVal );
            if( ERROR( ret ) )
                pContext = Py_None;
            else
                pContext = ( PyObject* )ptrVal;

            guint32 iRet = 0;
            CParamList oResp( pResp );
            ret = oResp.GetIntProp(
                propReturnValue, iRet );

            if( SUCCEEDED( ret ) &&
                ERROR( ( gint32 )iRet ) )
                ret = iRet;

            guint32 dwSeriProto = seriNone;
            if( ERROR( ret ) )
            {
                PyObject* pRet =
                    PyLong_FromLong( ret );
                PyList_SetItem( pPyResp, 0, pRet );
            }
            else
            {
                ret = oResp.GetIntProp(
                    propSeriProto, dwSeriProto );
                if( ERROR( ret ) )
                    dwSeriProto = seriNone;

                ret = 0;
                if( dwSeriProto != dwReqProto )
                    ret = -EBADMSG;

                PyObject* pRet = PyLong_FromLong( ret );
                PyList_SetItem( pPyResp, 0, pRet );
                if( SUCCEEDED( ret ) )
                    FillList( pResp, pPyResp );
            }

            PyObject* pHost = nullptr;
            ret = GetPyHost( pHost );
            if( ERROR( ret ) )
                break;

            PyObject_CallMethod( pHost,
                "HandleAsyncResp", "(OiOO)",
                pPyCb, dwSeriProto,
                pPyResp, pContext );

        }while( 0 );

        if( pPyCb != nullptr )
            Py_DECREF( pPyCb );

        if( pPyResp != nullptr )
            Py_DECREF( pPyResp );

        if( pContext != nullptr )
            Py_DECREF( pContext );

        PyGILState_Release(gstate);

        return 0;
    }
 
    PyObject* GetChanCtx( HANDLE hChannel )
    {
        PyObject* sipRes = Py_None;
        do{
            if( hChannel == INVALID_HANDLE )
                break;
            CfgPtr pCtx;
            gint32 ret = this->GetContext(
                hChannel, pCtx );
            if( ERROR( ret ) )
                break;
            CCfgOpener oCfg( ( IConfigDb* )pCtx );
            guint32* ptr = nullptr;
            ret = oCfg.GetIntPtr( propChanCtx, ptr );
            if( ERROR( ret ) )
                break;
            sipRes = ( PyObject* )ptr;
            Py_INCREF( sipRes );

        }while( 0 );

        return sipRes;
    }

    gint32 SetChanCtx(
        HANDLE hChannel, PyObject* pCtx )
    {
        gint32 ret = 0;
        do{
            if( hChannel == INVALID_HANDLE )
                break;

            CfgPtr pStmCtx;
            ret = this->GetContext(
                hChannel, pStmCtx );

            if( ERROR( ret ) )
                break;

            CCfgOpener oCfg(
               ( IConfigDb* )pStmCtx );

            guint32* ptr = nullptr;
            ret = oCfg.GetIntPtr(
                propChanCtx, ptr );

            if( SUCCEEDED( ret ) )
            {
                PyObject* pOld = ( PyObject* )ptr;
                if( pOld == pCtx )
                    break;
                if( pOld != nullptr &&
                    pOld != Py_None )
                    Py_DECREF( pOld );
                oCfg.RemoveProperty( propChanCtx );
            }
            ret = 0;

            if( pCtx == nullptr ||
                pCtx == Py_None )
                break;

            oCfg.SetIntPtr(
                propChanCtx, ( guint32*)pCtx );
            Py_INCREF( pCtx );

        }while( 0 );
        return ret;
    }

    gint32 RemoveChanCtx( HANDLE hChannel )
    {
        return SetChanCtx( hChannel, nullptr);
    }

    gint32 PyCompNotify(
        IEventSink* pCallback,
        gint32 iRet,
        intptr_t pCb,
        intptr_t pResp )
    {
        CCfgOpenerObj oCfg( pCallback );
        ObjPtr pObj;

        if( pCb == 0 || pResp == 0 )
            return -EINVAL;

        PyGILState_STATE gstate;
        gstate = PyGILState_Ensure();

        gint32 ret = 0;

        if( SUCCEEDED( iRet ) )
        {
            ret = oCfg.GetObjPtr(
                propRespPtr, pObj ); 
        }

        PyObject* pPyCb = ( PyObject*)pCb;
        PyObject* pPyResp = ( PyObject*)pResp;

        do{
            ret = iRet;
            if( !pObj.IsEmpty() )
            {
                CCfgOpener oResp(
                    ( IConfigDb* )pObj );
                guint32 dwRet = 0;
                ret = oResp.GetIntProp(
                    propReturnValue, dwRet );
                if( SUCCEEDED( ret ) )
                    ret = ( gint32 )dwRet;
                else
                    ret = iRet;
            }

            PyObject* pHost = nullptr;
            GetPyHost( pHost );

            PyObject* pPyRet =
                PyLong_FromLong( ret );

            PyList_SetItem( pPyResp, 0, pPyRet );
            // element 1 is not replaced with the
            // the true response parameters.
            PyObject_CallMethod(
                 pHost, "HandleAsyncResp", 
                 "(OiOO)", pPyCb, seriNone,
                 pPyResp, Py_None );

        }while( 0 );

        if( pPyCb )
            Py_DECREF( pPyCb );
        if( pPyResp )
            Py_DECREF( pPyResp );

        PyGILState_Release(gstate);

        return ret;
    }

    gint32 InstallCompNotify(
        IEventSink* pCallback,
        PyObject* pCb,
        PyObject* pListResp )
    {
        if( pCb == nullptr ||
            pListResp == nullptr )
            return -EINVAL;

        TaskletPtr pTask;
        gint32 ret = DEFER_CANCEL_HANDLER2(
            0, pTask, this,
            &CJavaInterfBase::PyCompNotify,
            pCallback, 0,
            ( intptr_t )pCb,
            ( intptr_t)pListResp );    

        if( SUCCEEDED( ret ) )
        {
            Py_INCREF( pCb );
            Py_INCREF( pListResp );
        }
        return ret;
    }

    gint32 OnStreamReady( HANDLE hChannel )
    {
        gint32 ret = 0;
        PyGILState_STATE gstate;
        gstate = PyGILState_Ensure();
        do{
            PyObject* pHost = nullptr;
            ret = GetPyHost( pHost );
            if( ERROR( ret ) )
                break;

            std::string strFmt = "(L)";
            if( sizeof( hChannel ) ==
                sizeof( guint32 ) )
                strFmt = "(i)";

            PyObject_CallMethod(
                 pHost, "OnStmReady", 
                 strFmt.c_str(), hChannel );
        }while( 0 );
        PyGILState_Release( gstate );
        return ret;
    }

    gint32 OnStmClosing( HANDLE hChannel )
    {
        if( hChannel == INVALID_HANDLE )
            return -EINVAL;

        PyGILState_STATE gstate;
        gstate = PyGILState_Ensure();
        PyObject* pCtx = GetChanCtx( hChannel );

        do{
            if( pCtx == nullptr )
                break;

            PyObject* pHost = nullptr;
            gint32 ret = GetPyHost( pHost );
            if( ERROR( ret ) )
                break;

            std::string strFmt = "(L)";
            if( sizeof( hChannel ) ==
                sizeof( guint32 ) )
                strFmt = "(i)";

            PyObject_CallMethod(
                 pHost, "OnStmClosing", 
                 strFmt.c_str(), hChannel );

        }while( 0 );

        if( pCtx != nullptr &&
            pCtx != Py_None )
        {
            Py_DECREF( pCtx );
            RemoveChanCtx( hChannel );
        }

        PyGILState_Release( gstate );

        return super::OnStmClosing( hChannel );
    }

    gint32 PyDeferCallback(
        intptr_t pCb, intptr_t pArgs )
    {
        gint32 ret = 0;
        PyGILState_STATE gstate;
        gstate = PyGILState_Ensure();
        PyObject* pPyCb = ( PyObject* )pCb;
        PyObject* pPyArgs = ( PyObject* )pArgs;
        do{
            PyObject* pHost = nullptr;
            ret = GetPyHost( pHost );
            if( ERROR( ret ) )
                break;

            PyObject_CallMethod(
                 pHost, "DeferCallback", 
                 "OO", pPyCb, pPyArgs );

        }while( 0 );
        if( pPyCb != nullptr &&
            pPyCb != Py_None )
            Py_DECREF( pPyCb );

        if( pPyArgs != nullptr &&
            pPyArgs != Py_None )
            Py_DECREF( pPyArgs );

        PyGILState_Release( gstate );
        return ret;
    }

    gint32 PyDeferCall(
        PyObject* pPyCb, PyObject* pPyArgs )
    {
        TaskletPtr pTask;
        gint32 ret = DEFER_IFCALLEX_NOSCHED(
            pTask, ObjPtr( this ),
            &CJavaInterfBase::PyDeferCallback,
            ( intptr_t )pPyCb,
            ( intptr_t )pPyArgs );
        if( ERROR( ret ) )
            return ret;
        ret = this->GetIoMgr()->
            RescheduleTask( pTask );
        if( ERROR( ret ) )
        {
            ( *pTask )( eventCancelTask );
            return ret;
        }
        // NOTE: if the task is canceled,
        // the reference won't be released
        Py_INCREF( pPyCb );
        Py_INCREF( pPyArgs );
        return ret;
    }
};

class CJavaProxyImpl :
    public CJavaInterfBase< CJavaProxy >
{
    public:
    typedef CJavaInterfBase< CJavaProxy > super;
    CJavaProxyImpl( const IConfigDb* pCfg ) 
        : super::_MyVirtBase( pCfg ), super( pCfg )
    { SetClassId( clsid( CJavaProxyImpl ) ); }

    gint32 AsyncCallVector(
        IEventSink* pTask,
        CfgPtr& pOptions,
        CfgPtr& pResp,
        const std::string& strcMethod,
        std::vector< BufPtr >& vecParams );

    jobject JavaProxyCall2(
        jobject pCb,
        jobject pContext,
        CfgPtr& pOptions,
        jobject listArgs,
        jobject listResp );
};

gint32 CJavaProxyImpl::AsyncCallVector(
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

        CopyUserOptions( pTask, pOptions );

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

jobject CJavaProxyImpl::JavaProxyCall2(
    jobject pCb,
    jobject pContext,
    CfgPtr& pOptions,
    jobject listArgs,
    jobject listResp )
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

        if( pOptions.IsEmpty() )
        {
            ret = -EINVAL;
            break;
        }
        CCfgOpener oOptions( ( IConfigDb* )pOptions );
        stdstr strMethod, strIfName;
        ret = oOptions.GetStrProp(
            propMethodName, strMethod );
        if( ERROR( ret ) )
            break;

        ret = oOptions.GetStrProp(
            propIfName, strIfName );
        if( ERROR( ret ) )
            break;

        bool bNoReply;
        ret = oOptions.GetBoolProp(
            propNoReply, bNoReply );
        if( ERROR( ret ) )
        {
            bNoReply = false;
            ret = 0;
        }

        guint32 dwSeriProto;
        ret = oOptions.GetIntProp(
            propSeriProto, dwSeriProto );
        if( ERROR( ret ) )
            break;

        IEventSink* pRespCb = nullptr;
        TaskletPtr pWrapperCb;
        CParamList oReqCtx;
        if( !bSync )
        {
            ret = NEW_PROXY_RESP_HANDLER2(
                pWrapperCb, ObjPtr( this ),
                &CJavaProxyImpl::OnAsyncCallResp,
                nullptr, oReqCtx.GetCfg() );

            if( ERROR( ret ) )
                break;

            oReqCtx.Push( ( intptr_t ) pCb );
            oReqCtx.Push( ( intptr_t ) listResp );
            oReqCtx.Push( strMethod );
            oReqCtx.Push( dwSeriProto );
            oReqCtx.Push( ( intptr_t )pContext );
            Py_INCREF( pCb );
            Py_INCREF( listResp );
            Py_INCREF( pContext );
        }
        else
        {
            ret = pWrapperCb.NewObj(
                clsid( CIoReqSyncCallback ) );
            if( ERROR( ret ) )
                break;
        }
        pRespCb = pWrapperCb;

        std::string strDBusIfName =
            DBUS_IF_NAME( strIfName );
        oOptions.SetStrProp(
            propIfName, strDBusIfName );

        CCfgOpener oResp;
        PyThreadState *_save;
        Py_UNBLOCK_THREADS;

        ret = this->AsyncCallVector(
            pRespCb, pOptions,
            oResp.GetCfg(), strMethod,
            vecParams );

        if( ret == STATUS_PENDING && bSync )
        {
            cpp::CIoReqSyncCallback*
                pSyncCb = pWrapperCb;
            ret = pSyncCb->WaitForComplete();
        }

        Py_BLOCK_THREADS;

        if( ret != STATUS_PENDING && !bSync )
        {
            Py_DECREF( pCb );
            Py_DECREF( listResp );
            Py_DECREF( pContext );
        }

        if( ERROR( ret ) )
            break;

        else if( ret == STATUS_PENDING )
        {
            guint64 qwTaskCancel = 0;
            oResp.GetQwordProp(
                propTaskId, qwTaskCancel );
            PyList_SetItem( listResp, 1,
                PyLong_FromLongLong( qwTaskCancel ) );
            break;
        }

        if( unlikely( bNoReply ) )
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

        if( ERROR( iRet ) )
        {
            ret = iRet;
            break;
        }

        EnumSeriProto dwSpRet = seriNone;
        GetSeriProto( pRmtResp, dwSpRet );
        if( dwSpRet != dwSeriProto )
        {
            ret = -EBADMSG;
            break;
        }
        ret = FillList( pRmtResp, listResp );
        if( ERROR( ret ) )
            break;

    }while( 0 );

    PyList_SetItem( listResp, 0,
        PyLong_FromLong( ret ) );

    Py_INCREF( listResp );

    return listResp;
}

static FactoryPtr InitClassFactory()
{
    BEGIN_FACTORY_MAPS;
    INIT_MAP_ENTRYCFG( CJavaProxy );
    INIT_MAP_ENTRYCFG( CJavaServer );
    INIT_MAP_ENTRYCFG( CJavaProxyImpl );
    INIT_MAP_ENTRYCFG( CJavaServerImpl );
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
        const char* szLib = "rpcbase.java";
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

%}
