/*
 * =====================================================================================
 *
 *       Filename:  proxy.i
 *
 *    Description:  a swig file as the wrapper of CJavaProxyImpl class and
 *                  helper classes for Java
 *
 *        Version:  1.0
 *        Created:  10/16/2021 02:45:30 PM
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
%{
#include "rpc.h"
#include "defines.h"
#include "streamex.h"
#include "counters.h"
%}

%header {

gint32 LoadJavaFactory( ObjPtr& pMgr );

typedef enum 
{
    // a pointer to pyobject of the python context
    propJavaVM = propReservedEnd + 120,
    // propChanCtx

} EnumJavaPropId;

class CJavaProxyBase :
    public CStreamProxyAsync
{
    public:
    typedef CStreamProxyAsync super;
    CJavaProxyBase( const IConfigDb* pCfg ) :
        super::_MyVirtBase( pCfg ), super( pCfg )
    {}

    virtual jobject JavaProxyCall2(
        JNIEnv *jenv,
        jobject pCb,
        jobject pContext,
        CfgPtr& pOptions,
        jobject listArgs ) = 0;
};

template< class T >
class CJavaInterfBase : public T
{
    public:
    typedef T super;
    CJavaInterfBase( const IConfigDb* pCfg ) 
        : super::_MyVirtBase( pCfg ), super( pCfg )
    {}

    gint32 ConvertBufToByteArray( JNIEnv *jenv,
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
                        ( jbyte* )pBuf->ptr() );

                    pObj = bytes;
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
        JNIEnv *jenv = nullptr;
        ret = GetJavaEnv( jenv, bAttach );
        if( ERROR( ret ) )
            return ret;
        do{
            jobject pHost = nullptr;
            ret = GetJavaHost( pHost );
            if( ERROR( ret ) )
                break;

            jclass cls =
                jenv->GetObjectClass( pHost );

            jobject pgcb = ( jobject )pCb;
            jobject pgctx = ( jobject )pCtx;

            jmethodID timercb = jenv->GetMethodID(
                cls, "timerCallback",
                "(Ljava/lang/Object;"
                "Ljava/lang/Object;I)V" );
            if( timercb == nullptr )
            {
                ret = -ENOENT;
                break;
            }
            jenv->CallVoidMethod( pHost,
                timercb, pgcb, pgctx, iRet );

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
        JNIEnv *jenv, ObjPtr& pTimer )
    {
        CIfDeferCallTaskEx* pTaskEx = pTimer;
        if( pTaskEx == nullptr )
            return -EINVAL;
        jobject pCb = nullptr;
        jobject pCtx = nullptr;
        gint32 ret = 0;

        do{
            ret = pTaskEx->DisableTimer();

            CStdRTMutex oTaskLock(
                pTaskEx->GetLock() );

            Variant oVar;
            ret = pTaskEx->GetParamAt( 1, oVar );
            if( ERROR( ret ) )
                break;

            intptr_t ptrval = ( intptr_t& )oVar;
            if( ptrval == 0 )
                break;

            pCb = ( jobject )ptrval;
            ret = pTaskEx->GetParamAt( 2, oVar );
            if( ERROR( ret ) )
                break;

            ptrval = ( intptr_t& )oVar;
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
        JNIEnv *jenv,
        guint32 dwTimeoutSec,
        jobject pCb,
        jobject pCtx )
    {
        if( pCb == nullptr || jenv == nullptr )
            return nullptr;

        jobject jret = NewJRet( jenv );
        if( jret == nullptr )
            return nullptr;

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

            ObjPtr* pTimer = new ObjPtr( pTask );
            jobject jtimer =
                NewObjPtr( jenv, ( jlong )pTimer );
            if( jtimer == nullptr )
            {
                delete pTimer;
                ret = -EFAULT;
                break;
            }

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

    gint32 FillList( JNIEnv *jenv,
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
                        {
                            delete val;
                            break;
                        }

                        jobject jval =
                            NewObjPtr( jenv,( jlong ) val );
                        if( jval == nullptr )
                        {
                            delete val;
                            ret = -EFAULT;
                            break;
                        }
                        AddElemToJRet( jenv, listResp, jval );
                        break;
                    }
                case typeByteArr:
                    {
                        BufPtr pBuf;
                        ret = oResp.GetBufPtr( i, pBuf );
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
                            0, pBuf->size(),
                            ( jbyte* )pBuf->ptr() );

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
        JNIEnv *jenv,
        jobject pObject,
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

        EnumTypeId iType = ( EnumTypeId )
            GetTypeId( jenv, pObject );

        switch( iType )
        {
        case typeByteArr:
            {
                jbyteArray pba = reinterpret_cast
                    < jbyteArray >( pObject );

                jsize len =
                    jenv->GetArrayLength( pba );

                if( len == 0 )
                    break;

                ret = pBuf->Resize( len );
                if( ERROR( ret ) )
                    break;

                jenv->GetByteArrayRegion( pba,
                    0, len, ( jbyte* )pBuf->ptr() );

                break;
            }
        default:

            ret = -ENOTSUP;
            break;
        }

        return ret;
    }

    gint32 List2Vector( JNIEnv *jenv,
        jobject jObj,
        std::vector< Variant >& vecArgs )
    {
        gint32 ret = 0;
        if( jObj == nullptr || jenv == nullptr )
            return -EINVAL;

        int typeId = GetObjType( jenv, jObj );
        if( typeId == 200 )
        {
            // jObj is a JRetVal
            ret = GetParamsJRet(
                jenv, jObj, vecArgs );
        }
        else if( typeId == 201 )
        {
            // jObj is a byte array
            ret = GetParamsObjArr(
                jenv, jObj, vecArgs );
        }
        if( ERROR( ret ) )
            vecArgs.clear();

        return ret;
    }

    gint32 SetJavaHost(
        JNIEnv *jenv, jobject pObj )
    {
        gint32 ret = 0;
        if( jenv == nullptr || pObj == nullptr )
            return -EINVAL;
        do{
            JavaVM* pvm = nullptr;
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

    gint32 GetVmPtr( JavaVM*& pvm )
    {
        guint32* pVal = nullptr;
        CCfgOpenerObj oCfg( this );
        gint32 ret = oCfg.GetIntPtr(
            propJavaVM, pVal );
        if( ERROR( ret ) )
            return ret;
        pvm = ( JavaVM* )pVal;
        return STATUS_SUCCESS;
    }

    gint32 GetJavaEnv(
        JNIEnv*& jenv, bool& bAttach )
    {
        JavaVM* pvm = nullptr;
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
                    ( void** )&jenv, NULL )!= 0 )
                {
                    ret = ERROR_FAIL;
                    OutputMsg( ret,
                        "Failed to attach jenv "
                        "to current thread" );
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
        JavaVM* pvm = nullptr;
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
            ret = oCfg.GetIntPtr(
                propJavaObj, pObj );
            if( ERROR( ret ) )
                break;
            DebugPrint( 0, "RemoveJavaHost()..." );
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

    virtual gint32 InvokeUserMethod(
        IConfigDb* pParams,
        IEventSink* pCallback )
    {
        if( pParams == nullptr )
            return -EINVAL;

        JNIEnv* jenv = nullptr;
        bool bAttach = false;
        gint32 ret = GetJavaEnv( jenv, bAttach );
        if( ERROR( ret ) )
            return ret;

        CParamList oResp;
        ret = 0;

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

            ObjPtr* pCb = new ObjPtr( pCallback );
            CParamList* pArgs = new CParamList( pParams );

            pjCb =
                NewObjPtr( jenv, ( jlong )pCb );

            if( unlikely( pjCb == nullptr ) )
            {
                delete pCb;
                delete pArgs;
                ret = -ENOMEM;
                break;
            }
            pjParams =
                NewCParamList( jenv, ( jlong )pArgs );

            if( unlikely( pjParams == nullptr ) ) 
            {           
                delete pArgs;
                ret = -ENOMEM;
                break;
            }       

            jclass cls =
                jenv->GetObjectClass( pHost );
            jmethodID invokeMethod = jenv->GetMethodID(
                cls, "invokeMethod",
                 "(Lorg/rpcf/rpcbase/ObjPtr;"
                 "Ljava/lang/String;"
                 "Ljava/lang/String;"
                 "ILorg/rpcf/rpcbase/CParamList;)"
                 "Lorg/rpcf/rpcbase/JRetVal;" );

            if( invokeMethod == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            jstring jsIfName = jenv->NewStringUTF(
                strIfName.c_str() );
            jstring jsMethod = jenv->NewStringUTF(
                strMethod.c_str() );

            jobject listResp =
                jenv->CallObjectMethod(
                 pHost, invokeMethod,
                 pjCb, jsIfName,
                 jsMethod, dwSeriProto,
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

            std::vector< Variant > vecResp;
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
        JNIEnv *jenv, jobject pjCb,
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

            jmethodID harsp = jenv->GetMethodID(
                cls, "handleAsyncResp",
                "(Ljava/lang/Object;I"
                "Ljava/lang/Object;"
                "Ljava/lang/Object;)V" );

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
                // so we do not put pBuf to the list here.
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

        gint32 ret = GetJavaEnv( jenv, bAttach );
        if( ERROR( ret ) )
            return ret;

        CParamList oReqCtx( pCtx );
        jobject pjBuf = nullptr; 
        jobject pjCb = nullptr;
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

            jobject pjResp = NewJRet( jenv );
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
                // most likely there is a proto value
                oResp.GetIntProp(
                    propSeriProto, dwSeriProto );

                if( dwSeriProto != dwReqProto )
                {
                    ret = -EBADMSG;
                    DebugPrint( ret,
                        "proto not agree,"
                        " resp %d, req %d",
                        dwSeriProto, dwReqProto );
                    break;
                }

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
        JNIEnv *jenv, HANDLE hChannel )
    {
        if( jenv == nullptr )
            return nullptr;

        jobject jret = NewJRet( jenv );
        gint32 ret = 0;
        do{
            if( hChannel == INVALID_HANDLE )
                break;

            CfgPtr pCtx;
            ret = this->GetContext(
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

    gint32 SetChanCtx( JNIEnv *jenv,
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
                ( guint32*)pgctx );

        }while( 0 );

        return ret;
    }

    gint32 RemoveChanCtx(
        JNIEnv *jenv, HANDLE hChannel )
    {
        return SetChanCtx(
            jenv, hChannel, nullptr );
    }

    gint32 JavaCancelNotify(
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
        JNIEnv *jenv = nullptr;
        ret = GetJavaEnv( jenv, bAttach );
        if( ERROR( ret ) )
            return ret;

        jobject pjCb = ( jobject)pCb;
        jobject pjResp = ( jobject)pResp;

        if( iRet == ERROR_USER_CANCEL ||
            iRet == ERROR_TIMEOUT ||
            iRet == ERROR_CANCEL )
        {
            SetErrorJRet( jenv, pjResp, iRet );
            ret = JavaHandleAsyncResp( jenv,
                pjCb, seriNone, pjResp, nullptr );
        }

        if( pjCb )
            jenv->DeleteGlobalRef( pjCb );
        if( pjResp )
            jenv->DeleteGlobalRef( pjResp );

        if( bAttach )
            PutJavaEnv();

        return ret;
    }

    gint32 InstallCancelNotify(
        JNIEnv *jenv,
        IEventSink* pCallback,
        jobject pCb,
        jobject pListResp )
    {
        if( pCb == nullptr ||
            pListResp == nullptr )
            return -EINVAL;

        jobject pjCb =
            jenv->NewGlobalRef( pCb );

        jobject pjResp =
            jenv->NewGlobalRef( pListResp );

        TaskletPtr pTask;
        gint32 ret = DEFER_CANCEL_HANDLER2(
            -1, pTask, this,
            &CJavaInterfBase::JavaCancelNotify,
            pCallback, 0,
            ( intptr_t )pjCb,
            ( intptr_t)pjResp );    

        if( SUCCEEDED( ret ) )
        {
            CIfAsyncCancelHandler* pasc = pTask;
            pasc->SetSelfCleanup();
        }
        else
        {
            jenv->DeleteGlobalRef( pjCb );
            jenv->DeleteGlobalRef( pjResp );
        }
        return ret;
    }

    gint32 JavaOnStmReady(
        JNIEnv *jenv, HANDLE hChannel )
    {
        gint32 ret = 0;
        do{
            jobject pHost = nullptr;
            ret = GetJavaHost( pHost );
            if( ERROR( ret ) )
                break;

            jclass cls =
                jenv->GetObjectClass( pHost );

            jmethodID stmReady = jenv->GetMethodID(
                cls, "onStmReady", "(J)I" );

            if( stmReady == nullptr )
            {
                ret = -ENOENT;
                break;
            }

            jenv->CallIntMethod( pHost,
                stmReady, ( jlong )hChannel );

        }while( 0 );

        return ret;
    }

    gint32 OnStreamReady( HANDLE hChannel )
    {
        gint32 ret = 0;

        bool bAttach = false;
        JNIEnv *jenv = nullptr;
        ret = GetJavaEnv( jenv, bAttach );
        if( ERROR( ret ) )
            return ret;

        JavaOnStmReady( jenv, hChannel );

        if( bAttach )
            PutJavaEnv();

        return ret;
    }

    gint32 JavaOnStmClosing(
        JNIEnv *jenv, HANDLE hChannel )
    {
        gint32 ret = 0;
        do{
            jobject pHost = nullptr;
            ret = GetJavaHost( pHost );
            if( ERROR( ret ) )
                break;

            jclass cls =
                jenv->GetObjectClass( pHost );

            jmethodID stmClosing = jenv->GetMethodID(
                cls, "onStmClosing", "(J)I" );

            if( stmClosing == nullptr )
            {
                ret = -ENOENT;
                break;
            }

            jenv->CallIntMethod( pHost,
                stmClosing, ( jlong )hChannel );

        }while( 0 );

        return ret;
    }

    gint32 OnStmClosing( HANDLE hChannel )
    {
        if( hChannel == INVALID_HANDLE )
            return -EINVAL;

        bool bAttach = false;
        JNIEnv *jenv = nullptr;
        gint32 ret = GetJavaEnv( jenv, bAttach );
        if( ERROR( ret ) )
            return ret;

        JavaOnStmClosing( jenv, hChannel );

        jobject pCtx =
            GetChanCtx( jenv, hChannel );

        if( pCtx != nullptr )
            RemoveChanCtx( jenv, hChannel );

        ret = super::OnStmClosing( hChannel );
        if( bAttach )
            PutJavaEnv();

        return ret;
    }

    void JavaDeferCallback(
        JNIEnv *jenv, jobject pjCb,
        jobject pjArgs )
    {
        do{
            jobject pHost = nullptr;
            gint32 ret = GetJavaHost( pHost );
            if( ERROR( ret ) )
                break;

            jclass cls =
                jenv->GetObjectClass( pHost );

            jmethodID deferCall = jenv->GetMethodID(
                cls, "deferCallback",
                "(Ljava/lang/Object;Ljava/lang/Object;)V" );

            if( deferCall == nullptr )
                break;

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
        JNIEnv *jenv, jobject pjCb,
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
                ( intptr_t )pjargs );

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

            if( pjargs != nullptr )
                jenv->DeleteGlobalRef( pjargs );
        }

        return ret;
    }

    jobject CastToObjPtr( JNIEnv *jenv )
    {
        ObjPtr* ppObj = new ObjPtr( this );
        if( ppObj == nullptr )
            return nullptr;

        jobject pNewObj =
            NewObjPtr( jenv, ( jlong )ppObj );
        if( pNewObj == nullptr )
            delete ppObj;
        return pNewObj;
    }
};

class CJavaProxy :
    public CJavaInterfBase< CJavaProxyBase >
{
    public:
    typedef CJavaInterfBase< CJavaProxyBase > super;
    CJavaProxy( const IConfigDb* pCfg ) 
        : super::_MyVirtBase( pCfg ), super( pCfg )
    {}

    virtual gint32 AsyncCallVector(
        IEventSink* pTask,
        CfgPtr& pOptions,
        CfgPtr& pResp,
        const std::string& strcMethod,
        std::vector< Variant >& vecParams );

    virtual jobject JavaProxyCall2(
        JNIEnv *jenv,
        jobject pCb,
        jobject pContext,
        CfgPtr& pOptions,
        jobject listArgs ) override;

    jobject GetIdHashByChan(
        JNIEnv *jenv,
        jlong hChannel )
    { return nullptr; }

    jobject GetPeerIdHash(
        JNIEnv *jenv,
        jlong hChannel )
    {
        if( jenv == nullptr )
            return nullptr;
        jobject jret = NewJRet( jenv );
        if( jret == nullptr )
            return nullptr;
        gint32 ret = 0;
        do{
            guint64 qwHash = 0;
            ret = super::GetPeerIdHash(
                ( HANDLE )hChannel, qwHash );
            if( ERROR( ret ) )
                break;

            jobject jhash = NewLong(
                jenv, ( gint64 )qwHash );

            if( jhash == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            AddElemToJRet( jenv, jret, jhash );

        }while( 0 );

        SetErrorJRet( jenv, jret, ret );
        return jret;
    }
};

gint32 CJavaProxy::AsyncCallVector(
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
        bool bNoReply = false;

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

            ret = oOptions.GetBoolProp(
                propNoReply, bNoReply );
            if( ERROR( ret ) )
            {
                bNoReply = false;
                ret = 0;
            }
        }

        ret = this->SendProxyReq( pTask, false,
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

jobject CJavaProxy::JavaProxyCall2(
    JNIEnv *jenv,
    jobject pCb,
    jobject pContext,
    CfgPtr& pOptions,
    jobject listArgs )
{
    gint32 ret = 0;
    if( jenv == nullptr )
        return nullptr; 
    jobject pjCb = nullptr;
    jobject jgret = nullptr;
    jobject pjContext = nullptr;
    jobject jret = NewJRet( jenv );
    do{
        bool bSync = false;

        if( pCb == nullptr )
            bSync = true;

        std::vector< Variant > vecParams;
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
                &CJavaProxy::OnAsyncCallResp,
                nullptr, oReqCtx.GetCfg() );

            if( ERROR( ret ) )
                break;

            pjCb = jenv->NewGlobalRef( pCb );
            jgret = jenv->NewGlobalRef( jret );
            if( pContext != nullptr )
            {
                pjContext =
                    jenv->NewGlobalRef( pContext );
            }
            oReqCtx.Push( ( intptr_t ) pjCb );
            oReqCtx.Push( ( intptr_t ) jgret );
            oReqCtx.Push( strMethod );
            oReqCtx.Push( dwSeriProto );
            oReqCtx.Push( ( intptr_t )pjContext );
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

        ret = this->AsyncCallVector(
            pRespCb, pOptions,
            oResp.GetCfg(), strMethod,
            vecParams );

        if( ret == STATUS_PENDING && bSync )
        {
            CIoReqSyncCallback*
                pSyncCb = pWrapperCb;
            ret = pSyncCb->WaitForComplete();
        }

        if( ret != STATUS_PENDING && !bSync )
        {
            jenv->DeleteGlobalRef( pjCb );
            jenv->DeleteGlobalRef( jgret );
            jenv->DeleteGlobalRef( pjContext );
            pjCb = nullptr;
            jgret = nullptr;
            pjContext = nullptr;
        }

        if( ERROR( ret ) )
            break;

        else if( ret == STATUS_PENDING )
        {
            // the original jret is passed to the task.
            jret = NewJRet( jenv );
            guint64 qwTaskCancel = 0;
            oResp.GetQwordProp(
                propTaskId, qwTaskCancel );

            jobject jtaskId =
                NewLong( jenv, qwTaskCancel );
            AddElemToJRet( jenv, jret, jtaskId );
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
            jenv, pRmtResp, jret );
        if( ERROR( ret ) )
            break;

    }while( 0 );

    SetErrorJRet( jenv, jret, ret );
    if( ret != STATUS_PENDING )
    {
        if( pjCb != nullptr )
            jenv->DeleteGlobalRef( pjCb );
        if( pjContext != nullptr )
            jenv->DeleteGlobalRef( pjContext );
        if( jgret != nullptr )
            jenv->DeleteGlobalRef( jgret );
    }

    return jret;
}

DECLARE_AGGREGATED_PROXY(
    CJavaProxyImpl,
    CStatCountersProxy,
    CJavaProxy );

jobject CreateProxy(
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
            strDesc, strObjName,
            false, pCfg );

        if( ERROR( ret ) )
            break;

        EnumClsid iClsid =
            clsid( CJavaProxyImpl );
        EnumClsid iStateClass = clsid( Invalid );

        CCfgOpener oCfg( ( IConfigDb* )pCfg );
        ret = oCfg.GetIntProp( propIfStateClass,
            ( guint32& )iStateClass );
        if( SUCCEEDED( ret ) &&
            iStateClass == clsid( CFastRpcProxyState ) )
            iClsid = clsid( CJavaProxyRosImpl );

        ObjPtr* pIf = new ObjPtr();
        ret = pIf->NewObj( iClsid, pCfg );
        if( ERROR( ret ) )
        {
            delete pIf;
            break;
        }

        jobject pjIf =
            NewObjPtr( jenv, ( jlong )pIf );

        AddElemToJRet( jenv, jret, pjIf );

    }while( 0 );

    SetErrorJRet( jenv, jret, ret );
    return jret;
}

jobject CastToProxy(
    JNIEnv *jenv,
    ObjPtr& pObj )
{
    gint32 ret = 0;
    jobject jret = NewJRet( jenv );
    CJavaProxy* pProxy = pObj;
    if( pProxy == nullptr )
    {
        SetErrorJRet( jenv, jret, -EFAULT );
        return jret;
    }
    jobject jproxy = NewJavaProxy(
        jenv, ( intptr_t )pProxy );
    if( jproxy == nullptr )
    {
        SetErrorJRet( jenv, jret, -EFAULT );
        return jret;
    }
    AddElemToJRet( jenv, jret, jproxy );
    return jret;
}

void JavaDbgPrint(
    const std::string strMsg, int level = 3 )
{
    DebugPrintEx(
        ( EnumLogLvl )level, 0,
        "%s", strMsg.c_str() );
}

void JavaOutputMsg(
    const std::string strMsg )
{
    OutputMsg( 0, "%s", strMsg.c_str() );
}

gint32 LoadThisLib( ObjPtr& pIoMgr )
{
    gint32 ret = 0;
    if( pIoMgr.IsEmpty() )
        return -EINVAL;
    do{
        std::string strResult;
        const char* szLib = "librpcbaseJNI.so";
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

gint32 LoadJavaFactory( ObjPtr& pMgr )
{ return LoadThisLib( pMgr ); }

}

jobject CreateProxy(
    JNIEnv *jenv,
    ObjPtr& pMgr,
    const std::string& strDesc,
    const std::string& strObjName,
    ObjPtr& pCfgObj );

jobject CastToProxy(
    JNIEnv *jenv,
    ObjPtr& pObj );

void JavaDbgPrint(
    const std::string strMsg, int level = 3 );

gint32 LoadJavaFactory( ObjPtr& pMgr );
void JavaOutputMsg( const std::string strMsg );

%nodefaultctor;
%typemap(javadestruct_derived, methodname="delete", methodmodifiers="public synchronized") CJavaProxyBase {
    if (swigCPtr != 0) {
      swigCPtr = 0;
    }
    super.delete();
  }
class CJavaProxyBase : public CInterfaceProxy
{
};

%typemap(javadestruct_derived, methodname="delete", methodmodifiers="public synchronized") CJavaInterfBase {
    if (swigCPtr != 0) {
      swigCPtr = 0;
    }
    super.delete();
}

template< typename T >
class CJavaInterfBase
    : public T
{
    public:

    gint32 SetJavaHost(
        JNIEnv *jenv, jobject pObj );

    gint32 GetJavaHost( jobject& pObj );
    %extend{

    jobject GetJavaHost()
    {
        jobject pObj = nullptr;
        gint32 ret = $self->GetJavaHost( pObj );
        if( ERROR( ret ) )
            return nullptr;
        return pObj;
    }

    gint32 RemoveJavaHost()
    {
        gint32 ret = 0;
        return $self->RemoveJavaHost();
    }

    gint32 CloseStream( jlong hChannel )
    {
        gint32 ret = 0;
        T* pImpl = static_cast< T* >( $self );
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
        jlong hChannel, jobject pjBuf )
    {
        gint32 ret = 0;
        do{
            T* pImpl = static_cast< T* >( $self );

            if( pImpl == nullptr ) 
            {
                ret = -EFAULT;
                break;
            }

            BufPtr pBuf( true );
            ret = $self->ConvertByteArrayToBuf(
                jenv, pjBuf, pBuf );
            if( ERROR( ret ) )
                break;

            ret = pImpl->WriteStreamNoWait(
                ( HANDLE )hChannel, pBuf );

        }while( 0 );
        return ret;
    }

    gint32 WriteStream( JNIEnv *jenv,
        jlong hChannel, jobject pjBuf )
    {
        gint32 ret = 0;
        do{
            T* pImpl = static_cast< T* >( $self );

            if( pImpl == nullptr ) 
            {
                ret = -EFAULT;
                break;
            }

            BufPtr pBuf( true );
            ret = $self->ConvertByteArrayToBuf(
                jenv, pjBuf, pBuf );
            if( ERROR( ret ) )
                break;

            ret = pImpl->WriteStream(
                ( HANDLE )hChannel, pBuf );

        }while( 0 );

        return ret;
    }

    jobject ReadStreamNoWait( 
        JNIEnv *jenv, jlong hChannel )
    {
        gint32 ret = 0;

        jobject pResp = NewJRet( jenv );

        do{

            T* pImpl = static_cast< T* >( $self );

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
            ret = $self->ConvertBufToByteArray( 
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
        jobject jret = NewJRet( jenv );
        gint32 ret = 0;
        do{
            T* pImpl = static_cast
                < T* >( $self );

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
            ret = $self->ConvertBufToByteArray(
                jenv, pBuf, pjBuf );
            if( ERROR( ret ) )
                break;

            AddElemToJRet( jenv, jret, pjBuf );
            break;

        }while( 0 );

        SetErrorJRet( jenv, jret, ret );
        return jret;
    }

    jobject ReadStreamAsync( JNIEnv *jenv,
        jlong hChannel,
        jobject pCb,
        guint32 dwSize )
    {
        jobject jret = NewJRet( jenv );
        gint32 ret = 0;
        do{
            T* pImpl = static_cast< T* >( $self );

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

            IConfigDb* pCtx = oReqCtx.GetCfg();
            ret = pImpl->ReadStreamAsync(
                ( HANDLE )hChannel, pBuf, pCtx );

            if( ret != STATUS_PENDING )
                jenv->DeleteGlobalRef( pjCb );

            if( ret == STATUS_PENDING || ERROR( ret ) )
                break;

            jobject pjBuf = nullptr;
            ret = $self->ConvertBufToByteArray(
                jenv, pBuf, pjBuf );
            if( ERROR( ret ) )
                break;

            AddElemToJRet( jenv, jret, pjBuf );

        }while( 0 );

        SetErrorJRet( jenv, jret, ret );
        return jret;
    }

    gint32 WriteStreamAsync(
        JNIEnv *jenv,
        jlong hChannel,
        jobject pjBuf,
        jobject pjCb )
    {
        if( jenv == nullptr ||
            ( HANDLE )hChannel == INVALID_HANDLE ||
            pjBuf == nullptr ||
            pjCb == nullptr )
            return -EINVAL;

        gint32 ret = 0;
        jobject pgbuf = nullptr;
        jobject pgcb = nullptr;
        do{
            T* pImpl = static_cast< T* >( $self );

            if( pImpl == nullptr ) 
            {
                ret = -EFAULT;
                break;
            }

            BufPtr pBuf( true );
            ret = $self->ConvertByteArrayToBuf(
                jenv, pjBuf, pBuf );
            if( ERROR( ret ) )
                break;

            pgbuf = jenv->NewGlobalRef( pjBuf );
            pgcb = jenv->NewGlobalRef( pjCb );

            CParamList oReqCtx;
            oReqCtx.Push( ( intptr_t ) pgbuf );
            oReqCtx.Push( ( intptr_t ) pgcb );

            IConfigDb* pCtx = oReqCtx.GetCfg();
            ret = pImpl->WriteStreamAsync(
                ( HANDLE )hChannel, pBuf, pCtx );

        }while( 0 );

        if( ret != STATUS_PENDING )
        {
            if( pgbuf != nullptr )
                jenv->DeleteGlobalRef( pgbuf );
            if( pgcb != nullptr )
                jenv->DeleteGlobalRef( pgcb );
        }
        return ret;
    }

    jlong GetChanByIdHash( gint64 qwHash )
    {
        T* pImpl = static_cast< T* >( $self );
        HANDLE hChannel =
            pImpl->GetChanByIdHash( qwHash );
        return ( jlong )hChannel;
    }

    gint32 RemoveChanCtx(
        JNIEnv *jenv, jlong hChannel )
    {
        return $self->SetChanCtx( jenv,
            ( HANDLE )hChannel, nullptr );
    }

    }

    jobject AddTimer(
        JNIEnv *jenv,
        guint32 dwTimeoutSec,
        jobject pCb,
        jobject pCtx );

    gint32 DisableTimer(
        JNIEnv *jenv, ObjPtr& pTimer );

    gint32 DeferCall(
        JNIEnv *jenv,
        jobject pjCb,
        jobject pjArgs );

    gint32 InstallCancelNotify(
        JNIEnv *jenv,
        ObjPtr& pCallback,
        jobject pCb,
        jobject pListResp );

    gint32 SetChanCtx( JNIEnv *jenv,
        jlong hChannel, jobject pCtx );

    jobject GetChanCtx(
        JNIEnv *jenv, jlong hChannel );

};

%template(CJavaInterfBaseP) CJavaInterfBase<CJavaProxyBase>;

%typemap(javadestruct_derived, methodname="delete", methodmodifiers="public synchronized") CJavaProxy {
    if (swigCPtr != 0) {
      swigCPtr = 0;
    }
    super.delete();
  }
class CJavaProxy :
    public CJavaInterfBase<CJavaProxyBase>
{
    public:
    jobject GetIdHashByChan(
        JNIEnv *jenv,
        jlong hChannel );

    jobject JavaProxyCall2(
        JNIEnv *jenv,
        jobject pCb,
        jobject pContext,
        CfgPtr& pOptions,
        jobject listArgs );

    jobject GetPeerIdHash(
        JNIEnv *jenv,
        jlong hChannel );

    jobject CastToObjPtr( JNIEnv *jenv );

%extend{
    jobject StartStream( JNIEnv *jenv, ObjPtr* ppObj )
    {
        jobject jret = NewJRet( jenv );
        gint32 ret = 0;
        do{
            IConfigDb* pDesc;
            if( ppObj != nullptr )
                pDesc = *ppObj;
            else
                pDesc = nullptr;
            HANDLE hChannel = INVALID_HANDLE;
            ret = $self->StartStream(
                hChannel, pDesc );
            if( ERROR( ret ) )
                break;

            jobject pChan = NewLong( jenv,
                ( jlong )hChannel );

            AddElemToJRet( jenv, jret, pChan );

        }while( 0 );
        SetErrorJRet( jenv, jret, ret );
        return jret;
    }
    }

};

%clearnodefaultctor;
%include "server.i"

%header{
class CJavaProxyRosImpl :
    public CJavaRpcSvc_CliImpl
{
    public:
    typedef CJavaRpcSvc_CliImpl super;
    CJavaProxyRosImpl( const IConfigDb* pCfg )
    : super::virtbase( pCfg ), super( pCfg )
    { SetClassId( clsid( CJavaProxyRosImpl ) ); }
};

static FactoryPtr InitClassFactory()
{
    BEGIN_FACTORY_MAPS;
    INIT_MAP_ENTRYCFG( CJavaProxyImpl );
    INIT_MAP_ENTRYCFG( CJavaServerImpl );
    INIT_MAP_ENTRYCFG( CJavaProxyRosImpl );
    INIT_MAP_ENTRYCFG( CJavaServerRosImpl );
    INIT_MAP_ENTRYCFG( CSwigRosRpcSvc_CliSkel );
    INIT_MAP_ENTRYCFG( CSwigRosRpcSvc_SvrSkel );
    INIT_MAP_ENTRYCFG( CSwigRosRpcSvc_ChannelCli );
    INIT_MAP_ENTRYCFG( CSwigRosRpcSvc_ChannelSvr );
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

}
