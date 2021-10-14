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
        JNIEnv* jenv,
        jobject pCb,
        jobject pContext,
        CfgPtr& pOptions,
        jobject listArgs )
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
            switch( iType )
            {
            case typeByteArr:
                {
                    jbyteArray bytes =
                        jenv->NewByteArray(
                            pBuf->size() );

                    jenv->SetByteArrayRegion(
                        bytes, 0, pBuf->size(),
                        pBuf->ptr() );

                    pObj = bytes;
                    break;
                }
            }
            default:
            {
                ret = -EINVAL;
                break;
            }

        }while( 0 );

        return ret;
    };

    gint32 TimerCallback(
        IEventSink* pCallback,
        intptr_t pCb,
        intptr_t pCtx,
        int iRet )
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
            ret = GetJavaHost( pHost );
            if( ERROR( ret ) )
                break;

            jobject pgcb = ( jobject )pCb;
            jobject pgctx = ( jobject )pCtx;

            jmethodId timercb = jenv->GetMethodID(
                cls, "timeCallback",
                "(Ljava/lang/Object;"
                "Ljava/lang/Object;I)V" );

            jenv->CallVoidMethod( pHost,
                timercb, pgcb, pgtx, iRet );

            if( pgcb != nullptr )
                jenv->DeleteGlobalRef( pgcb );

            if( pgctx != nullptr )
                jenv->DeleteGlobalRef( pgctx );

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
            jenv->DeleteGlobalRef( pCb );

        if( pCtx != nullptr && jenv != nullptr )
            jenv->DeleteGlobalRef( pCtx );

        ( *pTaskEx )( eventCancelTask );
        return ret;
    }

    jobject AddTimer(
        JNIEnv* jenv,
        guint32 dwTimeoutSec,
        jobject pCb,
        jobject pCtx )
    {
        if( pCb == nullptr || jenv == nullptr )
            return nullptr;

        jobject jret = NewJRet( jenv );
        if( jret == nullptr )
            return -EFAULT;

        gint32 ret = 0;
        jobject pgcb = nullptr;
        jobject pgctx = nullptr;
        do{
            if( dwTimeoutSec > MAX_TIMEOUT_VALUE )
            {
                ret = -EINVAL;
                break;
            }

            TaskletPtr pTask;
            pgcb =
                jenv->NewGlobalRef( pCb );

            if( pCtx != nullptr )
            {
                pgctx =
                    jenv->NewGlobalRef( pCtx );
            }

            ret = DEFER_IFCALLEX_NOSCHED2(
                0, pTask, ObjPtr( this ),
                &CJavaInterfBase::TimerCallback,
                nullptr, ( intptr_t )pgcb,
                ( intptr_t )pgctx, 0 );

            if( ERROR( ret ) )
                break;

            CIfDeferCallTaskEx* pTaskEx = pTask;
            ret = pTaskEx->EnableTimer(
                dwTimeoutSec, eventRetry );
            if( ERROR( ret ) )
                 break;

            ObjPtr pTimer( pTask );
            jobject jtimer =
                NewObjPtr( jenv, pTimer );
            AddElemToJRet( jenv, jret, jtimer );

        }while( 0 );

        if( ERROR( ret ) )
        {
            if( pgcb )
                jenv->DeleteGlobalRef( pgcb );
            if( pgctx )
                jenv->DeleteGlobalRef( pgctx );
        }

        SetErrorJRet( jenv, jret, ret );
        return jret;
    }

    gint32 FillList( JNIEnv* jenv,
        IConfigDb* pResp,
        jobject listResp )
    {
        gint32 ret = 0;
        if( jenv == nullptr || pResp == nullptr )
            return -EINVAL;

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

            jclass jcls = jenv->FindClass(
                "org/rpcf/rpcbase/JRetVal" );

            if( !jenv->IsInstanceOf( listResp, jcls ) )
            {
                ret = -EINVAL;
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
                        jobject jval = NewByte( jenv, val );
                        if( jval == nullptr )
                        {
                            ret = -EFAULT;
                            break;
                        }
                        AddElemToJRet( jenv, listResp, jval );
                        break;
                    }
                case typeUInt16:
                    {
                        guint16 val = 0;
                        ret = oResp.GetShortProp( i, val );
                        if( ERROR( ret ) )
                            break;
                        jobject jval = NewShort( jenv, val );
                        if( jval == nullptr )
                        {
                            ret = -EFAULT;
                            break;
                        }
                        AddElemToJRet( jenv, listResp, jval );
                        break;
                    }
                case typeUInt32:
                    {
                        guint32 val = 0;
                        ret = oResp.GetIntProp( i, val );
                        if( ERROR( ret ) )
                            break;
                        jobject jval = NewInt( jenv, val );
                        if( jval == nullptr )
                        {
                            ret = -EFAULT;
                            break;
                        }
                        AddElemToJRet( jenv, listResp, jval );
                        break;
                    }
                case typeUInt64:
                    {
                        guint64 val = 0;
                        ret = oResp.GetQwordProp( i, val );
                        if( ERROR( ret ) )
                            break;
                        jobject jval = NewLong( jenv, val );
                        if( jval == nullptr )
                        {
                            ret = -EFAULT;
                            break;
                        }
                        AddElemToJRet( jenv, listResp, jval );
                        break;
                    }
                case typeFloat:
                    {
                        float val = 0;
                        ret = oResp.GetFloatProp( i, val );
                        if( ERROR( ret ) )
                            break;
                        jobject jval = NewFloat( jenv, val );
                        if( jval == nullptr )
                        {
                            ret = -EFAULT;
                            break;
                        }
                        AddElemToJRet( jenv, listResp, jval );
                        break;
                    }
                case typeDouble:
                    {
                        double val = 0;
                        ret = oResp.GetDoubleProp( i, val );
                        if( ERROR( ret ) )
                            break;
                        jobject jval = NewDouble( jenv, val );
                        if( jval == nullptr )
                        {
                            ret = -EFAULT;
                            break;
                        }
                        AddElemToJRet( jenv, listResp, jval );
                        break;
                    }
                case typeString:
                    {
                        std::string val;
                        ret = oResp.GetStrProp( i, val );
                        if( ERROR( ret ) )
                            break;
                        jobject jval = GetJniString( jenv, val );
                        if( jval == nullptr )
                        {
                            ret = -EFAULT;
                            break;
                        }
                        AddElemToJRet( jenv, listResp, jval );
                        break;
                    }
                case typeObj:
                    {
                        ObjPtr* val = new ObjPtr();
                        ret = oResp.GetObjPtr( i, *val );
                        if( ERROR( ret ) )
                            break;

                        jobject jval = NewObjPtr( jenv, val );
                        if( jval == nullptr )
                        {
                            ret = -EFAULT;
                            break;
                        }
                        AddElemToJRet( jenv, listResp, jval );
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

                        jbyteArray jval  =
                            jenv->NewByteArray( pBuf->size() );

                        if( jval == nullptr )
                        {
                            ret = -EFAULT;
                            break;
                        }

                        jenv->SetByteArrayRegion( jval,
                            0, pBuf->size(), pBuf->ptr() );

                        AddElemToJRet(
                            jenv, listResp, ( jobject )jval );

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

        }while( 0 );

        SetErrorJRet( jenv, listResp, ret );

        return ret;
    }

    gint32 ConvertByteArrayToBuf(
        JNIEnv* jenv,
        jobject* pObject,
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
        case typeByteArr:
            {
                jbyteArray pba = reinterpret_cast
                    < jbyteArray >( pObject );

                jsize len =
                    jenv->GetArrayLength( pba )

                if( len == 0 )
                    break;

                ret = pBuf->Resize( len );
                if( ERROR( ret ) )
                    break;

                jenv->GetByteArrayRegion(
                    pba, 0, len, pBuf->ptr() );

                break;
            }
        default:

            ret = -ENOTSUP;
            break;
        }

        return ret;
    }

    gint32 List2Vector(
        JNIEnv* jenv, jobject jRet,
        std::vector< BufPtr >& vecArgs )
    {
        gint32 ret = 0;
        if( pList == nullptr || jenv == nullptr )
            return -EINVAL;

        ret = GetParamsJRet( jenv, vecArgs );
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
            jenv->GetJavaVM( &pvm );
            jobject pgobj =
                jenv->NewGlobalRef( pObj );

            CCfgOpenerObj oCfg( this );
            oCfg.SetIntPtr(
                propJavaObj, ( guint32* )pgobj );
            oCfg.SetIntPtr(
                propJavaVM, ( guint32* )pvm );

        }while( 0 );

        return ret;
    }

    gint32 GetJavaHost( jobject& pObj )
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
            JNIEnv* jenv = nullptr;
            ret = GetJavaEnv( jenv, bAttach );
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
                 jenv->DeleteGlobalRef( pjObj );

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

        JNIEnv* jenv = nullptr;
        bool bAttach = false;
        ret = GetJavaEnv( jenv, bAttach );
        if( ERROR( ret ) )
            return ret;

        CParamList oResp;
        gint32 ret = 0;

        jobject pjCb = nullptr;
        jobject pjParams = nullptr;
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

            jobject pHost = nullptr;
            ret = GetJavaHost( pHost );
            if( ERROR( ret ) )
            {
                ret = -ENOTSUP;
                break;
            }

            ObjPtr pCb( pCallback );
            ObjPtr pArgs( pParams );

            pjCb = NewObjPtr( jenv, pCb );
            if( unlikely( pjCb == nullptr ) )
            {
                ret = -ENOMEM;
                break;
            }
            pjParams = NewObjPtr( jenv, pArgs );
            if( unlikely( pPyParams == nullptr ) ) 
            {           
                ret = -ENOMEM;
                break;
            }       

            jmethodId invoke = jenv->GetMethodID(
                cls, "invokeMethod",
                 "(Ljava/lang/Object;"
                 "Ljava/lang/String;"
                 "Ljava/lang/String;"
                 "IL/java/lang/Object;)" );

            listResp = jenv->CallObjectMethod(
                 pHost, invoke
                 pjCb, strIfName.c_str(),
                 strMethod.c_str(), dwSeriProto,
                 pjParams );

            if( unlikely( listResp == nullptr ) )
            {
                ret = -EFAULT;
                break;
            }

            ret = GetErrorJRet( jenv, listResp );
            if( ERROR( ret ) )
                break;

            if( ret == STATUS_PENDING )
                break;

            gint32 iCount = GetParamCountJRet(
                jenv, listResp );

            if( ERROR( ret ) || bNoReply ) 
                break;

            // fine, no resp params
            if( iCount == 0 )
                break;

            std::vector< BufPtr > vecResp;
            ret = List2Vector(
                jenv, listResp, vecResp );
            if( ERROR( ret ) )
                break;

            if( vecResp.empty() )
                break;

            for( auto elem : vecResp )
                oResp.Push( elem );

            oResp[ propSeriProto ] = dwSeriProto;

        }while( 0 );

        // freeing pjCb pjParams and listResp
        // is left to JVM
        if( bAttach )
            PutJavaEnv();

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

    gint32 JavaHandleAsyncResp(
        JNIEnv* jenv, jobject pjCb,
        guint32 dwSeriProto, jobject pjResp,
        jobject pContext )
    {
        gint32 ret = 0;
        do{
            jobject pHost = nullptr;
            ret = GetJavaHost( pHost );
            if( ERROR( ret ) )
                break;

            jclass cls =
                jenv->GetObjectClass( pHost );

            jmethodId harsp = jenv->GetMethodID(
                cls, "handleAsyncResp",
                "(Ljava/lang/Object;I"
                "(Ljava/lang/Object;"
                "(Ljava/lang/Object;)V" );

            jenv->CallVoidMethod( pHost,
                harsp, pjCb, dwSeriProto,
                pjResp, pContext );

        }while( 0 );

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

        gint32 ret = 0;
        JNIEnv* jenv = nullptr;
        bool bAttach = false;

        ret = GetJavaEnv( jenv, bAttach );
        if( ERROR( ret ) )
            return ret;

        CParamList oReqCtx( pCtx );
        guint32* pCb = nullptr;

        ret = oReqCtx.GetIntPtr( 0, pCb );
        if( ERROR( ret ) || pCb == nullptr )
            return ret;

        jobject pjCb = nullptr;
        do{
            jobject pjResp = NewJRet( jenv );
            SetErrorJRet( jenv, pjResp, iRet );
            jobject pjBuf = nullptr;

            if( SUCCEEDED( iRet ) )
            {
                ret = ConvertBufToByteArray(
                    jenv, pBuf, pjBuf );
            }
            else
            {
                ret = iRet;
            }

            jobject pjHandle = nullptr;
            pjHandle = NewLong(
                jenv, ( jlong )hChannel );

            if( SUCCEEDED( ret ) )
            {
                // Note: callback dose not to worry 
                // the pPyBuf become invalid if pBuf is released.
                // so we don't put pBuf to the list here.
                // but do pay attention to consume the pPyBuf
                // with the call. It would become invalid when
                // the callback returns
                AddElemToJRet( jenv, pjResp, pjBuf );
            }

            pjCb = ( jobject )pCb;
            ret = JavaHandleAsyncResp( jenv,
                pjCb, seriNone, pjResp, pjHandle );

        }while( 0 );

        if( pjCb != nullptr )
            jenv->DeleteGlobalRef( pjCb );

        if( bAttach )
            PutJavaEnv();

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

        JNIEnv* jenv = nullptr;
        bool bAttach = false;

        ret = GetJavaEnv( jenv, bAttach );
        if( ERROR( ret ) )
            return ret;

        CParamList oReqCtx( pCtx );
        jobject pjBuf = nullptr; 
        jobject pjCb = nullptr;
        gint32 ret = 0;
        do{
            guint32* pVal = nullptr;
            ret = oReqCtx.GetIntPtr( 0, pVal );
            if( ERROR( ret ) )
                break;

            jobject pjBuf = ( jobject )pVal;

            ret = oReqCtx.GetIntPtr( 1, pVal );
            if( ERROR( ret ) )
                break;
            pjCb = ( jobject )pVal;

            JRetVal pjResp = NewJRet( jenv );
            SetErrorJRet( jenv, pjResp, iRet );
            jobject pjHandle = NewLong(
                jenv, ( jlong )hChannel );    
                        
            if( SUCCEEDED( iRet ) )
                AddElemToJRet( jenv, pjResp, pjBuf );

            ret = JavaHandleAsyncResp( jenv,
                pjCb, seriNone, pjResp, pjHandle );

        }while( 0 );

        if( pjBuf != nullptr )
            jenv->DeleteGlobalRef( pjBuf );

        if( pjCb != nullptr )
            jenv->DeleteGlobalRef( pjCb );

        if( bAttach )
            PutJavaEnv();

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

 
        bool bAttach = false;
        JNIEnv* jenv = nullptr;
        ret = GetJavaEnv( jenv, bAttach );
        if( ERROR( ret ) )
            return ret;

        jobject pjCb = nullptr;
        jobject pjResp = nullptr;
        jobject pContext = nullptr;

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
            jobject pjCb =
                ( jobject )ptrVal;

            ret = oReqCtx.GetIntPtr( 1, ptrVal );
            if( ERROR( ret ) )
                break;

            jobject pjResp =
                ( jobject )ptrVal;  

            oReqCtx.GetIntProp( 3, dwReqProto );

            ret = oReqCtx.GetIntPtr( 4, ptrVal );
            if( ERROR( ret ) )
                pContext = nullptr;
            else
                pContext = ( jobject )ptrVal;

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
                SetErrorJRet( jenv, pjResp, ret );
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

                SetErrorJRet( jenv, pjResp, ret );
                if( SUCCEEDED( ret ) )
                    FillList( jenv, pResp, pjResp );
            }

            ret = JavaHandleAsyncResp( jenv,
                pjCb, dwSeriProto,
                pjResp, pContext );

        }while( 0 );

        if( pjCb != nullptr )
            jenv->DeleteGlobalRef( pjCb );

        if( pjResp != nullptr )
            jenv->DeleteGlobalRef( pjResp );

        if( pContext != nullptr )
            jenv->DeleteGlobalRef( pContext );

        if( bAttach )
            PutJavaEnv();

        return 0;
    }
 
    jobject GetChanCtx(
        JNIEnv* jenv, HANDLE hChannel )
    {
        if( jenv == nullptr )
            return nullptr;

        jobject jret = NewJRet( jenv );
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
            ret = oCfg.GetIntPtr(
                propChanCtx, ptr );

            if( ERROR( ret ) )
                break;

            AddElemToJRet(
                jenv, jret, ( jobject )ptr );

        }while( 0 );
        SetErrorJRet( jenv, jret, ret );

        return jret;
    }

    gint32 SetChanCtx( JNIEnv* jenv,
        HANDLE hChannel, jobject pCtx )
    {
        gint32 ret = 0;
        jobject pgctx = nullptr;

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
                jobject pOld = ( jobject )ptr;
                if( pOld == pCtx )
                    break;
                if( pOld != nullptr && 
                    jenv != nullptr )
                   jenv->DeleteGlobalRef( pOld ); 
                oCfg.RemoveProperty( propChanCtx );
            }

            ret = 0;

            if( pCtx == nullptr ||
                jenv == nullptr )
                break;

            pgctx = jenv->NewGlobalRef(pCtx);

            oCfg.SetIntPtr( propChanCtx,
                ( guint32*)pjctx );

        }while( 0 );

        return ret;
    }

    gint32 RemoveChanCtx(
        JNIEnv* jenv, HANDLE hChannel )
    {
        return SetChanCtx(
            jenv, hChannel, nullptr );
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

        gint32 ret = 0;
        bool bAttach = false;
        JNIEnv* jenv = nullptr;
        ret = GetJavaEnv( jenv, bAttach );
        if( ERROR( ret ) )
            return ret;

        if( SUCCEEDED( iRet ) )
        {
            ret = oCfg.GetObjPtr(
                propRespPtr, pObj ); 
        }

        jobject pjCb = ( jobject)pCb;
        jobject pjResp = ( jobject)pResp;

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

            SetErrorJRet( jenv, pjResp, ret );
            ret = JavaHandleAsyncResp( jenv,
                pjCb, seriNone, pjResp, nullptr );

        }while( 0 );

        if( pjCb )
            jenv->DeleteGlobalRef( pjCb );
        if( pjResp )
            jenv->DeleteGlobalRef( pjResp );

        if( bAttach )
            PutJavaEnv();

        return ret;
    }

    gint32 InstallCancelNotify(
        JNIEnv* jenv,
        IEventSink* pCallback,
        jobject pCb,
        jobject pListResp )
    {
        if( pCb == nullptr ||
            pListResp == nullptr )
            return -EINVAL;

        TaskletPtr pTask;
        gint32 ret = DEFER_CANCEL_HANDLER2(
            -1, pTask, this,
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

    gint32 JavaOnStmReady(
        JNIEnv* jenv, HANDLE hChannel )
    {
        gint32 ret = 0;
        do{
            jobject pHost = nullptr;
            ret = GetJavaHost( pHost );
            if( ERROR( ret ) )
                break;

            jclass cls =
                jenv->GetObjectClass( pHost );

            jmethodId stmReady = jenv->GetMethodID(
                cls, "onStmReady", "(J)V" );

            ret = jenv->CallVoidMethod( pHost,
                stmReady, ( jlong )hChannel );

        }while( 0 );

        return ret;
    }

    gint32 OnStreamReady( HANDLE hChannel )
    {
        gint32 ret = 0;

        bool bAttach = false;
        JNIEnv* jenv = nullptr;
        ret = GetJavaEnv( jenv, bAttach );
        if( ERROR( ret ) )
            return ret;

        JavaOnStmReady( jenv, hChannel );

        if( bAttach )
            PutJavaEnv();

        return ret;
    }

    gint32 JavaOnStmClosing(
        JNIEnv* jenv, HANDLE hChannel )
    {
        gint32 ret = 0;
        do{
            jobject pHost = nullptr;
            ret = GetJavaHost( pHost );
            if( ERROR( ret ) )
                break;

            jclass cls =
                jenv->GetObjectClass( pHost );

            jmethodId stmClosing = jenv->GetMethodID(
                cls, "onStmClosing", "(J)V" );

            ret = jenv->CallVoidMethod( pHost,
                stmClosing, ( jlong )hChannel );

        }while( 0 );

        return ret;
    }

    gint32 OnStmClosing( HANDLE hChannel )
    {
        if( hChannel == INVALID_HANDLE )
            return -EINVAL;

        bool bAttach = false;
        JNIEnv* jenv = nullptr;
        ret = GetJavaEnv( jenv, bAttach );
        if( ERROR( ret ) )
            return ret;

        JavaOnStmClosing( jenv, hChannel );

        jobject pCtx = GetChanCtx( hChannel );
        if( pCtx != nullptr )
            RemoveChanCtx( jenv, hChannel );

        ret = super::OnStmClosing( hChannel );
        if( bAttach )
            PutJavaEnv();

        return ret;
    }

    void JavaDeferCallback(
        JNIEnv* jenv, jobject pjCb,
        jobject pjArgs )
    {
        do{
            jobject pHost = nullptr;
            ret = GetJavaHost( pHost );
            if( ERROR( ret ) )
                break;

            jclass cls =
                jenv->GetObjectClass( pHost );

            jmethodId deferCall = jenv->GetMethodID(
                cls, "DeferCallback",
                "(Ljava/lang/Object;Ljava/lang/Object;)V" );

            jenv->CallVoidMethod( pHost,
                deferCall, pjCb, pjArgs );

        }while( 0 );

        return;
    }

    gint32 DeferCallback(
        intptr_t pCb, intptr_t pArgs )
    {
        gint32 ret = 0;
        jobject pjCb = ( jobject )pCb;
        jobject pjArgs = ( jobject )pArgs;

        bool bAttach = false;
        JNIEnv* jenv = nullptr;
        ret = GetJavaEnv( jenv, bAttach );
        if( ERROR( ret ) )
            return ret;

        JavaDeferCallback( jenv, pjCb, pjArgs );

        if( pjCb != nullptr )
            jenv->DeleteGlobalRef( pjCb );

        if( pjArgs != nullptr )
            jenv->DeleteGlobalRef( pjArgs );

        if( bAttach )
            PutJavaEnv();

        return ret;
    }

    gint32 DeferCall(
        JNIEnv* jenv, jobject pjCb,
        jobject pjArgs )
    {
        TaskletPtr pTask;

        jobject pgcb =
            jenv->NewGlobalRef( pjCb );

        jobject pjargs =
            jenv->NewGlobalRef( pjArgs );

        gint32 ret = 0;

        do{
            ret = DEFER_IFCALLEX_NOSCHED(
                pTask, ObjPtr( this ),
                &CJavaInterfBase::DeferCallback,
                ( intptr_t )pgcb,
                ( intptr_t )pgargs );

            if( ERROR( ret ) )
                break;

            ret = this->GetIoMgr()->
                RescheduleTask( pTask );

            if( ERROR( ret ) )
            {
                ( *pTask )( eventCancelTask );
                break;
            }

        }while( 0 );

        if( ERROR( ret ) )
        {
            if( pgcb != nullptr )
                jenv->DeleteGlobalRef( pgcb );

            if( pgargs != nullptr )
                jenv->DeleteGlobalRef( pgargs );
        }

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
        JNIEnv* jenv,
        jobject pCb,
        jobject pContext,
        CfgPtr& pOptions,
        jobject listArgs );
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
    JNIEnv* jenv,
    jobject pCb,
    jobject pContext,
    CfgPtr& pOptions,
    jobject listArgs )
{
    gint32 ret = 0;
    if( jenv == nullptr )
        return -EINVAL;
    do{
        bool bSync = false;
        jobject listResp = NewJRet( jenv );

        if( pCb == nullptr )
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
        ret = List2Vector(
           jenv, listArgs, vecParams );
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
        ret = FillList(
            jenv, pRmtResp, listResp );
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

class CJavaRpcServices :
    public CRpcServices
{
};

%extend CJavaRpcServices
{
    public:
    gint32 SetJavaHost(
        JNIEnv *jenv, jobject pObj )
    {
        gint32 ret = 0;
        CJavaProxyImpl* pImpl = static_cast
            < CJavaProxyImpl* >( $self );
        if( pImpl == nullptr ) 
            ret = -EFAULT;
        else
        {
            ret = pImpl->SetJavaHost(
                jenv, pObj );
        }

        return ret;
    }

    jobject GetJavaHost()
    {
        CJavaProxyImpl* pImpl = static_cast
            < CJavaProxyImpl* >( $self );
        jobject pObj = nullptr;
        if( pImpl != nullptr ) 
        {
            gint32 ret = 
                pImpl->GetJavaHost( pObj );
            if( ERROR( ret ) )
                pObj = nullptr;
        }
        return pObj;
    }

    gint32 RemoveJavaHost()
    {
        gint32 ret = 0;
        CJavaProxyImpl* pImpl = static_cast
            < CJavaProxyImpl* >( $self );
        if( pImpl == nullptr ) 
            ret = -EFAULT;
        else
            ret = pImpl->RemoveJavaHost();

        return ret;
    }

    gint32 StartStream( JNIEnv *jenv, ObjPtr* ppObj )
    {
        jobject jret = NewJRet();
        gint32 ret = 0;
        do{
            CJavaProxyImpl* pImpl = static_cast
                < CJavaProxyImpl* >( $self );
            if( pImpl == nullptr ) 
            {
                ret = -EFAULT;
                break;
            }

            IConfigDb* pDesc = *ppObj;
            if( pDesc == nullptr )
            {
                ret = -EINVAL;
                break;
            }
            HANDLE hChannel = nullptr;
            ret = pImpl->StartStream(
                ( HANDLE )hChannel, pDesc );
            if( ERROR( ret ) )
                break;

            jobject pChan = NewLong( jenv,
                ( jlong )hChannel );

            AddElemToJRet( jenv, jret, pChan );

        }while( 0 );
        SetErrorJRet( jenv, jret, ret );
        return ret;
    }

    gint32 CloseStream( jlong hChannel );
    {
        gint32 ret = 0;
        CJavaProxyImpl* pImpl = static_cast
            < CPJavaProxyImpl* >( $self );
        if( pImpl == nullptr ) 
            ret = -EFAULT;
        else
        {
            ret = pImpl->CloseStream(
                ( HANDLE )hChannel );
        }
        return ret;
    }

    gint32 WriteStreamNoWait( JNIEnv *jenv,
        jlong hChannel, jobject pjBuf );
    {
        gint32 ret = 0;
        do{
            CJavaProxyImpl* pImpl = static_cast
                < CJavaProxyImpl* >( $self );

            if( pImpl == nullptr ) 
            {
                ret = -EFAULT;
                break;
            }

            BufPtr pBuf( true );
            ret = pImpl->ConvertByteArrayToBuf(
                jenv, pjBuf, pBuf );
            if( ERROR( ret ) )
                break;

            ret = pImpl->WriteStreamNoWait(
                ( HANDLE )hChannel, pBuf );

        }while( 0 );
        return ret;
    }

    gint32 WriteStream( JNIEnv *jenv,
        jlong hChannel, jobject pjBuf );
    {
        gint32 ret = 0;
        do{
            CJavaProxyImpl* pImpl = static_cast
                < CJavaProxyImpl* >( $self );

            if( pImpl == nullptr ) 
            {
                ret = -EFAULT;
                break;
            }

            BufPtr pBuf( true );
            ret = pImpl->ConvertByteArrayToBuf(
                pjBuf, pBuf );
            if( ERROR( ret ) )
                break;

            ret = pImpl->WriteStream(
                ( HANDLE )hChannel, pBuf );

        }while( 0 );

        return ret;
    }

    JRetVal ReadStreamNoWait( 
        JNIEnv *jenv, jlong hChannel )
    {
        gint32 ret = 0;

        jobject pResp = NewJRet( jenv );

        do{

            CJavaProxyImpl* pImpl = static_cast
                < CJavaProxyImpl* >( $self );

            if( pImpl == nullptr ) 
            {
                ret = -EFAULT;
                break;
            }

            BufPtr pBuf;
            ret = pImpl->ReadStreamNoWait(
                ( HANDLE )hChannel, pBuf );

            if( ERROR( ret ) )
                break;

            jobject pjBuf = nullptr;
            ret = pImpl->ConvertBufToByteArray( 
                jenv, pBuf, pjBuf );
            if( ERROR( ret ) )
                break;

            AddElemToJRet( jenv, pResp, pjBuf );

        }while( 0 );
        SetErrorJRet( jenv, pResp, ret );
        return pResp;
    }

    jobject ReadStream( JNIEnv *jenv,
        jlong hChannel, gint32 dwSize )
    {
        jobject jRet = NewJRet( jenv );
        gint32 ret = 0;
        do{
            CJavaProxyImpl* pImpl = static_cast
                < CJavaProxyImpl* >( $self );

            if( pImpl == nullptr ) 
            {
                ret = -EFAULT;
                break;
            }

            BufPtr pBuf;
            if( dwSize > 0 )
            {
                ret = pBuf.NewObj();
                if( ERROR( ret ) )
                    break;
                ret = pBuf->Resize( dwSize );
                if( ERROR( ret ) )
                    break;
            }

            ret = pImpl->ReadStream(
                ( HANDLE )hChannel, pBuf );
            if( ERROR( ret ) )
            {
                ret = -EFAULT;
                break;
            }

            jobject pjBuf = nullptr;
            ret = pImpl->ConvertBufToByteArray(
                jenv, pBuf, pjBuf );
            if( ERROR( ret ) )
                break;

            AddElemToJRet( jenv, jRet, pjBuf );
            break;

        }while( 0 );

        SetErrorJRet( jenv, jRet, ret );
        return jRet;
    }

    jobject ReadStreamAsync( JNIEnv *jenv,
        jlong hChannel,
        jobject pCb,
        guint32 dwSize );
    {
        jobject pResp = NewJRet( jenv );
        gint32 ret = 0;
        do{
            CJavaProxyImpl* pImpl = static_cast
                < CJavaProxyImpl* >( $self );

            if( pImpl == nullptr ||
                pCb == nullptr ||
                jenv == nullptr ) 
            {
                ret = -EFAULT;
                break;
            }

            jobject pjCb =
                jenv->NewGlobalRef( pCb );

            CParamList oReqCtx;
            oReqCtx.Push( ( intptr_t ) pjCb );

            cpp::BufPtr pBuf;

            if( dwSize > 0 )
            {
                ret = pBuf.NewObj();
                if( ERROR( ret ) )
                    break;

                ret = pBuf->Resize( dwSize );
                if( ERROR( ret ) )
                    break;
            }

            IConfigDb* pCtx = oReqCtx.GetCfg();
            ret = pImpl->ReadStreamAsync(
                ( HANDLE )hChannel, pBuf, pCtx );

            if( ret != STATUS_PENDING )
                jenv->DeleteGlobalRef( pjCb );

            if( ret == STATUS_PENDING || ERROR( ret ) )
                break;

            jobject pjBuf = nullptr;
            ret = pImpl->ConvertBufToByteArray(
                jenv, pBuf, pjBuf );
            if( ERROR( ret ) )
                break;

            AddElemToJRet( jenv, jRet, pjBuf );

        }while( 0 );

        SetErrorJRet( jenv, jRet, ret );
        return jRet;
    }

    gint32 WriteStreamAsync(
        JNIEnv* jenv,
        HANDLE hChannel,
        jobject* pPyBuf,
        jobject* pCb );
    %MethodCode
    do{
        sipRes = 0;
        CJavaProxyImpl* pImpl = static_cast
            < CJavaProxyImpl* >( $self );

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

        sipRes = pImpl->ConvertByteArrayToBuf(
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

    jobject AddTimer(
        JNIEnv* jenv;
        guint32 dwTimeoutSec,
        PyObject* pCb,
        PyObject* pCtx )
    {
        CJavaProxyImpl* pImpl =
        static_cast< CJavaProxyImpl* >( $self );
        return pImpl->AddTimer( jenv,
            dwTimeoutSec, pCb, pCtx );
    }

    gint32 DisableTimer(
        cpp::ObjPtr& pTimer );
    %MethodCode
        CJavaProxyImpl* pImpl =
        static_cast< CJavaProxyImpl* >
            ( $self );
        sipRes = pImpl->DisableTimer( *a0 );
    %End

    gint32 JavaDeferCall( JNIEnv* jenv,
        jobject pjCb,
        jobject pjArgs );
    {
        CJavaProxyImpl* pImpl =
            static_cast< CJavaProxyImpl* >( $self  );
        return pImpl->JavaDeferCall( jenv, a0, a1 );
    }
};

class CJavaInterfProxy :
    public CJavaRpcServices
{
    jobject JavaProxyCall2(
        JNIEnv* jenv,
        jobject pCb,
        jobject pContext,
        cpp::CfgPtr& pOptions,
        jobject listArgs );

    gint32 GetPeerIdHash(
        HANDLE hChannel,
        guint64& qwPeerIdHash );

    HANDLE GetChanByIdHash( guint64 );
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
            clsid( CJavaProxyImpl ),
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

CJavaProxy* CastToProxy(
    cpp::ObjPtr& pObj );
%MethodCode
    CJavaProxy* pProxy = *a0;
    sipRes = pProxy;
%End

void PyDbgPrint( char* szMsg, int level = 3 );
%MethodCode
    DebugPrintEx( ( EnumLogLvl )a1, 0, "%s", a0 );
%End

void PyOutputMsg( char* szMsg );
%MethodCode
    OutputMsg( 0, "%s", a0 );
%End
